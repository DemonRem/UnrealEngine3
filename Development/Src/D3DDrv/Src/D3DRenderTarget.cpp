/*=============================================================================
	D3DRenderTarget.cpp: D3D render target implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3DDrvPrivate.h"
#include "BatchedElements.h"
#include "ScreenRendering.h"

#if USE_D3D_RHI

/** An option to emulate platforms which require resolving surfaces to a texture before sampling. */
#define REQUIRE_D3D_RESOLVE 0

/**
* Copies the contents of the given surface to its resolve target texture.
* @param SourceSurface - surface with a resolve texture to copy to
* @param bKeepOriginalSurface - TRUE if the original surface will still be used after this function so must remain valid
* @param ResolveParams - optional resolve params
*/
void RHICopyToResolveTarget(FSurfaceRHIParamRef SourceSurface, UBOOL bKeepOriginalSurface, const FResolveParams& ResolveParams)
{
	if( SourceSurface.Texture2D &&
		(REQUIRE_D3D_RESOLVE || 
		 SourceSurface.Texture2D != SourceSurface.ResolveTargetTexture2D ||
		 IsValidRef(SourceSurface.ResolveTargetTextureCube)) )
	{
		// surface can't be a part of both 2d/cube textures
		check(!(SourceSurface.Texture2D && SourceSurface.TextureCube));
		// surface can't have both 2d/cube resolve target textures
		check(!(IsValidRef(SourceSurface.ResolveTargetTexture2D) && IsValidRef(SourceSurface.ResolveTargetTextureCube)));

		// Get a handle for the destination surface.
		TD3DRef<IDirect3DSurface9> DestinationSurface;
		// resolving to 2d texture
		if( IsValidRef(SourceSurface.ResolveTargetTexture2D) )
		{
			// get level 0 surface from 2d texture
			VERIFYD3DRESULT(SourceSurface.ResolveTargetTexture2D->GetSurfaceLevel(0,DestinationSurface.GetInitReference()));
		}
		// resolving to cube texture
		else
		{
			// get cube face from cube texture
			VERIFYD3DRESULT(SourceSurface.ResolveTargetTextureCube->GetCubeMapSurface(
				GetD3DCubeFace(ResolveParams.CubeFace),0,DestinationSurface.GetInitReference()));
		}		

		// Determine the destination surface size.
		D3DSURFACE_DESC DestinationDesc;
		VERIFYD3DRESULT(DestinationSurface->GetDesc(&DestinationDesc));

		// Construct a temporary FTexture to represent the source surface.
		FTexture TempTexture;
		TempTexture.SamplerStateRHI = TStaticSamplerState<SF_Nearest,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		TempTexture.TextureRHI = (FTextureRHIRef&)SourceSurface.Texture2D;

		// Generate the vertices used to copy from the source surface to the destination surface.
		FLOAT MinX = -1.0f - GPixelCenterOffset / ((FLOAT)DestinationDesc.Width * 0.5f);
		FLOAT MaxX = +1.0f - GPixelCenterOffset / ((FLOAT)DestinationDesc.Width * 0.5f);
		FLOAT MinY = +1.0f + GPixelCenterOffset / ((FLOAT)DestinationDesc.Height * 0.5f);
		FLOAT MaxY = -1.0f + GPixelCenterOffset / ((FLOAT)DestinationDesc.Height * 0.5f);

		FBatchedElements BatchedElements;
		INT V00 = BatchedElements.AddVertex(FVector4(MinX,MinY,0,1),FVector2D(0,0),FColor(255,255,255),FHitProxyId());
		INT V10 = BatchedElements.AddVertex(FVector4(MaxX,MinY,0,1),FVector2D(1,0),FColor(255,255,255),FHitProxyId());
		INT V01 = BatchedElements.AddVertex(FVector4(MinX,MaxY,0,1),FVector2D(0,1),FColor(255,255,255),FHitProxyId());
		INT V11 = BatchedElements.AddVertex(FVector4(MaxX,MaxY,0,1),FVector2D(1,1),FColor(255,255,255),FHitProxyId());

		// Set the destination texture as the render target.
		GDirect3DDevice->SetRenderTarget(0,DestinationSurface);
		GDirect3DDevice->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);

		// No alpha blending, no depth tests or writes.
		RHISetBlendState(NULL,TStaticBlendState<>::GetRHI());
		RHISetDepthState(NULL,TStaticDepthState<FALSE,CF_Always>::GetRHI());

		// Draw a quad using the generated vertices.
		BatchedElements.AddTriangle(V00,V10,V11,&TempTexture,BLEND_Opaque);
		BatchedElements.AddTriangle(V00,V11,V01,&TempTexture,BLEND_Opaque);
		BatchedElements.Draw(NULL,FMatrix::Identity,DestinationDesc.Width,DestinationDesc.Height,FALSE);
	}
}

void RHICopyFromResolveTargetFast(FSurfaceRHIParamRef DestSurface)
{
	// these need to be referenced in order for the FScreenVertexShader/FScreenPixelShader types to not be compiled out on PC
	TShaderMapRef<FScreenVertexShader> ScreenVertexShader(GetGlobalShaderMap());
	TShaderMapRef<FScreenPixelShader> ScreenPixelShader(GetGlobalShaderMap());
}

void RHICopyFromResolveTarget(FSurfaceRHIParamRef DestSurface)
{
}

/**
* Creates a RHI surface that can be bound as a render target.
* Note that a surface cannot be created which is both resolvable AND readable.
* @param SizeX - The width of the surface to create.
* @param SizeY - The height of the surface to create.
* @param Format - The surface format to create.
* @param ResolveTargetTexture - The 2d texture which the surface will be resolved to.  It must have been allocated with bResolveTargetable=TRUE
* @param Flags - Surface creation flags
* @param UsageStr - Text describing usage for this surface
* @return The surface that was created.
*/
FSurfaceRHIRef RHICreateTargetableSurface(
	UINT SizeX,
	UINT SizeY,
	BYTE Format,
	FTexture2DRHIParamRef ResolveTargetTexture,
	DWORD Flags,
	const TCHAR* UsageStr
	)
{
	UBOOL bDepthFormat = (Format == PF_DepthStencil || Format == PF_ShadowDepth|| Format == PF_FilteredShadowDepth || Format == PF_D24);

	if(IsValidRef(ResolveTargetTexture))
	{
		checkMsg(!(Flags&TargetSurfCreate_Readable),TEXT("Cannot allocate resolvable surfaces with the readable flag."));

		if((Flags&TargetSurfCreate_Dedicated) || REQUIRE_D3D_RESOLVE)
		{
			// Create a targetable texture.
			TD3DRef<IDirect3DTexture9> TargetableTexture;
			VERIFYD3DRESULT(GDirect3DDevice->CreateTexture(
				SizeX,
				SizeY,
				1,
				bDepthFormat ? D3DUSAGE_DEPTHSTENCIL : D3DUSAGE_RENDERTARGET,
				(D3DFORMAT)GPixelFormats[Format].PlatformFormat,
				D3DPOOL_DEFAULT,
				TargetableTexture.GetInitReference(),
				NULL
				));

			// Retrieve the texture's surface.
			TD3DRef<IDirect3DSurface9> TargetableSurface;
			VERIFYD3DRESULT(TargetableTexture->GetSurfaceLevel(0,TargetableSurface.GetInitReference()));

			return FSurfaceRHIRef(ResolveTargetTexture,TargetableTexture,TargetableSurface);
		}
		else
		{
			// Simply return resolve target texture's surface.
			TD3DRef<IDirect3DSurface9> TargetableSurface;
			VERIFYD3DRESULT(ResolveTargetTexture->GetSurfaceLevel(0,TargetableSurface.GetInitReference()));

			return FSurfaceRHIRef(ResolveTargetTexture,ResolveTargetTexture,TargetableSurface);
		}
	}
	else
	{
		checkMsg((Flags&TargetSurfCreate_Dedicated),TEXT("Cannot allocated non-dedicated unresolvable surfaces."));

		if(bDepthFormat)
		{
			// Create a depth-stencil target surface.
			TD3DRef<IDirect3DSurface9> Surface;
			VERIFYD3DRESULT(GDirect3DDevice->CreateDepthStencilSurface(
				SizeX,
				SizeY,
				(D3DFORMAT)GPixelFormats[Format].PlatformFormat,
				D3DMULTISAMPLE_NONE,
				0,
				TRUE,
				Surface.GetInitReference(),
				NULL
				));
			return FSurfaceRHIRef(FTexture2DRHIRef(),NULL,Surface);
		}
		else
		{
			checkMsg((Flags&TargetSurfCreate_Readable),TEXT("Surface created which isn't readable or resolvable.  Is that intentional?"));

			// Create a render target surface.
			TD3DRef<IDirect3DSurface9> Surface;
			VERIFYD3DRESULT(GDirect3DDevice->CreateRenderTarget(
				SizeX,
				SizeY,
				(D3DFORMAT)GPixelFormats[Format].PlatformFormat,
				D3DMULTISAMPLE_NONE,
				0,
				(Flags&TargetSurfCreate_Readable),
				Surface.GetInitReference(),
				NULL
				));
			return FSurfaceRHIRef(FTexture2DRHIRef(),NULL,Surface);
		}
	}
}

/**
* Creates a RHI surface that can be bound as a render target and can resolve w/ a cube texture
* Note that a surface cannot be created which is both resolvable AND readable.
* @param SizeX - The width of the surface to create.
* @param Format - The surface format to create.
* @param ResolveTargetTexture - The cube texture which the surface will be resolved to.  It must have been allocated with bResolveTargetable=TRUE
* @param CubeFace - face from resolve texture to use as surface
* @param Flags - Surface creation flags
* @param UsageStr - Text describing usage for this surface
* @return The surface that was created.
*/
FSurfaceRHIRef RHICreateTargetableCubeSurface(
	UINT SizeX,
	BYTE Format,
	FTextureCubeRHIRef ResolveTargetTexture,
	ECubeFace CubeFace,
	DWORD Flags,
	const TCHAR* UsageStr
	)
{
	check(Format != PF_DepthStencil);
	if(!IsValidRef(ResolveTargetTexture))
	{
		checkMsg(FALSE,TEXT("No resolve target cube texture specified.  Just use RHICreateTargetableSurface instead."));
	}
	else
	{
		checkMsg(!(Flags&TargetSurfCreate_Readable),TEXT("Cannot allocate resolvable surfaces with the readable flag."));

		// create a dedicated texture which contains the target surface
		if((Flags&TargetSurfCreate_Dedicated) || REQUIRE_D3D_RESOLVE)
		{
			// Create a targetable texture.
			TD3DRef<IDirect3DTexture9> TargetableTexture;
			VERIFYD3DRESULT(GDirect3DDevice->CreateTexture(
				SizeX,
				SizeX,
				1,
				(Format == PF_ShadowDepth) ? D3DUSAGE_DEPTHSTENCIL : D3DUSAGE_RENDERTARGET,
				(D3DFORMAT)GPixelFormats[Format].PlatformFormat,
				D3DPOOL_DEFAULT,
				TargetableTexture.GetInitReference(),
				NULL
				));

			// Retrieve the texture's surface.
			TD3DRef<IDirect3DSurface9> TargetableSurface;
			VERIFYD3DRESULT(TargetableTexture->GetSurfaceLevel(0,TargetableSurface.GetInitReference()));

			// use a dedicated texture for the target and the cube texture for resolves
			return FSurfaceRHIRef(ResolveTargetTexture,TargetableTexture,TargetableSurface);
		}
		// use a surface from the resolve texture
		else
		{
			// Simply return resolve target texture's surface corresponding to the given CubeFace.
			TD3DRef<IDirect3DSurface9> TargetableSurface;
			VERIFYD3DRESULT(ResolveTargetTexture->GetCubeMapSurface(GetD3DCubeFace(CubeFace),0,TargetableSurface.GetInitReference()));

			// use the same cube texture as the resolve and target textures 
			return FSurfaceRHIRef(ResolveTargetTexture,ResolveTargetTexture,TargetableSurface);
		}
	}

	return FSurfaceRHIRef();
}

UBOOL RHIIsSurfaceAllocated(FSurfaceRHIParamRef Surface)
{
	return TRUE;
}

void RHIDiscardSurface(FSurfaceRHIParamRef Surface)
{
}


/**
* Helper for storing IEEE 32 bit float components
*/
struct FFloatIEEE
{
	union
	{
		struct
		{
			DWORD	Mantissa : 23,
					Exponent : 8,
					Sign : 1;
		} Components;

		FLOAT	Float;
	};
};

/**
* Helper for storing 16 bit float components
*/
struct FD3DFloat16
{
	union
	{
		struct
		{
			WORD	Mantissa : 10,
					Exponent : 5,
					Sign : 1;
		} Components;

		WORD	Encoded;
	};

	/**
	* @return full 32 bit float from the 16 bit value
	*/
	operator FLOAT()
	{
		FFloatIEEE	Result;

		Result.Components.Sign = Components.Sign;
		Result.Components.Exponent = Components.Exponent - 15 + 127; // Stored exponents are biased by half their range.
		Result.Components.Mantissa = Min<DWORD>(appFloor((FLOAT)Components.Mantissa / 1024.0f * 8388608.0f),(1 << 23) - 1);

		return Result.Float;
	}
};

void RHIReadSurfaceData(FSurfaceRHIRef &Surface,UINT MinX,UINT MinY,UINT MaxX,UINT MaxY,TArray<BYTE>& OutData,ECubeFace CubeFace)
{
	UINT SizeX = MaxX - MinX + 1;
	UINT SizeY = MaxY - MinY + 1;

	// Check the format of the surface.
	D3DSURFACE_DESC SurfaceDesc;
	VERIFYD3DRESULT(Surface->GetDesc(&SurfaceDesc));

	check( SurfaceDesc.Format == D3DFMT_A8R8G8B8 || SurfaceDesc.Format == D3DFMT_A16B16G16R16F );

	// Allocate the output buffer.
	OutData.Empty(SizeX * SizeY * sizeof(FColor));

	// Read back the surface data from (MinX,MinY) to (MaxX,MaxY)
	D3DLOCKED_RECT	LockedRect;
	RECT			Rect;
	Rect.left	= MinX;
	Rect.top	= MinY;
	Rect.right	= MaxX + 1;
	Rect.bottom	= MaxY + 1;

	TD3DRef<IDirect3DSurface9> DestSurface = Surface;
	if( IsValidRef(Surface.ResolveTargetTexture2D) ||
		IsValidRef(Surface.ResolveTargetTextureCube) )
	{
		// create a temp 2d texture to copy render target to
		TD3DRef<IDirect3DTexture9> Texture2D;
		VERIFYD3DRESULT(GDirect3DDevice->CreateTexture(
			SizeX,
			SizeY,
			1,
			0,
			SurfaceDesc.Format,
			D3DPOOL_SYSTEMMEM,
			Texture2D.GetInitReference(),
			NULL
			));
		// get its surface 
		DestSurface = NULL;
		VERIFYD3DRESULT(Texture2D->GetSurfaceLevel(0,DestSurface.GetInitReference()));
		// get the render target surface
		TD3DRef<IDirect3DSurface9> SrcSurface;
		if( IsValidRef(Surface.ResolveTargetTextureCube) )
		{
			VERIFYD3DRESULT(Surface.ResolveTargetTextureCube->GetCubeMapSurface(GetD3DCubeFace(CubeFace),0,SrcSurface.GetInitReference()));
		}
		else
		{
			VERIFYD3DRESULT(Surface.ResolveTargetTexture2D->GetSurfaceLevel(0,SrcSurface.GetInitReference()));
		}		
        // copy render target data to memory
		VERIFYD3DRESULT(GDirect3DDevice->GetRenderTargetData(SrcSurface,DestSurface));
	}
	
	VERIFYD3DRESULT(DestSurface->LockRect(&LockedRect,&Rect,D3DLOCK_READONLY));

	if( SurfaceDesc.Format == D3DFMT_A8R8G8B8 )
	{
		for(UINT Y = MinY;Y <= MaxY;Y++)
		{
			BYTE* SrcPtr = (BYTE*)LockedRect.pBits + (Y - MinY) * LockedRect.Pitch;
			BYTE* DestPtr = (BYTE*)&OutData(OutData.Add(SizeX * sizeof(FColor)));
			appMemcpy(DestPtr,SrcPtr,SizeX * sizeof(FColor));
		}
	}
	else if ( SurfaceDesc.Format == D3DFMT_A16B16G16R16F )
	{
		FPlane	MinValue(0.0f,0.0f,0.0f,0.0f),
				MaxValue(1.0f,1.0f,1.0f,1.0f);

		check(sizeof(FD3DFloat16)==sizeof(WORD));

		for( UINT Y = MinY; Y <= MaxY; Y++ )
		{
			FD3DFloat16*	SrcPtr = (FD3DFloat16*)((BYTE*)LockedRect.pBits + (Y - MinY) * LockedRect.Pitch);

			for( UINT X = MinX; X <= MaxX; X++ )
			{
				MinValue.X = Min<FLOAT>(SrcPtr[0],MinValue.X);
				MinValue.Y = Min<FLOAT>(SrcPtr[1],MinValue.Y);
				MinValue.Z = Min<FLOAT>(SrcPtr[2],MinValue.Z);
				MinValue.W = Min<FLOAT>(SrcPtr[3],MinValue.W);
				MaxValue.X = Max<FLOAT>(SrcPtr[0],MaxValue.X);
				MaxValue.Y = Max<FLOAT>(SrcPtr[1],MaxValue.Y);
				MaxValue.Z = Max<FLOAT>(SrcPtr[2],MaxValue.Z);
				MaxValue.W = Max<FLOAT>(SrcPtr[3],MaxValue.W);
				SrcPtr += 4;
			}
		}

		for( UINT Y = MinY; Y <= MaxY; Y++ )
		{
			FD3DFloat16* SrcPtr = (FD3DFloat16*)((BYTE*)LockedRect.pBits + (Y - MinY) * LockedRect.Pitch);
			FColor* DestPtr = (FColor*)(BYTE*)&OutData(OutData.Add(SizeX * sizeof(FColor)));

			for( UINT X = MinY; X <= MaxX; X++ )
			{
				FColor NormalizedColor(
					FLinearColor(
					SrcPtr[0] / (MaxValue.X - MinValue.X),
					SrcPtr[1] / (MaxValue.Y - MinValue.Y),
					SrcPtr[2] / (MaxValue.Z - MinValue.Z),
					SrcPtr[3] / (MaxValue.W - MinValue.W)
					));
				appMemcpy(DestPtr++,&NormalizedColor,sizeof(FColor));
				SrcPtr += 4;
			}
		}
	}	

	DestSurface->UnlockRect();
}

#endif
