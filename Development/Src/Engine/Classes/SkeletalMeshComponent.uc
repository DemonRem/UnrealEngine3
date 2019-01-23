/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SkeletalMeshComponent extends MeshComponent
	native
	noexport
	AutoExpandCategories(Collision,Rendering,Lighting)
	dependson(AnimNode)
	hidecategories(Object)
	editinlinenew;

/** The skeletal mesh used by this component. */
var()	SkeletalMesh			SkeletalMesh;

/** The SkeletalMeshComponent that this one is possibly attached to. */
var		SkeletalMeshComponent	AttachedToSkelComponent;

/**
 *	This should point to the AnimTree in a content package.
 *	BeginPlay on this SkeletalMeshComponent will instance (copy) the tree and assign the instance to the Animations pointer.
 */
var()   const					AnimTree	AnimTreeTemplate;

/**
 *	This is the unique instance of the AnimTree used by this SkeletalMeshComponent.
 *	THIS SHOULD NEVER POINT TO AN ANIMTREE IN A CONTENT PACKAGE.
 */
var()	const editinline export	AnimNode	Animations;

/** Array of all AnimNodes in entire tree, in the order they should be ticked - that is, all parents appear before a child. */
var		const transient array<AnimNode>		AnimTickArray;

/**
 *	Physics and collision information used for this SkeletalMesh, set up in PhAT.
 *	This is used for per-bone hit detection, accurate bounding box calculation and ragdoll physics for example.
 */
var()	const PhysicsAsset										PhysicsAsset;

/**
 *	Any instanced physics engine information for this SkeletalMeshComponent.
 *	This is only required when you want to run physics or you want physical interaction with this skeletal mesh.
 */
var		const transient editinline export PhysicsAssetInstance	PhysicsAssetInstance;

/**
 *	Influence of rigid body physics on the mesh's pose (0.0 == use only animation, 1.0 == use only physics)
 */
var()	float					PhysicsWeight;

/** Used to scale speed of all animations on this skeletal mesh. */
var()	float					GlobalAnimRateScale;

var native transient const pointer MeshObject;
var() Color						WireframeColor;

/** Temporary array of of component-space bone matrices, update each frame and used for rendering the mesh. */
var native transient const array<matrix>				SpaceBases;

/** Temporary array of local-space (ie relative to parent bone) rotation/translation for each bone. */
var native transient const array<AnimNode.BoneAtom>		LocalAtoms;

/** Temporary array of bone indices required this frame. Filled in by UpdateSkelPose. */
var native transient const array<byte>					RequiredBones;

/**
 *	If set, this SkeletalMeshComponent will not use its Animations pointer to do its own animation blending, but will
 *	use the SpaceBases array in the ParentAnimComponent. This is used when constructing a character using multiple skeletal meshes sharing the same
 *	skeleton within the same Actor.
 */
var()	const SkeletalMeshComponent	ParentAnimComponent;

/**
 *	Mapping between bone indices in this component and the parent one. Each element is the index of the bone in the ParentAnimComponent.
 *	Size should be the same as SkeletalMesh.RefSkeleton size (ie number of bones in this skeleton).
 */
var native transient const array<int> ParentBoneMap;

/**
 *	The set of AnimSets that will be looked in to find a particular sequence, specified by name in an AnimNodeSequence.
 *	Array is search from last to first element, so you can replace a particular sequence but putting a set containing the new version later in the array.
 *	You will need to call SetAnim again on nodes that may be affected by any changes you make to this array.
 */
var()	array<AnimSet>			AnimSets;

// Morph targets

/**
 *	Array of MorphTargetSets that will be looked in to find a particular MorphTarget, specified by name.
 *	It is searched in the same way as the AnimSets array above.
 */
var()	array<MorphTargetSet>	MorphSets;

/** Struct used to indicate one active morph target that should be applied to this SkeletalMesh when rendered. */
struct ActiveMorph
{
	/** The morph target that we want to apply. */
	var	MorphTarget		Target;

	/** Strength of the morph target, between 0.0 and 1.0 */
	var	float			Weight;
};

/** Array indicating all active MorphTargets. This array is updated inside UpdateSkelPose based on the AnimTree's st of MorphNodes. */
var	array<ActiveMorph>	ActiveMorphs;


// Attachments.

struct Attachment
{
	var() editinline ActorComponent	Component;
	var() name						BoneName;
	var() vector					RelativeLocation;
	var() rotator					RelativeRotation;
	var() vector					RelativeScale;

	structdefaultproperties
	{
		RelativeScale=(X=1,Y=1,Z=1)
	}
};

var duplicatetransient const array<Attachment> Attachments;

var	transient const array<byte>	SkelControlIndex;

var() int			ForcedLodModel; // if 0, auto-select LOD level. if >0, force to (ForcedLodModel-1)
var	int			PredictedLODLevel;
var	int			OldPredictedLODLevel; // LOD level from previous frame, so we can detect changes in LOD to recalc required bones

/**	High (best) DistanceFactor that was desired for rendering this SkeletalMesh last frame. Represents how big this mesh was in screen space   */
var const float		MaxDistanceFactor;

var int			bForceWireframe;	// Forces the mesh to draw in wireframe mode.
var int			bForceRefpose;

/** Draw the skeleton hierarchy for this skel mesh. */
var int			bDisplayBones;

/** Bool that enables debug drawing of the skeleton before it is passed to the physics. Useful for debugging animation-driven physics. */
var int			bShowPrePhysBones;

var int			bHideSkin;
var int			bForceRawOffset;
var int			bIgnoreControllers;
var	int			bTransformFromAnimParent;
var	const transient int	TickTag;
var const transient int	CachedAtomsTag;
var	const int	bUseSingleBodyPhysics;
var transient int	bRequiredBonesUpToDate;

/**
 *	If non-zero, skeletal mesh component will not update kinematic bones and bone springs when distance factor is greater than this (or has not been rendered for a while).
 *	This also turns off BlockRigidBody, so you do not get collisions with 'left behind' ragdoll setups.
 */
var float		MinDistFactorForKinematicUpdate;

/** When blending in physics, translate data from physics so that this bone aligns with its current graphics position. Can be useful for hacking 'frame behind' issues. */
var	name		PhysicsBlendZeroDriftBoneName;

/** if true, update skeleton/attachments even when our Owner has not been rendered recently
 * @note if this is false, bone information may not be accurate, so be careful setting this to false if bone info is relevant to gameplay
 * @note you can use ForceSkelUpdate() to force an update
 */
var bool bUpdateSkelWhenNotRendered;

/** If true, do not apply any SkelControls when owner has not been rendered recently. */
var bool bIgnoreControllersWhenNotRendered;

/** If this is true, we are not updating kinematic bones and motors based on animation beacause the skeletal mesh is too far from any viewer. */
var	const bool bNotUpdatingKinematicDueToDistance;

/** force root motion to be discarded, no matter what the AnimNodeSequence(s) are set to do */
var() bool bForceDiscardRootMotion;

/**
 * if TRUE, notify owning actor of root motion mode changes.
 * This calls the Actor.RootMotionModeChanged() event.
 * This is useful for synchronizing movements.
 * For intance, when using RMM_Translate, and the event is called, we know that root motion will kick in on next frame.
 * It is possible to kill in-game physics, and then use root motion seemlessly.
 */
var bool bRootMotionModeChangeNotify;

/**
 * if TRUE, the event RootMotionExtracted() will be called on this owning actor,
 * after root motion has been extracted, and before it's been used.
 * This notification can be used to alter extracted root motion before it is forwarded to physics.
 */
var bool bRootMotionExtractedNotify;

/** If true, FaceFX will not automatically create material instances. */
var() bool bDisableFaceFXMaterialInstanceCreation;

/**
 *	Indicates whether this SkeletalMeshComponent should have a physics engine representation of its state.
 *	@see SetHasPhysicsAssetInstance
 */
var() const bool bHasPhysicsAssetInstance;

/** If we are running physics, should we update bFixed bones based on the animation bone positions. */
var() bool	bUpdateKinematicBonesFromAnimation;

/**
 *	If we should pass joint position to joints each frame, so that they can be used by motorised joints to drive the
 *	ragdoll based on the animation.
 */
var() bool	bUpdateJointsFromAnimation;

/** Indicates whether this SkeletalMeshComponent is currently considered 'fixed' (ie kinematic) */
var const bool	bSkelCompFixed;

/** Used for consistancy checking. Indicates that the results of physics have been blended into SpaceBases this frame. */
var	const bool	bHasHadPhysicsBlendedIn;

/** 
 *	If true, attachments will be updated twice a frame - once in Tick and again when UpdateTransform is called. 
 *	This can resolve some 'frame behind' issues if an attachment need to be in the correct location for it's Tick, but at a cost.
 */
var() bool	bForceUpdateAttachmentsInTick;

/** Enables blending in of physics bodies with the bAlwaysFullAnimWeight flag set. */
var() bool	bEnableFullAnimWeightBodies;

/**
 *	If true, when this skeletal mesh overlaps a physics volume, each body of it will be tested against the volume, so only limbs
 *	actually in the volume will be affected. Useful when gibbing bodies.
 */
var() bool		bPerBoneVolumeEffects;

/** If true, will move the Actors Location to match the root rigid body location when in PHYS_RigidBody. */
var() bool		bSyncActorLocationToRootRigidBody;

/** If TRUE, force usage of raw animation data when animating this skeltal mesh; if FALSE, use compressed data. */
var const bool	bUseRawData;

/** Disable warning when an AnimSequence is not found. FALSE by default. */
var bool		bDisableWarningWhenAnimNotFound;

/** if set, components that are attached to us have their bOwnerNoSee and bOnlyOwnerSee properties overridden by ours */
var bool bOverrideAttachmentOwnerVisibility;

/** pauses this component's animations (doesn't tick them) */
var bool bPauseAnims;
/** If true, DistanceFactor for this SkeletalMeshComponent will be added to global chart. */
var bool	bChartDistanceFactor;
// CLOTH
// Under Development! Not a fully supported feature at the moment.

/**
 *	Whether cloth simulation should currently be used on this SkeletalMeshComponent.
 *	@see SetEnableClothSimulation
 */
var(Cloth)	const bool		bEnableClothSimulation;

/** Turns off all cloth collision so not checks are done (improves performance). */
var(Cloth)	const bool		bDisableClothCollision;

/** If true, cloth is 'frozen' and no simulation is taking place for it, though it will keep its shape. */
var(Cloth)	const bool		bClothFrozen;

/** If true, cloth will automatically have bClothFrozen set when it is not rendered, and have it turned off when it is seen. */
var(Cloth)	bool			bAutoFreezeClothWhenNotRendered;

/** It true, clamp velocity of cloth particles to be within ClothOwnerVelClampRange of Base velocity. */
var(Cloth)	bool			bClothBaseVelClamp;

/** If true, fixed verts of the cloth are attached in the physics to the physics body that this components actor is attached to. */
var(Cloth)	bool			bAttachClothVertsToBaseBody;

/** Constant force applied to all vertices in the cloth. */
var(Cloth)	const vector	ClothExternalForce;

/** 'Wind' force applied to cloth. Force on each vertex is based on the dot product between the wind vector and the surface normal. */
var(Cloth)	vector			ClothWind;

/** Amount of variance from base's velocity the cloth is allowed. */
var(Cloth)	vector			ClothBaseVelClampRange;

/** How much to blend in results from cloth simulation with results from regular skinning. */
var(Cloth)	float			ClothBlendWeight;

var const native transient pointer	ClothSim;
var const native transient int		SceneIndex;

var const array<vector> ClothMeshPosData;
var const array<vector> ClothMeshNormalData;
var const array<int> ClothMeshIndexData;
var int NumClothMeshVerts;
var int NumClothMeshIndices;

/** Cloth parent indices contain the index of the original vertex when a vertex is created during tearing.
 *  If it is an original vertex then the parent index is the same as the vertex index.
 */
var const array<int> ClothMeshParentData;
var int NumClothMeshParentIndices;

/** bufferes used for reverse lookups to unweld vertices to support wrapped UVs. */
var const native transient array<vector>	ClothMeshWeldedPosData;
var const native transient array<vector>	ClothMeshWeldedNormalData;
var const native transient array<int>		ClothMeshWeldedIndexData;

/** flags to indicate which buffers were recently updated by the cloth simulation. */
var int ClothDirtyBufferFlag;

/** Enum indicating what type of object this cloth should be considered for rigid body collision. */
var(Cloth)	const ERBCollisionChannel			ClothRBChannel;

/** Types of objects that this cloth will collide with. */
var(Cloth)	const RBCollisionChannelContainer	ClothRBCollideWithChannels;

/** How much force to apply to cloth, in relation to the force(from a force field) applied to rigid bodies(zero applies no force to cloth, 1 applies the same) */
var(Cloth)	const float				ClothForceScale;

/** 
    The cloth tear factor for this SkeletalMeshComponent, negative values take the tear factor from the SkeletalMesh.
    Note: UpdateClothParams() should be called after modification so that the changes are reflected in the simulation.
*/
var(Cloth)	const float				ClothAttachmentTearFactor;

var	material	LimitMaterial;

/** Root Motion extracted from animation. */
var const	transient	BoneAtom	RootMotionDelta;
/** Root Motion velocity for this frame, set from RootMotionDelta. */
var			transient	Vector		RootMotionVelocity;

/**
 * Offset of the root bone from the reference pose.
 * Used to offset bounding box.
 */
var const transient Vector	RootBoneTranslation;

/** Scale applied in physics when RootMotionMode == RMM_Accel */
var vector RootMotionAccelScale;

enum ERootMotionMode
{
	RMM_Translate,	// move actor with root motion
	RMM_Velocity,	// extract magnitude from root motion, and limit max Actor velocity with it.
	RMM_Ignore,		// do nothing
	RMM_Accel,		// extract velocity from root motion and use it to derive acceleration of the Actor
};
var() ERootMotionMode		RootMotionMode;
/** Previous Root Motion Mode, to catch changes */
var	const ERootMotionMode	PreviousRMM;

var		ERootMotionMode		PendingRMM;
var		ERootMotionMode		OldPendingRMM;

/** Handle one frame delay with PendingRMM */
var	const INT				bRMMOneFrameDelay;

/** Root Motion Rotation mode */
enum ERootMotionRotationMode
{
	/** Ignore rotation delta passed from animation. */
	RMRM_Ignore,
	/** Apply rotation delta to actor */
	RMRM_RotateActor,
};
var() ERootMotionRotationMode RootMotionRotationMode;

enum EFaceFXBlendMode
{
	/**
	 * Face FX overwrites relevant bones on skeletal mesh.
	 * Default.
	 */
	FXBM_Overwrite,
	/**
	 * Face FX transforms are relative to ref skeleton and added
	 * in parent bone space.
	 */
	FXBM_Additive,
};

/** How FaceFX transforms should be blended with skeletal mesh */
var()	EFaceFXBlendMode	FaceFXBlendMode;

/** The valid FaceFX register operations. */
enum EFaceFXRegOp
{
	FXRO_Add,	   // Add the register value with the Face Graph node value.
	FXRO_Multiply, // Multiply the register value with the Face Graph node value.
	FXRO_Replace,  // Replace the Face Graph node value with the register value.
};

/** The FaceFX actor instance associated with the skeletal mesh component. */
var transient native pointer FaceFXActorInstance;

/**
 *	The audio component that we are using to play audio for a facial animation.
 *	Assigned in PlayFaceFXAnim and cleared in StopFaceFXAnim.
 */
var AudioComponent	CachedFaceFXAudioComp;

//=============================================================================
// Animation.

// Attachment functions.
native final function AttachComponent(ActorComponent Component,name BoneName,optional vector RelativeLocation,optional rotator RelativeRotation,optional vector RelativeScale);
native final function DetachComponent(ActorComponent Component);


/**
 * Attach an ActorComponent to a Socket.
 */

native final function AttachComponentToSocket(ActorComponent Component, name SocketName);

/**
 *	Find the current world space location and rotation of a named socket on the skeletal mesh component.
 *	If the socket is not found, then it returns false and does not change the OutLocation/OutRotation variables.
 *	@param InSocketName the name of the socket to find
 *	@param OutLocation (out) set to the world space location of the socket
 *	@param OutRotation (out) if specified, set to the world space rotation of the socket
 *	@return whether or not the socket was found
 */
native final function bool GetSocketWorldLocationAndRotation(name InSocketName, out vector OutLocation, optional out rotator OutRotation);


/**
 * Returns SkeletalMeshSocket of named socket on the skeletal mesh component.
 * Returns None if not found.
 */

native final function SkeletalMeshSocket GetSocketByName( Name InSocketName );


/**
 * Returns component attached to specified BoneName. (returns the first entry found).
 * @param	BoneName		Bone Name to look up.
 * @return	First ActorComponent found, attached to BoneName, if it exists.
 */

native final function ActorComponent FindComponentAttachedToBone( name InBoneName );


/**
 * Returns true if component is attached to skeletal mesh.
 * @param	Component	ActorComponent to check for.
 * @return	true if Component is attached to SkeletalMesh.
 */

native final function bool IsComponentAttached( ActorComponent Component, optional Name BoneName );

/** returns all attached components that are of the specified class or a subclass
 * @param BaseClass the base class of ActorComponent to return
 * @param (out) OutComponent the returned ActorComponent for each iteration
 */
native final iterator function AttachedComponents(class<ActorComponent> BaseClass, out ActorComponent OutComponent);

// Skeletal animation.

/** Change the SkeletalMesh that is rendered for this Component. Will re-initialise the animation tree etc. */
simulated native final function SetSkeletalMesh(SkeletalMesh NewMesh, optional bool bKeepSpaceBases);

/** Change the Physics Asset of the mesh */
simulated native final function SetPhysicsAsset(PhysicsAsset NewPhysicsAsset, optional bool bForceReInit);

// Cloth

/** Turn on and off cloth simulation for this skeletal mesh. */
simulated native final function SetEnableClothSimulation(bool bInEnable);

/** Toggle active simulation of cloth. Cheaper than doing SetEnableClothSimulation, and keeps its shape while frozen. */
simulated native final function SetClothFrozen(bool bNewFrozen);

/** Update params of the this components internal cloth sim from the SkeletalMesh properties. */
simulated native final function UpdateClothParams();

/** Modify the external force that is applied to the cloth. Will continue to be applied until it is changed. */
simulated native final function SetClothExternalForce(vector InForce);

/** Attach/detach verts from physics body that this components actor is attached to. */
simulated native final function SetAttachClothVertsToBaseBody(bool bAttachVerts);

/**
 * Find a named AnimSequence from the AnimSets array in the SkeletalMeshComponent.
 * This searches array from end to start, so specific sequence can be replaced by putting a set containing a sequence with the same name later in the array.
 *
 * @param AnimSeqName Name of AnimSequence to look for.
 *
 * @return Pointer to found AnimSequence. Returns NULL if could not find sequence with that name.
 */
native final function AnimSequence FindAnimSequence( Name AnimSeqName );


/**
 * Finds play Rate for a named AnimSequence to match a specified Duration in seconds.
 *
 * @param	AnimSeqName	Name of AnimSequence to look for.
 * @param	Duration	in seconds to match
 *
 * @return	play rate of animation, so it plays in <duration> seconds.
 */
final function float GetAnimRateByDuration( Name AnimSeqName, float Duration )
{
	local AnimSequence AnimSeq;

	AnimSeq = FindAnimSequence( AnimSeqName );
	if( AnimSeq == None )
	{
		return 1.f;
	}

	return (AnimSeq.SequenceLength / Duration);
}


/** Returns the duration (in seconds) for a named AnimSequence. Returns 0.f if no animation. */
final function float GetAnimLength(Name AnimSeqName)
{
	local AnimSequence AnimSeq;

	AnimSeq = FindAnimSequence(AnimSeqName);
	if( AnimSeq == None )
	{
		return 0.f;
	}

	return (AnimSeq.SequenceLength / AnimSeq.RateScale);
}


/**
 * Find a named MorphTarget from the MorphSets array in the SkeletalMeshComponent.
 * This searches the array in the same way as FindAnimSequence
 *
 * @param AnimSeqName Name of MorphTarget to look for.
 *
 * @return Pointer to found MorphTarget. Returns NULL if could not find target with that name.
 */
native final function MorphTarget FindMorphTarget( Name MorphTargetName );

/**
 * Find an Animation Node in the Animation Tree whose NodeName matches InNodeName.
 * Warning: The search is O(n^2), so for large AnimTrees, cache result.
 */
native final function	AnimNode			FindAnimNode(name InNodeName);

/** returns all AnimNodes in the animation tree that are the specfied class or a subclass
 * @param BaseClass base class to return
 * @param Node (out) the returned AnimNode for each iteration
 */
native final iterator function AllAnimNodes(class<AnimNode> BaseClass, out AnimNode Node);

native final function	SkelControlBase		FindSkelControl( name InControlName );

native final function	MorphNodeBase		FindMorphNode( name InNodeName );

native final function quat GetBoneQuaternion( name BoneName, optional int Space ); // 0 == World, 1 == Local (Component)
native final function vector GetBoneLocation( name BoneName, optional int Space ); // 0 == World, 1 == Local (Component)

/** returns the bone index of the specified bone, or INDEX_NONE if it couldn't be found */
native final function int MatchRefBone( name BoneName );

/** returns the matrix of the bone at the specified index */
native final function matrix GetBoneMatrix( int BoneIndex );

/** returns the name of the parent bone for the specified bone. Returns 'None' if the bone does not exist or it is the root bone */
native final function name GetParentBone(name BoneName);

/** fills the given array with the names of all the bones in this component's current SkeletalMesh */
native final function GetBoneNames(out array<name> BoneNames);

/** finds a vector pointing along the given axis of the given bone
 * @param BoneName the name of the bone to find
 * @param Axis the axis of that bone to return
 * @return the direction of the specified axis, or (0,0,0) if the specified bone was not found
 */
native final function vector GetBoneAxis(name BoneName, EAxis Axis);

/**
 *	Transform a location/rotation from world space to bone relative space.
 *	This is handy if you know the location in world space for a bone attachment, as AttachComponent takes location/rotation in bone-relative space.
 */
native final function TransformToBoneSpace( name BoneName, vector InPosition, rotator InRotation, out vector OutPosition, out rotator OutRotation );

/**
 *	Transfor a location/rotation in bone relative space to world space.
 */
native final function TransformFromBoneSpace( name BoneName, vector InPosition, rotator InRotation, out vector OutPosition, out rotator OutRotation );

/** finds the closest bone to the given location
 * @param TestLocation the location to test against
 * @param BoneLocation (optional, out) if specified, set to the world space location of the bone that was found, or (0,0,0) if no bone was found
 * @param IgnoreScale (optional) if specified, only bones with scaling larger than the specified factor are considered
 * @return the name of the bone that was found, or 'None' if no bone was found
 */
native final function name FindClosestBone(vector TestLocation, optional out vector BoneLocation, optional float IgnoreScale);

native final function SetAnimTreeTemplate(AnimTree NewTemplate);
native final function SetParentAnimComponent(SkeletalMeshComponent NewParentAnimComp);

native final function UpdateParentBoneMap();
native final function InitSkelControls();

final native function int	FindConstraintIndex(name ConstraintName);
final native function name	FindConstraintBoneName(int ConstraintIndex);

/** Find a BodyInstance by BoneName */
final native function	RB_BodyInstance	FindBodyInstanceNamed(Name BoneName);

/**
 *	Set value of bHasPhysicsAssetInstance flag.
 *	Will create/destroy PhysicsAssetInstance as desired.
 */
final native function SetHasPhysicsAssetInstance(bool bHasInstance);

/** Force an update of this meshes kinematic bodies and springs. */
native final function UpdateRBBonesFromSpaceBases(bool bMoveUnfixedBodies, bool bTeleport);

/** forces an update to the mesh's skeleton/attachments, even if bUpdateSkelWhenNotRendered is false and it has not been recently rendered
 * @note if bUpdateSkelWhenNotRendered is true, there is no reason to call this function (but doing so anyway will have no effect)
 */
native final function ForceSkelUpdate();

/**
 * Force AnimTree to recache all animations.
 * Call this when the AnimSets array has been changed.
 */
native final function UpdateAnimations();

/**
 *	Find all bones by name within given radius
 */
native final function bool GetBonesWithinRadius( Vector Origin, FLOAT Radius, INT TraceFlags, out array< Name > out_Bones );

// FaceFX.

/**
 * Play the specified FaceFX animation.
 * Returns TRUE if successful.
 * If animation couldn't be found, a log warning will be issued.
 */
native final function bool PlayFaceFXAnim(FaceFXAnimSet FaceFXAnimSetRef, string AnimName, string GroupName);

/** Stop any currently playing FaceFX animation. */
native final function StopFaceFXAnim();

/** Is playing a FaceFX animation. */
native final function bool IsPlayingFaceFXAnim();

/** Declare a new register in the FaceFX register system.  This is required
  * before using the register name in GetRegister() or SetRegister(). */
native final function DeclareFaceFXRegister( string RegName );

/** Retrieve the value of the specified FaceFX register. */
native final function float GetFaceFXRegister( string RegName );

/** Set the value and operation of the specified FaceFX register. */
native final function SetFaceFXRegister( string RegName, float RegVal, EFaceFXRegOp RegOp, optional float InterpDuration );
/** Set the value and operation of the specified FaceFX register. */
native final function SetFaceFXRegisterEx( string RegName, EFaceFXRegOp RegOp, float FirstValue, float FirstInterpDuration, float NextValue, float NextInterpDuration );

/** simple generic case animation player
 * requires that the one and only animation node in the AnimTree is an AnimNodeSequence
 * @param AnimName name of the animation to play
 * @param Duration (optional) override duration for the animation
 * @param bLoop (optional) whether the animation should loop
 * @param bRestartIfAlreadyPlaying whether or not to restart the animation if the specified anim is already playing
 */
function PlayAnim(name AnimName, optional float Duration, optional bool bLoop, optional bool bRestartIfAlreadyPlaying = true)
{
	local AnimNodeSequence AnimNode;
	local float DesiredRate;

	AnimNode = AnimNodeSequence(Animations);
	if (AnimNode == None && Animations.IsA('AnimTree'))
	{
		AnimNode = AnimNodeSequence(AnimTree(Animations).Children[0].Anim);
	}
	if (AnimNode == None)
	{
		`warn("Base animation node is not an AnimNodeSequence (Owner:" @ Owner $ ")");
	}
	else
	{
		if (AnimNode.AnimSeq != None && AnimNode.AnimSeq.SequenceName == AnimName)
		{
			DesiredRate = (Duration > 0.0) ? (AnimNode.AnimSeq.SequenceLength / Duration) : 1.0;
			if (bRestartIfAlreadyPlaying)
			{
				AnimNode.PlayAnim(bLoop, DesiredRate);
			}
			else
			{
				AnimNode.Rate = DesiredRate;
				AnimNode.bLooping = bLoop;
			}
		}
		else
		{
			AnimNode.SetAnim(AnimName);
			if (AnimNode.AnimSeq != None)
			{
				DesiredRate = (Duration > 0.0) ? (AnimNode.AnimSeq.SequenceLength / Duration) : 1.0;
				AnimNode.PlayAnim(bLoop, DesiredRate);
			}
		}
	}
}

/** simple generic case animation stopper
 * requires that the one and only animation node in the AnimTree is an AnimNodeSequence
 */
function StopAnim()
{
	local AnimNodeSequence AnimNode;

	AnimNode = AnimNodeSequence(Animations);
	if (AnimNode == None && Animations.IsA('AnimTree'))
	{
		AnimNode = AnimNodeSequence(AnimTree(Animations).Children[0].Anim);
	}
	if (AnimNode == None)
	{
		`warn("Base animation node is not an AnimNodeSequence (Owner:" @ Owner $ ")");
	}
	else
	{
		AnimNode.StopAnim();
	}
}

defaultproperties
{
	GlobalAnimRateScale=1.0

	// Various physics related items need to be ticked pre physics update
	TickGroup=TG_PreAsyncWork

	RootMotionMode=RMM_Ignore
	PreviousRMM=RMM_Ignore
	RootMotionRotationMode=RMRM_Ignore
	FaceFXBlendMode=FXBM_Additive

	WireframeColor=(R=221,G=221,B=28,A=255)
	bTransformFromAnimParent=1
	bUpdateSkelWhenNotRendered=TRUE
	bUpdateKinematicBonesFromAnimation=TRUE
	bSyncActorLocationToRootRigidBody=TRUE

	RootMotionAccelScale=(X=1.f,Y=1.f,Z=1.f)

	ClothRBChannel=RBCC_Cloth
	ClothBlendWeight=1.0

	ClothAttachmentTearFactor=-1.0

	//debug
//	bDisplayBones=true
}
