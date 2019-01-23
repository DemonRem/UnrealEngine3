/*=============================================================================
 Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "MeshUtils.h"

#if USE_D3D_RHI
#include "D3DMeshUtils.h"
#endif

UBOOL GenerateLOD(UStaticMesh* StaticMesh, INT DesiredLOD, INT DesiredTriangles)
{
#if USE_D3D_RHI
	FD3DMeshUtilities MeshUtils;
	return MeshUtils.GenerateLOD(StaticMesh, DesiredLOD, DesiredTriangles);
#endif
	return FALSE;
}

UBOOL GenerateUVs(UStaticMesh* StaticMesh, UINT LODIndex, UINT TexCoordIndex, FLOAT &MaxDesiredStretch, UINT &ChartsGenerated)
{
#if USE_D3D_RHI
	FD3DMeshUtilities MeshUtils;
	return MeshUtils.GenerateUVs(StaticMesh, LODIndex, TexCoordIndex, MaxDesiredStretch, ChartsGenerated);
#endif
	return FALSE;
}

