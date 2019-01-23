/**
 *	LensFlareVertexFactory.cpp: Lens flare vertex factory implementation.
 *	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "EnginePrivate.h"
#include "LensFlare.h"

/**
 *	
 */
/** The RHI vertex declaration used to render the factory normally. */
FVertexDeclarationRHIRef FLensFlareVertexFactory::LensFlareDeclaration;

// FRenderResource interface.
void FLensFlareVertexFactory::InitRHI()
{
	CreateCachedLensFlareDeclaration();
	SetDeclaration(LensFlareDeclaration);
}

void FLensFlareVertexFactory::ReleaseRHI()
{
	FRenderResource::ReleaseRHI();
}

/**
 * Should we cache the material's shadertype on this platform with this vertex factory? 
 */
UBOOL FLensFlareVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return Material->IsUsedWithLensFlare() || Material->IsSpecialEngineMaterial();
}

/**
 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
 */
void FLensFlareVertexFactory::ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(OutEnvironment);
	OutEnvironment.Definitions.Set(TEXT("USE_LENSFLARE"),TEXT("1"));
}

void FLensFlareVertexFactory::FillElementList(FVertexDeclarationElementList& Elements, INT& Offset)
{
	Offset = 0;
	/** The stream to read the vertex position from.		*/
	Elements.AddItem(FVertexElement(0,Offset,VET_Float4,VEU_Position,0));
	Offset += sizeof(FLOAT) * 4;
	/** The stream to read the vertex size from.			*/
	Elements.AddItem(FVertexElement(0,Offset,VET_Float4,VEU_Tangent,0));
	Offset += sizeof(FLOAT) * 4;
	/** The stream to read the rotation from.				*/
	Elements.AddItem(FVertexElement(0,Offset,VET_Float1,VEU_BlendWeight,0));
	Offset += sizeof(FLOAT) * 1;
	/** The stream to read the texture coordinates from.	*/
	Elements.AddItem(FVertexElement(0,Offset,VET_Float2,VEU_TextureCoordinate,0));
	Offset += sizeof(FLOAT) * 2;
	/** The stream to read the color from.					*/
	Elements.AddItem(FVertexElement(0,Offset,VET_Float4,VEU_TextureCoordinate,1));
	Offset += sizeof(FLOAT) * 4;
	/** The stream to read the radial_dist/source_ratio/ray distance from*/
	Elements.AddItem(FVertexElement(0,Offset,VET_Float3,VEU_TextureCoordinate,2));
	Offset += sizeof(FLOAT) * 3;
	/** The stream to read the occlusion/intensity from*/
	Elements.AddItem(FVertexElement(0,Offset,VET_Float2,VEU_TextureCoordinate,3));
	Offset += sizeof(FLOAT) * 2;
}

UBOOL FLensFlareVertexFactory::CreateCachedLensFlareDeclaration()
{
	if (IsValidRef(LensFlareDeclaration) == FALSE)
	{
		FVertexDeclarationElementList Elements;

		INT	Offset = 0;
		FillElementList(Elements, Offset);

		// Create the vertex declaration for rendering the factory normally.
		LensFlareDeclaration = RHICreateVertexDeclaration(Elements);
	}

	return TRUE;
}

/**
 *	
 */
void FLensFlareVertexFactoryShaderParameters::Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const
{
	FVector4 CameraRight, CameraUp;
	FVector UpRightScalarParam(0.0f);

	CameraUp	= -View->InvViewProjectionMatrix.TransformNormal(FVector(1.0f,0.0f,0.0f)).SafeNormal();
	CameraRight	= -View->InvViewProjectionMatrix.TransformNormal(FVector(0.0f,1.0f,0.0f)).SafeNormal();

	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),CameraRightParameter,CameraRight);
	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),CameraUpParameter,CameraUp);
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FLensFlareVertexFactory,FLensFlareVertexFactoryShaderParameters,"LensFlareVertexFactory",TRUE,FALSE, VER_ADDED_LENSFLARE_OCCLUSION,0);
