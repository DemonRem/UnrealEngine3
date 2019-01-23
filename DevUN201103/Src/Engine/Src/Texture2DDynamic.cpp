/*=============================================================================
	Texture2DDynamic.cpp: Implementation of UTexture2DDynamic.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UTexture2DDynamic);

/*-----------------------------------------------------------------------------
	FTexture2DDynamicResource
-----------------------------------------------------------------------------*/

/** Initialization constructor. */
FTexture2DDynamicResource::FTexture2DDynamicResource(UTexture2DDynamic* InOwner)
:	Owner(InOwner)
{
}

/** Returns the width of the texture in pixels. */
UINT FTexture2DDynamicResource::GetSizeX() const
{
	return Owner->SizeX;
}

/** Returns the height of the texture in pixels. */
UINT FTexture2DDynamicResource::GetSizeY() const
{
	return Owner->SizeY;
}

/** Called when the resource is initialized. This is only called by the rendering thread. */
void FTexture2DDynamicResource::InitRHI()
{
	// Create the sampler state RHI resource.
	FSamplerStateInitializerRHI SamplerStateInitializer
	(
		GSystemSettings.TextureLODSettings.GetSamplerFilter( Owner ),
		AM_Wrap,
		AM_Wrap,
		AM_Wrap
	);
	SamplerStateRHI = RHICreateSamplerState( SamplerStateInitializer );

	DWORD Flags = 0;
	if ( Owner->bIsResolveTarget )
	{
		Flags |= TexCreate_ResolveTargetable;
		bIgnoreGammaConversions = TRUE;		// Note, we're ignoring Owner->SRGB (it should be FALSE).
	}
	else if ( Owner->SRGB && (GEngine == NULL || !GEngine->bEmulateMobileRendering))
	{
		Flags |= TexCreate_SRGB;
	}
	if ( Owner->bNoTiling )
	{
		Flags |= TexCreate_NoTiling;
	}
	Texture2DRHI = RHICreateTexture2D( Owner->SizeX, Owner->SizeY, Owner->Format, Owner->NumMips, Flags, NULL );
	TextureRHI = Texture2DRHI;
}

/** Called when the resource is released. This is only called by the rendering thread. */
void FTexture2DDynamicResource::ReleaseRHI()
{
	FTextureResource::ReleaseRHI();
	Texture2DRHI.SafeRelease();
}

/** Returns the Texture2DRHI, which can be used for locking/unlocking the mips. */
FTexture2DRHIRef FTexture2DDynamicResource::GetTexture2DRHI()
{
	return Texture2DRHI;
}


/*-----------------------------------------------------------------------------
	UTexture2DDynamic
-----------------------------------------------------------------------------*/

/**

 * Initializes the texture and creates the render resource.
 * It will create 1 miplevel with the format PF_A8R8G8B8.
 *
 * @param InSizeX	- Width of the texture, in texels
 * @param InSizeY	- Height of the texture, in texels
 * @param InFormat	- Format of the texture, defaults to PF_A8R8G8B8
 */
void UTexture2DDynamic::Init( INT InSizeX, INT InSizeY, BYTE InFormat/*=2*/, UBOOL InIsResolveTarget/*=FALSE*/ )
{
	SizeX = InSizeX;
	SizeY = InSizeY;
	Format = (EPixelFormat) InFormat;
	NumMips = 1;
	bIsResolveTarget = InIsResolveTarget;

	// Initialize the resource.
	UpdateResource();
}

FTextureResource* UTexture2DDynamic::CreateResource()
{
	return new FTexture2DDynamicResource(this);
}

FLOAT UTexture2DDynamic::GetSurfaceWidth() const
{
	return SizeX;
}

FLOAT UTexture2DDynamic::GetSurfaceHeight() const
{
	return SizeY;
}

void UTexture2DDynamic::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

/** Script-accessible function to create and initialize a new Texture2DDynamic with the requested settings */
void UTexture2DDynamic::execCreate(FFrame& Stack, RESULT_DECL)
{
	P_GET_INT(InSizeX);
	P_GET_INT(InSizeY);
	P_GET_BYTE_OPTX(InFormat, PF_A8R8G8B8);
	P_GET_UBOOL_OPTX(InIsResolveTarget, FALSE);
	P_FINISH;

	EPixelFormat DesiredFormat = EPixelFormat(InFormat);
	if (InSizeX > 0 && InSizeY > 0 )
	{
		UTexture2DDynamic* NewTexture = Cast<UTexture2DDynamic>(StaticConstructObject(GetClass(), GetTransientPackage(), NAME_None, RF_Transient));
		if (NewTexture != NULL)
		{
			// Disable compression
			NewTexture->CompressionNone			= TRUE;
			NewTexture->CompressionSettings		= TC_Default;
			NewTexture->MipGenSettings			= TMGS_NoMipmaps;
			NewTexture->CompressionNoAlpha		= TRUE;
			NewTexture->DeferCompression		= FALSE;
			if ( InIsResolveTarget )
			{
//				NewTexture->SRGB				= FALSE;
				NewTexture->bNoTiling			= FALSE;
			}
			else
			{
				// Untiled format
				NewTexture->bNoTiling			= TRUE;
			}

			NewTexture->Init(InSizeX, InSizeY, DesiredFormat, InIsResolveTarget);
		}
		*(UTexture2DDynamic**)Result = NewTexture;
	}
	else
	{
		debugf(NAME_Warning, TEXT("Invalid parameters specified for UTexture2DDynamic::Create()"));
		*(UTexture2DDynamic**)Result = NULL;
	}
}
