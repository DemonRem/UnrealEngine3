/*=============================================================================
	UnEdModeTools.h: Tools that the editor modes rely on
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * Base class for utility classes that storing settings specific to each mode/tool.
 */
class FToolSettings
{
public:
	virtual ~FToolSettings() {}
};

// -----------------------------------------------------------------------------

class FTerrainToolSettings : public FToolSettings
{
public:
	FTerrainToolSettings():
		  RadiusMin(128.0f)
		, RadiusMax(512.0f)
		, Strength(100.0f)
		, DecoLayer(0)
		, LayerIndex(INDEX_NONE)
		, MaterialIndex(-1)
		, FlattenAngle(FALSE)
		, FlattenHeight(0.f)
		, UseFlattenHeight(FALSE)
		, ScaleFactor(1.0f)
		, MirrorSetting(TTMirror_NONE)
		, bSoftSelectionEnabled(FALSE)
		, CurrentTerrain(NULL)
	{}

	INT RadiusMin, RadiusMax;
	FLOAT Strength;

	UBOOL	DecoLayer;
	INT		LayerIndex;		// INDEX_NONE = Heightmap
	INT		MaterialIndex;	// -1 = No specific material

	UBOOL	FlattenAngle;
	FLOAT	FlattenHeight;
	UBOOL	UseFlattenHeight;

	FLOAT	ScaleFactor;

	FString	HeightMapExporterClass;

	enum MirrorFlags
	{
		TTMirror_NONE = 0,
		TTMirror_X,
		TTMirror_Y,
		TTMirror_XY
	};

	MirrorFlags MirrorSetting;

	UBOOL bSoftSelectionEnabled;

	class ATerrain* CurrentTerrain;
};

// -----------------------------------------------------------------------------

class FTextureToolSettings : public FToolSettings
{
public:
	FTextureToolSettings() {}
};

// -----------------------------------------------------------------------------

// Keep this mirrored to EGeometrySelectionType in script code!
enum EGeometrySelectionType
{
	GST_Object,
	GST_Poly,
	GST_Edge,
	GST_Vertex,
};

class FGeometryToolSettings : public FToolSettings
{
public:
	FGeometryToolSettings() :
		SelectionType( GST_Vertex )
		,	bShowNormals(0)
		,	bAffectWidgetOnly(0)
		,	bUseSoftSelection(0)
		,	SoftSelectionRadius(500)
		,	bShowModifierWindow(0)
	{}

	/** The type of geometry that is being selected and worked with. */
	EGeometrySelectionType SelectionType;

	/** Should normals be drawn in the editor viewports? */
	UBOOL bShowNormals;

	/**
	 * If TRUE, selected objects won't be dragged around allowing the widget
	 * to be placed manually for ease of rotations and other things.
	 */
	UBOOL bAffectWidgetOnly;

	/** If TRUE, we are using soft selection. */
	UBOOL bUseSoftSelection;

	/** The falloff radius used when soft selection is in use. */
	INT SoftSelectionRadius;

	/** If TRUE, show the modifier window. */
	UBOOL bShowModifierWindow;
};

/*-----------------------------------------------------------------------------
	Misc
-----------------------------------------------------------------------------*/

enum EModeTools
{
	MT_None,
	MT_TerrainPaint,			// Painting on heightmaps/layers
	MT_TerrainSmooth,			// Smoothing height/alpha maps
	MT_TerrainAverage,			// Averaging height/alpha maps
	MT_TerrainFlatten,			// Flattening height/alpha maps
	MT_TerrainFlattenSpecific,	// Flattening height/alpha maps to a specific value
	MT_TerrainNoise,			// Adds random noise into the height/alpha maps
	MT_TerrainVisibility,		// Add/remove holes in terrain
	MT_TerrainTexturePan,		// Pan the terrain texture
	MT_TerrainTextureRotate,	// Rotate the terrain texture
	MT_TerrainTextureScale,		// Scale the terrain texture
	MT_InterpEdit,
	MT_GeometryModify,			// Modification of geometry through modifiers
	MT_Texture,					// Modifying texture alignment via the widget
	MT_TerrainSplitX,			// Split the terrain along the X-axis
	MT_TerrainSplitY,			// Split the terrain along the Y-axis
	MT_TerrainMerge,			// Merge two terrains into a single one
	MT_TerrainAddRemoveSectors,	// Add/remove sectors to existing terrain
	MT_TerrainOrientationFlip,	// Flip the orientation of a terrain quad
	MT_TerrainVertexEdit		// Edit vertices of terrain directly
};

/*-----------------------------------------------------------------------------
	FModeTool.
-----------------------------------------------------------------------------*/

/**
 * Base class for all editor mode tools.
 */
class FModeTool
{
public:
	FModeTool();
	virtual ~FModeTool();

	/** Returns the name that gets reported to the editor. */
	virtual FString GetName() const		{ return TEXT("Default"); }

	// User input

	virtual UBOOL MouseMove(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,INT x, INT y) { return 0; }

	/**
	 * @return		TRUE if the delta was handled by this editor mode tool.
	 */
	virtual UBOOL InputAxis(FEditorLevelViewportClient* InViewportClient,FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime)
	{
		return FALSE;
	}

	/**
	 * @return		TRUE if the delta was handled by this editor mode tool.
	 */
	virtual UBOOL InputDelta(FEditorLevelViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale)
	{
		return FALSE;
	}

	/**
	 * @return		TRUE if the key was handled by this editor mode tool.
	 */
	virtual UBOOL InputKey(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,FName Key,EInputEvent Event)
	{
		return FALSE;
	}

	//@{
	virtual UBOOL StartModify()	{ return 0; }
	virtual UBOOL EndModify()	{ return 0; }
	//@}

	//@{
	virtual void StartTrans()	{}
	virtual void EndTrans()		{}
	//@}

	// Tick
	virtual void Tick(FEditorLevelViewportClient* ViewportClient,FLOAT DeltaTime) {}

	/** @name Selections */
	//@{
	virtual void SelectNone() {}
	virtual void BoxSelect( FBox& InBox, UBOOL InSelect = 1 ) {}
	//@}

	/** Returns the tool type. */
	EModeTools GetID() const			{ return ID; }

	/** Returns true if this tool wants to have input filtered through the editor widget. */
	UBOOL UseWidget() const				{ return bUseWidget; }

	/** Returns the active tool settings. */
	FToolSettings* GetSettings()	{ return Settings; }

protected:
	/** Which tool this is. */
	EModeTools ID;

	/** If true, this tool wants to have input filtered through the editor widget. */
	UBOOL bUseWidget;

	/** Tool settings. */
	FToolSettings* Settings;
};

/*-----------------------------------------------------------------------------
	FModeTool_GeometryModify.	
-----------------------------------------------------------------------------*/

/**
 * Widget manipulation of geometry.
 */
class FModeTool_GeometryModify : public FModeTool
{
public:
	FModeTool_GeometryModify();

	virtual FString GetName() const		{ return TEXT("Modifier"); }

	void SetCurrentModifier( UGeomModifier* InModifier )	{ CurrentModifier = InModifier; }
	UGeomModifier* GetCurrentModifier()						{ return CurrentModifier; }

	/**
	 * @return		TRUE if the delta was handled by this editor mode tool.
	 */
	virtual UBOOL InputDelta(FEditorLevelViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);

	virtual UBOOL StartModify();
	virtual UBOOL EndModify();

	virtual void StartTrans();
	virtual void EndTrans();

	virtual void SelectNone();
	virtual void BoxSelect( FBox& InBox, UBOOL InSelect = 1 );

	/** @name Modifier iterators */
	//@{
	typedef TIndexedContainerIterator< TArray<UGeomModifier*> >			TModifierIterator;
	typedef TIndexedContainerConstIterator< TArray<UGeomModifier*> >	TModifierConstIterator;

	TModifierIterator		ModifierIterator()				{ return TModifierIterator( Modifiers ); }
	TModifierConstIterator	ModifierConstIterator() const	{ return TModifierConstIterator( Modifiers ); }

	// @todo DB: Get rid of these; requires changes to UnGeom.cpp
	UGeomModifier* GetModifier(INT Index)					{ return Modifiers( Index ); }
	const UGeomModifier* GetModifier(INT Index) const		{ return Modifiers( Index ); }
	//@}

	/**
	 * @return		TRUE if the key was handled by this editor mode tool.
	 */
	virtual UBOOL InputKey(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,FName Key,EInputEvent Event);

protected:
	/** All available modifiers. */
	TArray<UGeomModifier*> Modifiers;

	/** The current modifier. */
	UGeomModifier* CurrentModifier;
};

/*-----------------------------------------------------------------------------
	FModeTool_Terrain.
-----------------------------------------------------------------------------*/

/**
 * Base class for all terrain tools.
 */
class FModeTool_Terrain : public FModeTool
{
	TArray<class ATerrain*> ModifiedTerrains;
	UBOOL					bIsTransacting;

public:
	FViewportClient*	PaintingViewportClient;

	FModeTool_Terrain();

	virtual FString GetName() const				{ return TEXT("N/A");	}
	virtual UBOOL SupportsMirroring() const		{ return TRUE;			}

	virtual UBOOL MouseMove(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,INT x, INT y);

	/**
	 * @return		TRUE if the delta was handled by this editor mode tool.
	 */
	virtual UBOOL InputDelta(FEditorLevelViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);

	/**
	 * @return		TRUE if the key was handled by this editor mode tool.
	 */
	virtual UBOOL InputKey(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,FName Key,EInputEvent Event);

	virtual void Tick(FEditorLevelViewportClient* ViewportClient,FLOAT DeltaTime);

	/** Casts a ray from the viewpoint through a given pixel and returns the hit terrain/location. */
	class ATerrain* TerrainTrace(FViewport* Viewport,const FSceneView* View,FVector& Location,FVector& Normal,FTerrainToolSettings* Settings, UBOOL bGetFirstHit = FALSE);

	// RadiusStrength
	FLOAT RadiusStrength(const FVector& ToolCenter,const FVector& Vertex,FLOAT MinRadius,FLOAT MaxRadius);

	// BeginApplyTool
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);

	// ApplyTool
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE) = 0;

	// EndApplyTool
	virtual void EndApplyTool();

	/**
	 *	Retreive the mirrored versions of the Min/Max<X/Y> values
	 *
	 *	@param	Terrain		Pointer to the terrain of interest
	 *	@param	InMinX		The 'source' MinX value
	 *	@param	InMinY		The 'source' MinY value
	 *	@param	InMaxX		The 'source' MaxX value
	 *	@param	InMaxY		The 'source' MaxY value
	 *	@param	bMirrorX	Whether to mirror about the X axis
	 *	@param	bMirrorY	Whether to mirror about the Y axis
	 *	@param	OutMinX		The output of the mirrored MinX value
	 *	@param	OutMinY		The output of the mirrored MinY value
	 *	@param	OutMaxX		The output of the mirrored MaxX value
	 *	@param	OutMaxY		The output of the mirrored MaxY value
	 */
	virtual void GetMirroredExtents(ATerrain* Terrain, INT InMinX, INT InMinY, INT InMaxX, INT InMaxY, 
		UBOOL bMirrorX, UBOOL bMirrorY, INT& OutMinX, INT& OutMinY, INT& OutMaxX, INT& OutMaxY);

	// TerrainIsValid
	UBOOL TerrainIsValid(ATerrain* TestTerrain, FTerrainToolSettings* Settings);

	// Render
	virtual UBOOL Render(ATerrain* Terrain, const FVector HitLocation, const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI ) { return TRUE; }

	/**
	 *	UpdateIntermediateValues
	 *	Update the intermediate values of the edited quads.
	 *
	 *	@param	Terrain			Pointer to the terrain of interest
	 *	@param	DecoLayer		Boolean indicating the layer is a deco layer
	 *	@param	LayerIndex		The index of the layer being edited
	 *	@param	MaterialIndex	The index of the material being edited
	 *	@param	InMinX			The 'source' MinX value
	 *	@param	InMinY			The 'source' MinY value
	 *	@param	InMaxX			The 'source' MaxX value
	 *	@param	InMaxY			The 'source' MaxY value
	 *	@param	bMirrorX		Whether to mirror about the X axis
	 *	@param	bMirrorY		Whether to mirror about the Y axis
	 */
	virtual void UpdateIntermediateValues(ATerrain* Terrain, 
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		INT InMinX, INT InMinY, INT InMaxX, INT InMaxY, 
		UBOOL bMirrorX, UBOOL bMirrorY);

protected:
	INT FindMatchingTerrainLayer( ATerrain* TestTerrain, FTerrainToolSettings* Settings );
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainPaint.
-----------------------------------------------------------------------------*/

/**
 * For painting terrain heightmaps/layers.
 */
class FModeTool_TerrainPaint : public FModeTool_Terrain
{
public:
	FModeTool_TerrainPaint();

	virtual FString GetName() const		{ return TEXT("Paint"); }

	// FModeTool_Terrain interface.

	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainSmooth.
-----------------------------------------------------------------------------*/

/**
 * Smooths heightmap vertices/layer alpha maps.
 */
class FModeTool_TerrainSmooth : public FModeTool_Terrain
{
public:
	FModeTool_TerrainSmooth();

	virtual FString GetName() const		{ return TEXT("Smooth"); }

	// FModeTool_Terrain interface.

	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainAverage.
-----------------------------------------------------------------------------*/

/**
 * Averages heightmap vertices/layer alpha maps.
 */
class FModeTool_TerrainAverage : public FModeTool_Terrain
{
public:
	FModeTool_TerrainAverage();

	virtual FString GetName() const		{ return TEXT("Average"); }

	// FModeTool_Terrain interface.

	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainFlatten.
-----------------------------------------------------------------------------*/

/**
 * Flattens heightmap vertices/layer alpha maps.
 */
class FModeTool_TerrainFlatten : public FModeTool_Terrain
{
public:
	FLOAT	FlatValue;
	FVector	FlatWorldPosition;
	FVector	FlatWorldNormal;

	FModeTool_TerrainFlatten();

	virtual FString GetName() const		{ return TEXT("Flatten"); }

	// FModeTool_Terrain interface.

	// BeginApplyTool
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);

	// ApplyTool
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainNoise.
-----------------------------------------------------------------------------*/

/**
 * For adding random noise heightmaps/layers.
 */
class FModeTool_TerrainNoise : public FModeTool_Terrain
{
public:
	FModeTool_TerrainNoise();

	virtual FString GetName() const		{ return TEXT("Noise"); }

	// FModeTool_Terrain interface.

	// ApplyTool
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainVisibility.
-----------------------------------------------------------------------------*/

/**
 *	Used for painting 'holes' in the terrain
 */
class FModeTool_TerrainVisibility : public FModeTool_Terrain
{
public:
	FModeTool_TerrainVisibility();

	virtual FString GetName() const		{ return TEXT("Visibility"); }

	// FModeTool_Terrain interface.

	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainTexturePan.
-----------------------------------------------------------------------------*/

/**
 *	
 */
class FModeTool_TerrainTexturePan : public FModeTool_Terrain
{
public:
	FModeTool_TerrainTexturePan();

	virtual FString GetName() const				{ return TEXT("Tex Pan");	}
	virtual UBOOL SupportsMirroring() const		{ return FALSE;				}

	// FModeTool_Terrain interface.
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);

	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);

protected:
	FVector	LastPosition;
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainTextureRotate.
-----------------------------------------------------------------------------*/

/**
 *	
 */
class FModeTool_TerrainTextureRotate : public FModeTool_Terrain
{
public:
	FModeTool_TerrainTextureRotate();

	virtual FString GetName() const				{ return TEXT("Tex Rotate");	}
	virtual UBOOL SupportsMirroring() const		{ return FALSE;					}

	// FModeTool_Terrain interface.
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);

	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);

protected:
	FVector	LastPosition;
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainTextureScale.
-----------------------------------------------------------------------------*/

/**
 *	
 */
class FModeTool_TerrainTextureScale : public FModeTool_Terrain
{
public:
	FModeTool_TerrainTextureScale();

	virtual FString GetName() const				{ return TEXT("Tex Scale"); }
	virtual UBOOL SupportsMirroring() const		{ return FALSE;				}

	// FModeTool_Terrain interface.
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);

	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,
		INT MinX,INT MinY,INT MaxX,INT MaxY, 
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);

protected:
	FVector	LastPosition;
};

/*-----------------------------------------------------------------------------
	FModeTool_Texture.
-----------------------------------------------------------------------------*/

class FModeTool_Texture : public FModeTool
{
public:
	FModeTool_Texture();

	virtual UBOOL InputDelta(FEditorLevelViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);
};

/*-----------------------------------------------------------------------------
	FModeTool_VertexLock.
-----------------------------------------------------------------------------*/

class FModeTool_VertexLock : public FModeTool
{
public:
	FModeTool_VertexLock();

	virtual UBOOL InputDelta(FEditorLevelViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainSplitX.
-----------------------------------------------------------------------------*/

class FModeTool_TerrainSplitX : public FModeTool_Terrain
{
public:
	FModeTool_TerrainSplitX() { ID = MT_TerrainSplitX; }
	virtual FString GetName() const				{ return TEXT("SplitX");	}
	virtual UBOOL SupportsMirroring() const		{ return FALSE;				}

	// FModeTool_Terrain interface.
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,INT MinX,INT MinY,INT MaxX,INT MaxY,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE) 
	{
		return TRUE;
	}
	virtual UBOOL Render(ATerrain* Terrain, const FVector HitLocation, const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI );
};
/*-----------------------------------------------------------------------------
	FModeTool_TerrainSplitY.
-----------------------------------------------------------------------------*/

class FModeTool_TerrainSplitY : public FModeTool_Terrain
{
public:
	FModeTool_TerrainSplitY() { ID = MT_TerrainSplitY; }
	virtual FString GetName() const				{ return TEXT("SplitY");	}
	virtual UBOOL SupportsMirroring() const		{ return FALSE;				}

	// FModeTool_Terrain interface.
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,INT MinX,INT MinY,INT MaxX,INT MaxY,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE)
	{
		return TRUE;
	}
	virtual UBOOL Render(ATerrain* Terrain, const FVector HitLocation, const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI );
};


/*-----------------------------------------------------------------------------
	FModeTool_Merge.
-----------------------------------------------------------------------------*/

class FModeTool_TerrainMerge : public FModeTool_Terrain
{
public:
	FModeTool_TerrainMerge() { ID = MT_TerrainMerge; }
	virtual FString GetName() const				{ return TEXT("Merge");		}
	virtual UBOOL SupportsMirroring() const		{ return FALSE;				}

	// FModeTool_Terrain interface.
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,INT MinX,INT MinY,INT MaxX,INT MaxY,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE)
	{
		return TRUE;
	}
	virtual UBOOL Render(ATerrain* Terrain, const FVector HitLocation, const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI );
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainAddRemoveSectors.
-----------------------------------------------------------------------------*/

/**
 * For adding/removing Sectors to existing terrain
 */
class FModeTool_TerrainAddRemoveSectors : public FModeTool_Terrain
{
public:
	FModeTool_TerrainAddRemoveSectors()	{ ID = MT_TerrainAddRemoveSectors; }

	virtual FString GetName() const		{ return TEXT("AddRemoveSectors"); }

	// FModeTool_Terrain interface.
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,INT MinX,INT MinY,INT MaxX,INT MaxY,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
	virtual void EndApplyTool();
	virtual UBOOL Render(ATerrain* Terrain, const FVector HitLocation, const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI );

protected:
	ATerrain* CurrentTerrain;
	FVector	StartPosition;
	FLOAT Direction;
	FVector	CurrPosition;
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainOrientationFlip.
-----------------------------------------------------------------------------*/

/**
 *  For flipping the orientation of a quad (edge flipping)
 */
class FModeTool_TerrainOrientationFlip : public FModeTool_Terrain
{
public:
	FModeTool_TerrainOrientationFlip()	{ ID = MT_TerrainOrientationFlip; }
	virtual FString GetName() const		{ return TEXT("OrientationFlip"); }

	// FModeTool_Terrain interface.
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,INT MinX,INT MinY,INT MaxX,INT MaxY,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
};

/*-----------------------------------------------------------------------------
	FModeTool_TerrainVertexEdit.
-----------------------------------------------------------------------------*/

/**
* For editing terrain vertices directly.
*/
class FModeTool_TerrainVertexEdit : public FModeTool_Terrain
{
public:
	FModeTool_TerrainVertexEdit()
	{
		ID = MT_TerrainVertexEdit;
		bCtrlIsPressed = FALSE;
		bAltIsPressed = FALSE;
		bSelectVertices = FALSE;
		bDeselectVertices = FALSE;
		bMouseLeftPressed = FALSE;
		bMouseRightPressed = FALSE;
		MouseYDelta = 0;
	}

	virtual FString GetName() const		{ return TEXT("VertexEdit"); }

	// FModeTool interface.
	virtual UBOOL MouseMove(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,INT x, INT y);

	/**
	 * @return		TRUE if the delta was handled by this editor mode tool.
	 */
	virtual UBOOL InputAxis(FEditorLevelViewportClient* InViewportClient,FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime);

	/**
	 * @return		TRUE if the delta was handled by this editor mode tool.
	 */
	virtual UBOOL InputDelta(FEditorLevelViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);

	/**
	 * @return		TRUE if the key was handled by this editor mode tool.
	 */
	virtual UBOOL InputKey(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,FName Key,EInputEvent Event);

	// FModeTool_Terrain interface.
	virtual UBOOL BeginApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,const FVector& WorldPosition, const FVector& WorldNormal,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
	virtual UBOOL ApplyTool(ATerrain* Terrain,
		UBOOL DecoLayer,INT LayerIndex,INT MaterialIndex,
		const FVector& LocalPosition,FLOAT LocalMinRadius,FLOAT LocalMaxRadius,
		FLOAT InDirection,FLOAT LocalStrength,INT MinX,INT MinY,INT MaxX,INT MaxY,
		UBOOL bMirrorX = FALSE, UBOOL bMirrorY = FALSE);
	virtual void EndApplyTool();
	virtual UBOOL Render(ATerrain* Terrain, const FVector HitLocation, const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI );

protected:
	UBOOL bCtrlIsPressed;
	UBOOL bAltIsPressed;
	UBOOL bSelectVertices;
	UBOOL bDeselectVertices;
	UBOOL bMouseLeftPressed;
	UBOOL bMouseRightPressed;
	INT MouseYDelta;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
