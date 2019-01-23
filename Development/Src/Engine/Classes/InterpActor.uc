/** dynamic static mesh actor intended to be used with Matinee
 *	replaces movers
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class InterpActor extends DynamicSMActor
	native
	placeable;

cpptext
{
	UBOOL ShouldTrace(UPrimitiveComponent* Primitive, AActor *SourceActor, DWORD TraceFlags);
	virtual void TickSpecial(FLOAT DeltaSeconds);

	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
}

/** NavigationPoint associated with this actor for sending AI related notifications (could be a LiftCenter or DoorMarker) */
var NavigationPoint MyMarker;
/** true when AI is waiting for us to finish moving */
var bool bMonitorMover;
/** if true, call MoverFinished() event on all Controllers with us as their PendingMover when we reach peak Z velocity */
var bool bMonitorZVelocity;
/** set while monitoring lift movement */
var float MaxZVelocity;
/** delay after mover finishes interpolating before it notifies any mover events */
var float StayOpenTime;
/** sound played when the mover is interpolated forward */
var() SoundCue OpenSound;
/** sound played when the mover is interpolated in reverse */
var() SoundCue CloseSound;

/** if set this mover blows up projectiles when it encroaches them */
var() bool bDestroyProjectilesOnEncroach;
/** true by default, prevents mover from completing the movement that would leave it encroaching another actor */
var() bool bStopOnEncroach;

event bool EncroachingOn(Actor Other)
{
	local int i;
	local SeqEvent_Mover MoverEvent;
	local Pawn P;
	local vector Height, HitLocation, HitNormal;
	local bool bLandingPawn;

	// if we're moving towards the actor
	if ( (Other.Base == self) || (Normal(Velocity) Dot Normal(Other.Location - Location) >= 0.f) )
	{
		// if we're moving up into a pawn, ignore it so it can land on us instead
		P = Pawn(Other);
		if (P != None)
		{
			if (P.Physics == PHYS_Falling && Velocity.Z > 0.f)
			{
				Height = P.GetCollisionHeight() * vect(0,0,1);
				// @note: only checking against our StaticMeshComponent, assumes we have no other colliding components
				if (TraceComponent(HitLocation, HitNormal, StaticMeshComponent, P.Location - Height, P.Location + Height, P.GetCollisionExtent()))
				{
					// make sure the pawn doesn't fall through us
					if (P.Location.Z < Location.Z)
					{
						P.SetLocation(HitLocation + Height);
					}
					bLandingPawn = true;
				}
			}
			else if (P.Base != self && P.Controller != None && P.Controller.PendingMover != None && P.Controller.PendingMover == self)
			{
				P.Controller.UnderLift(LiftCenter(MyMarker));
			}
		}
		else if (bDestroyProjectilesOnEncroach && Other.IsA('Projectile'))
		{
			Projectile(Other).Explode(Other.Location, -Normal(Velocity));
			return false;
		}

		if ( !bLandingPawn )
		{
			// search for any mover events
			for (i = 0; i < GeneratedEvents.Length; i++)
			{
				MoverEvent = SeqEvent_Mover(GeneratedEvents[i]);
				if (MoverEvent != None)
				{
					// notify the event that we encroached something
					MoverEvent.NotifyEncroachingOn(Other);
				}
			}
			return bStopOnEncroach;
		}
	}

	return false;
}

/*
 * called for encroaching actors which successfully moved the other actor out of the way
 */
event RanInto( Actor Other )
{
	local int i;
	local SeqEvent_Mover MoverEvent;

	if (bDestroyProjectilesOnEncroach && Other.IsA('Projectile'))
	{
		Projectile(Other).Explode(Other.Location, -Normal(Velocity));
	}
	else
	{
		// search for any mover events
		for (i = 0; i < GeneratedEvents.Length; i++)
		{
			MoverEvent = SeqEvent_Mover(GeneratedEvents[i]);
			if (MoverEvent != None)
			{
				// notify the event that we encroached something
				MoverEvent.NotifyEncroachingOn(Other);
			}
		}
	}
}


event Attach(Actor Other)
{
	local int i;
	local SeqEvent_Mover MoverEvent;

	// search for any mover events
	for (i = 0; i < GeneratedEvents.Length; i++)
	{
		MoverEvent = SeqEvent_Mover(GeneratedEvents[i]);
		if (MoverEvent != None)
		{
			// notify the event that an Actor has been attached
			MoverEvent.NotifyAttached(Other);
		}
	}
}

event Detach(Actor Other)
{
	local int i;
	local SeqEvent_Mover MoverEvent;

	// search for any mover events
	for (i = 0; i < GeneratedEvents.Length; i++)
	{
		MoverEvent = SeqEvent_Mover(GeneratedEvents[i]);
		if (MoverEvent != None)
		{
			// notify the event that an Actor has been detached
			MoverEvent.NotifyDetached(Other);
		}
	}
}

/** checks if anything is still attached to the mover, and if so notifies Kismet so that it may restart it if desired */
function Restart()
{
	local Actor A;

	foreach BasedActors(class'Actor', A)
	{
		Attach(A);
	}
}

/** called on a timer StayOpenTime seconds after the mover has finished opening (forward matinee playback) */
function FinishedOpen()
{
	local int i;
	local SeqEvent_Mover MoverEvent;

	// search for any mover events
	for (i = 0; i < GeneratedEvents.Length; i++)
	{
		MoverEvent = SeqEvent_Mover(GeneratedEvents[i]);
		if (MoverEvent != None)
		{
			// notify the event that all opening and associated delays are finished and it may now reverse our direction
			// (or do any other actions as set up in Kismet)
			MoverEvent.NotifyFinishedOpen();
		}
	}
}

simulated function PlayMovingSound(bool bClosing)
{
	local SoundCue SoundToPlay;

	SoundToPlay = bClosing ? CloseSound : OpenSound;
	if (SoundToPlay != None)
	{
		PlaySound(SoundToPlay, true);
	}
}

simulated event InterpolationStarted(SeqAct_Interp InterpAction)
{
	ClearTimer('Restart');
	ClearTimer('FinishedOpen');

	PlayMovingSound(InterpAction.bReversePlayback);
}

simulated event InterpolationFinished(SeqAct_Interp InterpAction)
{
	local DoorMarker DoorNav;
	local Controller C;

	DoorNav = DoorMarker(MyMarker);
	if (InterpAction.bReversePlayback)
	{
		// we are done; if something is still attached, set timer to try restart
		if (Attached.length > 0)
		{
			SetTimer(StayOpenTime, false, 'Restart');
		}
		if (DoorNav != None)
		{
			DoorNav.MoverClosed();
		}
	}
	else
	{
		// set timer to notify any mover events
		SetTimer(StayOpenTime, false, 'FinishedOpen');

		if (DoorNav != None)
		{
			DoorNav.MoverOpened();
		}
	}

	if (bMonitorMover)
	{
		// notify any Controllers with us as PendingMover that we have finished moving
		foreach WorldInfo.AllControllers(class'Controller', C)
		{
			if (C.PendingMover == self)
			{
				C.MoverFinished();
			}
		}
	}

	//@hack: force location update on clients if future matinee actions rely on it
	if (InterpAction.bNoResetOnRewind && InterpAction.bRewindOnPlay)
	{
		ForceNetRelevant();
		bUpdateSimulatedPosition = true;
		bReplicateMovement = true;
	}
}

simulated event InterpolationChanged(SeqAct_Interp InterpAction)
{
	PlayMovingSound(InterpAction.bReversePlayback);
}

defaultproperties
{
	Begin Object Name=MyLightEnvironment
		bEnabled=False
	End Object

	Begin Object Name=StaticMeshComponent0
		WireframeColor=(R=255,G=0,B=255,A=255)
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		RBCollideWithChannels=(Default=TRUE)
	End Object

	bStatic=false
	bWorldGeometry=false
	Physics=PHYS_Interpolating

	bNoDelete=true
	bAlwaysRelevant=true
	bSkipActorPropertyReplication=false
	bUpdateSimulatedPosition=false
	bOnlyDirtyReplication=true
	RemoteRole=ROLE_None
	NetPriority=2.7
	NetUpdateFrequency=1.0
	bDestroyProjectilesOnEncroach=true
	bStopOnEncroach=true
	bCollideWhenPlacing=FALSE

	SupportedEvents.Add(class'SeqEvent_Mover')
	SupportedEvents.Add(class'SeqEvent_TakeDamage')
}
