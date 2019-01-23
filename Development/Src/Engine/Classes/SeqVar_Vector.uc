/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqVar_Vector extends SequenceVariable
	native(Sequence);

cpptext
{
	virtual FVector* GetVectorRef()
	{
		return &VectValue;
	}

	virtual FString GetValueStr()
	{
		return VectValue.ToString();
	}
}

var() vector VectValue;

defaultproperties
{
	ObjName="Vector"
	ObjColor=(R=128,G=128,B=0,A=255)
}
