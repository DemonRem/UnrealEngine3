/*=============================================================================
	SceneFilterRendering.cpp: Filter rendering implementation.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"

/** Vertex declaration for the light function fullscreen 2D quad. */
TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

IMPLEMENT_SHADER_TYPE(,VisualizeTexturePixelShader,TEXT("FilterPixelShader"),TEXT("VisualizeTexturePS"),SF_Pixel,VER_VISUALIZETEXTURE,0); \

/** A macro to declaring a filter shader type for a specific number of samples. */
#define IMPLEMENT_FILTER_SHADER_TYPE(NumSamples) \
	IMPLEMENT_SHADER_TYPE(template<>,TFilterPixelShader<NumSamples>,TEXT("FilterPixelShader"),TEXT("Main"),SF_Pixel,VER_HALFRES_MOTIONBLURDOF5,0); \
	IMPLEMENT_SHADER_TYPE(template<>,TFilterPixelShaderDepthInAlpha<NumSamples>,TEXT("FilterPixelShader"),TEXT("MainDepthInAlpha"),SF_Pixel,VER_IMPROVED_MOTIONBLUR7,0); \
	IMPLEMENT_SHADER_TYPE(template<>,TFilterVertexShader<NumSamples>,TEXT("FilterVertexShader"),TEXT("Main"),SF_Vertex,VER_HALFSCENE_DEPTH_PARAM,0);

/*
 * The filter shader types for 1-MAX_FILTER_SAMPLES samples.
 */
IMPLEMENT_FILTER_SHADER_TYPE(1);
IMPLEMENT_FILTER_SHADER_TYPE(2);
IMPLEMENT_FILTER_SHADER_TYPE(3);
IMPLEMENT_FILTER_SHADER_TYPE(4);
IMPLEMENT_FILTER_SHADER_TYPE(5);
IMPLEMENT_FILTER_SHADER_TYPE(6);
IMPLEMENT_FILTER_SHADER_TYPE(7);
IMPLEMENT_FILTER_SHADER_TYPE(8);
IMPLEMENT_FILTER_SHADER_TYPE(9);
IMPLEMENT_FILTER_SHADER_TYPE(10);
IMPLEMENT_FILTER_SHADER_TYPE(11);
IMPLEMENT_FILTER_SHADER_TYPE(12);
IMPLEMENT_FILTER_SHADER_TYPE(13);
IMPLEMENT_FILTER_SHADER_TYPE(14);
IMPLEMENT_FILTER_SHADER_TYPE(15);
IMPLEMENT_FILTER_SHADER_TYPE(16);

/**
* Sets the filter shaders with the provided filter samples.
* @param SamplerStateRHI - The sampler state to use for the source texture.
* @param TextureRHI - The source texture.
* @param SampleOffsets - A pointer to an array of NumSamples UV offsets
* @param SampleOffsets - A pointer to an array of NumSamples 4-vector weights
* @param NumSamples - The number of samples used by the filter.
* @param View != 0 means we want depth in alpha
* @param SampleMaskMin default means no clamping, can be used to avoid leaking
* @param SampleMaskMax default means no clamping, can be used to avoid leaking
*/
void SetFilterShaders(
	FSamplerStateRHIParamRef SamplerStateRHI,
	FTextureRHIParamRef TextureRHI,
	FVector2D* SampleOffsets,
	FLinearColor* SampleWeights,
	UINT NumSamples,
	const FSceneView* View = 0, 
	FVector2D SampleMaskMin = FVector2D(-1.0f, -1.0f),
	FVector2D SampleMaskMax = FVector2D(2.0f, 2.0f)
	)
{
	UBOOL bDepthInAlpha = View != 0;

	// A macro to handle setting the filter shader for a specific number of samples.
#define SET_FILTER_SHADER_TYPE(NumSamples) \
	case NumSamples: \
	{ \
		TShaderMapRef<TFilterVertexShader<NumSamples> > VertexShader(GetGlobalShaderMap()); \
		VertexShader->SetParameters(SampleOffsets); \
		if(bDepthInAlpha)\
		{ \
			TShaderMapRef<TFilterPixelShaderDepthInAlpha<NumSamples> > PixelShader(GetGlobalShaderMap()); \
			PixelShader->SetParameters(View, SamplerStateRHI,TextureRHI,SampleWeights, SampleMaskMin, SampleMaskMax); \
			{ \
				static FGlobalBoundShaderState BoundShaderState; \
				SetGlobalBoundShaderState( BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, sizeof(FFilterVertex)); \
			} \
			RHIReduceTextureCachePenalty(PixelShader->GetPixelShader()); \
		} \
		else\
		{ \
			TShaderMapRef<TFilterPixelShader<NumSamples> > PixelShader(GetGlobalShaderMap()); \
			PixelShader->SetParameters(SamplerStateRHI,TextureRHI,SampleWeights, SampleMaskMin, SampleMaskMax); \
			{ \
				static FGlobalBoundShaderState BoundShaderState; \
				SetGlobalBoundShaderState( BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, sizeof(FFilterVertex)); \
			} \
			RHIReduceTextureCachePenalty(PixelShader->GetPixelShader()); \
		} \
		break; \
	};

	// Set the appropriate filter shader for the given number of samples.
	switch(NumSamples)
	{
	SET_FILTER_SHADER_TYPE(1);
	SET_FILTER_SHADER_TYPE(2);
	SET_FILTER_SHADER_TYPE(3);
	SET_FILTER_SHADER_TYPE(4);
	SET_FILTER_SHADER_TYPE(5);
	SET_FILTER_SHADER_TYPE(6);
	SET_FILTER_SHADER_TYPE(7);
	SET_FILTER_SHADER_TYPE(8);
	SET_FILTER_SHADER_TYPE(9);
	SET_FILTER_SHADER_TYPE(10);
	SET_FILTER_SHADER_TYPE(11);
	SET_FILTER_SHADER_TYPE(12);
	SET_FILTER_SHADER_TYPE(13);
	SET_FILTER_SHADER_TYPE(14);
	SET_FILTER_SHADER_TYPE(15);
	SET_FILTER_SHADER_TYPE(16);
	default:
		appErrorf(TEXT("Invalid number of samples: %u"),NumSamples);
	}

#undef SET_FILTER_SHADER_TYPE
}

/**
 * Draws a quad with the given vertex positions and UVs in denormalized pixel/texel coordinates.
 * The platform-dependent mapping from pixels to texels is done automatically.
 * Note that the positions are affected by the current viewport.
 * X, Y							Position in screen pixels of the top left corner of the quad
 * SizeX, SizeY					Size in screen pixels of the quad
 * U, V							Position in texels of the top left corner of the quad's UV's
 * SizeU, SizeV					Size in texels of the quad's UV's
 * TargetSizeX, TargetSizeY		Size in screen pixels of the target surface
 * TextureSizeX, TextureSizeY	Size in texels of the source texture
 */
void DrawDenormalizedQuad(
	FLOAT X,
	FLOAT Y,
	FLOAT SizeX,
	FLOAT SizeY,
	FLOAT U,
	FLOAT V,
	FLOAT SizeU,
	FLOAT SizeV,
	UINT TargetSizeX,
	UINT TargetSizeY,
	UINT TextureSizeX,
	UINT TextureSizeY,
	FLOAT ClipSpaceQuadZ
	)
{
	// Set up the vertices.
	FFilterVertex Vertices[4];

	Vertices[0].Position = FVector4(X,			Y,			ClipSpaceQuadZ,	1);
	Vertices[1].Position = FVector4(X + SizeX,	Y,			ClipSpaceQuadZ,	1);
	Vertices[2].Position = FVector4(X,			Y + SizeY,	ClipSpaceQuadZ,	1);
	Vertices[3].Position = FVector4(X + SizeX,	Y + SizeY,	ClipSpaceQuadZ,	1);

	Vertices[0].UV = FVector2D(U,			V);
	Vertices[1].UV = FVector2D(U + SizeU,	V);
	Vertices[2].UV = FVector2D(U,			V + SizeV);
	Vertices[3].UV = FVector2D(U + SizeU,	V + SizeV);

	for(INT VertexIndex = 0;VertexIndex < 4;VertexIndex++)
	{
		Vertices[VertexIndex].Position.X = -1.0f + 2.0f * (Vertices[VertexIndex].Position.X - GPixelCenterOffset) / (FLOAT)TargetSizeX;
#if WITH_ES2_RHI
		if( GUsingES2RHI )
		{
			// OpenGL has 0, 0 at bottom left, not top left
			Vertices[VertexIndex].Position.Y = -1.0f + 2.0f * (Vertices[VertexIndex].Position.Y - GPixelCenterOffset) / (FLOAT)TargetSizeY;
		}
		else
#endif
		{
			Vertices[VertexIndex].Position.Y = +1.0f - 2.0f * (Vertices[VertexIndex].Position.Y - GPixelCenterOffset) / (FLOAT)TargetSizeY;
		}

		Vertices[VertexIndex].UV.X = Vertices[VertexIndex].UV.X / (FLOAT)TextureSizeX;
		Vertices[VertexIndex].UV.Y = Vertices[VertexIndex].UV.Y / (FLOAT)TextureSizeY;
	}

	static WORD Indices[] =
	{
		0, 1, 3,
		0, 3, 2
	};

	RHIDrawIndexedPrimitiveUP(PT_TriangleList,0,4,2,Indices,sizeof(Indices[0]),Vertices,sizeof(Vertices[0]));
}

//
void DrawDownsampledTexture(
	const FSurfaceRHIRef &Dest,
	const FTexture2DRHIRef& Src,
	FIntPoint DstLeftTop,
	FIntRect SrcRect,
	FIntPoint DstTextureSize,
	FIntPoint SrcTextureSize,
	const FSceneView* View)
{
	const int DownsampleFactor = 2;

	FIntRect DstRect(DstLeftTop, DstLeftTop + FIntPoint(SrcRect.Width() / 2, SrcRect.Height() / 2));

	// No depth tests, no backface culling.
	RHISetDepthState(TStaticDepthState<FALSE,CF_Always>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	// Setup the downsamples.
	FVector2D DownsampleOffsets[MAX_FILTER_SAMPLES];
	FLinearColor DownsampleWeights[MAX_FILTER_SAMPLES];
#if NGP
	float InvSrcSizeX = 1.0f / (FLOAT)SrcTextureSize.X;
	float InvSrcSizeY = 1.0f / (FLOAT)SrcTextureSize.Y;
#else
	float InvSrcSizeX = 1.0f / (FLOAT)GSceneRenderTargets.GetBufferSizeX();
	float InvSrcSizeY = 1.0f / (FLOAT)GSceneRenderTargets.GetBufferSizeY();
#endif
	float OffsetX = - 0.5f * InvSrcSizeX;
	float OffsetY = - 0.5f * InvSrcSizeY;
	for(UINT SampleY = 0;SampleY < DownsampleFactor;SampleY++)
	{
		for(UINT SampleX = 0;SampleX < DownsampleFactor;SampleX++)
		{
			const INT SampleIndex = SampleY * DownsampleFactor + SampleX;
			DownsampleOffsets[SampleIndex] = FVector2D(
				(FLOAT)SampleX * InvSrcSizeX + OffsetX,
				(FLOAT)SampleY * InvSrcSizeY + OffsetY
				);
			DownsampleWeights[SampleIndex] = FLinearColor::White / Square(DownsampleFactor);
		}
	}

	// Downsample the scene color buffer.

	// Set the filter color surface as the render target
	RHISetRenderTarget(Dest, FSurfaceRHIRef());
	RHISetViewport(0, 0, 0.0f, DstTextureSize.X, DstTextureSize.Y, 1.0f);

//@TODO - We shouldn't have to clear since we fill the entire buffer anyway. :(
#if !PS3 && !NGP
	RHIClear(TRUE,FLinearColor(0,0,0,0),FALSE,0,FALSE,0);
#endif

	SetFilterShaders(
		TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
		Src,
		DownsampleOffsets,
		DownsampleWeights,
		DownsampleFactor * DownsampleFactor,
		View
		);
	DrawDenormalizedQuad(
		DstRect.Min.X, DstRect.Min.Y, DstRect.Width(), DstRect.Height(),
		SrcRect.Min.X, SrcRect.Min.X, SrcRect.Width(), SrcRect.Height(),
		DstTextureSize.X, DstTextureSize.Y,
		SrcTextureSize.X, SrcTextureSize.Y
		);
	// Resolve the filter color surface 
	RHICopyToResolveTarget(Dest, FALSE, FResolveParams());
}

/** Sets shader parameter values */
void VisualizeTexturePixelShader::SetParameters(FTextureRHIParamRef TextureRHI, FLOAT RGBMul, FLOAT AMul, FLOAT Add, FLOAT FracScale, UBOOL SaturateInsteadOfFrac)
{
	// alternates between 0 and 1 with a short pause
	const FLOAT FracTimeScale = 2.0f;
	FLOAT FracTime = GCurrentTime * FracTimeScale - floor(GCurrentTime * FracTimeScale);
	FLOAT BlinkState = (FracTime > 0.5f) ? 1.0f : 0.0f;

	FVector4 VisualizeParam[2];
	
	// w * almost_1 to avoid frac(1) => 0
	VisualizeParam[0] = FVector4(RGBMul, AMul, Add, FracScale * 0.9999f);
	VisualizeParam[1] = FVector4(BlinkState, SaturateInsteadOfFrac ? 1.0f : 0.0f, 0, 0);

	SetPixelShaderValues(GetPixelShader(), VisualizeParamParameter, VisualizeParam, 2);
	SetTextureParameter(
		GetPixelShader(),
		VisualizeTextureParameter,
		TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
		TextureRHI
		);
}


void VisualizeTexture(const FTexture2DRHIRef& Src, UBOOL RenderToBackbuffer, FIntPoint ViewExtent, FIntPoint RTExtent, FLOAT RGBMul, FLOAT AMul, FLOAT Add, FVector2D Tex00, FVector2D Tex11, FLOAT FracScale, UBOOL SaturateInsteadOfFrac)
{
	// No depth tests, no backface culling.
	RHISetDepthState(TStaticDepthState<FALSE,CF_Always>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	if(RenderToBackbuffer)
	{
		GSceneRenderTargets.BeginRenderingBackBuffer();
	}
	else
	{
		GSceneRenderTargets.BeginRenderingSceneColor();
	}

	TShaderMapRef<TFilterVertexShader<1> > VertexShader(GetGlobalShaderMap());
	TShaderMapRef<VisualizeTexturePixelShader> PixelShader(GetGlobalShaderMap());

	FVector2D SampleOffset(0, 0);
	VertexShader->SetParameters(&SampleOffset);

	PixelShader->SetParameters(Src, RGBMul, AMul, Add, FracScale, SaturateInsteadOfFrac);

	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, sizeof(FFilterVertex));

	DrawDenormalizedQuad(
		// XY
		0, 0,
		// SizeXY
		ViewExtent.X, ViewExtent.Y,
		// UV
		Tex00.X, Tex00.Y,
		// SizeUV
		Tex11.X - Tex00.X, Tex11.Y - Tex00.Y,
		// TargetSize
		RTExtent.X, RTExtent.Y,
		// TextureSize
		1, 1
		);

	GSceneRenderTargets.FinishRenderingSceneColor(TRUE);
}


/**
 * Evaluates a normal distribution PDF at given X.
 * This function misses the math for scaling the result (faster, not needed if the resulting values are renormalized).
 * @param X - The X to evaluate the PDF at.
 * @param Mean - The normal distribution's mean.
 * @param Variance - The normal distribution's variance.
 * @return The value of the normal distribution at X. (unscaled)
 */
static FLOAT NormalDistributionUnscaled(FLOAT X,FLOAT Mean,FLOAT Variance)
{
	return appExp(-Square(X - Mean) / (2.0f * Variance));
}

/**
 * @return NumSamples >0
 */
UINT Compute1DGaussianFilterKernel(FLOAT KernelRadius, FVector2D OutOffsetAndWeight[MAX_FILTER_SAMPLES])
{
	FLOAT ClampedKernelRadius = Clamp<FLOAT>(KernelRadius, DELTA, MAX_FILTER_SAMPLES - 1);
	INT IntegerKernelRadius = Min<INT>(appCeil(ClampedKernelRadius), MAX_FILTER_SAMPLES - 1);

	// smallest IntegerKernelRadius will be 1

	UINT NumSamples = 0;
	FLOAT WeightSum = 0.0f;
	for(INT SampleIndex = -IntegerKernelRadius; SampleIndex <= IntegerKernelRadius; SampleIndex += 2)
	{
		FLOAT Weight0 = NormalDistributionUnscaled(SampleIndex, 0, ClampedKernelRadius);
		FLOAT Weight1 = 0.0f;

		// Because we use bilinear filtering we only require half the sample count.
		// But we need to fix the last weight.
		// Example (radius one texel, c is center, a left, b right):
		//    a b c (a is left texel, b center and c right) becomes two lookups one with a*.. + b **, the other with
		//    c * .. but another texel to the right would accidentially leak into this computation.
		if(SampleIndex != IntegerKernelRadius)
		{
			Weight1 = NormalDistributionUnscaled(SampleIndex + 1, 0, ClampedKernelRadius);
		}

		FLOAT TotalWeight = Weight0 + Weight1;
		OutOffsetAndWeight[NumSamples].X = (SampleIndex + Weight1 / TotalWeight);
		OutOffsetAndWeight[NumSamples].Y = TotalWeight;
		WeightSum += TotalWeight;
		NumSamples++;
	}

	// Normalize blur weights.
	FLOAT InvWeightSum = 1.0f / WeightSum;
	for(UINT SampleIndex = 0;SampleIndex < NumSamples; ++SampleIndex)
	{
		OutOffsetAndWeight[SampleIndex].Y *= InvWeightSum;
	}

	return NumSamples;
}

/**
 * Blurs the subregion of the filter buffer along a single axis.
 * @param SizeX - The width of the subregion to blur.
 * @param SizeY - The height of the subregion to blur.
 */
static void ApplyGaussianBlurStep(
	UINT SizeX,
	UINT SizeY,
	FVector2D BlurOffsets[MAX_FILTER_SAMPLES],
	FLinearColor BlurWeights[MAX_FILTER_SAMPLES],
	UINT NumSamples,
	FSceneRenderTargetIndex SrcFilterColorIndex,
	FSceneRenderTargetIndex DstFilterColorIndex,
	FVector2D SampleMaskMin,
	FVector2D SampleMaskMax)
{
	GSceneRenderTargets.BeginRenderingFilter(DstFilterColorIndex);

	SetFilterShaders(
		TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
		GSceneRenderTargets.GetFilterColorTexture(SrcFilterColorIndex),
		BlurOffsets,
		BlurWeights,
		NumSamples,
		0,
		SampleMaskMin,
		SampleMaskMax
		);

	const UINT BufferSizeX = GSceneRenderTargets.GetFilterBufferSizeX();
	const UINT BufferSizeY = GSceneRenderTargets.GetFilterBufferSizeY();

	DrawDenormalizedQuad(
		1,1,
		SizeX,SizeY,
		1,1,
		SizeX,SizeY,
		BufferSizeX,BufferSizeY,
		BufferSizeX,BufferSizeY
		);
	GSceneRenderTargets.FinishRenderingFilter(DstFilterColorIndex);
}

/**
 * Uses a gaussian blur to filter a subregion of the filter buffer.
 * @param ViewSizeX - The view width in pixels to scale the radius.
 * @param SizeX - The width of the subregion to blur.
 * @param SizeY - The height of the subregion to blur.
 * @param KernelRadius - The radius of the gaussian kernel.
 */
void GaussianBlurFilterBuffer(
	FLOAT ViewSizeX,
	UINT SizeX,
	UINT SizeY,
	FLOAT KernelRadius,
	FLOAT KernelScale,
	FSceneRenderTargetIndex FilterColorIndex,
	FVector2D SampleMaskMin,
	FVector2D SampleMaskMax)
{
	const UINT FilterDownsampleFactor = GSceneRenderTargets.GetFilterDownsampleFactor();

	// Rescale the blur kernel to be resolution independent.
	// The effect blur scale is specified in pixel widths at a reference resolution of 1280x?.
	const FLOAT ResolutionFraction = ViewSizeX / 1280.0f;

	FLOAT EffectiveBlurRadius =  KernelRadius / FilterDownsampleFactor * ResolutionFraction;

	if(EffectiveBlurRadius <= 0.1f)
	{
		// optimization: no blur needed
		return;
	}

	const UINT BufferSizeX = GSceneRenderTargets.GetFilterBufferSizeX();
	const UINT BufferSizeY = GSceneRenderTargets.GetFilterBufferSizeY();

	FVector2D BlurOffsets[MAX_FILTER_SAMPLES];
	FLinearColor BlurWeights[MAX_FILTER_SAMPLES];
	UINT NumSamples1D;
	{
		FVector2D OffsetAndWeight[MAX_FILTER_SAMPLES];

		// compute 1D filtered samples
		NumSamples1D = Compute1DGaussianFilterKernel(EffectiveBlurRadius, OffsetAndWeight);

		// TRUE: single pass with n*n samples, FALSE:2 passes with 2*n samples
		// Experimentation on Xbox360 shows that 2x2 single pass is faster than two pass, 3x3 is a tiny bit slower and 4x4 is much slower
		// This should work well for a wide range of platforms but we can use other constants for different platforms.
		UBOOL SinglePass = NumSamples1D <= 2;

		// diagonal direction (combined horizontal and vertical direction in one variable)
		FVector2D DirectionXY(KernelScale / BufferSizeX, KernelScale / BufferSizeY);

		if(SinglePass)
		{
			UINT NumSamples2D = Square(NumSamples1D);

			// compute weights as weighted contributions of white
			// and compute 2d array of bilinear filtered samples
			for(UINT v = 0; v < NumSamples1D; ++v)
			{
				for(UINT u = 0; u < NumSamples1D; ++u)
				{
					UINT i = u + v * NumSamples1D;

					BlurWeights[i] = FLinearColor::White * (OffsetAndWeight[u].Y * OffsetAndWeight[v].Y);
					BlurOffsets[i] = FVector2D(DirectionXY.X * OffsetAndWeight[u].X, DirectionXY.Y * OffsetAndWeight[v].X);
				}
			}

			ApplyGaussianBlurStep(SizeX, SizeY, BlurOffsets, BlurWeights, NumSamples2D, FilterColorIndex, FilterColorIndex, SampleMaskMin, SampleMaskMax);
		}
		else
		{
			// compute weights as weighted contributions of white
			for(UINT i = 0; i < NumSamples1D; ++i)
			{
				BlurWeights[i] = FLinearColor::White * OffsetAndWeight[i].Y;
			}

			// make horizontal 1d array of bilinear filtered samples
			for(UINT i = 0; i < NumSamples1D; ++i)
			{
				BlurOffsets[i] = FVector2D(DirectionXY.X * OffsetAndWeight[i].X, 0.0f);
			}

			FSceneRenderTargetIndex TempFilterColorIndex = FilterColorIndex;
#if MOBILE
			if ( FilterColorIndex == SRTI_FilterColor0 )
			{
				TempFilterColorIndex = SRTI_FilterColor1;
			}
			else
			{
				TempFilterColorIndex = SRTI_FilterColor0;
			}
#endif
			ApplyGaussianBlurStep(SizeX, SizeY, BlurOffsets, BlurWeights, NumSamples1D, FilterColorIndex, TempFilterColorIndex, SampleMaskMin, SampleMaskMax);

			// make vertical 1d array of bilinear filtered samples
			for(UINT i = 0; i < NumSamples1D; ++i)
			{
				BlurOffsets[i] = FVector2D(0.0f, DirectionXY.Y * OffsetAndWeight[i].X);
			}

			ApplyGaussianBlurStep(SizeX, SizeY, BlurOffsets, BlurWeights, NumSamples1D, TempFilterColorIndex, FilterColorIndex, SampleMaskMin, SampleMaskMax);
		}
	}
}


// can be optimized by making the multi pass implementation a simple pass one
void CombineFilterBuffer(
	UINT SizeX,
	UINT SizeY,
	FSceneRenderTargetIndex FilterColorIndexFullRes, FLOAT FullResScale,
	FSceneRenderTargetIndex FilterColorIndexHalfRes, FLOAT HalfResScale,
	FSceneRenderTargetIndex FilterColorIndexQuarterRes, FLOAT QuarterResScale)
{
	const UINT BufferSizeX = GSceneRenderTargets.GetFilterBufferSizeX();
	const UINT BufferSizeY = GSceneRenderTargets.GetFilterBufferSizeY();

	GSceneRenderTargets.BeginRenderingFilter(FilterColorIndexFullRes);

	// setup sample with 0.5f as weight as we add two textures
	FVector2D BlurOffsets[1] = { FVector2D(0, 0) };
	UINT NumSamples = 1;

	// full resolution
	{
		FLinearColor BlurWeights[1] = { FLinearColor::White * FullResScale };

		SetFilterShaders(
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GSceneRenderTargets.GetFilterColorTexture(FilterColorIndexFullRes),
			BlurOffsets,
			BlurWeights,
			NumSamples);

		DrawDenormalizedQuad(
			1, 1,
			SizeX, SizeY,
			1, 1,
			SizeX, SizeY,
			BufferSizeX, BufferSizeY,
			BufferSizeX, BufferSizeY
			);
	}

	// additive blending
	RHISetBlendState(TStaticBlendState<BO_Add,BF_One,BF_One,BO_Add,BF_One,BF_One>::GetRHI());

	// half resolution
	if(FilterColorIndexHalfRes != SRTI_None)
	{
		FLinearColor BlurWeights[1] = { FLinearColor::White * HalfResScale };

		SetFilterShaders(
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GSceneRenderTargets.GetFilterColorTexture(FilterColorIndexHalfRes),
			BlurOffsets,
			BlurWeights,
			NumSamples);

		DrawDenormalizedQuad(
			1, 1,
			SizeX, SizeY,
			1, 1,
			SizeX / 2, SizeY / 2,
			BufferSizeX, BufferSizeY,
			BufferSizeX, BufferSizeY
			);

	}

	// quarter resolution
	if(FilterColorIndexQuarterRes != SRTI_None)
	{
		FLinearColor BlurWeights[1] = { FLinearColor::White * QuarterResScale };

		SetFilterShaders(
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GSceneRenderTargets.GetFilterColorTexture(FilterColorIndexQuarterRes),
			BlurOffsets,
			BlurWeights,
			NumSamples);

		DrawDenormalizedQuad(
			1, 1,
			SizeX, SizeY,
			1, 1,
			SizeX / 4,SizeY / 4,
			BufferSizeX, BufferSizeY,
			BufferSizeX, BufferSizeY
			);

	}

	// disable blending
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	GSceneRenderTargets.FinishRenderingFilter(FilterColorIndexFullRes);
}

/**
 * Blurs the subregion of the scene color buffer along a single axis.
 * @param SizeX - The width of the subregion to blur.
 * @param SizeY - The height of the subregion to blur.
 */
void SetupSceneColorGaussianBlurStep(FVector2D Direction, FLOAT KernelRadius, UBOOL bRenderingToLDR)
{
	FVector2D BlurOffsets[MAX_FILTER_SAMPLES];
	FLinearColor BlurWeights[MAX_FILTER_SAMPLES];
	UINT NumSamples;
	{
		FVector2D OffsetAndWeight[MAX_FILTER_SAMPLES];

		// compute 1D filtered samples
		NumSamples = Compute1DGaussianFilterKernel(KernelRadius, OffsetAndWeight);

		// make bilinear (2D) filtered samples
		for(UINT i = 0; i < NumSamples; ++i)
		{
			BlurOffsets[i] = Direction * OffsetAndWeight[i].X;
			BlurWeights[i] = FLinearColor::White * OffsetAndWeight[i].Y;
		}
	}

	SetFilterShaders(
		TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
		bRenderingToLDR ? GSceneRenderTargets.GetSceneColorLDRTexture() : GSceneRenderTargets.GetSceneColorTexture(),
		BlurOffsets,
		BlurWeights,
		NumSamples
		);
}
