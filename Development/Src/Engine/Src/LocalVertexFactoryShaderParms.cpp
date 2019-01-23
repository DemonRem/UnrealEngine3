/*=============================================================================
	LocalVertexFactoryShaderParms.cpp: Local vertex factory shader parameters
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "LocalVertexFactoryShaderParms.h"

void FLocalVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	LocalToWorldParameter.Bind(ParameterMap,TEXT("LocalToWorld"));
	WorldToLocalParameter.Bind(ParameterMap,TEXT("WorldToLocal"),TRUE);
}

void FLocalVertexFactoryShaderParameters::Serialize(FArchive& Ar)
{
	Ar << LocalToWorldParameter;
	Ar << WorldToLocalParameter;
}

void FLocalVertexFactoryShaderParameters::Set(FCommandContextRHI* Context,FShader* VertexShader,const FVertexFactory* VertexFactory,const FSceneView* View) const
{	
}

void FLocalVertexFactoryShaderParameters::SetLocalTransforms(FCommandContextRHI* Context,FShader* VertexShader,const FMatrix& LocalToWorld,const FMatrix& WorldToLocal) const
{
	SetVertexShaderValue(Context,VertexShader->GetVertexShader(),LocalToWorldParameter,LocalToWorld);
	SetVertexShaderValues(Context,VertexShader->GetVertexShader(),WorldToLocalParameter,(FVector4*)&WorldToLocal,3);
}
