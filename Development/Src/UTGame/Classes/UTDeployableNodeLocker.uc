/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTDeployableNodeLocker extends UTDeployablePickupFactory;

var byte CurrentTeam;
var name NextState;
var() UTOnslaughtObjective ONSObjectiveOverride;

simulated function PostBeginPlay()
{
	if(Role != Role_Authority) // Authority sets this in game type.
	{
		AddToClosestObjective();
	}
	Super.PostBeginPlay();
}

simulated function AddToClosestObjective()
{
	local UTGameObjective O, Best;
	local float Distance, BestDistance;

	if (ONSObjectiveOverride != None)
	{
		Best = ONSObjectiveOverride;
	}
	else if ( UTOnslaughtGame(WorldInfo.Game) != None )
	{
		Best = UTOnslaughtGame(WorldInfo.Game).ClosestObjectiveTo(self);
	}
	else
	{
		foreach WorldInfo.AllNavigationPoints(class'UTGameObjective', O)
		{
			Distance = VSize(Location - O.Location);
			if ( (Best == None) || (Distance < BestDistance) )
			{
				BestDistance = Distance;
				Best = O;
			}
		}
	}
	if ( Best != None )
	{
		Best.DeployableLockers[Best.DeployableLockers.Length] = self;
	}
}

function Activate(byte T)
{
	CurrentTeam = T;
	GotoState(NextState);
}
function DeActivate()
{
	Gotostate('Inactive');
}

function Reset()
{
	NextState='Sleeping';
	Gotostate('Inactive');
}
function StartSleeping()
{
	NextState = bDelayRespawn ? 'WaitingForDeployable' : 'Sleeping';
	super.StartSleeping();
}
state Pickup
{
	function bool ValidTouch( Pawn Other )
	{
		if(Other.GetTeamNum() == CurrentTeam)
		{
			return Super.ValidTouch(Other);
		}
		return false;
	}
	function Activate(byte T)
	{
		CurrentTeam = T;
	}



		/**
	 * PostRenderFor() Hook to allow pawns to render HUD overlays for themselves.
	 * Called only if pawn was rendered this tick. Assumes that appropriate font has already been set
	 *
	 * @param	PC		The Player Controller who is rendering this pawn
	 * @param	Canvas	The canvas to draw on
	 *
	 * -- Code modified from UTVehicle to draw locked icon
	 */
	simulated function PostRenderFor(PlayerController PC, Canvas Canvas, vector CameraPosition, vector CameraDir)
	{
		local float Dist, xscale, IconDistScale;
		local vector ScreenLoc, HitNormal, HitLocation;
		local actor HitActor;

		// must have been rendered
		if ( !LocalPlayer(PC.Player).GetActorVisibility(self) )
			return;

		// must be in front of player to render HUD overlay
		if ( (CameraDir dot (Location - CameraPosition)) <= 0 )
			return;

		// only render overlay for teammates or if un-piloted
		if ( (WorldInfo.GRI == None) || (PC.ViewTarget == None))
			return;

		Dist = VSize(CameraPosition - Location);

		if (PC.GetTeamNum() != CurrentTeam)
		{
				screenLoc = Canvas.Project(Location);
				if (screenLoc.X >= 0 &&
					screenLoc.X < Canvas.ClipX &&
					screenLoc.Y >= 0 &&
					screenLoc.Y < Canvas.ClipY)
				{
					// draw no entry indicator
					// don't draw it if there's something else in front of the vehicle
					if ( FastTrace(Location, CameraPosition) || FastTrace(Location+CylinderComponent.CollisionHeight*vect(0,0,0.5), CameraPosition) )
					{
						HitActor = Trace(HitLocation, HitNormal, Location, CameraPosition, true,,,TRACEFLAG_Blocking);
						if ( (HitActor != None) && !HitActor.bWorldGeometry )
						{
							if ( UTVehicle(HitActor) != None )
								return;
						}
					}

					// draw locked symbol
					IconDistScale = 3000.0;
					xscale = FClamp( (2*IconDistScale - Dist)/(2*IconDistScale), 0.55f, 1.f);
					xscale = xscale * xscale;
					Canvas.SetPos(ScreenLoc.X-16*xscale,ScreenLoc.Y-16*xscale);
					Canvas.DrawColor = class'UTHUD'.Default.WhiteColor;
					Canvas.DrawTile(class'UTHUD'.Default.HudTexture,32*xscale, 32*xscale,193,357,30,30);
				}
			return;
		}
	}

}
auto State Inactive
{
	ignores Touch;

	function bool ReadyToPickup(float MaxWait)
	{
		return false;
	}
	function BeginState(name PrevStateName)
	{
		SetPickupHidden();
	}

	function StartSleeping()
	{
		NextState = bDelayRespawn ? 'WaitingForDeployable' : 'Sleeping';
	}

	function DeployableUsed(actor ChildDeployable)
	{
		NextState = 'Sleeping';
	}

Begin:
}

state WaitingForDeployable
{
	ignores Touch;
	function DeployableUsed(actor ChildDeployable)
	{
		Super.DeployableUsed(ChildDeployable);
		NextState = 'Sleeping';
	}
}


defaultproperties
{
	NextState=Sleeping
}
