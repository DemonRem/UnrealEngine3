/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Hoverboard extends UTHoverVehicle
	native(Vehicle);

/** Hoverboard mesh visible attachment */
var     UTSkeletalMeshComponent HoverboardMesh;
var()   vector                  MeshLocationOffset;
var()   rotator                 MeshRotationOffset;

var()	float	JumpForceMag;

/** Sideways force when dodging (added to JumpForceMag * 0.8) */
var()	float	DodgeForceMag;

var()	float	TrickJumpWarmupMax;
var()   float   JumpCheckTraceDist;
var		float	TrickJumpWarmup;
var		float	TrickSpinWarmup;
var     float	JumpDelay, LastJumpTime;
var		bool	bInAJump; // True when in-air as the result of a jump
var		bool	bLeftGround;

var		float	TakeoffYaw;
var		float	AutoSpin;
var		float	SpinHeadingOffset;
var()	float	SpinSpeed;

var		float	LandedCountdown;

var repnotify bool bDoHoverboardJump;

/** True when requesting dodge */
var bool bIsDodging;

/** Dodge force to apply (has direction) */
var vector DodgeForce;

var		bool	bTrickJumping;
var		bool	bGrabbingBoard;
var		bool	bGrab1;
var		bool	bGrab2;
var		bool	bForceSpinWarmup;

/** Special StayUpright system based on lean */
/** Angle at which the vehicle will resist rolling */
var()      float    LeanUprightStiffness;
var()      float    LeanUprightDamping;

var editinline export	    RB_StayUprightSetup     LeanUprightConstraintSetup;
var editinline export	    RB_ConstraintInstance   LeanUprightConstraintInstance;

var editinline export		RB_ConstraintSetup		FootBoardConstraintSetup;
var editinline export		RB_ConstraintInstance	LeftFootBoardConstraintInstance;
var editinline export		RB_ConstraintInstance	RightFootBoardConstraintInstance;

var	transient vector GroundNormal;

/** The current angle (in radians) between the way the board is pointing and the way the player is looking. */
var transient float	CurrentLookYaw;

/** Controller used to turn the spine. */
var SkelControlSingleBone	SpineTurnControl;

/** Controller used to point arm at vehicle you are towed behind. */
var SkelControlLookAt		TowControl;

/** Max yaw applied to have head/spine track looking direction. */
var(HeadTracking)	float	MaxTrackYaw;

/** Used internally for limiting how quickly the head tracks. */
var	transient float	CurrentHeadYaw;

/** disable repulsors if the vehicle has negative Z velocity exceeds the Driver's MaxFallSpeed */
var bool bDisableRepulsorsAtMaxFallSpeed;

// TurnLeanFactor adjusts how much the hoverboard leans when turning
// MaxTurnLean is the maximum amount the hoverboard can lean in unreal angular units
// StrafeLean is the amount the hoverboard will lean when strafing in unreal angular units

var()   float   TurnLeanFactor;
var()	float	MaxLeanPitchSpeed;
var		transient float TargetPitch;
var()	float	DownhillDownForce;

// WaterCheckLevel is the distance to trace down to determine if water is too deep to travel over
var()   float           WaterCheckLevel;

var editinline export	    RB_DistanceJointSetup   DistanceJointSetup;
var editinline export	    RB_ConstraintInstance   DistanceJointInstance;

/** Tow Cable */
var     bool					bInTow;
var()   float                   MaxTowDistance;

struct native TowInfoData
{
	/** The vehicle we are attached to */
	var UTVehicle 	TowTruck;
	var Name		TowAttachPoint;
};

var repnotify TowInfoData TowInfo;

/** Location for towing constraint in vehicle space. */
var vector	TowLocalAttachPos;

/** When being towed, amount of steering you can do. */
var float	HoverboardTowSteerMaxAngle;

/** Used internally - current length of tow 'rope'. */
var	const float	CurrentTowDistance;

/** Controls how quickly length of tow rope changes when holding throttle. */
var float	TowDistanceChangeSpeed;

/** Time that tow line has to be blocked before it breaks. */
var float	TowLineBlockedBreakTime;

/** How long the tow line has been blocked for so far. */
var	float	TowLineBlockedFor;

/** The emitter that shows the attachment */
var ParticleSystemComponent TowBeamEmitter;

/** The emitter on the end point of the towbeam*/
var ParticleSystemComponent TowBeamEndPointEffect;

/** Emitter under the board making dust. */
var ParticleSystemComponent HoverboardDust;
var ParticleSystem			RedDustEffect;
var ParticleSystem			BlueDustEffect;

var	ParticleSystemComponent	ThrusterEffect;
var name					ThrusterEffectSocket;
var ParticleSystem			RedThrusterEffect;
var ParticleSystem			BlueThrusterEffect;

/** The reference particle systems for the beam emitter (per team) */
var ParticleSystem TowBeamTeamEmitters[2];

/** The reference particle systems for the end point emitter (per team) */
var ParticleSystem TowBeamTeamEndPoints[2];
/** parameter for tow beam endpoint */
var name TowBeamEndParameterName;

/** Sounds */
var AudioComponent  CurveSound;
var SoundCue        EngineThrustSound;
var SoundCue        TurnSound;
var SoundCue        JumpSound;

var AudioComponent TowLoopComponent;
var SoundCue TowLoopCue;
var SoundCue TowStartedSound;
var SoundCue TowEndedSound;
/** camera smooth out */
var float CameraInitialOut;

var(HoverboardCam)	vector	HoverCamOffset;
var(HoverboardCam)	rotator	HoverCamRotOffset;
var(HoverboardCam)	vector	VelLookAtOffset;
var(HoverboardCam)	vector	VelBasedCamOffset;
var(HoverboardCam)	float	VelRollFactor;
var(HoverboardCam)	float	HoverCamMaxVelUsed;
var(HoverboardCam)	float	ViewRollRate;
var					int		CurrentViewRoll;

var float	TargetPhysicsWeight;
var float	PhysWeightBlendTimeToGo;

var() float	PhysWeightBlendTime;

/** Save Doubleclick move from player */
var eDoubleClickDir	DoubleClickMove;

/** hoverboard handle mesh, attached seperately to driver */
var StaticMeshComponent HandleMesh;

/** How much falling damage player can take on hoverboard without ragdolling */
var int FallingDamageRagdollThreshold;

/** set when ragdolling, so DriverLeave() doesn't try to stick driver in another vehicle */
var bool bRagdolling;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );

#if WITH_NOVODEX
	virtual void PostInitRigidBody(NxActor* nActor, NxActorDesc& ActorDesc, UPrimitiveComponent* PrimComp);
#endif // WITH_NOVODEX
}

replication
{
	if (Role == ROLE_Authority)
		TowInfo, bGrab1, bGrab2;
	if (!bNetOwner && Role == ROLE_Authority)
		bDoHoverboardJump, bForceSpinWarmup, bTrickJumping;
}

/** Used by PlayerController.FindGoodView() in RoundEnded State */
simulated function FindGoodEndView(PlayerController InPC, out Rotator GoodRotation)
{
	if ( UTPawn(Driver) != None )
	{
		Driver.FindGoodEndView(InPC, GoodRotation);
	}
	else
	{
		super.FindGoodEndView(InPC, GoodRotation);
	}
}

simulated function float GetDisplayedHealth()
{
	return (Driver != None) ? Driver.Health : Health;
}

/**
  * returns the camera focus position (without camera lag)
  */
simulated function vector GetCameraFocus(int SeatIndex)
{
	return (Driver != None) ? Driver.Mesh.GetBoneLocation(Seats[SeatIndex].CameraTag) : Location;
}

function bool AnySeatAvailable()
{
	return (Driver != none) ? false : super.AnySeatAvailable();
}

/**
  * Called from TickSpecial().  Doubleclick detected, so pick dodge direction
  */
event RequestDodge()
{
	if (DoubleClickMove == DCLICK_Right)
	{
		Rise = 1.0;
		bIsDodging = TRUE;
		ServerRequestDodge(false);
	}
	else if (DoubleClickMove == DCLICK_Left)
	{
		Rise = 1.0;
		bIsDodging = TRUE;
		ServerRequestDodge(true);
	}
}

/**
  * Server adds dodge force
  * @PARAM bDodgeLeft is true if dodging left, false if dodging right
  */
unreliable server function ServerRequestDodge(bool bDodgeLeft)
{
	local vector X,Y,Z;

	bIsDodging = true;

	GetAxes(Rotation, X, Y, Z);
	Y.Z = 0.0;
	Rise = 1.0;
	DodgeForce = DodgeForceMag * Normal(Y);
	if ( bDodgeLeft ) DodgeForce *= -1;
//	DodgeForce.Z = -0.2f * JumpForceMag;
}

/** Creates a physics attachment to */
native final function AttachTowCable();

/**
  * Get double click status from playercontroller
  */
simulated function tick(float deltatime)
{
	super.tick(deltatime);

	if ( (PlayerController(Controller) != None) && (PlayerController(Controller).PlayerInput != None) )
	{
		DoubleClickMove = PlayerController(Controller).PlayerInput.CheckForDoubleClickMove(DeltaTime);
	}
}

simulated function WeaponRotationChanged(int SeatIndex)
{
	return;
}

function DriverDied()
{
	// to get more dual enforcer opportunities, throw out enforcer when kill player on hoverboard
	if ( Driver != None )
	{
		Driver.Weapon = None;
		Driver.ThrowActiveWeapon();
	}
	Super.DriverDied();
}

simulated function SetEffectsToTeam()
{
	local ParticleSystem TeamThrusterEffect;
	local ParticleSystem TeamDustEffect;

	// First pick the correct effects based on the team
	if(Team == 1)
	{
		TeamThrusterEffect = BlueThrusterEffect;
		TeamDustEffect = BlueDustEffect;
	}
	else
	{
		TeamThrusterEffect = RedThrusterEffect;
		TeamDustEffect = RedDustEffect;
	}

	// Then iterate through calling SetTemplate
	ThrusterEffect.SetTemplate(TeamThrusterEffect);
	HoverboardDust.SetTemplate(TeamDustEffect);
}

simulated function AttachHoverboardEffects()
{
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if (bGrabbingBoard)
		{
			HoverboardMesh.AttachComponentToSocket(ThrusterEffect, ThrusterEffectSocket);
		}
		else
		{
			Mesh.AttachComponentToSocket(ThrusterEffect, ThrusterEffectSocket);
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
	local int i;
	local RB_BodyInstance BodyInstance;
	local vehicle OldDrivenVehicle;

	// keep player from getting super jump while getting onto hoverboard
	if ( P.Velocity.Z > 0 )
	{
		LastJumpTime = WorldInfo.TimeSeconds - 1;
	}

	// give impulse to physics to match velocity player had before
	Mesh.AddImpulse(P.Velocity,,, true);
	// immediately set BodyInstance velocity properties so if we're already colliding with something RigidBodyCollision() has the correct values
	BodyInstance = Mesh.GetRootBodyInstance();
	BodyInstance.Velocity = P.Velocity;
	BodyInstance.PreviousVelocity = P.Velocity;

	if (bDisableRepulsorsAtMaxFallSpeed && P.Velocity.Z <= -P.MaxFallSpeed)
	{
		for (i = 0; i < Wheels.length; i++)
		{
			SetWheelCollision(i, false);
		}
	}

	CameraInitialOut = 0.1/SeatCameraScale;

	// temporarily set drivenvehicle so any loaded weapon fires don't hit me.
	OldDrivenVehicle = P.DrivenVehicle;
	P.DrivenVehicle = self;

	if ( !super.DriverEnter(P) )
	{
		P.DrivenVehicle = OldDrivenVehicle;
		return false;
	}
	return true;
}

simulated function SetTowBeamEffects()
{
	if(Team == 1) // blue
	{
		if(TowBeamTeamEmitters[1] != none && TowBeamTeamEndPoints[1] != none)
		{
			TowBeamEmitter.SetTemplate(TowBeamTeamEmitters[1]);
		}
		if(TowBeamEndPointEffect != none)
		{
			TowBeamEndPointEffect.SetTemplate(TowBeamTeamEndPoints[1]);
		}

	}
	else // red
	{
		if(TowBeamTeamEmitters[0] != none && TowBeamTeamEndPoints[0] != none)
		{
			TowBeamEmitter.SetTemplate(TowBeamTeamEmitters[0]);
		}
		if(TowBeamEndPointEffect != none)
		{
			TowBeamEndPointEffect.SetTemplate(TowBeamTeamEndPoints[0]);
		}
	}
}

simulated function TeamChanged()
{
	super.TeamChanged();
	SetTowBeamEffects();
	SetEffectsToTeam();
	AttachHoverboardEffects();
}

simulated function vector GetCameraStart(int SeatIndex)
{
	local vector NewStart, UseVel;

	UseVel = ClampLength(Velocity, HoverCamMaxVelUsed);
	NewStart = super.GetCameraStart(SeatIndex);
	return NewStart + (UseVel * VelLookAtOffset);
}

simulated function VehicleCalcCamera(float DeltaTime, int SeatIndex, out vector out_CamLoc, out rotator out_CamRot, out vector CamStart, optional bool bPivotOnly)
{
	local float RealCameraScale;
	local float VelSize;
	local vector AngVel;
	local int TargetRoll, DeltaRoll;

	RealCameraScale = SeatCameraScale;
	if ( CameraInitialOut < 1.0 )
	{
		CameraInitialOut = FMin(1.0, CameraInitialOut + 3*DeltaTime);
		SeatCameraScale *= CameraInitialOut;
	}
	super.VehicleCalcCamera(DeltaTime, SeatIndex, out_CamLoc, out_CamRot, CamStart, bPivotOnly);
	SeatCameraScale = RealCameraScale;

	VelSize = FMin(VSize(Velocity), HoverCamMaxVelUsed);

	out_CamRot += HoverCamRotOffset;
	out_CamRot = Normalize(out_CamRot);
	out_CamLoc += (HoverCamOffset + (VelSize * VelBasedCamOffset)) >> out_CamRot;

	TargetRoll = 0;
	if(!bInAJump)
	{
		AngVel = Mesh.BodyInstance.GetUnrealWorldAngularVelocity();
		TargetRoll = VelRollFactor * AngVel.Z * VelSize;
	}

	DeltaRoll = Clamp(TargetRoll - CurrentViewRoll, -ViewRollRate*DeltaTime, ViewRollRate*DeltaTime);
	CurrentViewRoll += DeltaRoll;
	out_CamRot.Roll = CurrentViewRoll;
}

simulated function AttachDriver( Pawn P )
{
	local UTPawn UTP;

	Super.AttachDriver(P);

	UTP = UTPawn(P);
	if (UTP != None)
	{
		UTP.Mesh.AttachComponent(HoverboardMesh, UTP.LeftFootBone, MeshLocationOffset, MeshRotationOffset);
		HandleMesh.SetShadowParent(UTP.Mesh);
		HandleMesh.SetLightEnvironment( UTP.LightEnvironment );
		UTP.Mesh.AttachComponentToSocket(HandleMesh, UTP.WeaponSocket);
		UTP.Mesh.AttachComponentToSocket(TowBeamEmitter, UTP.WeaponSocket);

		// Disable possible blending-out of hit reactions.
		UTP.bBlendOutTakeHitPhysics = FALSE;

		UTP.Mesh.PhysicsWeight = 0.0;
		UTP.Mesh.MinDistFactorForKinematicUpdate = 0.0;

		// delayed so animation has time to get into the proper starting pose first
		SetTimer(0.5, false, 'InitPhysicsAnimPawn');
		//InitPhysicsAnimPawn();
	}
}

simulated function SitDriver(UTPawn UTP, int SeatIndex)
{
	UTP.SetLocation(Location + vect(0,0,100));

	Super.SitDriver(UTP, SeatIndex);

	// don't reduce pawn culldistance as it's the most visible part of the hoverboard
	UTP.Mesh.SetCullDistance(UTP.default.Mesh.CachedCullDistance);
}

/** Set whether the flag is attached directly to kinematic bodies on the rider. */
simulated static function SetFlagAttachToBody(UTPawn UTP, bool bAttached)
{
	local int i;
	local UTCTFFlag Flag;

	for (i = 0; i < UTP.Attached.length; i++)
	{
		Flag = UTCTFFlag(UTP.Attached[i]);
		if (Flag != None && Flag.BaseSkelComponent != None)
		{
			Flag.SkelMesh.SetAttachClothVertsToBaseBody(FALSE);
			if(bAttached)
			{
				Flag.SkelMesh.SetAttachClothVertsToBaseBody(TRUE);
			}
		}
	}
}

simulated function DisplayDebug(HUD HUD, out float out_YL, out float out_YPos)
{
	local UTPawn UTP;
	local Canvas	Canvas;

	super.DisplayDebug(HUD, out_YL, out_YPos);

	UTP = UTPawn(Driver);
	Canvas = HUD.Canvas;

	Canvas.SetDrawColor(128,128,255);
	Canvas.DrawText("UTP PhysicsWeight "$UTP.Mesh.PhysicsWeight);
	out_YPos += out_YL;
	Canvas.SetPos(4,out_YPos);
}

/** Set up the rider with physics for riding. */
simulated function SetHoverboardRiderPhysics(UTPawn UTP)
{
	local array<name> SpringBodies, NoDriveBody;
	local int i;
	local vector WorldConPos, LocalConPos;
	local rotator WorldConRot, LocalConRot;

	// Turn off driver's collision with rigid bodies so it doesn't collide with hoverboard
	UTP.Mesh.SetRBChannel(RBCC_Untitled3);
	UTP.Mesh.SetRBCollidesWithChannel(RBCC_Vehicle, FALSE);
	UTP.Mesh.SetRBCollidesWithChannel(RBCC_Untitled3, TRUE);

	// Use kinematic actor method for bone springs. Makes damping relative to animated pose rather than 'world',
	// so we don't tend to lag behind the hoverboard when moving quickly.
	for( i=0; i<UTP.Mesh.PhysicsAssetInstance.Bodies.Length; i++)
	{
		UTP.Mesh.PhysicsAssetInstance.Bodies[i].bMakeSpringToBaseCollisionComponent = TRUE;

		if(UTP.Mesh.PhysicsAsset.BodySetup[i].bAlwaysFullAnimWeight)
		{
			NoDriveBody.AddItem(UTP.Mesh.PhysicsAsset.BodySetup[i].BoneName);
		}
	}

	// Set rider state based on kinematic update state.
	UTP.Mesh.PhysicsWeight = 1.f;
	UTP.Mesh.PhysicsAssetInstance.SetAllBodiesFixed(FALSE);

	// Make physics interaction with hoverboard one-way
	UTP.Mesh.SetRBDominanceGroup(Mesh.RBDominanceGroup+1);

	UTP.Mesh.UpdateRBBonesFromSpaceBases(TRUE,TRUE);
	UTP.Mesh.SetRBLinearVelocity(UTP.Velocity,FALSE);

	SetFlagAttachToBody(UTP, TRUE);

	WorldConPos = UTP.Mesh.GetBoneLocation(UTP.LeftFootBone, 0);
	WorldConRot = QuatToRotator( UTP.Mesh.GetBoneQuaternion(UTP.LeftFootBone, 0) );
	Mesh.TransformToBoneSpace('UpperBody', WorldConPos, WorldConRot, LocalConPos, LocalConRot);
	FootBoardConstraintSetup.ConstraintBone1 = UTP.LeftFootBone;
	FootBoardConstraintSetup.ConstraintBone2 = 'UpperBody';
	FootBoardConstraintSetup.Pos2 = 0.02f * LocalConPos;
	FootBoardConstraintSetup.PriAxis2 = vect(1,0,0) >> LocalConRot;
	FootBoardConstraintSetup.SecAxis2 = vect(0,1,0) >> LocalConRot;
	LeftFootBoardConstraintInstance.InitConstraint(UTP.Mesh, Mesh, FootBoardConstraintSetup, 1.f, self, None, false);

	WorldConPos = UTP.Mesh.GetBoneLocation(UTP.RightFootBone, 0);
	WorldConRot = QuatToRotator( UTP.Mesh.GetBoneQuaternion(UTP.RightFootBone, 0) );
	Mesh.TransformToBoneSpace('UpperBody', WorldConPos, WorldConRot, LocalConPos, LocalConRot);
	FootBoardConstraintSetup.ConstraintBone1 = UTP.RightFootBone;
	FootBoardConstraintSetup.ConstraintBone2 = 'UpperBody';
	FootBoardConstraintSetup.Pos2 = 0.02f * LocalConPos;
	FootBoardConstraintSetup.PriAxis2 = vect(1,0,0) >> LocalConRot;
	FootBoardConstraintSetup.SecAxis2 = vect(0,1,0) >> LocalConRot;
	RightFootBoardConstraintInstance.InitConstraint(UTP.Mesh, Mesh, FootBoardConstraintSetup, 1.f, self, None, false);

	SpringBodies.AddItem('b_Spine2');
	SpringBodies.AddItem('b_RightHand');
	SpringBodies.AddItem('b_LeftHand');
	UTP.Mesh.PhysicsAssetInstance.SetNamedRBBoneSprings(TRUE, SpringBodies, 10.f, 0.5f, UTP.Mesh);

	UTP.Mesh.bUpdateJointsFromAnimation = TRUE;
	// Set the global linear and angular drive scale that is applied to all bones to be 1.0f so it uses the values setup in UnrealEd
	//UTP.Mesh.PhysicsAssetInstance.SetAllMotorsAngularPositionDrive(true, true);
	UTP.Mesh.PhysicsAssetInstance.SetNamedMotorsAngularPositionDrive(false, false, NoDriveBody, UTP.Mesh, true);
	UTP.Mesh.PhysicsAssetInstance.SetAngularDriveScale(1.0f, 4.0f, 0.0f);

	UTP.BackSpring(10.0);
	UTP.BackDamp(0.25);
	UTP.HandSpring(2.0);
	UTP.HandDamp(0.005);

	UTP.bIsHoverboardAnimPawn=TRUE;
}

exec function BackSpring(float LinSpring)
{
	local UTPawn UTP;
	UTP = UTPawn(Driver);
	UTP.BackSpring(LinSpring);
}

exec function BackDamp(float LinDamp)
{
	local UTPawn UTP;
	UTP = UTPawn(Driver);
	UTP.BackDamp(LinDamp);
}

exec function HandSpring(float LinSpring)
{
	local UTPawn UTP;
	UTP = UTPawn(Driver);
	UTP.HandSpring(LinSpring);
}

exec function HandDamp(float LinDamp)
{
	local UTPawn UTP;
	UTP = UTPawn(Driver);
	UTP.HandDamp(LinDamp);
}

simulated function OnDriverPhysicsAssetChanged(UTPawn UTP)
{
	if(IsLocallyControlled() && IsHumanControlled())
	{
		if(!UTP.bIsHoverboardAnimPawn)
		{
			SetHoverboardRiderPhysics(UTP);
		}
	}
	else
	{
		UTP.Mesh.PhysicsAssetInstance.SetAllBodiesFixed(TRUE);
		UTP.Mesh.bUpdateKinematicBonesFromAnimation = FALSE;
	}

	UTP.SetHandIKEnabled(FALSE);
}

simulated function InitPhysicsAnimPawn()
{
	local UTPawn UTP;

	UTP = UTPawn(Driver);
	if (UTP != None && UTP.Mesh != None && UTP.Mesh.PhysicsAssetInstance != None)
	{
		// Switch over to physics representation of pawn
		Driver.CollisionComponent = Driver.Mesh;

		// If its us, or another player who is close enough to have kinematics updated, blend from animated to physics.
		if(IsLocallyControlled() && IsHumanControlled())
		{
			if(!UTP.bIsHoverboardAnimPawn)
			{
				SetHoverboardRiderPhysics(UTP);
			}

			// Animation should be entirely physics driven
			UTP.Mesh.PhysicsWeight = 0.0;
			TargetPhysicsWeight = 1.0;
			PhysWeightBlendTimeToGo = PhysWeightBlendTime;
		}
		else
		{
			UTP.Mesh.PhysicsAssetInstance.SetAllBodiesFixed(TRUE);
			UTP.Mesh.bUpdateKinematicBonesFromAnimation = FALSE;
			UTP.Mesh.PhysicsWeight = 0.0;
			PhysWeightBlendTimeToGo = 0.0;
		}

		SpineTurnControl = SkelControlSingleBone(UTP.Mesh.FindSkelControl('SpineTurn'));
		if(SpineTurnControl != None)
		{
			SpineTurnControl.BoneRotation = rot(0,0,0);
			SpineTurnControl.SetSkelControlStrength(1.0, 0.5);
		}

		TowControl = SkelControlLookAt(UTP.Mesh.FindSkelControl('TowControl'));
	}
}

simulated exec function TestResetPhys()
{
	`log("Reset Char Phys");
	UTPawn(Driver).ResetCharPhysState();
}


simulated function DetachDriver( Pawn P )
{
	local array<name> SpringBodies;
	local UTPawn UTP;

	if(SpineTurnControl != None)
	{
		SpineTurnControl.SetSkelControlStrength(0.0, 0.0);
		SpineTurnControl = None;
	}

	if(TowControl != None)
	{
		// Make sure tow control is turned off.
		TowControl.SetSkelControlStrength(0.0, 0.0);
		TowControl = None;
	}

	P.Mesh.DetachComponent(HoverboardMesh);
	P.Mesh.DetachComponent(HandleMesh);
	HandleMesh.SetShadowParent(None);
	HandleMesh.SetLightEnvironment( None );
	P.Mesh.DetachComponent(TowBeamEmitter);
	if(TowBeamEndPointEffect != none)
	{
		TowBeamEndPointEffect.SetHidden(true);
	}

	if (P.Mesh != None && P.Mesh.PhysicsAssetInstance != None)
	{
		P.CollisionComponent = P.CylinderComponent;
		P.Mesh.PhysicsWeight = 0.0;
		P.Mesh.bUpdateJointsFromAnimation = FALSE;

		P.Mesh.SetRBDominanceGroup(P.default.Mesh.RBDominanceGroup);

		// Turn off bone springs and drive
		SpringBodies.AddItem('b_Spine2');
		SpringBodies.AddItem('b_RightHand');
		SpringBodies.AddItem('b_LeftHand');
		P.Mesh.PhysicsAssetInstance.SetNamedRBBoneSprings(FALSE, SpringBodies, 10.f, 0.5f, P.Mesh);
		P.Mesh.PhysicsAssetInstance.SetAllMotorsAngularPositionDrive(false, false);

		P.Mesh.PhysicsAssetInstance.SetAllBodiesFixed(TRUE);
		P.Mesh.PhysicsAssetInstance.SetFullAnimWeightBonesFixed(FALSE, P.Mesh);

		P.Mesh.SetRBChannel(RBCC_Pawn);
		P.Mesh.SetRBCollidesWithChannel(RBCC_Vehicle, TRUE);
		P.Mesh.SetRBCollidesWithChannel(RBCC_Untitled3, FALSE);

		P.Mesh.MinDistFactorForKinematicUpdate = P.default.Mesh.MinDistFactorForKinematicUpdate;
		P.Mesh.bUpdateKinematicBonesFromAnimation = P.default.Mesh.bUpdateKinematicBonesFromAnimation;
	}
	P.SetTickGroup(P.default.TickGroup);
	P.Mesh.SetTickGroup(P.default.Mesh.TickGroup);

	UTP = UTPawn(Driver);
	if(UTP != None)
	{
		UTP.bIsHoverboardAnimPawn = FALSE;
	}

	Super.DetachDriver(P);
}

function PossessedBy(Controller C, bool bVehicleTransition)
{
	super.PossessedBy(C, bVehicleTransition);

	// reset jump/duck properties
	bDoHoverboardJump = false;
	bIsDodging = false;
	bGrab1 = false;
	bGrab2 = false;
}

reliable server function ServerChangeSeat(int RequestedSeat)
{
	// if pressed hoverboard weapon key again, leave hoverboard
	if ( RequestedSeat == -1 )
		DriverLeave(false);
}

simulated event BoardJumpEffect()
{
	PlaySound(JumpSound, true);
	VehicleEvent('BoostStart');
}

simulated function SetInputs(float InForward, float InStrafe, float InUp)
{
	Super.SetInputs(InForward, InStrafe, InUp);
}

// Force a spin jump
function ForceSpinJump()
{
	bForceSpinWarmup = false;
	TrickJumpWarmup = TrickJumpWarmupMax;
}

reliable server function ServerSpin(float Direction)
{

}

reliable client function ClientForceSpinWarmup()
{
	bForceSpinWarmup = true;
}

function OnHoverboardSpinJump(UTSeqAct_HoverboardSpinJump Action)
{
	bForceSpinWarmup = true;
	ClientForceSpinWarmup();
	SetTimer(Action.WarmupTime, false, 'ForceSpinJump');
}

//@fixme FIXME: E3 hack!!! do hoverboard spin
exec function SpinCheat()
{
	bForceSpinWarmup = true;
	ClientForceSpinWarmup();
	SetTimer(0.6, false, 'ForceSpinJump');
}

simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	if ( Role < ROLE_Authority )
		return;

	NetUpdateTime = WorldInfo.TimeSeconds - 1; // force quick net update

	// pass damage to driver
	if (Driver != None)
	{
		Driver.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
	}

	// if the driver wasn't killed from the hit, keep going
	if (Driver != None && Driver.Health > 0 && !bDeleteMe)
	{
		// take momentum from hit, but not damage
		Super.TakeDamage(0, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
	}
}

function bool Died(Controller Killer, class<DamageType> DamageType, vector HitLocation)
{
	if (Driver != None && !Driver.Died(Killer, DamageType, HitLocation))
	{
		RagdollDriver();
	}

	Destroy();

	return true;
}

// DriverRadiusDamage() ignored, since our TakeDamage() already passes damage to driver
function DriverRadiusDamage(float DamageAmount, float DamageRadius, Controller EventInstigator, class<DamageType> DamageType, float Momentum, vector HitLocation, Actor DamageCauser);

function NotifyDriverTakeHit(Controller InstigatedBy, vector HitLocation, int Damage, class<DamageType> DamageType, vector Momentum)
{
	local class<UTDamageType> UTDmgType;

	// if we take enemy weapons fire, toss driver into ragdoll
	if (Damage > 0 && InstigatedBy != None && !WorldInfo.GRI.OnSameTeam(InstigatedBy, self))
	{
		UTDmgType = class<UTDamageType>(DamageType);
		if (UTDmgType != None && UTDmgType.default.DamageWeaponClass != None)
		{
			// might be in physics tick and can't send things to ragdoll during that, so delay one tick
			SetTimer(0.01, false, 'RagdollDriver');
		}
	}
}

simulated function float GetChargePower()
{
	return FClamp( (WorldInfo.TimeSeconds - LastJumpTime), 0, JumpDelay)/JumpDelay;
}

//========================================
// AI Interface

function byte ChooseFireMode()
{
	return 0;
}

function bool Dodge(eDoubleClickDir InDoubleClickMove)
{
	Rise = 1;
	return true;
}

function IncomingMissile(Projectile P)
{
	local UTBot B;

	B = UTBot(Controller);
	if (B != None && B.Skill > 2.0 + 2.0 * FRand())
	{
		DriverLeave(false);
	}
}

// AI hint
function bool FastVehicle()
{
	return true;
}

event bool DriverLeave(bool bForceLeave)
{
	local Pawn SavedDriver;
	local Vehicle SavedTowTruck;

	SavedDriver = Driver;
	// Check to see if we should try to get in towing vehicle
	if (bInTow && TowInfo.TowTruck != None && VSize(TowInfo.TowTruck.Location - Driver.Location) < Driver.VehicleCheckRadius + TowInfo.TowTruck.CylinderComponent.CollisionRadius)
	{
		SavedTowTruck = TowInfo.TowTruck;
	}

	// turn off collision so that we can place the driver exactly where it is on the hoverboard
	SetCollision(false, false);
	ExitPositions[0] = Driver.Location - Location;

	if (Super.DriverLeave(bForceLeave))
	{
		if (!bRagdolling)
		{
			// Get into towing vehicle if close enough
			if ( SavedTowTruck != None )
			{
				SavedTowTruck.TryToDrive(SavedDriver);
			}
			else if ( PlayerController(SavedDriver.Controller) != None )
			{
				// try to get into any vehicle if close enough and looking at it
				PlayerController(SavedDriver.Controller).FindVehicleToDrive();
			}
		}
		return true;
	}
	else
	{
		// failed to exit, turn collision back on
		SetCollision(default.bCollideActors, default.bBlockActors);
		return false;
	}
}

simulated function DriverLeft()
{
	bGrab1 = false;
	bGrab2 = false;
	Driver.Velocity = Velocity;
	if ( UTPawn(Driver) != None )
	{
		UTPawn(Driver).LastHoverboardTime = WorldInfo.TimeSeconds;
	}
	Super.DriverLeft();
	Destroy();
}

simulated function CauseMuzzleFlashLight(int SeatIndex)
{
	Super.CauseMuzzleFlashLight(SeatIndex);

	VehicleEvent('FireTowCable');
}

function LinkUp(UTVehicle NewTowTruck)
{
	if ( NewTowTruck != none && !NewTowTruck.bDeleteMe && NewTowTruck.Health>0 )
	{
		bInTow = true;
		TowInfo.TowTruck = NewTowTruck;
		TowInfo.TowAttachPoint = NewTowTruck.GetHoverBoardAttachPoint(Location);
		AttachTowCable();
		PlaySound(TowStartedSound);
		if(TowLoopComponent == none)
		{
			TowLoopComponent = CreateAudioComponent(TowLoopCue, false, true);
		}
		if(TowLoopComponent != none)
		{
			TowLoopComponent.Play();
		}
	}
}

simulated event BreakTowLink()
{
	if(bInTow)
	{
		if(TowLoopComponent != none)
		{
			TowLoopComponent.Stop();
			TowLoopComponent = None;
		}
		PlaySound(TowEndedSound);
	}
	bInTow = false;
	DistanceJointInstance.TermConstraint();
	TowInfo.TowAttachPoint='';
	TowInfo.TowTruck = none;
}

simulated function ReplicatedEvent(name VarName)
{
	if (VarName == 'TowInfo' )
	{
		if (TowInfo.TowTruck != none)
		{
			bInTow = true;
			AttachTowCable();
		}
		else
		{
			BreakTowLink();
		}
	}
	else if (VarName == 'bDoHoverboardJump')
	{
		BoardJumpEffect();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function DisableVehicle()
{
	RagdollDriver();
}

/** kick the driver out and throw him into ragdoll because we ran into something */
function RagdollDriver()
{
	local UTPawn OldDriver;

	bRagdolling = true;
	OldDriver = UTPawn(Driver);
	DriverLeave(true);
	if (OldDriver != None)
	{
		OldDriver.SoundGroupClass.static.PlayFallingDamageLandSound(OldDriver);
		OldDriver.Velocity = Velocity;
		OldDriver.ForceRagdoll();
		if (OldDriver.Physics == PHYS_RigidBody)
		{
			// apply a small impulse towards the top of the player so the body pitches forward a bit
			OldDriver.Mesh.AddImpulse(Normal(Velocity) * 100.0, OldDriver.Location + vect(0,0,0.75) * OldDriver.GetCollisionHeight());
		}
	}
}

event bool EncroachingOn(Actor Other)
{
	RanInto(Other);
	return bDeleteMe; // return true if we were destroyed (driver kicked out) so the other thing doesn't think it got run over
}

event RanInto(Actor Other)
{
	if (Role == ROLE_Authority && Pawn(Other) != None && !WorldInfo.GRI.OnSameTeam(self, Other))
	{
		if (Driver == None)
		{
			// encroached while spawning, driver isn't in yet
			Destroy();
		}
		else
		{
			RagdollDriver();
		}
	}
}

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
					const CollisionImpactData RigidCollisionData, int ContactIndex )
{
	local RB_BodyInstance BodyInstance;
	local Vehicle OtherVehicle;
	local int OldHealth;

	if (Role == ROLE_Authority)
	{
		BodyInstance = Mesh.GetRootBodyInstance();
		OtherVehicle = (OtherComponent != None) ? Vehicle(OtherComponent.Owner) : None;

		if ( (OtherVehicle != None) && !WorldInfo.GRI.OnSameTeam(self, OtherVehicle)
			&& OtherVehicle.bDriving )
		{
			if ( UTVehicle_Hoverboard(OtherVehicle) != None )
			{
				RagdollDriver();
				Super(SVehicle).RigidBodyCollision(HitComponent, OtherComponent, RigidCollisionData, ContactIndex);
				return;
			}
			else
			{
				OtherVehicle.RanInto(self);
			}
		}
		if ( !IsTimerActive('RagdollDriver') )
		{
			if (BodyInstance.PreviousVelocity.Z < -Driver.MaxFallSpeed)
			{
				// only check fall damage for Z axis collisions
				if (Abs(RigidCollisionData.ContactInfos[0].ContactNormal.Z) > 0.5f)
				{
					Driver.Velocity = BodyInstance.PreviousVelocity;
					OldHealth = Driver.Health;
					Driver.TakeFallingDamage();
					// zero our velocity so that the ragdoll'ed driver won't take falling damage again
					// unless it gets into another long fall
					Velocity.Z = 0.0;
					if ( (Driver.Health < 0) || (OldHealth - Driver.Health > FallingDamageRagdollThreshold) )
					{
						RagdollDriver();
					}
					Super(SVehicle).RigidBodyCollision(HitComponent, OtherComponent, RigidCollisionData, ContactIndex);
					return;
				}
			}
			else if (OtherComponent != None && bTrickJumping && (bGrab1 || bGrab2))
			{
				RagdollDriver();
				Super(SVehicle).RigidBodyCollision(HitComponent, OtherComponent, RigidCollisionData, ContactIndex);
				return;
			}
		}

		Super.RigidBodyCollision(HitComponent, OtherComponent, RigidCollisionData, ContactIndex);
	}
}

simulated function StopVehicleSounds()
{
	super.StopVehicleSounds();
	CurveSound.Stop();
}

simulated event ToggleAnimBoard(bool bAnimBoard, float Delay)
{
	if (bAnimBoard)
	{
		bGrabbingBoard = TRUE;
		AttachHoverboardEffects();
		Mesh.SetHidden(TRUE);
    	HoverboardMesh.SetHidden(FALSE);

		if(IsLocallyControlled() && IsHumanControlled())
		{
			TargetPhysicsWeight = 0.0;
			PhysWeightBlendTimeToGo = 0.1;
		}
    }
    else
    {
		bGrabbingBoard = FALSE;
		if(Delay > 0.0)
		{
			SetTimer(Delay, false, 'HideBoard');
		}
		else
		{
			HideBoard();
		}

		if(IsLocallyControlled() && IsHumanControlled())
		{
			TargetPhysicsWeight = 1.0;
			PhysWeightBlendTimeToGo = 0.1;
		}
	}
}

simulated event HideBoard()
{
	AttachHoverboardEffects();
	Mesh.SetHidden(FALSE);
	HoverboardMesh.SetHidden(TRUE);
}

simulated event HoverboardLanded()
{
	if(bTrickJumping && (bGrab1 || bGrab2) && Role == ROLE_Authority)
	{
		SetTimer(0.01, false, 'RagdollDriver');
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=+44.0
		CollisionRadius=+40.0
		Translation=(Z=25.0)
	End Object

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Hoverboard.Mesh.SK_VH_Hoverboard'
		PhysicsAsset=PhysicsAsset'VH_Hoverboard.Mesh.SK_VH_Hoverboard_Physics'
		AnimTreeTemplate=VH_Hoverboard.Anims.AT_Hoverboard
		bUseAsOccluder=FALSE
		RBDominanceGroup=16
	End Object

	Begin Object Class=UTSkeletalMeshComponent Name=MeshComponent0
		SkeletalMesh=SkeletalMesh'VH_Hoverboard.Mesh.SK_VH_Hoverboard'
		PhysicsAsset=none
		CastShadow=true
		CollideActors=false
		HiddenGame=TRUE
		bUseAsOccluder=FALSE
	End Object
	HoverboardMesh=MeshComponent0

	MeshLocationOffset=(X=5,Y=5,Z=15)
	MeshRotationOffset=(Pitch=10012,Roll=16382,Yaw=18204)

	MeleeRange=-100.0
	bLightArmor=true
	bDrawHealthOnHUD=false

	COMOffset=(x=10.0,y=0.0,z=-35.0)

	JumpForceMag=5000.0
	DodgeForceMag=5000.0
	JumpCheckTraceDist=90.0
	TrickJumpWarmupMax=0.5
	JumpDelay=1.0

	MaxAngularVelocity=110000.0

	MaxTowDistance=1000.0

	AirSpeed=900
	GroundSpeed=900.0
	MaxSpeed=3500.0
	MomentumMult=2.0
	bDisableRepulsorsAtMaxFallSpeed=true

	bCanCarryFlag=true
	bFollowLookDir=true
	bTurnInPlace=true
	bScriptedRise=True
	bCanStrafe=false
	ObjectiveGetOutDist=750.0
	MaxDesireability=0.6
	SpawnRadius=125.0
	CollisionDamageMult=0.0013
	bValidLinkTarget=false

	bStayUpright=false
	LeanUprightStiffness=1000
	LeanUprightDamping=100

	Begin Object Class=RB_StayUprightSetup Name=MyLeanUprightSetup
		bSwingLimited=false
	End Object
	LeanUprightConstraintSetup=MyLeanUprightSetup

	Begin Object Class=RB_ConstraintInstance Name=MyLeanUprightConstraintInstance
		bSwingPositionDrive=true
	End Object
	LeanUprightConstraintInstance=MyLeanUprightConstraintInstance

	Begin Object Class=RB_ConstraintSetup Name=MyFootBoardConstraintSetup
		bTwistLimited=true
		bSwingLimited=true
	End Object
	FootBoardConstraintSetup=MyFootBoardConstraintSetup

	Begin Object Class=RB_ConstraintInstance Name=MyLeftFootBoardConstraintInstance
	End Object
	LeftFootBoardConstraintInstance=MyLeftFootBoardConstraintInstance

	Begin Object Class=RB_ConstraintInstance Name=MyRightFootBoardConstraintInstance
	End Object
	RightFootBoardConstraintInstance=MyRightFootBoardConstraintInstance

	TurnLeanFactor=0.0013
	MaxLeanPitchSpeed=10000.0

	MaxTrackYaw=1.0

	WaterCheckLevel=110.0f

	bUseSuspensionAxis=true

	Seats(0)={(GunClass=class'UTVWeap_TowCable',
				GunSocket=(FireSocket),
				CameraTag=b_Hips,
				CameraOffset=-200,
				DriverDamageMult=1.0,
				bSeatVisible=true,
				SeatBone=UpperBody,
				SeatOffset=(X=0,Y=0,Z=50))}

	InertiaTensorMultiplier=(x=1.0,y=1.0,z=1.0)

	Begin Object Class=UTVehicleSimHoverboard Name=SimObject
		WheelSuspensionStiffness=200.0
		WheelSuspensionDamping=20.0
		WheelSuspensionBias=0.0
		WheelLatExtremumValue=0.7

		MaxThrustForce=200.0
		MaxReverseForce=40.0
		MaxReverseVelocity=200.0
		LongDamping=0.3

		MaxStrafeForce=150.0
		LatDamping=0.3

		TurnTorqueFactor=800.0
		SpinTurnTorqueScale=3.5
		MaxTurnTorque=1000.0
		TurnDampingSpeedFunc=(Points=((InVal=0,OutVal=0.05),(InVal=300,OutVal=0.11),(InVal=800,OutVal=0.12)))
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	Begin Object Class=UTHoverWheel Name=HoverWheelFL
		BoneName="Front_Wheel"
		BoneOffset=(X=25.0,Y=0.0,Z=-50.0)
		WheelRadius=10
		SuspensionTravel=50
		bPoweredWheel=true
		LongSlipFactor=0
		LatSlipFactor=100
		HandbrakeLongSlipFactor=0
		HandbrakeLatSlipFactor=150
		SteerFactor=1.0
		Side=SIDE_Left
		bHoverWheel=true
	End Object
	Wheels(0)=HoverWheelFL

	Begin Object Class=UTHoverWheel Name=HoverWheelRL
		BoneName="Rear_Wheel"
		BoneOffset=(X=0.0,Y=0,Z=-50.0)
		WheelRadius=10
		SuspensionTravel=50
		bPoweredWheel=true
		LongSlipFactor=0
		LatSlipFactor=100
		HandbrakeLongSlipFactor=0
		HandbrakeLatSlipFactor=150
		SteerFactor=0.0
		Side=SIDE_Left
		bHoverWheel=true
		SkelControlName="BoardTire"
	End Object
	Wheels(1)=HoverWheelRL

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=HoverboardEngineSound
		SoundCue=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_EngineCue'
	End Object
	EngineSound=HoverboardEngineSound
	Components.Add(HoverboardEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=HoverboardCollideSound
		SoundCue=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_CollideCue'
	End Object
	CollideSound=HoverboardCollideSound
	Components.Add(HoverboardCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_EngineStartCue'
	ExitVehicleSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_EngineStopCue'

	// Scrape sound.
	Begin Object Class=AudioComponent Name=BaseScrapeSound
		SoundCue=SoundCue'A_Gameplay.A_Gameplay_Onslaught_MetalScrape01Cue'
	End Object
	ScrapeSound=BaseScrapeSound
	Components.Add(BaseScrapeSound);

	// Carving sound.
	Begin Object Class=AudioComponent Name=CarveSound
		SoundCue=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_HoverBoard_CurveCue'
	End Object
	CurveSound=CarveSound
	Components.Add(CarveSound);

	EngineThrustSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_HoverBoard_EngineThrustCue'
	TurnSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_HoverBoard_TurnCue'
	JumpSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_JumpCue'

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	EngineStartOffsetSecs=0.3
	EngineStopOffsetSecs=1.0

	RedThrusterEffect=ParticleSystem'VH_Hoverboard.Effects.P_VH_Hoverboard_Jet_Red01'
	BlueThrusterEffect=ParticleSystem'VH_Hoverboard.Effects.P_VH_Hoverboard_Jet_Blue01'

	Begin Object Class=ParticleSystemComponent 	Name=ThrusterEffect0
	SecondsBeforeInactive=1.0f
	End Object
	ThrusterEffect=ThrusterEffect0
	ThrusterEffectSocket=RearCenterThrusterSocket

	GroundEffectIndices=(0)

	bTeamLocked=false
	Team=255
	bAttachDriver=true
	bDriverIsVisible=true
	Eyeheight=35
	BaseEyeheight=35
	bShouldLeaveForCombat=true

	BurnOutTime=2.0

	MapSize=12
	IconXStart=0.625
	IconYStart=0.0
	IconXWidth=0.125
	IConYWidth=0.125

	// Tow Cable
	Begin Object Class=RB_DistanceJointSetup Name=MyDistanceJointSetup
	End Object
	DistanceJointSetup=MyDistanceJointSetup
	Begin Object Class=RB_ConstraintInstance Name=MyDistanceJointInstance
	End Object
	DistanceJointInstance=MyDistanceJointInstance

	Begin Object Class=UTParticleSystemComponent Name=TowBeamPCS0
		Template=ParticleSystem'VH_Hoverboard.Effects.P_VH_Hoverboard_TetherBeam_Blue'
		HiddenGame=true
		TickGroup=TG_PostAsyncWork // so endpoint gets updated after the target vehicle has finished movement
		Translation=(Y=8.0)
		SecondsBeforeInactive=1.0f
	End Object
	TowBeamEmitter=TowBeamPCS0

	Begin Object Class=UTParticleSystemComponent Name=TowBeamPCS1
		Template=ParticleSystem'VH_Hoverboard.Effects.P_VH_Hoverboard_TetherBeam_BlueEnd'
		HiddenGame=true
		AbsoluteTranslation=true
		AbsoluteRotation=true
		SecondsBeforeInactive=1.0f
	End Object
	Components.Add(TowBeamPCS1);
	TowBeamEndPointEffect=TowBeamPCS1

	Begin Object Class=UTParticleSystemComponent Name=HoverboardDust0
		Template=ParticleSystem'Envy_Effects.Smoke.P_HoverBoard_Ground_Dust'
		AbsoluteTranslation=true
		AbsoluteRotation=true
		bAutoActivate=false
		SecondsBeforeInactive=1.0f
	End Object
	Components.Add(HoverboardDust0);
	HoverboardDust=HoverboardDust0

	RedDustEffect=ParticleSystem'Envy_Effects.Smoke.P_HoverBoard_Ground_Dust'
	BlueDustEffect=ParticleSystem'Envy_Effects.Smoke.P_HoverBoard_Ground_Dust_Blue'

	Begin Object Class=StaticMeshComponent Name=Handle
		StaticMesh=StaticMesh'VH_Hoverboard.Mesh.S_Hoverboard_Handle'
		Rotation=(Yaw=16384,Roll=-16384)
		Translation=(Z=-1.0)
		CollideActors=false
		Scale=0.5
	End Object
	HandleMesh=Handle

	TowBeamEndParameterName=TetherEnd
	SeatCameraScale=0.8
	CameraScaleMin=0.2
	CameraScaleMax=1.0
	bRotateCameraUnderVehicle=true
	bNoZSmoothing=false
	bNoFollowJumpZ=true

	DefaultFOV=90
	CameraLag=0.0
	bDriverCastsShadow=true
	bDriverHoldsFlag=true

	MinCameraDistSq=144.0
	bStickDeflectionThrottle=true

	ExplosionSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_HoverBoard_ExplodeCue'

	PhysWeightBlendTime=1.0

	TowBeamTeamEmitters[0]=ParticleSystem'VH_Hoverboard.Effects.P_VH_Hoverboard_TetherBeam_Red'
	TowBeamTeamEndPoints[0]=ParticleSystem'VH_Hoverboard.Effects.P_VH_Hoverboard_TetherBeam_RedEnd'
	TowBeamTeamEmitters[1]=ParticleSystem'VH_Hoverboard.Effects.P_VH_Hoverboard_TetherBeam_Blue'
	TowBeamTeamEndPoints[1]=ParticleSystem'VH_Hoverboard.Effects.P_VH_Hoverboard_TetherBeam_BlueEnd'


	TowStartedSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_GrappleReleaseCue'
	TowEndedSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_GrappleRetractCue'

	TowLoopCue=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_GrappleLoopCue'

	FallingDamageRagdollThreshold=6

	TowLocalAttachPos=(X=50,Y=0,Z=50)
	HoverboardTowSteerMaxAngle=30
	TowDistanceChangeSpeed=300
	TowLineBlockedBreakTime=0.25

	bAlwaysRelevant=false
	bDoExtraNetRelevancyTraces=false

	HoverCamOffset=(X=60,Y=-20,Z=-15)
	HoverCamRotOffset=(Pitch=728)
	VelLookAtOffset=(X=-0.07,Y=-0.07,Z=-0.03)
	VelBasedCamOffset=(Z=-0.02)
	VelRollFactor=0.4
	HoverCamMaxVelUsed=800
	ViewRollRate=100

	SpinSpeed=11.0
}
