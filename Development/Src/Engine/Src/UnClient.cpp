/*=============================================================================
	UnClient.cpp: UClient implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineAudioDeviceClasses.h"

IMPLEMENT_CLASS(UClient);

UBOOL FViewport::bIsGameRenderingEnabled = TRUE;



/**
 * Helper class for FViewport::TiledScreenshot().
 *
 * This class creates a BMP file and allows the user to copy pieces of
 * FColor bitmaps directly into the file, without ever having to store
 * the entire bitmap in memory.
 */
class FBitmapFile
{
public:
	FBitmapFile( );
	~FBitmapFile( );

	/**
	 * Creates the BMP file and writes the header and leaves it open.
	 *
	 * @param	FilenamePattern	Prefix for the file name
	 * @param	Width			Width in pixels
	 * @param	Height			Height in pixels
	 * 
	 * @return	TRUE if the file was successfully created, FALSE otherwise
	 */
	UBOOL		Create( const TCHAR* FilenamePattern, INT Width, INT Height );

	/**
	 * Copies an FColor image directly into the BMP file.
	 *
	 * @param	Source		Pointer to the source image
	 * @param	SrcWidth	Width of the source, in pixels
	 * @param	SrcHeight	Height of the source, in pixels
	 * @param	DstX		Upper-left x-coordinate of where to store the source image
	 * @param	DstY		Upper-left y-coordinate of where to store the source image
	 * @param	SrcRect		Optional sub-rectangle to constrain the source image, may be NULL
	 */
	void		CopyRect( FColor* Source, INT SrcWidth, INT SrcHeight, INT DstX=0, INT DstY=0, const FIntRect* SrcRect=NULL );

	/**
	 * Closes the BMP file.
	 *
	 * This function is automatically called by the destructor.
	 */
	void		Close( );

protected:
	FArchive*	Ar;
	INT			Width;
	INT			Height;
	INT			BytesPerLine;
	INT			FileOffset;
	static INT	BitmapIndex;
};

INT	FBitmapFile::BitmapIndex = -1;

FBitmapFile::FBitmapFile()
{
	appMemzero( this, sizeof(FBitmapFile) );
}

FBitmapFile::~FBitmapFile()
{
	Close();
}

/**
 * Creates the BMP file and writes the header and leaves it open.
 *
 * @param	FilenamePattern	Prefix for the file name
 * @param	Width			Width in pixels
 * @param	Height			Height in pixels
 * 
 * @return	TRUE if the file was successfully created, FALSE otherwise
 */
UBOOL FBitmapFile::Create( const TCHAR* FilenamePattern, INT Width, INT Height )
{
	TCHAR File[MAX_SPRINTF]=TEXT("");
	if( BitmapIndex == -1 )
	{
		for( INT TestBitmapIndex=0; TestBitmapIndex<65536; TestBitmapIndex++ )
		{
			appSprintf( File, TEXT("%s%05i.bmp"), FilenamePattern, TestBitmapIndex );
			if( GFileManager->FileSize(File) < 0 )
			{
				BitmapIndex = TestBitmapIndex;
				break;
			}
		}
	}

	appSprintf( File, TEXT("%s%05i.bmp"), FilenamePattern, BitmapIndex++ );

	if( GFileManager->FileSize(File)<0 )
	{
		Ar = GFileManager->CreateFileWriter( File );
		if( Ar )
		{
			// Types.
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,1)
#endif
			struct BITMAPFILEHEADER
			{
				WORD	bfType GCC_PACK(1);
				DWORD	bfSize GCC_PACK(1);
				WORD	bfReserved1 GCC_PACK(1); 
				WORD	bfReserved2 GCC_PACK(1);
				DWORD	bfOffBits GCC_PACK(1);
			} FH; 
			struct BITMAPINFOHEADER
			{
				DWORD	biSize GCC_PACK(1); 
				INT		biWidth GCC_PACK(1);
				INT		biHeight GCC_PACK(1);
				WORD	biPlanes GCC_PACK(1);
				WORD	biBitCount GCC_PACK(1);
				DWORD	biCompression GCC_PACK(1);
				DWORD	biSizeImage GCC_PACK(1);
				INT		biXPelsPerMeter GCC_PACK(1); 
				INT		biYPelsPerMeter GCC_PACK(1);
				DWORD	biClrUsed GCC_PACK(1);
				DWORD	biClrImportant GCC_PACK(1); 
			} IH;
#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

			BytesPerLine = Align(Width * 3,4);
			this->Width = Width;
			this->Height = Height;

			// File header.
			FH.bfType       		= INTEL_ORDER16((WORD) ('B' + 256*'M'));
			FH.bfSize       		= INTEL_ORDER32((DWORD) (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + BytesPerLine * Height));
			FH.bfReserved1  		= INTEL_ORDER16((WORD) 0);
			FH.bfReserved2  		= INTEL_ORDER16((WORD) 0);
			FH.bfOffBits    		= INTEL_ORDER32((DWORD) (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)));
			Ar->Serialize( &FH, sizeof(FH) );

			// Info header.
			IH.biSize               = INTEL_ORDER32((DWORD) sizeof(BITMAPINFOHEADER));
			IH.biWidth              = INTEL_ORDER32((DWORD) Width);
			IH.biHeight             = INTEL_ORDER32((DWORD) Height);
			IH.biPlanes             = INTEL_ORDER16((WORD) 1);
			IH.biBitCount           = INTEL_ORDER16((WORD) 24);
			IH.biCompression        = INTEL_ORDER32((DWORD) 0); //BI_RGB
			IH.biSizeImage          = INTEL_ORDER32((DWORD) BytesPerLine * Height);
			IH.biXPelsPerMeter      = INTEL_ORDER32((DWORD) 0);
			IH.biYPelsPerMeter      = INTEL_ORDER32((DWORD) 0);
			IH.biClrUsed            = INTEL_ORDER32((DWORD) 0);
			IH.biClrImportant       = INTEL_ORDER32((DWORD) 0);
			Ar->Serialize( &IH, sizeof(IH) );

			FileOffset = Ar->Tell();
		}
		else 
			return FALSE;
	}
	else 
		return FALSE;

	// Success.
	return TRUE;
}

/**
 * Copies an FColor image directly into the BMP file.
 *
 * @param	Source		Pointer to the source image
 * @param	SrcWidth	Width of the source, in pixels
 * @param	SrcHeight	Height of the source, in pixels
 * @param	DstX		Upper-left x-coordinate of where to store the source image
 * @param	DstY		Upper-left y-coordinate of where to store the source image
 * @param	SrcRect		Optional sub-rectangle to constrain the source image, may be NULL
 */
void FBitmapFile::CopyRect( FColor* Source, INT SrcWidth, INT SrcHeight, INT DstX/*=0*/, INT DstY/*=0*/, const FIntRect* SrcRect/*=NULL*/ )
{
	// In pixels:
	BYTE PadBytes[4] = { 0, 0, 0, 0 };
	INT Padding		= 0;
	INT SrcOffset	= SrcRect ? (SrcRect->Min.Y * SrcWidth + SrcRect->Min.X) : 0;
	INT NumRows		= SrcRect ? SrcRect->Height() : SrcHeight;
	INT NumColumns	= SrcRect ? SrcRect->Width() : SrcWidth;
	Source			+= SrcOffset;

	// Clamp to bottom
	if ( NumRows >= (Height - DstY) )
		NumRows		= Height - DstY;

	// Clamp to right edge
	if ( NumColumns >= (Width - DstX) )
	{
		NumColumns	= Width - DstX;
		Padding		= BytesPerLine - Width*3;	// Pad line to 4 byte alignment
	}

	// In Bytes:
	INT DstOffset	= FileOffset + (Height - DstY - NumRows) * BytesPerLine + DstX * 3;

	for ( INT Y=0; Y < NumRows; ++Y )
	{
		FColor *Src = Source + (NumRows - Y - 1) * SrcWidth;
		Ar->Seek( DstOffset + Y*BytesPerLine );
		for ( INT X=0; X < NumColumns; ++X )
		{
			Ar->Serialize( &Src[X].B, 1 );
			Ar->Serialize( &Src[X].G, 1 );
			Ar->Serialize( &Src[X].R, 1 );
		}
		if ( Padding )
		{
			Ar->Serialize( PadBytes, Padding );
		}
	}
}

/**
 * Closes the BMP file.
 *
 * This function is automatically called by the destructor.
 */
void FBitmapFile::Close()
{
	if ( Ar )
		delete Ar;
	appMemzero( this, sizeof(FBitmapFile) );
}

/**
* Reads the viewport's displayed pixels into the given color buffer.
* @param OutputBuffer - RGBA8 values will be stored in this buffer
* @param CubeFace - optional cube face for when reading from a cube render target
* @return True if the read succeeded.
*/
UBOOL FRenderTarget::ReadPixels(TArray<FColor>& OutputBuffer,ECubeFace CubeFace)
{
	// Read the render target surface data back.	
	struct FReadSurfaceContext
	{
		FRenderTarget* SrcRenderTarget;
		TArray<BYTE>* OutData;
		UINT MinX;
		UINT MinY;
		UINT MaxX;
		UINT MaxY;
		ECubeFace CubeFace;
	};
	TArray<BYTE> SurfaceData;
	FReadSurfaceContext ReadSurfaceContext =
	{
		this,
		&SurfaceData,
		0, 0,
		GetSizeX() - 1, GetSizeY() - 1,
		CubeFace	
	};

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		ReadSurfaceCommand,
		FReadSurfaceContext,Context,ReadSurfaceContext,
	{
		RHIReadSurfaceData(
			Context.SrcRenderTarget->RenderTargetSurfaceRHI,
			Context.MinX,
			Context.MinY,
			Context.MaxX,
			Context.MaxY,
			*Context.OutData,
			Context.CubeFace
			);
	});
	FlushRenderingCommands();

	// Copy the surface data into the output array.
	OutputBuffer.Empty(GetSizeX() * GetSizeY());
	for(UINT Y = 0;Y < GetSizeY();Y++)
	{
		FColor* SourceData = ((FColor*)&SurfaceData(0)) + Y * GetSizeX();
		for(UINT X = 0;X < GetSizeX();X++)
		{
			OutputBuffer.AddItem(SourceData[X]);
		}
	}

	return TRUE;
}

/** 
* @return display gamma expected for rendering to this render target 
*/
FLOAT FRenderTarget::GetDisplayGamma() const
{
	check(GEngine && GEngine->Client && Abs(GEngine->Client->DisplayGamma) > 0.0f);
	return GEngine->Client->DisplayGamma; 
}

/**
* Accessor for the surface RHI when setting this render target
* @return render target surface RHI resource
*/
const FSurfaceRHIRef& FRenderTarget::GetRenderTargetSurface() const
{
	return RenderTargetSurfaceRHI;
}

/*=============================================================================
//
// FViewport implementation.
//
=============================================================================*/

FViewport::FViewport(FViewportClient* InViewportClient):
	ViewportClient(InViewportClient),
	SizeX(0),
	SizeY(0),
	bIsFullscreen(FALSE),
	bHitProxiesCached(FALSE),
	bHasRequestedToggleFreeze(FALSE)
{
	//initialize the hit proxy kernel
	HitProxySize = 5;
	if (GIsEditor) 
	{
		GConfig->GetInt( TEXT("UnrealEd.HitProxy"), TEXT("HitProxySize"), (INT&)HitProxySize, GEditorIni );
		Clamp( HitProxySize, (UINT)1, (UINT)MAX_HITPROXYSIZE );
	}

	// Cache the viewport client's hit proxy storage requirement.
	bRequiresHitProxyStorage = ViewportClient->RequiresHitProxyStorage();
}

extern UBOOL	GIsTiledScreenshot;
extern INT		GScreenshotTile;
extern INT		GScreenshotMargin;
extern INT		GScreenshotResolutionMultiplier;
extern FIntRect	GScreenshotRect;

/**
 * Take a tiled, high-resolution screenshot and save to disk.
 *
 * @ResolutionMultiplier Increase resolution in each dimension by this multiplier.
 */
void FViewport::TiledScreenshot( INT ResolutionMultiplier )
{
	FViewportClient* ViewportClient = GetClient();
	GScreenshotResolutionMultiplier = ResolutionMultiplier;

	// Calculate number of overlapping tiles:
	INT TileWidth	= (INT) GetSizeX();
	INT TileHeight	= (INT) GetSizeY();
	INT InnerWidth	= TileWidth - 2*GScreenshotMargin;
	INT InnerHeight	= TileHeight - 2*GScreenshotMargin;
	INT TotalWidth	= GScreenshotResolutionMultiplier * TileWidth;
	INT TotalHeight	= GScreenshotResolutionMultiplier * TileHeight;
	INT NumColumns	= appCeil( FLOAT(TotalWidth) / FLOAT(InnerWidth) );
	INT NumRows		= appCeil( FLOAT(TotalHeight) / FLOAT(InnerHeight) );
	INT NumTiles	= NumColumns * NumRows;
	UBOOL IsOk		= TRUE;

	// Create screenshot folder if not already present.
	GFileManager->MakeDirectory( *GSys->ScreenShotPath, TRUE );

	TCHAR Filename[MAX_SPRINTF]=TEXT("");
	FBitmapFile BitmapFile;
	appSprintf( Filename, TEXT("%s\\Highres_Screenshot_"), *GSys->ScreenShotPath );

	for ( GScreenshotTile = 0; IsOk && GScreenshotTile < NumTiles; ++GScreenshotTile )
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			BeginDrawingCommand,
			FViewport*,Viewport,this,
		{
			RHIBeginDrawingViewport(Viewport->ViewportRHI);
			Viewport->RenderTargetSurfaceRHI = RHIGetViewportBackBuffer(Viewport->ViewportRHI);
		});

		FCanvas Canvas(this,NULL);
		{
			ViewportClient->Draw(this,&Canvas);
		}
		Canvas.Flush();

		// Read the contents of the viewport into an temp array.
		TArray<FColor> TempBitmap;
		IsOk = ReadPixels(TempBitmap);

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			EndDrawingCommand,
			FViewport*,Viewport,this,
		{
			RHIEndDrawingViewport(Viewport->ViewportRHI,TRUE);
		});

		// If everything went fine, save the rendered tile.
		if ( IsOk )
		{
			// Copy tile into final bitmap file:
			check(TempBitmap.Num() == TileWidth * TileHeight);
			INT ShotWidth	= GScreenshotRect.Width();
			INT ShotHeight	= GScreenshotRect.Height();
			GScreenshotRect.Min += FIntPoint(GScreenshotMargin,GScreenshotMargin);
			GScreenshotRect.Max -= FIntPoint(GScreenshotMargin,GScreenshotMargin);
			InnerWidth		= GScreenshotRect.Width();
			InnerHeight		= GScreenshotRect.Height();
			INT TileRow		= GScreenshotTile / NumColumns;
			INT TileColumn	= GScreenshotTile % NumColumns;
			INT DstX		= TileColumn * InnerWidth;
			INT DstY		= TileRow * InnerHeight;

			if ( GScreenshotTile == 0 )
			{
				TotalWidth		= GScreenshotResolutionMultiplier * ShotWidth;
				TotalHeight		= GScreenshotResolutionMultiplier * ShotHeight;
				if ( !BitmapFile.Create( Filename, TotalWidth, TotalHeight ) )
				{
					GIsTiledScreenshot = 0;
					return;
				}
			}
//			FIntRect SrcRect( GScreenshotMargin, GScreenshotMargin, GScreenshotMargin + InnerWidth, GScreenshotMargin + InnerHeight );
//			BitmapFile.CopyRect( &TempBitmap(0), TileWidth, TileHeight, DstX, DstY, &SrcRect );
			BitmapFile.CopyRect( &TempBitmap(0), TileWidth, TileHeight, DstX, DstY, &GScreenshotRect );
		}
	}

	BitmapFile.Close();
	GIsTiledScreenshot = 0;
}

void FViewport::Draw()
{
	// if this is a game viewport, and game rendering is disabled, then we don't want to actually draw anything
	if (GIsGame && !bIsGameRenderingEnabled)
	{
		// since we aren't drawing the viewport, we still need to update streaming, which needs valid view info
		FSceneViewFamilyContext ViewFamily(this, GWorld->Scene, SHOW_DefaultGame, GWorld->GetTimeSeconds(),GWorld->GetRealTimeSeconds(),NULL);
		for(INT PlayerIndex = 0;PlayerIndex < GEngine->GamePlayers.Num();PlayerIndex++)
		{
			ULocalPlayer* Player = GEngine->GamePlayers(PlayerIndex);
			if(Player->Actor)
			{
				// Calculate the player's view information.
				FVector		ViewLocation;
				FRotator	ViewRotation;
				FSceneView* View = Player->CalcSceneView( &ViewFamily, ViewLocation, ViewRotation, this);

				// if we have a valid view, use it for resource streaming
				if(View)
				{
					// Add view information for resource streaming.
					GStreamingManager->AddViewInformation( View->ViewOrigin, View->SizeX, View->SizeX * View->ProjectionMatrix.M[0][0] );
				}
			}
		}

		// Update level streaming.
		GWorld->UpdateLevelStreaming( &ViewFamily );
		return;
	}
	else
	{
		// Tiled rendering for high-res screenshots.
		if ( GIsTiledScreenshot && IsValidRef(ViewportRHI) )
		{
			TiledScreenshot( GScreenshotResolutionMultiplier );
		}

		if(IsValidRef(ViewportRHI))
		{
			UBOOL bLockToVsync = FALSE;
			if ( GEngine->GamePlayers.Num() )
			{
				ULocalPlayer* Player = GEngine->GamePlayers(0);
				bLockToVsync = (Player && Player->Actor && Player->Actor->bCinematicMode);
			}

			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
				BeginDrawingCommand,
				FViewport*,Viewport,this,
			{
				RHIBeginDrawingViewport(Viewport->ViewportRHI);
				Viewport->RenderTargetSurfaceRHI = RHIGetViewportBackBuffer(Viewport->ViewportRHI);
			});

			FCanvas Canvas(this,NULL);
			{
				ViewportClient->Draw(this,&Canvas);
			}
			Canvas.Flush();

			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				EndDrawingCommand,
				FViewport*,Viewport,this,
				UBOOL,bLockToVsync,bLockToVsync,
			{
				RHIEndDrawingViewport(Viewport->ViewportRHI,TRUE,bLockToVsync);
			});
		}
	}
}


void FViewport::Invalidate()
{
	bHitProxiesCached = FALSE;
	HitProxyMap.Invalidate();
	InvalidateDisplay();
}

void FViewport::GetHitProxyMap(UINT MinX,UINT MinY,UINT MaxX,UINT MaxY,TArray<HHitProxy*>& OutMap)
{
	// If the hit proxy map isn't up to date, render the viewport client's hit proxies to it.
	if(!bHitProxiesCached)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			BeginDrawingCommand,
			FViewport*,Viewport,this,
		{
			RHIBeginDrawingViewport(Viewport->ViewportRHI);

			// Set the hit proxy map's render target.
			RHISetRenderTarget(NULL,Viewport->HitProxyMap.GetRenderTargetSurface(), FSurfaceRHIRef());

			// Clear the hit proxy map to white, which is overloaded to mean no hit proxy.
			RHIClear(NULL,TRUE,FLinearColor::White,FALSE,0,FALSE,0);
		});

		// Let the viewport client draw its hit proxies.
		FCanvas Canvas(&HitProxyMap,&HitProxyMap);
		{
			ViewportClient->Draw(this,&Canvas);
		}
		Canvas.Flush();


		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			EndDrawingCommand,
			FViewport*,Viewport,this,
		{
			RHIEndDrawingViewport(Viewport->ViewportRHI,FALSE);
		});

		// Cache the hit proxies for the next GetHitProxyMap call.
		bHitProxiesCached = TRUE;
	}

	// Read the hit proxy map surface data back.
	struct FReadSurfaceContext
	{
		FViewport* Viewport;
		TArray<BYTE>* OutData;
		UINT MinX;
		UINT MinY;
		UINT MaxX;
		UINT MaxY;
	};
	TArray<BYTE> SurfaceData;
	FReadSurfaceContext ReadSurfaceContext =
	{
		this,
		&SurfaceData,
		MinX, MinY,
		MaxX, MaxY
	};
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		ReadSurfaceCommand,
		FReadSurfaceContext,Context,ReadSurfaceContext,
		{
			RHIReadSurfaceData(
				*const_cast<FSurfaceRHIRef*>(&Context.Viewport->HitProxyMap.GetRenderTargetSurface()),
				Context.MinX,
				Context.MinY,
				Context.MaxX,
				Context.MaxY,
				*Context.OutData
				);
		});
	FlushRenderingCommands();

	// Map the hit proxy map surface data to hit proxies.
	OutMap.Empty((MaxY - MinY + 1) * (MaxX - MinX + 1));
	for(UINT Y = MinY;Y <= MaxY;Y++)
	{
		FColor* SourceData = ((FColor*)&SurfaceData(0)) + (Y - MinY) * (MaxX - MinX + 1);
		for(UINT X = MinX;X <= MaxX;X++)
		{
			FHitProxyId HitProxyId(SourceData[X - MinX]);
			OutMap.AddItem(GetHitProxyById(HitProxyId));
		}
	}
}

HHitProxy* FViewport::GetHitProxy(INT X,INT Y)
{
	// Compute a HitProxySize x HitProxySize test region with the center at (X,Y).
	INT		MinX = X - HitProxySize,
			MinY = Y - HitProxySize,
			MaxX = X + HitProxySize,
			MaxY = Y + HitProxySize;
	
	// Clip the region to the viewport bounds.
	MinX = Max(MinX,0);
	MinY = Max(MinY,0);
	MaxX = Min(MaxX,appTrunc(GetSizeX()) - 1);
	MaxY = Min(MaxY,appTrunc(GetSizeY()) - 1);

	INT			TestSizeX	= MaxX - MinX + 1,
				TestSizeY	= MaxY - MinY + 1;
	HHitProxy*	HitProxy	= NULL;

	if(TestSizeX > 0 && TestSizeY > 0)
	{
		// Read the hit proxy map from the device.
		TArray<HHitProxy*>	ProxyMap;
		GetHitProxyMap((UINT)MinX,(UINT)MinY,(UINT)MaxX,(UINT)MaxY,ProxyMap);
		check(ProxyMap.Num() == TestSizeX * TestSizeY);

		// Find the hit proxy in the test region with the highest order.
		INT ProxyIndex = TestSizeY/2 * TestSizeX + TestSizeX/2;
		check(ProxyIndex<ProxyMap.Num());
		HitProxy = ProxyMap(ProxyIndex);
		
		for(INT TestY = 0;TestY < TestSizeY;TestY++)
		{
			for(INT TestX = 0;TestX < TestSizeX;TestX++)
			{
				HHitProxy* TestProxy = ProxyMap(TestY * TestSizeX + TestX);
				if(TestProxy && (!HitProxy || TestProxy->Priority > HitProxy->Priority))
				{
					HitProxy = TestProxy;
				}
			}
		}
	}

	return HitProxy;
}

void FViewport::UpdateViewportRHI(UBOOL bDestroyed,UINT NewSizeX,UINT NewSizeY,UBOOL bNewIsFullscreen)
{
	// Make sure we're not in the middle of streaming textures.
	(*GFlushStreamingFunc)();

	// Temporarily stop rendering thread.
	StopRenderingThread();

	// Update the viewport attributes.
	// This is done AFTER the command flush done by UpdateViewportRHI, to avoid disrupting rendering thread accesses to the old viewport size.
	SizeX = NewSizeX;
	SizeY = NewSizeY;
	bIsFullscreen = bNewIsFullscreen;

	// Release the viewport's resources.
	BeginReleaseResource(this);
	
	// Don't reinitialize the viewport RHI if the viewport has been destroyed.
	if(bDestroyed)
	{
		if(IsValidRef(ViewportRHI))
		{
			// If the viewport RHI has already been initialized, release it.
			ViewportRHI.Release();
		}
	}
	else
	{
		if(IsValidRef(ViewportRHI))
		{
			// If the viewport RHI has already been initialized, resize it.
			RHIResizeViewport(
				ViewportRHI,
				SizeX,
				SizeY,
				bIsFullscreen
				);
		}
		else
		{
			// Initialize the viewport RHI with the new viewport state.
			ViewportRHI = RHICreateViewport(
				GetWindow(),
				SizeX,
				SizeY,
				bIsFullscreen
				);
		}
	
		// Initialize the viewport's resources.
		BeginInitResource(this);
	}

	// Restart the rendering thread.
	if(GUseThreadedRendering)
	{
		StartRenderingThread();
	}

	if ( !bDestroyed && GCallbackEvent != NULL )
	{
		// send a notification that the viewport has been resized
		GCallbackEvent->Send(CALLBACK_ViewportResized, this, 0);
	}
}

/**
 * Calculates the view inside the viewport when the aspect ratio is locked.
 * Used for creating cinematic bars.
 * @param Aspect [in] ratio to lock to
 * @param CurrentX [in][out] coordinates of aspect locked view
 * @param CurrentY [in][out]
 * @param CurrentSizeX [in][out] size of aspect locked view
 * @param CurrentSizeY [in][out]
 */
void FViewport::CalculateViewExtents( FLOAT AspectRatio, INT& CurrentX, INT& CurrentY, UINT& CurrentSizeX, UINT& CurrentSizeY )
{
	// the viewport's SizeX/SizeY may not always match the GetDesiredAspectRatio(), so adjust the requested AspectRatio to compensate
	FLOAT AdjustedAspectRatio = AspectRatio / (GetDesiredAspectRatio() / ((FLOAT)GetSizeX() / (FLOAT)GetSizeY()));

	// If desired, enforce a particular aspect ratio for the render of the scene. 
	// Results in black bars at top/bottom etc.
	FLOAT AspectRatioDifference = AdjustedAspectRatio - ( ( FLOAT ) CurrentSizeX ) / ( ( FLOAT )CurrentSizeY );

	if( ::Abs( AspectRatioDifference ) > 0.01f )
	{
		// If desired aspect ratio is bigger than current - we need black bars on top and bottom.
		if( AspectRatioDifference > 0.0f )
		{
			// Calculate desired Y size.
			INT NewSizeY = appRound( ( ( FLOAT )CurrentSizeX ) / AdjustedAspectRatio );

//TTP: 49854
			//old code
			//CurrentY = appRound( 0.5f * ( ( FLOAT )( CurrentSizeY - NewSizeY ) ) );
			//need to add new resolution to the older one, because the older one may be recaulculated by ScreenPercentage setting
			CurrentY += appRound( 0.5f * ( ( FLOAT )( CurrentSizeY - NewSizeY ) ) );
//TTP: 49854
			CurrentSizeY = NewSizeY;
		}
		// Otherwise - will place bars on the sides.
		else
		{
			INT NewSizeX = appRound( ( ( FLOAT )CurrentSizeY ) * AdjustedAspectRatio );
//TTP: 49854
			//old code
			//CurrentX = appRound( 0.5f * ( ( FLOAT )( CurrentSizeX - NewSizeX ) ) );
			//need to add new resolution to the older one, because the older one may be recaulculated by ScreenPercentage setting
			CurrentX += appRound( 0.5f * ( ( FLOAT )( CurrentSizeX - NewSizeX ) ) );
//TTP: 49854
			CurrentSizeX = NewSizeX;
		}
	}
}

void FViewport::InitDynamicRHI()
{
	// Capture the viewport's back buffer surface for use through the FRenderTarget interface.
	RenderTargetSurfaceRHI = RHIGetViewportBackBuffer(ViewportRHI);

	if(bRequiresHitProxyStorage)
	{
		// Initialize the hit proxy map.
		HitProxyMap.Init(SizeX,SizeY);
	}
}

void FViewport::ReleaseDynamicRHI()
{
	HitProxyMap.Release();
	RenderTargetSurfaceRHI.Release();
}

UBOOL IsCtrlDown(FViewport* Viewport) { return (Viewport->KeyState(KEY_LeftControl) || Viewport->KeyState(KEY_RightControl)); }
UBOOL IsShiftDown(FViewport* Viewport) { return (Viewport->KeyState(KEY_LeftShift) || Viewport->KeyState(KEY_RightShift)); }
UBOOL IsAltDown(FViewport* Viewport) { return (Viewport->KeyState(KEY_LeftAlt) || Viewport->KeyState(KEY_RightAlt)); }

void FViewport::FHitProxyMap::Init(UINT NewSizeX,UINT NewSizeY)
{
	SizeX = NewSizeX;
	SizeY = NewSizeY;

	// Create a render target to store the hit proxy map.
	RenderTargetSurfaceRHI = RHICreateTargetableSurface(
		SizeX,SizeY,PF_A8R8G8B8,FTexture2DRHIRef(),TargetSurfCreate_Dedicated|TargetSurfCreate_Readable,TEXT("HitProxyColor"));
}

void FViewport::FHitProxyMap::Release()
{
	RenderTargetSurfaceRHI.Release();
}

void FViewport::FHitProxyMap::Invalidate()
{
	HitProxies.Empty();
}

void FViewport::FHitProxyMap::AddHitProxy(HHitProxy* HitProxy)
{
	HitProxies.AddItem(HitProxy);
}

/**
 * Globally enables/disables rendering
 *
 * @param bIsEnabled TRUE if drawing should occur
 */
void FViewport::SetGameRenderingEnabled(UBOOL bIsEnabled)
{
	bIsGameRenderingEnabled = bIsEnabled;
}

/**
 * Handles freezing/unfreezing of rendering
 */
void FViewport::ProcessToggleFreezeCommand()
{
	bHasRequestedToggleFreeze = 1;
}

/**
 * Returns if there is a command to toggle freezing
 */
UBOOL FViewport::HasToggleFreezeCommand()
{
	// save the current command
	UBOOL ReturnVal = bHasRequestedToggleFreeze;
	
	// make sure that we no longer have the command, as we are now passing off "ownership"
	// of the command
	bHasRequestedToggleFreeze = FALSE;

	// return what it was
	return ReturnVal;
}

//
// UClient implementation.
//

void UClient::StaticConstructor()
{
	new(GetClass(),TEXT("DisplayGamma"),	RF_Public)UFloatProperty(CPP_PROPERTY(DisplayGamma	), TEXT("Display"), CPF_Config );
	new(GetClass(),TEXT("MinDesiredFrameRate"),	RF_Public)UFloatProperty(CPP_PROPERTY(MinDesiredFrameRate	), TEXT("Display"), CPF_Config );
	new(GetClass(),TEXT("InitialButtonRepeatDelay"), RF_Public) UFloatProperty(CPP_PROPERTY(InitialButtonRepeatDelay), TEXT("Input"), CPF_Config);
	new(GetClass(),TEXT("ButtonRepeatDelay"), RF_Public) UFloatProperty(CPP_PROPERTY(ButtonRepeatDelay), TEXT("Input"), CPF_Config);

#if !CONSOLE
	new(GetClass(),TEXT("StartupFullscreen"),		RF_Public)UBoolProperty	(CPP_PROPERTY(StartupFullscreen			), TEXT("Display"), CPF_Config );
	new(GetClass(),TEXT("StartupResolutionX"),		RF_Public)UIntProperty	(CPP_PROPERTY(StartupResolutionX		), TEXT("Display"), CPF_Config );
	new(GetClass(),TEXT("StartupResolutionY"),		RF_Public)UIntProperty	(CPP_PROPERTY(StartupResolutionY		), TEXT("Display"), CPF_Config );
#endif
}

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UClient::InitializeIntrinsicPropertyValues()
{
	InitialButtonRepeatDelay = 0.2f;
	ButtonRepeatDelay = 0.1f;

#if CONSOLE
	StartupFullscreen	= TRUE;
	StartupResolutionX	= GScreenWidth;
	StartupResolutionY	= GScreenHeight;
#endif
}
/**
 * Exec handler used to parse console commands.
 *
 * @param	Cmd		Command to parse
 * @param	Ar		Output device to use in case the handler prints anything
 * @return	TRUE if command was handled, FALSE otherwise
 */
UBOOL UClient::Exec(const TCHAR* Cmd,FOutputDevice& Ar)
{
	if (ParseCommand(&Cmd, TEXT("GAMMA")))
	{
		DisplayGamma = (*Cmd != 0) ? Clamp<FLOAT>(appAtof(*ParseToken(Cmd, FALSE)), 0.5f, 5.0f) : GetClass()->GetDefaultObject<UClient>()->DisplayGamma;
		return TRUE;
	}
	else if( GetAudioDevice() && GetAudioDevice()->Exec(Cmd,Ar) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
