/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPawn extends GamePawn
	config(Game)
	dependson(UTWeaponAttachment)
	dependson(UTEmitter)
	dependson(UTFamilyInfo)
	native
	nativereplication
	notplaceable;

var		bool	bFixedView;
var		bool	bIsTyping;				// play typing anim if idle
var		bool	bSpawnDone;				// true when spawn protection has been deactivated
var		bool	bSpawnIn;
var		bool	bShieldAbsorb;			// set true when shield absorbs damage
var		bool	bDodging;				// true while in air after dodging
var		bool	bNoJumpAdjust;			// set to tell controller not to modify velocity of a jump/fall
var		bool	bStopOnDoubleLanding;
var		bool	bReadyToDoubleJump;
var		bool	bIsInvulnerable;
var		bool	bIsInvisible;			// true if pawn is invisible to AI

/** true when in ragdoll due to feign death */
var repnotify bool bFeigningDeath;

/** true while playing feign death recovery animation */
var bool bPlayingFeignDeathRecovery;

/** true when feign death activation forced (e.g. knocked off hoverboard) */
var bool bForcedFeignDeath;

/** The pawn's light environment */
var LightEnvironmentComponent LightEnvironment;

/** anim node used for feign death recovery animations */
var AnimNodeBlend FeignDeathBlend;

/** Slot node used for playing full body anims. */
var AnimNodeSlot FullBodyAnimSlot;

/** Slot node used for playing animations only on the top half. */
var AnimNodeSlot TopHalfAnimSlot;

var(DeathAnim)	float	DeathHipLinSpring;
var(DeathAnim)	float	DeathHipLinDamp;
var(DeathAnim)	float	DeathHipAngSpring;
var(DeathAnim)	float	DeathHipAngDamp;

/** Struct used for communicating info to play an emote from server to clients. */
struct native PlayEmoteInfo
{
	var	name	EmoteTag;
	var int		EmoteID;
	var bool	bNewData;
};

/** Last time emote was played. */
var	float	LastEmoteTime;

/** Controls how often you can send an emote. */
var float	MinTimeBetweenEmotes;

/** Used to replicate on emote to play. */
var repnotify PlayEmoteInfo EmoteRepInfo;

var	bool		bUpdateEyeheight;		/** if true, UpdateEyeheight will get called every tick */
var		globalconfig bool bWeaponBob;
var bool		bJustLanded;			/** used by eyeheight adjustment.  True if pawn recently landed from a fall */
var bool		bLandRecovery;			/** used by eyeheight adjustment. True if pawn recovering (eyeheight increasing) after lowering from a landing */

var bool	bComponentDebug;

/*********************************************************************************************
 Camera related properties
********************************************************************************************* */
var vector 	FixedViewLoc;
var rotator FixedViewRot;
var float CameraScale, CurrentCameraScale; /** multiplier to default camera distance */
var float CameraScaleMin, CameraScaleMax;

/** Stop death camera using OldCameraPosition if true */
var bool bStopDeathCamera;

/** OldCameraPosition saved when dead for use if fall below killz */
var vector OldCameraPosition;

/** used to smoothly adjust camera z offset in third person */
var float CameraZOffset;

/** If true, use end of match "Hero" camera */
var bool bIsHero;

/** Used for end of match "Hero" camera */
var(HeroCamera) float HeroCameraScale;

/** Used for end of match "Hero" camera */
var(HeroCamera) int HeroCameraPitch;

var		bool	bCanDoubleJump;
var		bool	bRequiresDoubleJump;		/** set by suggestjumpvelocity() */
var		float	DodgeSpeed;
var		float	DodgeSpeedZ;
var		eDoubleClickDir CurrentDir;
var		int		MultiJumpRemaining;
var		int		MaxMultiJump;
var		int		MultiJumpBoost;
var		float	MaxDoubleJumpHeight;
var		float	SlopeBoostFriction;			// which materials allow slope boosting
var()	float	DoubleJumpEyeHeight;
var		float	DoubleJumpThreshold;
var		int		MaxLeanRoll;		/** how much pawn should lean into turns */
var		float	DefaultAirControl;

/** view bob properties */
var	globalconfig	float	Bob;
var					float	LandBob;
var					float	JumpBob;
var					float	AppliedBob;
var					float	bobtime;
var					vector	WalkBob;
var					float	EyeForward;

/** when a body in feign death's velocity is less than this, it is considered to be at rest (allowing the player to get up) */
var float FeignDeathBodyAtRestSpeed;
/** speed at which physics is blended out when bPlayingFeignDeathRecovery is true (amount subtracted from PhysicsWeight per second) */
var float FeignDeathPhysicsBlendOutSpeed;

/** when we entered feign death; used to increase FeignDeathBodyAtRestSpeed over time so we get up in a reasonable amount of time */
var float FeignDeathStartTime;

/** When feign death recovery started.  Used to pull feign death camera into body during recovery */
var float FeignDeathRecoveryStartTime;

var int  SuperHealthMax;					/** Maximum allowable boosted health */

var int	 Twisting;  // 0 = Not twisting, 1=twisting Left, -1 = Twisting Right

var enum EWeaponHand
{
	HAND_Right,
	HAND_Left,
	HAND_Centered,
	HAND_Hidden

} WeaponHand;

var class<UTPawnSoundGroup> SoundGroupClass;

/** This pawn's current family info **/
var class<UTFamilyInfo> CurrFamilyInfo;

//@todo steve - do we want this in species?
var class<UTEmit_HitEffect> BloodEmitterClass;
var array<DistanceBasedParticleTemplate> BloodEffects;

/** bio death effect - updates BioEffectName parameter in the mesh's materials from 0 to 10 over BioBurnAwayTime
 * activated BioBurnAway particle system on death, deactivates it when BioBurnAwayTime expires
 * could easily be overridden by a custom death effect to use other effects/materials, @see UTDmgType_BioGoo
 */
var bool bKilledByBio;
var ParticleSystemComponent BioBurnAway;
var float BioBurnAwayTime;
var name BioEffectName;

var ParticleSystem GibExplosionTemplate;

/** bones to set fixed when doing the physics take hit effects */
var array<name> TakeHitPhysicsFixedBones;


/** Whether or not the Death Mesh is active for this pawn **/
var bool bDeathMeshIsActive;

/** A mesh that plays for the dying player in first person; e.g. Spidermine face-attack*/
var SkeletalMeshComponent FirstPersonDeathVisionMesh;

/** Whether this pawn can play a falling impact. Set to false upon the fall, but getting up should reset it */
var bool bCanPlayFallingImpacts;
/** time above bool was set to true (for time out)*/
var float StartFallImpactTime;
/** name of the torso bone for playing impacts*/
var name TorsoBoneName;
/** sound to be played by Falling Impact*/
var SoundCue FallImpactSound;
/** Speed change that must be realized to trigger a fall sound*/
var float FallSpeedThreshold;
/** Stores the damage type the pawn was killed by*/
var class<DamageType> KilledByDamageType;

/** replicated information on a hit we've taken */
struct native TakeHitInfo
{
	/** the amount of damage */
	var int Damage;
	/** the location of the hit */
	var vector HitLocation;
	/** how much momentum was imparted */
	var vector Momentum;
	/** the damage type we were hit with */
	var class<DamageType> DamageType;
	/** the bone that was hit on our Mesh (if any) */
	var name HitBone;
};
var repnotify TakeHitInfo LastTakeHitInfo;
/** stop considering LastTakeHitInfo for replication when world time passes this (so we don't replicate out-of-date hits when pawns become relevant) */
var float LastTakeHitTimeout;
/** if set, blend Mesh PhysicsWeight to 0.0 in C++ and call TakeHitBlendedOut() event when finished */
var bool bBlendOutTakeHitPhysics;
/** speed at which physics is blended out when bBlendOutTakeHitPhysics is true (amount subtracted from PhysicsWeight per second) */
var float TakeHitPhysicsBlendOutSpeed;


/*********************************************************************************************
 Aiming
 ********************************************************************************************* */

/** Current yaw of the mesh root. This essentially lags behind the actual Pawn rotation yaw. */
var int	RootYaw;

/** Output - how quickly RootYaw is changing (unreal rot units per second). */
var float RootYawSpeed;

/** How far RootYaw differs from Rotation.Yaw before it is rotated to catch up. */
var() int	MaxYawAim;

/** 2D vector indicating aim direction relative to Pawn rotation. +/-1.0 indicating 180 degrees. */
var vector2D CurrentSkelAim;

var SkelControlSingleBone	RootRotControl;
var	AnimNodeAimOffset		AimNode;
var GameSkelCtrl_Recoil		GunRecoilNode;
var GameSkelCtrl_Recoil		LeftRecoilNode;
var GameSkelCtrl_Recoil		RightRecoilNode;


/*********************************************************************************************
  Gibs
********************************************************************************************* */

/** Track damage accumulated during a tick - used for gibbing determination */
var float AccumulateDamage;

/** Tick time for which damage is being accumulated */
var float AccumulationTime;

/** whether or not we have been gibbed already */
var bool bGibbed;

/** whether or not we have been decapitated already */
var bool bHeadGibbed;

/** The visual effect to play when a headshot gibs a head. */
var ParticleSystem HeadShotEffect;

/* Replicated when torn off body should gib */
var bool bTearOffGibs;

/*********************************************************************************************
 Hoverboard
********************************************************************************************* */
var		bool			bHasHoverboard;
var		float			LastHoverboardTime;
var		float			MinHoverboardInterval;
var		bool			bIsHoverboardAnimPawn;

/*********************************************************************************************
 Armor
********************************************************************************************* */
var float ShieldBeltArmor;
var float VestArmor;
var float ThighpadArmor;
var float HelmetArmor;

/*********************************************************************************************
 Weapon / Firing
********************************************************************************************* */

var bool bArmsAttached;
var UTSkeletalMeshComponent ArmsMesh[2]; // left and right arms, respectively.
var UTSkeletalMeshComponent ArmsOverlay[2];

//////////////////////////// UNCOMMENT THESE IF WE NEED THEM:
// LOOK FOR: NEUTRALARMSCOMMENT
//var AnimSet					NeutralArmsAnim;
//var name					NeutralArmAnimName;

/** Holds the class type of the current weapon attachment.  Replicated to all clients. */
var	repnotify	class<UTWeaponAttachment>	CurrentWeaponAttachmentClass;

/** This holds the local copy of the current attachment.  This "attachment" actor will exist
  * on all versions of the client. */
var				UTWeaponAttachment			CurrentWeaponAttachment;

/** WeaponSocket contains the name of the socket used for attaching weapons to this pawn. */
var name WeaponSocket, WeaponSocket2;

/** Socket to find the feet */
var name PawnEffectSockets[2];

/** whether we are dual-wielding our current weapon, passed to attachment to set up second mesh/anims */
var repnotify bool bDualWielding;
/** set when pawn is putting away its current weapon, for playing 3p animations - access with Set/GetPuttingDownWeapon() */
var protected repnotify bool bPuttingDownWeapon;

var				float FireRateMultiplier; /** affects firing rate of all weapons held by this pawn. */

/** These values are used for determining headshots */
var float			HeadOffset;
var float           HeadRadius;
var float           HeadHeight;
var name			HeadBone;
var repnotify float HeadScale;

var class<Actor> TransInEffects[2];
var class<Actor> TransOutEffects[2];
var	LinearColor  TranslocateColor[2];

/** pawn ambient sound (for powerups and such) */
var protected AudioComponent PawnAmbientSound;
/** ambient cue played on PawnAmbientSound component; automatically replicated and played on clients. Access via SetPawnAmbientSound() / GetPawnAmbientSound() */
var protected repnotify SoundCue PawnAmbientSoundCue;

/** seperate replicated ambient sound for weapon firing - access via SetWeaponAmbientSound() / GetWeaponAmbientSound() */
var protected AudioComponent WeaponAmbientSound;
var protected repnotify SoundCue WeaponAmbientSoundCue;

/** Component used for playing emote sounds. */
var protected AudioComponent EmoteSound;

var SoundCue ArmorHitSound;
/** sound played when initially spawning in */
var SoundCue SpawnSound;
/** sound played when we teleport */
var SoundCue TeleportSound;

/*********************************************************************************************
  Overlay system

  UTPawns support 2 separate overlay systems.   The first is a simple colorizer that can be used to
  apply a color to the pawn's skin.  This is accessed via the
********************************************************************************************* */

/** How long will it take for the current Body Material to fade out */
var float BodyMatFadeDuration;

/** how much time is left on this material */
var float RemainingBodyMatDuration;

/** This variable is used for replication of the value to remove clients */
var repnotify float ClientBodyMatDuration;

/** This is the color that will be applied */
var LinearColor BodyMatColor;

/** This is the current Body Material Color with any fading applied in */
var LinearColor CurrentBodyMatColor;

/** This is the color that will be applied when a pawn is firsted spawned in and is covered by protection */
var	LinearColor SpawnProtectionColor;

/** material parameter containing damage overlay color */
var name DamageParameterName;

/** This is the actual Material Instance that is used to affect the colors */
var protected array<MaterialInstanceConstant> BodyMaterialInstances;

/** material that is overlayed on the pawn via a seperate slightly larger scaled version of the pawn's mesh
 * Use SetOverlayMaterial()  / GetOverlayMaterial() to access. */
var protected repnotify MaterialInterface OverlayMaterialInstance;

/** mesh for overlay - should not be added to Components array in defaultproperties */
var protected SkeletalMeshComponent OverlayMesh;

/** The weapon overlay meshes need to be controlled by the pawn due to replication.  We use */
/** a bitmask to describe what effect needs to be overlay.  								*/
/**																							*/
/** 0x00 = No Overlays																		*/
/** bit 0 (0x01) = Damage Amp																*/
/** bit 1 (0x02) = Besrerk																	*/
/**																							*/
/** Use SetWeaponOverlayFlag() / ClearWeaponOverlayFlag to adjust							*/

var repnotify byte WeaponOverlayFlags;

/*********************************************************************************************
* Team beacons
********************************************************************************************* */
var(TeamBeacon) float      TeamBeaconMaxDist;
var(TeamBeacon) float      TeamBeaconPlayerInfoMaxDist;
var(TeamBeacon) Texture    SpeakingBeaconTexture;

/** Last time trace test check for drawing postrender beacon was performed */
var float LastPostRenderTraceTime;

/** true is last trace test check for drawing postrender beacon succeeded */
var bool bPostRenderTraceSucceeded;


/*********************************************************************************************
* HUD Icon
********************************************************************************************* */
var MaterialInstanceConstant HUDMaterialInstance;
var vector HUDLocation;
var float MapSize;
var float IconXStart;
var float IconYStart;
var float IconXWidth;
var float IconYWidth;

/*********************************************************************************************
* Pain
********************************************************************************************* */
const MINTIMEBETWEENPAINSOUNDS=0.35;
var			float		LastPainSound;
var float DeathTime;
var int LookYaw;	// The Yaw Used to looking around
var float RagdollLifespan;

/*********************************************************************************************
* Foot placement IK system
********************************************************************************************* */
var name			LeftFootBone, RightFootBone;
var name			LeftFootControlName, RightFootControlName;
var float			BaseTranslationOffset;
var float			CrouchTranslationOffset;
var float			OldLocationZ;
var	bool			bEnableFootPlacement;
var const float		ZSmoothingRate;
/** if the pawn is farther than this away from the viewer, foot placement is skipped */
var float MaxFootPlacementDistSquared;
/** cached references to skeletal controllers for foot placement */
var SkelControlFootPlacement LeftLegControl, RightLegControl;

/** cached references to skeletal control for hand IK */
var SkelControlLimb			LeftHandIK;
var SkelControlSingleBone	LeftHandIKRot;
var SkelControlLimb			RightHandIK;
var SkelControlSingleBone	RightHandIKRot;

/*********************************************************************************************
* Custom gravity support
********************************************************************************************* */
var float CustomGravityScaling;		// scaling factor for this pawn's gravity - reset when pawn lands/changes physics modes
var bool bNotifyStopFalling;		// if true, StoppedFalling() is called when the physics mode changes from falling

/*********************************************************************************************
* Skin swapping support
********************************************************************************************* */
var repnotify Material ReplicatedBodyMaterial;

var array<UTBot> Trackers;

/** type of vehicle spawned when activating the hoverboard */
var class<UTVehicle> HoverboardClass;

/** base vehicle pawn is in, used when the pawn is driving a UTWeaponPawn as those don't get replicated to non-owning clients */
struct native DrivenWeaponPawnInfo
{
	/** base vehicle we're in */
	var UTVehicle BaseVehicle;
	/** seat of that vehicle */
	var byte SeatIndex;
	/** ref to PRI since our PlayerReplicationInfo variable will be None while in a vehicle */
	var PlayerReplicationInfo PRI;
};
var repnotify DrivenWeaponPawnInfo DrivenWeaponPawn, LastDrivenWeaponPawn;
/** reference WeaponPawn spawned on non-owning clients for code that checks for DrivenVehicle */
var UTClientSideWeaponPawn ClientSideWeaponPawn;

/** set on client when valid team info is received; future changes are then ignored unless this gets reset first
 * this is used to prevent the problem where someone changes teams and their old dying pawn changes color
 * because the team change was received before the pawn's dying
 */
var bool bReceivedValidTeam;

/*********************************************************************************************
 * Hud Widgets
********************************************************************************************* */

/** Holds a reference to the hud scene template that will be brought in to focus when the hud
    detects that the Player Controller is possessing this pawn.  - Should be filled out in a subclass */

var UTUIScene_UTPawn PawnHudScene;

/** The default overlay to use when not in a team game */
var MaterialInterface ShieldBeltMaterialInstance;
/** A collection of overlays to use in team games */
var MaterialInterface ShieldBeltTeamMaterialInstances[4];

/** default mesh, materials, and family when custom character mesh has not yet been constructed */
var class<UTFamilyInfo> DefaultFamily;
var SkeletalMesh DefaultMesh;
var array<MaterialInterface> DefaultTeamMaterials;
var array<MaterialInterface> DefaultTeamHeadMaterials;

/** If true, head size will change based on ratio of kills to deaths */
var bool bKillsAffectHead;

/** Mirrors the # of charges available to jump boots */
var int JumpBootCharge;

cpptext
{
	virtual FLOAT DampenNoise(AActor* NoiseMaker, FLOAT Loudness, FName NoiseType=NAME_None );
	void RequestTrackingFor(AUTBot *Bot);
	virtual UBOOL TryJumpUp(FVector Dir, FVector Destination, DWORD TraceFlags, UBOOL bNoVisibility);
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual INT calcMoveFlags();
	virtual ETestMoveResult FindJumpUp(FVector Direction, FVector &CurrentPosition);
	virtual UBOOL SetHighJumpFlag();
	UBOOL UseFootPlacementThisTick();
	void EnableFootPlacement(UBOOL bEnabled);
	void DoFootPlacement(FLOAT DeltaSeconds);
	FLOAT GetGravityZ();
	void setPhysics(BYTE NewPhysics, AActor *NewFloor, FVector NewFloorV);
	virtual FVector CalculateSlopeSlide(const FVector& Adjusted, const FCheckResult& Hit);
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );

	virtual UBOOL HasAudibleAmbientSound(const FVector& SrcLocation);

	// camera
	virtual void UpdateEyeHeight(FLOAT DeltaSeconds);
	virtual void physicsRotation(FLOAT deltaTime, FVector OldVelocity);

protected:
	virtual void CalcVelocity(FVector &AccelDir, FLOAT DeltaTime, FLOAT MaxSpeed, FLOAT Friction, INT bFluid, INT bBrake, INT bBuoyant);
}

replication
{
	// replicated properties
	if ( bNetOwner && bNetDirty )
		ShieldBeltArmor, HelmetArmor, VestArmor, ThighpadArmor, bHasHoverboard;
	if ( bNetDirty )
		bFeigningDeath, CurrentWeaponAttachmentClass, bIsTyping, BodyMatColor, ClientBodyMatDuration,
		HeadScale, PawnAmbientSoundCue, WeaponAmbientSoundCue, ReplicatedBodyMaterial, OverlayMaterialInstance,
		WeaponOverlayFlags, CustomGravityScaling;
	if(bNetDirty && WorldInfo.TimeSeconds - LastEmoteTime <= MinTimeBetweenEmotes)
		EmoteRepInfo;
	if (bNetDirty && WorldInfo.TimeSeconds < LastTakeHitTimeout)
		LastTakeHitInfo;
	if (bNetDirty && !bNetOwner)
		DrivenWeaponPawn, bDualWielding, bPuttingDownWeapon;
	// variable sent to all clients when Pawn has been torn off. (bTearOff)
	if( bTearOff && bNetDirty && (Role==ROLE_Authority) )
		bTearOffGibs;
}

native function GetBoundingCylinder(out float CollisionRadius, out float CollisionHeight);

native function RestorePreRagdollCollisionComponent();

event StoppedFalling()
{
	CustomGravityScaling = 1.0;
	bNotifyStopFalling = false;
}

simulated event FellOutOfWorld(class<DamageType> dmgType)
{
    super.FellOutOfWorld(DmgType);
    bStopDeathCamera = true;
}

event HeadVolumeChange(PhysicsVolume newHeadVolume)
{
	if ( (WorldInfo.NetMode == NM_Client) || (Controller == None) )
		return;
	if (HeadVolume != None && HeadVolume.bWaterVolume)
	{
		if ( !newHeadVolume.bWaterVolume || (UTSpaceVolume(newHeadVolume) != None) )
		{
			if ( Controller.bIsPlayer && (BreathTime > 0) && (BreathTime < 8) )
				Gasp();
			BreathTime = -1.0;
		}
	}
	else if ( newHeadVolume != None && newHeadVolume.bWaterVolume && (UTSpaceVolume(newHeadVolume) == None) )
	{
		BreathTime = UnderWaterTime;
	}
}

/**
@RETURN true if pawn is invisible to AI
*/
native function bool IsInvisible();

/** PoweredUp()
returns true if pawn has game play advantages, as defined by specific game implementation
*/
function bool PoweredUp()
{
	return ( (DamageScaling > 1) || (FireRateMultiplier > 1) || bIsInvulnerable );
}

/** InCombat()
returns true if pawn is currently in combat, as defined by specific game implementation.
*/
function bool InCombat()
{
	return (WorldInfo.TimeSeconds - LastPainSound < 1) && !PhysicsVolume.bPainCausing;
}

/** function used to update where icon for this actor should be rendered on the HUD
 *  @param NewHUDLocation is a vector whose X and Y components are the X and Y components of this actor's icon's 2D position on the HUD
 */
simulated function SetHUDLocation(vector NewHUDLocation)
{
	HUDLocation = NewHUDLocation;
}

simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, LinearColor FinalColor)
{
	if ( HUDMaterialInstance == None )
	{
		HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		HUDMaterialInstance.SetParent(MP.HUDIcons);
	}
	HUDMaterialInstance.SetVectorParameterValue('HUDColor', FinalColor);
	MP.DrawRotatedMaterialTile(Canvas,HUDMaterialInstance, HUDLocation, Rotation.Yaw, MapSize, MapSize*Canvas.ClipY/Canvas.ClipX, IconXStart, IconYStart, IconXWidth, IconYWidth);
}

/**
SuggestJumpVelocity()
returns true if succesful jump from start to destination is possible
returns a suggested initial falling velocity in JumpVelocity
Uses GroundSpeed and JumpZ as limits
*/
native function bool SuggestJumpVelocity(out vector JumpVelocity, vector Destination, vector Start);

exec function togglefeet()
{
	bEnableFootPlacement = !bEnableFootPlacement;
}

exec function zpivot(float f)
{
	Mesh.SetTranslation(Mesh.Translation + Vect(0,0,1) * f);
}

/**
 * UTPawns not allowed to set bIsWalking true
 */
event SetWalking( bool bNewIsWalking )
{
}

simulated function ClearBodyMatColor()
{
	RemainingBodyMatDuration = 0;
	ClientBodyMatDuration = 0;
	BodyMatFadeDuration = 0;
}

simulated function SetBodyMatColor(LinearColor NewBodyMatColor, float NewOverlayDuration)
{
	RemainingBodyMatDuration = NewOverlayDuration;
	ClientBodyMatDuration = RemainingBodyMatDuration;
	BodyMatFadeDuration = 0.5 * RemainingBodyMatDuration;
	BodyMatColor = NewBodyMatColor;
	CurrentBodyMatColor = BodyMatColor;
	CurrentBodyMatColor.R += 1;				// make sure CurrentBodyMatColor differs from BodyMatColor to force update
	VerifyBodyMaterialInstance();
}

/**
 * SetSkin is used to apply a single material to the entire model, including any applicable attachments.
 * NOTE: Attachments (ie: the weapons) need to handle resetting their default skin if NewMaterinal = NONE
 *
 * @Param	NewMaterial		The material to apply
 */

simulated function SetSkin(Material NewMaterial)
{
	local int i;

	// Replicate the Material to remote clients

	ReplicatedBodyMaterial = NewMaterial;

	if (VerifyBodyMaterialInstance())		// Make sure we have setup the BodyMaterialInstances array
	{
		// Propagate it to the 3rd person weapon attachment
		if (CurrentWeaponAttachment != None)
		{
			CurrentWeaponAttachment.SetSkin(NewMaterial);
		}

		// Propagate it to the 1st person weapon
		if (UTWeapon(Weapon) != None)
		{
			UTWeapon(Weapon).SetSkin(NewMaterial);
		}

		// Set the skin
		if (NewMaterial == None)
		{
			for (i = 0; i < Mesh.SkeletalMesh.Materials.length; i++)
			{
				Mesh.SetMaterial(i, BodyMaterialInstances[i]);
			}
		}
		else
		{
			for (i = 0; i < Mesh.SkeletalMesh.Materials.length; i++)
			{
				Mesh.SetMaterial(i, NewMaterial);
			}
		}

		SetArmsSkin(NewMaterial);
	}
}

simulated protected function SetArmsSkin(MaterialInterface NewMaterial)
{
	local int i,Cnt;
	local UTPlayerReplicationInfo PRI;

	// if no material specified, grab default from PRI (if that's None too, use mesh default)
	if (NewMaterial == None)
	{
		PRI = UTPlayerReplicationInfo(PlayerReplicationInfo);
		if (PRI == None && DrivenVehicle != None)
		{
			PRI = UTPlayerReplicationInfo(DrivenVehicle.PlayerReplicationInfo);
		}
		if (PRI != None)
		{
			NewMaterial = PRI.FirstPersonArmMaterial;
		}
	}

	if ( NewMaterial == None )	// Clear the materials
	{
		if(default.ArmsMesh[0] != none && ArmsMesh[0] != none)
		{
			if( default.ArmsMesh[0].Materials.Length > 0)
			{
				Cnt = Default.ArmsMesh[0].Materials.Length;
				for(i=0;i<Cnt;i++)
				{
					ArmsMesh[0].SetMaterial(i,Default.ArmsMesh[0].GetMaterial(i) );
				}
			}
			else if(ArmsMesh[0].Materials.Length > 0)
			{
				Cnt = ArmsMesh[0].Materials.Length;
				for(i=0;i<Cnt;i++)
				{
					ArmsMesh[0].SetMaterial(i,none);
				}
			}
		}

		if(default.ArmsMesh[1] != none && ArmsMesh[1] != none)
		{
			if( default.ArmsMesh[1].Materials.Length > 0)
			{
				Cnt = Default.ArmsMesh[1].Materials.Length;
				for(i=0;i<Cnt;i++)
				{
					ArmsMesh[1].SetMaterial(i,Default.ArmsMesh[1].GetMaterial(i) );
				}
			}
			else if(ArmsMesh[1].Materials.Length > 0)
			{
				Cnt = ArmsMesh[1].Materials.Length;
				for(i=0;i<Cnt;i++)
				{
					ArmsMesh[1].SetMaterial(i,none);
				}
			}
		}
	}
	else
	{
		if ((default.ArmsMesh[0] != none && ArmsMesh[0] != none) && (default.ArmsMesh[0].Materials.Length > 0 || ArmsMesh[0].GetNumElements() > 0))
		{
			Cnt = default.ArmsMesh[0].Materials.Length > 0 ? default.ArmsMesh[0].Materials.Length : ArmsMesh[0].GetNumElements();
			for(i=0; i<Cnt;i++)
			{
				ArmsMesh[0].SetMaterial(i,NewMaterial);
			}
		}
		if ((default.ArmsMesh[1] != none && ArmsMesh[1] != none) && (default.ArmsMesh[1].Materials.Length > 0 || ArmsMesh[1].GetNumElements() > 0))
		{
			Cnt = default.ArmsMesh[1].Materials.Length > 0 ? default.ArmsMesh[1].Materials.Length : ArmsMesh[1].GetNumElements();
			for(i=0; i<Cnt;i++)
			{
				ArmsMesh[1].SetMaterial(i,NewMaterial);
			}
		}
	}
}
/**
 * This function will verify that the BodyMaterialInstance variable is setup and ready to go.  This is a key
 * component for the BodyMat overlay system
 */

simulated function bool VerifyBodyMaterialInstance()
{
	local int i;

	if (WorldInfo.NetMode != NM_DedicatedServer && Mesh != None && BodyMaterialInstances.length < Mesh.GetNumElements())
	{
		// set up material instances (for overlay effects)
		BodyMaterialInstances.length = Mesh.GetNumElements();
		for (i = 0; i < BodyMaterialInstances.length; i++)
		{
			if (BodyMaterialInstances[i] == None)
			{
				BodyMaterialInstances[i] = Mesh.CreateAndSetMaterialInstanceConstant(i);
			}
		}
	}
	return (BodyMaterialInstances.length > 0);
}

/** Set various properties for this UTPawn based on  */
simulated function SetInfoFromFamily(class<UTFamilyInfo> Info, SkeletalMesh SkelMesh)
{
	local int i;
	local bool bMeshChanged;

	// AnimSets
	Mesh.AnimSets = Info.default.AnimSets;

	// Mesh
	bMeshChanged = (Mesh.SkeletalMesh != SkelMesh);
	Mesh.SetSkeletalMesh(SkelMesh);

	// PhysicsAsset
	// Force it to re-initialise if the skeletal mesh has changed (might be flappy bones etc).
	Mesh.SetPhysicsAsset(Info.default.PhysAsset, bMeshChanged);

	// Make sure bEnableFullAnimWeightBodies is only TRUE if it needs to be (PhysicsAsset has flappy bits)
	Mesh.bEnableFullAnimWeightBodies = FALSE;
	for(i=0; i<Mesh.PhysicsAsset.BodySetup.length && !Mesh.bEnableFullAnimWeightBodies; i++)
	{
		if(Mesh.PhysicsAsset.BodySetup[i].bAlwaysFullAnimWeight)
		{
			Mesh.bEnableFullAnimWeightBodies = TRUE;
		}
	}

	// Bone names
	LeftFootBone = Info.default.LeftFootBone;
	RightFootBone = Info.default.RightFootBone;
	TakeHitPhysicsFixedBones = Info.default.TakeHitPhysicsFixedBones;

	// sounds
	SoundGroupClass = Info.default.SoundGroupClass;

	// set my famliy info!
	CurrFamilyInfo = Info;
}

simulated exec function TestResetPhys()
{
	`log("Reset Char Phys");
	ResetCharPhysState();
}

simulated function ResetCharPhysState()
{
	local UTVehicle UTV;

	if(Mesh.PhysicsAssetInstance != None)
	{
		// Now set up the physics based on what we are currently doing.
		if(Physics == PHYS_RigidBody)
		{
			// Ragdoll case
			Mesh.PhysicsAssetInstance.SetAllBodiesFixed(FALSE);
			Mesh.SetBlockRigidBody(TRUE);
			SetHandIKEnabled(FALSE);
		}
		else
		{
			// Normal case
			Mesh.PhysicsAssetInstance.SetAllBodiesFixed(TRUE);
			Mesh.PhysicsAssetInstance.SetFullAnimWeightBonesFixed(FALSE, Mesh);
			Mesh.SetBlockRigidBody(FALSE);
			SetHandIKEnabled(TRUE);

			// Allow vehicles to do any specific modifications to driver
			if(DrivenVehicle != None)
			{
				UTV = UTVehicle(DrivenVehicle);
				if(UTV != None)
				{
					UTV.OnDriverPhysicsAssetChanged(self);
				}
			}
		}
	}
}

simulated function NotifyTeamChanged()
{
	local UTPlayerReplicationInfo PRI;
	local byte TeamNum;
	local bool bMeshChanged, bAttachmentVisible, bPhysicsAssetChanged;
	local PhysicsAsset OldPhysAsset;

	// set mesh to the one in the PRI, or default for this team if not found
	PRI = UTPlayerReplicationInfo(PlayerReplicationInfo);
	if (PRI == None && DrivenVehicle != None)
	{
		PRI = UTPlayerReplicationInfo(DrivenVehicle.PlayerReplicationInfo);
	}
	if (PRI != None)
	{
		if (PRI.CharacterMesh != None && PRI.CharacterMesh != DefaultMesh)
		{
			bMeshChanged = (PRI.CharacterMesh != Mesh.SkeletalMesh);
			OldPhysAsset = Mesh.PhysicsAsset;

			SetInfoFromFamily(class'UTCustomChar_Data'.static.FindFamilyInfo(PRI.CharacterData.FamilyID), PRI.CharacterMesh);

			if(Mesh.PhysicsAsset != OldPhysAsset)
			{
				bPhysicsAssetChanged = TRUE;
			}

			if (OverlayMesh != None)
			{
				OverlayMesh.SetSkeletalMesh(PRI.CharacterMesh);
			}

			if (WorldInfo.NetMode != NM_DedicatedServer)
			{
				VerifyBodyMaterialInstance();
				BodyMaterialInstances[0].SetParent(PRI.CharacterMesh.Materials[0]);
				BodyMaterialInstances[1].SetParent(PRI.CharacterMesh.Materials[1]);
			}
		}
		else
		{
			bMeshChanged = (DefaultMesh != Mesh.SkeletalMesh);
			OldPhysAsset = Mesh.PhysicsAsset;

			SetInfoFromFamily(DefaultFamily, DefaultMesh);

			if(Mesh.PhysicsAsset != OldPhysAsset)
			{
				bPhysicsAssetChanged = TRUE;
			}

			if (OverlayMesh != None)
			{
				OverlayMesh.SetSkeletalMesh(DefaultMesh);
			}

			if (WorldInfo.NetMode != NM_DedicatedServer && (!bReceivedValidTeam || bMeshChanged))
			{
				TeamNum = GetTeamNum();
				VerifyBodyMaterialInstance();
				BodyMaterialInstances[0].SetParent((TeamNum < DefaultTeamHeadMaterials.length) ? DefaultTeamHeadMaterials[TeamNum] : DefaultMesh.Materials[0]);
				BodyMaterialInstances[1].SetParent((TeamNum < DefaultTeamMaterials.length) ? DefaultTeamMaterials[TeamNum] : DefaultMesh.Materials[1]);
			}
		}

		if (bMeshChanged)
		{
			// refresh weapon attachment
			if (CurrentWeaponAttachmentClass != None)
			{
				// recreate weapon attachment in case the socket on the new mesh is in a different place
				if (CurrentWeaponAttachment != None)
				{
					bAttachmentVisible = !CurrentWeaponAttachment.Mesh.HiddenGame;
					CurrentWeaponAttachment.Destroy();
					CurrentWeaponAttachment = None;
				}
				else
				{
					bAttachmentVisible = true;
				}
				WeaponAttachmentChanged();
				SetWeaponAttachmentVisibility(bAttachmentVisible);
			}
			// refresh overlay
			if (OverlayMaterialInstance != None)
			{
				SetOverlayMaterial(OverlayMaterialInstance);
			}
		}

		// Make sure physics is in the correct state.
		if(bPhysicsAssetChanged || bMeshChanged)
		{
			bIsHoverboardAnimPawn = FALSE;
			ResetCharPhysState();
		}

		// Update first person arms
		NotifyArmMeshChanged();
	}

	if (!bReceivedValidTeam)
	{
		SetTeamColor();
		bReceivedValidTeam = (GetTeam() != None);
	}
}

/** Updates first person arm meshes when the mesh they should be using changes. */
simulated function NotifyArmMeshChanged()
{
	local UTPlayerReplicationInfo UTPRI;

	UTPRI = UTPlayerReplicationInfo(PlayerReplicationInfo);

	// Arms
	ArmsMesh[0].SetSkeletalMesh(UTPRI.FirstPersonArmMesh);
	ArmsMesh[1].SetSkeletalMesh(UTPRI.FirstPersonArmMesh);
	if (ArmsOverlay[0] != None)
	{
		ArmsOverlay[0].SetSkeletalMesh(UTPRI.FirstPersonArmMesh);
	}
	if (ArmsOverlay[1] != None)
	{
		ArmsOverlay[1].SetSkeletalMesh(UTPRI.FirstPersonArmMesh);
	}
	SetArmsSkin(ReplicatedBodyMaterial);
}

/**
 * When a pawn's team is set or replicated, SetTeamColor is called.  By default, this will setup
 * any required material parameters.
 */
simulated function SetTeamColor()
{
	local int i;
	local UTPlayerReplicationInfo PRI;

	if ( PlayerReplicationInfo != None )
	{
		PRI = UTPlayerReplicationInfo(PlayerReplicationInfo);
	}
	else if ( DrivenVehicle != None )
	{
		PRI = UTPlayerReplicationInfo(DrivenVehicle.PlayerReplicationInfo);
	}
	if ( PRI == None )
		return;

	if ( PRI.Team == None )
	{
		if ( VerifyBodyMaterialInstance() )
		{
			for (i = 0; i < BodyMaterialInstances.length; i++)
			{
				BodyMaterialInstances[i].SetVectorParameterValue('TeamColor', PRI.SkinColor);
			}
		}
	}
	else if (VerifyBodyMaterialInstance())
	{
		if ( PRI.Team.TeamIndex == 0 )
		{
			for (i = 0; i < BodyMaterialInstances.length; i++)
			{
				BodyMaterialInstances[i].SetVectorParameterValue('TeamColor', MakeLinearColor(2,0,0,2));
			}
		}
		else
		{
			for (i = 0; i < BodyMaterialInstances.length; i++)
			{
				BodyMaterialInstances[i].SetVectorParameterValue('TeamColor', MakeLinearColor(0,0,2,2));
			}
		}
	}
}

exec function ShowCompDebug()
{
	bComponentDebug = !bComponentDebug;
}

simulated function PostBeginPlay()
{
	local rotator R;
	local PlayerController PC;

	Super.PostBeginPlay();

	if (Mesh != None)
	{
		BaseTranslationOffset = Mesh.Translation.Z + 4.0;
		CrouchTranslationOffset = Mesh.Translation.Z + CylinderComponent.CollisionHeight - CrouchHeight;
		OverlayMesh.SetSkeletalMesh(Mesh.SkeletalMesh);
		OverlayMesh.SetParentAnimComponent(Mesh);
		Mesh.AttachComponent(BioBurnAway,TorsoBoneName);
	}
	if (WorldInfo.NetMode != NM_DedicatedServer && ArmsMesh[0] != none)
	{
		CreateOverlayArmsMesh();
		// NEUTRALARMSCOMMENT:
		//ArmsMesh[0].AnimSets[0]=NeutralArmsAnim;
		//ArmsMesh[1].AnimSets[0]=NeutralArmsAnim;
	}

	bCanDoubleJump = CanMultiJump();

	// Zero out Pitch and Roll
	R.Yaw = Rotation.Yaw;
	SetRotation(R);

	// add to local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
		if ( UTHUD(PC.MyHUD) != None )
			UTHUD(PC.MyHUD).AddPostRenderedActor(self);

	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		if ( class'UTPlayerController'.Default.PawnShadowMode == SHADOW_None )
		{
			if ( Mesh != None )
			{
				Mesh.CastShadow = false;
				Mesh.bCastDynamicShadow = false;
			}
		}
	}
}


/** Retrieves settings from this pawn that were set by the player in their profile and stored in the UTPC. */
function RetrieveSettingsFromPC()
{
	/*local UTPlayerController UTPC;

	UTPC = UTPlayerController(Controller);

	if(UTPC != none)
	{
		bWeaponBob = UTPC.bProfileWeaponBob;

		if (UTInventoryManager(InvManager) != None)
		{
			UTInventoryManager(InvManager).bAutoSwitchWeaponOnPickup = UTPC.bProfileAutoSwitchWeaponOnPickup;
		}
	}
	*/
}

simulated event PostInitAnimTree(SkeletalMeshComponent SkelComp)
{
	if (SkelComp == Mesh)
	{
		LeftLegControl = SkelControlFootPlacement(Mesh.FindSkelControl(LeftFootControlName));
		RightLegControl = SkelControlFootPlacement(Mesh.FindSkelControl(RightFootControlName));
		FeignDeathBlend = AnimNodeBlend(Mesh.FindAnimNode('FeignDeathBlend'));
		FullBodyAnimSlot = AnimNodeSlot(Mesh.FindAnimNode('FullBodySlot'));
		TopHalfAnimSlot = AnimNodeSlot(Mesh.FindAnimNode('TopHalfSlot'));

		LeftHandIK = SkelControlLimb( mesh.FindSkelControl('LeftHandIK') );
		LeftHandIKRot = SkelControlSingleBone( mesh.FindSkelControl('LeftHandIKRot') );

		RightHandIK = SkelControlLimb( mesh.FindSkelControl('RightHandIK') );
		RightHandIKRot = SkelControlSingleBone( mesh.FindSkelControl('RightHandIKRot') );

		RootRotControl = SkelControlSingleBone( mesh.FindSkelControl('RootRot') );
		AimNode = AnimNodeAimOffset( mesh.FindAnimNode('AimNode') );
		GunRecoilNode = GameSkelCtrl_Recoil( mesh.FindSkelControl('GunRecoilNode') );
		LeftRecoilNode = GameSkelCtrl_Recoil( mesh.FindSkelControl('LeftRecoilNode') );
		RightRecoilNode = GameSkelCtrl_Recoil( mesh.FindSkelControl('RightRecoilNode') );
	}
}

/** Enable or disable IK that keeps hands on IK bones. */
simulated function SetHandIKEnabled(bool bEnabled)
{
	if (Mesh.Animations != None)
	{
		if (bEnabled)
		{
			LeftHandIK.SetSkelControlStrength(1.0, 0.0);
			LeftHandIKRot.SetSkelControlStrength(1.0, 0.0);
			RightHandIK.SetSkelControlStrength(1.0, 0.0);
			RightHandIKRot.SetSkelControlStrength(1.0, 0.0);
		}
		else
		{
			LeftHandIK.SetSkelControlStrength(0.0, 0.0);
			LeftHandIKRot.SetSkelControlStrength(0.0, 0.0);
			RightHandIK.SetSkelControlStrength(0.0, 0.0);
			RightHandIKRot.SetSkelControlStrength(0.0, 0.0);
		}
	}
}

/** Util for scaling running anims etc. */
simulated function SetAnimRateScale(float RateScale)
{
	Mesh.GlobalAnimRateScale = RateScale;
}

/** Change the type of weapon animation we are playing. */
simulated function SetWeapAnimType(UTWeaponAttachment.EWeapAnimType AnimType)
{
	if (AimNode != None)
	{
		switch(AnimType)
		{
			case EWAT_Default:
				AimNode.SetActiveProfileByName('Default');
				break;
			case EWAT_Pistol:
				AimNode.SetActiveProfileByName('SinglePistol');
				break;
			case EWAT_DualPistols:
				AimNode.SetActiveProfileByName('DualPistols');
				break;
			case EWAT_ShoulderRocket:
				AimNode.SetActiveProfileByName('ShoulderRocket');
				break;
			case EWAT_Stinger:
				AimNode.SetActiveProfileByName('Stinger');
				break;
		}
	}
}

/** This will trace against the world and leave a blood splatter decal **/
simulated function LeaveABloodSplatterDecal( vector HitLoc, vector HitNorm )
{
	local DecalComponent DC;
	local DecalLifetimeDataAge LifetimePolicy;
	local MaterialInstanceConstant MIC_Decal;

	local Actor TraceActor;
	local vector out_HitLocation;
	local vector out_HitNormal;
	local vector TraceDest;
	local vector TraceStart;
	local vector TraceExtent;
	local TraceHitInfo HitInfo;
	local float RandRotation;

	TraceStart = HitLoc;
	HitNorm.Z = 0;
	TraceDest =  HitLoc  + ( HitNorm * 105 );

	RandRotation = FRand() * 360;

	TraceActor = Trace( out_HitLocation, out_HitNormal, TraceDest, TraceStart, false, TraceExtent, HitInfo, TRACEFLAG_PhysicsVolumes );

	//FlushPersistentDebugLines();
	//DrawDebugLine(TraceStart, TraceDest, 000, 000, 255, TRUE);


	if( TraceActor != None )
	{
		//`log( "DECAL" );

		LifetimePolicy = new(TraceActor.Outer) class'DecalLifetimeDataAge';
		LifetimePolicy.LifeSpan = 10.0f;

		DC = new(self) class'DecalComponent';
		DC.Width = 100;
		DC.Height = 100;
		DC.Thickness = 50;
		DC.bNoClip = FALSE;

		// we might want to move this to the UTFamilyInfo
		MIC_Decal = new(outer) class'MaterialInstanceConstant';
		// T_FX.DecalMaterials.M_FX_BloodDecal_Large01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Large02
		// T_FX.DecalMaterials.M_FX_BloodDecal_Medium01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Medium02
		// T_FX.DecalMaterials.M_FX_BloodDecal_Small01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Small02

		MIC_Decal.SetParent( MaterialInstanceConstant'T_FX.DecalMaterials.M_FX_BloodDecal_Small01' );

		DC.DecalMaterial = MIC_Decal;

		TraceActor.AddDecal( DC.DecalMaterial, out_HitLocation, rotator(-out_HitNormal), RandRotation, DC.Width, DC.Height, DC.Thickness, DC.bNoClip, HitInfo.HitComponent, LifetimePolicy, TRUE, FALSE, HitInfo.BoneName, HitInfo.Item, HitInfo.LevelIndex );

		//SetTimer( 3.0f, FALSE, 'StartDissolving' );
	}
}


/**
 * Performs an Emote command.  This is typically an action that
 * tells the bots to do something.  It is server-side only
 *
 * @Param EInfo 		The emote we are working with
 * @Param PlayerID		The ID of the player this emote is directed at.  255 = All Players
 */


function PerformEmoteCommand(EmoteInfo EInfo, int PlayerID)
{
	local array<UTPlayerReplicationInfo> PRIs;
	local UTBot Bot;
	local int i;

	// If we require a player for this command, look it up
	if ( EInfo.bRequiresPlayer )
	{
		// Itterate over the PRI array
		for (i=0;i<WorldInfo.GRI.PRIArray.Length;i++)
		{
			// If we are looking for all players or just this player and it matches
			if (PlayerID == 255 || WorldInfo.GRI.PRIArray[i].PlayerID == PlayerID)
			{
				// Only send to bots

				if ( UTBot(WorldInfo.GRI.PRIArray[i].Owner) != none )
				{

					// If we are on the same team

					if ( WorldInfo.GRI.OnSameTeam(WorldInfo.GRI.PRIArray[i], PlayerReplicationInfo) )
					{
						PRIs[PRIs.Length] = UTPlayerReplicationInfo(WorldInfo.GRI.PRIArray[i]);
					}
				}
			}
		}

		// Quick out if we didn't find targets

		if ( PRIs.Length == 0 )
		{
			return;
		}
	}
	else	// See with our own just to have the loop work
	{
		PRIs[0] = UTPlayerReplicationInfo(PlayerReplicationInfo);
	}

	// Give the command to the bot...

	for (i=0;i<PRIs.Length;i++)
	{
		Bot = UTBot(PRIs[i].Owner);
		if ( Bot != none )
		{
			`log("### Command:"@EInfo.Command@"to"@PRIs[i].PlayerName);
			Bot.SetOrders(EInfo.Command,Controller);
		}
	}
}


/** Play an emote given a category and index within that category. */
simulated function DoPlayEmote(name InEmoteTag, int InPlayerID)
{
	local UTPlayerReplicationInfo UTPRI;
	local class<UTFamilyInfo> FamilyInfo;
	local int EmoteIndex;
	local EmoteInfo EInfo;

	UTPRI = UTPlayerReplicationInfo(PlayerReplicationInfo);
	if(UTPRI != None && Health > 0)
	{
		// Find the family we belong to (if we need to)
		if( CurrFamilyInfo != none )
		{
			FamilyInfo = CurrFamilyInfo;
		}
		else
		{
			FamilyInfo = class'UTCustomChar_Data'.static.FindFamilyInfo(UTPRI.CharacterData.FamilyID);
		}

		// If we couldn't find it (or empty), use the default
		if(FamilyInfo == None)
		{
			FamilyInfo = DefaultFamily;
		}

		EmoteIndex = FamilyInfo.static.GetEmoteIndex(InEmoteTag);

		if (EmoteIndex != INDEX_None)
		{

			EInfo = FamilyInfo.default.FamilyEmotes[EmoteIndex];

			// Perform any commands associated with this Emote if authoratitive

			if (Role == ROLE_Authority && EInfo.Command != '')
			{
				PerformEmoteCommand(EInfo, InPlayerID);
			}

			// If this isn't a dedicated server, perform the emote

			if(WorldInfo.NetMode != NM_DedicatedServer)
			{
				// Found it - play emote now and exit function
				// Play the sound
				if(EInfo.EmoteSound != None)
				{
					EmoteSound.Stop();
					EmoteSound.SoundCue = EInfo.EmoteSound;
					EmoteSound.Play();
				}

				// Play the anim in correct slot
				if(EInfo.EmoteAnim != '')
				{
					if(EInfo.bTopHalfEmote)
					{
						if(TopHalfAnimSlot != None)
						{
							TopHalfAnimSlot.PlayCustomAnim(EInfo.EmoteAnim, 1.0, 0.2, 0.2, false, false);
						}
					}
					else
					{
						if(FullBodyAnimSlot != None)
						{
							FullBodyAnimSlot.PlayCustomAnim(EInfo.EmoteAnim, 1.0, 0.2, 0.2, false, false);
						}
					}
				}
			}
		}
		else
		{
			// Failed to find emote specified - give a warnings
			`log("PlayEmote: Emote"@InEmoteTag@"not found");
		}
	}
}

reliable server function ServerPlayEmote(name InEmoteTag, int InPlayerID)
{
	EmoteRepInfo.EmoteTag = InEmoteTag;
	EmoteRepInfo.bNewData = !EmoteRepInfo.bNewData;

	DoPlayEmote(InEmoteTag, InPlayerID);
	LastEmoteTime = WorldInfo.TimeSeconds;
}

exec function PlayEmote(name InEmoteTag, int InPlayerID)
{
	// If it has been long enough since the last emote, play one now
	if(WorldInfo.TimeSeconds - LastEmoteTime > MinTimeBetweenEmotes)
	{
		ServerPlayEmote(InEmoteTag, InPlayerID);
		LastEmoteTime = WorldInfo.TimeSeconds;
	}
}

function SpawnDefaultController()
{
	local UTGame Game;
	local UTBot Bot;
	local CharacterInfo EmptyBotInfo;

	Super.SpawnDefaultController();

	Game = UTGame(WorldInfo.Game);
	Bot = UTBot(Controller);
	if (Game != None && Bot != None)
	{
		Game.InitializeBot(Bot, Game.GetBotTeam(), EmptyBotInfo);
	}
}

simulated function vector WeaponBob(float BobDamping, float JumpDamping)
{
	Local Vector WBob;

	WBob = BobDamping * WalkBob;
	WBob.Z = (0.45 + 0.55 * BobDamping)*WalkBob.Z + JumpDamping *(LandBob - JumpBob);
	return WBob;
}

/** TurnOff()
Freeze pawn - stop sounds, animations, physics, weapon firing
*/
simulated function TurnOff()
{
	super.TurnOff();
	PawnAmbientSound.Stop();
}

//==============
// Encroachment
event bool EncroachingOn( actor Other )
{
	if ( (Vehicle(Other) != None) && (Weapon != None) && Weapon.IsA('UTTranslauncher') )
		return true;

	return Super.EncroachingOn(Other);
}

event EncroachedBy(Actor Other)
{
	local UTPawn P;

	// don't get telefragged by non-vehicle ragdolls and pawns feigning death
	P = UTPawn(Other);
	if (P == None || (!P.IsInState('FeigningDeath') && P.Physics != PHYS_RigidBody))
	{
		Super.EncroachedBy(Other);
	}
}

function gibbedBy(actor Other)
{
	if ( Role < ROLE_Authority )
		return;
	if ( Pawn(Other) != None )
		Died(Pawn(Other).Controller, class'UTDmgType_Encroached', Location);
	else
		Died(None, class'UTDmgType_Encroached', Location);
}

//Base change - if new base is pawn or decoration, damage based on relative mass and old velocity
// Also, non-players will jump off pawns immediately
function JumpOffPawn()
{
	Super.JumpOffPawn();
	bNoJumpAdjust = true;
	if ( UTBot(Controller) != None )
		UTBot(Controller).SetFall();
}

event Falling()
{
	if ( UTBot(Controller) != None )
		UTBot(Controller).SetFall();
}

function AddVelocity( vector NewVelocity, vector HitLocation, class<DamageType> DamageType, optional TraceHitInfo HitInfo )
{
	local bool bRagdoll;

	if (!bIgnoreForces && !IsZero(NewVelocity))
	{
		if (Physics == PHYS_Falling && UTBot(Controller) != None)
		{
			UTBot(Controller).ImpactVelocity += NewVelocity;
		}

		if ( Role == ROLE_Authority && Physics == PHYS_Walking && DrivenVehicle == None && Vehicle(Base) != None
			&& VSize(Base.Velocity) > GroundSpeed )
		{
			bRagdoll = true;
		}

		Super.AddVelocity(NewVelocity, HitLocation, DamageType, HitInfo);

		// wait until velocity is applied before sending to ragdoll so that new velocity is used to start the physics
		if (bRagdoll)
		{
			ForceRagdoll();
		}
	}
}

exec function SpiderDeath()
{
	Died(none, class'UTDmgType_SpiderMine', Location);
}
function bool Died(Controller Killer, class<DamageType> damageType, vector HitLocation)
{
	if (Super.Died(Killer, DamageType, HitLocation))
	{
		StartFallImpactTime = WorldInfo.TimeSeconds;
		bCanPlayFallingImpacts=true;
		if(ArmsMesh[0] != none)
		{
			ArmsMesh[0].SetHidden(true);
			if(ArmsOverlay[0] != none)
			{
				ArmsOverlay[0].SetHidden(true);
			}
		}
		if(ArmsMesh[1] != none)
		{
			ArmsMesh[1].SetHidden(true);
			if(ArmsOverlay[1] != none)
			{
				ArmsOverlay[1].SetHidden(true);
			}
		}
		SetPawnAmbientSound(None);
		SetWeaponAmbientSound(None);
		KilledByDamageType=damageType;
		return true;
	}
	return false;
}

simulated function StartFire(byte FireModeNum)
{
	// firing cancels feign death
	if (bFeigningDeath)
	{
		FeignDeath();
	}
	else
	{
		Super.StartFire(FireModeNum);
	}
}

function bool StopFiring()
{
	return StopWeaponFiring();
}

function bool BotFire(bool bFinished)
{
	local UTWeapon Weap;
	local UTBot Bot;

	Weap = UTWeapon(Weapon);
	if (Weap == None || (!Weap.ReadyToFire(bFinished) && !Weap.IsFiring()))
	{
		return false;
	}

	Bot = UTBot(Controller);
	if (Bot != None && Bot.ScriptedFireMode != 255)
	{
		StartFire(Bot.ScriptedFireMode);
	}
	else
	{
		StartFire(ChooseFireMode());
	}
	return true;
}

function bool StopWeaponFiring()
{
	local int i;
	local bool bResult;
	local UTWeapon UTWeap;

	UTWeap = UTWeapon(Weapon);

	if ( UTWeap != None )
	{
		if ( Weapon.IsFiring() )
		{
			UTWeap.ClientEndFire(0);
			UTWeap.ClientEndFire(1);
			UTWeap.ServerStopFire(0);
			UTWeap.ServerStopFire(1);
		}
		bResult = true;
	}

	for (i = 0; i < InvManager.PendingFire.length; i++)
	{
		if (InvManager.PendingFire[i] > 0)
		{
			bResult = true;
			InvManager.PendingFire[i] = 0;
		}
	}

	return bResult;
}

function byte ChooseFireMode()
{
	if ( UTWeapon(Weapon) != None )
	{
		return UTWeapon(Weapon).BestMode();
	}
	return 0;
}

function bool RecommendLongRangedAttack()
{
	if ( UTWeapon(Weapon) != None )
		return UTWeapon(Weapon).RecommendLongRangedAttack();
	return false;
}


function float RangedAttackTime()
{
	if ( UTWeapon(Weapon) != None )
		return UTWeapon(Weapon).RangedAttackTime();
	return 0;
}

simulated function float GetEyeHeight()
{
	if ( !IsLocallyControlled() )
		return BaseEyeHeight;
	else
		return EyeHeight;
}

function PlayVictoryAnimation();

function OnHealDamage(SeqAct_HealDamage Action)
{
	local UTSeqAct_HealDamage UTHealAction;

	UTHealAction = UTSeqAct_HealDamage(Action);
	GiveHealth(Action.HealAmount, (UTHealAction != None && UTHealAction.bSuperHeal) ? SuperHealthMax : default.Health);
}

/**
PostRenderFor()
Hook to allow pawns to render HUD overlays for themselves.
Called only if pawn was rendered this tick.
Assumes that appropriate font has already been set
@todo steve - special beacon when speaking (SpeakingBeaconTexture)
*/
simulated function PostRenderFor(PlayerController PC, Canvas Canvas, vector CameraPosition, vector CameraDir)
{
	local float TextXL, XL, YL, Dist, HealthX, HealthY;
	local vector ScreenLoc;
	local LinearColor TeamColor;
	local Color	TextColor;
	local string ScreenName;
	local UTWeapon Weap;
	local UTPlayerReplicationInfo PRI;
	local UTHUD HUD;

	// don't render for own pawn or if not rendered
	if ( (PC.ViewTarget == self) || !LocalPlayer(PC.Player).GetActorVisibility(self) )
		return;

	// must be in front of player to render HUD overlay
	if ( (CameraDir dot (Location - CameraPosition)) <= 0 )
		return;

	// only render overlay for teammates
	if ( (WorldInfo.GRI == None) || (PlayerReplicationInfo == None) || (PC.ViewTarget == None) )
	{
		return;
	}

	screenLoc = Canvas.Project(Location + GetCollisionHeight()*vect(0,0,1));
	// make sure not clipped out
	if (screenLoc.X < 0 ||
		screenLoc.X >= Canvas.ClipX ||
		screenLoc.Y < 0 ||
		screenLoc.Y >= Canvas.ClipY)
	{
		return;
	}

	PRI = UTPlayerReplicationInfo(PlayerReplicationInfo);
	if ( !WorldInfo.GRI.OnSameTeam(self, PC) )
	{
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

	Dist = VSize(CameraPosition - Location);

	if ( PC.LODDistanceFactor * Dist > TeamBeaconMaxDist )
	{
		// check whether close enough to crosshair
		if ( (Abs(screenLoc.X - 0.5*Canvas.ClipX) > 0.05 * Canvas.ClipX)
			|| (Abs(screenLoc.Y - 0.5*Canvas.ClipY) > 0.05 * Canvas.ClipY) )
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
		bPostRenderTraceSucceeded = FastTrace(Location, CameraPosition)
									|| FastTrace(Location+GetCollisionHeight()*vect(0,0,1), CameraPosition);
	}
	if ( !bPostRenderTraceSucceeded )
	{
		return;
	}

	if ( Dist > TeamBeaconPlayerInfoMaxDist )
	{
		HealthY = Canvas.ClipX * 16 * TeamBeaconPlayerInfoMaxDist/(Dist * 1024);
	}
	else
	{
		HealthY = 16 * Canvas.ClipX/1024 * (1 + 2*Square((TeamBeaconPlayerInfoMaxDist-Dist)/TeamBeaconPlayerInfoMaxDist));
	}

	class'UTHUD'.Static.GetTeamColor( GetTeamNum(), TeamColor, TextColor);

	if ( Dist < TeamBeaconPlayerInfoMaxDist )
	{
		if ( PRI != None && PRI.bBot && OnlineSubsystemCommonImpl(PC.OnlineSub) != None &&
			OnlineSubsystemCommonImpl(PC.OnlineSub).bIsUsingSpeechRecognition )
		{
			ScreenName = PRI.GetNameCallSign();
		}
		else
		{
			ScreenName = PlayerReplicationInfo.PlayerName;
		}

		Canvas.StrLen(ScreenName, TextXL, YL);
		XL = Max( TextXL, 1.5 * HealthY);
		HealthY *= 0.5;
		HealthX = XL;
	}
	else
	{
		XL = HealthY;
		YL = 0;
		HealthX = XL;
	}
	HealthX *= FMin(1.0, float(Health) / float(Default.Health));;

	Class'UTHUD'.static.DrawBackground(ScreenLoc.X-0.7*XL,ScreenLoc.Y-1.8*YL-1.8*HealthY,1.4*XL,1.8*YL+1.8*HealthY, TeamColor, Canvas);

	if ( (PlayerReplicationInfo != None) && (Dist < TeamBeaconPlayerInfoMaxDist) )
	{
		Canvas.DrawColor = TextColor;
		Canvas.SetPos(ScreenLoc.X-0.5*TextXL,ScreenLoc.Y-1.2*YL-1.4*HealthY);
		Canvas.DrawTextClipped(ScreenName, true);
	}

	if ( Dist < TeamBeaconPlayerInfoMaxDist )
	{
		Class'UTHUD'.static.DrawHealth(ScreenLoc.X-0.5*HealthX,ScreenLoc.Y-0.1*YL-1.8*HealthY, HealthX, XL, HealthY, Canvas);
	}
	else
	{
		Class'UTHUD'.static.DrawHealth(ScreenLoc.X-0.5*HealthX,ScreenLoc.Y-1.4*HealthY, HealthX, HealthY, HealthY, Canvas);
	}

	HUD = UTHUD(PC.MyHUD);
	if ( (HUD != None) && !HUD.bCrosshairOnFriendly
		&& (Abs(screenLoc.X - 0.5*Canvas.ClipX) < 0.1 * Canvas.ClipX)
		&& (screenLoc.Y <= 0.5*Canvas.ClipY) )
	{
		// check if top to bottom crosses center of screen
		screenLoc = Canvas.Project(Location - GetCollisionHeight()*vect(0,0,1));
		if ( screenLoc.Y >= 0.5*Canvas.ClipY )
		{
			HUD.bCrosshairOnFriendly = true;
		}
	}
}


simulated function FaceRotation(rotator NewRotation, float DeltaTime)
{
	if ( Physics == PHYS_Ladder )
	{
		NewRotation = OnLadder.Walldir;
	}
	else if ( (Physics == PHYS_Walking) || (Physics == PHYS_Falling) )
	{
		NewRotation.Pitch = 0;
	}
	NewRotation.Roll = Rotation.Roll;
	SetRotation(NewRotation);
}

/* UpdateEyeHeight()
* Update player eye position, based on smoothing view while moving up and down stairs, and adding view bobs for landing and taking steps.
* Called every tick only if bUpdateEyeHeight==true.
*/
event UpdateEyeHeight( float DeltaTime )
{
	local float smooth, MaxEyeHeight, OldEyeHeight, Speed2D, OldBobTime;
	local Actor HitActor;
	local vector HitLocation,HitNormal, X, Y, Z;
	local int m,n;

	if ( bTearOff )
	{
		// no eyeheight updates if dead
		EyeHeight = Default.BaseEyeheight;
		bUpdateEyeHeight = false;
		return;
	}

	if ( abs(Location.Z - OldZ) > 15 )
	{
		// if position difference too great, don't do smooth land recovery
		bJustLanded = false;
		bLandRecovery = false;
	}

	if ( !bJustLanded )
	{
		// normal walking around
		// smooth eye position changes while going up/down stairs
		smooth = FMin(0.9, 10.0 * DeltaTime/WorldInfo.TimeDilation);
		LandBob *= (1 - smooth);
		if( Physics == PHYS_Walking || Physics==PHYS_Spider || Controller.IsInState('PlayerSwimming') )
		{
			OldEyeHeight = EyeHeight;
			EyeHeight = FMax((EyeHeight - Location.Z + OldZ) * (1 - smooth) + BaseEyeHeight * smooth,
								-0.5 * CylinderComponent.CollisionHeight);
		}
		else
		    EyeHeight = EyeHeight * ( 1 - smooth) + BaseEyeHeight * smooth;
	}
	else if ( bLandRecovery )
	{
		// return eyeheight back up to full height
		smooth = FMin(0.9, 9.0 * DeltaTime);
		OldEyeHeight = EyeHeight;
		LandBob *= (1 - smooth);
		// linear interpolation at end
		if ( Eyeheight > 0.9 * BaseEyeHeight )
		{
			Eyeheight = Eyeheight + 0.15*BaseEyeheight*Smooth;  // 0.15 = (1-0.75)*0.6
		}
		else
		    EyeHeight = EyeHeight * (1 - 0.6*smooth) + BaseEyeHeight*0.6*smooth;
		if ( Eyeheight >= BaseEyeheight)
		{
			bJustLanded = false;
			bLandRecovery = false;
			Eyeheight = BaseEyeheight;
		}
	}
	else
	{
		// drop eyeheight a bit on landing
		smooth = FMin(0.65, 8.0 * DeltaTime);
		OldEyeHeight = EyeHeight;
		EyeHeight = EyeHeight * (1 - 1.5*smooth);
		LandBob += 0.08 * (OldEyeHeight - Eyeheight);
		if ( (Eyeheight < 0.25 * BaseEyeheight + 1) || (LandBob > 2.4)  )
		{
			bLandRecovery = true;
			Eyeheight = 0.25 * BaseEyeheight + 1;
		}
	}

	// don't bob if disabled, or just landed
    if( !bWeaponBob || bJustLanded || !bUpdateEyeheight )
    {
		BobTime = 0;
		WalkBob = Vect(0,0,0);
    }
	else
	{
		// add some weapon bob based on jumping
		if ( Velocity.Z > 0 )
		{
			JumpBob = FMax(-1.5, JumpBob - 0.03 * DeltaTime * FMin(Velocity.Z,300));
		}
		else
		{
			JumpBob *= (1 -  FMin(1.0, 8.0 * DeltaTime));
		}

		// Add walk bob to movement
		OldBobTime = BobTime;
		Bob = FClamp(Bob, -0.05, 0.05);

		if (Physics == PHYS_Walking )
		{
			GetAxes(Rotation,X,Y,Z);
			Speed2D = VSize(Velocity);
			if ( Speed2D < 10 )
				BobTime += 0.2 * DeltaTime;
			else
				BobTime += DeltaTime * (0.3 + 0.7 * Speed2D/GroundSpeed);
			WalkBob = Y * Bob * Speed2D * sin(8 * BobTime);
			AppliedBob = AppliedBob * (1 - FMin(1, 16 * deltatime));
			WalkBob.Z = AppliedBob;
			if ( Speed2D > 10 )
				WalkBob.Z = WalkBob.Z + 0.75 * Bob * Speed2D * sin(16 * BobTime);
		}
		else if ( Physics == PHYS_Swimming )
		{
			GetAxes(Rotation,X,Y,Z);
			BobTime += DeltaTime;
			Speed2D = Sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y);
			WalkBob = Y * Bob *  0.5 * Speed2D * sin(4.0 * BobTime);
			WalkBob.Z = Bob * 1.5 * Speed2D * sin(8.0 * BobTime);
		}
		else
		{
			BobTime = 0;
			WalkBob = WalkBob * (1 - FMin(1, 8 * deltatime));
		}
		WalkBob += X*EyeForward;

		if ( (Physics != PHYS_Walking) || (VSize(Velocity) < 10) || !IsFirstPerson() )
			return;

		m = int(0.5 * Pi + 9.0 * OldBobTime/Pi);
		n = int(0.5 * Pi + 9.0 * BobTime/Pi);

		if ( (m != n) && !bIsWalking && !bIsCrouched )
		{
			ActuallyPlayFootStepSound(0);
		}
	}

	if ( CylinderComponent.CollisionHeight - Eyeheight < 12 )
	{
		// desired eye position is abovwe collision box
		// check to make sure that viewpoint doesn't penetrate another actor
		// min clip distance 12
		HitActor = trace(HitLocation,HitNormal, Location + WalkBob + (MaxStepHeight + CylinderComponent.CollisionHeight) * vect(0,0,1),
						Location + WalkBob, true, vect(12,12,12),, TRACEFLAG_Blocking);
		MaxEyeHeight = (HitActor == None) ? CylinderComponent.CollisionHeight + MaxStepHeight : HitLocation.Z - Location.Z;
		Eyeheight = FMin(Eyeheight, MaxEyeHeight);
	}

	if ( UTBot(Controller) != None )
	{
		UTBot(Controller).AdjustView( DeltaTime );
	}

}

/* GetPawnViewLocation()
Called by PlayerController to determine camera position in first person view.  Returns
the location at which to place the camera
*/
simulated function Vector GetPawnViewLocation()
{
	if ( bUpdateEyeHeight )
		return Location + EyeHeight * vect(0,0,1) + WalkBob;
	else
		return Location + BaseEyeHeight * vect(0,0,1);
}

/* BecomeViewTarget
	Called by Camera when this actor becomes its ViewTarget */
simulated event BecomeViewTarget( PlayerController PC )
{
	local UTPlayerController UTPC;
	local UTWeapon UTWeap;
	local AnimNodeSequence WeaponAnimNode, ArmAnimNode;
	local int i;

	if (LocalPlayer(PC.Player) != None)
	{
		UTPC = UTPlayerController(PC);
		if ( UTPC != None )
		{
			SetMeshVisibility(UTPC.bBehindView);
		}
		else
		{
			SetMeshVisibility(true);
		}
		bUpdateEyeHeight = true;
		bArmsAttached=true;
		AttachComponent(ArmsMesh[0]);
		UTWeap = UTWeapon(Weapon);
		if (UTWeap != None)
		{
			if (UTWeap.bUsesOffhand)
			{
				AttachComponent(ArmsMesh[1]);
			}
			// make the arm animations copy the current weapon anim
			WeaponAnimNode = UTWeap.GetWeaponAnimNodeSeq();
			if (WeaponAnimNode != None)
			{
				for (i = 0; i < ArrayCount(ArmsMesh); i++)
				{
					if (ArmsMesh[i].bAttached)
					{
						ArmAnimNode = AnimNodeSequence(ArmsMesh[i].Animations);
						if (ArmAnimNode != None)
						{
							ArmAnimNode.SetAnim(WeaponAnimNode.AnimSeqName);
							ArmAnimNode.PlayAnim(WeaponAnimNode.bLooping, WeaponAnimNode.Rate, WeaponAnimNode.CurrentTime);
						}
					}
				}
			}
		}

	}
}

/* EndViewTarget
	Called by Camera when this actor becomes its ViewTarget */
simulated event EndViewTarget( PlayerController PC )
{
	if (LocalPlayer(PC.Player) != None)
	{
		SetMeshVisibility(true);
		bArmsAttached=false;
		DetachComponent(ArmsMesh[0]);
		DetachComponent(ArmsMesh[1]);
	}
}

simulated function SetWeaponVisibility(bool bWeaponVisible)
{
	local UTWeapon Weap;

	Weap = UTWeapon(Weapon);
	if (Weap != None)
	{
		Weap.ChangeVisibility(bWeaponVisible);
	}
}

simulated function SetWeaponAttachmentVisibility(bool bAttachmentVisible)
{
	if (CurrentWeaponAttachment != None )
	{
		CurrentWeaponAttachment.ChangeVisibility(bAttachmentVisible);
	}
}

/** sets whether or not the owner of this pawn can see it */
simulated function SetMeshVisibility(bool bVisible)
{
	local UTCarriedObject Flag;

	// Handle the main player mesh
	if (Mesh != None)
	{
		Mesh.SetOwnerNoSee(!bVisible);
	}

	SetOverlayVisibility(bVisible);

	// Handle any weapons they might have
	SetWeaponVisibility(!bVisible);

	// realign any attached flags
	foreach BasedActors(class'UTCarriedObject', Flag)
	{
		HoldGameObject(Flag);
	}
}

exec function FixedView(string VisibleMeshes)
{
	local bool bVisibleMeshes;
	local float fov;

	if (WorldInfo.NetMode == NM_Standalone)
	{
		if (VisibleMeshes != "")
		{
			bVisibleMeshes = ( VisibleMeshes ~= "yes" || VisibleMeshes~="true" || VisibleMeshes~="1" );

			if (VisibleMeshes ~= "default")
				bVisibleMeshes = !IsFirstPerson();

			SetMeshVisibility(bVisibleMeshes);
		}

		if (!bFixedView)
			CalcCamera( 0.0f, FixedViewLoc, FixedViewRot, fov );

		bFixedView = !bFixedView;
		`Log("FixedView:" @ bFixedView);
	}
}

function DeactivateSpawnProtection()
{
	if ( Role < ROLE_Authority )
		return;
	if ( !bSpawnDone )
	{
		bSpawnDone = true;
		if (WorldInfo.TimeSeconds - SpawnTime < UTGame(WorldInfo.Game).SpawnProtectionTime)
		{
			bSpawnIn = true;
			if (BodyMatColor == SpawnProtectionColor)
			{
				ClearBodyMatColor();
			}
			SpawnTime = WorldInfo.TimeSeconds - UTGame(WorldInfo.Game).SpawnProtectionTime - 1;
		}
		SpawnTime = -100000;
	}
}

function DoTranslocate(vector PrevLocation)
{
	local int TeamNum;

	if ( (PlayerReplicationInfo != None) && (PlayerReplicationInfo.Team != None) )
	{
		TeamNum = PlayerReplicationInfo.Team.TeamIndex;
	}

	PlayTeleportEffect(false, false);
	DoTranslocateOut(PrevLocation,TeamNum);
}

function PlayTeleportEffect(bool bOut, bool bSound)
{
	local int TeamNum;

	if ( (PlayerReplicationInfo != None) && (PlayerReplicationInfo.Team != None) )
	{
		TeamNum = PlayerReplicationInfo.Team.TeamIndex;
	}
	if ( !bSpawnIn && (WorldInfo.TimeSeconds - SpawnTime < UTGame(WorldInfo.Game).SpawnProtectionTime) )
	{
		bSpawnIn = true;
		SetBodyMatColor( SpawnProtectionColor, UTGame(WorldInfo.Game).SpawnProtectionTime );
		SpawnTransEffect(TeamNum);
		if (bSound)
		{
			PlaySound(SpawnSound);
		}
	}
	else
	{
		SetBodyMatColor( TranslocateColor[TeamNum], 1.0 );

		if ( bOut )
		{
			DoTranslocateOut(Location,TeamNum);
		}
		else
		{
			SpawnTransEffect(TeamNum);
		}
		if (bSound)
		{
			PlaySound(TeleportSound);
		}
	}
	Super.PlayTeleportEffect( bOut, bSound );
}

function SpawnTransEffect(int TeamNum)
{
	if (TransInEffects[0] != None)
	{
		Spawn(TransInEffects[TeamNum],self,,Location + GetCollisionHeight() * vect(0,0,0.75));
	}
}

function DoTranslocateOut(Vector PrevLocation, int TeamNum)
{
	local UTEmit_TransLocateOut TLEffect;

	if ( TransOutEffects[0] == None )
		return;

	TLEffect = UTEmit_TranslocateOut( Spawn(TransOutEffects[TeamNum], self,, PrevLocation, rotator(Location - PrevLocation)) );

	if (TLEffect != none && TLEffect.CollisionComponent != none)
	{
		TLEffect.CollisionComponent.SetActorCollision(true, false);
	}
}

simulated event StartDriving(Vehicle V)
{
	local UTWeaponPawn WeaponPawn;
	local UTWeapon UTWeap;

	Super.StartDriving(V);

	DeactivateSpawnProtection();

	UTWeap = UTWeapon(Weapon);
	if (UTWeap != None)
	{
		UTWeap.HolderEnteredVehicle();
	}

	SetWeaponVisibility(false);
	SetWeaponAmbientSound(None);

	SetTeamColor();

	if (Role == ROLE_Authority)
	{
		// if we're driving a UTWeaponPawn, fill in the DrivenWeaponPawn info for remote clients
		WeaponPawn = UTWeaponPawn(V);
		if (WeaponPawn != None && WeaponPawn.MyVehicle != None && WeaponPawn.MySeatIndex != INDEX_NONE)
		{
			DrivenWeaponPawn.BaseVehicle = WeaponPawn.MyVehicle;
			DrivenWeaponPawn.SeatIndex = WeaponPawn.MySeatIndex;
			DrivenWeaponPawn.PRI = PlayerReplicationInfo;
			bNetDirty = true; // must manually set because editing replicated struct members doesn't do so automatically
		}
	}
}

/**
 * StartDriving() and StopDriving() also called on clients
 * on transitions of DrivenVehicle variable.
 * Network: ALL
 */
simulated event StopDriving(Vehicle V)
{
	local DrivenWeaponPawnInfo EmptyWeaponPawnInfo;

	if ( Mesh != None )
	{
		Mesh.SetLightEnvironment(LightEnvironment);
	}

	// ignore on non-owning client if we still have valid DrivenWeaponPawn
	if (Role < ROLE_Authority && DrivenWeaponPawn.BaseVehicle != None && !IsLocallyControlled())
	{
		// restore DrivenVehicle reference
		DrivenVehicle = ClientSideWeaponPawn;
	}
	else
	{
		if ( (Role == ROLE_Authority) && (PlayerController(Controller) != None) && (UTVehicle(V) != None) )
			UTVehicle(V).PlayerStartTime = WorldInfo.TimeSeconds + 12;
		Super.StopDriving(V);

		// don't allow pawn to double jump on exit (if was jumping when entered)
		MultiJumpRemaining = 0;

		SetWeaponVisibility(IsFirstPerson());
		bIgnoreForces = ( (UTGame(WorldInfo.Game) != None) && UTGame(WorldInfo.Game).bDemoMode && (PlayerController(Controller) != None) );

		if (Role == ROLE_Authority)
		{
			DrivenWeaponPawn = EmptyWeaponPawnInfo;
		}
	}
}

function DoComboName( string ComboClassName );
function bool InCurrentCombo()
{
	return false;
}

//=============================================================================
// Armor interface.

/** GetShieldStrength()
returns total armor value currently held by this pawn (not including helmet).
*/
function int GetShieldStrength()
{
	return ShieldBeltArmor + VestArmor + ThighpadArmor + HelmetArmor;
}

/** AbsorbDamage()
reduce damage and remove shields based on the absorption rate.
returns the remaining armor strength.
*/
function int AbsorbDamage(out int Damage, int CurrentShieldStrength, float AbsorptionRate)
{
	local int MaxAbsorbedDamage;

	MaxAbsorbedDamage = Min(Damage * AbsorptionRate, CurrentShieldStrength);
	Damage -= MaxAbsorbedDamage;
	return CurrentShieldStrength - MaxAbsorbedDamage;
}


/** ShieldAbsorb()
returns the resultant amount of damage after shields have absorbed what they can
*/
function int ShieldAbsorb( int Damage )
{
	if ( Health <= 0 )
	{
		return damage;
	}

	// shield belt absorbs 100% of damage
	if ( ShieldBeltArmor > 0 )
	{
		bShieldAbsorb = true;
		ShieldBeltArmor = AbsorbDamage(Damage, ShieldBeltArmor, 1.0);
		if (ShieldBeltArmor == 0)
		{
			SetOverlayMaterial(None);
		}
		if ( Damage == 0 )
		{
			return 0;
		}
	}

	// vest absorbs 75% of damage
	if ( VestArmor > 0 )
	{
		bShieldAbsorb = true;
		VestArmor = AbsorbDamage(Damage, VestArmor, 0.75);
		if ( Damage == 0 )
		{
			return 0;
		}
	}

	// thighpads absorb 50% of damage
	if ( ThighpadArmor > 0 )
	{
		bShieldAbsorb = true;
		ThighpadArmor = AbsorbDamage(Damage, ThighpadArmor, 0.5);
		if ( Damage == 0 )
		{
			return 0;
		}
	}

	// helmet absorbs 20% of damage
	if ( HelmetArmor > 0 )
	{
		bShieldAbsorb = true;
		HelmetArmor = AbsorbDamage(Damage, HelmetArmor, 0.5);
		if ( Damage == 0 )
		{
			return 0;
		}
	}

	return Damage;
}

/* AdjustDamage()
adjust damage based on inventory, other attributes
*/
function AdjustDamage( out int inDamage, out Vector momentum, Controller instigatedBy, Vector hitlocation, class<DamageType> damageType, optional TraceHitInfo HitInfo )
{
	if ( bIsInvulnerable )
		inDamage = 0;

	if ( UTWeapon(Weapon) != None )
		UTWeapon(Weapon).AdjustPlayerDamage( inDamage, InstigatedBy, HitLocation, Momentum, DamageType );

	//@todo steve - what about damage to other things (vehicles, objectives, etc. - need a function called by base takedamage() or in game adjustdamage

	if( DamageType.default.bArmorStops && (inDamage > 0) )
		inDamage = ShieldAbsorb(inDamage);
}

//=============================================================================

function SetHand(EWeaponHand NewWeaponHand)
{
	WeaponHand = NewWeaponHand;
	if ( UTWeapon(Weapon) != none )
		UTWeapon(Weapon).SetHand(WeaponHand);
}

function DropFlag()
{
	if ( UTPlayerReplicationInfo(PlayerReplicationInfo)==None || !PlayerReplicationInfo.bHasFlag )
		return;

	UTPlayerReplicationInfo(PlayerReplicationInfo).GetFlag().Drop();
}

/** HoldGameObject()
* Attach GameObject to mesh.
* @param GameObj : Game object to hold
*/
simulated event HoldGameObject(UTCarriedObject UTGameObj)
{
	UTGameObj.SetHardAttach(UTGameObj.default.bHardAttach);
	UTGameObj.bIgnoreBaseRotation = UTGameObj.default.bIgnoreBaseRotation;

	if (IsFirstPerson())
	{
		UTGameObj.SetBase(self);
		UTGameObj.SetRelativeRotation(UTGameObj.GameObjRot1P);
		UTGameObj.SetRelativeLocation(UTGameObj.GameObjOffset1P);
	}
	else
	{
		if ( UTGameObj.GameObjBone3P != '' )
		{
			UTGameObj.SetBase(self,,Mesh,UTGameObj.GameObjBone3P);
		}
		else
		{
			UTGameObj.SetBase(self);
		}
		UTGameObj.SetRelativeRotation(UTGameObj.GameObjRot3P);
		UTGameObj.SetRelativeLocation(UTGameObj.GameObjOffset3P);
	}
}

function bool GiveHealth(int HealAmount, int HealMax)
{
	if (Health < HealMax)
	{
		Health = Min(HealMax, Health + HealAmount);
		return true;
	}
	return false;
}

/**
 * Overridden to return the actual player name from this Pawn's
 * PlayerReplicationInfo (PRI) if available.
 */
function String GetDebugName()
{
	// return the actual player name from the PRI if available
	if (PlayerReplicationInfo != None)
	{
		return "";
	}
	// otherwise return the formatted object name
	return GetItemName(string(self));
}

simulated event PlayFootStepSound(int FootDown)
{
	if (!IsFirstPerson())
	{
		ActuallyPlayFootstepSound(FootDown);
	}
}

/**
 * Handles actual playing of sound.  Separated from PlayFootstepSound so we can
 * ignore footstep sound notifies in first person.
 */
simulated function ActuallyPlayFootstepSound(int FootDown)
{
	local SoundCue FootSound;

	FootSound = SoundGroupClass.static.GetFootstepSound(FootDown, GetMaterialBelowFeet());
	if (FootSound != None)
	{
		PlaySound(FootSound, false, true,,, true);
	}
}

simulated function name GetMaterialBelowFeet()
{
	local vector HitLocation, HitNormal, TraceStart;
	local TraceHitInfo HitInfo;
	local UTPhysicalMaterialProperty PhysicalProperty;

	TraceStart = Location - (GetCollisionHeight() * vect(0,0,1));
	Trace(HitLocation, HitNormal, TraceStart - vect(0,0,32), TraceStart, false,, HitInfo);
	if (HitInfo.PhysMaterial != None)
	{
		PhysicalProperty = UTPhysicalMaterialProperty(HitInfo.PhysMaterial.GetPhysicalMaterialProperty(class'UTPhysicalMaterialProperty'));
		if (PhysicalProperty != None)
		{
			return PhysicalProperty.MaterialType;
		}
	}
	return '';

}
event PlayLandingSound()
{
	PlaySound(SoundGroupClass.static.GetLandSound(GetMaterialBelowFeet()));
}

event PlayJumpingSound()
{
	PlaySound(SoundGroupClass.static.GetJumpSound(GetMaterialBelowFeet()));
}

/** @return whether or not we should gib due to damage from the passed in damagetype */
simulated function bool ShouldGib(class<UTDamageType> UTDamageType)
{
	return ( !class'GameInfo'.static.UseLowGore() && (Mesh != None) && (bTearOffGibs || UTDamageType.Static.ShouldGib(self)) );
}

/** spawn a special gib for this pawn's head and sets it as the ViewTarget for any players that were viewing this pawn */
simulated function SpawnHeadGib(class<UTDamageType> UTDamageType, vector HitLocation)
{
	local UTGib Gib;
	local PlayerController PC;

	if (!bHeadGibbed)
	{
		// create separate actor for the head so it can bounce around independently
		if ( HitLocation == Location )
		{
			HitLocation = Location + vector(Rotation);
		}
		Gib = SpawnGib(CurrFamilyInfo.default.HeadGib.GibClass, CurrFamilyInfo.default.HeadGib.BoneName, UTDamageType, HitLocation);

		if (Gib != None)
		{
			Gib.SetRotation(Rotation);
			SetHeadScale(0.f);
			WorldInfo.MyEmitterPool.SpawnEmitter(HeadShotEffect, Gib.Location, rotator(vect(0,0,1)), Gib);

			foreach LocalPlayerControllers(class'PlayerController', PC)
			{
				if (PC.ViewTarget == self)
				{
					PC.SetViewTarget(Gib);

					if (UTDamageType.default.DeathCameraEffectVictim != None)
					{
						UTPlayerController(PC).ClientSpawnCameraEffect(self, UTDamageType.default.DeathCameraEffectVictim);
					}
				}
			}

			bHeadGibbed = true;
		}
	}
}

simulated function UTGib SpawnGib(class<UTGib> GibClass, name BoneName, class<UTDamageType> UTDamageType, vector HitLocation)
{
	local UTGib Gib;
	local rotator SpawnRot;
	local int SavedPitch;
	local float GibPerterbation;
	local rotator VelRotation;
	local vector X, Y, Z;

	SpawnRot = QuatToRotator(Mesh.GetBoneQuaternion(BoneName));

	// @todo fixmesteve temp workaround for gib orientation problem
	SavedPitch = SpawnRot.Pitch;
	SpawnRot.Pitch = SpawnRot.Yaw;
	SpawnRot.Yaw = SavedPitch;
	Gib = Spawn(GibClass, self,, Mesh.GetBoneLocation(BoneName), SpawnRot);

	if ( Gib != None )
	{
		// add initial impulse
		GibPerterbation = UTDamageType.default.GibPerterbation * 32768.0;
		VelRotation = rotator(Gib.Location - HitLocation);
		VelRotation.Pitch += (FRand() * 2.0 * GibPerterbation) - GibPerterbation;
		VelRotation.Yaw += (FRand() * 2.0 * GibPerterbation) - GibPerterbation;
		VelRotation.Roll += (FRand() * 2.0 * GibPerterbation) - GibPerterbation;
		GetAxes(VelRotation, X, Y, Z);

		Gib.Velocity = Velocity + Z * ( FRand() * 50 ) ;
		Gib.GibMeshComp.WakeRigidBody();
		Gib.GibMeshComp.SetRBLinearVelocity(Gib.Velocity, false);
		Gib.GibMeshComp.SetRBAngularVelocity(VRand() * 50, false);

		// let damagetype spawn any additional effects
		UTDamageType.static.SpawnGibEffects(Gib);
		Gib.LifeSpan = Gib.LifeSpan + (2.0 * FRand());
	}

	return Gib;
}

/** spawns gibs and hides the pawn's mesh */
simulated function SpawnGibs(class<UTDamageType> UTDamageType, vector HitLocation)
{
	local int i;
	local bool bSpawnHighDetail;
	local GibInfo GibInfo;

	// make sure client gibs me too
	bTearOffGibs = true;

	if ( !bGibbed )
	{
		if ( WorldInfo.NetMode == NM_DedicatedServer )
		{
			bGibbed = true;
			return;
		}

		// play sound
		if(WorldInfo.TimeSeconds - DeathTime < 1.0) // had to have died within a second or so to do a death scream.
		{
			SoundGroupClass.static.PlayGibSound(self);
		}
		SoundGroupClass.static.PlayBodyExplosion(self); // the body sounds can go off any time

		// gib particles
		if (GibExplosionTemplate != None && EffectIsRelevant(Location, false, 7000))
		{
			WorldInfo.MyEmitterPool.SpawnEmitter(GibExplosionTemplate, Location, Rotation);
		}

		// spawn all other gibs
		// NOTE: do we want to spawn all gibs?
		bSpawnHighDetail = !WorldInfo.bDropDetail && (Worldinfo.TimeSeconds - LastRenderTime < 1);
		for (i = 0; i < CurrFamilyInfo.default.Gibs.length; i++)
		{
			GibInfo = CurrFamilyInfo.default.Gibs[i];

			if ( bSpawnHighDetail || !GibInfo.bHighDetailOnly )
			{
				SpawnGib(GibInfo.GibClass, GibInfo.BoneName, UTDamageType, HitLocation);
			}
		}

		// if standalone or client, destroy here
		if (WorldInfo.NetMode != NM_DedicatedServer && WorldInfo.NetMode != NM_ListenServer)
		{
			Destroy();
		}
		else
		{
			// hide everything, turn off collision
			if (Physics == PHYS_RigidBody)
			{
				Mesh.SetHasPhysicsAssetInstance(FALSE);
				Mesh.PhysicsWeight = 0.f;
				SetPhysics(PHYS_None);
			}
			GotoState('Dying');
			SetPhysics(PHYS_None);
			SetCollision(false, false);
			//@warning: can't set bHidden - that will make us lose net relevancy to everyone
			Mesh.SetHidden(true);
			SetOverlayHidden(true);
		}

		bGibbed = true;
	}
}

/**
 * Responsible for playing any death effects, animations, etc.
 *
 * @param 	DamageType - type of damage responsible for this pawn's death
 *
 * @param	HitLoc - location of the final shot
 */
simulated function PlayDying(class<DamageType> DamageType, vector HitLoc)
{
	local vector ApplyImpulse, ShotDir;
	local TraceHitInfo HitInfo;
	local PlayerController PC;
	local bool bPlayersRagdoll;
	local class<UTDamageType> UTDamageType;
	local RB_BodyInstance HipBodyInst;
	local int HipBoneIndex;
	local matrix HipMatrix;
	local bool bUseHipSpring;

	bCanTeleport = false;
	bReplicateMovement = false;
	bTearOff = true;
	bPlayedDeath = true;
	bForcedFeignDeath = false;
	bPlayingFeignDeathRecovery = false;

	HitDamageType = DamageType; // these are replicated to other clients
	TakeHitLocation = HitLoc;

	// make sure I don't have an active weaponattachment
	CurrentWeaponAttachmentClass = None;
	WeaponAttachmentChanged();

	if ( WorldInfo.NetMode == NM_DedicatedServer )
	{
 		UTDamageType = class<UTDamageType>(DamageType);
		// tell clients whether to gib
		bTearOffGibs = (UTDamageType != None && ShouldGib(UTDamageType));
		GotoState('Dying');
		return;
	}

	// Is this the local player's ragdoll?
	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if( pc.ViewTarget == self )
		{
			if ( UTHud(pc.MyHud)!=none )
				UTHud(pc.MyHud).DisplayHit(HitLoc, 100, DamageType);
			bPlayersRagdoll = true;
			break;
		}
	}
	if ( (WorldInfo.TimeSeconds - LastRenderTime > 3) && (WorldInfo.NetMode != NM_ListenServer) && !bPlayersRagdoll )
	{
		// In low physics detail, if we were not just controlling this pawn,
		// and it has not been rendered in 3 seconds, just destroy it.
		Destroy();
		return;
	}

	UTDamageType = class<UTDamageType>(DamageType);
	if (UTDamageType != None && ShouldGib(UTDamageType))
	{
		//`log( "GIBBING!" );
		SpawnGibs(UTDamageType, HitLoc);
	}
	else
	{
		CheckHitInfo( HitInfo, Mesh, Normal(TearOffMomentum), TakeHitLocation );

		// check to see if we should do a CustomDamage Effect
		if( UTDamageType != None )
		{
			if( UTDamageType.default.bUseDamageBasedDeathEffects )
			{
				UTDamageType.static.DoCustomDamageEffects(self, UTDamageType, HitInfo, TakeHitLocation);
			}

			if (UTDamageType.default.DeathCameraEffectVictim != None)
			{
				UTPlayerController(PC).ClientSpawnCameraEffect(self, UTDamageType.default.DeathCameraEffectVictim);
			}
		}

		bBlendOutTakeHitPhysics = false;

		// Turn off hand IK when dead.
		SetHandIKEnabled(false);

		// if we had some other rigid body thing going on, cancel it
		if (Physics == PHYS_RigidBody)
		{
			//@note: Falling instead of None so Velocity/Acceleration don't get cleared
			setPhysics(PHYS_Falling);
		}

		PreRagdollCollisionComponent = CollisionComponent;
		CollisionComponent = Mesh;

		Mesh.MinDistFactorForKinematicUpdate = 0.f;

		// If we had stopped updating kinematic bodies on this character due to distance from camera, force an update of bones now.
		if( Mesh.bNotUpdatingKinematicDueToDistance )
		{
			Mesh.UpdateRBBonesFromSpaceBases(TRUE, TRUE);
		}

		Mesh.PhysicsWeight = 1.0;

		if(UTDamageType != None && UTDamageType.default.DeathAnim != '')
		{
			// Don't want to use stop player and use hip-spring if in the air (eg PHYS_Falling)
			if(Physics == PHYS_Walking && UTDamageType.default.bAnimateHipsForDeathAnim)
			{
				SetPhysics(PHYS_None);
				bUseHipSpring=true;
			}
			else
			{
				SetPhysics(PHYS_RigidBody);
			}

			Mesh.PhysicsAssetInstance.SetAllBodiesFixed(FALSE);

			// Turn on angular motors on skeleton.
			Mesh.bUpdateJointsFromAnimation = TRUE;
			Mesh.PhysicsAssetInstance.SetAllMotorsAngularPositionDrive(true, true);
			Mesh.PhysicsAssetInstance.SetAngularDriveScale(10.0f, 4.0f, 0.0f);

			// If desired, turn on hip spring to keep physics character upright
			if(bUseHipSpring)
			{
				HipBodyInst = Mesh.PhysicsAssetInstance.FindBodyInstance('b_Hips', Mesh.PhysicsAsset);
				HipBoneIndex = Mesh.MatchRefBone('b_Hips');
				HipMatrix = Mesh.GetBoneMatrix(HipBoneIndex);
				HipBodyInst.SetBoneSpringParams(DeathHipLinSpring, DeathHipLinDamp, DeathHipAngSpring, DeathHipAngDamp);
				HipBodyInst.bMakeSpringToBaseCollisionComponent = FALSE;
				HipBodyInst.EnableBoneSpring(TRUE, TRUE, HipMatrix);
				HipBodyInst.bDisableOnOverextension = TRUE;
				HipBodyInst.OverextensionThreshold = 100.f;
			}

			FullBodyAnimSlot.PlayCustomAnim(UTDamageType.default.DeathAnim, 1.0, 0.1, 0.1, false, false);
			SetTimer(0.1, true, 'DoingDeathAnim');
		}
		else
		{
			SetPhysics(PHYS_RigidBody);
			Mesh.PhysicsAssetInstance.SetAllBodiesFixed(FALSE);
			Mesh.SetBlockRigidBody(TRUE);

			if( TearOffMomentum != vect(0,0,0) )
			{
				ShotDir = normal(TearOffMomentum);
				ApplyImpulse = ShotDir * DamageType.default.KDamageImpulse;

				// If not moving downwards - give extra upward kick
				if ( Velocity.Z > -10 )
				{
					ApplyImpulse += Vect(0,0,1)*DamageType.default.KDeathUpKick;
				}
				Mesh.AddImpulse(ApplyImpulse, TakeHitLocation, HitInfo.BoneName, true);
			}
		}
		GotoState('Dying');

		if (WorldInfo.NetMode != NM_DedicatedServer && UTDamageType != None && UTDamageType.default.bSeversHead && !bDeleteMe)
		{
			SpawnHeadGib(UTDamageType, HitLoc);
		}
	}
}

simulated function DoingDeathAnim()
{
	local RB_BodyInstance HipBodyInst;
	local matrix DummyMatrix;

	// If done playing custom death anim - turn off bone motors.
	if(!FullBodyAnimSlot.bIsPlayingCustomAnim)
	{
		SetPhysics(PHYS_RigidBody);
		Mesh.PhysicsAssetInstance.SetAllMotorsAngularPositionDrive(false, false);
		HipBodyInst = Mesh.PhysicsAssetInstance.FindBodyInstance('b_Hips', Mesh.PhysicsAsset);
		HipBodyInst.EnableBoneSpring(FALSE, FALSE, DummyMatrix);
		Mesh.SetBlockRigidBody(TRUE);

		ClearTimer('DoingDeathAnim');
	}
}

simulated event Destroyed()
{
	local int i, Idx;
	local PlayerController PC;
	local array<texture> Textures;
	local Actor A;
	local class<UTFamilyInfo> FamilyInfo;

	Super.Destroyed();

	FamilyInfo = GetFamilyInfo();
	// reset all of the death skeleton textures to not be resident
	if( FamilyInfo.default.DeathMeshSkelMesh != None )
	{
		for( Idx = 0; Idx < FamilyInfo.default.DeathMeshNumMaterialsToSetResident; ++Idx )
		{
			Textures = FamilyInfo.default.DeathMeshSkelMesh.Materials[Idx].GetMaterial().GetTextures();

			for( i = 0; i < Textures.Length; ++i )
			{
				//`log( "Texture setting bForceMiplevelsToBeResident FALSE: " $ Textures[i] );
				Texture2D(Textures[i]).bForceMiplevelsToBeResident = FALSE;
			}
		}
	}

	foreach BasedActors(class'Actor', A)
	{
		A.PawnBaseDied();
	}

	// remove from local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if ( UTHUD(PC.MyHUD) != None )
		{
			UTHUD(PC.MyHUD).RemovePostRenderedActor(self);
		}
	}
}


function AddDefaultInventory()
{
	Controller.ClientSwitchToBestWeapon();
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
simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	// Handle the fixed camera

	if (bFixedView)
	{
		out_CamLoc = FixedViewLoc;
		out_CamRot = FixedViewRot;
	}
	else
	{
		if ( !IsFirstPerson() )	// Handle BehindView
		{
			CalcThirdPersonCam(fDeltaTime, out_CamLoc, out_CamRot, out_FOV);
		}
		else
		{
			// By default, we view through the Pawn's eyes..
			GetActorEyesViewPoint( out_CamLoc, out_CamRot );
		}

		if ( UTWeapon(Weapon) != none)
		{
			UTWeapon(Weapon).WeaponCalcCamera(fDeltaTime, out_CamLoc, out_CamRot);
		}
	}

	return true;
}

simulated function SetThirdPersonCamera(bool bNewBehindView)
{
	if ( bNewBehindView )
	{
		CurrentCameraScale = 1.0;
		CameraZOffset = GetCollisionHeight() + Mesh.Translation.Z;
	}
	SetMeshVisibility(bNewBehindView);
}


/** Used by PlayerController.FindGoodView() in RoundEnded State */
simulated function FindGoodEndView(PlayerController InPC, out Rotator GoodRotation)
{
	local rotator ViewRotation;
	local int tries;
	local float bestdist, newdist;
	local UTPlayerController PC;

	PC = UTPlayerController(InPC);

	bIsHero = true;
	SetHeroCam(GoodRotation);
	GoodRotation.Pitch = HeroCameraPitch;
	ViewRotation = GoodRotation;
	ViewRotation.Yaw = Rotation.Yaw + 32768 + 8192;
	if ( TryNewCamRot(PC, ViewRotation, newdist) )
	{
		GoodRotation = ViewRotation;
		return;
	}

	ViewRotation = GoodRotation;
	ViewRotation.Yaw = Rotation.Yaw + 32768 - 8192;
	if ( TryNewCamRot(PC, ViewRotation, newdist) )
	{
		GoodRotation = ViewRotation;
		return;
	}

	// failed with Hero cam
	ViewRotation.Pitch = 56000;
	tries = 0;
	bestdist = 0.0;
	CameraScale = Default.CameraScale;
	CurrentCameraScale = Default.CameraScale;
	for (tries=0; tries<16; tries++)
	{
		if ( TryNewCamRot(PC, ViewRotation, newdist) )
		{
			GoodRotation = ViewRotation;
			return;
		}

		if (newdist > bestdist)
		{
			bestdist = newdist;
			GoodRotation = ViewRotation;
		}
		ViewRotation.Yaw += 4096;
	}
}

simulated function bool TryNewCamRot(UTPlayerController PC, rotator ViewRotation, out float CamDist)
{
	local vector cameraLoc;
	local rotator cameraRot;
	local float FOVAngle;

	cameraLoc = Location;
	cameraRot = ViewRotation;
	if ( CalcThirdPersonCam(0, cameraLoc, cameraRot, FOVAngle) )
	{
		CamDist = VSize(cameraLoc - Location - vect(0,0,1)*CameraZOffset);
		return true;
	}
	CamDist = VSize(cameraLoc - Location - vect(0,0,1)*CameraZOffset);
	return false;
}

simulated function SetHeroCam(out rotator out_CamRot)
{
	CameraZOffset = BaseEyeheight;
	CameraScale = HeroCameraScale;
	CurrentCameraScale = HeroCameraScale;
}

simulated function bool CalcThirdPersonCam( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	local vector CamStart, HitLocation, HitNormal, CamDir;
	local float DesiredCameraZOffset;

	ModifyRotForDebugFreeCam(out_CamRot);

	CamStart = Location;

	if ( bIsHero )
	{
		// use "hero" cam
		SetHeroCam(out_CamRot);
	}
	else
	{
		DesiredCameraZOffset = (Health > 0) ? 1.5 * GetCollisionHeight() + Mesh.Translation.Z : 0.f;
		CameraZOffset = (fDeltaTime < 0.2) ? DesiredCameraZOffset * 5 * fDeltaTime + (1 - 5*fDeltaTime) * CameraZOffset : DesiredCameraZOffset;
	}
	CamStart.Z += CameraZOffset;
	CamDir = Vector(out_CamRot) * GetCollisionRadius() * CurrentCameraScale;

	if ( (Health <= 0) || bFeigningDeath )
	{
		// adjust camera position to make sure it's not clipping into world
		// @todo fixmesteve.  Note that you can still get clipping if FindSpot fails (happens rarely)
		FindSpot(GetCollisionExtent(),CamStart);
	}
	if (CurrentCameraScale < CameraScale)
	{
		CurrentCameraScale = FMin(CameraScale, CurrentCameraScale + 5 * FMax(CameraScale - CurrentCameraScale, 0.3)*fDeltaTime);
	}
	else if (CurrentCameraScale > CameraScale)
	{
		CurrentCameraScale = FMax(CameraScale, CurrentCameraScale - 5 * FMax(CameraScale - CurrentCameraScale, 0.3)*fDeltaTime);
	}
	if (CamDir.Z > GetCollisionHeight())
	{
		CamDir *= square(cos(out_CamRot.Pitch * 0.0000958738)); // 0.0000958738 = 2*PI/65536
	}
	out_CamLoc = CamStart - CamDir;
	if (Trace(HitLocation, HitNormal, out_CamLoc, CamStart, false, vect(12,12,12)) != None)
	{
		out_CamLoc = HitLocation;
		return false;
	}
	return true;
}

/**
 * Return world location to start a weapon fire trace from.
 *
 * @return	World location where to start weapon fire traces from
 */
simulated function Vector GetWeaponStartTraceLocation(optional Weapon CurrentWeapon)
{
	return GetPawnViewLocation();
}

//=============================================
// Jumping functionality

function bool Dodge(eDoubleClickDir DoubleClickMove)
{
	local vector X,Y,Z, TraceStart, TraceEnd, Dir, Cross, HitLocation, HitNormal;
	local Actor HitActor;
	local rotator TurnRot;

	if ( bIsCrouched || bWantsToCrouch || (Physics != PHYS_Walking && Physics != PHYS_Falling) )
		return false;

	TurnRot.Yaw = Rotation.Yaw;
	GetAxes(TurnRot,X,Y,Z);

	if ( Physics == PHYS_Falling )
	{
		if (DoubleClickMove == DCLICK_Forward)
			TraceEnd = -X;
		else if (DoubleClickMove == DCLICK_Back)
			TraceEnd = X;
		else if (DoubleClickMove == DCLICK_Left)
			TraceEnd = Y;
		else if (DoubleClickMove == DCLICK_Right)
			TraceEnd = -Y;
		TraceStart = Location - (CylinderComponent.CollisionHeight - 16)*Vect(0,0,1) + TraceEnd*(CylinderComponent.CollisionRadius-16);
		TraceEnd = TraceStart + TraceEnd*40.0;
		HitActor = Trace(HitLocation, HitNormal, TraceEnd, TraceStart, false, vect(16,16,16));

		if ( (HitActor == None) || (HitNormal.Z < -0.1) )
			 return false;
		if (  !HitActor.bWorldGeometry )
		{
			if ( !HitActor.bBlockActors )
				return false;
			if ( (Pawn(HitActor) != None) && (Vehicle(HitActor) == None) )
				return false;
		}
	}
	if (DoubleClickMove == DCLICK_Forward)
	{
		Dir = X;
		Cross = Y;
	}
	else if (DoubleClickMove == DCLICK_Back)
	{
		Dir = -1 * X;
		Cross = Y;
	}
	else if (DoubleClickMove == DCLICK_Left)
	{
		Dir = -1 * Y;
		Cross = X;
	}
	else if (DoubleClickMove == DCLICK_Right)
	{
		Dir = Y;
		Cross = X;
	}
	if ( AIController(Controller) != None )
		Cross = vect(0,0,0);
	return PerformDodge(DoubleClickMove, Dir,Cross);
}

/* BotDodge()
returns appropriate vector for dodge in direction Dir (which should be normalized)
*/
function vector BotDodge(Vector Dir)
{
	local vector Vel;

	Vel = DodgeSpeed*Dir;
	Vel.Z = DodgeSpeedZ;
	return Vel;
}

function bool PerformDodge(eDoubleClickDir DoubleClickMove, vector Dir, vector Cross)
{
	local float VelocityZ;

	if ( Physics == PHYS_Falling )
	{
		TakeFallingDamage();
	}

	bDodging = true;
	bReadyToDoubleJump = false;
	VelocityZ = Velocity.Z;
	Velocity = DodgeSpeed*Dir + (Velocity Dot Cross)*Cross;

	if ( VelocityZ < -200 )
		Velocity.Z = VelocityZ + DodgeSpeedZ;
	else
		Velocity.Z = DodgeSpeedZ;

	CurrentDir = DoubleClickMove;
	SetPhysics(PHYS_Falling);
	SoundGroupClass.Static.PlayDodgeSound(self);
	return true;
}

function DoDoubleJump( bool bUpdating )
{
	if ( !bIsCrouched && !bWantsToCrouch )
	{
		if ( !IsLocallyControlled() || AIController(Controller) != None )
		{
			MultiJumpRemaining -= 1;
		}
		Velocity.Z = JumpZ + MultiJumpBoost;
		InvManager.OwnerEvent('MultiJump');
		SetPhysics(PHYS_Falling);
		BaseEyeHeight = DoubleJumpEyeHeight;
		if (!bUpdating)
		{
			SoundGroupClass.Static.PlayDoubleJumpSound(self);
		}
	}
}

function bool DoJump( bool bUpdating )
{
	// This extra jump allows a jumping or dodging pawn to jump again mid-air
	// (via thrusters). The pawn must be within +/- DoubleJumpThreshold velocity units of the
	// apex of the jump to do this special move.
	if ( !bUpdating && CanDoubleJump()&& (Abs(Velocity.Z) < DoubleJumpThreshold) && IsLocallyControlled() )
	{
		if ( PlayerController(Controller) != None )
			PlayerController(Controller).bDoubleJump = true;
		DoDoubleJump(bUpdating);
		MultiJumpRemaining -= 1;
		return true;
	}

	if ( Super.DoJump(bUpdating) )
	{
		bReadyToDoubleJump = true;
		bDodging = false;
		if ( !bUpdating )
		    PlayJumpingSound();
		return true;
	}
	return false;
}

event Landed(vector HitNormal, actor FloorActor)
{
	local vector Impulse;

	Super.Landed(HitNormal, FloorActor);

	// adds impulses to vehicles and dynamicSMActors (e.g. KActors)
	Impulse.Z = Velocity.Z * 4.0f; // 4.0f works well for landing on a Scorpion
	if (UTVehicle(FloorActor) != None)
	{
		UTVehicle(FloorActor).Mesh.AddImpulse(Impulse, Location);
	}
	else if (DynamicSMActor(FloorActor) != None)
	{
		DynamicSMActor(FloorActor).StaticMeshComponent.AddImpulse(Impulse, Location);
	}

	if ( Velocity.Z < -200 )
	{
		OldZ = Location.Z;
		bJustLanded = bUpdateEyeHeight && Controller.LandingShake();
	}

	if (InvManager != None)
	{
		InvManager.OwnerEvent('Landed');
	}
	if ((MultiJumpRemaining < MaxMultiJump && bStopOnDoubleLanding) || bDodging || Velocity.Z < -2 * JumpZ)
	{
		// slow player down if double jump landing
		Velocity.X *= 0.1;
		Velocity.Y *= 0.1;
	}

	AirControl = DefaultAirControl;
	MultiJumpRemaining = MaxMultiJump;
	bDodging = false;
	bReadyToDoubleJump = false;
	if (UTBot(Controller) != None)
	{
		UTBot(Controller).ImpactVelocity = vect(0,0,0);
	}

	if(!bHidden)
	{
		PlayLandingSound();
	}
	if (Velocity.Z < -MaxFallSpeed)
	{
		SoundGroupClass.Static.PlayFallingDamageLandSound(self);
	}
	else if (Velocity.Z < MaxFallSpeed * -0.5)
	{
		SoundGroupClass.Static.PlayLandSound(self);
	}
	/*
	else if (Health > 0 && !bHidden && (WorldInfo.TimeSeconds - SplashTime > 0.25))
	{
		PlayLandingSound();
	}
	*/
	SetBaseEyeheight();
}

function JumpOutOfWater(vector jumpDir)
{
	bReadyToDoubleJump = true;
	bDodging = false;
	Super.JumpOutOfWater(jumpDir);
}

function bool CanDoubleJump()
{
	return ( (MultiJumpRemaining > 0) && (Physics == PHYS_Falling) && (bReadyToDoubleJump || (UTBot(Controller) != None)) );
}

function bool CanMultiJump()
{
	return ( MaxMultiJump > 0 );
}

function PlayDyingSound()
{
	SoundGroupClass.Static.PlayDyingSound(self);
}

simulated function DisplayDebug(HUD HUD, out float out_YL, out float out_YPos)
{
	local int i,j;
	local PrimitiveComponent P;
	local string s;
	local float xl,yl;

	Super.DisplayDebug(HUD, out_YL, out_YPos);

	if (HUD.ShouldDisplayDebug('twist'))
	{
		Hud.Canvas.SetDrawColor(255,255,200);
		Hud.Canvas.SetPos(4,out_YPos);
		Hud.Canvas.DrawText(""$Self$" - "@Rotation@" RootYaw:"@RootYaw@" CurrentSkelAim"@CurrentSkelAim.X@CurrentSkelAim.Y);
		out_YPos += out_YL;
	}

	if (!bComponentDebug)
		return;

	Hud.Canvas.SetDrawColor(255,255,128,255);

	for (i=0;i<Mesh.Attachments.Length;i++)
	{
	    HUD.Canvas.SetPos(4,out_YPos);

	    s = ""$Mesh.Attachments[i].Component;
		Hud.Canvas.Strlen(s,xl,yl);
		j = len(s);
		while ( xl > (Hud.Canvas.ClipX*0.5) && j>10)
		{
			j--;
			s = Right(S,j);
			Hud.Canvas.StrLen(s,xl,yl);
		}

		HUD.Canvas.DrawText("Attachment"@i@" = "@Mesh.Attachments[i].BoneName@s);
	    out_YPos += out_YL;

	    P = PrimitiveComponent(Mesh.Attachments[i].Component);
	    if (P!=None)
	    {
			HUD.Canvas.SetPos(24,out_YPos);
			HUD.Canvas.DrawText("Component = "@P.Owner@P.HiddenGame@P.bOnlyOwnerSee@P.bOwnerNoSee);
			out_YPos += out_YL;

			s = ""$P;
			Hud.Canvas.Strlen(s,xl,yl);
			j = len(s);
			while ( xl > (Hud.Canvas.ClipX*0.5) && j>10)
			{
				j--;
				s = Right(S,j);
				Hud.Canvas.StrLen(s,xl,yl);
			}

			HUD.Canvas.SetPos(24,out_YPos);
			HUD.Canvas.DrawText("Component = "@s);
			out_YPos += out_YL;
		}
	}

	out_YPos += out_YL*2;
	HUD.Canvas.SetPos(24,out_YPos);
	HUD.Canvas.DrawText("Driven Vehicle = "@DrivenVehicle);
	out_YPos += out_YL;


}

/** starts playing the given sound via the PawnAmbientSound AudioComponent and sets PawnAmbientSoundCue for replicating to clients
 *  @param NewAmbientSound the new sound to play, or None to stop any ambient that was playing
 */
simulated function SetPawnAmbientSound(SoundCue NewAmbientSound)
{
	// if the component is already playing this sound, don't restart it
	if (NewAmbientSound != PawnAmbientSound.SoundCue)
	{
		PawnAmbientSoundCue = NewAmbientSound;
		PawnAmbientSound.Stop();
		PawnAmbientSound.SoundCue = NewAmbientSound;
		if (NewAmbientSound != None)
		{
			PawnAmbientSound.Play();
		}
	}
}

simulated function SoundCue GetPawnAmbientSound()
{
	return PawnAmbientSoundCue;
}

/** starts playing the given sound via the WeaponAmbientSound AudioComponent and sets WeaponAmbientSoundCue for replicating to clients
 *  @param NewAmbientSound the new sound to play, or None to stop any ambient that was playing
 */
simulated function SetWeaponAmbientSound(SoundCue NewAmbientSound)
{
	// if the component is already playing this sound, don't restart it
	if (NewAmbientSound != WeaponAmbientSound.SoundCue)
	{
		WeaponAmbientSoundCue = NewAmbientSound;
		WeaponAmbientSound.Stop();
		WeaponAmbientSound.SoundCue = NewAmbientSound;
		if (NewAmbientSound != None)
		{
			WeaponAmbientSound.Play();
		}
	}
}

simulated function SoundCue GetWeaponAmbientSound()
{
	return WeaponAmbientSoundCue;
}

simulated function CreateOverlayArmsMesh()
{
	local int i;

	for (i = 0; i < ArrayCount(ArmsMesh); i++)
	{
		if (ArmsMesh[i] != None)
		{
			ArmsOverlay[i] = new(outer) ArmsMesh[i].Class(ArmsMesh[i]);
			if (ArmsOverlay[i] != None)
			{
				ArmsOverlay[i].bTransformFromAnimParent = 0;
				ArmsOverlay[i].CastShadow = false;
				ArmsOverlay[i].SetParentAnimComponent(ArmsMesh[i]);
				ArmsOverlay[i].SetHidden(ArmsMesh[i].HiddenGame);
			}
		}
	}
}
/**
 * Apply a given overlay material to the overlay mesh.
 *
 * @Param	NewOverlay		The material to overlay
 */
simulated function SetOverlayMaterial(MaterialInterface NewOverlay)
{
	local int i;

	// If we are authoratative, then setup replication of the new overlay
	if (Role == ROLE_Authority)
	{
		OverlayMaterialInstance = NewOverlay;
	}

	if (Mesh.SkeletalMesh != None)
	{
		if (NewOverlay != None)
		{
			for (i = 0; i < OverlayMesh.SkeletalMesh.Materials.Length; i++)
			{
				OverlayMesh.SetMaterial(i, OverlayMaterialInstance);
			}

			// attach the overlay mesh
			if (!OverlayMesh.bAttached)
			{
				AttachComponent(OverlayMesh);
			}

			if (ArmsOverlay[0] != None)
			{
				for (i = 0; i < ArmsOverlay[0].SkeletalMesh.Materials.length; i++)
				{
					ArmsOverlay[0].SetMaterial(i, OverlayMaterialInstance);
				}
				for (i = 0; i < ArmsOverlay[1].SkeletalMesh.Materials.length; i++)
				{
					ArmsOverlay[1].SetMaterial(i, OverlayMaterialInstance);
				}
				if (!ArmsOverlay[0].bAttached)
				{
					AttachComponent(ArmsOverlay[0]);
					if(UTWeapon(Weapon) != none && UTWeapon(Weapon).bUsesOffhand)
					{
						AttachComponent(ArmsOverlay[1]);
					}
				}
				ArmsOverlay[0].SetHidden(ArmsMesh[0].HiddenGame);
			}
		}
		else if (OverlayMesh.bAttached)
		{
			if (ShieldBeltArmor > 0)
			{
				// reapply shield belt overlay -- FIXME: Should we precache it?
				SetOverlayMaterial(GetShieldMaterialInstance(WorldInfo.Game.bTeamGame));
			}
			else
			{
				DetachComponent(OverlayMesh);
				if (ArmsOverlay[0] != None && ArmsOverlay[0].bAttached)
				{
					DetachComponent(ArmsOverlay[0]);
					DetachComponent(ArmsOverlay[1]);
				}
			}
		}
	}
}

/** @DEBUG */
simulated exec function TestOverlay()
{
	SetOverlayMaterial( GetShieldMaterialInstance(WorldInfo.GRI.GameClass.default.bTeamGame) );
}

/**
* @Returns the material to use for an overlay
*/
simulated function MaterialInterface GetShieldMaterialInstance(bool bTeamGame)
{
	return (bTeamGame ? default.ShieldBeltTeamMaterialInstances[GetTeamNum()] : default.ShieldBeltMaterialInstance);
}


/**
 * This function allows you to access the overlay material stack.
 *
 * @returns the requested material instance
 */
simulated function MaterialInterface GetOverlayMaterial()
{
	return OverlayMaterialInstance;
}

function SetWeaponOverlayFlag(byte FlagToSet)
{
	ApplyWeaponOverlayFlags(WeaponOverlayFlags | (1 << FlagToSet));
}

function ClearWeaponOverlayFlag(byte FlagToClear)
{
	ApplyWeaponOverlayFlags(WeaponOverlayFlags & ( 0xFF ^ (1 << FlagToClear)) );
}

/**
 * This function is a pass-through to the weapon/weapon attachment that is used to set the various overlays
 */

simulated function ApplyWeaponOverlayFlags(byte NewFlags)
{
	local UTWeapon Weap;

	if (Role == ROLE_Authority)
	{
		WeaponOverlayFlags = NewFlags;
	}

	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		Weap = UTWeapon(Weapon);
		if ( Weap != none)
		{
			Weap.SetWeaponOverlayFlags(self);
		}

		if ( CurrentWeaponAttachment != none )
		{
			CurrentWeaponAttachment.SetWeaponOverlayFlags(self);
		}
	}
}


/** called when bPlayingFeignDeathRecovery and interpolating our Mesh's PhysicsWeight to 0 has completed
 *	starts the recovery anim playing
 */
simulated event StartFeignDeathRecoveryAnim()
{
	local UTWeapon UTWeap;

	// we're done with the ragdoll, so get rid of it
	RestorePreRagdollCollisionComponent();
	Mesh.PhysicsWeight = 0.f;
	Mesh.MinDistFactorForKinematicUpdate = default.Mesh.MinDistFactorForKinematicUpdate;
	Mesh.PhysicsAssetInstance.SetAllBodiesFixed(TRUE);
	Mesh.PhysicsAssetInstance.SetFullAnimWeightBonesFixed(FALSE, Mesh);
	Mesh.SetBlockRigidBody(FALSE);
	Mesh.bUpdateKinematicBonesFromAnimation=TRUE;

	if (Physics == PHYS_RigidBody)
	{
		setPhysics(PHYS_Falling);
	}

	UTWeap = UTWeapon(Weapon);
	if (UTWeap != None)
	{
		UTWeap.PlayWeaponEquip();
	}

	if (FeignDeathBlend != None && FeignDeathBlend.Children[1].Anim != None)
	{
		FeignDeathBlend.Children[1].Anim.PlayAnim(false, 1.1);
	}
	else
	{
		// failed to find recovery node, so just pop out of ragdoll
		bNoWeaponFiring = default.bNoWeaponFiring;
		GotoState('Auto');
	}
}

/** prevents player from getting out of feign death until the body has come to rest */
function FeignDeathDelayTimer()
{
	if ( (WorldInfo.TimeSeconds - FeignDeathStartTime > 1.0)
		&& (VSize(Velocity) < FeignDeathBodyAtRestSpeed * (WorldInfo.TimeSeconds - FeignDeathStartTime + 1.0)) )
	{
		// clear timer, so we can come out of feign death
		ClearTimer('FeignDeathDelayTimer');
		// automatically get up if we were forced into it
		if (bFeigningDeath && bForcedFeignDeath)
		{
			bFeigningDeath = false;
			PlayFeignDeath();
		}
	}
}

simulated function PlayFeignDeath()
{
	local vector HitLocation, HitNormal, TraceStart, TraceEnd;
	local rotator NewRotation;
	local UTWeapon UTWeap;
	local UTVehicle V;
	local Controller Killer;

	if (bFeigningDeath)
	{
		`log("FEIGN ON"@Mesh@Mesh.BlockRigidBody@Mesh.PhysicsAssetInstance@Mesh.SkeletalMesh);

		StartFallImpactTime = WorldInfo.TimeSeconds;
		bCanPlayFallingImpacts=true;
		GotoState('FeigningDeath');

		// if we had some other rigid body thing going on, cancel it
		if (Physics == PHYS_RigidBody)
		{
			//@note: Falling instead of None so Velocity/Acceleration don't get cleared
			setPhysics(PHYS_Falling);
		}

		// Ensure we are always updating kinematic
		Mesh.MinDistFactorForKinematicUpdate = 0.0;

		Mesh.SetBlockRigidBody(TRUE);
		Mesh.ForceSkelUpdate();

		bBlendOutTakeHitPhysics = false;

		PreRagdollCollisionComponent = CollisionComponent;
		CollisionComponent = Mesh;
		SetPhysics(PHYS_RigidBody);
		Mesh.PhysicsWeight = 1.0;

		// If we had stopped updating kinematic bodies on this character due to distance from camera, force an update of bones now.
		if( Mesh.bNotUpdatingKinematicDueToDistance )
		{
			Mesh.UpdateRBBonesFromSpaceBases(TRUE, TRUE);
		}

		Mesh.PhysicsAssetInstance.SetAllBodiesFixed(FALSE);
		Mesh.bUpdateKinematicBonesFromAnimation=FALSE;

		FeignDeathStartTime = WorldInfo.TimeSeconds;
		// reset mesh translation since adjustment code isn't executed on the server
		// but the ragdoll code uses the translation so we need them to match up for the
		// most accurate simulation
		Mesh.SetTranslation(default.Mesh.Translation);
		// we'll use the rigid body collision to check for falling damage
		Mesh.ScriptRigidBodyCollisionThreshold = MaxFallSpeed;
		Mesh.SetNotifyRigidBodyCollision(true);
		Mesh.WakeRigidBody();

		if (Role == ROLE_Authority)
		{
			SetTimer(0.25, true, 'FeignDeathDelayTimer');
		}
	}
	else
	{
		// fit cylinder collision into location, crouching if necessary
		CollisionComponent = PreRagdollCollisionComponent;
		TraceStart = Location + vect(0,0,1) * GetCollisionHeight();
		TraceEnd = Location - vect(0,0,10);
		if (Trace(HitLocation, HitNormal, TraceEnd, TraceStart, true, GetCollisionExtent()) != None)
		{
			if (!SetLocation(HitLocation))
			{
				// try crouching
				ForceCrouch();
				if (Trace(HitLocation, HitNormal, TraceEnd, TraceStart, true, GetCollisionExtent()) != None)
				{
					SetLocation(HitLocation);
				}
			}
		}
		CollisionComponent = Mesh;

		bPlayingFeignDeathRecovery = true;
		FeignDeathRecoveryStartTime = WorldInfo.TimeSeconds;

		// don't need collision events anymore
		Mesh.SetNotifyRigidBodyCollision(false);
		// don't allow player to move while animation is in progress
		SetPhysics(PHYS_None);

		if (Role == ROLE_Authority)
		{
			// if cylinder is penetrating a vehicle, kill the pawn to prevent exploits
			CollisionComponent = PreRagdollCollisionComponent;
			foreach CollidingActors(class'UTVehicle', V, GetCollisionRadius(),, true)
			{
				if (IsOverlapping(V))
				{
					CollisionComponent = Mesh;
					if (V.Controller != None)
					{
						Killer = V.Controller;
					}
					else if (V.Instigator != None)
					{
						Killer = V.Instigator.Controller;
					}
					Died(Killer, V.RanOverDamageType, Location);
					return;
				}
			}
			CollisionComponent = Mesh;
		}

		// find getup animation, and freeze it at the first frame
		if (FeignDeathBlend != None)
		{
			// physics weight interpolated to 0 in C++, then StartFeignDeathRecoveryAnim() is called
			Mesh.PhysicsWeight = 1.0;
			FeignDeathBlend.SetBlendTarget(1.0, 0.0);
			// force rotation to match the body's direction so the blend to the getup animation looks more natural
			NewRotation = Rotation;
			NewRotation.Yaw = rotator(Mesh.GetBoneAxis('b_Hips', AXIS_X)).Yaw;
			// flip it around if the head is facing upwards, since the animation for that makes the character
			// end up facing in the opposite direction that its body is pointing on the ground
			// FIXME: generalize this somehow (stick it in the AnimNode, I guess...)
			if (Mesh.GetBoneAxis(HeadBone, AXIS_Y).Z < 0.0)
			{
				NewRotation.Yaw += 32768;
			}
			SetRotation(NewRotation);
		}
		else
		{
			// failed to find recovery node, so just pop out of ragdoll
			RestorePreRagdollCollisionComponent();
			Mesh.PhysicsWeight = 0.f;
			Mesh.PhysicsAssetInstance.SetAllBodiesFixed(TRUE);
			Mesh.bUpdateKinematicBonesFromAnimation=TRUE;
			Mesh.MinDistFactorForKinematicUpdate = default.Mesh.MinDistFactorForKinematicUpdate;
			Mesh.SetBlockRigidBody(FALSE);

			if (Physics == PHYS_RigidBody)
			{
				setPhysics(PHYS_Falling);
			}

			UTWeap = UTWeapon(Weapon);
			if (UTWeap != None)
			{
				UTWeap.PlayWeaponEquip();
			}
			GotoState('Auto');
		}
	}
}

reliable server function ServerFeignDeath()
{
	if (Role == ROLE_Authority && !WorldInfo.GRI.bMatchIsOver && DrivenVehicle == None && Controller != None && !bFeigningDeath)
	{
		bFeigningDeath = true;
		PlayFeignDeath();
	}
}

exec simulated function FeignDeath()
{
	ServerFeignDeath();
}

/** force the player to ragdoll, automatically getting up when the body comes to rest
 * (basically, force activate the feign death code)
 */
function ForceRagdoll()
{
	bFeigningDeath = true;
	bForcedFeignDeath = true;
	PlayFeignDeath();
}

simulated function FiringModeUpdated(bool bViaReplication)
{
	super.FiringModeUpdated(bViaReplication);
	if(CurrentWeaponAttachment != none)
	{
		CurrentWeaponAttachment.FireModeUpdated(FiringMode, bViaReplication);
	}
}

/**
  * Called by Bighead mutator when spawned, and also if bKillsAffectHead when kill someone
  */
function SetBigHead()
{
	bKillsAffectHead = true;
	if ( PlayerReplicationInfo != None )
	{
		SetHeadScale(FClamp((5+PlayerReplicationInfo.Kills)/(5+PlayerReplicationInfo.Deaths), 0.75, 2.0));
	}
}

/**
 * Check on various replicated data and act accordingly.
 */
simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'Controller' && Controller != None )
	{
		// Reset the weapon when you get the controller and
		// make sure it has ammo.
		if (UTWeapon(Weapon) != None)
		{
			UTWeapon(Weapon).ClientEndFire(0);
			UTWeapon(Weapon).ClientEndFire(1);
			if ( !Weapon.HasAnyAmmo() )
			{
				Weapon.WeaponEmpty();
			}
		}
	}
	// If CurrentWeaponAttachmentClass has changed, the player has switched weapons and
	// will need to update itself accordingly.
	else if ( VarName == 'CurrentWeaponAttachmentClass' )
	{
		WeaponAttachmentChanged();
		return;
	}
	else if ( VarName == 'ClientBodyMatDuration' )
	{
		SetBodyMatColor(BodyMatColor,ClientBodyMatDuration);
	}
	else if ( VarName == 'HeadScale' )
	{
		SetHeadScale(HeadScale);
	}
	else if (VarName == 'PawnAmbientSoundCue')
	{
		SetPawnAmbientSound(PawnAmbientSoundCue);
	}
	else if (VarName == 'WeaponAmbientSoundCue')
	{
		SetWeaponAmbientSound(WeaponAmbientSoundCue);
	}
	else if (VarName == 'ReplicatedBodyMaterial')
	{
		SetSkin(ReplicatedBodyMaterial);
	}
	else if (VarName == 'OverlayMaterialInstance')
	{
		SetOverlayMaterial(OverlayMaterialInstance);
	}
	else if (VarName == 'bFeigningDeath')
	{
		PlayFeignDeath();
	}
	else if (VarName == 'WeaponOverlayFlags')
	{
		ApplyWeaponOverlayFlags(WeaponOverlayFlags);
	}
	else if (VarName == 'LastTakeHitInfo')
	{
		PlayTakeHitEffects();
	}
	else if (VarName == 'DrivenWeaponPawn')
	{
		if (DrivenWeaponPawn.BaseVehicle != LastDrivenWeaponPawn.BaseVehicle || DrivenWeaponPawn.SeatIndex != LastDrivenWeaponPawn.SeatIndex)
		{
			if (DrivenWeaponPawn.BaseVehicle != None)
			{
				// create a client side pawn to drive
				if (ClientSideWeaponPawn == None || ClientSideWeaponPawn.bDeleteMe)
				{
					ClientSideWeaponPawn = Spawn(class'UTClientSideWeaponPawn', DrivenWeaponPawn.BaseVehicle);
				}
				ClientSideWeaponPawn.MyVehicle = DrivenWeaponPawn.BaseVehicle;
				ClientSideWeaponPawn.MySeatIndex = DrivenWeaponPawn.SeatIndex;
				StartDriving(ClientSideWeaponPawn);
			}
			else if (ClientSideWeaponPawn != None && ClientSideWeaponPawn == DrivenVehicle)
			{
				StopDriving(ClientSideWeaponPawn);
			}
		}
		if (ClientSideWeaponPawn != None && ClientSideWeaponPawn == DrivenVehicle && ClientSideWeaponPawn.PlayerReplicationInfo != DrivenWeaponPawn.PRI)
		{
			ClientSideWeaponPawn.PlayerReplicationInfo = DrivenWeaponPawn.PRI;
			ClientSideWeaponPawn.NotifyTeamChanged();
		}
		LastDrivenWeaponPawn = DrivenWeaponPawn;
	}
	else if (VarName == 'bDualWielding')
	{
		if (CurrentWeaponAttachment != None)
		{
			CurrentWeaponAttachment.SetDualWielding(bDualWielding);
		}
	}
	else if (VarName == 'bPuttingDownWeapon')
	{
		SetPuttingDownWeapon(bPuttingDownWeapon);
	}
	else if (VarName == 'EmoteRepInfo')
	{
		DoPlayEmote(EmoteRepInfo.EmoteTag, EmoteRepInfo.EmoteID);
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated event SetHeadScale(float NewScale)
{
	local SkelControlBase SkelControl;

	HeadScale = NewScale;
	SkelControl = Mesh.FindSkelControl('HeadControl');
	if (SkelControl != None)
	{
		SkelControl.BoneScale = NewScale;
	}
}

/** sets the value of bPuttingDownWeapon and plays any appropriate animations for the change */
simulated function SetPuttingDownWeapon(bool bNowPuttingDownWeapon)
{
	bPuttingDownWeapon = bNowPuttingDownWeapon;
	if (CurrentWeaponAttachment != None)
	{
		CurrentWeaponAttachment.SetPuttingDownWeapon(bPuttingDownWeapon);
	}
}

/** @return the value of bPuttingDownWeapon */
simulated function bool GetPuttingDownWeapon()
{
	return bPuttingDownWeapon;
}

/**
 * We override TakeDamage and allow the weapon to modify it
 * @See Pawn.TakeDamage
 */
event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local int OldHealth;

	// un comment this if you want to be able to test in a level and not be tossed around nor have damage effects on screen making it impossible to see what is going on
	//if( InGodMode() )
	//{
	//	return;
	//}

	// reduce rocket jumping
	if (EventInstigator == Controller)
	{
		momentum *= 0.6;
	}

	// accumulate damage taken in a single tick
	if ( AccumulationTime != WorldInfo.TimeSeconds )
	{
		AccumulateDamage = 0;
		AccumulationTime = WorldInfo.TimeSeconds;
	}
    OldHealth = Health;
	AccumulateDamage += Damage;
	Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
	AccumulateDamage = AccumulateDamage + OldHealth - Health - Damage;
}

/**
 * Called when a pawn's weapon has fired and is responsibile for
 * delegating the creation off all of the different effects.
 *
 * bViaReplication denotes if this call in as the result of the
 * flashcount/flashlocation being replicated.  It's used filter out
 * when to make the effects.
 */

simulated function WeaponFired( bool bViaReplication, optional vector HitLocation)
{
	if (CurrentWeaponAttachment != None)
	{
		if ( !IsFirstPerson() )
		{
			CurrentWeaponAttachment.ThirdPersonFireEffects(HitLocation);
		}
		else
		{
			CurrentWeaponAttachment.FirstPersonFireEffects(Weapon, HitLocation);
		}

		if ( HitLocation != Vect(0,0,0) && (WorldInfo.NetMode == NM_ListenServer || WorldInfo.NetMode == NM_Standalone || bViaReplication) )
		{
			CurrentWeaponAttachment.PlayImpactEffects(HitLocation);
		}
	}
}

simulated function WeaponStoppedFiring( bool bViaReplication )
{
	if (CurrentWeaponAttachment != None)
	{
		// always call function for both viewpoints, as during the delay between calling EndFire() on the weapon
		// and it actually stopping, we might have switched viewpoints (e.g. this commonly happens when entering a vehicle)
		CurrentWeaponAttachment.StopThirdPersonFireEffects();
		CurrentWeaponAttachment.StopFirstPersonFireEffects(Weapon);
	}
}

/**
 * Called when a weapon is changed and is responsible for making sure
 * the new weapon respects the current pawn's states/etc.
 */

simulated function WeaponChanged(UTWeapon NewWeapon)
{
	local UTSkeletalMeshComponent UTSkel;

	// Make sure the new weapon respects behindview
	if (NewWeapon.Mesh != None)
	{
		NewWeapon.Mesh.SetHidden(!IsFirstPerson());
		UTSkel = UTSkeletalMeshComponent(NewWeapon.Mesh);
		if (UTSkel != none)
		{
			ArmsMesh[0].SetFOV(UTSkel.FOV);
			ArmsMesh[1].SetFOV(UTSkel.FOV);
			ArmsMesh[0].SetScale(UTSkel.Scale);
			ArmsMesh[1].SetScale(UTSkel.Scale);
			if (ArmsOverlay[0] != None)
			{
				ArmsOverlay[0].SetScale(UTSkel.Scale);
				ArmsOverlay[1].SetScale(UTSkel.Scale);
				ArmsOverlay[0].SetFOV(UTSkel.FOV);
				ArmsOverlay[1].SetFOV(UTSkel.FOV);
			}
			NewWeapon.PlayWeaponEquip();
		}
	}
}

/**
 * Called when there is a need to change the weapon attachment (either via
 * replication or locally if controlled.
 */
simulated function WeaponAttachmentChanged()
{
	if ((CurrentWeaponAttachment == None || CurrentWeaponAttachment.Class != CurrentWeaponAttachmentClass) && Mesh.SkeletalMesh != None)
	{
		// Detach/Destroy the current attachment if we have one
		if (CurrentWeaponAttachment!=None)
		{
			CurrentWeaponAttachment.DetachFrom(Mesh);
			CurrentWeaponAttachment.Destroy();
		}

		// Create the new Attachment.
		if (CurrentWeaponAttachmentClass!=None)
		{
			CurrentWeaponAttachment = Spawn(CurrentWeaponAttachmentClass,self);
			CurrentWeaponAttachment.Instigator = self;
		}
		else
			CurrentWeaponAttachment = none;

		// If all is good, attach it to the Pawn's Mesh.
		if (CurrentWeaponAttachment != None)
		{
			CurrentWeaponAttachment.AttachTo(self);
			CurrentWeaponAttachment.SetSkin(ReplicatedBodyMaterial);
			CurrentWeaponAttachment.SetDualWielding(bDualWielding);
		}
	}
}

/**
 * Temp Code - Adjust the Weapon Attachment
 */
exec function AttachAdjustMesh(string cmd)
{
	local string c,v;
	local vector t,s,l;
	local rotator r;
	local float sc;

	c = left(Cmd,InStr(Cmd,"="));
	v = mid(Cmd,InStr(Cmd,"=")+1);

	t  = CurrentWeaponAttachment.Mesh.Translation;
	r  = CurrentWeaponAttachment.Mesh.Rotation;
	s  = CurrentWeaponAttachment.Mesh.Scale3D;
	sc = CurrentWeaponAttachment.Mesh.Scale;
	l  = CurrentWeaponAttachment.MuzzleFlashLight.Translation;

	if (c~="x")  t.X += float(v);
	if (c~="ax") t.X =  float(v);
	if (c~="y")  t.Y += float(v);
	if (c~="ay") t.Y =  float(v);
	if (c~="z")  t.Z += float(v);
	if (c~="az") t.Z =  float(v);

	if (c~="r")   R.Roll  += int(v);
	if (c~="ar")  R.Roll  =  int(v);
	if (c~="p")   R.Pitch += int(v);
	if (c~="ap")  R.Pitch =  int(v);
	if (c~="w")   R.Yaw   += int(v);
	if (c~="aw")  R.Yaw   =  int(v);

	if (c~="scalex") s.X = float(v);
	if (c~="scaley") s.Y = float(v);
	if (c~="scalez") s.Z = float(v);

	if (c~="scale") sc = float(v);

	if (c~="lx") l.X = float(v);
	if (c~="ly") l.Y = float(v);
	if (c~="lz") l.Z = float(v);

	CurrentWeaponAttachment.Mesh.SetTranslation(t);
	CurrentWeaponAttachment.Mesh.SetRotation(r);
	CurrentWeaponAttachment.Mesh.SetScale(sc);
	CurrentWeaponAttachment.Mesh.SetScale3D(s);
	CurrentWeaponAttachment.MuzzleFlashLight.SetTranslation(l);

	`log("#### AdjustMesh ####");
	`log("####    Translation :"@CurrentWeaponAttachment.Mesh.Translation);
	`log("####    Rotation    :"@CurrentWeaponAttachment.Mesh.Rotation);
	`log("####    Scale3D     :"@CurrentWeaponAttachment.Mesh.Scale3D);
	`log("####    scale       :"@CurrentWeaponAttachment.Mesh.Scale);
	`log("####	 light       :"@CurrentWeaponAttachment.MuzzleFlashLight.Translation);
}

function PlayHit(float Damage, Controller InstigatedBy, vector HitLocation, class<DamageType> damageType, vector Momentum, TraceHitInfo HitInfo)
{
	local UTPlayerController Hearer;
	local class<UTDamageType> UTDamage;

	if ( InstigatedBy != None && (class<UTDamageType>(DamageType) != None) && class<UTDamageType>(DamageType).default.bDirectDamage )
	{
		Hearer = UTPlayerController(InstigatedBy);
		if (Hearer != None)
		{
			Hearer.bAcuteHearing = true;
		}
	}

	if (WorldInfo.TimeSeconds - LastPainSound >= MinTimeBetweenPainSounds)
	{
		LastPainSound = WorldInfo.TimeSeconds;

		if (Damage > 0)
		{
			SoundGroupClass.static.PlayTakeHitSound(self, Damage);
		}
	}

	if ( Health <= 0 && PhysicsVolume.bDestructive && (WaterVolume(PhysicsVolume) != None) && (WaterVolume(PhysicsVolume).ExitActor != None) )
	{
			Spawn(WaterVolume(PhysicsVolume).ExitActor);
	}

	Super.PlayHit(Damage, InstigatedBy, HitLocation, DamageType, Momentum, HitInfo);

	if (Hearer != None)
	{
		Hearer.bAcuteHearing = false;
	}

	if (Damage > 0 || (Controller != None && Controller.bGodMode))
	{
		UTDamage = class<UTDamageType>(DamageType);

		CheckHitInfo( HitInfo, Mesh, Normal(Momentum), HitLocation );

		// play serverside effects
		if (bShieldAbsorb)
		{
			SetBodyMatColor(SpawnProtectionColor, 1.0);
			PlaySound(ArmorHitSound);
			bShieldAbsorb = false;
			return;
		}
		else
		{
			if (UTDamage != None && UTDamage.default.DamageOverlayTime > 0.0)
			{
				SetBodyMatColor(UTDamage.default.DamageBodyMatColor, UTDamage.default.DamageOverlayTime);
			}
		}

		LastTakeHitInfo.Damage = Damage;
		LastTakeHitInfo.HitLocation = HitLocation;
		LastTakeHitInfo.Momentum = Momentum;
		LastTakeHitInfo.DamageType = DamageType;
		LastTakeHitInfo.HitBone = HitInfo.BoneName;
		LastTakeHitTimeout = WorldInfo.TimeSeconds + ( (UTDamage != None) ? UTDamage.static.GetHitEffectDuration(self, Damage)
									: class'UTDamageType'.static.GetHitEffectDuration(self, Damage) );

		// play clientside effects
		PlayTakeHitEffects();
	}
}

/** plays clientside hit effects using the data in LastTakeHitInfo */
simulated function PlayTakeHitEffects()
{
	local class<UTDamageType> UTDamage;
	local vector BloodMomentum;
	local UTEmit_HitEffect HitEffect;
	local ParticleSystem BloodTemplate;

	if (EffectIsRelevant(Location, false))
	{
		if (LastTakeHitInfo.DamageType.default.bCausesBloodSplatterDecals)
		{
			if( ! IsZero(LastTakeHitInfo.Momentum) )
			{
				LeaveABloodSplatterDecal( LastTakeHitInfo.HitLocation, LastTakeHitInfo.Momentum );
			}
		}

		if (!IsFirstPerson())
		{
			if (LastTakeHitInfo.DamageType.default.bCausesBlood)
			{
				BloodTemplate = class'UTEmitter'.static.GetTemplateForDistance(BloodEffects, LastTakeHitInfo.HitLocation, WorldInfo);
				if (BloodTemplate != None)
				{
					BloodMomentum = Normal(-1.0 * LastTakeHitInfo.Momentum) + (0.5 * VRand());
					HitEffect = Spawn(BloodEmitterClass, self,, LastTakeHitInfo.HitLocation, rotator(BloodMomentum));
					HitEffect.SetTemplate(BloodTemplate);
					HitEffect.AttachTo(self, LastTakeHitInfo.HitBone);
				}
			}

			if (!Mesh.bNotUpdatingKinematicDueToDistance)
			{
				// physics based takehit animations
				UTDamage = class<UTDamageType>(LastTakeHitInfo.DamageType);
				if (UTDamage != None)
				{
					//@todo: apply impulse when in full ragdoll too (that also needs to happen on the server)
					if ( Health > 0 && DrivenVehicle == None && Physics != PHYS_RigidBody &&
						VSize(LastTakeHitInfo.Momentum) > UTDamage.default.PhysicsTakeHitMomentumThreshold )
					{
						if (Mesh.PhysicsAssetInstance != None)
						{
							// just add an impulse to the asset that's already there
							Mesh.AddImpulse(LastTakeHitInfo.Momentum, LastTakeHitInfo.HitLocation);
							// if we were already playing a take hit effect, restart it
							if (bBlendOutTakeHitPhysics)
							{
								Mesh.PhysicsWeight = 0.5;
							}
						}
						else if (Mesh.PhysicsAsset != None)
						{
							Mesh.PhysicsWeight = 0.5;
							Mesh.PhysicsAssetInstance.SetNamedBodiesFixed(true, TakeHitPhysicsFixedBones, Mesh, true);
							Mesh.AddImpulse(LastTakeHitInfo.Momentum, LastTakeHitInfo.HitLocation);
							bBlendOutTakeHitPhysics = true;
						}
					}
					UTDamage.static.SpawnHitEffect(self, LastTakeHitInfo.Damage, LastTakeHitInfo.Momentum, LastTakeHitInfo.HitBone, LastTakeHitInfo.HitLocation);
				}
			}
		}
	}
}

/** called when bBlendOutTakeHitPhysics is true and our Mesh's PhysicsWeight has reached 0.0 */
simulated event TakeHitBlendedOut()
{
	Mesh.PhysicsWeight = 0.0;
	Mesh.PhysicsAssetInstance.SetAllBodiesFixed(TRUE);
}

unreliable server function ServerHoverboard()
{
	local UTVehicle Board;

	if ( bHasHoverboard && !bIsCrouched && (DrivenVehicle == None) && !PhysicsVolume.bWaterVolume && (WorldInfo.TimeSeconds - LastHoverboardTime > MinHoverboardInterval) )
	{
		//Temp turn off collision
		SetCollision(false, false);
		Board = Spawn(HoverboardClass);
		if (Board != None)
		{
			Board.TryToDrive(self);
			LastHoverboardTime = WorldInfo.TimeSeconds;
		}
		else
		{
			SetCollision(true, true);
		}
	}
}

function OnUseHoverboard(UTSeqAct_UseHoverboard Action)
{
	bHasHoverboard = true;
	ServerHoverboard();
	Action.Hoverboard = UTVehicle(DrivenVehicle);
}

simulated function SwitchWeapon(byte NewGroup)
{
	if (NewGroup == 0 && bHasHoverboard && DrivenVehicle == None)
	{
		if ( WorldInfo.TimeSeconds - LastHoverboardTime > MinHoverboardInterval )
		{
			ServerHoverboard();
			LastHoverboardTime = WorldInfo.TimeSeconds;
		}
		return;
	}

	if (UTInventoryManager(InvManager) != None)
	{
		UTInventoryManager(InvManager).SwitchWeapon(NewGroup);
	}
}

function TakeDrowningDamage()
{
	TakeDamage(5, None, Location + GetCollisionHeight() * vect(0,0,0.5)+ 0.7 * GetCollisionRadius() * vector(Controller.Rotation), vect(0,0,0), class'UTDmgType_Drowned');
}

/** TakeHeadShot()
 * @param	Impact - impact information (where the shot hit)
 * @param	HeadShotDamageType - damagetype to use if this is a headshot
 * @param	HeadDamage - amount of damage the weapon causes if this is a headshot
 * @param	AdditionalScale - head sphere scaling for head hit determination
 * @return		true if pawn handled this as a headshot, in which case the weapon doesn't need to cause damage to the pawn.
*/
function bool TakeHeadShot(const out ImpactInfo Impact, class<UTDamageType> HeadShotDamageType, int HeadDamage, float AdditionalScale, controller InstigatingController)
{
	if(IsLocationOnHead(Impact, AdditionalScale))
	{
		if ( HelmetArmor > 0 )
		{
			HelmetArmor = 0;
			bShieldAbsorb = true;
			Spawn(class'UTEmit_HeadShotHelmet',,,Impact.HitLocation);
		}
		else
		{
			TakeDamage(HeadDamage, InstigatingController, Impact.HitLocation, Impact.RayDir, HeadShotDamageType, Impact.HitInfo);
			UTGame(WorldInfo.Game).BonusEvent('head_shot',InstigatingController.PlayerReplicationInfo);
		}
		return true;
	}
	return false;
}

function bool IsLocationOnHead(const out ImpactInfo Impact, float AdditionalScale)
{
	local vector HeadLocation;
	local float Distance;

	if (HeadBone == '')
	{
		return False;
	}

	HeadLocation = (WorldInfo.NetMode == NM_DedicatedServer) ?
	 			(Location + vect(0,0,1) * (HeadOffset + HeadHeight)) :
				(Mesh.GetBoneLocation(HeadBone) + vect(0,0,1) * HeadHeight);

	// Find distance from head location to bullet vector
	Distance = PointDistToLine(HeadLocation, Impact.RayDir, Impact.HitLocation);

	return ( Distance < (HeadRadius * HeadScale * AdditionalScale) );
}

simulated function ModifyRotForDebugFreeCam(out rotator out_CamRot)
{
	local UTPlayerController UPC;

	UPC = UTPlayerController(Controller);
	//`log(GetFuncName()@self@UPC@Controller@UPC.bDebugFreeCam@DrivenVehicle);

	if ( (UPC == None) && (DrivenVehicle != None) )
	{
		UPC = UTPlayerController(DrivenVehicle.Controller);
	}

	if (UPC != None)
	{
		if (UPC.bDebugFreeCam)
		{
	//		`log(GetFuncName()@"setting rot");
			out_CamRot = UPC.DebugFreeCamRot;
		}
	}
}

simulated function bool IsFirstPerson()
{
	local PlayerController PC;

	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if ( (PC.ViewTarget == self) && PC.UsingFirstPersonCamera() )
			return true;
	}
	return false;
}

/** moves the camera in or out one */
simulated function AdjustCameraScale(bool bMoveCameraIn)
{
	if ( !IsFirstPerson() )
	{
		CameraScale = FClamp(CameraScale + (bMoveCameraIn ? -1.0 : 1.0), CameraScaleMin, CameraScaleMax);
	}
}

simulated event rotator GetViewRotation()
{
	local rotator Result;

	//@FIXME: eventually bot Rotation.Pitch will be nonzero?
	if (UTBot(Controller) != None)
	{
		Result = Controller.Rotation;
		Result.Pitch = rotator(Controller.FocalPoint - Location).Pitch;
		return Result;
	}
	else
	{
		return Super.GetViewRotation();
	}
}

simulated event TornOff()
{
	local class<UTDamageType> UTDamage;

   	Super.TornOff();

	SetPawnAmbientSound(None);
	SetWeaponAmbientSound(None);

	UTDamage = class<UTDamageType>(HitDamageType);

	if ( UTDamage != None)
	{
		if ( UTDamage.default.DamageOverlayTime > 0 )
		{
			SetBodyMatColor(UTDamage.default.DamageBodyMatColor, UTDamage.default.DamageOverlayTime);
		}
		UTDamage.Static.PawnTornOff(self);
	}
}

simulated function SetOverlayVisibility(bool bVisible)
{
	OverlayMesh.SetOwnerNoSee(!bVisible);
}

simulated function SetOverlayHidden(bool bIsHidden)
{
	OverlayMesh.SetHidden(bIsHidden);
	if ( ArmsOverlay[0] != None )
	{
	    ArmsOverlay[0].SetHidden(bIsHidden);
	}
}

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
					const CollisionImpactData RigidCollisionData, int ContactIndex )
{
	// only check fall damage for Z axis collisions
	if (Abs(RigidCollisionData.ContactInfos[0].ContactNormal.Z) > 0.5)
	{
		Velocity = Mesh.GetRootBodyInstance().PreviousVelocity;
		TakeFallingDamage();
		// zero out the z velocity on the body now so that we don't get stacked collisions
		Velocity.Z = 0.0;
		Mesh.SetRBLinearVelocity(Velocity, false);
		Mesh.GetRootBodyInstance().PreviousVelocity = Velocity;
		Mesh.GetRootBodyInstance().Velocity = Velocity;
	}
}

/** Kismet hook for kicking a Pawn out of a vehicle */
function OnExitVehicle(UTSeqAct_ExitVehicle Action)
{
	if (DrivenVehicle != None)
	{
		DrivenVehicle.DriverLeave(true);
	}
}

/** Kismet hook for enabling/disabling infinite ammo for this Pawn */
function OnInfiniteAmmo(UTSeqAct_InfiniteAmmo Action)
{
	local UTInventoryManager UTInvManager;

	UTInvManager = UTInventoryManager(InvManager);
	if (UTInvManager != None)
	{
		UTInvManager.bInfiniteAmmo = Action.bInfiniteAmmo;
	}
}

/**
 * Toss active weapon using default settings (location+velocity).
 */
function ThrowActiveWeapon()
{
	local Weapon TossedGun;

	// to get more dual enforcer opportunities, throw out enforcer when kill player on hoverboard or with impact hammer or translocator.
	if ( Health <= 0 )
	{
		if ( (Weapon == None) || !Weapon.bCanThrow )
		{
			// throw out enforcer instead
			TossedGun = Weapon(FindInventoryType(class'UTGame.UTWeap_Enforcer'));
			if ( (TossedGun != None) && TossedGun.CanThrow() )
			{
				TossWeapon(TossedGun);
			}
			return;
		}
	}
	if ( Weapon != None )
	{
		TossWeapon(Weapon);
	}
}

function PossessedBy(Controller C, bool bVehicleTransition)
{
	Super.PossessedBy(C, bVehicleTransition);
	NotifyTeamChanged();

	// Retrieve our profile configurable properties from the PlayerController
	RetrieveSettingsFromPC();
}

/**
 * Event called from native code when Pawn stops crouching.
 * Called on non owned Pawns through bIsCrouched replication.
 * Network: ALL
 *
 * @param	HeightAdjust	height difference in unreal units between default collision height, and actual crouched cylinder height.
 */
simulated event EndCrouch(float HeightAdjust)
{
	Super.EndCrouch(HeightAdjust);

	// offset mesh by height adjustment
	if (Mesh != None)
	{
		Mesh.SetTranslation(Mesh.Translation - Vect(0,0,1)*HeightAdjust);
	}
}

/**
 * Event called from native code when Pawn starts crouching.
 * Called on non owned Pawns through bIsCrouched replication.
 * Network: ALL
 *
 * @param	HeightAdjust	height difference in unreal units between default collision height, and actual crouched cylinder height.
 */
simulated event StartCrouch(float HeightAdjust)
{
	Super.StartCrouch(HeightAdjust);

	// offset mesh by height adjustment
	if (Mesh != None)
	{
		Mesh.SetTranslation(Mesh.Translation + Vect(0,0,1)*HeightAdjust);
	}
}

state FeigningDeath
{
	ignores ServerHoverboard, SwitchWeapon, FaceRotation, ForceRagdoll, AdjustCameraScale, SetMovementPhysics;

	exec simulated function FeignDeath()
	{
		if (bFeigningDeath)
		{
			Global.FeignDeath();
		}
	}

	reliable server function ServerFeignDeath()
	{
		if (Role == ROLE_Authority && !WorldInfo.GRI.bMatchIsOver && !IsTimerActive('FeignDeathDelayTimer') && bFeigningDeath)
		{
			bFeigningDeath = false;
			PlayFeignDeath();
		}
	}

	event bool EncroachingOn(Actor Other)
	{
		// don't abort moves in ragdoll
		return false;
	}

	simulated function bool CanThrowWeapon()
	{
		return false;
	}

	simulated function Tick(float DeltaTime)
	{
		local rotator NewRotation;

		if (bPlayingFeignDeathRecovery && PlayerController(Controller) != None)
		{
			// interpolate Controller yaw to our yaw so that we don't get our rotation snapped around when we get out of feign death
			NewRotation = Controller.Rotation;
			NewRotation.Yaw = RInterpTo(NewRotation, Rotation, DeltaTime, 2.0).Yaw;
			Controller.SetRotation(NewRotation);

			if ( WorldInfo.TimeSeconds - FeignDeathRecoveryStartTime > 0.8 )
			{
				CameraScale = 1.0;
			}
		}
	}

	simulated function bool CalcThirdPersonCam( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
	{
		local vector CamStart, HitLocation, HitNormal, CamDir;

		if (CurrentCameraScale < CameraScale)
		{
			CurrentCameraScale = FMin(CameraScale, CurrentCameraScale + 5 * FMax(CameraScale - CurrentCameraScale, 0.3)*fDeltaTime);
		}
		else if (CurrentCameraScale > CameraScale)
		{
			CurrentCameraScale = FMax(CameraScale, CurrentCameraScale - 5 * FMax(CameraScale - CurrentCameraScale, 0.3)*fDeltaTime);
		}

//		CamStart = Mesh.Bounds.Origin + vect(0,0,1) * BaseEyeHeight; (Replaced with below due to Bounds being updated 2x per frame which can result in jitter-cam
		CamStart = Mesh.GetPosition() + vect(0,0,1) * BaseEyeHeight;
		CamDir = vector(out_CamRot) * GetCollisionRadius() * CurrentCameraScale;
//		`log("Mesh"@Mesh.Bounds.Origin@" --- Base Eye Height "@BaseEyeHeight);

		if (CamDir.Z > GetCollisionHeight())
		{
			CamDir *= square(cos(out_CamRot.Pitch * 0.0000958738)); // 0.0000958738 = 2*PI/65536
		}
		out_CamLoc = CamStart - CamDir;
		if (Trace(HitLocation, HitNormal, out_CamLoc, CamStart, false, vect(12,12,12)) != None)
		{
			out_CamLoc = HitLocation;
		}
		return true;
	}

	simulated event OnAnimEnd(AnimNodeSequence SeqNode, float PlayedTime, float ExcessTime)
	{
		if (Physics != PHYS_RigidBody && !bPlayingFeignDeathRecovery)
		{
			// blend out of feign death animation
			if (FeignDeathBlend != None)
			{
				FeignDeathBlend.SetBlendTarget(0.0, 0.5);
			}
			GotoState('Auto');
		}
	}

	simulated event BeginState(name PreviousStateName)
	{
		local UTPlayerController PC;
		local UTWeapon UTWeap;

		bCanPickupInventory = false;
		StopFiring();
		bNoWeaponFiring = true;

		UTWeap = UTWeapon(Weapon);
		if (UTWeap != None)
		{
			UTWeap.PlayWeaponPutDown();
		}
		if(UTWeap != none && PC != none)
		{
			UTPlayerController(Controller).EndZoom();
		}

		PC = UTPlayerController(Controller);
		if (PC != None)
		{
			PC.SetBehindView(true);
			CurrentCameraScale = 1.5;
			CameraScale = 2.25;
		}

		DropFlag();
	}

	simulated function EndState(name NextStateName)
	{
		local UTPlayerController PC;
		local UTPawn P;
		local UTWeapon UTWeap;

		if (NextStateName != 'Dying')
		{
			bNoWeaponFiring = default.bNoWeaponFiring;
			bCanPickupInventory = default.bCanPickupInventory;
			Global.SetMovementPhysics();
			PC = UTPlayerController(Controller);
			if (PC != None)
			{
				PC.SetBehindView(PC.default.bBehindView);
			}

			UTWeap = UTWeapon(Weapon);
			if (UTWeap != None)
			{
				UTWeap.PlayWeaponEquip();
			}

			CurrentCameraScale = default.CurrentCameraScale;
			CameraScale = default.CameraScale;
			bForcedFeignDeath = false;
			bPlayingFeignDeathRecovery = false;

			// jump away from other feigning death pawns to make sure we don't get stuck
			foreach TouchingActors(class'UTPawn', P)
			{
				if (P.IsInState('FeigningDeath'))
				{
					JumpOffPawn();
				}
			}
		}
	}
}

/***
SetFirstPersonDeathMeshLoc()
This code lines up the death mesh on top of the player's view. It steals liberally from UTWeapon's SetPosition.
***/
simulated event SetFirstPersonDeathMeshLoc()
{
	local vector DrawOffset, ViewOffset;
	local rotator NewRotation, FinalRotation;
	ViewOffset = Vect(0,0,0);
	// Calculate the draw offset
	if ( Controller == None )
	{
		DrawOffset = (ViewOffset >> Rotation) + GetEyeHeight() * vect(0,0,1);
	}
	else
	{
		DrawOffset.Z = GetEyeHeight();

		if ( UTPlayerController(Controller) != None )
		{
			DrawOffset += UTPlayerController(Controller).ShakeOffset >> Controller.Rotation;
		}

		DrawOffset = DrawOffset + ( ViewOffset >> Controller.Rotation );
	}

	FirstPersonDeathVisionMesh.SetTranslation(DrawOffset);

	NewRotation = (Controller == None ) ? Rotation : Controller.Rotation;
	FinalRotation = NewRotation;
	FirstPersonDeathVisionMesh.SetRotation(FinalRotation);
}

simulated State Dying
{
ignores OnAnimEnd, Bump, HitWall, HeadVolumeChange, PhysicsVolumeChange, Falling, BreathTimer, StartFeignDeathRecoveryAnim, ForceRagdoll, FellOutOfWorld;

	exec simulated function FeignDeath();
	reliable server function ServerFeignDeath();

	event bool EncroachingOn(Actor Other)
	{
		// don't abort moves in ragdoll
		return false;
	}

	event Timer()
	{
		local PlayerController PC;
		local bool bBehindAllPlayers;
		local vector ViewLocation;
		local rotator ViewRotation;

		// let the dead bodies stay if the game is over
		if (WorldInfo.GRI != None && WorldInfo.GRI.bMatchIsOver)
		{
			LifeSpan = 0.0;
			return;
		}

		if ( !PlayerCanSeeMe() )
		{
			Destroy();
			return;
		}
		// go away if not viewtarget
		//@todo steve - use drop detail, get rid of backup visibility check
		bBehindAllPlayers = true;
		ForEach LocalPlayerControllers(class'PlayerController', PC)
		{
			if ( (PC.ViewTarget == self) || (PC.ViewTarget == Base) )
			{
				if ( LifeSpan < 3.5 )
					LifeSpan = 3.5;
				SetTimer(2.0, false);
				return;
			}

			PC.GetPlayerViewPoint( ViewLocation, ViewRotation );
			if ( ((Location - ViewLocation) dot vector(ViewRotation) > 0) )
			{
				bBehindAllPlayers = false;
				break;
			}
		}
		if ( bBehindAllPlayers )
		{
			Destroy();
			return;
		}
		SetTimer(2.0, false);
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
	simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
	{
		local vector LookAt;
		if(KilledByDamageType != class'UTDmgType_SpiderMine') // checking for a special death sequence here seems hackish, I'd rather put it in
															  // the damage type itself somehow. Unfortunately I see a variety of possibilities here that would need
															  // to call global CalcCamera, which another class can't do. :-/
		{

 			CalcThirdPersonCam(fDeltaTime, out_CamLoc, out_CamRot, out_FOV);
			bStopDeathCamera = bStopDeathCamera || (out_CamLoc.Z < WorldInfo.KillZ);
			if ( bStopDeathCamera && (OldCameraPosition != vect(0,0,0)) )
			{
					// Don't allow camera to go below killz, by re-using old camera position once dead pawn falls below killz
				out_CamLoc = OldCameraPosition;
				LookAt = Location;
					CameraZOffset = (fDeltaTime < 0.2) ? (1 - 5*fDeltaTime) * CameraZOffset : 0.0;
					LookAt.Z += CameraZOffset;
				out_CamRot = rotator(LookAt - out_CamLoc);
			}
			OldCameraPosition = out_CamLoc;
			return true;
		}
		else
		{
			if(!FirstPersonDeathVisionMesh.bAttached)
			{
				AttachComponent(FirstPersonDeathVisionMesh);
				FirstPersonDeathVisionMesh.PlayAnim('FaceAttack');
			}
			GetActorEyesViewPoint( out_CamLoc, out_CamRot );
			SetFirstPersonDeathMeshLoc();
			return true;
		}
	}

	simulated event Landed(vector HitNormal, Actor FloorActor)
	{
		local vector BounceDir;

		if( Velocity.Z < -500 )
		{
			BounceDir = 0.5 * (Velocity - 2.0*HitNormal*(Velocity dot HitNormal));
			TakeDamage( (1-Velocity.Z/30), Controller, Location, BounceDir, class'DmgType_Crushed');
		}
	}

	simulated event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{
		local Vector shotDir, ApplyImpulse,BloodMomentum;
		local class<UTDamageType> UTDamage;
		local UTEmit_HitEffect HitEffect;

		if (!bGibbed && (InstigatedBy != None || EffectIsRelevant(Location, true, 0)))
		{
			UTDamage = class<UTDamageType>(DamageType);

			// accumulate damage taken in a single tick
			if ( AccumulationTime != WorldInfo.TimeSeconds )
			{
				AccumulateDamage = 0;
				AccumulationTime = WorldInfo.TimeSeconds;
			}
			AccumulateDamage += Damage;

			Health -= Damage;
			if (UTDamage != None && ShouldGib(UTDamage))
			{
				SpawnGibs(UTDamage, HitLocation);
			}
			else
			{
				CheckHitInfo( HitInfo, Mesh, Normal(Momentum), HitLocation );
				if ( UTDamage != None )
				{
					UTDamage.Static.SpawnHitEffect(self, Damage, Momentum, HitInfo.BoneName, HitLocation);
				}
				if ( DamageType.default.bCausesBlood
					&& ((PlayerController(Controller) == None) || (WorldInfo.NetMode != NM_Standalone)) )
				{
					BloodMomentum = Momentum;
					if ( BloodMomentum.Z > 0 )
						BloodMomentum.Z *= 0.5;
					HitEffect = Spawn(BloodEmitterClass,self,, HitLocation, rotator(BloodMomentum));
					HitEffect.AttachTo(Self,HitInfo.BoneName);
				}

				if ( (UTDamage != None) && (UTDamage.default.DamageOverlayTime > 0) && (UTDamage.default.DamageBodyMatColor != class'UTDamageType'.default.DamageBodyMatColor) )
				{
					SetBodyMatColor(UTDamage.default.DamageBodyMatColor, UTDamage.default.DamageOverlayTime);
				}

				if( (Physics != PHYS_RigidBody) || (Momentum == vect(0,0,0)) || (HitInfo.BoneName == '') )
					return;

				shotDir = Normal(Momentum);
				ApplyImpulse = (DamageType.Default.KDamageImpulse * shotDir);

				if( (UTDamage != None) && UTDamage.Default.bThrowRagdoll && (Velocity.Z > -10) )
				{
					ApplyImpulse += Vect(0,0,1)*DamageType.default.KDeathUpKick;
				}
				// AddImpulse() will only wake up the body for the bone we hit, so force the others to wake up
				Mesh.WakeRigidBody();
				Mesh.AddImpulse(ApplyImpulse, HitLocation, HitInfo.BoneName, true);
			}
		}
	}

	simulated function BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);
		CustomGravityScaling = 1.0;
		DeathTime = WorldInfo.TimeSeconds;
		CylinderComponent.SetActorCollision(false, false);
		if ( Mesh != None )
		{
			Mesh.SetTraceBlocking(true, true);
			Mesh.SetActorCollision(true, true);
		}

		if ( bTearOff && (WorldInfo.NetMode == NM_DedicatedServer) )
			LifeSpan = 1.0;
		else
		{
			SetTimer(2.0, false);
			LifeSpan = RagDollLifeSpan;
		}
	}
}

/** This will determine and then return the FamilyInfo for this pawn **/
simulated function class<UTFamilyInfo> GetFamilyInfo()
{
	local class<UTFamilyInfo> FamilyInfo;
	local UTPlayerReplicationInfo UTPRI;

	// Find the family we belong to (if we need to)
	if( CurrFamilyInfo != none )
	{
		FamilyInfo = CurrFamilyInfo;
	}
	else
	{
		UTPRI = UTPlayerReplicationInfo(PlayerReplicationInfo);
		if( UTPRI != None )
		{
			FamilyInfo = class'UTCustomChar_Data'.static.FindFamilyInfo(UTPRI.CharacterData.FamilyID);
		}
	}

	// If we couldn't find it (or empty), use the default
	if(FamilyInfo == None)
	{
		FamilyInfo = DefaultFamily;
	}

	return FamilyInfo;
}

exec function AngDriveSpring(float Spring)
{
	Mesh.PhysicsAssetInstance.SetAngularDriveScale(Spring, Mesh.PhysicsAssetInstance.AngularDampingScale, 0.0f);

	`log("Angular Spring scaled by "$Spring);
}

exec function AngDriveDAmp(float Damp)
{
	Mesh.PhysicsAssetInstance.SetAngularDriveScale(Mesh.PhysicsAssetInstance.AngularSpringScale, Damp, 0.0f);

	`log("Angular Damp scaled by "$Damp);
}

exec function LinDriveSpring(float Spring, float Damp)
{
	Mesh.PhysicsAssetInstance.SetLinearDriveScale(Spring, Mesh.PhysicsAssetInstance.LinearDampingScale, 0.0f);

	`log("Linear Spring scaled by "$Spring);
}

exec function LinDriveDAmp(float Spring, float Damp)
{
	Mesh.PhysicsAssetInstance.SetLinearDriveScale(Mesh.PhysicsAssetInstance.LinearSpringScale, Damp, 0.0f);

	`log("Linear Damp scaled by"$Damp);
}

exec function BackSpring(float LinSpring)
{
	local RB_BodyInstance BodyInst;
	local RB_BodySetup BodySetup;
	local int i;

	for(i=0; i<Mesh.PhysicsAsset.BodySetup.Length; i++)
	{
		BodyInst = Mesh.PhysicsAssetInstance.Bodies[i];
		BodySetup = Mesh.PhysicsAsset.BodySetup[i];

		if (BodySetup.BoneName == 'b_Spine2')
		{
			BodyInst.SetBoneSpringParams(LinSpring, BodyInst.BoneLinearDamping, 0.1*LinSpring, BodyInst.BoneAngularDamping);
		}
	}

	//`log("Hip Spring set to "$LinSpring);
}

exec function BackDamp(float LinDamp)
{
	local RB_BodyInstance BodyInst;
	local RB_BodySetup BodySetup;
	local int i;

	for(i=0; i<Mesh.PhysicsAsset.BodySetup.Length; i++)
	{
		BodyInst = Mesh.PhysicsAssetInstance.Bodies[i];
		BodySetup = Mesh.PhysicsAsset.BodySetup[i];

		if (BodySetup.BoneName == 'b_Spine2')
		{
			BodyInst.SetBoneSpringParams(BodyInst.BoneLinearSpring, LinDamp, BodyInst.BoneAngularDamping, LinDamp);
		}
	}

	//`log("Hip Damp set to "$LinDamp);
}

exec function HandSpring(float LinSpring)
{
	local RB_BodyInstance BodyInst;
	local RB_BodySetup BodySetup;
	local int i;

	for(i=0; i<Mesh.PhysicsAsset.BodySetup.Length; i++)
	{
		BodyInst = Mesh.PhysicsAssetInstance.Bodies[i];
		BodySetup = Mesh.PhysicsAsset.BodySetup[i];

		if (BodySetup.BoneName == 'b_RightHand' || BodySetup.BoneName == 'b_LeftHand')
		{
			BodyInst.SetBoneSpringParams(LinSpring, BodyInst.BoneLinearDamping, 0.1*LinSpring, BodyInst.BoneAngularDamping);
		}
	}

	//`log("Hip Spring set to "$LinSpring);
}

exec function HandDamp(float LinDamp)
{
	local RB_BodyInstance BodyInst;
	local RB_BodySetup BodySetup;
	local int i;

	for(i=0; i<Mesh.PhysicsAsset.BodySetup.Length; i++)
	{
		BodyInst = Mesh.PhysicsAssetInstance.Bodies[i];
		BodySetup = Mesh.PhysicsAsset.BodySetup[i];

		if (BodySetup.BoneName == 'b_RightHand' || BodySetup.BoneName == 'b_LeftHand')
		{
			BodyInst.SetBoneSpringParams(BodyInst.BoneLinearSpring, LinDamp, BodyInst.BoneAngularDamping, LinDamp);
		}
	}

	//`log("Hip Damp set to "$LinDamp);
}


defaultproperties
{
	Components.Remove(Sprite)

	Begin Object Class=DynamicLightEnvironmentComponent Name=MyLightEnvironment
		ModShadowFadeoutTime=1.0
	End Object
	Components.Add(MyLightEnvironment)
	LightEnvironment=MyLightEnvironment

	Begin Object Class=SkeletalMeshComponent Name=WPawnSkeletalMeshComponent
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		bOwnerNoSee=true
		CastShadow=true
		BlockRigidBody=FALSE
		bUpdateSkelWhenNotRendered=false
		bUpdateKinematicBonesFromAnimation=true
		bCastDynamicShadow=true
		Translation=(Z=2.0)
		RBChannel=RBCC_Pawn
		RBCollideWithChannels=(Default=true,Pawn=true,Vehicle=true)
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
		bOverrideAttachmentOwnerVisibility=true
		bAcceptsDecals=false
		AnimTreeTemplate=AnimTree'CH_AnimHuman_Tree.AT_CH_Human'
		bHasPhysicsAssetInstance=true
		bEnableFullAnimWeightBodies=true
		TickGroup=TG_PreAsyncWork
		MinDistFactorForKinematicUpdate=0.2
		bChartDistanceFactor=true
	End Object
	Mesh=WPawnSkeletalMeshComponent
	Components.Add(WPawnSkeletalMeshComponent)

	Begin Object Name=OverlayMeshComponent0 Class=SkeletalMeshComponent
		Scale=1.015
		bAcceptsDecals=false
		CastShadow=false
		bOwnerNoSee=true
		bUpdateSkelWhenNotRendered=false
		bUseAsOccluder=FALSE
		bOverrideAttachmentOwnerVisibility=true
		TickGroup=TG_PostAsyncWork
	End Object
	OverlayMesh=OverlayMeshComponent0

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object class=AnimNodeSequence Name=MeshSequenceB
	End Object

	Begin Object Class=AnimNodeSequence Name=MeshSequenceC
	End Object
// NEUTRALARMSCOMMENT
//	NeutralArmsAnim=AnimSet'WP_Arms.Anims.K_WP_Arms'
//	NeutralArmAnimName=NeutralPose

	Begin Object Class=UTSkeletalMeshComponent Name=FirstPersonArms
		PhysicsAsset=None
		FOV=55
		Animations=MeshSequenceA
		DepthPriorityGroup=SDPG_Foreground
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bOnlyOwnerSee=true
		bOverrideAttachmentOwnerVisibility=true
		bAcceptsDecals=false
		AbsoluteTranslation=false
		AbsoluteRotation=true
		AbsoluteScale=true
		bSyncActorLocationToRootRigidBody=false
		CastShadow=false
		TickGroup=TG_DuringASyncWork
	End Object
	ArmsMesh[0]=FirstPersonArms

	Begin Object Class=UTSkeletalMeshComponent Name=FirstPersonArms2
		PhysicsAsset=None
		FOV=55
		Scale3D=(Y=-1.0)
		Animations=MeshSequenceB
		DepthPriorityGroup=SDPG_Foreground
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bOnlyOwnerSee=true
		bOverrideAttachmentOwnerVisibility=true
		HiddenGame=true
		bAcceptsDecals=false
		AbsoluteTranslation=false
		AbsoluteRotation=true
		AbsoluteScale=true
		bSyncActorLocationToRootRigidBody=false
		CastShadow=false
		TickGroup=TG_DuringASyncWork
	End Object
	ArmsMesh[1]=FirstPersonArms2

	Begin Object Class=UTSkeletalMeshComponent Name=DeathVisionMesh
		SkeletalMesh=SkeletalMesh'Pickups.Deployables.Mesh.SK_Deployables_Spider_1P'
		FOV=55
		Scale=3.0
		Animations=MeshSequenceC
		AnimSets(0)=AnimSet'Pickups.Deployables.Anims.K_Deployables_Spider_1P'
		bOnlyOwnerSee=true
		DepthPriorityGroup=SDPG_Foreground
		AbsoluteTranslation=false
		AbsoluteRotation=true
		AbsoluteScale=true
		bSyncActorLocationToRootRigidBody=false
		CastShadow=false
	End Object
	FirstPersonDeathVisionMesh = DeathVisionMesh

	Begin Object Name=CollisionCylinder
		CollisionRadius=+0021.000000
		CollisionHeight=+0044.000000
	End Object
	CylinderComponent=CollisionCylinder

	Begin Object Class=UTAmbientSoundComponent name=AmbientSoundComponent
	End Object
	PawnAmbientSound=AmbientSoundComponent
	Components.Add(AmbientSoundComponent)

	Begin Object Class=UTAmbientSoundComponent name=AmbientSoundComponent2
	End Object
	WeaponAmbientSound=AmbientSoundComponent2
	Components.Add(AmbientSoundComponent2)

	Begin Object Class=AudioComponent name=EmoteComponent
	End Object
	EmoteSound=EmoteComponent
	Components.Add(EmoteComponent)

	ViewPitchMin=-18000
	ViewPitchMax=18000
	MaxYawAim=7000

	WalkingPct=+0.4
	CrouchedPct=+0.4
	BaseEyeHeight=38.0
	EyeHeight=38.0
	GroundSpeed=440.0
	AirSpeed=440.0
	WaterSpeed=220.0
	DodgeSpeed=600.0
	DodgeSpeedZ=295.0
	AccelRate=2048.0
	JumpZ=322.0
	CrouchHeight=29.0
	CrouchRadius=21.0
	WalkableFloorZ=0.78

	AlwaysRelevantDistanceSquared=+1960000.0
	InventoryManagerClass=class'UTInventoryManager'

	MeleeRange=+20.0
	bMuffledHearing=true

	Buoyancy=+000.99000000
	UnderWaterTime=+00020.000000
	bCanStrafe=True
	bCanSwim=true
	RotationRate=(Pitch=20000,Yaw=20000,Roll=20000)
	MaxLeanRoll=2048
	AirControl=+0.35
	DefaultAirControl=+0.35
	bStasis=false
	bCanCrouch=true
	bCanClimbLadders=True
	bCanPickupInventory=True
	bCanDoubleJump=true
	SightRadius=+12000.0

	MaxMultiJump=1
	MultiJumpRemaining=1
	MultiJumpBoost=-45.0

	SoundGroupClass=class'UTGame.UTPawnSoundGroup'
	BloodEmitterClass=class'UTGame.UTEmit_BloodSpray'

	TransInEffects(0)=class'UTEmit_TransLocateOutRed'
	TransInEffects(1)=class'UTEmit_TransLocateOut'

	TransOutEffects(0)=class'UTEmit_TransLocateOutRed'
	TransOutEffects(1)=class'UTEmit_TransLocateOut'

	MaxStepHeight=26.0
	MaxJumpHeight=49.0
	MaxDoubleJumpHeight=87.0
	DoubleJumpEyeHeight=43.0

	HeadRadius=+9.0
	HeadHeight=5.0
	HeadScale=+1.0
	HeadOffset=32

	SpawnProtectionColor=(R=50,G=50)
	TranslocateColor[0]=(R=20)
	TranslocateColor[1]=(B=20)
	DamageParameterName=DamageOverlay

	TeamBeaconMaxDist=6000.f
	TeamBeaconPlayerInfoMaxDist=3000.f
	RagdollLifespan=18.0

	ControllerClass=class'UTGame.UTBot'

	CurrentCameraScale=1.0
	CameraScale=9.0
	CameraScaleMin=3.0
	CameraScaleMax=40.0

	LeftFootControlName=LeftFootControl
	RightFootControlName=RightFootControl
	bEnableFootPlacement=true
	MaxFootPlacementDistSquared=56250000.0 // 7500 squared

	CustomGravityScaling=1.0
	SlopeBoostFriction=0.2
	bStopOnDoubleLanding=true
	DoubleJumpThreshold=160.0
	FireRateMultiplier=1.0

	ArmorHitSound=SoundCue'A_Gameplay.Gameplay.A_Gameplay_ArmorHitCue'
	SpawnSound=SoundCue'A_Gameplay.A_Gameplay_PlayerSpawn01Cue'
	TeleportSound=SoundCue'A_Weapon_Translocator.Translocator.A_Weapon_Translocator_Teleport_Cue'

	MaxFallSpeed=+1250.0
	AIMaxFallSpeedFactor=1.1 // so bots will accept a little falling damage for shorter routes
	LastPainSound=-1000.0

	FeignDeathBodyAtRestSpeed=8.0
	bReplicateRigidBodyLocation=true

	MinHoverboardInterval=0.7
	HoverboardClass=class'UTVehicle_Hoverboard'

	GibExplosionTemplate=ParticleSystem'T_FX.Effects.P_FX_gibexplode'

	FeignDeathPhysicsBlendOutSpeed=2.0
	TakeHitPhysicsBlendOutSpeed=0.5

	PawnHudScene=UTUIScene_UTPawn'UI_Scenes_HUD.HUD.UTHud_UTPawn'
	TorsoBoneName=b_Spine2
	FallImpactSound=SoundCue'A_Character_BodyImpacts.BodyImpacts.A_Character_BodyImpact_BodyFall_Cue'
	FallSpeedThreshold=125.0

	SuperHealthMax=199

	// moving here for now until we can fix up the code to have it pass in the armor object
	ShieldBeltMaterialInstance=Material'Pickups.Armor_ShieldBelt.M_ShieldBelt_Overlay'
	ShieldBeltTeamMaterialInstances(0)=Material'Pickups.Armor_ShieldBelt.M_ShieldBelt_Red'
	ShieldBeltTeamMaterialInstances(1)=Material'Pickups.Armor_ShieldBelt.M_ShieldBelt_Blue'
	ShieldBeltTeamMaterialInstances(2)=Material'Pickups.Armor_ShieldBelt.M_ShieldBelt_Red'
	ShieldBeltTeamMaterialInstances(3)=Material'Pickups.Armor_ShieldBelt.M_ShieldBelt_Blue'

	HeroCameraPitch=6000
	HeroCameraScale=6.0

	MapSize=12
	IconXStart=0.625
	IconYStart=0.0
	IconXWidth=0.125
	IConYWidth=0.125

	DefaultFamily=class'UTFamilyInfo_Ironguard_Male'
	DefaultMesh=SkeletalMesh'CH_IronGuard_Male.Mesh.SK_CH_IronGuard_MaleA'
	DefaultTeamMaterials[0]=MaterialInterface'CH_IronGuard_Male.Materials.MI_CH_IronG_Mbody01_VRed'
	DefaultTeamHeadMaterials[0]=MaterialInterface'CH_IronGuard_Male.Materials.MI_CH_IronG_MHead01_VRed'
	DefaultTeamMaterials[1]=MaterialInterface'CH_IronGuard_Male.Materials.MI_CH_IronG_Mbody01_VBlue'
	DefaultTeamHeadMaterials[1]=MaterialInterface'CH_IronGuard_Male.Materials.MI_CH_IronG_MHead01_VBlue'

	// default bone names
	WeaponSocket=WeaponPoint
	WeaponSocket2=DualWeaponPoint
	HeadBone=b_Head
	PawnEffectSockets[0]=LeftSole
	PawnEffectSockets[1]=RightSole

	BloodEffects[0]=(Template=ParticleSystem'T_FX.Effects.P_FX_Bloodhit_01_Far',MinDistance=750.0)
	BloodEffects[1]=(Template=ParticleSystem'T_FX.Effects.P_FX_Bloodhit_01_Mid',MinDistance=350.0)
	BloodEffects[2]=(Template=ParticleSystem'T_FX.Effects.P_FX_Bloodhit_01_Near',MinDistance=0.0)

	MinTimeBetweenEmotes=1.0

	Begin Object Class=ParticleSystemComponent Name=GooDeath
		Template=ParticleSystem'WP_BioRifle.Particles.P_WP_Biorifle_DeathBurn'
		bAutoActivate=false
		SecondsBeforeInactive=1.0f
	End Object
	BioBurnAway=GooDeath
	BioBurnAwayTime=3.5f
	BioEffectName=BioRifleDeathBurnTime

	DeathHipLinSpring=10000.0
	DeathHipLinDamp=500.0
	DeathHipAngSpring=10000.0
	DeathHipAngDamp=500.0

	HeadShotEffect=ParticleSystem'T_FX.Effects.P_FX_HeadShot'
}
