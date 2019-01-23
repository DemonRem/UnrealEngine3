/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
/*-----------------------------------------------------------------------------
	FPropertyTag.
-----------------------------------------------------------------------------*/

/**
 *  A tag describing a class property, to aid in serialization.
 */
struct FPropertyTag
{
	// Archive for counting property sizes.
	class FArchiveCountSize : public FArchive
	{
	public:
		FArchiveCountSize( FArchive& InSaveAr )
		: Size(0), SaveAr(InSaveAr)
		{
			ArIsSaving				= InSaveAr.IsSaving();
			ArIsPersistent			= InSaveAr.IsPersistent();
			ArSerializingDefaults	= InSaveAr.IsSerializingDefaults();
		}
		INT Size;
	private:
		FArchive& SaveAr;
		FArchive& operator<<( UObject*& Obj )
		{
			INT Index = SaveAr.MapObject(Obj);
			FArchive& Ar = *this;
			return Ar << Index;
		}
		FArchive& operator<<( FName& Name )
		{
			INT Index = SaveAr.MapName(&Name);
			INT Number = Name.GetNumber();
			FArchive& Ar = *this;
			return Ar << Index << Number;
		}
		void Serialize( void* V, INT Length )
		{
			Size += Length;
		}
	};

	// Variables.
	FName	Type;		// Type of property
	UBOOL	BoolVal;	// a boolean property's value (never need to serialize data for bool properties except here)
	FName	Name;		// Name of property.
	FName	ItemName;	// Struct name if UStructProperty.
	INT		Size;       // Property size.
	INT		ArrayIndex;	// Index if an array; else 0.

	// Constructors.
	FPropertyTag()
	{}
	FPropertyTag( FArchive& InSaveAr, UProperty* Property, INT InIndex, BYTE* Value, BYTE* Defaults )
	:	Type		(Property->GetID())
	,	Name		(Property->GetFName())
	,	ItemName	(NAME_None)
	,	Size		(0)
	,	ArrayIndex	(InIndex)
	{
		// Handle structs.
		UStructProperty* StructProperty = Cast<UStructProperty>( Property, CLASS_IsAUStructProperty );
		if( StructProperty )
			ItemName = StructProperty->Struct->GetFName();

		// Set size.
		FArchiveCountSize ArCount( InSaveAr );
		SerializeTaggedProperty( ArCount, Property, Value, 0, Defaults );
		Size = ArCount.Size;

		UBoolProperty* Bool = Cast<UBoolProperty>(Property);
		BoolVal = (Bool && (*(BITFIELD*)Value & Bool->BitMask)) ? TRUE : FALSE;
	}

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FPropertyTag& Tag )
	{
		// Name.
		Ar << Tag.Name;
		if( Tag.Name == NAME_None )
		{
			return Ar;
		}

		Ar << Tag.Type << Tag.Size << Tag.ArrayIndex;

		// only need to serialize this for structs
		if (Tag.Type == NAME_StructProperty)
		{
			Ar << Tag.ItemName;
		}
		// only need to serialize this for bools
		if (Tag.Type == NAME_BoolProperty)
		{
			Ar << Tag.BoolVal;
		}

		return Ar;
	}

	// Property serializer.
	void SerializeTaggedProperty( FArchive& Ar, UProperty* Property, BYTE* Value, INT MaxReadBytes, BYTE* Defaults )
	{
		if (Property->GetClass() == UBoolProperty::StaticClass())
		{
			UBoolProperty* Bool = (UBoolProperty*)Property;
			check(Bool->BitMask!=0);
			if (Ar.IsLoading())
			{
				if (BoolVal)
				{
					*(BITFIELD*)Value |=  Bool->BitMask;
				}
				else
				{
					*(BITFIELD*)Value &= ~Bool->BitMask;
				}
			}
		}
		else
		{
			UProperty* OldSerializedProperty = GSerializedProperty;
			GSerializedProperty = Property;

			Property->SerializeItem( Ar, Value, MaxReadBytes, Defaults );

			GSerializedProperty = OldSerializedProperty;
		}
	}
};

