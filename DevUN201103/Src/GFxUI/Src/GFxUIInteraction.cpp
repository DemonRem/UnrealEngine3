/**********************************************************************

Filename    :   GFxUIInteraction.cpp
Content     :   UGFxInteraction class implementation for GFx

Copyright   :   Copyright 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxUI.h"


#include "GFxUIEngine.h"
#include "GFxUIRenderer.h"
#include "GFxUIAllocator.h"

#include "EngineSequenceClasses.h"
#include "GFxUIUISequenceClasses.h"

IMPLEMENT_CLASS(UGFxAction_CloseMovie);
IMPLEMENT_CLASS(UGFxAction_GetVariable);
IMPLEMENT_CLASS(UGFxAction_Invoke);
IMPLEMENT_CLASS(UGFxAction_OpenMovie);
IMPLEMENT_CLASS(UGFxAction_SetCaptureKeys);
IMPLEMENT_CLASS(UGFxAction_SetVariable);
IMPLEMENT_CLASS(UGFxFSCmdHandler_Kismet);
IMPLEMENT_CLASS(UGFxInteraction);
IMPLEMENT_CLASS(UGFxEvent_FSCommand);

#if WITH_GFx

extern FGFxAllocator GGFxAllocator; 


/** Initializes this interaction, allocates the GFxEngine */
void UGFxInteraction::Init()
{
    if(NULL == GGFxEngine)
    {
        GGFxEngine = FGFxEngine::GetEngine();
    }
    check(NULL != GGFxGCManager);

	GCallbackEvent->Register(CALLBACK_PreViewportResized, this);
    GCallbackEvent->Register(CALLBACK_ViewportResized, this);
}

void UGFxInteraction::BeginDestroy()
{
    Super::BeginDestroy();
    if (GGFxEngine)
        GGFxEngine->RenderCmdFence.BeginFence();
}

UBOOL UGFxInteraction::IsReadyForFinishDestroy()
{
    if (GGFxEngine)
        return (GGFxEngine->RenderCmdFence.GetNumPendingFences() == 0);
    else
        return TRUE;
}


void UGFxInteraction::FinishDestroy()
{
	GCallbackEvent->Unregister(CALLBACK_PreViewportResized, this);
    GCallbackEvent->Unregister(CALLBACK_ViewportResized, this);

    if (GGFxEngine)
        GGFxEngine->NotifyGameSessionEnded();


    Super::FinishDestroy();
}


/** Set the Engine's viewport to the viewport specified */
void UGFxInteraction::SetRenderViewport(FViewport* InViewport)
{
	if(GGFxEngine)
	{
		GGFxEngine->SetRenderViewport(InViewport);
	}
}

#endif // WITH_GFx

void UGFxInteraction::NotifyGameSessionEnded()
{
#if WITH_GFx
    if (GGFxEngine)
        GGFxEngine->NotifyGameSessionEnded();
#endif // WITH_GFx
}


void UGFxInteraction::CloseAllMoviePlayers()
{
#if WITH_GFx
	if (GGFxEngine)
	{
		GGFxEngine->CloseAllMovies(FALSE);
	}
#endif // WITH_GFx
}

void UGFxInteraction::NotifyPlayerAdded(INT PlayerIndex, ULocalPlayer* AddedPlayer)
{
#if WITH_GFx
	FGFxEngine::GetEngine()->AddPlayerState();
#endif // WITH_GFx
}

void UGFxInteraction::NotifyPlayerRemoved(INT PlayerIndex, ULocalPlayer* RemovedPlayer)
{
#if WITH_GFx
	FGFxEngine::GetEngine()->RemovePlayerState(PlayerIndex);
#endif // WITH_GFx
}


#if WITH_GFx
/**
 * Called once a frame to update the interaction's state.
 * @param	DeltaTime - The time since the last frame.
 */
void UGFxInteraction::Tick(FLOAT DeltaTime)
{
	extern UBOOL GRenderGFxUI;

	if (GGFxEngine && GRenderGFxUI)
	{
        #if STATS
          GGFxAllocator.StartFrame();
        #endif // STATS

        GGFxEngine->Tick(DeltaTime);

        CaptureRenderFrameStats();
	}
}

/**	FExec interface */
UBOOL UGFxInteraction::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	UBOOL bResult = FALSE;

	// Command for opening up an swf file located on a harddrive or inside of an unreal package
	// just pass a string with the full file path on disk, or point it to a right package resource
	if(ParseCommand(&Cmd, TEXT("OPENSCENE")))
	{
		// Check if the specified path is not empty
		if(appStrcmp(TEXT(""), Cmd) != 0 && appStrcmp(TEXT(" "), Cmd) != 0)
		{
			// Attempt to open the specified scene
            if(FGFxEngine::GetEngine())
			{
                FGFxMovie* Movie = GGFxEngine->LoadMovie(Cmd);
                if (Movie)
				{
    				GGFxEngine->StartScene(Movie);
				}
			}
		}
		bResult = TRUE;
	}
	else if(ParseCommand(&Cmd, TEXT("CLOSESCENE")))
	{
		if(GGFxEngine)
		{
			GGFxEngine->CloseTopmostScene();
		}
		bResult = TRUE;
	}
    else if(ParseCommand(&Cmd, TEXT("CAPTUREINPUT")))
    {
        if(GGFxEngine)
		{
			GGFxEngine->PlayerStates(0).FocusedMovie->pUMovie->bCaptureInput = TRUE;
		}

		bResult = TRUE;
    }
    else if(ParseCommand(&Cmd, TEXT("RELEASEINPUT")))
    {
        if(GGFxEngine)
		{
			GGFxEngine->PlayerStates(0).FocusedMovie->pUMovie->bCaptureInput = FALSE;
		}

		bResult = TRUE;
    }
	else if(ParseCommand(&Cmd, TEXT("GFXRESTARTMOVIE")))
	{
		if(GGFxEngine)
		{
			FGFxMovie* CurrentMovie = GGFxEngine->GetTopmostMovie();

			if( CurrentMovie != NULL )
			{
				UGFxMoviePlayer* MoviePlayer = CurrentMovie->pUMovie;
				MoviePlayer->Load(CurrentMovie->FileName);
				MoviePlayer->eventStart(FALSE);
				MoviePlayer->Advance(0.f);
			}
		}

		bResult = TRUE;
	}

	else if(ParseCommand(&Cmd, TEXT("GFXGOTOANDPLAY")))
	{
		// Make sure we have an engine, and a parameter string to jump to
		if( GGFxEngine )
		{
			const FString ObjectPath = ParseToken(Cmd, FALSE);
			const FString TargetFrame = ParseToken(Cmd, FALSE);
			
			if( ObjectPath.Len() && TargetFrame.Len() )
		{
			GFxValue MovieRoot;
				if( GGFxEngine->GetTopmostMovie()->pView->GetVariable(&MovieRoot, FTCHARToUTF8(*ObjectPath)) )
			{
					MovieRoot.GotoAndPlay(FTCHARToUTF8(*TargetFrame));
			}
				else
				{
					debugf(TEXT("Could not find object at specified path...%s"), *ObjectPath);
		}
	}
			else
			{
				debugf(TEXT("Invalid path to object or target frame specified..."));
			}
		}

		bResult = TRUE;
	}
	else if(ParseCommand(&Cmd, TEXT("GFXGOTOANDSTOP")))
	{
		// Make sure we have an engine, and a parameter string to jump to
		if( GGFxEngine )
		{
			const FString ObjectPath = ParseToken(Cmd, FALSE);
			const FString TargetFrame = ParseToken(Cmd, FALSE);

			if( ObjectPath.Len() && TargetFrame.Len() )
		{
			GFxValue MovieRoot;
				if( GGFxEngine->GetTopmostMovie()->pView->GetVariable(&MovieRoot, FTCHARToUTF8(*ObjectPath)) )
			{
					MovieRoot.GotoAndStop(FTCHARToUTF8(*TargetFrame));
				}
				else
				{
					debugf(TEXT("Could not find object at specified path...%s"), *ObjectPath);
			}
		}
			else
			{
				debugf(L"Invalid path to object or target frame specified...");
			}
		}

		bResult = TRUE;
	}
	else if(ParseCommand(&Cmd, TEXT("GFXINVOKE")))
	{
		// Make sure we have an engine, and a parameter string to jump to
		if( GGFxEngine )
		{
			const FString ObjectPath = ParseToken(Cmd, FALSE);
			const FString InvokeCommand = ParseToken(Cmd, FALSE);

			if( ObjectPath.Len() && InvokeCommand.Len() )
		{
			GFxValue MovieRoot;
				if( GGFxEngine->GetTopmostMovie()->pView->GetVariable(&MovieRoot, FTCHARToUTF8(*ObjectPath)) )
			{
					MovieRoot.Invoke(FTCHARToUTF8(*InvokeCommand));
			}
				else
				{
					debugf(TEXT("Could not find object at specified path...%s"), *ObjectPath);
				}
			}
			else
			{
				debugf(TEXT("Invalid path to object or target frame specified..."));
		}
	}

		bResult = TRUE;
	}
	else if(GGFxGCManager && ParseCommand(&Cmd, TEXT("DUMPSFTEXTURES")))
	{
		debugf(TEXT("GFx Texture Usage"));
		debugf(TEXT("-----------------------"));

		INT textureSizeTotal = 0;
		INT textureCount = 0;

		for(TArray<FGCReference>::TIterator it(GGFxGCManager->GCReferences); it; ++it)
		{
			if (it->m_object->IsA(UTexture2D::StaticClass()))
			{
				UTexture2D* pTexture = (UTexture2D*)it->m_object;
				FString textureName = pTexture->GetName();
				INT textureSize = pTexture->CalcTextureMemorySize( TMC_ResidentMips );
				textureSizeTotal += textureSize;
				debugf(TEXT("%s - %d Mips - %d Kb"), *textureName, pTexture->ResidentMips, textureSize / 1024);
				++textureCount;
			}
		}

		debugf(TEXT(""));
		debugf(TEXT("%d textures, total of %d Kb used"), textureCount, textureSizeTotal / 1024);
	}
	else if( ParseCommand(&Cmd, TEXT("GFXLISTMOVIES")) )
	{
		if(GGFxEngine)
		{
			debugf( NAME_Log, TEXT("--- OPEN GFX MOVIES --------------------------------") );
			for( INT i = 0; i < GGFxEngine->OpenMovies.Num(); ++i )
			{
				debugf( TEXT("   %s"), *(GGFxEngine->OpenMovies(i)->FileName));
			}
			debugf( NAME_Log, TEXT("----------------------------------------------------") );
		}

		bResult = TRUE;
	}


	return bResult;
}

void UGFxInteraction::Send( ECallbackEventType InType, class FViewport* InViewport, UINT InMessage)
{
	if (GGFxEngine)
	{
		if (InType == CALLBACK_PreViewportResized)
		{
			GGFxEngine->SetRenderViewport(NULL);
		}

		if (InType == CALLBACK_ViewportResized) // && GGFxEngine->GetRenderViewport() == InViewport)
		{
			debugf( NAME_DevGFxUI, TEXT("UGFxInteraction::Send - CALLBACK_ViewportResized") );
			GGFxEngine->SetRenderViewport( InViewport );
		}
	}
}

/** Statistics Gathering */
void UGFxInteraction::CaptureRenderFrameStats()
{
#if STATS
	// Update the GFx Performance counters and stats
    DWORD TextureStats[8]; appMemset(TextureStats, 0, sizeof(DWORD)*8);
    DWORD totalTextureMem = 0;
	DWORD textureObjectCount = 0;
	DWORD gcObjectCount = 0;	// Mem used by other objects.

    if (GGFxGCManager)
	for ( TArray<FGCReference>::TIterator it(GGFxGCManager->GCReferences); it; ++it)
	{
		if (it->m_object->IsA(UTexture2D::StaticClass()))
		{
			UTexture2D* pTexture = (UTexture2D*)it->m_object;
            TextureStats[it->m_statid-STAT_GFxTextureCount]++;
            TextureStats[it->m_statid-STAT_GFxTextureCount+1] += pTexture->CalcTextureMemorySize( TMC_ResidentMips );
		}
		else if (it->m_object->IsA(UTextureRenderTarget2D::StaticClass()))
		{
			UTextureRenderTarget2D* pTexture = (UTextureRenderTarget2D*)it->m_object;
            TextureStats[it->m_statid-STAT_GFxTextureCount]++;
		}
		else
		{
			//NOTE: Right now, this value will always be ZERO, as we only cache Texture2D objects. This case
			//      is being kept around to support later caching of meshes etc.
			++gcObjectCount;
		}
	}

    totalTextureMem = TextureStats[1] + TextureStats[3] + TextureStats[5] + GStatManager.GetStatValueDWORD(STAT_GFxRTTextureMem);
	SET_DWORD_STAT(STAT_GFxGCManagedCount, gcObjectCount);
	SET_DWORD_STAT(STAT_GFxTextureCount, TextureStats[0]);
	SET_DWORD_STAT(STAT_GFxTextureMem, TextureStats[1]);
    SET_DWORD_STAT(STAT_GFxFontTextureCount, TextureStats[2]);
    SET_DWORD_STAT(STAT_GFxFontTextureMem, TextureStats[3]);
    SET_DWORD_STAT(STAT_GFxVideoTextureCount, TextureStats[4]);
    SET_DWORD_STAT(STAT_GFxVideoTextureMem, TextureStats[5]);
	SET_DWORD_STAT(STAT_GFxTotalMem, GGFxAllocator.m_TotalAlloc + totalTextureMem);

	// Update the renderer's primitive stats.
	FGFxRenderer::Stats info;
	GGFxEngine->GetRenderer()->GetRenderStats(&info, TRUE);
	SET_DWORD_STAT(STAT_GFxLinesDrawn, info.Lines );
	SET_DWORD_STAT(STAT_GFxTrianglesDrawn, info.Triangles );
#endif // STATS
}

/** Texture GC Management */

// Returns true if object reference was added, false otherwise
UBOOL UGFxEngine::AddGCReferenceFor( const UObject* const pObjectToBeAdded, INT statid )
{
	// Meant for UI renderer gc texture management
	//CONSIDER: We may want to remove this check in future when we add GC handling for meshes / cached data.
	check( NULL != ConstCast<UTexture>( pObjectToBeAdded ) );
	const INT PreNumObjects = GCReferences.Num();

	for ( TArray<FGCReference>::TIterator it(GCReferences); it; ++it)
	{
		if (it->m_object == pObjectToBeAdded)
		{		
			++it->m_count;
			return TRUE;
		}
	}

	// Didn't find it, so...

	FGCReference gcRef;
	gcRef.m_count = 1;
    gcRef.m_statid = statid;

	// HACK - cast away constness for now since script can't generate const * types that can be gc'd
	gcRef.m_object = const_cast<UObject*>(pObjectToBeAdded);
	
	GCReferences.AddItem( gcRef );
	return ( GCReferences.Num() == ( PreNumObjects + 1 ) );
}

// Returns true if object reference is found (and removed), false otherwise
UBOOL UGFxEngine::RemoveGCReferenceFor( const UObject* const pObjectToBeRemoved )
{
	//CONSIDER: We may want to remove this check in future when we add GC handling for meshes / cached data.
	check( NULL != ConstCast<UTexture>( pObjectToBeRemoved ) );
	for ( TArray<FGCReference>::TIterator Iter( GCReferences ); Iter; ++Iter )
	{
		if (Iter->m_object == pObjectToBeRemoved)
		{
			--Iter->m_count;
			if (Iter->m_count == 0)
			{
                //Nov2007 or earlier
                //Iter.RemoveCurrent();
                GCReferences.Remove(Iter.GetIndex());
			}

			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
 *
 * @return size of resource as to be displayed to artists/ LDs in the Editor.
 */
INT UGFxEngine::GetResourceSize()
{
	extern FGFxAllocator GGFxAllocator;

	return GGFxAllocator.m_FrameAllocPeak;
}

/**
* Dumps memory information about the GFX system
*/ 
void UGFxEngine::DumpGFXMemoryStats(FOutputDevice& Ar)
{
	extern FGFxAllocator GGFxAllocator;

	Ar.Logf(TEXT("GFX System Memory Stats:"));

	Ar.Logf(TEXT("Internal Memory: %dK"), GGFxAllocator.m_FrameAllocPeak/1024);

#if STATS
	DWORD OtherTexture = GET_DWORD_STAT(STAT_GFxTextureMem);
	Ar.Logf(TEXT("Other Texture Memory: %dK"), OtherTexture/1024);

	DWORD FontTexture = GET_DWORD_STAT(STAT_GFxFontTextureMem);
	Ar.Logf(TEXT("Font Texture Memory: %dK"), FontTexture/1024);

	DWORD VideoTexture = GET_DWORD_STAT(STAT_GFxVideoTextureMem);
	Ar.Logf(TEXT("Video Texture Memory: %dK"), VideoTexture/1024);

	DWORD RenderTexture = GET_DWORD_STAT(STAT_GFxRTTextureMem);
	Ar.Logf(TEXT("Render Target Memory: %dK"), RenderTexture/1024);

	DWORD Total = GET_DWORD_STAT(STAT_GFxTotalMem);
	Ar.Logf(TEXT("Total Memory: %dK"), Total/1024);
#endif

}

#endif // WITH_GFx


UGFxMoviePlayer* UGFxInteraction::GetFocusMovie(INT ControllerId)
{
#if WITH_GFx

    if (GGFxEngine && GGFxEngine->GetFocusMovie(ControllerId))
        return GGFxEngine->GetFocusMovie(ControllerId)->pUMovie;
    else
        return NULL;
#else

	return NULL;

#endif // WITH_GFx
}

#if WITH_GFx
UBOOL UGFxInteraction::InputKey(INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed,UBOOL bGamepad)
{
	extern UBOOL GRenderGFxUI;

	if (GGFxEngine && GRenderGFxUI)
        return GGFxEngine->InputKey(ControllerId,Key, Event);
    return 0;
}

UBOOL UGFxInteraction::InputAxis(INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime,UBOOL bGamepad)
{
	extern UBOOL GRenderGFxUI;

	if (GGFxEngine && GRenderGFxUI)
        return GGFxEngine->InputAxis(ControllerId,Key,Delta,DeltaTime,bGamepad);
    return 0;
}

UBOOL UGFxInteraction::InputChar(INT ControllerId,TCHAR Character)
{
	extern UBOOL GRenderGFxUI;

	if (GGFxEngine && GRenderGFxUI)
        return GGFxEngine->InputChar(ControllerId,Character);
    return 0;
}
#endif // WITH_GFx

void UGFxAction_OpenMovie::Activated()
{
#if WITH_GFx

    if (!GEngine || !GEngine->GameViewport)
        return;

    Super::Activated();

	// create the movie player
	if ( Movie && MoviePlayerClass )
	{
		MoviePlayer = Cast<UGFxMoviePlayer>(StaticConstructObject( MoviePlayerClass, GetOuter() )); 
		MoviePlayer->MovieInfo = Movie;
	}

	// assign to any Linked variables
	TArray<UObject**> ObjVars;
	GetObjectVars(ObjVars,TEXT("Movie Player"));
	for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
	{
		*(ObjVars(Idx)) = MoviePlayer;
	}

	// start the movie player
    if (MoviePlayer)
    {
        TArray<UObject**> ObjectVars;
        GetObjectVars(ObjectVars,TEXT("External Interface"));
        if (ObjectVars.Num() > 0)
            MoviePlayer->ExternalInterface = *ObjectVars(0);

        ObjectVars.Empty();
        GetObjectVars(ObjectVars,TEXT("Player Owner"));
        if (ObjectVars.Num() > 0)
			MoviePlayer->LocalPlayerOwnerIndex = GEngine->GamePlayers.FindItemIndex(Cast<ULocalPlayer>(Cast<APlayerController>(*ObjectVars(0))->Player));

		MoviePlayer->bEnableGammaCorrection = this->bEnableGammaCorrection;
		MoviePlayer->bDisplayWithHudOff = bDisplayWithHudOff;
		MoviePlayer->RenderTextureMode = RenderTextureMode;
		MoviePlayer->RenderTexture = RenderTexture;

		for ( INT i=0; i<CaptureKeys.Num(); i++ )
		{
			if ( CaptureKeys(i) != NAME_None )
			{
				MoviePlayer->AddCaptureKey(CaptureKeys(i));
			}
		}

		for ( INT i=0; i<FocusIgnoreKeys.Num(); i++ )
		{
			if ( FocusIgnoreKeys(i) != NAME_None )
			{
				MoviePlayer->AddFocusIgnoreKey(CaptureKeys(i));
			}
		}

		UBOOL success = MoviePlayer->eventStart(bStartPaused);

        if (OutputLinks.Num() > 0)
        {
            if (success)
			{
                OutputLinks(0).ActivateOutputLink();
			}
            else if (OutputLinks.Num() > 1)
			{
                OutputLinks(1).ActivateOutputLink();
        }
    }
    }
    else if (OutputLinks.Num() > 1)
	{
        OutputLinks(1).ActivateOutputLink();
	}
#endif // WITH_GFx
}

void UGFxAction_CloseMovie::Activated()
{

#if WITH_GFx

    Super::Activated();

	// get the attached movie player
	TArray<UObject**> MovieVars;
	GetObjectVars(MovieVars,TEXT("Movie Player"));
	if (MovieVars.Num() > 0)
	{
	Movie = Cast<UGFxMoviePlayer>(*(MovieVars(0)));
	}
	else
	{
		Movie = NULL;
	}

    if (Movie)
    {
        Movie->Close(bUnload);

        if (OutputLinks.Num() > 0)
            OutputLinks(0).ActivateOutputLink();
    }
    else if (OutputLinks.Num() > 1)
        OutputLinks(1).ActivateOutputLink();

#endif // WITH_GFx
}

void UGFxAction_SetCaptureKeys::Activated()
{
#if WITH_GFx

	check(InputLinks.Num() == 2);
    Super::Activated();

	// adding keys
	if (InputLinks(0).bHasImpulse)
	{
		SetKeys();
	}
	
	// remove keys
	if (InputLinks(1).bHasImpulse)
	{
		RemoveKeys();
	}

#endif // WITH_GFx
}

#if WITH_GFx

/**
 * Activate the keys that we'd like to listen to
 */
void UGFxAction_SetCaptureKeys::SetKeys()
{
	// get the attached movie player
	TArray<UObject**> MovieVars;
	GetObjectVars(MovieVars,TEXT("Movie Player"));
	if (MovieVars.Num() > 0)
	{
	Movie = Cast<UGFxMoviePlayer>(*(MovieVars(0)));
	}
	else
	{
		Movie = NULL;
	}

    if (Movie)
    {
        Movie->FlushPlayerInput(TRUE);
		//Movie->ClearCaptureKeys();

        if (CaptureKeys.Num())
        {
			//if no key capture list exists yet, make one
			if (Movie->pCaptureKeys == NULL)
			{
            Movie->pCaptureKeys = new TSet<NAME_INDEX>;
			}

            for (INT i = 0; i < CaptureKeys.Num(); i++)
			{
                Movie->pCaptureKeys->Add(CaptureKeys(i).GetIndex());
			}

			Movie->FlushPlayerInput(TRUE);
		}
	}
}


/**
 * Deactivated the keys that were previously set
 */
void UGFxAction_SetCaptureKeys::RemoveKeys()
{
	// get the attached movie player
	TArray<UObject**> MovieVars;
	GetObjectVars(MovieVars,TEXT("Movie Player"));
	if (MovieVars.Num() > 0)
	{
	Movie = Cast<UGFxMoviePlayer>(*(MovieVars(0)));
	}
	else
	{
		Movie = NULL;
	}

	if (Movie)
	{
		Movie->FlushPlayerInput(TRUE);
		//Movie->ClearCaptureKeys();

		if (CaptureKeys.Num() && (Movie->pCaptureKeys != NULL))
		{
			for (INT i = 0; i < CaptureKeys.Num(); i++)
			{
				Movie->pCaptureKeys->RemoveKey(CaptureKeys(i).GetIndex());
			}
			//if that was the last of the keys, clean up the memory.
			if (Movie->pCaptureKeys->Num() == 0)
			{
				delete Movie->pCaptureKeys;
				Movie->pCaptureKeys = NULL;
			}

            Movie->FlushPlayerInput(TRUE);
        }
    }
}


static UBOOL SeqVarToASValue(FASValue& ASVal, USequenceVariable* SeqVar)
{
    FLOAT*   FloatRef = 0;
    INT*     IntRef   = 0;
    FString* StrRef   = 0;
    UBOOL*   BoolRef  = 0;

    if (SeqVar->GetClass() == USeqVar_Float::StaticClass())
        FloatRef = Cast<USeqVar_Float>(SeqVar)->GetRef();

    if (SeqVar->GetClass() == USeqVar_Int::StaticClass())
        IntRef = Cast<USeqVar_Int>(SeqVar)->GetRef();

    if (SeqVar->GetClass() == USeqVar_String::StaticClass())
        StrRef = Cast<USeqVar_String>(SeqVar)->GetRef();

    if (SeqVar->GetClass() == USeqVar_Bool::StaticClass())
        BoolRef = Cast<USeqVar_Bool>(SeqVar)->GetRef();

    if (FloatRef && (*FloatRef != 0 || !StrRef))
    {
        ASVal.Type = AS_Number;
        ASVal.N = *FloatRef;
        return TRUE;
    }
    else if (IntRef && (*IntRef != 0 || !StrRef))
    {
        ASVal.Type = AS_Number;
        ASVal.N = *IntRef;
        return TRUE;
    }
    else if (StrRef)
    {
        ASVal.Type = AS_String;
        ASVal.S = *StrRef;
        return TRUE;
    }
    else if (BoolRef)
    {
        ASVal.Type = AS_Boolean;
        ASVal.B = *BoolRef;
        return TRUE;
    }
    return FALSE;
}

static UBOOL ASValueToSeqVar(USequenceVariable* SeqVar, const FASValue& ASVal)
{
    FLOAT*   FloatRef  = 0;
    INT*     IntRef    = 0;
    UBOOL*   BoolRef   = 0;
    FString* StringRef = 0;

    switch (ASVal.Type)
    {
    case AS_Number:
        if (SeqVar->GetClass() == USeqVar_Float::StaticClass())
            FloatRef = Cast<USeqVar_Float>(SeqVar)->GetRef();
        if (FloatRef)
            *FloatRef = ASVal.N;
        else
        {
            if (SeqVar->GetClass() == USeqVar_Int::StaticClass())
                IntRef = Cast<USeqVar_Int>(SeqVar)->GetRef();
            if (IntRef)
                *IntRef = INT(ASVal.N);
        }
        return TRUE;

    case AS_Boolean:
        if (SeqVar->GetClass() == USeqVar_Bool::StaticClass())
            BoolRef = Cast<USeqVar_Bool>(SeqVar)->GetRef();
        if (BoolRef)
            *BoolRef = ASVal.B;
        return TRUE;

    case AS_String:
        if (SeqVar->GetClass() == USeqVar_String::StaticClass())
            StringRef = Cast<USeqVar_String>(SeqVar)->GetRef();
        if (StringRef)
            *StringRef = ASVal.S;
        return TRUE;
    }
    return FALSE;
}
#endif // WITH_GFx

void UGFxAction_GetVariable::Activated()
{
#if WITH_GFx
    Super::Activated();

	// get the attached movie player
	TArray<UObject**> MovieVars;
	GetObjectVars(MovieVars,TEXT("Movie Player"));
	if (MovieVars.Num() > 0)
	{
	Movie = Cast<UGFxMoviePlayer>(*(MovieVars(0)));
	}
	else
	{
		Movie = NULL;
	}

    if (Movie)
    {
        FASValue Result = Movie->GetVariable(Variable);

        for (INT Idx = 0; Idx < VariableLinks.Num(); Idx++)
            if (VariableLinks(Idx).LinkDesc == TEXT("Result"))
            {
                for (INT LinkIdx = 0; LinkIdx < VariableLinks(Idx).LinkedVariables.Num(); LinkIdx++)
                    if (VariableLinks(Idx).LinkedVariables(LinkIdx) != NULL)
                        ASValueToSeqVar(VariableLinks(Idx).LinkedVariables(LinkIdx), Result);
            }

            if (OutputLinks.Num() > 0)
                OutputLinks(0).ActivateOutputLink();
    }

#endif // WITH_GFx
}

void UGFxAction_SetVariable::Activated()
{
#if WITH_GFx
    Super::Activated();

	// get the attached movie player
	TArray<UObject**> MovieVars;
	GetObjectVars(MovieVars,TEXT("Movie Player"));
	if (MovieVars.Num() > 0)
	{
	Movie = Cast<UGFxMoviePlayer>(*(MovieVars(0)));
	}
	else
	{
		Movie = NULL;
	}

    if (Movie)
    {
        FASValue Result = Movie->GetVariable(Variable);

        for (INT Idx = 0; Idx < VariableLinks.Num(); Idx++)
            if (VariableLinks(Idx).LinkDesc == TEXT("Value"))
            {
                for (INT LinkIdx = 0; LinkIdx < VariableLinks(Idx).LinkedVariables.Num(); LinkIdx++)
                    if (VariableLinks(Idx).LinkedVariables(LinkIdx) != NULL)
                    {
                        FASValue ASVal;
                        SeqVarToASValue(ASVal, VariableLinks(Idx).LinkedVariables(LinkIdx));
                        Movie->SetVariable(Variable, ASVal);

                        if (OutputLinks.Num() > 0)
                            OutputLinks(0).ActivateOutputLink();
                        return;
                    }
            }
    }

#endif // WITH_GFx
}

void UGFxAction_Invoke::Activated()
{
#if WITH_GFx
    Super::Activated();

	// get the attached movie player
	TArray<UObject**> MovieVars;
	GetObjectVars(MovieVars,TEXT("Movie Player"));
	if (MovieVars.Num() > 0)
	{
	Movie = Cast<UGFxMoviePlayer>(*(MovieVars(0)));
	}
	else
	{
		Movie = NULL;
	}

    if (Movie)
    {
        for (INT Idx = 0; Idx < VariableLinks.Num(); Idx++)
            if (VariableLinks(Idx).LinkDesc.Left(9) == TEXT("Argument["))
            {
                INT ArgIdx = appAtoi(*VariableLinks(Idx).LinkDesc.Mid(9));
                for (INT LinkIdx = 0; LinkIdx < VariableLinks(Idx).LinkedVariables.Num(); LinkIdx++)
                    if (VariableLinks(Idx).LinkedVariables(LinkIdx) != NULL)
                    {
                        if (SeqVarToASValue(Arguments(ArgIdx), VariableLinks(Idx).LinkedVariables(LinkIdx)))
                            break;
                    }
            }

        FASValue Result = Movie->Invoke(MethodName, Arguments);

        for (INT Idx = 0; Idx < VariableLinks.Num(); Idx++)
        {
            if (VariableLinks(Idx).LinkDesc == TEXT("Result"))
            {
                for (INT LinkIdx = 0; LinkIdx < VariableLinks(Idx).LinkedVariables.Num(); LinkIdx++)
                    if (VariableLinks(Idx).LinkedVariables(LinkIdx) != NULL)
                    {
                        ASValueToSeqVar(VariableLinks(Idx).LinkedVariables(LinkIdx), Result);
                    }
            }
        }

        if (OutputLinks.Num() > 0)
            OutputLinks(0).ActivateOutputLink();
    }
#endif // WITH_GFx
}


void UGFxEvent_FSCommand::FinishDestroy()
{
#if WITH_GFx
	Handler = NULL;
#endif // WITH_GFx

	Super::FinishDestroy();
}

UBOOL UGFxEvent_FSCommand::RegisterEvent()
{
#if WITH_GFx
    UBOOL Result = Super::RegisterEvent();

	if (Result)
	{
		Handler = ConstructObject<UGFxFSCmdHandler_Kismet>(UGFxFSCmdHandler_Kismet::StaticClass());
    }

    return Result;

#else // WITH_GFX = 0

	return FALSE;

#endif // WITH_GFx
}

/**
 * Adds an error message to the map check dialog if this SequenceEvent's EventActivator is bStatic
 */
#if WITH_EDITOR
void  UGFxEvent_FSCommand::CheckForErrors()
{
	Super::CheckForErrors();

	GWarn->MapCheck_Add(
		MCTYPE_KISMET, 
		this,
		*FString::Printf(TEXT("FS Commands should not be used for production.  This is for prototyping only %s (Originator)"), *GetFullName())
		);
}
#endif

UBOOL UGFxFSCmdHandler_Kismet::FSCommand(class UGFxMoviePlayer* Movie, class UGFxEvent_FSCommand* Event, const FString& Cmd,const FString& arg)
{

#if WITH_GFx
	check(Event);
	if( Event->CheckActivate(GWorld->GetWorldInfo(),NULL,0) )
	{
		TArray<FString*> ArgVars;
		Event->GetStringVars(ArgVars,TEXT("Argument"));
		for( INT i = 0; i < ArgVars.Num(); i++)
		{
			*ArgVars(0) = arg;
		}
		return TRUE;
	}
#endif //WITH_GFx

	return FALSE;
	
}

