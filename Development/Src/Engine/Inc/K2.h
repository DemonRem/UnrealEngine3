/*=============================================================================
	K2.h
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __K2_H__
#define __K2_H__


// Object that represents one line of resulting source code
struct FK2CodeLine
{
	// Current indent level of code
	INT	IndentLevel;
	// Actual text of code line
	FString Code;

	FK2CodeLine(INT InIndent, const FString& InCode)
	{
		IndentLevel = InIndent;
		Code = InCode;
	}

	FK2CodeLine(const FString& InCode)
	{
		IndentLevel = 0;
		Code = InCode;
	}

	void Indent()
	{
		IndentLevel++;
	}

	FString GetString()
	{
		FString OutString;
		for (int i=0; i<IndentLevel; i++)
		{
			OutString += TEXT("\t");
		}
		OutString += Code + (TEXT("\n"));;
		return OutString;
	}

	// Util to indent all lines passed in
	static void IndentAllCodeLines(TArray<FK2CodeLine>& Lines)
	{
		for (int i = 0; i < Lines.Num(); i++)
		{
			Lines(i).Indent();
		}
	}
};

// Util that generates unique names given a 'base'
struct FK2VarNameGenerator
{
	/** Map of variable name, to the number of times it has been used */
	TMap<FString, INT>	VarNameToCountMap;

	/** Util that gets a unique name for a varibale, based on the 'base' name passed in */
	FString GetUniqueName(const FString& BaseName)
	{
		FString OutVarName;
		INT VarCount = 0;

		INT* VarCountPtr = VarNameToCountMap.Find(BaseName);

		if(VarCountPtr == NULL)
		{
			VarCount++;
			OutVarName = FString::Printf(TEXT("%s_%d"), *BaseName, VarCount);
			VarNameToCountMap.Set(BaseName, VarCount);
		}
		else
		{
			OutVarName = FString::Printf(TEXT("%s_%d"), *BaseName, VarCount);
			VarNameToCountMap.Set(BaseName, VarCount);
		}

		return OutVarName;
	}
};


struct FK2CodeGenContext
{
	/** If code generation failed */
	UBOOL							bGenFailure;

	/** Information about the failure */
	FString							GenFailMessage;

	/** Used to generate unique variable names */
	FK2VarNameGenerator				VarNameGen;

	/** Map of output connector to the variable name it represents */
	TMap<UK2Output*, FString>		OutputToVarNameMap;

	/** Used to store nodes that we have processed so far */
	TMap<UK2NodeBase*, UBOOL>		ProcessedNodes;

	FK2CodeGenContext()
	{
		bGenFailure = FALSE;
	}

	/** See if the supplied node has already been processed */
	UBOOL IsNodeAlreadyProcessed(UK2NodeBase* InNode)
	{
		// if its in the map, it has been processed
		return( ProcessedNodes.Find(InNode) != NULL );
	}

	/** Indicate that the supplied node has been processed */
	void MarkNodeProcessed(UK2NodeBase* InNode)
	{
		if( !IsNodeAlreadyProcessed(InNode) )
		{
			ProcessedNodes.Set(InNode, TRUE);
		}
	}
};

/** Util for getting the K2 type from an Unreal property */
EK2ConnectorType GetTypeFromProperty(UProperty* InProp, UClass*& OutClass);

/** Convert function name (e.g. 'Add_FloatFloat') into operator (e.g. '+') */
FString FuncNameToOperator(const FString& InFuncName);

#endif // #define __K2_H__
