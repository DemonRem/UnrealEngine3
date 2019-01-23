/*=============================================================================
	FoliageVertexFactory.cpp: Foliage vertex factory implementation.
	Copyright 2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "FoliageVertexFactory.h"

void FFoliageVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	InvNumVerticesPerInstanceParameter.Bind(ParameterMap,TEXT("InvNumVerticesPerInstance"),TRUE);
	NumVerticesPerInstanceParameter.Bind(ParameterMap,TEXT("NumVerticesPerInstance"),TRUE);
}

void FFoliageVertexFactoryShaderParameters::Serialize(FArchive& Ar)
{
	Ar << InvNumVerticesPerInstanceParameter;
	Ar << NumVerticesPerInstanceParameter;
}

void FFoliageVertexFactoryShaderParameters::Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const
{
	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),InvNumVerticesPerInstanceParameter,1.0f / (FLOAT)VertexFactory->GetNumVerticesPerInstance());
	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),NumVerticesPerInstanceParameter,(FLOAT)VertexFactory->GetNumVerticesPerInstance());
}

void FFoliageVertexFactoryShaderParameters::SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const
{
}


UBOOL FFoliageVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return Material->IsUsedWithFoliage() || Material->IsSpecialEngineMaterial();
}

void FFoliageVertexFactory::InitRHI()
{
	FVertexDeclarationElementList Elements;

	Elements.AddItem(AccessStreamComponent(Data.PositionComponent,VEU_Position));

	EVertexElementUsage TangentBasisUsages[3] = { VEU_Tangent, VEU_Binormal, VEU_Normal };
	for(INT AxisIndex = 0;AxisIndex < 3;AxisIndex++)
	{
		if(Data.TangentBasisComponents[AxisIndex].VertexBuffer != NULL)
		{
			Elements.AddItem(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex],TangentBasisUsages[AxisIndex]));
		}
	}

	if(Data.TextureCoordinateComponent.VertexBuffer)
	{
		Elements.AddItem(AccessStreamComponent(Data.TextureCoordinateComponent,VEU_TextureCoordinate));
	}

	if(Data.ShadowMapCoordinateComponent.VertexBuffer)
	{
		Elements.AddItem(AccessStreamComponent(Data.ShadowMapCoordinateComponent,VEU_Color));
	}
	else if(Data.TextureCoordinateComponent.VertexBuffer)
	{
		Elements.AddItem(AccessStreamComponent(Data.TextureCoordinateComponent,VEU_Color));
	}

	Elements.AddItem(AccessStreamComponent(Data.InstanceOffsetComponent,VEU_TextureCoordinate,1));
	Elements.AddItem(AccessStreamComponent(Data.InstanceAxisComponents[0],VEU_TextureCoordinate,2));
	Elements.AddItem(AccessStreamComponent(Data.InstanceAxisComponents[1],VEU_TextureCoordinate,3));
	Elements.AddItem(AccessStreamComponent(Data.InstanceAxisComponents[2],VEU_TextureCoordinate,4));

	InitDeclaration(Elements,Data);
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FFoliageVertexFactory,FFoliageVertexFactoryShaderParameters,"FoliageVertexFactory",TRUE,TRUE, VER_FOLIAGE_VERTEX_FACTORY_INSTANCING_SHADER, 0);

