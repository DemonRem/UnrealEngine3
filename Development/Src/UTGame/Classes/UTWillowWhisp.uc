/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** used to show the path to game objectives */
class UTWillowWhisp extends UTReplicatedEmitter;

const MAX_WAYPOINTS = 15;

/** path points to travel to */
var vector WayPoints[MAX_WAYPOINTS];
/** total number of valid points in WayPoints list */
var repnotify int NumPoints;
/** current position in WayPoints list */
var int Position;

var bool bHeadedRight;

replication
{
	if (bNetInitial)
		NumPoints, WayPoints;
}

simulated function PostBeginPlay()
{
	local int i, Start;
	local PlayerController P;
	local Actor HitActor;
	local Vector HitLocation,HitNormal;

	Super.PostBeginPlay();

	if (Role == ROLE_Authority)
	{
		P = PlayerController(Owner);
		if (P == None || P.Pawn == None)
		{
			Destroy();
		}
		else
		{
			SetLocation(P.Pawn.Location);

			WayPoints[0] = Location + P.Pawn.GetCollisionHeight() * vect(0,0,1) + 200.0 * vector(P.Rotation);
			HitActor = Trace(HitLocation, HitNormal, WayPoints[0], Location, false);
			if (HitActor != None)
			{
				WayPoints[0] = HitLocation;
			}
			NumPoints++;

			if (P.RouteCache[0] != None && P.RouteCache.length > 1 && P.ActorReachable(P.RouteCache[1]))
			{
				Start = 1;
			}
			for (i = Start; NumPoints < MAX_WAYPOINTS && i < P.RouteCache.length && P.RouteCache[i] != None; i++)
			{
				WayPoints[NumPoints++] = P.RouteCache[i].Location + P.Pawn.GetCollisionHeight() * Vect(0,0,1);
			}
		}
	}
}

simulated event SetInitialState()
{
	bScriptInitialized = true;

	if (Role == ROLE_Authority)
	{
		if (PlayerController(Owner).IsLocalPlayerController())
		{
			StartNextPath();
		}
		else
		{
			//@warning: can't set bHidden because that would get replicated
			ParticleSystemComponent.DeactivateSystem();
			LifeSpan = ServerLifeSpan;
			SetPhysics(PHYS_None);
		}
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'NumPoints')
	{
		StartNextPath();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function StartNextPath()
{
	local vector TrackDir;
	local Pawn OwnerPawn;

	if (++Position >= NumPoints)
	{
		LifeSpan = 1.5;
		Velocity = vect(0,0,0);
		Acceleration = vect(0,0,0);
		ParticleSystemComponent.DeactivateSystem();
		SetPhysics(PHYS_None);
		GotoState('');
	}
	else
	{
		GotoState('Pathing');
		bHeadedRight = false;
		OwnerPawn = (PlayerController(Owner) != None) ? PlayerController(Owner).Pawn : None; 
		if (Position == 0 && OwnerPawn != None)
		{
			// initialize velocity
			TrackDir = vector(OwnerPawn.Rotation);
			Velocity = (100 + (OwnerPawn.velocity dot trackdir)) * TrackDir;
		}
		Acceleration = 1500 * Normal(WayPoints[Position] - Location);
		Velocity.Z = 0.5 * (Velocity.Z + Acceleration.Z);
		SetRotation(rotator(Acceleration));
	}
}

state Pathing
{
	simulated function Tick(float DeltaTime)
	{
		local float MaxSpeed;
		local Pawn OwnerPawn;

		Acceleration = 1500.0 * Normal(WayPoints[Position] - Location);
		Velocity = Velocity + DeltaTime * Acceleration; // force double acceleration
		OwnerPawn = (PlayerController(Owner) != None) ? PlayerController(Owner).Pawn : None; 
		MaxSpeed = (OwnerPawn != None) ? VSize(OwnerPawn.Velocity) + 60.0 : 60.0;
		if ( VSizeSq(Velocity) > MaxSpeed*MaxSpeed )
		{
			Velocity = MaxSpeed * Normal(Velocity);
		}
		if (!bHeadedRight)
		{
			bHeadedRight = ((Velocity Dot Acceleration) > 0);
		}
		else if ((Velocity Dot Acceleration) < 0.0)
		{
			StartNextPath();
		}
		if (VSize(WayPoints[Position] - Location) < 80.0)
		{
			StartNextPath();
		}
	}
}

defaultproperties
{
	EmitterTemplate=ParticleSystem'GamePlaceholders.Effects.WillowWhisp'
	LifeSpan=24.0
	Physics=PHYS_Projectile
	bOnlyRelevantToOwner=true
	bOnlyOwnerSee=false
	Position=-1
	bReplicateMovement=false
}
