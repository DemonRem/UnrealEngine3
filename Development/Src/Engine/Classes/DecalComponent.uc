/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class DecalComponent extends PrimitiveComponent
	native(Decal)
	editinlinenew
	hidecategories(Collision,Object,Physics,PrimitiveComponent);

/** Decal material. */
var(Decal) MaterialInterface	DecalMaterial;

/** Decal world space width. */
var(Decal) float Width;

/** Decal world space height. */
var(Decal) float Height;

/** Decal tiling along the tangent. */
var(Decal) float TileX;

/** Decal tiling along the binormal. */
var(Decal) float TileY;

/** Decal offset along the tangent. */
var(Decal) float OffsetX;

/** Decal offset along the binormal. */
var(Decal) float OffsetY;

/** Decal in-plane rotation, in degrees. */
var(Decal) float DecalRotation;

/** Decal world space depth. */
var(Decal) float Thickness;

/** Horizontal field of view. */
var float FieldOfView;

/** Near plane clip distance. */
var(Decal) float NearPlane;

/** Far plane clip distance. */
var(Decal) float FarPlane;

/** Decal's frustum location, set in code or copied from DecalActor in UnrealEd. */
var vector		Location;

/** Decal's frustum orientation, set in code or copied from DecalActor in UnrealEd. */
var rotator		Orientation;

/** Decal's impact location, as computed by eg weapon trace. */
var vector		HitLocation;

/** Decal's impact normal, as computed by eg weapon trace.*/
var vector		HitNormal;

/** Decal's impact tangent, as computed by eg weapon trace.*/
var vector		HitTangent;

/** Decal's impact binormal, as computed by eg weapon trace.*/
var vector		HitBinormal;

/**
 * If FALSE (the default), use precise clipping to compute the decal geometry.
 * If TRUE, decal geometry generation is faster, but the decal material will have
 * to use clamped texture coordinates.
 */
var(Decal) bool bNoClip;

/**
 * TRUE for decals created in the editor, FALSE for decals created at runtime.
 */
var const bool bStaticDecal;

/**
 * If non-NULL, consider HitComponent only when computing receivers.
 */
var transient PrimitiveComponent HitComponent;

/**
 * The name of hit bone.
 */
var transient name HitBone;

/** If not -1, specifies the index of the BSP node that was hit. */
var transient int HitNodeIndex;

/** If not -1, specifies the level into the world's level array of the BSP node that was hit. */
var transient int HitLevelIndex;

/** Used to pass information of which BSP nodes where hit */
var private const transient array<int> HitNodeIndices;

/**
 * Instance data used by the decal's lifetime policy.
 * The decal goes unmanaged if this property is NULL.
 */
var instanced DecalLifetimeData LifetimeData;

/**
 * A decal receiver and its associated render data.
 */
struct native DecalReceiver
{
	var const PrimitiveComponent Component;
	var native const pointer RenderData{class FDecalRenderData};
};

/** List of receivers to which this decal is attached. */
var private noimport duplicatetransient const array<DecalReceiver> DecalReceivers;

/** List of receivers for static decals.  Empty if the decal has bStaticDecal=FALSE.*/
var native private noimport transient duplicatetransient const array<pointer> StaticReceivers{class FStaticReceiverData};

/** Render data for this decal, one per receiver. */
var deprecated private noimport native const array<pointer> RenderData{class FDecalRenderData};

/** Command fence used to shut down properly. */
var native const transient duplicatetransient pointer		ReleaseResourcesFence{FRenderCommandFence};

/** Ortho planes. */
var private transient array<Plane> Planes;

var(DecalRender) float DepthBias;
var(DecalRender) float SlopeScaleDepthBias;

/** Controls the order in which decal elements are rendered.  Higher values draw later (on top). */
var(DecalRender) int	SortOrder;

/** Dot product of the minimum angle that surfaces can make with the decal normal to be considered backfacing. */
var(DecalRender) float	BackfaceAngle;

/**
 * Specifies how the decal application filter should be interpreted.
 */
enum EFilterMode
{
	/** Filter is ignored. */
	FM_None,
	/** Filter specifies list of components to ignore. */
	FM_Ignore,
	/** Filter specifies list of components to affect.*/
	FM_Affect
};

/** Currect filter application mode. */
var(DecalFilter) EFilterMode	FilterMode;

/** Component filter. */
var(DecalFilter) array<Actor>	Filter;

/** @hack: Gears hack to avoid an octree look-up for level-placed decals. To be replaced with receiver serialization after ship.*/
var(DecalFilter) array<PrimitiveComponent>	ReceiverImages;

/** If FALSE (the default), don't project decal onto back-facing polygons. */
var(DecalFilter) bool bProjectOnBackfaces;

/** If FALSE (the default), don't project decal onto hidden receivers. */
var(DecalFilter) bool bProjectOnHidden;

/** If FALSE, don't project decal onto BSP. */
var(DecalFilter) bool bProjectOnBSP;

/** If FALSE, don't project decal onto static meshes. */
var(DecalFilter) bool bProjectOnStaticMeshes;

/** If FALSE, don't project decal onto skeletal meshes. */
var(DecalFilter) bool bProjectOnSkeletalMeshes;

/** If FALSE, don't project decal onto terrain. */
var(DecalFilter) bool bProjectOnTerrain;

/** If TRUE, invert the direction considered to be backfacing receiver triangles.  Set e.g. when decal actors are mirrored. */
var bool bFlipBackfaceDirection;

/**
 * return	The UDecalManager object appropriate for this decal.
 */
native function DecalManager GetManager();

/**
 * Connects this decal to the appropriate manager.
 */
native function ConnectToManager();

/**
 * Disconnects the decal from its manager.
 */
native function DisconnectFromManager();

cpptext
{
public:
	/**
	 * Builds orthogonal planes from HitLocation, HitNormal, Width, Height and Depth.
	 */
	void UpdateOrthoPlanes();

	/**
	 * @return	TRUE if the decal is enabled as specified in GEngine, FALSE otherwise.
	 */
	UBOOL IsEnabled() const;

	/**
	 * Allocates a sort key to the decal if the decal is non-static.
	 */
	void AllocateSortKey();

	/**
	 * Fills in the specified vertex list with the local-space decal frustum vertices.
	 */
	void GenerateDecalFrustumVerts(FVector Verts[8]) const;

	/**
	 * Fills in the specified decal state object with this decal's state.
	 */
	void CaptureDecalState(class FDecalState* DecalState) const;

	void AttachReceiver(UPrimitiveComponent* Receiver);

	/**
	 * Updates ortho planes, computes the receiver set, and connects to a decal manager.
	 */
	void ComputeReceivers();

	/**
	 * Attaches the decal to static receivers, according to the data stored in StaticReceivers.
	 */
	void AttachToStaticReceivers();

	/**
	 * Disconnects the decal from its manager and detaches receivers.
	 */
	void DetachFromReceivers();

	/**
	 * @return		TRUE if the application filter passes the specified component, FALSE otherwise.
	 */
	UBOOL FilterComponent(UPrimitiveComponent* Component) const;

	/**
	 * Frees any StaticReceivers data.
	 */
	void FreeStaticReceivers();

	// PrimitiveComponent interface.
	/**
	 * Sets the component's bounds based on the vertices of the decal frustum.
	 */
	virtual void UpdateBounds();

	/**
	 * Updates all associated lightmaps with the new value for allowing directional lightmaps.
	 * This is only called when switching lightmap modes in-game and should never be called on cooked builds.
	 */
	virtual void UpdateLightmapType(UBOOL bAllowDirectionalLightMaps);

	/**
	 * Creates a FPrimitiveSceneProxy proxy to implement debug rendering of the decal in the rendering thread.
	 *
	 * @return		A heap-allocated proxy rendering object corresponding to this decal component.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();

	/**
	 * @return		TRUE if both IsEnabled() and Super::IsValidComponent() return TRUE.
	 */
	virtual UBOOL IsValidComponent() const;

	// ActorComponent interface.
	virtual void CheckForErrors();

	// UObject interface;
	virtual void BeginPlay();
	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual void FinishDestroy();
	virtual void PreSave();
	virtual void Serialize(FArchive& Ar);

protected:
	void ReleaseResources(UBOOL bBlockOnRelease);

	// ActorComponent interface.
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach();
}

defaultproperties
{
	bAcceptsLights=FALSE
	bAcceptsDynamicLights=FALSE
	bCastDynamicShadow=FALSE

	bAcceptsDecals=FALSE

	Width=200
	Height=200
	TileX=1
	TileY=1
	Thickness=10
	NearPlane=0
	FarPlane=300
	FieldOfView=80
	HitNodeIndex=-1
	HitLevelIndex=-1
	DepthBias=-0.0001
	SlopeScaleDepthBias=0.0
	BackfaceAngle=0.001			// Corresponds to an angle of 89.94 degrees

	bProjectOnBSP=TRUE
	bProjectOnSkeletalMeshes=TRUE
	bProjectOnStaticMeshes=TRUE
	bProjectOnTerrain=TRUE
}
