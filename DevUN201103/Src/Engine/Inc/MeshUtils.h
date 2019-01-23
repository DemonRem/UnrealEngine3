/*=============================================================================
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/


/**
*
* Utility helpers. This builds on D3DX functionality on appropriate platforms, otherwise does nothing.
* Used for editor helpers.
*
*/
UBOOL GenerateLOD(UStaticMesh* SourceStaticMesh, UStaticMesh* DestStaticMesh, INT DesiredLOD, INT DesiredTriangles);

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
				  );

