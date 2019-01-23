/*=============================================================================
	UnContentStreaming.cpp: Implementation of content streaming classes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	Stats.
-----------------------------------------------------------------------------*/

/** Streaming stats */
DECLARE_STATS_GROUP(TEXT("Streaming"),STATGROUP_Streaming);

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Streaming Textures"),STAT_StreamingTextures,STATGROUP_Streaming);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Intermediate Textures"),STAT_IntermediateTextures,STATGROUP_Streaming);

DECLARE_MEMORY_STAT(TEXT("Streaming Textures Size"),STAT_StreamingTexturesSize,STATGROUP_Streaming);
DECLARE_MEMORY_STAT(TEXT("Textures Max Size"),STAT_StreamingTexturesMaxSize,STATGROUP_Streaming);
DECLARE_MEMORY_STAT(TEXT("Intermediate Textures Size"),STAT_IntermediateTexturesSize,STATGROUP_Streaming);

//DECLARE_MEMORY_STAT2(TEXT("Texture Pool Size"),STAT_TexturePoolAllocatedSize,STATGROUP_Streaming,MCR_TexturePool1,TRUE);
// texture pool stat in the memory groupthat is hidden if nothing is allocated in it
DECLARE_MEMORY_STAT2(TEXT("Texture Pool Size"), STAT_TexturePoolAllocatedSize, STATGROUP_Memory, MCR_TexturePool1, FALSE);

DECLARE_MEMORY_STAT(TEXT("Request Size Current Frame"),STAT_RequestSizeCurrentFrame,STATGROUP_Streaming);
DECLARE_MEMORY_STAT(TEXT("Request Size Total"),STAT_RequestSizeTotal,STATGROUP_Streaming);

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Requests In Cancelation Phase"),STAT_RequestsInCancelationPhase,STATGROUP_Streaming);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Requests In Update Phase"),STAT_RequestsInUpdatePhase,STATGROUP_Streaming);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Requests in Finalize Phase"),STAT_RequestsInFinalizePhase,STATGROUP_Streaming);

DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Streaming fudge factor"),STAT_StreamingFudgeFactor,STATGROUP_Streaming);

DECLARE_CYCLE_STAT(TEXT("Game Thread Update Time"),STAT_GameThreadUpdateTime,STATGROUP_Streaming);
DECLARE_CYCLE_STAT(TEXT("Rendering Thread Update Time"),STAT_RenderingThreadUpdateTime,STATGROUP_Streaming);
DECLARE_CYCLE_STAT(TEXT("Rendering Thread Finalize Time"),STAT_RenderingThreadFinalizeTime,STATGROUP_Streaming)

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

/** Global streaming manager */
FStreamingManagerCollection* GStreamingManager;

/** Collection of views that need to be taken into account for streaming. */
TArray<FStreamingViewInfo> FStreamingManagerBase::ViewInfos;

/**
 * Helper function to flush resource streaming from within Core project.
 */
void FlushResourceStreaming()
{
	GStreamingManager->BlockTillAllRequestsFinished();
}

/*-----------------------------------------------------------------------------
	FStreamingManagerBase implementation.
-----------------------------------------------------------------------------*/

/**
 * Adds the passed in view information to the static array.
 *
 * @param ViewOrigin		View origin
 * @param ScreenSize		Screen size
 * @param FOVScreenSize		Screen size taking FOV into account
 */
void FStreamingManagerBase::AddViewInformation( const FVector& ViewOrigin, FLOAT ScreenSize, FLOAT FOVScreenSize )
{
	FStreamingViewInfo ViewInfo;
	ViewInfo.ViewOrigin		= ViewOrigin;
	ViewInfo.ScreenSize	= ScreenSize;
	ViewInfo.FOVScreenSize	= FOVScreenSize;
	ViewInfos.AddItem( ViewInfo );
}

/*-----------------------------------------------------------------------------
	FStreamingManagerCollection implementation.
-----------------------------------------------------------------------------*/

/**
 * Sets the number of iterations to use for the next time UpdateResourceStreaming is being called. This 
 * is being reset to 1 afterwards.
 *
 * @param InNumIterations	Number of iterations to perform the next time UpdateResourceStreaming is being called.
 */
void FStreamingManagerCollection::SetNumIterationsForNextFrame( INT InNumIterations )
{
	NumIterations = InNumIterations;
}

/**
 * Updates streaming, taking into account all view infos and empties ViewInfos array.
 *
 * @param DeltaTime		Time since last call in seconds
 */
void FStreamingManagerCollection::UpdateResourceStreaming( FLOAT DeltaTime )
{
	for( INT Iteration=0; Iteration<NumIterations; Iteration++ )
	{
		// Flush rendering commands in the case of multiple iterations to sync up resource streaming
		// with the GPU/ rendering thread.
		if( Iteration > 0 )
		{
			FlushRenderingCommands();
		}

		// Route to streaming managers.
		for( INT ManagerIndex=0; ManagerIndex<StreamingManagers.Num(); ManagerIndex++ )
		{
			FStreamingManagerBase* StreamingManager = StreamingManagers(ManagerIndex);
			StreamingManager->UpdateResourceStreaming( DeltaTime );
		}
	}

	// Reset number of iterations to 1 for next frame.
	NumIterations = 1;

	// Empty view infos array to be populated again next frame.
	ViewInfos.Empty();
}

/**
 * Blocks till all pending requests are fulfilled.
 */
void FStreamingManagerCollection::BlockTillAllRequestsFinished()
{
	// Route to streaming managers.
	for( INT ManagerIndex=0; ManagerIndex<StreamingManagers.Num(); ManagerIndex++ )
	{
		FStreamingManagerBase* StreamingManager = StreamingManagers(ManagerIndex);
		StreamingManager->BlockTillAllRequestsFinished();
	}
}

/**
 * Notifies managers of "level" change.
 */
void FStreamingManagerCollection::NotifyLevelChange()
{
	// Route to streaming managers.
	for( INT ManagerIndex=0; ManagerIndex<StreamingManagers.Num(); ManagerIndex++ )
	{
		FStreamingManagerBase* StreamingManager = StreamingManagers(ManagerIndex);
		StreamingManager->NotifyLevelChange();
	}
}

/** Don't stream world resources for the next NumFrames. */
void FStreamingManagerCollection::SetDisregardWorldResourcesForFrames(INT NumFrames )
{
	// Route to streaming managers.
	for( INT ManagerIndex=0; ManagerIndex<StreamingManagers.Num(); ManagerIndex++ )
	{
		FStreamingManagerBase* StreamingManager = StreamingManagers(ManagerIndex);
		StreamingManager->SetDisregardWorldResourcesForFrames(NumFrames);
	}
}

/**
 * Adds a streaming manager to the array of managers to route function calls to.
 *
 * @param StreamingManager	Streaming manager to add
 */
void FStreamingManagerCollection::AddStreamingManager( FStreamingManagerBase* StreamingManager )
{
	StreamingManagers.AddItem( StreamingManager );
}

/**
 * Removes a streaming manager from the array of managers to route function calls to.
 *
 * @param StreamingManager	Streaming manager to remove
 */
void FStreamingManagerCollection::RemoveStreamingManager( FStreamingManagerBase* StreamingManager )
{
	StreamingManagers.RemoveItem( StreamingManager );
}


/*-----------------------------------------------------------------------------
	FStreamingManagerTexture implementation.
-----------------------------------------------------------------------------*/

/** Constructor, initializing all members and  */
FStreamingManagerTexture::FStreamingManagerTexture()
:	RemainingTicksToDisregardWorldTextures(0)
,	RemainingTicksToSuspendActivity(0)
,	FudgeFactor(1.f)
,	FudgeFactorRateOfChange(0.f)
,	LastLevelChangeTime(0.0)
,	bUseMinRequestLimit(FALSE)
,	MemoryHysteresisLimit(0)
,	MemoryDropMipLevelsLimit(0)
,	MemoryStopIncreasingLimit(0)
,	MemoryStopStreamingLimit(0)
,	FudgeFactorIncreaseRateOfChange(0)
,	FudgeFactorDecreaseRateOfChange(0)
,	MinRequestedMipsToConsider(0)
,	MinTimeToGuaranteeMinMipCount(0)
,	MaxTimeToGuaranteeMinMipCount(0)
,	NumStreamingTextures(0)
,	NumRequestsInCancelationPhase(0)
,	NumRequestsInUpdatePhase(0)
,	NumRequestsInFinalizePhase(0)
,	TotalIntermediateTexturesSize(0)
,	NumIntermediateTextures(0)
,	TotalStreamingTexturesSize(0)
,	TotalStreamingTexturesMaxSize(0)
,	TotalMipCountIncreaseRequestsInFlight(0)
{
	// Read settings from ini file.
	verify( GConfig->GetInt( TEXT("TextureStreaming"), TEXT("HysteresisLimit"),				MemoryHysteresisLimit,				GEngineIni ) );
	verify( GConfig->GetInt( TEXT("TextureStreaming"), TEXT("DropMipLevelsLimit"),			MemoryDropMipLevelsLimit,			GEngineIni ) );
	verify( GConfig->GetInt( TEXT("TextureStreaming"), TEXT("StopIncreasingLimit"),			MemoryStopIncreasingLimit,			GEngineIni ) );
	verify( GConfig->GetInt( TEXT("TextureStreaming"), TEXT("StopStreamingLimit"),			MemoryStopStreamingLimit,			GEngineIni ) );
	verify( GConfig->GetInt( TEXT("TextureStreaming"), TEXT("MinRequestedMipsToConsider"),	MinRequestedMipsToConsider,			GEngineIni ) );

	verify( GConfig->GetFloat( TEXT("TextureStreaming"), TEXT("MinTimeToGuaranteeMinMipCount"),		MinTimeToGuaranteeMinMipCount,	 GEngineIni ) );
	verify( GConfig->GetFloat( TEXT("TextureStreaming"), TEXT("MaxTimeToGuaranteeMinMipCount"),		MaxTimeToGuaranteeMinMipCount,	 GEngineIni ) );
	verify( GConfig->GetFloat( TEXT("TextureStreaming"), TEXT("FudgeFactorIncreaseRateOfChange"),	FudgeFactorIncreaseRateOfChange, GEngineIni ) );
	verify( GConfig->GetFloat( TEXT("TextureStreaming"), TEXT("FudgeFactorDecreaseRateOfChange"),	FudgeFactorDecreaseRateOfChange, GEngineIni ) );

	// Convert from MByte to byte.
	MemoryHysteresisLimit		*= 1024 * 1024;
	MemoryDropMipLevelsLimit	*= 1024 * 1024;
	MemoryStopIncreasingLimit	*= 1024 * 1024;
	MemoryStopStreamingLimit	*= 1024 * 1024;
}

/** Number of ticks for full iteration over texture list. */
#define NUM_TICKS_FOR_FULL_ITERATION 10

/** Maximum number of bytes to change per frame */
#define MAX_PER_FRAME_REQUEST_LIMIT (3 * 1024 * 1024)

/**
 * Notifies manager of "level" change so it can prioritize character textures for a few frames.
 */
void FStreamingManagerTexture::NotifyLevelChange()
{
	// Disregard world textures for one iteration to prioritize other requests.
	RemainingTicksToDisregardWorldTextures = NUM_TICKS_FOR_FULL_ITERATION;
	// Keep track of last time this function was called.
	LastLevelChangeTime = appSeconds();
	// Prioritize higher miplevels to avoid texture popping on initial level load.
	bUseMinRequestLimit = TRUE;
}

/** Don't stream world resources for the next NumFrames. */
void FStreamingManagerTexture::SetDisregardWorldResourcesForFrames(INT NumFrames )
{
	RemainingTicksToDisregardWorldTextures = NumFrames;
}

/**
 * Updates streaming, taking into all views infos.
 *
 * @param DeltaTime		Time since last call in seconds
 */
void FStreamingManagerTexture::UpdateResourceStreaming( FLOAT DeltaTime )
{
	SCOPE_CYCLE_COUNTER(STAT_GameThreadUpdateTime);

	// Reset stats first texture. Split up from updating as GC might cause a reset in the middle of iteration
	// and we don't want to report bogus stats in this case.
	if( UTexture2D::GetCurrentStreamableLink() == UTexture2D::GetStreamableList() )
	{
		NumStreamingTextures								= 0;
		NumRequestsInCancelationPhase						= 0;
		NumRequestsInUpdatePhase							= 0;
		NumRequestsInFinalizePhase							= 0;
		TotalIntermediateTexturesSize						= 0;
		NumIntermediateTextures								= 0;
		TotalStreamingTexturesSize							= 0;
		TotalStreamingTexturesMaxSize						= 0;
		TotalMipCountIncreaseRequestsInFlight				= 0;
	}

	DWORD ThisFrameNumStreamingTextures						= 0;
	DWORD ThisFrameNumRequestsInCancelationPhase			= 0;
	DWORD ThisFrameNumRequestsInUpdatePhase					= 0;
	DWORD ThisFrameNumRequestsInFinalizePhase				= 0;
	DWORD ThisFrameTotalIntermediateTexturesSize			= 0;
	DWORD ThisFrameNumIntermediateTextures					= 0;
	DWORD ThisFrameTotalStreamingTexturesSize				= 0;
	DWORD ThisFrameTotalStreamingTexturesMaxSize			= 0;
	DWORD ThisFrameTotalRequestSize							= 0; 
	DWORD ThisFrameTotalMipCountIncreaseRequestsInFlight	= 0;

	// Decrase counter to disregard world textures if it's set.
	if( RemainingTicksToDisregardWorldTextures > 0 )
	{
		RemainingTicksToDisregardWorldTextures--;
	}

	// Fallback handler used if texture is not handled by any other handlers. Guaranteed to handle texture and not return INDEX_NONE.
	FStreamingHandlerTextureLastRender FallbackStreamingHandler;

	// Available texture memory, if supported by RHI. This stat is async as the renderer allocates the memory in its own thread so we
	// only query once and roughly adjust the values as needed.
	INT		AllocatedMemorySize		= INDEX_NONE;
	INT		AvailableMemorySize		= INDEX_NONE;
	UBOOL	bRHISupportsMemoryStats = RHIGetTextureMemoryStats( AllocatedMemorySize, AvailableMemorySize );

	// Update stats if supported.
	if( bRHISupportsMemoryStats )
	{
		// set total size for the pool (used to available)
		STAT(GStatManager.SetAvailableMemory(MCR_TexturePool1, AvailableMemorySize + AllocatedMemorySize));
		SET_DWORD_STAT(STAT_TexturePoolAllocatedSize,AllocatedMemorySize);
	}
	else
	{
		SET_DWORD_STAT(STAT_TexturePoolAllocatedSize,0);
	}

	// Update suspend count.
	if( RemainingTicksToSuspendActivity > 0 )
	{
		RemainingTicksToSuspendActivity--;
	}

	// Update fudge factor after clamping delta time to something reasonable.
	FudgeFactor = Clamp( FudgeFactor + FudgeFactorRateOfChange * Min( 0.1f, DeltaTime ), 1.f, 10.f );

	// Get current streamable link and iterate over subset of textures each frame.
	TLinkedList<UTexture2D*>*& CurrentStreamableLink = UTexture2D::GetCurrentStreamableLink();
	INT MaxTexturesToProcess = Max( 1, UTexture2D::GetNumStreamableTextures() / NUM_TICKS_FOR_FULL_ITERATION );
	while( CurrentStreamableLink && MaxTexturesToProcess-- )
	{
		// Advance streamable link.
		UTexture2D* Texture		= **CurrentStreamableLink;
		CurrentStreamableLink	= CurrentStreamableLink->Next();
			
		// Skip world textures to allow e.g. character textures to stream in first.
		if( RemainingTicksToDisregardWorldTextures 
		&&	((	Texture->LODGroup == TEXTUREGROUP_World)
			||	Texture->LODGroup == TEXTUREGROUP_WorldNormalMap) )
		{
			continue;
		}

		// Figure out max number of miplevels allowed by LOD code.
		INT RequestedMips	= INDEX_NONE;
		INT	MaxResidentMips	= Max( 1, Min( Texture->Mips.Num() - Texture->GetCachedLODBias(), GMaxTextureMipCount ) );

		// Only work on streamable textures that have enough miplevels.
		if( Texture->bIsStreamable
		&&	Texture->NeverStream == FALSE
		&&	Texture->Mips.Num() > GMinTextureResidentMipCount )
		{
			STAT(ThisFrameNumStreamingTextures++);
			STAT(ThisFrameTotalStreamingTexturesSize += Texture->GetSize(Texture->ResidentMips));
			STAT(ThisFrameTotalStreamingTexturesMaxSize += Texture->GetSize(MaxResidentMips));

			// No need to figure out which miplevels we want if the texture is still in the process of being
			// created for the first time. We also cannot change the texture during that time outside of 
			// UpdateResource.
			if( Texture->IsReadyForStreaming() )
			{
				// Calculate LOD allowed mips passed to GetWantedMips.
				INT	TextureLODBias	= Texture->GetCachedLODBias();
				INT LODAllowedMips	= Max( 1, Texture->Mips.Num() - TextureLODBias );					

				// Figure out miplevels to request based on handlers and whether streaming is enabled.
				if( !Texture->ShouldMipLevelsBeForcedResident() && GEngine->bUseTextureStreaming && !GSystemSettings.bOnlyStreamInTextures )
				{
					// Iterate over all handlers and figure out the maximum requested number of mips.
					for( INT HandlerIndex=0; HandlerIndex<TextureStreamingHandlers.Num(); HandlerIndex++ )
					{
						FStreamingHandlerTextureBase* TextureStreamingHandler = TextureStreamingHandlers(HandlerIndex);
						INT WantedMips	= TextureStreamingHandler->GetWantedMips( Texture, ViewInfos, FudgeFactor, LODAllowedMips );
						RequestedMips	= Max( RequestedMips, WantedMips );
					}

					// Not handled by any handler, use fallback handler.
					if( RequestedMips == INDEX_NONE )
					{
						RequestedMips	= FallbackStreamingHandler.GetWantedMips( Texture, ViewInfos, FudgeFactor, LODAllowedMips );
					}
				}
				// Stream in all allowed miplevels if requested.
				else
				{
					RequestedMips = LODAllowedMips;
				}
	
				// Take texture LOD and maximum texture size into account.
				INT MinAllowedMips  = Min( LODAllowedMips, GMinTextureResidentMipCount );
				INT MaxAllowedMips	= Min( LODAllowedMips, GMaxTextureMipCount );
				RequestedMips		= Clamp( RequestedMips, MinAllowedMips, MaxAllowedMips );

				// Number of levels in the packed miptail.
				INT NumMipTailLevels= Max(0,Texture->Mips.Num() - Texture->MipTailBaseIdx);
				// Make sure that we at least load the mips that reside in the packed miptail.
				RequestedMips		= Max( RequestedMips, NumMipTailLevels );
				
				check( RequestedMips != INDEX_NONE );
				check( RequestedMips >= 1 );
				check( RequestedMips <= Texture->Mips.Num() ); 

				// If TRUE we're so low on memory that we cannot request any changes.
				UBOOL	bIsStopLimitExceeded	= bRHISupportsMemoryStats && (AvailableMemorySize < MemoryStopStreamingLimit);
				// And due to the async nature of memory requests we must suspend activity for at least 2 frames.
				if( bIsStopLimitExceeded )
				{
					// Value gets decremented at top so setting it to 3 will suspend 2 ticks.
					RemainingTicksToSuspendActivity = 3;
				}
				UBOOL	bAllowRequests			= RemainingTicksToSuspendActivity == 0;

				// Update streaming status. A return value of FALSE means that we're done streaming this texture
				// so we can potentially request another change.
				UBOOL	bSafeToRequest			= (Texture->UpdateStreamingStatus() == FALSE);
				INT		RequestStatus			= Texture->PendingMipChangeRequestStatus.GetValue();
				UBOOL	bHasCancelationPending	= Texture->bHasCancelationPending;
			
				// Checked whether we should cancel the pending request if the new one is different.
				if( !bSafeToRequest && (Texture->RequestedMips != RequestedMips) && !bHasCancelationPending )
				{
					// Cancel load.
					if( (RequestedMips < Texture->RequestedMips) && (RequestedMips >= Texture->ResidentMips) )
					{
						bHasCancelationPending = Texture->CancelPendingMipChangeRequest();
					}
					// Cancel unload.
					else if( (RequestedMips > Texture->RequestedMips) && (RequestedMips <= Texture->ResidentMips) )
					{
						bHasCancelationPending = Texture->CancelPendingMipChangeRequest();
					}
				}

				if( bHasCancelationPending )
				{
					ThisFrameNumRequestsInCancelationPhase++;
				}
				else if( RequestStatus >= TEXTURE_READY_FOR_FINALIZATION )
				{
					ThisFrameNumRequestsInUpdatePhase++;
				}
				else if( RequestStatus == TEXTURE_FINALIZATION_IN_PROGRESS )
				{
					ThisFrameNumRequestsInFinalizePhase++;
				}	
		
				// Request is in flight so there is an intermediate texture with RequestedMips miplevels.
				if( RequestStatus > 0 )
				{
					ThisFrameTotalIntermediateTexturesSize += Texture->GetSize(Texture->RequestedMips);
					ThisFrameNumIntermediateTextures++;
					// Update texture increase request stats.
					if( Texture->RequestedMips > Texture->ResidentMips )
					{
						ThisFrameTotalMipCountIncreaseRequestsInFlight++;
					}
				}

				// Request a change if it's safe and requested mip count differs from resident.
				if( bSafeToRequest && bAllowRequests && RequestedMips != Texture->ResidentMips )
				{
					UBOOL bCanRequestTextureIncrease = TRUE;

					// If memory stats are supported we respect the memory limit to not stream in miplevels.
					if( ( bRHISupportsMemoryStats && (AvailableMemorySize <= MemoryStopIncreasingLimit) )
					// Don't request increase if requested mip count is below threshold.
					||	(bUseMinRequestLimit && RequestedMips < MinRequestedMipsToConsider) )
					{
						bCanRequestTextureIncrease = FALSE;
					}
						
					// Only request change if it's a decrease or we can request an increase.
					if( (RequestedMips < Texture->ResidentMips) || bCanRequestTextureIncrease )						
					{
						// Manually update size as allocations are deferred/ happening in rendering thread.
						INT CurrentRequestSize		= Texture->GetSize(RequestedMips);
						AvailableMemorySize			-= CurrentRequestSize;
						// Keep track of current allocations this frame and stop iteration if we've exceeded
						// frame limit.
						ThisFrameTotalRequestSize	+= CurrentRequestSize;
						if( ThisFrameTotalRequestSize > MAX_PER_FRAME_REQUEST_LIMIT )
						{
							// We've exceeded the request limit for this frame.
							MaxTexturesToProcess = 0;
						}

						check(RequestStatus==TEXTURE_READY_FOR_REQUESTS);
						check(Texture->PendingMipChangeRequestStatus.GetValue()==TEXTURE_READY_FOR_REQUESTS);
						check(!Texture->bHasCancelationPending);
						// Set new requested mip count.
						Texture->RequestedMips = RequestedMips;
						// Enqueue command to update mip count.
						FTexture2DResource* Texture2DResource = (FTexture2DResource*) Texture->Resource;
						Texture2DResource->BeginUpdateMipCount();
					}
				}
			}
		}
	}

	// Update fudge factor rate of change if memory stats are supported.
	if( bRHISupportsMemoryStats )
	{
		if( AvailableMemorySize <= MemoryDropMipLevelsLimit )
		{
			FudgeFactorRateOfChange = FudgeFactorIncreaseRateOfChange;
		}
		else if( AvailableMemorySize > MemoryHysteresisLimit )
		{
			FudgeFactorRateOfChange	= FudgeFactorDecreaseRateOfChange;
		}
		else
		{
			FudgeFactorRateOfChange = 0;
		}
	}
	
	// Update running counts with this frames stats.
	NumStreamingTextures					+= ThisFrameNumStreamingTextures;
	NumRequestsInCancelationPhase			+= ThisFrameNumRequestsInCancelationPhase;
	NumRequestsInUpdatePhase				+= ThisFrameNumRequestsInUpdatePhase;
	NumRequestsInFinalizePhase				+= ThisFrameNumRequestsInFinalizePhase;
	TotalIntermediateTexturesSize			+= ThisFrameTotalIntermediateTexturesSize;
	NumIntermediateTextures					+= ThisFrameNumIntermediateTextures;
	TotalStreamingTexturesSize				+= ThisFrameTotalStreamingTexturesSize;
	TotalStreamingTexturesMaxSize			+= ThisFrameTotalStreamingTexturesMaxSize;
	TotalMipCountIncreaseRequestsInFlight	+= ThisFrameTotalMipCountIncreaseRequestsInFlight;

	// Set the stats on wrap-around. Reset happens independently to correctly handle resetting in the middle of iteration.
	if( CurrentStreamableLink == NULL )
	{
		SET_DWORD_STAT( STAT_StreamingTextures,				NumStreamingTextures			);
		SET_DWORD_STAT( STAT_RequestsInCancelationPhase,	NumRequestsInCancelationPhase	);
		SET_DWORD_STAT( STAT_RequestsInUpdatePhase,			NumRequestsInUpdatePhase		);
		SET_DWORD_STAT( STAT_RequestsInFinalizePhase,		NumRequestsInFinalizePhase		);
		SET_DWORD_STAT( STAT_IntermediateTexturesSize,		TotalIntermediateTexturesSize	);
		SET_DWORD_STAT( STAT_IntermediateTextures,			NumIntermediateTextures			);
		SET_DWORD_STAT( STAT_StreamingTexturesSize,			TotalStreamingTexturesSize		);
		SET_DWORD_STAT( STAT_StreamingTexturesMaxSize,		TotalStreamingTexturesMaxSize	);

		// Determine whether should disable this feature.
		if( bUseMinRequestLimit )
		{
			FLOAT TimeSinceLastLevelChange = appSeconds() - LastLevelChangeTime;
			// No longer use min request limit if we're out of requests and the min guaranteed time has elapsed...
			if( (TimeSinceLastLevelChange > MinTimeToGuaranteeMinMipCount && ThisFrameTotalMipCountIncreaseRequestsInFlight == 0)
			// ... or if the max allowed time has elapsed.
			||	(TimeSinceLastLevelChange > MaxTimeToGuaranteeMinMipCount) )
			{
				bUseMinRequestLimit = FALSE;
			}
		}
		// debugf( TEXT("%5.2f MByte outstanding requests, %6.2f"), TotalIntermediateTexturesSize / 1024.f / 1024.f, appSeconds() - GStartTime );
	}

	SET_DWORD_STAT(		STAT_RequestSizeCurrentFrame,		ThisFrameTotalRequestSize		);
	INC_DWORD_STAT_BY(	STAT_RequestSizeTotal,				ThisFrameTotalRequestSize		);
	SET_FLOAT_STAT(		STAT_StreamingFudgeFactor,			FudgeFactor						);
}

/**
 * Blocks till all pending requests are fulfilled.
 */
void FStreamingManagerTexture::BlockTillAllRequestsFinished()
{
	// Flush rendering commands.
	FlushRenderingCommands();
	
	// Iterate over all textures and make sure that they are done streaming.
	for( TObjectIterator<UTexture2D> It; It; ++It )
	{
		UTexture2D* Texture	= *It;
		// Update the streaming portion till we're done streaming.
		while( Texture->UpdateStreamingStatus() )
		{
			// Give up the timeslice.
			appSleep(0);
		}
		INT RequestStatus = Texture->PendingMipChangeRequestStatus.GetValue();
		check( RequestStatus == TEXTURE_READY_FOR_REQUESTS || RequestStatus == TEXTURE_PENDING_INITIALIZATION );
	}
}

/**
 * Adds a textures streaming handler to the array of handlers used to determine which
 * miplevels need to be streamed in/ out.
 *
 * @param TextureStreamingHandler	Handler to add
 */
void FStreamingManagerTexture::AddTextureStreamingHandler( FStreamingHandlerTextureBase* TextureStreamingHandler )
{
	TextureStreamingHandlers.AddItem( TextureStreamingHandler );
}

/**
 * Removes a textures streaming handler from the array of handlers used to determine which
 * miplevels need to be streamed in/ out.
 *
 * @param TextureStreamingHandler	Handler to remove
 */
void FStreamingManagerTexture::RemoveTextureStreamingHandler( FStreamingHandlerTextureBase* TextureStreamingHandler )
{
	TextureStreamingHandlers.RemoveItem( TextureStreamingHandler );
}


/*-----------------------------------------------------------------------------
	Streaming handler implementations.
-----------------------------------------------------------------------------*/

/**
 * Returns mip count wanted by this handler for the passed in texture. 
 * 
 * @param	Texture		Texture to determine wanted mip count for
 * @param	ViewInfos	Array of view infos to use for mip count determination
 * @param	FudgeFactor	Fudge factor hint; [1,inf] with higher values meaning that more mips should be dropped
 * @param	LODAllowedMips	Max number of mips allowed by LOD code
 * @return	Number of miplevels that should be streamed in or INDEX_NONE if 
 *			texture isn't handled by this handler.
 */
INT FStreamingHandlerTextureStatic::GetWantedMips( UTexture2D* Texture, const TArray<FStreamingViewInfo>& ViewInfos, FLOAT FudgeFactor, INT LODAllowedMips )
{
	// INDEX_NONE signals handler  not handling the texture.
	INT		WantedMipCount		= INDEX_NONE;
	// Whether we should early out if we e.g. know that we need all allowed mips.
	UBOOL	bShouldAbortLoop	= FALSE;

	// Nothing do to if there are no views.
	if( ViewInfos.Num() )
	{
		// Cached so it's only calculated once per texture.
		const FLOAT SquaredFudgeFactor = Square(FudgeFactor);

		// Iterate over all associated/ visible levels.
		for( INT LevelIndex=0; LevelIndex<GWorld->Levels.Num() && !bShouldAbortLoop; LevelIndex++ )
		{
			// Find instances of the texture in this level.
			ULevel*								Level				= GWorld->Levels(LevelIndex);
			TArray<FStreamableTextureInstance>* TextureInstances	= Level->TextureToInstancesMap.Find( Texture );

			// Nothing to do if there are no instances.
			if( TextureInstances && TextureInstances->Num() )
			{
				// Iterate over all view infos.
				for( INT ViewIndex=0; ViewIndex<ViewInfos.Num() && !bShouldAbortLoop; ViewIndex++ )
				{
					const FStreamingViewInfo& ViewInfo	= ViewInfos(ViewIndex);

					// Iterate over all instances of the texture in the level.
					for( INT InstanceIndex=0; InstanceIndex<TextureInstances->Num() && !bShouldAbortLoop; InstanceIndex++ )
					{
						FStreamableTextureInstance& TextureInstance = (*TextureInstances)(InstanceIndex);

						// Calculate distance of viewer to bounding sphere.
						FLOAT	DistSq					= FDistSquared( ViewInfo.ViewOrigin, TextureInstance.BoundingSphere ) * SquaredFudgeFactor;
						FLOAT	DistSqMinusRadiusSq		= DistSq - Square(TextureInstance.BoundingSphere.W);

						// Outside the texture instance bounding sphere, calculate miplevel based on screen space size of bounding sphere.
						if( DistSqMinusRadiusSq > 1.f )
						{
							// Calculate the maximum screen space dimension in pixels.
							FLOAT	SilhouetteRadius	= TextureInstance.BoundingSphere.W * appInvSqrtEst( DistSqMinusRadiusSq );
							FLOAT	ScreenSizeInPixels	= 2.f * SilhouetteRadius * ViewInfo.ScreenSize; //@todo streaming: add option to use FOV or not
							FLOAT	ScreenSizeInTexels	= TextureInstance.TexelFactor / (2.f * TextureInstance.BoundingSphere.W) * ScreenSizeInPixels;

							// WantedMipCount is the number of mips so we need to adjust with "+ 1".
							WantedMipCount				= Max( WantedMipCount, 1 + appCeilLogTwo( appTrunc(ScreenSizeInTexels) ) );
						}
						// Request all miplevels to be loaded if we're inside the bounding sphere.
						else
						{
							WantedMipCount				= Texture->Mips.Num();
						}

						// Early out if we're already at the max.
						if( WantedMipCount >= LODAllowedMips )
						{
							bShouldAbortLoop = TRUE;
						}
					}
				}
			}
		}
	}
	return WantedMipCount;
}

/**
 * Returns mip count wanted by this handler for the passed in texture. 
 * 
 * @param	Texture		Texture to determine wanted mip count for
 * @param	ViewInfos	Array of view infos to use for mip count determination
 * @param	FudgeFactor	Fudge factor hint; [1,inf] with higher values meaning that more mips should be dropped
 * @param	LODAllowedMips	Max number of mips allowed by LOD code
 * @return	Number of miplevels that should be streamed in or INDEX_NONE if 
 *			texture isn't handled by this handler.
 */
INT FStreamingHandlerTextureLastRender::GetWantedMips( UTexture2D* Texture, const TArray<FStreamingViewInfo>& /*ViewInfos*/, FLOAT FudgeFactor, INT LODAllowedMips )
{
	FLOAT SecondsSinceLastRender = (GCurrentTime - Texture->Resource->LastRenderTime); // * FudgeFactor;

	if( SecondsSinceLastRender < 45 )
	{
		return LODAllowedMips;
	}
	else if( SecondsSinceLastRender < 90 )
	{
		return LODAllowedMips - 1;
	}
	else
	{
		return 0;
	}
}

/**
 * Returns mip count wanted by this handler for the passed in texture. 
 * 
 * @param	Texture		Texture to determine wanted mip count for
 * @param	ViewInfos	Array of view infos to use for mip count determination
 * @param	FudgeFactor	Fudge factor hint; [1,inf] with higher values meaning that more mips should be dropped
 * @param	LODAllowedMips	Max number of mips allowed by LOD code
 * @return	Number of miplevels that should be streamed in or INDEX_NONE if 
 *			texture isn't handled by this handler.
 */
INT FStreamingHandlerTextureLevelForced::GetWantedMips( UTexture2D* Texture, const TArray<FStreamingViewInfo>& ViewInfos, FLOAT FudgeFactor, INT LODAllowedMips )
{
	INT WantedMipCount = INDEX_NONE;
	for( INT LevelIndex=0; LevelIndex<GWorld->Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = GWorld->Levels(LevelIndex);
		if( Level->ForceStreamTextures.Find( Texture ) )
		{
			WantedMipCount = LODAllowedMips;
			break;
		}
	}
	return WantedMipCount;
}


/*-----------------------------------------------------------------------------
	Texture streaming helper structs.
-----------------------------------------------------------------------------*/

/**
 * FStreamableTextureInstance serialize operator.
 *
 * @param	Ar					Archive to to serialize object to/ from
 * @param	TextureInstance		Object to serialize
 * @return	Returns the archive passed in
 */
FArchive& operator<<( FArchive& Ar, FStreamableTextureInstance& TextureInstance )
{
	Ar << TextureInstance.BoundingSphere;
	Ar << TextureInstance.TexelFactor;
	return Ar;
}


























