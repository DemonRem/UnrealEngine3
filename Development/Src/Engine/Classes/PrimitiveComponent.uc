/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class PrimitiveComponent extends ActorComponent
	dependson(Scene)
	native
	noexport
	abstract;

/** Mirrored from Scene.h */
struct MaterialViewRelevance
{
	var bool bOpaque;
	var bool bTranslucent;
	var bool bDistortion;
	var bool bLit;
	var bool bUsesSceneColor;
};

/** The primitive's scene info. */
var private native transient const pointer SceneInfo{FPrimitiveSceneInfo};

/** A fence to track when the primitive is detached from the scene in the rendering thread. */
var private native const int DetachFence;

// Scene data.

var native transient const float LocalToWorldDeterminant;
var native transient const matrix LocalToWorld;
/**
 *	The index for the primitive component in the MotionBlurInfo array of the scene.
 *	Render-thread usage only.
 *	This assumes that there is only one scene that requires motion blur, as there is only
 *	a single index... If the application requires a primitive component to exist in multiple
 *	scenes and have motion blur in each of them, this can be changed into a mapping of the
 *	scene pointer to the index. (Associated functions would have to be updated as well...)
 */
var native transient const int MotionBlurInfoIndex;

var native private noimport const array<pointer> DecalList{class FDecalInteraction};

var const native transient int Tag;

// Shadow grouping.  An optimization which tells the renderer to use a single shadow for a group of primitive components.

var const PrimitiveComponent ShadowParent;

/** Keeps track of which fog component this primitive is using. */
var const transient FogVolumeDensityComponent FogVolumeComponent;
// Primitive generated bounds.

var const native transient BoxSphereBounds Bounds;

// Rendering flags.

/** The lighting environment to take the primitive's lighting from. */
var const LightEnvironmentComponent LightEnvironment;

/** Cull distance exposed to LDs. The real cull distance is the min (disregarding 0) of this and volumes affecting this object. */
var(Rendering) const private noexport float CullDistance;

/**
 * The distance to cull this primitive at.  A CullDistance of 0 indicates that the primitive should not be culled by distance.
 * The distance calculation uses the center of the primitive's bounding sphere.
 */
var(Rendering) editconst float CachedCullDistance;

/** The scene depth priority group to draw the primitive in. */
var(Rendering) const ESceneDepthPriorityGroup DepthPriorityGroup;

/** The scene depth priority group to draw the primitive in, if it's being viewed by its owner. */
var const ESceneDepthPriorityGroup ViewOwnerDepthPriorityGroup;

/** If detail mode is >= system detail mode, primitive won't be rendered. */
var(Rendering) const EDetailMode DetailMode;

/** Scalar controlling the amount of motion blur to be applied when object moves */
var(Rendering) float		MotionBlurScale;

/** True if the primitive should be rendered using ViewOwnerDepthPriorityGroup if viewed by its owner. */
var const bool bUseViewOwnerDepthPriorityGroup;

/** Whether to accept cull distance volumes to modify cached cull distance. */
var(Rendering) const bool	bAllowCullDistanceVolume;

var(Rendering) const bool	HiddenGame;
var(Rendering) const bool	HiddenEditor;

/** If this is True, this component won't be visible when the view actor is the component's owner, directly or indirectly. */
var(Rendering) const bool bOwnerNoSee;

/** If this is True, this component will only be visible when the view actor is the component's owner, directly or indirectly. */
var(Rendering) const bool bOnlyOwnerSee;

/** If this is True, this primitive will be used to occlusion cull other primitives. */
var(Rendering) bool bUseAsOccluder;

/** If this is True, this component doesn't need exact occlusion info. */
var(Rendering) bool bAllowApproximateOcclusion;

/** If TRUE, forces mips for textures used by this component to be resident when this component's level is loaded. */
var(Rendering) const bool bForceMipStreaming;

/** If TRUE, this primitive accepts decals.*/
var(Rendering) const bool bAcceptsDecals;

/** If TRUE, this primitive accepts decals spawned during gameplay.  Effectively FALSE if bAcceptsDecals is FALSE. */
var(Rendering) const bool bAcceptsDecalsDuringGameplay;

var native transient const bool bIsRefreshingDecals;

/** If TRUE, this primitive accepts foliage. */
var(Rendering) const bool bAcceptsFoliage;

/** 
 * Translucent objects with a lower sort priority draw before objects with a higher priority.
 * Translucent objects with the same priority are rendered from back-to-front based on their bounds origin.
 *
 * Ignored if the object is not translucent.
 * The default priority is zero. 
 **/
var(Rendering) int TranslucencySortPriority;

// Lighting flags

/**
 * Whether to cast any shadows or not
 *
 * controls whether the primitive component should cast a shadow or not. Currently dynamic primitives will not receive shadows from static objects unless both this flag and bCastDynamicSahdow are enabled.
 **/
var(Lighting)	bool		CastShadow;

/**
 * If true, forces all static lights to use light-maps for direct lighting on this primitive, regardless of the light's UseDirectLightMap property.
 *
 * forces the use of lightmaps for all static lights affecting this primitive even though the light might not be set to use light maps. This means that the primitive will not receive any shadows from dynamic objects obstructing static lights. It will correctly shadow in the case of dynamic lights
 */
var(Lighting)	const bool	bForceDirectLightMap;

/** If false, primitive does not cast dynamic shadows.
 *
 * controls whether the primitive should cast shadows in the case of non precomputed shadowing like e.g. the primitive being in between a light and a dynamic object. This flag is only used if CastShadow is TRUE. Currently dynamic primitives will not receive shadows from static objects unless both this flag and CastShadow are enabled.
 *
 **/
var(Lighting)	bool		bCastDynamicShadow;

/** 
 *	If TRUE, the primitive will cast shadows even if bHidden is TRUE.
 *
 *	Controls whether the primitive should cast shadows when hidden. 
 *	This flag is only used if CastShadow is TRUE. 
 *
 */
var(Lighting)	bool		bCastHiddenShadow;

/**
 * Does this primitive accept lights?
 *
 * controls whether the primitive accepts any lights. Should be set to FALSE for e.g. unlit objects as its a nice optimization - especially for larger objects.
 **/
var(Lighting)	const bool	bAcceptsLights;

/**
 * Whether this primitives accepts dynamic lights
 *
 * controls whether the object should be affected by dynamic lights.
 **/
var(Lighting)	const bool	bAcceptsDynamicLights;

/**
 * Lighting channels controlling light/ primitive interaction. Only allows interaction if at least one channel is shared
 *
 **/
var(Lighting)	const LightingChannelContainer	LightingChannels;

/** Whether the primitive supports/ allows static shadowing */
var(Lighting)	const bool	bUsePrecomputedShadows;

// Collision flags.

var	const bool	CollideActors;
var	const bool	BlockActors;
var	const bool	BlockZeroExtent;
var	const bool	BlockNonZeroExtent;
var(Collision)	const bool	BlockRigidBody;

/** DEPRECATED! Use RBChannel/RBCollideWithChannels instead now */
var	const bool	RigidBodyIgnorePawns;

/** Enum indicating different type of objects for rigid-body collision purposes. */
enum ERBCollisionChannel
{
	RBCC_Default,
	RBCC_Nothing, // Special channel that nothing should request collision with.
	RBCC_Pawn,
	RBCC_Vehicle,
	RBCC_Water,
	RBCC_GameplayPhysics,
	RBCC_EffectPhysics,
	RBCC_Untitled1,
	RBCC_Untitled2,
	RBCC_Untitled3,
	RBCC_Untitled4,
	RBCC_Cloth,
	RBCC_FluidDrain,
};

/** Enum indicating what type of object this should be considered for rigid body collision. */
var(Collision)	const ERBCollisionChannel	RBChannel;

/** 
 *	Container for indicating a set of collision channel that this object will collide with. 
 *	Mirrored manually in UnPhysPublic.h
 */
struct RBCollisionChannelContainer
{
	var()	const bool	Default;
	var		const bool	Nothing; // This is reserved to allow an object to opt-out of all collisions, and should not be set.
	var()	const bool	Pawn;
	var()	const bool	Vehicle;
	var()	const bool	Water;
	var()	const bool	GameplayPhysics;
	var()	const bool	EffectPhysics;
	var()	const bool	Untitled1;
	var()	const bool	Untitled2;
	var()	const bool	Untitled3;
	var()	const bool	Untitled4;
	var()	const bool	Cloth;
	var()	const bool	FluidDrain;
};

/** Types of objects that this physics objects will collide with. */
var(Collision) const RBCollisionChannelContainer	RBCollideWithChannels;

/**
 *	Flag that indicates if OnRigidBodyCollision function should be called for physics collisions involving this PrimitiveComponent.
 */
var(Physics) const bool	bNotifyRigidBodyCollision;

// Novodex fluids

/** Whether this object should act as a 'drain' for fluid, and destroy fluid particles when they contact it. */
var(Physics) const bool	bFluidDrain;

/** Indicates that fluid interaction with this object should be 'two-way' - that is, force should be applied to both fluid and object. */
var(Physics) const bool	bFluidTwoWay;

// Physics

/** Will ignore radial impulses applied to this component. */
var(Physics)	bool		bIgnoreRadialImpulse;

/** Will ignore radial forces applied to this component. */
var(Physics)	bool		bIgnoreRadialForce;

/** Will ignore force field applied to this component. */
var(Physics)	bool		bIgnoreForceField;

/** Place into a NxCompartment that will run in parallel with the primary scene's physics with potentially different simulation parameters.
 *  If double buffering is enabled in the WorldInfo then physics will run in parallel with the entire game for this component. */
var(Physics)	const bool		bUseCompartment;

// General flags.

/** If this is True, this component must always be loaded on clients, even if HiddenGame && !CollideActors. */
var private const bool AlwaysLoadOnClient;

/** If this is True, this component must always be loaded on servers, even if !CollideActors. */
var private const bool AlwaysLoadOnServer;

/**
* @fixme JF, Hack for gears ship to allow certain components to render even if the parent actor
* is part of the camera's HiddenActors array.  Potentially worth cleaning up and integrating
* back to mainline, but I'm not really caring about that right now. :)
*/
var() bool bIgnoreHiddenActorsMembership;

// Internal scene data.

var const native transient bool bWasSNFiltered;
var const native transient array<int> OctreeNodes;

// Internal physics engine data.

/** Allows you to override the PhysicalMaterial to use for this PrimitiveComponent. */
var(Physics)	const PhysicalMaterial			PhysMaterialOverride;

var				const native RB_BodyInstance	BodyInstance;

/** 
 *	Used for creating one-way physics interactions (via constraints or contacts) 
 *	Groups with lower RBDominanceGroup push around higher values in a 'one way' fashion. Must be <32.
 */
var(Physics)	byte		RBDominanceGroup;

// Properties moved from TransformComponent
var native transient const matrix CachedParentToWorld; //@todo please remove me if possible

var() const vector			Translation;
var() const rotator			Rotation;
var() const float			Scale;
var() const vector			Scale3D;

var() const bool			AbsoluteTranslation;
var() const bool			AbsoluteRotation;
var() const bool			AbsoluteScale;

/** Last time the component was submitted for rendering (called FScene::AddPrimitive). */
var const transient float	LastSubmitTime;

/** Last render time in seconds since level started play. Updated to WorldInfo->TimeSeconds so float is sufficient. */
var transient float	LastRenderTime;

//=============================================================================
// Physics.

/** if > 0, the script RigidBodyCollision() event will be called on our Owner when a physics collision involving
 * this PrimitiveComponent occurs and the relative velocity is greater than or equal to this
 */
var float ScriptRigidBodyCollisionThreshold;

/** Enum for controlling the falloff of strength of a radial impulse as a function of distance from Origin. */
enum ERadialImpulseFalloff
{
	/** Impulse is a constant strength, up to the limit of its range. */
	RIF_Constant,

	/** Impulse should get linearly weaker the further from origin. */
	RIF_Linear
};

/**
 *	Add an impulse to the physics of this PrimitiveComponent.
 *
 *	@param	Impulse		Magnitude and direction of impulse to apply.
 *	@param	Position	Point in world space to apply impulse at. If Position is (0,0,0), impulse is applied at center of mass ie. no rotation.
 *	@param	BoneName	If a SkeletalMeshComponent, name of bone to apply impulse to.
 *	@param	bVelChange	If true, the Strength is taken as a change in velocity instead of an impulse (ie. mass will have no affect).
 */
native final function AddImpulse(vector Impulse, optional vector Position, optional name BoneName, optional bool bVelChange);

/**
 * Add an impulse to this component, radiating out from the specified position.
 * In the case of a skeletal mesh, may affect each bone of the mesh.
 *
 * @param Origin		Point of origin for the radial impulse blast
 * @param Radius		Size of radial impulse. Beyond this distance from Origin, there will be no affect.
 * @param Strength		Maximum strength of impulse applied to body.
 * @param Falloff		Allows you to control the strength of the impulse as a function of distance from Origin.
 * @param bVelChange	If true, the Strength is taken as a change in velocity instead of an impulse (ie. mass will have no affect).
 */
native final function AddRadialImpulse(vector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, optional bool bVelChange);

/**
 *	Add a force to this component.
 *	@param Force		Force vector to apply. Magnitude indicates strength of force.
 *	@param Position		Position on object to apply force. If (0,0,0), force is applied at center of mass.
 *	@param BoneName		Used in the skeletal case to apply a force to a single body.
 */
native final function AddForce(vector Force, optional vector Position, optional name BoneName);

/**
 *	Add a force originating from the supplied world-space location.
 *
 *	@param Origin		Origin of force in world space.
 *	@param Radius		Radius within which to apply the force.
 *	@param Strength		Strength of force to apply.
 *  @param Falloff		Allows you to control the strength of the force as a function of distance from Origin.
 */
native final function AddRadialForce(vector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff);

/**
 * Set the linear velocity of the rigid body physics of this PrimitiveComponent. If no rigid-body physics is active, will do nothing.
 * In the case of a SkeletalMeshComponent will affect all bones.
 * This should be used cautiously - it may be better to use AddForce or AddImpulse.
 *
 * @param	NewVel			New linear velocity to apply to physics.
 * @param	bAddToCurrent	If true, NewVel is added to the existing velocity of the body.
 */
native final function SetRBLinearVelocity(vector NewVel, optional bool bAddToCurrent);

/**
 * Set the angular velocity of the rigid body physics of this PrimitiveComponent. If no rigid-body physics is active, will do nothing.
 * In the case of a SkeletalMeshComponent will affect all bones - and will apply the linear velocity necessary to get all bones to rotate around the root.
 * This should be used cautiously - it may be better to use AddForce or AddImpulse.
 *
 * @param	NewAngVel		New angular velocity to apply to physics.
 * @param	bAddToCurrent	If true, NewAngVel is added to the existing velocity of the body.
 */
native final function SetRBAngularVelocity(vector NewAngVel, optional bool bAddToCurrent);

/**
 * Called if you want to move the physics of a component which has dynamics running (ie actor is in PHYS_RigidBody).
 * Be careful calling this when this is jointed to something else, or when it does not fit in the destination (no checking is done).
 * @param NewPos new position of the body
 * @param BoneName (SkeletalMeshComponent only) if specified, the bone to change position of
 * 			if not specified for a SkeletalMeshComponent, all bodies are moved by the delta
 * 			between the desired location and that of the root body.
 */
native final function SetRBPosition(vector NewPos, optional name BoneName);

/**
 * Called if you want to rotate the physics of a component which has dynamics running (ie actor is in PHYS_RigidBody).
 * @param NewRot new rotation of the body
 * @param BoneName (SkeletalMeshComponent only) if specified, the bone to change rotation of
 * 			if not specified for a SkeletalMeshComponent, all bodies are moved by the delta
 * 			between the desired rotation and that of the root body.
 */
native final function SetRBRotation(rotator NewRot, optional name BoneName);

/**
 *	Ensure simulation is running for this component.
 *	If a SkeletalMeshComponent and no BoneName is specified, will wake all bones in the PhysicsAsset.
 */
native final function WakeRigidBody(optional name BoneName);

/**
 * Put a simulation back to see.
 */
native final function PutRigidBodyToSleep(optional name BoneName);

/**
 *	Returns if the body is currently awake and simulating.
 *	If a SkeletalMeshComponent, and no BoneName is specified, will pick a random bone -
 *	so does not make much sense if not all bones are jointed together.
 */
native final function bool RigidBodyIsAwake(optional name BoneName);

/**
 *	Change the value of BlockRigidBody.
 *
 *	@param NewBlockRigidBody - The value to assign to BlockRigidBody.
 */
native final function SetBlockRigidBody(bool bNewBlockRigidBody);

/** 
 *	Changes a member of the RBCollideWithChannels container for this PrimitiveComponent.
 */
final native function SetRBCollidesWithChannel(ERBCollisionChannel Channel, bool bNewCollides);

/**
 *	Changes the rigid-body channel that this object is defined in.
 */
final native function SetRBChannel(ERBCollisionChannel Channel);

/** Changes the value of bNotifyRigidBodyCollision
 * @param bNewNotifyRigidBodyCollision - The value to assign to bNotifyRigidBodyCollision
 */
native final function SetNotifyRigidBodyCollision(bool bNewNotifyRigidBodyCollision);

/** 
 *	Changes the current PhysMaterialOverride for this component. 
 *	Note that if physics is already running on this component, this will _not_ alter its mass/inertia etc, it will only change its 
 *	surface properties like friction and the damping.
 */
native final function SetPhysMaterialOverride(PhysicalMaterial NewPhysMaterial);

/** returns the physics RB_BodyInstance for the root body of this component (if any) */
native final function RB_BodyInstance GetRootBodyInstance();

/** 
 *	Used for creating one-way physics interactions.
 *	@see RBDominanceGroup
 */
native final function SetRBDominanceGroup(BYTE InDomGroup);

/**
 * Changes the value of HiddenGame.
 *
 * @param NewHidden	- The value to assign to HiddenGame.
 */
native final function SetHidden(bool NewHidden);

/**
 * Changes the value of bOwnerNoSee.
 */
native final function SetOwnerNoSee(bool bNewOwnerNoSee);

/**
 * Changes the value of bOnlyOwnerSee.
 */
native final function SetOnlyOwnerSee(bool bNewOnlyOwnerSee);

/**
 * Changes the value of ShadowParent.
 * @param NewShadowParent - The value to assign to ShadowParent.
 */
native final function SetShadowParent(PrimitiveComponent NewShadowParent);

/**
 * Changes the value of LightEnvironment.
 * @param NewLightEnvironment - The value to assign to LightEnvironment.
 */
native final function SetLightEnvironment(LightEnvironmentComponent NewLightEnvironment);

/**
 * Changes the value of CullDistance.
 * @param NewCullDistance - The value to assign to CullDistance.
 */
native final function SetCullDistance(float NewCullDistance);

/**
 * Changes the value of LightingChannels.
 * @param NewLightingChannels - The value to assign to LightingChannels.
 */
native final function SetLightingChannels(LightingChannelContainer NewLightingChannels);

/**
 * Changes the value of DepthPriorityGroup.
 * @param NewDepthPriorityGroup - The value to assign to DepthPriorityGroup.
 */
native final function SetDepthPriorityGroup(ESceneDepthPriorityGroup NewDepthPriorityGroup);

/**
 * Changes the value of bUseViewOwnerDepthPriorityGroup and ViewOwnerDepthPriorityGroup.
 * @param bNewUseViewOwnerDepthPriorityGroup - The value to assign to bUseViewOwnerDepthPriorityGroup.
 * @param NewViewOwnerDepthPriorityGroup - The value to assign to ViewOwnerDepthPriorityGroup.
 */
native final function SetViewOwnerDepthPriorityGroup(
	bool bNewUseViewOwnerDepthPriorityGroup,
	ESceneDepthPriorityGroup NewViewOwnerDepthPriorityGroup
	);

native final function SetTraceBlocking(bool NewBlockZeroExtent, bool NewBlockNonZeroExtent);

native final function SetActorCollision(bool NewCollideActors, bool NewBlockActors);

// Copied from TransformComponent
native function SetTranslation(vector NewTranslation);
native function SetRotation(rotator NewRotation);
native function SetScale(float NewScale);
native function SetScale3D(vector NewScale3D);
native function SetAbsolute(optional bool NewAbsoluteTranslation,optional bool NewAbsoluteRotation,optional bool NewAbsoluteScale);

final function vector GetPosition()
{
	local vector Position;

	Position.X = LocalToWorld.WPlane.X;
	Position.Y = LocalToWorld.WPlane.Y;
	Position.Z = LocalToWorld.WPlane.Z;
	return Position;
}

defaultproperties
{
	Scale=1.0
	MotionBlurScale=1.0
	Scale3D=(X=1.0,Y=1.0,Z=1.0)
	DepthPriorityGroup=SDPG_World
	bAllowCullDistanceVolume=TRUE
	bUseAsOccluder=FALSE
	CastShadow=FALSE
	bCastDynamicShadow=TRUE
	bAcceptsLights=FALSE
	bAcceptsDynamicLights=TRUE
	bAcceptsDecals=FALSE
	bAcceptsDecalsDuringGameplay=TRUE
	bAcceptsFoliage=TRUE
	AlwaysLoadOnClient=TRUE
	AlwaysLoadOnServer=TRUE
	RBChannel=RBCC_Default
	RBDominanceGroup=15
}
