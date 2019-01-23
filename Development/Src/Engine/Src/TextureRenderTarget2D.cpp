/*=============================================================================
	TextureRenderTarget2D.cpp: UTextureRenderTarget2D implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	UTextureRenderTarget2D
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UTextureRenderTarget2D);

/**
 * Create a new 2D render target texture resource
 * @return newly created FTextureRenderTarget2DResource
 */
FTextureResource* UTextureRenderTarget2D::CreateResource()
{
	FTextureRenderTarget2DResource* Result = new FTextureRenderTarget2DResource(*this);
	return Result;
}

/**
 * Materials should treat a render target 2D texture like a regular 2D texture resource.
 * @return EMaterialValueType for this resource
 */
EMaterialValueType UTextureRenderTarget2D::GetMaterialType()
{
	return MCT_Texture2D;
}

/**
 * Serialize properties (used for backwards compatibility with main branch)
 */
void UTextureRenderTarget2D::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// if this UTextureRenderTarget2D was actually a UTextureRenderTarget from the main branch
	// we need to serialize some properties a little differently that aren't read in with the
	// default serialization (see [UTextureRenderTarget|FRenderTarget]::Serialize() in 
	// main branch for how it used to serialize them)
	if (Ar.Ver() < VER_RENDERING_REFACTOR)
	{
		Ar << SizeX << SizeY << Format;
		UINT TempNumMips;
		Ar << TempNumMips;
	}
}

/** 
 * Initialize the settings needed to create a render target texture and create its resource
 * @param	InSizeX - width of the texture
 * @param	InSizeY - height of the texture
 * @param	InFormat - format of the texture
 */
void UTextureRenderTarget2D::Init( UINT InSizeX, UINT InSizeY, EPixelFormat InFormat )
{
	check(InSizeX > 0 && InSizeY > 0);
	check(!(InSizeX % GPixelFormats[InFormat].BlockSizeX));
	check(!(InSizeY % GPixelFormats[InFormat].BlockSizeY));
	check(FTextureRenderTargetResource::IsSupportedFormat(InFormat));

	// set required size/format
	SizeX		= InSizeX;
	SizeY		= InSizeY;
	Format		= InFormat;

	// Recreate the texture's resource.
	UpdateResource();
}

/** script accessible function to create and initialize a new TextureRenderTarget2D with the requested settings */
void UTextureRenderTarget2D::execCreate(FFrame& Stack, RESULT_DECL)
{
	P_GET_INT(InSizeX);
	P_GET_INT(InSizeY);
	P_GET_BYTE_OPTX(InFormat, PF_A8R8G8B8);
	P_GET_STRUCT_OPTX(FLinearColor, InClearColor, FLinearColor(0.0, 0.0, 0.0, 0.0));
	P_FINISH;

	EPixelFormat DesiredFormat = EPixelFormat(InFormat);
	if (InSizeX > 0 && InSizeY > 0 && FTextureRenderTargetResource::IsSupportedFormat(DesiredFormat))
	{
		// make sure resolution is a power of 2
		InSizeX = 1 << appCeilLogTwo(InSizeX);
		InSizeY = 1 << appCeilLogTwo(InSizeY);
		UTextureRenderTarget2D* NewTexture = CastChecked<UTextureRenderTarget2D>(StaticConstructObject(UTextureRenderTarget2D::StaticClass(), GetTransientPackage(), NAME_None, RF_Transient));
		if (InClearColor != FLinearColor(0.0, 0.0, 0.0, 0.0))
		{
			NewTexture->ClearColor = InClearColor;
		}
		NewTexture->Init(InSizeX, InSizeY, DesiredFormat);
		*(UTextureRenderTarget2D**)Result = NewTexture;
	}
	else
	{
		debugf(NAME_Warning, TEXT("Invalid parameters specified for TextureRenderTarget2D::Create()"));
		*(UTextureRenderTarget2D**)Result = NULL;
	}
}

/** 
 * Called when any property in this object is modified in UnrealEd
 * @param	PropertyThatChanged - changed property
 */
void UTextureRenderTarget2D::PostEditChange(UProperty* PropertyThatChanged)
{
	const INT MaxSize=2048;
	SizeX = Clamp<INT>(SizeX - (SizeX % GPixelFormats[Format].BlockSizeX),1,MaxSize);
	SizeY = Clamp<INT>(SizeY - (SizeY % GPixelFormats[Format].BlockSizeY),1,MaxSize);

#if CONSOLE
	// clamp the render target size in order to avoid reallocating the scene render targets
	// (note, PEC shouldn't really be called on consoles)
	SizeX = Min<INT>(SizeX,GScreenWidth);
	SizeY = Min<INT>(SizeY,GScreenHeight);
#endif

    Super::PostEditChange(PropertyThatChanged);
}

/** 
* Called after the object has been loaded
*/
void UTextureRenderTarget2D::PostLoad()
{
	Super::PostLoad();

#if CONSOLE
	// clamp the render target size in order to avoid reallocating the scene render targets
	SizeX = Min<INT>(SizeX,GScreenWidth);
	SizeY = Min<INT>(SizeY,GScreenHeight);
#endif
}

/** 
 * Returns a one line description of an object for viewing in the thumbnail view of the generic browser
 */
FString UTextureRenderTarget2D::GetDesc()	
{
	// size and format string
	return FString::Printf( TEXT("Render to Texture %dx%d[%s]"), SizeX, SizeY, GPixelFormats[Format].Name );
}

/** 
 * Returns detailed info to populate listview columns
 */
FString UTextureRenderTarget2D::GetDetailedDescription( INT InIndex )
{
	FString Description = TEXT( "" );
	switch( InIndex )
	{
	case 0:
		Description = FString::Printf( TEXT( "%dx%d" ), SizeX, SizeY );
		break;
	case 1:
		Description = GPixelFormats[Format].Name;
		break;
	}
	return( Description );
}

/**
 * Utility for creating a new UTexture2D from a TextureRenderTarget2D
 * TextureRenderTarget2D must be square and a power of two size.
 * @param	ObjOuter - Outer to use when constructing the new Texture2D.
 * @param	NewTexName	-Name of new UTexture2D object.
 * @return	New UTexture2D object.
 */
UTexture2D* UTextureRenderTarget2D::ConstructTexture2D(UObject* ObjOuter, const FString& NewTexName, EObjectFlags InFlags)
{
	UTexture2D* Result = NULL;
#if !CONSOLE
	// Check render target size is valid and power of two.
	if( SizeX != 0 && !(SizeX & (SizeX-1)) &&
		SizeY != 0 && !(SizeY & (SizeY-1)) )
	{
		// The r2t resource will be needed to read its surface contents
		FRenderTarget* RenderTarget = GetRenderTargetResource();
		if( RenderTarget )
		{
			// create the 2d texture
			Result = CastChecked<UTexture2D>( 
				StaticConstructObject(UTexture2D::StaticClass(), ObjOuter, FName(*NewTexName), InFlags) );
			// init to the same size as the 2d texture
			Result->Init(SizeX,SizeY,PF_A8R8G8B8);

			// read the 2d surface 
			TArray<FColor> SurfData;
			RenderTarget->ReadPixels(SurfData);

			// copy the 2d surface data to the first mip of the static 2d texture
			FTexture2DMipMap& Mip = Result->Mips(0);
			DWORD* TextureData = (DWORD*)Mip.Data.Lock(LOCK_READ_WRITE);
			INT TextureDataSize = Mip.Data.GetBulkDataSize();
			check(TextureDataSize==SurfData.Num()*sizeof(FColor));
			appMemcpy(TextureData,&SurfData(0),TextureDataSize);
			Mip.Data.Unlock();

			// Set compression options.
			Result->CompressionSettings	= TC_Default;
			Result->CompressionNoMipmaps = TRUE;
			Result->CompressionNoAlpha	= TRUE;
			Result->DeferCompression	= FALSE;

			// This will trigger compressions.
			Result->PostEditChange(NULL);

		}
	}	
#endif
	return Result;
}

/*-----------------------------------------------------------------------------
	FTextureRenderTarget2DResource
-----------------------------------------------------------------------------*/

FTextureRenderTarget2DResource::FTextureRenderTarget2DResource(const class UTextureRenderTarget2D& InOwner)
	:	Owner(InOwner), ClearColor(InOwner.ClearColor)
{
}

/**
 * Initializes the RHI render target resources used by this resource.
 * Called when the resource is initialized, or when reseting all RHI resources.
 * This is only called by the rendering thread.
 */
void FTextureRenderTarget2DResource::InitDynamicRHI()
{
	if( Owner.SizeX > 0 && Owner.SizeY > 0 )
	{
		// Create the RHI texture. Only one mip is used and the texture is targetable for resolve.
		DWORD TexCreateFlags = 0;
		Texture2DRHI = RHICreateTexture2D(
			Owner.SizeX, 
			Owner.SizeY, 
			Owner.Format, 
			1,
			TexCreate_ResolveTargetable|TexCreateFlags | 
			(Owner.bRenderOnce ? TexCreate_WriteOnce : 0)
			);
		TextureRHI = (FTextureRHIRef&)Texture2DRHI;

		// Create the RHI target surface used for rendering to
		RenderTargetSurfaceRHI = RHICreateTargetableSurface(
			Owner.SizeX,
			Owner.SizeY,
			Owner.Format,
			Texture2DRHI,
			(Owner.bNeedsTwoCopies ? TargetSurfCreate_Dedicated : 0) |
			(Owner.bRenderOnce ? TexCreate_WriteOnce : 0),
			TEXT("AuxColor")
			);

		// make sure the texture target gets cleared when possible after init
		if(Owner.bUpdateImmediate)
		{
			UpdateResource();
		}
		else
		{
			AddToDeferredUpdateList(TRUE);
		}
	}

	// Create the sampler state RHI resource.
	FSamplerStateInitializerRHI SamplerStateInitializer =
	{
		Owner.Filter == TF_Linear ? SF_AnisotropicLinear : SF_Nearest,
		Owner.AddressX == TA_Wrap ? AM_Wrap : (Owner.AddressX == TA_Clamp ? AM_Clamp : AM_Mirror),
		Owner.AddressY == TA_Wrap ? AM_Wrap : (Owner.AddressY == TA_Clamp ? AM_Clamp : AM_Mirror),
		AM_Wrap
	};
	SamplerStateRHI = RHICreateSamplerState( SamplerStateInitializer );
}

/**
 * Release the RHI render target resources used by this resource.
 * Called when the resource is released, or when reseting all RHI resources.
 * This is only called by the rendering thread.
 */
void FTextureRenderTarget2DResource::ReleaseDynamicRHI()
{
	// release the FTexture RHI resources here as well
	ReleaseRHI();

	Texture2DRHI.Release();
	RenderTargetSurfaceRHI.Release();	

	// remove grom global list of deferred clears
	RemoveFromDeferredUpdateList();
}

/**
 * Clear contents of the render target. 
 */
void FTextureRenderTarget2DResource::UpdateResource()
{	
 	// clear the target surface to green
 	RHISetRenderTarget(RHIGetGlobalContext(),RenderTargetSurfaceRHI,FSurfaceRHIRef());
 	RHISetViewport(RHIGetGlobalContext(),0,0,0.0f,Owner.SizeX,Owner.SizeY,1.0f);
 	RHIClear(RHIGetGlobalContext(),TRUE,ClearColor,FALSE,0.f,FALSE,0);
 
 	// copy surface to the texture for use
 	RHICopyToResolveTarget(RenderTargetSurfaceRHI, TRUE);
}

/** 
 * @return width of target surface
 */
UINT FTextureRenderTarget2DResource::GetSizeX() const
{ 
	return Owner.SizeX; 
}

/** 
 * @return height of target surface
 */
UINT FTextureRenderTarget2DResource::GetSizeY() const
{ 
	return Owner.SizeY; 
}

