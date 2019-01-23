/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqVar_String extends SequenceVariable
	native(Sequence);

cpptext
{
	FString* GetStringRef()
	{
		return &StrValue;
	}

	FString GetValueStr()
	{
		return StrValue;
	}

	virtual UBOOL SupportsProperty(UProperty *Property)
	{
		return (Property->IsA(UStrProperty::StaticClass()));
	}

	virtual void PublishValue(USequenceOp *Op, UProperty *Property, FSeqVarLink &VarLink);
	virtual void PopulateValue(USequenceOp *Op, UProperty *Property, FSeqVarLink &VarLink);
}

var() string			StrValue;

defaultproperties
{
	ObjName="String"
	ObjColor=(R=0,G=255,B=0,A=255)
}
