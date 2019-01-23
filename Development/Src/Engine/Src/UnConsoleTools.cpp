/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "EnginePrivate.h"
#include "UnConsoleTools.h"

// we don't need any of this on console
#if !CONSOLE

/**
 * TranslateCompilerFlag - translates the platform-independent compiler flags into D3DX defines
 * @param CompilerFlag - the platform-independent compiler flag to translate
 * @return DWORD - the value of the appropriate D3DX enum
 */
static DWORD TranslateCompilerFlag(ECompilerFlags CompilerFlag)
{
#if USE_D3D_RHI
	//@todo: Make this platform independent.
	switch(CompilerFlag)
	{

	case CFLAG_PreferFlowControl: return D3DXSHADER_PREFER_FLOW_CONTROL;
	case CFLAG_Debug: return D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;
	case CFLAG_AvoidFlowControl: return D3DXSHADER_AVOID_FLOW_CONTROL;
	default: return 0;
	};
#else
	return 0;
#endif // USE_D3D_RHI
}

bool FConsoleShaderPrecompiler::CallPrecompiler(
	const TCHAR* SourceFilename, 
	const TCHAR* FunctionName, 
	const FShaderTarget& Target, 
	const FShaderCompilerEnvironment& Environment, 
	FShaderCompilerOutput& Output)
{
	// build up path to pass to precompiler
	FString ShaderDir = FString(appBaseDir()) * TEXT("..") PATH_SEPARATOR TEXT("Engine") PATH_SEPARATOR TEXT("Shaders");
	FString ShaderPath = ShaderDir * FString(SourceFilename) + TEXT(".usf");
	
	// build up a string of definitions
	FString Definitions;
	for(TMap<FName,FString>::TConstIterator DefinitionIt(Environment.Definitions);DefinitionIt;++DefinitionIt)
	{
		if (Definitions.Len())
		{
			Definitions += TEXT(" ");
		}

		FString Name = DefinitionIt.Key().ToString();
		FString Definition = DefinitionIt.Value();
		Definitions += FString("-D") + Name + TEXT("=") + Definition;
	}

	// write out the files that may need to be included
	for(TMap<FString,FString>::TConstIterator IncludeIt(Environment.IncludeFiles); IncludeIt; ++IncludeIt)
	{
		// the name already has the .usf extension
		FString IncludePath = ShaderDir * IncludeIt.Key();
		if (appSaveStringToFile(IncludeIt.Value(), *IncludePath) == FALSE)
		{
			warnf(TEXT("ShaderPrecompiler: Failed to save to file %s - Is it READONLY?"), *IncludePath);
		}
	}

	DWORD CompileFlags = 0;

	for (INT i = 0; i < Environment.CompilerFlags.Num(); i++)
	{
		CompileFlags |= TranslateCompilerFlag(Environment.CompilerFlags(i));
	}

	// allocate a huge buffer for constants and bytecode
	// @GEMINI_TODO: Validate this in the dll or something by passing in the size
	BYTE* BytecodeBuffer = (BYTE*)appMalloc(1024 * 1024); // 1M
	char* ConstantBuffer = (char*)appMalloc(256 * 1024); // 256k
	char* ErrorBuffer = (char*)appMalloc(256 * 1024); // 256k
	// to avoid crashing if the DLL doesn't set these
	ConstantBuffer[0] = 0;
	ErrorBuffer[0] = 0;
	INT BytecodeSize = 0;

	// call the DLL precompiler
	bool bSucceeded = PrecompileShader(TCHAR_TO_ANSI(*ShaderPath), TCHAR_TO_ANSI(FunctionName), Target.Frequency == SF_Vertex, CompileFlags, TCHAR_TO_ANSI(*Definitions), BytecodeBuffer, BytecodeSize, ConstantBuffer, ErrorBuffer);

	// output any erorrs
	if (ErrorBuffer[0])
	{
		warnf(ANSI_TO_TCHAR(ErrorBuffer));
	}

	if (bSucceeded)
	{
		// copy the bytecode into the output structure
		Output.Code.Empty(BytecodeSize);
		Output.Code.Add(BytecodeSize);
		appMemcpy(Output.Code.GetData(), BytecodeBuffer, BytecodeSize);

		// make a string of the constant returns
		FString AllConstants(ANSI_TO_TCHAR(ConstantBuffer));
		TArray<FString> ConstantArray;
		// break "WorldToLocal,100,4 Scale,101,1" into "WorldToLocal,100,4" and "Scale,101,1"
		AllConstants.ParseIntoArray(&ConstantArray, TEXT(" "), TRUE);
		for (INT ConstantIndex = 0; ConstantIndex < ConstantArray.Num(); ConstantIndex++)
		{
			TArray<FString> ConstantValues;
			// break "WorldToLocal,100,4" into "WorldToLocal","100","4"
			ConstantArray(ConstantIndex).ParseIntoArray(&ConstantValues, TEXT(","), TRUE);

			// make sure we only have 3 values
			if (ConstantValues.Num() != 3)
			{
				warnf(NAME_Warning, TEXT("Shader precompiler returned a bad constant string [%s]"), *ConstantArray(ConstantIndex));
			}

			FString ConstantName = *ConstantValues(0);
			INT RegisterIndex = appAtoi(*ConstantValues(1));
			INT RegisterCount = appAtoi(*ConstantValues(2));

			// collapse arrays down to the base parameter
			INT Bracket = ConstantName.InStr("[");
			if (Bracket != -1)
			{
				// convert the number after the [ to an integer
				INT ArraySize = appAtoi(*ConstantName.Right((ConstantName.Len() - Bracket) - 1)) + 1;
				// cut down the name to before the [
				ConstantName = ConstantName.Left(Bracket);

				// multiple array index by element size (registercount per element) for total 
				//  number of registers needed by the array
				ArraySize *= RegisterCount;

				WORD ExistingRegisterIndex = RegisterIndex;
				WORD ExistingRegisterCount = 0;
				// find any existing thing with the base name
				Output.ParameterMap.FindParameterAllocation(*ConstantName, ExistingRegisterIndex, ExistingRegisterCount);

				// update the parameter if we see a bigger index
				if (ArraySize >= ExistingRegisterCount)
				{
					Output.ParameterMap.AddParameterAllocation(*ConstantName, ExistingRegisterIndex, ArraySize);
				}
			}
			else
			{
				// output the constants to the parameter map
				Output.ParameterMap.AddParameterAllocation(*ConstantName, RegisterIndex, RegisterCount);
			}
		}
	}

	// Pass the target through to the output.
	Output.Target = Target;

	// free buffers
	appFree(ConstantBuffer);
	appFree(BytecodeBuffer);
	appFree(ErrorBuffer);

	return bSucceeded;
}


#endif // !CONSOLE
