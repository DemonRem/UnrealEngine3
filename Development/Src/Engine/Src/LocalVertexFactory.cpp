/*=============================================================================
	LocalVertexFactory.cpp: Local vertex factory implementation
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "LocalVertexFactoryShaderParms.h"

/**
 * Should we cache the material's shadertype on this platform with this vertex factory? 
 */
UBOOL FLocalVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	//don't compile for terrain materials, since they are never rendered in preview windows
	return !Material->IsTerrainMaterial(); 
}

void FLocalVertexFactory::InitRHI()
{
	FVertexDeclarationElementList PositionOnlyStreamElements;
	if(Data.PositionComponent.VertexBuffer != Data.TangentBasisComponents[0].VertexBuffer)
	{
		PositionOnlyStreamElements.AddItem(AccessPositionStreamComponent(Data.PositionComponent,VEU_Position));
	}
	InitPositionDeclaration(PositionOnlyStreamElements);

	FVertexDeclarationElementList Elements;
	if(Data.PositionComponent.VertexBuffer != NULL)
	{
		Elements.AddItem(AccessStreamComponent(Data.PositionComponent,VEU_Position));
	}

	// only tangent,normal are used by the stream. the binormal is derived in the shader
	EVertexElementUsage TangentBasisUsages[2] = { VEU_Tangent, VEU_Normal };
	for(INT AxisIndex = 0;AxisIndex < 2;AxisIndex++)
	{
		if(Data.TangentBasisComponents[AxisIndex].VertexBuffer != NULL)
		{
			Elements.AddItem(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex],TangentBasisUsages[AxisIndex]));
		}
	}

	if(Data.ColorComponent.VertexBuffer)
	{
		Elements.AddItem(AccessStreamComponent(Data.ColorComponent,VEU_Color,1));
	}
	else
	{
		//If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
		//This wastes 4 bytes of bandwidth per vertex, but prevents having to compile out twice the number of vertex factories.
		FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color);
		Elements.AddItem(AccessStreamComponent(NullColorComponent,VEU_Color,1));
	}

	if(Data.TextureCoordinates.Num())
	{
		for(UINT CoordinateIndex = 0;CoordinateIndex < Data.TextureCoordinates.Num();CoordinateIndex++)
		{
			Elements.AddItem(AccessStreamComponent(
				Data.TextureCoordinates(CoordinateIndex),
				VEU_TextureCoordinate,
				CoordinateIndex
				));
		}

		for(UINT CoordinateIndex = Data.TextureCoordinates.Num();CoordinateIndex < MAX_TEXCOORDS;CoordinateIndex++)
		{
			Elements.AddItem(AccessStreamComponent(
				Data.TextureCoordinates(Data.TextureCoordinates.Num() - 1),
				VEU_TextureCoordinate,
				CoordinateIndex
				));
		}
	}

	if(Data.ShadowMapCoordinateComponent.VertexBuffer)
	{
		Elements.AddItem(AccessStreamComponent(Data.ShadowMapCoordinateComponent,VEU_Color));
	}
	else if(Data.TextureCoordinates.Num())
	{
		Elements.AddItem(AccessStreamComponent(Data.TextureCoordinates(0),VEU_Color));
	}

	InitDeclaration(Elements,Data);
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FLocalVertexFactory,FLocalVertexFactoryShaderParameters,"LocalVertexFactory",TRUE,TRUE, VER_REMOVE_BINORMAL_TANGENT_VECTOR,0);
IMPLEMENT_VERTEX_FACTORY_TYPE(FLocalShadowVertexFactory,FLocalVertexFactoryShaderParameters,"LocalVertexFactory",FALSE,FALSE, VER_REMOVE_BINORMAL_TANGENT_VECTOR,0);
