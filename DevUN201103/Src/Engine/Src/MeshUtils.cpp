/*=============================================================================
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
#include "EnginePrivate.h"
#include "MeshUtils.h"

#if _WINDOWS && !UE3_LEAN_AND_MEAN && !CONSOLE
#include "../../D3D9Drv/Inc/D3D9Drv.h"
#endif

UBOOL GenerateLOD(UStaticMesh* SourceStaticMesh, UStaticMesh* DestStaticMesh, INT DesiredLOD, INT DesiredTriangles)
{
#if _WINDOWS && !USE_NULL_RHI && !UE3_LEAN_AND_MEAN && !CONSOLE
	FD3DMeshUtilities MeshUtils;
	return MeshUtils.GenerateLOD( SourceStaticMesh, DestStaticMesh, DesiredLOD, DesiredTriangles);
#endif
	return FALSE;
}

UBOOL GenerateUVs(
				  UStaticMesh* StaticMesh,
				  UINT LODIndex,
				  UINT TexCoordIndex,
				  UBOOL bKeepExistingCoordinates,
				  FLOAT MinChartSpacingPercent,
				  FLOAT BorderSpacingPercent,
				  UBOOL bUseMaxStretch,
				  const TArray< INT >* InFalseEdgeIndices,
				  UINT& MaxCharts,
				  FLOAT& MaxDesiredStretch
				  )
{
#if _WINDOWS && !USE_NULL_RHI && !UE3_LEAN_AND_MEAN && !CONSOLE
	FD3DMeshUtilities MeshUtils;
	return MeshUtils.GenerateUVs(StaticMesh, LODIndex, TexCoordIndex, bKeepExistingCoordinates, MinChartSpacingPercent, BorderSpacingPercent, bUseMaxStretch, InFalseEdgeIndices, MaxCharts, MaxDesiredStretch);
#endif
	return FALSE;
}

