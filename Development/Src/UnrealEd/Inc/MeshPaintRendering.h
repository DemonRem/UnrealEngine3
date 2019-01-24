/*================================================================================
	MeshPaintRendering.h: Mesh texture paint brush rendering
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
================================================================================*/

#ifndef __MeshPaintRendering_h__
#define __MeshPaintRendering_h__

#ifdef _MSC_VER
	#pragma once
#endif



namespace MeshPaintRendering
{
	/** Batched element parameters for mesh paint shaders */
	struct FMeshPaintShaderParameters
	{

	public:

		// @todo MeshPaint: Should be serialized no?
		UTextureRenderTarget2D* CloneTexture;

		FMatrix WorldToBrushMatrix;

		FLOAT BrushRadius;
		FLOAT BrushRadialFalloffRange;
		FLOAT BrushDepth;
		FLOAT BrushDepthFalloffRange;
		FLOAT BrushStrength;
		FLinearColor BrushColor;

	};


	/** Binds the mesh paint vertex and pixel shaders to the graphics device */
	void SetMeshPaintShaders_RenderThread( const FMatrix& InTransform,
										   const FLOAT InGamma,
										   const FMeshPaintShaderParameters& InShaderParams );


}



#endif	// __MeshPaintRendering_h__
