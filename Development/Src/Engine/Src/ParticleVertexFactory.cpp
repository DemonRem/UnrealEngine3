/*=============================================================================
	ParticleVertexFactory.cpp: Particle vertex factory implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "EngineParticleClasses.h"

/** The RHI vertex declaration used to render the factory normally. */
FVertexDeclarationRHIRef FParticleVertexFactory::SpriteParticleDeclaration;

UBOOL FParticleVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return Material->IsUsedWithParticleSprites() || Material->IsSpecialEngineMaterial();
}

/**
 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
 */
void FParticleVertexFactory::ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(OutEnvironment);
#if 1
	OutEnvironment.Definitions.Set(TEXT("PARTICLES_ALLOW_AXIS_ROTATION"),TEXT("1"));
	// There are only 2 slots required for the axis rotation vectors.
	OutEnvironment.Definitions.Set(TEXT("NUM_AXIS_ROTATION_VECTORS"),TEXT("2"));
#endif	//#if defined _PARTICLES_ALLOW_AXIS_ROTATIONS_
}

/**
 *	Initialize the Render Hardare Interface for this vertex factory
 */
void FParticleVertexFactory::InitRHI()
{
	CreateCachedSpriteParticleDeclaration();
	SetDeclaration(SpriteParticleDeclaration);
}

UBOOL FParticleVertexFactory::CreateCachedSpriteParticleDeclaration()
{
	if (IsValidRef(SpriteParticleDeclaration) == FALSE)
	{
		FVertexDeclarationElementList Elements;

		INT	Offset = 0;
		/** The stream to read the vertex position from.		*/
		Elements.AddItem(FVertexElement(0,Offset,VET_Float3,VEU_Position,0));
		Offset += sizeof(FLOAT) * 3;
		/** The stream to read the vertex old position from.	*/
		Elements.AddItem(FVertexElement(0,Offset,VET_Float3,VEU_Normal,0));
		Offset += sizeof(FLOAT) * 3;
		/** The stream to read the vertex size from.			*/
		Elements.AddItem(FVertexElement(0,Offset,VET_Float3,VEU_Tangent,0));
		Offset += sizeof(FLOAT) * 3;
		/** The stream to read the texture coordinates from.	*/
		Elements.AddItem(FVertexElement(0,Offset,VET_Float2,VEU_TextureCoordinate,0));
		Offset += sizeof(FLOAT) * 2;
		/** The stream to read the rotation from.				*/
		Elements.AddItem(FVertexElement(0,Offset,VET_Float1,VEU_BlendWeight,0));
		Offset += sizeof(FLOAT) * 1;
		/** The stream to read the color from.					*/
		Elements.AddItem(FVertexElement(0,Offset,VET_Float4,VEU_TextureCoordinate,1));
		Offset += sizeof(FLOAT) * 4;

		// Create the vertex declaration for rendering the factory normally.
		SpriteParticleDeclaration = RHICreateVertexDeclaration(Elements);
	}

	return TRUE;
}

void FParticleVertexFactoryShaderParameters::Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const
{
	FParticleVertexFactory* ParticleVF = (FParticleVertexFactory*)VertexFactory;

	FVector4 CameraRight, CameraUp;
	FVector UpRightScalarParam(0.0f);

	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),CameraWorldPositionParameter,FVector4(View->ViewOrigin, 0.0f));
	BYTE LockAxisFlag = ParticleVF->GetLockAxisFlag();
	if (LockAxisFlag == EPAL_NONE)
	{
		CameraUp	= -View->InvViewProjectionMatrix.TransformNormal(FVector(1.0f,0.0f,0.0f)).SafeNormal();
		CameraRight	= -View->InvViewProjectionMatrix.TransformNormal(FVector(0.0f,1.0f,0.0f)).SafeNormal();

		if (ParticleVF->GetScreenAlignment() == PSA_Velocity)
		{
			UpRightScalarParam.Y = 1.0f;
		}
		else
		{
			UpRightScalarParam.X = 1.0f;
		}
	}
	else
	if ((LockAxisFlag >= EPAL_ROTATE_X) && (LockAxisFlag <= EPAL_ROTATE_Z))
	{
		switch (LockAxisFlag)
		{
		case EPAL_ROTATE_X:
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorsArrayParameter, FVector4( 1.0f, 0.0f, 0.0f, 1.0f), 0);
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorsArrayParameter, FVector4( 0.0f, 0.0f, 0.0f, 0.0f), 1);
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorSourceIndexParameter, 0.0f);
			UpRightScalarParam.Z = 1.0f;
			break;
		case EPAL_ROTATE_Y:
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorsArrayParameter, FVector4( 0.0f, 1.0f, 0.0f, 1.0f), 0);
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorsArrayParameter, FVector4( 0.0f, 0.0f, 0.0f, 0.0f), 1);
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorSourceIndexParameter, 0.0f);
			UpRightScalarParam.Z = 1.0f;
			break;
		case EPAL_ROTATE_Z:
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorsArrayParameter, FVector4( 0.0f, 0.0f, 0.0f, 0.0f), 0);
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorsArrayParameter, FVector4( 0.0f, 0.0f,-1.0f,-1.0f), 1);
			SetVertexShaderValue(Context, VertexShader->GetVertexShader(), AxisRotationVectorSourceIndexParameter, 1.0f);
			UpRightScalarParam.Z = 1.0f;
			break;
		}
	}
	else
	{
		CameraUp	= FVector4(ParticleVF->GetLockAxisUp(), 0.0f);
		CameraRight	= FVector4(ParticleVF->GetLockAxisRight(), 0.0f);
		UpRightScalarParam.X = 1.0f;
	}

	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),CameraRightParameter,CameraRight);
	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),CameraUpParameter,CameraUp);
	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),ScreenAlignmentParameter,FVector4((FLOAT)ParticleVF->GetScreenAlignment(),0.0f,0.0f,0.0f));
	SetVertexShaderValue(Context, VertexShader->GetVertexShader(), ParticleUpRightResultScalarsParameter, UpRightScalarParam);
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FParticleVertexFactory,FParticleVertexFactoryShaderParameters,"ParticleSpriteVertexFactory",TRUE,FALSE, VER_PARTICLE_SPRITE_SUBUV_MERGE,0);
