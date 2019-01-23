/*================================================================================
	MeshPaintEdMode.h: Mesh paint tool
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
================================================================================*/


#ifndef __MeshPaintEdMode_h__
#define __MeshPaintEdMode_h__

#ifdef _MSC_VER
	#pragma once
#endif



/** Mesh paint resource types */
namespace EMeshPaintResource
{
	enum Type
	{
		/** Editing vertex colors */
		VertexColors,

		/** Editing textures */
		Texture
	};
}



/** Mesh paint mode */
namespace EMeshPaintMode
{
	enum Type
	{
		/** Painting colors directly */
		PaintColors,

		/** Painting texture blend weights */
		PaintWeights
	};
}



/** Vertex paint target */
namespace EMeshVertexPaintTarget
{
	enum Type
	{
		/** Paint the static mesh component instance in the level */
		ComponentInstance,

		/** Paint the actual static mesh asset */
		Mesh
	};
}




/** Mesh paint color view modes (somewhat maps to EVertexColorViewMode engine enum.) */
namespace EMeshPaintColorViewMode
{
	enum Type
	{
		/** Normal view mode (vertex color visualization off) */
		Normal,

		/** RGB only */
		RGB,
		
		/** Alpha only */
		Alpha,

		/** Red only */
		Red,

		/** Green only */
		Green,

		/** Blue only */
		Blue,
	};
}


/**
 * Mesh Paint settings
 */
class FMeshPaintSettings
{

public:

	/** Static: Returns global mesh paint settings */
	static FMeshPaintSettings& Get()
	{
		return StaticMeshPaintSettings;
	}


protected:

	/** Static: Global mesh paint settings */
	static FMeshPaintSettings StaticMeshPaintSettings;



public:

	/** Constructor */
	FMeshPaintSettings()
		: BrushRadius( 32.0f ),
		  BrushFalloffAmount( 1.0f ),
		  BrushStrength( 0.2f ),
		  bEnableFlow( TRUE ),
		  FlowAmount( 1.0f ),
		  bOnlyFrontFacingTriangles( TRUE ),
		  ResourceType( EMeshPaintResource::VertexColors ),
		  PaintMode( EMeshPaintMode::PaintColors ),
		  PaintColor( 1.0f, 1.0f, 1.0f, 1.0f ),
		  EraseColor( 0.0f, 0.0f, 0.0f, 0.0f ),
		  bWriteRed( TRUE ),
		  bWriteGreen( TRUE ),
		  bWriteBlue( TRUE ),
		  bWriteAlpha( FALSE ),		// NOTE: Don't write alpha by default
		  TotalWeightCount( 2 ),
		  PaintWeightIndex( 0 ),
		  EraseWeightIndex( 1 ),
		  VertexPaintTarget( EMeshVertexPaintTarget::ComponentInstance ),
		  UVChannel( 1 ),
		  ColorViewMode( EMeshPaintColorViewMode::Normal ),
		  WindowPositionX( -1 ),
		  WindowPositionY( -1 )
	{
	}


public:

	/** Radius of the brush (world space units) */
	FLOAT BrushRadius;

	/** Amount of falloff to apply (0.0 - 1.0) */
	FLOAT BrushFalloffAmount;

	/** Strength of the brush (0.0 - 1.0) */
	FLOAT BrushStrength;

	/** Enables "Flow" painting where paint is continually applied from the brush every tick */
	UBOOL bEnableFlow;

	/** When "Flow" is enabled, this scale is applied to the brush strength for paint applied every tick (0.0-1.0) */
	FLOAT FlowAmount;

	/** Whether back-facing triangles should be ignored */
	UBOOL bOnlyFrontFacingTriangles;

	/** Resource type we're editing */
	EMeshPaintResource::Type ResourceType;


	/** Mode we're painting in.  If this is set to PaintColors we're painting colors directly; if it's set
	    to PaintWeights we're painting texture blend weights using a single channel. */
	EMeshPaintMode::Type PaintMode;


	/** Colors Mode: Paint color */
	FLinearColor PaintColor;

	/** Colors Mode: Erase color */
	FLinearColor EraseColor;

	/** Colors Mode: True if red colors values should be written */
	UBOOL bWriteRed;

	/** Colors Mode: True if green colors values should be written */
	UBOOL bWriteGreen;

	/** Colors Mode: True if blue colors values should be written */
	UBOOL bWriteBlue;

	/** Colors Mode: True if alpha colors values should be written */
	UBOOL bWriteAlpha;


	/** Weights Mode: Total weight count */
	INT TotalWeightCount;

	/** Weights Mode: Weight index that we're currently painting */
	INT PaintWeightIndex;

	/** Weights Mode: Weight index that we're currently using to erase with */
	INT EraseWeightIndex;


	/**
	 * Vertex paint settings
	 */
	
	/** Vertex paint target */
	EMeshVertexPaintTarget::Type VertexPaintTarget;


	/**
	 * Texture paint settings
	 */
	
	/** UV channel to paint textures using */
	INT UVChannel;


	/**
	 * View settings
	 */

	/** Color visualization mode */
	EMeshPaintColorViewMode::Type ColorViewMode;


	/**
	 * Window settings
	 */

	/** Horizontal window position */
	INT WindowPositionX;

	/** Vertical window position */
	INT WindowPositionY;



};



/** Mesh painting action (paint, erase) */
namespace EMeshPaintAction
{
	enum Type
	{
		/** Paint (add color or increase blending weight) */
		Paint,

		/** Erase (remove color or decrease blending weight) */
		Erase,

		/** Fill with the active paint color */
		Fill,

		/** Push instance colors to mesh color */
		PushInstanceColorsToMesh
	};
}



namespace MeshPaintDefs
{
	// Design constraints

	// Currently we never support more than five channels (R, G, B, A, OneMinusTotal)
	static const INT MaxSupportedPhysicalWeights = 4;
	static const INT MaxSupportedWeights = MaxSupportedPhysicalWeights + 1;
}






/**
 * Mesh Paint editor mode
 */

class FEdModeMeshPaint
	: public FEdMode
{

public:

	/** Constructor */
	FEdModeMeshPaint();

	/** Destructor */
	virtual ~FEdModeMeshPaint();

	/** FSerializableObject: Serializer */
	virtual void Serialize( FArchive &Ar );

	/** FEdMode: Called when the mode is entered */
	virtual void Enter();

	/** FEdMode: Called when the mode is exited */
	virtual void Exit();

	/** FEdMode: Called when the mouse is moved over the viewport */
	virtual UBOOL MouseMove( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, INT x, INT y );

	/**
	 * FEdMode: Called when the mouse is moved while a window input capture is in effect
	 *
	 * @param	InViewportClient	Level editor viewport client that captured the mouse input
	 * @param	InViewport			Viewport that captured the mouse input
	 * @param	InMouseX			New mouse cursor X coordinate
	 * @param	InMouseY			New mouse cursor Y coordinate
	 *
	 * @return	TRUE if input was handled
	 */
	virtual UBOOL CapturedMouseMove( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, INT InMouseX, INT InMouseY );

	/** FEdMode: Called when a mouse button is pressed */
	virtual UBOOL StartTracking();

	/** FEdMode: Called when a mouse button is released */
	virtual UBOOL EndTracking();

	/** FEdMode: Called when a key is pressed */
	virtual UBOOL InputKey( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FName InKey, EInputEvent InEvent );

	/** FEdMode: Called when mouse drag input it applied */
	virtual UBOOL InputDelta( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale );

	/** Returns TRUE if we need to force a render/update through based fill/copy */
	UBOOL IsForceRendered (void) const;

	/** FEdMode: Render the mesh paint tool */
	virtual void Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI );

	/** FEdMode: Render HUD elements for this tool */
	virtual void DrawHUD( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas );

	/** FEdMode: Called when the currently selected actor has changed */
	virtual void ActorSelectionChangeNotify();

	/** FEdMode: If the EdMode is handling InputDelta (ie returning true from it), this allows a mode to indicated whether or not the Widget should also move. */
	virtual UBOOL AllowWidgetMove() { return FALSE; }

	/** FEdMode: Draw the transform widget while in this mode? */
	virtual UBOOL ShouldDrawWidget() const { return FALSE; }

	/** FEdMode: Returns true if this mode uses the transform widget */
	virtual UBOOL UsesTransformWidget() const { return FALSE; }

	/** Helper function to get the current paint action for use in DoPaint */
	EMeshPaintAction::Type GetPaintAction(FViewport* InViewport);

	/** Removes vertex colors associated with the currently selected mesh */
	void RemoveInstanceVertexColors() const;

	/** Returns whether the instance vertex colors associated with the currently selected mesh need to be fixed up or not */
	UBOOL RequiresInstanceVertexColorsFixup() const;

	/** Attempts to fix up the instance vertex colors associated with the currently selected mesh, if necessary */
	void FixupInstanceVertexColors() const;

	/** Fills the vertex colors associated with the currently selected mesh*/
	void FillInstanceVertexColors();

	/** Pushes instance vertex colors to the  mesh*/
	void PushInstanceVertexColorsToMesh();

	/** Creates a paintable material/texture for the selected mesh */
	void CreateInstanceMaterialAndTexture() const;

	/** Removes instance of paintable material/texture for the selected mesh */
	void RemoveInstanceMaterialAndTexture() const;

	/** Returns information about the currently selected mesh */
	UBOOL GetSelectedMeshInfo( INT& OutTotalInstanceVertexColorBytes, UBOOL& bOutHasInstanceMaterialAndTexture ) const;

	typedef UStaticMesh::kDOPTreeType kDOPTreeType;
private:

	/** Static: Determines if a world space point is influenced by the brush and reports metrics if so */
	static UBOOL IsPointInfluencedByBrush( const FVector& InPosition,
										   const class FMeshPaintParameters& InParams,
										   FLOAT& OutSquaredDistanceToVertex2D,
										   FLOAT& OutVertexDepthToBrush );

	/** Static: Paints the specified vertex!  Returns TRUE if the vertex was in range. */
	static UBOOL PaintVertex( const FVector& InVertexPosition,
							  const class FMeshPaintParameters& InParams,
							  const UBOOL bIsPainting,
							  FColor& InOutVertexColor );

	/** Paint the mesh that impacts the specified ray */
	void DoPaint( const FVector& InCameraOrigin,
				  const FVector& InRayOrigin,
				  const FVector& InRayDirection,
				  FPrimitiveDrawInterface* PDI,
				  const EMeshPaintAction::Type InPaintAction,
				  const UBOOL bVisualCueOnly,
				  const FLOAT InStrengthScale,
				  OUT UBOOL& bAnyPaintAbleActorsUnderCursor);

	/** Paints mesh vertices */
	void PaintMeshVertices( UStaticMeshComponent* StaticMeshComponent, const FMeshPaintParameters& Params, const UBOOL bShouldApplyPaint, FStaticMeshRenderData& LODModel, const FVector& ActorSpaceCameraPosition, const FMatrix& ActorToWorldMatrix, FPrimitiveDrawInterface* PDI, const FLOAT VisualBiasDistance );

	/** Paints mesh texture */
	void PaintMeshTexture( UStaticMeshComponent* StaticMeshComponent, const FMeshPaintParameters& Params, const UBOOL bShouldApplyPaint, FStaticMeshRenderData& LODModel, const FVector& ActorSpaceCameraPosition, const FMatrix& ActorToWorldMatrix, const FLOAT ActorSpaceSquaredBrushRadius, const FVector& ActorSpaceBrushPosition );
	
	/** Forces real-time perspective viewports */
	void ForceRealTimeViewports( const UBOOL bEnable, const UBOOL bStoreCurrentState );

	/** Sets show flags for perspective viewports */
	void SetViewportShowFlags( const UBOOL bAllowColorViewModes );

	/** Starts painting a texture */
	void StartPaintingTexture( UStaticMeshComponent* InStaticMeshComponent );

	/** Paints on a texture */
	void PaintTexture( const FMeshPaintParameters& InParams,
					   const TArray< INT >& InInfluencedTriangles,
					   const FMatrix& InActorToWorldMatrix );

	/** Finishes painting a texture */
	void FinishPaintingTexture();

	/** Makes sure that the render target is ready to paint on */
	void SetupInitialRenderTargetData();

	/** Static: Copies a texture to a render target texture */
	static void CopyTextureToRenderTargetTexture( UTexture* SourceTexture, UTextureRenderTarget2D* RenderTargetTexture );

	/** Static: Creates a temporary texture used to transfer data to a render target in memory */
	UTexture2D* CreateTempUncompressedTexture( const INT InWidth, const INT InHeight, const TArray< BYTE >& InRawData, const UBOOL bInUseSRGB );


private:

	/** Whether we're currently painting */
	UBOOL bIsPainting;

	/** Whether we are doing a flood fill the next render */
	UBOOL bIsFloodFill;

	/** Whether we are pushing the instance colors to the mesh next render */
	UBOOL bPushInstanceColorsToMesh;

	/** Real time that we started painting */
	DOUBLE PaintingStartTime;

	/** Array of static meshes that we've modified */
	TArray< UStaticMesh* > ModifiedStaticMeshes;

	/** A mapping of static meshes to temp kDOP trees that were built for static meshes without collision */
	TMap<UStaticMesh*, kDOPTreeType> StaticMeshToTempkDOPMap;

#if WITH_MANAGED_CODE
	/** Mesh Paint tool palette window */
	TScopedPointer< class FMeshPaintWindow > MeshPaintWindow;
#endif

	/** Which mesh LOD index we're painting */
	// @todo MeshPaint: Allow painting on other LODs?
	static const UINT PaintingMeshLODIndex = 0;

	/** Texture paint: The static mesh components that we're currently painting */
	UStaticMeshComponent* TexturePaintingStaticMeshComponent;

	/** The original texture that we're painting */
	UTexture2D* PaintingTexture2D;

	/** Render target texture for painting */
	UTextureRenderTarget2D* PaintRenderTargetTexture;

	/** Render target texture used as an input while painting that contains a clone of the original image */
	UTextureRenderTarget2D* CloneRenderTargetTexture;

};


#endif	// __MeshPaintEdMode_h__
