//=============================================================================
// Actor: The base class of all actors.
// Actor is the base class of all gameplay objects.
// A large number of properties, behaviors and interfaces are implemented in Actor, including:
//
// -	Display
// -	Animation
// -	Physics and world interaction
// -	Making sounds
// -	Networking properties
// -	Actor creation and destruction
// -	Actor iterator functions
// -	Message broadcasting
//
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================

class Actor extends Object
	abstract
	native
	nativereplication
	hidecategories(Navigation)
	DependsOn(AnimNode);

/** List of extra trace flags */
const TRACEFLAG_Bullet			= 1;
const TRACEFLAG_PhysicsVolumes	= 2;
const TRACEFLAG_SkipMovers		= 4;
const TRACEFLAG_Blocking		= 8;

/**
 * Actor components.
 * These are not exposed by default to level designers for several reasons.
 * The main one being that properties are not propagated to network clients
 * when is actor is dynamic (bStatic=FALSE and bNoDelete=FALSE).
 * So instead the actor should expose and interface the necessary component variables.
 */

/** The actor components which are attached directly to the actor's location/rotation. */
var private const array<ActorComponent>	Components;

/** All actor components which are directly or indirectly attached to the actor. */
var private transient const array<ActorComponent> AllComponents;

struct RenderCommandFence
{
	var private native const int NumPendingFences;
};

/** A fence to track when the primitive is detached from the scene in the rendering thread. */
var private native const RenderCommandFence DetachFence;

/** Allow each actor to run at a different time speed */
var float CustomTimeDilation;

// Priority Parameters
// Actor's current physics mode.
var(Movement) const enum EPhysics
{
	PHYS_None,
	PHYS_Walking,
	PHYS_Falling,
	PHYS_Swimming,
	PHYS_Flying,
	PHYS_Rotating,
	PHYS_Projectile,
	PHYS_Interpolating,
	PHYS_Spider,
	PHYS_Ladder,
	PHYS_RigidBody,
	PHYS_SoftBody, /** update bounding boxes and killzone test, otherwise like PHYS_None */
	PHYS_Unused
} Physics;

// Owner.
var const Actor	Owner;			// Owner actor.
var(Attachment) const Actor	Base;           // Actor we're standing on.

struct native TimerData
{
	var bool			bLoop;
	var Name			FuncName;
	var float			Rate, Count;
	var Object			TimerObj;
};
var const array<TimerData>			Timers;			// list of currently active timers

// Flags.
var			  const bool	bStatic;			// Does not move or change over time. Don't let L.D.s change this - screws up net play

/** If this is True, all PrimitiveComponents of the actor are hidden.  If this is false, only PrimitiveComponents with HiddenGame=True are hidden. */
var(Advanced) const bool	bHidden;

var			  const	bool	bNoDelete;			// Cannot be deleted during play.
var			  const	bool	bDeleteMe;			// About to be deleted.
var transient const bool	bTicked;			// Actor has been updated.
var const				bool    bOnlyOwnerSee;		// Only owner can see this actor.
var					bool	bStasis;			// In StandAlone games, turn off if not in a recently rendered zone turned off if  bStasis  and physics = PHYS_None or PHYS_Rotating.
var					bool	bWorldGeometry;		// Collision and Physics treats this actor as static world geometry
/** Ignore Unreal collisions between PHYS_RigidBody pawns (vehicles/ragdolls) and this actor (only relevant if bIgnoreEncroachers is false) */
var					bool	bIgnoreRigidBodyPawns;
var					bool	bOrientOnSlope;		// when landing, orient base on slope of floor
var			  const	bool	bIgnoreEncroachers;	// Ignore collisions between movers and this actor
/** whether encroachers can push this Actor (only relevant if bIgnoreEncroachers is false and not an encroacher ourselves)
 * if false, the encroacher gets EncroachingOn() called immediately instead of trying to safely move this actor first
 */
var bool bPushedByEncroachers;
/** Whether to route BeginPlay even if the actor is static. */
var			  const bool	bRouteBeginPlayEvenIfStatic;

/** Used to determine when we stop moving, so we can update PreviousLocalToWorld to stop motion blurring. */
var			  const	bool	bIsMoving;

/**
 *	If true (and is an encroacher) will do the encroachment check inside MoveActor even if there is no movement.
 *	This is useful for objects that may change bounding box but not actually move.
 */
var					bool	bAlwaysEncroachCheck;

// Networking flags
var			  const	bool	bNetTemporary;				// Tear-off simulation in network play.
var			  const	bool	bOnlyRelevantToOwner;			// this actor is only relevant to its owner. If this flag is changed during play, all non-owner channels would need to be explicitly closed.
var transient				bool	bNetDirty;				// set when any attribute is assigned a value in unrealscript, reset when the actor is replicated
var					bool	bAlwaysRelevant;			// Always relevant for network.
var					bool	bReplicateInstigator;		// Replicate instigator to client (used by bNetTemporary projectiles).
var					bool	bReplicateMovement;			// if true, replicate movement/location related properties
var					bool	bSkipActorPropertyReplication; // if true, don't replicate actor class variables for this actor
var					bool	bUpdateSimulatedPosition;	// if true, update velocity/location after initialization for simulated proxies
var					bool	bTearOff;					// if true, this actor is no longer replicated to new clients, and
														// is "torn off" (becomes a ROLE_Authority) on clients to which it was being replicated.
var					bool	bOnlyDirtyReplication;		// if true, only replicate actor if bNetDirty is true - useful if no C++ changed attributes (such as physics)
														// bOnlyDirtyReplication only used with bAlwaysRelevant actors


/** Demop recording variables */
var					bool	bDemoRecording;				// True if we are currently demo recording
var					bool	bDemoOwner;					// Demo recording driver owns this actor.

/** Should replicate initial rotation.  This property should never be changed during execution, as the client and server rely on the default value of this property always being the same. */
var const           bool    bNetInitialRotation;

var					bool	bReplicatePredictedVelocity;	// Replicate extrapolated velocity to improve client prediction (extrapolation based on server frame rate)
var					bool	bReplicateRigidBodyLocation;	// replicate Location property even when in PHYS_RigidBody
var					bool	bKillDuringLevelTransition;	// If set, actor and its components are marked as pending kill during seamless map transitions
/** whether we already exchanged Role/RemoteRole on the client, as removing then readding a streaming level
 * causes all initialization to be performed again even though the actor may not have actually been reloaded
 */
var const				bool	bExchangedRoles;

/** If true, texture streaming code iterates over all StaticMeshComponents found on this actor when building texture streaming information. */
var(Advanced)				bool	bConsiderAllStaticMeshComponentsForStreaming;

//debug
var(Debug)					bool	 bDebug;	// Used to toggle debug logging

// Net variables.
enum ENetRole
{
	ROLE_None,              // No role at all.
	ROLE_SimulatedProxy,	// Locally simulated proxy of this actor.
	ROLE_AutonomousProxy,	// Locally autonomous proxy of this actor.
	ROLE_Authority,			// Authoritative control over the actor.
};
var ENetRole RemoteRole, Role;
var const transient int		NetTag;
var float NetUpdateTime;	// time of last update
var float NetUpdateFrequency; // How many net updates per second.
var float NetPriority; // Higher priorities means update it more frequently.
var Pawn                  Instigator;    // Pawn responsible for damage caused by this actor.

var const transient WorldInfo	WorldInfo;
var	float						LifeSpan;		// How old the object lives before dying, 0=forever.
var const float					CreationTime;	// The time this actor was created, relative to WorldInfo.TimeSeconds

//-----------------------------------------------------------------------------
// Structures.

struct native transient TraceHitInfo
{
	var Material			Material; // Material we hit.
	var PhysicalMaterial    PhysMaterial; // The Physical Material that was hit
	var int					Item; // Extra info about thing we hit.
	var int					LevelIndex; // Level index, if we hit BSP.
	var name				BoneName; // Name of bone if we hit a skeletal mesh.
	var PrimitiveComponent	HitComponent; // Component of the actor that we hit.
};


/** Hit definition struct. Mainly used by Instant Hit Weapons. */
struct native transient ImpactInfo
{
	/** Actor Hit */
	var	Actor			HitActor;
	/** world location of hit impact */
	var	vector			HitLocation;
	/** Hit normal of impact */
	var	vector			HitNormal;
	/** Direction of ray when hitting actor */
	var	vector			RayDir;
	/** Trace Hit Info (material, bonename...) */
	var	TraceHitInfo	HitInfo;
};

/** Struct used for passing information from Matinee to an Actor for blending animations during a sequence. */
struct native transient AnimSlotInfo
{
	/** Name of slot that we want to play the animtion in. */
	var	name			SlotName;

	/** Strength of each Channel within this Slot. Channel indexs are determined by track order in Matinee. */
	var array<float>	ChannelWeights;
};

/** Used to indicate each slot name and how many channels they have. */
struct native transient AnimSlotDesc
{
	/** Name of the slot. */
	var name			SlotName;

	/** Number of channels that are available in this slot. */
	var int				NumChannels;
};

//-----------------------------------------------------------------------------
// Major actor properties.

var transient float		LastRenderTime;	// last time this actor was rendered.
var(Object)	name			Tag;			// Actor's tag name.
var			name			InitialState;
var(Object)	name			Group;

// Internal.
var transient const array<Actor>		Touching;		 // List of touching actors.
var transient const array<Actor> Children;		// array of actors owned by this actor
var const float				LatentFloat;   // Internal latent function use.
var const AnimNodeSequence	LatentSeqNode; // Internal latent function use.

// The actor's position and rotation.
var transient const PhysicsVolume	PhysicsVolume;	// physics volume this actor is currently in
var(Movement) const vector			Location;		// Actor's location; use Move to set.
var(Movement) const rotator			Rotation;		// Rotation.
var					vector			Velocity;		// Velocity.
var					vector			Acceleration;	// Acceleration.

// Attachment related variables
var(Attachment) SkeletalMeshComponent	BaseSkelComponent;
var(Attachment) name					BaseBoneName;

var const array<Actor>  Attached;			// array of actors attached to this actor.
var const vector		RelativeLocation;	// location relative to base/bone (valid if base exists)
var const rotator		RelativeRotation;	// rotation relative to base/bone (valid if base exists)

var(Attachment) const bool bHardAttach;		// Uses 'hard' attachment code. bBlockActor must also be false.
											// This actor cannot then move relative to base (setlocation etc.).
											// Dont set while currently based on something!

var(Attachment) bool bIgnoreBaseRotation;	/** If true, this actor ignores the effects of changes in its base's rotation on its location and rotation */

/** If TRUE, BaseSkelComponent is used as the shadow parent for this actor. */
var(Attachment) bool bShadowParented;

/** Determines whether or not adhesion code should attempt to adhere to this actor. **/
var bool bCanBeAdheredTo;

/** Determines whether or not friction code should attempt to friction to this actor. **/
var bool bCanBeFrictionedTo;


//-----------------------------------------------------------------------------
// Display properties.

var(Display) const interp	float	DrawScale;		// Scaling factor, 1.0=normal size.
var(Display) const interp	vector	DrawScale3D;	// Scaling vector, (1.0,1.0,1.0)=normal size.
var(Display) const			vector	PrePivot;		// Offset from box center for drawing.

var deprecated const bool UseCharacterLights;

// Advanced.
var			  bool		bHurtEntry;				// keep HurtRadius from being reentrant
var			  bool		bGameRelevant;			// Always relevant for game
var const     bool		bMovable;				// Actor can be moved.
var			  bool		bDestroyInPainVolume;	// destroy this actor if it enters a pain volume
var			  bool		bCanBeDamaged;			// can take damage
var			  bool		bShouldBaseAtStartup;	// if true, find base for this actor at level startup, if collides with world and PHYS_None or PHYS_Rotating
var			  bool		bPendingDelete;			// set when actor is about to be deleted (since endstate and other functions called
												// during deletion process before bDeleteMe is set).
var			  bool		bCanTeleport;			// This actor can be teleported.
var			  const	bool	bAlwaysTick;		// Update even when paused
/** indicates that this Actor can dynamically block AI paths */
var(Navigation) bool bBlocksNavigation;

//-----------------------------------------------------------------------------
// Collision.

// Collision primitive.
var(Collision) PrimitiveComponent	CollisionComponent;

var				native int	  		OverlapTag;

/** enum for LDs to select collision options - sets Actor flags and that of our CollisionComponent via PostEditChange() */
var(Collision) const transient enum ECollisionType
{
	COLLIDE_CustomDefault, // custom programmer set collison (PostEditChange() will restore collision to defaults when this is selected)
	COLLIDE_NoCollision, // doesn't collide
	COLLIDE_BlockAll, // blocks everything
	COLLIDE_BlockWeapons, // only blocks zero extent things (usually weapons)
	COLLIDE_TouchAll, // touches (doesn't block) everything
	COLLIDE_TouchWeapons, // touches (doesn't block) only zero extent things
	COLLIDE_BlockAllButWeapons, // only blocks non-zero extent things (Pawns, etc)
	COLLIDE_TouchAllButWeapons, // touches (doesn't block) only non-zero extent things
} CollisionType;
/** mirrored copy of CollisionComponent's BlockRigidBody for the Actor property window for LDs (so it's next to CollisionType)
 * purely for editing convenience and not used at all by the physics code
 */
var(Collision) const transient bool BlockRigidBody;

// Collision flags.
var 			bool		bCollideWhenPlacing;	// This actor collides with the world when placing.
var const	bool		bCollideActors;			// Collides with other actors.
var		bool		bCollideWorld;			// Collides with the world.
var(Collision)			bool		bCollideComplex;		// Ignore Simple Collision on Static Meshes, and collide per Poly.
var			bool		bBlockActors;			// Blocks other nonplayer actors.
var						bool		bProjTarget;			// Projectiles should potentially target this actor.
var						bool		bBlocksTeleport;

/**
 *	For encroachers, don't do the overlap check when they move. You will not get touch events for this actor moving, but it is much faster.
 *	This is an optimisation for large numbers of PHYS_RigidBody actors for example.
 */
var(Collision)			bool		bNoEncroachCheck;

//-----------------------------------------------------------------------------
// Physics.

// Options.
var			  bool        bBounce;           // Bounces when hits ground fast.
var			  const bool  bJustTeleported;   // Used by engine physics - not valid for scripts.

// Physics properties.
var(Movement) rotator	  RotationRate;		// Change in rotation per second.
var(Movement) rotator     DesiredRotation;	// Physics will smoothly rotate actor to this rotation.
var			  Actor		  PendingTouch;		// Actor touched during move which wants to add an effect after the movement completes

//@note: Pawns have properties that override these values
const MINFLOORZ = 0.7; // minimum z value for floor normal (if less, not a walkable floor)
					   // 0.7 ~= 45 degree angle for floor
const ACTORMAXSTEPHEIGHT = 35.0; // max height floor walking actor can step up to

const RBSTATE_LINVELSCALE = 10.0;
const RBSTATE_ANGVELSCALE = 1000.0;

/** describes the physical state of a rigid body
 * @warning: C++ mirroring is in UnPhysPublic.h
 */
struct RigidBodyState
{
	var vector	Position;
	var Quat	Quaternion;
	var vector	LinVel; // RBSTATE_LINVELSCALE times actual (precision reasons)
	var vector	AngVel; // RBSTATE_ANGVELSCALE times actual (precision reasons)
	var	int		bNewData;
};

const RB_None=0x00;			// Not set, empty
const RB_NeedsUpdate=0x01;	// If bNewData & RB_NeedsUpdate != 0 then an update is needed
const RB_Sleeping=0x02;		// if bNewData & RB_Sleeping != 0 then this RigidBody needs to sleep

/** Information about one contact between a pair of rigid bodies
 * @warning: C++ mirroring is in UnPhysPublic.h
 */
struct RigidBodyContactInfo
{
	var vector ContactPosition;
	var vector ContactNormal;
	var vector ContactVelocity[2];
	var PhysicalMaterial PhysMaterial[2];
};

/** Information about an overall collision, including contacts
 * @warning: C++ mirroring is in UnPhysPublic.h
 */
struct CollisionImpactData
{
	/** all the contact points in the collision*/
	var array<RigidBodyContactInfo> ContactInfos;

	/** the total force applied as the two objects push against each other*/
	var vector TotalNormalForceVector;
	/** the total counterforce applied of the two objects sliding against each other*/
	var vector TotalFrictionForceVector;
};

/** Structure filled in by async line check when it completes.
 * @warning: C++ mirroring is in UnPhysPublic.h
 */
struct AsyncLineCheckResult
{
	/** Indicates that there is an outstanding async line check that will be filling in this structure. */
	var int	bCheckStarted;

	/** Indicates that the async line check has finished, and bHit now contains the result. */
	var int	bCheckCompleted;

	/** Indicates result of line check. If bHit is TRUE, then the line hit some part of the level. */
	var int	bHit;
};

struct native ReplicatedHitImpulse
{
	var vector AppliedImpulse; //contains DamageRadius in X and damageimpulse in y for radial impulse
	var vector HitLocation;  // HurtOrigin for radial impulse
	var name BoneName;
	var byte ImpulseCount;
	var bool bRadialImpulse;
};

// endif

//-----------------------------------------------------------------------------
// Networking.

// Symmetric network flags, valid during replication only.
var const bool bNetInitial;       // Initial network update.
var const bool bNetOwner;         // Player owns this actor.

//Editing flags
var(Advanced) noimport edlayer     Layer;		// The layer this actor belongs to in UnrealEd
var(Advanced) const bool  bHiddenEd;     // Is hidden during editing.
var(Advanced) const bool  bHiddenEdGroup;// Is hidden by the group brower.
var const bool bHiddenEdCustom; // custom visibility flag for game-specific editor modes; not used by base editor functionality
var(Advanced) bool        bEdShouldSnap; // Snap to grid in editor.
var transient const bool  bTempEditor;   // Internal UnrealEd.
var(Collision) bool		  bPathColliding;// this actor should collide (if bWorldGeometry && bBlockActors is true) during path building (ignored if bStatic is true, as actor will always collide during path building)
var transient bool		  bPathTemp;	 // Internal/path building
var	bool				  bScriptInitialized; // set to prevent re-initializing of actors spawned during level startup
var(Advanced) bool        bLockLocation; // Prevent the actor from being moved in the editor.

var class<LocalMessage> MessageClass;

//-----------------------------------------------------------------------------
// Enums.

// Traveling from server to server.
enum ETravelType
{
	TRAVEL_Absolute,	// Absolute URL.
	TRAVEL_Partial,		// Partial (carry name, reset server).
	TRAVEL_Relative,	// Relative URL.
};


// double click move direction.
enum EDoubleClickDir
{
	DCLICK_None,
	DCLICK_Left,
	DCLICK_Right,
	DCLICK_Forward,
	DCLICK_Back,
	DCLICK_Active,
	DCLICK_Done
};

/** The ticking group this actor belongs to */
var const ETickingGroup TickGroup;

//-----------------------------------------------------------------------------
// Kismet

/** List of all events that this actor can support, for use by the editor */
var const array<class<SequenceEvent> > SupportedEvents;

/** List of all events currently associated with this actor */
var const array<SequenceEvent> GeneratedEvents;

/** List of all latent actions currently active on this actor */
var array<SeqAct_Latent> LatentActions;

/**
 * Struct used for cross level navigation point references.
 */
struct native NavReference
{
	var() NavigationPoint Nav;
	var() editconst const guid Guid;

	structcpptext
	{
		FNavReference()
		{
			Nav = NULL;
		}
		explicit FNavReference(class ANavigationPoint *InNav, FGuid &InGuid)
		{
			Nav = InNav;
			Guid = InGuid;
		}
		// overload various operators to make the reference struct as transparent as possible
		FORCEINLINE ANavigationPoint* operator*()
		{
			return Nav;
		}
		FORCEINLINE ANavigationPoint* operator->()
		{
			return Nav;
		}
		/** Slow version of deref that will use GUID if Nav is NULL */
		ANavigationPoint* operator~();
		FORCEINLINE FNavReference* operator=(ANavigationPoint *TargetNav)
		{
			Nav = TargetNav;
			return this;
		}
		FORCEINLINE UBOOL operator==(const FNavReference &Ref) const
		{
			return (Ref != NULL && (Ref.Nav == Nav));
		}
		FORCEINLINE UBOOL operator!=(const FNavReference &Ref) const
		{
			return (Ref == NULL || (Ref.Nav != Nav));
		}
		FORCEINLINE UBOOL operator==(ANavigationPoint *TestNav) const
		{
			return (Nav == TestNav);
		}
		FORCEINLINE UBOOL operator!=(ANavigationPoint *TestNav) const
		{
			return (Nav != TestNav);
		}
		FORCEINLINE operator AActor*()
		{
			return (AActor*)Nav;
		}
		FORCEINLINE operator ANavigationPoint*()
		{
			return Nav;
		}
		FORCEINLINE operator UBOOL()
		{
			return (Nav != NULL);
		}
		FORCEINLINE UBOOL operator!()
		{
			return (Nav == NULL);
		}
	}
};

//-----------------------------------------------------------------------------
// cpptext.

cpptext
{
	// Used to adjust box used for collision in overlap checks which are performed at a location other than the actor's current location.
	static FVector OverlapAdjust;

	// Constructors.
	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();

	// UObject interface.
	virtual INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
	virtual void ExtrapolateVelocity( FVector& ExtrapolatedVelocity);
	void ProcessEvent( UFunction* Function, void* Parms, void* Result=NULL );
	void ProcessState( FLOAT DeltaSeconds );
	UBOOL ProcessRemoteFunction( UFunction* Function, void* Parms, FFrame* Stack );
	void ProcessDemoRecFunction( UFunction* Function, void* Parms, FFrame* Stack );
	void InitExecution();
	virtual void PreEditChange(UProperty* PropertyThatWillChange);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void PreSave();
	virtual void PostLoad();
	void NetDirty(UProperty* property);

	// AActor interface.
	virtual APawn* GetPlayerPawn() const {return NULL;}
	virtual UBOOL IsPlayerPawn() const {return false;}
	virtual UBOOL IgnoreBlockingBy( const AActor *Other) const;
	UBOOL IsOwnedBy( const AActor *TestOwner ) const;
	UBOOL IsBlockedBy( const AActor* Other, const UPrimitiveComponent* Primitive ) const;
	UBOOL IsBasedOn( const AActor *Other ) const;

	/**
	 * Utility for finding the PrefabInstance that 'owns' this actor.
	 * If the actor is not part of a prefab instance, returns NULL.
	 * If the actor _is_ a PrefabInstance, return itself.
	 */
	class APrefabInstance* FindOwningPrefabInstance() const;

	/**
	 * @return		TRUE if the actor is in the named group, FALSE otherwise.
	 */
	UBOOL IsInGroup(const TCHAR* GroupName) const;

	/**
	 * Parses the actor's group string into a list of group names (strings).
	 * @param		OutGroups		[out] Receives the list of group names.
	 */
	void GetGroups(TArray<FString>& OutGroups) const;

	AActor* GetBase() const;

	/**
	 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
	 * The default implementation is simply to translate the actor's location.
	 */
	virtual void EditorApplyTranslation(const FVector& DeltaTranslation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);

	/**
	 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
	 * The default implementation is simply to modify the actor's rotation.
	 */
	virtual void EditorApplyRotation(const FRotator& DeltaRotation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);

	/**
	 * Called by ApplyDeltaToActor to perform an actor class-specific operation based on widget manipulation.
	 * The default implementation is simply to modify the actor's draw scale.
	 */
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);

	void EditorUpdateBase();
	void EditorUpdateAttachedActors(const TArray<AActor*>& IgnoreActors);

	// Editor specific
	UBOOL IsHiddenEd() const;
	virtual UBOOL IsSelected() const
	{
		return (UObject::IsSelected() && !bDeleteMe);
	}

	virtual FLOAT GetNetPriority(const FVector& ViewPos, const FVector& ViewDir, APlayerController* Viewer, UActorChannel* InChannel, FLOAT Time, UBOOL bLowBandwidth);
	/** ticks the actor
	 * @return TRUE if the actor was ticked, FALSE if it was aborted (e.g. because it's in stasis)
	 */
	virtual UBOOL Tick( FLOAT DeltaTime, enum ELevelTick TickType );
	/* AActor::InStasis()
	 * Called from AActor::Tick() if Actor->bStasis==true
	 * @return true if this actor ands its components can safely not be ticked.
	 */
	virtual UBOOL InStasis();
	/**
	 * bFinished is FALSE while the actor is being continually moved, and becomes TRUE on the last call.
	 * This can be used to defer computationally intensive calculations to the final PostEditMove call of
	 * eg a drag operation.
	 */
	virtual void PostEditMove(UBOOL bFinished);
	virtual void PostRename();
	virtual void Spawned();
	/** sets CollisionType to a default value based on the current collision settings of this Actor and its CollisionComponent */
	void SetDefaultCollisionType();
	/** sets collision flags based on the current CollisionType */
	void SetCollisionFromCollisionType();
	virtual void PreNetReceive();
	virtual void PostNetReceive();
	virtual void PostNetReceiveLocation();
	virtual void PostNetReceiveBase(AActor* NewBase);

	// Rendering info.

	virtual FMatrix LocalToWorld() const
	{
#if 0
		FTranslationMatrix	LToW		( -PrePivot					);
		FScaleMatrix		TempScale	( DrawScale3D * DrawScale	);
		FRotationMatrix		TempRot		( Rotation					);
		FTranslationMatrix	TempTrans	( Location					);
		LToW *= TempScale;
		LToW *= TempRot;
		LToW *= TempTrans;
		return LToW;
#else
		FMatrix Result;

		FLOAT	SR	= GMath.SinTab(Rotation.Roll),
				SP	= GMath.SinTab(Rotation.Pitch),
				SY	= GMath.SinTab(Rotation.Yaw),
				CR	= GMath.CosTab(Rotation.Roll),
				CP	= GMath.CosTab(Rotation.Pitch),
				CY	= GMath.CosTab(Rotation.Yaw);

		FLOAT	LX	= Location.X,
				LY	= Location.Y,
				LZ	= Location.Z,
				PX	= PrePivot.X,
				PY	= PrePivot.Y,
				PZ	= PrePivot.Z;

		FLOAT	DX	= DrawScale3D.X * DrawScale,
				DY	= DrawScale3D.Y * DrawScale,
				DZ	= DrawScale3D.Z * DrawScale;

		Result.M[0][0] = CP * CY * DX;
		Result.M[0][1] = CP * DX * SY;
		Result.M[0][2] = DX * SP;
		Result.M[0][3] = 0.f;

		Result.M[1][0] = DY * ( CY * SP * SR - CR * SY );
		Result.M[1][1] = DY * ( CR * CY + SP * SR * SY );
		Result.M[1][2] = -CP * DY * SR;
		Result.M[1][3] = 0.f;

		Result.M[2][0] = -DZ * ( CR * CY * SP + SR * SY );
		Result.M[2][1] =  DZ * ( CY * SR - CR * SP * SY );
		Result.M[2][2] = CP * CR * DZ;
		Result.M[2][3] = 0.f;

		Result.M[3][0] = LX - CP * CY * DX * PX + CR * CY * DZ * PZ * SP - CY * DY * PY * SP * SR + CR * DY * PY * SY + DZ * PZ * SR * SY;
		Result.M[3][1] = LY - (CR * CY * DY * PY + CY * DZ * PZ * SR + CP * DX * PX * SY - CR * DZ * PZ * SP * SY + DY * PY * SP * SR * SY);
		Result.M[3][2] = LZ - (CP * CR * DZ * PZ + DX * PX * SP - CP * DY * PY * SR);
		Result.M[3][3] = 1.f;

		return Result;
#endif
	}
	virtual FMatrix WorldToLocal() const
	{
		return	FTranslationMatrix(-Location) *
				FInverseRotationMatrix(Rotation) *
				FScaleMatrix(FVector( 1.f / DrawScale3D.X, 1.f / DrawScale3D.Y, 1.f / DrawScale3D.Z) / DrawScale) *
				FTranslationMatrix(PrePivot);
	}

	/** Returns the size of the extent to use when moving the object through the world */
	FVector GetCylinderExtent() const;

	AActor* GetTopOwner();
	virtual UBOOL IsPendingKill() const
	{
		return bDeleteMe || HasAnyFlags(RF_PendingKill);
	}
	/** Fast check to see if an actor is alive by not being virtual */
	FORCEINLINE UBOOL ActorIsPendingKill(void) const
	{
		return bDeleteMe || HasAnyFlags(RF_PendingKill);
	}
	virtual void PostScriptDestroyed() {} // C++ notification that the script Destroyed() function has been called.

	// AActor collision functions.
	virtual UBOOL ShouldTrace(UPrimitiveComponent* Primitive,AActor *SourceActor, DWORD TraceFlags);
	UBOOL IsOverlapping( AActor *Other, FCheckResult* Hit=NULL );

	FBox GetComponentsBoundingBox(UBOOL bNonColliding=0) const;

	/**
	 * This will check to see if the Actor is still in the world.  It will check things like
	 * the KillZ, SoftKillZ, outside world bounds, etc. and handle the situation.
	 **/
	void CheckStillInWorld();

	// AActor general functions.
	void UnTouchActors();
	void FindTouchingActors();
	void BeginTouch(AActor *Other, UPrimitiveComponent* OtherComp, const FVector &HitLocation, const FVector &HitNormal);
	void EndTouch(AActor *Other, UBOOL NoNotifySelf);
	UBOOL IsBrush()       const;
	UBOOL IsStaticBrush() const;
	UBOOL IsVolumeBrush() const;
	UBOOL IsEncroacher() const;

	UBOOL FindInterpMoveTrack(class UInterpTrackMove** MoveTrack, class UInterpTrackInstMove** MoveTrackInst, class USeqAct_Interp** OutSeq);

	/**
	 * Returns True if an actor cannot move or be destroyed during gameplay, and can thus cast and receive static shadowing.
	 */
	UBOOL HasStaticShadowing() const { return bStatic || (bNoDelete && !bMovable); }

	/**
	 * Sets the hard attach flag by first handling the case of already being
	 * based upon another actor
	 *
	 * @param bNewHardAttach the new hard attach setting
	 */
	virtual void SetHardAttach(UBOOL bNewHardAttach);

	virtual void NotifyBump(AActor *Other, UPrimitiveComponent* OtherComp, const FVector &HitNormal);
	/** notification when actor has bumped against the level */
	virtual void NotifyBumpLevel(const FVector &HitLocation, const FVector &HitNormal);

	void SetCollision( UBOOL bNewCollideActors, UBOOL bNewBlockActors, UBOOL bNewIgnoreEncroachers );
	virtual void SetBase(AActor *NewBase, FVector NewFloor = FVector(0,0,1), int bNotifyActor=1, USkeletalMeshComponent* SkelComp=NULL, FName AttachName=NAME_None );
	void UpdateTimers(FLOAT DeltaSeconds);
	virtual void TickAuthoritative( FLOAT DeltaSeconds );
	virtual void TickSimulated( FLOAT DeltaSeconds );
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual UBOOL PlayerControlled();
	virtual UBOOL IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation);
	virtual UBOOL DelayScriptReplication(FLOAT LastFullUpdateTime) { return false; }

	/** returns true if this actor should be considered relevancy owner for ReplicatedActor, which has bOnlyRelevantToOwner=true
	*/
	virtual UBOOL IsRelevancyOwnerFor(AActor* ReplicatedActor, AActor* ActorOwner);

	/** returns whether this Actor should be considered relevant because it is visible through
	 * the other side of any the passed in list of PortalTeleporters
	 */
	UBOOL IsRelevantThroughPortals(const TArray<class APortalTeleporter*>& Portals);

	// Level functions
	virtual void SetZone( UBOOL bTest, UBOOL bForceRefresh );
	virtual void SetVolumes();
	virtual void SetVolumes(const TArray<class AVolume*>& Volumes);
	virtual void PreBeginPlay();
	virtual void PostBeginPlay();

	/*
	 * Play a sound.  Creates an AudioComponent only if the sound is determined to be audible, and replicates the sound to clients based on optional flags
	 *
	 * @param	SoundLocation	the location to play the sound; if not specified, uses the actor's location.
	 */
	void PlaySound(class USoundCue* InSoundCue, UBOOL bNotReplicated = FALSE, UBOOL bNoRepToOwner = FALSE, UBOOL bStopWhenOwnerDestroyed = FALSE, FVector* SoundLocation = NULL, UBOOL bNoRepToRelevant = FALSE);

	// Physics functions.
	virtual void setPhysics(BYTE NewPhysics, AActor *NewFloor = NULL, FVector NewFloorV = FVector(0,0,1) );
	virtual void performPhysics(FLOAT DeltaSeconds);
	virtual void physProjectile(FLOAT deltaTime, INT Iterations);
	virtual void BoundProjectileVelocity();
	virtual void processHitWall(FVector HitNormal, AActor *HitActor, UPrimitiveComponent* HitComp);
	virtual void processLanded(FVector HitNormal, AActor *HitActor, FLOAT remainingTime, INT Iterations);
	virtual void physFalling(FLOAT deltaTime, INT Iterations);
	virtual void physWalking(FLOAT deltaTime, INT Iterations);
	virtual void physicsRotation(FLOAT deltaTime);
	int fixedTurn(int current, int desired, int deltaRate);
	inline void TwoWallAdjust(const FVector &DesiredDir, FVector &Delta, const FVector &HitNormal, const FVector &OldHitNormal, FLOAT HitTime)
	{
		if ((OldHitNormal | HitNormal) <= 0.f) //90 or less corner, so use cross product for dir
		{
			FVector NewDir = (HitNormal ^ OldHitNormal);
			NewDir = NewDir.SafeNormal();
			Delta = (Delta | NewDir) * (1.f - HitTime) * NewDir;
			if ((DesiredDir | Delta) < 0.f)
				Delta = -1.f * Delta;
		}
		else //adjust to new wall
		{
			Delta = (Delta - HitNormal * (Delta | HitNormal)) * (1.f - HitTime);
			if ((Delta | DesiredDir) <= 0.f)
				Delta = FVector(0.f,0.f,0.f);
		}
	}
	UBOOL moveSmooth(FVector Delta);
	virtual FRotator FindSlopeRotation(FVector FloorNormal, FRotator NewRotation);
	void UpdateRelativeRotation();
	virtual void GetNetBuoyancy(FLOAT &NetBuoyancy, FLOAT &NetFluidFriction);
	virtual void SmoothHitWall(FVector HitNormal, AActor *HitActor);
	virtual void stepUp(const FVector& GravDir, const FVector& DesiredDir, const FVector& Delta, FCheckResult &Hit);
	virtual UBOOL ShrinkCollision(AActor *HitActor, const FVector &StartLocation);
	virtual void GrowCollision() {};
	virtual UBOOL MoveWithInterpMoveTrack(UInterpTrackMove* MoveTrack, UInterpTrackInstMove* MoveInst, FLOAT CurTime, FLOAT DeltaTime);
	virtual void physInterpolating(FLOAT DeltaTime);
	virtual void PushedBy(AActor* Other) {};
	virtual void UpdateBasedRotation(FRotator &FinalRotation, const FRotator& ReducedRotation) {};
	virtual void ReverseBasedRotation() {};

	virtual void physRigidBody(FLOAT DeltaTime);
	virtual void physSoftBody(FLOAT DeltaTime);

	virtual void InitRBPhys();
	virtual void TermRBPhys(FRBPhysScene* Scene);

	void ApplyNewRBState(const FRigidBodyState& NewState, FLOAT* AngErrorAccumulator);
	UBOOL GetCurrentRBState(FRigidBodyState& OutState);

	/**
	 *	Event called when this Actor is involved in a rigid body collision.
	 *	bNotifyRigidBodyCollision must be true on the physics PrimitiveComponent within this Actor for this event to be called.
	 *	This base class implementation fires off the RigidBodyCollision Kismet event if attached.
	 */
	virtual void OnRigidBodyCollision(const FRigidBodyCollisionInfo& Info0, const FRigidBodyCollisionInfo& Info1, const FCollisionImpactData& RigidCollisionData);

	/** Update information used to detect overlaps between this actor and physics objects, used for 'pushing' things */
	virtual void UpdatePushBody() {};

#if WITH_NOVODEX
	virtual void ModifyNxActorDesc(NxActorDesc& ActorDesc,UPrimitiveComponent* PrimComp) {}
	virtual void PostInitRigidBody(NxActor* nActor, NxActorDesc& ActorDesc, UPrimitiveComponent* PrimComp) {}
	virtual void PreTermRigidBody(NxActor* nActor) {}
	void SyncActorToRBPhysics();
	void SyncActorToClothPhysics();
#endif // WITH_NOVODEX

	// AnimControl Matinee Track support

	/** Used to provide information on the slots that this Actor provides for animation to Matinee. */
	virtual void GetAnimControlSlotDesc(TArray<struct FAnimSlotDesc>& OutSlotDescs) {}

	/**
	 *	Called by Matinee when we open it to start controlling animation on this Actor.
	 *	Is also called again when the GroupAnimSets array changes in Matinee, so must support multiple calls.
	 */
	virtual void PreviewBeginAnimControl(TArray<class UAnimSet*>& InAnimSets) {}

	/** Called each frame by Matinee to update the desired sequence by name and position within it. */
	virtual void PreviewSetAnimPosition(FName SlotName, INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bLooping) {}

	/** Called each frame by Matinee to update the desired animation channel weights for this Actor. */
	virtual void PreviewSetAnimWeights(TArray<FAnimSlotInfo>& SlotInfos) {}

	/** Called by Matinee when we close it after we have been controlling animation on this Actor. */
	virtual void PreviewFinishAnimControl() {}

	/** Function used to control FaceFX animation in the editor (Matinee). */
	virtual void PreviewUpdateFaceFX(UBOOL bForceAnim, const FString& GroupName, const FString& SeqName, FLOAT InPosition) {}

	/** Used by Matinee playback to start a FaceFX animation playing. */
	virtual void PreviewActorPlayFaceFX(const FString& GroupName, const FString& SeqName) {}

	/** Used by Matinee to stop current FaceFX animation playing. */
	virtual void PreviewActorStopFaceFX() {}

	/** Used in Matinee to get the AudioComponent we should play facial animation audio on. */
	virtual UAudioComponent* PreviewGetFaceFXAudioComponent() { return NULL; }

	/** Get the UFaceFXAsset that is currently being used by this Actor when playing facial animations. */
	virtual class UFaceFXAsset* PreviewGetActorFaceFXAsset() { return NULL; }

	/** Called each frame by Matinee to update the weight of a particular MorphNodeWeight. */
	virtual void PreviewSetMorphWeight(FName MorphNodeName, FLOAT MorphWeight) {}

	/** Called each frame by Matinee to update the scaling on a SkelControl. */
	virtual void PreviewSetSkelControlScale(FName SkelControlName, FLOAT Scale) {}

	// AI functions.
	int TestCanSeeMe(class APlayerController *Viewer);
	virtual INT AddMyMarker(AActor *S) { return 0; };
	virtual void ClearMarker() {};
	virtual AActor* AssociatedLevelGeometry();
	virtual UBOOL HasAssociatedLevelGeometry(AActor *Other);
	UBOOL SuggestTossVelocity(FVector* TossVelocity, const FVector& Dest, const FVector& Start, FLOAT TossSpeed, FLOAT BaseTossZ, FLOAT DesiredZPct, const FVector& CollisionSize, FLOAT TerminalVelocity);
	virtual UBOOL ReachedBy(APawn* P, const FVector& TestPosition, const FVector& Dest);
	virtual UBOOL TouchReachSucceeded(APawn *P, const FVector &TestPosition);
	virtual UBOOL BlockedByVehicle();

	// Special editor behavior
	AActor* GetHitActor();
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
	virtual void CheckForDeprecated();

	// path creation
	virtual void PrePath() {};
	virtual void PostPath() {};

	/**
	 * Return whether this actor is a builder brush or not.
	 *
	 * @return TRUE if this actor is a builder brush, FALSE otherwise
	 */
	virtual UBOOL IsABuilderBrush() const { return FALSE; }

	/**
	 * Return whether this actor is the current builder brush or not
	 *
	 * @return TRUE if htis actor is the current builder brush, FALSE otherwise
	 */
	virtual UBOOL IsCurrentBuilderBrush() const { return FALSE; }

	virtual UBOOL IsABrush() const {return FALSE;}
	virtual UBOOL IsAVolume() const {return FALSE;}

	virtual APlayerController* GetAPlayerController() { return NULL; }
	virtual AController* GetAController() { return NULL; }
	virtual APawn* GetAPawn() { return NULL; }
	virtual const APawn* GetAPawn() const { return NULL; }
	virtual AVolume* GetAVolume() { return NULL; }
	virtual class AProjectile* GetAProjectile() { return NULL; }
	virtual const class AProjectile* GetAProjectile() const { return NULL; }
	virtual class APortalTeleporter* GetAPortalTeleporter() { return NULL; };

	virtual APlayerController* GetTopPlayerController()
	{
		AActor* TopActor = GetTopOwner();
		return (TopActor ? TopActor->GetAPlayerController() : NULL);
	}

	/**
	 * Verifies that neither this actor nor any of its components are RF_Unreachable and therefore pending
	 * deletion via the GC.
	 *
	 * @return TRUE if no unreachable actors are referenced, FALSE otherwise
	 */
	virtual UBOOL VerifyNoUnreachableReferences();

	virtual void ClearComponents();
	void ConditionalUpdateComponents(UBOOL bCollisionUpdate = FALSE);
protected:
	virtual void UpdateComponentsInternal(UBOOL bCollisionUpdate = FALSE);
public:

	/**
	 * Flags all components as dirty if in the editor, and then calls UpdateComponents().
	 *
	 * @param	bCollisionUpdate	[opt] As per UpdateComponents; defaults to FALSE.
	 * @param	bTransformOnly		[opt] TRUE to update only the component transforms, FALSE to update the entire component.
	 */
	virtual void ConditionalForceUpdateComponents(UBOOL bCollisionUpdate = FALSE,UBOOL bTransformOnly = TRUE);

	/**
	 * Flags all components as dirty so that they will be guaranteed an update from
	 * AActor::Tick(), and also be conditionally reattached by AActor::ConditionalUpdateComponents().
	 * @param	bTransformOnly	- True if only the transform has changed.
	 */
	void MarkComponentsAsDirty(UBOOL bTransformOnly = TRUE);

	/**
	 * Works through the component arrays marking entries as pending kill so references to them
	 * will be NULL'ed.
	 */
	virtual void MarkComponentsAsPendingKill();

	void InvalidateLightingCache();

	virtual UBOOL ActorLineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags);

	// Natives.
	DECLARE_FUNCTION(execPollSleep);
	DECLARE_FUNCTION(execPollFinishAnim);

	// Matinee
	void GetInterpFloatPropertyNames(TArray<FName> &outNames);
	void GetInterpVectorPropertyNames(TArray<FName> &outNames);
	void GetInterpColorPropertyNames(TArray<FName> &outNames);
	FLOAT* GetInterpFloatPropertyRef(FName inName);
	FVector* GetInterpVectorPropertyRef(FName inName);
	FColor* GetInterpColorPropertyRef(FName inName);

	/**
	 * Returns TRUE if this actor is contained by TestLevel.
	 * @todo seamless: update once Actor->Outer != Level
	 */
	UBOOL IsInLevel(const ULevel *TestLevel) const;
	/** Return the ULevel that this Actor is part of. */
	ULevel* GetLevel() const;

	/**
	 * Determine whether this actor is referenced by its level's GameSequence.
	 *
	 * @param	pReferencer		if specified, will be set to the SequenceObject that is referencing this actor.
	 *
	 * @return TRUE if this actor is referenced by kismet.
	 */
	UBOOL IsReferencedByKismet( class USequenceObject** pReferencer=NULL ) const;

	/**
	 * Called when a level is loaded/unloaded, to get a list of all the crosslevel
	 * paths that need to be fixed up.
	 */
	virtual void GetNavReferences(TArray<FNavReference*> &NavRefs, UBOOL bIsRemovingLevel) {}

	/*
	 * Route finding notifications (sent to target)
	 */
	virtual ANavigationPoint* SpecifyEndAnchor(APawn* RouteFinder) { return NULL; }
	virtual UBOOL AnchorNeedNotBeReachable();
	virtual void NotifyAnchorFindingResult(ANavigationPoint* EndAnchor, APawn* RouteFinder) {}
	virtual UBOOL ShouldHideActor(FVector CameraLocation) { return FALSE; }
}

//-----------------------------------------------------------------------------
// Network replication.

replication
{
	// Location
	if ( (!bSkipActorPropertyReplication || bNetInitial) && bReplicateMovement
					&& (((RemoteRole == ROLE_AutonomousProxy) && bNetInitial)
						|| ((RemoteRole == ROLE_SimulatedProxy) && (bNetInitial || bUpdateSimulatedPosition) && ((Base == None) || Base.bWorldGeometry))) )
		Location, Rotation;

	if ( (!bSkipActorPropertyReplication || bNetInitial) && bReplicateMovement
					&& RemoteRole==ROLE_SimulatedProxy )
		Base;

	if( (!bSkipActorPropertyReplication || bNetInitial) && bReplicateMovement && (bNetInitial || bUpdateSimulatedPosition)
					&& RemoteRole==ROLE_SimulatedProxy && (Base != None) && !Base.bWorldGeometry)
		RelativeRotation, RelativeLocation;

	// Physics
	if( (!bSkipActorPropertyReplication || bNetInitial) && bReplicateMovement
					&& ((RemoteRole == ROLE_SimulatedProxy) && (bNetInitial || bUpdateSimulatedPosition)) )
		Velocity, Physics;

	// Animation.
	if ( (!bSkipActorPropertyReplication || bNetInitial) && (Role==ROLE_Authority) )
		bHardAttach;

	// Properties changed using accessor functions
	if ( (!bSkipActorPropertyReplication || bNetInitial) && (Role==ROLE_Authority) && bNetDirty )
		bHidden;

	if ( (!bSkipActorPropertyReplication || bNetInitial) && (Role==ROLE_Authority) && bNetDirty
					&& (bCollideActors || bCollideWorld) )
		bProjTarget, bBlockActors;

	// Properties changed only when spawning or in script (relationships, rendering, lighting)
	if ( (!bSkipActorPropertyReplication || bNetInitial) && (Role==ROLE_Authority) )
		Role,RemoteRole,bNetOwner,bTearOff;

	if ( (!bSkipActorPropertyReplication || bNetInitial) && (Role==ROLE_Authority)
					&& bNetDirty && bReplicateInstigator )
		Instigator;

	// Infrequently changed mesh properties
	if ( (!bSkipActorPropertyReplication || bNetInitial) && (Role==ROLE_Authority)	&& bNetDirty )
		DrawScale, bCollideActors, bCollideWorld;

	// Properties changed using accessor functions
	if ( bNetOwner && (!bSkipActorPropertyReplication || bNetInitial) && (Role==ROLE_Authority) && bNetDirty )
		Owner;
}

//-----------------------------------------------------------------------------
// natives.

/**
 * Flags all components as dirty and then calls UpdateComponents().
 *
 * @param	bCollisionUpdate	[opt] As per UpdateComponents; defaults to FALSE.
 * @param	bTransformOnly		[opt] TRUE to update only the component transforms, FALSE to update the entire component.
 */
native final function ForceUpdateComponents(optional bool bCollisionUpdate = FALSE, optional bool bTransformOnly = TRUE);

// Execute a console command in the context of the current level and game engine.
native function string ConsoleCommand(string Command, optional bool bWriteToLog = true);

//=============================================================================
// General functions.

// Latent functions.
native(256) final latent function Sleep( float Seconds );
native(261) final latent function FinishAnim( AnimNodeSequence SeqNode );

// Collision.
native(262) final noexport function SetCollision( optional bool bNewColActors, optional bool bNewBlockActors, optional bool bNewIgnoreEncroachers );
native(283) final function SetCollisionSize( float NewRadius, float NewHeight );
native final function SetDrawScale(float NewScale);
native final function SetDrawScale3D(vector NewScale3D);

// Movement.
native(266) final function bool Move( vector Delta );
native(267) final function bool SetLocation( vector NewLocation );
native(299) final function bool SetRotation( rotator NewRotation );

// SetRelativeRotation() sets the rotation relative to the actor's base
native final function bool SetRelativeRotation( rotator NewRotation );
native final function bool SetRelativeLocation( vector NewLocation );
native final function noexport SetHardAttach(optional bool bNewHardAttach);

native(3969) noexport final function bool MoveSmooth( vector Delta );
native(3971) final function AutonomousPhysics(float DeltaSeconds);

/** returns terminal velocity (max speed while falling) for this actor.  Unless overridden, it returns the TerminalVelocity of the PhysicsVolume in which this actor is located.
*/
native function float GetTerminalVelocity();

// Relations.
native(298) noexport final function SetBase( actor NewBase, optional vector NewFloor, optional SkeletalMeshComponent SkelComp, optional name AttachName );
native(272) final function SetOwner( actor NewOwner );

/** Attempts to find a valid base for this actor */
native function FindBase();

/** iterates up the Base chain to see whether or not this Actor is based on the given Actor
 * @param TestActor the Actor to test for
 * @return whether or not this Actor is based on TestActor
 */
native noexport final function bool IsBasedOn(Actor TestActor);

/** Walks up the Base chain from this Actor and returns the Actor at the top (the eventual Base). this->Base is NULL, returns this. */
native function Actor GetBaseMost();

/** iterates up the Owner chain to see whether or not this Actor is owned by the given Actor
 * @param TestActor the Actor to test for
 * @return whether or not this Actor is owned by TestActor
 */
native noexport final function bool IsOwnedBy(Actor TestActor);

simulated event ReplicatedEvent(name VarName);	// Called when a variable with the property flag "RepNotify" is replicated

/** adds/removes a property from a list of properties that will always be replicated when this Actor is bNetInitial, even if the code thinks
 * the client has the same value the server already does
 * This is a workaround to the problem where an LD places an Actor in the level, changes a replicated variable away from the defaults,
 * then at runtime the variable is changed back to the default but it doesn't replicate because initial replication is based on class defaults
 * Only has an effect when called on bStatic or bNoDelete Actors
 * Only properties already in the owning class's replication block may be specified
 * @param PropToReplicate the property to add or remove to the list
 * @param bAdd true to add the property, false to remove the property
 */
native final function SetForcedInitialReplicatedProperty(Property PropToReplicate, bool bAdd);

//=========================================================================
// Rendering.

/** Flush persistent lines */
native static final function FlushPersistentDebugLines();

/** Draw a debug line */
native static final function DrawDebugLine(vector LineStart, vector LineEnd, byte R, byte G, byte B, optional bool bPersistentLines); // SLOW! Use for debugging only!

/** Draw a debug box */
native static final function DrawDebugBox(vector Center, vector Extent, byte R, byte G, byte B, optional bool bPersistentLines); // SLOW! Use for debugging only!

/** Draw Debug coordinate system */
native static final function DrawDebugCoordinateSystem(vector AxisLoc, Rotator AxisRot, float Scale, optional bool bPersistentLines); // SLOW! Use for debugging only!

/** Draw a debug sphere */
native static final function DrawDebugSphere(vector Center, float Radius, INT Segments, byte R, byte G, byte B, optional bool bPersistentLines); // SLOW! Use for debugging only!

/** Draw a debug cylinder */
native static final function DrawDebugCylinder(vector Start, vector End, float Radius, INT Segments, byte R, byte G, byte B, optional bool bPersistentLines); // SLOW! Use for debugging only!

/** Draw a debug cone */
native static final function DrawDebugCone(Vector Origin, Vector Direction, FLOAT Length, FLOAT AngleWidth, FLOAT AngleHeight, INT NumSides, Color DrawColor, optional bool bPersistentLines);

/** Draw some value over time onto the StatChart. Toggle on and off with */
native final function ChartData(string DataName, float DataValue);

/**
 * Changes the value of bHidden.
 *
 * @param bNewHidden	- The value to assign to bHidden.
 */
native final function SetHidden(bool bNewHidden);

/** changes the value of bOnlyOwnerSee
 * @param bNewOnlyOwnerSee the new value to assign to bOnlyOwnerSee
 */
native final function SetOnlyOwnerSee(bool bNewOnlyOwnerSee);

//=========================================================================
// Physics.

native(3970) noexport final function SetPhysics( EPhysics newPhysics );

// Timing
native final function Clock(out float time);
native final function UnClock(out float time);

// Components

/**
 * Adds a component to the actor's components array, attaching it to the actor.
 * @param NewComponent - The component to attach.
 */
native final function AttachComponent(ActorComponent NewComponent);

/**
 * Removes a component from the actor's components array, detaching it from the actor.
 * @param ExComponent - The component to detach.
 */
native final function DetachComponent(ActorComponent ExComponent);

/** Changes the ticking group for this actor */
native final function SetTickGroup(ETickingGroup NewTickGroup);

//=========================================================================
// Engine notification functions.

//
// Major notifications.
//
event Destroyed();
event GainedChild( Actor Other );
event LostChild( Actor Other );
event Tick( float DeltaTime );

//
// Physics & world interaction.
//
event Timer();
event HitWall( vector HitNormal, actor Wall, PrimitiveComponent WallComp);
event Falling();
event Landed( vector HitNormal, actor FloorActor );
event PhysicsVolumeChange( PhysicsVolume NewVolume );
event Touch( Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal );
event PostTouch( Actor Other ); // called for PendingTouch actor after physics completes
event UnTouch( Actor Other );
event Bump( Actor Other, PrimitiveComponent OtherComp, Vector HitNormal );
event BaseChange();
event Attach( Actor Other );
event Detach( Actor Other );
event Actor SpecialHandling(Pawn Other);
/**
 * Called when collision values change for this actor (via SetCollision/SetCollisionSize).
 */
event CollisionChanged();
/** called when this Actor is encroaching on Other and we couldn't find an appropriate place to push Other to
 * @return true to abort the move, false to allow it
 * @warning do not abort moves of PHYS_RigidBody actors as that will cause the Unreal location and physics engine location to mismatch
 */
event bool EncroachingOn(Actor Other);
event EncroachedBy( actor Other );
event RanInto( Actor Other );	// called for encroaching actors which successfully moved the other actor out of the way

/** Clamps out_Rot between the upper and lower limits offset from the base */
simulated final native function bool ClampRotation( out Rotator out_Rot, Rotator rBase, Rotator rUpperLimits, Rotator rLowerLimits );
/** Called by ClampRotation if the rotator was outside of the limits */
simulated event bool OverRotated( out Rotator out_Desired, out Rotator out_Actual );

/**
 * Called when being activated by the specified pawn.  Default
 * implementation searches for any SeqEvent_Used and activates
 * them.
 *
 * @return		true to indicate this actor was activated
 */
function bool UsedBy(Pawn User)
{
	return TriggerEventClass(class'SeqEvent_Used', User, -1);
}

/** called when the actor falls out of the world 'safely' (below KillZ and such) */
simulated event FellOutOfWorld(class<DamageType> dmgType)
{
	SetPhysics(PHYS_None);
	SetHidden(True);
	SetCollision(false,false);
	Destroy();
}

/** called when the Actor is outside the hard limit on world bounds
 * @note physics and collision are automatically turned off after calling this function
 */
simulated event OutsideWorldBounds()
{
	Destroy();
}

/**
 * Trace a line and see what it collides with first.
 * Takes this actor's collision properties into account.
 * Returns first hit actor, Level if hit level, or None if hit nothing.
 */
native(277) noexport final function Actor Trace
(
	out vector					HitLocation,
	out vector					HitNormal,
	vector						TraceEnd,
	optional vector				TraceStart,
	optional bool				bTraceActors,
	optional vector				Extent,
	optional out TraceHitInfo	HitInfo,
	optional int				ExtraTraceFlags
);

/**
 *	Run a line check against just this PrimitiveComponent. Return TRUE if we hit.
 *  NOTE: the actual Actor we call this on is irrelevant!
 */
native noexport final function bool TraceComponent
(
	out vector						HitLocation,
	out vector						HitNormal,
	PrimitiveComponent				InComponent,
	vector							TraceEnd,
	optional vector					TraceStart,
	optional vector					Extent,
	optional out TraceHitInfo		HitInfo
);

// returns true if did not hit world geometry
native(548) noexport final function bool FastTrace
(
	vector          TraceEnd,
	optional vector TraceStart,
	optional vector BoxExtent,
	optional bool	bTraceBullet
);

/*
 * Tries to position a box to avoid overlapping world geometry.
 * If no overlap, the box is placed at SpotLocation, otherwise the position is adjusted
 * @Parameter BoxExtent is the collision extent (X and Y=CollisionRadius, Z=CollisionHeight)
 * @Parameter SpotLocation is the position where the box should be placed.  Contains the adjusted location if it is adjusted.
 * @Return true if successful in finding a valid non-world geometry overlapping location
 */
native final function bool FindSpot(vector BoxExtent, out vector SpotLocation);

native final function bool ContainsPoint(vector Spot);
native noexport final function bool IsOverlapping(Actor A);
native final function GetComponentsBoundingBox(out box ActorBox) const;
native function GetBoundingCylinder(out float CollisionRadius, out float CollisionHeight) const;

/** Spawn an actor. Returns an actor of the specified class, not
 * of class Actor (this is hardcoded in the compiler). Returns None
 * if the actor could not be spawned (if that happens, there will be a log warning indicating why)
 * Defaults to spawning at the spawner's location.
 *
 * @note: ActorTemplate is sent for replicated actors and therefore its properties will also be applied
 * at initial creation on the client. However, because of this, ActorTemplate must be a static resource
 * (an actor archetype, default object, or a bStatic/bNoDelete actor in a level package)
 * or the spawned Actor cannot be replicated
 */
native(278) noexport final function coerce actor Spawn
(
	class<actor>      SpawnClass,
	optional actor	  SpawnOwner,
	optional name     SpawnTag,
	optional vector   SpawnLocation,
	optional rotator  SpawnRotation,
	optional Actor    ActorTemplate,
	optional bool	  bNoCollisionFail
);

//
// Destroy this actor. Returns true if destroyed, false if indestructible.
// Destruction is latent. It occurs at the end of the tick.
//
native(279) final noexport function bool Destroy();

// Networking - called on client when actor is torn off (bTearOff==true)
event TornOff();

//=============================================================================
// Timing.

/**
 * Sets a timer to call the given function at a set
 * interval.  Defaults to calling the 'Timer' event if
 * no function is specified.  If inRate is set to
 * 0.f it will effectively disable the previous timer.
 *
 * NOTE: Functions with parameters are not supported!
 *
 * @param inRate the amount of time to pass between firing
 * @param inbLoop whether to keep firing or only fire once
 * @param inTimerFunc the name of the function to call when the timer fires
 */
native(280) final function SetTimer(float inRate, optional bool inbLoop, optional Name inTimerFunc='Timer', optional Object inObj);

/**
 * Clears a previously set timer, identical to calling
 * SetTimer() with a <= 0.f rate.
 *
 * @param inTimerFunc the name of the timer to remove or the default one if not specified
 */
native final function ClearTimer(optional Name inTimerFunc='Timer', optional Object inObj);

/**
 * Returns true if the specified timer is active, defaults
 * to 'Timer' if no function is specified.
 *
 * @param inTimerFunc the name of the timer to remove or the default one if not specified
 */
native final function bool IsTimerActive(optional Name inTimerFunc='Timer', optional Object inObj);

/**
 * Gets the current count for the specified timer, defaults
 * to 'Timer' if no function is specified.  Returns -1.f
 * if the timer is not currently active.
 *
 * @param inTimerFunc the name of the timer to remove or the default one if not specified
 */
native final function float GetTimerCount(optional Name inTimerFunc='Timer', optional Object inObj);

/**
 * Gets the current rate for the specified timer.
 *
 * @note: GetTimerRate('SomeTimer') - GetTimerCount('SomeTimer') is the time remaining before 'SomeTimer' is called
 *
 * @param: TimerFuncName the name of the function to check for a timer for; 'Timer' is the default
 *
 * @return the rate for the given timer, or -1.f if that timer is not active
 */
native final function float GetTimerRate(optional name TimerFuncName = 'Timer', optional Object inObj);

//=============================================================================
// Sound functions.

/* Create an audio component.
 * may fail and return None if sound is disabled, there are too many sounds playing, or if the Location is out of range of all listeners
 */
native final function AudioComponent CreateAudioComponent(SoundCue InSoundCue, optional bool bPlay, optional bool bStopWhenOwnerDestroyed, optional bool bUseLocation, optional vector SourceLocation, optional bool bAttachToSelf = true);

/*
 * Play a sound.  Creates an AudioComponent only if the sound is determined to be audible, and replicates the sound to clients based on optional flags
 * @param InSoundCue - the sound to play
 * @param bNotReplicated (opt) - sound is considered only for players on this machine (supercedes other flags)
 * @param bNoRepToOwner (opt) - sound is not replicated to the Owner of this Actor (typically for Inventory sounds)
 * @param bStopWhenOwnerDestroyed (opt) - whether the sound should cut out early if the playing Actor is destroyed
 * @param SoundLocation (opt) - alternate location to play the sound instead of this Actor's Location
 * @param bNoRepToRelevant (opt) - sound is not replicated to clients for which this Actor is relevant (for important sounds that are locally simulated when possible)
 */
native noexport final function PlaySound(SoundCue InSoundCue, optional bool bNotReplicated, optional bool bNoRepToOwner, optional bool bStopWhenOwnerDestroyed, optional vector SoundLocation, optional bool bNoRepToRelevant);

//=============================================================================
// AI functions.

/* Inform other creatures that you've made a noise
 they might hear (they are sent a HearNoise message)
 Senders of MakeNoise should have an instigator if they are not pawns.
*/
native(512) final function MakeNoise( float Loudness, optional Name NoiseType );

/* PlayerCanSeeMe returns true if any player (server) or the local player (standalone
or client) has a line of sight to actor's location.
*/
native(532) final function bool PlayerCanSeeMe();

/* epic ===============================================
* ::SuggestTossVelocity()
*
* returns true if a successful toss from Start to Destination was found, and returns the suggested toss velocity in TossVelocity.
* TossSpeed in the magnitude of the toss
* BaseTossZ is an additional Z direction force added to the toss. (defaults to 0)
* DesiredZPct is the requested pct of the toss in the z direction (0=toss horizontally, 0.5 = toss at 45 degrees).  This is the starting point for finding a toss.  (Defaults to 0.05).
* CollisionSize is the size of bunding box of the tossed actor (defaults to (0,0,0)
*/
native noexport final function bool SuggestTossVelocity(out vector TossVelocity, vector Destination, vector Start, float TossSpeed, optional float BaseTossZ, optional float DesiredZPct, optional vector CollisionSize, optional float TerminalVelocity);

/** returns the position the AI should move toward to reach this actor
 * accounts for AI using path lanes, cutting corners, and other special adjustments
 */
native final virtual function vector GetDestination(Controller C);

//=============================================================================
// Regular engine functions.

// Teleportation.
function bool PreTeleport(Teleporter InTeleporter);
function PostTeleport(Teleporter OutTeleporter);

//========================================================================
// Disk access.

// Find files.
native(547) final function string GetURLMap();

//=============================================================================
// Iterator functions.

// Iterator functions for dealing with sets of actors.

/* AllActors() - avoid using AllActors() too often as it iterates through the whole actor list and is therefore slow
*/
native(304) final iterator function AllActors     ( class<actor> BaseClass, out actor Actor );

/* DynamicActors() only iterates through the non-static actors on the list (still relatively slow, but
 much better than AllActors).  This should be used in most cases and replaces AllActors in most of
 Epic's game code.
*/
native(313) final iterator function DynamicActors     ( class<actor> BaseClass, out actor Actor );

/* ChildActors() returns all actors owned by this actor.  Slow like AllActors()
*/
native(305) final iterator function ChildActors   ( class<actor> BaseClass, out actor Actor );

/* BasedActors() returns all actors based on the current actor (fast)
*/
native(306) final iterator function BasedActors   ( class<actor> BaseClass, out actor Actor );

/* TouchingActors() returns all actors touching the current actor (fast)
*/
native(307) final iterator function TouchingActors( class<actor> BaseClass, out actor Actor );

/* TraceActors() return all actors along a traced line.  Reasonably fast (like any trace)
*/
native(309) final iterator function TraceActors   ( class<actor> BaseClass, out actor Actor, out vector HitLoc, out vector HitNorm, vector End, optional vector Start, optional vector Extent, optional out TraceHitInfo HitInfo );

/* VisibleActors() returns all visible (not bHidden) actors within a radius
for which a trace from Loc (which defaults to caller's Location) to that actor's Location does not hit the world.
Slow like AllActors(). Use VisibleCollidingActors() instead if desired actor types are in the collision hash (bCollideActors is true)
*/
native(311) final iterator function VisibleActors ( class<actor> BaseClass, out actor Actor, optional float Radius, optional vector Loc );

/* VisibleCollidingActors() returns all colliding (bCollideActors==true) actors within a certain radius
for which a trace from Loc (which defaults to caller's Location) to that actor's Location does not hit the world.
Much faster than AllActors() since it uses the collision octree
bUseOverlapCheck uses a sphere vs. box check instead of checking to see if the center of an object lies within a sphere
*/
native(312) final iterator function VisibleCollidingActors ( class<actor> BaseClass, out actor Actor, float Radius, optional vector Loc, optional bool bIgnoreHidden );

/* CollidingActors() returns colliding (bCollideActors==true) actors within a certain radius.
Much faster than AllActors() for reasonably small radii since it uses the collision octree
bUseOverlapCheck uses a sphere vs. box check instead of checking to see if the center of an object lies within a sphere
*/
native(321) final iterator function CollidingActors ( class<actor> BaseClass, out actor Actor, float Radius, optional vector Loc, optional bool bUseOverlapCheck );

/**
 * Returns colliding (bCollideActors==true) which overlap a Sphere from location 'Loc' and 'Radius' radius.
 *
 * @param BaseClass		The Actor returns must be a subclass of this.
 * @param out_Actor		returned Actor at each iteration.
 * @param Radius		Radius of sphere for overlapping check.
 * @param Loc			Center of sphere for overlapping check. (Optional, caller's location is used otherwise).
 * @param bIgnoreHidden	if true, ignore bHidden actors.
 */
native final iterator function OverlappingActors( class<Actor> BaseClass, out Actor out_Actor, float Radius, optional vector Loc, optional bool bIgnoreHidden );

/**
*	Returns each component in the Components list
*	author: superville
**/
native final iterator function ComponentList( class<Object> BaseClass, out ActorComponent out_Component );

/**
 * Iterates over all components directly or indirectly attached to this actor.
 * @param BaseClass - Only components deriving from BaseClass will be iterated upon.
 * @param OutComponent - The iteration variable.
 */
native final iterator function AllOwnedComponents(class<Component> BaseClass,out ActorComponent OutComponent);

/**
 iterator LocalPlayerControllers()
 returns all locally rendered/controlled player controllers (typically 1 per client, unless split screen)
*/
native final iterator function LocalPlayerControllers(class<PlayerController> BaseClass, out PlayerController PC);

/**
 * Searches for actors of the specified class.
 */
final function bool FindActorsOfClass(class<Actor> ActorClass, out array<Actor> out_Actors)
{
	local Actor TestActor;
	out_Actors.Length = 0;
	foreach AllActors(ActorClass,TestActor)
	{
		out_Actors[out_Actors.Length] = TestActor;
	}
	return (out_Actors.Length > 0);
}

//=============================================================================
// Scripted Actor functions.

//
// Called immediately before gameplay begins.
//
event PreBeginPlay()
{
	// Handle autodestruction if desired.
	if (!bGameRelevant && !bStatic && WorldInfo.NetMode != NM_Client && !WorldInfo.Game.CheckRelevance(self))
	{
		if (bNoDelete)
		{
			ShutDown();
		}
		else
		{
			Destroy();
		}
	}
}

//
// Broadcast a localized message to all players.
// Most message deal with 0 to 2 related PRIs.
// The LocalMessage class defines how the PRI's and optional actor are used.
//
event BroadcastLocalizedMessage( class<LocalMessage> InMessageClass, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1, optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	WorldInfo.Game.BroadcastLocalized( self, InMessageClass, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject );
}

//
// Broadcast a localized message to all players on a team.
// Most message deal with 0 to 2 related PRIs.
// The LocalMessage class defines how the PRI's and optional actor are used.
//
event BroadcastLocalizedTeamMessage( int TeamIndex, class<LocalMessage> InMessageClass, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1, optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	WorldInfo.Game.BroadcastLocalizedTeam( TeamIndex, self, InMessageClass, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject );
}

// Called immediately after gameplay begins.
//
event PostBeginPlay();

// Called after PostBeginPlay.
//
simulated event SetInitialState()
{
	bScriptInitialized = true;
	if( InitialState!='' )
		GotoState( InitialState );
	else
		GotoState( 'Auto' );
}


/**
 * When a constraint is broken we will get this event from c++ land.
 **/
simulated event ConstraintBrokenNotify( Actor ConOwner, RB_ConstraintSetup ConSetup, RB_ConstraintInstance ConInstance  )
{

}

simulated event NotifySkelControlBeyondLimit( SkelControlLookAt LookAt );

/* epic ===============================================
* ::StopsProjectile()
*
* returns true if Projectiles should call ProcessTouch() when they touch this actor
*/
simulated function bool StopsProjectile(Projectile P)
{
	return bProjTarget || bBlockActors;
}

/* HurtRadius()
 Hurt locally authoritative actors within the radius.
*/
simulated function bool HurtRadius
(
	float				BaseDamage,
	float				DamageRadius,
	class<DamageType>	DamageType,
	float				Momentum,
	vector				HurtOrigin,
	optional Actor		IgnoredActor,
	optional Controller InstigatedByController = Instigator != None ? Instigator.Controller : None,
	optional bool       bDoFullDamage
)
{
	local Actor	Victim;
	local bool bCausedDamage;

	// Prevent HurtRadius() from being reentrant.
	if ( bHurtEntry )
		return false;

	bHurtEntry = true;
	bCausedDamage = false;
	foreach VisibleCollidingActors( class'Actor', Victim, DamageRadius, HurtOrigin )
	{
		if ( !Victim.bWorldGeometry && (Victim != self) && (Victim != IgnoredActor) && (Victim.bProjTarget || (NavigationPoint(Victim) == None)) )
		{
			Victim.TakeRadiusDamage(InstigatedByController, BaseDamage, DamageRadius, DamageType, Momentum, HurtOrigin, bDoFullDamage, self);
			bCausedDamage = bCausedDamage || Victim.bProjTarget;
		}
	}
	bHurtEntry = false;
	return bCausedDamage;
}

//
// Damage and kills.
//
function KilledBy( pawn EventInstigator );

/** apply some amount of damage to this actor
 * @param Damage the base damage to apply
 * @param EventInstigator the Controller responsible for the damage
 * @param HitLocation world location where the hit occurred
 * @param Momentum force caused by this hit
 * @param DamageType class describing the damage that was done
 * @param HitInfo additional info about where the hit occurred
 * @param DamageCauser the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
 */
event TakeDamage(int DamageAmount, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local int idx;
	local SeqEvent_TakeDamage dmgEvent;
	// search for any damage events
	for (idx = 0; idx < GeneratedEvents.Length; idx++)
	{
		dmgEvent = SeqEvent_TakeDamage(GeneratedEvents[idx]);
		if (dmgEvent != None)
		{
			// notify the event of the damage received
			dmgEvent.HandleDamage(self, EventInstigator, DamageType, DamageAmount);
		}
	}
}
/**
 * the reverse of TakeDamage(); heals the specified amount
 *
 * @param	Amount		The amount of damage to heal
 * @param	Healer		Who is doing the healing
 * @param	DamageType	What type of healing is it
 */
function bool HealDamage(int Amount, Controller Healer, class<DamageType> DamageType);

/**
 * Take Radius Damage
 * by default scales damage based on distance from HurtOrigin to Actor's location.
 * This can be overridden by the actor receiving the damage for special conditions (see KAsset.uc).
 * This then calls TakeDamage() to go through the same damage pipeline.
 *
 * @param	InstigatedBy, instigator of the damage
 * @param	Base Damage
 * @param	Damage Radius (from Origin)
 * @param	DamageType class
 * @param	Momentum (float)
 * @param	HurtOrigin, origin of the damage radius.
 * @param	bFullDamage, if true, damage not scaled based on distance HurtOrigin
 * @param DamageCauser the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
 */
simulated function TakeRadiusDamage
(
	Controller			InstigatedBy,
	float				BaseDamage,
	float				DamageRadius,
	class<DamageType>	DamageType,
	float				Momentum,
	vector				HurtOrigin,
	bool				bFullDamage,
	Actor DamageCauser
)
{
	local float		ColRadius, ColHeight;
	local float		damageScale, dist;
	local vector	dir;

	GetBoundingCylinder(ColRadius, ColHeight);

	dir	= Location - HurtOrigin;
	dist = VSize(dir);
	dir	= Normal(Dir);

	if ( bFullDamage )
	{
		damageScale = 1;
	}
	else
	{
		dist = FClamp(dist - ColRadius, 0.f, DamageRadius);
		damageScale = 1 - dist/DamageRadius;
	}

	if (damageScale > 0.f)
	{
		TakeDamage
		(
			damageScale * BaseDamage,
			InstigatedBy,
			Location - 0.5 * (ColHeight + ColRadius) * dir,
			(damageScale * Momentum * dir),
			DamageType,,
			DamageCauser
		);
	}
}

/**
 * Make sure we pass along a valid HitInfo struct for damage.
 * The main reason behind this is that SkeletalMeshes do require a BoneName to receive and process an impulse...
 * So if we don't have access to it (through touch() or for any non trace damage results), we need to perform an extra trace call().
 *
 * @param	HitInfo, initial structure to check
 * @param	FallBackComponent, PrimitiveComponent to use if HitInfo.HitComponent is none
 * @param	Dir, Direction to use if a Trace needs to be performed to find BoneName on skeletalmesh. Trace from HitLocation.
 * @param	out_HitLocation, HitLocation to use for potential Trace, will get updated by Trace.
 */
final simulated function CheckHitInfo
(
	out	TraceHitInfo		HitInfo,
		PrimitiveComponent	FallBackComponent,
		Vector				Dir,
	out Vector				out_HitLocation
)
{
	local vector			out_NewHitLocation, out_HitNormal, TraceEnd, TraceStart;
	local TraceHitInfo		newHitInfo;

	//`log("Actor::CheckHitInfo - HitInfo.HitComponent:" @ HitInfo.HitComponent @ "FallBackComponent:" @ FallBackComponent );

	// we're good, return!
	if( SkeletalMeshComponent(HitInfo.HitComponent) != None && HitInfo.BoneName != '' )
	{
		return;
	}

	// Use FallBack PrimitiveComponent if possible
	if( HitInfo.HitComponent == None ||
		(SkeletalMeshComponent(HitInfo.HitComponent) == None && SkeletalMeshComponent(FallBackComponent) != None) )
	{
		HitInfo.HitComponent = FallBackComponent;
	}

	// if we do not have a valid BoneName, perform a trace against component to try to find one.
	if( SkeletalMeshComponent(HitInfo.HitComponent) != None && HitInfo.BoneName == '' )
	{
		if( IsZero(Dir) )
		{
			//`warn("passed zero dir for trace");
			Dir = Vector(Rotation);
		}

		if( IsZero(out_HitLocation) )
		{
			//`warn("IsZero(out_HitLocation)");
			//assert(false);
			out_HitLocation = Location;
		}

		TraceStart	= out_HitLocation - 128 * Normal(Dir);
		TraceEnd	= out_HitLocation + 128 * Normal(Dir);

		if( TraceComponent( out_NewHitLocation, out_HitNormal, HitInfo.HitComponent, TraceEnd, TraceStart, vect(0,0,0), newHitInfo ) )
		{	// Update HitLocation
			HitInfo.BoneName	= newHitInfo.BoneName;
			HitInfo.PhysMaterial = newHitInfo.PhysMaterial;
			out_HitLocation		= out_NewHitLocation;
		}
		/*
		else
		{
			// FIXME LAURENT -- The test fails when a just spawned projectile triggers a touch() event, the trace performed will be slightly off and fail.
			`log("Actor::CheckHitInfo non successful TraceComponent!!");
			`log("HitInfo.HitComponent:" @ HitInfo.HitComponent );
			`log("TraceEnd:" @ TraceEnd );
			`log("TraceStart:" @ TraceStart );
			`log("out_HitLocation:" @ out_HitLocation );

			ScriptTrace();
			//DrawDebugLine(TraceEnd, TraceStart, 255, 0, 0, TRUE);
			//DebugFreezeGame();
		}
		*/
	}
}

/**
 * Get gravity currently affecting this actor
 */
native function float GetGravityZ();

/**
 * Debug Freeze Game
 * dumps the current script function stack and pauses the game with PlayersOnly (still allowing the player to move around).
 */
function DebugFreezeGame()
{
	local PlayerController	PC;

	ScriptTrace();
	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		PC.ConsoleCommand("PlayersOnly");
		return;
	}
}

function bool CheckForErrors();

/* BecomeViewTarget
	Called by Camera when this actor becomes its ViewTarget */
event BecomeViewTarget( PlayerController PC );

/* EndViewTarget
	Called by Camera when this actor no longer its ViewTarget */
event EndViewTarget( PlayerController PC );

/**
 *	Calculate camera view point, when viewing this actor.
 *
 * @param	fDeltaTime	delta time seconds since last update
 * @param	out_CamLoc	Camera Location
 * @param	out_CamRot	Camera Rotation
 * @param	out_FOV		Field of View
 *
 * @return	true if Actor should provide the camera point of view.
 */
simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	local vector HitNormal;
	local float Radius, Height;

	GetBoundingCylinder(Radius, Height);

	if (Trace(out_CamLoc, HitNormal, Location - vector(out_CamRot) * Radius * 20, Location, false) == None)
	{
		out_CamLoc = Location - vector(out_CamRot) * Radius * 20;
	}
	else
	{
		out_CamLoc = Location + Height * vector(Rotation);
	}

	return false;
}

// Returns the string representation of the name of an object without the package
// prefixes.
//
simulated function String GetItemName( string FullName )
{
	local int pos;

	pos = InStr(FullName, ".");
	While ( pos != -1 )
	{
		FullName = Right(FullName, Len(FullName) - pos - 1);
		pos = InStr(FullName, ".");
	}

	return FullName;
}

// Returns the human readable string representation of an object.
//
simulated function String GetHumanReadableName()
{
	return GetItemName(string(class));
}

static function ReplaceText(out string Text, string Replace, string With)
{
	local int i;
	local string Input;

	Input = Text;
	Text = "";
	i = InStr(Input, Replace);
	while(i != -1)
	{
		Text = Text $ Left(Input, i) $ With;
		Input = Mid(Input, i + Len(Replace));
		i = InStr(Input, Replace);
	}
	Text = Text $ Input;
}

// Get localized message string associated with this actor
static function string GetLocalString(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2
	)
{
	return "";
}

function MatchStarting(); // called when gameplay actually starts
function SetGRI(GameReplicationInfo GRI);

function String GetDebugName()
{
	return GetItemName(string(self));
}

/**
 * list important Actor variables on canvas.  HUD will call DisplayDebug() on the current ViewTarget when
 * the ShowDebug exec is used
 *
 * @param	HUD		- HUD with canvas to draw on
 * @input	out_YL		- Height of the current font
 * @input	out_YPos	- Y position on Canvas. out_YPos += out_YL, gives position to draw text for next debug line.
 */
simulated function DisplayDebug(HUD HUD, out float out_YL, out float out_YPos)
{
	local string	T;
	local Actor		A;
	local float MyRadius, MyHeight;
	local Canvas Canvas;

	Canvas = HUD.Canvas;

	Canvas.SetPos(4, out_YPos);
	Canvas.SetDrawColor(255,0,0);

	T = GetDebugName();
	if( bDeleteMe )
	{
		T = T$" DELETED (bDeleteMe == true)";
	}

	if( T != "" )
	{
		Canvas.DrawText(T, FALSE);
		out_YPos += out_YL;
		Canvas.SetPos(4, out_YPos);
	}

	Canvas.SetDrawColor(255,255,255);

	if( HUD.ShouldDisplayDebug('net') )
	{
		if( WorldInfo.NetMode != NM_Standalone )
		{
			// networking attributes
			T = "ROLE:" @ Role @ "RemoteRole:" @ RemoteRole @ "NetMode:" @ WorldInfo.NetMode;

			if( bTearOff )
			{
				T = T @ "Tear Off";
			}
			Canvas.DrawText(T, FALSE);
			out_YPos += out_YL;
			Canvas.SetPos(4, out_YPos);
		}
	}

	Canvas.DrawText("Location:" @ Location @ "Rotation:" @ Rotation, FALSE);
	out_YPos += out_YL;
	Canvas.SetPos(4,out_YPos);

	if( HUD.ShouldDisplayDebug('physics') )
	{
		T = "Physics" @ GetPhysicsName() @ "in physicsvolume" @ GetItemName(string(PhysicsVolume)) @ "on base" @ GetItemName(string(Base)) @ "gravity" @ GetGravityZ();
		if( bBounce )
		{
			T = T$" - will bounce";
		}
		Canvas.DrawText(T, FALSE);
		out_YPos += out_YL;
		Canvas.SetPos(4,out_YPos);

		Canvas.DrawText("bHardAttach:" @ bHardAttach @ "RelativeLoc:" @ RelativeLocation @ "RelativeRot:" @ RelativeRotation @ "SkelComp:" @ BaseSkelComponent @ "Bone:" @ string(BaseBoneName), FALSE);
		out_YPos += out_YL;
		Canvas.SetPos(4,out_YPos);

		Canvas.DrawText("Velocity:" @ Velocity @ "Speed:" @ VSize(Velocity) @ "Speed2D:" @ VSize2D(Velocity), FALSE);
		out_YPos += out_YL;
		Canvas.SetPos(4,out_YPos);

		Canvas.DrawText("Acceleration:" @ Acceleration, FALSE);
		out_YPos += out_YL;
		Canvas.SetPos(4,out_YPos);
	}

	if( HUD.ShouldDisplayDebug('collision') )
	{
		Canvas.DrawColor.B = 0;
		GetBoundingCylinder(MyRadius, MyHeight);
		Canvas.DrawText("Collision Radius:" @ MyRadius @ "Height:" @ MyHeight);
		out_YPos += out_YL;
		Canvas.SetPos(4,out_YPos);

		Canvas.DrawText("Collides with Actors:" @ bCollideActors @ " world:" @ bCollideWorld @ "proj. target:" @ bProjTarget);
		out_YPos += out_YL;
		Canvas.SetPos(4,out_YPos);
		Canvas.DrawText("Blocks Actors:" @ bBlockActors);
		out_YPos += out_YL;
		Canvas.SetPos(4,out_YPos);

		T = "Touching ";
		ForEach TouchingActors(class'Actor', A)
			T = T$GetItemName(string(A))$" ";
		if ( T == "Touching ")
			T = "Touching nothing";
		Canvas.DrawText(T, FALSE);
		out_YPos += out_YL;
		Canvas.SetPos(4,out_YPos);
	}

	Canvas.DrawColor.B = 255;
	Canvas.DrawText(" STATE:" @ GetStateName(), FALSE);
	out_YPos += out_YL;
	Canvas.SetPos(4,out_YPos);

	Canvas.DrawText( " Instigator:" @ GetItemName(string(Instigator)) @ "Owner:" @ GetItemName(string(Owner)) );
	out_YPos += out_YL;
	Canvas.SetPos(4,out_YPos);
}

simulated function String GetPhysicsName()
{
	Switch( PHYSICS )
	{
		case PHYS_None:				return "None"; break;
		case PHYS_Walking:			return "Walking"; break;
		case PHYS_Falling:			return "Falling"; break;
		case PHYS_Swimming:			return "Swimming"; break;
		case PHYS_Flying:			return "Flying"; break;
		case PHYS_Rotating:			return "Rotating"; break;
		case PHYS_Projectile:		return "Projectile"; break;
		case PHYS_Interpolating:	return "Interpolating"; break;
		case PHYS_Spider:			return "Spider"; break;
		case PHYS_Ladder:			return "Ladder"; break;
		case PHYS_RigidBody:		return "RigidBody"; break;
		case PHYS_Unused:			return "Unused"; break;
	}
	return "Unknown";
}

/** called when a sound is going to be played on this Actor via PlayerController::ClientHearSound()
 * gives it a chance to modify the component that will be used (add parameter values, etc)
 */
simulated event ModifyHearSoundComponent(AudioComponent AC);

/**
 *	Function for allowing you to tell FaceFX which AudioComponent it should use for playing audio
 *	for corresponding facial animation.
 */
simulated event AudioComponent GetFaceFXAudioComponent()
{
	return None;
}

/* Reset()
reset actor to initial state - used when restarting level without reloading.
*/
function Reset();

function bool IsInVolume(Volume aVolume)
{
	local Volume V;

	ForEach TouchingActors(class'Volume',V)
		if ( V == aVolume )
			return true;
	return false;
}

function bool IsInPain()
{
	local PhysicsVolume V;

	ForEach TouchingActors(class'PhysicsVolume',V)
		if ( V.bPainCausing && (V.DamagePerSec > 0) )
			return true;
	return false;
}

function PlayTeleportEffect(bool bOut, bool bSound);

simulated function bool CanSplash()
{
	return false;
}

simulated function bool CheckMaxEffectDistance(PlayerController P, vector SpawnLocation, optional float CullDistance)
{
	local float Dist;

	if ( P.ViewTarget == None )
		return true;

	if ( (Vector(P.Rotation) Dot (SpawnLocation - P.ViewTarget.Location)) < 0.0 )
	{
		return (VSize(P.ViewTarget.Location - SpawnLocation) < 1600);
	}

	Dist = VSize(SpawnLocation - P.ViewTarget.Location);

	if (CullDistance > 0 && CullDistance < Dist * P.LODDistanceFactor)
	{
		return false;
	}

	return !P.BeyondFogDistance(P.ViewTarget.Location,SpawnLocation);
}

simulated function bool EffectIsRelevant(vector SpawnLocation, bool bForceDedicated, optional float CullDistance )
{
	local PlayerController	P;
	local bool				bResult;

	if ( WorldInfo.NetMode == NM_DedicatedServer )
	{
		return bForceDedicated;
	}

	if ( WorldInfo.NetMode == NM_ListenServer )
	{
		if ( bForceDedicated )
			return true;
		if ( (Instigator != None) && Instigator.IsHumanControlled() && Instigator.IsLocallyControlled() )
			return true;
	}
	else if ( (Instigator != None) && Instigator.IsHumanControlled() )
	{
		return true;
	}

	if ( SpawnLocation == Location )
	{
		bResult = ( WorldInfo.TimeSeconds - LastRenderTime < 0.5 );
	}
	else if ( (Instigator != None) && (WorldInfo.TimeSeconds - Instigator.LastRenderTime < 1.0) )
	{
		bResult = true;
	}

	if ( bResult )
	{
		bResult = false;
		ForEach LocalPlayerControllers(class'PlayerController', P)
		{
			if ( P.ViewTarget != None )
			{
				if ( (P.Pawn == Instigator) && (Instigator != None) )
				{
					return true;
				}
				else
				{
					bResult = CheckMaxEffectDistance(P, SpawnLocation, CullDistance);
					break;
				}
			}
		}
	}
	return bResult;
}

/** Retrieves difference between world time and given time */
simulated final function float TimeSince( float Time )
{
	return WorldInfo.TimeSeconds - Time;
}

//-----------------------------------------------------------------------------
// Scripting support


/** Convenience function for triggering events in the GeneratedEvents list
 * If you need more options (activating multiple outputs, etc), call ActivateEventClass() directly
 */
function bool TriggerEventClass(class<SequenceEvent> InEventClass, Actor InInstigator, optional int ActivateIndex = -1, optional bool bTest)
{
	local array<int> ActivateIndices;

	if (ActivateIndex >= 0)
	{
		ActivateIndices[0] = ActivateIndex;
	}
	return ActivateEventClass(InEventClass, InInstigator, GeneratedEvents, ActivateIndices, bTest);
}

/**
 * Iterates through the given list of events and looks for all
 * matching events, activating them as found.
 *
 * @return		true if an event was found and activated
 */
final function bool ActivateEventClass( class<SequenceEvent> InClass, Actor InInstigator, const out array<SequenceEvent> EventList,
					optional const out array<int> ActivateIndices, optional bool bTest )
{
	local int Idx, ActivateCnt;

	for (Idx = 0; Idx < EventList.Length; Idx++)
	{
		if ( ClassIsChildOf(EventList[idx].Class, InClass) &&
			EventList[idx].CheckActivate(self, InInstigator, bTest, ActivateIndices) )
		{
			ActivateCnt++;
		}
	}
	return (ActivateCnt > 0);
}

/**
 * Builds a list of all events of the specified class.
 *
 * @param	eventClass - type of event to search for
 * @param	out_EventList - list of found events
 *
 * @return	true if any events were found
 */
final function bool FindEventsOfClass(class<SequenceEvent> EventClass, optional out array<SequenceEvent> out_EventList)
{
	local int idx;
	local bool bFoundEvent;
	for (idx = 0; idx < GeneratedEvents.Length; idx++)
	{
		if (ClassIsChildOf(GeneratedEvents[idx].Class, EventClass) &&
			GeneratedEvents[idx].bEnabled &&
			(GeneratedEvents[idx].MaxTriggerCount == 0 ||
			 GeneratedEvents[idx].MaxTriggerCount > GeneratedEvents[idx].TriggerCount))
		{
			out_EventList[out_EventList.Length] = GeneratedEvents[idx];
			bFoundEvent = true;
		}
	}
	return bFoundEvent;
}

/**
 * Clears all latent actions of the specified class.
 *
 * @param	actionClass - type of latent action to clear
 * @param	bAborted - was this latent action aborted?
 * @param	exceptionAction - action to skip
 */
final function ClearLatentAction(class<SeqAct_Latent> actionClass,optional bool bAborted,optional SeqAct_Latent exceptionAction)
{
	local int idx;
	for (idx = 0; idx < LatentActions.Length; idx++)
	{
		if (LatentActions[idx] == None)
		{
			// remove dead entry
			LatentActions.Remove(idx--,1);
		}
		else
		if (ClassIsChildOf(LatentActions[idx].class,actionClass) &&
			LatentActions[idx] != exceptionAction)
		{
			// if aborted,
			if (bAborted)
			{
				// then notify the action
				LatentActions[idx].AbortFor(self);
			}
			// remove action from list
			LatentActions.Remove(idx--,1);
		}
	}
}

/**
 * If this actor is not already scheduled for destruction,
 * destroy it now.
 */
simulated function OnDestroy(SeqAct_Destroy Action)
{
	if (bNoDelete || Role < ROLE_Authority)
	{
		// bNoDelete actors cannot be destroyed, and are shut down instead.
		ShutDown();
	}
	else if( !bDeleteMe )
	{
		Destroy();
	}
}

/** forces this actor to be net relevant if it is not already
 * by default, only works on level placed actors (bNoDelete)
 */
function ForceNetRelevant()
{
	if (RemoteRole == ROLE_None && bNoDelete && !bStatic)
	{
		// we now need to replicate this Actor so clients get the updated bHidden
		RemoteRole = ROLE_SimulatedProxy;
		bAlwaysRelevant = true;
		NetUpdateFrequency = 0.1;
	}
	NetUpdateTime = WorldInfo.TimeSeconds - 1.0f;
}

/**
 * ShutDown an actor.
 */

simulated function ShutDown()
{
	// Shut down physics
	SetPhysics(PHYS_None);
	// shut down collision
	SetCollision(false, false);
	if (CollisionComponent != None)
	{
		CollisionComponent.SetBlockRigidBody(false);
	}

	// shut down rendering
	SetHidden(true);
	// ignore if in a non rendered zone
	bStasis = true;

	ForceNetRelevant();

	if (RemoteRole != ROLE_None)
	{
		// force replicate flags if necessary
		SetForcedInitialReplicatedProperty(Property'Engine.Actor.bCollideActors', (bCollideActors == default.bCollideActors));
		SetForcedInitialReplicatedProperty(Property'Engine.Actor.bBlockActors', (bBlockActors == default.bBlockActors));
		SetForcedInitialReplicatedProperty(Property'Engine.Actor.bHidden', (bHidden == default.bHidden));
		SetForcedInitialReplicatedProperty(Property'Engine.Actor.Physics', (Physics == default.Physics));
	}

	// we can't set bTearOff here as that will prevent newly joining clients from receiving the state changes
	// so we just set a really low NetUpdateFrequency
	NetUpdateFrequency = 0.1;
	// force immediate network update of these changes
	NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
}

/**
 * Called upon receiving a SeqAct_CauseDamage action, calls
 * TakeDamage() with the given parameters.
 *
 * @param	Action - damage action that was activated
 */
simulated function OnCauseDamage(SeqAct_CauseDamage Action)
{
	local Controller InstigatorController;
	local Pawn InstigatorPawn;

	InstigatorController = Controller(Action.Instigator);
	if (InstigatorController == None)
	{
		InstigatorPawn = Pawn(Action.Instigator);
		if (InstigatorPawn != None)
		{
			InstigatorController = InstigatorPawn.Controller;
		}
	}
	TakeDamage(Action.DamageAmount, InstigatorController, Location, vector(Rotation) * -Action.Momentum, Action.DamageType);
}

/**
 * Called upon receiving a SeqAct_HealDamage action, calls
 * HealDamage() with the given parameters.
 *
 * @param	Action - heal action that was activated
 */
function OnHealDamage(SeqAct_HealDamage Action)
{
	local Controller InstigatorController;
	local Pawn InstigatorPawn;

	InstigatorController = Controller(Action.Instigator);
	if (InstigatorController == None)
	{
		InstigatorPawn = Pawn(Action.Instigator);
		if (InstigatorPawn != None)
		{
			InstigatorController = InstigatorPawn.Controller;
		}
	}
	HealDamage(Action.HealAmount, InstigatorController, Action.DamageType);
}

/**
 * Called upon receiving a SeqAct_Teleport action.  Grabs
 * the first destination available and attempts to teleport
 * this actor.
 *
 * @param	Action - teleport action that was activated
 */
simulated function OnTeleport(SeqAct_Teleport Action)
{
	local array<Object> objVars;
	local int idx;
	local Actor destActor;
	local Controller C;

	// find the first supplied actor
	Action.GetObjectVars(objVars,"Destination");
	for (idx = 0; idx < objVars.Length && destActor == None; idx++)
	{
		destActor = Actor(objVars[idx]);

		// If its a player variable, teleport to the Pawn not the Controller.
		C = Controller(destActor);
		if(C != None && C.Pawn != None)
		{
			destActor = C.Pawn;
		}

		// make sure the changes get replicated
		ForceNetRelevant();
		bUpdateSimulatedPosition = true;
	}
	// and set to that actor's location
	if (destActor != None && SetLocation(destActor.Location))
	{
		PlayTeleportEffect(false, true);
		if (Action.bUpdateRotation)
		{
			SetRotation(destActor.Rotation);
		}

		// make sure the changes get replicated
		ForceNetRelevant();
		bUpdateSimulatedPosition = true;
	}
	else
	{
		`warn("Unable to teleport to"@destActor);
	}
}

/**
 *	Handler for the SeqAct_SetBlockRigidBody action. Allows level designer to toggle the rigid-body blocking
 *	flag on an Actor, and will handle updating the physics engine etc.
 */
simulated function OnSetBlockRigidBody(SeqAct_SetBlockRigidBody Action)
{
	if(CollisionComponent != None)
	{
		// Turn on
		if(Action.InputLinks[0].bHasImpulse)
		{
			CollisionComponent.SetBlockRigidBody(true);
		}
		// Turn off
		else if(Action.InputLinks[1].bHasImpulse)
		{
			CollisionComponent.SetBlockRigidBody(false);
		}
	}
}

/** Handler for the SeqAct_SetPhysics action, allowing designer to change the Physics mode of an Actor. */
simulated function OnSetPhysics(SeqAct_SetPhysics Action)
{
	ForceNetRelevant();
	SetPhysics( Action.NewPhysics );
	if (RemoteRole != ROLE_None)
	{
		SetForcedInitialReplicatedProperty(Property'Engine.Actor.Physics', (Physics == default.Physics));
	}
}

/** Handler for collision action, allow designer to toggle collide/block actors */
function OnChangeCollision(SeqAct_ChangeCollision Action)
{
	SetCollision( Action.bCollideActors, Action.bBlockActors, Action.bIgnoreEncroachers );
	ForceNetRelevant();
	if (RemoteRole != ROLE_None)
	{
		// force replicate flags if necessary
		SetForcedInitialReplicatedProperty(Property'Engine.Actor.bCollideActors', (bCollideActors == default.bCollideActors));
		SetForcedInitialReplicatedProperty(Property'Engine.Actor.bBlockActors', (bBlockActors == default.bBlockActors));
		// don't bother with bIgnoreEncroachers as it isn't editable
	}
}

/** Handler for SeqAct_ToggleHidden, just sets bHidden. */
simulated function OnToggleHidden(SeqAct_ToggleHidden Action)
{
	if (Action.InputLinks[0].bHasImpulse)
	{
		SetHidden(True);
	}
	else if (Action.InputLinks[1].bHasImpulse)
	{
		SetHidden(False);
	}
	else
	{
		SetHidden(!bHidden);
	}

	ForceNetRelevant();
	if (RemoteRole != ROLE_None)
	{
		SetForcedInitialReplicatedProperty(Property'Engine.Actor.bHidden', (bHidden == default.bHidden));
	}
}

/** Attach an actor to another one. Kismet action. */
function OnAttachToActor(SeqAct_AttachToActor Action)
{
	local int			idx;
	local Actor			Attachment;
	local Controller	C;
	local Array<Object> ObjVars;

	Action.GetObjectVars(ObjVars,"Attachment");
	for( idx=0; idx<ObjVars.Length && Attachment == None; idx++ )
	{
		Attachment = Actor(ObjVars[idx]);

		// If its a player variable, attach the Pawn, not the controller
		C = Controller(Attachment);
		if( C != None && C.Pawn != None )
		{
			Attachment = C.Pawn;
		}

		if( Attachment != None )
		{
			if( Action.bDetach )
			{
				Attachment.SetBase(None);
			}
			else
			{
				// if we're a controller and have a pawn, then attach to pawn instead.
				C = Controller(Self);
				if( C != None && C.Pawn != None )
				{
					C.Pawn.DoKismetAttachment(Attachment, Action);
				}
				else
				{
					DoKismetAttachment(Attachment, Action);
				}
			}
		}
	}
}


/** Performs actual attachment. Can be subclassed for class specific behaviors. */
function DoKismetAttachment(Actor Attachment, SeqAct_AttachToActor Action)
{
	local bool		bOldCollideActors, bOldBlockActors;
	local vector	X, Y, Z;

	Attachment.SetBase(None);
	Attachment.SetHardAttach(Action.bHardAttach);

	if( Action.bUseRelativeOffset || Action.bUseRelativeRotation )
	{
		// Disable collision, so we can successfully move the attachment
		bOldCollideActors	= Attachment.bCollideActors;
		bOldBlockActors		= Attachment.bBlockActors;

		Attachment.SetCollision(FALSE, FALSE);

		if( Action.bUseRelativeRotation )
		{
			Attachment.SetRotation(Rotation + Action.RelativeRotation);
		}

		// if we're using the offset, place attachment relatively to the target
		if( Action.bUseRelativeOffset )
		{
			GetAxes(Rotation, X, Y, Z);
			Attachment.SetLocation(Location + Action.RelativeOffset.X * X + Action.RelativeOffset.Y * Y + Action.RelativeOffset.Z * Z);
		}

		// restore previous collision
		Attachment.SetCollision(bOldCollideActors, bOldBlockActors);
	}

	// Attach Actor to Base
	Attachment.SetBase(Self);
}


/**
 * Force this actor to make a noise that the AI may hear
 */
simulated function OnMakeNoise( SeqAct_MakeNoise Action )
{
	MakeNoise( Action.Loudness, 'ScriptNoise' );
}

/**
 * Event called when an AnimNodeSequence (in the animation tree of one of this Actor's SkeletalMeshComponents) reaches the end and stops.
 * Will not get called if bLooping is 'true' on the AnimNodeSequence.
 * bCauseActorAnimEnd must be set 'true' on the AnimNodeSequence for this event to get generated.
 *
 * @param	SeqNode		- Node that finished playing. You can get to the SkeletalMeshComponent by looking at SeqNode->SkelComponent
 * @param	PlayedTime	- Time played on this animation. (play rate independant).
 * @param	ExcessTime	- how much time overlapped beyond end of animation. (play rate independant).
 */
event OnAnimEnd(AnimNodeSequence SeqNode, float PlayedTime, float ExcessTime);

/**
 * Event called when a PlayAnim is called AnimNodeSequence in the animation tree of one of this Actor's SkeletalMeshComponents.
 * bCauseActorAnimPlay must be set 'true' on the AnimNodeSequence for this event to get generated.
 *
 * @param	SeqNode - Node had PlayAnim called. You can get to the SkeletalMeshComponent by looking at SeqNode->SkelComponent
 */
event OnAnimPlay(AnimNodeSequence SeqNode);

// AnimControl Matinee Track Support

/** Called when we start an AnimControl track operating on this Actor. Supplied is the set of AnimSets we are going to want to play from. */
event BeginAnimControl(array<AnimSet> InAnimSets);

/** Called each from while the Matinee action is running, with the desired sequence name and position we want to be at. */
event SetAnimPosition(name SlotName, int ChannelIndex, name InAnimSeqName, float InPosition, bool bFireNotifies, bool bLooping);

/** Called each from while the Matinee action is running, to set the animation weights for the actor. */
event SetAnimWeights(array<AnimSlotInfo> SlotInfos);

/** Called when we are done with the AnimControl track. */
event FinishAnimControl();

/**
 * Play FaceFX animations on this Actor.
 * Returns TRUE if succeeded, if failed, a log warning will be issued.
 */
event bool PlayActorFaceFXAnim(FaceFXAnimSet AnimSet, String GroupName, String SeqName);

/** Stop any matinee FaceFX animations on this Actor. */
event StopActorFaceFXAnim();

/** Called each frame by Matinee to update the weight of a particular MorphNodeWeight. */
event SetMorphWeight(name MorphNodeName, float MorphWeight);

/** Called each frame by Matinee to update the scaling on a SkelControl. */
event SetSkelControlScale(name SkelControlName, float Scale);


/**
 * Returns TRUE if Actor is playing a FaceFX anim.
 * Implement in sub-class.
 */
simulated function bool IsActorPlayingFaceFXAnim()
{
	return FALSE;
}

/** Used by Matinee in-game to mount FaceFXAnimSets before playing animations. */
event FaceFXAsset GetActorFaceFXAsset();

// for AI... bots have perfect aim shooting non-pawn stationary targets
function bool IsStationary()
{
	return true;
}

/**
 * returns the point of view of the actor.
 * note that this doesn't mean the camera, but the 'eyes' of the actor.
 * For example, for a Pawn, this would define the eye height location,
 * and view rotation (which is different from the pawn rotation which has a zeroed pitch component).
 * A camera first person view will typically use this view point. Most traces (weapon, AI) will be done from this view point.
 *
 * @param	out_Location - location of view point
 * @param	out_Rotation - view rotation of actor.
 */
simulated event GetActorEyesViewPoint( out vector out_Location, out Rotator out_Rotation )
{
	out_Location = Location;
	out_Rotation = Rotation;
}

/**
 * Searches the owner chain looking for a player.
 */
native simulated function bool IsPlayerOwned();

/* PawnBaseDied()
The pawn on which this actor is based has just died
*/
function PawnBaseDied();

simulated event byte GetTeamNum()
{
	return 255;
}

simulated function string GetLocationStringFor(PlayerReplicationInfo PRI)
{
	return "";
}

simulated function NotifyLocalPlayerTeamReceived();

/** Used by PlayerController.FindGoodView() in RoundEnded State */
simulated function FindGoodEndView(PlayerController PC, out Rotator GoodRotation)
{
	GoodRotation = PC.Rotation;
}

/** @return the optimal location to fire weapons at this actor */
simulated native function vector GetTargetLocation(optional actor RequestedBy);

/** called when this Actor was spawned by a Kismet actor factory (SeqAct_ActorFactory)
 *	after all other spawn events (PostBeginPlay(), etc) have been called
 */
event SpawnedByKismet();

/**
 * implemented by pickup type Actors to do things following a successful pickup
 * @param P the Pawn that picked us up
 *
 * @todo remove this and fix up the DenyPickupQuery() calls that use this
 */
function PickedUpBy(Pawn P);

/** called when a SeqAct_Interp action starts interpolating this Actor via matinee
 * @note this function is called on clients for actors that are interpolated clientside via MatineeActor
 * @param InterpAction the SeqAct_Interp that is affecting the Actor
 */
simulated event InterpolationStarted(SeqAct_Interp InterpAction);

/** called when a SeqAct_Interp action finished interpolating this Actor
 * @note this function is called on clients for actors that are interpolated clientside via MatineeActor
 * @param InterpAction the SeqAct_Interp that was affecting the Actor
 */
simulated event InterpolationFinished(SeqAct_Interp InterpAction);

/** called when a SeqAct_Interp action affecting this Actor received an event that changed its properties
 *	(paused, reversed direction, etc)
 * @note this function is called on clients for actors that are interpolated clientside via MatineeActor
 * @param InterpAction the SeqAct_Interp that is affecting the Actor
 */
simulated event InterpolationChanged(SeqAct_Interp InterpAction);

// @todo DB: update function comment for Actor.uc::AddDecal(....).
/** adds a DecalComponent to this Actor with the given parameters and returns it
 * @param DecalMaterial the material to use for the decal
 * @param Width decal width
 * @param Height decal height
 * @param Thickness decal thickness; if unspecified or not greater than 0, uses the default in DecalComponent
 * @param bNoClip if true, use the bNoClip code path for decal generation (requires DecalMaterial to have clamped texture coordinates)
 * @param LifeSpan if specified and non-zero, the decal will be destroyed after this many seconds
 * @param DepthBias (optional) used to override the default when specific decal types need a special depth bias, ie bloodpools
 *
 * @return the created DecalComponent
 */
simulated function DecalComponent AddDecal( MaterialInterface DecalMaterial, vector DecalLocation, rotator DecalOrientation,
						float DecalRotation, float Width, float Height, float Thickness, bool bNoClip,
						PrimitiveComponent HitComponent, DecalLifetimeData LifetimeData,
						bool bProjectOnTerrain, bool bProjectOnSkeletalMeshes, name HitBone,
						int HitNodeIndex, int HitLevelIndex,
						optional float DepthBias = -0.0001, optional float Backface = 0.001
						)
{
	local DecalComponent Decal;

	if (WorldInfo.NetMode != NM_DedicatedServer && !bDeleteMe)
	{
		Decal = new(Outer) class'DecalComponent';
		Decal.Location = DecalLocation;
		Decal.Orientation = DecalOrientation;
		Decal.DecalRotation = DecalRotation;
		Decal.Width = Width;
		Decal.Height = Height;
		Decal.Thickness = Thickness;
		Decal.FarPlane = Decal.Thickness * 0.5;
		Decal.NearPlane = -Decal.FarPlane;
		Decal.BackfaceAngle = Backface;
		Decal.bNoClip = bNoClip;
		Decal.HitComponent = HitComponent;
		Decal.HitBone = HitBone;
		Decal.HitNodeIndex = HitNodeIndex;
		Decal.HitLevelIndex = HitLevelIndex;
		Decal.LifetimeData = LifetimeData;
		Decal.DecalMaterial = DecalMaterial;
		Decal.bProjectOnTerrain = bProjectOnTerrain;
		Decal.bProjectOnSkeletalMeshes = bProjectOnSkeletalMeshes;
		Decal.DepthBias = DepthBias;
		AttachComponent(Decal);
	}
	return Decal;
}

/** Called when a PrimitiveComponent this Actor owns has:
 *     -bNotifyRigidBodyCollision set to true
 *     -ScriptRigidBodyCollisionThreshold > 0
 *     -it is involved in a physics collision where the relative velocity exceeds ScriptRigidBodyCollisionThreshold
 *
 * @param HitComponent the component of this Actor that collided
 * @param OtherComponent the other component that collided
 * @param RigidCollisionData information on the collision itslef, including contact points
 * @param ContactIndex the element in each ContactInfos' ContactVelocity and PhysMaterial arrays that corresponds
 *			to this Actor/HitComponent
 */
event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
				const out CollisionImpactData RigidCollisionData, int ContactIndex );

/** function used to update where icon for this actor should be rendered on the HUD
 *  @param NewHUDLocation is a vector whose X and Y components are the X and Y components of this actor's icon's 2D position on the HUD
 */
simulated function SetHUDLocation(vector NewHUDLocation);

/**
PostRenderFor()
Hook to allow actors to render HUD overlays for themselves.
Called only if actor was rendered this tick.
Assumes that appropriate font has already been set
*/
simulated function PostRenderFor(PlayerController PC, Canvas Canvas, vector CameraPosition, vector CameraDir);

/**
 * Notification that root motion mode changed.
 * Called only from SkelMeshComponents that have bRootMotionModeChangeNotify set.
 * This is useful for synchronizing movements.
 * For intance, when using RMM_Translate, and the event is called, we know that root motion will kick in on next frame.
 * It is possible to kill in-game physics, and then use root motion seemlessly.
 */
simulated event RootMotionModeChanged(SkeletalMeshComponent SkelComp);

/**
 * Notification called after root motion has been extracted, and before it's been used.
 * This notification can be used to alter extracted root motion before it is forwarded to physics.
 * It is only called when bRootMotionExtractedNotify is TRUE on the SkeletalMeshComponent.
 * @note: It is fairly slow in Script, so enable only when really needed.
 */
simulated event RootMotionExtracted(SkeletalMeshComponent SkelComp, out BoneAtom ExtractedRootMotionDelta);

/** called after initializing the AnimTree for the given SkeletalMeshComponent that has this Actor as its Owner
 * this is a good place to cache references to skeletal controllers, etc that the Actor modifies
 */
event PostInitAnimTree(SkeletalMeshComponent SkelComp);

defaultproperties
{
	// For safety, make everything before the async work. Move actors to
	// the during group one at a time to find bugs.
	TickGroup=TG_PreAsyncWork
	CustomTimeDilation=+1.0

	DrawScale=+00001.000000
	DrawScale3D=(X=1,Y=1,Z=1)
	bJustTeleported=true
	Role=ROLE_Authority
	RemoteRole=ROLE_None
	NetPriority=+00001.000000
	bMovable=true
	InitialState=None
	NetUpdateFrequency=100
	MessageClass=class'LocalMessage'
	bHiddenEdGroup=false
	bReplicateMovement=true
	bRouteBeginPlayEvenIfStatic=TRUE
	bPushedByEncroachers=true

	SupportedEvents(0)=class'SeqEvent_Touch'
	SupportedEvents(1)=class'SeqEvent_UnTouch'
	SupportedEvents(2)=class'SeqEvent_Destroyed'
	SupportedEvents(3)=class'SeqEvent_TakeDamage'
}
