/*=============================================================================
	Texture.cpp: Implementation of UTexture.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UTexture);

void UTexture::ReleaseResource()
{
	check(Resource);

	// Free the resource.
	ReleaseResourceAndFlush(Resource);
	delete Resource;
	Resource = NULL;
}

void UTexture::UpdateResource()
{
	if(Resource)
	{
		// Release the existing texture resource.
		ReleaseResource();
	}

	if( !GIsUCC && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Create a new texture resource.
		Resource = CreateResource();
		if( Resource )
		{
			BeginInitResource(Resource);
		}
	}
}

void UTexture::PreEditChange(UProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
	if( Resource )
	{
		ReleaseResource();
	}
}

void UTexture::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange(PropertyThatChanged);

	// Determine whether any property that requires recompression of the texture has changed.
	UBOOL RequiresRecompression = 0;
	if( PropertyThatChanged )
	{
		FString PropertyName = *PropertyThatChanged->GetName();

		if( appStricmp( *PropertyName, TEXT("RGBE")							) == 0
		||	appStricmp( *PropertyName, TEXT("CompressionNoAlpha")			) == 0
		||	appStricmp( *PropertyName, TEXT("CompressionNone")				) == 0
		||	appStricmp( *PropertyName, TEXT("CompressionNoMipmaps")			) == 0
		||	appStricmp( *PropertyName, TEXT("CompressionFullDynamicRange")	) == 0
		||	appStricmp( *PropertyName, TEXT("CompressionSettings")			) == 0
		||	appStricmp( *PropertyName, TEXT("bDitherMipMapAlpha")			) == 0
		||	appStricmp( *PropertyName, TEXT("bPreserveBorderR")				) == 0
		||	appStricmp( *PropertyName, TEXT("bPreserveBorderG")				) == 0
		||	appStricmp( *PropertyName, TEXT("bPreserveBorderB")				) == 0
		||	appStricmp( *PropertyName, TEXT("bPreserveBorderA")				) == 0
		)
		{
			RequiresRecompression = 1;
		}
	}
	else
	{
		RequiresRecompression = 1;
	}

	// Only compress when we really need to to avoid lag when level designers/ artists manipulate properties like clamping in the editor.
	if (RequiresRecompression)
	{
		UBOOL CompressionNoneSave = CompressionNone;
		if (!(
			(CompressionSettings == TC_Default)	||
			(CompressionSettings == TC_Normalmap) || 
			(CompressionSettings == TC_NormalmapAlpha)
			))
		{
			DeferCompression = FALSE;
		}

		if (DeferCompression)
		{
			CompressionNone = TRUE;
		}
		Compress();
		if (DeferCompression)
		{
			CompressionNone = CompressionNoneSave;
		}
	}

	// Update cached LOD bias.
	CachedCombinedLODBias = GSystemSettings.TextureLODSettings.CalculateLODBias( this );

	// Recreate the texture's resource.
	UpdateResource();
}

void UTexture::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if( Ar.Ver() < VER_REPLACED_LAZY_ARRAY_WITH_UNTYPED_BULK_DATA )
	{
		SourceArt.SerializeLikeLazyArray( Ar, this );
	}
	else
	{
		SourceArt.Serialize( Ar, this );
	}

	// Set initial texture grouping based on type.
	if( Ar.Ver() < VER_TEXTURE_GROUPS )
	{
		if( IsA(ULightMapTexture2D::StaticClass()) || IsA(UShadowMapTexture2D::StaticClass()) )
		{
			LODGroup = TEXTUREGROUP_LightAndShadowMap;
		}
		else
		{
			if( CompressionSettings == TC_Normalmap || CompressionSettings == TC_Normalmap )
			{
				LODGroup = TEXTUREGROUP_WorldNormalMap;
			}
			else
			{
				LODGroup = TEXTUREGROUP_World;
			}
		}
	}


	// Remap enums.
	//
	//	Before	After	Name
	//
	//	 0		 0		TEXTUREGROUP_World,
	//	 1		 1		TEXTUREGROUP_WorldNormalMap,
	//			 2		TEXTUREGROUP_WorldSpecular,
	//	 2		 3		TEXTUREGROUP_Character,
	//	 3		 4		TEXTUREGROUP_CharacterNormalMap,
	//			 5		TEXTUREGROUP_CharacterSpecular,
	//	 4		 6		TEXTUREGROUP_Weapon,
	//	 5		 7		TEXTUREGROUP_WeaponNormalMap,
	//			 8		TEXTUREGROUP_WeaponSpecular,
	//	 6		 9		TEXTUREGROUP_Vehicle,
	//	 7		10		TEXTUREGROUP_VehicleNormalMap,
	//			11		TEXTUREGROUP_VehicleSpecular,
	//	 8		12		TEXTUREGROUP_Effects,
	//	 9		13		TEXTUREGROUP_Skybox,
	//	10		14		TEXTUREGROUP_UI,
	//	11		15		TEXTUREGROUP_LightAndShadowMap,
	//	12		16		TEXTUREGROUP_RenderTarget
	
	// Map from corrupt to before.
	if( Ar.Ver() >= VER_HANDLE_NOT_EXISTING_VERTEX_FACTORIES 
	&&	Ar.Ver() < VER_ENUM_VALUE_SERIALIZED_BY_NAME )
	{		
		if( LODGroup >= 12 )
		{
			LODGroup -= 4;
		}
		else if( LODGroup >= 10 )
		{
			LODGroup -= 3;
		}
		else if( LODGroup >= 7 )
		{
			LODGroup -= 2;
		}
		else if( LODGroup >= 3 )
		{
			LODGroup -= 1;
		}
	}
	// Map from before to after.
	if( Ar.Ver() < VER_ENUM_VALUE_SERIALIZED_BY_NAME )
	{
		if( LODGroup >= 8 )
		{
			LODGroup += 4;
		}
		else if( LODGroup >= 6 )
		{
			LODGroup += 3;
		}
		else if( LODGroup >= 4 )
		{
			LODGroup += 2;
		}
		else if( LODGroup >= 2 )
		{
			LODGroup += 1;
		}
	}

	if( Ar.IsCountingMemory() )
	{
		//TODO: Count this member when calculating memory used
		// Ar << Resource;
	}
}

void UTexture::PostLoad()
{
	Super::PostLoad();

	// High dynamic range textures are currently always stored as RGBE (shared exponent) textures.
	// We explicitely set this here as older versions of the engine didn't correctly update the RGBE field.
	// @todo: Ensures that RGBE is correctly set to work around a bug in older versions of the engine.
	RGBE = (CompressionSettings == TC_HighDynamicRange);

	if( !IsTemplate() )
	{
		// Update cached LOD bias.
		CachedCombinedLODBias = GSystemSettings.TextureLODSettings.CalculateLODBias( this );

		// The texture will be cached by the cubemap it is contained within on consoles.
		UTextureCube* CubeMap = Cast<UTextureCube>(GetOuter());
		if (CubeMap == NULL)
		{
			// Recreate the texture's resource.
			UpdateResource();
		}
	}
}

void UTexture::BeginDestroy()
{
	Super::BeginDestroy();
	if( !UpdateStreamingStatus() && Resource )
	{
		// Send the rendering thread a release message for the texture's resource.
		BeginReleaseResource(Resource);
		Resource->ReleaseFence.BeginFence();
		// Keep track that we alrady kicked off the async release.
		bAsyncResourceReleaseHasBeenStarted = TRUE;
	}
}

UBOOL UTexture::IsReadyForFinishDestroy()
{
	UBOOL bReadyForFinishDestroy = FALSE;
	// Check whether super class is ready and whether we have any pending streaming requests in flight.
	if( Super::IsReadyForFinishDestroy() && !UpdateStreamingStatus() )
	{
		// Kick off async resource release if we haven't already.
		if( !bAsyncResourceReleaseHasBeenStarted && Resource )
		{
			// Send the rendering thread a release message for the texture's resource.
			BeginReleaseResource(Resource);
			Resource->ReleaseFence.BeginFence();
			// Keep track that we alrady kicked off the async release.
			bAsyncResourceReleaseHasBeenStarted = TRUE;
		}
		// Only allow FinishDestroy to be called once the texture resource has finished its rendering thread cleanup.
		else if( !Resource || !Resource->ReleaseFence.GetNumPendingFences() )
		{
			bReadyForFinishDestroy = TRUE;
		}
	}
	return bReadyForFinishDestroy;
}

void UTexture::FinishDestroy()
{
	Super::FinishDestroy();

	if(Resource)
	{
		check(!Resource->ReleaseFence.GetNumPendingFences());

		// Free the resource.
		delete Resource;
		Resource = NULL;
	}
}

void UTexture::PreSave()
{
	Super::PreSave();

	// If "no compress" is set, don't do it...
	if (CompressionNone)
	{
		return;
	}

	// Otherwise, if we are not already compressed, do it now.
	if (DeferCompression)
	{
		Compress();
		DeferCompression = false;
	}
}

/**
 * Used by various commandlets to purge Editor only data from the object.
 * 
 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
 */
void UTexture::StripData(UE3::EPlatformType TargetPlatform)
{
	Super::StripData(TargetPlatform);
	// Remove source art.
	SourceArt.RemoveBulkData();
}


/**
 * Initializes LOD settings by reading them from the passed in filename/ section.
 *
 * @param	IniFilename		Filename of ini to read from.
 * @param	IniSection		Section in ini to look for settings
 */
void FTextureLODSettings::Initialize( const TCHAR* IniFilename, const TCHAR* IniSection )
{
	// Zero initialize.
	appMemzero( TextureLODGroups, sizeof(FTextureLODGroup) * ARRAY_COUNT(TextureLODGroups) );
	
	// Read individual entries. This must be updated whenever new entries are added to the enumeration.
	ReadEntry( TEXTUREGROUP_World				, TEXT("TEXTUREGROUP_World")				, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_WorldNormalMap		, TEXT("TEXTUREGROUP_WorldNormalMap")		, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_WorldSpecular		, TEXT("TEXTUREGROUP_WorldSpecular")		, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_Character			, TEXT("TEXTUREGROUP_Character")			, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_CharacterNormalMap	, TEXT("TEXTUREGROUP_CharacterNormalMap")	, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_CharacterSpecular	, TEXT("TEXTUREGROUP_CharacterSpecular")	, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_Weapon				, TEXT("TEXTUREGROUP_Weapon")				, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_WeaponNormalMap		, TEXT("TEXTUREGROUP_WeaponNormalMap")		, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_WeaponSpecular		, TEXT("TEXTUREGROUP_WeaponSpecular")		, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_Vehicle				, TEXT("TEXTUREGROUP_Vehicle")				, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_VehicleNormalMap	, TEXT("TEXTUREGROUP_VehicleNormalMap")		, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_VehicleSpecular		, TEXT("TEXTUREGROUP_VehicleSpecular")		, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_Effects				, TEXT("TEXTUREGROUP_Effects")				, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_Skybox				, TEXT("TEXTUREGROUP_Skybox")				, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_UI					, TEXT("TEXTUREGROUP_UI")					, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_LightAndShadowMap	, TEXT("TEXTUREGROUP_LightAndShadowMap")	, IniFilename, IniSection );
	ReadEntry( TEXTUREGROUP_RenderTarget		, TEXT("TEXTUREGROUP_RenderTarget")			, IniFilename, IniSection );
}

/**
 * Returns the texture group names, sorted like enum.
 *
 * @return array of texture group names
 */
TArray<FString> FTextureLODSettings::GetTextureGroupNames()
{
	// Order is important!
	TArray<FString> TextureGroupNames;
	new(TextureGroupNames) FString(TEXT("World"));
	new(TextureGroupNames) FString(TEXT("WorldNormalMap"));
	new(TextureGroupNames) FString(TEXT("WorldSpecular"));
	new(TextureGroupNames) FString(TEXT("Character"));
	new(TextureGroupNames) FString(TEXT("CharacterNormalMap"));
	new(TextureGroupNames) FString(TEXT("CharacterSpecular"));
	new(TextureGroupNames) FString(TEXT("Weapon"));
	new(TextureGroupNames) FString(TEXT("WeaponNormalMap"));
	new(TextureGroupNames) FString(TEXT("WeaponSpecular"));
	new(TextureGroupNames) FString(TEXT("Vehicle"));
	new(TextureGroupNames) FString(TEXT("VehicleNormalMap"));
	new(TextureGroupNames) FString(TEXT("VehicleSpecular"));
	new(TextureGroupNames) FString(TEXT("Effects"));
	new(TextureGroupNames) FString(TEXT("Skybox"));
	new(TextureGroupNames) FString(TEXT("UI"));
	new(TextureGroupNames) FString(TEXT("LightAndShadowMap"));
	new(TextureGroupNames) FString(TEXT("RenderTarget"));
	return TextureGroupNames;
}

/**
 * Reads a single entry and parses it into the group array.
 *
 * @param	GroupId			Id/ enum of group to parse
 * @param	GroupName		Name of group to look for in ini
 * @param	IniFilename		Filename of ini to read from.
 * @param	IniSection		Section in ini to look for settings
 */
void FTextureLODSettings::ReadEntry( INT GroupId, const TCHAR* GroupName, const TCHAR* IniFilename, const TCHAR* IniSection )
{
	// Look for string in filename/ section.
	FString Entry;
	if( GConfig->GetString( IniSection, GroupName, Entry, IniFilename ) )
	{
		// Trim whitespace at the beginning.
		Entry = Entry.Trim();
		// Remove brackets.
		Entry = Entry.Replace( TEXT("("), TEXT("") );
		Entry = Entry.Replace( TEXT(")"), TEXT("") );
		// Parse comma delimited entries into array.
		TArray<FString> Entries;
		Entry.ParseIntoArray( &Entries, TEXT(","), TRUE );

		for( INT EntryIndex=0; EntryIndex<Entries.Num(); EntryIndex++ )
		{
			// Parse minimum LOD mip count.
			INT		MinLODSize		= 0;
			FString ParsableEntry	= Entries(EntryIndex);
			if( Parse( *ParsableEntry, TEXT("MinLODSize="), MinLODSize ) )
			{
				TextureLODGroups[GroupId].MinLODMipCount = appCeilLogTwo( MinLODSize );
			}

			// Parse maximum LOD mip count.
			INT MaxLODSize = 0;
			if( Parse( *Entry, TEXT("MaxLODSize="), MaxLODSize ) )
			{
				TextureLODGroups[GroupId].MaxLODMipCount = appCeilLogTwo( MaxLODSize );
			}

			// Parse LOD bias.
			INT LODBias = 0;
			if( Parse( *Entry, TEXT("LODBias="), LODBias ) )
			{
				TextureLODGroups[GroupId].LODBias = LODBias;
			}
		}
	}
}

/**
 * Calculates and returns the LOD bias based on texture LOD group, LOD bias and maximum size.
 *
 * @param	Texture		Texture object to calculate LOD bias for.
 * @return	LOD bias
 */
INT FTextureLODSettings::CalculateLODBias( UTexture* Texture ) const
{	
	// Find LOD group.
	check( Texture );
	const FTextureLODGroup& LODGroup = TextureLODGroups[Texture->LODGroup];

	// Calculate maximum number of miplevels.
	const INT TextureMipCount	= appCeilLogTwo( appTrunc( Max( Texture->GetSurfaceWidth(), Texture->GetSurfaceHeight() ) ) );
	// Calculate LOD bias.
	INT UsedLODBias		= LODGroup.LODBias + Texture->LODBias;
	UsedLODBias			= TextureMipCount - Clamp( TextureMipCount - UsedLODBias, LODGroup.MinLODMipCount, LODGroup.MaxLODMipCount );

	// Return clamped LOD bias; we never increase detail.
	return Max( UsedLODBias, 0 );
}

/**
 * Will return the LODBias for a passed in LODGroup
 *
 * @param	InLODGroup		The LOD Group ID 
 * @return	LODBias
 */
INT FTextureLODSettings::GetTextureLODGroupLODBias( INT InLODGroup ) const
{
	INT Retval = 0;

	const FTextureLODGroup& LODGroup = TextureLODGroups[InLODGroup]; 

	Retval = LODGroup.LODBias;

	return Retval;
}

