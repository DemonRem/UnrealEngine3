/*=============================================================================
	ParticleSubUVVertexFactory.cpp: Particle vertex factory implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"

/** The RHI vertex declaration used to render the factory normally. */
FVertexDeclarationRHIRef FParticleSubUVVertexFactory::SubUVParticleDeclaration;

UBOOL FParticleSubUVVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return Material->IsUsedWithParticleSubUV() || Material->IsSpecialEngineMaterial();
}

/**
 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
 */
void FParticleSubUVVertexFactory::ModifyCompilationEnvironment(FShaderCompilerEnvironment& OutEnvironment)
{
	FParticleVertexFactory::ModifyCompilationEnvironment(OutEnvironment);
	OutEnvironment.Definitions.Set(TEXT("SUBUV_PARTICLES"),TEXT("1"));
}

/**
 *	Initialize the Render Hardare Interface for this vertex factory
 */
void FParticleSubUVVertexFactory::InitRHI()
{
	CreateCachedSubUVParticleDeclaration();
	SetDeclaration(SubUVParticleDeclaration);
}

UBOOL FParticleSubUVVertexFactory::CreateCachedSubUVParticleDeclaration()
{
	if (IsValidRef(SubUVParticleDeclaration) == FALSE)
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
		Elements.AddItem(FVertexElement(0,Offset,VET_Float4,VEU_TextureCoordinate,3));
		Offset += sizeof(FLOAT) * 4;
		/** The stream to read the second UV set from.			*/
		Elements.AddItem(FVertexElement(0,Offset,VET_Float2,VEU_TextureCoordinate,1));
		Offset += sizeof(FLOAT) * 2;
		/** The stream to read the interoplation value from.	*/
		/** SHARED WITH the size scaling information.			*/
		Elements.AddItem(FVertexElement(0,Offset,VET_Float4,VEU_TextureCoordinate,2));
		Offset += sizeof(FLOAT) * 4;

		// Create the vertex declaration for rendering the factory normally.
		SubUVParticleDeclaration = RHICreateVertexDeclaration(Elements);
	}

	return TRUE;
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FParticleSubUVVertexFactory,FParticleVertexFactoryShaderParameters,"ParticleSpriteVertexFactory",TRUE,FALSE, VER_PARTICLE_SPRITE_SUBUV_MERGE,0);
