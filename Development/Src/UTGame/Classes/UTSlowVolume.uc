/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** slows down all actors inside the volume */
class UTSlowVolume extends GravityVolume
	native
	notplaceable
	abstract;

var float ScalingFactor;
var float RBScalingFactor;
var float AnimSlowdownFactor;

/** Gravity applied to rigid bodies within this volume. */
var float RBGravityZ;

/** actors which have had their PendingTouch set to us due to an ActorEnteredVolume() event */
var array<Actor> PendingEntered;
/** actors which have had their PendingTouch set to us due to an ActorLeavingVolume() event */
var array<Actor> PendingLeaving;

/** array of ReachSpecs we modified the distance for, so we reset them when we are destroyed */
var array<ReachSpec> ModifiedSpecs;

var ParticleSystemComponent SlowEffect;
var SkeletalMeshComponent GeneratorMesh;

/** We use a delegate so that different types of creators can get the OnDestroyed event */
delegate OnDeployableUsedUp(actor ChildDeployable);

cpptext
{
	virtual FLOAT GetVolumeRBGravityZ() { return RBGravityZ; }
}

simulated function PostBeginPlay()
{
	local vector HitLocation, HitNormal;
	local int i;

	Super.PostBeginPlay();

	RBGravityZ = GravityZ * RBScalingFactor;
	GravityZ *= Square(ScalingFactor);

	// align to floor
	if (Trace(HitLocation, HitNormal, Location - vect(0,0,250), Location, false) != None)
	{
		SetLocation(HitLocation);
	}

	if (CollisionComponent == None)
	{
		`Warn("UTSlowVolume with no CollisionComponent");
	}
	else if (!(ScalingFactor ~= 0.0))
	{
		// increase cost of overlapping ReachSpecs
		WorldInfo.NavigationPointCheck(CollisionComponent.GetPosition(), CollisionComponent.Bounds.BoxExtent,, ModifiedSpecs);
		for (i = 0; i < ModifiedSpecs.length; i++)
		{
			ModifiedSpecs[i].Distance /= ScalingFactor;
		}
	}
	if(GeneratorMesh != none)
	{
		GeneratorMesh.PlayAnim('Deploy');
		SetTimer(1.3f,false,'ActivateSlowEffect');
	}
	else
	{
		ActivateSlowEffect();
	}
}

simulated function ActivateSlowEffect()
{
	if(SlowEffect != none)
	{
		SlowEffect.ActivateSystem();
	}
}

simulated function Destroyed()
{
	local Actor A;
	local int i;

	Super.Destroyed();

	// force any actors inside us to go back to normal
	foreach DynamicActors(class'Actor', A)
	{
		if (Encompasses(A))
		{
			if (A.IsA('Pawn'))
			{
				PawnLeavingVolume(Pawn(A));
			}
			else
			{
				ActorLeavingVolume(A);
			}
			PostTouch(A);
			A.PendingTouch = None;
		}
	}

	// return cost of ReachSpecs to normal
	for (i = 0; i < ModifiedSpecs.length; i++)
	{
		if (ModifiedSpecs[i] != None)
		{
			ModifiedSpecs[i].Distance *= ScalingFactor;
		}
	}

	OnDeployableUsedUp(self);
}


simulated function ActorEnteredVolume(Actor Other)
{
	// we can't change velocity here as it may be during the physics tick which will overwrite it
	PendingEntered[PendingEntered.length] = Other;
	PendingTouch = Other.PendingTouch;
	Other.PendingTouch = self;
}

simulated function ActorLeavingVolume(Actor Other)
{
	// we can't change velocity here as it may be during the physics tick which will overwrite it
	PendingLeaving[PendingLeaving.length] = Other;
	PendingTouch = Other.PendingTouch;
	Other.PendingTouch = self;
}

simulated function PostTouch(Actor Other)
{
	local int i;

	// adjust the velocity of the actor
	//@warning: this assumes there can't be two entered events for a single actor without at least one leaving event (and vice versa)
	for (i = 0; i < PendingEntered.length; i++)
	{
		if (PendingEntered[i] == None)
		{
			PendingEntered.Remove(i, 1);
			i--;
		}
		else if (PendingEntered[i] == Other)
		{
			Other.Velocity *= ScalingFactor;
			Other.Acceleration *= Square(ScalingFactor);
			PendingEntered.Remove(i, 1);
			i--;
		}
	}
	for (i = 0; i < PendingLeaving.length; i++)
	{
		if (PendingLeaving[i] == None)
		{
			PendingLeaving.Remove(i, 1);
			i--;
		}
		else if (PendingLeaving[i] == Other)
		{
			Other.Velocity /= ScalingFactor;
			Other.Acceleration /= Square(ScalingFactor);
			PendingLeaving.Remove(i, 1);
			i--;
		}
	}
}

simulated function PawnEnteredVolume(Pawn Other)
{
	if ( Role == ROLE_Authority )
		GotoState('DrainingFromPawns');
	SlowPawnDown(Other);
}

simulated function SlowPawnDown(Pawn Other)
{
	local UTPawn P;
	local SVehicle V;

	ActorEnteredVolume(Other);

	if (Other.Role == ROLE_Authority)
	{
		// these are replicated, so don't change them clientside
		Other.GroundSpeed *= ScalingFactor;
		Other.AirSpeed *= ScalingFactor;
		Other.AccelRate *= ScalingFactor;
		Other.JumpZ *= ScalingFactor * 2.0;
	}

	P = UTPawn(Other);
	if (P != None)
	{
		P.DodgeSpeed *= ScalingFactor;
		P.DodgeSpeedZ *= ScalingFactor;
		P.SetAnimRateScale(AnimSlowdownFactor);
	}
	V = SVehicle(Other);
	if (V != None)
	{
		V.MaxSpeed *= ScalingFactor;
	}
}

simulated function PawnLeavingVolume(Pawn Other)
{
	local UTPawn P;
	local SVehicle V;

	ActorLeavingVolume(Other);

	if (Other.Role == ROLE_Authority)
	{
		// these are replicated, so don't change them clientside
		Other.GroundSpeed /= ScalingFactor;
		Other.AirSpeed /= ScalingFactor;
		Other.AccelRate /= ScalingFactor;
		Other.JumpZ /= ScalingFactor * 2.0;
	}

	P = UTPawn(Other);
	if (P != None)
	{
		P.DodgeSpeed /= ScalingFactor;
		P.DodgeSpeedZ /= ScalingFactor;
		P.SetAnimRateScale(1.0);
	}
	V = SVehicle(Other);
	if (V != None)
	{
		V.MaxSpeed /= ScalingFactor;
	}
}

State DrainingFromPawns
{
	function Timer()
	{
		local Pawn P;
		local int Count;

		ForEach TouchingActors(class'Pawn', P)
		{
			if ( P.PlayerReplicationInfo != None )
			{
				LifeSpan -= 4.0;
				Count++;
			}
		}
		if ( LifeSpan == 0.0 )
		{
			Lifespan = 0.001;
		}
		if ( Count == 0 )
		{
			ClearTimer();
			GotoState('');
		}
	}

	simulated function PawnEnteredVolume(Pawn Other)
	{
		SlowPawnDown(Other);
	}

	function BeginState(name PreviousStateName)
	{
		settimer(1.0, true);
	}

}

function ModifyPlayer(Pawn PlayerPawn)
{
	PlayerPawn.GroundSpeed *= ScalingFactor;
	PlayerPawn.AirSpeed *= ScalingFactor;
	PlayerPawn.AccelRate *= ScalingFactor;
	PlayerPawn.JumpZ *= ScalingFactor * 2.0;
}

function Reset()
{
	Destroy();
}

defaultproperties
{
	bCollideActors=true
	bBlockActors=false
	bStatic=false
	bNoDelete=false
	bHidden=false
	RemoteRole=ROLE_SimulatedProxy
	Priority=100000
	LifeSpan=180.0

	ScalingFactor=0.125
	RBScalingFactor=0.4
	RigidBodyDamping=3.5
	AnimSlowdownFactor=0.3
}
