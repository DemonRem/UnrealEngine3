/*=============================================================================
	UnDOFEffect.cpp: DOF (Depth of Field) post process effect implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"

IMPLEMENT_CLASS(UDOFEffect);

/*-----------------------------------------------------------------------------
DOF shaders
-----------------------------------------------------------------------------*/

/**
* A vertex shader for rendering the full screen depth of field effect
*/
class FDepthOfFieldVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDepthOfFieldVertexShader,Global);
public:

	/** 
	* Always cache this global shader
	* @return TRUE if this shader should be cached 
	*/
	static UBOOL ShouldCache(EShaderPlatform Platform) 
	{ 
		return TRUE; 
	}

	/**
	* Constructor
	* @param Initializer - info about compiled shader; parameter map,byte code,etc
	*/
	FDepthOfFieldVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
		ScreenPositionScaleBiasParameter.Bind(Initializer.ParameterMap,TEXT("ScreenPositionScaleBias"));
	}
	
	/**
	* Destructor 
	*/
	FDepthOfFieldVertexShader() 
	{
	}

	/**
	* Sets the current shader
	* @param Transform - 4x4 amtrix transform to apply to verts
	*/
	void SetParameters(FCommandContextRHI* Context,const FViewInfo& View)
	{
		// Set the transform from screen coordinates to scene texture coordinates.
		// NOTE: Need to set explicitly, since this is a vertex shader!
		SetVertexShaderValue(Context,GetVertexShader(),ScreenPositionScaleBiasParameter,View.ScreenPositionScaleBias);
	}

    /**
	* Serialize the shader params to an archive
	* @param Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << ScreenPositionScaleBiasParameter;
	}

private:	
	FShaderParameter ScreenPositionScaleBiasParameter;
};
IMPLEMENT_SHADER_TYPE(,FDepthOfFieldVertexShader,TEXT("DepthOfFieldVertexShader"),TEXT("Main"),SF_Vertex,0,0);

/**
* A pixel shader for rendering the full screen depth of field effect
*/
class FDepthOfFieldPixelShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDepthOfFieldPixelShader,Global);
public:

	/** DOF specific parameter for shader constants */
	struct FDOFShaderParams
	{
		FTextureRHIParamRef SceneColorTexture;
		FTextureRHIParamRef SceneBlurTexture;
		FTextureRHIParamRef SceneDepthTexture;
		FVector4 MinZ_MaxZRatio;
		FVector2D BlurTexelSize;
		FLOAT FocusDistance;
		FLOAT FocusRadius;
		FLOAT FalloffExponent;
		FLOAT BlurKernelSize;
		FVector2D MinMaxBlurClamp;
		FLinearColor ModulateBlurColor;
	};

	/** 
	* Always cache this global shader
	* @return TRUE if this shader should be cached 
	*/
	static UBOOL ShouldCache(EShaderPlatform Platform) 
	{ 
		return TRUE; 
	}

	/**
	* Constructor
	* @param Initializer - info about compiled shader; parameter map,byte code,etc
	*/
	FDepthOfFieldPixelShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FGlobalShader(Initializer)
	{
		// bind constants, none of these are optional
		SceneBlurTextureParam.Bind(Initializer.ParameterMap,TEXT("SceneBlurTexture"));
		SceneColorTextureParam.Bind(Initializer.ParameterMap,TEXT("SceneColorTexture"));		
		BlurTexelSizeParam.Bind(Initializer.ParameterMap,TEXT("BlurTexelSize"));
		FocusDistanceParam.Bind(Initializer.ParameterMap,TEXT("FocusDistance"));
		FocusRadiusParam.Bind(Initializer.ParameterMap,TEXT("FocusRadius"));
		FalloffExponentParam.Bind(Initializer.ParameterMap,TEXT("FalloffExponent"));
		BlurKernelSizeParam.Bind(Initializer.ParameterMap,TEXT("BlurKernelSize"));
		MinMaxBlurClampParam.Bind(Initializer.ParameterMap,TEXT("MinMaxBlurClamp"));
		ModulateBlurColorParam.Bind(Initializer.ParameterMap,TEXT("ModulateBlurColor"));
		// only used by shader code on certain platforms
		SceneDepthTextureParam.Bind(Initializer.ParameterMap,TEXT("SceneDepthTexture"),TRUE);
		SceneDepthCalcParam.Bind(Initializer.ParameterMap,TEXT("MinZ_MaxZRatio"),TRUE);
	}

	/**
	* Destructor 
	*/
	FDepthOfFieldPixelShader() 
	{
	}

	/**
	* Sets the current shader
	* @param ShaderParams - struct of DoF specific shader values
	*/
	void SetParameters(FCommandContextRHI* Context,const FDOFShaderParams& ShaderParams)
	{
		// set shader constants 
		SetTextureParameter(
			Context,
			GetPixelShader(),
			SceneBlurTextureParam,
			TStaticSamplerState<SF_Linear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			ShaderParams.SceneBlurTexture
			);
		SetTextureParameter(
			Context,
			GetPixelShader(),
			SceneColorTextureParam,
			TStaticSamplerState<SF_Nearest,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			ShaderParams.SceneColorTexture
			);
		SetTextureParameter(
			Context,
			GetPixelShader(),
			SceneDepthTextureParam,
			TStaticSamplerState<SF_Nearest,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			ShaderParams.SceneDepthTexture
			);
		// NOTE: Setting PSR_MinZ_MaxZ_Ratio manually here, because RHISetViewPixelParameters() is only a fallback and may not do anything!
		SetPixelShaderValue(Context,GetPixelShader(),SceneDepthCalcParam,ShaderParams.MinZ_MaxZRatio);
		SetPixelShaderValue(Context,GetPixelShader(),BlurTexelSizeParam,ShaderParams.BlurTexelSize);
		SetPixelShaderValue(Context,GetPixelShader(),FocusDistanceParam,ShaderParams.FocusDistance);
		SetPixelShaderValue(Context,GetPixelShader(),FocusRadiusParam,ShaderParams.FocusRadius);
		SetPixelShaderValue(Context,GetPixelShader(),FalloffExponentParam,ShaderParams.FalloffExponent);
		SetPixelShaderValue(Context,GetPixelShader(),BlurKernelSizeParam,ShaderParams.BlurKernelSize);
		SetPixelShaderValue(Context,GetPixelShader(),MinMaxBlurClampParam,ShaderParams.MinMaxBlurClamp);
		SetPixelShaderValue(Context,GetPixelShader(),ModulateBlurColorParam,ShaderParams.ModulateBlurColor);
	}

	/**
	* Serialize the shader params to an archive
	* @param Ar - archive to serialize to
	*/
	virtual void Serialize(FArchive& Ar)
	{
		FShader::Serialize(Ar);
		Ar << SceneBlurTextureParam;
		Ar << SceneColorTextureParam;
		Ar << SceneDepthTextureParam;
		Ar << SceneDepthCalcParam;
		Ar << BlurTexelSizeParam;
		Ar << FocusDistanceParam;
		Ar << FocusRadiusParam;
		Ar << FalloffExponentParam;
		Ar << BlurKernelSizeParam;
		Ar << MinMaxBlurClampParam;
		Ar << ModulateBlurColorParam;
	}

private:
	/** see DepthOfFieldPixelShader.usf for descriptions */
	FShaderParameter SceneBlurTextureParam;
	FShaderParameter SceneColorTextureParam;
	FShaderParameter SceneDepthTextureParam;
	FShaderParameter SceneDepthCalcParam;
	FShaderParameter BlurTexelSizeParam;
	FShaderParameter FocusDistanceParam;
	FShaderParameter FocusRadiusParam;
	FShaderParameter FalloffExponentParam;
	FShaderParameter BlurKernelSizeParam;
	FShaderParameter MinMaxBlurClampParam;
	FShaderParameter ModulateBlurColorParam;
};
IMPLEMENT_SHADER_TYPE(,FDepthOfFieldPixelShader,TEXT("DepthOfFieldPixelShader"),TEXT("Main"),SF_Pixel,VER_FP_BLENDING_FALLBACK,0);

extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;

/*-----------------------------------------------------------------------------
FDepthOfFieldPostProcessSceneProxy
-----------------------------------------------------------------------------*/

class FDepthOfFieldPostProcessSceneProxy : public FPostProcessSceneProxy
{
public:
	/** 
	* Initialization constructor. 
	* @param InEffect - DOF post process effect to mirror in this proxy
	*/
	FDepthOfFieldPostProcessSceneProxy(const UDOFEffect* InEffect,const FPostProcessSettings* WorldSettings)
		:	FPostProcessSceneProxy(InEffect)
		,	FalloffExponent(WorldSettings ? WorldSettings->DOF_FalloffExponent : InEffect->FalloffExponent)
		,	BlurKernelSize(WorldSettings ? WorldSettings->DOF_BlurKernelSize : InEffect->BlurKernelSize)
		,	MaxNearBlurAmount(WorldSettings ? WorldSettings->DOF_MaxNearBlurAmount : InEffect->MaxNearBlurAmount)
		,	MaxFarBlurAmount(WorldSettings ? WorldSettings->DOF_MaxFarBlurAmount : InEffect->MaxFarBlurAmount)
		,	ModulateBlurColor(WorldSettings ? WorldSettings->DOF_ModulateBlurColor : InEffect->ModulateBlurColor)
		,	FocusType(WorldSettings ? WorldSettings->DOF_FocusType : InEffect->FocusType)
		,	FocusInnerRadius(WorldSettings ? WorldSettings->DOF_FocusInnerRadius : InEffect->FocusInnerRadius)
		,	FocusDistance(WorldSettings ? WorldSettings->DOF_FocusDistance : InEffect->FocusDistance)
		,	FocusPosition(WorldSettings ? WorldSettings->DOF_FocusPosition : InEffect->FocusPosition)
	{
	}

	/**
	* Render the post process effect
	* Called by the rendering thread during scene rendering
	* @param InDepthPriorityGroup - scene DPG currently being rendered
	* @param View - current view
	* @param CanvasTransform - same canvas transform used to render the scene
	* @return TRUE if anything was rendered
	*/
	UBOOL Render(FCommandContextRHI* Context, UINT InDepthPriorityGroup,FViewInfo& View,const FMatrix& CanvasTransform)
	{
		SCOPED_DRAW_EVENT(Event)(DEC_SCENE_ITEMS,TEXT("DOF"));

		UBOOL bDirty=TRUE;

		// Downsample the scene into the blur buffer.
		DrawDownsampledTexture(Context, View);

		// post process effects are rendered to the scene color
		GSceneRenderTargets.BeginRenderingSceneColor();

		// get the view direction vector (normalized)
		FPlane ViewDir( 
			View.ViewMatrix.M[0][2], 
			View.ViewMatrix.M[1][2], 
			View.ViewMatrix.M[2][2], 
			View.ViewMatrix.M[3][2]	);
		UBOOL bIsValid = ViewDir.Normalize();	
		// calc the focus point
		FPlane FocusPoint(0.f,0.f,0.f,1.f);
		switch( FocusType )
		{
		case FOCUS_Distance:
			{
				// focus point based on view distance
				FocusPoint = FPlane( (ViewDir * FocusDistance) + View.ViewOrigin, 1.f );
			}		
			break;
		case FOCUS_Position:
			{
				// world space focus point specified projected onto the view direction
				FocusPoint = FPlane((((FocusPosition - View.ViewOrigin) | ViewDir) * ViewDir) + View.ViewOrigin, 1.f );
			}		
			break;
		default:
			check(0 && "Invalid focus type");
		};

		// far point of focus (used to calculate radius)
		FPlane FocusFarInner( (ViewDir * FocusInnerRadius) + FocusPoint, 1.f );

		// transform to projected space in order to get w depth values
		// focus point w depth - clamped to 0 (near plane)
		FLOAT FocusPointDepth = Max( 0.f, View.WorldToScreen( FocusPoint ).W );
		// focus far w depth - shouldn't ever be less than the focus depth
		FLOAT FocusFarDepth = Max( FocusPointDepth, View.WorldToScreen( FocusFarInner ).W );
		// radius w depth - shouldn't == 0
		FLOAT Radius = Max( (FLOAT)KINDA_SMALL_NUMBER, Abs(FocusPointDepth - FocusFarDepth) );

		FDepthOfFieldPixelShader::FDOFShaderParams ShaderParams =
		{
            GSceneRenderTargets.GetSceneColorTexture(),
			GSceneRenderTargets.GetFilterColorTexture(),
			GSceneRenderTargets.GetSceneDepthTexture(),
			View.InvDeviceZToWorldZTransform,
			FVector2D(
				1.0f / (FLOAT)GSceneRenderTargets.GetFilterBufferSizeX(),
				1.0f / (FLOAT)GSceneRenderTargets.GetFilterBufferSizeY()
				),	// @dof sz
			FocusPointDepth,
			Radius,
			FalloffExponent,
			BlurKernelSize,
			FVector2D(MaxNearBlurAmount,MaxFarBlurAmount),
			FLinearColor(ModulateBlurColor)
		};

		TShaderMapRef<FDepthOfFieldVertexShader> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FDepthOfFieldPixelShader> PixelShader(GetGlobalShaderMap());

		SetGlobalBoundShaderState(Context, BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, sizeof(FFilterVertex));

		VertexShader->SetParameters(Context,View);
		PixelShader->SetParameters(Context,ShaderParams);
	
		// disable depth test & writes
		RHISetDepthState(Context,TStaticDepthState<FALSE,CF_Always>::GetRHI());		
		// default fill/cull mode
		RHISetRasterizerState(Context,TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		// set blend state
		RHISetBlendState(Context,TStaticBlendState<>::GetRHI());

		RHISetViewport(Context,View.RenderTargetX,View.RenderTargetY,0,View.RenderTargetSizeX,View.RenderTargetSizeY,1.0f);

		DrawDenormalizedQuad(
			Context,
			0,0,
			View.RenderTargetSizeX,View.RenderTargetSizeY,
			1,1,
			(FLOAT)View.RenderTargetSizeX / (FLOAT)GSceneRenderTargets.GetFilterDownsampleFactor(),
			(FLOAT)View.RenderTargetSizeY / (FLOAT)GSceneRenderTargets.GetFilterDownsampleFactor(),
			View.RenderTargetSizeX,View.RenderTargetSizeY,
			GSceneRenderTargets.GetFilterBufferSizeX(),GSceneRenderTargets.GetFilterBufferSizeY()
			);

        // scene color is resolved if something was rendered
		GSceneRenderTargets.FinishRenderingSceneColor(bDirty);

		return bDirty;
	}

private:

	/** cached bound shader state for DoF filtering */
	static FGlobalBoundShaderStateRHIRef BoundShaderState;

	/** mirrored properties. See DOFEffect.uc for descriptions */
	FLOAT FalloffExponent;
	FLOAT BlurKernelSize;
	FLOAT MaxNearBlurAmount;
	FLOAT MaxFarBlurAmount;
	FColor ModulateBlurColor;
	BYTE FocusType;
	FLOAT FocusInnerRadius;
	FLOAT FocusDistance;
	FVector FocusPosition;
};

FGlobalBoundShaderStateRHIRef FDepthOfFieldPostProcessSceneProxy::BoundShaderState;

/*-----------------------------------------------------------------------------
UDOFEffect
-----------------------------------------------------------------------------*/

/** callback for changed property */
void UDOFEffect::PostEditChange(UProperty* PropertyThatChanged)
{
	MaxNearBlurAmount = Clamp<FLOAT>( MaxNearBlurAmount, 0.f, 1.f );
	MaxFarBlurAmount = Clamp<FLOAT>( MaxFarBlurAmount, 0.f, 1.f );

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Creates a proxy to represent the render info for a post process effect
 * @param WorldSettings - The world's post process settings for the view.
 * @return The proxy object.
 */
FPostProcessSceneProxy* UDOFEffect::CreateSceneProxy(const FPostProcessSettings* WorldSettings)
{
	if(!WorldSettings || WorldSettings->bEnableDOF)
	{
		return new FDepthOfFieldPostProcessSceneProxy(this,WorldSettings);
	}
	else
	{
		return NULL;
	}
}

/**
 * @param View - current view
 * @return TRUE if the effect should be rendered
 */
UBOOL UDOFEffect::IsShown(const FSceneView* View) const
{
	return GSystemSettings.bAllowDepthOfField && Super::IsShown( View );
}


