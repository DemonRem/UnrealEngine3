/**********************************************************************

Copyright   :   (c) 2006-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef GFxUIRendererImpl_h
#define GFxUIRendererImpl_h


#if WITH_GFx


//---------------------------------------------------------------------------------------------------------------------
// FGFxPixelShader
//---------------------------------------------------------------------------------------------------------------------

enum EGFxPixelShaderType
{
    FS2_None = 0,
    FS2_start_shadows,
    FS2_FBox2InnerShadow = 1,
    FS2_FBox2InnerShadowHighlight,
    FS2_FBox2InnerShadowMul,
    FS2_FBox2InnerShadowMulHighlight,
    FS2_FBox2InnerShadowKnockout,
    FS2_FBox2InnerShadowHighlightKnockout,
    FS2_FBox2InnerShadowMulKnockout,
    FS2_FBox2InnerShadowMulHighlightKnockout,
    FS2_FBox2Shadow,
    FS2_FBox2ShadowHighlight,
    FS2_FBox2ShadowMul,
    FS2_FBox2ShadowMulHighlight,
    FS2_FBox2ShadowKnockout,
    FS2_FBox2ShadowHighlightKnockout,
    FS2_FBox2ShadowMulKnockout,
    FS2_FBox2ShadowMulHighlightKnockout,
    FS2_FBox2Shadowonly,
    FS2_FBox2ShadowonlyHighlight,
    FS2_FBox2ShadowonlyMul,
    FS2_FBox2ShadowonlyMulHighlight,
    FS2_end_shadows = 20,
    FS2_start_blurs,
    FS2_FBox2Blur,
    FS2_FBox2BlurMul = 24,
    FS2_FBox1Blur,
    FS2_FBox1BlurMul = 27,
    FS2_end_blurs = 27,
    FS2_start_cmatrix,
    FS2_FCMatrix = 28,
    FS2_FCMatrixMul,
    FS2_end_cmatrix = 29,

    GFx_PS_SolidColor,
    GFx_PS_CxformTexture,
    GFx_PS_CxformTextureMultiply,
    GFx_PS_TextTexture,
    GFx_PS_TextTextureColor,
    GFx_PS_TextTextureColorMultiply,
    GFx_PS_TextTextureSRGB,
    GFx_PS_TextTextureSRGBMultiply,

    GFx_PS_CxformGouraud,
    GFx_PS_CxformGouraudNoAddAlpha,
    GFx_PS_CxformGouraudTexture,
    GFx_PS_Cxform2Texture,

    // Multiplies - must come in same order as other Gouraud types
    GFx_PS_CxformGouraudMultiply,
    GFx_PS_CxformGouraudMultiplyNoAddAlpha,
    GFx_PS_CxformGouraudMultiplyTexture,
    GFx_PS_CxformMultiply2Texture,

    GFx_PS_TextTextureYUV,
    GFx_PS_TextTextureYUVMultiply,
    GFx_PS_TextTextureYUVA,
    GFx_PS_TextTextureYUVAMultiply,

    GFx_PS_TextTextureDFA,

    GFx_PS_Count,
    FS2_Count = GFx_PS_Count,

    FS2_shadows_Highlight            = 0x00000001,
    FS2_shadows_Mul                  = 0x00000002,
    FS2_shadows_Knockout             = 0x00000004,
    FS2_blurs_Box2                 = 0x00000001,
    FS2_blurs_Mul                  = 0x00000002,
    FS2_cmatrix_Mul                  = 0x00000001,
};

/**
* Pixel Shaders for rendering GFx UI
*/
template<EGFxPixelShaderType PSType> class FGFxPixelShader : public FShader, public FGFxPixelShaderInterface
{
    DECLARE_SHADER_TYPE(FGFxPixelShader,Global);
public:

	const static INT NumTextures = 4;
    static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

    FGFxPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
    {
        TextureParams[0].Bind(Initializer.ParameterMap,TEXT("TextureImage"), TRUE);
        TextureParams[1].Bind(Initializer.ParameterMap,TEXT("TextureImage2"), TRUE);
        TextureParams[2].Bind(Initializer.ParameterMap,TEXT("TextureImage3"), TRUE);
        TextureParams[3].Bind(Initializer.ParameterMap,TEXT("TextureImage4"), TRUE);

        ConstantColorParameter.Bind(Initializer.ParameterMap,TEXT("ConstantColor"), TRUE);
        ColorScaleParameter.Bind(Initializer.ParameterMap,TEXT("ColorScale"), TRUE);
        ColorBiasParameter.Bind(Initializer.ParameterMap,TEXT("ColorBias"), TRUE);

        InverseGammaParameter.Bind(Initializer.ParameterMap,TEXT("InverseGamma"), TRUE);

        DFWidthParameter.Bind(Initializer.ParameterMap,TEXT("DFWidth"), TRUE);
        DFShadowWidthParameter.Bind(Initializer.ParameterMap,TEXT("DFShadowWidth"), TRUE);
        DFShadowColorParameter.Bind(Initializer.ParameterMap,TEXT("DFShadowColor"), TRUE);
        DFShadowOffsetParameter.Bind(Initializer.ParameterMap,TEXT("DFShadowOffset"), TRUE);
        DFShadowEnableParameter.Bind(Initializer.ParameterMap,TEXT("DFShadowEnable"), TRUE);
        DFGlowColorParameter.Bind(Initializer.ParameterMap,TEXT("DFGlowColor"), TRUE);
        DFGlowSizeParameter.Bind(Initializer.ParameterMap,TEXT("DFGlowRadius"), TRUE);
        DFGlowEnableParameter.Bind(Initializer.ParameterMap,TEXT("DFGlowEnable"), TRUE);
    }
    FGFxPixelShader() {}

    // FGFxPixelShaderInterface
    void SetParameterTextureRHI(FPixelShaderRHIParamRef PixelShader,FSamplerStateRHIParamRef SamplerState,FTextureRHIParamRef Texture,INT i=0) const
    {
        SetTextureParameter(PixelShader, TextureParams[i], SamplerState, Texture);
    }

    void SetParameterConstantColor(FPixelShaderRHIParamRef PixelShader,const FLinearColor& ConstantColor) const
    {
        SetPixelShaderValue(PixelShader, ConstantColorParameter, ConstantColor);
    }

    void SetParametersColorScaleAndColorBias(FPixelShaderRHIParamRef PixelShader,const GRenderer::Cxform& ColorXForm) const
    {
        //@TODO:  It would be cleaner to just treat Cxform as a 4x2 matrix 
        // --do that instead (but don't forget about the 0->255 scale on the bias part...
        const FLOAT Mult = 1.0f / 255.0f;
        const FLOAT ColorScale[4] = { ColorXForm.M_[0][0],        ColorXForm.M_[1][0],        ColorXForm.M_[2][0],        ColorXForm.M_[3][0] };
        const FLOAT ColorBias[4] =  { ColorXForm.M_[0][1] * Mult, ColorXForm.M_[1][1] * Mult, ColorXForm.M_[2][1] * Mult, ColorXForm.M_[3][1] * Mult };

        SetPixelShaderValue(PixelShader,ColorScaleParameter,ColorScale);
        SetPixelShaderValue(PixelShader,ColorBiasParameter,ColorBias);
    }

    void SetParameterInverseGamma(FPixelShaderRHIParamRef PixelShader,const FLOAT InverseGamma) const
    {
        SetPixelShaderValue(PixelShader,InverseGammaParameter,InverseGamma);
    }

    void SetParameterColorMatrix(FPixelShaderRHIParamRef PixelShader, const FLOAT* CMatrix) const
    {
    }

    void SetDistanceFieldParams(FPixelShaderRHIParamRef PixelShader, const UTexture2D* Texture, const GRenderer::DistanceFieldParams& Params) const
    {
        SetPixelShaderValue(PixelShader,DFWidthParameter, Params.Width / Texture->SizeX);

        if (Params.ShadowColor != 0)
        {
            SetPixelShaderBool(PixelShader,DFShadowEnableParameter, TRUE);
            SetPixelShaderValue(PixelShader,DFShadowWidthParameter, Params.ShadowWidth / Texture->SizeX);

            const FLOAT Mult = 1.0f / 255.0f;
            const FLOAT Color[4] =  {
                Params.ShadowColor.Channels.Red * Mult, Params.ShadowColor.Channels.Green * Mult,
                Params.ShadowColor.Channels.Blue * Mult, Params.ShadowColor.Channels.Alpha * Mult };
            SetPixelShaderValue(PixelShader,DFShadowColorParameter, Color);

            const FLOAT Offset[4] = {-Params.ShadowOffset.x / Texture->SizeX, -Params.ShadowOffset.y / Texture->SizeY, 0,0};
            SetPixelShaderValue(PixelShader,DFShadowOffsetParameter, Offset);
        }
        else
            SetPixelShaderBool(PixelShader,DFShadowEnableParameter, FALSE);

        if (Params.GlowColor != 0)
        {
            SetPixelShaderBool(PixelShader,DFGlowEnableParameter, TRUE);

            const FLOAT Mult = 1.0f / 255.0f;
            const FLOAT Color[4] =  {
                Params.GlowColor.Channels.Red * Mult, Params.GlowColor.Channels.Green * Mult,
                Params.GlowColor.Channels.Blue * Mult, Params.GlowColor.Channels.Alpha * Mult };
                SetPixelShaderValue(PixelShader,DFGlowColorParameter, Color);

                const FLOAT Size[4] = {Params.GlowSize[0], Params.GlowSize[1], 0,0};
                SetPixelShaderValue(PixelShader,DFGlowSizeParameter, Size);
        }
        else
            SetPixelShaderBool(PixelShader,DFGlowEnableParameter, FALSE);
    }


    // Hooks for bound shader state caching and parameter manipulating
    virtual FShader*                  GetNativeShader()    { return this; }
    virtual FGFxPixelShaderInterface* GetShaderInterface() { return this; }

    virtual UBOOL Serialize(FArchive& Ar)
    {
        UBOOL Result = FShader::Serialize(Ar);

        for (int i = 0; i < NumTextures; i++)
		{
            Ar << TextureParams[i];
		}

        Ar << ConstantColorParameter;
        Ar << ColorScaleParameter;
        Ar << ColorBiasParameter;
        Ar << InverseGammaParameter;
        Ar << DFWidthParameter;
        Ar << DFShadowWidthParameter;
        Ar << DFShadowOffsetParameter;
        Ar << DFShadowEnableParameter;
        Ar << DFShadowColorParameter;
        Ar << DFGlowSizeParameter;
        Ar << DFGlowEnableParameter;
        Ar << DFGlowColorParameter;

        return Result;
    }

private:

    FShaderResourceParameter TextureParams[NumTextures];
    FShaderParameter ConstantColorParameter;
    FShaderParameter ColorScaleParameter;
    FShaderParameter ColorBiasParameter;
    FShaderParameter InverseGammaParameter;
    FShaderParameter DFWidthParameter;
    FShaderParameter DFShadowWidthParameter;
    FShaderParameter DFShadowColorParameter;
    FShaderParameter DFShadowOffsetParameter;
    FShaderParameter DFShadowEnableParameter;
    FShaderParameter DFGlowColorParameter;
    FShaderParameter DFGlowSizeParameter;
    FShaderParameter DFGlowEnableParameter;
};

template<EGFxPixelShaderType PSType> class FGFxFilterPixelShader : public FShader, public FGFxPixelShaderInterface
{
    DECLARE_SHADER_TYPE(FGFxFilterPixelShader,Global);
public:

    static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

    FGFxFilterPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
    FShader(Initializer)
    {
        InverseGammaParameter.Bind(Initializer.ParameterMap,TEXT("InverseGamma"), TRUE);

        TextureParams[0].Bind(Initializer.ParameterMap,TEXT("tex"), TRUE);
        TextureParams[1].Bind(Initializer.ParameterMap,TEXT("srctex"), TRUE);
        TexScaleParams[0].Bind(Initializer.ParameterMap,TEXT("texscale"), TRUE);
        TexScaleParams[1].Bind(Initializer.ParameterMap,TEXT("srctexscale"), TRUE);

        CxformParameters[0].Bind(Initializer.ParameterMap,TEXT("cxmul"), TRUE);
        CxformParameters[1].Bind(Initializer.ParameterMap,TEXT("cxadd"), TRUE);
        ColorMatrixParameter.Bind(Initializer.ParameterMap,TEXT("cxmul1"), TRUE);

        BlurSizeParameter.Bind(Initializer.ParameterMap,TEXT("fsize"), TRUE);
        ShadowColorParams[0].Bind(Initializer.ParameterMap,TEXT("scolor"), TRUE);
        ShadowColorParams[1].Bind(Initializer.ParameterMap,TEXT("scolor2"), TRUE);
        ShadowOffsetParameter.Bind(Initializer.ParameterMap,TEXT("offset"), TRUE);
    }
    FGFxFilterPixelShader() {}

    // FGFxPixelShaderInterface
    void SetParameterTextureRHI(FPixelShaderRHIParamRef PixelShader,FSamplerStateRHIParamRef SamplerState,FTextureRHIParamRef Texture,INT i=0) const
    {
        SetTextureParameter(PixelShader, TextureParams[i], SamplerState, Texture);
    }

    void SetParameterConstantColor(FPixelShaderRHIParamRef PixelShader,const FLinearColor& ConstantColor) const
    {
    }

    void SetDistanceFieldParams(FPixelShaderRHIParamRef PixelShader, const UTexture2D* Texture, const GRenderer::DistanceFieldParams& Params) const
    {
    }

    void SetParametersColorScaleAndColorBias(FPixelShaderRHIParamRef PixelShader,const GRenderer::Cxform& ColorXForm) const
    {
        const float mult = 1.0f / 255.0f;
        float ColorScale[4] = { ColorXForm.M_[0][0],        ColorXForm.M_[1][0],        ColorXForm.M_[2][0],        ColorXForm.M_[3][0] };
        float ColorBias[4] =  { ColorXForm.M_[0][1] * mult, ColorXForm.M_[1][1] * mult, ColorXForm.M_[2][1] * mult, ColorXForm.M_[3][1] * mult };

        SetPixelShaderValue(PixelShader,CxformParameters[0],ColorScale);
        SetPixelShaderValue(PixelShader,CxformParameters[1],ColorBias);
    }

    void SetParametersCxformAc(FPixelShaderRHIParamRef PixelShader,const GRenderer::Cxform& ColorXForm) const
    {
        const float mult = 1.0f / 255.0f;
        const float   cxformMul[4] =
        { 
            ColorXForm.M_[0][0] * ColorXForm.M_[3][0],
            ColorXForm.M_[1][0] * ColorXForm.M_[3][0],
            ColorXForm.M_[2][0] * ColorXForm.M_[3][0],
            ColorXForm.M_[3][0]
        };
        const float   cxformAdd[4] =
        { 
            ColorXForm.M_[0][1] * mult * ColorXForm.M_[3][0],
            ColorXForm.M_[1][1] * mult * ColorXForm.M_[3][0],
            ColorXForm.M_[2][1] * mult * ColorXForm.M_[3][0],
            ColorXForm.M_[3][1] * mult
        };

        SetPixelShaderValue(PixelShader,CxformParameters[0],cxformMul);
        SetPixelShaderValue(PixelShader,CxformParameters[1],cxformAdd);
    }

    void SetParameterInverseGamma(FPixelShaderRHIParamRef PixelShader,const FLOAT InverseGamma) const
    {
        SetPixelShaderValue(PixelShader,InverseGammaParameter,InverseGamma);
    }

    void SetParameterColorMatrix(FPixelShaderRHIParamRef PixelShader, const FLOAT* CMatrix) const
    {
        SetPixelShaderValue(PixelShader,ColorMatrixParameter,*(FMatrix*)CMatrix);
        const float cxadd[4] = {CMatrix[16], CMatrix[17], CMatrix[18], CMatrix[19]};
        SetPixelShaderValue(PixelShader,CxformParameters[1],cxadd);
    }

    void SetParameterTexScale(FPixelShaderRHIParamRef PixelShader,INT N, FLOAT XScale, FLOAT YScale) const
    {
        const FLOAT Scales[] = {XScale, YScale, 0, 0};
        SetPixelShaderValue(PixelShader,TexScaleParams[N],Scales);
    }

    void SetParameterFilterSize4(FPixelShaderRHIParamRef PixelShader, FLOAT SizeX, FLOAT SizeY, FLOAT Z, FLOAT W) const
    {
        const float FSize[] = {SizeX, SizeY, Z, W};
        SetPixelShaderValue(PixelShader,BlurSizeParameter,FSize);
    }

    void SetParameterShadowColor(FPixelShaderRHIParamRef PixelShader,INT N, GColor Color) const
    {
        const float mult = 1.0f / 255.0f;
        const float scolor[] = {Color.GetRed() * mult, Color.GetGreen() * mult, Color.GetBlue() * mult, Color.GetAlpha() * mult};
        SetPixelShaderValue(PixelShader,ShadowColorParams[N], scolor);
    }

    void SetParameterShadowOffset(FPixelShaderRHIParamRef PixelShader, FLOAT SizeX, FLOAT SizeY) const
    {
        const float FSize[] = {SizeX, SizeY, 0, 0};
        SetPixelShaderValue(PixelShader,ShadowOffsetParameter,FSize);
    }

    // Hooks for bound shader state caching and parameter manipulating
    virtual FShader*                  GetNativeShader()    { return this; }
    virtual FGFxPixelShaderInterface* GetShaderInterface() { return this; }

    virtual UBOOL Serialize(FArchive& Ar)
    {
        UBOOL Result = FShader::Serialize(Ar);

        for (int i = 0; i < 2; i++)
        {
            Ar << TextureParams[i];
            Ar << TexScaleParams[i];
        }
        for (int i = 0; i < 2; i++)
        {
            Ar << CxformParameters[i];
            Ar << ShadowColorParams[i];
        }

        Ar << InverseGammaParameter;
        Ar << ColorMatrixParameter;
        Ar << BlurSizeParameter;
        Ar << ShadowOffsetParameter;

        return Result;
    }

private:

    FShaderResourceParameter TextureParams[2];
    FShaderParameter         TexScaleParams[2];

    FShaderParameter InverseGammaParameter;
    FShaderParameter CxformParameters[2];
    FShaderParameter ColorMatrixParameter;
    FShaderParameter BlurSizeParameter;
    FShaderParameter ShadowColorParams[2];
    FShaderParameter ShadowOffsetParameter;
    FShaderParameter HighlightParameter;
};

FGFxPixelShaderInterface* GetUIPixelShaderInterface_RenderThread( const EGFxPixelShaderType PixelShaderType );


//---------------------------------------------------------------------------------------------------------------------
// FGFxVertexShader
//---------------------------------------------------------------------------------------------------------------------

enum EGFxVertexShaderType
{
    GFx_VS_None						= 0,
    GFx_VS_Strip,
    GFx_VS_Glyph,
    GFx_VS_XY16iC32,
    GFx_VS_XY16iCF32,
    GFx_VS_XY16iCF32_NoTex,
    GFx_VS_XY16iCF32_NoTexNoAlpha,
    GFx_VS_XY16iCF32_T2,

    GFx_VS_Count
};

/**
* Vertex Shaders for rendering GFx UI
*/
template<EGFxVertexShaderType VSType> class FGFxVertexShader : public FShader, public FGFxVertexShaderInterface
{
    DECLARE_SHADER_TYPE(FGFxVertexShader,Global);
public:

    static UBOOL ShouldCache(EShaderPlatform Platform) { return TRUE; }

    FGFxVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FShader(Initializer)
    {
        TransformParameter.Bind(Initializer.ParameterMap,TEXT("Transform"));
        TextureMatrixParams[0].Bind(Initializer.ParameterMap,TEXT("TextureMatrix"),TRUE);
        TextureMatrixParams[1].Bind(Initializer.ParameterMap,TEXT("TextureMatrix2"),TRUE);
    }
    FGFxVertexShader() {}

    // FGFxVertexShaderInterface
    void SetParameterTransform(FVertexShaderRHIParamRef VertexShader, const FMatrix& Transform) const
    {
        SetVertexShaderValue( VertexShader, TransformParameter,Transform );
    }

    void SetParameterTextureMatrix(FVertexShaderRHIParamRef VertexShader, const FMatrix& TextureMatrix, INT i) const
    {
        SetVertexShaderValue( VertexShader, TextureMatrixParams[i], TextureMatrix );
    }

	void SetParameters(FVertexShaderRHIParamRef VertexShader,const FMatrix& Transform,
		const FMatrix& TextureMatrix, const FMatrix& TextureMatrix2) const
	{
		SetParameterTransform( VertexShader, Transform );
		SetParameterTextureMatrix( VertexShader, TextureMatrix, 0 );
		SetParameterTextureMatrix( VertexShader, TextureMatrix, 1 );
	}

    // Hooks for bound shader state caching and parameter manipulating
    FShader*                   GetNativeShader()    { return this; }
    FGFxVertexShaderInterface* GetShaderInterface() { return this; }

    virtual UBOOL Serialize(FArchive& Ar)
    {
        UBOOL Result = FShader::Serialize(Ar);
        Ar << TransformParameter;
        Ar << TextureMatrixParams[0];
        Ar << TextureMatrixParams[1];

        return Result;
    }

private:
    FShaderParameter TransformParameter;
    FShaderParameter TextureMatrixParams[2];
};

FGFxVertexShaderInterface* GetUIVertexShaderInterface_RenderThread( const EGFxVertexShaderType VertexType );


//---------------------------------------------------------------------------------------------------------------------
// FGFxVertex
//---------------------------------------------------------------------------------------------------------------------

struct FGFxVertex_XY16i
{
    SInt16 X, Y;
};

typedef FGFxVertex_XY16i FGFxVertex_Strip;

struct FGFxVertex_XY32F
{
    Float X, Y;
};

struct FGFxVertex_Glyph
{
    Float X, Y;
    Float U, V;
    GColor Color;

    void SetVertex2D( const Float InX, const Float InY, const Float InU, const Float InV, GColor c )
    {
        X = InX; Y = InY; U = InU; V = InV;
        Color = c;
    }
};

// *** Gouraud shaded fills - used for EdgeAA 

// Compact Gouraud-shaded vertex used for solid color/texture factor EdgeAA.
// Can be used with GFill_Color, GFill_1Texture, GFill_Texture2.

// Based on Scaleform GFx GRenderer::Vertex_XY16iC32
// Changed UInt32 to FColor
struct FGFxVertex_XY16iC32
{
    SInt16 X, Y;
    FColor Color;
};

// Based on Scaleform GFx GRenderer::Vertex_XY16iCF32
// Changed UInt32 to FColor
struct FGFxVertex_XY16iCF32
{
    SInt16 X, Y;
    FColor Color;
    FColor Factors;
    // Factors are interpreted as follows:
    //   .a  - texture 0 factor
    //   .b  - texture 1 factor
    //   .g  - texture 2 factor
};


//---------------------------------------------------------------------------------------------------------------------
// Vertex DECLs
//---------------------------------------------------------------------------------------------------------------------

enum EGFxVertexDeclarationType
{
    GFx_VD_None		= 0,
    GFx_VD_Strip,
    GFx_VD_Glyph,
    GFx_VD_XY16iC32,
    GFx_VD_XY16iCF32,
    GFx_VD_Count
};

//---------------------------------------------------------------------------------------------------------------------
// FGFxBoundShaderState
//---------------------------------------------------------------------------------------------------------------------

struct FGFxBoundShaderState 
{
    FGFxPixelShaderInterface* PixelShaderInterface;
    FGFxVertexShaderInterface* VertexShaderInterface;
    FBoundShaderStateRHIRef NativeBoundShaderState;
};

typedef TMap< DWORD, FBoundShaderStateRHIRef > FGFxBoundShaderStateCache;

struct FGFxEnumeratedBoundShaderState
{
    EGFxPixelShaderType         PixelShaderType;
    EGFxVertexShaderType        VertexShaderType;
    EGFxVertexDeclarationType   VertexDeclarationType;

    FGFxEnumeratedBoundShaderState() 
        : PixelShaderType(GFx_PS_SolidColor), VertexShaderType(GFx_VS_None), VertexDeclarationType(GFx_VD_None) { }

    UBOOL IsValid() const
    {
        return	(//(PixelShaderType != GFx_PS_None) && 
            (VertexShaderType != GFx_VS_None) && 
            (VertexDeclarationType != GFx_VD_None));
    }
};

//---------------------------------------------------------------------------------------------------------------------
// GFx UI Transforms
//---------------------------------------------------------------------------------------------------------------------

// For passing around transform data to render thread
struct FGFxTransformHandles
{
    GRenderer::Matrix& UserMatrix;
    GRenderer::Matrix& ViewportMatrix;
    GRenderer::Matrix& CurrentMatrix;

    FGFxTransformHandles( GRenderer::Matrix& InUserMatrix,
        GRenderer::Matrix& InViewportMatrix, GRenderer::Matrix& InCurrentMatrix )
        : UserMatrix(InUserMatrix), ViewportMatrix(InViewportMatrix), CurrentMatrix(InCurrentMatrix) { }
};

//---------------------------------------------------------------------------------------------------------------------
// GFx UI Vertex and Index Stores
//---------------------------------------------------------------------------------------------------------------------

static inline SIZE_T GetUIRenderElementSize(GRenderer::VertexFormat format)
{
    SIZE_T byteSize = 0;
    check( sizeof(FGFxVertex_XY16i) == sizeof(FGFxVertex_Strip)  );

    switch(format)
    {
    case GRenderer::Vertex_None:        byteSize = 0; break;
    case GRenderer::Vertex_XY16i:       byteSize = sizeof( FGFxVertex_XY16i ); break;
    case GRenderer::Vertex_XY32f:       byteSize = sizeof( FGFxVertex_XY32F ); break;
    case GRenderer::Vertex_XY16iC32:    byteSize = sizeof( FGFxVertex_XY16iC32 ); break;
    case GRenderer::Vertex_XY16iCF32:   byteSize = sizeof( FGFxVertex_XY16iCF32 ); break;
    default: checkMsg( 0, "FGFxRenderer::GetUIRenderElementSize - Unrecognized vertex format" ); byteSize = 0; break;
    }
    return byteSize;
}

static inline SIZE_T GetUIRenderElementSize(GRenderer::IndexFormat format)
{
    SIZE_T byteSize = 0;
    switch(format)
    {
    case GRenderer::Index_None: byteSize = 0; break;
    case GRenderer::Index_16:   byteSize = 2; break;
    case GRenderer::Index_32:   byteSize = 4; break;
    default: checkMsg( 0, "FGFxRenderer::GetUIRenderElementSize - Unrecognized index format" );
        byteSize = 0; break;
    };
    return byteSize;
}

static inline SIZE_T GetUIRenderElementSize(GRenderer::BitmapDesc* pdummy)
{
    return sizeof(GRenderer::BitmapDesc);
}

// Context for applying shader parameters
struct FGFxRenderStyleContext
{
    GRenderer::VertexFormat VertexFmt;
    GRenderer::BlendType    BlendFmt;
};

class FGFxUpdatableTexture : public FTextureResource
{
public:
    UINT                Width, Height, Levels, Flags;
    EPixelFormat        TexFormat;
    FTexture2DRHIRef    Texture2DRHI;

    FGFxUpdatableTexture(UINT w, UINT h, UINT levels, EPixelFormat fmt, UINT flags)
    {
        Width = w; Height = h;
        Levels = levels;
        TexFormat = fmt;
        Flags = flags;
        bGreyScaleFormat = (fmt == PF_G8);
    }

    void InitRHI()
    {
        Texture2DRHI = RHICreateTexture2D(Width, Height, TexFormat, Levels, Flags, NULL);
        TextureRHI = Texture2DRHI;

        FSamplerStateInitializerRHI SamplerStateInitializer( SF_Bilinear, AM_Clamp, AM_Wrap );
        if (Levels > 1)
		{
            SamplerStateInitializer.Filter = SF_Trilinear;
		}
        SamplerStateRHI = RHICreateSamplerState( SamplerStateInitializer );
    }

    void ReleaseRHI()
    {
        FTextureResource::ReleaseRHI();
        Texture2DRHI.SafeRelease();
    }

    UINT GetSizeX() const    { return Width; }
    UINT GetSizeY() const    { return Height; }
};

class UGFxUpdatableTexture : public UTexture2D
{
public:
    FTextureResource* CreateResource()
    {
        return new FGFxUpdatableTexture(SizeX, SizeY, RequestedMips, EPixelFormat(Format), TexCreate_NoTiling);
    }

    virtual UBOOL UpdateStreamingStatus( UBOOL bWaitForMipFading = FALSE )
    {
        ResidentMips = RequestedMips;
        return FALSE;
    }

    UBOOL CancelPendingMipChangeRequest()
    {
        return FALSE;
    }
};

class UGFxMappableTexture : public UTexture2D
{
public:
    FTextureResource* CreateResource()
    {
        return new FGFxUpdatableTexture(SizeX, SizeY, RequestedMips, EPixelFormat(Format), TexCreate_NoTiling | TexCreate_Dynamic);
    }

    virtual UBOOL UpdateStreamingStatus( UBOOL bWaitForMipFading = FALSE )
    {
        ResidentMips = RequestedMips;
        return FALSE;
    }

    UBOOL CancelPendingMipChangeRequest()
    {
        return FALSE;
    }
};

#endif // WITH_GFx

#endif
