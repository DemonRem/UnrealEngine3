/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqVar_RandomFloat extends SeqVar_Float
	native(Sequence);

cpptext
{
	virtual FLOAT* GetFloatRef()
	{
		FloatValue = Min + appFrand() * (Max - Min);
		return &FloatValue;
	}

	virtual FString GetValueStr()
	{
		return FString::Printf(TEXT("%2.1f..%2.1f"),Min,Max);
	}

	virtual UBOOL SupportsProperty(UProperty *Property)
	{
		return FALSE;
	}
}

/** Min value for randomness */
var() float Min;

/** Max value for randomness */
var() float Max;

defaultproperties
{
	ObjName="Random Float"
	ObjCategory="Float"

	Min=0.f
	Max=1.f
}
