/*=============================================================================
	Texture2D.cpp: Implementation of UTexture2D.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UTexture2D);

/** First streamable texture link. Not handled by GC as BeginDestroy automatically unlinks.	*/
TLinkedList<UTexture2D*>* UTexture2D::FirstStreamableLink = NULL;
/** Current streamable texture link for iteration over textures. Not handled by GC as BeginDestroy automatically unlinks. */
TLinkedList<UTexture2D*>* UTexture2D::CurrentStreamableLink = NULL;
/** Number of streamable textures. */
INT UTexture2D::NumStreamableTextures = 0;


/**
 * Special serialize function passing the owning UObject along as required by FUnytpedBulkData
 * serialization.
 *
 * @param	Ar		Archive to serialize with
 * @param	Owner	UObject this structure is serialized within
 */
void FTexture2DMipMap::Serialize( FArchive& Ar, UObject* Owner )
{
	if( Ar.Ver() < VER_REPLACED_LAZY_ARRAY_WITH_UNTYPED_BULK_DATA )
	{
		Data.SerializeLikeLazyArray( Ar, Owner );
	}
	else
	{
		Data.Serialize( Ar, Owner );
	}
	Ar << SizeX;
	Ar << SizeY;
}

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UTexture2D::InitializeIntrinsicPropertyValues()
{
	SRGB = TRUE;
	UnpackMin[0] = UnpackMin[1] = UnpackMin[2] = UnpackMin[3] = 0.0f;
	UnpackMax[0] = UnpackMax[1] = UnpackMax[2] = UnpackMax[3] = 1.0f;
}

void UTexture2D::LegacySerialize(FArchive& Ar)
{
	if(Ar.Ver() < VER_RENDERING_REFACTOR)
	{
		INT OldFormat, OldSizeX, OldSizeY;
		Ar << OldSizeX << OldSizeY << OldFormat;
		SizeX	= OldSizeX;
		SizeY	= OldSizeY;
		Format	= OldFormat;
	}
	Mips.Serialize( Ar, this );
}

void UTexture2D::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	LegacySerialize(Ar);

	// Keep track of the fact that we have been loaded from a persistent archive as it's a prerequisite of
	// being streamable.
	if( Ar.IsLoading() && Ar.IsPersistent() )
	{
		bHasBeenLoadedFromPersistentArchive = TRUE;
	}
}

/**
 * Returns a reference to the global list of streamable textures.
 *
 * @return reference to global list of streamable textures.
 */
TLinkedList<UTexture2D*>*& UTexture2D::GetStreamableList()
{
	return FirstStreamableLink;
}

/**
 * Returns a reference to the current streamable link.
 *
 * @return reference to current streamable link
 */
TLinkedList<UTexture2D*>*& UTexture2D::GetCurrentStreamableLink()
{
	// Use first if current link wasn't set yet or has been reset.
	if( !CurrentStreamableLink )
	{
		CurrentStreamableLink = FirstStreamableLink;
	}
	return CurrentStreamableLink;
}

/**
 * Links texture to streamable list and updates streamable texture count.
 */
void UTexture2D::LinkStreaming()
{
	StreamableTexturesLink = TLinkedList<UTexture2D*>(this);
	StreamableTexturesLink.Link( GetStreamableList() );
	NumStreamableTextures++;
}

/**
 * Unlinks texture from streamable list, resets CurrentStreamableLink if it matches
 * StreamableTexturesLink and also updates the streamable texture count.
 */
void UTexture2D::UnlinkStreaming()
{
	// Reset current streamable link if it equals current texture.
	if( &StreamableTexturesLink == CurrentStreamableLink )
	{
		CurrentStreamableLink = NULL;
	}

	// Only decrease count if texture was linked.
	if( StreamableTexturesLink.IsLinked() )
	{
		NumStreamableTextures--;
	}

	// Unlink from list.
	StreamableTexturesLink.Unlink();
}
	
/**
 * Returns the number of streamable textures, maintained by link/ unlink code
 *
 * @return	Number of streamable textures
 */
INT UTexture2D::GetNumStreamableTextures()
{
	return NumStreamableTextures;
}

/**
 * Called after object and all its dependencies have been serialized.
 */
void UTexture2D::PostLoad()
{
#if XBOX
#if !FINAL_RELEASE
	// make sure we have at least enough bulk mip data available
	// to load the miptail
	INT FirstAvailableMip=INDEX_NONE;
	for( INT MipIndex=Mips.Num()-1; MipIndex >= 0; MipIndex-- )
	{
		if( Mips.IsValidIndex(MipIndex) &&
			Mips(MipIndex).Data.IsAvailableForUse() )
		{
			FirstAvailableMip = MipIndex;
			break;
		}
	}	
	checkf( FirstAvailableMip != INDEX_NONE, TEXT("No mips available: %s"), *GetFullName() );
	checkf( FirstAvailableMip >= MipTailBaseIdx || MipTailBaseIdx >= Mips.Num(), TEXT("Not enough mips (%d:%d) available: %s"), FirstAvailableMip, MipTailBaseIdx, *GetFullName() );
#endif
#else
	if( !HasAnyFlags(RF_Cooked) )
	{
		// no packed mips on other platforms
		MipTailBaseIdx = Max(0,Mips.Num()-1);
	}
#endif

	// Route postload, which will update bIsStreamable as UTexture::PostLoad calls UpdateResource.
	Super::PostLoad();
}

/**
 * Creates a new resource for the texture, and updates any cached references to the resource.
 */
void UTexture2D::UpdateResource()
{
	// Make sure there are no pending requests in flight.
	while( UpdateStreamingStatus() == TRUE )
	{
		// Give up timeslice.
		appSleep(0);
	}

	// Route to super.
	Super::UpdateResource();
}

#if !CONSOLE
/**
 * Changes the linker and linker index to the passed in one. A linker of NULL and linker index of INDEX_NONE
 * indicates that the object is without a linker.
 *
 * @param LinkerLoad	New LinkerLoad object to set
 * @param LinkerIndex	New LinkerIndex to set
 */
void UTexture2D::SetLinker( ULinkerLoad* LinkerLoad, INT LinkerIndex )
{
	// We never change linkers in the case of seekfree loading though will reset them/ set them to NULL
	// and don't want to load the texture data in this case.
	if( GUseSeekFreeLoading )
	{
		// Route the call to change the linker.
		Super::SetLinker( LinkerLoad, LinkerIndex );
	}
	else
	{
		// Only update resource if linker changes.
		UBOOL bRequiresUpdate = FALSE;
		if( LinkerLoad != GetLinker() )
		{
			bRequiresUpdate = TRUE;
		}

		// Route the call to change the linker.
		Super::SetLinker( LinkerLoad, LinkerIndex );

		// Changing the linker requires re-creating the resource to make sure streaming behavior is right.
		if( bRequiresUpdate && !HasAnyFlags( RF_Unreachable | RF_BeginDestroyed | RF_NeedLoad | RF_NeedPostLoad ) )
		{
			// Reset to FALSE as Serialize is being called after SetLinker in the case of it being reloaded from disk
			// and we want this to be FALSE if we are not going to serialize it again.
			bHasBeenLoadedFromPersistentArchive = FALSE;

			// Update the resource.
			UpdateResource();

			// Unlink texture...
			UnlinkStreaming();

			// Can't be streamable as we just changed the linker outside of regular load.
			check( !bIsStreamable );
		}
	}
}
#endif

/**
 * Called after the garbage collection mark phase on unreachable objects.
 */
void UTexture2D::BeginDestroy()
{
	// Route BeginDestroy.
	Super::BeginDestroy();

	// Cancel any in flight IO requests
	CancelPendingMipChangeRequest();

	// Safely unlink texture from list of streamable ones.
	UnlinkStreaming();
}

//@warning:	Do NOT call Compress from within Init as it could cause an infinite recursion.
void UTexture2D::Init(UINT InSizeX,UINT InSizeY,EPixelFormat InFormat)
{
	// Check that the dimensions are powers of two and evenly divisible by the format block size.
//	check(!(InSizeX & (InSizeX - 1)));
//	check(!(InSizeY & (InSizeY - 1)));
	check(!(InSizeX % GPixelFormats[InFormat].BlockSizeX));
	check(!(InSizeY % GPixelFormats[InFormat].BlockSizeY));

	// We need to flush all rendering commands before we can modify a textures' mip array, size or format.
	if( Mips.Num() )
	{
		// Flush rendering commands.
		FlushRenderingCommands();
		// Start with a clean plate.
		Mips.Empty();
	}

	SizeX = InSizeX;
	SizeY = InSizeY;
	Format = InFormat;

	// Allocate first mipmap.
	FTexture2DMipMap* MipMap = new(Mips) FTexture2DMipMap;

	MipMap->SizeX = SizeX;
	MipMap->SizeY = SizeY;

	SIZE_T ImageSize = CalculateImageBytes(SizeX,SizeY,0,(EPixelFormat)Format);

	MipMap->Data.Lock( LOCK_READ_WRITE );
	MipMap->Data.Realloc( ImageSize );
	MipMap->Data.Unlock();
}

/** 
 * Returns a one line description of an object for viewing in the generic browser
 */
FString UTexture2D::GetDesc()
{
	return FString::Printf( TEXT("%s %dx%d[%s%s]"), NeverStream ? TEXT("NeverStreamed") : TEXT("Streamed"), SizeX, SizeY, GPixelFormats[Format].Name, DeferCompression ? TEXT("*") : TEXT(""));
}

/** 
 * Returns detailed info to populate listview columns
 */
FString UTexture2D::GetDetailedDescription( INT InIndex )
{
	FString Description = TEXT( "" );
	switch( InIndex )
	{
	case 0:
		Description = FString::Printf( TEXT( "%dx%d" ), SizeX, SizeY );
		break;
	case 1:
		Description = GPixelFormats[Format].Name;
		if( DeferCompression )
		{
			Description += TEXT( "*" );
		}
		break;
	case 2:
		{
			TArray<FString> TextureGroupNames = FTextureLODSettings::GetTextureGroupNames();
			if( LODGroup < TextureGroupNames.Num() )
			{
				Description = TextureGroupNames(LODGroup);
			}
		}
		break;
	case 3:
		Description = NeverStream ? TEXT( "NeverStreamed" ) : TEXT( "Streamed" );
		break;
	default:
		break;
	}
	return( Description );
}

/**
 * Returns whether the texture is ready for streaming aka whether it has had InitRHI called on it.
 *
 * @return TRUE if initialized and ready for streaming, FALSE otherwise
 */
UBOOL UTexture2D::IsReadyForStreaming()
{
	// A value < 0 indicates that the resource is still in the process of being created 
	// for the first time.
	INT RequestStatus = PendingMipChangeRequestStatus.GetValue();
	return RequestStatus != TEXTURE_PENDING_INITIALIZATION;
}

/**
 * Updates the streaming status of the texture and performs finalization when appropriate. The function returns
 * TRUE while there are pending requests in flight and updating needs to continue.
 *
 * @return	TRUE if there are requests in flight, FALSE otherwise
 */
UBOOL UTexture2D::UpdateStreamingStatus()
{
	UBOOL	bHasPendingRequestInFlight	= TRUE;
	INT		RequestStatus				= PendingMipChangeRequestStatus.GetValue();

	// No pending request in flight if resident and requested mip counts match.
	if( ResidentMips == RequestedMips )
	{
		check( RequestStatus == TEXTURE_READY_FOR_REQUESTS || RequestStatus == TEXTURE_PENDING_INITIALIZATION );
		check( !bHasCancelationPending );
		bHasPendingRequestInFlight = FALSE;
	}
	// Pending request in flight, though we might be able to finish it.
	else
	{
		// Update part of mip change request is done, time to kick off finalization.
		if( RequestStatus == TEXTURE_READY_FOR_FINALIZATION )
		{
			// Finalize mip request, aka unlock textures involved, perform switcheroo and free original one.
			check( Resource );
			FTexture2DResource* Texture2DResource = (FTexture2DResource*) Resource;
			Texture2DResource->BeginFinalizeMipCount();				
		}
		// Finalization finished. We're done.
		else if( RequestStatus == TEXTURE_READY_FOR_REQUESTS )
		{
			// We have a cancelation request pending which means we did not change anything.
			if( bHasCancelationPending )
			{
				// Reset requested mip count to resident one as we no longer have a request outstanding.
				RequestedMips = ResidentMips;
				// We're done canceling the request.
				bHasCancelationPending = FALSE;
			}
			// Resident mips now match requested ones.
			else
			{
				ResidentMips = RequestedMips;
			}
			bHasPendingRequestInFlight = FALSE;
		}
	}
	return bHasPendingRequestInFlight;
}

/**
 * Tries to cancel a pending mip change request. Requests cannot be canceled if they are in the
 * finalization phase.
 *
 * @param	TRUE if cancelation was successful, FALSE otherwise
 */
UBOOL UTexture2D::CancelPendingMipChangeRequest()
{
	INT RequestStatus = PendingMipChangeRequestStatus.GetValue();
	// Nothing to do if we're already in the process of canceling the request.
	if( !bHasCancelationPending )
	{
		// We can only cancel textures that either have a pending request in flight or are pending finalization.
		if( RequestStatus >= TEXTURE_READY_FOR_FINALIZATION )
		{
			// We now have a cancelation pending!
			bHasCancelationPending = TRUE;

			// Begin async cancelation of current request.
			check(Resource);
			FTexture2DResource* Texture2DResource = (FTexture2DResource*) Resource;
			Texture2DResource->BeginCancelUpdate();
		}
		// Texture is either pending finalization or doesn't have a request in flight.
		else
		{
		}
	}
	return bHasCancelationPending;
}

/**
 * Returns the size of this texture in bytes if it had MipCount miplevels streamed in.
 *
 * @param	MipCount	Number of toplevel mips to calculate size for
 * @return	size of top mipcount mips in bytes
 */
INT UTexture2D::GetSize( INT MipCount ) const
{
	INT Size		= 0;
	// Figure out what the first mip to use is.
	INT FirstMip	= Max( 0, Mips.Num() - MipCount );
	// Iterate over all relevant miplevels and sum up their size.
	for( INT MipIndex=FirstMip; MipIndex<Mips.Num(); MipIndex++ )
	{
		const FTexture2DMipMap& MipMap = Mips(MipIndex);
		// The bulk data size matches up with the size in video memory and in the case of consoles even takes
		// alignment restrictions into account as the data is expected to be a 1:1 copy including size.
		Size += MipMap.Data.GetBulkDataSize();
	}
	return Size;
}

FTextureResource* UTexture2D::CreateResource()
{
	FString Filename	= TEXT("");	
	// This might be a new texture that has never been loaded, a texture which just had it's linker detached
	// as it was renamed or saved or a texture that was re-imported and therefore has a linker but has never
	// been loaded from disk.
	bIsStreamable		= FALSE;

	// We can only stream textures that have been loaded from "disk" (aka a persistent archive).
	if( bHasBeenLoadedFromPersistentArchive 
	// Disregard textures that are marked as not being streamable.
	&&	!NeverStream
	// We don't stream UI textures.
	&&	LODGroup != TEXTUREGROUP_UI )
	{
		// Use the linker's filename if it exists. We can't do this if we're using seekfree loading as the linker might potentially
		// be the seekfree package's linker and hence be the wrong filename.
		if( GetLinker() && !(GetLinker()->LinkerRoot->PackageFlags & PKG_Cooked))
		{
			bIsStreamable	= TRUE;
			Filename		= GetLinker()->Filename;
		}
		else
		// Look up the filename in the package file cache. We cannot do this in the Editor as the texture might have
		// been newly created and therefore never has been saved. Yet, if it was created in an already existing package
		// FindPackageFile would return TRUE! There is also the edge case of creating a texture in a package that 
		// hasn't been saved in which case FindPackageFile would return FALSE.
		if( !GIsEditor && GPackageFileCache->FindPackageFile( *GetOutermost()->GetName(), NULL, Filename, NULL ) )
		{
			// Found package file. A case for a streamable texture without a linker attached are objects that were
			// forced into the exports table.
			bIsStreamable	= TRUE;
		}
		// Package file not found.
		else
		{
			// now, check by Guid, in case the package was downloaded (we don't use the Guid above, because when 
			// checking by Guid it must serialize the header from the package, which can be slow)
			FGuid PackageGuid = GetOutermost()->GetGuid();
			if( !GIsEditor && GPackageFileCache->FindPackageFile( *GetOutermost()->GetName(), &PackageGuid, Filename, NULL ) )
			{
				bIsStreamable = TRUE;
			}
			else
			{
				// This should only ever happen in the Editor as the game shouldn't be creating texture at run-time.
				checkf(GIsEditor, TEXT("Cannot create textures at run-time in the game [unable to find %s]"), *GetOutermost()->GetName());
			}
		}
	}

	// Only allow streaming if enabled on the engine level.
	bIsStreamable = bIsStreamable && (!GEngine || GEngine->bUseTextureStreaming);

	// number of levels in the packed miptail
	INT NumMipTailLevels = Max(0,Mips.Num() - MipTailBaseIdx);

	// Handle corrupted textures :(
	if( Mips.Num() == 0 )
	{
		debugf( NAME_Error, TEXT("%s contains no miplevels! Please delete."), *GetFullName() );
		ResidentMips	= 0;
		RequestedMips	= 0;
	}
	else
	{
		// Handle streaming textures.
		if( bIsStreamable )
		{
			// Only request lower miplevels and let texture streaming code load the rest.
			RequestedMips	= GMinTextureResidentMipCount;
		}
		// Handle non- streaming textures.
		else
		{
			// Request all miplevels allowed by device. LOD settings are taken into account below.
			RequestedMips	= GMaxTextureMipCount;
		}

		// Take LOD bias into account.
		RequestedMips	= Min( Mips.Num() - GetCachedLODBias(), RequestedMips );
		// Make sure that we at least load the mips that reside in the packed miptail
		RequestedMips	= Max( RequestedMips, NumMipTailLevels );
		RequestedMips	= Max( RequestedMips, 1 );
		ResidentMips	= RequestedMips;
	}

	// Unlink and relink if streamable.
	UnlinkStreaming();
	if( bIsStreamable )
	{
		LinkStreaming();
	}

	FTexture2DResource* Texture2DResource = NULL;

	// Create and return 2D resource if there are any miplevels.
	if( RequestedMips > 0 )
	{
		Texture2DResource = new FTexture2DResource( this, RequestedMips, Filename );
	}

	return Texture2DResource;
}

/**
 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
 *
 * @return size of resource as to be displayed to artists/ LDs in the Editor.
 */
INT UTexture2D::GetResourceSize()
{
	FArchiveCountMem CountBytesSize( this );
	INT ResourceSize = CountBytesSize.GetNum();
	for( INT MipIndex=0; MipIndex<Mips.Num(); MipIndex++ )
	{
		ResourceSize += Mips(MipIndex).Data.GetBulkDataSize();
	}
	return ResourceSize;
}

/**
 * Whether all miplevels of this texture have been fully streamed in, LOD settings permitting.
 */
UBOOL UTexture2D::IsFullyStreamedIn()
{
	// Non-streamable textures are considered to be fully streamed in.
	UBOOL bIsFullyStreamedIn = TRUE;
	if( bIsStreamable )
	{
		// Calculate maximum number of mips potentially being resident based on LOD settings and device max texture count.
		INT MaxResidentMips = Max( 1, Min( Mips.Num() - GetCachedLODBias(), GMaxTextureMipCount ) );
		// >= as LOD settings can change dynamically and we consider a texture that is about to loose miplevels to still
		// be fully streamed.
		bIsFullyStreamedIn = ResidentMips >= MaxResidentMips;
	}
	return bIsFullyStreamedIn;
}

/*-----------------------------------------------------------------------------
	FTexture2DResource implementation.
-----------------------------------------------------------------------------*/

/**
 * Minimal initialization constructor.
 *
 * @param InOwner			UTexture2D which this FTexture2DResource represents.
 * @param InitialMipCount	Initial number of miplevels to upload to card
 * @param InFilename		Filename to read data from
 */
FTexture2DResource::FTexture2DResource( UTexture2D* InOwner, INT InitialMipCount, const FString& InFilename )
:	Owner( InOwner )
,	Filename( InFilename )
,	IORequestCount( 0 )
#if STATS
,	TextureSize( 0 )
,	IntermediateTextureSize( 0 )
#endif
{
	// First request to create the resource. Decrement the counter to signal that the resource is not ready
	// for streaming yet.
	if( Owner->PendingMipChangeRequestStatus.GetValue() == TEXTURE_READY_FOR_REQUESTS )
	{
		Owner->PendingMipChangeRequestStatus.Decrement();
	}
	// This can happen if the resource is re-created without ever having had InitRHI called on it.
	else
	{
		check(Owner->PendingMipChangeRequestStatus.GetValue() == TEXTURE_PENDING_INITIALIZATION );
	}

	check(InitialMipCount>0);
	check(ARRAY_COUNT(MipData)>=GMaxTextureMipCount);
	check(InitialMipCount==Owner->ResidentMips);
	check(InitialMipCount==Owner->RequestedMips);

	// Keep track of first miplevel to use.
	FirstMip = Owner->Mips.Num() - InitialMipCount;
	check(FirstMip>=0);
	// texture must be as big as base miptail level
	check(FirstMip<=Owner->MipTailBaseIdx);

	// Retrieve initial bulk data.
	for( INT MipIndex=0; MipIndex<ARRAY_COUNT(MipData); MipIndex++ )
	{
		MipData[MipIndex] = NULL;
		if( MipIndex >= FirstMip && MipIndex < Owner->Mips.Num() ) 
		{
			FTexture2DMipMap& Mip = InOwner->Mips(MipIndex);
			if( Mip.Data.IsAvailableForUse() )
			{
				if( Mip.Data.IsStoredInSeparateFile() )
				{
					debugf( NAME_Error, TEXT("Corrupt texture [%s]! Missing bulk data for MipIndex=%d"),*InOwner->GetFullName(),MipIndex );
				}
				else			
				{
					// Get copy of data, potentially loading array or using already loaded version.
					Mip.Data.GetCopy( &MipData[MipIndex], TRUE );
					check(MipData[MipIndex]);
				}
			}
		}
	}

	STAT( TextureSize = Owner->GetSize( InitialMipCount ); )
}

/**
 * Destructor, freeing MipData in the case of resource being destroyed without ever 
 * having been initialized by the rendering thread via InitRHI.
 */
FTexture2DResource::~FTexture2DResource()
{
	// Make sure we're not leaking memory if InitRHI has never been called.
	for( INT MipIndex=0; MipIndex<ARRAY_COUNT(MipData); MipIndex++ )
	{
		appFree( MipData[MipIndex] );
		MipData[MipIndex] = NULL;
	}
}

/**
 * Called when tNhe resource is initialized. This is only called by the rendering thread.
 */
void FTexture2DResource::InitRHI()
{
	INC_DWORD_STAT_BY( STAT_TextureMemory, TextureSize );
	STAT( check( IntermediateTextureSize == 0 ) );

	check( Owner->PendingMipChangeRequestStatus.GetValue() == TEXTURE_PENDING_INITIALIZATION );
	UINT SizeX = Owner->Mips(FirstMip).SizeX;
	UINT SizeY = Owner->Mips(FirstMip).SizeY;

	// Create the RHI texture.
	DWORD TexCreateFlags = (Owner->SRGB) ? TexCreate_SRGB : 0;
	// if no miptail is available then create the texture without a packed miptail
	if( Owner->MipTailBaseIdx == -1 )
	{
		TexCreateFlags |= TexCreate_NoMipTail;
	}
	Texture2DRHI	= RHICreateTexture2D( SizeX, SizeY, Owner->Format, Owner->RequestedMips, TexCreateFlags );
	TextureRHI		= Texture2DRHI;

	// Read the resident mip-levels into the RHI texture.
	for( INT MipIndex=FirstMip; MipIndex<Owner->Mips.Num(); MipIndex++ )
	{
		if( MipData[MipIndex] != NULL )
		{
			UINT DestPitch;
			void* TheMipData = RHILockTexture2D( Texture2DRHI, MipIndex - FirstMip, TRUE, DestPitch );
			GetData( MipIndex, TheMipData, DestPitch );
			RHIUnlockTexture2D( Texture2DRHI, MipIndex - FirstMip );
		}
	}

	// Create the sampler state RHI resource.
	FSamplerStateInitializerRHI SamplerStateInitializer =
	{
		Owner->Filter == TF_Linear ? SF_AnisotropicLinear : SF_Nearest,
		Owner->AddressX == TA_Wrap ? AM_Wrap : (Owner->AddressX == TA_Clamp ? AM_Clamp : AM_Mirror),
		Owner->AddressY == TA_Wrap ? AM_Wrap : (Owner->AddressY == TA_Clamp ? AM_Clamp : AM_Mirror),
		AM_Wrap
	};
	SamplerStateRHI = RHICreateSamplerState( SamplerStateInitializer );

	// We're done with initialization.
	Owner->PendingMipChangeRequestStatus.Increment();
}

/**
 * Called when the resource is released. This is only called by the rendering thread.
 */
void FTexture2DResource::ReleaseRHI()
{
	DEC_DWORD_STAT_BY( STAT_TextureMemory, TextureSize );
	STAT( check( IntermediateTextureSize == 0 ) );
	check( Owner->PendingMipChangeRequestStatus.GetValue() == TEXTURE_READY_FOR_REQUESTS );
	FTextureResource::ReleaseRHI();
	Texture2DRHI.Release();
}

/** Returns the width of the texture in pixels. */
UINT FTexture2DResource::GetSizeX() const
{
	return Owner->SizeX;
}

/** Returns the height of the texture in pixels. */
UINT FTexture2DResource::GetSizeY() const
{
	return Owner->SizeY;
}

/**
 * Writes the data for a single mip-level into a destination buffer.
 *
 * @param MipIndex		Index of the mip-level to read.
 * @param Dest			Address of the destination buffer to receive the mip-level's data.
 * @param DestPitch		Number of bytes per row
 */
void FTexture2DResource::GetData( UINT MipIndex,void* Dest,UINT DestPitch )
{
	const FTexture2DMipMap& MipMap = Owner->Mips(MipIndex);
	check( MipData[MipIndex] );

#ifndef XBOX
	UINT BlockSizeX = GPixelFormats[Owner->Format].BlockSizeX;	// Block width in pixels
	UINT BlockSizeY = GPixelFormats[Owner->Format].BlockSizeY;	// Block height in pixels
	UINT BlockBytes = GPixelFormats[Owner->Format].BlockBytes;
	UINT NumColumns = (MipMap.SizeX + BlockSizeX - 1) / BlockSizeX;	// Num-of columns in the source data (in blocks)
	UINT NumRows    = (MipMap.SizeY + BlockSizeY - 1) / BlockSizeY;	// Num-of rows in the source data (in blocks)
	UINT SrcPitch   = NumColumns * BlockBytes;		// Num-of bytes per row in the source data

	if ( SrcPitch == DestPitch )
	{
		// Copy data, not taking into account stride!
		appMemcpy( Dest, MipData[MipIndex], MipMap.Data.GetBulkDataSize() );
	}
	else
	{
		// Copy data, taking the stride into account!
		BYTE *Src = (BYTE*) MipData[MipIndex];
		BYTE *Dst = (BYTE*) Dest;
		for ( UINT Row=0; Row < NumRows; ++Row )
		{
			appMemcpy( Dst, Src, SrcPitch );
			Src += SrcPitch;
			Dst += DestPitch;
		}
		check( (PTRINT(Src) - PTRINT(MipData[MipIndex])) == PTRINT(MipMap.Data.GetBulkDataSize()) );
	}
#else
	// Copy data directly into memory - cooking takes care of it being the right format to allow in-place copy.
	appMemcpy( Dest, MipData[MipIndex], MipMap.Data.GetBulkDataSize() );
#endif

	// Free data retrieved via GetCopy inside constructor.
	appFree( MipData[MipIndex] );
	MipData[MipIndex] = NULL;
}

/**
 * Called from the game thread to kick off a change in ResidentMips after modifying
 * RequestedMips.
 */
void FTexture2DResource::BeginUpdateMipCount()
{
	STAT( IntermediateTextureSize = Owner->GetSize( Owner->RequestedMips ); )

	check( Owner->PendingMipChangeRequestStatus.GetValue() == TEXTURE_READY_FOR_REQUESTS );
	// Means that BeginFinalizeMipCount has been called.
	Owner->PendingMipChangeRequestStatus.Increment();
	// Means that FinalizeMipCount should be called.
	Owner->PendingMipChangeRequestStatus.Increment();
	// Means that we're still waiting for task to complete.
	Owner->PendingMipChangeRequestStatus.Increment();

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FUpdateMipCountCommand,
		FTexture2DResource*, Texture2DResource, this,
		{
			Texture2DResource->UpdateMipCount();
		});
}

/**
 * Called from the game thread to kick off finalization of mip change.
 */
void FTexture2DResource::BeginFinalizeMipCount()
{
	check( Owner->PendingMipChangeRequestStatus.GetValue() == TEXTURE_READY_FOR_FINALIZATION );
	// Finalization is now in flight.
	Owner->PendingMipChangeRequestStatus.Decrement();
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FFinalineMipCountCommand,
		FTexture2DResource*, Texture2DResource, this,
		{
			Texture2DResource->FinalizeMipCount();
		});
}

/**
 * Called from the game thread to kick off cancelation of async operations for request.
 */
void FTexture2DResource::BeginCancelUpdate()
{
	check( Owner->PendingMipChangeRequestStatus.GetValue() >= TEXTURE_READY_FOR_FINALIZATION );
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FCancelUpdateCommand,
		FTexture2DResource*, Texture2DResource, this,
		{
			Texture2DResource->CancelUpdate();
		});
}

/**
 * Called from the rendering thread to perform the work to kick off a change in ResidentMips.
 */
void FTexture2DResource::UpdateMipCount()
{
	SCOPE_CYCLE_COUNTER(STAT_RenderingThreadUpdateTime);
	INC_DWORD_STAT_BY( STAT_TextureMemory, IntermediateTextureSize );

	check(Owner->bIsStreamable);
	check(Owner->PendingMipChangeRequestStatus.GetValue()==TEXTURE_REQUEST_IN_FLIGHT);

	FirstMip	= Owner->Mips.Num() - Owner->RequestedMips;
	check(FirstMip>=0);
	
	UINT SizeX	= Owner->Mips(FirstMip).SizeX;
	UINT SizeY	= Owner->Mips(FirstMip).SizeY;

	// Create the RHI texture.
	DWORD TexCreateFlags = (Owner->SRGB) ? TexCreate_SRGB : 0;
	// if no miptail is available then create the texture without a packed miptail
	if( Owner->MipTailBaseIdx == -1 )
	{
		TexCreateFlags |= TexCreate_NoMipTail;
	}
	IntermediateTextureRHI = RHICreateTexture2D( SizeX, SizeY, Owner->Format, Owner->RequestedMips, TexCreateFlags );

	// Calculate conversion offsets used for copying shared miplevels.
	INT	SrcMipOffset	= Max( 0, Owner->ResidentMips - Owner->RequestedMips );
	INT	DstMipOffset	= Max( 0, Owner->RequestedMips - Owner->ResidentMips );

	// base index of source texture's miptail. 
	// No need to copy smaller mips than this since they are stored in the packed miptail
	INT SrcMipTailBaseIdx = Owner->MipTailBaseIdx - (Owner->Mips.Num() - Owner->ResidentMips); check(SrcMipTailBaseIdx>=0);	

	// Copy shared miplevels.
	for( INT MipIndex=0; MipIndex<Min(Owner->ResidentMips,Owner->RequestedMips) && ((MipIndex+SrcMipOffset)<=SrcMipTailBaseIdx); MipIndex++ )
	{
		// let platform perform the copy from mip to mip, it may ue the PendingMipChangeRequestStatus to sync up copy later;
		// figures out size of memory transfer. Includes alignment mojo on consoles.
		RHICopyMipToMipAsync(Texture2DRHI, MipIndex + SrcMipOffset, IntermediateTextureRHI, MipIndex + DstMipOffset, 
			Owner->Mips(MipIndex + FirstMip + DstMipOffset).Data.GetBulkDataSize(), Owner->PendingMipChangeRequestStatus);
	}

	FIOSystem* IO = GIOManager->GetIOSystem( IOSYSTEM_GenericAsync );
	check(IO);

	// Read into new miplevels, if any.
	INT FirstSharedMip	= (Owner->RequestedMips - Min(Owner->ResidentMips,Owner->RequestedMips));
	IORequestCount		= 0;
	for( INT MipIndex=0; MipIndex<FirstSharedMip; MipIndex++ )
	{
		const FTexture2DMipMap& MipMap = Owner->Mips( MipIndex + FirstMip );
		
		// Lock new texture.
		UINT DestStride;
		void* TheMipData = RHILockTexture2D( IntermediateTextureRHI, MipIndex, TRUE, DestStride );

		// Pass the request on to the async io manager after increasing the request count. The request count 
		// has been pre-incremented before fielding the update request so we don't have to worry about file
		// I/O immediately completing and the game thread kicking off FinalizeMipCount before this function
		// returns.
		Owner->PendingMipChangeRequestStatus.Increment();
		
		// Load and decompress async.
		if( MipMap.Data.IsStoredCompressedOnDisk() )
		{
			IORequestIndices[IORequestCount++] = IO->LoadCompressedData( 
				Filename,											// filename
				MipMap.Data.GetBulkDataOffsetInFile(),				// offset
				MipMap.Data.GetBulkDataSizeOnDisk(),				// compressed size
				MipMap.Data.GetBulkDataSize(),						// uncompressed size
				TheMipData,											// dest pointer
				MipMap.Data.GetDecompressionFlags(),				// compressed data format
				&Owner->PendingMipChangeRequestStatus,				// counter to decrement
				AIOP_Low											// priority
				);
		}
		// Load async.
		else
		{
			IORequestIndices[IORequestCount++] = IO->LoadData( 
				Filename,											// filename
				MipMap.Data.GetBulkDataOffsetInFile(),				// offset
				MipMap.Data.GetBulkDataSize(),						// size
				TheMipData,											// dest pointer
				&Owner->PendingMipChangeRequestStatus,				// counter to decrement
				AIOP_Low											// priority
				);
		}
		check(IORequestIndices[MipIndex]);
	}

	// We pre-incremented the request status in the main thread to take into account file I/O so
	// we now need to decrement it.
	check(Owner->PendingMipChangeRequestStatus.GetValue()>TEXTURE_READY_FOR_FINALIZATION);
	Owner->PendingMipChangeRequestStatus.Decrement();
}

/**
 * Called from the rendering thread to finalize a mip change.
 */
void FTexture2DResource::FinalizeMipCount()
{
	SCOPE_CYCLE_COUNTER(STAT_RenderingThreadFinalizeTime);

	check(Owner->bIsStreamable);
	check(Owner->PendingMipChangeRequestStatus.GetValue()==TEXTURE_FINALIZATION_IN_PROGRESS);

	// base index of destination texture's miptail. Skip mips smaller than the base miptail level 
	INT DstMipTailBaseIdx = Owner->MipTailBaseIdx - (Owner->Mips.Num() - Owner->RequestedMips); check(DstMipTailBaseIdx>=0);

	// Unlock texture mips.
	for( INT MipIndex=0; MipIndex<Owner->RequestedMips && MipIndex<=DstMipTailBaseIdx; MipIndex++ )
	{
		// Intermediate texture has RequestedMips miplevels, all of which have been locked.
		RHIUnlockTexture2D( IntermediateTextureRHI, MipIndex );
#if PLATFORM_SUPPORTS_RENDERING_FROM_LOCKED_TEXTURES
		// Old texture has ResidentMips miplevels, some of which might not have been locked
		// in case we are requesting a mip count decrease.
		if( MipIndex < Owner->ResidentMips )
		{
			RHIUnlockTexture2D( Texture2DRHI, MipIndex );
		}
#endif
	}

	// Perform switcheroo if the request hasn't been canceled.
	if( !Owner->bHasCancelationPending )
	{
		TextureRHI		= IntermediateTextureRHI;
		Texture2DRHI	= IntermediateTextureRHI;

		DEC_DWORD_STAT_BY( STAT_TextureMemory, TextureSize );
		STAT( TextureSize = IntermediateTextureSize; )
		STAT( IntermediateTextureSize = 0; )
	}
	// Request has been canceled.
	else
	{
		DEC_DWORD_STAT_BY( STAT_TextureMemory, IntermediateTextureSize );
		STAT( IntermediateTextureSize = 0; )
	}
	IntermediateTextureRHI.Release();

	// We're done.
	Owner->PendingMipChangeRequestStatus.Decrement();
}

/**
 * Called from the rendering thread to cancel async operations for request.
 */
void FTexture2DResource::CancelUpdate()
{
	SCOPE_CYCLE_COUNTER(STAT_RenderingThreadUpdateTime);

	// TEXTURE_FINALIZATION_IN_PROGRESS is valid as the request status gets decremented in the main thread. The actual
	// call to FinalizeMipCount will happen after this one though.
	check(Owner->PendingMipChangeRequestStatus.GetValue()>=TEXTURE_FINALIZATION_IN_PROGRESS);
	check(Owner->bHasCancelationPending);

	// We only have anything worth cancelation if there are outstanding I/O requests.
	if( IORequestCount )
	{
		// Retrieve IO system
		FIOSystem* IO = GIOManager->GetIOSystem( IOSYSTEM_GenericAsync );
		check(IO);
		// Cancel requests. Operation is all or nothing! @todo streaming: could be also done for partially in flight
		if( IO->CancelRequests( IORequestIndices, IORequestCount ) )
		{
			//@todo streaming: this could be done in a single step if FThreadSafeCounter exposed it
			for( INT i=0; i<IORequestCount; i++ )
			{
				Owner->PendingMipChangeRequestStatus.Decrement();
			}
		}
	}
}

