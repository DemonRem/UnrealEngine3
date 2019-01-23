/*=============================================================================
	UnFont.cpp: Unreal font code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UFont);
IMPLEMENT_CLASS(UMultiFont);


/**
* Serialize the object struct with the given archive
*
* @param Ar - archive to serialize with
*/
void UFont::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << Characters << Textures << Kerning;
	Ar << CharRemap << IsRemapped;
}

/**
* Called after object and all its dependencies have been serialized.
*/
void UFont::PostLoad()
{
	Super::PostLoad();
	for( INT TextureIndex = 0 ; TextureIndex < Textures.Num() ; ++TextureIndex )
	{
		if( Textures(TextureIndex) )
		{
			Textures(TextureIndex)->SetFlags(RF_Public);
		}
	}
}

INT UFont::GetResolutionPageIndex(FLOAT HeightTest)
{
	return 0;
}

FLOAT UFont::GetScalingFactor(FLOAT HeightTest)
{
	return 1.f;
}

/**
 * Retruns the maximum height for this font
 */
FLOAT UFont::GetMaxCharHeight()
{
	FLOAT Max = 0;
	for (INT i=0;i<Characters.Num();i++)
	{
		if ( i==0 || Max < Characters(i).VSize )
		{
			Max = Characters(i).VSize;
		}
	}
	return Max;
}

FLOAT UFont::StrWidth(const FString& Text)
{
	return GetStringSize(*Text, 0);
}



/**
 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
 *
 * @return		Size of resource as to be displayed to artists/ LDs in the Editor.
 */
INT UFont::GetResourceSize()
{
	FArchiveCountMem CountBytesSize( this );
	INT ResourceSize = CountBytesSize.GetNum();
	for( INT TextureIndex = 0 ; TextureIndex < Textures.Num() ; ++TextureIndex )
	{
		if ( Textures(TextureIndex) )
		{
			ResourceSize += Textures(TextureIndex)->GetResourceSize();
		}
	}
	return ResourceSize;
}

/**
 * Serialize the ResolutionTestTable as well
 */
void UMultiFont::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << ResolutionTestTable;
}


/**
 * Find the best fit in the resolution table.
 *
 * @returns the index that best fits the HeightTest
 */
INT UMultiFont::GetResolutionTestTableIndex(FLOAT HeightTest)
{
	int RTTIndex = 0;
	for (INT i=1;i< ResolutionTestTable.Num(); i++)
	{
		if (Abs( ResolutionTestTable(i) - HeightTest) < Abs( ResolutionTestTable(RTTIndex) - HeightTest) )
		{
			RTTIndex = i;
		}
	}
	return RTTIndex;
}

/**
 * Calculate the starting index in to the character array for a given resolution
 *
 * @returns the index
 */
INT UMultiFont::GetResolutionPageIndex(FLOAT HeightTest)
{
	INT RTTIndex = GetResolutionTestTableIndex(HeightTest);
	if ( RTTIndex < ResolutionTestTable.Num() )
	{
		return RTTIndex * ( Characters.Num() / ResolutionTestTable.Num() );
	}
	return 0;
}

/**
 * Calculate the scaling factor for between resolutions
 *
 * @returns the scaling factor
 */
FLOAT UMultiFont::GetScalingFactor(FLOAT HeightTest)
{
	INT RTTIndex = GetResolutionTestTableIndex(HeightTest);
	if (RTTIndex < ResolutionTestTable.Num() )
	{
		return HeightTest / ResolutionTestTable(RTTIndex);
	}
	else
	{
		return 1.f;
	}
}
