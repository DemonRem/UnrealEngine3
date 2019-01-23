/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTVehicle_Walker extends UTVehicle
	native(Vehicle)
	abstract;


enum EWalkerStance
{
	WalkerStance_None,
	WalkerStance_Standing,
	WalkerStance_Parked,
	WalkerStance_Crouched,
};

var transient EWalkerStance CurrentStance;
var transient EWalkerStance PreviousStance;		// stance last frame

/** Suspension settings for each stance */
var() protected float WheelSuspensionTravel[EWalkerStance.EnumCount];

var UTWalkerBody BodyActor;
var() RB_Handle BodyHandle;
var class<UTWalkerBody> BodyType;
var() protected const Name BodyAttachSocketName;

/** Interpolation speed (for RInterpTo) used for interpolating the rotation of the body handle */
var() float BodyHandleOrientInterpSpeed;

var		bool							bWasOnGround;
var		bool							bPreviousInAir;
var     bool                            bHoldingDuck;
var		bool							bAnimateDeadLegs;
var()   float   DuckForceMag;

var		float	InAirStart;
var		float	LandingFinishTime;

var()	vector	BaseBodyOffset;

/** Specifies offset to suspension travel to use for determining approximate distance to ground (takes into account suspension being loaded) */
var		float	HoverAdjust[EWalkerStance.EnumCount];

/** controls how fast to interpolate between various suspension heights */
var() protected float SuspensionTravelAdjustSpeed;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual UBOOL Tick( FLOAT DeltaTime, enum ELevelTick TickType );
	virtual FVector GetWalkerBodyLoc();
	UBOOL IsMoving2D() const;
}

simulated function PostBeginPlay()
{
	local vector X, Y, Z;

	Super.PostBeginPlay();

	// no spider body on server
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		GetAxes(Rotation, X,Y,Z);
		BodyActor = Spawn(BodyType, self,, Location+BaseBodyOffset.X*X+BaseBodyOffset.Y*Y+BaseBodyOffset.Z*Z);
		BodyActor.SetWalkerVehicle(self);
	}
}

simulated function BlowupVehicle()
{
	Super.BlowupVehicle();
	if ( BodyActor != None )
	{
		BodyHandle.ReleaseComponent();
		BodyActor.PlayDying();
		BodyActor.SkeletalMeshComponent.SetMaterial(0, Mesh.GetMaterial(0));
		BodyActor.SkeletalMeshComponent.SetMaterial(1, Mesh.GetMaterial(0));
	}
	if ( UTVehicleSimHover(SimObj) != None )
	{
		UTVehicleSimHover(SimObj).bDisableWheelsWhenOff = true;
	}
}

simulated function Destroyed()
{
	Super.Destroyed();
	if ( BodyActor != None )
	{
		if ( !bTearOff )
			BodyActor.Destroy();
		else if ( BodyActor.LifeSpan == 0 )
			BodyActor.PlayDying();
	}
}

function bool DriverEnter(Pawn P)
{
	if ( !Super.DriverEnter(P) )
		return false;
	SetDriving(true);		// @fixme, does this need to be for Role_Authority only? (See DriverLeft)
	return true;
}


simulated function DriverLeft()
{
	Super.DriverLeft();

	if (Role == ROLE_Authority)
	{
		SetDriving(NumPassengers() > 0);
	}
}

simulated function bool AllowLinkThroughOwnedActor(Actor OwnedActor)
{
	return (OwnedActor == BodyActor);
}


defaultproperties
{
	bCanBeBaseForPawns=false
	CollisionDamageMult=0.0

	Begin Object Class=RB_Handle Name=RB_BodyHandle
		LinearDamping=100.0
		LinearStiffness=4000.0
		AngularDamping=200.0
		AngularStiffness=4000.0
	End Object
	BodyHandle=RB_BodyHandle
	Components.Add(RB_BodyHandle)
	
	BaseEyeheight=300
	Eyeheight=300

	BodyHandleOrientInterpSpeed=5.f
	bAnimateDeadLegs=false
}
