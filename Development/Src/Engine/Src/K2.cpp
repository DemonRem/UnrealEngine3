/*=============================================================================
	K2.cpp
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineClasses.h"
#include "EngineK2Classes.h"
#include "K2.h"

IMPLEMENT_CLASS(UK2GraphBase);

IMPLEMENT_CLASS(UK2Connector);

IMPLEMENT_CLASS(UK2Input);
IMPLEMENT_CLASS(UK2Input_Bool);
IMPLEMENT_CLASS(UK2Input_Exec);
IMPLEMENT_CLASS(UK2Input_Int);
IMPLEMENT_CLASS(UK2Input_Float);
IMPLEMENT_CLASS(UK2Input_Vector);
IMPLEMENT_CLASS(UK2Input_Rotator);
IMPLEMENT_CLASS(UK2Input_String);
IMPLEMENT_CLASS(UK2Input_Object);

IMPLEMENT_CLASS(UK2Output);

IMPLEMENT_CLASS(UK2Output_Object);

IMPLEMENT_CLASS(UK2NodeBase);
IMPLEMENT_CLASS(UK2Node_Code);
IMPLEMENT_CLASS(UK2Node_FuncBase);
IMPLEMENT_CLASS(UK2Node_Func);
IMPLEMENT_CLASS(UK2Node_FuncPure);
IMPLEMENT_CLASS(UK2Node_Func_NewComp);
IMPLEMENT_CLASS(UK2Node_IfElse);
IMPLEMENT_CLASS(UK2Node_Event);
IMPLEMENT_CLASS(UK2Node_MemberVar);
IMPLEMENT_CLASS(UK2Node_ForLoop);

// code graphs/generation/editing only in editor
#if WITH_EDITOR

//////////////////////////////////////////////////////////////////////////
// UTILS

/** Util for getting the K2 type from an Unreal property */
EK2ConnectorType GetTypeFromProperty(UProperty* InProp, UClass*& OutClass)
{
	UStructProperty* StructProp = Cast<UStructProperty>(InProp);
	if(StructProp != NULL)
	{
		if(StructProp->Struct->GetFName() == NAME_Vector)
		{
			return K2CT_Vector;
		}
		else if(StructProp->Struct->GetFName() == NAME_Rotator)
		{
			return K2CT_Rotator;
		}
		else
		{
			return K2CT_Unsupported;
		}
	}
	else if( InProp->IsA(UBoolProperty::StaticClass()) )
	{
		return K2CT_Bool;
	}
	else if( InProp->IsA(UFloatProperty::StaticClass()) )
	{
		return K2CT_Float;
	}
	else if( InProp->IsA(UIntProperty::StaticClass()) )
	{
		return K2CT_Int;
	}
	else if( InProp->IsA(UStrProperty::StaticClass()) )
	{
		return K2CT_String;
	}
	else if( InProp->IsA(UObjectProperty::StaticClass()) )
	{
		// Grab the class of the object property
		OutClass = ((UObjectProperty*)InProp)->PropertyClass;
		return K2CT_Object;
	}
	else
	{
		return K2CT_Unsupported;
	}
}

/** Convert function name (e.g. 'Add_FloatFloat') into operator (e.g. '+') */
FString FuncNameToOperator(const FString& InFuncName)
{
	INT UScorePos = InFuncName.InStr(TEXT("_"));
	if(UScorePos != INDEX_NONE)
	{
		FString OpName = InFuncName.Left(UScorePos);
		if(OpName == TEXT("Add"))
		{
			return TEXT("+");
		}
		else if(OpName == TEXT("Subtract"))
		{
			return TEXT("-");
		}
		else if(OpName == TEXT("Multiply"))
		{
			return TEXT("*");
		}
		else if(OpName == TEXT("Divide"))
		{
			return TEXT("/");
		}
		else if(OpName == TEXT("Less"))
		{
			return TEXT("<");
		}
		else if(OpName == TEXT("LessEqual"))
		{
			return TEXT("<=");
		}
		else if(OpName == TEXT("Greater"))
		{
			return TEXT(">");
		}
		else if(OpName == TEXT("GreaterEqual"))
		{
			return TEXT(">=");
		}
		else if(OpName == TEXT("Equal"))
		{
			return TEXT("=");
		}
		else if(OpName == TEXT("EqualEqual"))
		{
			return TEXT("==");
		}
		else if(OpName == TEXT("At"))
		{
			return TEXT("@");
		}
	}

	return InFuncName;
}

//////////////////////////////////////////////////////////////////////////
// UK2Connector

/** Util to get the type of this connector as a code string */
FString UK2Connector::GetTypeAsCodeString()
{
		if(Type == K2CT_Bool)
		{
			return TEXT("bool");
		}
		else if(Type == K2CT_Float)
		{
			return TEXT("float");
		}
		else if(Type == K2CT_Vector)
		{
			return TEXT("vector");
		}
		else if(Type == K2CT_Rotator)
		{
			return TEXT("rotator");
		}
		else if(Type == K2CT_String)
		{
			return TEXT("string");
		}
		else if(Type == K2CT_Int)
		{
			return TEXT("int");
		}
		else if(Type == K2CT_Object)
		{
			return TEXT("object");
		}
		else if(Type == K2CT_Exec)
		{
			return TEXT("execution");
		}

		return TEXT("UNKNOWN");
}

//////////////////////////////////////////////////////////////////////////
// UK2Input

void UK2Input::Break()
{
	if (FromOutput != NULL)
	{
		if (!FromOutput->ToInputs.ContainsItem(this))
		{
			debugf(TEXT("mFromOutput.mToInput != this"));
		}

		FromOutput->ToInputs.RemoveItem(this);
		FromOutput = NULL;
	}
}

FString UK2Input::GetValueString()
{
	return TEXT("");
}

FString UK2Input::GetValueCodeString()
{
	return GetValueString();
}

//////////////////////////////////////////////////////////////////////////
// UK2Input_Bool

FString UK2Input_Bool::GetValueString()
{
	return bDefaultBool ? TEXT("TRUE") : TEXT("FALSE");
}

void UK2Input_Bool::SetDefaultFromString(const FString& InString)
{
	if( appStricmp(*InString, TEXT("true")) || appStricmp(*InString, TEXT("1")) || appStricmp(*InString, TEXT("t")) )
	{
		bDefaultBool = TRUE;
	}
	else
	{
		bDefaultBool = FALSE;
	}
}

//////////////////////////////////////////////////////////////////////////
// UK2Input_Exec

//////////////////////////////////////////////////////////////////////////
// UK2Input_Int

FString UK2Input_Int::GetValueString()
{
	return FString::Printf(TEXT("%d"), DefaultInt);
}

void UK2Input_Int::SetDefaultFromString(const FString& InString)
{
	DefaultInt = appAtoi(*InString);
}

//////////////////////////////////////////////////////////////////////////
// UK2Input_Float

FString UK2Input_Float::GetValueString()
{
	return FString::Printf(TEXT("%.4f"), DefaultFloat);
}

void UK2Input_Float::SetDefaultFromString(const FString& InString)
{
	DefaultFloat = appAtof(*InString);
}

//////////////////////////////////////////////////////////////////////////
// UK2Input_Vector

FString UK2Input_Vector::GetValueString()
{
	return FString::Printf(TEXT("%.4f, %.4f, %.4f"), DefaultVector.X, DefaultVector.Y, DefaultVector.Z);
}

FString UK2Input_Vector::GetValueCodeString()
{
	return FString::Printf(TEXT("vect(%s)"), *GetValueString());
}

void UK2Input_Vector::SetDefaultFromString(const FString& InString)
{
	TArray<FString> Pieces;
	InString.ParseIntoArray(&Pieces, TEXT(","), FALSE);

	DefaultVector.X = (Pieces.Num() > 0) ? appAtof(*Pieces(0)) : 0.f;
	DefaultVector.Y = (Pieces.Num() > 1) ? appAtof(*Pieces(1)) : 0.f;
	DefaultVector.Z = (Pieces.Num() > 2) ? appAtof(*Pieces(2)) : 0.f;
}

//////////////////////////////////////////////////////////////////////////
// UK2Input_Rotator

FString UK2Input_Rotator::GetValueString()
{
	FLOAT PitchDeg = 360.f * (DefaultRotator.Pitch / 65536.f);
	FLOAT YawDeg = 360.f * (DefaultRotator.Yaw / 65536.f);
	FLOAT RollDeg = 360.f * (DefaultRotator.Roll / 65536.f);

	return FString::Printf(TEXT("%.4f, %.4f, %.4f"), PitchDeg, YawDeg, RollDeg);
}

FString UK2Input_Rotator::GetValueCodeString()
{
	return FString::Printf(TEXT("rot(%d,%d,%d)"), DefaultRotator.Pitch, DefaultRotator.Yaw, DefaultRotator.Roll);
}

void UK2Input_Rotator::SetDefaultFromString(const FString& InString)
{
	TArray<FString> Pieces;
	InString.ParseIntoArray(&Pieces, TEXT(","), FALSE);

	if(Pieces.Num() > 0)
	{
		FLOAT PitchDeg = appAtof(*Pieces(0));
		DefaultRotator.Pitch = appRound(65536.f * (PitchDeg / 360.f));
	}

	if(Pieces.Num() > 1)
	{
		FLOAT YawDeg = appAtof(*Pieces(1));
		DefaultRotator.Yaw = appRound(65536.f * (YawDeg / 360.f));
	}

	if(Pieces.Num() > 2)
	{
		FLOAT RollDeg = appAtof(*Pieces(2));
		DefaultRotator.Roll = appRound(65536.f * (RollDeg / 360.f));
	}
}

//////////////////////////////////////////////////////////////////////////
// UK2Input_String

FString UK2Input_String::GetValueString()
{
	return DefaultString;
}

FString UK2Input_String::GetValueCodeString()
{
	return FString::Printf(TEXT("\"%s\""), *GetValueString());
}

void UK2Input_String::SetDefaultFromString(const FString& InString)
{
	DefaultString = InString;
}

//////////////////////////////////////////////////////////////////////////
// UK2Input_Object

FString UK2Input_Object::GetValueString()
{
	return TEXT("");
}

void UK2Input_Object::SetDefaultFromString(const FString& InString)
{
	// TODO?
}

//////////////////////////////////////////////////////////////////////////
// UK2Output

void UK2Output::BreakConnectionTo(class UK2Input* ToInput)
{
	if(ToInput)
	{
		// If we do indeed connect to the passed in input...
		if (ToInputs.ContainsItem(ToInput))
		{
			// Check that the input thinks it connects to us
			if (ToInput->FromOutput != this)
			{
				debugf(TEXT("ToInput.mFromOutput != this"));
			}

			ToInput->FromOutput = NULL;
			ToInputs.RemoveItem(ToInput);
		}
	}
}

void UK2Output::BreakAllConnections()
{
	TArray<UK2Input*> ToInputsCopy = ToInputs;

	for(INT i=0; i<ToInputsCopy.Num(); i++)
	{
		BreakConnectionTo( ToInputsCopy(i) );
	}
}

//////////////////////////////////////////////////////////////////////////
// UK2Output_Object

FString UK2Output_Object::GetTypeAsCodeString()
{
	if(ObjClass == NULL)
	{
		debugf(TEXT("UK2Output_Object::GetTypeAsCodeString - no ObjClass!"));
		return TEXT("object");
	}
	else
	{
		return ObjClass->GetName();
	}
}


//////////////////////////////////////////////////////////////////////////
// UK2NodeBase

void UK2NodeBase::BreakAllConnections()
{
	for(INT i=0; i<Inputs.Num(); i++)
	{
		Inputs(i)->Break();
	}

	for(INT i=0; i<Outputs.Num(); i++)
	{
		Outputs(i)->BreakAllConnections();
	}
}

UK2Input* UK2NodeBase::GetInputFromName(const FString& InputName)
{
	for(INT i=0; i<Inputs.Num(); i++)
	{
		UK2Input* Input = Inputs(i);
		if((Input != NULL) && (Input->ConnName == InputName))
		{
			return Input;
		}
	}

	return NULL;
}

UK2Output* UK2NodeBase::GetOutputFromName(const FString& OutputName)
{
	for(INT i=0; i<Outputs.Num(); i++)
	{
		UK2Output* Output = Outputs(i);
		if((Output != NULL) && (Output->ConnName == OutputName))
		{
			return Output;
		}
	}

	return NULL;
}

FString UK2NodeBase::GetDisplayName()
{
	return GetName();
}

UBOOL UK2NodeBase::InputDefaultsAreEditable()
{
	return TRUE;
}

FColor UK2NodeBase::GetBorderColor()
{
	return FColor(0,0,0);
}


void UK2NodeBase::CreateConnector(EK2ConnectorDirection Dir, EK2ConnectorType Type, const FString& ConnName, UClass* ObjConnClass)
{
	UK2Connector* NewConn = NULL;

	// Output
	if(Dir == K2CD_Output)
	{
		UK2Output* NewOutput = NULL;

		if(Type == K2CT_Object)
		{
			UK2Output_Object* NewObjOutput = ConstructObject<UK2Output_Object>(UK2Output_Object::StaticClass(), this);
			NewObjOutput->ObjClass = (ObjConnClass == NULL) ? UObject::StaticClass() : ObjConnClass;

			NewOutput = NewObjOutput;
		}
		else
		{
			NewOutput = ConstructObject<UK2Output>(UK2Output::StaticClass(), this);
		}

		Outputs.AddItem(NewOutput);
		NewConn = NewOutput;
	}
	// Input
	else
	{
		UK2Input* NewInput = NULL;

		if(Type == K2CT_Bool)
		{
			NewInput = ConstructObject<UK2Input_Bool>(UK2Input_Bool::StaticClass(), this);
		}
		else if(Type == K2CT_Float)
		{
			NewInput = ConstructObject<UK2Input_Float>(UK2Input_Float::StaticClass(), this);
		}
		else if(Type == K2CT_Int)
		{
			NewInput = ConstructObject<UK2Input_Int>(UK2Input_Int::StaticClass(), this);
		}
		else if(Type == K2CT_Vector)
		{
			NewInput = ConstructObject<UK2Input_Vector>(UK2Input_Vector::StaticClass(), this);
		}
		else if(Type == K2CT_Rotator)
		{
			NewInput = ConstructObject<UK2Input_Rotator>(UK2Input_Rotator::StaticClass(), this);
		}
		else if(Type == K2CT_String)
		{
			NewInput = ConstructObject<UK2Input_String>(UK2Input_String::StaticClass(), this);
		}
		else if(Type == K2CT_Object)
		{
			NewInput = ConstructObject<UK2Input_Object>(UK2Input_Object::StaticClass(), this);
		}
		else if(Type == K2CT_Exec)
		{
			NewInput = ConstructObject<UK2Input_Exec>(UK2Input_Exec::StaticClass(), this);
		}
		else
		{
			check(0);
		}

		Inputs.AddItem(NewInput);
		NewConn = NewInput;
	}

	NewConn->Type = Type;
	NewConn->ConnName = ConnName;
	NewConn->OwningNode = this;
}







//////////////////////////////////////////////////////////////////////////
// UK2Node_Code


void UK2Node_Code::GetCodeText(FK2CodeGenContext& Context, TArray<FK2CodeLine>& OutCode)
{

}

UK2Node_Code* UK2Node_Code::GetCodeNodeFromOutputName(const FString& OutputName)
{
	UK2Output* Output = GetOutputFromName(OutputName);
	if (Output != NULL)
	{
		if ((Output->ToInputs.Num() > 0) && (Output->ToInputs(0) != NULL))
		{
			return Cast<UK2Node_Code>(Output->ToInputs(0)->OwningNode);
		}
	}

	return NULL;
}

void UK2Node_Code::CreateConnectorsFromFunction(UFunction* InFunction, UClass* ReturnConClass)
{
	for (TFieldIterator<UProperty> PropIt(InFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* FuncParam = *PropIt;

		// See if its an input or output parameter
		EK2ConnectorDirection ConnDir = K2CD_Output;

		// If a non-event node, determine if its an in or an out
		// for event nodes, we treat all params as output connectors
		if( !this->IsA(UK2Node_Event::StaticClass()) )
		{
			ConnDir = (FuncParam->PropertyFlags & CPF_OutParm) ? K2CD_Output : K2CD_Input;
		}

		// See if this is the return param.
		UBOOL bReturn = (FuncParam->PropertyFlags & CPF_ReturnParm);

		// Use param name, unless its the return param (always call that 'Return')
		FString ConnName = bReturn ? TEXT("Return") : FuncParam->GetName();

		// Get the enum type of this param (also obj class if applicable)
		UClass* ConnObjClass = NULL;
		EK2ConnectorType ConnType = GetTypeFromProperty(FuncParam, ConnObjClass);

		// If returning an object, allow node to override the return type
		if((ConnType == K2CT_Object) && bReturn && (ReturnConClass != NULL))
		{
			ConnObjClass = ReturnConClass;
		}

		// If supported type, make a connector
		if(ConnType != K2CT_Unsupported)
		{
			CreateConnector(ConnDir, ConnType, ConnName, ConnObjClass);
		}
	}
}

// Util that gets the code (constant or variable name) that is generated by the input with the supplied name
FString UK2Node_Code::GetCodeFromParamInput(const FString& InputName, FK2CodeGenContext& Context)
{
	// See if we have something connected
	UK2Input* Input = GetInputFromName(InputName);
	if(Input != NULL)
	{
		if(Input->FromOutput != NULL)
		{
			// See if we are connected to a variable node
			UK2Node_MemberVar* MemVar = Cast<UK2Node_MemberVar>(Input->FromOutput->OwningNode);
			if(MemVar)
			{
				// If so, just use the var name (it should be in scope!)
				return MemVar->VarName;
			}
			else
			{
				// Not a var node, now we need to look for the local var which stores the output from that function
				FString* VarName = Context.OutputToVarNameMap.Find(Input->FromOutput);
				if(VarName == NULL)
				{
					Context.bGenFailure = TRUE;
					Context.GenFailMessage = FString::Printf(TEXT("Output '%s' on node '%s' use by '%s' before it is executed."), *Input->FromOutput->ConnName, *Input->FromOutput->OwningNode->GetName(), *GetName() );
					return TEXT("");
				}
				else
				{
					return *VarName;
				}
			}
		}
		else
		{
			// Not connected, so we use the saved value on the input
			return Input->GetValueCodeString();
		}
	}
	else
	{
		return TEXT("");
	}
}

/** 
 *	Generated an ordered list of pure functions that need to be added before this non-pure node can be called
 *	Marks nodes as processed in Context
 */
void UK2Node_Code::GetDependentPureFunctions(struct FK2CodeGenContext& Context, TArray<class UK2Node_FuncPure*>& OutPureFuncs)
{
	// Need to add nodes before this one in the graph first
	for(INT i=0; i<Inputs.Num(); i++)
	{
		UK2Input* Input = Inputs(i);

		// Find each variable (ie non-event) input, and see if something is connected
		if ((Input != NULL) && (Input->Type != K2CT_Exec) && (Input->FromOutput != NULL))
		{
			UK2Node_FuncPure* InPureFunc = Cast<UK2Node_FuncPure>(Input->FromOutput->OwningNode);

			// If not already processed, get its set of pure functions
			if ((InPureFunc != NULL) && !Context.IsNodeAlreadyProcessed(InPureFunc))
			{
				Context.MarkNodeProcessed(InPureFunc); // prevents loops

				TArray<UK2Node_FuncPure*> InputPureFuncs;
				InPureFunc->GetDependentPureFunctions(Context, InputPureFuncs);

				OutPureFuncs.Append(InputPureFuncs);

				OutPureFuncs.AddItem(InPureFunc);
			}
		}
	}
}

/** Generate code to assign any output vars to local variables */
void UK2Node_Code::GetMemberVarAssignCode(struct FK2CodeGenContext& Context, TArray<struct FK2CodeLine>& OutCode)
{
	// Find any member var nodes connected to an output
	for(INT OutIdx=0; OutIdx<Outputs.Num(); OutIdx++)
	{
		UK2Output* Output = Outputs(OutIdx);

		// Find each variable (ie non-event) output, 
		if ((Output != NULL) && (Output->Type != K2CT_Exec))
		{
			// See what it is connected to
			for(INT ToInIdx=0; ToInIdx<Output->ToInputs.Num(); ToInIdx++)
			{
				// See if it is connected to a member var
				UK2Input* ToInput = Output->ToInputs(ToInIdx);
				UK2Node_MemberVar* ToMemberVar = Cast<UK2Node_MemberVar>(ToInput->OwningNode);

				if(ToMemberVar != NULL)
				{
					FString* VarName = Context.OutputToVarNameMap.Find(Output);
					check(VarName); // should always have been added to map!

					// create code that does assignment
					FString AssignMemCode = FString::Printf( TEXT("%s = %s;"), *(ToMemberVar->VarName), *(*VarName) );

					// And add a line of code
					OutCode.AddItem( FK2CodeLine(*AssignMemCode) );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// UK2Node_FuncBase

void UK2Node_FuncBase::GetCodeText(FK2CodeGenContext& Context, TArray<FK2CodeLine>& OutCode)
{
	if(Function == NULL)
	{
		Context.bGenFailure = TRUE;
		Context.GenFailMessage = FString::Printf(TEXT("Missing function '%s'"), *GetName());
		return;
	}

	FString FunctionName = GetFunctionName();

	// Build string to call the function with desired parameters
	FString FuncCall;
	// if we should put a trailing brace at the end of the line, used when casting result of function call
	UBOOL bTrailingBrace = FALSE;

	// Make a list of all params, so we know what the last one is
	// TODO: Ensure there is no non-return parameter called Return!
	TArray<UProperty*> Params;
	UProperty* ReturnParam = NULL;
	for (TFieldIterator<UProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* FuncParam = *PropIt;

		if(FuncParam->PropertyFlags & CPF_ReturnParm)
		{
			ReturnParam = FuncParam;
		}
		else
		{
			Params.AddItem(FuncParam);
		}
	}

	// Return stuff
	UK2Output* Output = GetOutputFromName("Return");
	// If we have an output called 'Return' and it's connected to something, put the return value into a variable
	if((Output != NULL) && (Output->ToInputs.Num() > 0))
	{
		FString VarName = Context.VarNameGen.GetUniqueName(FunctionName + TEXT("Ret"));
		Context.OutputToVarNameMap.Set(Output, VarName);

		// Now we see if a cast is necessary, by looking at whether the function return type and the 
		FString VarCast = TEXT("");
		UObjectProperty* ObjReturnParam = Cast<UObjectProperty>(ReturnParam);
		UK2Output_Object* ObjOutput = Cast<UK2Output_Object>(Output);
		if(	(ObjReturnParam != NULL) && 
			(ObjOutput != NULL) && 
			(ObjReturnParam->PropertyClass != ObjOutput->ObjClass) )
		{
			VarCast = FString::Printf( TEXT("%s( "), *ObjOutput->ObjClass->GetName() );
			bTrailingBrace = TRUE;
		}

		FuncCall = VarName + TEXT(" = ") + VarCast;
	}

	// Iterate over non-return params
	TArray<FString> ParamCode;
	for(INT i=0; i<Params.Num(); i++)
	{
		UProperty* FuncParam = Params(i);

		// Output parameter
		if (FuncParam->PropertyFlags & CPF_OutParm)
		{
			// Create a var name, and associate with this output
			UK2Output* Output = GetOutputFromName(FuncParam->GetName());
			FString VarName = Context.VarNameGen.GetUniqueName(FuncParam->GetName());
			Context.OutputToVarNameMap.Set(Output, VarName);

			ParamCode.AddItem(VarName);
		}
		// Input parameter
		else
		{
			FString InputCode = GetCodeFromParamInput(FuncParam->GetName(), Context);
			ParamCode.AddItem(InputCode);
		}
	}

	// See if it is an operator
	if(Function->HasAnyFunctionFlags(FUNC_Operator))
	{
		if(ParamCode.Num() == 2)
		{
			// convert into single-character function name
			FString Operator = FuncNameToOperator(FunctionName);

			FuncCall += FString::Printf(TEXT("%s %s %s;"), *ParamCode(0), *Operator, *ParamCode(1));
		}
		else
		{
			Context.bGenFailure = TRUE;
			Context.GenFailMessage = FString::Printf(TEXT("Operator needs 2 params '%s'"), *GetName());
			return;
		}
	}
	// Non-operator, regular call
	else
	{
		// See if we have the name of a variable to call function on
		FString FuncTargetName = GetCodeFromParamInput(TEXT("Target"), Context);
		if(FuncTargetName.Len() > 0)
		{
			FuncCall += FuncTargetName + TEXT(".");
		}

		// Any cast of return type, then function name and opening bracket
		FuncCall += FunctionName + TEXT("( ");

		for(INT i=0; i<ParamCode.Num(); i++)
		{
			FuncCall += ParamCode(i);

			// If not last param, 
			if (i < ParamCode.Num()-1)
			{
				FuncCall += TEXT(", ");
			}
		}

		if(bTrailingBrace)
		{
			FuncCall += TEXT(" ) ");
		}

		FuncCall += TEXT(" );");
	}

	OutCode.AddItem( FK2CodeLine(*FuncCall) );
}

/** Gets the name of the function currently being used by this node */
FString UK2Node_FuncBase::GetFunctionName()
{
	if(Function != NULL)
	{
		return Function->GetName();
	}
	else
	{
		return TEXT("None");
	}
}

FString UK2Node_FuncBase::GetDisplayName()
{
	// If an operator, just use the operator char as the display name
	if(Function && Function->HasAnyFunctionFlags(FUNC_Operator))
	{
		// convert into single-character function name
		FString FuncName = Function->GetName();
		return FuncNameToOperator(FuncName);
	}
	else
	{
		// Get the name of the class
		FString ClassName;
		if(Function != NULL)
		{
			ClassName = *(Function->GetOwnerClass()->GetName());
		}

		// The put together class and function name
		FString DisplayName = FString::Printf(TEXT("%s::%s"), *ClassName, *GetFunctionName());

		return DisplayName;
	}
}

//////////////////////////////////////////////////////////////////////////
// K2Node_Func

FColor UK2Node_Func::GetBorderColor()
{
	if(Function != NULL)
	{
		UClass* Class = Function->GetOwnerClass();
		if(Class->IsChildOf(UActorComponent::StaticClass()))
		{
			return FColor(30,144,255);
		}
	}

	return FColor(0,0,0);
}

void UK2Node_Func::CreateAutoConnectors()
{
	CreateConnector(K2CD_Input, K2CT_Exec, TEXT("Run"));
	CreateConnector(K2CD_Input, K2CT_Object, TEXT("Target"));

	CreateConnector(K2CD_Output, K2CT_Exec, TEXT("Done"));
}

void UK2Node_Func::GetCodeText(FK2CodeGenContext& Context, TArray<FK2CodeLine>& OutCode)
{
	// For imperative functions, we first we want to get code for any pure functions connected by data
	TArray<UK2Node_FuncPure*> PureFuncs;
	GetDependentPureFunctions(Context, PureFuncs);

	for(INT i=0; i<PureFuncs.Num(); i++)
	{
		UK2Node_FuncPure* PureFunc = PureFuncs(i);
		check(PureFunc);

		TArray<FK2CodeLine> PureCode;
		PureFunc->GetCodeText(Context, PureCode);
		OutCode.Append(PureCode);
	}

	// If we have encountered a problem, go no further
	if(Context.bGenFailure)
	{
		return;
	}

	// Then get text for this actual function call
	TArray<FK2CodeLine> ThisText;
	Super::GetCodeText(Context, ThisText);
	OutCode.Append(ThisText);

	// If we have encountered a problem, go no further
	if(Context.bGenFailure)
	{
		return;
	}

	// Assign any output variables to member vars
	TArray<FK2CodeLine> AssignToMemVarText;
	GetMemberVarAssignCode(Context, AssignToMemVarText);
	OutCode.Append(AssignToMemVarText);


	// Then find what is connected to the 'Done' output and call that
	UK2Node_Code* NextNode = GetCodeNodeFromOutputName("Done");
	if (NextNode != NULL)
	{
		if (Context.IsNodeAlreadyProcessed(NextNode))
		{
			Context.bGenFailure = TRUE;
			Context.GenFailMessage = FString::Printf(TEXT("Loop detected at '%s'"), *GetName());
		}
		else
		{
			TArray<FK2CodeLine> NextCode;
			NextNode->GetCodeText(Context, NextCode);
			OutCode.Append(NextCode);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// UK2Node_FuncPure

//////////////////////////////////////////////////////////////////////////
// UK2Node_Func_NewComp

FString UK2Node_Func_NewComp::GetDisplayName()
{
	FString CompType = TEXT("None");
	if(ComponentTemplate != NULL)
	{
		CompType = ComponentTemplate->GetClass()->GetName();
	}

	return FString::Printf(TEXT("%s: %s"), *GetFunctionName(), *CompType);
}


FString UK2Node_Func_NewComp::GetCodeFromParamInput(const FString& InputName, struct FK2CodeGenContext& Context)
{
	// Special case where we return the template object owned by this node, rather than anything connected to this input
	if(InputName == TEXT("Template"))
	{
		if(ComponentTemplate == NULL)
		{
			debugf(TEXT("UK2Node_Func_NewComp::GetCodeFromParamInput : Missing ComponentTemplate."));
			return TEXT("");
		}
		return FString::Printf( TEXT("%s'%s'"), *ComponentTemplate->GetClass()->GetName(), *ComponentTemplate->GetPathName() );
	}
	else
	{
		return Super::GetCodeFromParamInput(InputName, Context);
	}
}

//////////////////////////////////////////////////////////////////////////
// UK2Node_IfElse

void UK2Node_IfElse::CreateAutoConnectors()
{
	CreateConnector(K2CD_Input, K2CT_Exec, TEXT("Run"));
	CreateConnector(K2CD_Input, K2CT_Bool, TEXT("Bool"));

	CreateConnector(K2CD_Output, K2CT_Exec, TEXT("True"));
	CreateConnector(K2CD_Output, K2CT_Exec, TEXT("False"));
}

void UK2Node_IfElse::GetCodeText(FK2CodeGenContext& Context, TArray<FK2CodeLine>& OutCode)
{
	// For imperative functions, we first we want to get code for any pure functions connected by data
	TArray<UK2Node_FuncPure*> PureFuncs;
	GetDependentPureFunctions(Context, PureFuncs);

	for(INT i=0; i<PureFuncs.Num(); i++)
	{
		UK2Node_FuncPure* PureFunc = PureFuncs(i);
		check(PureFunc);

		TArray<FK2CodeLine> PureCode;
		PureFunc->GetCodeText(Context, PureCode);
		OutCode.Append(PureCode);
	}

	// If we have encountered a problem, go no further
	if(Context.bGenFailure)
	{
		return;
	}

	FString ParamCode = GetCodeFromParamInput(TEXT("Bool"), Context);

	FString IfCode = FString::Printf( TEXT("if( %s == TRUE)"), *ParamCode );
	OutCode.AddItem( FK2CodeLine(IfCode) );
	OutCode.AddItem( FK2CodeLine(TEXT("{")) );

	UK2Node_Code* TrueNode = GetCodeNodeFromOutputName(TEXT("True"));
	if (TrueNode != NULL)
	{
		if (Context.IsNodeAlreadyProcessed(TrueNode))
		{
			Context.bGenFailure = TRUE;
			Context.GenFailMessage = FString::Printf(TEXT("Loop detected at '%s'"), *TrueNode->GetName());
		}
		else
		{
			TArray<FK2CodeLine> NextCode;
			TrueNode->GetCodeText(Context, NextCode);
			FK2CodeLine::IndentAllCodeLines(NextCode);
			OutCode.Append(NextCode);
		}
	}

	OutCode.AddItem( FK2CodeLine(TEXT("}")) );
	OutCode.AddItem( FK2CodeLine(TEXT("else")) );
	OutCode.AddItem( FK2CodeLine(TEXT("{")) );

	UK2Node_Code* FalseNode = GetCodeNodeFromOutputName(TEXT("False"));
	if (FalseNode != NULL)
	{
		if (Context.IsNodeAlreadyProcessed(FalseNode))
		{
			Context.bGenFailure = TRUE;
			Context.GenFailMessage = FString::Printf(TEXT("Loop detected at '%s'"), *FalseNode->GetName());
		}
		else
		{
			TArray<FK2CodeLine> NextCode;
			FalseNode->GetCodeText(Context, NextCode);
			FK2CodeLine::IndentAllCodeLines(NextCode);
			OutCode.Append(NextCode);
		}
	}

	OutCode.AddItem( FK2CodeLine(TEXT("}")) );
}

FString UK2Node_IfElse::GetDisplayName()
{
	return TEXT("IfElse");
}


//////////////////////////////////////////////////////////////////////////
// UK2Node_ForLoop

void UK2Node_ForLoop::CreateAutoConnectors()
{
	CreateConnector(K2CD_Input, K2CT_Exec, TEXT("Run"));
	CreateConnector(K2CD_Input, K2CT_Int, TEXT("Count"));

	CreateConnector(K2CD_Output, K2CT_Exec, TEXT("Loop"));
	CreateConnector(K2CD_Output, K2CT_Int, TEXT("LoopCount"));
	CreateConnector(K2CD_Output, K2CT_Exec, TEXT("Then"));
}

void UK2Node_ForLoop::GetCodeText(FK2CodeGenContext& Context, TArray<FK2CodeLine>& OutCode)
{
	// For imperative functions, we first we want to get code for any pure functions connected by data
	TArray<UK2Node_FuncPure*> PureFuncs;
	GetDependentPureFunctions(Context, PureFuncs);

	for(INT i=0; i<PureFuncs.Num(); i++)
	{
		UK2Node_FuncPure* PureFunc = PureFuncs(i);
		check(PureFunc);

		TArray<FK2CodeLine> PureCode;
		PureFunc->GetCodeText(Context, PureCode);
		OutCode.Append(PureCode);
	}

	// If we have encountered a problem, go no further
	if(Context.bGenFailure)
	{
		return;
	}

	FString CountCode = GetCodeFromParamInput(TEXT("Count"), Context);

	// Return stuff
	UK2Output* LoopCountOutput = GetOutputFromName("LoopCount");
	FString LoopCountVarName = Context.VarNameGen.GetUniqueName(TEXT("ForCount"));
	Context.OutputToVarNameMap.Set(LoopCountOutput, LoopCountVarName);

	FString IfCode = FString::Printf( TEXT("for( %s=0; %s<%s; %s++ )"), *LoopCountVarName, *LoopCountVarName, *CountCode, *LoopCountVarName );
	OutCode.AddItem( FK2CodeLine(IfCode) );
	OutCode.AddItem( FK2CodeLine(TEXT("{")) );

	UK2Node_Code* LoopNode = GetCodeNodeFromOutputName(TEXT("Loop"));
	if (LoopNode != NULL)
	{
		if (Context.IsNodeAlreadyProcessed(LoopNode))
		{
			Context.bGenFailure = TRUE;
			Context.GenFailMessage = FString::Printf(TEXT("Loop detected at '%s'"), *LoopNode->GetName());
		}
		else
		{
			TArray<FK2CodeLine> NextCode;
			LoopNode->GetCodeText(Context, NextCode);
			FK2CodeLine::IndentAllCodeLines(NextCode);
			OutCode.Append(NextCode);
		}
	}

	OutCode.AddItem( FK2CodeLine(TEXT("}")) );
	OutCode.AddItem( FK2CodeLine(TEXT("")) );

	UK2Node_Code* ThenNode = GetCodeNodeFromOutputName(TEXT("Then"));
	if (ThenNode != NULL)
	{
		if (Context.IsNodeAlreadyProcessed(ThenNode))
		{
			Context.bGenFailure = TRUE;
			Context.GenFailMessage = FString::Printf(TEXT("Loop detected at '%s'"), *ThenNode->GetName());
		}
		else
		{
			TArray<FK2CodeLine> NextCode;
			ThenNode->GetCodeText(Context, NextCode);
			OutCode.Append(NextCode);
		}
	}

}

FString UK2Node_ForLoop::GetDisplayName()
{
	return TEXT("ForLoop");
}

//////////////////////////////////////////////////////////////////////////
// UK2Node_Event

void UK2Node_Event::CreateAutoConnectors()
{
	CreateConnector(K2CD_Output, K2CT_Exec, TEXT("Done"));
}

FString UK2Node_Event::GetDisplayName()
{
	return EventName;
}

FColor UK2Node_Event::GetBorderColor()
{
	return FColor(255,0,0);
}

void UK2Node_Event::GetEventText(FK2CodeGenContext& Context, TArray<struct FK2CodeLine>& OutCode)
{
	// Build first line of event - with name and parameter line
	FString EventDec = FString::Printf(TEXT("event %s("), *EventName);

	// Make a list of all non-return params, so we know what the last one is
	TArray<UProperty*> Params;
	for (TFieldIterator<UProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* FuncParam = *PropIt;

		if(!(FuncParam->PropertyFlags & CPF_ReturnParm))
		{
			Params.AddItem(FuncParam);
		}
	}

	// Put in each parameter
	for (int i=0; i<Params.Num(); i++)
	{
		UProperty* FuncParam = Params(i);

		// Create a var name, and associate with this output
		UK2Output* VarOutput = GetOutputFromName(FuncParam->GetName());
		FString VarName = FuncParam->GetName();
		Context.OutputToVarNameMap.Set(VarOutput, VarName);

		EventDec += FString::Printf(TEXT("%s %s"), *VarOutput->GetTypeAsCodeString(), *VarName);

		if (i < Params.Num()-1)
		{
			EventDec += TEXT(", ");
		}
	}

	EventDec += TEXT(")");
	OutCode.AddItem( FK2CodeLine(*EventDec) );

	// Then put squiggly brackets, and code generated from the following code node
	OutCode.AddItem( FK2CodeLine(TEXT("{")) );

	UK2Node_Code* CodeNode = GetCodeNodeFromOutputName(TEXT("Done"));
	if (CodeNode != NULL)
	{
		TArray<FK2CodeLine> NextCode;
		CodeNode->GetCodeText(Context, NextCode);

		if(Context.bGenFailure)
		{
			OutCode.Empty();
			return;
		}

		// We need to create a local var for all non-event-output variables
		for ( TMap<UK2Output*, FString>::TIterator It(Context.OutputToVarNameMap); It; ++It )
		{
			UK2Output* Output = It.Key();
			FString VarName = It.Value();

			UK2Node_Event* EventNode = Cast<UK2Node_Event>(Output->OwningNode);
			if(EventNode == NULL)
			{
				FString VarTypeString = Output->GetTypeAsCodeString();
				FString VarDeclareString = FString::Printf(TEXT("local %s %s;"), *VarTypeString, *VarName);
				OutCode.AddItem( FK2CodeLine(1, VarDeclareString) );
			}
		}

		OutCode.AddItem( FK2CodeLine("") ); // Blank line between var declarations and code

		FK2CodeLine::IndentAllCodeLines(NextCode); // Indent all the code we got
		OutCode.Append(NextCode);
	}

	OutCode.AddItem( FK2CodeLine(TEXT("}")) );
}

//////////////////////////////////////////////////////////////////////////
// UK2Node_MemberVar

FString UK2Node_MemberVar::GetDisplayName()
{
	if(VarType == K2CT_Object)
	{
		check( Outputs.Num() == 1 );
		UK2Output_Object* ObjOutput = CastChecked<UK2Output_Object>(Outputs(0));

		// For object vars, show name and type
		return FString::Printf(TEXT("%s (%s)"), *VarName, *ObjOutput->ObjClass->GetName());
	}
	else
	{
		return VarName;
	}
}

UBOOL UK2Node_MemberVar::InputDefaultsAreEditable()
{
	return FALSE;
}

void UK2Node_MemberVar::CreateAutoConnectors()
{
}

void UK2Node_MemberVar::CreateConnectorsFromVariable(UProperty* InVar)
{
	UClass* ObjVarClass = NULL; // Class of object, if this is an object variable
	UObjectProperty* ObjProp = Cast<UObjectProperty>(InVar);
	if(ObjProp)
	{
		check(VarType == K2CT_Object);
		ObjVarClass = ObjProp->PropertyClass;
	}	

	// Only create an input to write to this var if its non-const
	if(!InVar->HasAllPropertyFlags(CPF_Const))
	{
		CreateConnector(K2CD_Input, (EK2ConnectorType)VarType, TEXT("Set"), ObjVarClass);
	}

	// can always read the var
	CreateConnector(K2CD_Output, (EK2ConnectorType)VarType, TEXT("Get"), ObjVarClass);
}


#endif // WITH_EDITOR
