/**********************************************************************

Filename    :   GFxUIFullscreenMovie.cpp
Content     :   

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxUI.h"


#if WITH_GFx

#include "GFxUIEngine.h"
#include "GFxUIRenderer.h"
#include "GFxUIRendererRT.h"
#include "GFxUIImageInfo.h"
#include "GFxUIStats.h"
#include "GFxUIAllocator.h"
#include "GFxUIFullscreenMovie.h"
#include "../../Engine/Src/SceneRenderTargets.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push, 8)
#endif

#include "GTypes.h"
#include "GMemory.h"
#include "GRefCount.h"
#include "GRenderer.h"
#include "GFxPlayer.h"
#include "GFile.h"
#include "GThreads.h"
#include "GSysFile.h"
#include "GFxLoader.h"
#include "GFxImageResource.h"
#include "GFxLog.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(pop)
#endif

class FGFxFSMFSCommandHandler : public GFxFSCommandHandler
{
public:
	/** Callback through which the movie sends messages to the game */
	virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg)    
	{
#if WITH_GFx_FULLSCREEN_MOVIE
		check(pmovie);

		if (pmovie->GetUserData())
		{
			FFullScreenMovieGFx* Player = (FFullScreenMovieGFx*)pmovie->GetUserData();

			if (!strcmp(pcommand, "stopMovie"))
			{
				Player->DoStop = 1;
			}
		}
#endif
	}
};

class FGFxFSMImageCreator : public GFxImageCreator
{
public:
	virtual GImageInfoBase* CreateImage(const GFxImageCreateInfo &info)
	{
		GImageInfoBase *pimageInfo = NULL;

		switch(info.Type)
		{
		case GFxImageCreateInfo::Input_None:
			pimageInfo = new GImageInfo(); break;
		case GFxImageCreateInfo::Input_Image:             
			pimageInfo = new GImageInfo(info.pImage); break;

		case GFxImageCreateInfo::Input_File:
			{
				const char *pkgpath = IsPackagePath(info.pFileInfo->FileName);
				check(pkgpath);

				FString   strFilename = FFilename(pkgpath).GetBaseFilename(FALSE);
				strFilename = FGFxEngine::CollapseRelativePath(strFilename);

				FGFxEngine::ReplaceCharsInFString(strFilename, PATH_SEPARATOR, L'.');

				pimageInfo = new FGFxImageInfo(TCHAR_TO_ANSI(*strFilename),
					info.pFileInfo->TargetWidth, info.pFileInfo->TargetHeight);
			}
			break;


		default:
			checkMsg( 0, "GFxFSMImageCreateCallback: Unrecognized Image Creation Info" ); 
			pimageInfo = new GImageInfo(); break;
		}

		pimageInfo->GetTexture(info.pRenderConfig->GetRenderer());
		return pimageInfo;
	}
};

static const int RTTempTextureCounts[] = {0, 24, 8, 8};

#if WITH_GFx_FULLSCREEN_MOVIE

FFullScreenMovieGFx::FFullScreenMovieGFx(UBOOL bUseSound/* =TRUE */) : Viewport(NULL)
{
	Loader = new GFxLoader;
	Loader->SetResourceLib(GGFxEngine->Loader.GetResourceLib());
	FGFxEngine::InitGFxLoaderCommon(*Loader);

	Renderer = new FGFxRendererRT();
	GPtr<GFxRenderConfig> prenderConfig = 
		*new GFxRenderConfig(Renderer, DEFAULT_STROKE_TYPE | GFxRenderConfig::RF_EdgeAA);
	prenderConfig->SetMaxCurvePixelError(DEFAULT_CURVE_ERROR);
	Loader->SetRenderConfig(prenderConfig);

	GPtr<FGFxFSMImageCreator> pimageCreator = *new FGFxFSMImageCreator();
	Loader->SetImageCreator(pimageCreator);

	GPtr<GFxFSCommandHandler> pfscommandHandler = *new FGFxFSMFSCommandHandler();
	Loader->SetFSCommandHandler(pfscommandHandler);

#ifndef GFX_NO_LOCALIZATION
	FontlibInitialized = (GGFxEngine->pFontLib.GetPtr() != NULL);
	if (GGFxEngine->pFontMap)
		Loader->SetFontMap(GGFxEngine->pFontMap);
	if (GGFxEngine->pFontLib)
		Loader->SetFontLib(GGFxEngine->pFontLib);
	if (GGFxEngine->pTranslator)
		Loader->SetTranslator(GGFxEngine->pTranslator);
#endif

	DoStop = 2;
	GGFxEngine->FullscreenMovie = this;
}

static FFullScreenMovieGFx* StaticInstance = NULL;

FFullScreenMovieSupport* FFullScreenMovieGFx::StaticInitialize(UBOOL bUseSound)
{
	if( !StaticInstance )
	{
		FGFxEngine::GetEngine();

		StaticInstance = new FFullScreenMovieGFx(bUseSound);
	}
	return StaticInstance;
}

FFullScreenMovieGFx::~FFullScreenMovieGFx()
{

}

void FFullScreenMovieGFx::Shutdown()
{
	if (StaticInstance)
	{
		if (IsInGameThread())
		{
			StaticInstance->GameThreadStopMovie(0, true, true);
			StaticInstance->MovieView = NULL;
			StaticInstance->MovieDef = NULL;

			if (StaticInstance->Loader)
			{
				delete StaticInstance->Loader;
			}
			StaticInstance->Loader = NULL;
		}
		else if (IsInRenderingThread())
		{
			delete StaticInstance;
			StaticInstance = NULL;
		}
	}
}

UBOOL FFullScreenMovieGFx::IsTickable() const
{
	return (IsStopped == 0);
}

void FFullScreenMovieGFx::Tick(FLOAT DeltaTime)
{
	UBOOL bisDrawing = RHIIsDrawingViewport();

	if (DoStop >= (GGFxEngine->GameHasRendered > 1 ? 1 : 2))
	{
		Mutex.Lock();
		IsStopped = TRUE;
		MovieFinished.NotifyAll();
		Mutex.Unlock();
		return;
	}

	if (Viewport==NULL || ((GFxMovieView*)MovieView==NULL))
		return;

	if (!bisDrawing)
	{
		Viewport->BeginRenderFrame();
	}

	FGFxRenderTarget::NativeRenderTarget rt;

	static FSceneRenderTargetProxy SceneColorTarget;
	static FSceneDepthTargetProxy SceneDepthTarget;
	bool   bSetState = TRUE;

	if (GSystemSettings.UsesMSAA())
	{
		// need non-msaa depth buffer to match viewport.
		/*
		if (!GGFxEngine->ExtraDepthBuffer)
		GGFxEngine->ExtraDepthBuffer = new FGFxRenderResources;
		GGFxEngine->ExtraDepthBuffer->Allocate_RenderThread(Viewport->GetSizeX(), Viewport->GetSizeY());
		RHISetRenderTarget(Viewport->GetRenderTargetSurface(), GGFxEngine->ExtraDepthBuffer->GetDepthSurface());
		*/
		rt.Owner = &SceneColorTarget;
		// XXX
	}
	else
	{
		// Set viewport buffer as RT, and "Scene Depth" as depth+stencil. The viewport does not provide a depth buffer.

		Renderer->SetRenderTarget(Viewport);
		//RHISetRenderTarget(Viewport->GetRenderTargetSurface(), GSceneRenderTargets.GetSceneDepthSurface());
		rt.Owner = Viewport;
		rt.OwnerDepth = &SceneDepthTarget;
	}
	GGFxEngine->HudRenderTarget->InitRenderTarget_RenderThread(rt);
	Renderer->SetDisplayRenderTarget_RenderThread(GGFxEngine->HudRenderTarget, bSetState);

	Float CurTime = appSeconds();
	if (NextAdvanceTime > CurTime)
	{
		appSleep(NextAdvanceTime-CurTime);
		CurTime = appSeconds();
	}
	NextAdvanceTime = CurTime + MovieView->Advance(CurTime-LastAdvanceTime, 0);
	if (!MovieHidden)
	{
		MovieView->Display();
	}

	if (!bisDrawing)
	{
		Viewport->EndRenderFrame(TRUE, TRUE);
	}
}

void FFullScreenMovieGFx::GameThreadPlayMovie(EMovieMode InMovieMode, const TCHAR* InMovieFilename, INT StartFrame, INT InStartOfRenderingMovieFrame, INT InEndOfRenderingMovieFrame)
{
	if (!Loader)
	{
		return;
	}
	GameThreadStopMovie(0,TRUE,TRUE);

	UInt            loadConstants = GFxLoader::LoadAll | GFxLoader::LoadWaitCompletion;
	FViewport*      Viewport = GEngine->GameViewport->Viewport;
	FString         PkgPath = FString(TEXT("/ package/")) + InMovieFilename;
	USwfMovie* pmovieInfo = LoadObject<USwfMovie>(NULL, InMovieFilename, NULL, LOAD_None, NULL);

	FGFxEngine::ReplaceCharsInFString(PkgPath, TEXT("."), TEXT('/'));

#ifndef GFX_NO_LOCALIZATION
	if (!FontlibInitialized)
	{
		// Fontlib must be configured before attempting to load movies that use it.
		if (pmovieInfo && pmovieInfo->bUsesFontlib)
		{
			GGFxEngine->InitFontlib();
			Loader->SetFontLib(GGFxEngine->pFontLib);
			FontlibInitialized = TRUE;
		}
	}
#endif

	GPtr<GFxMovieDef> MovieDef = *Loader->CreateMovie(FTCHARToUTF8(*PkgPath), loadConstants);
	if (!MovieDef)
	{
		return;
	}

	// preallocate textures for "creation" on render thread
	for (INT Type = 1; Type < 4; Type++)
	{
		INT NumTex = RTTempTextureCounts[Type];
		if (Type == 1 && pmovieInfo->RTTextures > NumTex)
		{
			NumTex = pmovieInfo->RTTextures;
		}
		else if (Type == 3 && pmovieInfo->RTVideoTextures * 4 > NumTex)
		{
			NumTex = pmovieInfo->RTVideoTextures * 4;
		}

		FGFxRendererRT* pRenderer = (FGFxRendererRT*) Loader->GetRenderer().GetPtr();
		for (INT i = pRenderer->RTAllocTextures[Type].Num(); i < NumTex; i++)
		{
			UTexture2D* Tex = FGFxRendererImpl::CreateNativeTexture(pRenderer, Type, STAT_GFxVideoTextureCount);
			pRenderer->RTAllocTextures[Type].Push(Tex);
			pRenderer->RTAllTextures.Set(Tex,Type);
		}
	}

	GPtr<GFxMovieView> NewMovieView = *MovieDef->CreateInstance(1);
	NewMovieView->SetViewport(Viewport->GetSizeX(),Viewport->GetSizeY(),0,0,Viewport->GetSizeX(),Viewport->GetSizeY(), GViewport_NoGamma);
	NewMovieView->SetUserData(this);

	if (StartFrame)
	{
		NewMovieView->Advance(StartFrame / MovieDef->GetFrameRate());
	}

	DoStop = 0;
	IsStopped = FALSE;
	MovieHidden = 0;
	MovieFilename = InMovieFilename;

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(FFullScreenMovieGFxPlayer,
		FFullScreenMovieGFx*,Player,this, GPtr<GFxMovieView>,MovieView,NewMovieView,GPtr<GFxMovieDef>,MovieDef,MovieDef,
	{
		// ensure depth buffer is large enough
		GSceneRenderTargets.Allocate(GEngine->GameViewport->Viewport->GetSizeX(), GEngine->GameViewport->Viewport->GetSizeY());

		Player->MovieDef = MovieDef;
		Player->MovieView = MovieView;
		Player->LastAdvanceTime = appSeconds();
		Player->NextAdvanceTime = 0;
		Player->Viewport = GEngine->GameViewport->Viewport;
	});
}

void FFullScreenMovieGFx::GameThreadStopMovie(FLOAT DelayInSeconds,UBOOL bWaitForMovie,UBOOL bForceStop)
{
	DoStop = 2;
	if (bWaitForMovie)
	{
		GameThreadWaitForMovie();
	}
}

void FFullScreenMovieGFx::GameThreadRequestDelayedStopMovie()
{
	GGFxEngine->GameHasRendered = 0;
	DoStop = 1;
}

void FFullScreenMovieGFx::GameThreadWaitForMovie()
{
	Mutex.Lock();
	if (MovieView)
	{
		MovieFinished.Wait(&Mutex);
	}
	Mutex.Unlock();
	MovieDef = 0;
	MovieView = 0;
}

UBOOL FFullScreenMovieGFx::GameThreadIsMovieFinished(const TCHAR* InMovieFilename)
{
	return !GameThreadIsMoviePlaying(InMovieFilename);
}

UBOOL FFullScreenMovieGFx::GameThreadIsMoviePlaying(const TCHAR* InMovieFilename)
{
	UBOOL Result = MovieView.GetPtr() != 0 && (MovieFilename == InMovieFilename) && IsStopped;
	if (IsStopped)
	{
		MovieDef = 0;
		MovieView = 0;
	}
	return Result;
}

FString FFullScreenMovieGFx::GameThreadGetLastMovieName()
{
	return MovieFilename;
}

void FFullScreenMovieGFx::GameThreadInitiateStartupSequence()
{

}

INT FFullScreenMovieGFx::GameThreadGetCurrentFrame()
{
	GMutex::Locker locker(&Mutex);
	if (MovieView)
	{
		return MovieView->GetCurrentFrame();
	}
	else
	{
		return 0;
	}
}

void FFullScreenMovieGFx::ReleaseDynamicResources()
{

}

void FFullScreenMovieGFx::GameThreadSetMovieHidden( UBOOL bInHidden )
{
	MovieHidden = bInHidden;
}

UBOOL FFullScreenMovieGFx::InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent EventType,FLOAT AmountDepressed,UBOOL bGamepad)
{
	return FALSE;
}

#endif // WITH_GFx_FULLSCREEN_MOVIE

#endif // WITH_GFx
