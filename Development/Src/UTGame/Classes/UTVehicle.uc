/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicle extends UTVehicleBase
	abstract
	native(Vehicle)
	nativereplication
	notplaceable
	dependson(UTPlayerController);

/** value for Team property that indicates the vehicle's team hasn't been set */
const UTVEHICLE_UNSET_TEAM = 128;

/** If true the driver will have the flag attached to its model */
var bool bDriverHoldsFlag;

/** Determines if a driver/passenger in this vehicle can carry the flag */
var bool bCanCarryFlag;

/** Team defines which players are allowed to enter the vehicle */
var	bool bTeamLocked;

/** if true, can be healing target for link gun */
var bool bValidLinkTarget;

/** Sound played if tries to enter a locked vehicle */
var soundCue VehicleLockedSound;

/** Vehicle is unlocked when a player enters it.. */
var	bool bEnteringUnlocks;

/** Vehicle has special entry radius rules */
var bool bHasCustomEntryRadius;

/** hint for AI */
var	bool bKeyVehicle;

/** should be used by defenders */
var	bool bDefensive;

/** If true, all passengers (inc. the driver) will be ejected if the vehicle flips over */
var	bool bEjectPassengersWhenFlipped;

/** true if vehicle must be upright to be entered */
var				bool		bMustBeUpright;

/** Use stick deflection to determine throttle magnitude (used for console controllers) */
var	bool bStickDeflectionThrottle;

/** HUD should draw weapon bar for this vehicle */
var bool bHasWeaponBar;

/** PhysicalMaterial to use while driving */
var transient PhysicalMaterial DrivingPhysicalMaterial;

/** PhysicalMaterial to use while not driving */
var transient PhysicalMaterial DefaultPhysicalMaterial;

/** The variable can be used to restrict this vehicle from being reset. */
var bool bNeverReset;

/** If true, any dead bodies that get ejected when the vehicle is flipped/destroy will be destoryed immediately */
var	bool bEjectKilledBodies;

/** Used to designate this vehicle as having light armor */
var	bool bLightArmor;

/** whether or not bots should leave this vehicle if they encounter enemies */
var bool bShouldLeaveForCombat;

/** whether or not to draw this vehicle's health on the HUD in addition to the driver's health */
var bool bDrawHealthOnHUD;

/** whether or not driver pawn should always cast shadow */
var bool bDriverCastsShadow;

/** whether this vehicle has been driven at any time */
var bool bHasBeenDriven;

/** Used natively to determine if the vehicle has been upside down */
var float LastCheckUpsideDownTime;

/** Used natively to give a little pause before kicking everyone out */
var	float FlippedCount;

/** The pawn's light environment */
var LightEnvironmentComponent LightEnvironment;

/** Holds the team designation for this vehicle */
var	repnotify byte Team;

/** Track different milestones (in terms of time) for this vehicle */
var float VehicleLostTime, PlayerStartTime;

/** How long vehicle takes to respawn */
var		float           RespawnTime;

/** How long to wait before spawning this vehicle when its factory becomes active */
var		float			InitialSpawnDelay;

/** If > 0, Link Gun secondary heals an amount equal to its damage times this */
var	float LinkHealMult;

var audiocomponent	LinkedToAudio;
var soundcue		LinkedToCue;
var soundcue		LinkedEndSound;

/** Sockets this vehicle can be linked to from */
var array<Name> LinkToSockets;

/** hint for AI */
var float MaxDesireability;

/** if AI controlled and bot needs to trigger an objective not triggerable by vehicles, it will try to get out this far away */
var float ObjectiveGetOutDist;

/** The sounds to play when the horn is played */
var array<SoundCue>	HornSounds;
/** radius in which friendly bots respond to the horn by trying to get into any unoccupied seats */
var float HornAIRadius;
/** The time at which the horn was last played */
var float LastHornTime;

/** Set internally by physics - if the contact force is pressing along vehicle forward direction. */
var const bool	bFrontalCollision;

/** If bFrontalCollision is true, this indicates the collision is with a fixed object (ie not PHYS_RigidBody) */
var const bool	bFrontalCollisionWithFixed;

/*********************************************************************************************
 Look Steering
********************************************************************************************* */

/** If true, and bLookToSteer is enabled in the UTConsolePlayerController.  */
var(LookSteer) bool bUseLookSteer;

/** If true and bLookToSteer if true, steer to the direction the left stick is pointing (relative to camera). */
var(LookSteer) bool bSteerToLeftStickDirection;

/** When using 'steer to left stick dir', this is a straight ahead dead zone which makes steering directly forwards easier. */
var(LookSteer) float LeftStickDirDeadZone;

/** When bLookToSteer is enabled, relates angle between 'looking' and 'facing' and steering angle. */
var(LookSteer) float LookSteerSensitivity;

/** When error is more than this, turn on the handbrake. */
var(LookSteer) float LookSteerDeadZone;

/*********************************************************************************************
 Force Steering
********************************************************************************************* */

/** Direction the Vehicle must be moving in, (0,0,0) means 'as desired' */
var		vector				ForceMovementDirection;
/** Whether the ForceMovementDirection allows movement along its negative, thus being a line of movement instead of one way*/
var		bool				bForceDirectionAllowedNegative;
/** Whether the driver can exit inside the force volume.*/
var		bool				bAllowedExit;
/*********************************************************************************************
 Missile Warnings
********************************************************************************************* */

/** Sound to play when something locks on */
var SoundCue LockedOnSound;

/** If true, certain weapons/vehicles can lock their weapons on to this vehicle */
var bool	bHomingTarget;

/*********************************************************************************************
 Vehicular Manslaughter / Hijacking
********************************************************************************************* */

/** The Damage type to use when something get's run over */
var class<UTDamageType> RanOverDamageType;

/** speed must be greater than this for running into someone to do damage */
var float MinRunOverSpeed;

/** Sound to play when someone is run over */
var SoundCue RanOverSound;

/** The Message index for the "Hijack" announcement */
var int StolenAnnouncementIndex;

/** SoundCue to play when someone steals this vehicle */
var SoundCue StolenSound;

/** last time checked for pawns in front of vehicle and warned them of their impending doom. */
var	float LastRunOverWarningTime;

/** last time checked for pawns in front of vehicle and warned them of their impending doom. */
var	float MinRunOverWarningAim;

/** Quick link to the next vehicle in the chain */
var	UTVehicle NextVehicle;

/** Quick link to the factory that spawned this vehicle */
var	UTVehicleFactory ParentFactory;

/** bot that's about to get in this vehicle */
var	UTBot Reservation;

/** if vehicle has no driver, CheckReset() will be called at this time */
var	float	ResetTime;

/** String that describes "In a vehicle name"*/
var localized string VehiclePositionString;

/** The human readable name of this vehicle */
var localized string VehicleNameString;

/*********************************************************************************************
 Team beacons
********************************************************************************************* */

/** The maximum distance out that the Team Beacon will be displayed */
var	float TeamBeaconMaxDist;

/** The maximum distance out that the player info will be displayed */
var float TeamBeaconPlayerInfoMaxDist;

/** Scaling factor used to determine if crosshair might be over this vehicle */
var float HUDExtent;

/*********************************************************************************************
 Water Damage
********************************************************************************************* */

/** How much damage the vehicle will take when submerged in the water */
var float WaterDamage;

/** Accumulated water damage (only call take damage when > 1 */
var float AccumulatedWaterDamage;

/** Damage type it takes when submerged in the water */
var  class<DamageType> VehicleDrowningDamType;

/** Class of ExplosionLight */
var class<UTExplosionLight> ExplosionLightClass;

/** Max distance to create ExplosionLight */
var float	MaxExplosionLightDistance;

/*********************************************************************************************
 Turret controllers / Aim Variables
********************************************************************************************* */
/*
	Vehicles need to handle the replication of all variables required for the firing of a weapon.
	Each vehicle needs to have a set of variables that begin with a common prefix and will be used
	to line up the needed replicated data with that weapon.  The first weapon of the vehicle (ie: that
	which is associated with the driver/seat 0 has no prefix.

	<prefix>WeaponRotation 	- This defines the physical desired rotation of the weapon in the world.
	<prefix>FlashLocation	- This defines the hit location when an instant-hit weapon is fired
	<prefix>FlashCount		- This value is incremented after each shot
	<prefix>FiringMode		- This is assigned the firemode the weapon is currently functioning in

	Additionally, each seat can have any number of SkelControl_TurretConstrained controls associated with
	it.  When a <prefix>WeaponRotation value is set or replicated, those controls will automatically be
	updated.

	FlashLocation, FlashCount and FiringMode (associated with seat 0) are predefined in PAWN.UC.  WeaponRotation
	is defined below.  All "turret" variables must be defined "repnotify".  FlashLocation, FlashCount and FiringMode
	variables should only be replicated to non-owning clients.
*/

/** rotation for the vehicle's main weapon */
var repnotify rotator WeaponRotation;

/**	The VehicleSeat struct defines each availalbe seat in the vehicle. */
struct native VehicleSeat
{
	// ---[ Connections] ------------------------

	/** Who is sitting in this seat. */
	var() editinline  Pawn StoragePawn;

	/** Reference to the WeaponPawn if any */
	var() editinline  Vehicle SeatPawn;

	// ---[ Weapon ] ------------------------

	/** class of weapon for this seat */
	var() class<UTVehicleWeapon> GunClass;

	/** Reference to the gun */
	var() editinline  UTVehicleWeapon Gun;

	/** Name of the socket to use for effects/spawming */
	var() array<name> GunSocket;

	/** Where to pivot the weapon */
	var() array<name> GunPivotPoints;

	var	int BarrelIndex;

	/** This is the prefix for the various weapon vars (WeaponRotation, FlashCount, etc)  */
	var() string TurretVarPrefix;

	/** Cachaed names for this turret */

	var name	WeaponRotationName;
	var name	FlashLocationName;
	var name	FlashCountName;
	var name	FiringModeName;

	/** Cache pointers to the actual UProperty that is needed */

	var const pointer	WeaponRotationProperty;
	var const pointer	FlashLocationProperty;
	var const pointer	FlashCountProperty;
	var const pointer	FiringModeProperty;

	/** Holds a duplicate of the WeaponRotation value.  It's used to determine if a turret is turning */

	var rotator LastWeaponRotation;

	/** This holds all associated TurretInfos for this seat */
	var() array<name> TurretControls;

	/** Hold the actual controllers */
	var() editinline array<UTSkelControl_TurretConstrained> TurretControllers;

	/** Cached in ApplyWeaponRotation, this is the vector in the world where the player is currently aiming */
	var vector AimPoint;

	/** Cached in ApplyWeaponRotation, this is the actor the seat is currently aiming at (can be none) */
	var actor AimTarget;

	// ---[ Camera ] ----------------------------------

	/** Name of the Bone/Socket to base the camera on */
	var() name CameraTag;

	/** Optional offset to add to the cameratag location, to determine base camera */
	var() vector CameraBaseOffset;

	/** Optional offset to add to the vehicle location, to determine safe trace start point */
	var() vector CameraSafeOffset;

	/** how far camera is pulled back */
	var() float CameraOffset;

	/** The Eye Height for Weapon Pawns */
	var() float CameraEyeHeight;

	// ---[ View Limits ] ----------------------------------
	// - NOTE!! If ViewPitchMin/Max are set to 0.0f, the values associated with the host vehicle will be used

	/** Used for setting the ViewPitchMin on the Weapon pawn */
	var() float ViewPitchMin;

	/** Used for setting the ViewPitchMax on the Weapon pawn */
	var() float ViewPitchMax;

	// ---[  Pawn Visibility ] ----------------------------------

	/** Is this a visible Seat */
	var() bool bSeatVisible;

	/** Name of the Bone to use as an anchor for the pawn */
	var() name SeatBone;

	/** Offset from the origin to place the based pawn */
	var() vector SeatOffset;

	/** Any additional rotation needed when placing the based pawn */
	var() rotator SeatRotation;

	/** Name of the Socket to attach to */
	var() name SeatSocket;

	// ---[ Muzzle Flashes ] ----------------------------------
	var class<UTExplosionLight> MuzzleFlashLightClass;

	var	UTExplosionLight		MuzzleFlashLight;

	// ---[ Impact Flashes (for instant hit only) ] ----------------------------------
	var class<UTExplosionLight> ImpactFlashLightClass;

	// ---[ Misc ] ----------------------------------

	/** damage to the driver is multiplied by this value */
	var() float DriverDamageMult;

	// ---[ Sounds ] ----------------------------------

	/** The sound to play when this seat is in motion (ie: turning) */
	var AudioComponent SeatMotionAudio;

	var VehicleMovementEffect SeatMovementEffect;

	// ---[ HUD ] ----------------------------------

	var vector2D SeatIconPOS;

};

/** information for each seat a player may occupy
 * @note: this array is on clients as well, but SeatPawn and Gun will only be valid for the client in that seat
 */
var(Seats)	array<VehicleSeat> 	Seats;

/** This repliced property holds a mask of which seats are occupied.  */
var int SeatMask;

/*********************************************************************************************
 Vehicle Effects
********************************************************************************************* */

/** Holds the needed data to create various effects that repsond to different actions on the vehicle */
struct native VehicleEffect
{
	/** Tag used to trigger the effect */
	var() name EffectStartTag;

	/** Tag used to kill the effect */
	var() name EffectEndTag;

	/** If true should restart running effects, if false will just keep running */
	var() bool bRestartRunning;

	/** Template to use */
	var() ParticleSystem EffectTemplate;

	/** Socket to attach to */
	var() name EffectSocket;

	/** The Actual PSC */
	var ParticleSystemComponent EffectRef;

	structdefaultproperties
	{
		bRestartRunning=true;
	}
};

/** Holds the Vehicle Effects data */
var(Effects) array<VehicleEffect>	VehicleEffects;

struct native VehicleAnim
{
	/** Used to look up the animation */
	var() name AnimTag;

	/** Animation Sequence sets to play */
	var() array<name> AnimSeqs;

	/** Rate to play it at */
	var() float AnimRate;

	/** Does it loop */
	var() bool bAnimLoopLastSeq;

	/**  The name of the UTAnimNodeSequence to use */
	var() name AnimPlayerName;
};

/** Holds a list of vehicle animations */
var(Effects) array<VehicleAnim>	VehicleAnims;

struct native VehicleSound
{
	var() name SoundStartTag;
	var() name SoundEndTag;
	var() SoundCue SoundTemplate;
	var AudioComponent SoundRef;
};

var(Effects) array<VehicleSound> VehicleSounds;
/*********************************************************************************************
 Damage Morphing
********************************************************************************************* */

struct native FDamageMorphTargets
{
	/** These are used to reference the MorphNode that is represented by this struct */
	var	name MorphNodeName;

	/** Link to the actual node */
	var	MorphNodeWeight	MorphNode;

	/** These are used to reference the next node if this is at 0 health.  It can be none */
	var name LinkedMorphNodeName;

	/** Actual Node pointed to by LinkMorphNodeName*/
	var	int LinkedMorphNodeIndex;

	/** This holds the bone that influences this node */
	var Name InfluenceBone;

	/** This is the current health of the node.  If it reaches 0, then we should pass damage to the linked node */
	var	int Health;

	/** Holds the name of the Damage Material Scalar property to adjust when it takes damage */
	var array<name> DamagePropNames;
};


/** Holds the Damage Morph Targets */
var	array<FDamageMorphTargets> DamageMorphTargets;

/** Holds the damage skel controls */
var array<UTSkelControl_Damage> DamageSkelControls;

/** client-side health for morph targets. Used to adjust everything to the replicated Health whenever we receive it */
var int ClientHealth;

/* This allows access to the Damage Material parameters */
var MaterialInstanceConstant	DamageMaterialInstance;

/** The Team Skins (0 = red/unknown, 1 = blue)*/
var MaterialInterface TeamMaterials[2];

/*********************************************************************************************
 Smoke and Fire
********************************************************************************************* */

/** The health ratio threshold at which the vehicle will begin smoking */
var float DamageSmokeThreshold;

/** The health ratio threshhold at which the vehicle will catch on fire (and begin to take continuous damage if empty) */
var float FireDamageThreshold;

/** Damage per second if vehicle is on fire */
var float FireDamagePerSec;

/** Damage per second if vehicle is upside down*/
var float UpsideDownDamagePerSec;

/** Accrued Fire Damage */
var float AccruedFireDamage;

/*********************************************************************************************
 Misc
********************************************************************************************* */

/** The Damage Type of the explosion when the vehicle is upside down */
var class<DamageType> ExplosionDamageType;

/** Is this vehicle dead */
var repnotify bool bDeadVehicle;
/** player that killed the vehicle (replicated, but only to that player) */
var Controller KillerController;

/** view shaking that is applied to nearby players */
var UTPlayerController.ViewShakeInfo ProximityShake;

/** maximum distance at which proximity shake occurs */
var float ProximityShakeRadius;

/** The maxium distance out where an impact effect will be spawned */
var float MaxImpactEffectDistance;

/** The maxium distance out where a fire effect will be spawned */
var float MaxFireEffectDistance;

/** Natively used in determining when a bot should just out of the vehicle */
var float LastJumpOutCheck;

/** Two templates used for explosions */
var ParticleSystem ExplosionTemplate;
var array<DistanceBasedParticleTemplate> BigExplosionTemplates;
/** socket to attach big explosion to (if 'None' it won't be attached at all) */
var name BigExplosionSocket;

/** The materal instances used when showing the burning hulk */
var array<MaterialInstanceConstant> BurnOutMaterialInstances;

/** How long does it take to burn out */
var float BurnOutTime;

/** How long should the vehicle should last after being destroyed */
var float DeadVehicleLifeSpan;

/** Damage/Radius/Momentum parameters for dying explosions */
var float ExplosionDamage, ExplosionRadius, ExplosionMomentum;
/** camera shake for players near the vehicle when it explodes */
var CameraAnim DeathExplosionShake;
/** radius at which the death camera shake is full intensity */
var float InnerExplosionShakeRadius;
/** radius at which the death camera shake reaches zero intensity */
var float OuterExplosionShakeRadius;

/** Whether or not there is a turret explosion sequence on death */
var bool bHasTurretExplosion;
/** Name of the turret skel control to scale the turret to nothing*/
var name TurretScaleControlName;
/** Name of the socket location to spawn the explosion effect for the turret blowing up*/
var name TurretSocketName;
/** Explosion of the turret*/
var array<DistanceBasedParticleTemplate> DistanceTurretExplosionTemplates;

/** The offset from the TurretSocketName to spawn the turret*/
var vector TurretOffset;
/** Reference to destroyed turret for death effects */
var UTVehicleDeathPiece DestroyedTurret;
/** Class to spawn when turret destroyed */
var StaticMesh DestroyedTurretTemplate;
/** Force applied to the turret explosion */
var float TurretExplosiveForce;

/** sound for dying explosion */
var SoundCue ExplosionSound;
/** Sound for being hit by impact hammer*/
var SoundCue ImpactHitSound;
/** stores the time of the last death impact to stop spamming them from occuring*/
var float LastDeathImpactTime;

/** The sounds this vehicle will play based on impact force */
var SoundCue LargeChunkImpactSound, MediumChunkImpactSound, SmallChunkImpactSound;

/** How long until it's done burning */
var float RemainingBurn;

/** This vehicle is dead and burning */
var bool bIsBurning;

/** names of material parameters for burnout material effect */
var name BurnTimeParameterName, BurnValueParameterName;

/** If true, this vehicle is scraping against something */
var bool bIsScraping;

/** Sound to play when the vehicle is scraping against something */
var(Sounds) AudioComponent ScrapeSound;

/** Sound to play from the tires */
var(Sounds) editconst const AudioComponent TireAudioComp;
var(Sounds) array<MaterialSoundEffect> TireSoundList;
var name CurrentTireMaterial;

/** This is used to detemine a safe zone around the spawn point of the vehicle.  It won't spawn until this zone is clear of pawns */
var float	SpawnRadius;

/** Anim to play when a visible driver is driving */
var	name	DrivingAnim;

/** Sound to play when spawning in */
var SoundCue SpawnInSound;

/** Sound to play when despawning */
var SoundCue SpawnOutSound;

/*********************************************************************************************
 Flag carrying
********************************************************************************************* */
var(Flag) vector	FlagOffset;
var(Flag) rotator	FlagRotation;
var(Flag) name		FlagBone;

/*********************************************************************************************
 HUD Beacon
********************************************************************************************* */

/** Texture to display when the vehicle is locked */
var Texture2D LockedTexture;

var MaterialInstanceConstant HUDMaterialInstance;
var vector HUDLocation;
var float MapSize;
var float IconXStart;
var float IconYStart;
var float IconXWidth;
var float IconYWidth;

/** Last time trace test check for drawing postrender hud icons was performed */
var float LastPostRenderTraceTime;

/** true is last trace test check for drawing postrender hud icons succeeded */
var bool bPostRenderTraceSucceeded;

var int LastHealth;
var float HealthPulseTime;


/** offset for team beacon */
var vector TeamBeaconOffset;

/** PRI of player in passenger turret */
var UTPlayerReplicationInfo PassengerPRI;

/** offset for passenger team beacon */
var vector PassengerTeamBeaconOffset;

/** Team specific effect played when the vehicle is spawned */
var ParticleSystem SpawnInTemplates[2];
/** How long is the SpawnIn effect */
var float SpawnInTime;
/** reference to the emitter spawned */
var UTEmitter SpawnEffect;
/** whether we're currently playing the spawn effect */
var repnotify bool bPlayingSpawnEffect;
/** Burn out material (per team) */
var MaterialInterface BurnOutMaterial[2];

/** multiplier to damage from colliding with other rigid bodies */
var float CollisionDamageMult;

/** last time we took collision damage, so we don't take collision damage multiple times in the same tick */
var float LastCollisionDamageTime;

/** if true, collision damage is reduced when the vehicle collided with something below it */
var bool bReducedFallingCollisionDamage;

/*********************************************************************************************
 Camera
********************************************************************************************* */

var(Seats)	float	SeatCameraScale;
var float CameraScaleMin, CameraScaleMax;
var bool bUnlimitedCameraDistance;

/** If true, this will allow the camera to rotate under the vehicle which may obscure the view */
var(Camera)	bool bRotateCameraUnderVehicle;

/** If true, don't Z smooth lagged camera (for bumpier looking ride */
var bool bNoZSmoothing;

/** If true, don't change Z while jumping, for more dramatic looking jumps */
var bool bNoFollowJumpZ;

/** Used only if bNoFollowJumpZ=true.  True when Camera Z is being fixed. */
var bool bFixedCamZ;

/** Used only if bNoFollowJumpZ=true.  saves the Camera Z position from the previous tick. */
var float OldCamPosZ;

/** Smoothing scale for lagged camera - higher values = shorter smoothing time. */
var float CameraSmoothingFactor;

/** FOV to use when driving this vehicle */
var float DefaultFOV;

/** Saved Camera positions (for lagging camera) */
struct native TimePosition
{
	var vector Position;
	var float Time;
};
var array<TimePosition> OldPositions;

/** Amount of camera lag for this vehicle (in seconds */
var float CameraLag;

/** Smoothed Camera Offset */
var vector CameraOffset;

/** How far forward to bring camera if looking over nose of vehicle */
var(Camera) float LookForwardDist;

/** hide vehicle if camera is too close */
var	float	MinCameraDistSq;

var bool bCameraNeverHidesVehicle;

/** Stop death camera using OldCameraPosition if true */
var bool bStopDeathCamera;

/** OldCameraPosition saved when dead for use if fall below killz */
var vector OldCameraPosition;

/** for player controlled !bSeparateTurretFocus vehicles on console, this is updated to indicate whether the controller is currently turning
 * (server and owning client only)
 */
var bool bIsConsoleTurning;

/*********************************************************************************************/

var array<UTBot> Trackers;

var bool bShowDamageDebug;

/** This vehicle should display the <Locked> cusrsor */
var bool bStealthVehicle;

/** replicated information on a hit we've taken */
var UTPawn.TakeHitInfo LastTakeHitInfo;

/** stop considering LastTakeHitInfo for replication when world time passes this (so we don't replicate out-of-date hits when pawns become relevant) */
var float LastTakeHitTimeout;

// when hit with EMP, a Vehicle is Disabled.
var repnotify bool bIsDisabled;

// how long it takes to restart this vehicle when disabled.
var() float DisabledTime;

// The time at which the last EMP grenade hit this vehicle.
var float TimeLastDisabled;

/** effect played when disabled */
var ParticleSystem DisabledTemplate;
/** component that holds the disabled effect (created dynamically) */
var ParticleSystemComponent DisabledEffectComponent;

/*********************************************************************************************/

var array<name> HoverBoardAttachSockets;

/** If true, don't damp z component of vehicle velocity while it is in the air */
var bool bNoZDampingInAir;

/** If true, don't damp z component of vehicle velocity even when on the ground */
var bool bNoZDamping;

/** material specific wheel effects, applied to all attached UTVehicleWheels with bUseMaterialSpecificEffects set to true */
var array<MaterialParticleEffect> WheelParticleEffects;

/** Reference Mesh for the movement effect on seats*/
var StaticMesh ReferenceMovementMesh;

replication
{
	if (bNetDirty && !bNetOwner)
		WeaponRotation;
	if (bNetDirty)
		bTeamLocked, Team, bDeadVehicle, bPlayingSpawnEffect, bIsDisabled, SeatMask, PassengerPRI,ForceMovementDirection;

	if (bNetDirty && WorldInfo.TimeSeconds < LastTakeHitTimeout)
		LastTakeHitInfo;

	if (bNetDirty && WorldInfo.ReplicationViewers.Find('InViewer', PlayerController(KillerController)) != INDEX_NONE)
		KillerController;
}

cpptext
{
	virtual FVector GetDampingForce(const FVector& InForce);
	void RequestTrackingFor(AUTBot *Bot);
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual UBOOL JumpOutCheck(AActor *GoalActor, FLOAT Distance, FLOAT ZDiff);
	virtual FLOAT GetMaxRiseForce();
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
	virtual void OnRigidBodyCollision(const FRigidBodyCollisionInfo& Info0, const FRigidBodyCollisionInfo& Info1, const FCollisionImpactData& RigidCollisionData);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void ApplyWeaponRotation(INT SeatIndex, FRotator NewRotation);
	UBOOL CheckAutoDestruct(ATeamInfo* InstigatorTeam, FLOAT CheckRadius);
	virtual void PreNetReceive();
	virtual void PostNetReceive();
}

/*********************************************************************************************
  Native Accessors for the WeaponRotation, FlashLocation, FlashCount and FiringMode
********************************************************************************************* */
native simulated function rotator 	SeatWeaponRotation	(int SeatIndex, optional rotator NewRot,	optional bool bReadValue);
native simulated function vector  	SeatFlashLocation	(int SeatIndex, optional vector  NewLoc,	optional bool bReadValue);
native simulated function byte		SeatFlashCount		(int SeatIndex, optional byte NewCount, 	optional bool bReadValue);
native simulated function byte		SeatFiringMode		(int SeatIndex, optional byte NewFireMode,	optional bool bReadValue);

native simulated function ForceWeaponRotation(int SeatIndex, Rotator NewRotation);
native simulated function vector GetSeatPivotPoint(int SeatIndex);
native simulated function int GetBarrelIndex(int SeatIndex);

/** @return whether we are currently replicating to the Controller of the given seat
 * this would be equivalent to checking bNetOwner on that seat,
 * but bNetOwner is only valid during that Actor's replication, not during the base vehicle's
 * not complex logic, but since it's for use in vehicle replication statements, the faster the better
 */
native(999) noexport final function bool IsSeatControllerReplicationViewer(int SeatIndex);

/**
 * Initialization
 */
simulated function PostBeginPlay()
{
	local PlayerController PC;

	super.PostBeginPlay();

	ClientHealth = Health;
	LastHealth = Health;

	if (Role==ROLE_Authority)
	{
		if ( !bDeleteMe && UTGame(WorldInfo.Game) != None )
		{
			UTGame(WorldInfo.Game).RegisterVehicle(self);
		}

		// Setup the Seats array
		InitializeSeats();
	}
	else if (Seats.length > 0)
	{
		// Insure our reference to self is always setup
		Seats[0].SeatPawn = self;
	}

	PreCacheSeatNames();

	InitializeTurrets();		// Setup the turrets
	InitializeEffects();		// Setup any effects for this vehicle
	InitializeMorphs();			// Setup the damage morph targets

	// add to local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if ( UTHUD(PC.MyHUD) != None )
		{
			UTHUD(PC.MyHUD).AddPostRenderedActor(self);
		}
	}

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if ( class'UTPlayerController'.Default.PawnShadowMode == SHADOW_None )
		{
			if ( Mesh != None )
			{
				Mesh.CastShadow = false;
				Mesh.bCastDynamicShadow = false;
			}
		}
		if ( DamageMaterialInstance == none )
		{
			DamageMaterialInstance = Mesh.CreateAndSetMaterialInstanceConstant(0);
		}
	}

	VehicleEvent('Created');
}

/** Function that allows vehicles to override look-steering if they desire. */
simulated function bool AllowLookSteer()
{
	return bUseLookSteer;
}

/**
 * Console specific input modification
 */
simulated function SetInputs(float InForward, float InStrafe, float InUp)
{
	local bool bThrottlePositive;
	local UTConsolePlayerController ConsolePC;
	local rotator SteerRot, VehicleRot;
	local vector SteerDir, VehicleDir;
	local float VehicleHeading, SteerHeading, DeltaTargetHeading, LeftStickAngle, Deflection;

	Throttle = InForward;
	Steering = InStrafe;
	Rise = InUp;

	ConsolePC = UTConsolePlayerController(Controller);
	if (ConsolePC != None)
	{
		// tank, wheeled / heavy vehicles will use this

		// If we desire 'look steering' on this vehicle, do it here.
		if (ConsolePC.bLookToSteer && IsHumanControlled() && AllowLookSteer())
		{
			// If there is a deflection, look at the angle that its point in.
			Deflection = Sqrt(Throttle*Throttle + Steering*Steering);

			if(bSteerToLeftStickDirection)
			{
				if(Deflection > 0.1)
				{
					LeftStickAngle = -1.0 * Atan(Steering, Throttle);

					if(Abs(LeftStickAngle) < LeftStickDirDeadZone)
					{
						LeftStickAngle = 0.f;
					}
					else if(Abs(LeftStickAngle) < (PI/2))
					{
						if(LeftStickAngle > 0)
						{
							LeftStickAngle = ((LeftStickAngle - LeftStickDirDeadZone)/((PI/2) - LeftStickDirDeadZone))*(PI/2);
						}
						else
						{
							LeftStickAngle = ((LeftStickAngle + LeftStickDirDeadZone)/((-PI/2) + LeftStickDirDeadZone))*(-PI/2);
						}
					}

					//`log( "LeftStickAngle: " $ LeftStickAngle );
				}

				// Throttle based on deflection
				Throttle = Deflection;
			}
			else if(bStickDeflectionThrottle)
			{
				bThrottlePositive = (Throttle > 0);
				Throttle = Deflection;

				if (!bThrottlePositive)
				{
					Throttle *= -1;
				}
			}

			VehicleRot.Yaw = Rotation.Yaw;
			VehicleDir = vector(VehicleRot);

			SteerRot.Yaw = DriverViewYaw + ((32768/PI)*LeftStickAngle);
			SteerDir = vector(SteerRot);

			VehicleHeading = GetHeadingAngle(VehicleDir);
			SteerHeading = GetHeadingAngle(SteerDir);
			DeltaTargetHeading = FindDeltaAngle(SteerHeading, VehicleHeading);

			// If the way we want to go is pretty much behind us, reverse in that direction instead.
			if(bSteerToLeftStickDirection && (Abs(DeltaTargetHeading) > 0.75*PI))
			{
				SteerDir = -1.0 * SteerDir;
				SteerHeading = GetHeadingAngle(SteerDir);
				DeltaTargetHeading = FindDeltaAngle(SteerHeading, VehicleHeading);

				Throttle *= -1.0;
			}

			if (DeltaTargetHeading > LookSteerDeadZone)
			{
				Steering = FMin((DeltaTargetHeading - LookSteerDeadZone) * LookSteerSensitivity, 1.0);
			}
			else if (DeltaTargetHeading < -LookSteerDeadZone)
			{
				Steering = FMax((DeltaTargetHeading + LookSteerDeadZone) * LookSteerSensitivity, -1.0);
			}
			else
			{
				Steering = 0.0;
			}


			// Reverse steering when reversing
			if (Throttle < 0.0)
			{
				Steering = -1.0 * Steering;
			}
		}
		// flying hovering vehicles will use this
		else
		{
			//`log( " flying hovering vehicle" );
			if (bStickDeflectionThrottle)
			{
				bThrottlePositive = (Throttle > 0);

				Deflection = Sqrt(Throttle*Throttle + Steering*Steering);
				Throttle = Deflection;

				if (!bThrottlePositive)
				{
					Throttle *= -1;
				}
			}
		}

		//`log( "Throttle: " $ Throttle $ " Steering: " $ Steering );
	}
}

simulated event FellOutOfWorld(class<DamageType> dmgType)
{
    super.FellOutOfWorld(DmgType);
    bStopDeathCamera = true;
}

simulated function float GetChargePower();

simulated function PlaySpawnEffect()
{
	local ParticleSystem SpawnTemplate;

	SpawnTemplate = (Team < ArrayCount(SpawnInTemplates)) ? SpawnInTemplates[Team] : SpawnInTemplates[0];
	if (SpawnTemplate != None)
	{
		bPlayingSpawnEffect = true;
		SetHidden(True);
		SetPhysics(PHYS_None);
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			SpawnEffect = Spawn(class'UTEmit_VehicleSpawn');
			SpawnEffect.SetBase(self);
			SpawnEffect.SetLightEnvironment(LightEnvironment);
			SpawnEffect.SetTemplate(SpawnTemplate, false);


			if(SpawnInSound != none)
				PlaySound(SpawnInSound);

		}
		SetTimer(SpawnInTime, false, 'StopSpawnEffect');
	}
}

simulated function StopSpawnEffect()
{
	if (SpawnEffect != None)
	{
		SpawnEffect.Destroy();
	}
	SetHidden(False);
	bPlayingSpawnEffect = false;
	SetPhysics(PHYS_RigidBody);
	ClearTimer('StopSpawnEffect');
}

event SelfDestruct(Actor ImpactedActor);

simulated function EjectSeat(int SeatIdx)
{
	local UTVehicleBase VB;
	bShouldEject=true;
	if(SeatIdx == 0)
	{
		DriverLeave(true);
	}
	else
	{
		VB = UTVehicleBase(Seats[SeatIdx].SeatPawn);
		if(VB != none)
		{
			VB.bShouldEject = true;
			VB.DriverLeave(true);
		}
	}
}

function DisplayWeaponBar(Canvas canvas, UTHUD HUD);

/**
 * function used to update where icon for this actor should be rendered on the HUD
 *  @param 	NewHUDLocation 		is a vector whose X and Y components are the X and Y components of this actor's icon's 2D position on the HUD
 */
simulated function SetHUDLocation(vector NewHUDLocation)
{
	HUDLocation = NewHUDLocation;
}


simulated static function DrawKillIcon(Canvas Canvas, float ScreenX, float ScreenY, float HUDScaleX, float HUDScaleY)
{
	local color CanvasColor;

	// save current canvas color
	CanvasColor = Canvas.DrawColor;

	// draw vehicle shadow
	Canvas.DrawColor = class'UTHUD'.default.BlackColor;
	Canvas.DrawColor.A = CanvasColor.A;
	Canvas.SetPos( ScreenX - 2, ScreenY - 2 );
	Canvas.DrawTile(class'UTMapInfo'.default.HUDIconsT, 4 + HUDScaleX * 96, 4 + HUDScaleY * 64, default.IconXStart, default.IconYStart, default.IconXWidth, default.IconYWidth);

	// draw the vehicle icon
	Canvas.DrawColor =  class'UTHUD'.default.WhiteColor;
	Canvas.DrawColor.A = CanvasColor.A;
	Canvas.SetPos( ScreenX, ScreenY );
	Canvas.DrawTile(class'UTMapInfo'.default.HUDIconsT, HUDScaleX * 96, HUDScaleY * 64, default.IconXStart, default.IconYStart, default.IconXWidth, default.IconYWidth);
	Canvas.DrawColor = CanvasColor;
}

/**
 * When an icon for this vehicle is needed on the hud, this function is called
 */
simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, LinearColor FinalColor)
{
	if ( HUDMaterialInstance == None )
	{
		HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		HUDMaterialInstance.SetParent(MP.HUDIcons);
	}
	HUDMaterialInstance.SetVectorParameterValue('HUDColor', FinalColor);
	MP.DrawRotatedMaterialTile(Canvas,HUDMaterialInstance, HUDLocation, Rotation.Yaw + 16384, MapSize, MapSize*Canvas.ClipY/Canvas.ClipX, IconXStart, IconYStart, IconXWidth, IconYWidth);
}

/**
 * JumpOutCheck()
 * Check if bot wants to jump out of vehicle, which is currently descending towards its destination
 */
event JumpOutCheck();

/**
 * ContinueOnFoot() - used by AI Called from route finding if route can only be continued on foot.
 * @Returns true if driver left vehicle
 */
event bool ContinueOnFoot()
{
	local UTBot B;

	//@fixme FIXME: don't abandon if have enemy and other bot on team can complete whatever objective the bot is going towards

	if (bKeyVehicle)
	{
		return false;
	}
	else
	{
		VehicleLostTime = WorldInfo.TimeSeconds + 12;

		B = UTBot(Controller);
		if (B != None)
		{
			B.NoVehicleGoal = B.RouteGoal;
			if (B.RouteCache.Length > 0 && B.RouteCache[0] != None)
			{
				B.DirectionHint = Normal(B.RouteCache[0].Location - Location);
			}

			B.LeaveVehicle(false);
			return true;
		}
		else
		{
			return Super.ContinueOnFoot();
		}
	}
}

/**
 *  AI hint
 */
function bool FastVehicle()
{
	return false;
}

/************************************************************************************
 * Effects
 ***********************************************************************************/

/**
 * Initialize the effects system.  Create all the needed PSCs and set their templates
 */
simulated function InitializeEffects()
{
	local int i;

	if (WorldInfo.NetMode == NM_DedicatedServer)
		return;

	for (i=0;i<VehicleEffects.Length;i++)
	{
		VehicleEffects[i].EffectRef = new(self) class'UTParticleSystemComponent';
		if (VehicleEffects[i].EffectStartTag != 'BeginPlay')
		{
			VehicleEffects[i].EffectRef.bAutoActivate = false;
		}
		VehicleEffects[i].EffectRef.SetTemplate(VehicleEffects[i].EffectTemplate);
		Mesh.AttachComponentToSocket(VehicleEffects[i].EffectRef, VehicleEffects[i].EffectSocket);
	}
}

/**
 * Whenever a vehicle effect is triggered, this function is called (after activation) to allow for the
 * setting of any parameters associated with the effect.
 *
 * @param	TriggerName		The effect tag that describes the effect that was activated
 * @param	PSC				The Particle System component associated with the effect
 */
simulated function SetVehicleEffectParms(name TriggerName, ParticleSystemComponent PSC)
{
	local float Pct;

	if (TriggerName == 'DamageSmoke')
	{
		Pct = float(Health) / float(default.Health);
		PSC.SetFloatParameter('smokeamount', (Pct < DamageSmokeThreshold) ? (1.0 - Pct) : 0.0);
		PSC.SetFloatParameter('fireamount', (Pct < FireDamageThreshold) ? (1.0 - Pct) : 0.0);
	}
}

/**
 * Trigger or untrigger a vehicle effect
 *
 * @param	EventTag	The tag that describes the effect
 *
 */
simulated function TriggerVehicleEffect( name EventTag )
{
	local int i;

	for (i=0;i<VehicleEffects.Length;i++)
	{
		if (VehicleEffects[i].EffectRef != none && VehicleEffects[i].EffectStartTag == EventTag)
		{
			if(VehicleEffects[i].bRestartRunning)
			{
				VehicleEffects[i].EffectRef.KillParticlesForced();
				VehicleEffects[i].EffectRef.ActivateSystem();
			}
			else if(!VehicleEffects[i].EffectRef.bIsActive)
			{
				VehicleEffects[i].EffectRef.ActivateSystem();
			}


			SetVehicleEffectParms(EventTag, VehicleEffects[i].EffectRef);
		}
		else if (VehicleEffects[i].EffectRef != none && VehicleEffects[i].EffectEndTag == EventTag)
		{
			VehicleEffects[i].EffectRef.DeActivateSystem();
		}
	}
}

/**
 * Trigger or untrigger a vehicle sound
 *
 * @param	EventTag	The tag that describes the effect
 *
 */
simulated function PlayVehicleSound(name SoundTag)
{
	local int i;
	for(i=0;i<VehicleSounds.Length;++i)
	{
		if(VehicleSounds[i].SoundEndTag == SoundTag)
		{
			if(VehicleSounds[i].SoundRef != none)
			{
				VehicleSounds[i].SoundRef.Stop();
				VehicleSounds[i].SoundRef = none;
			}
		}
		if(VehicleSounds[i].SoundStartTag == SoundTag)
		{
			if(VehicleSounds[i].SoundRef == none)
			{
				VehicleSounds[i].SoundRef = CreateAudioComponent(VehicleSounds[i].SoundTemplate, false, true);
			}
			if(VehicleSounds[i].SoundRef != none && (!VehicleSounds[i].SoundRef.bWasPlaying || VehicleSounds[i].SoundRef.bFinished))
			{
				VehicleSounds[i].SoundRef.Play();
			}
		}
	}
}
/**
 * Plays a Vehicle Animation
 */
simulated function PlayVehicleAnimation(name EventTag)
{
	local int i;
	local UTAnimNodeSequence Player;

	if ( Mesh != none && mesh.Animations != none && VehicleAnims.Length > 0 )
	{
		for (i=0;i<VehicleAnims.Length;i++)
		{
			if (VehicleAnims[i].AnimTag == EventTag)
			{
				Player = UTAnimNodeSequence(Mesh.Animations.FindAnimNode(VehicleAnims[i].AnimPlayerName));
				if ( Player != none )
				{
					Player.PlayAnimationSet( VehicleAnims[i].AnimSeqs,
												VehicleAnims[i].AnimRate,
												VehicleAnims[i].bAnimLoopLastSeq );
				}
			}
		}
	}
}

function bool EagleEyeTarget()
{
	return bCanFly;
}

/**
 * An interface for causing various events on the vehicle.
 */
simulated function VehicleEvent(name EventTag)
{
	// Cause/kill any effects

	TriggerVehicleEffect(EventTag);

	// Play any animations

	PlayVehicleAnimation(EventTag);

	PlayVehicleSound(EventTag);
}


/**
 * EntryAnnouncement() - Called when Controller possesses vehicle, for any visual/audio effects
 *
 * @param	C		The controller of that possessed the vehicle
 */
simulated function EntryAnnouncement(Controller C)
{
	Super.EntryAnnouncement(C);

	if ( Role < ROLE_Authority )
		return;

	// If Stole another team's vehicle, set Team to new owner's team
	if(C != none)
	{
		if ( Team != C.GetTeamNum() )
		{
			//add stat tracking event/variable here?
			if ( Team != 255 && PlayerController(C) != None )
			{
				PlayerController(C).ReceiveLocalizedMessage( class'UTVehicleMessage', StolenAnnouncementIndex);
				if( StolenSound != None )
					PlaySound(StolenSound);
			}
			if ( C.GetTeamNum() != 255 )
				SetTeamNum( C.GetTeamNum() );
		}
	}
}

/**
  * Returns rotation used for determining valid exit positions
  */
function Rotator ExitRotation()
{
	return Rotation;
}

/**
 * FindAutoExit() Tries to find exit position on either side of vehicle, in back, or in front
 * returns true if driver successfully exited.
 *
 * @param	ExitingDriver	The Pawn that is leaving the vehicle
 */
function bool FindAutoExit(Pawn ExitingDriver)
{
	local vector X,Y,Z;
	local float PlaceDist;

	GetAxes(ExitRotation(), X,Y,Z);
	Y *= -1;

	if ( ExitRadius == 0 )
	{
		ExitRadius = CylinderComponent.CollisionRadius + 2*ExitingDriver.GetCollisionRadius();
	}
	PlaceDist = ExitRadius + ExitingDriver.GetCollisionRadius();

	if ( Controller != None )
	{
		if ( UTBot(ExitingDriver.Controller) != None )
		{
			// bot picks which side he'd prefer to get out on (since bots are bad at running around vehicles)
			if ( (UTBot(ExitingDriver.Controller).GetDirectionHint() Dot Y) < 0 )
				Y *= -1;
		}
		else
		{
			// use the controller's rotation as a hint
			if ( (Y dot vector(Controller.Rotation)) < 0 )
			{
				Y *= -1;
			}
		}
	}

	if ( VSize(Velocity) > MinCrushSpeed )
	{
		//avoid running driver over by placing in direction away from velocity
		if ( (Velocity Dot X) < 0 )
			X *= -1;
		// check if going sideways fast enough
		if ( (Velocity Dot Y) > MinCrushSpeed )
			Y *= -1;
	}

	if ( TryExitPos(ExitingDriver, GetTargetLocation() + ExitOffset + PlaceDist * Y) )
		return true;
	if ( TryExitPos(ExitingDriver, GetTargetLocation() + ExitOffset - PlaceDist * Y) )
		return true;
	if ( TryExitPos(ExitingDriver, GetTargetLocation() + ExitOffset - PlaceDist * X) )
		return true;
	if ( TryExitPos(ExitingDriver, GetTargetLocation() + ExitOffset + PlaceDist * X) )
		return true;
	return false;
}

/**
 * RanInto() called for encroaching actors which successfully moved the other actor out of the way
 *
 * @param	Other 		The pawn that was hit
 */
event RanInto(Actor Other)
{
	local vector Momentum;
	local float Speed;

	if ( Pawn(Other) == None || (Vehicle(Other) != None && !Other.IsA('UTVehicle_Hoverboard')) || Other == Instigator || Other.Role != ROLE_Authority )
		return;

	Speed = VSize(Velocity);
	if (Speed > MinRunOverSpeed)
	{
		Momentum = Velocity * 0.25 * Pawn(Other).Mass;
		if ( RanOverSound != None )
			PlaySound(RanOverSound);

		if ( WorldInfo.GRI.OnSameTeam(self,Other) )
		{
			Momentum += Speed * 0.25 * Pawn(Other).Mass * Normal(Velocity cross vect(0,0,1));
		}
		else
		{
			Other.TakeDamage(int(Speed * 0.075), GetCollisionDamageInstigator(), Other.Location, Momentum, RanOverDamageType);

			if( Pawn(Other).Health <= 0 )
			{
				if (RanOverDamageType.default.DeathCameraEffectInstigator != None)
				{
					UTPlayerController(Instigator.Controller).ClientSpawnCameraEffect(None, RanOverDamageType.default.DeathCameraEffectInstigator);
				}
			}
		}
	}
}

/**
 * TakeWaterDamage() called every tick when WaterDamage>0 and PhysicsVolume.bWaterVolume=true
 *
 * @param	DeltaTime		The amount of time passed since it was last called
 */

event TakeWaterDamage(float DeltaTime)
{
	local int ImpartedWaterDamage;
//	local vector HitLocation,HitNormal;
//	local actor EntryActor;
//	local WaterVolume W;

	// accumulate water damage to 1 before calling takedamage()
	AccumulatedWaterDamage += WaterDamage * DeltaTime;
	if ( AccumulatedWaterDamage > 1 )
	{
		ImpartedWaterDamage = AccumulatedWaterDamage;
		AccumulatedWaterDamage -= ImpartedWaterDamage;
		TakeDamage(ImpartedWaterDamage, Controller, Location, vect(0,0,0), VehicleDrowningDamType);
	}
/* FIXMESTEVE
	if ( WorldInfo.TimeSeconds - SplashTime > 0.3 )
	{
		W = WaterVolume(PhysicsVolume);

		if ( (W != None) && (W.PawnEntryActor != None) && EffectIsRelevant(Location,false) && (VSize(Velocity) > 300) )
		{
			SplashTime = WorldInfo.TimeSeconds;
			if ( !PhysicsVolume.TraceThisActor(HitLocation, HitNormal, Location - GetCollisionHeight()*vect(0,0,1), Location + GetCollisionHeight()*vect(0,0,1)) )
			{
				EntryActor = Spawn(PhysicsVolume.PawnEntryActor,self,,HitLocation,rot(16384,0,0));
			}
		}
	}
*/
}

/**
 * This function is called to see if radius damage should be applied to the driver.  It is called
 * from SVehicle::TakeRadiusDamage().
 *
 * @param	DamageAmount		The amount of damage taken
 * @param	DamageRadius		The radius that the damage covered
 * @param	EventInstigator		Who caused the damage
 * @param	DamageType			What type of damage
 * @param	Momentum			How much force should be imparted
 * @param	HitLocation			Where
 */
function DriverRadiusDamage( float DamageAmount, float DamageRadius, Controller EventInstigator,
				class<DamageType> DamageType, float Momentum, vector HitLocation, Actor DamageCauser )
{
	local int i;

	if ( bDriverIsVisible )
	{
		Super.DriverRadiusDamage(DamageAmount, DamageRadius, EventInstigator, DamageType, Momentum, HitLocation, DamageCauser);
	}

	// pass damage to seats as well but skip seats[0] since that is us and was already handled by the Super

	for (i = 1; i < Seats.length; i++)
	{
		if ( Seats[i].SeatPawn.bDriverIsVisible )
		{
			Seats[i].SeatPawn.DriverRadiusDamage(DamageAmount, DamageRadius, EventInstigator, DamageType, Momentum, HitLocation, DamageCauser);
		}
	}

}

/**
 * Called when the vehicle is destroyed.  Clean up the seats/effects/etc
 */
simulated function Destroyed()
{
	local UTVehicle	V, Prev;
	local int i;
	local PlayerController PC;

	for(i=1;i<Seats.Length;i++)
	{
   		if ( Seats[i].SeatPawn != None )
			Seats[i].SeatPawn.Destroy();
		if(Seats[i].SeatMovementEffect != none)
			SetMovementEffect(i, false);
	}
	if ( ParentFactory != None )
		ParentFactory.VehicleDestroyed( Self );		// Notify parent factory of death

	if ( UTGame(WorldInfo.Game) != None )
	{
		if ( UTGame(WorldInfo.Game).VehicleList == Self )
			UTGame(WorldInfo.Game).VehicleList = NextVehicle;
		else
		{
			Prev = UTGame(WorldInfo.Game).VehicleList;
			if ( Prev != None )
				for ( V=UTGame(WorldInfo.Game).VehicleList.NextVehicle; V!=None; V=V.NextVehicle )
				{
					if ( V == self )
					{
						Prev.NextVehicle = NextVehicle;
						break;
					}
					else
						Prev = V;
				}
		}
	}
	super.Destroyed();

	// remove from local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
		if ( UTHUD(PC.MyHUD) != None )
			UTHUD(PC.MyHUD).RemovePostRenderedActor(self);
}

simulated function DisableVehicle()
{
	local int seatIdx;

	bIsDisabled = true;

	if (Role == ROLE_Authority)
	{
		SetTimer(DisabledTime, false, 'EnableVehicle');

		// everybody out!
		if (bDriving)
		{
			DriverLeave(true);
		}
		for (seatIdx = 0; seatIdx < seats.Length; ++seatIdx)
		{
			if (Seats[seatIdx].SeatPawn != None && Seats[SeatIdx].SeatPawn.bDriving)
			{
				Seats[seatIdx].SeatPawn.DriverLeave(true); // and all the passengers
			}
		}
	}

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if (DisabledEffectComponent == None)
		{
			DisabledEffectComponent = new(self) class'ParticleSystemComponent';
			DisabledEffectComponent.SetTemplate(DisabledTemplate);
			AttachComponent(DisabledEffectComponent);
		}
		DisabledEffectComponent.ActivateSystem();
	}
}

simulated function EnableVehicle()
{
	bIsDisabled = false;
	if (WorldInfo.NetMode != NM_DedicatedServer && DisabledEffectComponent != None)
	{
		DetachComponent(DisabledEffectComponent);
		DisabledEffectComponent = None;
	}
}

/**
 * This event occurs when the physics determines the vehicle is upside down or empty and on fire.  Called from AUTVehicle::TickSpecial()
 */
event TakeFireDamage()
{
	local int CurrentDamage;

	CurrentDamage = int(AccruedFireDamage);
	AccruedFireDamage -= CurrentDamage;
	TakeDamage(CurrentDamage, Controller, Location, vect(0,0,0), ExplosionDamageType);
}

/**
 * Given the variable prefix, find the seat index that is associated with it
 *
 * @returns the index if found or -1 if not found
 */

simulated function int GetSeatIndexFromPrefix(string Prefix)
{
	local int i;

	for (i=0; i < Seats.Length; i++)
	{
		if (Seats[i].TurretVarPrefix ~= Prefix)
		{
			return i;
		}
	}
	return -1;
}

/** used on console builds to set the value of bIsConsoleTurning on the server */
reliable server function ServerSetConsoleTurning(bool bNewConsoleTurning)
{
	bIsConsoleTurning = bNewConsoleTurning;
}

simulated function ProcessViewRotation(float DeltaTime, out rotator out_ViewRotation, out rotator out_DeltaRot)
{
	local int i, MaxDelta;
	local float MaxDeltaDegrees;

	if (WorldInfo.bUseConsoleInput)
	{
		if (!bSeparateTurretFocus && ShouldClamp())
		{
			if (out_DeltaRot.Yaw == 0)
			{
				if (bIsConsoleTurning)
				{
					// if controller stops rotating on a vehicle whose view rotation yaw gets clamped,
					// set the controller's yaw to where we got so that there's no control lag
					out_ViewRotation.Yaw = GetClampedViewRotation().Yaw;
					bIsConsoleTurning = false;
					ServerSetConsoleTurning(false);
				}
			}

			else if (!bIsConsoleTurning)
			{
				// don't allow starting a new turn if the view would already be clamped
				// because that causes nasty jerking
				if (GetClampedViewRotation().Yaw == Controller.Rotation.Yaw)
				{
					bIsConsoleTurning = true;
					ServerSetConsoleTurning(true);
				}
				else
				{
					out_DeltaRot.Yaw = 0;
				}
			}
		}

		// clamp player rotation to turret rotation speed
		for (i = 0; i < Seats[0].TurretControllers.length; i++)
		{
			MaxDeltaDegrees = FMax(MaxDeltaDegrees, Seats[0].TurretControllers[i].LagDegreesPerSecond);
		}
		if (MaxDeltaDegrees > 0.0)
		{
			MaxDelta = int(MaxDeltaDegrees * 182.0444 * DeltaTime);
			out_DeltaRot.Pitch = (out_DeltaRot.Pitch >= 0) ? Min(out_DeltaRot.Pitch, MaxDelta) : Max(out_DeltaRot.Pitch, -MaxDelta);
			out_DeltaRot.Yaw = (out_DeltaRot.Yaw >= 0) ? Min(out_DeltaRot.Yaw, MaxDelta) : Max(out_DeltaRot.Yaw, -MaxDelta);
			out_DeltaRot.Roll = (out_DeltaRot.Roll >= 0) ? Min(out_DeltaRot.Roll, MaxDelta) : Max(out_DeltaRot.Roll, -MaxDelta);
		}
	}
	Super.ProcessViewRotation(DeltaTime, out_ViewRotation, out_DeltaRot);
}

simulated function rotator GetClampedViewRotation()
{
	local rotator ViewRotation, ControlRotation, MaxDelta;
	local UTVehicleWeapon VWeap;

	VWeap = UTVehicleWeapon(Weapon);
	if (VWeap != None && ShouldClamp())
	{
		// clamp view yaw so that it doesn't exceed how far the vehicle can aim on console
		ViewRotation.Yaw = Controller.Rotation.Yaw;
		MaxDelta.Yaw = ACos(VWeap.GetMaxFinalAimAdjustment()) * 180.0 / Pi * 182.0444;
		if (!ClampRotation(ViewRotation, Rotation, MaxDelta, MaxDelta))
		{
			// prevent the controller's rotation from diverging too much from the actual view rotation
			ControlRotation.Yaw = Controller.Rotation.Yaw;
			if (!ClampRotation(ControlRotation, ViewRotation, rot(0,16384,0), rot(0,16384,0)))
			{
				ControlRotation.Pitch = Controller.Rotation.Pitch;
				ControlRotation.Roll = Controller.Rotation.Roll;
				Controller.SetRotation(ControlRotation);
			}
		}

		ViewRotation.Pitch = Controller.Rotation.Pitch;
		ViewRotation.Roll = Controller.Rotation.Roll;
		return ViewRotation;
	}
	else
	{
		return super.GetViewRotation();
	}
}

simulated function bool ShouldClamp()
{
	return true;
}

simulated event rotator GetViewRotation()
{
	if (bIsConsoleTurning && !bSeparateTurretFocus && PlayerController(Controller) != None)
	{
		return GetClampedViewRotation();
	}
	else
	{
		return Super.GetViewRotation();
	}
}

/**
 * this function is called when a weapon rotation value has changed.  It sets the DesiredboneRotations for each controller
 * associated with the turret.
 *
 * Network: Remote clients.  All other cases are handled natively
 * FIXME: Look at handling remote clients natively as well
 *
 * @param	SeatIndex		The seat at which the rotation changed
 */

simulated function WeaponRotationChanged(int SeatIndex)
{
	local int i;

	if ( SeatIndex>=0 )
	{
		for (i=0;i<Seats[SeatIndex].TurretControllers.Length;i++)
		{
			Seats[SeatIndex].TurretControllers[i].DesiredBoneRotation = SeatWeaponRotation(SeatIndex,,true);
		}
	}
}

/**
 * This event is triggered when a repnotify variable is received
 *
 * @param	VarName		The name of the variable replicated
 */

simulated event ReplicatedEvent(name VarName)
{
	local string VarString;
	local int SeatIndex;

	if (VarName == 'bPlayingSpawnEffect')
	{
		if (bPlayingSpawnEffect)
		{
			if (Team != UTVEHICLE_UNSET_TEAM)
			{
				PlaySpawnEffect();
			}
		}
		else
		{
			StopSpawnEffect();
		}
	}
	else if ( VarName == 'Team' )
	{
		TeamChanged();
		if (bPlayingSpawnEffect)
		{
			PlaySpawnEffect();
		}
	}
	else if (VarName == 'bDeadVehicle')
	{
		BlowupVehicle();
	}
	else if (VarName == 'bIsDisabled')
	{
		if (bIsDisabled)
		{
			DisableVehicle();
		}
		else
		{
			EnableVehicle();
		}
	}
	else
	{

		// Ok, some magic occurs here.  The turrets/seat use a prefix system to determine
		// which values to adjust. Here we decode those values and call the appropriate functions

		// First check for <xxxxx>weaponrotation

		VarString = ""$VarName;
		if ( Right(VarString, 14) ~= "weaponrotation" )
		{
			SeatIndex = GetSeatIndexFromPrefix( Left(VarString, Len(VarString)-14) );
			if (SeatIndex >= 0)
			{
				WeaponRotationChanged(SeatIndex);
			}
		}

		// Next, check for <xxxxx>flashcount

		else if ( Right(VarString, 10) ~= "flashcount" )
		{
			SeatIndex = GetSeatIndexFromPrefix( Left(VarString, Len(VarString)-10) );
			if ( SeatIndex>=0 )
			{
				Seats[SeatIndex].BarrelIndex++;

				if ( SeatFlashCount(SeatIndex,,true) > 0 )
				{
					VehicleWeaponFired(true, vect(0,0,0), SeatIndex);
				}
				else
				{
					VehicleWeaponStoppedFiring(true,SeatIndex);
				}
			}
		}

		// finally <xxxxxx>flashlocation

		else if ( Right(VarString, 13) ~= "flashlocation" )
		{
			SeatIndex = GetSeatIndexFromPrefix( Left(VarString, Len(VarString)-13) );
			if ( SeatIndex>=0 )
			{
				Seats[SeatIndex].BarrelIndex++;

				if ( !IsZero(SeatFlashLocation(SeatIndex,,true)) )
				{
					VehicleWeaponFired(true, SeatFlashLocation(SeatIndex,,true), SeatIndex);
				}
				else
				{
					VehicleWeaponStoppedFiring(true,SeatIndex);
				}
			}
		}
		else
		{
			super.ReplicatedEvent(VarName);
		}
	}
}

/**
 * AI Hint
 * @returns true if there is an occupied turret
 */

function bool HasOccupiedTurret()
{
	local int i;

	for (i = 1; i < Seats.length; i++)
		if ( Seats[i].SeatPawn.Controller != None )
			return true;

	return false;
}

/**
 * This function is called when the driver's status has changed.
 */
simulated function DrivingStatusChanged()
{
	/* FIXMESTEVE
    local PlayerController PC;

	PC = Level.GetLocalPlayerController();

	if (bDriving && PC != None && (PC.ViewTarget == None || !(PC.ViewTarget.IsBasedOn(self))))
	bDropDetail = (Level.bDropDetail || (Level.DetailMode == DM_Low));
    else
		bDropDetail = False;
	*/

	// turn parking friction on or off
	bUpdateWheelShapes = true;

	// possibly use different physical material while being driven (to allow properties like friction to change).
	if ( bDriving )
	{
		if ( DrivingPhysicalMaterial != None )
		{
			Mesh.SetPhysMaterialOverride(DrivingPhysicalMaterial);
		}
	}
	else if ( DefaultPhysicalMaterial != None )
	{
		Mesh.SetPhysMaterialOverride(DefaultPhysicalMaterial);
	}

	if ( bDriving && !bIsDisabled )
	{
		VehiclePlayEnterSound();
	}
	else if ( Health > 0 )
	{
		VehiclePlayExitSound();
	}

	bBlocksNavigation = !bDriving;

	VehicleEvent(bDriving ? 'EngineStart' : 'EngineStop');

	// if the vehicle is being driven and this vehicle should cause view shaking, turn on the timer to check for nearby local players
	if (bDriving && WorldInfo.NetMode != NM_DedicatedServer && ProximityShakeRadius > 0.0 && (ProximityShake.OffsetTime > 1.0 || ProximityShake.RotTime > 1.0))
	{
		SetTimer(0.5, true, 'ProximityShakeTimer');
	}
	else
	{
		ClearTimer('ProximityShakeTimer');
	}
}

event OnAnimEnd(AnimNodeSequence SeqNode, float PlayedTime, float ExcessTime)
{
	super.OnAnimEnd(SeqNode,PlayedTime,ExcessTime);
	if(bDriving)
	{
		VehicleEvent('Idle');
	}
}
/**
 * called on a timer to check if any local players are close enough to this vehicle to shake their view
 */
simulated function ProximityShakeTimer()
{
	local PlayerController PC;
	local bool bIsWeaponPawn;
	local UTPlayerController.ViewShakeInfo NewViewShake;
	local int i;
	local float ShakeMag;

	if (!IsZero(Velocity))
	{
		foreach LocalPlayerControllers(class'PlayerController', PC)
		{
			// only players controlling a pawn that is currently walking should shake
			if (PC.IsA('UTPlayerController') && PC.Pawn != None && PC.Pawn.Physics == PHYS_Walking)
			{
				// never shake for players controlling another seat in this vehicle
				for (i = 0; i < Seats.Length && !bIsWeaponPawn; i++)
				{
					if (Seats[i].SeatPawn == PC.Pawn)
					{
						bIsWeaponPawn = true;
					}
				}
				if (!bIsWeaponPawn)
				{
					// shake magnitude is adjusted by how far away the player is relative to the shake radius
					ShakeMag = FClamp(1.f - (VSize(Location - PC.Pawn.Location) / ProximityShakeRadius), 0.f, 1.f);
					if (ShakeMag > 0.f)
					{
						NewViewShake = ProximityShake;
						NewViewShake.OffsetMag *= ShakeMag;
						NewViewShake.RotMag *= ShakeMag;
						UTPlayerController(PC).ShakeView(NewViewShake);
					}
				}
			}
		}
	}
}

/**
 * @Returns true if a seat is not occupied
 */

function bool SeatAvailable(int SeatIndex)
{
	return Seats[SeatIndex].SeatPawn.Controller == none;
}

/**
 * @return true if there is a seat
 */
function bool AnySeatAvailable()
{
	local int i;
	for (i=0;i<Seats.Length;i++)
	{
		if (Seats[i].SeatPawn.Controller==none)
			return true;
	}
	return false;
}

/**
 * @returns the Index for this Controller's current seat or -1 if there isn't one
 */
simulated function int GetSeatIndexForController(controller ControllerToMove)
{
	local int i;
	for (i=0;i<Seats.Length;i++)
	{
		if (Seats[i].SeatPawn.Controller != none && Seats[i].SeatPawn.Controller == ControllerToMove )
		{
			return i;
		}
	}
	return -1;
}

/**
 * @returns the controller of a given seat.  Can be none if the seat is empty
 */
function controller GetControllerForSeatIndex(int SeatIndex)
{
	return Seats[SeatIndex].SeatPawn.Controller;
}

/**
request change to adjacent vehicle seat
*/
reliable server function ServerAdjacentSeat(int Direction, Controller C)
{
	local int CurrentSeat, NewSeat;

	CurrentSeat = GetSeatIndexForController(C);
	if (CurrentSeat != INDEX_NONE)
	{
		NewSeat = CurrentSeat;
		do
		{
			NewSeat += Direction;
			if (NewSeat < 0)
			{
				NewSeat = Seats.Length - 1;
			}
			else if (NewSeat == Seats.Length)
			{
				NewSeat = 0;
			}
			if (NewSeat == CurrentSeat)
			{
				// no available seat
				if ( PlayerController(C) != None )
				{
					PlayerController(C).ClientPlaySound(VehicleLockedSound);
				}
				return;
			}
		} until (SeatAvailable(NewSeat));

		// change to the seat we found
		ChangeSeat(C, NewSeat);
	}
}

/**
 * Called when a client is requesting a seat change
 *
 * @network	Server-Side
 */
reliable server function ServerChangeSeat(int RequestedSeat)
{
	if ( RequestedSeat == -1 )
		DriverLeave(false);
	else
		ChangeSeat(Controller, RequestedSeat);
}

/**
 * This function looks at 2 controllers and decides if one as priority over the other.  Right now
 * it looks to see if a human is against a bot but it could be extended to use rank/etc.
 *
 * @returns	ture if First has priority over second
 */
function bool HasPriority(controller First, controller Second)
{
	if ( First != Second && PlayerController(First) != none && PlayerController(Second) == none)
		return true;
	else
		return false;
}

/**
 * ChangeSeat, this controller to change from it's current seat to a new one if (A) the new
 * set is empty or (B) the controller looking to move has Priority over the controller already
 * there.
 *
 * If the seat is filled but the new controller has priority, the current seat holder will be
 * bumped and swapped in to the seat left vacant.
 *
 * @param	ControllerToMove		The Controller we are trying to move
 * @param	RequestedSeat			Where are we trying to move him to
 *
 * @returns true if successful
 */
function bool ChangeSeat(Controller ControllerToMove, int RequestedSeat)
{
	local int OldSeatIndex;
	local Pawn OldPawn, BumpPawn;
	local Controller BumpController;

	// Make sure we are looking to switch to a valid seat
	if ( (RequestedSeat >= Seats.Length) || (RequestedSeat < 0) )
	{
		return false;
	}

	// get the seat index of the pawn looking to move.
	OldSeatIndex = GetSeatIndexForController(ControllerToMove);
	if (OldSeatIndex == -1)
	{
		// Couldn't Find the controller, should never happen
		`Warn("[Vehicles] Attempted to switch" @ ControllerToMove @ "to a seat in" @ self @ " when he is not already in the vehicle");
		return false;
	}

	// If someone is in the seat, see if we can bump him
	if (!SeatAvailable(RequestedSeat))
	{
		// Get the Seat holder's controller and check it for Priority
		BumpController = GetControllerForSeatIndex(RequestedSeat);
		if (BumpController == none)
		{
			`warn("[Vehicles]" @ ControllertoMove @ "Attempted to bump a phantom Controller in seat in" @ RequestedSeat @ " (" $ Seats[RequestedSeat].SeatPawn $ ")");
			return false;
		}

		if ( !HasPriority(ControllerToMove,BumpController) )
		{
			// Nope, same or great priority on the seat holder, deny the move
			if ( PlayerController(ControllerToMove) != None )
			{
				PlayerController(ControllerToMove).ClientPlaySound(VehicleLockedSound);
			}
			return false;
		}

		// If we are bumping someone, free their seat.
		if (BumpController != None)
		{
			BumpPawn = Seats[RequestedSeat].StoragePawn;
			Seats[RequestedSeat].SeatPawn.DriverLeave(true);

			// Handle if we bump the driver
			if (RequestedSeat == 0)
			{
				// Reset the controller's AI if needed
				if (BumpController.RouteGoal == self)
				{
					BumpController.RouteGoal = None;
				}
				if (BumpController.MoveTarget == self)
				{
					BumpController.MoveTarget = None;
				}
			}
		}
	}

	OldPawn = Seats[OldSeatIndex].StoragePawn;

	// Leave the current seat and take over the new one
	Seats[OldSeatIndex].SeatPawn.DriverLeave(true);
	if (OldSeatIndex == 0)
	{
		// Reset the controller's AI if needed
		if (ControllerToMove.RouteGoal == self)
		{
			ControllerToMove.RouteGoal = None;
		}
		if (ControllerToMove.MoveTarget == self)
		{
			ControllerToMove.MoveTarget = None;
		}
	}

	if (RequestedSeat == 0)
	{
		DriverEnter(OldPawn);
	}
	else
	{
		PassengerEnter(OldPawn, RequestedSeat);
	}


	// If we had to bump a pawn, seat them in this controller's old seat.
	if (BumpPawn != None)
	{
		if (OldSeatIndex == 0)
		{
			DriverEnter(BumpPawn);
		}
		else
		{
			PassengerEnter(BumpPawn, OldSeatIndex);
		}
	}
	return true;
}

/**
 * This event is called when the pawn is torn off
 */
simulated event TornOff()
{
	`warn(self @ "Torn off");
}

function Controller GetCollisionDamageInstigator()
{
	// give vehicle killer credit if possible
	return (KillerController != None) ? KillerController : Super.GetCollisionDamageInstigator();
}

/**
 * See Pawn::Died()
 */
function bool Died(Controller Killer, class<DamageType> DamageType, vector HitLocation)
{
	local UTCarriedObject Flag;
	local UTPawn UTP;
	local int i;

	// before we kill the vehicle and everyone inside, eject anyone who has a shieldbelt
	for (i=0;i<Seats.Length;i++)
	{
		if (Seats[i].SeatPawn != None || Seats[i].StoragePawn != None)
		{
			UTP = UTPawn(Seats[i].StoragePawn);
			if((UTP != none) && UTP.ShieldBeltArmor > 0)
			{
				UTP.ShieldBeltArmor = 0;
				UTP.SetOverlayMaterial(none);
				EjectSeat(i);
			}
		}
	}
	if ( Super(Vehicle).Died(Killer, DamageType, HitLocation) )
	{
		KillerController = Killer;
		HitDamageType = DamageType; // these are replicated to other clients
		TakeHitLocation = HitLocation;
		BlowupVehicle();

		HandleDeadVehicleDriver();

		for (i = 1; i < Seats.Length; i++)
		{
			if (Seats[i].SeatPawn != None)
			{
				// kill the WeaponPawn with the appropriate killer, etc for kill credit and death messages
				Seats[i].SeatPawn.Died(Killer, DamageType, HitLocation);
			}
		}

		// drop flag
		ForEach BasedActors(class'UTCarriedObject' , Flag)
		{
			Flag.SetBase(None);
		}

		// notify vehicle factory
		if (ParentFactory != None)
		{
			ParentFactory.VehicleDestroyed(self);
			ParentFactory = None;
		}
		return true;
	}
	return false;
}

/**
 * Call this function to blow up the vehicle
 */
simulated function BlowupVehicle()
{
	if(bDriving)
	{
		VehicleEvent('EngineStop');
	}
	bCanBeBaseForPawns = false;
	LinkHealMult = 0.0;
	GotoState('DyingVehicle');
	AddVelocity(TearOffMomentum, TakeHitLocation, HitDamageType);
	bDeadVehicle = true;
	bStayUpright = false;
	if ( StayUprightConstraintInstance != None )
	{
		StayUprightConstraintInstance.TermConstraint();
	}
}

/**
  * if bHasCustomEntryRadius, this is called to see if Pawn P is in it.
  */
function bool InCustomEntryRadius(Pawn P)
{
	return false;
}

simulated function PlayerReplicationInfo GetSeatPRI(int SeatNum)
{
	if ( Role == ROLE_Authority )
	{
		return Seats[SeatNum].SeatPawn.PlayerReplicationInfo;
	}
	else
	{
		return (SeatNum==0) ? PlayerReplicationInfo : PassengerPRI;
	}
}

/**
 * CanEnterVehicle()
 * @return true if Pawn P is allowed to enter this vehicle
 */
simulated function bool CanEnterVehicle(Pawn P)
{
	local int i;
	local bool bSeatAvailable, bIsHuman;
	local PlayerReplicationInfo SeatPRI;

	if ( P.bIsCrouched || (P.DrivenVehicle != None) || (P.Controller == None) || !P.Controller.bIsPlayer
	     || Health <= 0 )
	{
		return false;
	}

	// check for available seat, and no enemies in vehicle
	// allow humans to enter if full but with bots (TryToDrive() will kick one out if possible)
	bIsHuman = P.IsHumanControlled();
	bSeatAvailable = false;
	for (i=0;i<Seats.Length;i++)
	{
		SeatPRI = GetSeatPRI(i);
		if (SeatPRI == None || (bIsHuman && SeatPRI.bBot))
		{
			bSeatAvailable = true;
		}
		else if ( !WorldInfo.GRI.OnSameTeam(P, SeatPRI) )
			return false;
	}

	return bSeatAvailable;
}

/**
 * The pawn Driver has tried to take control of this vehicle
 *
 * @param	P		The pawn who wants to drive this vehicle
 */
function bool TryToDrive(Pawn P)
{
	local vector X,Y,Z;
	local bool bFreedSeat;
	local int i;
	local UTBot B;

	// don't allow while playing spawn effect
	if (bPlayingSpawnEffect)
	{
		return false;
	}

	// Does the vehicle need to be uprighted?
	if ( bIsInverted && bMustBeUpright && !bVehicleOnGround && VSize(Velocity) <= 5.0f )
	{
		if ( bCanFlip )
		{
			bIsUprighting = true;
			UprightStartTime = WorldInfo.TimeSeconds;
			GetAxes(Rotation,X,Y,Z);
			bFlipRight = ((P.Location - Location) dot Y) > 0;
		}
		return false;
	}

	if ( !CanEnterVehicle(P) || (Vehicle(P) != None) )
	{
		return false;
	}

	// Check vehicle Locking....
	if (!bIsDisabled && (Team == UTVEHICLE_UNSET_TEAM || !bTeamLocked || !WorldInfo.Game.bTeamGame || WorldInfo.GRI.OnSameTeam(self,P) ))
	{
		if (bEnteringUnlocks)
		{
			bTeamLocked = false;
			if (ParentFactory != None)
			{
				ParentFactory.VehicleTaken();
			}
		}

		if (!AnySeatAvailable())
		{
			if (WorldInfo.GRI.OnSameTeam(self, P))
			{
				// kick out the first bot in the vehicle to make way for this driver
				for (i = 0; i < Seats.length; i++)
				{
					B = UTBot(Seats[i].SeatPawn.Controller);
					if (B != None && Seats[i].SeatPawn.DriverLeave(false))
					{
						// we can't tell the bot to re-evaluate right away, because it will often get right back in this vehicle
						B.SetTimer(0.01, false, 'WhatToDoNext');
						bFreedSeat = true;
						break;
					}
				}
			}

			if (!bFreedSeat)
			{
				// we were unable to kick a bot out
				return false;
			}
		}

		// Look to see if the driver seat is open
		return (Driver == None) ? DriverEnter(P) : PassengerEnter(P, GetFirstAvailableSeat());
	}
	else
	{
		VehicleLocked( P );
		return false;
	}
}

/**
 * Pawn tried to enter vehicle, but it's locked!!
 *
 * @param	P	The pawn that tried
 */
function VehicleLocked( Pawn P )
{
	local PlayerController PC;

	PC = PlayerController(P.Controller);

	if ( PC != None )
	{
		PC.ClientPlaySound(VehicleLockedSound);
		PC.ReceiveLocalizedMessage(class'UTVehicleMessage', 1);
	}
}

/**
 * PostRenderFor() Hook to allow pawns to render HUD overlays for themselves.
 * Assumes that appropriate font has already been set
 * FIXMESTEVE - special beacon when speaking (SpeakingBeaconTexture)
 *
 * @param	PC		The Player Controller who is rendering this pawn
 * @param	Canvas	The canvas to draw on
 */
simulated function PostRenderFor(PlayerController PC, Canvas Canvas, vector CameraPosition, vector CameraDir)
{
	local float TextXL, XL, YL, Dist, ScaledDist, HealthX, HealthY, xscale, EntryCheckDist, MaxOffset;
	local vector ScreenLoc, HitNormal, HitLocation, CrossDir;
	local actor HitActor;
	local LinearColor TeamColor;
	local Color	TextColor;
	local string ScreenName;
	local bool bShowLocked, bShowUseable;
	local UTWeapon Weap;
	local UTHUD HUD;

	// must have been rendered
	if ( !LocalPlayer(PC.Player).GetActorVisibility(self) )
		return;

	// must be in front of player to render HUD overlay
	if ( (CameraDir dot (Location - CameraPosition)) <= 0 )
		return;

	// only render overlay for teammates or if un-piloted
	if ( (WorldInfo.GRI == None) || (PC.ViewTarget == None) || (PC == Controller) || (PC.ViewTarget == self) || ((UTWeaponPawn(PC.ViewTarget) != None) && (UTWeaponPawn(PC.ViewTarget).MyVehicle == self)) )
		return;

	bShowLocked = bIsDisabled || (bTeamLocked && (PlayerReplicationInfo == None) && (Team != 255));
	if ( !WorldInfo.GRI.OnSameTeam(self, PC) )
	{
		if ( !bShowLocked && !PC.PlayerReplicationInfo.bOnlySpectator )
		{
			// if not on same team, then only draw icon if locked
			// maybe change to action music if close enough
			if ( (WorldInfo.TimeSeconds - LastPostRenderTraceTime > 0.5) && !UTPlayerController(PC).AlreadyInActionMusic() && (Dist < VSize(PC.ViewTarget.Location - Location)) && !IsInvisible() )
			{
				// check whether close enough to crosshair
				if ( (Abs(screenLoc.X - 0.5*Canvas.ClipX) < 0.1 * Canvas.ClipX)
					&& (Abs(screenLoc.Y - 0.5*Canvas.ClipY) < 0.1 * Canvas.ClipY) )
				{
					// periodically make sure really visible using traces
					if ( FastTrace(Location, CameraPosition,, true)
									|| FastTrace(Location+GetCollisionHeight()*vect(0,0,1), CameraPosition,, true) )
					{
						UTPlayerController(PC).ClientMusicEvent(0);;
					}
				}
				LastPostRenderTraceTime = WorldInfo.TimeSeconds + 0.2*FRand();
			}
			return;
		}
	}
	else
	{
		// can't be locked if on same team
		bShowLocked = false;
	}

	Dist = VSize(CameraPosition - Location);

	ScaledDist = TeamBeaconMaxDist * FClamp(0.02f * GetCollisionRadius(),1.f,3.f);
	if ( PlayerReplicationInfo == None )
	{
		ScaledDist *= 0.25;
	}
	if ( PC.LODDistanceFactor * Dist > ScaledDist )
	{
		// check whether close enough to crosshair
		screenLoc = Canvas.Project(Location + TeamBeaconOffset);
		if ( (Abs(screenLoc.X - 0.5*Canvas.ClipX) > 0.1 * Canvas.ClipX)
			|| (Abs(screenLoc.Y - 0.5*Canvas.ClipY) > 0.1 * Canvas.ClipY) )
			return;
	}
	else
	{
		screenLoc = Canvas.Project( bShowLocked ? Location : Location + TeamBeaconOffset);
	}

	// make sure not clipped out
	if (screenLoc.X < 0 ||
		screenLoc.X >= Canvas.ClipX ||
		screenLoc.Y < 0 ||
		screenLoc.Y >= Canvas.ClipY)
	{
		return;
	}

	// make sure not behind weapon
	if ( UTPawn(PC.Pawn) != None )
	{
		Weap = UTWeapon(UTPawn(PC.Pawn).Weapon);
		if ( (Weap != None) && Weap.CoversScreenSpace(screenLoc, Canvas) )
		{
			return;
		}
	}

	// periodically make sure really visible using traces
	if ( WorldInfo.TimeSeconds - LastPostRenderTraceTime > 0.5 )
	{
		LastPostRenderTraceTime = WorldInfo.TimeSeconds + 0.2*FRand();
		bPostRenderTraceSucceeded = FastTrace(Location, CameraPosition);

		if ( !bPostRenderTraceSucceeded )
		{
			if ( bShowLocked )
			{
				return;
			}
			bPostRenderTraceSucceeded = FastTrace(Location+GetCollisionHeight()*vect(0,0,1), CameraPosition);
			if ( !bPostRenderTraceSucceeded )
			{
				// now try top corners of vehicle, before giving up
				CrossDir = Normal(Vect(0,0,1) cross (Location - CameraPosition));
				bPostRenderTraceSucceeded = FastTrace(Location+GetCollisionHeight()*vect(0,0,1)+CylinderComponent.CollisionRadius*CrossDir, CameraPosition)
										|| FastTrace(Location+GetCollisionHeight()*vect(0,0,1)-CylinderComponent.CollisionRadius*CrossDir, CameraPosition);
			}
		}
	}
	if ( !bPostRenderTraceSucceeded )
	{
		return;
	}

	if ( bShowLocked )
	{
		// draw no entry indicator
		// don't draw it if there's another vehicle in front of the vehicle
		HitActor = Trace(HitLocation, HitNormal, Location, CameraPosition, true,,,TRACEFLAG_Blocking);
		if ( UTVehicle(HitActor) != None )
		{
			return;
		}

		// draw locked symbol
		xscale = FClamp( (2*TeamBeaconPlayerInfoMaxDist - Dist)/(2*TeamBeaconPlayerInfoMaxDist), 0.55f, 1.f);
		xscale = xscale * xscale;
		Canvas.SetPos(ScreenLoc.X-16*xscale,ScreenLoc.Y-16*xscale);
		Canvas.DrawColor = class'UTHUD'.Default.WhiteColor;
		Canvas.DrawColorizedTile(class'UTHUD'.Default.HudTexture,32*xscale, 32*xscale,193,357,30,30, MakeLinearColor( 4.0, 2.0, 0.5, 1.0) );
		return;
	}

	if ( Dist > TeamBeaconPlayerInfoMaxDist )
	{
		HealthY = 8 * Canvas.ClipX/1024;
	}
	else if ( PlayerReplicationInfo == None )
	{
		HealthY = 8 * Canvas.ClipX/1024 * (1 + 2*Square((TeamBeaconPlayerInfoMaxDist-Dist)/TeamBeaconPlayerInfoMaxDist));
	}
	else
	{
		HealthY = 16 * Canvas.ClipX/1024 * (1 + 2*Square((TeamBeaconPlayerInfoMaxDist-Dist)/TeamBeaconPlayerInfoMaxDist));
	}

	class'UTHUD'.Static.GetTeamColor( GetTeamNum(), TeamColor, TextColor);

	if ( Dist > TeamBeaconPlayerInfoMaxDist )
	{
		XL = 2 * HealthY;
	}
	else if ( PlayerReplicationInfo != None )
	{
		ScreenName = PlayerReplicationInfo.PlayerName;

		Canvas.StrLen(ScreenName, TextXL, YL);
		XL = Max( TextXL, 2 * HealthY);
		HealthY *= 0.5;
	}
	else if ( !bCanCarryFlag && (UTPlayerReplicationInfo(PC.PlayerReplicationInfo).GetFlag() != None) )
	{
		// @todo FIXMESTEVE - need icon instead
		ScreenName = "NO FLAG";
		Canvas.StrLen(ScreenName, TextXL, YL);
		XL = Max( TextXL, 2 * HealthY);
		HealthY *= 0.5;
	}
	else
	{
		XL = 2 * HealthY;
	}

	// customize beacon if this vehicle could be entered
	if ( (UTPawn(PC.Pawn) != None) || (UTVehicle_Hoverboard(PC.Pawn) != None) )
	{
		// @TODO FIXMESTEVE - optimize radius of this check, to minimize unnecessary work
		EntryCheckDist = CylinderComponent.CollisionRadius + CylinderComponent.CollisionHeight + PC.Pawn.VehicleCheckRadius * UTPlayerController(PC).VehicleCheckRadiusScaling;
		if ( (Dist < EntryCheckDist) && CanEnterVehicle(PC.Pawn) && (UTPlayerController(PC).CheckVehicleToDrive(false) == self) )
		{
			// programmer art - just color it greenish
			bShowUseable = true;
			TeamColor.R = 0.5;
			TeamColor.G = 1.0;
			TeamColor.B = 0.0;
			if ( UTConsolePlayerController(PC) != None )
				ScreenName = "PRESS 'X' TO ENTER";
			else
				ScreenName = "PRESS 'E' TO ENTER";
			Canvas.StrLen(ScreenName, TextXL, YL);
			XL = Max( TextXL, 2 * HealthY);
			HealthY *= 0.5;
			ScreenLoc = Canvas.Project(Location);
			ScreenLoc.X = 0.5 * Canvas.ClipX;
		}
	}
	Class'UTHUD'.static.DrawBackground(ScreenLoc.X-0.7*XL,ScreenLoc.Y-1.8*YL-1.8*HealthY,1.4*XL,1.8*YL+1.8*HealthY, TeamColor, Canvas);

	if ( ScreenName != "" )
	{
		Canvas.DrawColor = TextColor;
		Canvas.SetPos(ScreenLoc.X-0.5*TextXL,ScreenLoc.Y-1.2*YL-1.4*HealthY);
		Canvas.DrawTextClipped(ScreenName, true);
	}

	HealthX = XL * FMin(1.0, GetDisplayedHealth()/float(Default.Health));

	if ( (PlayerReplicationInfo != None) && (Dist < TeamBeaconPlayerInfoMaxDist) )
	{
		Class'UTHUD'.static.DrawHealth(ScreenLoc.X-0.5*XL,ScreenLoc.Y-0.1*YL-1.8*HealthY, HealthX, XL, HealthY, Canvas);
	}
	else
	{
		Class'UTHUD'.static.DrawHealth(ScreenLoc.X-0.5*XL,ScreenLoc.Y-0.1*YL-1.4*HealthY, HealthX, XL, HealthY, Canvas);
	}
	if ( Dist < TeamBeaconPlayerInfoMaxDist )
	{
		RenderPassengerBeacons(PC, Canvas, TeamColor, TextColor, Weap);
	}
	if ( !bShowUseable )
	{
		HUD = UTHUD(PC.MyHUD);
		if ( (HUD != None) && !HUD.bCrosshairOnFriendly )
		{
			MaxOffset = HUDExtent/Dist;
			if ( (Abs(screenLoc.X - 0.5*Canvas.ClipX) < MaxOffset * Canvas.ClipX)
			&& (Abs(screenLoc.Y - 0.5*Canvas.ClipY) < MaxOffset * Canvas.ClipY) )
			{
				HUD.bCrosshairOnFriendly = true;
			}
		}
	}
}

simulated function float GetDisplayedHealth()
{
	return Health;
}

simulated function RenderPassengerBeacons(PlayerController PC, Canvas Canvas, LinearColor TeamColor, Color TextColor, UTWeapon Weap)
{
	if ( PassengerPRI != None )
	{
		PostRenderPassengerBeacon(PC, Canvas, TeamColor, TextColor, Weap, PassengerPRI, PassengerTeamBeaconOffset);
	}
}

/**
 * PostRenderPassengerBeacon() renders
 * Assumes that appropriate font has already been set
 * FIXMESTEVE - special beacon when speaking (SpeakingBeaconTexture)
 *
 * @param	PC		The Player Controller who is rendering this pawn
 * @param	Canvas	The canvas to draw on
 */
simulated function PostRenderPassengerBeacon(PlayerController PC, Canvas Canvas, LinearColor TeamColor, Color TextColor, UTWeapon Weap, UTPlayerReplicationInfo InPassengerPRI, vector InPassengerTeamBeaconOffset)
{
	local float TextXL, XL, YL;
	local vector ScreenLoc, X, Y,Z;

	GetAxes(Rotation, X, Y, Z);
	screenLoc = Canvas.Project(Location + InPassengerTeamBeaconOffset.X * X + InPassengerTeamBeaconOffset.Y * Y + InPassengerTeamBeaconOffset.Z * Z);

	// make sure not clipped out
	if (screenLoc.X < 0 ||
		screenLoc.X >= Canvas.ClipX ||
		screenLoc.Y < 0 ||
		screenLoc.Y >= Canvas.ClipY)
	{
		return;
	}

	// make sure not behind weapon
	if ( (Weap != None) && Weap.CoversScreenSpace(screenLoc, Canvas) )
	{
		return;
	}

	Canvas.StrLen(InPassengerPRI.PlayerName, TextXL, YL);
	XL = FMax( TextXL, 64.0 * Canvas.ClipX/1024);

	Class'UTHUD'.static.DrawBackground(ScreenLoc.X-0.7*XL,ScreenLoc.Y-1.8*YL,1.4*XL,1.8*YL, TeamColor, Canvas);

	Canvas.DrawColor = TextColor;
	Canvas.SetPos(ScreenLoc.X-0.5*TextXL,ScreenLoc.Y-1.4*YL);
	Canvas.DrawTextClipped(InPassengerPRI.PlayerName, true);
}

/**
 * Team is changed when vehicle is possessed
 */
event SetTeamNum(byte T)
{
	if ( T != Team )
	{
		Team = T;
		TeamChanged();
	}
}

/**
 * This function is called when the team has changed.  Use it to setup team specific overlays/etc
 */
simulated function TeamChanged()
{
	local MaterialInterface NewMaterial;

	if (TeamMaterials[1] != none && TeamMaterials[0] != none)
	{
		NewMaterial = TeamMaterials[(Team == 1) ? 1 : 0];
		if (DamageMaterialInstance != None)
		{
			DamageMaterialInstance.SetParent(NewMaterial);
		}
		else
		{
			Mesh.SetMaterial(0, NewMaterial);
			if(DestroyedTurret != none)
			{
				DestroyedTurret.SMMesh.SetMaterial(0, NewMaterial);
			}
		}
	}
	UpdateDamageMaterial();

}

/**
 * Stub out the Dodge event.  Override if the vehicle needs a dodge
 *
 * See Pawn::Dodge()
 */
function bool Dodge(eDoubleClickDir DoubleClickMove);

/**
 * This function is called from an incoming missile that is targetting this vehicle
 *
 * @param P		The incoming projectile
 */
function IncomingMissile(Projectile P)
{
	local AIController C;

	C = AIController(Controller);
	if (C != None && C.Skill >= 5.0 && (C.Enemy == None || !C.LineOfSightTo(C.Enemy)))
	{
		ShootMissile(P);
	}
}

/**
 * AI hint - Shoot at the missle
 *
 * @param P		The incoming projectile
 */

function ShootMissile(Projectile P)
{
	Controller.Focus = P;
	Controller.FireWeaponAt(P);
}

/**
 * sends the LockOn message to all seats in this vehicle with the specified switch
 *
 * @param Switch 	The message switch
 */
simulated function SendLockOnMessage(int Switch)
{
	local int i;

	for (i = 0; i < Seats.length; i++)
	{
		if ( (PlayerController(Seats[i].SeatPawn.Controller) != None) && Seats[i].SeatPawn.IsLocallyControlled() )
		{
			PlayerController(Seats[i].SeatPawn.Controller).ReceiveLocalizedMessage(class'UTLockWarningMessage', Switch);
			PlayerController(Seats[i].SeatPawn.Controller).ClientPlaySound(LockedOnSound);
		}
	}
}

/**
 *  LockOnWarning() called by seeking missiles to warn vehicle they are incoming
 */
simulated function LockOnWarning(UTProjectile IncomingMissile)
{
	SendLockOnMessage(1);
}

/**
 * Check to see if Other is too close to attack
 *
 * @param	Other		Actor to check against
 * @returns true if he's too close
 */

function bool TooCloseToAttack(Actor Other)
{
	return false;
/*
@TODO FIXMESTEVE
	if (VSize(Location - Other.Location) > 2500)
		return false;

	if ( !HasRangedAttack() )
	{
		if (WeaponPawns.length == 0)
			return false;
		for (i = 0; i < WeaponPawns.length; i++)
			if (WeaponPawns[i].Controller != None)
			{
				bControlledWeaponPawn = true;
				if (!WeaponPawns[i].TooCloseToAttack(Other))
					return false;
			}

		if (!bControlledWeaponPawn)
			return false;

		return true;
	}



	Weapons[ActiveWeapon].CalcWeaponFire();
	NeededPitch = rotator(Other.Location - Weapons[ActiveWeapon].WeaponFireLocation).Pitch;
	NeededPitch = NeededPitch & 65535;
	return (LimitPitch(NeededPitch) != NeededPitch);
*/
}

/**
 * Play the horn for this vehicle
 *
 * @param 	HornIndex	guess :)
 */

function PlayHorn(int HornIndex)
{
	local int i, NumPositions;
	local Pawn P;
	local UTPawn UTP;
	local UTVehicle V;

	if (WorldInfo.TimeSeconds - LastHornTime > 3.0 && HornIndex >= 0 && HornIndex < HornSounds.Length)
	{
		PlaySound(HornSounds[HornIndex]);
		LastHornTime = WorldInfo.TimeSeconds;
	}

	if (PlayerController(Controller) != None)
	{
		// if a seat is available, nearby friendly bots respond to horn and try to get in
		for (i = 0; i < Seats.length; i++)
		{
			if (Seats[i].SeatPawn.Controller == None)
			{
				NumPositions++;
			}
		}
		if (NumPositions > 0)
		{
			foreach VisibleCollidingActors(class'Pawn', P, HornAIRadius)
			{
				if (UTBot(P.Controller) != None && WorldInfo.GRI.OnSameTeam(P, self))
				{
					V = UTVehicle(P);
					if (V != None)
					{
						// if bot is on a hoverboard, get off it to respond to horn
						UTP = UTPawn(V.Driver);
						if (UTP != None && UTP.HoverboardClass == V.Class && V.DriverLeave(false))
						{
							P = UTP;
						}
					}
					if (Vehicle(P) == None)
					{
						UTBot(P.Controller).SetTemporaryOrders('Follow', Controller);
						if (--NumPositions == 0)
						{
							break;
						}
					}
				}
			}
		}
	}
}

/**
 * UpdateControllerOnPossess() override Pawn.UpdateControllerOnPossess() to keep from changing controller's rotation
 *
 * @param	bVehicleTransition	Will be true if this the pawn is entering/leaving a vehicle
 */
function UpdateControllerOnPossess(bool bVehicleTransition);

/**
 * @returns the number of passengers in this vehicle
 */
simulated function int NumPassengers()
{
	local int i, Num;

	for (i = 0; i < Seats.length; i++)
	{
		if (Seats[i].SeatPawn.Controller != None)
		{
			Num++;
		}
	}

	return Num;
}

/**
 * AI Hint
 */
function UTVehicle GetMoveTargetFor(Pawn P)
{
	return self;
}

/** handles dealing with any flag the given driver/passenger may be holding */
function HandleEnteringFlag(UTPlayerReplicationInfo EnteringPRI)
{
	local UTPlayerController PC;

	if (EnteringPRI.bHasFlag)
	{
		if (!bCanCarryFlag)
		{
			PC = UTPlayerController(Controller);
			if (PC != None)
			{
				PC.ReceiveLocalizedMessage(class'UTVehicleCantCarryFlagMessage');
			}
			EnteringPRI.GetFlag().Drop();
		}
		else if (!bDriverHoldsFlag)
		{
			AttachFlag(EnteringPRI.GetFlag(), Driver);
		}
	}
}

/**
 * Called when a pawn enters the vehicle
 *
 * @Param P		The Pawn entering the vehicle
 */
function bool DriverEnter(Pawn P)
{
	P.StopFiring();

	if (Seats[0].Gun != none)
	{
		InvManager.SetCurrentWeapon(Seats[0].Gun);
	}

	Instigator = self;

	if ( !Super.DriverEnter(P) )
		return false;

	HandleEnteringFlag(UTPlayerReplicationInfo(PlayerReplicationInfo));

	SetSeatStoragePawn(0,P);
//	Seats[0].StoragePawn = P;

	if (ParentFactory != None)
	{
		ParentFactory.TriggerEventClass(class'UTSeqEvent_VehicleFactory', None, 3);
	}

	if ( PlayerController(Controller) != None )
	{
		VehicleLostTime = 0;
	}
	StuckCount = 0;
	ResetTime = WorldInfo.TimeSeconds - 1;
	bHasBeenDriven = true;
	return true;
}

/**
 * HoldGameObject() Attach GameObject to mesh.
 * @param 	GameObj 	Game object to hold
 */
simulated event HoldGameObject(UTCarriedObject GameObj)
{
	local UTPawn P;

	GameObj.SetHardAttach(GameObj.default.bHardAttach);
	GameObj.bIgnoreBaseRotation = GameObj.default.bIgnoreBaseRotation;

	if (bDriverHoldsFlag)
	{
		P = UTPawn(Driver);
		if (P != None)
		{
			P.HoldGameObject(GameObj);
		}
	}
	else
	{
		AttachFlag(GameObj, Driver);
	}
}

/**
 * If the driver enters the vehicle with a UTCarriedObject, this event
 * is triggered.
 *
 * @param	FlagActor		The object being carried
 * @param	NewDriver		The driver (may not yet have been set)
 */
simulated function AttachFlag(UTCarriedObject FlagActor, Pawn NewDriver)
{
	if ( FlagActor != None )
	{
		if ( FlagBone == '' )
		{
			FlagActor.BaseBoneName = '';
			FlagActor.BaseSkelComponent = None;
			FlagActor.SetBase(self);
		}
		else
		{
			FlagActor.SetBase(self,,Mesh,FlagBone);
		}
		FlagActor.SetRelativeRotation(FlagRotation);
		FlagActor.SetRelativeLocation(FlagOffset);
	}
}

// @TODO FIXMESTEVE temp remove
exec function FlagX(float F)
{
	FlagOffset.X = F;
	UTPlayerReplicationInfo(PlayerReplicationInfo).GetFlag().SetRelativeLocation(FlagOffset);
}

exec function FlagY(float F)
{
	FlagOffset.Y = F;
	UTPlayerReplicationInfo(PlayerReplicationInfo).GetFlag().SetRelativeLocation(FlagOffset);
}

exec function FlagZ(float F)
{
	FlagOffset.Z = F;
	UTPlayerReplicationInfo(PlayerReplicationInfo).GetFlag().SetRelativeLocation(FlagOffset);
}

/**
 * DriverLeft() called by DriverLeave() after the drive has been taken out of the vehicle
 */
simulated function DriverLeft()
{
	local float Dist;

	Super.DriverLeft();

	if ( (WorldInfo.NetMode == NM_Client) || bNeverReset || (ParentFactory == None) || Occupied() )
	{
		return;
	}

	Dist = VSize(Location - ParentFactory.Location);
	if (Dist >= 2000 && (Dist > 5000 || !FastTrace(ParentFactory.Location, Location)))
	{
		ResetTime = WorldInfo.TimeSeconds + (bKeyVehicle ? 15.0 : 30.0);
	}
	SetMovementEffect(0, false);
}

/**
 * @returns the first available passenger seat, or -1 if there are none available
 */
function int GetFirstAvailableSeat()
{
	local int i;

	for (i = 1; i < Seats.Length; i++)
	{
		if (SeatAvailable(i))
		{
			return i;
		}
	}

	return -1;
}

/**
 * Called when a passenger enters the vehicle
 *
 * @param P				The Pawn entering the vehicle
 * @param SeatIndex		The seat where he is to sit
 */

function bool PassengerEnter(Pawn P, int SeatIndex)
{
	// Restrict someone not on the same team
	if ( WorldInfo.Game.bTeamGame && !WorldInfo.GRI.OnSameTeam(P,self) )
	{
		return false;
	}

	if (SeatIndex <= 0 || SeatIndex >= Seats.Length)
	{
		`warn("Attempted to add a passenger to unavailable passenger seat" @ SeatIndex);
		return false;
	}

	if ( !Seats[SeatIndex].SeatPawn.DriverEnter(p) )
	{
		return false;
	}

	HandleEnteringFlag(UTPlayerReplicationInfo(Seats[SeatIndex].SeatPawn.PlayerReplicationInfo));

	SetSeatStoragePawn(SeatIndex,P);

	bHasBeenDriven = true;
	return true;
}

/**
 * Called when the driver leaves the vehicle
 *
 * @param	bForceLeave		Is true if the driver was forced out
 */

event bool DriverLeave(bool bForceLeave)
{
	local bool bResult;
	local Pawn OldDriver;
	if(!bForceLeave)
	{
		if(!bAllowedExit && ForceMovementDirection != Vect(0,0,0))
		{
			return false;
		}
	}

	OldDriver = Driver;
	bResult = Super.DriverLeave(bForceLeave);
	if (bResult)
	{
		SetSeatStoragePawn(0,None);
//		Seats[0].StoragePawn = None;
		// set Instigator to old driver so if vehicle continues on and runs someone over, the appropriate credit is given
		Instigator = OldDriver;
		if (ParentFactory != None)
		{
			ParentFactory.TriggerEventClass(class'UTSeqEvent_VehicleFactory', None, 4);
		}
	}

	return bResult;
}

/**
 * Called when a passenger leaves the vehicle
 *
 * @param	SeatIndex		Leaving from which seat
 */

function PassengerLeave(int SeatIndex)
{
	SetSeatStoragePawn(SeatIndex, None);
//	Seats[SeatIndex].StoragePawn = None;
}

/**
 *  Vehicle has been in the middle of nowhere with no driver for a while, so consider resetting it
 */
event CheckReset()
{
	local Pawn P;

	if ( bKeyVehicle && !Occupied() )
	{
		Died(None, class'DamageType', Location);
		return;
	}

	if ( Occupied() )
	{
		ResetTime = WorldInfo.TimeSeconds - 1;
		return;
	}

	foreach WorldInfo.AllPawns(class'Pawn', P)
	{
		if ( P != None && (P.Controller != None || (P.DrivenVehicle != None && !P.DrivenVehicle.bAttachDriver)) &&
			WorldInfo.GRI.OnSameTeam(P, self) && VSize(Location - P.Location) < 2500.0 &&
			FastTrace(P.Location + P.GetCollisionHeight() * vect(0,0,1), Location + GetCollisionHeight() * vect(0,0,1)) )
		{
			ResetTime = WorldInfo.TimeSeconds + 10;
			return;
		}
	}

	//if factory is active, we want it to spawn new vehicle NOW
	if ( ParentFactory != None )
	{
		ParentFactory.ChildVehicle = None;
		ParentFactory.SpawnVehicle();
		ParentFactory = None; //so doesn't call ParentFactory.VehicleDestroyed() again in Destroyed()
	}

	Destroy();
}

/**
 *  AI code
 */
function bool Occupied()
{
	local int i;

	if ( Controller != None )
		return true;

	for ( i=0; i<Seats.Length; i++ )
		if ( !SeatAvailable(i) )
			return true;

	return false;
}

/**
 * OpenPositionFor() returns true if there is a seat available for P
 *
 * @param P		The Pawn to test for
 * @returns true if open
 */
function bool OpenPositionFor(Pawn P)
{
	local int i;

	if ( UTGame(WorldInfo.Game).JustStarted(20) && (Reservation != None) && (Reservation != P.Controller) && (Reservation.RouteGoal == Self)
		&& (Reservation.Pawn != None) && (VSize(Reservation.Pawn.Location - Location) <= VSize(P.Location - Location)) )
	{
		for ( i=0; i<Seats.Length; i++ )
			if ( SeatAvailable(i) )
				return true;
		return false;
	}

	if ( Controller == None )
		return true;

	if ( !WorldInfo.GRI.OnSameTeam(Controller,P) )
		return false;

	for ( i=0; i<Seats.Length; i++ )
		if ( SeatAvailable(i) )
			return true;

	return false;
}

/**
 * @Returns the TeamIndex of this vehicle
 */

simulated function byte GetTeamNum()
{
	local byte Result;

	Result = Super.GetTeamNum();

	if ( Result == 255 )
		return Team;

	return Result;
}

/**
 * return a value indicating how useful this vehicle is to bots
 *
 * @param S				The Actor who desires this vehicle
 * @param TeamIndex		The Team index of S
 * @param Objective		The objective
 */

function float BotDesireability(Actor S, int TeamIndex, Actor Objective)
{
	local bool bSameTeam;
	local PlayerController P;

	bSameTeam = (Team == TeamIndex || Team == UTVEHICLE_UNSET_TEAM);
	if ( bSameTeam )
	{
		if ( !bKeyVehicle && (WorldInfo.TimeSeconds < PlayerStartTime) )
		{
			ForEach LocalPlayerControllers(class'PlayerController', P)
				break;
			if ( (P == None) || ((P.Pawn != None) && (Vehicle(P.Pawn) == None)) )
				return 0;
		}
	}
	if ( !bKeyVehicle && !bStationary && (WorldInfo.TimeSeconds < VehicleLostTime) )
		return 0;
	else if (Health <= 0 || Occupied() || (bTeamLocked && !bSameTeam))
	{
		return 0;
	}

	if (bKeyVehicle)
		return 100;

	return ((MaxDesireability * 0.5) + (MaxDesireability * 0.5 * (float(Health) / Default.Health)));
}

/**
 * AT Hint
 */

function float ReservationCostMultiplier(Pawn P)
{
	if ( Reservation == P.Controller )
		return 1.0;
	if ( UTGame(WorldInfo.Game).JustStarted(20) && (Reservation != None) && (Reservation.Pawn != None)
		&& (VSize(Reservation.Pawn.Location - Location) <= VSize(P.Location - Location)) )
	{
		return 0;
	}
	if ( (Reservation == None) || (Reservation.Pawn == None) )
		return 1.0;
	if ( (Reservation.MoveTarget == self) && Reservation.InLatentExecution(Reservation.LATENT_MOVETOWARD) )
		return 0;
	return 0.25;
}

/**
 * AI Hint
 */
function bool ChangedReservation(Pawn P)
{
	if ( UTGame(WorldInfo.Game).JustStarted(20) && (Reservation != None) && (Reservation != P.Controller) )
	{
		if ( (Reservation.RouteGoal == Self) && (Reservation.Pawn != None) && (VSize(Reservation.Pawn.Location - Location) <= VSize(P.Location - Location)) )
		{
			return true;
		}
		Reservation = UTBot(P.Controller);
		return false;
	}
	return false;
}

/**
 * AI Hint
 */

function bool SpokenFor(Controller C)
{
	local UTBot B;

	if ( Reservation == None )
		return false;
	if ( (Reservation.Pawn == None) || (Vehicle(Reservation.Pawn) != None) )
	{
		Reservation = None;
		return false;
	}
	if ( ((Reservation.RouteGoal != self) && (Reservation.MoveTarget != self))
		|| !Reservation.InLatentExecution(Reservation.LATENT_MOVETOWARD) )
	{
		Reservation = None;
		return false;
	}

	if ( !WorldInfo.GRI.OnSameTeam(Reservation,C) )
		return false;

	B = UTBot(C);
	if ( B == None )
		return true;
	if ( Seats.Length > 0 )
		return ( B.Squad != Reservation.Squad );
	if( UTGame(WorldInfo.Game).JustStarted(20) )
	{
		if ( VSize(Reservation.Pawn.Location - Location) > VSize(C.Pawn.Location - Location) )
			return false;
	}

	return true;
}

/* epic ===============================================
* ::StopsProjectile()
*
* returns true if Projectiles should call ProcessTouch() when they touch this actor
*/
simulated function bool StopsProjectile(Projectile P)
{
	// Don't block projectiles fired from this vehicle
	if ( P.Instigator == self )
		return false;

	// Don't block projectiles fired from turret on this vehicle
	return ( (P.Instigator == None) || (P.Instigator.Base != self) || !P.Instigator.IsA('UTWeaponPawn') );
}

/**
 * AI Hint
 */
function SetReservation(controller C)
{
	if ( !SpokenFor(C) )
		Reservation = UTBot(C);
}

/**
 * AI Hint
 */

function bool TeamLink(int TeamNum)
{
	return (LinkHealMult > 0 && Team == TeamNum && Health > 0);
}


/** called when the link gun hits an Actor that has this vehicle as its Owner
 * @param OwnedActor - the Actor owned by this vehicle that was hit
 * @return whether attempting to link to OwnedActor should be treated as linking to this vehicle
 */
simulated function bool AllowLinkThroughOwnedActor(Actor OwnedActor);

/**
 * This function is called to heal the vehicle
 * @See Actor.HealDamage()
 */

function bool HealDamage(int Amount, Controller Healer, class<DamageType> DamageType)
{
	if ( PlayerController(Healer) != None )
		PlayerStartTime = WorldInfo.TimeSeconds + 3;
	if (Health <= 0 || Health >= Default.Health || Amount <= 0 || Healer == None || !TeamLink(Healer.GetTeamNum()))
		return false;

	Amount = Min(Amount * LinkHealMult, Default.Health - Health);
	Health += Amount;
	NetUpdateTime = WorldInfo.TimeSeconds - 1;

	ApplyMorphHeal(Amount);

	//  Add time to the reset timer if you are healing it.
	if ( ResetTime - WorldInfo.TimeSeconds < 10.0 )
		ResetTime = WorldInfo.TimeSeconds+10.0;

	CheckDamageSmoke();

	return true;
}
/** function to call whenever a link gun links to this vehicle*/
function StartLinkedEffect()
{
	local MaterialInstanceConstant mic;
	local LinearColor Red, Blue;
	Red = MakeLinearColor(4,0.1,0,1);
	Blue = MakeLinearColor(0,1,4,1);
	if(Mesh.Materials.Length != 0)
	{
		MIC = MaterialInstanceConstant(Mesh.Materials[0]);
	}
	if(MIC == none)
	{
		if( Mesh != none)
		{
			if(Mesh.SkeletalMesh.Materials.Length != 0)
			{
				Mesh.SetMaterial(0,Mesh.SkeletalMesh.Materials[0]);
				MIC = MaterialInstanceConstant(Mesh.Materials[0]);
			}

		}
	}
	if(MIC != none)
	{
		if(Team == 1)
		{
			MIC.SetVectorParameterValue('Veh_OverlayColor',Blue);
		}
		else
		{
			MIC.SetVectorParameterValue('Veh_OverlayColor',Red);
		}
	}
	if(LinkedToAudio == none)
	{
		LinkedToAudio = CreateAudioComponent(LinkedToCue, false, true);
	}
	if(LinkedToAudio != none)
	{
		LinkedToAudio.FadeIn(0.2f,1.0f);
	}
}

/** function to call when a link gun unlinks */
function StopLinkedEffect()
{
	local MaterialInstanceConstant mic;
	local LinearColor Black;
	Black = MakeLinearColor(0,0,0,1);
	MIC = MaterialInstanceConstant(Mesh.Materials[0]);
	if(MIC != none)
	{
		MIC.SetVectorParameterValue('Veh_OverlayColor',Black);
	}
	PlaySound(LinkedEndSound);
	if(LinkedToAudio != none)
	{
		LinkedToAudio.FadeOut(0.1f,0.0f);
		LinkedToAudio = none;
	}
}

function PlayHit(float Damage, Controller InstigatedBy, vector HitLocation, class<DamageType> damageType, vector Momentum, TraceHitInfo HitInfo)
{
	local UTPlayerController Hearer;
	local class<UTDamageType> UTDamage;

	if (InstigatedBy != None && class<UTDamageType>(DamageType) != None && class<UTDamageType>(DamageType).default.bDirectDamage)
	{
		Hearer = UTPlayerController(InstigatedBy);
		if (Hearer != None)
		{
			Hearer.bAcuteHearing = true;
		}
	}
	//@todo FIXME: play vehicle hit sound here?
	Super.PlayHit(Damage, InstigatedBy, HitLocation, DamageType, Momentum, HitInfo);
	if (Hearer != None)
	{
		Hearer.bAcuteHearing = false;
	}

	if (Damage > 0 || ((Controller != None) && Controller.bGodMode))
	{
		CheckHitInfo( HitInfo, Mesh, Normal(Momentum), HitLocation );

		LastTakeHitInfo.Damage = Damage;
		LastTakeHitInfo.HitLocation = HitLocation;
		LastTakeHitInfo.Momentum = Momentum;
		LastTakeHitInfo.DamageType = DamageType;
		LastTakeHitInfo.HitBone = HitInfo.BoneName;
		UTDamage = class<UTDamageType>(DamageType);
		LastTakeHitTimeout = WorldInfo.TimeSeconds + ( (UTDamage != None) ? UTDamage.static.GetHitEffectDuration(self, Damage)
									: class'UTDamageType'.static.GetHitEffectDuration(self, Damage) );

		PlayTakeHitEffects();
	}
}

/** plays take hit effects; called from PlayHit() on server and whenever LastTakeHitInfo is received on the client */
simulated event PlayTakeHitEffects()
{
	local class<UTDamageType> UTDamage;

	if (EffectIsRelevant(Location, false))
	{
		UTDamage = class<UTDamageType>(LastTakeHitInfo.DamageType);
		if (UTDamage != None)
		{
			UTDamage.static.SpawnHitEffect(self, LastTakeHitInfo.Damage, LastTakeHitInfo.Momentum, LastTakeHitInfo.HitBone, LastTakeHitInfo.HitLocation);
		}
	}

	ApplyMorphDamage(LastTakeHitInfo.HitLocation, LastTakeHitInfo.Damage, LastTakeHitInfo.Momentum);
	ClientHealth -= LastTakeHitInfo.Damage;
}

function NotifyTakeHit(Controller InstigatedBy, vector HitLocation, int Damage, class<DamageType> damageType, vector Momentum)
{
	local int i;

	Super.NotifyTakeHit(InstigatedBy, HitLocation, Damage, DamageType, Momentum);

	// notify anyone in turrets
	for (i = 1; i < Seats.length; i++)
	{
		if (Seats[i].SeatPawn != None)
		{
			Seats[i].SeatPawn.NotifyTakeHit(InstigatedBy, HitLocation, Damage, DamageType, Momentum);
		}
	}
}

/**
 * @See Actor.TakeDamage()
 */
simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		if ( (UTPawn(Driver) != None) && UTPawn(Driver).bIsInvulnerable )
			Damage = 0;
		else if ( DamageType == class'UTDmgType_ScorpionSelfDestruct' )
			Damage = 610;
	}

	Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);

	if (Role == ROLE_Authority)
	{
		CheckDamageSmoke();
	}

	if(DamageType == class'UTDmgType_ImpactHammer')
	{
		PlaySound(ImpactHitSound);
	}
}

/**
 * GetHomingTarget is called from projectiles looking to seek to this vehicle.  It returns the actor the projectile should target
 *
 * @param	Seeker			The projectile that seeking this vehcile
 * @param	InstigatedBy	Who is controlling that projectile
 * @returns the target to see
 */
function Actor GetHomingTarget(UTProjectile Seeker, Controller InstigatedBy)
{
	return self;
}

/**
 * AI Hint
 */
function bool IsArtillery()
{
	return false;
}

function bool ImportantVehicle()
{
	return false;
}

/*********************************************************************************************
 * Vehicle Weapons, Drivers and Passengers
 *********************************************************************************************/

/**
 * Create all of the vehicle weapons
 */
function InitializeSeats()
{
	local int i;
	if (Seats.Length==0)
	{
		`log("WARNING: Vehicle ("$self$") **MUST** have at least one seat defined");
		destroy();
		return;
	}

	for(i=0;i<Seats.Length;i++)
	{
		// Seat 0 = Driver Seat.  It doesn't get a WeaponPawn

		if (i>0)
		{
	   		Seats[i].SeatPawn = Spawn(class'UTWeaponPawn');
	   		Seats[i].SeatPawn.SetBase(self);
			Seats[i].Gun = UTVehicleWeapon(Seats[i].SeatPawn.InvManager.CreateInventory(Seats[i].GunClass));
			Seats[i].Gun.SetBase(self);
			UTWeaponPawn(Seats[i].SeatPawn).MyVehicleWeapon = Seats[i].Gun;
			UTWeaponPawn(Seats[i].SeatPawn).MyVehicle = self;
	   		UTWeaponPawn(Seats[i].SeatPawn).MySeatIndex = i;

	   		if ( Seats[i].ViewPitchMin != 0.0f )
	   		{
				UTWeaponPawn(Seats[i].SeatPawn).ViewPitchMin = Seats[i].ViewPitchMin;
			}
			else
	   		{
				UTWeaponPawn(Seats[i].SeatPawn).ViewPitchMin = ViewPitchMin;
			}


	   		if ( Seats[i].ViewPitchMax != 0.0f )
	   		{
				UTWeaponPawn(Seats[i].SeatPawn).ViewPitchMax = Seats[i].ViewPitchMax;
			}
			else
	   		{
				UTWeaponPawn(Seats[i].SeatPawn).ViewPitchMax = ViewPitchMax;
			}
		}
		else
		{
			Seats[i].SeatPawn = self;
			Seats[i].Gun = UTVehicleWeapon(InvManager.CreateInventory(Seats[i].GunClass));
			Seats[i].Gun.SetBase(self);
		}

		Seats[i].SeatPawn.DriverDamageMult = Seats[i].DriverDamageMult;
		Seats[i].SeatPawn.bDriverIsVisible = Seats[i].bSeatVisible;

		if (Seats[i].Gun!=none)
		{
			Seats[i].Gun.SeatIndex = i;
			Seats[i].Gun.MyVehicle = self;
		}

		// Cache the names used to access various variables
   	}
}

simulated function PreCacheSeatNames()
{
	local int i;
	for (i=0;i<Seats.Length;i++)
	{
		Seats[i].WeaponRotationName	= NAME( Seats[i].TurretVarPrefix$"WeaponRotation" );
		Seats[i].FlashLocationName	= NAME( Seats[i].TurretVarPrefix$"FlashLocation" );
		Seats[i].FlashCountName		= NAME( Seats[i].TurretVarPrefix$"FlashCount" );
		Seats[i].FiringModeName		= NAME( Seats[i].TurretVarPrefix$"FiringMode" );
	}
}

simulated function InitializeTurrets()
{
	local int Seat, i;
	local UTSkelControl_TurretConstrained Turret;

	if (Mesh == None)
	{
		`warn("No Mesh for" @ self);
	}
	else
	{
		for (Seat = 0; Seat < Seats.Length; Seat++)
		{
			for (i = 0; i < Seats[Seat].TurretControls.Length; i++)
			{
				Turret = UTSkelControl_TurretConstrained( Mesh.FindSkelControl(Seats[Seat].TurretControls[i]) );
				if ( Turret != none )
				{
					Turret.AssociatedSeatIndex = Seat;
					Seats[Seat].TurretControllers[i] = Turret;
				}
				else
				{
					`warn("Failed to find skeletal controller named" @ Seats[Seat].TurretControls[i] @ "(Seat "$Seat$") for" @ self @ "in AnimTree" @ Mesh.AnimTreeTemplate);
				}
			}
		}
	}
}

function PossessedBy(Controller C, bool bVehicleTransition)
{
	super.PossessedBy(C,bVehicleTransition);

	if (Seats[0].Gun!=none)
		Seats[0].Gun.ClientWeaponSet(false);
}

simulated function SetFiringMode(byte FiringModeNum)
{
	SeatFiringMode(0, FiringModeNum, false);
}

simulated function ClearFlashCount(Weapon Who)
{
	local UTVehicleWeapon VWeap;

	VWeap = UTVehicleWeapon(Who);
	if (VWeap != none)
	{
		VehicleAdjustFlashCount(VWeap.SeatIndex, SeatFiringMode(VWeap.SeatIndex,,true), true);
	}

}

simulated function IncrementFlashCount(Weapon Who, byte FireModeNum)
{
	local UTVehicleWeapon VWeap;

	VWeap = UTVehicleWeapon(Who);
	if (VWeap != none)
	{
		VehicleAdjustFlashCount(VWeap.SeatIndex, FireModeNum, false);
	}
}

function SetFlashLocation( Weapon Who, byte FireModeNum, vector NewLoc )
{
	local UTVehicleWeapon VWeap;

	VWeap = UTVehicleWeapon(Who);
	if (VWeap != none)
	{
		VehicleAdjustFlashLocation(VWeap.SeatIndex, FireModeNum, NewLoc,  false);
	}
}

/**
 * Reset flash location variable. and call stop firing.
 * Network: Server only
 */
function ClearFlashLocation( Weapon Who )
{
	local UTVehicleWeapon VWeap;

	VWeap = UTVehicleWeapon(Who);
	if (VWeap != none)
	{
		VehicleAdjustFlashLocation(VWeap.SeatIndex, SeatFiringMode(VWeap.SeatIndex,,true), Vect(0,0,0),  true);
	}
}

simulated event GetBarrelLocationAndRotation(int SeatIndex, out vector SocketLocation, optional out rotator SocketRotation)
{
	if (Seats[SeatIndex].GunSocket.Length>0)
	{
		Mesh.GetSocketWorldLocationAndRotation(Seats[SeatIndex].GunSocket[GetBarrelIndex(SeatIndex)], SocketLocation, SocketRotation);
	}
	else
	{
		SocketLocation = Location;
		SocketRotation = Rotation;
	}
}

simulated function vector GetEffectLocation(int SeatIndex)
{
	local vector SocketLocation;

	if ( Seats[SeatIndex].GunSocket.Length == 0 )
		return Location;

	GetBarrelLocationAndRotation(SeatIndex,SocketLocation);
	return SocketLocation;
}

simulated function Vector GetPhysicalFireStartLoc(UTWeapon ForWeapon)
{
	local UTVehicleWeapon VWeap;

	VWeap = UTVehicleWeapon(ForWeapon);
	if ( VWeap != none )
	{
		return GetEffectLocation(VWeap.SeatIndex);
	}
	else
		return location;
}




/**
 * This function returns the aim for the weapon
 */
function rotator GetWeaponAim(UTVehicleWeapon VWeapon)
{
	local vector SocketLocation, CameraLocation, RealAimPoint, DesiredAimPoint, HitLocation, HitRotation, DirA, DirB;
	local rotator CameraRotation, SocketRotation, ControllerAim, AdjustedAim;
	local float DiffAngle;
	local Controller C;
	local PlayerController PC;
	local Quat Q;

	if ( VWeapon != none )
	{
		C = Seats[VWeapon.SeatIndex].SeatPawn.Controller;

		PC = PlayerController(C);
		if (PC != None)
		{
			PC.GetPlayerViewPoint(CameraLocation, CameraRotation);
			DesiredAimPoint = CameraLocation + Vector(CameraRotation) * VWeapon.GetTraceRange();
			if (Trace(HitLocation, HitRotation, DesiredAimPoint, CameraLocation) != None)
			{
				DesiredAimPoint = HitLocation;
			}
		}
		else if (C != None)
		{
			DesiredAimPoint = C.FocalPoint;
		}

		if ( Seats[VWeapon.SeatIndex].GunSocket.Length>0 )
		{
			GetBarrelLocationAndRotation(VWeapon.SeatIndex, SocketLocation, SocketRotation);
			if(VWeapon.bIgnoreSocketPitchRotation || ((DesiredAimPoint.Z - Location.Z)<0 && VWeapon.bIgnoreDownwardPitch))
			{
				SocketRotation.Pitch = Rotator(DesiredAimPoint - Location).Pitch;
			}
		}
		else
		{
			SocketLocation = Location;
			SocketRotation = Rotator(DesiredAimPoint - Location);
		}

		RealAimPoint = SocketLocation + Vector(SocketRotation) * VWeapon.GetTraceRange();
		DirA = normal(DesiredAimPoint - SocketLocation);
		DirB = normal(RealAimPoint - SocketLocation);
		DiffAngle = ( DirA dot DirB );

		if ( DiffAngle >= VWeapon.GetMaxFinalAimAdjustment() )
		{
			// bit of a hack here to make bot aiming and single player autoaim work
			ControllerAim = C.Rotation;
			AdjustedAim = VWeapon.GetAdjustedAim(SocketLocation);
			if (AdjustedAim == VWeapon.Instigator.GetBaseAimRotation() || AdjustedAim == ControllerAim)
			{
				// no adjustment
				return rotator(DesiredAimPoint - SocketLocation);
			}
			else
			{
				// FIXME: AdjustedAim.Pitch = Instigator.LimitPitch(AdjustedAim.Pitch);
				return AdjustedAim;
			}
		}
		else
		{
			Q = QuatFromAxisAndAngle(Normal(DirB cross DirA), ACos(VWeapon.GetMaxFinalAimAdjustment()));
			return Rotator( QuatRotateVector(Q,DirB));
		}
	}
	else
	{
		return Rotation;
	}
}

/** Gives the vehicle an opportunity to override the functionality of the given fire mode, called on both the owning client and the server
	@return false to allow the vehicle weapon to use its behavior, true to override it */
simulated function bool OverrideBeginFire(byte FireModeNum);
simulated function bool OverrideEndFire(byte FireModeNum);

/**
 * GetWeaponViewAxes should be subclassed to support returningthe rotator of the various weapon points.
 */
simulated function GetWeaponViewAxes( UTWeapon WhichWeapon, out vector xaxis, out vector yaxis, out vector zaxis )
{
	GetAxes( Controller.Rotation, xaxis, yaxis, zaxis );
}

/**
 * Causes the muzzle flashlight to turn on and setup a time to
 * turn it back off again.
 */
simulated function CauseMuzzleFlashLight(int SeatIndex)
{
	// must have valid gunsocket
	if (Seats[SeatIndex].GunSocket.Length == 0 || bDeadVehicle)
		return;

	// only enable muzzleflash light if performance is high enough
	if ( !WorldInfo.bDropDetail || (Seats[SeatIndex].SeatPawn != None && PlayerController(Seats[SeatIndex].SeatPawn.Controller) != None && Seats[SeatIndex].SeatPawn.IsLocallyControlled()) )
	{
		if ( Seats[SeatIndex].MuzzleFlashLight == None )
		{
			if ( Seats[SeatIndex].MuzzleFlashLightClass != None )
			{
				Seats[SeatIndex].MuzzleFlashLight = new(Outer) Seats[SeatIndex].MuzzleFlashLightClass;
			}
		}
		else
		{
			Seats[SeatIndex].MuzzleFlashLight.ResetLight();
		}

		// FIXMESTEVE: OFFSET!

		if ( Seats[SeatIndex].MuzzleFlashLight != none )
		{
			Mesh.DetachComponent(Seats[SeatIndex].MuzzleFlashLight);
			Mesh.AttachComponentToSocket(Seats[SeatIndex].MuzzleFlashLight, Seats[SeatIndex].GunSocket[GetBarrelIndex(SeatIndex)]);
		}
	}
}

simulated function WeaponFired( bool bViaReplication, optional vector HitLocation)
{
	VehicleWeaponFired(bViaReplication, HitLocation, 0);
}

/**
 * Vehicle will want to override WeaponFired and pass off the effects to the proper Seat
 */
simulated function VehicleWeaponFired( bool bViaReplication, vector HitLocation, int SeatIndex )
{
	// Trigger any vehicle Firing Effects
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		VehicleWeaponFireEffects(HitLocation, SeatIndex);

		if (Role == ROLE_Authority || bViaReplication)
		{
			VehicleWeaponImpactEffects(HitLocation, SeatIndex);
		}

		if (SeatIndex == 0)
		{
			Seats[SeatIndex].Gun = UTVehicleWeapon(Weapon);
		}
		if (Seats[SeatIndex].Gun != None)
		{
			Seats[SeatIndex].Gun.ShakeView();
		}
		if ( EffectIsRelevant(Location,false,MaxFireEffectDistance) )
		{
			CauseMuzzleFlashLight(SeatIndex);
		}
	}
}

simulated function WeaponStoppedFiring( bool bViaReplication )
{
	VehicleWeaponStoppedFiring(bViaReplication, 0);
}

simulated function VehicleWeaponStoppedFiring( bool bViaReplication, int SeatIndex )
{
	local name StopName;

	// Trigger any vehicle Firing Effects
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		if (Role == ROLE_Authority || bViaReplication)
		{
			StopName = Name( "STOP_"$Seats[SeatIndex].GunClass.static.GetFireTriggerTag( GetBarrelIndex(SeatIndex) , SeatFiringMode(SeatIndex,,true) ) );
			VehicleEvent( StopName );
		}
	}
}

/**
 * This function should be subclassed and manage the different effects
 */
simulated function VehicleWeaponFireEffects(vector HitLocation, int SeatIndex)
{
	VehicleEvent( Seats[SeatIndex].GunClass.static.GetFireTriggerTag( GetBarrelIndex(SeatIndex), SeatFiringMode(SeatIndex,,true) ) );
}

/**
 * This function is here so that children vehicles can get access to the retrace to get the hitnormal.  See the Dark Walker
 */

simulated function actor FindWeaponHitNormal(out vector HitLocation, out Vector HitNormal, vector End, vector Start, out TraceHitInfo HitInfo)
{
	return Trace(HitLocation, HitNormal, End, Start, true,, HitInfo, TRACEFLAG_Bullet);
}

/**
 * Spawn any effects that occur at the impact point.  It's called from the pawn.
 */
simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local vector NewHitLoc, HitNormal;
	local Actor HitActor;
	local TraceHitInfo HitInfo;
	local MaterialImpactEffect ImpactEffect;
	local Vehicle V;
	local Pawn EffectInstigator;
	local DecalLifetimeData LifetimeData;
	local UTPlayerController PC;
	local Emitter E;
	local UTExplosionLight ImpactLight;

	HitNormal = Normal(Location - HitLocation);
	HitActor = FindWeaponHitNormal(NewHitLoc, HitNormal, (HitLocation - (HitNormal * 32)), HitLocation + (HitNormal * 32),HitInfo);

	if (Pawn(HitActor) != None)
	{
		CheckHitInfo(HitInfo, Pawn(HitActor).Mesh, -HitNormal, NewHitLoc);
	}
	// figure out the impact effect to use
	ImpactEffect = Seats[SeatIndex].GunClass.static.GetImpactEffect(HitActor, HitInfo.PhysMaterial, SeatFiringMode(SeatIndex,, true));
	if (ImpactEffect.Sound != None)
	{
		// if hit a vehicle controlled by the local player, always play it full volume
		V = Vehicle(HitActor);
		if (V != None && V.IsLocallyControlled() && V.IsHumanControlled())
		{
			PlayerController(V.Controller).ClientPlaySound(ImpactEffect.Sound);
		}
		else
		{
			if ( Seats[SeatIndex].GunClass.default.BulletWhip != None )
			{
				ForEach LocalPlayerControllers(class'UTPlayerController', PC)
				{
					if (!WorldInfo.GRI.OnSameTeam(self, PC))
					{
						PC.CheckBulletWhip(Seats[SeatIndex].GunClass.default.BulletWhip, Location, Normal(HitLocation - Location), HitLocation);
					}
				}
			}
			PlaySound(ImpactEffect.Sound, true,,, HitLocation);
		}
	}

	EffectInstigator = Seats[SeatIndex].SeatPawn;
	if (EffectInstigator == None)
	{
		EffectInstigator = self;
	}
	if (EffectInstigator.EffectIsRelevant(HitLocation, false, MaxImpactEffectDistance))
	{
		// Pawns handle their own hit effects
		if (HitActor != None && (Pawn(HitActor) == None || Vehicle(HitActor) != None))
		{
			if (ImpactEffect.DecalMaterial != None)
			{
				// FIXME: decal lifespan based on detail level
				LifetimeData = new(HitActor.Outer) class'DecalLifetimeDataAge';
				HitActor.AddDecal( ImpactEffect.DecalMaterial, HitLocation, rotator(-HitNormal), 0,
							ImpactEffect.DecalWidth, ImpactEffect.DecalHeight, 10.0, false, HitInfo.HitComponent, LifetimeData, FALSE, FALSE, HitInfo.BoneName, HitInfo.Item, HitInfo.LevelIndex );
			}

			if (ImpactEffect.ParticleTemplate != None)
			{
				E = SpawnImpactEmitter(HitLocation, HitNormal, ImpactEffect);
				if ( (Seats[SeatIndex].ImpactFlashLightClass != None)
					&& (!WorldInfo.bDropDetail || (Seats[SeatIndex].SeatPawn != None && PlayerController(Seats[SeatIndex].SeatPawn.Controller) != None && Seats[SeatIndex].SeatPawn.IsLocallyControlled())) )
				{
					ImpactLight = new(Outer) Seats[SeatIndex].ImpactFlashLightClass;
					// translation is in emitter's local space
					ImpactLight.SetTranslation(0.5*ImpactLight.Radius*vect(1,0,0));
					E.AttachComponent(ImpactLight);
				}
			}
		}
	}
}

simulated function Emitter SpawnImpactEmitter(vector HitLocation, vector HitNormal, MaterialImpactEffect ImpactEffect)
{
	local Emitter E;

	E = Spawn(class'UTEmitter',,, HitLocation, rotator(HitNormal));
	E.SetTemplate(ImpactEffect.ParticleTemplate, true);
	return E;
}

/**
 * These two functions needs to be subclassed in each weapon
 */
simulated function VehicleAdjustFlashCount(int SeatIndex, byte FireModeNum, optional bool bClear)
{
	if (bClear)
	{
		SeatFlashCount( SeatIndex, 0 );
		VehicleWeaponStoppedFiring( false, SeatIndex );
	}
	else
	{
		SeatFiringMode(SeatIndex,FireModeNum);
		SeatFlashCount( SeatIndex, SeatFlashCount(SeatIndex,,true)+1 );
		VehicleWeaponFired( false, vect(0,0,0), SeatIndex );
		Seats[SeatIndex].BarrelIndex++;
	}

	NetUpdateTime	= WorldInfo.TimeSeconds - 1;	// Force replication
}

simulated function VehicleAdjustFlashLocation(int SeatIndex, byte FireModeNum, vector NewLocation, optional bool bClear)
{
	if (bClear)
	{
		SeatFlashLocation( SeatIndex, Vect(0,0,0) );
		VehicleWeaponStoppedFiring( false, SeatIndex );
	}
	else
	{
		// Make sure 2 consecutive flash locations are different, for replication
		if( NewLocation == SeatFlashLocation(SeatIndex,,true) )
		{
			NewLocation += vect(0,0,1);
		}

		// If we are aiming at the origin, aim slightly up since we use 0,0,0 to denote
		// not firing.
		if( NewLocation == vect(0,0,0) )
		{
			NewLocation = vect(0,0,1);
		}

		SeatFiringMode(SeatIndex,FireModeNum);
		SeatFlashLocation( SeatIndex, NewLocation );
		VehicleWeaponFired( false, NewLocation, SeatIndex );
		Seats[SeatIndex].BarrelIndex++;
	}


	NetUpdateTime	= WorldInfo.TimeSeconds - 1;	// Force replication
}

/** Used by PlayerController.FindGoodView() in RoundEnded State */
simulated function FindGoodEndView(PlayerController PC, out Rotator GoodRotation)
{
	local vector cameraLoc;
	local rotator cameraRot, ViewRotation;
	local int tries;
	local float bestdist, newdist, FOVAngle;

	ViewRotation = GoodRotation;
	ViewRotation.Pitch = 56000;
	tries = 0;
	bestdist = 0.0;
	for (tries=0; tries<16; tries++)
	{
		cameraLoc = Location;
		cameraRot = ViewRotation;
		CalcCamera( 0, cameraLoc, cameraRot, FOVAngle );
		newdist = VSize(cameraLoc - Location);
		if (newdist > bestdist)
		{
			bestdist = newdist;
			GoodRotation = cameraRot;
		}
		ViewRotation.Yaw += 4096;
	}
}

/**
 * We override CalcCamera so as to use the Camera Distance of the seat
 */
simulated function bool CalcCamera(float DeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV)
{
    local vector out_CamStart;

	VehicleCalcCamera(DeltaTime, 0, out_CamLoc, out_CamRot, out_CamStart);
	return true;
}

/**
  * returns the camera focus position (without camera lag)
  */
simulated function vector GetCameraFocus(int SeatIndex)
{
	local vector CamStart;

	//  calculate camera focus
	if ( !bDeadVehicle && Seats[SeatIndex].CameraTag != '' )
	{
		Mesh.GetSocketWorldLocationAndRotation(Seats[SeatIndex].CameraTag, CamStart);
	}
	else
	{
		CamStart = Location;
	}
	CamStart += (Seats[SeatIndex].CameraBaseOffset >> Rotation);
	//DrawDebugSphere(CamStart, 8, 10, 0, 255, 0, FALSE);
	//DrawDebugSphere(Location, 8, 10, 255, 255, 0, FALSE);
	return CamStart;
}

/**
  * returns the camera focus position (adjusted for camera lag)
  */
simulated function vector GetCameraStart(int SeatIndex)
{
	local int i, len, obsolete;
	local vector CamStart;
	local TimePosition NewPos, PrevPos;
	local float DeltaTime;

	// If we've already updated the cameraoffset, just return it
	len = OldPositions.Length;
	if (len > 0 && SeatIndex == 0 && OldPositions[len-1].Time == WorldInfo.TimeSeconds)
	{
		return CameraOffset + Location;
	}

	CamStart = GetCameraFocus(SeatIndex);
	if (CameraLag == 0 || SeatIndex != 0 || !IsHumanControlled())
	{
		return CamStart;
	}

	// cache our current location
	NewPos.Time = WorldInfo.TimeSeconds;
	NewPos.Position = CamStart;
	OldPositions[len] = NewPos;

	// if no old locations saved, return offset
	if ( len == 0 )
	{
		CameraOffset = CamStart - Location;
		return CamStart;
	}
	DeltaTime = (len > 1) ? (WorldInfo.TimeSeconds - OldPositions[len-2].Time) : 0.0;

	len = OldPositions.Length;
	obsolete = 0;
	for ( i=0; i<len; i++ )
	{
		if ( OldPositions[i].Time < WorldInfo.TimeSeconds - CameraLag )
		{
			PrevPos = OldPositions[i];
			obsolete++;
		}
		else
		{
			if ( Obsolete > 0 )
			{
				// linear interpolation to maintain same distance in past
				if ( (i == 0) || (OldPositions[i].Time - PrevPos.Time > 0.2) )
				{
					CamStart = OldPositions[i].Position;
				}
				else
				{
					CamStart = PrevPos.Position + (OldPositions[i].Position - PrevPos.Position)*(WorldInfo.TimeSeconds - CameraLag - PrevPos.Time)/(OldPositions[i].Time - PrevPos.Time);
				}
				if ( Obsolete > 1)
					OldPositions.Remove(0, obsolete-1);
			}
			else
			{
				CamStart = OldPositions[i].Position;
			}
			// need to smooth camera to vehicle distance, since vehicle update rate not synched with frame rate
			if ( DeltaTime > 0 )
			{
				DeltaTime *= CameraSmoothingFactor;
				CameraOffset = (CamStart - Location)*DeltaTime + CameraOffset*(1-DeltaTime);
				if ( bNoZSmoothing )
				{
					// don't smooth z - want it bouncy
					CameraOffset.Z = CamStart.Z - Location.Z;
				}
			}
			else
			{
				CameraOffset = CamStart - Location;
			}
			return CameraOffset + Location;
		}
	}
	return OldPositions[len-1].Position;
}

exec function FixedView(string VisibleMeshes)
{
	local int SeatIdx;
	local UTPawn P;

	// find a local player in one of the seats and pass call onto him
	for (SeatIdx=0; SeatIdx<Seats.length; ++SeatIdx)
	{
		P = UTPawn(Seats[SeatIdx].SeatPawn.Driver);
		if (P != None)
		{
			P.FixedView(VisibleMeshes);
		}
	}
}

simulated function VehicleCalcCamera(float DeltaTime, int SeatIndex, out vector out_CamLoc, out rotator out_CamRot, out vector CamStart, optional bool bPivotOnly)
{
	local vector CamPos, CamDir, HitLocation, FirstHitLocation, HitNormal, CamRotX, CamRotY, CamRotZ, SafeLocation, X, Y, Z;
	local actor HitActor;
	local float NewCamStartZ;
	local UTPawn P;
	local bool bObstructed, bInsideVehicle;

	Mesh.SetOwnerNoSee(false);
	if ( (UTPawn(Driver) != None) && !Driver.bHidden && Driver.Mesh.bOwnerNoSee )
		UTPawn(Driver).SetMeshVisibility(true);

	// Handle the fixed view
	P = UTPawn(Seats[SeatIndex].SeatPawn.Driver);
	if (P != None && P.bFixedView)
	{
		out_CamLoc = P.FixedViewLoc;
		out_CamRot = P.FixedViewRot;
		return;
	}

	CamStart = GetCameraStart(SeatIndex);

	Seats[SeatIndex].SeatPawn.EyeHeight = FMax(1,sqrt(SeatCameraScale))*Seats[SeatIndex].SeatPawn.BaseEyeheight;

	// Get the rotation
	if ( Seats[SeatIndex].SeatPawn.Controller != None )
		out_CamRot = Seats[SeatIndex].SeatPawn.GetViewRotation();

	// support debug 3rd person cam
	if (P != None)
	{
		P.ModifyRotForDebugFreeCam(out_CamRot);
	}

	GetAxes(out_CamRot, CamRotX, CamRotY, CamRotZ);
	CamStart += (Seats[SeatIndex].SeatPawn.EyeHeight + LookForwardDist * FMax(0,(Vector(Rotation) dot CamRotZ)))* CamRotZ;

	/* if bNoFollowJumpZ, Z component of Camera position is fixed during a jump */
	if ( bNoFollowJumpZ )
	{
		NewCamStartZ = CamStart.Z;
		if ( (Velocity.Z > 0) && !HasWheelsOnGround() && (OldCamPosZ != 0) )
		{
			// upward part of jump. Fix camera Z position.
			bFixedCamZ = true;
			if ( OldPositions.Length > 0 )
				OldPositions[OldPositions.Length-1].Position.Z += (OldCamPosZ - CamStart.Z);
			CamStart.Z = OldCamPosZ;
			if ( NewCamStartZ - CamStart.Z > 64 )
				CamStart.Z = NewCamStartZ - 64;
		}
		else if ( bFixedCamZ )
		{
			// Camera z position is being fixed, now descending
			if ( HasWheelsOnGround() || (CamStart.Z <= OldCamPosZ) )
			{
				// jump has ended
				if ( DeltaTime >= 0.1 )
				{
					// all done
					bFixedCamZ = false;
				}
				else
				{
					// Smoothly return to normal camera mode.
					CamStart.Z = 10*DeltaTime*CamStart.Z + (1 - 10*DeltaTime)*OldCamPosZ;
					if ( abs(NewCamStartZ - CamStart.Z) < 1.f )
						bFixedCamZ = false;
				}
			}
			else
			{
				// descending from jump, still in the air, so fix camera Z position
				if ( OldPositions.Length > 0 )
					OldPositions[OldPositions.Length-1].Position.Z += (OldCamPosZ - CamStart.Z);
				CamStart.Z = OldCamPosZ;
			}
		}
	}

	// Trace up to the view point to make sure it's not obstructed.
	if ( Seats[SeatIndex].CameraSafeOffset == vect(0,0,0) )
	{
		SafeLocation = Location;
	}
	else
	{
	    GetAxes(Rotation, X, Y, Z);
	    SafeLocation = Location + Seats[SeatIndex].CameraSafeOffset.X * X + Seats[SeatIndex].CameraSafeOffset.Y * Y + Seats[SeatIndex].CameraSafeOffset.Z * Z;
	}
	// DrawDebugSphere(SafeLocation, 16, 10, 255, 0, 255, FALSE);
	// DrawDebugSphere(CamStart, 16, 10, 255, 255, 0, FALSE);

	HitActor = Trace(HitLocation, HitNormal, CamStart, SafeLocation, false, vect(12, 12, 12));
	if ( HitActor != None)
	{
			bObstructed = true;
			CamStart = HitLocation;
			//`log("obstructed 0");
	}

	OldCamPosZ = CamStart.Z;
	if (bPivotOnly)
	{
		out_CamLoc = CamStart;
		return;
	}

	// Calculate the optimal camera position
	CamDir = CamRotX * Seats[SeatIndex].CameraOffset * SeatCameraScale;

	// keep camera from going below vehicle
	if ( !bRotateCameraUnderVehicle && (CamDir.Z < 0) )
	{
		CamDir *= (VSize(CamDir) - abs(CamDir.Z))/(VSize(CamDir) + abs(CamDir.Z));
	}

	CamPos = CamStart + CamDir;

	// Adjust for obstructions
	HitActor = Trace(HitLocation, HitNormal, CamPos, CamStart, false, vect(12, 12, 12));

	if ( HitActor != None )
	{
		out_CamLoc = HitLocation;
		bObstructed = true;
		//`log("obstructed 2");
	}
	else
	{
		out_CamLoc = CamPos;
	}
	if ( !bRotateCameraUnderVehicle && (CamDir.Z < 0) && TraceComponent( FirstHitLocation, HitNormal, CollisionComponent, out_CamLoc, CamStart, vect(0,0,0)) )
	{
		// going through vehicle - it's ok if outside collision on other side
		if ( !TraceComponent( HitLocation, HitNormal, CollisionComponent, CamStart, out_CamLoc, vect(0,0,0)) )
		{
			// end point is inside collision - that's bad
			out_CamLoc = FirstHitLocation;
			bObstructed = true;
			bInsideVehicle = true;
			//`log("obstructed 1");
		}
	}

	// if trace doesn't hit collisioncomponent going back in, it means we are inside the collision box
	// in which case we want to hide the vehicle
	if ( !bCameraNeverHidesVehicle && bObstructed )
	{
		bInsideVehicle = bInsideVehicle
						|| !TraceComponent( HitLocation, HitNormal, CollisionComponent, SafeLocation, out_CamLoc, vect(0,0,0))
						|| (VSizeSq(HitLocation - out_CamLoc) < MinCameraDistSq);
		Mesh.SetOwnerNoSee(bInsideVehicle);
		if ( (UTPawn(Driver) != None) && !Driver.bHidden && (Driver.Mesh.bOwnerNoSee != Mesh.bOwnerNoSee) )
		{
			// Handle the main player mesh
			Driver.Mesh.SetOwnerNoSee(Mesh.bOwnerNoSee);
		}
	}
}

/** moves the camera in or out */
simulated function AdjustCameraScale(bool bMoveCameraIn)
{
    if (!bUnlimitedCameraDistance)
	   SeatCameraScale = FClamp(SeatCameraScale + (bMoveCameraIn ? -0.1 : 0.1), CameraScaleMin, CameraScaleMax);
	else
	   SeatCameraScale = FMax(0.0, SeatCameraScale + (bMoveCameraIn ? -0.1 : 0.1));
}

simulated function StartBurnOut()
{
	if (SpawnOutSound != none)
	{
		PlaySound(SpawnOutSound, true);
	}
	bIsBurning = true;
	SetTimer(BurnOutTime - 0.5, false, 'DisableCollision');
}

/** turns off collision on the vehicle when it's almost fully burned out */
simulated function DisableCollision()
{
	SetCollision(false);
	Mesh.SetBlockRigidBody(false);
}

simulated function SetBurnOut()
{
	local int i, NumElements;

	if ( LifeSpan != 0 )
		return;

	LifeSpan = DeadVehicleLifeSpan;

	// set up material instance (for burnout effects)
	if (BurnOutMaterial[GetTeamNum()] != None)
	{
		Mesh.SetMaterial(0,BurnOutMaterial[GetTeamNum()]);
		if(DestroyedTurret != none)
		{
			DestroyedTurret.SMMesh.SetMaterial(0,BurnOutMaterial[GetTeamNum()]);
		}
	}
	NumElements = Mesh.GetNumElements();
	for (i = 0; i < NumElements; i++)
	{
		BurnOutMaterialInstances[i] = Mesh.CreateAndSetMaterialInstanceConstant(i);
		if(DestroyedTurret != none)
		{
			DestroyedTurret.SMMesh.SetMaterial(i,BurnOutMaterialInstances[i]);
		}
	}
	SetTimer(LifeSpan - BurnOutTime, false, 'StartBurnOut');
}

/** ShouldSpawnExplosionLight()
Decide whether or not to create an explosion light for this explosion
*/
simulated function bool ShouldSpawnExplosionLight(vector HitLocation, vector HitNormal)
{
	local PlayerController P;
	local float Dist;

	// decide whether to spawn explosion light
	ForEach LocalPlayerControllers(class'PlayerController', P)
	{
		Dist = VSize(P.ViewTarget.Location - Location);
		if ( (P.Pawn == Instigator) || (Dist < ExplosionLightClass.Default.Radius) || ((Dist < MaxExplosionLightDistance) && ((vector(P.Rotation) dot (Location - P.ViewTarget.Location)) > 0)) )
		{
			return true;
		}
	}
	return false;
}

simulated state DyingVehicle
{
	ignores Bump, HitWall, HeadVolumeChange, PhysicsVolumeChange, Falling, BreathTimer, FellOutOfWorld;

	simulated function PlayWeaponSwitch(Weapon OldWeapon, Weapon NewWeapon) {}
	simulated function PlayNextAnimation() {}
	singular event BaseChange() {}
	event Landed(vector HitNormal, Actor FloorActor) {}

	function bool Died(Controller Killer, class<DamageType> damageType, vector HitLocation);

	simulated function PostRenderFor(PlayerController PC, Canvas Canvas, vector CameraPosition, vector CameraDir) {}

	simulated function BlowupVehicle() {}

	/** spawn an explosion effect and damage nearby actors */
	simulated function InitialVehicleExplosion()
	{
		local Emitter ProjExplosion;
		local UTExplosionLight L;
		local UTPlayerController UTPC;
		local float Dist, ShakeScale;
		local ParticleSystem Template;

		if (BigExplosionTemplates.length > 0 && EffectIsRelevant(Location, false) )
		{
			Template = class'UTEmitter'.static.GetTemplateForDistance(BigExplosionTemplates, Location, WorldInfo);
			if (Template != None)
			{
				ProjExplosion = Spawn(class'UTEmitter', self);
				if (BigExplosionSocket != 'None')
				{
					ProjExplosion.SetBase(self,, Mesh, BigExplosionSocket);
				}
				ProjExplosion.SetTemplate(Template, true);
				ProjExplosion.ParticleSystemComponent.SetFloatParameter('Velocity', VSize(Velocity) / GroundSpeed);
				if (ExplosionLightClass != None && !WorldInfo.bDropDetail && ShouldSpawnExplosionLight(Location, vect(0,0,1)))
				{
					L = new(ProjExplosion) ExplosionLightClass;
					ProjExplosion.AttachComponent(L);
				}
			}
		}
		if (ExplosionSound != None)
		{
			PlaySound(ExplosionSound, true);
		}
		// viewshakes
		if (DeathExplosionShake != None)
		{
			foreach LocalPlayerControllers(class'UTPlayerController', UTPC)
			{
				Dist = VSize(Location - UTPC.ViewTarget.Location);
				if (UTPC == KillerController)
				{
					Dist *= 0.5;
				}
				if (Dist < OuterExplosionShakeRadius)
				{
					ShakeScale = 1.0;
					if (Dist > InnerExplosionShakeRadius)
					{
						ShakeScale -= (Dist - InnerExplosionShakeRadius) / (OuterExplosionShakeRadius - InnerExplosionShakeRadius);
					}
					UTPC.PlayCameraAnim(DeathExplosionShake, ShakeScale);
				}
			}
		}

		HurtRadius(ExplosionDamage, ExplosionRadius, class'UTDmgType_VehicleExplosion', ExplosionMomentum, Location,, GetCollisionDamageInstigator());
		AddVelocity((ExplosionMomentum / Mass) * vect(0,0,1), Location, class'UTDmgType_VehicleExplosion');
	}

	simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{
		if (DamageType != None)
		{
			Damage *= DamageType.static.VehicleDamageScalingFor(self);

			Health -= Damage;
			AddVelocity(Momentum, HitLocation, DamageType, HitInfo);

			if (DamageType == class'UTDmgType_VehicleCollision')
			{
				if ( EffectIsRelevant(Location, false) )
				{
					WorldInfo.MyEmitterPool.SpawnEmitter(ExplosionTemplate, HitLocation, rotator(vect(0,0,1)));
				}
				if (ExplosionSound != None)
				{
					PlaySound(ExplosionSound, true);
				}
			}
		}
	}

	/**
	*	Calculate camera view point, when viewing this pawn.
	*
	* @param	fDeltaTime	delta time seconds since last update
	* @param	out_CamLoc	Camera Location
	* @param	out_CamRot	Camera Rotation
	* @param	out_FOV		Field of View
	*
	* @return	true if Pawn should provide the camera point of view.
	*/
	simulated function VehicleCalcCamera(float fDeltaTime, int SeatIndex, out vector out_CamLoc, out rotator out_CamRot, out vector CamStart, optional bool bPivotOnly)
	{
 		Global.VehicleCalcCamera(fDeltaTime, SeatIndex, out_CamLoc, out_CamRot, CamStart, bPivotOnly);
		bStopDeathCamera = bStopDeathCamera || (out_CamLoc.Z < WorldInfo.KillZ);
		if ( bStopDeathCamera && (OldCameraPosition != vect(0,0,0)) )
		{
			// Don't allow camera to go below killz, by re-using old camera position once dead vehicle falls below killz
		   	out_CamLoc = OldCameraPosition;
			out_CamRot = rotator(CamStart - out_CamLoc);
		}
		OldCameraPosition = out_CamLoc;
	}

	simulated function BeginState(name PreviousStateName)
	{
		local int i;

		StopVehicleSounds();

		// fully destroy all morph targets
		for (i = 0; i < DamageMorphTargets.length; i++)
		{
			DamageMorphTargets[i].Health = 0;
			DamageMorphTargets[i].MorphNode.SetNodeWeight(1.0);
		}
		for(i=0; i<DamageSkelControls.length; i++)
		{
			DamageSkelControls[i].HealthPerc = 0.f;
		}
		UpdateDamageMaterial();
		ClientHealth = Min(ClientHealth, 0);

		LastCollisionSoundTime = WorldInfo.TimeSeconds;
		PerformDeathEffects();
		SetBurnOut();

		if (Controller != None)
		{
			if (Controller.bIsPlayer)
			{
				DetachFromController();
			}
			else
			{
				Controller.Destroy();
			}
		}

		for (i = 0; i < Attached.length; i++)
		{
			if (Attached[i] != None)
			{
				Attached[i].PawnBaseDied();
			}
		}
	}
	simulated function PerformDeathEffects()
	{
		InitialVehicleExplosion();
		if(bHasTurretExplosion)
		{
			TurretExplosion();
		}
		CustomDeathEffects();
	}


}

simulated function CustomDeathEffects();
simulated function TurretExplosion()
{
	local vector SpawnLoc;
	local rotator SpawnRot;
	local SkelControlBase SK;
	local vector Force;

	Mesh.GetSocketWorldLocationAndRotation(TurretSocketName,SpawnLoc,SpawnRot);

	WorldInfo.MyEmitterPool.SpawnEmitter( class'UTEmitter'.static.GetTemplateForDistance(DistanceTurretExplosionTemplates, SpawnLoc, WorldInfo),
						Location, Rotation );

	DestroyedTurret = Spawn(class'UTVehicleDeathPiece',self,,SpawnLoc+TurretOffset,SpawnRot,,true);
	if(DestroyedTurret != none)
	{
		DestroyedTurret.SMMesh.SetStaticMesh(DestroyedTurretTemplate);
		SK = Mesh.FindSkelControl(TurretScaleControlName);
		if(SK != none)
		{
			SK.boneScale = 0.f;
		}
		Force = Vect(0,0,1);
		Force *= TurretExplosiveForce;
		// Let's at least try and go off in some direction
		Force.X = FRand()*1000.0;
		Force.Y = FRand()*1000.0;
		DestroyedTurret.SMMesh.AddImpulse(Force);
		DestroyedTurret.SMMesh.SetRBAngularVelocity(VRand()*500.0);
	}
}

simulated function StopVehicleSounds()
{
	local int seatIdx;
	Super.StopVehicleSounds();
	for(seatIdx=0;seatIdx < Seats.Length; ++seatIdx)
	{
		if(Seats[seatIdx].SeatMotionAudio != none)
		{
			Seats[seatIdx].SeatMotionAudio.Stop();
		}
	}
}

simulated function CheckDamageSmoke()
{
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		VehicleEvent((float(Health) / float(default.Health) < DamageSmokeThreshold) ? 'DamageSmoke' : 'NoDamageSmoke');
	}
}

simulated function AttachDriver( Pawn P )
{
	local UTPawn UTP;

	// reset vehicle camera
	OldPositions.remove(0,OldPositions.Length);
	Eyeheight = BaseEyeheight;

	UTP = UTPawn(P);
	if (UTP != None)
	{
		UTP.SetWeaponAttachmentVisibility(false);
		UTP.SetHandIKEnabled(false);
		if (bAttachDriver)
		{
			UTP.SetCollision( false, false);
			UTP.bCollideWorld = false;
			UTP.SetBase(none);
			UTP.SetHardAttach(true);
			UTP.SetLocation( Location );
			UTP.SetPhysics( PHYS_None );

			SitDriver( UTP, 0);
		}
	}
}

simulated function SitDriver( UTPawn UTP, int SeatIndex)
{
	if (Seats[SeatIndex].SeatBone != '')
	{
		UTP.SetBase( Self, , Mesh, Seats[SeatIndex].SeatBone);
	}
	else
	{
		UTP.SetBase( Self );
	}
	SetMovementEffect(SeatIndex,true, UTP);

	if ( Seats[SeatIndex].bSeatVisible )
	{
		if ( (UTP.Mesh != None) && (Mesh != None) )
		{
			UTP.Mesh.SetShadowParent(Mesh);
			UTP.Mesh.SetLightEnvironment(LightEnvironment);
		}
		UTP.SetMeshVisibility(true);
		UTP.SetRelativeLocation( Seats[SeatIndex].SeatOffset );
		UTP.SetRelativeRotation( Seats[SeatIndex].SeatRotation );
		UTP.Mesh.SetCullDistance(5000);
		UTP.SetHidden(false);
	}
	else
	{
		UTP.SetHidden(True);
	}
}

simulated function DetachDriver(Pawn P)
{
	local UTPawn UTP;

	Super.DetachDriver(P);

	UTP = UTPawn(P);
	if (UTP != None)
	{
		UTP.SetWeaponAttachmentVisibility(true);
		UTP.SetHandIKEnabled(true);
		SetMovementEffect(0,false);
	}
}

/** Allows a vehicle to do specific physics setup on a driver when physics asset changes. */
simulated function OnDriverPhysicsAssetChanged(UTPawn UTP);

simulated function String GetHumanReadableName()
{
	if (VehicleNameString == "")
	{
		return ""$Class;
	}
	else
	{
		return VehicleNameString;
	}
}


event OnPropertyChange(name PropName)
{
	local int i;

	for (i=0;i<Seats.Length;i++)
	{
		if ( Seats[i].bSeatVisible )
		{
			Seats[i].StoragePawn.SetRelativeLocation( Seats[i].SeatOffset );
			Seats[i].StoragePawn.SetRelativeRotation( Seats[i].SeatRotation );
		}
	}
}

simulated function int GetHealth(int SeatIndex)
{
	return Health;
}

function float GetCollisionDamageModifier(const out array<RigidBodyContactInfo> ContactInfos)
{
	local float Angle;
	local vector X, Y, Z;

	if (bReducedFallingCollisionDamage)
	{
		GetAxes(Rotation, X, Y, Z);
		Angle = ContactInfos[0].ContactNormal Dot Z;
		return (Angle < 0.f) ? Square(CollisionDamageMult * (1.0+Angle)) : Square(CollisionDamageMult);
	}
	else
	{
		return Square(CollisionDamageMult);
	}
}

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
					const out CollisionImpactData Collision, int ContactIndex )
{
	local int Damage;

	if (LastCollisionDamageTime != WorldInfo.TimeSeconds)
	{
		Super.RigidBodyCollision(HitComponent, OtherComponent, Collision, ContactIndex);

		if (OtherComponent != None && UTPawn(OtherComponent.Owner) != None && OtherComponent.Owner.Physics == PHYS_RigidBody)
		{
			RanInto(OtherComponent.Owner);
		}
		else
		{
			// take impact damage
			Damage = int(VSizeSq(Mesh.GetRootBodyInstance().PreviousVelocity) * GetCollisionDamageModifier(Collision.ContactInfos));
			if (Damage > 1)
			{
				TakeDamage(Damage, GetCollisionDamageInstigator(), Collision.ContactInfos[0].ContactPosition, vect(0,0,0), class'UTDmgType_VehicleCollision');
				LastCollisionDamageTime = WorldInfo.TimeSeconds;
			}
		}
	}
}

/************************************************************************************
 * Morphs
 ***********************************************************************************/

/**
 * Initialize the damage modeling system
 */
simulated function InitializeMorphs()
{
	local int i,j;

	for (i=0;i<DamageMorphTargets.Length;i++)
	{
		// Find this node

		DamageMorphTargets[i].MorphNode 	  = MorphNodeWeight( Mesh.FindMorphNode(DamageMorphTargets[i].MorphNodeName) );
		if (DamageMorphTargets[i].MorphNode == None)
		{
			`Warn("Failed to find Morph node named" @ DamageMorphTargets[i].MorphNodeName @ "in" @ Mesh.SkeletalMesh);
		}

		// Fix up all linked references to this node.

		for (j=0;j<DamageMorphTargets.Length;j++)
		{
			if ( DamageMorphTargets[j].LinkedMorphNodeName == DamageMorphTargets[i].MorphNodeName )
			{
				DamageMorphTargets[j].LinkedMorphNodeIndex = i;
			}
		}
	}
	InitDamageSkel();

}

native simulated function InitDamageSkel();

/**
 * Whenever the morph system adjusts a health, it should call UpdateDamageMaterial() so that
 * any associated skins can be adjusted.  This is also native
 */
native simulated function UpdateDamageMaterial();

/**
 * When damage occur, we need to apply it to any MorphTargets.  We do this natively for speed
 *
 * @param	HitLocation		Where did the hit occured
 * @param	Damage			How much damage occured
 */
native function ApplyMorphDamage(vector HitLocation, int Damage, vector Momentum);

/** called when the client receives a change to Health
 * if LastTakeHitInfo changed in the same received bunch, always called *after* PlayTakeHitEffects()
 * (this is so we can use the damage info first for more accurate modelling and only use the direct health change for corrections)
 */
simulated event ReceivedHealthChange()
{
	local int Diff;

	Diff = Health - ClientHealth;
	if (Diff > 0)
	{
		ApplyMorphHeal(Diff);
	}
	else
	{
		ApplyRandomMorphDamage(Diff);
	}
	ClientHealth = Health;

	CheckDamageSmoke();
}

/**
 * Since vehicles can be healed, we need to apply the healing to each MorphTarget.  Since damage modeling is
 * client-side and healing is server-side, we evenly apply healing to all nodes
 *
 * @param	Amount		How much health to heal
 */
simulated event ApplyMorphHeal(int Amount)
{
	local int Individual, Total, Remaining;
	local int i;
	local float Weight;

	if (Health >= default.Health)
	{
		// fully heal everything
		for (i = 0; i < DamageMorphTargets.length; i++)
		{
			DamageMorphTargets[i].Health = default.DamageMorphTargets[i].Health;
			DamageMorphTargets[i].MorphNode.SetNodeWeight(0.0);
		}
	}
	else
	{
		// Find out the total amount of health needed for all nodes that have been "hurt"
		for ( i = 0; i < DamageMorphTargets.Length; i++)
		{
			if ( DamageMorphTargets[i].Health < Default.DamageMorphTargets[i].Health )
			{
				Total += Default.DamageMorphTargets[i].Health;
			}
		}

		// Deal out health evenly
		if (Amount > 0 && Total > 0)
		{
			Remaining = Amount;
			for (i = 0; i < DamageMorphTargets.length; i++)
			{
				Individual = Min( default.DamageMorphTargets[i].Health - DamageMorphTargets[i].Health,
						int(float(Amount) * float(Default.DamageMorphTargets[i].Health) / float(Total)) );
				DamageMorphTargets[i].Health += Individual;
				Remaining -= Individual;
			}

			// deal out any leftovers and update node weights
			for (i = 0; i < DamageMorphTargets.length; i++)
			{
				if (Remaining > 0)
				{
					Individual = Min(Remaining, default.DamageMorphTargets[i].Health - DamageMorphTargets[i].Health);
					DamageMorphTargets[i].Health += Individual;
					Remaining -= Individual;
				}
				Weight = 1.0 - (float(DamageMorphTargets[i].Health) / float(Default.DamageMorphTargets[i].Health));
				DamageMorphTargets[i].MorphNode.SetNodeWeight(Weight);
			}
		}
	}

	UpdateDamageMaterial();
}

/** called to apply morph damage where we don't know what was actually hit
 * (i.e. because the client detected it by receiving a new Health value from the server)
 */
simulated function ApplyRandomMorphDamage(int Amount)
{
	local int min, minindex;
	local int i;
	local float MinAmt;
	local float Weight;

	// Search for the skel control to damage (if any)
	for(i=0;i<DamageSkelControls.Length;i++)
	{
		if(DamageSkelControls[i].HealthPerc > 0 && minindex < 0)
		{
			MinAmt = FMin(Amount/(DamageSkelControls[i].DamageMax*DamageSkelControls.Length),DamageSkelControls[i].HealthPerc);
			DamageSkelControls[i].HealthPerc -= MinAmt;
		}
	}
	while (Amount > 0)
	{
		minindex = -1;

		// Search for the node with the least amount of health
		minindex=-1;
		for (i=0;i<DamageMorphTargets.Length;i++)
		{
			if ((DamageMorphTargets[i].Health > 0) && (minindex < 0 || DamageMorphTargets[i].Health < min))
			{
				min = DamageMorphTargets[i].Health;
				minindex = i;
			}
		}

		// Deal out damage to that node
		if (minindex>=0)
		{
			if (min < Amount)
			{
				DamageMorphTargets[minindex].Health = 0;
				Amount -= min;
			}
			else
			{
				DamageMorphTargets[minindex].Health -= Amount;
				Amount = 0;
			}

			// Adjust the target
			Weight = 1.0 - ( FLOAT(DamageMorphTargets[minindex].Health) / FLOAT(Default.DamageMorphTargets[minindex].Health) );
			DamageMorphTargets[minindex].MorphNode.SetNodeWeight(Weight);
		}
		else
		{
			break;
		}

	}

	UpdateDamageMaterial();
}

/**
 * The event is called from the native function ApplyMorphDamage when a node is destroyed (health <= 0).
 *
 * @param	MorphNodeIndex 		The Index of the node that was destroyed
 */
simulated event MorphTargetDestroyed(int MorphNodeIndex);

/**
 * We extend GetSVehicleDebug to include information about the seats array
 *
 * @param	DebugInfo		We return the text to display here
 */

simulated function GetSVehicleDebug( out Array<String> DebugInfo )
{
	local int i;

	Super.GetSVehicleDebug(DebugInfo);

	if (UTVehicleSimCar(SimObj) != None)
	{
		DebugInfo[DebugInfo.Length] = "ActualThrottle: "$UTVehicleSimCar(SimObj).ActualThrottle;
	}

	DebugInfo[DebugInfo.Length] = "";
	DebugInfo[DebugInfo.Length] = "----Seats----: ";
	for (i=0;i<Seats.Length;i++)
	{
		DebugInfo[DebugInfo.Length] = "Seat"@i$":"@Seats[i].Gun @ "Rotation" @ SeatWeaponRotation(i,,true) @ "FiringMode" @ SeatFiringMode(i,,true) @ "Barrel" @ Seats[i].BarrelIndex;
		if (Seats[i].Gun != None)
		{
			DebugInfo[DebugInfo.length - 1] @= "IsAimCorrect" @ Seats[i].Gun.IsAimCorrect();
		}
	}
}

/** Kismet hook for kicking a pawn out of a vehicle */
function OnExitVehicle(UTSeqAct_ExitVehicle Action)
{
	local int i;

	for (i = 0; i < Seats.length; i++)
	{
		if (Seats[i].SeatPawn != None && Seats[i].SeatPawn.Driver != None)
		{
			Seats[i].SeatPawn.DriverLeave(true);
		}
	}
}
simulated function DrawHUD( HUD H )
{
	local int i;
	local vector v;
	local Canvas Canvas;
	local string s,n;
	local float xl,yl;
	local UTSkelControl_Damage SK;

	super.DrawHud(h);

	if (bShowDamageDebug)
	{
		Canvas = H.Canvas;
		for (i=0;i<	DamageMorphTargets.Length;i++)
		{
			v = Mesh.GetBoneLocation(DamageMorphTargets[i].InfluenceBone);
			V = Canvas.Project(v);
			n = ""$DamageMorphTargets[i].MorphNodeName;
			n = right(n,len(n)-11);
			s = n$":"@DamageMorphTargets[i].Health;
			Canvas.Strlen(s,xl,yl);
			Canvas.SetPos(V.x-(xl/2),V.y);
			Canvas.SetDrawColor(0,0,0,255);
			Canvas.DrawRect(xl,yl);
			Canvas.SetPos(V.x-(xl/2),V.y);
			Canvas.SetDrawColor(255,255,0,255);
			Canvas.DrawText(s);

			n = "Damage_"$n;
			SK = UTSkelControl_Damage(Mesh.FindSkelControl(name(n)));
			if (SK != none)
			{
				Canvas.SetPos(V.x-(xl/2),V.y+yl);
				Canvas.SetDrawColor(0,0,0,255);
				Canvas.DrawRect(xl,yl);
				Canvas.SetPos(V.x-(xl/2),V.y+yl);
				Canvas.SetDrawColor(255,255,0,255);
				if (UTSkelControl_DamageSpring(sk) != none && UTSkelControl_DamageSpring(sk).bIsBroken)
					Canvas.DrawText(SK.ControlStrength$" [BROKEN]");
				else
					Canvas.DrawText(SK.ControlStrength);
			}
		}
	}
}

simulated exec function DamageDebug()
{
	bShowDamageDebug = !bShowDamageDebug;
}

simulated exec function DamageDamage(int index)
{
	DamageMorphTargets[index].Health = 0;
}

/** stub for vehicles with shield firemodes */
function SetShieldActive(int SeatIndex, bool bActive);


simulated function name GetHoverBoardAttachPoint(vector HoverBoardLocation)
{
	local name Result;
	local float Dist, MinDist;
	local vector v;
	local int i;

	if ( HoverBoardAttachSockets.Length > 0 )
	{
		for (i=0;i<HoverBoardAttachSockets.Length;i++)
		{
			Mesh.GetSocketWorldLocationAndRotation(HoverBoardAttachSockets[i],v);
			Dist = VSize( HoverBoardLocation - v );
			if ( i==0 || Dist < MinDist )
			{
				Result = HoverBoardAttachSockets[i];
				MinDist = Dist;
			}
		}
	}

	return result;
}

function SetSeatStoragePawn(int SeatIndex, Pawn PawnToSit)
{
	local int Mask;

	Seats[SeatIndex].StoragePawn = PawnToSit;
	if ( (SeatIndex == 1) && (Role == ROLE_Authority) )
	{
		PassengerPRI = (PawnToSit == None) ? None : Seats[SeatIndex].SeatPawn.PlayerReplicationInfo;
	}

	Mask = 1 << SeatIndex;

	if ( PawnToSit != none )
	{
		SeatMask = SeatMask | Mask;
	}
	else
	{
		if ( (SeatMask & Mask) > 0)
		{
			SeatMask = SeatMask ^ Mask;
		}
	}

}

simulated function SetMovementEffect(int SeatIndex, bool bSetActive,optional UTPawn UTP)
{
	local bool bIsLocal;
	if(bSetActive && ReferenceMovementMesh != none)
	{
		bIsLocal = UTP==none? IsLocallyControlled():UTP.IsLocallyControlled();
		if(bIsLocal && WorldInfo.Netmode != NM_DEDICATEDSERVER)
		{
			// Should never happen, but just in case:
			if(Seats[SeatIndex].SeatMovementEffect != none)
			{
				Seats[SeatIndex].SeatMovementEffect.Destroyed();
			}
			Seats[SeatIndex].SeatMovementEffect = Spawn(class'VehicleMovementEffect',Seats[SeatIndex].SeatPawn,,Location,Rotation);
			if(Seats[SeatIndex].SeatMovementEffect != none)
			{
				Seats[SeatIndex].SeatMovementEffect.SetBase(self);
				Seats[SeatIndex].SeatMovementEffect.AirEffect.SetStaticMesh(ReferenceMovementMesh);
			}
		}
	}
	else
	{
		if(Seats[SeatIndex].SeatMovementEffect != none)
		{
			Seats[SeatIndex].SeatMovementEffect.Destroyed();
		}
	}
}

/** @return Actor that can be locked on to by weapons as if it were this vehicle */
function Actor GetAlternateLockTarget();

function bool CanAttack(Actor Other)
{
	local float MaxRange;
	local Weapon W;

	if (bShouldLeaveForCombat && Driver != None && Driver.InvManager != None && Controller != None && !IsHumanControlled())
	{
		// return whether can attack with handheld weapons (assume bot will leave if it actually decides to attack)
		foreach Driver.InvManager.InventoryActors(class'Weapon', W)
		{
			MaxRange = FMax(MaxRange, W.MaxRange());
		}
		return (VSize(Location - Other.Location) <= MaxRange && Controller.LineOfSightTo(Other));
	}
	else
	{
		return Super.CanAttack(Other);
	}
}

simulated function DisplayHud(UTHud Hud, Canvas Canvas, vector2D HudPOS, optional int SeatIndex)
{
	local vector2D POS;
	local float W,H;
	local float PulseScale;
	local float PercValue;

	// Figure out dims and resolve the hud position

	W = Abs(HudCoords.UL) * HUD.ResolutionScale;
	H = Abs(HudCoords.VL) * HUD.ResolutionScale;
	POS = Hud.ResolveHUDPosition(HudPOS, W, H);

	// Draw the Vehicle icon

	Canvas.SetPos(POS.X, POS.Y);
	Canvas.DrawColorizedTile(HudIcons, W, H, HudCoords.U, HudCoords.V, HudCoords.UL, HudCoords.VL, Hud.TeamHudColor);

	// Draw the Seats.

	DisplaySeats(HUD, Canvas, POS, W,H, SeatIndex);

	// Recalc the Positons given the healt bar

	POS = Hud.ResolveHUDPosition(HudPOS, W + 140, 60);
	Canvas.SetPos(POS.X,POS.Y);
	Canvas.DrawColorizedTile(HUD.AltHudTexture, (140 * HUD.ResolutionScale), 44 * (HUD.ResolutionScale), 4,346,112,35, HUD.TeamHUDColor);

	// Pulse if health is being added

	if ( Health > LastHealth )
	{
		HealthPulseTime = HUD.PulseDuration;
	}

	LastHealth = Health;

	if (HealthPulsetime > 0)
	{
		HUD.CalculatePulse(PulseScale, HealthPulseTime);
	}
	else
	{
		PulseScale = 1.0;
	}


	// Draw the health Text

    Hud.DrawGlowText( string(Health), POS.X + (130 * HUD.ResolutionSCale), POS.Y + (-9 * HUD.ResolutionScale), 53,PulseScale,true);

    // Draw the Bar Graph

    PercValue = float(Health) / float(Default.Health);
	HUD.DrawHealth(POS.X + (25 * HUD.ResolutionScale), POS.Y + (32 * HUD.ResolutionScale), 110 * PercValue * HUD.ResolutionScale,
								110 * HUD.ResolutionScale, 16 * HUD.ResolutionScale, Canvas);

	// Draw any extra data.  We pass in the full bounds of the widget as well as the full X/Y

	W += 140 * HUD.ResolutionScale;
	POS = Hud.ResolveHUDPosition(HUDPOS, W, H);
	DisplayExtraHud(Hud, Canvas, POS, W, H, SeatIndex);

}

simulated function DisplayExtraHud(UTHud Hud, Canvas Canvas, vector2D POS, float Width, float Height, int SIndex);

simulated function DisplaySeats(UTHud Hud, Canvas Canvas, vector2D HudPOS, float Width, float Height, int SIndex)
{
	local int i;
	local float X,Y;

	for (i=0;i<Seats.Length;i++)
	{
		X = HudPOS.X + (Width * Seats[i].SeatIconPOS.X);
		Y = HudPOS.Y + (Height * Seats[i].SeatIconPOS.Y);
		Canvas.SetPos(X,Y);
		Hud.DrawTileCentered(Hud.AltHudTexture, 18 * Hud.ResolutionScale, 18 * Hud.ResolutionScale, 267,1,20,20, GetSeatColor(i, i == SIndex) );
	}
}

simulated function LinearColor GetSeatColor(int SeatIndex, bool bIsPlayersSeat)
{
	local byte TestMask;
	if (!bIsPlayersSeat)
	{
		TestMask = 1 << SeatIndex;
		if ( (SeatMask & TestMask) > 0)
		{
			return MakeLinearColor(1.0,1.0,1.0,1.0);
		}
		else
		{
			return MakeLinearColor(0.5,0.5,0.5,1.0);
		}
	}
	else
	{
		return MakeLinearColor(1.0,1.0,0.0,1.0);
	}
}


exec function SPX(float Adj)
{
	Seats[0].SeatIconPOS.X = Adj;
}

exec function SPY(float Adj)
{
	Seats[0].SeatIconPOS.Y = Adj;
}


defaultproperties
{

	Begin Object Class=DynamicLightEnvironmentComponent Name=MyLightEnvironment
	End Object
	Components.Add(MyLightEnvironment)
	LightEnvironment=MyLightEnvironment

	Begin Object Name=SVehicleMesh
		CastShadow=true
		bCastDynamicShadow=true
		LightEnvironment=MyLightEnvironment
		bOverrideAttachmentOwnerVisibility=true
		bAcceptsDecals=false
	End Object

	Begin Object Name=CollisionCylinder
		BlockNonZeroExtent=false
		BlockZeroExtent=false
		BlockActors=false
		BlockRigidBody=false
		CollideActors=false
	End Object

	InventoryManagerClass=class'UTInventoryManager'
	bCanCarryFlag=true
	bDriverHoldsFlag=false
	bEjectKilledBodies=true
	LinkHealMult=0.35
	VehicleLostTime=0.0
	TeamBeaconMaxDist=8000.0
	TeamBeaconPlayerInfoMaxDist=3000.f
	MaxDesireability=0.5
	bTeamLocked=true
	bEnteringUnlocks=true
	StolenAnnouncementIndex=4
	bEjectPassengersWhenFlipped=true
	MinRunOverSpeed=250.0
	MinCrushSpeed=100.0
	LookForwardDist=0.0
	MomentumMult=2.0
	bNoZDampingInAir=true

	LookSteerSensitivity=2.0

	bCanFlip=false
	RanOverDamageType=class'UTGame.UTDmgType_RanOver'
	CrushedDamageType=class'UTGame.UTDmgType_Pancake'
	MinRunOverWarningAim=0.88
	ObjectiveGetOutDist=1000.0
	ExplosionTemplate=ParticleSystem'FX_VehicleExplosions.Effects.P_FX_GeneralExplosion'
	BigExplosionTemplates[0]=(Template=ParticleSystem'FX_VehicleExplosions.Effects.P_FX_VehicleDeathExplosion')

	SeatCameraScale=1.0

	DamageSmokeThreshold=0.5
	FireDamageThreshold=0.25
	MaxImpactEffectDistance=6000.0
	MaxFireEffectDistance=7000.0

	Team=UTVEHICLE_UNSET_TEAM // impossible value so that we know if it hasn't replicated yet
	TeamBeaconOffset=(z=100.0)
	PassengerTeamBeaconOffset=(z=100.0)
	SpawnRadius=320.0
	BurnOutTime=4.0
	DeadVehicleLifeSpan=9.0
	BurnTimeParameterName=BurnTime
	BurnValueParameterName=BurnValue
	SpawnInTime=+2.0

	LockedTexture=Texture2D'T_UTHudGraphics.Textures.UTOldHUD'

	MapSize=8
	IconXStart=0.75
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	BaseEyeheight=30
	Eyeheight=30

	DrivingAnim=Manta_Idle_Sitting

	bDrawHealthOnHUD=true

	LockedOnSound=Soundcue'A_Weapon.AVRiL.Cue.A_Weapon_AVRiL_Lock_Cue'

	CollisionDamageMult=0.002
	CameraScaleMin=0.15
	CameraScaleMax=2.0
	CameraLag=0.12
	ViewPitchMin=-15000
	MinCameraDistSq=1.0

	DefaultFOV=75
	bNoZSmoothing=true
	CameraSmoothingFactor=2.0

	RespawnTime=30.0
	InitialSpawnDelay=+0.0

	ExplosionDamage=100.0
	ExplosionRadius=300.0
	ExplosionMomentum=60000
	ExplosionLightClass=class'UTGame.UTTankShellExplosionLight'
	MaxExplosionLightDistance=+4000.0
	DeathExplosionShake=CameraAnim'Envy_Effects.Camera_Shakes.C_VH_Death_Shake'
	InnerExplosionShakeRadius=450.0
	OuterExplosionShakeRadius=1000.0
	ExplosionDamageType=class'UTDmgType_VehicleExplosion'

	WaterDamage=10.0
	FireDamagePerSec=2.0
	UpsideDownDamagePerSec=500.0
	bValidLinkTarget=true

	bMustBeUpright=true
	FlagOffset=(Z=120.0)
	FlagRotation=(Yaw=32768)

	SpawnInSound = SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeIn01Cue'
	SpawnOutSound = SoundCue'A_Vehicle_Generic.Vehicle.VehicleFadeOut01Cue'
	LinkedEndSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleChargeCompleteCue'
	LinkedToCue=SoundCue'A_Vehicle_Generic.Vehicle.VehicleChargeLoopCue'

	DisabledTime=20.0
	VehicleLockedSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleNoEntry01Cue';
	LargeChunkImpactSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleImpact_MetalLargeCue'
	MediumChunkImpactSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleImpact_MetalMediumCue'
	SmallChunkImpactSound=SoundCue'A_Vehicle_Generic.Vehicle.VehicleImpact_MetalSmallCue'
	HornSounds[0]=SoundCue'A_Vehicle_Generic.Vehicle.VehicleHornCue'
	ImpactHitSound=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_FireImpactVehicle_Cue'

	HornAIRadius=800.0

	bPushedByEncroachers=false
	bAlwaysRelevant=true //@TODO FIXMESTEVE - lots of vehicles don't need this

	DisabledTemplate=ParticleSystem'Pickups.Deployables.Effects.P_Deployables_EMP_Mine_VehicleDisabled'
	TurretScaleControlName=TurretScale
	TurretSocketName=VH_Death
	TurretOffset=(X=0.0,Y=0.0,Z=200.0);
	DistanceTurretExplosionTemplates[0]=(Template=ParticleSystem'Envy_Effects.VH_Deaths.P_VH_Death_SpecialCase_1_Base_Near',MinDistance=1500.0)
	DistanceTurretExplosionTemplates[1]=(Template=ParticleSystem'Envy_Effects.VH_Deaths.P_VH_Death_SpecialCase_1_Base_Far',MinDistance=0.0)
	TurretExplosiveForce=10000.0f

	HUDExtent=100.0

	VehicleSounds(0)=(SoundStartTag=DamageSmoke,SoundEndTag=NoDamageSmoke,SoundTemplate=SoundCue'A_Vehicle_Generic.Vehicle.Vehicle_Damage_Burst_Cue')
	VehicleSounds(1)=(SoundStartTag=DamageSmoke,SoundEndTag=NoDamageSmoke,SoundTemplate=SoundCue'A_Vehicle_Generic.Vehicle.Vehicle_Damage_FireLoop_Cue')
}
