/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class GameCrowdBehavior_RunFromPanic extends GameCrowdAgentBehavior
	native
	placeable
	dependsOn(GameCrowdAgent);
	
/** Actor who caused panic - if set, flee this actor */
var transient Actor PanicFocus;

/** How long this Panic will last **/
var() float DurationOfPanic;

/** The time we will stop panicking (we may start up immediately after but we will stop first)**/
var transient float TimeToStopPanic;


function ActivatedBy(Actor NewActionTarget)
{
	local GameCrowdDestination TempDest, PrevDest;

	// don't pass action target to super - we don't want to look at it
	PanicFocus = NewActionTarget; 
	PrevDest = MyAgent.PreviousDestination;

	// see if already heading away from danger
	if ( (MyAgent.CurrentDestination != None) && AllowThisDestination(MyAgent.CurrentDestination) )
	{
		return;
	}
	else if ( (PrevDest != None) && PrevDest.AllowableDestinationFor(MyAgent) )
	{
		// try heading back where agent was coming from
		TempDest = MyAgent.CurrentDestination;
		MyAgent.CurrentDestination.DecrementCustomerCount(MyAgent);
		MyAgent.SetCurrentDestination(MyAgent.PreviousDestination);
		MyAgent.PreviousDestination = TempDest;
		MyAgent.UpdateIntermediatePoint();
	}
}
	
function InitBehavior(GameCrowdAgent Agent)
{
	super.InitBehavior(Agent);

	MyAgent.bIsPanicked = true; //just for running anim
	MyAgent.SetMaxSpeed();

	if( DurationOfPanic > 0.0f )
	{
		TimeToStopPanic = MyAgent.WorldInfo.TimeSeconds + DurationOfPanic;
	}
}

function StopBehavior()
{
	Super.StopBehavior();
	MyAgent.bIsPanicked = false; //just for running anim
	MyAgent.SetMaxSpeed();
}

event PropagateViralBehaviorTo(GameCrowdAgent OtherAgent)
{
	if ( !OtherAgent.IsPanicked() )
	{
		OtherAgent.SetPanic(PanicFocus, true);

		if( OtherAgent.CurrentBehavior.bPassOnIsViralBehaviorFlag == FALSE )
		{
			//`log( "setting bIsViralBehavior to FALSE on propagated behavior" @ `showvar(OtherAgent.CurrentBehavior) @ `showvar(MyAgent) @ `showvar(OtherAgent));
			OtherAgent.CurrentBehavior.bIsViralBehavior = FALSE;
		}
	}
}

/**
  * Return true if agent is allowed to go to destination while panicked
  */
function bool AllowThisDestination(GameCrowdDestination Destination)
{
	return !Destination.bAvoidWhenPanicked && !Destination.AtCapacity() && (Destination.bFleeDestination || (PanicFocus == None) || (((Destination.Location - MyAgent.Location) dot (MyAgent.Location - PanicFocus.Location)) > 0.0));
}

/** 
  * return true if get kismet or new behavior from this destination
  */
function bool AllowBehaviorAt(GameCrowdDestination Destination)
{
	return !Destination.bSkipBehaviorIfPanicked && !Destination.bAvoidWhenPanicked;
}

function string GetBehaviorString()
{
	return "Run from PANIC "@PanicFocus;
}

/**
  *  Agent's current behavior gets ticked
  */
native function Tick(float DeltaTime);


defaultproperties
{
	bIsViralBehavior=TRUE
	bIsPanicked=TRUE
	MaxPlayerDistance=20000.0

	DurationOfPanic=0.0f
}
