/*=============================================================================
	LocalDecalVertexFactory.cpp: Local vertex factory bound to a shader that computes decal texture coordinates.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "LocalVertexFactoryShaderParms.h"
#include "LocalDecalVertexFactory.h"

/**
 * Shader parameters for use with FGPUSkinDecalVertexFactory.
 */
class FLocalDecalVertexFactoryShaderParameters : public FLocalVertexFactoryShaderParameters
{
public:
	typedef FLocalVertexFactoryShaderParameters Super;

	/**
	 * Bind shader constants by name
	 * @param	ParameterMap - mapping of named shader constants to indices
	 */
	virtual void Bind(const FShaderParameterMap& ParameterMap)
	{
		Super::Bind( ParameterMap );
		BoneToDecalParameter.Bind( ParameterMap, TEXT("BoneToDecal"), TRUE );
		DecalLocationParameter.Bind( ParameterMap, TEXT("DecalLocation"), TRUE );
		DecalOffsetParameter.Bind( ParameterMap, TEXT("DecalOffset"), TRUE );
	}

	/**
	 * Serialize shader params to an archive
	 * @param	Ar - archive to serialize to
	 */
	virtual void Serialize(FArchive& Ar)
	{
		Super::Serialize( Ar );
		Ar << BoneToDecalParameter;
		Ar << DecalLocationParameter;
		Ar << DecalOffsetParameter;
	}

	/**
	 * Set any shader data specific to this vertex factory
	 */
	virtual void Set(FCommandContextRHI* Context, FShader* VertexShader, const FVertexFactory* VertexFactory, const FSceneView* View) const
	{
		Super::Set( Context, VertexShader, VertexFactory, View );

		FLocalDecalVertexFactory* DecalVertexFactory = (FLocalDecalVertexFactory*)VertexFactory;
		if ( BoneToDecalParameter.IsBound() )
		{
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), BoneToDecalParameter, DecalVertexFactory->GetDecalMatrix() );
		}
		if ( DecalLocationParameter.IsBound() )
		{
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), DecalLocationParameter, DecalVertexFactory->GetDecalLocation() );
		}
		if ( DecalOffsetParameter.IsBound() )
		{
			SetVertexShaderValue( Context, VertexShader->GetVertexShader(), DecalOffsetParameter, DecalVertexFactory->GetDecalOffset() );
		}
	}

private:
	FShaderParameter BoneToDecalParameter;
	FShaderParameter DecalLocationParameter;
	FShaderParameter DecalOffsetParameter;
};

IMPLEMENT_VERTEX_FACTORY_TYPE(FLocalDecalVertexFactory,FLocalDecalVertexFactoryShaderParameters,"LocalDecalVertexFactory",TRUE,TRUE,VER_REMOVE_BINORMAL_TANGENT_VECTOR,0);
