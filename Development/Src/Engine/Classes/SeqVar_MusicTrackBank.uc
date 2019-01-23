/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqVar_MusicTrackBank extends SeqVar_Object
	native(Sequence);


/**
 * The bank of tracks
 **/
var() array<MusicTrackStruct> MusicTrackBank;



cpptext
{
	virtual UObject** GetObjectRef( INT Idx )
	{
		if( Idx != 0 )
		{
			return NULL;
		}
		return &ObjValue;
	}

	virtual FString GetValueStr()
	{
		return FString::Printf(TEXT("%s"),ObjValue!=NULL?*ObjValue->GetName():TEXT("???"));
	}

	virtual void OnExport()
	{
		ObjValue = NULL;
	}

	// USequenceVariable interface
	virtual void DrawExtraInfo(FCanvas* Canvas, const FVector& CircleCenter);

	virtual UBOOL SupportsProperty(UProperty *Property)
	{
		return (Property->IsA(UObjectProperty::StaticClass()) ||
				(Property->IsA(UArrayProperty::StaticClass()) && ((UArrayProperty*)Property)->Inner->IsA(UObjectProperty::StaticClass())));
	}

	virtual void PublishValue(USequenceOp *Op, UProperty *Property, FSeqVarLink &VarLink);
	virtual void PopulateValue(USequenceOp *Op, UProperty *Property, FSeqVarLink &VarLink);
}



defaultproperties
{
	ObjName="Music Track Bank"
	ObjCategory="Sound"
	ObjColor=(R=255,G=0,B=255,A=255)
}
