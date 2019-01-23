/**********************************************************************

Filename    :   GFxUIRenderer.cpp
Content     :   GFx GRenderer implementation for UE3, defines the
				FGFxRenderer class wrapped around RHI.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

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
#include "GFxUIStats.h"
#include "../../Engine/Src/SceneRenderTargets.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push, 8)
#endif

#include "GFxPlayer.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(pop)
#endif

#include "GFxUIRendererImpl.h"

// *** Defines

// Currently, font and line data are converted
// into list structures for rendering.  This
// parameter defines the basis size of the local user
// pointer vertex buffer used for storing the lists.
#define GFX_LOCAL_BUFFER_BASIS_SIZE 192 

#ifndef GFX_ENQUEUE_RENDER_COMMAND
#define GFX_ENQUEUE_RENDER_COMMAND ENQUEUE_RENDER_COMMAND
#define GFX_ENQUEUE_UNIQUE_RENDER_COMMAND ENQUEUE_UNIQUE_RENDER_COMMAND
#define GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
#define GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
#define GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER
#endif

#ifndef GFXUIRENDERER_NO_COMMON

#include "GFxUIShaders.cpp"

IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_SolidColor>,TEXT("GFxPixelShader"),TEXT("Main_SolidColor"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformTexture>,TEXT("GFxPixelShader"),TEXT("Main_CxformTexture"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformTextureMultiply>,TEXT("GFxPixelShader"),TEXT("Main_CxformTextureMultiply"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_TextTexture>,TEXT("GFxPixelShader"),TEXT("Main_Glyph"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_TextTextureColor>,TEXT("GFxPixelShader"),TEXT("Main_GlyphColor"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_TextTextureColorMultiply>,TEXT("GFxPixelShader"),TEXT("Main_GlyphColorMultiply"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_TextTextureSRGB>,TEXT("GFxPixelShader"),TEXT("Main_GlyphSRGB"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_TextTextureSRGBMultiply>,TEXT("GFxPixelShader"),TEXT("Main_GlyphSRGBMultiply"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_TextTextureDFA>,TEXT("GFxPixelShader"),TEXT("Main_GlyphDFA"),SF_Pixel,0,0);

IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformGouraud>,TEXT("GFxPixelShader"),TEXT("Main_CxformGouraud"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformGouraudNoAddAlpha>,TEXT("GFxPixelShader"),TEXT("Main_CxformGouraud_NoAddAlpha"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformGouraudTexture>,TEXT("GFxPixelShader"),TEXT("Main_CxformGouraudTexture"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_Cxform2Texture>,TEXT("GFxPixelShader"),TEXT("Main_Cxform2Texture"),SF_Pixel,0,0);

IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformGouraudMultiply>,TEXT("GFxPixelShader"),TEXT("Main_CxformGouraudMultiply"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformGouraudMultiplyNoAddAlpha>,TEXT("GFxPixelShader"),TEXT("Main_CxformGouraudMultiply_NoAddAlpha"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformGouraudMultiplyTexture>,TEXT("GFxPixelShader"),TEXT("Main_CxformGouraudMultiplyTexture"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxPixelShader<GFx_PS_CxformMultiply2Texture>,TEXT("GFxPixelShader"),TEXT("Main_CxformMultiply2Texture"),SF_Pixel,0,0);

FGFxPixelShaderInterface* GetUIPixelShaderInterface_RenderThread( const EGFxPixelShaderType PixelShaderType )
{
	FGFxPixelShaderInterface* ShaderInterface = NULL;

#ifdef IMPLEMENT_GFX_SHADER_CASE
	#error "IMPLEMENT_GFX_SHADER_CASE already defined!"
#else
	#define IMPLEMENT_GFX_SHADER_CASE( Class, EShaderType )												\
		case ( EShaderType ):																			\
		{																								\
			TShaderMapRef< FGFx##Class##PixelShader< EShaderType > > NativeShader( GetGlobalShaderMap() );   	\
			check( NULL != *NativeShader );																\
			ShaderInterface = NativeShader->GetShaderInterface();										\
		}																								\
		break; 
#endif // IMPLEMENT_GFX_SHADER_CASE

	switch( PixelShaderType )
	{
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_SolidColor)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformTexture)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformTextureMultiply)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_TextTexture)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_TextTextureColor)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_TextTextureColorMultiply)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_TextTextureSRGB)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_TextTextureSRGBMultiply)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_TextTextureDFA)

	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformGouraud)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformGouraudNoAddAlpha)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformGouraudTexture)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_Cxform2Texture)

	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformGouraudMultiply)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformGouraudMultiplyNoAddAlpha)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformGouraudMultiplyTexture)
	IMPLEMENT_GFX_SHADER_CASE(,GFx_PS_CxformMultiply2Texture)

	default:
		ShaderInterface = GetUIPixelShaderInterface2_RenderThread(PixelShaderType);
	}

#undef IMPLEMENT_GFX_SHADER_CASE

	return ShaderInterface;
}

IMPLEMENT_SHADER_TYPE(template<>,FGFxVertexShader<GFx_VS_Strip>,TEXT("GFxVertexShader"),TEXT("MainStrip"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxVertexShader<GFx_VS_Glyph>,TEXT("GFxVertexShader"),TEXT("MainGlyph"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxVertexShader<GFx_VS_XY16iC32>,TEXT("GFxVertexShader"),TEXT("MainStripXY16iC32"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxVertexShader<GFx_VS_XY16iCF32>,TEXT("GFxVertexShader"),TEXT("MainStripXY16iCF32"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxVertexShader<GFx_VS_XY16iCF32_NoTex>,TEXT("GFxVertexShader"),TEXT("MainStripXY16iCF32_NoTex"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxVertexShader<GFx_VS_XY16iCF32_NoTexNoAlpha>,TEXT("GFxVertexShader"),TEXT("MainStripXY16iC32_NoTexNoAlpha"),SF_Vertex,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxVertexShader<GFx_VS_XY16iCF32_T2>,TEXT("GFxVertexShader"),TEXT("MainStripXY16iCF32_T2"),SF_Vertex,0,0);

FGFxVertexShaderInterface* GetUIVertexShaderInterface_RenderThread( const EGFxVertexShaderType VertexType )
{
	FGFxVertexShaderInterface* ShaderInterface = NULL;

#ifdef IMPLEMENT_GFX_SHADER_CASE
	#error "IMPLEMENT_GFX_SHADER_CASE already defined!"
#else
	#define IMPLEMENT_GFX_SHADER_CASE( EShaderType )													\
		case ( EShaderType ):																			\
		{																								\
			TShaderMapRef< FGFxVertexShader< EShaderType > > NativeShader( GetGlobalShaderMap() );	    \
			check( NULL != *NativeShader );																\
			ShaderInterface = NativeShader->GetShaderInterface();										\
		}																								\
		break; 
#endif // IMPLEMENT_GFX_SHADER_CASE

	switch( VertexType )
	{
	IMPLEMENT_GFX_SHADER_CASE( GFx_VS_Strip )
	IMPLEMENT_GFX_SHADER_CASE( GFx_VS_Glyph )
	IMPLEMENT_GFX_SHADER_CASE( GFx_VS_XY16iC32 )
	IMPLEMENT_GFX_SHADER_CASE( GFx_VS_XY16iCF32 )
	IMPLEMENT_GFX_SHADER_CASE( GFx_VS_XY16iCF32_NoTex )
	IMPLEMENT_GFX_SHADER_CASE( GFx_VS_XY16iCF32_NoTexNoAlpha )
	IMPLEMENT_GFX_SHADER_CASE( GFx_VS_XY16iCF32_T2 )
	default: checkMsg( 0, "GetVertexShaderInterface_RenderThread: Unrecognized Vertex Shader Type" ); break;
	}
#undef IMPLEMENT_GFX_SHADER_CASE

	return ShaderInterface;
}

void FGFxRenderResources::Allocate_RenderThread(INT InSizeX, INT InSizeY)
{
	if (SizeX >= InSizeX && SizeY >= InSizeY)
		return;

	ReleaseResource();
	SizeX = InSizeX;
	SizeY = InSizeY;
	InitResource();
}

void FGFxRenderResources::InitDynamicRHI()
{
	DepthSurface = RHICreateTargetableSurface(SizeX,SizeY,PF_DepthStencil,FTexture2DRHIRef(),TargetSurfCreate_Dedicated,TEXT("GFxDepth"));

	INC_DWORD_STAT_BY(STAT_GFxRTTextureMem, SizeX*SizeY*4);
}

void FGFxRenderResources::ReleaseDynamicRHI()
{
	DepthSurface.SafeRelease();

	DEC_DWORD_STAT_BY(STAT_GFxRTTextureMem, SizeX*SizeY*4);
}

FGFxRenderResources::~FGFxRenderResources()
{
	if (IsInGameThread())
		BeginReleaseResource(this);
	else
		ReleaseResource();
}

#endif // NO_COMMON

// *** Begin helper namespace
namespace FGFxRendererImpl
{

FVertexDeclarationRHIRef GetUIVertexDecl_RenderThread( const EGFxVertexDeclarationType DeclType, DWORD* Strides)
{
	FVertexDeclarationElementList VertexDeclElementList;
	switch( DeclType )
	{
	case GFx_VD_Strip:
		// Our vertex coords consist of two signed 16-bit integers, for (x,y) position only.
		VertexDeclElementList.AddItem( FVertexElement( 
			  0				/*InStreamIndex*/
			, 0				/*InOffset*/
			, VET_Short2	/*InType*/
			, VEU_Position	/*InUsage*/
			, 0				/*InUsageIndex*/
			) );
		Strides[0] = 4; // Short 2 = 2x unsigned 16-bit ints = 4 bytes
		break;
	case GFx_VD_Glyph:
		VertexDeclElementList.AddItem( FVertexElement( 
			  0				/*InStreamIndex*/
			, 0				/*InOffset*/
			, VET_Float2	/*InType*/
			, VEU_Position	/*InUsage*/
			, 0				/*InUsageIndex*/
			) );
		VertexDeclElementList.AddItem( FVertexElement( 
			  0						/*InStreamIndex*/
			, 8						/*InOffset*/
			, VET_Float2			/*InType*/
			, VEU_TextureCoordinate	/*InUsage*/
			, 0						/*InUsageIndex*/
			) );
		VertexDeclElementList.AddItem( FVertexElement(
			  0				/*InStreamIndex*/
			, 16    		/*InOffset*/
			, VET_UByte4N	/*InType*/
			, VEU_Color		/*InUsage*/
			, 0				/*InUsageIndex*/
			) );
		Strides[0] = 20; // 2x Float2 + color = 20 bytes. 
		break;
	case GFx_VD_XY16iC32:
		VertexDeclElementList.AddItem( FVertexElement( 
			  0				/*InStreamIndex*/
			, 0				/*InOffset*/
			, VET_Short2	/*InType*/
			, VEU_Position	/*InUsage*/
			, 0				/*InUsageIndex*/
			) );
		VertexDeclElementList.AddItem( FVertexElement(
			  0				/*InStreamIndex*/
			, 4				/*InOffset*/
			, VET_UByte4N	/*InType*/
			, VEU_Color		/*InUsage*/
			, 0				/*InUsageIndex*/
			) );
		Strides[0] = 8; // Short2 = 4 bytes, UByte4N = 4 bytes.
		break;
	case GFx_VD_XY16iCF32:
		VertexDeclElementList.AddItem( FVertexElement( 
			  0				/*InStreamIndex*/
			, 0				/*InOffset*/
			, VET_Short2	/*InType*/
			, VEU_Position	/*InUsage*/
			, 0				/*InUsageIndex*/
			) );
		VertexDeclElementList.AddItem( FVertexElement(
			  0				/*InStreamIndex*/
			, 4				/*InOffset*/
			, VET_UByte4N	/*InType*/
			, VEU_Color		/*InUsage*/
			, 0				/*InUsageIndex*/
			) );
		VertexDeclElementList.AddItem( FVertexElement(
			  0				/*InStreamIndex*/
			, 8				/*InOffset*/
			, VET_UByte4N	/*InType*/
			, VEU_Color		/*InUsage*/
			, 1				/*InUsageIndex*/
			) );
		Strides[0] = 12; // Short2 = 4 bytes, UByte4N = 4 bytes; total = 12;
		break;
	default: checkMsg( 0, "CreateUIVertexDecl: Unrecognized vertex declaration type" );
		break;
	}

	FVertexDeclarationRHIRef VertexDecl = RHICreateVertexDeclaration( VertexDeclElementList );
	checkMsg( IsValidRef( VertexDecl ), "Invalid Vertex Declaration" );
	return VertexDecl;
}


UBOOL IsValidUIBoundShaderState( const FGFxBoundShaderState& BoundShaderState )
{
	return IsValidRef(BoundShaderState.NativeBoundShaderState)
		&& IsValidRef(BoundShaderState.PixelShaderInterface->GetNativeShader()->GetPixelShader());
}

void GetUIBoundShaderState_RenderThread(
	FGFxBoundShaderState& BoundShaderState,
	FGFxBoundShaderStateCache& BoundShaderStateCache,
	const FGFxEnumeratedBoundShaderState& EnumeratedBoundShaderState
	)
{
	check( EnumeratedBoundShaderState.IsValid() );
	const EGFxPixelShaderType       PixelShaderType       = EnumeratedBoundShaderState.PixelShaderType;
	const EGFxVertexShaderType      VertexShaderType      = EnumeratedBoundShaderState.VertexShaderType;
	const EGFxVertexDeclarationType VertexDeclarationType = EnumeratedBoundShaderState.VertexDeclarationType;

	BoundShaderState.PixelShaderInterface  = GetUIPixelShaderInterface_RenderThread(PixelShaderType);
	BoundShaderState.VertexShaderInterface = GetUIVertexShaderInterface_RenderThread(VertexShaderType);
	check( NULL != BoundShaderState.PixelShaderInterface );
	check( NULL != BoundShaderState.VertexShaderInterface );

	const DWORD NativeBoundShaderStateKey = PixelShaderType | (VertexShaderType << 8) | (VertexDeclarationType << 16);
	FBoundShaderStateRHIRef* const NativeBoundShaderState = BoundShaderStateCache.Find(NativeBoundShaderStateKey);
	if ( NativeBoundShaderState )
	{
		BoundShaderState.NativeBoundShaderState = *NativeBoundShaderState;
		check( IsValidUIBoundShaderState(BoundShaderState) );
	}
	else
	{
		DWORD Strides[MaxVertexElementCount];
		appMemzero(Strides, sizeof(Strides));

		FVertexDeclarationRHIRef VertexDecl = GetUIVertexDecl_RenderThread(VertexDeclarationType, Strides);
		check( IsValidRef(VertexDecl) );
		// Create bound shader state
		BoundShaderState.NativeBoundShaderState = RHICreateBoundShaderState( 
			  VertexDecl, Strides,            
			  BoundShaderState.VertexShaderInterface->GetNativeShader()->GetVertexShader(),
			  BoundShaderState.PixelShaderInterface->GetNativeShader()->GetPixelShader() );
		check( IsValidUIBoundShaderState(BoundShaderState) );
		BoundShaderStateCache.Set(NativeBoundShaderStateKey, BoundShaderState.NativeBoundShaderState);
	}
}

// NOTE: AxisInfo.ViewStart and AxisInfo.ViewLength are assumed to be already initialized with proper defaults
static void ClipUIViewportAxis(FGFxViewportAxisInfo& AxisInfo)
{
	AxisInfo.PixelClipStart = AxisInfo.PixelStart;
	AxisInfo.PixelClipEnd = AxisInfo.PixelEnd;
	AxisInfo.PixelClipLength = Min( 1.f, AxisInfo.PixelEnd - AxisInfo.PixelStart);

	const INT ViewStartInitial = AxisInfo.ViewStart;

	if (AxisInfo.ViewStart < 0)
	{
		AxisInfo.PixelClipStart = AxisInfo.PixelStart + ((-AxisInfo.ViewStart) * AxisInfo.PixelClipLength) / (float) AxisInfo.ViewLength;
		AxisInfo.ViewStart = 0;
	}
	if (ViewStartInitial + AxisInfo.ViewLength > AxisInfo.ViewLengthMax)
	{
		AxisInfo.PixelClipEnd = AxisInfo.PixelStart + ((AxisInfo.ViewLengthMax - ViewStartInitial) * AxisInfo.PixelClipLength) / (float) AxisInfo.ViewLength;
		AxisInfo.ViewLength = AxisInfo.ViewLengthMax - AxisInfo.ViewStart;
	}
	AxisInfo.PixelClipLength  = AxisInfo.PixelClipEnd - AxisInfo.PixelClipStart;
}

// Can be used with GRenderer::Cxform or GRenderer::Matrix
template< typename TransformType > 
void SetUITransformMatrix(const TransformType& NewMatrix, TransformType& OutMatrix)
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
		(
		SetUITransformMatrixCommand,
		const TransformType, SrcMatrix, NewMatrix,
		TransformType* const, OutMatrixPtr, &OutMatrix, 
	{ 
		*OutMatrixPtr = SrcMatrix;
	}
	)
}

static void MakeViewportMatrix(GRenderer::Matrix& NewViewportMatrix, const FGFxViewportUserParams& ViewportParams)
{
	NewViewportMatrix.SetIdentity();
	NewViewportMatrix.M_[0][0] = 2.0f  / ViewportParams.xAxis.PixelClipLength;
	NewViewportMatrix.M_[1][1] = -2.0f / ViewportParams.yAxis.PixelClipLength;

	Float xHalfPixelAdjust = (ViewportParams.xAxis.ViewLength > 0) ? ((2.0f * GPixelCenterOffset) / (Float) ViewportParams.xAxis.ViewLength) : 0.0f;
	Float yHalfPixelAdjust = (ViewportParams.yAxis.ViewLength > 0) ? ((2.0f * GPixelCenterOffset) / (Float) ViewportParams.yAxis.ViewLength) : 0.0f;

	// MA: it does not seem necessary to subtract Viewport.
	// Need to work through details of Viewport cutting, there still seems to
	// be a 1-pixel issue at edges. Could that be due to edge fill rules ?
	NewViewportMatrix.M_[0][2] = -1.0f - NewViewportMatrix.M_[0][0] * ( ViewportParams.xAxis.PixelClipStart ) - xHalfPixelAdjust; 
	NewViewportMatrix.M_[1][2] = 1.0f  - NewViewportMatrix.M_[1][1] * ( ViewportParams.yAxis.PixelClipStart ) + yHalfPixelAdjust;
}

//---------------------------------------------------------------------------------------------------------------------
// GFx UI Converters
//---------------------------------------------------------------------------------------------------------------------

void ConvertFromUI( const GRenderer::Matrix& InUIMatrix, FMatrix& OutMatrix )
{
	// Assuming FMatrix is a column matrix
	OutMatrix = FMatrix::Identity;
	OutMatrix.M[0][0] = InUIMatrix.M_[0][0];
	OutMatrix.M[1][0] = InUIMatrix.M_[0][1];
	OutMatrix.M[2][0] = 0;
	OutMatrix.M[3][0] = InUIMatrix.M_[0][2];

	OutMatrix.M[0][1] = InUIMatrix.M_[1][0];
	OutMatrix.M[1][1] = InUIMatrix.M_[1][1];
	OutMatrix.M[2][1] = 0;
	OutMatrix.M[3][1] = InUIMatrix.M_[1][2];
}

void ConvertFromUI( const GColor InUIColor, FLinearColor& OutColor )
{
	InUIColor.GetRGBAFloat(
		&(OutColor.R),
		&(OutColor.G),
		&(OutColor.B),
		&(OutColor.A) );
}

void ConvertFromUI( const GRenderer::FillTexture& InFillTex, FGFxRenderer::FFillTextureInfo& OutTexInfo )
{
	// Texture can be null if loading failed.
	if (InFillTex.pTexture)
	{
		const FGFxTexture& UITexture = *static_cast<FGFxTexture*>( InFillTex.pTexture );
		// HACK - till it's determined that the pointer can be const
		OutTexInfo.Texture = const_cast<FTexture*>( UITexture.GetTextureResource() );
		check( NULL != OutTexInfo.Texture );
		OutTexInfo.TextureMatrix = InFillTex.TextureMatrix;
		OutTexInfo.WrapMode      = InFillTex.WrapMode;
		OutTexInfo.SampleMode    = InFillTex.SampleMode;
		OutTexInfo.bUseMips      = UITexture.UsesMips();
	}
}


//---------------------------------------------------------------------------------------------------------------------
// GFx UI Blending
//---------------------------------------------------------------------------------------------------------------------

// Some blend modes (see //??? below) are impractical to support without additional rendering passes or intermediate buffers.
void ApplyUIBlendMode_RenderThread(UBOOL bAlphaComposite, const GRenderer::BlendType NewMode, UBOOL bSourceAc = FALSE)
{
	if (bSourceAc)
	{
		switch(NewMode)
		{
		case GRenderer::Blend_None:     RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Normal:   RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Layer:    RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Multiply: RHISetBlendState( TStaticBlendState<BO_Add, BF_DestColor,   BF_Zero,
																			BO_Add, BF_DestAlpha,   BF_Zero>::GetRHI() ); break;
		case GRenderer::Blend_Screen:   RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break; //???
		case GRenderer::Blend_Lighten:  RHISetBlendState( TStaticBlendState<BO_Max, BF_One,         BF_One,
																			BO_Max, BF_One,         BF_One>::GetRHI() ); break;
		case GRenderer::Blend_Darken:   RHISetBlendState( TStaticBlendState<BO_Min, BF_One,         BF_One,
																			BO_Min, BF_One,         BF_One>::GetRHI() ); break;
		case GRenderer::Blend_Difference:RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Add:      RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_One,
																			BO_Add, BF_Zero,        BF_One>::GetRHI() ); break;
		case GRenderer::Blend_Subtract: RHISetBlendState( TStaticBlendState<BO_ReverseSubtract, BF_One,         BF_One,
																			BO_ReverseSubtract, BF_Zero,        BF_One>::GetRHI() ); break;
		case GRenderer::Blend_Invert:   RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Alpha:    RHISetBlendState( TStaticBlendState<BO_Add, BF_Zero, BF_One>::GetRHI() ); break; //???
		case GRenderer::Blend_Erase:    RHISetBlendState( TStaticBlendState<BO_Add, BF_Zero, BF_One>::GetRHI() ); break; //???
		case GRenderer::Blend_Overlay:  RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_HardLight:RHISetBlendState( TStaticBlendState<BO_Add, BF_One,         BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		default:
			checkMsg( 0, "ApplyBlendMode:  Invalid BlendType ApplyBlendMode encountered by FGFxRenderer" );
			break;
		}
	}
	else if (bAlphaComposite)
	{
		switch(NewMode)
		{
		case GRenderer::Blend_None:     RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Normal:   RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Layer:    RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Multiply: RHISetBlendState( TStaticBlendState<BO_Add, BF_DestColor,   BF_Zero,
																			BO_Add, BF_DestAlpha,   BF_Zero>::GetRHI() ); break;
		case GRenderer::Blend_Screen:   RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break; //???
		case GRenderer::Blend_Lighten:  RHISetBlendState( TStaticBlendState<BO_Max, BF_SourceAlpha, BF_One,
																			BO_Max, BF_One,         BF_One>::GetRHI() ); break;
		case GRenderer::Blend_Darken:   RHISetBlendState( TStaticBlendState<BO_Min, BF_SourceAlpha, BF_One,
																			BO_Min, BF_One,         BF_One>::GetRHI() ); break;
		case GRenderer::Blend_Difference:RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Add:      RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_One,
																			BO_Add, BF_Zero,        BF_One>::GetRHI() ); break;
		case GRenderer::Blend_Subtract: RHISetBlendState( TStaticBlendState<BO_ReverseSubtract, BF_SourceAlpha, BF_One,
																			BO_ReverseSubtract, BF_Zero,        BF_One>::GetRHI() ); break;
		case GRenderer::Blend_Invert:   RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_Alpha:    RHISetBlendState( TStaticBlendState<BO_Add, BF_Zero, BF_One>::GetRHI() ); break; //???
		case GRenderer::Blend_Erase:    RHISetBlendState( TStaticBlendState<BO_Add, BF_Zero, BF_One>::GetRHI() ); break; //???
		case GRenderer::Blend_Overlay:  RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		case GRenderer::Blend_HardLight:RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha,
																			BO_Add, BF_One,         BF_InverseSourceAlpha>::GetRHI() ); break;
		default:
			checkMsg( 0, "ApplyBlendMode:  Invalid BlendType ApplyBlendMode encountered by FGFxRenderer" );
			break;
		}
	}
	else
	switch(NewMode)
	{
	case GRenderer::Blend_None:     RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI() ); break;
	case GRenderer::Blend_Normal:   RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI() ); break;
	case GRenderer::Blend_Layer:    RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI() ); break;
	case GRenderer::Blend_Multiply: RHISetBlendState( TStaticBlendState<BO_Add, BF_DestColor, BF_Zero>::GetRHI() ); break;
	case GRenderer::Blend_Screen:   RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI() ); break; //???
	case GRenderer::Blend_Lighten:  RHISetBlendState( TStaticBlendState<BO_Max, BF_SourceAlpha, BF_One>::GetRHI() ); break;
	case GRenderer::Blend_Darken:   RHISetBlendState( TStaticBlendState<BO_Min, BF_SourceAlpha, BF_One>::GetRHI() ); break;
	case GRenderer::Blend_Difference: RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI() ); break; //???
	case GRenderer::Blend_Add:      RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_One>::GetRHI() ); break;
	case GRenderer::Blend_Subtract: RHISetBlendState( TStaticBlendState<BO_ReverseSubtract, BF_SourceAlpha, BF_One>::GetRHI() ); break;
	case GRenderer::Blend_Invert:   RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI() ); break; //???
	case GRenderer::Blend_Alpha:    RHISetBlendState( TStaticBlendState<BO_Add, BF_Zero, BF_One>::GetRHI() ); break; //???
	case GRenderer::Blend_Erase:    RHISetBlendState( TStaticBlendState<BO_Add, BF_Zero, BF_One>::GetRHI() ); break; //???
	case GRenderer::Blend_Overlay:  RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI() ); break; //???
	case GRenderer::Blend_HardLight: RHISetBlendState( TStaticBlendState<BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI() ); break; //???
	default:
		checkMsg( 0, "ApplyBlendMode:  Invalid BlendType ApplyBlendMode encountered by FGFxRenderer" );
		break;
	}
}

void PreBlendNativeColor(FLinearColor& Color, const GRenderer::BlendType Mode)
{
	if ((Mode == GRenderer::Blend_Multiply) ||
		(Mode == GRenderer::Blend_Darken) )
	{
		Color.R = gflerp(1.f, Color.R, Color.A);
		Color.G = gflerp(1.f, Color.G, Color.A);
		Color.B = gflerp(1.f, Color.B, Color.A);
	}
}

//---------------------------------------------------------------------------------------------------------------------
// GFx UI Color Binding
//---------------------------------------------------------------------------------------------------------------------

void ApplyUIColor_RenderThread(
	FGFxRenderer* Renderer, const GColor Color,
	GRenderer::BlendType BlendMode,
	FGFxPixelShaderInterface& PixelShader )
{
	FLinearColor NativeColor;
	ConvertFromUI(Color, NativeColor );
	PreBlendNativeColor( NativeColor, BlendMode );
	PixelShader.SetParameterConstantColor(PixelShader.GetNativeShader()->GetPixelShader(), Renderer->CorrectGamma(NativeColor));
}


//---------------------------------------------------------------------------------------------------------------------
// GFx UI Background Rendering
//---------------------------------------------------------------------------------------------------------------------

// Render background color quad
void DrawUIBackgroundColor_RenderThread(
	FGFxRenderer* Renderer, 
	const GColor BackgroundColor,
	const TArray<FGFxVertex_XY16i>& BackgroundQuad,
	const FGFxTransformHandles& TransformHandles,
	FGFxBoundShaderStateCache& BoundShaderStateCache)
{
	check(BackgroundQuad.Num() == 4);

	FGFxBoundShaderState BoundShaderState;
	FGFxEnumeratedBoundShaderState EnumeratedBoundShaderState;
	EnumeratedBoundShaderState.PixelShaderType       = GFx_PS_SolidColor;
	EnumeratedBoundShaderState.VertexShaderType      = GFx_VS_Strip;
	EnumeratedBoundShaderState.VertexDeclarationType = GFx_VD_Strip;
    
	GetUIBoundShaderState_RenderThread(BoundShaderState, BoundShaderStateCache, EnumeratedBoundShaderState);
	check(IsValidUIBoundShaderState(BoundShaderState));

	// Obtain shaders
	FGFxVertexShaderInterface& VertexShader = *BoundShaderState.VertexShaderInterface;
	FGFxPixelShaderInterface& PixelShader = *BoundShaderState.PixelShaderInterface;

	PixelShader.SetParameterInverseGamma(PixelShader.GetNativeShader()->GetPixelShader(), Renderer->GetCurRenderTarget()->Resource.InverseGamma);

	// Apply background color to pixel shader
	ApplyUIColor_RenderThread(Renderer, BackgroundColor, GRenderer::Blend_None, PixelShader);

	// Apply transformation matrix to vertex shader
	Renderer->ApplyUITransform_RenderThread(TransformHandles.ViewportMatrix, TransformHandles.CurrentMatrix, VertexShader);
        
	// Apply bound shader state.
	RHISetBoundShaderState(BoundShaderState.NativeBoundShaderState);

	// Draw the background quad
	check( sizeof(FGFxVertex_Strip) == sizeof(FGFxVertex_XY16i) );
	RHIDrawPrimitiveUP(PT_TriangleStrip, 2, &(BackgroundQuad(0)), sizeof(FGFxVertex_Strip));
        
}

/** FGFxRenderElementStoreBase - Base class for vertex and index store.
 *
 * The main purpose of this class is to enable vertex data to be passed
 * into the render thread without allocation and copy. In most cases this is safe
 * since GFx keeps the data around in memory until GRenderer::ReleaseCachedData
 * is called on the main data; however, if the call is made we may need to make
 * a copy, if necessary. Such events are very rare in practice.
 * Note that FGFxRenderElementStoreBase data is stored in a renderer list that 
 * is updated whenever new data is set or GRenderer::ReleaseCachedData is called.
 */
struct FGFxRenderElementStoreBase : public GRendererNode
{    
	// Reference count is used to control lifetime of CachedDataStore
	// as follows:
	//  - When created and stored in CachedList, RefCount is 1.
	//  - Incremented when put into render thread command list.
	//  - Decremented when VertexStore is cleared in DestroyUIRenderElementStore_RenderThread.
	//    If 0, it is destroyed.
	//  - Decremented when ReleaseCachedData is called. Here the data
	//    is removed from cached list and if RefCount is 0, it is destroyed.
	//    However, if CachedData is not 0, buffer data is COPIED and AllocatedElements
	//    flag is set. This requires a lock since we shouldn't be accessing
	//    the object after atomic decrement.
	GAtomicInt<UInt32> RefCount;

	// Cached data that points to this, if not null.
	GRenderer::CachedData* CachedData;

	// Size and type of data.
	UInt32     ElementSize;
	UInt32     NumElements;    

	// *** Lockable Data
	// This data is only accessed when FGFxRenderer::Lock is held.
	// Locked access is required to ensure that GRenderer::ReleaseCachedData
	// can still be called and can re-allocate Elements if necessary.
	void*        Elements;
	bool         AllocatedElements;

	FGFxRenderElementStoreBase() : 
		RefCount(1), 
		CachedData(0),
		ElementSize(0), 
		NumElements(0), 
		Elements(0), 
		AllocatedElements(false)
	{}

	// Helper: insert our node into linked list.
	void InsertIntoList(GRendererNode* List)
	{
		pNext = List->pFirst;
		pPrev = List;
		List->pFirst->pPrev = this;
		List->pFirst = this;
	}

	// Initializes node and adds to list (preserves data pointer).
	void InitElements(FGFxRenderer* Renderer, GRenderer::CachedData* InCachedData, size_t InElementSize, size_t InNumElements, void* InElements)
	{        
		check(Elements == 0); // We must start with an empty node.

		InsertIntoList(&Renderer->ElementStoreList);
		// List gets an extra reference to us.
		AddRef_MainThread();

		CachedData       = InCachedData;
		ElementSize       = InElementSize;
		NumElements       = InNumElements;
		AllocatedElements = false;
		Elements         = InElements;
	}

	// Initializes node and copies elements, used if cache provider is not available.
	void InitElementsCopy(size_t InElementSize, size_t InNumElements, const void* InElements)
	{        
		check(Elements == 0); // We must start with an empty node.
		pNext = pPrev = 0;

		ElementSize       = InElementSize;
		NumElements       = InNumElements;
		AllocatedElements = true; 

		Elements = appMalloc(ElementSize * NumElements);
		check(Elements != 0);
		appMemcpy(Elements, InElements, ElementSize * NumElements);
	}

	void AddRef_MainThread()
	{
		RefCount++;
	}

	void Release_MainThread(GLock* ElementsLock)
	{
		// While we still own the reference count, check if there are other
		// references. This is ok because ours is the only thread that is allowed
		// to increment the reference count (render thread can only decrement it).
		// This means that the worst thing that can happen in a race condition
		// is an extra allocation and release of buffer here, but that is not likely.
		if (RefCount > 1)
		{
			GLock::Locker Guard(ElementsLock);

			if (!AllocatedElements)
			{                
				void* NewElements = appMalloc(ElementSize * NumElements);
				check( NULL != NewElements );                              
				appMemcpy(NewElements, Elements, ElementSize * NumElements);

				Elements         = NewElements;
				AllocatedElements = true;
			}
		}

		if (pNext)
		{
			RemoveNode();
		}

		// Decrement ref count and release us.
		if ((RefCount.ExchangeAdd_NoSync(-1) - 1) == 0)
		{
			{ 
				GLock::Locker Guard(ElementsLock);

				if (AllocatedElements)
				{
					appFree(Elements);
					AllocatedElements = false;
					Elements         = 0; // For safety.
				}
			}
			delete this;
		}
	}


	void Release_RenderThread(GLock* ElementsLock)
	{
		// Decrement ref count and release us.
		if ((RefCount.ExchangeAdd_NoSync(-1) - 1) == 0)
		{
			// Main thread must have already released the node if we are here!
			check(pNext == 0);
			{ 
				GLock::Locker Guard(ElementsLock);

				if (AllocatedElements)
				{
					appFree(Elements);
					AllocatedElements = false;
					Elements         = 0; // For safety.
				}
			}
			delete this;
		}
	}

};


template < typename RenderElementStore >
void SetUIRenderElementStore(
	FGFxRenderer* Renderer,
	GRenderer::CachedDataType BuffType,
	const void* Elements,
	const int NumElements,
	const typename RenderElementStore::ElementFormat Format,
	GRenderer::CacheProvider* const Cache,
	RenderElementStore*& OwnedStore)
{
	// If we are passing null data, just queue up a clear command.
	if (!Elements || (NumElements <= 0))
	{
		// Wipe-out current element store
		GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
		(
			DestroyUIElementStoreCommand,
			GLock*, Lock, Renderer->GetElementsLock(),
			RenderElementStore** const, OwnedStorePtr, &OwnedStore,
		{
			DestroyUIRenderElementStore_RenderThread(Lock, *OwnedStorePtr);
		}	
		)
	}
	else
	{
		RenderElementStore* Store = 0;
		if (Cache)
		{
			GRenderer::CachedData* CachedData    = Cache->GetCachedData(Renderer);
			if (CachedData)
			{   // If this element is already cached, just AddRef for command and reuse the node.
				Store = (RenderElementStore*)CachedData->GetRendererData();
				Store->AddRef_MainThread();
			}
			else if ((CachedData = Cache->CreateCachedData(BuffType, Renderer)) != 0)
			{
				// List-inserting constructor.
				Store = new RenderElementStore();
				Store->InitElements(Renderer, CachedData, Format, NumElements, const_cast<void*>(Elements));
				CachedData->SetRendererData(Store);
			}            
		}
		else
		{
			// Regular queue
			// Data-copy constructor; initialize without inserting into the list.
			Store = new RenderElementStore();
			Store->InitElementsCopy(Format, NumElements, Elements);
		}
		check(Store != 0);

		// Push ownership to render thread.
		GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			TransferNewUIRenderElementStoreCommand,
			GLock*, Lock, Renderer->GetElementsLock(),
			RenderElementStore* const, NewStore, Store,
			RenderElementStore** const, OwnedStorePtr, &OwnedStore,
		{
			DestroyUIRenderElementStore_RenderThread(Lock, *OwnedStorePtr);
			// Element store is already AddRefed buy the caller,
			// so simply initialize the pointer to it.
			*OwnedStorePtr = NewStore;
		}
		)
	}
}

template < typename ElementFormatType >
struct FGFxRenderElementStore : public FGFxRenderElementStoreBase
{
	typedef ElementFormatType  ElementFormat;    
	ElementFormat           Format;

	FGFxRenderElementStore() { }

	// *** Derived Class initializers

	// These Init functions are different from the base class because they
	// accept ElementFormatType argument instead of size.

	// Initializes node and adds to list (preserves data pointer).
	void InitElements(FGFxRenderer* Renderer, GRenderer::CachedData* CachedData, ElementFormatType InFormat, size_t NumElements, void* Elements)
	{   
		Format = InFormat;
		FGFxRenderElementStoreBase::InitElements(Renderer, CachedData, GetUIRenderElementSize(Format), NumElements, Elements);
	}      
	// Initializes node and copies elements, used it cache provider is not available.
	void InitElementsCopy(ElementFormatType InFormat, size_t NumElements, const void* Elements)
	{  
		Format = InFormat;
		FGFxRenderElementStoreBase::InitElementsCopy(GetUIRenderElementSize(Format), NumElements, Elements);        
	}    
};


struct FGFxVertexStore : public FGFxRenderElementStore< GRenderer::VertexFormat > {};
struct FGFxIndexStore : public FGFxRenderElementStore< GRenderer::IndexFormat > {};
struct FGFxBitmapDescStore : public FGFxRenderElementStore< GRenderer::BitmapDesc* > {};

// Pushed outside namespace as it is referenced by render thread
template < typename RenderElementStore >
void DestroyUIRenderElementStore_RenderThread(GLock* Lock,  RenderElementStore*& pOwnedStore)
{
	if (pOwnedStore)
	{
		// This can clear one of the following:
		//  FGFxRenderer::VertexStore, FGFxRenderer::IndexStore
		pOwnedStore->Release_RenderThread(Lock);
		pOwnedStore = NULL;
	}
};

}
// End helper namespace

//---------------------------------------------------------------------------------------------------------------------
// GFx UI Transform Binding
//---------------------------------------------------------------------------------------------------------------------

void FGFxRenderer::ApplyUITransform_RenderThread(const GRenderer::Matrix& ViewportMatrix, const GRenderer::Matrix& TransformMatrix, FGFxVertexShaderInterface& VertexShader)
{
#if (GFC_FX_MAJOR_VERSION >= 3 && GFC_FX_MINOR_VERSION >= 2) && !defined(GFC_NO_3D)
	if (Is3DEnabled)
	{
		GMatrix3D Final = GMatrix3D(TransformMatrix)* WorldMatrix;

        if (UVPMatricesChanged)
        {
            UVPMatricesChanged = 0;
            UVPMatrix = ViewMatrix;
		    UVPMatrix.Append(ProjMatrix);

            if (RenderTargetStack.GetSize())
                Adjust3DMatrixForRT(UVPMatrix, ProjMatrix, RenderTargetStack[0].ViewMatrix, ViewportMatrix);
        }

        Final.Append(UVPMatrix);

		FMatrix NativeMatrix;
		appMemcpy(NativeMatrix.M, Final.M_, sizeof(Float)*16);
		VertexShader.SetParameterTransform(VertexShader.GetNativeShader()->GetVertexShader(), NativeMatrix);
	}
	else
#endif
	{
		FMatrix OutMatrix( FMatrix::Identity );
		GMatrix2D InUIMatrix = ViewportMatrix;
		InUIMatrix.Prepend(TransformMatrix);

		OutMatrix.M[0][0] = InUIMatrix.M_[0][0];
		OutMatrix.M[1][0] = InUIMatrix.M_[0][1];
		OutMatrix.M[2][0] = 0;
		OutMatrix.M[3][0] = InUIMatrix.M_[0][2];

		OutMatrix.M[0][1] = InUIMatrix.M_[1][0];
		OutMatrix.M[1][1] = InUIMatrix.M_[1][1];
		OutMatrix.M[2][1] = 0;
		OutMatrix.M[3][1] = InUIMatrix.M_[1][2];

		VertexShader.SetParameterTransform(VertexShader.GetNativeShader()->GetVertexShader(), OutMatrix);
	}
}

using namespace FGFxRendererImpl;

//---------------------------------------------------------------------------------------------------------------------
// FGFxRenderStyle
//---------------------------------------------------------------------------------------------------------------------

void FGFxRenderer::FGFxRenderStyle::EndDisplay_RenderThread()
{
	StyleMode = GFx_SM_Disabled;
	Color = GColor::Red;
}

void FGFxRenderer::FFillTextureInfo::EndDisplay_RenderThread()
{
	Texture = NULL;
	TextureMatrix = GRenderer::Matrix::Identity;
	WrapMode      = GRenderer::Wrap_Clamp;
	SampleMode    = GRenderer::Sample_Linear;
}

void FGFxRenderer::FGFxFillStyle::EndDisplay_RenderThread()
{
	FGFxRenderStyle::EndDisplay_RenderThread();
	CxColorMatrix   = GRenderer::Cxform::Identity;
	GouraudFillMode = GRenderer::GFill_Color;
	TexInfo.EndDisplay_RenderThread();
	TexInfo2.EndDisplay_RenderThread();
}

void FGFxRenderer::FGFxRenderStyle::SetStyleColor_RenderThread( const GColor NewColor, const GRenderer::Cxform& PersistentCxMatrix )
{
	Color = PersistentCxMatrix.Transform( NewColor );
	StyleMode = GFx_SM_Color;
}

void FGFxRenderer::FGFxRenderStyle::Disable()
{
	// HACK - casting enum* to DWORD* to allow style types to be private
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		DisableUIStyleCommand,
		DWORD* const, StyleMode, (DWORD *) &StyleMode,
		const DWORD, NewStyleMode, GFx_SM_Disabled,
	{
		*StyleMode = NewStyleMode;
	}
	)
}

void FGFxRenderer::FGFxRenderStyle::Apply_RenderThread(FGFxRenderer* Renderer, const void* const BoundShaderState, void* const RenderStyleContext)
{
	check( GFx_SM_Disabled != StyleMode );
	ApplyColor_RenderThread( Renderer, BoundShaderState, RenderStyleContext );
	ApplyColorScaleAndColorBias_RenderThread( BoundShaderState, RenderStyleContext );
}

void FGFxRenderer::FGFxRenderStyle::ApplyColor_RenderThread(FGFxRenderer* Renderer, const void* const BoundShaderState, const void* const RenderStyleContext)
{
	check( GFx_SM_Disabled != StyleMode );

	const FGFxBoundShaderState& GFxBoundShaderState = 
		*static_cast<const FGFxBoundShaderState* const>(BoundShaderState);
	const FGFxRenderStyleContext& StyleContext = 
		*static_cast<const FGFxRenderStyleContext* const>(RenderStyleContext);

	ApplyUIColor_RenderThread(Renderer, Color, StyleContext.BlendFmt, *GFxBoundShaderState.PixelShaderInterface);
}

void FGFxRenderer::FGFxRenderStyle::ApplyColorScaleAndColorBias_RenderThread(const void* const BoundShaderState, const void* const RenderStyleContext)
{
	check( GFx_SM_Disabled != StyleMode );
	const FGFxBoundShaderState& GFxBoundShaderState = *static_cast<const FGFxBoundShaderState* const>(BoundShaderState);
	const FGFxRenderStyleContext& StyleContext = *static_cast<const FGFxRenderStyleContext* const>(RenderStyleContext);
	GFxBoundShaderState.PixelShaderInterface->SetParametersColorScaleAndColorBias(GFxBoundShaderState.PixelShaderInterface->GetNativeShader()->GetPixelShader(), this->CxColorMatrix);
}

void FGFxRenderer::FGFxRenderStyle::GetEnumeratedBoundShaderState_RenderThread(void* const OutEnumeratedBoundShaderState, const void* const RenderStyleContext)
{
	check( GFx_SM_Disabled != StyleMode );

	FGFxEnumeratedBoundShaderState& EnumeratedBoundShaderState = *static_cast<FGFxEnumeratedBoundShaderState*>(OutEnumeratedBoundShaderState);
	const FGFxRenderStyleContext& StyleContext = *static_cast<const FGFxRenderStyleContext* const>(RenderStyleContext);

	// Default Vertex Shader and Vertex Declaration
	if (StyleContext.VertexFmt == GRenderer::Vertex_XY16i)
	{
		EnumeratedBoundShaderState.VertexShaderType      = GFx_VS_Strip;
		EnumeratedBoundShaderState.VertexDeclarationType = GFx_VD_Strip;
	}

	// Default Pixel Shader
	EnumeratedBoundShaderState.PixelShaderType = GFx_PS_SolidColor;
}

void FGFxRenderer::FGFxFillStyle::SetStyleBitmap_RenderThread(const FFillTextureInfo& InTexInfo, const GRenderer::Cxform& PersistentCxMatrix)
{
	TexInfo       = InTexInfo;
	CxColorMatrix = PersistentCxMatrix;
	StyleMode     = GFx_SM_Bitmap;
}

void FGFxRenderer::FGFxFillStyle::SetStyleGouraud_RenderThread(
	const GouraudFillType InGouraudFillMode,
	const FFillTextureInfo* const InTexInfo0,
	const FFillTextureInfo* const InTexInfo1,
	const FFillTextureInfo* const InTexInfo2,
	const Cxform& PersistenctCxMatrix)
{
	if (InTexInfo0 || InTexInfo1 || InTexInfo2)
	{
		// Note: more than 2 textures is not currently supported
		if (InTexInfo0 || InTexInfo1 )
		{
			SetStyleBitmap_RenderThread((NULL != InTexInfo0) ? *InTexInfo0 : *InTexInfo1, PersistenctCxMatrix);
		}
		if (InTexInfo1)
		{
			TexInfo2 = *InTexInfo1;
		}
		else
		{
			TexInfo2.Texture = NULL;
		}
	}
	else
	{
		CxColorMatrix = PersistenctCxMatrix;
		TexInfo.Texture = NULL;
		TexInfo2.Texture = NULL;
	}

	StyleMode = GFx_SM_Gouraud;
	GouraudFillMode = InGouraudFillMode;
}

void FGFxRenderer::FGFxFillStyle::GetEnumeratedBoundShaderState_RenderThread(void* const OutEnumeratedBoundShaderState, const void* const RenderStyleContext)
{
	check( GFx_SM_Disabled != StyleMode );

	FGFxEnumeratedBoundShaderState& EnumeratedBoundShaderState = *static_cast<FGFxEnumeratedBoundShaderState*>(OutEnumeratedBoundShaderState);
	const FGFxRenderStyleContext& StyleContext = *static_cast<const FGFxRenderStyleContext* const>(RenderStyleContext);

	// Get default enumerated types
	FGFxRenderStyle::GetEnumeratedBoundShaderState_RenderThread( OutEnumeratedBoundShaderState, RenderStyleContext );

	// Bitmap
	if (GFx_SM_Bitmap == StyleMode)
	{
		// Pixel Shader - using super's vertex shader and decl
		if  ((GRenderer::Blend_Multiply == StyleContext.BlendFmt) ||
			 (GRenderer::Blend_Darken == StyleContext.BlendFmt) )
		{
			EnumeratedBoundShaderState.PixelShaderType = GFx_PS_CxformTextureMultiply;
		}
		else
		{
			EnumeratedBoundShaderState.PixelShaderType = GFx_PS_CxformTexture;
		}
	}

	// Gouraud
	else if (GFx_SM_Gouraud == StyleMode)
	{
		// Vertex Shader
		switch(StyleContext.VertexFmt)
		{
		case GRenderer::Vertex_XY16iC32:
			EnumeratedBoundShaderState.VertexShaderType      = GFx_VS_XY16iC32;
			EnumeratedBoundShaderState.VertexDeclarationType = GFx_VD_XY16iC32;
			break;

		case GRenderer::Vertex_XY16iCF32:
			EnumeratedBoundShaderState.VertexDeclarationType = GFx_VD_XY16iCF32;
			if (GouraudFillMode == GRenderer::GFill_Color)
			{
				EnumeratedBoundShaderState.VertexShaderType = GFx_VS_XY16iCF32_NoTex;
			}
			else if ( GRenderer::GFill_2Texture != GouraudFillMode )
			{
				EnumeratedBoundShaderState.VertexShaderType = GFx_VS_XY16iCF32;
			}
			else
			{
				EnumeratedBoundShaderState.VertexShaderType = GFx_VS_XY16iCF32_T2;
			}
			break;

		default:
			checkMsg( 0, "GetEnumeratedBoundShaderState: Incompatible vertex declaration and shader types" );
			break;
		}

		// Pixel Shader
		EnumeratedBoundShaderState.PixelShaderType = GFx_PS_SolidColor;
		if (NULL == TexInfo.Texture)
		{
			// No texture: generate color-shaded triangles.
			if ( GRenderer::Vertex_XY16iC32 == StyleContext.VertexFmt )
			{
				EnumeratedBoundShaderState.VertexShaderType = GFx_VS_XY16iCF32_NoTexNoAlpha;
				EnumeratedBoundShaderState.PixelShaderType = GFx_PS_CxformGouraudNoAddAlpha;
			}
			else
			{
				EnumeratedBoundShaderState.PixelShaderType = GFx_PS_CxformGouraud;
			}
		}
		else
		{
			if (( GRenderer::GFill_1TextureColor == GouraudFillMode) ||
				( GRenderer::GFill_1Texture == GouraudFillMode) )
			{
				EnumeratedBoundShaderState.PixelShaderType = GFx_PS_CxformGouraudTexture;
			}
			else
			{
				EnumeratedBoundShaderState.PixelShaderType = GFx_PS_Cxform2Texture;
			}
		}

		if ((GRenderer::Blend_Multiply == StyleContext.BlendFmt) ||
			(GRenderer::Blend_Darken == StyleContext.BlendFmt) )
		{ 
			EnumeratedBoundShaderState.PixelShaderType = ( EGFxPixelShaderType )
				( EnumeratedBoundShaderState.PixelShaderType + (GFx_PS_CxformGouraudMultiply - GFx_PS_CxformGouraud) );
			check ( EnumeratedBoundShaderState.PixelShaderType < GFx_PS_Count );
		}
	} // End gouraud mode check
}

void FGFxRenderer::FGFxFillStyle::Apply_RenderThread(FGFxRenderer* Renderer, const void* const BoundShaderState, void* const RenderStyleContext)
{
	check( GFx_SM_Disabled != StyleMode );

	FGFxRenderStyle::Apply_RenderThread( Renderer, BoundShaderState, RenderStyleContext );

	if ( GFx_SM_Color != StyleMode && TexInfo.Texture)
	{
		StaticApplyTextureMatrix_RenderThread( BoundShaderState, RenderStyleContext, TexInfo, 0 );
		StaticApplyTexture_RenderThread( Renderer, BoundShaderState, RenderStyleContext, TexInfo, 0 );

		if (TexInfo2.Texture &&
			(GouraudFillMode == GFill_2TextureColor) ||
			(GouraudFillMode == GFill_2Texture))
		{
			StaticApplyTextureMatrix_RenderThread( BoundShaderState, RenderStyleContext, TexInfo2, 1 );
			StaticApplyTexture_RenderThread( Renderer, BoundShaderState, RenderStyleContext, TexInfo2, 1 );
		}
	}
}

void FGFxRenderer::FGFxFillStyle::StaticApplyTextureMatrix_RenderThread( 
	const void* const BoundShaderState, 
	const void* const RenderStyleContext, 
	const FFillTextureInfo& TexInfo, 
	INT i )
{
	const FGFxBoundShaderState& GFxBoundShaderState = *static_cast<const FGFxBoundShaderState* const>(BoundShaderState);

	if (TexInfo.Texture)    
	{
		FMatrix NativeTextureMatrix( FMatrix::Identity );
		FGFxRendererImpl::ConvertFromUI(TexInfo.TextureMatrix, NativeTextureMatrix);

		GFxBoundShaderState.VertexShaderInterface->SetParameterTextureMatrix(GFxBoundShaderState.VertexShaderInterface->GetNativeShader()->GetVertexShader(), NativeTextureMatrix, i );
	}
}

void FGFxRenderer::FGFxFillStyle::StaticApplyTexture_RenderThread(
	FGFxRenderer* Renderer, 
	const void* const BoundShaderState, 
	const void* const RenderStyleContext, 
	const FFillTextureInfo& TexInfo, 
	INT i )
{
	const FGFxBoundShaderState& GFxBoundShaderState = *static_cast<const FGFxBoundShaderState* const>( BoundShaderState );

	if (TexInfo.Texture)
	{
		GFxBoundShaderState.PixelShaderInterface->SetParameterTexture(
			GFxBoundShaderState.PixelShaderInterface->GetNativeShader()->GetPixelShader(),
			Renderer->GetSamplerState(TexInfo.SampleMode, TexInfo.WrapMode, TexInfo.bUseMips), TexInfo.Texture, i);
	}
}


//---------------------------------------------------------------------------------------------------------------------
// FGFxRenderer
//---------------------------------------------------------------------------------------------------------------------

FGFxRenderer::FGFxRenderer()
:     Viewport(NULL),
	  RenderTarget(NULL),
	  VertexStore(NULL), 
	  IndexStore(NULL),
	  BlendMode(Blend_None), 
	  BlendModeStack(16),
	  MaxTempRTSize(1024),
	  StencilCounter(0)
{
	SamplerStates[0] = SamplerStates[1] = SamplerStates[2] = SamplerStates[3] = 0;
    SamplerStates[4] = SamplerStates[5] = SamplerStates[6] = SamplerStates[7] = 0;
}

void FGFxRenderer::ReleaseResources()
{
	while (ElementStoreList.pFirst != &ElementStoreList)
	{
		FGFxRendererImpl::FGFxRenderElementStoreBase* Store = (FGFxRendererImpl::FGFxRenderElementStoreBase*)ElementStoreList.pFirst;
		Store->CachedData->ReleaseDataByRenderer();
		// This removes the node.
		Store->Release_MainThread(&ElementAccessLock);
	}

	// TBD: We need top know the semantics of when renderer is destroyed
	// in relation to the render thread to clean this up properly.

	// Is it safe to delete vertex and index stores here? - does it matter?

	if (IsInGameThread())
	{
		ReleaseTempRenderTargets(0);
		FlushRenderingCommands(); // FRenderResource links
	}
	else
		ReleaseTempRenderTargets_RenderThread(0);
}

FGFxRenderer::~FGFxRenderer()
{
	ReleaseResources();
}

FSamplerStateRHIRef FGFxRenderer::GetSamplerState(BitmapSampleMode SampleMode, BitmapWrapMode WrapMode, UBOOL bUseMips)
{
	DWORD SamplerStateIndex = SampleMode | (WrapMode<<1) | (bUseMips ? (1<<2) : 0);
	if (SamplerStates[SamplerStateIndex])
	{
		return SamplerStates[SamplerStateIndex];
	}

	FSamplerStateInitializerRHI SamplerInitializer((SampleMode == GRenderer::Sample_Linear) ? SF_Trilinear : SF_Point);
	SamplerInitializer.AddressU = (WrapMode == GRenderer::Wrap_Clamp) ? AM_Clamp : AM_Wrap;
	SamplerInitializer.AddressV = SamplerInitializer.AddressU;
	SamplerInitializer.AddressW = SamplerInitializer.AddressU;
	SamplerInitializer.MipBias = bUseMips ? MIPBIAS_None : MIPBIAS_HigherResolution_13;

	FSamplerStateRHIRef NewSamplerState = RHICreateSamplerState( SamplerInitializer );
	check( IsValidRef( NewSamplerState ) );
	SamplerStates[SamplerStateIndex] = NewSamplerState;

	return NewSamplerState;
}

FLinearColor FGFxRenderer::CorrectGamma(FLinearColor Color) const
{
	return FLinearColor(powf(Color.R, InverseGamma), powf(Color.G, InverseGamma), powf(Color.B, InverseGamma), Color.A);
}

void FGFxRenderer::Render(GFxMovieView& Movie)
{
	Movie.Display();
}

bool FGFxRenderer::GetRenderCaps(RenderCaps* Caps)
{   
	Caps->CapBits       = Cap_Index16 | Cap_CxformAdd;
	Caps->BlendModes    = (1<<Blend_None) | (1<<Blend_Normal) |
						   (1<<Blend_Multiply) | (1<<Blend_Lighten) | (1<<Blend_Darken) |
						   (1<<Blend_Add) | (1<<Blend_Subtract);
	Caps->VertexFormats = (1<<Vertex_None) | (1<<Vertex_XY16i);
	if (GVertexElementTypeSupport.IsSupported(VET_UByte4N))
	{
		Caps->VertexFormats |= (1<<Vertex_XY16iC32) | (1<<Vertex_XY16iCF32);
		Caps->CapBits |= Cap_FillGouraud | Cap_FillGouraudTex;
	}

	// Add stencil masking support
	Caps->CapBits |= Cap_NestedMasks;

	// Filters
#if XBOX
	Caps->CapBits |= Cap_RenderTargetPrePass | Cap_Filter_ColorMatrix | Cap_Filter_Blurs;
#else
	Caps->CapBits |= Cap_RenderTargets | Cap_Filter_ColorMatrix | Cap_Filter_Blurs;
#endif

	Caps->MaxTextureSize = 1 << GMaxTextureMipCount;

	return TRUE;
}

FGFxTexture* FGFxRenderer::CreateTexture()
{
	return new FGFxTexture( this );
}

FGFxRenderTarget* FGFxRenderer::CreateRenderTarget()
{
	return new FGFxRenderTarget( this );
}

#if (GFC_FX_MAJOR_VERSION >= 3 && GFC_FX_MINOR_VERSION >= 2) && !defined(GFC_NO_3D)
void FGFxRenderer::SetPerspective3D(const GMatrix3D& ProjMatIn)
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	  (SetPerspective3DCommand,
      FGFxRenderer*, Renderer, this,
      GMatrix3D, InMatrix, ProjMatIn,
    {
        Renderer->ProjMatrix = InMatrix;
        Renderer->UVPMatricesChanged = 1;
    });
}

void FGFxRenderer::SetView3D(const GMatrix3D& MatIn)
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	  (SetView3DCommand,
	   FGFxRenderer*, Renderer, this,
	   GMatrix3D, InMatrix, MatIn,
	   {
		   Renderer->ViewMatrix = InMatrix;
           Renderer->UVPMatricesChanged = 1;
	   });
}

void FGFxRenderer::SetWorld3D(const GMatrix3D* MatIn)
{
	if (MatIn)
	{
		GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER
			(SetWorld3DCommand,
			GMatrix3D*, OutMatrix, &WorldMatrix,
			UBOOL*, pIs3DEnabled, &Is3DEnabled,
			GMatrix3D, InMatrix, *MatIn,
			{
				*OutMatrix = InMatrix;
				*pIs3DEnabled = TRUE;
			});
	}
	else
	{
		GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
			(SetNullWorld3DCommand,
			UBOOL*, pIs3DEnabled, &Is3DEnabled,
			{
				*pIs3DEnabled = FALSE;
			});
	}
}
#endif

void FGFxRenderer::SetDisplayRenderTarget_RenderThread(GRenderTarget* InRT, bool bsetstate)
{
	FGFxRenderTarget* RT = (FGFxRenderTarget*)InRT;

	GASSERT(RenderTargetStack.GetSize() == 0);
	CurRenderTarget = RT;
	if (bsetstate)
		RHISetRenderTarget(RT->Resource.ColorBuffer, RT->Resource.DepthBuffer);
}

void FGFxRenderer::SetDisplayRenderTarget(GRenderTarget* prt, bool setstate)
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER
		(SetRenderTarget,
		FGFxRenderer*, Renderer, this,
		GPtr<FGFxRenderTarget>, pRT, (FGFxRenderTarget*)prt,
		bool, bsetstate, setstate,
	{
		Renderer->SetDisplayRenderTarget_RenderThread(pRT, bsetstate);
	});
}

void FGFxRenderer::PushRenderTarget(const GRectF& FrameRect, GRenderTarget* InRT)
{
	FGFxRenderTarget* RT = (FGFxRenderTarget*)InRT;
	if (RT->Texture)
		RT->Texture->AddRef();

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER
		(PushRenderTargetCommand,
		FGFxRenderer*, Renderer, this,
		GPtr<FGFxRenderTarget>, pRT, RT,
		GRectF, rect, FrameRect,
	{
		Renderer->PushRenderTarget_RenderThread(rect, pRT);
	});
}

void FGFxRenderer::PushRenderTarget_RenderThread(const GRectF& FrameRect, GRenderTarget* InRT)
{
	FGFxRenderTarget* RT = (FGFxRenderTarget*)InRT;
	if (RT->Texture)
		RT->Texture->AddRef();

	RenderTargetStack.PushBack(RTState(CurRenderTarget,ViewportMatrix, 
#ifndef GFC_NO_3D
		ViewMatrix, ProjMatrix, WorldMatrix, Is3DEnabled,
#else
		GMatrix3D(), GMatrix3D(), GMatrix3D(), FALSE,
#endif
		ViewRect,bAlphaComposite, CurStencilState));
	CurRenderTarget = RT;
	CurStencilState = TStaticStencilState< FALSE /*bEnableFrontFaceStencil*/ >::GetRHI();

	float   dx = FrameRect.Width(); 
	float   dy = FrameRect.Height();
	if (dx < 1) { dx = 1; }
	if (dy < 1) { dy = 1; }

    check(CurRenderTarget->TargetWidth <= UInt(CurRenderTarget->Texture->RenderTarget->Resource.SizeX));
    check(CurRenderTarget->TargetHeight <= UInt(CurRenderTarget->Texture->RenderTarget->Resource.SizeY));

	//*** Viewport
	FGFxTransformHandles TransformHandles( UserMatrix, ViewportMatrix,CurrentMatrix );

	appMemzero( &ViewRect, sizeof( FGFxViewportUserParams ) );
	// x-axis
	ViewRect.xAxis.PixelStart    =
	ViewRect.xAxis.PixelClipStart= FrameRect.Left;
	ViewRect.xAxis.PixelEnd      = FrameRect.Right;
	ViewRect.xAxis.PixelClipLength = dx;
	ViewRect.xAxis.ViewStart     = 0;
	ViewRect.xAxis.ViewLength    = CurRenderTarget->TargetWidth;
	ViewRect.xAxis.ViewLengthMax = CurRenderTarget->Texture->RenderTarget->Resource.SizeX;
	// y-axis
	ViewRect.yAxis.PixelStart    =
	ViewRect.yAxis.PixelClipStart= FrameRect.Top;
	ViewRect.yAxis.PixelEnd      = FrameRect.Bottom;
	ViewRect.yAxis.PixelClipLength = dy;
	ViewRect.yAxis.ViewStart     = 0;
	ViewRect.yAxis.ViewLength    = CurRenderTarget->TargetHeight;
	ViewRect.yAxis.ViewLengthMax = CurRenderTarget->Texture->RenderTarget->Resource.SizeY;

	FGFxRendererImpl::MakeViewportMatrix(ViewportMatrix, ViewRect);

#ifndef GFC_NO_3D
	// set 3D view
	GMatrix3D matView, matPersp;
	MakeViewAndPersp3D(FrameRect, matView, matPersp, true /*invert Y */);
	ViewMatrix = matView;
	ProjMatrix = matPersp;
	//WorldMatrix.SetIdentity();
	Is3DEnabled = FALSE;
#endif

    UVPMatricesChanged = 1;
	bAlphaComposite = 1;
	CurRenderTargetSet = 0;
}

void FGFxRenderer::PopRenderTarget()
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
		(PushRenderTargetCommand,
		FGFxRenderer*, Renderer, this,
	{
		Renderer->PopRenderTarget_RenderThread();
	});
}

void        FGFxRenderer::PopRenderTarget_RenderThread()
{
	RHICopyToResolveTarget(CurRenderTarget->Resource.ColorBuffer, TRUE, FResolveParams());

	if (CurRenderTarget->IsTemp)
		CurRenderTarget->StencilBuffer = 0;

	if (CurRenderTarget->Texture)
	{
		// 1 from main thread, 1 from render thread
		CurRenderTarget->Texture->Release();
		CurRenderTarget->Texture->Release();
	}

	RTState rts = RenderTargetStack.Back();
	RenderTargetStack.PopBack();
	CurRenderTarget = rts.pRT;
	ViewportMatrix = rts.ViewMatrix;
	ViewRect = rts.ViewRect;
	bAlphaComposite = rts.RenderMode ? 1 : 0;
	CurStencilState = rts.StencilState;

#ifndef GFC_NO_3D
	// restore 3D view matrix
	ViewMatrix = rts.ViewMatrix3D;
	ProjMatrix = rts.PerspMatrix3D;
	WorldMatrix = rts.WorldMatrix3D;
	Is3DEnabled = rts.Is3DEnabled;
#endif

	CurRenderTargetSet = 0;
    UVPMatricesChanged = 1;
}

void        FGFxRenderer::CheckRenderTarget_RenderThread()
{
	if (CurRenderTargetSet)
		return;

	RHISetRenderTarget(CurRenderTarget->Resource.ColorBuffer, 
		CurRenderTarget->StencilBuffer ? CurRenderTarget->StencilBuffer->DepthSurface : CurRenderTarget->Resource.DepthBuffer);
	RHISetStencilState(CurStencilState);

	if (CurRenderTarget->Texture)
	{
		RHISetViewport(0,0,0, CurRenderTarget->Texture->RenderTarget->Resource.SizeX, CurRenderTarget->Texture->RenderTarget->Resource.SizeY, 1);
		RHIClear(TRUE, FLinearColor(0,0,0,0), FALSE, 0, CurRenderTarget->StencilBuffer ? TRUE : FALSE, 0);
	}

    if (CurRenderTarget->Texture)
    {
        check(CurRenderTarget->TargetWidth <= UInt(CurRenderTarget->Texture->RenderTarget->Resource.SizeX));
        check(CurRenderTarget->TargetHeight <= UInt(CurRenderTarget->Texture->RenderTarget->Resource.SizeY));
    }
	RHISetViewport( 
		ViewRect.xAxis.ViewStart,
		ViewRect.yAxis.ViewStart,
		0,
		ViewRect.xAxis.ViewStart + ViewRect.xAxis.ViewLength,
		ViewRect.yAxis.ViewStart + ViewRect.yAxis.ViewLength,
		0);

	FGFxRendererImpl::ApplyUIBlendMode_RenderThread(bAlphaComposite, BlendMode );

	// Set gamma for normal (non-filter) shaders. Temp render targets don't use gamma because it would be applied twice.
	FGFxPixelShaderInterface* pShaderInterface = GetUIPixelShaderInterface_RenderThread(GFx_PS_TextTexture);
	pShaderInterface->SetParameterInverseGamma(pShaderInterface->GetNativeShader()->GetPixelShader(), /*CurRenderTarget->Resource.InverseGamma*/1);

	CurRenderTargetSet = 1;
}

FGFxTexture*  FGFxRenderer::PushTempRenderTarget(const GRectF& FrameRect, UInt inw, UInt inh, bool wantStencil)
{
	FGFxTexture* StubTexture = CreateTexture();

	struct Params
	{
		GRectF FrameRect;
		UInt   Width, Height;
		bool   WantStencil;
	} Params;
	Params.FrameRect = FrameRect;
	Params.Width = inw;
	Params.Height = inh;
	Params.WantStencil = wantStencil;

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER
		(PushRenderTargetCommand,
		FGFxRenderer*, Renderer, this,
		FGFxTexture*, Texture, StubTexture,
		struct Params, Params, Params,
	{
		Renderer->PushTempRenderTarget_RenderThread(Texture, Params.FrameRect, Params.Width, Params.Height, Params.WantStencil);
	});

	return StubTexture;
}

FGFxTexture*  FGFxRenderer::PushTempRenderTarget_RenderThread(FGFxTexture* StubTexture, const GRectF& FrameRect, UInt inw, UInt inh, bool wantStencil)
{
	static const UInt TempRTSizes[] =
	{
#if XBOX
		64,
#endif
		128, 256, 384, 512, 784, 1024, 1344, 1664, 2048, 2560, 3072
	};
	static const UInt MaxTempRTSize = 1024;

	FGFxRenderTarget* pRT = 0;
	UInt w = TempRTSizes[0], h = TempRTSizes[0];
	bool needsinit = false, needsfixstub = true;

	if (inw > MaxTempRTSize || inh > MaxTempRTSize)
	{
		if (inw > inh)
		{
			inh = (UInt)ceilf(MaxTempRTSize*inh/Float(inw));
			inw = MaxTempRTSize;
		}
		else
		{
			inw = (UInt)ceilf(MaxTempRTSize*inw/Float(inh));
			inh = MaxTempRTSize;
		}
	}

	for (UInt i = 0; i < TempRenderTargets.GetSize(); i++)
		if (TempRenderTargets[i]->Texture.GetPtr() == NULL ||
			(TempRenderTargets[i]->Texture->GetRefCount() <= 1 &&
			 TempRenderTargets[i]->Texture->RenderTarget->Resource.SizeX >= SInt(inw) &&
			 TempRenderTargets[i]->Texture->RenderTarget->Resource.SizeY >= SInt(inh)))
		{
            w = inw;
            h = inh;
			pRT = TempRenderTargets[i];
			goto dostencil;
		}

		{
			UInt iw = 0, ih = 0;
			while (w < inw) w = TempRTSizes[iw++];
			while (h < inh) h = TempRTSizes[ih++];
		}

	for (UInt i = 0; i < TempRenderTargets.GetSize(); i++)
		if (TempRenderTargets[i]->Texture->GetRefCount() <= 1)
		{
			pRT = TempRenderTargets[i];
			needsinit = true;
			goto dostencil;
		}

	{
		FGFxTexture* prtt = StubTexture ? StubTexture : CreateTexture();
		pRT = CreateRenderTarget();
		pRT->IsTemp = 1;
		pRT->Texture = prtt; // will be initialized later
		TempRenderTargets.PushBack(*pRT);
		//prtt->Release();
		needsinit = 1;
		needsfixstub = 0;
	}

dostencil:
	FGFxRenderResources* pDepthStencil = 0;

#if !XBOX
	if (pRT->Texture && pRT->Texture->RenderTarget)
	{
		if (w < UInt(pRT->Texture->RenderTarget->Resource.SizeX))
			w = pRT->Texture->RenderTarget->Resource.SizeX;
		if (h < UInt(pRT->Texture->RenderTarget->Resource.SizeY))
			h = pRT->Texture->RenderTarget->Resource.SizeY;
	}

	GASSERT(!pRT->StencilBuffer);
	if (wantStencil && (pRT->StencilBuffer.GetPtr() == 0 || pRT->StencilBuffer->GetRefCount() > 1))
	{
		for (UInt i = 0; i < TempStencilBuffers.GetSize(); i++)
		{
			if (TempStencilBuffers[i]->GetRefCount() <= 1 &&
				TempStencilBuffers[i]->SizeX >= SInt(inw) &&
				TempStencilBuffers[i]->SizeY >= SInt(inh))
			{
				pDepthStencil = TempStencilBuffers[i];
				break;
			}
		}
		if (!pDepthStencil)
		{
			for (UInt i = 0; i < TempStencilBuffers.GetSize(); i++)
				if (TempStencilBuffers[i]->GetRefCount() <= 1)
				{
					pDepthStencil = TempStencilBuffers[i];
					pDepthStencil->Allocate_RenderThread(w, h);
					break;
				}

			if (!pDepthStencil)
			{
				pDepthStencil = new FGFxRenderResources();
				pDepthStencil->Allocate_RenderThread(w, h);
				TempStencilBuffers.PushBack(*pDepthStencil);
			}
		}

		if (!needsinit)
		{
			pRT->StencilBuffer = pDepthStencil;
		}
	}
#endif

	if (needsinit)
	{
		pRT->InitRenderTarget_RenderThread(pRT->Texture, pDepthStencil, w, h);
		pRT->Resource.InverseGamma = 1.0f;
		pRT->Texture->RenderTarget = pRT;
	}

	if (needsfixstub)
	{
		if (StubTexture)
		{
			if (pRT->Texture)
				pRT->Texture->RenderTarget = NULL;
			pRT->Texture = StubTexture;
			StubTexture->RenderTarget = pRT;
		}
		else if (!pRT->Texture)
		{
			pRT->Texture = CreateTexture();
			pRT->Texture->RenderTarget = pRT;
		}
		else
			pRT->Texture->AddRef(); // match PushRenderTarget from game thread
	}

	pRT->TargetWidth = inw;
	pRT->TargetHeight = inh;
	PushRenderTarget_RenderThread(FrameRect, pRT);
	return pRT->Texture;		
}

void FGFxRenderer::ReleaseTempRenderTargets(UInt keepArea)
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
		(ReleaseTempRenderTargetCommand,
		FGFxRenderer*, Renderer, this,
		UInt, KeepArea, keepArea,
	{
		Renderer->ReleaseTempRenderTargets_RenderThread(KeepArea);
	});
}

void FGFxRenderer::ReleaseTempRenderTargets_RenderThread(UInt keepArea)
{
	GArray<GPtr<FGFxRenderTarget> >    NewTempRenderTargets;
	GArray<GPtr<FGFxRenderResources> > NewTempStencilBuffers;
	UInt stencilArea = keepArea;

	for (UInt i = 0; i < TempRenderTargets.GetSize(); i++)
		if (TempRenderTargets[i]->Texture->GetRefCount() > 1 || (TempRenderTargets[i]->Texture->Texture2D &&
			UInt(TempRenderTargets[i]->Texture->RenderTarget->Resource.SizeX * TempRenderTargets[i]->Texture->RenderTarget->Resource.SizeY) <= keepArea))
		{
			NewTempRenderTargets.PushBack(TempRenderTargets[i]);
            if (UInt(TempRenderTargets[i]->Texture->RenderTarget->Resource.SizeX * TempRenderTargets[i]->Texture->RenderTarget->Resource.SizeY) <= keepArea)
                keepArea -= TempRenderTargets[i]->Texture->RenderTarget->Resource.SizeX * TempRenderTargets[i]->Texture->RenderTarget->Resource.SizeY;
		}

	TempRenderTargets.Clear();
	TempRenderTargets.Append(NewTempRenderTargets.GetDataPtr(), NewTempRenderTargets.GetSize());

	for (UInt i = 0; i < TempStencilBuffers.GetSize(); i++)
		if (TempStencilBuffers[i]->GetRefCount() > 1 ||
			UInt(TempStencilBuffers[i]->SizeX * TempStencilBuffers[i]->SizeY) <= stencilArea)
		{
			NewTempStencilBuffers.PushBack(TempStencilBuffers[i]);
			stencilArea -= TempStencilBuffers[i]->SizeX * TempStencilBuffers[i]->SizeY;
		}

	TempStencilBuffers.Clear();
	TempStencilBuffers.Append(NewTempStencilBuffers.GetDataPtr(), NewTempStencilBuffers.GetSize());
}

void FGFxRenderer::SetUIViewport( FGFxViewportUserParams& InViewportParams)
{
	// Clip x-axis
	FGFxRendererImpl::ClipUIViewportAxis(InViewportParams.xAxis);

	// Clip y-axis
	FGFxRendererImpl::ClipUIViewportAxis(InViewportParams.yAxis);

	GRenderer::Matrix NewViewportMatrix;
	FGFxRendererImpl::MakeViewportMatrix(NewViewportMatrix, InViewportParams);

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER( 
		SetUIViewportCommand,
		FGFxViewportUserParams, ViewportParams, InViewportParams,
		GRenderer::Matrix, NewViewMatrix, NewViewportMatrix,
		FGFxRenderer*, Renderer, this,
	{ 
		Renderer->ViewRect = ViewportParams;

		RHISetViewport( 
			ViewportParams.xAxis.ViewStart,
			ViewportParams.yAxis.ViewStart,
			0,
			ViewportParams.xAxis.ViewStart + ViewportParams.xAxis.ViewLength,
			ViewportParams.yAxis.ViewStart + ViewportParams.yAxis.ViewLength,
			0);
		Renderer->ViewportMatrix = NewViewMatrix;
		Renderer->ViewportMatrix *= Renderer->UserMatrix;
	}
	);
}

void FGFxRenderer::InitUIBlendStackAndMiscRenderState_RenderingThread(FMiscRenderStateInitParams& Params)
{
	Params.Renderer->CheckRenderTarget_RenderThread();

	//*** Blending
	Params.BlendModeStack->Empty(16);
	BlendType& BlendMode = *Params.BlendMode;
	BlendMode = Blend_None;
	*Params.bAlphaComposite = (Params.ViewFlags & GViewport::View_AlphaComposite) != 0;

	FGFxRendererImpl::ApplyUIBlendMode_RenderThread( *Params.bAlphaComposite, BlendMode );

	//*** Disable depth testing and back-face culling
	RHISetDepthState( TStaticDepthState<0/*bEnableDepthWrite*/, CF_Always/*DepthTest*/>::GetRHI() );
	RHISetRasterizerState( TStaticRasterizerState<FM_Solid/*FillMode*/, CM_None/*CullMode*/>::GetRHI() );
	RHISetStencilState( TStaticStencilState<FALSE/*bEnableFrontFaceStencil*/>::GetRHI() );

	//*** Masking
	*Params.StencilCounter = 0;

	Params.Renderer->Is3DEnabled = 0;

	INT Gamma;
	if (Params.ViewFlags & GViewport_NoGamma)
	{
		Gamma = 0;
	}
	else if (Params.ViewFlags & GViewport_InverseGamma)
	{
		Gamma = -1;
	}
	else
	{
		Gamma = 1;
	}
	if (Params.Renderer->RenderTarget == NULL)
	{
		Gamma--;
	}

	{
		const FLOAT DisplayGamma = Params.Renderer->RenderTarget ? Params.Renderer->RenderTarget->GetDisplayGamma() :
			((GEngine && GEngine->Client) ? GEngine->Client->DisplayGamma : 2.2f);
		if (Gamma < 0)
		{
			if (DisplayGamma > 0)
			{
				*Params.InverseGamma = DisplayGamma;
			}
			else
			{
				*Params.InverseGamma = 2.2f;
			}
		}
		else if (Gamma > 0)
		{
			if (DisplayGamma > 0)
			{
				*Params.InverseGamma = 1.0f / DisplayGamma;
			}
			else
			{
				*Params.InverseGamma = 1.0f / 2.2f;
			}
		}
		else
		{
			*Params.InverseGamma = 1.0f;
		}
	}

	FGFxPixelShaderInterface* ShaderInterface = GetUIPixelShaderInterface_RenderThread(GFx_PS_TextTexture);
	ShaderInterface->SetParameterInverseGamma(ShaderInterface->GetNativeShader()->GetPixelShader(), *Params.InverseGamma);
	Params.Renderer->CurRenderTarget->Resource.InverseGamma = *Params.InverseGamma;
}

// Set up to render a full frame from a movie and fills the
// background.  Sets up necessary transforms, to scale the
// movie to fit within the given dimensions.  Call
// EndDisplay() when you're done.
//
// The Rectangle (viewportX0, viewportY0, viewportX0 +
// viewportWidth, viewportY0 + viewportHeight) defines the
// window coordinates taken up by the movie.
//
// The Rectangle (x0, y0, x1, y1) defines the pixel
// coordinates of the movie that correspond to the Viewport
// bounds.
void FGFxRenderer::BeginDisplay(GColor BackgroundColor, const GViewport& Viewport, Float x0, Float x1, Float y0, Float y1)
{
	//*** Viewport
	FGFxTransformHandles TransformHandles( UserMatrix, ViewportMatrix, CurrentMatrix );
	FGFxViewportUserParams ViewportParams;
	appMemzero( &ViewportParams, sizeof( FGFxViewportUserParams ) );
	// x-axis
	ViewportParams.xAxis.PixelStart    = x0;
	ViewportParams.xAxis.PixelEnd      = x1;
	ViewportParams.xAxis.ViewStart     = Viewport.Left;
	ViewportParams.xAxis.ViewLength    = Viewport.Width;
	ViewportParams.xAxis.ViewLengthMax = Viewport.BufferWidth;;
	// y-axis
	ViewportParams.yAxis.PixelStart    = y0;
	ViewportParams.yAxis.PixelEnd      = y1;
	ViewportParams.yAxis.ViewStart     = Viewport.Top;
	ViewportParams.yAxis.ViewLength    = Viewport.Height;
	ViewportParams.yAxis.ViewLengthMax = Viewport.BufferHeight;
    
	RenderMode = (Viewport.Flags & GViewport::View_AlphaComposite) != 0;
    
	SetUIViewport( ViewportParams );

	FMiscRenderStateInitParams InitParams = {this,&BlendMode, &BlendModeStack, &StencilCounter, &InverseGamma, &bAlphaComposite, Viewport.Flags};
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		InitUIBlendStackAndMiscRenderStateCommand,
		FMiscRenderStateInitParams, Params, InitParams,
		FGFxRenderer*, Renderer, this,
	{
		Renderer->CurStencilState = TStaticStencilState<FALSE/*bEnableFrontFaceStencil*/>::GetRHI();
		Renderer->CheckRenderTarget_RenderThread();
		Renderer->InitUIBlendStackAndMiscRenderState_RenderingThread(Params);
	}
	)

	//*** Render background color quad
	if ( BackgroundColor.GetAlpha() > 0 )
	{
		SetMatrix( Matrix::Identity );

		struct FDrawUIBackGroundColorParams
		{
			FGFxRenderer* Renderer;
			const GColor BackgroundColor;
			const FGFxTransformHandles TransformHandles;
			FGFxBoundShaderStateCache& BoundShaderStateCache;
			TArray< FGFxVertex_XY16i > BgQuad;
			FDrawUIBackGroundColorParams( 
				  FGFxRenderer* Renderer,
				  const GColor& InBackgroundColor,
				  const FGFxTransformHandles& InTransformHandles,
				  FGFxBoundShaderStateCache& InBoundShaderStateCache )
				: Renderer(Renderer), BackgroundColor(InBackgroundColor), TransformHandles(InTransformHandles),
				  BoundShaderStateCache(InBoundShaderStateCache), BgQuad(4) { } 
		};

		FDrawUIBackGroundColorParams DrawBgParams(this,BackgroundColor, TransformHandles, BoundShaderStateCache);
		// Should these be the clipped pixel coords?
		DrawBgParams.BgQuad( 0 ).X = (SInt16)x0; DrawBgParams.BgQuad( 0 ).Y = (SInt16)y0;
		DrawBgParams.BgQuad( 1 ).X = (SInt16)x1; DrawBgParams.BgQuad( 1 ).Y = (SInt16)y0;
		DrawBgParams.BgQuad( 2 ).X = (SInt16)x0; DrawBgParams.BgQuad( 2 ).Y = (SInt16)y1;
		DrawBgParams.BgQuad( 3 ).X = (SInt16)x1; DrawBgParams.BgQuad( 3 ).Y = (SInt16)y1;

		GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
		(
			DrawUIBackgroundColorCommand,
			FDrawUIBackGroundColorParams, DrawBg, DrawBgParams,
		{
			DrawUIBackgroundColor_RenderThread(
				  DrawBg.Renderer,
				  DrawBg.BackgroundColor,
				  DrawBg.BgQuad,
				  DrawBg.TransformHandles, 
				  DrawBg.BoundShaderStateCache );
		}	
		)
	}
}

void FGFxRenderer::EndDisplay()
{
	// TODO:  Restore any necessary state, end scene
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		EndDisplayCommand,
		FGFxRenderer* const, Renderer, this,
	{
		Renderer->EndDisplay_RenderThread();
	}
	)
}

void FGFxRenderer::EndDisplay_RenderThread()
{
	FGFxRendererImpl::DestroyUIRenderElementStore_RenderThread(GetElementsLock(), VertexStore);
	FGFxRendererImpl::DestroyUIRenderElementStore_RenderThread(GetElementsLock(), IndexStore);
	LineStyle.EndDisplay_RenderThread();
	FillStyle.EndDisplay_RenderThread();
	checkMsg( (StencilCounter == 0), "FGFxRenderer::EndDisplay - BeginSubmitMask/DisableSubmitMask mismatch" );
}

void FGFxRenderer::SetMatrix(const Matrix& M)
{
	SetUITransformMatrix( M, CurrentMatrix );
}

void FGFxRenderer::SetUserMatrix(const Matrix& M)
{
	SetUITransformMatrix( M, UserMatrix );
}

void FGFxRenderer::SetCxform(const Cxform& Cx)
{
	SetUITransformMatrix( Cx, CurrentCxform );
}

void FGFxRenderer::PushBlendMode(BlendType Mode)
{
	// The following note is from Scaleform GFx DX9 implementation (which this implementation mimics)

	// Blend modes need to be applied cumulatively, ideally through an extra RT texture.
	// If the nested clip has a "Multiply" effect, and its parent has an "Add", the result
	// of Multiply should be written to a buffer, and then used in Add.

	// For now we only simulate it on one Level -> we apply the last effect on top of stack.
	// (Note that the current "top" element is BlendMode, it is not actually stored on stack).
	// Although incorrect, this will at least ensure that parent's specified effect
	// will be applied to children.

	struct FBlendModeParams
	{
		BlendType*          BlendMode;
		TArray<BlendType>*  BlendModeStack;
		UBOOL*              bAlphaComposite;
		BlendType           NewBlendMode;
	};
	FBlendModeParams InitParams = {&BlendMode, &BlendModeStack, &bAlphaComposite, Mode};
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		PushUIBlendModeCommand,
		FBlendModeParams, Params, InitParams,
	{
		BlendType& BlendMode = *Params.BlendMode;
		TArray<BlendType>& BlendModeStack = *Params.BlendModeStack;
		BlendModeStack.Push(BlendMode);
		if( (Params.NewBlendMode > Blend_Layer) && (BlendMode != Params.NewBlendMode))
		{
			BlendMode = Params.NewBlendMode;
			FGFxRendererImpl::ApplyUIBlendMode_RenderThread( *Params.bAlphaComposite, BlendMode );
		}
	}
	)
}

void FGFxRenderer::PopBlendMode()
{
	struct FBlendModeParams
	{
		BlendType*          BlendMode;
		TArray<BlendType>*  BlendModeStack;
		UBOOL*              bAlphaComposite;
	};
	FBlendModeParams InitParams = {&BlendMode, &BlendModeStack, &bAlphaComposite};
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		PopUIBlendModeCommand,
		FBlendModeParams, Params, InitParams,
	{
		BlendType& BlendMode = *Params.BlendMode;
		TArray<BlendType>& BlendModeStack = *Params.BlendModeStack;
		checkMsg( BlendModeStack.Num() != 0, "FGFxRenderer::PopBlendMode - blend mode stack is empty" );

		const BlendType NewBlendMode = BlendModeStack.Pop();
		if( NewBlendMode != BlendMode )
		{
			BlendMode = NewBlendMode;
			FGFxRendererImpl::ApplyUIBlendMode_RenderThread( *Params.bAlphaComposite, BlendMode );
		}
	}
	)
}

void FGFxRenderer::SetVertexData(const void* Vertices, int NumVertices, VertexFormat Vf, CacheProvider* Cache)
{
	SetUIRenderElementStore(this, Cached_Vertex, Vertices, NumVertices, Vf, Cache, VertexStore);
}

void FGFxRenderer::SetIndexData(const void* Indices, int NumIndices, IndexFormat Idxf, CacheProvider* Cache)
{
	SetUIRenderElementStore(this, Cached_Index, Indices, NumIndices, Idxf, Cache, IndexStore);	
}

void FGFxRenderer::ReleaseCachedData(CachedData* Data, CachedDataType Type)
{
	GUNUSED(Type);    
	FGFxRendererImpl::FGFxRenderElementStoreBase* Store = (FGFxRendererImpl::FGFxRenderElementStoreBase*)Data->GetRendererData();
	// Release data removing it from list. The node may actually survive 
	// if it is in use by the render thread; in that case buffer us copied
	// and it's render threads responsibility to release it later.
	Store->Release_MainThread(&ElementAccessLock);    
}


void FGFxRenderer::DrawIndexedTriList(int BaseVertexIndex, int MinVertexIndex, int NumVertices, int StartIndex, int TriangleCount)
{
	struct FGFxDrawIndexedTriListParams
	{
		INT BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, TriangleCount;
	};
    
	const FGFxDrawIndexedTriListParams InDrawParams =
	{
		BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, TriangleCount
	};

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		DrawUIIndexedTriListCommand,
		FGFxRenderer* const, Renderer, this,
		const FGFxDrawIndexedTriListParams, DrawParams, InDrawParams,
	{
		Renderer->DrawIndexedTriList_RenderThread( 
			  DrawParams.BaseVertexIndex,
			  DrawParams.MinVertexIndex,
			  DrawParams.NumVertices,
			  DrawParams.StartIndex,
			  DrawParams.TriangleCount );
	}
	)

#ifdef STATS
	RenderStats.Triangles += TriangleCount;
#endif
}

// Render-thread analogue of DrawIndexedTriList
void FGFxRenderer::DrawIndexedTriList_RenderThread(const INT BaseVertexIndex, const INT MinVertexIndex, const INT NumVertices, const INT StartIndex, const INT TriangleCount)
{
	if (FillStyle.IsDisabled_RenderThread())  
	{
		return;
	}
    
	CheckRenderTarget_RenderThread();

	const FGFxRendererImpl::FGFxVertexStore& LocalVertexStore  = *VertexStore;
	const FGFxRendererImpl::FGFxIndexStore& LocalIndexStore    = *IndexStore;
	checkMsg( LocalVertexStore.Format != Vertex_None, "FGFxRenderer::DrawIndexedTriList failed, vertex data not specified" );
	checkMsg( LocalIndexStore.Format != Index_None, "FGFxRenderer::DrawIndexedTriList failed, index data not specified" );

	// Ask style for enumerated bound shader state.
	FGFxEnumeratedBoundShaderState EnumeratedBoundShaderState;
	FGFxRenderStyleContext         FillContext;
	FillContext.VertexFmt = LocalVertexStore.Format;
	FillContext.BlendFmt  = BlendMode;
	FillStyle.GetEnumeratedBoundShaderState_RenderThread( &EnumeratedBoundShaderState, &FillContext );

	// Obtain actual bound shader state.
	FGFxBoundShaderState BoundShaderState;
	appMemzero( &BoundShaderState, sizeof(FGFxBoundShaderState) );
	FGFxRendererImpl::GetUIBoundShaderState_RenderThread( BoundShaderState, BoundShaderStateCache, EnumeratedBoundShaderState );
	check( FGFxRendererImpl::IsValidUIBoundShaderState( BoundShaderState ) );
	FGFxVertexShaderInterface& VertexShader = *BoundShaderState.VertexShaderInterface;
	FGFxPixelShaderInterface& PixelShader = *BoundShaderState.PixelShaderInterface;

	// Set up vertex transform.
	ApplyUITransform_RenderThread( ViewportMatrix, CurrentMatrix, VertexShader );

	// Apply style
	FillStyle.Apply_RenderThread( this, &BoundShaderState, &FillContext );
    
	// Apply blend mode - was part of fill style
	FGFxRendererImpl::ApplyUIBlendMode_RenderThread( bAlphaComposite, BlendMode );

	PixelShader.SetParameterInverseGamma(PixelShader.GetNativeShader()->GetPixelShader(), CurRenderTarget->Resource.InverseGamma);

	// Apply bound shader state.
	RHISetBoundShaderState(BoundShaderState.NativeBoundShaderState);    

	// Draw indexed primitives (immediate mode for now)
	const size_t VertexElementSize = LocalVertexStore.ElementSize;
	check( VertexElementSize > 0 );

	// Lock to prevent ReleaseCachedData from modifying our pointer.
	GLock::Locker Guard(GetElementsLock());

	const void* const VertexElements = ( ((char*) LocalVertexStore.Elements) + ( VertexElementSize * BaseVertexIndex ) );
	check( VertexElements >= LocalVertexStore.Elements );
	check( VertexElements < ( ((char*) LocalVertexStore.Elements) + ( VertexElementSize * LocalVertexStore.NumElements ) ) );

	const void* const IndexElements = ( ((char*) LocalIndexStore.Elements) + ( LocalIndexStore.ElementSize * StartIndex ) );
	check( IndexElements >= LocalIndexStore.Elements );
	check( IndexElements < ( ((char*) LocalIndexStore.Elements) + ( LocalIndexStore.ElementSize * LocalIndexStore.NumElements ) ) );

	RHIDrawIndexedPrimitiveUP( 
		PT_TriangleList,
		MinVertexIndex,
		( LocalVertexStore.NumElements - BaseVertexIndex ),
		TriangleCount,
		IndexElements,
		GetUIRenderElementSize( LocalIndexStore.Format ),
		VertexElements,
		VertexElementSize );
}

/**
* Draws a line-strip using the current line style. 
* Vertex coordinate data for the line strip is taken from the buffer specified by SetVertexData.
* Indexing is not used.
*/
void FGFxRenderer::DrawLineStrip(int BaseVertexIndex, int LineCount)
{
#ifdef STATS
	RenderStats.Lines += LineCount;
#endif

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER
	(
		DrawLineStripCommand,
		const INT, BaseVertexIndex2, BaseVertexIndex,
		const INT, LineCount2, LineCount,
		FGFxRenderer* const, Renderer, this,
	{
		Renderer->DrawLineStrip_RenderThread(BaseVertexIndex2, LineCount2);
	}
	)
}

void FGFxRenderer::DrawLineStrip_RenderThread(const INT BaseVertexIndex, const INT LineCount)
{
	CheckRenderTarget_RenderThread();

	// Note: Line strips are not supported by the RHI, line lists are however
	const FGFxRendererImpl::FGFxVertexStore& LocalVertexStore = *VertexStore;
	check( Vertex_XY16i == LocalVertexStore.Format );
	check( ( BaseVertexIndex >= 0 ) && ( BaseVertexIndex < (INT) LocalVertexStore.NumElements ) &&
		   ( BaseVertexIndex + LineCount <= (INT) LocalVertexStore.NumElements ) );

	// Ask line style what shaders and vertex declaration we should use
	FGFxEnumeratedBoundShaderState EnumeratedBoundShaderState;
	FGFxRenderStyleContext         StyleContext = { LocalVertexStore.Format, BlendMode };
	LineStyle.GetEnumeratedBoundShaderState_RenderThread( &EnumeratedBoundShaderState, &StyleContext );
	check( GFx_VD_Strip == EnumeratedBoundShaderState.VertexDeclarationType );
    
	// Obtain actual RHI bound shader state
	FGFxBoundShaderState BoundShaderState;
	FGFxRendererImpl::GetUIBoundShaderState_RenderThread( BoundShaderState, BoundShaderStateCache, EnumeratedBoundShaderState );
	FGFxPixelShaderInterface&   PixelShader = *BoundShaderState.PixelShaderInterface;
	FGFxVertexShaderInterface&  VertexShader = *BoundShaderState.VertexShaderInterface;

	PixelShader.SetParameterInverseGamma(PixelShader.GetNativeShader()->GetPixelShader(), CurRenderTarget->Resource.InverseGamma);

	// Set vertex shader transform
	ApplyUITransform_RenderThread( ViewportMatrix, CurrentMatrix, VertexShader );

	// Apply line style (color, color scale, color bias...)
	LineStyle.Apply_RenderThread( this, &BoundShaderState, &StyleContext );

	// Apply bound shader state.
	RHISetBoundShaderState(BoundShaderState.NativeBoundShaderState);

	// Lock to prevent ReleaseCachedData from modifying our pointer.
	GLock::Locker Guard(GetElementsLock());

	// Allocate buffer for storing list vertices
	INT         NumLinesRendered = 0;
	const INT   MaxLineVertices = 2 * GFX_LOCAL_BUFFER_BASIS_SIZE;
	const INT   TotalVertices = 2 * LineCount;
	const INT   VerticesPerLineVertexBuffer = Min( MaxLineVertices, TotalVertices );
	check( ( VerticesPerLineVertexBuffer % 2 ) == 0 );
	FGFxVertex_Strip* const LineVertexBufferUP = new FGFxVertex_Strip[ VerticesPerLineVertexBuffer ];
	check ( NULL != LineVertexBufferUP );
	const FGFxVertex_Strip* VertexData = &( ((FGFxVertex_Strip*) LocalVertexStore.Elements)[ BaseVertexIndex ] );
    
	// Convert line strip to line list since RHI doesn't support line strips
	while ( NumLinesRendered < LineCount )
	{
		INT VertexCount = 0;
		for ( ; ( ( VertexCount < VerticesPerLineVertexBuffer ) && ( NumLinesRendered < LineCount ) ); VertexCount += 2, ++NumLinesRendered )
		{
			FGFxVertex_Strip* const pLocalLineVertexBufferUP = &( LineVertexBufferUP[ 0 ] ) + VertexCount;
			pLocalLineVertexBufferUP[ 0 ] = VertexData[ 0 ];
			pLocalLineVertexBufferUP[ 1 ] = VertexData[ 1 ];
			++VertexData;
		}

		// Immediate mode - render user pointer buffer.
		RHIDrawPrimitiveUP( PT_LineList, VertexCount / 2, LineVertexBufferUP, sizeof(FGFxVertex_Strip)  );
	}

	// Free list buffer
	check( NumLinesRendered == LineCount );
	delete LineVertexBufferUP;
}

/**
* Draws a set of transformed bitmaps; intended for glyph rendering.
*/
void FGFxRenderer::DrawBitmaps( BitmapDesc* BitmapList, int ListSize, int StartIndex, int Count, const GTexture* Ti, const Matrix& M, CacheProvider* Cache /*= 0*/ ) 
{
	if ((!BitmapList) || (!Ti) || (Count==0))    
	{
		return;    
	}

	check( (StartIndex >= 0) && (StartIndex < ListSize) &&
		   (StartIndex + Count <= ListSize) );

	FGFxRendererImpl::FGFxBitmapDescStore* Store = 0;

	if (Cache)
	{
		GRenderer::CachedData* CachedData = Cache->GetCachedData(this);
		if (CachedData)
		{   // If this element is already cached, just AddRef for command and reuse the node.
			Store = (FGFxRendererImpl::FGFxBitmapDescStore*)CachedData->GetRendererData();
			Store->AddRef_MainThread();
		}
		else if ((CachedData = Cache->CreateCachedData(Cached_BitmapList, this)) != 0)
		{
			// List-inserting constructor.
			Store = new FGFxRendererImpl::FGFxBitmapDescStore();
			Store->InitElements(this, CachedData, 0, ListSize, const_cast<BitmapDesc*>(BitmapList));
			CachedData->SetRendererData(Store);
		}            
	}
	else
	{
		// Regular queue
		// Data-copy constructor; initialize without inserting into the list.
		Store = new FGFxRendererImpl::FGFxBitmapDescStore();
		Store->InitElementsCopy(0, Count, BitmapList+StartIndex);
		StartIndex = 0;
		ListSize = Count;
	}
	check(Store != 0);
	const_cast<GTexture*>(Ti)->AddRef();

	struct FGFxDrawBitmapsParams
	{
		FGFxRendererImpl::FGFxBitmapDescStore* BmpDescStore;
		INT StartIndex;
		INT Count;
		const FGFxTexture* Texture;
		Matrix FontMatrix;        
	};
    
	const FGFxDrawBitmapsParams Params =
	{
		Store, StartIndex, Count, (FGFxTexture*)Ti, M
	};
    
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		DrawBitmapsCommand,
		const FGFxDrawBitmapsParams, DbParams, Params,
		FGFxRenderer* const, Renderer, this,
	{
		Renderer->DrawBitmaps_RenderThread( 
			  DbParams.BmpDescStore,
			  DbParams.StartIndex,
			  DbParams.Count,
			  DbParams.Texture,
			DbParams.FontMatrix,
			NULL);
	}
	)

#ifdef STATS
		RenderStats.Triangles += Count * 2;
#endif
}

void FGFxRenderer::DrawDistanceFieldBitmaps( BitmapDesc* BitmapList, int ListSize, int StartIndex, int Count, const GTexture* Ti, const Matrix& M, const DistanceFieldParams& DFParams, CacheProvider* Cache /*= 0*/ ) 
{
	if ((!BitmapList) || (!Ti) || (Count==0))    
	{
		return;    
	}

	check( (StartIndex >= 0) && (StartIndex < ListSize) &&
		   (StartIndex + Count <= ListSize) );

	FGFxRendererImpl::FGFxBitmapDescStore* Store = 0;

	if (Cache)
	{
		GRenderer::CachedData* CachedData = Cache->GetCachedData(this);
		if (CachedData)
		{   // If this element is already cached, just AddRef for command and reuse the node.
			Store = (FGFxRendererImpl::FGFxBitmapDescStore*)CachedData->GetRendererData();
			Store->AddRef_MainThread();
		}
		else if ((CachedData = Cache->CreateCachedData(Cached_BitmapList, this)) != 0)
		{
			// List-inserting constructor.
			Store = new FGFxRendererImpl::FGFxBitmapDescStore();
			Store->InitElements(this, CachedData, 0, ListSize, const_cast<BitmapDesc*>(BitmapList));
			CachedData->SetRendererData(Store);
		}            
	}
	else
	{
		// Regular queue
		// Data-copy constructor; initialize without inserting into the list.
		Store = new FGFxRendererImpl::FGFxBitmapDescStore();
		Store->InitElementsCopy(0, Count, BitmapList+StartIndex);
		StartIndex = 0;
		ListSize = Count;
	}
	check(Store != 0);
	const_cast<GTexture*>(Ti)->AddRef();

	struct FGFxDrawBitmapsParams
	{
		FGFxRendererImpl::FGFxBitmapDescStore* BmpDescStore;
		INT StartIndex;
		INT Count;
		const FGFxTexture* Texture;
		Matrix FontMatrix;     
		DistanceFieldParams DFParams;
	};
    
	const FGFxDrawBitmapsParams Params =
	{
		Store, StartIndex, Count, (FGFxTexture*)Ti, M, DFParams
	};
    
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		DrawBitmapsCommand,
		const FGFxDrawBitmapsParams, DbParams, Params,
		FGFxRenderer* const, Renderer, this,
	{
		Renderer->DrawBitmaps_RenderThread( 
			  DbParams.BmpDescStore,
			  DbParams.StartIndex,
			  DbParams.Count,
			  DbParams.Texture,
			  DbParams.FontMatrix,
			  &DbParams.DFParams);
	}
	)

#ifdef STATS
	RenderStats.Triangles += Count * 2;
#endif
}

// BitmapDesc* pOwnedBmDescList is destroyed within this function
void FGFxRenderer::DrawBitmaps_RenderThread(
	FGFxRendererImpl::FGFxBitmapDescStore* BmpDescStore,
	const INT StartIndex,
	const INT GlyphCount,
	const FGFxTexture* Texture,
	const Matrix& FontMatrix,
	const DistanceFieldParams* Params) 
{
	check( NULL != BmpDescStore );

	CheckRenderTarget_RenderThread();
	FGFxRendererImpl::ApplyUIBlendMode_RenderThread( bAlphaComposite, BlendMode );

	// Set enumerated bound shader state
	FGFxEnumeratedBoundShaderState EnumeratedBoundShaderState;

	if (Params)
	{
		EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureDFA;
	}
	else if (Texture->IsYUVTexture() == 2)
	{
		if (BlendMode == Blend_Multiply || BlendMode == Blend_Darken)
		{
			EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureYUVAMultiply;
		}
		else
		{
			EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureYUVA;
		}
	}
	else if (Texture->IsYUVTexture() == 1)
	{
		if (BlendMode == Blend_Multiply || BlendMode == Blend_Darken)
		{
			EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureYUVMultiply;
		}
		else
		{
			EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureYUV;
		}
	}
	else if (Texture->Texture->Resource->bGreyScaleFormat)
	{
		EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTexture;
	}
	else if (Texture->Texture->SRGB)
	{
		if (BlendMode == Blend_Multiply || BlendMode == Blend_Darken)
		{
			EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureSRGBMultiply;
		}
		else
		{
			EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureSRGB;
		}
	}
	else if (BlendMode == Blend_Multiply || BlendMode == Blend_Darken)
	{
		EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureColorMultiply;
	}
	else
	{
		EnumeratedBoundShaderState.PixelShaderType = GFx_PS_TextTextureColor;
	}

	EnumeratedBoundShaderState.VertexShaderType      = GFx_VS_Glyph;
	EnumeratedBoundShaderState.VertexDeclarationType = GFx_VD_Glyph;

	// Obtain bound shader state specified by enumeration
	FGFxBoundShaderState BoundShaderState;
	FGFxRendererImpl::GetUIBoundShaderState_RenderThread( BoundShaderState, BoundShaderStateCache, EnumeratedBoundShaderState );
	FGFxPixelShaderInterface& PixelShader   = *BoundShaderState.PixelShaderInterface;
	FGFxVertexShaderInterface& VertexShader = *BoundShaderState.VertexShaderInterface;

	// TBD: Apply color scale and bias
	PixelShader.SetParametersColorScaleAndColorBias(PixelShader.GetNativeShader()->GetPixelShader(), CurrentCxform);
	PixelShader.SetParameterInverseGamma(PixelShader.GetNativeShader()->GetPixelShader(), CurRenderTarget->Resource.InverseGamma);

	if (Params)
	{
		PixelShader.SetDistanceFieldParams(PixelShader.GetNativeShader()->GetPixelShader(), Texture->Texture2D, *Params);
	}

	Texture->Bind(0, PixelShader, GRenderer::Wrap_Clamp, GRenderer::Sample_Linear, 1);

	// Set up vertex transform
	ApplyUITransform_RenderThread(ViewportMatrix, FontMatrix, VertexShader);

	// Apply bound shader state.
	RHISetBoundShaderState(BoundShaderState.NativeBoundShaderState);

	// Allocate buffer for storing list vertices	
	INT         NumGlyphsRendered   = 0;
	const INT   MaxGlyphVertices    = 6 * GFX_LOCAL_BUFFER_BASIS_SIZE;
	const INT   TotalVertices       = 6 * GlyphCount;
	const INT   VerticesPerGlyphVertexBuffer = MaxGlyphVertices;
	check( ( VerticesPerGlyphVertexBuffer % 6 ) == 0 );

	FGFxVertex_Glyph GlyphVertexBufferUP[ VerticesPerGlyphVertexBuffer ];

	// Lock to prevent ReleaseCachedData from modifying our pointer.
	GLock::Locker Guard(GetElementsLock());

	const BitmapDesc* BmpDescList = (const BitmapDesc*)BmpDescStore->Elements;
	BmpDescList += StartIndex;

	// Create triangle list from list of BitmapDescs
	while ( NumGlyphsRendered < GlyphCount )
	{
		INT VertexCount = 0;
		for ( ; ( ( VertexCount < VerticesPerGlyphVertexBuffer ) && ( NumGlyphsRendered < GlyphCount ) ); VertexCount += 6, ++NumGlyphsRendered )
		{
			const BitmapDesc& Bd = BmpDescList[ NumGlyphsRendered ];
			FGFxVertex_Glyph* const LocalGlyphVertexBufferUP = &( GlyphVertexBufferUP[ 0 ] ) + VertexCount;

			// 1st triangle
			LocalGlyphVertexBufferUP[ 0 ].SetVertex2D
				( Bd.Coords.Left, Bd.Coords.Top, Bd.TextureCoords.Left, Bd.TextureCoords.Top, Bd.Color );
			LocalGlyphVertexBufferUP[ 1 ].SetVertex2D
				( Bd.Coords.Right, Bd.Coords.Top, Bd.TextureCoords.Right, Bd.TextureCoords.Top, Bd.Color );
			LocalGlyphVertexBufferUP[ 2 ].SetVertex2D
				( Bd.Coords.Left, Bd.Coords.Bottom, Bd.TextureCoords.Left, Bd.TextureCoords.Bottom, Bd.Color );

			// 2nd triangle
			LocalGlyphVertexBufferUP[ 3 ].SetVertex2D
				( Bd.Coords.Left, Bd.Coords.Bottom, Bd.TextureCoords.Left, Bd.TextureCoords.Bottom, Bd.Color );
			LocalGlyphVertexBufferUP[ 4 ].SetVertex2D
				( Bd.Coords.Right, Bd.Coords.Top, Bd.TextureCoords.Right, Bd.TextureCoords.Top, Bd.Color );
			LocalGlyphVertexBufferUP[ 5 ].SetVertex2D
				( Bd.Coords.Right, Bd.Coords.Bottom, Bd.TextureCoords.Right, Bd.TextureCoords.Bottom, Bd.Color );
		}

		// Immediate mode - render user pointer buffer.
		RHIDrawPrimitiveUP(PT_TriangleList, VertexCount / 3, GlyphVertexBufferUP, sizeof(FGFxVertex_Glyph));
	}

	// Free list buffer and descriptions
	check( GlyphCount == NumGlyphsRendered );
	BmpDescStore->Release_RenderThread(GetElementsLock());
	const_cast<FGFxTexture*>(Texture)->Release();
}

/**
* Sets the fill style used for triangle mesh rendering to a texture. 
* This fill style is used by the DrawIndexedTriList method.
*/
void FGFxRenderer::FillStyleBitmap( const FillTexture* Fill ) 
{ 
	FFillTextureInfo NewTexInfo;
	ConvertFromUI( *Fill, NewTexInfo );

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		FillStyleBitmapCommand,
		FGFxRenderer* const, Renderer, this,
		const FFillTextureInfo, NewTexInfoRT, NewTexInfo,
	{
		Renderer->FillStyleBitmap_RenderThread( NewTexInfoRT );
	}
	)
}

void FGFxRenderer::FillStyleBitmap_RenderThread( const FFillTextureInfo& NewTexInfo )
{
	FillStyle.SetStyleBitmap_RenderThread( NewTexInfo, CurrentCxform  );
}

/**
* Sets the fill style used for triangle mesh rendering to a solid color.
* This fill style is used by the DrawIndexedTriList method.
*/
void FGFxRenderer::FillStyleColor( GColor Color )
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		FillStyleColorCommand,
		FGFxRenderer* const, Renderer, this,
		const GColor, Color, Color,
	{
		Renderer->FillStyleColor_RenderThread(Color);
	}
	)
}

// Render-thread analogue of FillStyleColor
void FGFxRenderer::FillStyleColor_RenderThread(const GColor Color)
{
	FillStyle.SetStyleColor_RenderThread(Color, CurrentCxform);	
}

/**
* Disables the specified fill style.
*/
void FGFxRenderer::FillStyleDisable()
{
	FillStyle.Disable();
}

/**
* Sets the interpolated color/texture fill style used for shapes with edge anti-aliasing. 
*
* The specified textures are applied to triangle based on factors of gouraud vertex.
* Trailing argument texture pointers can be NULL, in which case texture is not applied 
* and vertex colors are used instead. All texture arguments can be null only if
* GFill_Color FillType is used. 
*
* The fill style set by FillStyleGouraud is always used in conjunction with
* VertexXY16iC32 or VertexXY16iCF32 vertex types. See documentation for these vertex formats
* for details on how textures and vertex colors are mixed together based on specified FillType. 
*
* If the renderer implementation does not support EdgeAA it can implement this function as a no-op.
* In this case, the renderer must NOT report the Cap_FillGouraud and Cap_FillGouraudTexture capability Flags.
*/
void FGFxRenderer::FillStyleGouraud(GouraudFillType FillType, const FillTexture* Texture0, const FillTexture* Texture1, const FillTexture* Texture2) 
{ 
	// Implement FRenderCommand directly instead of using
	// GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER; doing so simplifies
	// and optimizes Tex0/1/2 assignment logic (because it avoids copy 
	// constructor that would require extra ifs to init pointers correctly).
	class FillStyleGouraudCommand : public FRenderCommand
	{
		FGFxRenderer*       Renderer;
		GouraudFillType     FillType;
		FFillTextureInfo*	Tex0Ptr;
		FFillTextureInfo*	Tex1Ptr;
		FFillTextureInfo*	Tex2Ptr;
		FFillTextureInfo    Tex0, Tex1, Tex2;

	public:
		FillStyleGouraudCommand(FGFxRenderer* const Renderer,
			GouraudFillType FillType,
			const FillTexture* Texture0,
			const FillTexture* Texture1,
			const FillTexture* Texture2) 
		: 
			Renderer(Renderer),
			FillType(FillType), 
			Tex0Ptr(0), 
			Tex1Ptr(0), 
			Tex2Ptr(0)
		{
			if (Texture0)
			{
				ConvertFromUI(*Texture0, Tex0);
				Tex0Ptr = &Tex0;
			}
			if (Texture1)
			{
				ConvertFromUI(*Texture1, Tex1);
				Tex1Ptr = &Tex1;
			}
			if (Texture2)
			{
				ConvertFromUI(*Texture2, Tex2);
				Tex2Ptr = &Tex2;
			}
		}

		virtual UINT Execute()
		{              
			Renderer->FillStyleGouraud_RenderThread(FillType, Tex0Ptr, Tex1Ptr, Tex2Ptr);
			return sizeof(*this);
		}         
		virtual const TCHAR* DescribeCommand()
		{
			return TEXT("FillStyleGouraudCommand");
		}
	};
	// GFX_ENQUEUE command with our arguments.
	GFX_ENQUEUE_RENDER_COMMAND(FillStyleGouraudCommand,(this,FillType, Texture0,Texture1,Texture2));
}

// Render-thread analogue of FillStyleGouraud
void FGFxRenderer::FillStyleGouraud_RenderThread(
	const GouraudFillType FillType,
	const FFillTextureInfo* const TexInfo0, /*= 0*/
	const FFillTextureInfo* const TexInfo1, /*= 0*/
	const FFillTextureInfo* const TexInfo2 /*= 0*/ )
{
	FillStyle.SetStyleGouraud_RenderThread( FillType, TexInfo0, TexInfo1, TexInfo2, CurrentCxform );
}

void FGFxRenderer::GetRenderStats(Stats* Stats, bool bResetStats)
{
	if (Stats)
	{
		appMemcpy(Stats, &RenderStats, sizeof(Stats));
	}
	if (bResetStats)
	{
		RenderStats.Clear();
	}
}

/**
* Sets the line style used for line strips to a solid color.
*/
void FGFxRenderer::LineStyleColor(GColor Color)
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		LineStyleColorCommand,
		FGFxRenderer* const, Renderer, this,
		const GColor, Color, Color,
	{
		Renderer->LineStyleColor_RenderThread( Color );
	}
	)
}

// Render-thread analogue of LineStyleColor
void FGFxRenderer::LineStyleColor_RenderThread(const GColor Color)
{
	LineStyle.SetStyleColor_RenderThread(Color, CurrentCxform);	
}

/**
* Disables the specified line style
*/
void FGFxRenderer::LineStyleDisable() 
{
	LineStyle.Disable();
}

bool FGFxRenderTarget::AdjustBounds(Float* InWidth, Float* InHeight)
{
	if (Resource.SizeX < *InWidth || Resource.SizeY < *InHeight)
	{
		Float aspect = *InWidth / *InHeight;
		if (*InWidth > *InHeight)
		{
			if (Resource.SizeX >= (SInt)ceilf(aspect * Resource.SizeY))
			{
				*InWidth = (UInt)ceilf(Resource.SizeY / aspect);
				*InHeight = Resource.SizeY;
			}
			else
			{
                *InWidth = Resource.SizeX;
				*InHeight = (UInt)ceilf(Resource.SizeX / aspect);
			}
		}
		else
		{
			if (Resource.SizeX >= (SInt)ceilf(aspect * Resource.SizeY))
			{
				*InHeight = Resource.SizeY;
				*InWidth = (UInt)ceilf(aspect * Resource.SizeY);
			}
			else
			{
				*InWidth = Resource.SizeX;
				*InHeight = (UInt)ceilf(aspect * Resource.SizeX);
			}
		}
		return TRUE;
	}
	return FALSE;
}


// Render background color quad
void FGFxRenderer::DrawColorMatrixRect_RenderThread(GTexture* InTexture, const GRectF& SrcRect, const GRectF& DestRect, const Float* CMatrix)
{
	CheckRenderTarget_RenderThread();

	FGFxBoundShaderState           BoundShaderState;
	FGFxEnumeratedBoundShaderState Shaders;

	Shaders.PixelShaderType       = (BlendMode == Blend_Darken || BlendMode == Blend_Multiply) ? FS2_FCMatrixMul : FS2_FCMatrix;
	Shaders.VertexShaderType      = GFx_VS_Glyph;
	Shaders.VertexDeclarationType = GFx_VD_Glyph;

	FGFxTexture* Texture = (FGFxTexture*)InTexture;

	FGFxRendererImpl::GetUIBoundShaderState_RenderThread (BoundShaderState, BoundShaderStateCache, Shaders);
	check(FGFxRendererImpl::IsValidUIBoundShaderState(BoundShaderState));

	FGFxVertexShaderInterface& VertexShader = *BoundShaderState.VertexShaderInterface;
	FGFxPixelShaderInterface&  PixelShader  = *BoundShaderState.PixelShaderInterface;

	ApplyUITransform_RenderThread(ViewportMatrix, CurrentMatrix, VertexShader);
	FGFxRendererImpl::ApplyUIBlendMode_RenderThread(TRUE, BlendMode, TRUE);

	PixelShader.SetParameterColorMatrix(PixelShader.GetNativeShader()->GetPixelShader(), CMatrix);
	PixelShader.SetParameterInverseGamma(PixelShader.GetNativeShader()->GetPixelShader(), CurRenderTarget->Resource.InverseGamma);

	Texture->Bind(0, PixelShader, Wrap_Clamp, Sample_Linear, 0);

	RHISetBoundShaderState(BoundShaderState.NativeBoundShaderState);

	FGFxVertex_Glyph verts[6];
	float TexScaleX = 1.0f/Texture->RenderTarget->Resource.SizeX;
	float TexScaleY = 1.0f/Texture->RenderTarget->Resource.SizeY;
	Float bufWidthf = SrcRect.Width();
	Float bufHeightf = SrcRect.Height();

	if (Texture->RenderTarget->AdjustBounds(&bufWidthf, &bufHeightf))
	{
		TexScaleX *= (bufWidthf/SrcRect.Width());
		TexScaleY *= (bufHeightf/SrcRect.Height());
	}

	verts[0].SetVertex2D(DestRect.Left, DestRect.Top, SrcRect.Left * TexScaleX, SrcRect.Top * TexScaleY, 0);
	verts[1].SetVertex2D(DestRect.Right, DestRect.Top, SrcRect.Right * TexScaleX, SrcRect.Top * TexScaleY, 0);
	verts[2].SetVertex2D(DestRect.Left, DestRect.Bottom, SrcRect.Left * TexScaleX, SrcRect.Bottom * TexScaleY, 0);
	verts[3].SetVertex2D(DestRect.Right, DestRect.Bottom, SrcRect.Right * TexScaleX, SrcRect.Bottom * TexScaleY, 0);

	RHIDrawPrimitiveUP(PT_TriangleStrip, 2, verts, sizeof(FGFxVertex_Glyph));
}

UInt    FGFxRenderer::CheckFilterSupport(const BlurFilterParams& params)
{
	UInt flags = FilterSupport_Ok;
	if (params.Passes > 1 || (params.BlurX * params.BlurY > /* XXX */ 64))
		flags |= FilterSupport_Multipass;

	return flags;
}

void  FGFxRenderer::DrawBlurRect_RenderThread(GTexture* psrcin, const GRectF& insrcrect, const GRectF& indestrect, const BlurFilterParams& params)
{
	GPtr<FGFxTexture> psrcinRef = (FGFxTexture*)psrcin;
	GPtr<FGFxTexture> psrc = (FGFxTexture*)psrcin;

	CheckRenderTarget_RenderThread();

	if (!psrc->RenderTarget)
		return;

	GRectF SrcRect, DestRect(-1,-1,1,1);
	UInt n = params.Passes;

	BlurFilterParams     pass[3];
	EGFxPixelShaderType  passis[3];

	pass[0] = params;
	pass[1] = params;
	pass[2] = params;

	bool mul = (BlendMode == Blend_Multiply || BlendMode == Blend_Darken);
	passis[0] = passis[1] = FS2_FBox2Blur;
	passis[2] = mul ? FS2_FBox2BlurMul : FS2_FBox2Blur;

	if (params.Mode & Filter_Shadow)
	{
		if (params.Mode & Filter_HideObject)
		{
			passis[2] = FS2_FBox2Shadowonly;
			if (params.Mode & Filter_Highlight)
				passis[2] = (EGFxPixelShaderType) (passis[2] + FS2_shadows_Highlight);
		}
		else
		{
			if (params.Mode & Filter_Inner)
				passis[2] = FS2_FBox2InnerShadow;
			else
				passis[2] = FS2_FBox2Shadow;

			if (params.Mode & Filter_Knockout)
				passis[2] = (EGFxPixelShaderType) (passis[2] + FS2_shadows_Knockout);

			if (params.Mode & Filter_Highlight)
				passis[2] = (EGFxPixelShaderType) (passis[2] + FS2_shadows_Highlight);
		}

		if (mul)
			passis[2] = (EGFxPixelShaderType) (passis[2] + FS2_shadows_Mul);
	}

	if (params.BlurX * params.BlurY > 32)
	{
		n *= 2;
		pass[0].BlurY = 1;
		pass[1].BlurX = 1;
		pass[2].BlurX = 1;

		passis[0] = passis[1] = FS2_FBox1Blur;
		if (passis[2] == FS2_FBox2Blur)
			passis[2] = FS2_FBox1Blur;
		else if (passis[2] == FS2_FBox2BlurMul)
			passis[2] = FS2_FBox1BlurMul;
	}

	Float  bufWidthf0 = insrcrect.Width();
	Float  bufHeightf0 = insrcrect.Height();

	FGFxBoundShaderState           BoundShaderState;
	FGFxEnumeratedBoundShaderState Shaders;
	Shaders.VertexShaderType      = GFx_VS_Glyph;
	Shaders.VertexDeclarationType = GFx_VD_Glyph;

	for (UInt i = 0; i < n; i++)
	{
        Float  srcXScale = (1.0f/psrc->RenderTarget->Resource.SizeX);
        Float  srcYScale = (1.0f/psrc->RenderTarget->Resource.SizeY);
        Float  bufWidthf = bufWidthf0;
        Float  bufHeightf = bufHeightf0;
        if (psrc->RenderTarget->AdjustBounds(&bufWidthf, &bufHeightf))
        {
            srcXScale *= (bufWidthf/insrcrect.Width());
            srcYScale *= (bufHeightf/insrcrect.Height());
        }

        UInt   bufWidth = (UInt)ceilf(bufWidthf);
        UInt   bufHeight = (UInt)ceilf(bufHeightf);

		UInt passi = (i == n-1) ? 2 : (i&1);

		FGFxTexture* pnextsrc;
		if (i != n - 1)
		{
			pnextsrc = PushTempRenderTarget_RenderThread(NULL, GRectF(-1,-1,1,1), bufWidth, bufHeight, false);
			if (!pnextsrc)
			{
				i = n-1;
				passi = 2;
			}
		}
		else
			pnextsrc = 0;

		Shaders.PixelShaderType = passis[passi];
		CheckRenderTarget_RenderThread();

		FGFxRendererImpl::GetUIBoundShaderState_RenderThread (BoundShaderState, BoundShaderStateCache, Shaders);
		check(FGFxRendererImpl::IsValidUIBoundShaderState(BoundShaderState));

		FGFxVertexShaderInterface& VertexShader = *BoundShaderState.VertexShaderInterface;
		FGFxPixelShaderInterface&  PixelShader  = *BoundShaderState.PixelShaderInterface;

		if (i != n - 1)
		{
			ApplyUITransform_RenderThread(ViewportMatrix, GMatrix2D::Identity, VertexShader);
			DestRect = GRectF(-1,-1,1,1);
		}
		else
		{
			ApplyUITransform_RenderThread(ViewportMatrix, CurrentMatrix, VertexShader);
			DestRect = indestrect;
		}

		const BlurFilterParams& pparams = pass[passi];

		//CheckRenderTarget_RenderThread();
		RHISetBoundShaderState(BoundShaderState.NativeBoundShaderState);

		SrcRect = GRectF(insrcrect.Left * srcXScale,
						 insrcrect.Top * srcYScale,
						 insrcrect.Right * srcXScale,
						 insrcrect.Bottom * srcYScale);

		if (i < n - 1)
			FGFxRendererImpl::ApplyUIBlendMode_RenderThread(TRUE, Blend_Normal, TRUE);
		else
			FGFxRendererImpl::ApplyUIBlendMode_RenderThread(TRUE, Blend_Normal, TRUE);

		PixelShader.SetParameterInverseGamma(PixelShader.GetNativeShader()->GetPixelShader(), CurRenderTarget->Resource.InverseGamma);

		if (i == n - 1)
			PixelShader.SetParametersCxformAc(PixelShader.GetNativeShader()->GetPixelShader(), params.cxform);
		else
			PixelShader.SetParametersCxformAc(PixelShader.GetNativeShader()->GetPixelShader(), GRenderer::Cxform::Identity);

		Float SizeX = UInt(pparams.BlurX-1) * 0.5f;
		Float SizeY = UInt(pparams.BlurY-1) * 0.5f;

		if (passis[passi] == FS2_FBox1Blur || passis[passi] == FS2_FBox1BlurMul)
		{
			PixelShader.SetParameterTexScale(PixelShader.GetNativeShader()->GetPixelShader(), 0,
				pparams.BlurX > 1 ? 1.0f/psrc->RenderTarget->Resource.SizeX : 0,
				pparams.BlurY > 1 ? 1.0f/psrc->RenderTarget->Resource.SizeY : 0);

			PixelShader.SetParameterFilterSize4(PixelShader.GetNativeShader()->GetPixelShader(),
				pparams.BlurX > 1 ? SizeX : SizeY, 0, 0, 1.0f/(1+2*(pparams.BlurX > 1 ? SizeX : SizeY)));
		}
		else
		{
			PixelShader.SetParameterTexScale(PixelShader.GetNativeShader()->GetPixelShader(), 0,
				1.0f/psrc->RenderTarget->Resource.SizeX,
				1.0f/psrc->RenderTarget->Resource.SizeY);

			PixelShader.SetParameterFilterSize(PixelShader.GetNativeShader()->GetPixelShader(), SizeX, SizeY);
		}

		if (passis[passi] >= FS2_start_shadows && passis[passi] <= FS2_end_shadows)
		{
			psrcinRef->Bind(1, PixelShader, Wrap_Clamp, Sample_Linear, 0);
			PixelShader.SetParameterTexScale(PixelShader.GetNativeShader()->GetPixelShader(), 1,
				psrc->RenderTarget->Resource.SizeX/Float(psrcinRef->RenderTarget->Resource.SizeX),
				psrc->RenderTarget->Resource.SizeY/Float(psrcinRef->RenderTarget->Resource.SizeY));

			PixelShader.SetParameterShadowOffset(PixelShader.GetNativeShader()->GetPixelShader(), -params.Offset.x, -params.Offset.y);

			PixelShader.SetParameterShadowColor(PixelShader.GetNativeShader()->GetPixelShader(), 0, params.Color);

			if ((params.Mode & Filter_Highlight) && passi == 2)
			{
				PixelShader.SetParameterShadowColor(PixelShader.GetNativeShader()->GetPixelShader(), 1, params.Color2);
			}
		}

		psrc->Bind(0, PixelShader, Wrap_Clamp, Sample_Linear, 0);

		FGFxVertex_Glyph verts[6];
		verts[0].SetVertex2D(DestRect.Left, DestRect.Top, SrcRect.Left, SrcRect.Top, 0);
		verts[1].SetVertex2D(DestRect.Right, DestRect.Top, SrcRect.Right, SrcRect.Top, 0);
		verts[2].SetVertex2D(DestRect.Left, DestRect.Bottom, SrcRect.Left, SrcRect.Bottom, 0);
		verts[3].SetVertex2D(DestRect.Right, DestRect.Bottom, SrcRect.Right, SrcRect.Bottom, 0);

		RHIDrawPrimitiveUP(PT_TriangleStrip, 2, verts, sizeof(FGFxVertex_Glyph));

		if (i != n - 1)
		{
			PopRenderTarget_RenderThread();
			psrc = pnextsrc;
		}
	}

	FGFxRendererImpl::ApplyUIBlendMode_RenderThread(bAlphaComposite, BlendMode);
}

void FGFxRenderer::DrawBlurRect(GTexture* Texture, const GRectF& SrcRect, const GRectF& DestRect, const BlurFilterParams& InParams)
{
	struct BlurRectParams
	{
		GPtr<GTexture> Texture;
		GRectF    SrcRect;
		GRectF    DestRect;
		BlurFilterParams  Params;
		UInt              NTempRTs;
		FGFxRenderTarget* TempRTs[32];
	} Params;
	Params.Texture = Texture;
	Params.SrcRect = SrcRect;
	Params.DestRect = DestRect;
	Params.Params = InParams;

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
		(
		DrawBlurRectCommand,
		BlurRectParams, Params, Params,
		FGFxRenderer* const, pRenderer, this,
	{
		pRenderer->DrawBlurRect_RenderThread(Params.Texture, Params.SrcRect, Params.DestRect, Params.Params);
	}
	)
}

void FGFxRenderer::DrawColorMatrixRect(GTexture* Texture, const GRectF& SrcRect, const GRectF& DestRect, const Float* CMatrix)
{
	struct ColorMatrixParams
	{
		GPtr<GTexture> Texture;
		GRectF    SrcRect;
		GRectF    DestRect;
		Float     CMatrix[20];
	} Params;
	Params.Texture = Texture;
	Params.SrcRect = SrcRect;
	Params.DestRect = DestRect;
	appMemcpy(Params.CMatrix, CMatrix, sizeof(Float)*20);

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
		(
		DrawColorMatrixRectCommand,
		ColorMatrixParams, Params, Params,
		FGFxRenderer* const, pRenderer, this,
	{
		pRenderer->DrawColorMatrixRect_RenderThread(Params.Texture, Params.SrcRect, Params.DestRect, Params.CMatrix);
	}
	)
}

/** 
* Begins rendering of a submit mask that can be used to cull other rendered contents. 
* All shapes submitted from this point on will generate a mask.
*/
void FGFxRenderer::BeginSubmitMask(SubmitMaskMode MaskMode /*= Mask_Clear*/)
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
	(
		BeginSubmitMaskCommand,
		const SubmitMaskMode, Mode, MaskMode,
		FGFxRenderer* const, Renderer, this,
	{
		Renderer->BeginSubmitMask_RenderThread(Mode);
	}
	)
}

// Render-thread analogue of BeginSubmitMask
void FGFxRenderer::BeginSubmitMask_RenderThread(const SubmitMaskMode MaskMode)
{
	CheckRenderTarget_RenderThread();

	// Assuming stencil buffer is available and nested stenciling is supported

	// Disable color writes.
	RHISetColorWriteEnable(FALSE);

	FStencilStateInitializerRHI Initializer(
		  TRUE,      // bEnableFrontFaceStencil
		  CF_Always, // FrontFaceStencilTest 
		  SO_Keep,   // FrontFaceStencilFailStencilOp 
		  SO_Keep,   // FrontFaceDepthFailStencilOp
		  SO_Keep,   // FrontFacePassStencilOp
		  FALSE,     // bEnableBackFaceStencil
		  CF_Always, // BackFaceStencilTest
		  SO_Keep,   // BackFaceStencilFailStencilOp
		  SO_Keep,   // BackFaceDepthFailStencilOp
		  SO_Keep,   // BackFacePassStencilOp
		  0xFFFFFFFF,// StencilReadMask
		  0xFFFFFFFF,// StencilWriteMask
		  0 	     // StencilRef
	);

	// Handle modes
	switch(MaskMode)
	{
	case Mask_Clear:
	{
		// Clear stencil buffer
		RHIClear(
			FALSE,               // bClearColor 
			FLinearColor::Black, // Color 
			FALSE,               // bClearDepth 
			0.f,                 // Depth 
			TRUE,                // bClearStencil 
			0                    // Stencil 
			);

		// Set stencil state
		Initializer.FrontFacePassStencilOp = SO_Replace;
		Initializer.StencilRef = 1;

		// Reset stencil counter	
		StencilCounter = 1;
	}
		break;
	case Mask_Increment:
	{
		// Set stencil state
		Initializer.FrontFaceStencilTest   = CF_Equal;
		Initializer.FrontFacePassStencilOp = SO_Increment;
		Initializer.StencilRef             = StencilCounter;

		// Update stencil counter
		++StencilCounter;
	}
		break;
	case Mask_Decrement:
	{
		// Set stencil state
		Initializer.FrontFaceStencilTest   = CF_Equal;
		Initializer.FrontFacePassStencilOp = SO_Decrement;
		Initializer.StencilRef             = StencilCounter;

		// Update stencil counter
		--StencilCounter;
	}
		break;
	default:
		checkMsg( 0, "FGFxRenderer::BeginSubmitMask: Failed, unrecognized stencil mode." );
		break;
	}

	CurStencilState = RHICreateStencilState(Initializer);
	RHISetStencilState(CurStencilState);
}

/**
* Ends rendering of a submit mask an enables its use. All shapes rendered from
* this point on are clipped by the mask. The mask is originally generated by
* rendering shapes after a call to BeginSubmitMask, it is used after EndSubmitMask.
*/
void FGFxRenderer::EndSubmitMask()
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		EndSubmitMaskCommand,
		FGFxRenderer* const, Renderer, this,
	{
		Renderer->EndSubmitMask_RenderThread();
	}
	)
}

void FGFxRenderer::EndSubmitMask_RenderThread()
{
	// Assuming stencil buffer is available and nested stenciling is supported

	// Re-enable color write
	RHISetColorWriteEnable(TRUE);

	// We draw only where the (stencil == StencilCounter), i.e. where the latest mask was drawn.
	// However, we don't change the stencil buffer

	FStencilStateInitializerRHI Initializer(
		  TRUE,          // bEnableFrontFaceStencil
		  CF_Equal,      // FrontFaceStencilTest 
		  SO_Keep,       // FrontFaceStencilFailStencilOp 
		  SO_Keep,       // FrontFaceDepthFailStencilOp
		  SO_Keep,       // FrontFacePassStencilOp
		  FALSE,         // bEnableBackFaceStencil
		  CF_Always,     // BackFaceStencilTest
		  SO_Keep,       // BackFaceStencilFailStencilOp
		  SO_Keep,       // BackFaceDepthFailStencilOp
		  SO_Keep,       // BackFacePassStencilOp
		  0xFFFFFFFF,    // StencilReadMask
		  0xFFFFFFFF,    // StencilWriteMask
		  StencilCounter // StencilRef
	);

	CurStencilState = RHICreateStencilState(Initializer);
	RHISetStencilState(CurStencilState);
}

/**
* Disables the use of a submit mask, all content is fully rendered from this point on. 
* The mask is originally generated by rendering shapes after a call to BeginSubmitMask; 
* it is used after EndSubmitMask.
*/
void FGFxRenderer::DisableMask()
{
	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		DisableMaskCommand,
		FGFxRenderer* const, Renderer, this,
	{
		Renderer->DisableMask_RenderThread();
	}
	)
}

void FGFxRenderer::DisableMask_RenderThread()
{
	// Assuming stencil buffer is available and nested stenciling is supported
	// Re-enable color write.
	RHISetColorWriteEnable(TRUE);

	// Disable masking through stencil state
	StencilCounter = 0;
	CurStencilState = TStaticStencilState< FALSE /*bEnableFrontFaceStencil*/ >::GetRHI();
	RHISetStencilState(CurStencilState);	
}

namespace FGFxRendererImpl
{
	static UByte* SoftwareResample(
		int Format,
		int SrcWidth,
		int SrcHeight,
		int SrcPitch,
		UByte* SrcData,
		int DstWidth,
		int DstHeight)
	{
		UByte* Rescaled = (UByte*)UGALLOC(DstWidth * DstHeight * 4);

		switch(Format)
		{
		case GImage::Image_RGB_888:
			GRenderer::ResizeImage(&Rescaled[0], DstWidth, DstHeight, DstWidth * 3,
				SrcData, SrcWidth, SrcHeight, SrcPitch,
				GRenderer::ResizeRgbToRgb);
			break;

		case GImage::Image_ARGB_8888:
			GRenderer::ResizeImage(&Rescaled[0], DstWidth, DstHeight, DstWidth * 4,
				SrcData, SrcWidth, SrcHeight, SrcPitch,
				GRenderer::ResizeRgbaToRgba);
			break;

		case GImage::Image_A_8:
		case GImage::Image_P_8:
			GRenderer::ResizeImage(&Rescaled[0], DstWidth, DstHeight, DstWidth,
				SrcData, SrcWidth, SrcHeight, SrcPitch,
				GRenderer::ResizeGray);
			break;
		}

		return Rescaled;
	}

	static inline UByte* LoadTexture(UByte* Dest, UInt DestPitch, const UByte* Src, UInt w, UInt h, SInt Bpp, UInt Pitch)
	{
		UPInt Pofs;

		switch (Bpp)
		{
		case 4:
			Pofs = DestPitch - (w * 4);
			for (UInt j = 0; j < h; j++, Dest += Pofs)
			{
				for (UInt i = 0; i < w; i++, Dest += 4)
				{
					Dest[0] = Src[j*Pitch+i*4+2];
					Dest[1] = Src[j*Pitch+i*4+1];
					Dest[2] = Src[j*Pitch+i*4+0];
					Dest[3] = Src[j*Pitch+i*4+3];
				}
			}

			return Dest;

		case 3:
			Pofs = DestPitch - (w * 4);
			for (UInt j = 0; j < h; j++, Dest += Pofs)
			{
				for (UInt i = 0; i < w; i++, Dest += 4)
				{
					Dest[0] = Src[j*Pitch+i*3+2];
					Dest[1] = Src[j*Pitch+i*3+1];
					Dest[2] = Src[j*Pitch+i*3+0];
					Dest[3] = 255;
				}
			}

			return Dest;

		case 1:
			for (UInt j = 0; j < h; j++)
			{
				appMemcpy(Dest + j*DestPitch, Src + j*Pitch, w);
			}
			return Dest + h * DestPitch;
		}

		return 0;
	}

#ifndef FGFxRendererImpl

UTexture2D* CreateNativeTexture(GRenderer* Renderer, INT Type, INT StatId)
{
	UTexture2D* Texture;

	switch (Type)
	{
	case GFxCreateTex_Normal:
		Texture = new UTexture2D;
		break;
	case GFxCreateTex_Mappable:
		Texture = new UGFxMappableTexture;
		break;
	case GFxCreateTex_Updatable:
		Texture = new UGFxUpdatableTexture;
		break;
	default:
		return NULL;
	}

	const UBOOL Result = GGFxGCManager->AddGCReferenceFor(Texture, StatId);
	check( TRUE == Result );
	return Texture;
}

void TermGCState(GRenderer* pRenderer, UTexture* Texture)
{
	check ( NULL != Texture );
	const UBOOL Result = GGFxGCManager->RemoveGCReferenceFor(Texture);
	check( TRUE == Result );
}

#endif
}

FGFxTexture::~FGFxTexture()
{
	if (Texture)		
		InternalTermGCState();
	if (RenderTarget)
	{
		RenderTarget->Texture = NULL;
	}
}

void FGFxTexture::InternalTermGCState()
{
	if (Texture)
		FGFxRendererImpl::TermGCState(Renderer, Texture);
	if (RenderTarget)
		RenderTarget->Texture = 0;
	Texture = Texture2D = NULL;
	RenderTarget = NULL;
}

void FGFxTexture::ReleaseTexture()
{
	InternalTermGCState();
}


#if XBOX
#include "GFxUIRendererXbox360.cpp"

#elif PS3
#include "GFxUIRendererPS3.cpp"

#else
void FGFxTexture::LoadMipLevel(int Level, int DestPitch, const UByte* Src, UInt w, UInt h, SInt Bpp, UInt Pitch)
{
	if (Texture2D)
	{
		FTexture2DMipMap& Tex = Texture2D->Mips(Level);
		UByte* Dest = (UByte*)Tex.Data.Lock(LOCK_READ_WRITE);
		FGFxRendererImpl::LoadTexture(Dest, DestPitch, Src, w, h, Bpp, Pitch);
		Tex.Data.Unlock();
	}
}
#endif

bool FGFxTexture::InitTexture(UTexture* Texture, bool unused)
{
	GUNUSED(unused);

	ReleaseTexture();
	this->Texture = Texture;
	Texture2D = Cast<UTexture2D>(Texture);

	check( NULL != Texture );
	const UBOOL Result = GGFxGCManager->AddGCReferenceFor(Texture, STAT_GFxTextureCount);
	check( TRUE == Result );

	return TRUE;
}

bool FGFxTexture::InitTexture(GImageBase* Im, UInt)
{
	// Delete old data
	ReleaseTexture();
	if (!Im)
	{
		return 1;
	}

	// Determine Format
	UInt BytesPerPixel = 0, DestBpp = 1;
	GImage::ImageFormat DestFmt = Im->Format;
	EPixelFormat TexFormat = PF_A8R8G8B8;

	if (Im->Format == GImage::Image_ARGB_8888)
	{
		BytesPerPixel   = 4;
		DestBpp         = 4;
		TexFormat       = PF_A8R8G8B8;
	}
	else if (Im->Format == GImage::Image_RGB_888)
	{
		BytesPerPixel   = 3;
		DestBpp         = 4;
		DestFmt         = GImage::Image_ARGB_8888;
		TexFormat       = PF_A8R8G8B8;
	}
	else if (Im->Format == GImage::Image_A_8)
	{
		BytesPerPixel   = 1;
		TexFormat       = PF_G8;
	}
	else if (Im->Format == GImage::Image_DXT1)
	{
		BytesPerPixel   = 1;
		TexFormat       = PF_DXT1;
	}
	else if (Im->Format == GImage::Image_DXT3)
	{
		BytesPerPixel   = 1;
		TexFormat       = PF_DXT3;
	}
	else if (Im->Format == GImage::Image_DXT5)
	{
		BytesPerPixel   = 1;
		TexFormat       = PF_DXT5;
	}
	else
	{
		// Unsupported Format
		check(0);
	}

	UInt    w, h;

	if (Im->IsDataCompressed())
	{
		w = Im->Width;
		h = Im->Height;
	}
	else
	{
		w = 1; 
		while (w < Im->Width) 
		{
			w <<= 1; 
		}
		h = 1; 
		while (h < Im->Height) 
		{ 
			h <<= 1; 
		}
#ifdef GFXUIRENDERER_MIN_TEXTURE_SIZE
		if (w < GFXUIRENDERER_MIN_TEXTURE_SIZE) 
		{
			w = GFXUIRENDERER_MIN_TEXTURE_SIZE;
		}
		if (h < GFXUIRENDERER_MIN_TEXTURE_SIZE) 
		{
			h = GFXUIRENDERER_MIN_TEXTURE_SIZE;
		}
#endif
	}

	Texture = Texture2D = FGFxRendererImpl::CreateNativeTexture(Renderer,FGFxRendererImpl::GFxCreateTex_Normal,STAT_GFxTextureCount);
	Texture2D->RequestedMips = 1;
	Texture2D->SRGB = FALSE;
	Texture2D->Init(w, h, TexFormat);
	check(Texture2D->RequestedMips == 1);

	if (w != Im->Width || h != Im->Height)
	{
		GASSERT_ON_RENDERER_RESAMPLING;

		// Faster/simpler software bilinear rescale.
		UByte *Src = FGFxRendererImpl::SoftwareResample(Im->Format, Im->Width, Im->Height, Im->Pitch, Im->pData, w, h);

		LoadMipLevel(0, w*DestBpp, Src, w, h, BytesPerPixel, w*BytesPerPixel);

		GFREE(Src);
	}
	else if (Im->MipMapCount <= 1 && !Im->IsDataCompressed())
	{
		LoadMipLevel(0, w*DestBpp, Im->pData, w, h, BytesPerPixel, Im->Pitch);
	}
	else
	{
		// Use original image directly.
		UInt Level = 0;
		do {
			UInt MipW, MipH, MipPitch;
			const UByte* Data = Im->GetMipMapLevelData(Level, &MipW, &MipH, &MipPitch);
			if (Data == 0) //????
			{
				GFC_DEBUG_WARNING(1, "FGFxRenderer: can't find mipmap Level in texture");
				break;
			}
			else if (Im->IsDataCompressed())
			{
				if (Texture2D)
				{
					FTexture2DMipMap& Tex = Texture2D->Mips(Level);
					UByte* Dest = (UByte*) Tex.Data.Lock(LOCK_READ_WRITE);
					appMemcpy(Dest, Data, GImage::GetMipMapLevelSize(Im->Format, MipW, MipH));
					Tex.Data.Unlock();
				}
			}
			else
			{
				LoadMipLevel(Level, w*DestBpp, Data, MipW, MipH, BytesPerPixel, MipPitch);
			}
		} while(++Level < Im->MipMapCount);
	}

	Texture2D->UpdateResource();
	return 1;
}


bool FGFxTexture::InitDynamicTexture(int Width, int Height, GImage::ImageFormat Format, int Mipmaps, UInt Usage)
{
	// Delete old data
	ReleaseTexture();

	// Determine Format
	EPixelFormat TexFormat = PF_A8R8G8B8;

	if (Format == GImage::Image_ARGB_8888)
	{
		TexFormat = PF_A8R8G8B8;
	}
	else if (Format == GImage::Image_A_8)
	{
		TexFormat = PF_G8;
	}
	else
	{
		// Unsupported Format
		check(0);
	}

	if (Usage & Usage_Map)
	{
		UGFxMappableTexture* TextureUp2D = (UGFxMappableTexture*)
			FGFxRendererImpl::CreateNativeTexture(Renderer,FGFxRendererImpl::GFxCreateTex_Mappable, STAT_GFxVideoTextureCount);
		Texture = Texture2D = TextureUp2D;
	}
	else if (Usage & Usage_Update)
	{
		UGFxUpdatableTexture* TextureUp2D = (UGFxUpdatableTexture*)
			FGFxRendererImpl::CreateNativeTexture(Renderer,FGFxRendererImpl::GFxCreateTex_Updatable, STAT_GFxFontTextureCount);
		Texture = Texture2D = TextureUp2D;
	}
	else
	{
		RenderTarget = NULL;

		const UBOOL Result = GGFxGCManager->AddGCReferenceFor(Texture, STAT_GFxRTTextureCount);
		check( TRUE == Result );
	}
	if (Texture2D)
	{
	Texture2D->NeverStream = TRUE;
	Texture2D->RequestedMips = 1+Mipmaps;
	Texture2D->SRGB = FALSE;
	Texture2D->Init(Width, Height, TexFormat);
	}

#ifdef FGFxTexture
	if (Texture->Resource)
	{
		Texture->Resource->ReleaseResource();
		delete Texture->Resource;
	}
	Texture->Resource = Texture->CreateResource();
	Texture->Resource->InitResource();
#else
	Texture->UpdateResource();
#endif
	if (Texture2D)
	check(Texture2D->RequestedMips == 1+Mipmaps);

	return 1;
}

void FGFxTexture::Update_RenderThread(int Level, int n, const FUpdateTextureRegion2D* Regions, const GImageBase *Im)
{
	FGFxUpdatableTexture* Tex2d = (FGFxUpdatableTexture*)Texture->Resource;

	UINT Pitch;
	int Bpp = 4, sbpp = Im->GetBytesPerPixel();
	if (sbpp == 1)
	{
		Bpp = 1;
	}

	if (RHIUpdateTexture2D(Tex2d->Texture2DRHI, Level, n, Regions, Im->Pitch, sbpp, Im->pData))
	{
		return;
	}

	UByte* Tex = (UByte*)RHILockTexture2D(Tex2d->Texture2DRHI, Level, 1, Pitch, 0);

	for (int i = 0; i < n; i++)
	{
		FGFxRendererImpl::LoadTexture(
			Tex + Regions[i].DestY * Pitch + Regions[i].DestX * Bpp, 
			Pitch, 
			Im->pData + Im->Pitch * Regions[i].SrcY + sbpp * Regions[i].SrcX,
			Regions[i].Width, 
			Regions[i].Height, 
			sbpp, 
			Im->Pitch);
	}

	RHIUnlockTexture2D(Tex2d->Texture2DRHI, Level, 0);
}

void FGFxTexture::Update(int Level, int n, const UpdateRect* Rects, const GImageBase* Im)
{
	struct FGFxUpdateParams
	{
		FUpdateTextureRegion2D* Regions;
		GImage* Im;
		int Level, n;
	} Params = {0,0, Level, n};

	Params.Im = new GImage(*Im);
	Params.Regions = (FUpdateTextureRegion2D*)appMalloc(sizeof(FUpdateTextureRegion2D)*n);
	for (int i = 0; i < n; i++)
	{
		Params.Regions[i].DestX = Rects[i].dest.x;
		Params.Regions[i].DestY = Rects[i].dest.y;
		Params.Regions[i].SrcX = Rects[i].src.Left;
		Params.Regions[i].SrcY = Rects[i].src.Top;
		Params.Regions[i].Width = Rects[i].src.Width();
		Params.Regions[i].Height = Rects[i].src.Height();
	}

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
		(
		UpdateCommand,
		const FGFxUpdateParams, UpdateParams, Params,
		FGFxTexture* const, Texture, this,
	{
		Texture->Update_RenderThread(UpdateParams.Level, UpdateParams.n, UpdateParams.Regions, UpdateParams.Im);
		appFree(UpdateParams.Regions);
		UpdateParams.Im->Release();
	}
	)
}

void FGFxTexture::Bind(
	int Stage, 
	FGFxPixelShaderInterface& PixelShader,
	GRenderer::BitmapWrapMode WrapMode, 
	GRenderer::BitmapSampleMode SampleMode, 
	bool bUseMipmaps) const
{
	if (Texture)
		PixelShader.SetParameterTexture(PixelShader.GetNativeShader()->GetPixelShader(),
			((FGFxRenderer*)Renderer)->GetSamplerState(SampleMode, WrapMode, UsesMips()), Texture->Resource, Stage );
	else if (RenderTarget)
		PixelShader.SetParameterTextureRHI(PixelShader.GetNativeShader()->GetPixelShader(),
			((FGFxRenderer*)Renderer)->GetSamplerState(SampleMode, WrapMode, FALSE), RenderTarget->Resource.ColorTexture, Stage );
}

#if (GFC_FX_MAJOR_VERSION >= 3)
int FGFxTexture::Map(int Level, int n, MapRect* Maps, int Flags)
{
	FGFxUpdatableTexture* Tex2d = (FGFxUpdatableTexture*)Texture->Resource;
	UINT Pitch;
	UByte* Tex = (UByte*)RHILockTexture2D(Tex2d->Texture2DRHI, Level, 1, Pitch, 0);
	if (Tex == NULL)
	{
		return 0;
	}

	int w = Texture->Resource->GetSizeX();
	int h = Texture->Resource->GetSizeY();
	for (int i = 0; i < Level; i++)
	{
		h >>= 1;
		if (h < 1)
		{
			h = 1;
		}
		w >>= 1;
		if (w < 1)
		{
			w = 1;
		}
	}

	Maps[0].width = w;
	Maps[0].height = h;
	Maps[0].pitch = Pitch;
	Maps[0].pData = Tex;
	return 1;
}

bool FGFxTexture::Unmap(int level, int n, MapRect* maps, int flags)
{
	FGFxUpdatableTexture* ptex2d = (FGFxUpdatableTexture*)Texture->Resource;
	if (n >= 1)
		RHIUnlockTexture2D(ptex2d->Texture2DRHI, level, 0);
	return 1;
}
#endif


#ifndef GFXUIRENDERER_NO_COMMON

void  FGFxRenderTargetResource::InitDynamicRHI()
{
	if (Owner)
		ColorBuffer = Owner->GetRenderTargetSurface();
	else
	{
		ColorTexture = RHICreateTexture2D(SizeX, SizeY, PF_A8R8G8B8, 1, TexCreate_ResolveTargetable, NULL);
		ColorBuffer = RHICreateTargetableSurface(SizeX, SizeY, PF_A8R8G8B8, ColorTexture, 0, TEXT("GFxTempColor"));

		StatSize = SizeX*SizeY*4;
		INC_DWORD_STAT(STAT_GFxRTTextureCount);
		INC_DWORD_STAT_BY(STAT_GFxRTTextureMem, StatSize);
	}

	if (OwnerDepth)
		DepthBuffer = OwnerDepth->GetDepthTargetSurface();
}

FGFxRenderTargetResource::~FGFxRenderTargetResource()
{
	if (IsInGameThread())
		BeginReleaseResource(this);
	else
		ReleaseResource();
}

void  FGFxRenderTargetResource::ReleaseDynamicRHI()
{
	ColorTexture.SafeRelease();
	ColorBuffer.SafeRelease();
	DepthBuffer.SafeRelease();
	if (StatSize > 0)
	{
		DEC_DWORD_STAT(STAT_GFxRTTextureCount);
		DEC_DWORD_STAT_BY(STAT_GFxRTTextureMem, StatSize);
		StatSize = 0;
	}
}

bool        FGFxRenderTargetResource::InitRenderTarget(const NativeRenderTarget& params)
{
	if (params.Owner != Owner ||
		params.OwnerDepth != OwnerDepth)
		BeginReleaseResource(this);
#if PS3
	else if ((params.Owner == Owner && Owner) ||
			 (params.OwnerDepth == OwnerDepth && OwnerDepth))
	{
		GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
			(CheckRenderTargetResourceCommand,
			FGFxRenderTargetResource*, RT, this,
			NativeRenderTarget, RTParams, params,
		{
			if (RT->Owner)
				RT->ColorBuffer = RT->Owner->GetRenderTargetSurface();
			if (RT->OwnerDepth)
				RT->DepthBuffer = RT->OwnerDepth->GetDepthTargetSurface();
		});
	}
#endif

	Owner = params.Owner;
	OwnerDepth = params.OwnerDepth;
	if (Owner)
	{
		SizeX = Owner->GetSizeX();
		SizeY = Owner->GetSizeY();
		BeginInitResource(this);
	}

	return TRUE;
}

bool        FGFxRenderTargetResource::InitRenderTarget_RenderThread(const NativeRenderTarget& params)
{
	if (params.Owner != Owner ||
		params.OwnerDepth != OwnerDepth)
		ReleaseResource();
#if PS3
	else
	{
		if (params.Owner == Owner && Owner)
			ColorBuffer = Owner->GetRenderTargetSurface();
		if (params.OwnerDepth == OwnerDepth && OwnerDepth)
			DepthBuffer = OwnerDepth->GetDepthTargetSurface();
	}
#endif

	Owner = params.Owner;
	OwnerDepth = params.OwnerDepth;
	if (Owner)
	{
		SizeX = Owner->GetSizeX();
		SizeY = Owner->GetSizeY();
		InitResource();
	}

	return TRUE;
}

#endif // NO_COMMON

FGFxRenderTarget::~FGFxRenderTarget()
{
	if (Texture)
		Texture->RenderTarget = NULL;
	if (IsInGameThread())
	{
		GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
			(InitRenderTargetCommand,
			FGFxRenderTargetResource*, RTResource, &Resource,
		{
			delete RTResource;
		});
	}
	else
		delete &Resource;
}

bool        FGFxRenderTarget::InitRenderTarget(GTexture *InTarget, GTexture* InDepth, GTexture* InStencil)
{
	BeginReleaseResource(&Resource);
	check(!InDepth);
	check(!InStencil);

	FGFxTexture* Target = (FGFxTexture*)InTarget;

	GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
		(InitRenderTargetCommand,
		FGFxRenderTarget*, RT, this,
		FGFxTexture*, Texture, Target,
	{
		RT->InitRenderTarget_RenderThread(Texture, NULL, Texture->RenderTarget->Resource.SizeX, Texture->RenderTarget->Resource.SizeY);
	});

	return TRUE;
}

bool        FGFxRenderTarget::InitRenderTarget_RenderThread(GTexture *ptarget, FGFxRenderResources* pdepth, UInt InWidth, UInt InHeight)
{
	Resource.ReleaseResource();

	GPtr<GTexture> TextureRef (ptarget);
	Texture = (FGFxTexture*)ptarget;
	StencilBuffer = pdepth;
	Resource.Owner = NULL;
	Resource.OwnerDepth = NULL;
	TargetWidth = Resource.SizeX = InWidth;
	TargetHeight = Resource.SizeY = InHeight;

	Resource.InitResource();
	return TRUE;
}

#undef GFX_ENQUEUE_RENDER_COMMAND
#undef GFX_ENQUEUE_UNIQUE_RENDER_COMMAND
#undef GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
#undef GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER
#undef GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER


#endif // WITH_GFx
