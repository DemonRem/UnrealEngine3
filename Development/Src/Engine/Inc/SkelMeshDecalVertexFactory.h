/*=============================================================================
	SkelMeshDecalVertexFactory.h: Base class for vertex factories of decals on skeletal meshes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __SKELMESHDECALVERTEXFACTORY_H__
#define __SKELMESHDECALVERTEXFACTORY_H__

/** Baseclass for vertex factories of decals on skeletal meshes. */
class FSkelMeshDecalVertexFactoryBase
{
public:
	virtual ~FSkelMeshDecalVertexFactoryBase() {}

	virtual FVertexFactory* CastToFVertexFactory() = 0;

	virtual void SetDecalMatrix(const FMatrix& InDecalMatrix) {}
	virtual void SetDecalLocation(const FVector& InDecalLocation) {}
	virtual void SetDecalOffset(const FVector2D& InDecalOffset) {}
};

class FSkelMeshDecalVertexFactory : public FSkelMeshDecalVertexFactoryBase
{
public:
	FSkelMeshDecalVertexFactory()
		:	DecalMatrix(FMatrix::Identity)
		,	DecalLocation(0.f,0.f,0.f)
		,	DecalOffset(0.f,0.f)
	{}

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

	virtual void SetDecalMatrix(const FMatrix& InDecalMatrix)
	{
		DecalMatrix = InDecalMatrix;
	}

	virtual void SetDecalLocation(const FVector& InDecalLocation)
	{
		DecalLocation = InDecalLocation;
	}

	virtual void SetDecalOffset(const FVector2D& InDecalOffset)
	{
		DecalOffset = InDecalOffset;
	}

protected:
	FMatrix DecalMatrix;
	FVector DecalLocation;
	FVector2D DecalOffset;
};

#endif // __SKELMESHDECALVERTEXFACTORY_H__
