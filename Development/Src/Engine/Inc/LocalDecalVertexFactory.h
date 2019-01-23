/*=============================================================================
	LocalDecalVertexFactory.h: Local vertex factory bound to a shader that computes decal texture coordinates.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __LOCALDECALVERTEXFACTORY_H__
#define __LOCALDECALVERTEXFACTORY_H__

#include "SkelMeshDecalVertexFactory.h"

/**
 * A vertex factory which simply transforms explicit vertex attributes from local to world space.
 */
class FLocalDecalVertexFactory : public FLocalVertexFactory, public FSkelMeshDecalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FLocalDecalVertexFactory);
public:
	typedef FLocalVertexFactory Super;

	virtual FVertexFactory* CastToFVertexFactory()
	{
		return static_cast<FLocalDecalVertexFactory*>( this );
	}

	const FMatrix& GetDecalMatrix() const
	{
		return DecalMatrix;
	}

	const FVector& GetDecalLocation() const
	{
		return DecalLocation;
	}

	const FVector2D& GetDecalOffset() const
	{
		return DecalOffset;
	}

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static UBOOL ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return Material->IsDecalMaterial() && Super::ShouldCache( Platform, Material, ShaderType );
	}
};

#endif // __LOCALDECALVERTEXFACTORY_H__
