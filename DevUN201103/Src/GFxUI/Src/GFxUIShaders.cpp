IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2InnerShadow>,TEXT("GFxFilterPixelShader"),TEXT("FBox2InnerShadow"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2InnerShadowHighlight>,TEXT("GFxFilterPixelShader"),TEXT("FBox2InnerShadowHighlight"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2InnerShadowMul>,TEXT("GFxFilterPixelShader"),TEXT("FBox2InnerShadowMul"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2InnerShadowMulHighlight>,TEXT("GFxFilterPixelShader"),TEXT("FBox2InnerShadowMulHighlight"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2InnerShadowKnockout>,TEXT("GFxFilterPixelShader"),TEXT("FBox2InnerShadowKnockout"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2InnerShadowHighlightKnockout>,TEXT("GFxFilterPixelShader"),TEXT("FBox2InnerShadowHighlightKnockout"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2InnerShadowMulKnockout>,TEXT("GFxFilterPixelShader"),TEXT("FBox2InnerShadowMulKnockout"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2InnerShadowMulHighlightKnockout>,TEXT("GFxFilterPixelShader"),TEXT("FBox2InnerShadowMulHighlightKnockout"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2Shadow>,TEXT("GFxFilterPixelShader"),TEXT("FBox2Shadow"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowHighlight>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowHighlight"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowMul>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowMul"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowMulHighlight>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowMulHighlight"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowKnockout>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowKnockout"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowHighlightKnockout>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowHighlightKnockout"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowMulKnockout>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowMulKnockout"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowMulHighlightKnockout>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowMulHighlightKnockout"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2Shadowonly>,TEXT("GFxFilterPixelShader"),TEXT("FBox2Shadowonly"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowonlyHighlight>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowonlyHighlight"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowonlyMul>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowonlyMul"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2ShadowonlyMulHighlight>,TEXT("GFxFilterPixelShader"),TEXT("FBox2ShadowonlyMulHighlight"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2Blur>,TEXT("GFxFilterPixelShader"),TEXT("FBox2Blur"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox2BlurMul>,TEXT("GFxFilterPixelShader"),TEXT("FBox2BlurMul"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox1Blur>,TEXT("GFxFilterPixelShader"),TEXT("FBox1Blur"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FBox1BlurMul>,TEXT("GFxFilterPixelShader"),TEXT("FBox1BlurMul"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FCMatrix>,TEXT("GFxFilterPixelShader"),TEXT("FCMatrix"),SF_Pixel,0,0);
IMPLEMENT_SHADER_TYPE(template<>,FGFxFilterPixelShader<FS2_FCMatrixMul>,TEXT("GFxFilterPixelShader"),TEXT("FCMatrixMul"),SF_Pixel,0,0);

FGFxPixelShaderInterface* GetUIPixelShaderInterface2_RenderThread( const EGFxPixelShaderType ShaderType )
{
    switch( ShaderType )
    {
    case FS2_FBox2InnerShadow:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2InnerShadow> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2InnerShadowHighlight:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2InnerShadowHighlight> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2InnerShadowMul:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2InnerShadowMul> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2InnerShadowMulHighlight:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2InnerShadowMulHighlight> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2InnerShadowKnockout:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2InnerShadowKnockout> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2InnerShadowHighlightKnockout:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2InnerShadowHighlightKnockout> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2InnerShadowMulKnockout:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2InnerShadowMulKnockout> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2InnerShadowMulHighlightKnockout:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2InnerShadowMulHighlightKnockout> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2Shadow:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2Shadow> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowHighlight:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowHighlight> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowMul:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowMul> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowMulHighlight:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowMulHighlight> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowKnockout:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowKnockout> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowHighlightKnockout:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowHighlightKnockout> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowMulKnockout:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowMulKnockout> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowMulHighlightKnockout:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowMulHighlightKnockout> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2Shadowonly:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2Shadowonly> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowonlyHighlight:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowonlyHighlight> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowonlyMul:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowonlyMul> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2ShadowonlyMulHighlight:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2ShadowonlyMulHighlight> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2Blur:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2Blur> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox2BlurMul:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox2BlurMul> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox1Blur:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox1Blur> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FBox1BlurMul:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FBox1BlurMul> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FCMatrix:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FCMatrix> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }
    case FS2_FCMatrixMul:
    {
        TShaderMapRef< FGFxFilterPixelShader<FS2_FCMatrixMul> > NativeShader( GetGlobalShaderMap() );
        check( NULL != *NativeShader );
        return NativeShader->GetShaderInterface();
    }

    default: checkMsg( 0, "GetUIPixelShaderInterface_RenderThread: Undefined UI Pixel Shader Type" ); break;
    }
    return NULL;
}
