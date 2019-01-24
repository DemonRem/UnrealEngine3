/*================================================================================
	MeshPaintRendering.cpp: Mesh texture paint brush rendering
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
================================================================================*/

#include "UnrealEd.h"
#include "../../Engine/Src/ScenePrivate.h"
#include "MeshPaintRendering.h"


namespace MeshPaintRendering
{

	/** Mesh paint vertex shader */
	class TMeshPaintVertexShader
		: public FGlobalShader
	{
		DECLARE_SHADER_TYPE( TMeshPaintVertexShader, Global );

	public:

		static UBOOL ShouldCache( EShaderPlatform Platform )
		{
			return TRUE;
		}

		static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
		{
	//		OutEnvironment.Definitions.Set(TEXT("NUM_SAMPLES"),*FString::Printf(TEXT("%u"),NumSamples));
		}

		/** Default constructor. */
		TMeshPaintVertexShader() {}

		/** Initialization constructor. */
		TMeshPaintVertexShader( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
			: FGlobalShader( Initializer )
		{
			TransformParameter.Bind( Initializer.ParameterMap, TEXT( "c_Transform" ) );
		}

		/** Serializer */
		virtual UBOOL Serialize( FArchive& Ar )
		{
			UBOOL bShaderHasOutdatedParameters = FShader::Serialize( Ar );
			Ar << TransformParameter;
			return bShaderHasOutdatedParameters;
		}

		/** Sets shader parameter values */
		void SetParameters( const FMatrix& InTransform )
		{
			SetVertexShaderValue( GetVertexShader(), TransformParameter, InTransform );
		}


	private:

		FShaderParameter TransformParameter;
	};


	IMPLEMENT_SHADER_TYPE( , TMeshPaintVertexShader, TEXT( "MeshPaintVertexShader" ), TEXT( "Main" ), SF_Vertex, 0, 0 );



	/** Mesh paint pixel shader */
	class TMeshPaintPixelShader
		: public FGlobalShader
	{
		DECLARE_SHADER_TYPE( TMeshPaintPixelShader, Global );

	public:

		static UBOOL ShouldCache(EShaderPlatform Platform)
		{
			return TRUE;
		}

		static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
		{
	// 		OutEnvironment.Definitions.Set(TEXT("NUM_SAMPLES"),*FString::Printf(TEXT("%u"),NumSamples));
		}

		/** Default constructor. */
		TMeshPaintPixelShader() {}

		/** Initialization constructor. */
		TMeshPaintPixelShader( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
			: FGlobalShader( Initializer )
		{
			// @todo MeshPaint: These shader params should not be optional (remove TRUE)
			CloneTextureParameter.Bind( Initializer.ParameterMap, TEXT( "s_CloneTexture" ), TRUE );
			WorldToBrushMatrixParameter.Bind( Initializer.ParameterMap, TEXT( "c_WorldToBrushMatrix" ), TRUE );
			BrushMetricsParameter.Bind( Initializer.ParameterMap, TEXT( "c_BrushMetrics" ), TRUE );
			BrushStrengthParameter.Bind( Initializer.ParameterMap, TEXT( "c_BrushStrength" ), TRUE );
			BrushColorParameter.Bind( Initializer.ParameterMap, TEXT( "c_BrushColor" ), TRUE );
			GammaParameter.Bind( Initializer.ParameterMap, TEXT( "c_Gamma" ), TRUE );
		}

		/** Serializer */
		virtual UBOOL Serialize(FArchive& Ar)
		{
			UBOOL bShaderHasOutdatedParameters = FShader::Serialize(Ar);
			Ar << CloneTextureParameter;
			Ar << WorldToBrushMatrixParameter;
			Ar << BrushMetricsParameter;
			Ar << BrushStrengthParameter;
			Ar << BrushColorParameter;
			Ar << GammaParameter;
			return bShaderHasOutdatedParameters;
		}

		/** Sets shader parameter values */
		void SetParameters( const FLOAT InGamma, const FMeshPaintShaderParameters& InShaderParams )
		{
			SetTextureParameter(
				GetPixelShader(),
				CloneTextureParameter,
				TStaticSamplerState< SF_Point, AM_Clamp, AM_Clamp, AM_Clamp >::GetRHI(),
				InShaderParams.CloneTexture->GetRenderTargetResource()->TextureRHI );

			SetPixelShaderValue( GetPixelShader(), WorldToBrushMatrixParameter, InShaderParams.WorldToBrushMatrix );

			FVector4 BrushMetrics;
			BrushMetrics.X = InShaderParams.BrushRadius;
			BrushMetrics.Y = InShaderParams.BrushRadialFalloffRange;
			BrushMetrics.Z = InShaderParams.BrushDepth;
			BrushMetrics.W = InShaderParams.BrushDepthFalloffRange;
			SetPixelShaderValue( GetPixelShader(), BrushMetricsParameter, BrushMetrics );

			FVector4 BrushStrength4( InShaderParams.BrushStrength, 0.0f, 0.0f, 0.0f );
			SetPixelShaderValue( GetPixelShader(), BrushStrengthParameter, BrushStrength4 );

			SetPixelShaderValue( GetPixelShader(), BrushColorParameter, InShaderParams.BrushColor );

			// @todo MeshPaint
			SetPixelShaderValue( GetPixelShader(), GammaParameter, InGamma );
			//RHISetRenderTargetBias(appPow(2.0f,GCurrentColorExpBias));
		}


	private:

		/** Texture that is a clone of the destination render target before we start drawing */
		FShaderResourceParameter CloneTextureParameter;

		/** Brush -> World matrix */
		FShaderParameter WorldToBrushMatrixParameter;

		/** Brush metrics: x = radius, y = falloff range, z = depth, w = depth falloff range */
		FShaderParameter BrushMetricsParameter;

		/** Brush strength */
		FShaderParameter BrushStrengthParameter;

		/** Brush color */
		FShaderParameter BrushColorParameter;

		/** Gamma */
		// @todo MeshPaint: Remove this?
		FShaderParameter GammaParameter;
	};


	IMPLEMENT_SHADER_TYPE( , TMeshPaintPixelShader, TEXT( "MeshPaintPixelShader" ), TEXT( "Main" ), SF_Pixel, 0, 0 );




	/** Mesh paint vertex format */
	typedef FSimpleElementVertex FMeshPaintVertex;
// 	{
// 		FVector4 Position;
// 		FVector2D UV;
// 	};


	/** Mesh paint vertex declaration resource */
	typedef FSimpleElementVertexDeclaration FMeshPaintVertexDeclaration;
// 		: public FRenderResource
// 	{
// 	public:
// 		FVertexDeclarationRHIRef VertexDeclarationRHI;
// 
// 		/** Destructor. */
// 		virtual ~FMeshPaintVertexDeclaration() {}
// 
// 		virtual void InitRHI()
// 		{
// 			FVertexDeclarationElementList Elements;
// 			Elements.AddItem(FVertexElement(0,STRUCT_OFFSET(FMeshPaintVertex,Position),VET_Float4,VEU_Position,0));
// 			Elements.AddItem(FVertexElement(0,STRUCT_OFFSET(FMeshPaintVertex,TextureCoordinate),VET_Float2,VEU_TextureCoordinate,0));
// 			VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
// 		}
// 
// 		virtual void ReleaseRHI()
// 		{
// 			VertexDeclarationRHI.SafeRelease();
// 		}
// 	};




	/** Global mesh paint vertex declaration resource */
	TGlobalResource< FMeshPaintVertexDeclaration > GMeshPaintVertexDeclaration;




	/** Binds the mesh paint vertex and pixel shaders to the graphics device */
	void SetMeshPaintShaders_RenderThread( const FMatrix& InTransform,
										   const FLOAT InGamma,
										   const FMeshPaintShaderParameters& InShaderParams )
	{
		TShaderMapRef< TMeshPaintVertexShader > VertexShader( GetGlobalShaderMap() );

		// Set vertex shader parameters
		VertexShader->SetParameters( InTransform );

		TShaderMapRef< TMeshPaintPixelShader > PixelShader( GetGlobalShaderMap() );

		// Set pixel shader parameters
		PixelShader->SetParameters( InGamma, InShaderParams );


		// @todo MeshPaint: Make sure blending/color writes are setup so we can write to ALPHA if needed!

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState( BoundShaderState, GMeshPaintVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, sizeof( FMeshPaintVertex ) );
	}

}
