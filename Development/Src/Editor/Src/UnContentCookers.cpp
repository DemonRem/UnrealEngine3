/*=============================================================================
	UnContentCookers.cpp: Various platform specific content cookers.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "EngineAIClasses.h"
#include "EngineMaterialClasses.h"
#include "EngineSoundNodeClasses.h"
#include "UnTerrain.h"
#include "EngineParticleClasses.h"
#include "UnCodecs.h"

#include "DebuggingDefines.h"

#define MPSounds_PackageName		TEXT("MPSounds")
#define MPSounds_ReferencerName		MPSounds_PackageName TEXT("ObjRef")

/** Which to use when compressing streaming mips data */
#if WITH_LZO
#define MIP_COMPRESSION_METHOD COMPRESS_LZO
#else	//#if WITH_LZO
#define MIP_COMPRESSION_METHOD COMPRESS_ZLIB
#endif	//#if WITH_LZO

static INT BrushPhysDataBytes = 0;


/**
 * Detailed execution time stats
 */
static DOUBLE	TotalTime;

static DOUBLE		LoadSectionPackagesTime;
static DOUBLE		LoadNativePackagesTime;
static DOUBLE		LoadDependenciesTime;
static DOUBLE		LoadPackagesTime;
static DOUBLE		LoadPerMapPackagesTime;
static DOUBLE		LoadCombinedStartupPackagesTime;

static DOUBLE		CookTime;
static DOUBLE			CookPhysicsTime;
static DOUBLE			CookTextureTime;
static DOUBLE			CookSoundTime;
static DOUBLE			CookMovieTime;
static DOUBLE			CookStripTime;
static DOUBLE			CookSkeletalMeshTime;
static DOUBLE			CookStaticMeshTime;

static DOUBLE		PackageSaveTime;

static DOUBLE		PrepareForSavingTime;
static DOUBLE			PrepareForSavingTextureTime;
static DOUBLE			PrepareForSavingTerrainTime;
static DOUBLE			PrepareForSavingMaterialTime;
static DOUBLE			PrepareForSavingMaterialInterfaceTime;

static DOUBLE		CollectGarbageAndVerifyTime;


using namespace UE3;

/**
 * @return A string representing the platform
 */
FString UCookPackagesCommandlet::GetPlatformString()
{
	switch (Platform)
	{
		case PLATFORM_Windows:
			return TEXT("PC");

		case PLATFORM_Xenon:
			return TEXT("Xenon");

		case PLATFORM_PS3:
			return TEXT("PS3");

		case PLATFORM_Linux:
			return TEXT("Linux");

		case PLATFORM_Mac:
			return TEXT("Mac");

		default:
			return TEXT("");
	}	
}

/**
 * @return The prefix used in ini files, etc for the platform
 */
FString UCookPackagesCommandlet::GetPlatformPrefix()
{
	switch (Platform)
	{
		case PLATFORM_Xenon:
			return TEXT("Xe");

		default:
			return GetPlatformString();
	}	
}

/**
 * @return The name of the output cooked data directory
 */
FString UCookPackagesCommandlet::GetCookedDirectory()
{
	return FString(TEXT("Cooked")) + GetPlatformString();
}

/**
 * @return The name of the directory where cooked ini files go
 */
FString UCookPackagesCommandlet::GetConfigOutputDirectory()
{
	return GetPlatformString() * TEXT("Cooked");
}

/**
 * @return The prefix to pass to appCreateIniNamesAndThenCheckForOutdatedness for non-platform specific inis
 */
FString UCookPackagesCommandlet::GetConfigOutputPrefix()
{
	return GetConfigOutputDirectory() + PATH_SEPARATOR;

}

/**
 * @return The prefix to pass to appCreateIniNamesAndThenCheckForOutdatedness for platform specific inis
 */
FString UCookPackagesCommandlet::GetPlatformConfigOutputPrefix()
{
	return GetConfigOutputPrefix() + GetPlatformPrefix() + TEXT("-");
}

/**
 * @return The default ini prefix to pass to appCreateIniNamesAndThenCheckForOutdatedness for 
 */
FString UCookPackagesCommandlet::GetPlatformDefaultIniPrefix()
{
	return GetPlatformString() * GetPlatformString();
}

/**
 * @return TRUE if the destination platform expects pre-byteswapped data (packages, coalesced ini files, etc)
 */
UBOOL UCookPackagesCommandlet::ShouldByteSwapData()
{
	return Platform == PLATFORM_Xenon || Platform == PLATFORM_PS3;
}

/**
 * Cooks passed in object if it hasn't been already.
 *
 * @param	 Object		Object to cook
 */
void UCookPackagesCommandlet::CookObject( UObject* Object )
{ 
	if( Object && !Object->HasAnyFlags( RF_Cooked ) )
	{
		if( !Object->HasAnyFlags( RF_ClassDefaultObject ) )
		{
			STAT(FScopeSecondsCounter CookTimeCounter(CookTime));

			UTexture2D*			Texture2D			= Cast<UTexture2D>(Object);
			UTextureMovie*		TextureMovie		= Cast<UTextureMovie>(Object);
			USoundNodeWave*		SoundNodeWave		= Cast<USoundNodeWave>(Object);
			UTerrainComponent*	TerrainComponent	= Cast<UTerrainComponent>(Object);
			UBrushComponent*	BrushComponent		= Cast<UBrushComponent>(Object);
			UParticleSystem*	ParticleSystem		= Cast<UParticleSystem>(Object);

			USkeletalMesh*		SkeletalMesh		= Cast<USkeletalMesh>(Object);
			UStaticMesh*		StaticMesh			= Cast<UStaticMesh>(Object);
			UClass*				ClassObj			= Cast<UClass>(Object);

			{
				STAT(FScopeSecondsCounter CookPhysicsTimeCounter(CookPhysicsTime));


				// Rebuild Novodex brush data for BrushComponents that are part of Volumes (eg blocking volumes)
				if (BrushComponent && !BrushComponent->IsTemplate() && BrushComponent->GetOwner() && BrushComponent->GetOwner()->IsVolumeBrush())
				{
					BrushComponent->BuildSimpleBrushCollision();
					BrushComponent->BuildPhysBrushData();

					// Add to memory used total.
					for(INT HullIdx = 0; HullIdx < BrushComponent->CachedPhysBrushData.CachedConvexElements.Num(); HullIdx++)
					{
						FKCachedConvexDataElement& Hull = BrushComponent->CachedPhysBrushData.CachedConvexElements(HullIdx);
						BrushPhysDataBytes += Hull.ConvexElementData.Num();
					}
				}
			}

			// Cook texture, storing in platform format.
			if( Texture2D )
			{
				CookTexture( Texture2D );
			}
			// Cook movie texture data
			else if( TextureMovie )
			{
				CookMovieTexture( TextureMovie );
			}
			// Cook sound, compressing to platform format
			else if( SoundNodeWave )
			{
				CookSoundNodeWave( SoundNodeWave );

				// Free up the data cached for the platforms we are not cooking for
				if( Platform == PLATFORM_Xenon )
				{
					SoundNodeWave->CompressedPCData.RemoveBulkData();
					SoundNodeWave->CompressedPS3Data.RemoveBulkData();
				}
				else if( Platform == PLATFORM_PS3 )
				{
					SoundNodeWave->CompressedPCData.RemoveBulkData();
					SoundNodeWave->CompressedXbox360Data.RemoveBulkData();
				}
				else if( Platform == PLATFORM_Windows )
				{
					SoundNodeWave->CompressedPS3Data.RemoveBulkData();
					SoundNodeWave->CompressedXbox360Data.RemoveBulkData();
				}
			}

			// Cook particle systems
			else if (ParticleSystem)
			{
				CookParticleSystem(ParticleSystem);
			}

			else if (SkeletalMesh)
			{
				CookSkeletalMesh(SkeletalMesh);
			}

			else if (StaticMesh)
			{
				CookStaticMesh(StaticMesh);
			}
			else if ( ClassObj && (Platform&PLATFORM_Console) != 0 )
			{
				// if cooking for console and this is the StaticMeshCollectionActor class, mark it placeable
				// so that it can be spawned at runtime
				if ( ClassObj == AStaticMeshCollectionActor::StaticClass() )
				{
					ClassObj->ClassFlags |= CLASS_Placeable;
				}
			}
		}

		{
			STAT(FScopeSecondsCounter CookStripTimeCounter(CookStripTime));
			// Remove extra data by platform
			Object->StripData(Platform);
		}

		// Avoid re-cooking.
		Object->SetFlags( RF_Cooked );
	}
}

/** mirrored in XeTools.cpp */
enum TextureCookerCreateFlags
{
	// skip miptail packing
	TextureCreate_NoPackedMip = 1<<0
};

// always use packed miptails
DWORD GCreationFlags = 0;

/**
 * Helper function used by CookObject - performs texture specific cooking.
 *
 * @param	Texture		Texture to cook
 */
void UCookPackagesCommandlet::CookTexture( UTexture2D* Texture2D )
{
	// Cook 2D textures.
	check(Texture2D);
	if( !Texture2D->IsTemplate( RF_ClassDefaultObject ) )
	{
		STAT(FScopeSecondsCounter CookTextureTimeCounter(CookTextureTime));

		// Strip out miplevels for UI textures.
		if( Texture2D->LODGroup == TEXTUREGROUP_UI )
		{
			if( Texture2D->Mips.Num() > 1 )
			{
				Texture2D->Mips.Remove( 1, Texture2D->Mips.Num() - 1 );
			}
		}

		// Take texture LOD settings into account, avoiding cooking and keeping memory around for unused miplevels.
		INT	FirstMipIndex = Clamp<INT>( PlatformLODSettings.CalculateLODBias( Texture2D ), 0, Texture2D->Mips.Num()-1 );		

		if( Platform == PLATFORM_PS3 || Platform == PLATFORM_Xenon )
		{
			// Initialize texture cooker for given format and size.
			TextureCooker->Init( 
				Texture2D->Format, 
				Texture2D->SizeX, 
				Texture2D->SizeY, 
				Texture2D->Mips.Num(),
				GCreationFlags);

			// first level of the packed miptail
			Texture2D->MipTailBaseIdx = TextureCooker->GetMipTailBase();

			// make sure we load at least the first packed mip level
			FirstMipIndex = Min(FirstMipIndex, Texture2D->MipTailBaseIdx);

			// Only cook mips up to the first packed mip level
			INT MaxMipLevel = Texture2D->Mips.Num();
			if( !(GCreationFlags&TextureCreate_NoPackedMip) )
			{
				MaxMipLevel = Min(Texture2D->MipTailBaseIdx,MaxMipLevel);
			}		

			for( INT MipLevel=FirstMipIndex; MipLevel<MaxMipLevel; MipLevel++ )
			{
				FTexture2DMipMap& Mip = Texture2D->Mips(MipLevel);

				// Allocate enough memory for cooked miplevel.
				UINT	MipSize				= TextureCooker->GetMipSize( MipLevel );
				// a size of 0 means to use original data size as dest size
				if (MipSize == 0)
				{
					MipSize = Mip.Data.GetBulkDataSize();
				}

				void*	IntermediateData	= appMalloc( MipSize );
				appMemzero(IntermediateData, MipSize);

				UINT	SrcRowPitch			= Max<UINT>( 1,	(Texture2D->SizeX >> MipLevel) / GPixelFormats[Texture2D->Format].BlockSizeX ) 
											* GPixelFormats[Texture2D->Format].BlockBytes;
				// Resize upfront to new size to work around issue in Xbox 360 texture cooker reading memory out of bounds.
				// zero-out the newly allocated block of memory as we may not end up using it all
				Mip.Data.Lock(LOCK_READ_WRITE);

				// remember the size of the buffer before realloc
				const INT SizeBeforeRealloc = Mip.Data.GetBulkDataSize();
				void*	MipData				= Mip.Data.Realloc( MipSize );

				// get the size of the newly allocated region
				const INT SizeOfReallocRegion = Mip.Data.GetBulkDataSize() - SizeBeforeRealloc;
				if ( SizeOfReallocRegion > 0 )
				{
					appMemzero((BYTE*)MipData + SizeBeforeRealloc, SizeOfReallocRegion);
				}

				// Cook the miplevel into the intermediate memory.
				TextureCooker->CookMip( MipLevel, MipData, IntermediateData, SrcRowPitch );

				// And replace existing data.
				appMemcpy( MipData, IntermediateData, MipSize );
				appFree( IntermediateData );
				Mip.Data.Unlock();
			}

			// Cook the miptail. This will be the last mip level of the texture
			if( !(GCreationFlags&TextureCreate_NoPackedMip) &&
				Texture2D->MipTailBaseIdx < Texture2D->Mips.Num() )
			{
				// Should always be a multiple of the tile size for this texture's format			
				UINT MipTailSize = TextureCooker->GetMipSize(Texture2D->MipTailBaseIdx); 
				// Source mip data for base level
				FTexture2DMipMap& BaseLevelMip = Texture2D->Mips(Texture2D->MipTailBaseIdx);

				// Allocate space for the mip tail
				void* DstMipTail = new BYTE[MipTailSize];
				appMemzero(DstMipTail, MipTailSize);

				// Arrays to hold the data for the tail mip levels
				const INT TailMipLevelCount = Texture2D->Mips.Num() - Texture2D->MipTailBaseIdx;
				void** SrcMipTailData = new void*[TailMipLevelCount];
				UINT* SrcMipPitch = new UINT[TailMipLevelCount];
				appMemzero(SrcMipPitch, TailMipLevelCount * sizeof(UINT));

				// Build up arrays of data to send to the MipTail cooker
				for( INT MipLevel = Texture2D->MipTailBaseIdx; MipLevel < Texture2D->Mips.Num(); MipLevel++ )
				{
					// source mip data for this MipLevel
					FTexture2DMipMap& Mip = Texture2D->Mips(MipLevel);
					// surface pitch for this mip level
					SrcMipPitch[MipLevel - Texture2D->MipTailBaseIdx] = Max<UINT>( 1, (Texture2D->SizeX >> MipLevel) / GPixelFormats[Texture2D->Format].BlockSizeX ) 
						* GPixelFormats[Texture2D->Format].BlockBytes;

					// lock source data for use. realloc to MipTailSize to work around issue with Xbox 360 texture cooker.
					// zero-out the newly allocated block of memory as we may not end up using it all
					Mip.Data.Lock(LOCK_READ_WRITE);

					// remember the size of the buffer before realloc
					const INT SizeBeforeRealloc = Mip.Data.GetBulkDataSize();
					void* MipData = Mip.Data.Realloc(MipTailSize);

					// get the size of the newly allocated region
					const INT SizeOfReallocRegion = Mip.Data.GetBulkDataSize() - SizeBeforeRealloc;
					if ( SizeOfReallocRegion > 0 )
					{
						appMemzero((BYTE*)MipData + SizeBeforeRealloc, SizeOfReallocRegion);
					}

					SrcMipTailData[MipLevel - Texture2D->MipTailBaseIdx] = MipData;
				}

				// Cook the tail mips together
				TextureCooker->CookMipTail( SrcMipTailData, SrcMipPitch, DstMipTail );

				// And replace existing data. Base level is already locked for writing
				appMemcpy( BaseLevelMip.Data.Realloc( MipTailSize ), DstMipTail, MipTailSize );
				delete [] DstMipTail;
				
				// Unlock the src mip data
				for( INT MipLevel = Texture2D->MipTailBaseIdx+1; MipLevel < Texture2D->Mips.Num(); MipLevel++ )
				{
					FTexture2DMipMap& Mip = Texture2D->Mips(MipLevel);
					// Clear out unused tail mips.
					Mip.Data.Realloc(0);
					Mip.Data.Unlock();
				}

				BaseLevelMip.Data.Unlock();

				delete [] SrcMipTailData;
				delete [] SrcMipPitch;
			}
		}
		else
		{
			// Make sure data is (and remains) loaded for (and after) saving.
			for( INT MipLevel=FirstMipIndex; MipLevel<Texture2D->Mips.Num(); MipLevel++ )
			{
				FTexture2DMipMap& Mip = Texture2D->Mips(MipLevel);
				Mip.Data.Lock(LOCK_READ_WRITE);
				Mip.Data.Unlock();
			}
		}
	}
}

/**
* Check the first bytes of the movie stream for a valid signature
*
* @param Buffer - movie stream buffer including header
* @param BufferSize - total size of movie stream buffer
* @param Signature - signature to compare against
* @param SignatureSize - size of the signature buffer
* @return TRUE if success
*/
UBOOL IsMovieSignatureValid(void* Buffer,INT BufferSize,BYTE* Signature,INT SignatureSize)
{
	UBOOL Result = TRUE;
	// need at least enough room for the signature
	if ( BufferSize >= SignatureSize )
	{
		BYTE* BufferSignature = static_cast<BYTE*>(Buffer);
		// make sure there is a matching signature in the buffer
		for( INT i=0; i < SignatureSize; ++i )
		{
			if( Signature[i] != BufferSignature[i] )
			{
				Result = FALSE;
				break;
			}
		}
	}
	return Result;
}

/**
* Byte swap raw movie data byte stream 
*
* @param Buffer - movie stream buffer including header
* @param BufferSize - total size of movie stream buffer
*/
void EnsureMovieEndianess(void* Buffer, INT BufferSize)
{
	// endian swap the data
	DWORD* Data = static_cast<DWORD*>(Buffer);
	UINT   DataSize = BufferSize / 4;
	for (UINT DataIndex = 0; DataIndex < DataSize; ++DataIndex)
	{
		DWORD SourceData = Data[DataIndex];
		Data[DataIndex] = ((SourceData & 0x000000FF) << 24) |
			((SourceData & 0x0000FF00) <<  8) |
			((SourceData & 0x00FF0000) >>  8) |
			((SourceData & 0xFF000000) >> 24) ;
	}
}

/**
* Helper function used by CookObject - performs movie specific cooking.
*
* @param	TextureMovie	Movie texture to cook
*/
void UCookPackagesCommandlet::CookMovieTexture( UTextureMovie* TextureMovie )
{
	check(TextureMovie);
	if( !TextureMovie->IsTemplate(RF_ClassDefaultObject) )
	{
		if( ShouldByteSwapData() )
		{
			STAT(FScopeSecondsCounter CookMovieTimeCounter(CookMovieTime));

			// load the movie stream data
			void* Buffer = TextureMovie->Data.Lock(LOCK_READ_WRITE);
			INT BufferSize = TextureMovie->Data.GetBulkDataSize();
			UBOOL Result = FALSE;
			if( TextureMovie->DecoderClass == UCodecMovieBink::StaticClass() )
			{
				// check for a correct signature in the movie stream
				BYTE Signature[]={'B','I','K'};
				if( IsMovieSignatureValid(Buffer,BufferSize,Signature,ARRAY_COUNT(Signature)) )
				{
					// byte swap the data
					EnsureMovieEndianess(Buffer,BufferSize);
					Result = TRUE;
				}
			}
			else
			{
				warnf(NAME_Error, TEXT("Codec type [%s] not implemented! Removing movie data."),
					TextureMovie->DecoderClass ? *TextureMovie->DecoderClass->GetName() : TEXT("None"));
			}
			TextureMovie->Data.Unlock();
			if( !Result )
			{
				// invalid movie type so remove its data
				TextureMovie->Data.RemoveBulkData();
			}			
		}
	}
}

/**
 * Helper function used by CookSoundNodeWave - performs sound specific cooking.
 */
void UCookPackagesCommandlet::CookSoundNodeWave( USoundNodeWave* SoundNodeWave )
{
	STAT(FScopeSecondsCounter CookSoundTimeCounter(CookSoundTime));

	// Ensure the current platforms are cooked
	::CookSoundNodeWave( SoundNodeWave, Platform );
}

/**
 * Helper function used by CookObject - performs ParticleSystem specific cooking.
 *
 * @param	ParticleSystem	ParticleSystem to cook
 */
void UCookPackagesCommandlet::CookParticleSystem(UParticleSystem* ParticleSystem)
{
	check(ParticleSystem);

	// Cook out the thumbnail image - no reason to store it for gameplay.
	ParticleSystem->ThumbnailImageOutOfDate = TRUE;
	ParticleSystem->ThumbnailImage = NULL;
}

/**
 * Helper function used by CookSkeletalMesh - performs SkeletalMesh specific cooking.
 */
void UCookPackagesCommandlet::CookSkeletalMesh( USkeletalMesh* SkeletalMesh )
{
	STAT(FScopeSecondsCounter CookSkeletalMeshTimeCounter(CookSkeletalMeshTime));

	// this is only needed on PS3, but it could be called for Xbox
	if (Platform == PLATFORM_PS3)
	{
		SkeletalMeshCooker->Init();

		// loop through each LOD model
		for (INT nLOD=0; nLOD<SkeletalMesh->LODModels.Num(); nLOD++)
		{
			// loop through each section of the particular LOD
			for (INT nSection=0; nSection<SkeletalMesh->LODModels(nLOD).Sections.Num(); nSection++)
			{
				FRawStaticIndexBuffer<TRUE>&	indexBuf = SkeletalMesh->LODModels(nLOD).IndexBuffer;
				INT								numTrisPerSection = SkeletalMesh->LODModels(nLOD).Sections(nSection).NumTriangles;
				DWORD							baseIndex = SkeletalMesh->LODModels(nLOD).Sections(nSection).BaseIndex;
				WORD*							outputTriangleList = (WORD*)appMalloc(sizeof(WORD)*numTrisPerSection*3);

				if (outputTriangleList)
				{
					SkeletalMeshCooker->CookMesh(&indexBuf.Indices(baseIndex), numTrisPerSection, outputTriangleList);

					// replace the current section of indices with the newly reordered indices
					for (INT i=0; i<numTrisPerSection*3; i++)
					{
						indexBuf.Indices(baseIndex + i) = *(outputTriangleList+i);
					}
					appFree(outputTriangleList);
				}
			}
		}
	}
}

/**
 * Helper function used by CookStaticMesh - performs StaticMesh specific cooking.
 */
void UCookPackagesCommandlet::CookStaticMesh( UStaticMesh* StaticMesh )
{
	STAT(FScopeSecondsCounter CookStaticMeshTimeCounter(CookStaticMeshTime));

	// this is only needed on PS3, but it could be called for Xbox
	if (Platform == PLATFORM_PS3)
	{
		StaticMeshCooker->Init();

		// loop through each LOD model
		for (INT nLOD=0; nLOD<StaticMesh->LODModels.Num(); nLOD++)
		{
			// loop through each section of the particular LOD
			for (INT nElement=0; nElement<StaticMesh->LODModels(nLOD).Elements.Num(); nElement++)
			{
				FRawStaticIndexBuffer<TRUE>&	indexBuf = StaticMesh->LODModels(nLOD).IndexBuffer;
				INT								numTrisPerElement = StaticMesh->LODModels(nLOD).Elements(nElement).NumTriangles;
				DWORD							baseIndex = StaticMesh->LODModels(nLOD).Elements(nElement).FirstIndex;
				WORD*							outputTriangleList = (WORD*)appMalloc(sizeof(WORD)*numTrisPerElement*3);

				if (outputTriangleList)
				{
					StaticMeshCooker->CookMesh(&indexBuf.Indices(baseIndex), numTrisPerElement, outputTriangleList);

					// replace the current section of indices with the newly reordered indices
					for (INT i=0; i<numTrisPerElement*3; i++)
					{
						indexBuf.Indices(baseIndex + i) = *(outputTriangleList+i);
					}
					appFree(outputTriangleList);
				}
			}
		}
	}
}

/**
 * Creates an instance of a StaticMeshCollectorActor.  If a valid World is specified, uses SpawnActor;
 * otherwise, uses ConstructObject.
 *
 * @param	Package		the package to create the new actor in
 * @param	World		if Package corresponds to a map package, the reference to the UWorld object for the map.
 */
namespace
{
	template< class T >
	T* CreateComponentCollector( UPackage* Package, UWorld* World )
	{
		T* Result = NULL;
		
		if ( Package != NULL )
		{
			if ( World != NULL && World->PersistentLevel != NULL )
			{
				Result = Cast<T>(World->SpawnActor(T::StaticClass()));
			}
			else
			{
				Result = ConstructObject<T>(T::StaticClass(), Package);
			}

			if ( Result != NULL )
			{
				Result->SetFlags(RF_Cooked);
			}
		}

		return Result;
	}
};

/**
 * Cooks out all static mesh actors in the specified package by re-attaching their StaticMeshComponents to
 * a StaticMeshCollectionActor referenced by the world.
 *
 * @param	Package		the package being cooked
 */
void UCookPackagesCommandlet::CookStaticMeshActors( UPackage* Package )
{
	// 'Cook-out' material expressions on consoles
	// In this case, simply don't load them on the client
	// Keep 'parameter' types around so that we can get their defaults.
	if ( (GCookingTarget & PLATFORM_Console) != 0 )
	{
		// only cook-out StaticMeshActors when cooking for console
		check(Package);

		// find all StaticMeshActors and static Light actors which are referenced by something in the map
		UWorld* World = FindObject<UWorld>( Package, TEXT("TheWorld") );
		if ( World != NULL )
		{
			TArray<ULevel*> LevelsToSearch = World->Levels;

			// make sure that the world's PersistentLevel is part of the levels array for the purpose of this test.
			if ( World->PersistentLevel != NULL )
			{
				LevelsToSearch.AddUniqueItem(World->PersistentLevel);
			}
			for ( INT LevelIndex = 0; LevelIndex < LevelsToSearch.Num(); LevelIndex++ )
			{
				ULevel* Level = LevelsToSearch(LevelIndex);

				// we need to remove all StaticMeshActors and static Light actors from the level's Actors array so that we don't
				// get false positives during our reference checking.
				// however, we'll need to restore any actors which don't get cooked out, so keep track of their indices
				TMap<AStaticMeshActor*,INT> StaticActorIndices;
				TLookupMap<AStaticMeshActor*> StaticMeshActors;

				// remove all StaticMeshActors from the level's Actor array so that we don't get false positives.
				for ( INT ActorIndex = 0; ActorIndex < Level->Actors.Num(); ActorIndex++ )
				{
					AActor* Actor = Level->Actors(ActorIndex);
					if ( Actor != NULL && Actor->IsA(AStaticMeshActor::StaticClass()) )
					{
						AStaticMeshActor* StaticMeshActor = static_cast<AStaticMeshActor*>(Actor);

						StaticMeshActors.AddItem(StaticMeshActor);
						StaticActorIndices.Set(StaticMeshActor, ActorIndex);
						Level->Actors(ActorIndex) = NULL;
					}
				}

				// now use the object reference collector to find the static mesh actors that are still being referenced
				TArray<AStaticMeshActor*> ReferencedStaticMeshActors;
				TArchiveObjectReferenceCollector<AStaticMeshActor> SMACollector(&ReferencedStaticMeshActors, Package, FALSE, TRUE, TRUE, TRUE);
				Level->Serialize( SMACollector );

				// remove any StaticMeshActors which aren't valid for cooking out
				TFindObjectReferencers<AStaticMeshActor> StaticMeshReferencers(ReferencedStaticMeshActors, Package);
				for ( INT ActorIndex = ReferencedStaticMeshActors.Num() - 1; ActorIndex >= 0; ActorIndex-- )
				{
					AStaticMeshActor* StaticMeshActor = ReferencedStaticMeshActors(ActorIndex);
					UStaticMeshComponent* Component = StaticMeshActor->StaticMeshComponent;

					// for now, we'll ignore StaticMeshActors that are archetypes or instances of archetypes.
					if ( Component == NULL || Component->IsTemplate(RF_ArchetypeObject) || Component->GetArchetype()->IsTemplate(RF_ArchetypeObject) )
					{
						ReferencedStaticMeshActors.Remove(ActorIndex);
					}
					else
					{
						TArray<UObject*> Referencers;
						StaticMeshReferencers.MultiFind(StaticMeshActor, Referencers);
						for ( INT ReferencerIndex = Referencers.Num() - 1; ReferencerIndex >= 0; ReferencerIndex-- )
						{
							UObject* Referencer = Referencers(ReferencerIndex);
							if ( Referencer == StaticMeshActor->GetLevel() )
							{
								// if this is the actor's level, ignore this reference
								Referencers.Remove(ReferencerIndex);
							}
							else if ( Referencer == StaticMeshActor->StaticMeshComponent )
							{
								// if the referencer is the StaticMeshActor's StaticMeshComponent, we can ignore this reference as this means that
								// something else in the level is referencing the StaticMeshComponent (which will still be around even if we cook
								// out the StaticMeshActor)
								Referencers.Remove(ReferencerIndex);
							}
							else if ( Referencer->IsA(ANavigationPoint::StaticClass())
							&&	(static_cast<ANavigationPoint*>(Referencer)->Base == StaticMeshActor) )
							{
								// if this actor references the StaticMeshActor because it's based on the static mesh actor, ignore this reference
								Referencers.Remove(ReferencerIndex);
							}
						}

						// if this StaticMeshActor is still referenced by something, do not cook it out
						if ( Referencers.Num() > 0 )
						{
							ReferencedStaticMeshActors.Remove(ActorIndex);
						}
					}
				}

				// remove the referenced static mesh actors from the list of actors to be cooked-out.
				for ( INT ActorIndex = 0; ActorIndex < ReferencedStaticMeshActors.Num(); ActorIndex++ )
				{
					StaticMeshActors.RemoveItem(ReferencedStaticMeshActors(ActorIndex));
				}

				AStaticMeshCollectionActor* MeshCollector = NULL;
				for ( INT ActorIndex = 0; ActorIndex < StaticMeshActors.Num(); ActorIndex++ )
				{
					AStaticMeshActor* StaticMeshActor = StaticMeshActors(ActorIndex);
					UStaticMeshComponent* Component = StaticMeshActor->StaticMeshComponent;

					// if there is a limit to the number of components that can be added to a StaticMeshCollectorActor, create a new
					// one if we have reached the limit
					if (MeshCollector == NULL
					|| (MeshCollector->MaxStaticMeshComponents > 0
					&&  MeshCollector->StaticMeshComponents.Num() >= MeshCollector->MaxStaticMeshComponents) )
					{
						MeshCollector = CreateComponentCollector<AStaticMeshCollectionActor>(Package, World);
					}

					// UPrimitiveComponent::Detach() will clear the ShadowParent but it will never be restored, so save the reference and restore it later
					UPrimitiveComponent* ComponentShadowParent = Component->ShadowParent;

					// remove it from the StaticMeshActor.
					StaticMeshActor->DetachComponent(Component);

					// rather than duplicating the StaticMeshComponent into the mesh collector, rename the component into the collector
					// so that we don't lose any outside references to this component (@todo ronp - are external references to components even allowed?)
					const UBOOL bWasPublic = Component->HasAnyFlags(RF_Public);

					// clear the RF_Public flag so that we don't create a redirector
					Component->ClearFlags(RF_Public);

					// try to use the same component name - however, since we're renaming multiple components into the same Outer,
					// it's likely that we'll end up with a name conflict so first test to see if it would work and if not, use a
					// new name
					if ( Component->Rename(*Component->GetName(), MeshCollector, REN_ForceNoResetLoaders|REN_Test) )
					{
						Component->Rename(*Component->GetName(), MeshCollector, REN_ForceNoResetLoaders);
					}
					else
					{
						Component->Rename(NULL, MeshCollector, REN_ForceNoResetLoaders);
					}

					if ( bWasPublic )
					{
						Component->SetFlags(RF_Public);
					}

					// now add it to the mesh collector's StaticMeshComponents array
					MeshCollector->StaticMeshComponents.AddItem(Component);

					// it must also exist in the AllComponents array so that the component's physics data can be cooked
					MeshCollector->AllComponents.AddItem(Component);

					// copy any properties which are usually pulled from the Owner at runtime
					Component->Scale *= StaticMeshActor->DrawScale;
					Component->Scale3D *= StaticMeshActor->DrawScale3D;
					Component->ShadowParent = ComponentShadowParent;
					Component->HiddenGame = Component->HiddenGame || (!Component->bCastHiddenShadow && StaticMeshActor->bHidden);
					Component->HiddenEditor = Component->HiddenEditor || StaticMeshActor->bHiddenEd;

					// Since we folder the actor's DrawScale and DrawScale3D into the component's Scale/Scale3D properties, we
					// don't want the actor's scale included in the LocalToWorld matrix used for the component, so reset the actor's
					// scale to 1
					StaticMeshActor->DrawScale = 1.f;
					StaticMeshActor->DrawScale3D = FVector(1.f,1.f,1.f);
					Component->ConditionalUpdateTransform(StaticMeshActor->LocalToWorld());

					// now mark the StaticMeshActor with no-load flags so that it will disappear on save
					StaticMeshActor->SetFlags(RF_NotForClient|RF_NotForServer|RF_NotForEdit);

					debugf(NAME_DevCooking, TEXT("Cooking out StaticMeshActor %s; re-attaching %s to %s"),
						*StaticMeshActor->GetName(), *Component->GetName(), *MeshCollector->GetName());

					StaticActorIndices.Remove(StaticMeshActor);
				}

				// finally, restore the entries in the level's Actors array for the StaticMeshActors not being cooked out
				for ( TMap<AStaticMeshActor*,INT>::TIterator It(StaticActorIndices); It; ++It )
				{
					INT ActorIndex = It.Value();

					// make sure nothing filled in this entry in the array
					checkSlow(Level->Actors(ActorIndex)==NULL);
					Level->Actors(ActorIndex) = It.Key();
				}
			}
		}
	}
}

/**
 * Cooks out all static Light actors in the specified package by re-attaching their LightComponents to a 
 * StaticLightCollectionActor referenced by the world.
 */
void UCookPackagesCommandlet::CookStaticLightActors( UPackage* Package )
{
	if ( (GCookingTarget & PLATFORM_Console) != 0 )
	{
		// only cook-out static Lights when cooking for console
		check(Package);

		// find all StaticMeshActors and static Light actors which are referenced by something in the map
		UWorld* World = FindObject<UWorld>( Package, TEXT("TheWorld") );
		if ( World != NULL )
		{
			TArray<ULevel*> LevelsToSearch = World->Levels;

			// make sure that the world's PersistentLevel is part of the levels array for the purpose of this test.
			if ( World->PersistentLevel != NULL )
			{
				LevelsToSearch.AddUniqueItem(World->PersistentLevel);
			}
			for ( INT LevelIndex = 0; LevelIndex < LevelsToSearch.Num(); LevelIndex++ )
			{
				ULevel* Level = LevelsToSearch(LevelIndex);

				// we need to remove all static Light actors from the level's Actors array so that we don't
				// get false positives during our reference checking.
				// however, we'll need to restore any actors which don't get cooked out, so keep track of their indices
				TMap<ALight*,INT> StaticActorIndices;
				TLookupMap<ALight*> StaticLightActors;

				// remove all StaticMeshActors from the level's Actor array so that we don't get false positives.
				for ( INT ActorIndex = 0; ActorIndex < Level->Actors.Num(); ActorIndex++ )
				{
					AActor* Actor = Level->Actors(ActorIndex);
					if ( Actor != NULL && Actor->IsA(ALight::StaticClass()) && Actor->bStatic )
					{
						ALight* Light = static_cast<ALight*>(Actor);

						StaticLightActors.AddItem(Light);
						StaticActorIndices.Set(Light, ActorIndex);
						Level->Actors(ActorIndex) = NULL;
					}
				}

				// now use the object reference collector to find the static mesh actors that are still being referenced
				TArray<ALight*> ReferencedStaticLightActors;
				{
					TArchiveObjectReferenceCollector<ALight> LightCollector(&ReferencedStaticLightActors, Package, FALSE, TRUE, TRUE, TRUE);
					Level->Serialize( LightCollector );
				}

				// remove any static light actors which aren't valid for cooking out
				for ( INT ActorIndex = ReferencedStaticLightActors.Num() - 1; ActorIndex >= 0; ActorIndex-- )
				{
					ALight* Light = ReferencedStaticLightActors(ActorIndex);
					if ( Light->bStatic )
					{
						ULightComponent* Component = ReferencedStaticLightActors(ActorIndex)->LightComponent;

						// for now, we'll ignore static Lights that are archetypes or instances of archetypes.
						if ( Component != NULL
						&&	(Component->IsTemplate(RF_ArchetypeObject) || Component->GetArchetype()->IsTemplate(RF_ArchetypeObject)) )
						{
							ReferencedStaticLightActors.Remove(ActorIndex);
						}
					}
					else
					{
						ReferencedStaticLightActors.Remove(ActorIndex);
					}
				}

				TFindObjectReferencers<ALight> StaticLightReferencers(ReferencedStaticLightActors, Package);
				for ( INT ActorIndex = ReferencedStaticLightActors.Num() - 1; ActorIndex >= 0; ActorIndex-- )
				{
					ALight* LightActor = ReferencedStaticLightActors(ActorIndex);

					TArray<UObject*> Referencers;
					StaticLightReferencers.MultiFind(LightActor, Referencers);
					for ( INT ReferencerIndex = Referencers.Num() - 1; ReferencerIndex >= 0; ReferencerIndex-- )
					{
						UObject* Referencer = Referencers(ReferencerIndex);
						if ( Referencer == LightActor->GetLevel() )
						{
							// if this is the actor's level, ignore this reference
							Referencers.Remove(ReferencerIndex);
						}
						else if ( Referencer->IsIn(LightActor) && Referencer->IsA(UComponent::StaticClass()) )
						{
							// if the referencer is one of the LightActor's components, we can ignore this reference as this means that
							// something else in the level is referencing the component directly (which will still be around even if we cook
							// out the Light actor)
							Referencers.Remove(ReferencerIndex);
						}
					}

					// if this actor is still referenced by something, do not cook it out
					if ( Referencers.Num() > 0 )
					{
						ReferencedStaticLightActors.Remove(ActorIndex);
					}
				}

				for ( INT ActorIndex = 0; ActorIndex < ReferencedStaticLightActors.Num(); ActorIndex++ )
				{
					StaticLightActors.RemoveItem(ReferencedStaticLightActors(ActorIndex));
				}

				AStaticLightCollectionActor* LightCollector = NULL;
				for ( INT ActorIndex = 0; ActorIndex < StaticLightActors.Num(); ActorIndex++ )
				{
					ALight* LightActor = StaticLightActors(ActorIndex);
					ULightComponent* Component = LightActor->LightComponent;

					// if there is a limit to the number of components that can be added to a StaticLightCollectorActor, create a new
					// one if we have reached the limit
					if (LightCollector == NULL
					|| (LightCollector->MaxLightComponents > 0
					&&  LightCollector->LightComponents.Num() >= LightCollector->MaxLightComponents) )
					{
						LightCollector = CreateComponentCollector<AStaticLightCollectionActor>(Package, World);
					}

					// remove it from the Light actor.
					LightActor->DetachComponent(Component);

					// rather than duplicating the LightComponent into the light collector, rename the component into the collector
					// so that we don't lose any outside references to this component (@todo ronp - are external references to components even allowed?)
					const UBOOL bWasPublic = Component->HasAnyFlags(RF_Public);

					// clear the RF_Public flag so that we don't create a redirector
					Component->ClearFlags(RF_Public);

					// try to use the same component name - however, since we're renaming multiple components into the same Outer,
					// it's likely that we'll end up with a name conflict so first test to see if it would work and if not, use a
					// new name
					if ( Component->Rename(*Component->GetName(), LightCollector, REN_ForceNoResetLoaders|REN_Test) )
					{
						Component->Rename(*Component->GetName(), LightCollector, REN_ForceNoResetLoaders);
					}
					else
					{
						Component->Rename(NULL, LightCollector, REN_ForceNoResetLoaders);
					}

					if ( bWasPublic )
					{
						Component->SetFlags(RF_Public);
					}

					// now add it to the light collector's LightComponents array
					LightCollector->LightComponents.AddItem(Component);

					// it must also exist in the AllComponents array so that the component's physics data can be cooked
					LightCollector->AllComponents.AddItem(Component);

					// copy any properties which are usually pulled from the Owner at runtime
					// @todo


					// set the component's LightToWorld while we still have a reference to the original owning Light actor.  This matrix
					// will be serialized to disk by the LightCollector

					Component->LightToWorld = LightActor->LocalToWorld();

					// now mark the Light actor with no-load flags so that it will disappear on save
					LightActor->SetFlags(RF_NotForClient|RF_NotForServer|RF_NotForEdit);

					debugf(NAME_DevCooking, TEXT("Cooking out %s %s; re-attaching %s to %s"),
						*LightActor->GetClass()->GetName(), *LightActor->GetName(), *Component->GetName(), *LightCollector->GetName());

					StaticActorIndices.Remove(LightActor);
				}


				// finally, restore the entries in the level's Actors array for the static Lights not being cooked out
				for ( TMap<ALight*,INT>::TIterator It(StaticActorIndices); It; ++It )
				{
					INT ActorIndex = It.Value();

					// make sure nothing filled in this entry in the array
					checkSlow(Level->Actors(ActorIndex)==NULL);
					Level->Actors(ActorIndex) = It.Key();
				}
			}
		}
	}
}

/**
 * Make sure materials are compiled for the target platform and add them to the shader cache embedded 
 * into seekfree packages.
 * @param Material - Material to process
 */
void UCookPackagesCommandlet::CompileMaterialShaders( UMaterial* Material )
{	
	check(Material);
	FMaterialResource * MaterialResource = Material->GetMaterialResource(ShaderPlatform);

	if( !Material->HasAnyFlags( RF_ClassDefaultObject ) 
	//@todo: warn about a NULL MaterialResource?
	&&	MaterialResource
	&&	ShaderCache	
	&&	!AlreadyHandledMaterials.FindRef( Material )
	)
	{
		// Compile the material...
		TRefCountPtr<FMaterialShaderMap> MaterialShaderMapRef;
		//create an empty static parameter set since this is the base material
		FStaticParameterSet EmptySet(MaterialResource->GetId());
		if(MaterialResource->Compile( &EmptySet, ShaderPlatform, MaterialShaderMapRef, FALSE ))
		{
			check(MaterialShaderMapRef);
			// ... and add it to the shader cache being saved into the seekfree package.
			ShaderCache->AddMaterialShaderMap( MaterialShaderMapRef );
		}

		// Expressions are no longer needed as material is compiled now.
		//@todo. Put this back in one form or another.
		// Temporarily removed as it was removing expressions for materials that were
		// needed later when cooking terrain materials.
		//Material->RemoveExpressions();
	}
}

/**
* Make sure material instances are compiled and add them to the shader cache embedded into seekfree packages.
* @param MaterialInterface - MaterialInterface to process
*/

void UCookPackagesCommandlet::CompileMaterialInstanceShaders( UMaterialInstance* MaterialInterface )
{	
	
	check(MaterialInterface);
	//only process if the material instance has a static permutation 
	if( !MaterialInterface->HasAnyFlags( RF_ClassDefaultObject ) 
		&&	MaterialInterface->bHasStaticPermutationResource
		&&	ShaderCache	
		&&	!AlreadyHandledMaterialInstances.FindRef( MaterialInterface )
		)
	{
		const EMaterialShaderPlatform RequestedMaterialPlatform = GetMaterialPlatform(ShaderPlatform);
		// Compile the material instance...
		TRefCountPtr<FMaterialShaderMap> MaterialShaderMapRef;
		//setup static parameter overrides in the base material
		MaterialInterface->Parent->SetStaticParameterOverrides(MaterialInterface->StaticParameters[RequestedMaterialPlatform]);    
		//compile the static permutation resource for this platform
		if (MaterialInterface->StaticPermutationResources[RequestedMaterialPlatform]->Compile( MaterialInterface->StaticParameters[RequestedMaterialPlatform], ShaderPlatform, MaterialShaderMapRef, FALSE ))
		{
			check(MaterialShaderMapRef);
			// ... and add it to the shader cache being saved into the seekfree package.
			ShaderCache->AddMaterialShaderMap( MaterialShaderMapRef );
		}
		else
		{
			warnf(TEXT("Failed to compile static permutation for Material Instance %s"), *MaterialInterface->GetName());
		}
		//clear the static parameter overrides, so the base material's defaults will be used in the future
		MaterialInterface->Parent->ClearStaticParameterOverrides();
	}
}

/**
 * Prepares object for saving into package. Called once for each object being saved 
 * into a new package.
 *
 * @param	Package						Package going to be saved
 * @param	Object						Object to prepare
 * @param	bIsSavedInSeekFreePackage	Whether object is going to be saved into a seekfree package
 */
void UCookPackagesCommandlet::PrepareForSaving( UPackage* Package, UObject* Object, UBOOL bIsSavedInSeekFreePackage )
{
	STAT(FScopeSecondsCounter PrepareForSavingTimeCounter(PrepareForSavingTime));

	// Prepare texture for saving unless it's the class default object.
	UTexture2D* Texture2D = Cast<UTexture2D>(Object);
	if( Texture2D && !Texture2D->HasAnyFlags( RF_ClassDefaultObject ) )
	{
		STAT(FScopeSecondsCounter PrepareForSavingTextureTimeCounter(PrepareForSavingTextureTime));

		INT		NumMips				= Texture2D->Mips.Num();								
		INT		FirstMipIndex		= Clamp<INT>( PlatformLODSettings.CalculateLODBias( Texture2D ), 0, NumMips-1 );
		UBOOL	bIsStreamingTexture	= !Texture2D->NeverStream;

		// make sure we load at least the first packed mip level
		FirstMipIndex = Min(FirstMipIndex, Texture2D->MipTailBaseIdx);

		// Textures residing in seekfree packages cannot be streamed as the file offset is not known till after
		// they are saved.
		if( (bIsSavedInSeekFreePackage && Texture2D->IsIn( Package ))
		// Cubemap faces cannot be streamed.
		||	Texture2D->GetOuter()->IsA(UTextureCube::StaticClass()) )
		{
			bIsStreamingTexture		= FALSE;
			Texture2D->NeverStream	= TRUE;
		}

		for( INT MipIndex=NumMips-1; MipIndex>=0; MipIndex-- )
		{	
			FTexture2DMipMap& Mip = Texture2D->Mips(MipIndex);
			
			UBOOL bCutoffMipLevel = FALSE;
			UBOOL bUnusedMipLevel = FALSE;

			// We still require MinStreamedInMips to be present in the seek free package as the streaming code 
			// assumes those to be always loaded.
			if( bIsSavedInSeekFreePackage
			&&	bIsStreamingTexture
			// @todo streaming, @todo cooking: This assumes the same value of GMinTextureResidentMipCount across platforms.
			&&	(MipIndex < NumMips - GMinTextureResidentMipCount)
			// Miptail must always be loaded.
			&&	MipIndex < Texture2D->MipTailBaseIdx )
			{
				bCutoffMipLevel = TRUE;
			}

			// Cut off miplevels that are never going to be used. This requires a FULL recook every time the texture
			// resolution is increased.
			if( MipIndex < FirstMipIndex )
			{
				bUnusedMipLevel	= TRUE;
			}

			// Cut off miplevels that are already packed in the textures miptail level 
			// since they're all stored in the base miptail level
			if( MipIndex > Texture2D->MipTailBaseIdx )
			{
				bUnusedMipLevel	= TRUE;
			}

			// skip mip levels that are not being used	
			if( bUnusedMipLevel )
			{
				// bulk data is not saved 
				Mip.Data.StoreInSeparateFile( 
					TRUE,
					BULKDATA_None|BULKDATA_Unused,
					0,
					INDEX_NONE,
					INDEX_NONE );
			}
			// Cut off miplevels that are being streamed.
			else if( bCutoffMipLevel )
			{
				// Retrieve offset and size of payload in separate file.
				FString					BulkDataName = *FString::Printf(TEXT("MipLevel_%i"),MipIndex);
				FCookedBulkDataInfo*	BulkDataInfo = CookedBulkDataInfoContainer->RetrieveInfo( Texture2D, *BulkDataName );

				if( BulkDataInfo != NULL )
				{
                    // use previous bulk data info for setting info
					Mip.Data.StoreInSeparateFile( 
						TRUE,
						BulkDataInfo->SavedBulkDataFlags,
						BulkDataInfo->SavedElementCount,
						BulkDataInfo->SavedBulkDataOffsetInFile,
						BulkDataInfo->SavedBulkDataSizeOnDisk );
				}
				else
				{
					appErrorf( TEXT("Couldn't find seek free bulk data info: %s for %s\nRecook with -full"), *BulkDataName, *Texture2D->GetFullName() );
				}
				
			}
			// We're not cutting off miplevels as we're either a regular package or the texture is not streamable.
			else
			{
				Mip.Data.StoreInSeparateFile( FALSE );
			}

			// Miplevels that are saved into the map/ seekfree package shouldn't be individually compressed as we're not going
			// to stream out of them and it just makes serialization slower. In the long run whole package compression is going
			// to take care of getting the file size down.
#if DEBUG_DISTRIBUTED_COOKING
			if ( TRUE )
#else
			if( (bIsSavedInSeekFreePackage && !bCutoffMipLevel) || Platform == PLATFORM_PS3 )
#endif
			{
				Mip.Data.StoreCompressedOnDisk( COMPRESS_None );
			}
			// Store mips compressed in regular packages so we can stream compressed from disk.
			else
			{
				Mip.Data.StoreCompressedOnDisk( MIP_COMPRESSION_METHOD );
			}
		}
	}

	ATerrain* Terrain = Cast<ATerrain>(Object);
	if (Terrain)
	{
		STAT(FScopeSecondsCounter PrepareForSavingTerrainTimeCounter(PrepareForSavingTerrainTime));
		if (Terrain->CachedMaterials != NULL)
		{
			// Make sure materials are compiled for the platform and add them to the shader cache embedded into seekfree packages.
			for (INT CachedMatIndex = 0; CachedMatIndex < Terrain->CachedMaterialCount; CachedMatIndex++)
			{
				FTerrainMaterialResource* TMatRes = Terrain->CachedMaterials[CachedMatIndex];
				if (TMatRes && ShaderCache)
				{
					// Compile the material...
					TRefCountPtr<FMaterialShaderMap> MaterialShaderMapRef;
					FStaticParameterSet EmptySet(TMatRes->GetId());
					if (TMatRes->Compile(&EmptySet, ShaderPlatform, MaterialShaderMapRef, FALSE))
					{
						check(MaterialShaderMapRef);
						// ... and add it to the shader cache being saved into the seekfree package.
						ShaderCache->AddMaterialShaderMap(MaterialShaderMapRef);
					}
				}
			}
		}
	}

	// Compile shaders for materials
	UMaterial* Material = Cast<UMaterial>(Object);
	if( Material ) 
	{
		STAT(FScopeSecondsCounter PrepareForSavingMaterialTimeCounter(PrepareForSavingMaterialTime));
		CompileMaterialShaders( Material );
	}

	// Compile shaders for material instances with static parameters
	UMaterialInstance* MaterialInterface = Cast<UMaterialInstance>(Object);
	if( MaterialInterface ) 
	{
		STAT(FScopeSecondsCounter PrepareForSavingMaterialInterfaceTimeCounter(PrepareForSavingMaterialInterfaceTime));
		CompileMaterialInstanceShaders( MaterialInterface );
	}

	// 'Cook-out' material expressions on consoles
	// In this case, simply don't load them on the client
	// Keep 'parameter' types around so that we can get their defaults.
	if (GCookingTarget & PLATFORM_Console)
	{
		UMaterialExpression* MatExp = Cast<UMaterialExpression>(Object);
		if (MatExp)
		{
			if( MatExp->IsTemplate() == FALSE && !MatExp->bIsParameterExpression )
			{
				debugfSuppressed(NAME_DevCooking, TEXT("Cooking out material expression %s (%s)"), 
					*(MatExp->GetName()), MatExp->GetOuter() ? *(MatExp->GetOuter()->GetName()) : TEXT("????"));
				MatExp->SetFlags(RF_NotForClient|RF_NotForServer|RF_NotForEdit);
			}
		}
	}
	
}

/**
 * Setup the commandlet's platform setting based on commandlet params
 * @param Params The commandline parameters to the commandlet - should include "platform=xxx"
 */
UBOOL UCookPackagesCommandlet::SetPlatform(const FString& Params)
{
	// default to success
	UBOOL Ret = TRUE;

	FString PlatformStr;
	if (Parse(*Params, TEXT("PLATFORM="), PlatformStr))
	{
		if (PlatformStr == TEXT("PS3"))
		{
			Platform = PLATFORM_PS3;
			ShaderPlatform = SP_PS3;
			GCreationFlags = TextureCreate_NoPackedMip;
		}
		else if (PlatformStr == TEXT("xenon") || PlatformStr == TEXT("xbox360"))
		{	
			Platform = PLATFORM_Xenon;
			ShaderPlatform = SP_XBOXD3D;
			GCreationFlags = 0;
		}
		else if (PlatformStr == TEXT("pc") || PlatformStr == TEXT("win32"))
		{
			Platform = PLATFORM_Windows;
			ShaderPlatform = SP_PCD3D_SM3;
			GCreationFlags = TextureCreate_NoPackedMip;
		}
		else
		{
			SET_WARN_COLOR(COLOR_RED);
			warnf(NAME_Error, TEXT("Unknown platform! No shader platform exists for this platform yet!"));
			CLEAR_WARN_COLOR();

			// this is a failure
			Ret = FALSE;
		}
	}
	else
	{
		Ret = FALSE;
	}

	return Ret;
}

/**
 * Tried to load the DLLs and bind entry points.
 *
 * @return	TRUE if successful, FALSE otherwise
 */
UBOOL UCookPackagesCommandlet::BindDLLs()
{
	if( Platform == PLATFORM_PS3 || Platform == PLATFORM_Xenon )
	{
		// Load in the console support containers
		GConsoleSupportContainer->LoadAllConsoleSupportModules();

		// Try to load XeToolsDll if loading xbdm.dll succeeded.
		FString PlatformToolsDllFileName = FString(appBaseDir()) * GetPlatformString() * GetPlatformPrefix() + TEXT("Tools.dll");
		PlatformToolsDllHandle = appGetDllHandle(*PlatformToolsDllFileName);
		if (!PlatformToolsDllHandle)
		{
			appDebugMessagef(TEXT("Couldn't bind to %s."), *PlatformToolsDllFileName);
			return FALSE;
		}

		// Bind function entry points.
		CreateTextureCooker			= (FuncCreateTextureCooker)			appGetDllExport( PlatformToolsDllHandle, TEXT("CreateTextureCooker"		) );
		DestroyTextureCooker		= (FuncDestroyTextureCooker)		appGetDllExport( PlatformToolsDllHandle, TEXT("DestroyTextureCooker"		) );
		CreateSoundCooker			= (FuncCreateSoundCooker)			appGetDllExport( PlatformToolsDllHandle, TEXT("CreateSoundCooker"			) );
		DestroySoundCooker			= (FuncDestroySoundCooker)			appGetDllExport( PlatformToolsDllHandle, TEXT("DestroySoundCooker"		) );
		CreateSkeletalMeshCooker	= (FuncCreateSkeletalMeshCooker)	appGetDllExport( PlatformToolsDllHandle, TEXT("CreateSkeletalMeshCooker"		) );
		DestroySkeletalMeshCooker	= (FuncDestroySkeletalMeshCooker)	appGetDllExport( PlatformToolsDllHandle, TEXT("DestroySkeletalMeshCooker"		) );
		CreateStaticMeshCooker		=	(FuncCreateStaticMeshCooker)	appGetDllExport( PlatformToolsDllHandle, TEXT("CreateStaticMeshCooker"		) );
		DestroyStaticMeshCooker		=	(FuncDestroyStaticMeshCooker)	appGetDllExport( PlatformToolsDllHandle, TEXT("DestroyStaticMeshCooker"		) );
		CreateShaderPrecompiler		= (FuncCreateShaderPrecompiler)		appGetDllExport( PlatformToolsDllHandle, TEXT("CreateShaderPrecompiler"	) );
		DestroyShaderPrecompiler	= (FuncDestroyShaderPrecompiler)	appGetDllExport( PlatformToolsDllHandle, TEXT("DestroyShaderPrecompiler"	) );

		// Clean up and abort on failure.
		if( !CreateTextureCooker 
		||	!DestroyTextureCooker 
		||	!CreateSkeletalMeshCooker
		||	!DestroySkeletalMeshCooker
		||	!CreateStaticMeshCooker
		||	!DestroyStaticMeshCooker
		||	!CreateSoundCooker 
		||	!DestroySoundCooker 
		||	!CreateShaderPrecompiler
		||	!DestroyShaderPrecompiler )
		{
			appFreeDllHandle( PlatformToolsDllHandle );
			appDebugMessagef(TEXT("Couldn't bind function entry points."));
			return FALSE;
		}

		// Create the platform specific texture and sound cookers and shader compiler.
		TextureCooker		= CreateTextureCooker();
		SoundCooker			= CreateSoundCooker();

		SkeletalMeshCooker	= CreateSkeletalMeshCooker();
		StaticMeshCooker	= CreateStaticMeshCooker();

		check(!GConsoleShaderPrecompilers[ShaderPlatform]);
		GConsoleShaderPrecompilers[ShaderPlatform] = CreateShaderPrecompiler();
		
		// Clean up and abort on failure.
		if( !TextureCooker 
		||	!SoundCooker 
		||	!SkeletalMeshCooker
		||	!StaticMeshCooker
		||	!GConsoleShaderPrecompilers[ShaderPlatform] )
		{
			appFreeDllHandle( PlatformToolsDllHandle );
			appDebugMessagef(TEXT("Couldn't create texture cookers."));
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Precreate all the .ini files that the console will use at runtime
 */
void UCookPackagesCommandlet::CreateIniFiles()
{
	if( Platform == PLATFORM_PS3 || Platform == PLATFORM_Xenon )
	{
		// make sure the output config directory exists
		GFileManager->MakeDirectory(*(appGameConfigDir() + GetConfigOutputDirectory()));

		// force unattended for ini generation so it doesn't prompt
		UBOOL bWasUnattended = GIsUnattended;
		GIsUnattended = TRUE;

		UINT YesNoToAll = ART_No;

		// assemble standard ini files
		appCreateIniNamesAndThenCheckForOutdatedness(
			PlatformEngineConfigFilename, 
			NULL, NULL, 
			TEXT("Engine.ini"), 
			*GetPlatformDefaultIniPrefix(),
			*GetPlatformConfigOutputPrefix(),
			YesNoToAll);

		TCHAR PlatformGameIni[1024] = TEXT("");
		appCreateIniNamesAndThenCheckForOutdatedness(
			PlatformGameIni, 
			NULL, NULL, 
			TEXT("Game.ini"), 
			*GetPlatformDefaultIniPrefix(),
			*GetPlatformConfigOutputPrefix(),
			YesNoToAll);

		TCHAR PlatformInputIni[1024] = TEXT("");
		appCreateIniNamesAndThenCheckForOutdatedness(
			PlatformInputIni, 
			NULL, NULL, 
			TEXT("Input.ini"), 
			*GetPlatformDefaultIniPrefix(),
			*GetPlatformConfigOutputPrefix(),
			YesNoToAll);

		TCHAR PlatformUIIni[1024] = TEXT("");
		appCreateIniNamesAndThenCheckForOutdatedness(
			PlatformUIIni, 
			NULL, NULL, 
			TEXT("UI.ini"), 
			*GetPlatformDefaultIniPrefix(),
			*GetPlatformConfigOutputPrefix(),
			YesNoToAll);

		// get all files in the config dir
		TArray<FString> IniFiles;
		appFindFilesInDirectory(IniFiles, *appGameConfigDir(), FALSE, TRUE);

		// go over ini files and generate non-default ini files from default inis
		for (INT FileIndex = 0; FileIndex < IniFiles.Num(); FileIndex++)
		{
			// the ini path/file name
			FFilename IniPath = IniFiles(FileIndex);
			FString IniFilename = IniPath.GetCleanFilename();

			// we want to process all the Default*.ini files
			if (IniFilename.Left(7) == TEXT("Default"))
			{
				// generate ini files
				TCHAR GeneratedIniName[1024] = TEXT("");
				appCreateIniNamesAndThenCheckForOutdatedness(
					GeneratedIniName, 
					NULL, NULL,
					*IniFilename.Right(IniFilename.Len() - 7),
					TEXT("Default"),
					*GetConfigOutputPrefix(),
					YesNoToAll);

			}
		}

		// restore unattendedness
		GIsUnattended = bWasUnattended;

		// coalesce the ini and int files
		GConfig->CoalesceFilesFromDisk(*(appGameConfigDir() + GetConfigOutputDirectory()), ShouldByteSwapData(), PlatformEngineConfigFilename);

		// mark the coalesced ini file for sha
		FilesForSHA.AddItem(*FString::Printf(TEXT("%s%s%sCoalesced.ini"), *appGameConfigDir(), *GetConfigOutputDirectory(), PATH_SEPARATOR));
		for (INT LangIndex = 0; GKnownLanguageExtensions[LangIndex]; LangIndex++)
		{
			const TCHAR* Ext = GKnownLanguageExtensions[LangIndex];
			// mark each coalesced loc file for sha
			FilesForSHA.AddItem(*FString::Printf(TEXT("%sLocalization%sCoalesced.%s"), *appGameDir(), PATH_SEPARATOR, Ext));
		}
	}
	// Use the current engine ini file on the PC.
	else
	{
		appStrcpy( PlatformEngineConfigFilename, GEngineIni );
	}
}

/**
 * Load all packages in a specified ini section with the Package= key
 * @param SectionName Name of the .ini section ([Engine.PackagesToAlwaysCook])
 * @param PackageNames Paths of the loaded packages
 * @param KeyName Optional name for the key of the list to load (defaults to "Package")
 * @param bShouldSkipLoading If TRUE, this function will only fill out PackageNames, and not load the package
 */
void UCookPackagesCommandlet::LoadSectionPackages(const TCHAR* SectionName, TArray<FString>& PackageNames, const TCHAR* KeyName, UBOOL bShouldSkipLoading)
{
	// here we need to look in the .ini to see which packages to load
	TMultiMap<FString,FString>* PackagesToLoad = GConfig->GetSectionPrivate( SectionName, 0, 1, PlatformEngineConfigFilename );
	if (PackagesToLoad)
	{
		STAT(FScopeSecondsCounter LoadSectionPackagesTimeCounter(LoadSectionPackagesTime));
		for( TMultiMap<FString,FString>::TIterator It(*PackagesToLoad); It; ++It )
		{
			if (It.Key() == KeyName)
			{
				FString& PackageName = It.Value();
				FString PackageFileName;
				if( GPackageFileCache->FindPackageFile( *PackageName, NULL, PackageFileName ) == TRUE )
				{
					PackageNames.AddItem( FString(*PackageName).ToUpper() );
					if (!bShouldSkipLoading)
					{
						warnf( NAME_Log, TEXT("Force loading:  %s"), *PackageName );
						LoadPackage( NULL, *PackageFileName, LOAD_None );
					}
				}
			}
		}
	}
}

/**
 * Performs command line and engine specific initialization.
 *
 * @param	Params	command line
 * @return	TRUE if successful, FALSE otherwise
 */
UBOOL UCookPackagesCommandlet::Init( const TCHAR* Params )
{
	// Parse command line args.
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine( Params, Tokens, Switches );

	// Set the "we're cooking" flags.
	GIsCooking				= TRUE;
	GCookingTarget 			= Platform;

	// Create folder for cooked data.
	CookedDir = appGameDir() + GetCookedDirectory() + PATH_SEPARATOR;
	if( !GFileManager->MakeDirectory( *CookedDir ) )
	{
		appDebugMessagef(TEXT("Couldn't create %s"), *CookedDir);
		return FALSE;
	}

	// create the ini files for the target platform.
	CreateIniFiles();

	// Initialize LOD settings from the patform's engine ini.
	PlatformLODSettings.Initialize(PlatformEngineConfigFilename, TEXT("TextureLODSettings"));
	// Initialize global shadow volume setting from Xenon ini
	GConfig->GetBool( TEXT("Engine.Engine"), TEXT("AllowShadowVolumes"), GAllowShadowVolumes, PlatformEngineConfigFilename );

	bSeparateSharedMPResources = FALSE;
	GConfig->GetBool( TEXT("Engine.Engine"), TEXT("bCookSeparateSharedMPResources"), bSeparateSharedMPResources, GEngineIni );
	if ( bSeparateSharedMPResources )
	{
		warnf( NAME_Log, TEXT("Saw bCookSeparateSharedMPResources flag.") );
	}

	// Look for -SKIPMAPS command line switch.
	bSkipCookingMaps			= Switches.FindItemIndex(TEXT("SKIPMAPS")) != INDEX_NONE;
	// Look for -ALWAYSRECOOKMAPS command line switch.
	bAlwaysRecookMaps			= Switches.FindItemIndex(TEXT("ALWAYSRECOOKMAPS")) != INDEX_NONE;
	// Look for -ALWAYSRECOOKSCRIPT command line switch.
	bAlwaysRecookScript			= Switches.FindItemIndex(TEXT("ALWAYSRECOOKSCRIPT")) != INDEX_NONE;
	// Look for -INISONLY command line switch
	bIniFilesOnly				= Switches.FindItemIndex(TEXT("INISONLY")) != INDEX_NONE;
	// Check if we are only doing ini files
	bGenerateSHAHashes			= Switches.FindItemIndex(TEXT("SHA")) != INDEX_NONE;
	// Skip saving maps if SKIPSAVINGMAPS is specified, useful for LOC cooking.
	bSkipSavingMaps				= Switches.FindItemIndex(TEXT("SKIPSAVINGMAPS")) != INDEX_NONE;
	// Skip loading & saving packages not required for cooking process to speed up LOC cooking.
	bSkipNotRequiredPackages	= Switches.FindItemIndex(TEXT("SKIPNOTREQUIREDPACKAGES")) != INDEX_NONE;
	// Check for a flag to cook all packages, not just dependencies
	bCookAllNonMapPackages		= Switches.FindItemIndex(TEXT("COOKALLNONMAPPACKAGES")) != INDEX_NONE;

	// determine whether we should cook-out StaticMeshActors
	GConfig->GetBool(TEXT("Engine.StaticMeshCollectionActor"), TEXT("bCookOutStaticMeshActors"), bCookOutStaticMeshActors, PlatformEngineConfigFilename);
	bCookOutStaticMeshActors = (bCookOutStaticMeshActors || Switches.ContainsItem(TEXT("RemoveStaticMeshActors"))) && !Switches.ContainsItem(TEXT("KeepStaticMeshActors"));
	if ( bCookOutStaticMeshActors )
	{
		warnf(NAME_Log, TEXT("StaticMeshActors will be removed from cooked files."));
	}
	else
	{
		warnf(NAME_Log, TEXT("StaticMeshActors will NOT be removed from cooked files."));
	}

	// determine whether we should cook-out static Lights
	GConfig->GetBool(TEXT("Engine.StaticLightCollectionActor"), TEXT("bCookOutStaticLightActors"), bCookOutStaticLightActors, PlatformEngineConfigFilename);
	bCookOutStaticLightActors = (bCookOutStaticLightActors || Switches.ContainsItem(TEXT("RemoveStaticLights"))) && !Switches.ContainsItem(TEXT("KeepStaticLights"));
	if ( bCookOutStaticLightActors )
	{
		warnf(NAME_Log, TEXT("Static Light actors will be removed from cooked files."));
	}
	else
	{
		warnf(NAME_Log, TEXT("Static Light actors will NOT be removed from cooked files."));
	}

	if (bIniFilesOnly)
	{
		warnf(NAME_Log, TEXT("Cooked .ini and localization files."));
		// return false so further processing is not done
		return TRUE;
	}

	if (Platform == PLATFORM_Xenon)
	{
		// Change physics to cook content for Xenon target.
		SetPhysCookingXenon();
	}
	else if (Platform == PLATFORM_PS3)
	{
		// Change physics to cook content for PS3 target.
		SetPhysCookingPS3();

		// set the compression chunk size to 64k to fit in SPU local ram
		extern INT GSavingCompressionChunkSize;
		GSavingCompressionChunkSize = 64 * 1024;
	}

	// Check for -FULL command line switch.
	if( Switches.FindItemIndex(TEXT("FULL")) != INDEX_NONE )
	{
		// if we have passed in -FULL we want to cook all packages
		// we want to make certain there are no outdated packages in existence
		// so we delete all of the files in the CookedDir!
		TArray<FString> AllFiles; 
		appFindFilesInDirectory(AllFiles, *CookedDir, TRUE, TRUE);

		// delete em all
		for (INT FileIndex = 0; FileIndex < AllFiles.Num(); FileIndex++)
		{
			warnf(NAME_Log, TEXT("Deleting: %s"), *AllFiles(FileIndex));
			if (!GFileManager->Delete(*AllFiles(FileIndex)))
			{
				appDebugMessagef(TEXT("Couldn't delete %s"), *AllFiles(FileIndex));
				return FALSE;
			}
		}
	}

	// recompile the global shaders if we need to
	if( Switches.FindItemIndex(TEXT("RECOMPILEGLOBALSHADERS")) != INDEX_NONE )
	{
		extern TShaderMap<FGlobalShaderType>* GGlobalShaderMap[SP_NumPlatforms];	
		delete GGlobalShaderMap[ShaderPlatform];
		GGlobalShaderMap[ShaderPlatform] = NULL;
	}

	// Keep track of time spent loading packages.
	DOUBLE StartTime = appSeconds();

	{
		STAT(FScopeSecondsCounter LoadNativePackagesTimeCounter(LoadNativePackagesTime));
		// Load up all native script files, not excluding game specific ones.
		LoadAllNativeScriptPackages( FALSE );
	}
		
	// Make sure that none of the loaded script packages get garbage collected.
	for( FObjectIterator It; It; ++It )
	{
		UObject* Object = *It;
		// Check for code packages and add them and their contents to root set.
		if( Object->GetOutermost()->PackageFlags & PKG_ContainsScript )
		{
			Object->AddToRoot();
		}
	}

	// find the group of packages that may need to always be up-to-date, as nothing else references them
	LoadSectionPackages(TEXT("Engine.PackagesToAlwaysCook"), RequiredPackages);
	LoadSectionPackages(TEXT("Engine.PackagesToAlwaysCook"), RequiredPackages, TEXT("SeekFreePackage"));
	LoadSectionPackages(TEXT("Engine.StartupPackages"), RequiredPackages);

	// read in the per-map packages to cook
	TMap<FName, TArray<FName> > PerMapCookPackages;
	GConfig->Parse1ToNSectionOfNames(TEXT("Engine.PackagesToForceCookPerMap"), TEXT("Map"), TEXT("Package"), PerMapCookPackages, PlatformEngineConfigFilename);

	// Iterate over all objects and mark them to be excluded from being put into the seek free package.
	for( FObjectIterator It; It; ++It )
	{
		UObject*		Object				= *It;
		UPackage*		Package				= Cast<UPackage>(Object);
		ULinkerLoad*	LinkerLoad			= Object->GetLinker();
		
		// Toplevel packages don't have a linker so we try to find it unless we know it's not a package or it is
		// the transient package.
		if( !LinkerLoad && Package && Package != UObject::GetTransientPackage() )
		{
			STAT(FScopeSecondsCounter LoadPackagesTimeCounter(LoadPackagesTime));
			UObject::BeginLoad();
			LinkerLoad = UObject::GetPackageLinker( Package, NULL, LOAD_NoWarn, NULL, NULL );
			UObject::EndLoad();
		}

		// Mark objects that reside in a code package, are native, a default object or transient to not be put 
		// into the seek free package.
		if( (LinkerLoad && LinkerLoad->ContainsCode()) || Object->HasAnyFlags(RF_Transient|RF_Native|RF_ClassDefaultObject))
		{
			Object->SetFlags( RF_MarkedByCooker );
		}
	}

	// Check whether we only want to cook passed in packages and their dependencies, unless
	// it was specified to cook all packages
	bOnlyCookDependencies = Tokens.Num() > 0;
	if( bOnlyCookDependencies )
	{
		STAT(FScopeSecondsCounter LoadDependenciesTimeCounter(LoadDependenciesTime));

		// Iterate over all passed in packages, loading them.
		for( INT TokenIndex=0; TokenIndex<Tokens.Num(); TokenIndex++ )
		{
			// Load package if found.
			FString PackageFilename;
			UPackage* Result = NULL;
			if( GPackageFileCache->FindPackageFile( *Tokens(TokenIndex), NULL, PackageFilename ) )
			{
				warnf(NAME_Log, TEXT("Loading base level %s"), *PackageFilename);
				Result = LoadPackage(NULL, *PackageFilename, LOAD_None);

				// add dependencies for the per-map packages for this map (if any)
				TArray<FName>* Packages = PerMapCookPackages.Find(Result->GetFName());
				if (Packages != NULL)
				{
					for (INT PackageIndex = 0; PackageIndex < Packages->Num(); PackageIndex++)
					{
						FName PackageName = (*Packages)(PackageIndex);
						PackageDependencies.Set(*PackageName.ToString(), 0);
					}
				}
			}
			if (Result == NULL)
			{
				warnf(NAME_Error, TEXT("Failed to load base level %s"), *Tokens(TokenIndex));
				return FALSE;
			}
		}

		TArray<FString> SubLevelFilenames;

		// Iterate over all UWorld objects and load the referenced levels.
		for( TObjectIterator<UWorld> It; It; ++It )
		{
			UWorld*		World		= *It;
			AWorldInfo* WorldInfo	= World->GetWorldInfo();
			// Iterate over streaming level objects loading the levels.
			for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
			{
				ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels(LevelIndex);
				if( StreamingLevel )
				{
					// Load package if found.
					FString PackageFilename;
					if( GPackageFileCache->FindPackageFile( *StreamingLevel->PackageName.ToString(), NULL, PackageFilename ) )
					{
						SubLevelFilenames.AddItem(*PackageFilename);
					}
				}
			}
		}

		for (INT SubLevelIndex = 0; SubLevelIndex < SubLevelFilenames.Num(); SubLevelIndex++)
		{
			warnf(NAME_Log, TEXT("Loading sub-level %s"), *SubLevelFilenames(SubLevelIndex));
			LoadPackage( NULL, *SubLevelFilenames(SubLevelIndex), LOAD_None );
		}

		// Iterate over all objects and add the filename of their linker (if existing) to the dependencies map.
		for( FObjectIterator It; It; ++It )
		{
			UObject* Object = *It;
			if( Object->GetLinker() )
			{
				// We need to (potentially) cook this package.
				PackageDependencies.Set( *Object->GetLinker()->Filename, 0 );
			}
		}
	}

	// Create container helper object for keeping track of where bulk data ends up inside cooked packages. This is needed so we can fix
	// them up in the case of forced exports that don't need their bulk data inside the package they were forced into.
	CookedBulkDataInfoContainer = UCookedBulkDataInfoContainer::CreateInstance( *(CookedDir + TEXT("GlobalCookedBulkDataInfos.upk")) );
	check( CookedBulkDataInfoContainer );

	// Compile all global shaders.
	VerifyGlobalShaders( ShaderPlatform );

	return TRUE;
}

/**
 * Generates list of src/ dst filename mappings of packages that need to be cooked after taking the command
 * line options into account.
 *
 * @param [out] FirstRequiredIndex		index of first non-startup required package in returned array, untouched if there are none
 * @param [out] FirstStartupIndex		index of first startup package in returned array, untouched if there are none
 * @param [out]	FirstScriptIndex		index of first script package in returned array, untouched if there are none
 * @param [out] FirstGameScriptIndex	index of first game script package in returned array, untouched if there are none
 * @param [out] FirstMapIndex			index of first map package in returned array, untouched if there are none
 * @param [out] FirstMPMapIndex			index of first map package in returned array, untouched if there are none
 *
 * @return	array of src/ dst filename mappings for packages that need to be cooked
 */
TArray<UCookPackagesCommandlet::FPackageCookerInfo> UCookPackagesCommandlet::GeneratePackageList( INT& FirstRequiredIndex, INT& FirstStartupIndex, INT& FirstScriptIndex, INT& FirstGameScriptIndex, INT& FirstMapIndex, INT& FirstMPMapIndex )
{
	// Split into two to allow easy sorting via array concatenation at the end.
	TArray<FPackageCookerInfo>	NotRequiredFilenamePairs;
	TArray<FPackageCookerInfo>	RegularFilenamePairs;
	TArray<FPackageCookerInfo>	MapFilenamePairs;
	// Maintain a separate list of multiplayer maps.
	TArray<FPackageCookerInfo>	MPMapFilenamePairs;
	TArray<FPackageCookerInfo>	ScriptFilenamePairs;
	TArray<FPackageCookerInfo>	StartupFilenamePairs;
	TArray<FPackageCookerInfo>	StandaloneSeekfreeFilenamePairs;


	// Get a list of all script package names split by type, excluding editor- only ones.
	TArray<FString>				EngineScriptPackageNames;
	TArray<FString>				GameNativeScriptPackageNames;
	TArray<FString>				GameScriptPackageNames;
	appGetEngineScriptPackageNames( EngineScriptPackageNames, Platform == PLATFORM_Windows );
	appGetGameNativeScriptPackageNames( GameNativeScriptPackageNames, Platform == PLATFORM_Windows );
	appGetGameScriptPackageNames( GameScriptPackageNames, Platform == PLATFORM_Windows );

	// Get combined list of all script package names, including editor- only ones.
	TArray<FString>				AllScriptPackageNames;
	appGetEngineScriptPackageNames( AllScriptPackageNames, TRUE );
	appGetGameNativeScriptPackageNames( AllScriptPackageNames, TRUE );
	appGetGameScriptPackageNames( AllScriptPackageNames, TRUE );

	TArray<FString>				AllStartupPackageNames;
	// get all the startup packages that the runtime will use (so we need to pass it the platform-specific engine config name)
	appGetAllPotentialStartupPackageNames(AllStartupPackageNames, PlatformEngineConfigFilename);

	// get a list of the seekfree, always cook maps, but don't reload them
	TArray<FString>				SeekFreeAlwaysCookMaps;
	LoadSectionPackages(TEXT("Engine.PackagesToAlwaysCook"), SeekFreeAlwaysCookMaps, TEXT("SeekFreePackage"), TRUE);
	
	TArray<FString> PackageList = GPackageFileCache->GetPackageFileList();
	for( INT PackageIndex = 0; PackageIndex < PackageList.Num(); PackageIndex++ )
	{
		// We store cooked packages in a folder that is outside the Package search path and use an "unregistered" extension.
		const FFilename&	SrcFilename	= PackageList(PackageIndex);
		FFilename			BaseSrcFilename	= SrcFilename.GetBaseFilename();

		// Dest pathname depends on platform
		FFilename			DstFilename;
		if (Platform == PLATFORM_Windows)
		{
			// get top level directory under appGameDir of the source (..\ExampleGame\Content\)
			FFilename AfterGameDir = SrcFilename.Right(SrcFilename.Len() - appGameDir().Len());
			FFilename AfterTopLevelDir = AfterGameDir.Right(AfterGameDir.Len() - AfterGameDir.InStr(PATH_SEPARATOR));
			DstFilename = CookedDir + AfterTopLevelDir;
		}
		else
		{
			DstFilename = CookedDir + SrcFilename.GetBaseFilename() + TEXT(".xxx");
		}


		// Check whether we are autosave, PIE, a manual dependency or a map file.
		UBOOL	bIsShaderCacheFile		= FString(*SrcFilename).ToUpper().InStr( TEXT("SHADERCACHE") ) != INDEX_NONE;
		UBOOL	bIsAutoSave				= FString(*SrcFilename).ToUpper().InStr( TEXT("AUTOSAVES") ) != INDEX_NONE;
		UBOOL	bIsPIE					= bIsAutoSave && FString(*SrcFilename).InStr( PLAYWORLD_CONSOLE_BASE_PACKAGE_PREFIX ) != INDEX_NONE;
		UBOOL	bIsADependency			= PackageDependencies.Find( SrcFilename ) != NULL;
		UBOOL	bIsEngineScriptFile		= EngineScriptPackageNames.FindItemIndex( BaseSrcFilename ) != INDEX_NONE;
		UBOOL	bIsGameNativeScriptFile	= GameNativeScriptPackageNames.FindItemIndex( BaseSrcFilename ) != INDEX_NONE;
		UBOOL	bIsNativeScriptFile		= bIsEngineScriptFile || bIsGameNativeScriptFile;
		UBOOL	bIsNonNativeScriptFile	= !bIsGameNativeScriptFile && GameScriptPackageNames.FindItemIndex( BaseSrcFilename ) != INDEX_NONE;
		UBOOL	bIsEditorScriptFile		= !(bIsNativeScriptFile || bIsNonNativeScriptFile) && AllScriptPackageNames.FindItemIndex( BaseSrcFilename ) != INDEX_NONE;
		UBOOL	bIsMapFile				= (SrcFilename.GetExtension() == FURL::DefaultMapExt) || (BaseSrcFilename == TEXT("entry"));
		UBOOL	bIsStartupPackage		= AllStartupPackageNames.FindItemIndex( BaseSrcFilename ) != INDEX_NONE;
		UBOOL   bIsScriptFile           = bIsNativeScriptFile || bIsNonNativeScriptFile ||  bIsEditorScriptFile;
		UBOOL	bIsCombinedStartupPackage = bIsStartupPackage && !bIsMapFile && !bIsScriptFile;
		UBOOL	bIsSeekfreeAlwaysCookPackage = SeekFreeAlwaysCookMaps.FindItemIndex(BaseSrcFilename) != INDEX_NONE;

		// Compare file times.
		DOUBLE	DstFileAge				= GFileManager->GetFileAgeSeconds( *DstFilename );
		DOUBLE	SrcFileAge				= GFileManager->GetFileAgeSeconds( *SrcFilename );
		UBOOL	DstFileExists			= GFileManager->FileSize( *DstFilename ) > 0;
		UBOOL	DstFileNewer			= DstFileExists && (DstFileAge < SrcFileAge) && !bIsScriptFile;

		// It would be nice if we could just rely on the map extension to be set but in theory we would have to load up the package
		// and see whether it contained a UWorld object to be sure. Checking the map extension with a FindObject should give us
		// very good coverage and should always work in the case of dependency cooking. In theory we could also use LoadObject albeit
		// at the cost of having to create linkers for all existing packages.
		if( FindObject<UWorld>( NULL, *FString::Printf(TEXT("%s.TheWorld"),*BaseSrcFilename) ) )
		{
			bIsMapFile = TRUE;
		}
		// Special case handling for Entry.upk.
		if( BaseSrcFilename == TEXT("Entry") )
		{
			bIsMapFile = TRUE;
		}

		// Skip over shader caches.
		if( bIsShaderCacheFile )
		{
			warnf(NAME_Log, TEXT("Skipping shader cache %s"), *SrcFilename);
			continue;
		}
		// Skip over packages we don't depend on if we're only cooking dependencies .
		else if( bOnlyCookDependencies && !bIsADependency )
		{
			// Actually, if we don't want to skip non-map packages, don't
			if (!bCookAllNonMapPackages || bIsMapFile)
			{
				//warnf(NAME_Log, TEXT("Skipping %s"), *SrcFilename);
				continue;
			}
		}
		// Skip cooking maps if wanted.
		else if( bSkipCookingMaps && bIsMapFile )
		{
			//warnf(NAME_Log, TEXT("Skipping %s"), *SrcFilename);
			continue;
		}
		// Skip over autosaves, unless it is a PlayInEditor file.
		else if( bIsAutoSave && !bIsPIE )
		{
			warnf(NAME_Log, TEXT("Disregarding %s"), *SrcFilename);
			continue;
		}
		// Skip over script packages not containing native classes as all they are going to be duplicated into the map.
		else if( bIsNonNativeScriptFile && !bCookAllNonMapPackages )
		{
			continue;
		}
		// Skip over editor script packages.
		else if( bIsEditorScriptFile )
		{
			continue;
		}

		// Load the potentially existing destination file to see whether it needs to be recooked because
		// the CookedContentVersion has been increased.
		UBOOL bCookedVersionIsOutDated = FALSE;
		if( DstFileExists == TRUE )
		{
			STAT(FScopeSecondsCounter LoadPackagesTimeCounter(LoadPackagesTime));
			// Create a dummy package for the already existing cooked package.
			UPackage* Package = UObject::CreatePackage( NULL, *FString::Printf( TEXT( "CookedLinker%d"), PackageIndex ) );
			// GetPackageLinker will find already existing packages first and we cannot reset loaders at this point
			// so we revert to manually creating the linker load ourselves. Don't do this at home, kids!
			UObject::BeginLoad();
			ULinkerLoad* Linker	= ULinkerLoad::CreateLinker( Package, *DstFilename, LOAD_NoWarn | LOAD_NoVerify );
			UObject::EndLoad();

			// Force a recook if there is no linker or the packge summary is outdated.
			if( !Linker || (Linker->Summary.CookedContentVersion < GPackageFileCookedContentVersion) )
			{
				bCookedVersionIsOutDated = TRUE;
			}
		}

		// Skip over unchanged files (unless we specify ignoring it for e.g. maps or script).
		// Also skip over combined startup packages (@todo: Do date checking for all combined packages)
		if(		( DstFileNewer == TRUE ) 
			&&	( bCookedVersionIsOutDated == FALSE ) 
			&&	( !bIsMapFile || !bAlwaysRecookMaps ) 
			&&	( !bIsNativeScriptFile || !bAlwaysRecookScript ) 
			&&	( !bIsCombinedStartupPackage ) ) 
		{
			warnf(NAME_Log, TEXT("UpToDate %s"), *SrcFilename);
			continue;
		}
		// Skip over any cooked files residing in cooked folder.
		else if( SrcFilename.InStr( GetCookedDirectory() ) != INDEX_NONE )
		{
			//warnf(NAME_Log, TEXT("Skipping %s"), *SrcFilename);
			continue;
		}		

		// Determine which container to add the item to so it can be cooked correctly

		// Package is definitely a map file.
		if( bIsMapFile )
		{
			const UBOOL bIsMultiplayerMap = bSeparateSharedMPResources && SrcFilename.GetBaseFilename( TRUE ).StartsWith(TEXT("MP_"));
			if ( bIsMultiplayerMap )
			{
				// Maintain a separate list of multiplayer maps.
				warnf( NAME_Log, TEXT("GeneratePackageList: Treating %s as a multiplayer map"), *SrcFilename );
				MPMapFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, TRUE, FALSE, FALSE, FALSE ) );
			}
			else
			{
				MapFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, TRUE, FALSE, FALSE, FALSE ) );
			}
		}
		// Package is a script file.
		else if( bIsNativeScriptFile )
		{
			ScriptFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, TRUE, TRUE, FALSE, FALSE ) );
		}
		// Package is a combined startup pacakge
		else if (bIsCombinedStartupPackage)
		{
			StartupFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, TRUE, TRUE, TRUE, FALSE ) );

			// also addit to the notrequired list so that the texture mips are saved out (with no other cooking going on)
			NotRequiredFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, FALSE, FALSE, FALSE, FALSE ) );
		}
		else if (bIsSeekfreeAlwaysCookPackage)
		{
			StandaloneSeekfreeFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, TRUE, FALSE, FALSE, TRUE ) );

			// also addit to the notrequired list so that the texture mips are saved out (with no other cooking going on)
			NotRequiredFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, FALSE, FALSE, FALSE, FALSE ) );
		}
		// Package is a required package, but not startup package (always cook package, etc)
		else if( RequiredPackages.FindItemIndex( SrcFilename.GetBaseFilename().ToUpper() ) != INDEX_NONE )
		{
			RegularFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, FALSE, FALSE, FALSE, FALSE ) );
		}
		// Package is not required and can be stripped of everything but texture miplevels.
		else
		{
			NotRequiredFilenamePairs.AddItem( FPackageCookerInfo( *SrcFilename, *DstFilename, FALSE, FALSE, FALSE, FALSE ) );
		}
	}

	// Sort regular files, script and finally maps to be last.
	TArray<FPackageCookerInfo> SortedFilenamePairs;
	
	// Always cooked regular (non script, non map) packages first.
	SortedFilenamePairs += NotRequiredFilenamePairs;

	// Followed by required packages.
	FirstRequiredIndex = SortedFilenamePairs.Num();
	SortedFilenamePairs += RegularFilenamePairs;

	// And script.
	FirstScriptIndex		= SortedFilenamePairs.Num();
	if( ScriptFilenamePairs.Num() )
	{
		FirstGameScriptIndex	= FirstScriptIndex + EngineScriptPackageNames.Num();
		// Add in order of appearance in EditPackages and not layout on file determined by filename.
		TArray<FString> ScriptPackageNames;
		ScriptPackageNames += EngineScriptPackageNames;
		ScriptPackageNames += GameNativeScriptPackageNames;
		for( INT NameIndex=0; NameIndex<ScriptPackageNames.Num(); NameIndex++ )
		{
			for( INT PairIndex=0; PairIndex<ScriptFilenamePairs.Num(); PairIndex++ )
			{
				if( ScriptPackageNames(NameIndex) == ScriptFilenamePairs(PairIndex).SrcFilename.GetBaseFilename() )
				{
					SortedFilenamePairs.AddItem( ScriptFilenamePairs(PairIndex) );
					break;
				}
			}
		}
	}

	// Then cook startup packages
	FirstStartupIndex = SortedFilenamePairs.Num();
	SortedFilenamePairs += StartupFilenamePairs;

	SortedFilenamePairs += StandaloneSeekfreeFilenamePairs;

	// Now append maps.
	if( MapFilenamePairs.Num() )
	{
		FirstMapIndex = SortedFilenamePairs.Num();
		SortedFilenamePairs += MapFilenamePairs;
	}

	// Maintain a separate list of multiplayer maps.
	if( MPMapFilenamePairs.Num() )
	{
		FirstMPMapIndex = SortedFilenamePairs.Num();
		SortedFilenamePairs += MPMapFilenamePairs;
	}

	return SortedFilenamePairs;
}

/**
 * Cleans up DLL handles and destroys cookers
 */
void UCookPackagesCommandlet::Cleanup()
{
	if( TextureCooker )
	{
		DestroyTextureCooker( TextureCooker );
		TextureCooker = NULL;
	}
	if( SoundCooker )
	{
		DestroySoundCooker( SoundCooker );
		SoundCooker = NULL;
	}
	if( SkeletalMeshCooker )
	{
		DestroySkeletalMeshCooker( SkeletalMeshCooker );
		SkeletalMeshCooker = NULL;
	}
	if( StaticMeshCooker )
	{
		DestroyStaticMeshCooker( StaticMeshCooker );
		StaticMeshCooker = NULL;
	}

	if( GConsoleShaderPrecompilers[ShaderPlatform] )
	{
		DestroyShaderPrecompiler( GConsoleShaderPrecompilers[ShaderPlatform] );
		GConsoleShaderPrecompilers[ShaderPlatform] = NULL;
	}

	if( PlatformToolsDllHandle )
	{
		appFreeDllHandle( PlatformToolsDllHandle );
		PlatformToolsDllHandle = NULL;
	}
}

/**
 * Handles duplicating cubemap faces that are about to be saved with the passed in package.
 *
 * @param	Package	 Package for which cubemaps that are going to be saved with it need to be handled.
 */
void UCookPackagesCommandlet::HandleCubemaps( UPackage* Package )
{
#if 0 //@todo cooking: this breaks regular textures being used for cubemap faces
	// Create dummy, non serialized package old cubemap faces get renamed into.
	UPackage* OldCubemapFacesRenamedByCooker = CreatePackage( NULL, TEXT("OldCubemapFacesRenamedByCooker") );
#endif
	// Duplicate textures used by cubemaps.
	for( TObjectIterator<UTextureCube> It; It; ++It )
	{
		UTextureCube* Cubemap = *It;
		// Valid cubemap (square textures of same size and format assigned to all faces)
		if (Cubemap->bIsCubemapValid)
		{
			// Only duplicate cubemap faces saved into this package.
			if( Cubemap->IsIn( Package ) || Cubemap->HasAnyFlags( RF_ForceTagExp ) )
			{
				for( INT FaceIndex=0; FaceIndex<6; FaceIndex++ )
				{
					UTexture2D* OldFaceTexture	= Cubemap->GetFace(FaceIndex);
					// Only duplicate once.
					if( OldFaceTexture && OldFaceTexture->GetOuter() != Cubemap )
					{
						// Duplicate cubemap faces so every single texture only gets loaded once.
						Cubemap->SetFace(
							FaceIndex,
							CastChecked<UTexture2D>( UObject::StaticDuplicateObject( 
							OldFaceTexture, 
							OldFaceTexture, 
							Cubemap, 
							*FString::Printf(TEXT("CubemapFace%i"),FaceIndex)
									) )
								);
#if 0 //@todo cooking: this breaks regular textures being used for cubemap faces
						// Rename old cubemap faces into non- serialized package and make sure they are never put into
						// a seekfree package by clearing flags and marking them.
						OldFaceTexture->Rename( NULL, OldCubemapFacesRenamedByCooker, REN_ForceNoResetLoaders );
						OldFaceTexture->ClearFlags( RF_ForceTagExp );
						OldFaceTexture->SetFlags( RF_MarkedByCooker );
#endif
					}
				}
			}
		}
		// Cubemap is invalid!
		else
		{
			// No need to reference texture from invalid cubemaps.
			for( INT FaceIndex=0; FaceIndex<6; FaceIndex++ )
			{
				Cubemap->SetFace(FaceIndex,NULL);
			}
		}
	}
}

/**
 * Collects garbage and verifies all maps have been garbage collected.
 */
void UCookPackagesCommandlet::CollectGarbageAndVerify()
{
	STAT(FScopeSecondsCounter CollectGarbageAndVerifyTimeCounter(CollectGarbageAndVerifyTime));

	// Clear all components as quick sanity check to ensure GC can remove everything.
	GWorld->ClearComponents();

	// Collect garbage up-front to ensure that only required objects will be put into the seekfree package.
	UObject::CollectGarbage(RF_Native);

	// At this point the only world still around should be GWorld, which is the one created in CreateNew.
	for( TObjectIterator<UWorld> It; It; ++It )
	{
		UWorld* World = *It;
		if( World != GWorld )
		{
			UObject::StaticExec(*FString::Printf(TEXT("OBJ REFS CLASS=WORLD NAME=%s.TheWorld"), *World->GetOutermost()->GetName()));

			TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( World, TRUE, GARBAGE_COLLECTION_KEEPFLAGS );
			FString						ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, World );
			debugf(TEXT("%s"),*ErrorString);		
		
			// We cannot safely recover from this.
			appErrorf( TEXT("%s not cleaned up by garbage collection!") LINE_TERMINATOR TEXT("%s"), *World->GetFullName(), *ErrorString );
		}
	}
}

/**
 * Saves the passed in package, gathers and stores bulk data info and keeps track of time spent.
 *
 * @param	Package						Package to save
 * @param	Base						Base/ root object passed to SavePackage, can be NULL
 * @param	TopLevelFlags				Top level "keep"/ save flags, passed onto SavePackage
 * @param	DstFilename					Filename to save under
 * @param	bStripEverythingButTextures	Whether to strip everything but textures
 */
void UCookPackagesCommandlet::SaveCookedPackage( UPackage* Package, UObject* Base, EObjectFlags TopLevelFlags, const TCHAR* DstFilename, UBOOL bStripEverythingButTextures, UBOOL bCleanupAfterSave, UBOOL bRemeberSavedObjects )
{
	STAT(FScopeSecondsCounter PackageSaveTimeCounter(PackageSaveTime));

	// for PC cooking, we leave all objects in their original packages, and in cooked packages
	if (Platform == PLATFORM_Windows)
	{
		bStripEverythingButTextures = FALSE;

		// make sure destination exists
		GFileManager->MakeDirectory(*FFilename(DstFilename).GetPath(), TRUE);

		for( TObjectIterator<UObject> It; It; ++It )
		{
			// never allow any shader objects to get cooked into another package
			if( It->IsA(UShaderCache::StaticClass()) )
			{
				It->ClearFlags( RF_ForceTagExp );
			}
		}
	}

	// Remove RF_Standalone from everything but UTexture2D objects if we're stripping out data.
	TMap<UObject*,UBOOL> ObjectsToRestoreStandaloneFlag;
	TMap<UObject*,UBOOL> ObjectsToRemoveStandaloneFlag;
	UBOOL bPackageContainsTextures = FALSE;
	if( bStripEverythingButTextures )
	{
		check(Base==NULL);
		// Iterate over objects, remove RF_Standalone fron non- UTexture2D objects and keep track of them for later.
		for( TObjectIterator<UObject> It; It; ++It )
		{
			UObject* Object = *It;
			// Non UTexture2D object that is marked RF_Standalone.
			if( Object->IsA(UTexture2D::StaticClass()) )
			{
				if( Object->IsIn( Package ) )
				{
					bPackageContainsTextures = TRUE;

					// fonts aren't standalone (and in the future maybe other's won't be as well), so
					// temporarily mark them as standalone so they are saved out in the streaming texture package
					// yes, fonts shouldn't be streaming, but just in case, we don't want to cause crashes!
					if (Object->HasAllFlags(RF_Standalone) == FALSE)
					{
						// Keep track of object and set flag.
						ObjectsToRemoveStandaloneFlag.Set( Object, TRUE );
						Object->SetFlags( RF_Standalone );
					}
				}
			}
			else if( Object->HasAllFlags( RF_Standalone ) )
			{
				// Keep track of object and remove flag.
				ObjectsToRestoreStandaloneFlag.Set( Object, TRUE );
				Object->ClearFlags( RF_Standalone );
			}
		}
	}

	//@todo cooker: Skipping unchanged packages relies on dummy package to be present. This could be solved via a package flag
	//@todo cooker: but that's probably not worth it. 
	//@todo cooker: if( !bStripEverythingButTextures || bPackageContainsTextures )
	{
		// Save package. World can be NULL for regular packages. @warning: DO NOT conform here, as that will cause 
		// the number of generations in the summary of package files to mismatch with the number of generations in 
		// forced export copies of that package! SavePackage has code for the cooking case to match up GUIDs with 
		// the original package
		UObject::SavePackage( Package, Base, TopLevelFlags, DstFilename, GWarn, NULL, ShouldByteSwapData());
	}

	// Restore RF_Standalone flag on objects.
	if( bStripEverythingButTextures )
	{
		for( TMap<UObject*,UBOOL>::TIterator It(ObjectsToRestoreStandaloneFlag); It; ++It )
		{
			UObject* Object = It.Key();
			Object->SetFlags( RF_Standalone );
		}
		for( TMap<UObject*,UBOOL>::TIterator It(ObjectsToRemoveStandaloneFlag); It; ++It )
		{
			UObject* Object = It.Key();
			Object->ClearFlags( RF_Standalone );
		}
	}

	// Clean up objects for subsequent runs.
	if( bCleanupAfterSave )
	{
		// Remove RF_ForceTagExp again for subsequent calls to SavePackage and set RF_MarkedByCooker for objects
		// saved into script. Also ensures that always loaded materials are only put into a single seekfree shader 
		// cache.
		for( FObjectIterator It; It; ++It )
		{
			UObject*	Object		= *It;
			UMaterial*	Material	= Cast<UMaterial>(Object);
			UMaterialInstance*	MaterialInterface	= Cast<UMaterialInstance>(Object);

			// Avoid object from being put into subsequent script or map packages.
			if( bRemeberSavedObjects && Object->HasAnyFlags( RF_Saved ) )
			{
				Object->SetFlags( RF_MarkedByCooker );
			}
			Object->ClearFlags( RF_ForceTagExp | RF_Saved );

			// Don't put material into subsequent seekfree shader caches.
			if( bRemeberSavedObjects && ShaderCache && Material )
			{
				AlreadyHandledMaterials.Set( Material, TRUE );
			}
			if( bRemeberSavedObjects && ShaderCache && MaterialInterface )
			{
				AlreadyHandledMaterialInstances.Set( MaterialInterface, TRUE );
			}
		}
	}

	// Gather bulk data information from just saved packages and save it to disk.
	CookedBulkDataInfoContainer->GatherCookedInfos( Package );
	CookedBulkDataInfoContainer->SaveToDisk();
}

/**
 * Returns whether there are any localized resources that need to be handled.
 *
 * @param Package			Current package that is going to be saved
 * @param TopLevelFlags		TopLevelFlags that are going to be passed to SavePackage
 * 
 * @return TRUE if there are any localized resources pending save, FALSE otherwise
 */
UBOOL UCookPackagesCommandlet::AreThereLocalizedResourcesPendingSave( UPackage* Package, EObjectFlags TopLevelFlags )
{
	UBOOL bAreLocalizedResourcesPendingSave = FALSE;
	for( FObjectIterator It; It; ++It )
	{
		UObject* Object = *It;
		if( Object->IsLocalizedResource() 
		&& (Object->IsIn(Package) || Object->HasAnyFlags(TopLevelFlags | RF_ForceTagExp)) )
		{
			bAreLocalizedResourcesPendingSave = TRUE;
			break;
		}
	}
	return bAreLocalizedResourcesPendingSave;
}

/**
 * @return		TRUE if there are localized resources using the current language that need to be handled.
 */
static UBOOL AreThereNonEnglishLocalizedResourcesPendingSave(UPackage* Package)
{
	// Shouldn't be called in English, as _INT doesn't appear as a package file prefix.
	check( appStricmp( TEXT("INT"), UObject::GetLanguage() ) != 0 );

	UBOOL bAreNonEnglishLocalizedResourcesPendingSave = FALSE;
	for( FObjectIterator It; It; ++It )
	{
		UObject* Object = *It;
		if( Object->IsLocalizedResource() 
			&& (Object->IsIn(Package) || Object->HasAnyFlags(RF_ForceTagExp)) )
		{
			UPackage* ObjPackage = Object->GetOutermost();
			const FString ObjPackageName( ObjPackage->GetName() );

			FString FoundPackageFileName;
			if( GPackageFileCache->FindPackageFile( *ObjPackageName, NULL, FoundPackageFileName ) )
			{
				// Check if the found filename contains for a cached package of the same name.
				FFilename LocalizedPackageName = FPackageFileCache::PackageFromPath( *ObjPackageName );
				LocalizedPackageName = LocalizedPackageName.GetLocalizedFilename( UObject::GetLanguage() );

				// InStr does a case-sensitive compare, so convert package names to the same case.
				LocalizedPackageName = LocalizedPackageName.ToLower();
				FoundPackageFileName = FoundPackageFileName.ToLower();

				if ( FoundPackageFileName.InStr(*LocalizedPackageName) != -1 )
				{
					bAreNonEnglishLocalizedResourcesPendingSave = TRUE;
					break;
				}
			}
		}
	}
	return bAreNonEnglishLocalizedResourcesPendingSave;
}

template < class T >
class TMPSoundObjectReferenceCollector : public FArchive
{
public:

	/**
	* Constructor
	*
	* @param	InObjectArray			Array to add object references to
	* @param	InOuters				value for LimitOuter
	* @param	bInRequireDirectOuter	value for bRequireDirectOuter
	* @param	bShouldIgnoreArchetype	whether to disable serialization of ObjectArchetype references
	* @param	bInSerializeRecursively	only applicable when LimitOuter != NULL && bRequireDirectOuter==TRUE;
	*									serializes each object encountered looking for subobjects of referenced
	*									objects that have LimitOuter for their Outer (i.e. nested subobjects/components)
	* @param	bShouldIgnoreTransient	TRUE to skip serialization of transient properties
	*/
	TMPSoundObjectReferenceCollector( TArray<T*>* InObjectArray, const TArray<FString>& InOuters, UBOOL bInRequireDirectOuter=TRUE, UBOOL bShouldIgnoreArchetype=FALSE, UBOOL bInSerializeRecursively=FALSE, UBOOL bShouldIgnoreTransient=FALSE )
		:	ObjectArray( InObjectArray )
		,	LimitOuters(InOuters)
		,	bRequireDirectOuter(bInRequireDirectOuter)
	{
		ArIsObjectReferenceCollector = TRUE;
		ArIsPersistent = bShouldIgnoreTransient;
		ArIgnoreArchetypeRef = bShouldIgnoreArchetype;
		bSerializeRecursively = bInSerializeRecursively && LimitOuters.Num() > 0;
	}
protected:

	/** Stored pointer to array of objects we add object references to */
	TArray<T*>*		ObjectArray;

	/** List of objects that have been recursively serialized */
	TArray<UObject*> SerializedObjects;

	/** only objects within these outers will be considered. */
	TArray<FString>	LimitOuters;

	/** determines whether nested objects contained within LimitOuter are considered */
	UBOOL			bRequireDirectOuter;

	/** determines whether we serialize objects that are encounterd by this archive */
	UBOOL			bSerializeRecursively;
};

/**
* Helper implementation of FArchive used to collect object references, avoiding duplicate entries.
*/
class FArchiveMPSoundObjectReferenceCollector : public TMPSoundObjectReferenceCollector<UObject>
{
public:
	/**
	* Constructor
	*
	* @param	InObjectArray			Array to add object references to
	* @param	InOuters				value for LimitOuter
	* @param	bInRequireDirectOuter	value for bRequireDirectOuter
	* @param	bShouldIgnoreArchetype	whether to disable serialization of ObjectArchetype references
	* @param	bInSerializeRecursively	only applicable when LimitOuter != NULL && bRequireDirectOuter==TRUE;
	*									serializes each object encountered looking for subobjects of referenced
	*									objects that have LimitOuter for their Outer (i.e. nested subobjects/components)
	* @param	bShouldIgnoreTransient	TRUE to skip serialization of transient properties
	*/
	FArchiveMPSoundObjectReferenceCollector( TArray<UObject*>* InObjectArray, const TArray<FString>& InOuters, UBOOL bInRequireDirectOuter=TRUE, UBOOL bShouldIgnoreArchetypes=FALSE, UBOOL bInSerializeRecursively=FALSE, UBOOL bShouldIgnoreTransient=FALSE )
		:	TMPSoundObjectReferenceCollector<UObject>( InObjectArray, InOuters, bInRequireDirectOuter, bShouldIgnoreArchetypes, bInSerializeRecursively, bShouldIgnoreTransient )
	{}

private:
	/** 
	* UObject serialize operator implementation
	*
	* @param Object	reference to Object reference
	* @return reference to instance of this class
	*/
	FArchive& operator<<( UObject*& Object )
	{
		// Avoid duplicate entries.
		if ( Object != NULL )
		{
			if ( LimitOuters.Num() == 0 || LimitOuters.ContainsItem(Object->GetOutermost()->GetName()) )
			{
				if ( !ObjectArray->ContainsItem(Object) )
				{
					ObjectArray->AddItem( Object );

					// check this object for any potential object references
					if ( bSerializeRecursively )
					{
						Object->Serialize(*this);
					}
				}
			}
		}
		return *this;
	}
};

/**
 * Generates a list of all objects in the object tree rooted at the specified sound cues.
 *
 * @param	MPCueNames				The names of the MP sound 'root set'.
 * @param	OutMPObjectNames		[out] The names of all objects referenced by the input set of objects, including those objects.
 */
static void GenerateMPObjectList(const TArray<FString>& MPCueNames, TArray<FString>& OutMPObjectNames)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Load all MP-referenced cues.
	TArray<USoundCue*> MPCues;
	for ( INT CueIndex = 0 ; CueIndex < MPCueNames.Num() ; ++CueIndex )
	{
		const FString& CueName = MPCueNames(CueIndex);
		USoundCue* Cue = FindObject<USoundCue>( NULL, *CueName );
		if ( !Cue )
		{
			Cue = LoadObject<USoundCue>( NULL, *CueName, NULL, LOAD_None, NULL );
		}
		if ( Cue )
		{
			MPCues.AddItem( Cue );
		}
		else
		{
			warnf( NAME_Log, TEXT("MP Sound Cues: couldn't load %s"), *CueName );
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Load all MP-referenced sound cue packages.
	const TCHAR* MPSoundcuePackagesIniSection = TEXT("Cooker.MPSoundCuePackages");
	TMultiMap<FString,FString>* IniMPPackagesList = GConfig->GetSectionPrivate( MPSoundcuePackagesIniSection, FALSE, TRUE, GEditorIni );
	//TArray<UObject*> SoundPackages;
	TArray<FString> SoundPackages;
	if ( MPSoundcuePackagesIniSection && IniMPPackagesList )
	{
		for( TMultiMap<FString,FString>::TConstIterator It(*IniMPPackagesList) ; It ; ++It )
		{
			const FString& PackageName = It.Value();
			FString PackageFilename;
			UPackage* Result = NULL;
			if( GPackageFileCache->FindPackageFile( *PackageName, NULL, PackageFilename ) )
			{
				SoundPackages.AddUniqueItem( PackageName );
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Serialize all the cues into a reference collector.
	TArray<UObject*> References;
	FArchiveMPSoundObjectReferenceCollector ObjectReferenceCollector( &References, SoundPackages, FALSE, FALSE, TRUE, TRUE );
	for ( INT CueIndex = 0 ; CueIndex < MPCues.Num() ; ++ CueIndex )
	{
		USoundCue* Cue = MPCues(CueIndex);
		Cue->Serialize( ObjectReferenceCollector );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create a list of the names of all objects referenced by the MP cues.
	OutMPObjectNames.Empty();

	// Seed the list with the cues themselves.
	OutMPObjectNames = MPCueNames;

	// Add referenced objects.
	for ( INT Index = 0 ; Index < References.Num() ; ++Index )
	{
		const UObject* Object = References(Index);
		// Exclude transient objects.
		if( !Object->HasAnyFlags( RF_Transient ) &&	!Object->IsIn( UObject::GetTransientPackage() ) )
		{
			const FString ObjectPath = Object->GetPathName();
			debugfSuppressed( NAME_DevCooking, TEXT("MPObjects: %s (%s)"), *ObjectPath, *Object->GetClass()->GetName() );
			OutMPObjectNames.AddItem( ObjectPath );
		}
	}
}

/**
 * Cooks the specified cues into the shared MP sound package.
 *
 * @param	MPCueNames		The set of sound cue names to cook.
 */
void UCookPackagesCommandlet::CookMPSoundPackages(const TArray<FString>& MPCueNames)
{
	if ( bSeparateSharedMPResources && MPCueNames.Num() > 0 )
	{
		// Mark all objects (cues) that are already in memory (and thus already cooked),
		// so we don't cook into the MP sounds package.
		for( FObjectIterator It ; It ; ++It )
		{
			UObject*	Object		= *It;
			// Mark the object as unwanted.
			Object->SetFlags( RF_MarkedByCooker );
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Find/Load all the requested cues.
		for ( INT CueIndex = 0 ; CueIndex < MPCueNames.Num() ; ++CueIndex )
		{
			const FString& CueName = MPCueNames(CueIndex);
			USoundCue* Cue = FindObject<USoundCue>( NULL, *CueName );
			if ( !Cue )
			{
				Cue = LoadObject<USoundCue>( NULL, *CueName, NULL, LOAD_None, NULL );
				if ( !Cue )
				{
					warnf( NAME_Log, TEXT("MP Sound Cues: couldn't load %s"), *CueName );
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Iterate over all objects and mark them as RF_ForceTagExp where appropriate.
		for( FObjectIterator It ; It ; ++It )
		{
			UObject* Object = *It;
			// Exclude objects that are either marked "marked" or transient.
			if( !Object->HasAnyFlags( RF_MarkedByCooker|RF_Transient ) 
				// Exlcude objects that reside in the transient package.
				&&	!Object->IsIn( UObject::GetTransientPackage() ) )
			{
				// Mark object and outer chain so they get forced into the export table.
				while( Object )
				{
					Object->SetFlags( RF_ForceTagExp );
					Object = Object->GetOuter();
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Create a new package.

		FFilename DstFilename	= CookedDir + MPSounds_PackageName + (Platform == PLATFORM_Windows ? TEXT(".upk") : TEXT(".xxx"));
		UPackage* MPSoundsPackage = CreatePackage( NULL, MPSounds_PackageName );
		check( !(MPSoundsPackage->PackageFlags & PKG_Cooked) ); 
		MPSoundsPackage->PackageFlags |= PKG_Cooked;
		MPSoundsPackage->PackageFlags |= PKG_DisallowLazyLoading;

		// Treat the MP sounds package as seek free.
		MPSoundsPackage->PackageFlags |= PKG_RequireImportsAlreadyLoaded;
		MPSoundsPackage->PackageFlags |= PKG_StoreCompressed;
		MPSoundsPackage->PackageFlags |= PKG_ServerSideOnly;

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Iterate over all objects and cook them if they are going to be saved with the package.
		for( FObjectIterator It ; It ; ++It )
		{
			UObject* Object = *It;
			if( Object->HasAnyFlags( RF_ForceTagExp ) )
			{
				// Cook object.
				CookObject( Object );
				// Prepare it for serialization.
				PrepareForSaving( MPSoundsPackage, Object, TRUE );
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cook localized objects.

		const FString MPsoundsObjectReferencerName( MPSounds_ReferencerName );

		// The MP sounds package is seekfree, and most likely localized.
		if ( AreThereLocalizedResourcesPendingSave( NULL, 0 ) )
		{
			const UBOOL bCookingEnglish = appStricmp( TEXT("INT"), UObject::GetLanguage() ) == 0;

			// The cooker should not save out localization packages when cooking for non-English if no non-English resources were found.
			if ( bCookingEnglish || AreThereNonEnglishLocalizedResourcesPendingSave( NULL ) )
			{
				// Figure out localized package name and file name.
				FFilename LocalizedPackageName		= FFilename(DstFilename.GetBaseFilename() + LOCALIZED_SEEKFREE_SUFFIX).GetLocalizedFilename();
				FFilename LocalizedPacakgeFilename	= DstFilename.GetPath() + PATH_SEPARATOR + LocalizedPackageName + TEXT(".") + DstFilename.GetExtension();

				// Create a localized package and disallow lazy loading as we're seek free.
				UPackage* LocalizedPackage = CreatePackage( NULL, *LocalizedPackageName );
				LocalizedPackage->PackageFlags |= PKG_Cooked;
				LocalizedPackage->PackageFlags |= PKG_DisallowLazyLoading;
				LocalizedPackage->PackageFlags |= PKG_RequireImportsAlreadyLoaded;
				LocalizedPackage->PackageFlags |= PKG_ServerSideOnly;
				LocalizedPackage->PackageFlags |= PKG_StoreCompressed;

				// Create object references within package.
				const FString LocalizedObjectReferencerName( MPsoundsObjectReferencerName + TEXT("_") + UObject::GetLanguage() );
				UObjectReferencer* ObjectReferencer = ConstructObject<UObjectReferencer>(
					UObjectReferencer::StaticClass(),
					LocalizedPackage,
					FName(*LocalizedObjectReferencerName),
					RF_Cooked );

				// Reference all localized audio with the appropriate flags by the helper object.
				for( FObjectIterator It; It; ++It )
				{
					UObject* Object = *It;
					if( Object->IsLocalizedResource() )
					{
						if( Object->HasAnyFlags(RF_ForceTagExp) )
						{
							ObjectReferencer->ReferencedObjects.AddItem( Object );
						}
					}
				}

				// Save localized assets referenced by object referencer.
				SaveCookedPackage( LocalizedPackage, ObjectReferencer, RF_Standalone, *LocalizedPacakgeFilename, FALSE);

				// Prevent objects from being saved into seekfree package.
				for( INT ResourceIndex=0; ResourceIndex<ObjectReferencer->ReferencedObjects.Num(); ResourceIndex++ )
				{
					UObject* Object = ObjectReferencer->ReferencedObjects(ResourceIndex);
					Object->ClearFlags( RF_ForceTagExp );
					Object->SetFlags( RF_MarkedByCooker );
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cook remaining (unlocalized) objects.

		// Create an object referencer for any remaining (unlocalized) objects.
		UObjectReferencer* ObjectReferencer = ConstructObject<UObjectReferencer>(
			UObjectReferencer::StaticClass(),
			MPSoundsPackage,
			FName(*MPsoundsObjectReferencerName),
			RF_Cooked );

		// Reference all localized audio with the appropriate flags by the helper object.
		for( FObjectIterator It; It; ++It )
		{
			UObject* Object = *It;
			if( Object->HasAnyFlags(RF_ForceTagExp) )
			{
				ObjectReferencer->ReferencedObjects.AddItem( Object );
			}
		}

		// Saved any remaining unlocalized objects.
		SaveCookedPackage( MPSoundsPackage, ObjectReferencer, RF_Standalone, *DstFilename, FALSE );

		for( FObjectIterator It ; It ; ++It )
		{
			UObject* Object = *It;
			Object->ClearFlags( RF_ForceTagExp );
			Object->SetFlags( RF_MarkedByCooker );
		}
	}
}

/**
 * Main entry point for commandlets.
 */
INT UCookPackagesCommandlet::Main( const FString& Params )
{
	TotalTime = appSeconds();

	// get the platform from the Params (MUST happen first thing)
	if ( !SetPlatform(Params) )
	{
		SET_WARN_COLOR(COLOR_RED);
		warnf(NAME_Error, TEXT("Platform not specified. You must use:\n   platform=[xenon|pc|ps3|linux|mac]"));
		CLEAR_WARN_COLOR();

		return 1;
	}

	// Bind to cooking DLLs
 	if( !BindDLLs() )
	{
		return 1;
	}

	// Parse command line and perform any initialization that isn't per package.
	if( !Init( *Params ) )
	{
		return 1;
	}

	// if we only wanted to cook inis, then skip over package processing
	if (!bIniFilesOnly)
	{
		// Generate list of packages to cook.
		INT FirstStartupIndex		= INDEX_NONE;
		INT FirstRequiredIndex		= INDEX_NONE;
		INT FirstScriptIndex		= INDEX_NONE;
		INT FirstMapIndex			= INDEX_NONE;
		INT FirstMPMapIndex			= INDEX_NONE;
		INT FirstGameScriptIndex	= INDEX_NONE;
		TArray<FPackageCookerInfo> PackageList = GeneratePackageList( FirstRequiredIndex, FirstStartupIndex, FirstScriptIndex, FirstGameScriptIndex, FirstMapIndex, FirstMPMapIndex );

		TArray<FString> MPCueNames;
		TArray<FString> MPObjectNames;
		if ( bSeparateSharedMPResources )
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			// Look to the .ini for the list of sound cues referenced by MP
			const TCHAR* MPSoundcueIniSection = TEXT("Cooker.MPSoundCues");
			TMultiMap<FString,FString>* IniMPCuesList = GConfig->GetSectionPrivate( MPSoundcueIniSection, FALSE, TRUE, GEditorIni );
			if ( IniMPCuesList )
			{
				for( TMultiMap<FString,FString>::TConstIterator It(*IniMPCuesList) ; It ; ++It )
				{
					const FString& CueName = It.Value();
					MPCueNames.AddUniqueItem( CueName );
				}
			}

			if ( MPCueNames.Num() == 0 )
			{
				warnf( NAME_Warning, TEXT("No MP sounds to cook -- was the %s section of %s empty?"), MPSoundcueIniSection, GEditorIni );
			}
			else
			{
				// Generate the full set of objects referenced by MP sound cues.
				GenerateMPObjectList( MPCueNames, MPObjectNames );
			}
		}

		// find out if the target platform wants to fully load packages from memory at startup
		// if so, we will fully compress some files (only really makes sense if we preload them from memory)
		UBOOL bPreloadPackagesFromMemory = FALSE;
		GConfig->GetBool(TEXT("Engine.SeekFree"), TEXT("bPreloadPackagesFromMemory"), bPreloadPackagesFromMemory, PlatformEngineConfigFilename);

		// create the combined startup package
		FString StartupPackageName = FString(TEXT("Startup_")) + UObject::GetLanguage();

		UPackage* CombinedStartupPackage = UObject::CreatePackage(NULL, *StartupPackageName);
		// make sure this package isn't GC'd
		CombinedStartupPackage->AddToRoot();

		// create an object to reference all the objects in the startup packages to be combined together
		UObjectReferencer* CombinedStartupReferencer = ConstructObject<UObjectReferencer>( UObjectReferencer::StaticClass(), CombinedStartupPackage, NAME_None, RF_Cooked );
		// make sure this refrencer isn't GC'd
		CombinedStartupReferencer->AddToRoot();

		// read in the per-map packages to cook
		TMap<FName, TArray<FName> > PerMapCookPackages;
		GConfig->Parse1ToNSectionOfNames(TEXT("Engine.PackagesToForceCookPerMap"), TEXT("Map"), TEXT("Package"), PerMapCookPackages, PlatformEngineConfigFilename);

		// Iterate over all packages.
		for( INT PackageIndex=0; PackageIndex<PackageList.Num(); PackageIndex++ )
		{
			// Skip maps not required for cooking process. This is skipping packages that are only going to contain
			// higher miplevels of textures.
			if( bSkipNotRequiredPackages && (PackageIndex < FirstRequiredIndex) )
			{
				continue;
			}

			// Collect garbage and verify it worked as expected.	
			CollectGarbageAndVerify();
		
			// Save the shader caches after each package has been verified.
			SaveLocalShaderCaches();

			const FFilename& SrcFilename			= PackageList(PackageIndex).SrcFilename;
			FFilename DstFilename					= PackageList(PackageIndex).DstFilename;
			UBOOL bShouldBeSeekFree					= PackageList(PackageIndex).bShouldBeSeekFree;
			UBOOL bIsNativeScriptFile				= PackageList(PackageIndex).bIsNativeScriptFile;
			UBOOL bIsCombinedStartupPackage			= PackageList(PackageIndex).bIsCombinedStartupPackage;
			UBOOL bIsStandaloneSeekFreePackage		= PackageList(PackageIndex).bIsStandaloneSeekFreePackage;
			UBOOL bShouldBeFullyCompressed			= bIsNativeScriptFile && bPreloadPackagesFromMemory;
			FString PackageName						= SrcFilename.GetBaseFilename( TRUE );
			const UBOOL bIsMultiplayerMap			= bSeparateSharedMPResources && PackageName.StartsWith(TEXT("MP_"));
			if ( bIsMultiplayerMap )
			{
				// Assert that MP maps are being cooked last.
				check( PackageIndex >= FirstMPMapIndex );
				warnf( NAME_Log, TEXT("Treating %s as a multiplayer map"), *PackageName );
			}

			// for PC we don't strip out non-texture objects
			const UBOOL bStripEverythingButTextures	= PackageIndex < FirstRequiredIndex && Platform != PLATFORM_Windows;
		
			// Make sure that none of the currently existing objects get put into a map file. This is required for
			// the code to work correctly in combination with not always recooking script packages.
			if( PackageIndex == FirstMapIndex )
			{
				// Iterate over all objects, setting RF_MarkedByCooker and marking materials as already handled.
				for( FObjectIterator It; It; ++It )
				{
					UObject*	Object		= *It;
					UMaterial*	Material	= Cast<UMaterial>(Object);
					UMaterialInstance*	MaterialInterface	= Cast<UMaterialInstance>(Object);
					// Don't put into any seekfree packages.
					Object->SetFlags( RF_MarkedByCooker );
					// Don't put material into seekfree shader caches. Also remove all expressions.
					if( Material )
					{
						AlreadyHandledMaterials.Set( Material, TRUE );
						//@todo. Put this back in one form or another.
							// Temporarily removed as it was removing expressions for materials that were
						// needed later when cooking terrain materials.
						//Material->RemoveExpressions();
					}

					if( MaterialInterface )
					{
						AlreadyHandledMaterialInstances.Set( MaterialInterface, TRUE );
					}
				}
			}

			// MP maps are cooked last.  Before cooking any MP maps, though,
			// first cook shared MP sounds, and mark them so they're not cooked into MP maps.
			if ( bSeparateSharedMPResources && PackageIndex == FirstMPMapIndex )
			{
				CookMPSoundPackages( MPCueNames );
			}

			UPackage* Package;

			UObjectReferencer* StandaloneSeekFreeReferencer = NULL;

			// handle combining several packages into one
			if (bIsCombinedStartupPackage)
			{
				STAT(FScopeSecondsCounter LoadCombinedStartupPackagesTimeCounter(LoadCombinedStartupPackagesTime));

				Package = CombinedStartupPackage;
				PackageName = StartupPackageName;
				DstFilename = CookedDir + StartupPackageName + (Platform == PLATFORM_Windows ? TEXT(".upk") : TEXT(".xxx"));

				warnf( NAME_Log, TEXT("Cooking [Combined] %s with:"), *PackageName); 

				// load all the combined startup packages (they'll be in a row)
				for (; PackageIndex < PackageList.Num() && PackageList(PackageIndex).bIsCombinedStartupPackage; PackageIndex++)
				{
					const FFilename& Filename = PackageList(PackageIndex).SrcFilename;
					warnf( NAME_Log, TEXT("   %s"), *Filename.GetBaseFilename()); 

					// load the package and all objects inside
					UPackage* StartupPackage = UObject::LoadPackage( NULL, *Filename, LOAD_None );

					// mark anything inside as unmarked, for any objects already loaded (like EngineMaterials.WireframeMaterial, etc)
					for (FObjectIterator It; It; ++It)
					{
						if (It->IsIn(StartupPackage))
						{
							// reference it so SavePackage will save it
							CombinedStartupReferencer->ReferencedObjects.AddItem(*It);
							It->ClearFlags(RF_MarkedByCooker);
						}
					}
				}

				// decrement the PackageIndex as the outer for loop will jump to the next package as well
				PackageIndex--;
			}
			else if (bIsStandaloneSeekFreePackage)
			{
				STAT(FScopeSecondsCounter LoadPackagesTimeCounter(LoadPackagesTime));

				FString CookedPackageName = PackageName + STANDALONE_SEEKFREE_SUFFIX;
				warnf( NAME_Log, TEXT("Cooking [Seekfree Standalone] %s from\n   %s"), *CookedPackageName, *PackageName);

				// open the source pacakge
				UPackage* SourcePackage = Cast<UPackage>(UObject::LoadPackage( NULL, *SrcFilename, LOAD_None ));

				// create the new package to cook into
				Package = UObject::CreatePackage(NULL, *CookedPackageName);
				
				// this package needs a new guid, because it's cooked and so SavePackage won't create a new guid for it, expecting it to have a 
				// come from some source linker to get it from, that's not the case
				Package->MakeNewGuid();
				Package->PackageFlags = SourcePackage->PackageFlags & ~PKG_Cooked;

				// fix up the destination filename
				DstFilename = DstFilename.Replace(*(PackageName + TEXT(".")), *(CookedPackageName + TEXT(".")));
				
				//create an opbject to refenrece all the objects in the original package
				StandaloneSeekFreeReferencer = ConstructObject<UObjectReferencer>( UObjectReferencer::StaticClass(), Package, NAME_None, RF_Cooked );

				for (FObjectIterator It; It; ++It)
				{
					if (It->IsIn(SourcePackage))
					{
						// reference it so SavePackage will save it
						StandaloneSeekFreeReferencer->ReferencedObjects.AddItem(*It);
					}
				}
			}
			else
			{
				STAT(FScopeSecondsCounter LoadPackagesTimeCounter(LoadPackagesTime));

				warnf( NAME_Log, TEXT("Cooking%s%s %s"), 
					bShouldBeSeekFree ? TEXT(" [Seekfree]") : TEXT(""),
					bShouldBeFullyCompressed ? TEXT(" [Fully compressed]") : TEXT(""),
					*SrcFilename.GetBaseFilename() );

				Package = Cast<UPackage>(UObject::LoadPackage( NULL, *SrcFilename, LOAD_None ));
			}
	
			if( Package )
			{
				// Don't try cooking already cooked data!
				checkf( !(Package->PackageFlags & PKG_Cooked), TEXT("%s is already cooked!"), *SrcFilename ); 
				Package->PackageFlags |= PKG_Cooked;

				// Find the ULevel object (only valid if we're looking at a map).
				UWorld* World = FindObject<UWorld>( Package, TEXT("TheWorld") );

				// Initialize components, so they have correct bounds, Owners and such
				if( World && World->PersistentLevel )
				{
					// Reset cooked physics data counters.
					BrushPhysDataBytes = 0;

					World->PersistentLevel->UpdateComponents();
				}

				if (bShouldBeSeekFree && World != NULL)
				{
					// look up per-map packages
					TArray<FName>* PerMapPackages = PerMapCookPackages.Find(Package->GetFName());

					if (PerMapPackages != NULL)
					{
						STAT(FScopeSecondsCounter LoadPerMapPackagesTimeCounter(LoadPerMapPackagesTime));

						for (INT PerMapPackageIndex = 0; PerMapPackageIndex < PerMapPackages->Num(); PerMapPackageIndex++)
						{
							FName PackageName = (*PerMapPackages)(PerMapPackageIndex);

							UPackage* Pkg = UObject::LoadPackage(NULL, *PackageName.ToString(), LOAD_None);
							if (Pkg != NULL)
							{
								warnf( NAME_Log, TEXT("   Forcing %s into %s"), *PackageName.ToString(), *SrcFilename.GetBaseFilename());

								// refrence all objects in this package with the referencer
								for (FObjectIterator It; It; ++It)
								{
									if (It->IsIn(Pkg))
									{
										// only add objects that are going to be cooked into this package (public, not already marked)
										if (!It->HasAnyFlags(RF_MarkedByCooker|RF_Transient) && It->HasAllFlags(RF_Public))
										{
											World->ExtraReferencedObjects.AddItem(*It);
										}
									}
								}
							}
							else
							{
								warnf( NAME_Log, TEXT("   Failed to force %s into %s"), *PackageName.ToString(), *SrcFilename.GetBaseFilename());
							}
						}
					}
				}

				// Disallow lazy loading for all packages
				Package->PackageFlags |= PKG_DisallowLazyLoading;

				// packages that should be fully compressed need to be marked as such
				if (bShouldBeFullyCompressed)
				{
					Package->PackageFlags |= PKG_StoreFullyCompressed;
				}

				// Handle creation of seek free packages/ levels.
				if( bShouldBeSeekFree )
				{
					Package->PackageFlags |= PKG_RequireImportsAlreadyLoaded;
					// fully compressed packages should not be marked with the PKG_StoreCompressed flag, as that will compress
					// on the export level, not the package level
					if (!bShouldBeFullyCompressed)
					{
						Package->PackageFlags |= PKG_StoreCompressed;
					}

					// Iterate over all objects and mark them as RF_ForceTagExp where appropriate.
					for( FObjectIterator It; It; ++It )
					{
						UObject* Object = *It;
						// Exclude objects that are either marked "marked" or transient.
						if( !Object->HasAnyFlags( RF_MarkedByCooker|RF_Transient ) 
						// Exlcude objects that reside in the transient package.
						&&	!Object->IsIn( UObject::GetTransientPackage() ) 
						// Exclude objects that are in the package as they will be serialized anyways.
						&&	!Object->IsIn( Package ) )
						{
							UBOOL bShouldDuplicate = TRUE;
							// If we're cooking an MP map . . .
							if ( bIsMultiplayerMap && bSeparateSharedMPResources )
							{
								const FString ObjectPath = Object->GetPathName();
								if ( MPObjectNames.ContainsItem( ObjectPath ) )
								{
									bShouldDuplicate = FALSE;
								}
							}
							if ( bShouldDuplicate )
							{
								// Mark object and outer chain so they get forced into the export table.
								while( Object )
								{
									Object->SetFlags( RF_ForceTagExp );
									Object = Object->GetOuter();
								}
							}
						}
					}
				}

				// make a local shader cache in each of the seekfree packages for consoles
				if (bShouldBeSeekFree && Platform != PLATFORM_Windows)
				{
					// We can't save the shader cache into Core as the UShaderCache class is loaded afterwards.
					if( Package->GetFName() != NAME_Core )
					{
						// Construct a shader cache in the package with RF_Standalone so it is being saved into the package even though it is not
						// referenced by any objects in the case of passing in a root, like done e.g. when saving worlds.
						ShaderCache = new(Package,TEXT("SeekFreeShaderCache"),RF_Standalone) UShaderCache(ShaderPlatform);
					}
				}

				// Handle duplicating cubemap faces. This needs to be done before the below object iterator as it creates new objects.
				HandleCubemaps( Package );

				// Replace static mesh actors with a StaticMeshCollectionActor
				if ( !bStripEverythingButTextures )
				{
					if ( bCookOutStaticMeshActors )
					{
						CookStaticMeshActors( Package );
					}

					if ( bCookOutStaticLightActors )
					{
						CookStaticLightActors(Package);
					}
				}

				// Iterate over all objects and cook them if they are going to be saved with the package.
				for( FObjectIterator It; It; ++It )
				{
					UObject* Object = *It;
					if( Object->IsIn( Package ) || Object->HasAnyFlags( RF_ForceTagExp ) )
					{
						// Don't cook non texture objects for packages that will only contain texture mips.
						if( !bStripEverythingButTextures || Object->IsA(UTexture2D::StaticClass()) )
						{
							// Cook object.
							CookObject( Object );
							// Prepare it for serialization.
							PrepareForSaving( Package, Object, bShouldBeSeekFree );
						}
					}
				}

				// Clear components before saving and recook static mesh physics data cache.
				if( World && World->PersistentLevel )
				{
					// Re-build the cooked staticmesh physics data cache for the map, in target format.
					World->PersistentLevel->BuildPhysStaticMeshCache();

					// Clear components before we save.
					World->PersistentLevel->ClearComponents();

					// Print out physics cooked data sizes.
					debugfSuppressed( NAME_DevCooking, TEXT("COOKEDPHYSICS: Brush: %f KB"), ((FLOAT)BrushPhysDataBytes)/1024.f );
				}

				// Figure out whether there are any localized resource that would be saved into seek free package and create a special case package
				// that is loaded upfront instead.
				//
				// @warning: Engine does not support localized materials. Mostly because the localized packages don't have their own shader cache.
				UBOOL bIsLocalizedScriptPackage = bIsNativeScriptFile && (PackageIndex == FirstGameScriptIndex) && bShouldBeSeekFree;
				UBOOL bSupportsLocalization = bIsLocalizedScriptPackage || (!bIsNativeScriptFile && bShouldBeSeekFree);
				if( bSupportsLocalization && AreThereLocalizedResourcesPendingSave( Package, 0 ) )
				{
					const UBOOL bCookingEnglish = appStricmp( TEXT("INT"), UObject::GetLanguage() ) == 0;

					// The cooker should not save out localization packages when cooking for non-English if no non-English resources were found.
					if ( bCookingEnglish || AreThereNonEnglishLocalizedResourcesPendingSave( Package ) )
					{
						// Figure out localized package name and file name.
						FFilename LocalizedPackageName		= FFilename(DstFilename.GetBaseFilename() + LOCALIZED_SEEKFREE_SUFFIX).GetLocalizedFilename();
						FFilename LocalizedPacakgeFilename	= DstFilename.GetPath() + PATH_SEPARATOR + LocalizedPackageName + TEXT(".") + DstFilename.GetExtension();

						// Create a localized package and disallow lazy loading as we're seek free.
						UPackage* LocalizedPackage = CreatePackage( NULL, *LocalizedPackageName );
						LocalizedPackage->PackageFlags |= PKG_Cooked;
						LocalizedPackage->PackageFlags |= PKG_DisallowLazyLoading;
						LocalizedPackage->PackageFlags |= PKG_RequireImportsAlreadyLoaded;
						LocalizedPackage->PackageFlags |= PKG_ServerSideOnly;
						// fully compressed packages should not be marked with the PKG_StoreCompressed flag, as that will compress
						// on the export level, not the package level
						if (bShouldBeFullyCompressed)
						{
							LocalizedPackage->PackageFlags |= PKG_StoreFullyCompressed;
						}
						else
						{
							LocalizedPackage->PackageFlags |= PKG_StoreCompressed;
						}
						// Create object references within package.
						UObjectReferencer* ObjectReferencer = ConstructObject<UObjectReferencer>( UObjectReferencer::StaticClass(), LocalizedPackage, NAME_None, RF_Cooked );

						// Reference all localized audio with the appropriate flags by the helper object.
						for( FObjectIterator It; It; ++It )
						{
							UObject* Object = *It;
							if( Object->IsLocalizedResource() )
							{
								if( Object->HasAnyFlags(RF_ForceTagExp) )
								{
									ObjectReferencer->ReferencedObjects.AddItem( Object );
								}
								// We don't localize objects that are part of the map package as we can't easily avoid duplication.
								else if( Object->IsIn( Package ) )
								{
									warnf( NAME_Warning, TEXT("Localized resources cannot reside in map packages! Please move %s."), *Object->GetFullName() );
								}
							}
						}

						// Save localized assets referenced by object referencer.
						check( bStripEverythingButTextures == FALSE );
						SaveCookedPackage( LocalizedPackage, ObjectReferencer, RF_Standalone, *LocalizedPacakgeFilename, FALSE);

						// Prevent objects from being saved into seekfree package.
						for( INT ResourceIndex=0; ResourceIndex<ObjectReferencer->ReferencedObjects.Num(); ResourceIndex++ )
						{
							UObject* Object = ObjectReferencer->ReferencedObjects(ResourceIndex);
							Object->ClearFlags( RF_ForceTagExp );
							// Avoid object from being put into subsequent script or map packages.
							if( bIsLocalizedScriptPackage && Object->HasAnyFlags( RF_TagExp ) )
							{
								Object->SetFlags( RF_MarkedByCooker );
							}
						}
					}
				}

				// Save the cooked package if wanted.
				if( !World || !bSkipSavingMaps )
				{
					UObject* Base = World;
					if (bIsCombinedStartupPackage)
					{
						check(CombinedStartupReferencer);
						Base = CombinedStartupReferencer;
					}
					else if (bIsStandaloneSeekFreePackage)
					{
						check(StandaloneSeekFreeReferencer);
						Base = StandaloneSeekFreeReferencer;
					}

					SaveCookedPackage( Package, 
						Base, 
						RF_Standalone, 
						*DstFilename, 
						bStripEverythingButTextures,
						bShouldBeSeekFree, // should we clean up objects for next packages
						bIsNativeScriptFile || bIsCombinedStartupPackage); // should we mark saved objects to not be saved again

				}

				// NULL out shader cache reference before garbage collection to avoid dangling pointer.
				ShaderCache = NULL;
			}
			else
			{
				warnf( NAME_Error, TEXT("Failed loading %s"), *SrcFilename );
			}
		}

		// for Windows, we just copy over the (now up to date) shader caches
		if (Platform == PLATFORM_Windows)
		{
			TArray<FString> PackageList = GPackageFileCache->GetPackageFileList();
			for( INT PackageIndex = 0; PackageIndex < PackageList.Num(); PackageIndex++ )
			{
				// look for PC shaders in the shader cache
				const FFilename&	SrcFilename	= PackageList(PackageIndex);
				UBOOL bIsPCShaderCacheFile = FString(*SrcFilename).ToUpper().InStr( TEXT("SHADERCACHE-PC-D3D") ) != INDEX_NONE;

				// operate on PC shader cache files only
				if (bIsPCShaderCacheFile)
				{
					// get top level directory under appGameDir of the source (..\ExampleGame\Content\)
					FFilename AfterGameDir = SrcFilename.Right(SrcFilename.Len() - appGameDir().Len());
					FFilename AfterTopLevelDir = AfterGameDir.Right(AfterGameDir.Len() - AfterGameDir.InStr(PATH_SEPARATOR));
					FFilename DstFilename = CookedDir + AfterTopLevelDir;

					warnf(NAME_Log, TEXT("Copying PC shader cache %s"), *SrcFilename);
					// load the shader cache package
					UPackage* Package = UObject::LoadPackage(NULL, *SrcFilename, LOAD_None);

					// find the shader cache object
					UShaderCache* ShaderCache = FindObject<UShaderCache>(Package, TEXT("CacheObject"));
					check(ShaderCache);

					// save code copied from SaveLocalShaderCache
					Package->PackageFlags |= PKG_ServerSideOnly;
					UObject::SavePackage(Package, ShaderCache, 0, *DstFilename);
				}
			}
		}

		// Print out detailed stats
		TotalTime = appSeconds() - TotalTime;		
		warnf( NAME_Log, TEXT("") );
		warnf( NAME_Log, TEXT("TotalTime                                %7.2f seconds"), TotalTime );
		warnf( NAME_Log, TEXT("  LoadSectionPackages                    %7.2f seconds"), LoadSectionPackagesTime );
		warnf( NAME_Log, TEXT("  LoadNativePackages                     %7.2f seconds"), LoadNativePackagesTime );
		warnf( NAME_Log, TEXT("  LoadDependencies                       %7.2f seconds"), LoadDependenciesTime );
		warnf( NAME_Log, TEXT("  LoadPackages                           %7.2f seconds"), LoadPackagesTime );
		warnf( NAME_Log, TEXT("  LoadPerMapPackages                     %7.2f seconds"), LoadPerMapPackagesTime );
		warnf( NAME_Log, TEXT("  LoadCombinedStartupPackages            %7.2f seconds"), LoadCombinedStartupPackagesTime );
		warnf( NAME_Log, TEXT("  CookTime;                              %7.2f seconds"), CookTime );
		warnf( NAME_Log, TEXT("    CookPhysics                          %7.2f seconds"), CookPhysicsTime );
		warnf( NAME_Log, TEXT("    CookTexture                          %7.2f seconds"), CookTextureTime );
		warnf( NAME_Log, TEXT("    CookSound                            %7.2f seconds"), CookSoundTime );
		warnf( NAME_Log, TEXT("    CookMovie                            %7.2f seconds"), CookMovieTime );
		warnf( NAME_Log, TEXT("    CookStrip                            %7.2f seconds"), CookStripTime );
		warnf( NAME_Log, TEXT("    CookSkeletalMesh                     %7.2f seconds"), CookSkeletalMeshTime );
		warnf( NAME_Log, TEXT("    CookStaticMesh                       %7.2f seconds"), CookStaticMeshTime );
		warnf( NAME_Log, TEXT("  PackageSave                            %7.2f seconds"), PackageSaveTime );
		warnf( NAME_Log, TEXT("  PrepareForSaving                       %7.2f seconds"), PrepareForSavingTime );
		warnf( NAME_Log, TEXT("    PrepareForSavingTexture              %7.2f seconds"), PrepareForSavingTextureTime );
		warnf( NAME_Log, TEXT("    PrepareForSavingTerrain              %7.2f seconds"), PrepareForSavingTerrainTime );
		warnf( NAME_Log, TEXT("    PrepareForSavingMaterial             %7.2f seconds"), PrepareForSavingMaterialTime );
		warnf( NAME_Log, TEXT("    PrepareForSavingMaterialInterface    %7.2f seconds"), PrepareForSavingMaterialInterfaceTime );
		warnf( NAME_Log, TEXT("  CollectGarbageAndVerify                %7.2f seconds"), CollectGarbageAndVerifyTime );

		// Frees DLL handles and deletes cooker objects.
		Cleanup();
	}

	// generate SHA hashes if desired
	if (bGenerateSHAHashes)
	{
		// add the fully loaded packages to the list to verify (since we fully load the memory for
		// them, it is not crazy to verify the data at runtime) if we are going to preload anythinhg
		UBOOL bPreloadPackagesFromMemory = FALSE;
		GConfig->GetBool(TEXT("Engine.SeekFree"), TEXT("bPreloadPackagesFromMemory"), bPreloadPackagesFromMemory, PlatformEngineConfigFilename);

		if (bPreloadPackagesFromMemory)
		{
			checkf(Platform != PLATFORM_Windows, TEXT("Genering SHA files for Windows with fully compressed pacakges is currently not supported because of .xxx extension restriction. Support will need to be added!"));

			// get the startup packages
			TArray<FString> StartupPackages;
			appGetAllPotentialStartupPackageNames(StartupPackages, PlatformEngineConfigFilename);

			// turn them into file names that can be opened
			for (INT PackageIndex = 0; PackageIndex < StartupPackages.Num(); PackageIndex++)
			{
				FilesForSHA.AddItem(*(CookedDir + (FFilename(StartupPackages(PackageIndex)).GetBaseFilename()) + TEXT(".xxx")));
			}
		}

		warnf(TEXT("Generating Hashes.sha:"));
		// open the output file to store all the hashes
		FArchive* SHAWriter = GFileManager->CreateFileWriter(*(appGameDir() + TEXT("Build") PATH_SEPARATOR TEXT("Hashes.sha")));

		for (INT SHAIndex = 0; SHAIndex < FilesForSHA.Num(); SHAIndex++)
		{
			FString& Filename = FilesForSHA(SHAIndex);
			TArray<BYTE> Contents;

			// look to see if the file was fully compressed
			FString UncompressedSizeStr;
			if (appLoadFileToString(UncompressedSizeStr, *(Filename + TEXT(".uncompressed_size"))))
			{
				// if it was, we need to generate the hash on the uncomprseed
				INT UncompressedSize = appAtoi(*UncompressedSizeStr);

				// read and uncompress the data
				FArchive* Reader = GFileManager->CreateFileReader(*Filename);
				if (Reader)
				{
					warnf(TEXT("  Decompressing %s"), *Filename);
					Contents.Add(UncompressedSize);
#if WITH_LZO
					Reader->SerializeCompressed(Contents.GetData(), 0, COMPRESS_LZO);
#else	//#if WITH_LZO
					Reader->SerializeCompressed(Contents.GetData(), 0, COMPRESS_ZLIB);
#endif	//#if WITH_LZO
					delete Reader;
				}
			}
			else
			{
				// otherwise try to load the file
				appLoadFileToArray(Contents, *Filename);
			}

			// is there any data to hash?
			if (Contents.Num() != 0)
			{
				warnf(TEXT("  Hashing %s"), *Filename);
				// if we loaded it, generate the hash
				BYTE Hash[20];
				FSHA1::HashBuffer(Contents.GetData(), Contents.Num(), Hash);

				// write out the filename (including trailing 0)
				SHAWriter->Serialize(TCHAR_TO_ANSI(*Filename), Filename.Len() + 1);

				// write out hash
				SHAWriter->Serialize(Hash, sizeof(Hash));
			}
		}

		// close the file
		delete SHAWriter;
	}

	return 0;
}
IMPLEMENT_CLASS(UCookPackagesCommandlet)

/*-----------------------------------------------------------------------------
	UCookedBulkDataInfoContainer implementation.
-----------------------------------------------------------------------------*/

/**
 * Create an instance of this class given a filename. First try to load from disk and if not found
 * will construct object and store the filename for later use during saving.
 *
 * @param	Filename	Filename to use for serialization
 * @return	instance of the container associated with the filename
 */
UCookedBulkDataInfoContainer* UCookedBulkDataInfoContainer::CreateInstance( const TCHAR* Filename )
{
	UCookedBulkDataInfoContainer* Instance = NULL;

	// Find it on disk first.
	if( !Instance )
	{
		// Try to load the package.
		UPackage* Package = LoadPackage( NULL, Filename, LOAD_NoWarn );

		// Find in memory if package loaded successfully.
		if( Package )
		{
			Instance = FindObject<UCookedBulkDataInfoContainer>( Package, TEXT("CookedBulkDataInfoContainer") );
		}
	}

	// If not create an instance.
	if( !Instance )
	{
		UPackage* Package = UObject::CreatePackage( NULL, NULL );
		Instance = ConstructObject<UCookedBulkDataInfoContainer>( 
							UCookedBulkDataInfoContainer::StaticClass(),
							Package,
							TEXT("CookedBulkDataInfoContainer")
							);
		check( Instance );
	}
	// Keep the filename around for serialization and add to root to prevent garbage collection.
	Instance->Filename = Filename;
	Instance->AddToRoot();

	return Instance;
}

/**
 * Saves the data to disk.
 */
void UCookedBulkDataInfoContainer::SaveToDisk()
{
	// Save package to disk using filename that was passed to CreateInstance.
	UObject::SavePackage( GetOutermost(), this, 0, *Filename );
}

/**
 * Serialize function.
 *
 * @param	Ar	Archive to serialize with.
 */
void UCookedBulkDataInfoContainer::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << CookedBulkDataInfoMap;
}

/**
 * Stores the bulk data info after creating a unique name out of the object and name of bulk data
 *
 * @param	Object					Object used to create unique name
 * @param	BulkDataName			Unique name of bulk data within object, e.g. MipLevel_3
 * @param	CookedBulkDataInfo		Data to store
 */
void UCookedBulkDataInfoContainer::SetInfo( UObject* Object, const TCHAR* BulkDataName, const FCookedBulkDataInfo& CookedBulkDataInfo )
{
	FString UniqueName = Object->GetPathName() + TEXT(".") + BulkDataName;
	CookedBulkDataInfoMap.Set( *UniqueName, CookedBulkDataInfo );
}

/**
 * Retrieves previously stored bulk data info data for object/ bulk data name combination.
 *
 * @param	Object					Object used to create unique name
 * @param	BulkDataName			Unique name of bulk data within object, e.g. MipLevel_3
 * @return	cooked bulk data info if found, NULL otherwise
 */
FCookedBulkDataInfo* UCookedBulkDataInfoContainer::RetrieveInfo( UObject* Object, const TCHAR* BulkDataName )
{
	FString UniqueName = Object->GetPathName() + TEXT(".") + BulkDataName;
	FCookedBulkDataInfo* CookedBulkDataInfo = CookedBulkDataInfoMap.Find( *UniqueName );
	return CookedBulkDataInfo;
}

/**
 * Dumps all info data to the log.
 */
void UCookedBulkDataInfoContainer::DumpInfos()
{
	for( TMap<FString,FCookedBulkDataInfo>::TIterator It(CookedBulkDataInfoMap); It; ++It )
	{
		const FString&				UniqueName	= It.Key();
		const FCookedBulkDataInfo&	Info		= It.Value();

		debugf( TEXT("%s Flags: %x  ElementCount: %i  Offset in File: %i  Size on Disk: %i"), 
			*UniqueName, 
			Info.SavedBulkDataFlags, 
			Info.SavedElementCount,
			Info.SavedBulkDataOffsetInFile,
			Info.SavedBulkDataSizeOnDisk );
	}
}

/**
 * Gathers bulk data infos of objects in passed in Outer.
 *
 * @param	Outer	Outer to use for object traversal when looking for bulk data
 */
void UCookedBulkDataInfoContainer::GatherCookedInfos( UObject* Outer )
{
	// Iterate over all objects in passed in outer and store the bulk data info for types supported.
	for( TObjectIterator<UObject> It; It; ++It )
	{
		UObject* Object = *It;
		// Make sure we're in the same package.
		if( Object->IsIn( Outer ) )
		{
			// Texture handling.
			UTexture2D* Texture2D = Cast<UTexture2D>( Object );		
			if( Texture2D )
			{
				check( Texture2D->HasAllFlags( RF_Cooked ) );
				for( INT MipLevel=0; MipLevel<Texture2D->Mips.Num(); MipLevel++ )
				{
					FTexture2DMipMap& Mip = Texture2D->Mips(MipLevel);
					// Only store if we've actually saved the bulk data in the archive.			
					if( !Mip.Data.IsStoredInSeparateFile() )
					{
						FCookedBulkDataInfo Info;
						Info.SavedBulkDataFlags			= Mip.Data.GetSavedBulkDataFlags();
						Info.SavedElementCount			= Mip.Data.GetSavedElementCount();
						Info.SavedBulkDataOffsetInFile	= Mip.Data.GetSavedBulkDataOffsetInFile();
						Info.SavedBulkDataSizeOnDisk	= Mip.Data.GetSavedBulkDataSizeOnDisk();
						SetInfo( Texture2D, *FString::Printf(TEXT("MipLevel_%i"),MipLevel), Info );
					}
				}
			}
		}
	}
}

IMPLEMENT_CLASS(UCookedBulkDataInfoContainer);
