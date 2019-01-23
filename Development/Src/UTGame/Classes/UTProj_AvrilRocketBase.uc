/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_AvrilRocketBase extends UTProjectile;

/** Holds the vehicle this rocket is locked on to.  The vehicle will give us our Homing Target */
var Actor LockedTarget;

/** Holds a pointer back to the firing Avril - Only valid server side */
var UTWeap_Avril MyWeapon;
/** the AVRiL that is currently controlling the target lock (could be different if MyWeapon has no target) */
var UTWeap_Avril LockingWeapon;

/** Set to true if the lock for this rocket has been redirected */
var bool bRedirectedLock;

/** The last time a lock message was sent */
var float	LastLockWarningTime;

/** How long before re-sending the next Lock On message update */
var float	LockWarningInterval;

var float InitialPostRenderTime;

var Texture2D BeaconTexture;

replication
{
	if (bNetDirty && ROLE == ROLE_Authority)
		LockedTarget;
}

simulated event Destroyed()
{
	local PlayerController PC;

	if (Role==ROLE_Authority)
	{
		// Notify the launcher I'm dead
		if ( MyWeapon != None )
			MyWeapon.RocketDestroyed(self);
	}

	// remove from local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if ( UTHUD(PC.MyHUD) != None )
		{
			UTHUD(PC.MyHUD).RemovePostRenderedActor(self);
		}
	}

	super.Destroyed();
}

simulated function Tick(float DeltaTime)
{
	local Projectile P;

	//@TODO FIXMESTEVE move to C++ once gameplay validated
	if ( (LockedTarget != None) && LockedTarget.IsA('UTVehicle_Raptor') )
	{
		// check for nearby projectiles from target (explode if they are nearby, making me easier to kill)
		ForEach VisibleCollidingActors( class'Projectile', P, 40,, true)
		{
			if ( P.Instigator == LockedTarget )
			{
				P.Touch( self, None, P.Location, Normal(P.Velocity));
				if ( bDeleteMe || (Physics == PHYS_None) )
				{
					// destroyed
					break;
				}
			}
		}
	}
}

simulated function PostBeginPlay()
{
	local PlayerController PC;

	super.PostBeginPlay();
	SetTimer(0.1, true);	// Set the lock timer

	// add to local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if ( UTHUD(PC.MyHUD) != None )
		{
			UTHUD(PC.MyHUD).AddPostRenderedActor(self);
		}
	}
}

function ForceLock(Actor ForcedLock)
{
	LockedTarget = ForcedLock;
// don't get any more target updates from weapon.
	MyWeapon.RocketDestroyed(self);
	if ( (UTVehicle_SPMA(ForcedLock) != None) && (PlayerController(InstigatorController) != None) )
		PlayerController(InstigatorController).ReceiveLocalizedMessage(class'UTLockWarningMessage', 3);
}

/**
 * Clean up
 */
simulated function Shutdown()
{
	local PlayerController PC;

	if (Role==ROLE_Authority)
	{
		// Notify the launcher I'm dead
		if ( MyWeapon != None )
		{
			MyWeapon.RocketDestroyed(self);
			MyWeapon = None;
		}
	}

	// remove from local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if ( UTHUD(PC.MyHUD) != None )
		{
			UTHUD(PC.MyHUD).RemovePostRenderedActor(self);
		}
	}
	Super.ShutDown();

	SetTimer(0.0,false);
}

/**
 * This timer holds the AI needed to home in on a target
 */
simulated function Timer()
{
	local actor ActualTarget;
	local vector Dir, ForceDir;
	local float VelMag, LowestDesiredZ;
	local UTVehicle VehicleTarget;

	// possibly warn target
	VehicleTarget = UTVehicle(LockedTarget);
	if ( VehicleTarget !=None )
	{
		if ( WorldInfo.TimeSeconds - LastLockWarningTime > LockWarningInterval )
		{
			LastLockWarningTime = WorldInfo.TimeSeconds;

			// warning message for players
			VehicleTarget.LockOnWarning(self);

			// update LockWarningInterval based on target proximity
			LockWarningInterval = FClamp(0.25*VSize(Location - LockedTarget.Location)/VSize(Velocity), 0.1, 1.5);
		}
	}

	if ( Role < ROLE_Authority )
		return;

	// If we are the server and have a valid target lock, ask the vehicle who we should be tracking.
	if ( VehicleTarget != None )
	{
		ActualTarget = VehicleTarget.GetHomingTarget(self, InstigatorController);
	}
	else if ( UTProjectile(LockedTarget) != none)
	{
		ActualTarget = UTProjectile(LockedTarget).GetHomingTarget(self, InstigatorController);
	}
	else
	{
		ActualTarget = LockedTarget;
	}

	// If we are homing in on a target, do it here.
	if (ActualTarget != none)
	{
		if ( UTVehicle(ActualTarget) != none)
		{
			// warning for AI
			UTVehicle(ActualTarget).IncomingMissile(self);
		}

		Dir = ActualTarget.GetTargetLocation() - Location;

		VelMag = VSize(Velocity);

		if ( VelMag == 0 )
			VelMag = 1;

		ForceDir = Dir + ActualTarget.Velocity * VSize(Dir) / (VelMag * 2);

		if (Instigator != None)
			LowestDesiredZ = FMin(Instigator.Location.Z, ActualTarget.Location.Z); //missile should avoid going any lower than this
		else
			LowestDesiredZ = ActualTarget.Location.Z;

		if (ForceDir.Z + Location.Z < LowestDesiredZ)
    		ForceDir.Z += LowestDesiredZ - (ForceDir.Z + Location.Z);

		ForceDir = Normal(ForceDir * 0.8 * VelMag + Velocity);
		Velocity =  VelMag * ForceDir;
		Acceleration += 5 * ForceDir;
	}
}

/** sets the target we're locked on to. May fail (return false) if NewLockOwner isn't allowed to control me */
function bool SetTarget(Actor NewTarget, UTWeap_Avril NewLockOwner)
{
	// follow the lock if it's the shooter's target or the shooter doesn't have a target and a friendly player is giving one
	// short delay before accepting other players' targets to avoid rocket twisting through shooter's body, etc
	if ( NewLockOwner == MyWeapon ||
		( (LockingWeapon == None || LockingWeapon == NewLockOwner) && WorldInfo.TimeSeconds - CreationTime > 2.0 &&
			WorldInfo.GRI.OnSameTeam(NewLockOwner.Instigator, InstigatorController) ) )
	{
		LockedTarget = NewTarget;
		LockingWeapon = (LockedTarget != None) ? NewLockOwner : None;
		return true;
	}
	else
	{
		return false;
	}
}

event TakeDamage(int DamageAmount, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	if (Damage > 0 && (InstigatorController == None || !WorldInfo.GRI.OnSameTeam(EventInstigator, InstigatorController)))
	{
		Explode(HitLocation, vect(0,0,0));
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
	local vector ScreenLoc;
	local float CrossScaler, CrossScaleTime;
	local LinearColor TeamColor;
	local color c;
	local float ResScale, XStart, XLen, YStart, YLen, OldY;
	local float width,height;

	// Only render for UTAirVehicle HUD if targeting it, and rendered
	if ( (LockedTarget != PC.Pawn) || (UTAirVehicle(PC.Pawn) == None)
		|| !LocalPlayer(PC.Player).GetActorVisibility(self) )
		return;

	// don't show friendly AVRiLs
	if ( WorldInfo.GRI.OnSameTeam(self, PC) )
		return;

	// make sure visible
	if ( !FastTrace(Location, CameraPosition) )
		return;

	screenLoc = Canvas.Project(Location);

	class'UTHud'.static.GetTeamColor( 1 - PC.Pawn.GetTeamNum(), TeamColor, c);
	CrossScaleTime = FMax(0.05,(1 - 3*(WorldInfo.TimeSeconds - InitialPostRenderTime)));
	TeamColor.A = 0.8 - CrossScaleTime;

	ResScale = Canvas.ClipX / 1024;

	if ( InitialPostRenderTime == 0.0 )
	{
		InitialPostRenderTime = WorldInfo.TimeSeconds;
	}
	CrossScaler = CrossScaleTime * Canvas.ClipX;
	width = ResScale * CrossScaler;
	height = ResScale * CrossScaler;
	XStart = 662;
	YStart = 260;
	XLen = 56;
	YLen = 56;

	// if clipped out, draw offscreen indicator
	if (screenLoc.X < 0 ||
		screenLoc.X >= Canvas.ClipX ||
		screenLoc.Y < 0 ||
		screenLoc.Y >= Canvas.ClipY)
	{
		OldY = screenLoc.Y;
		screenLoc.X = FClamp(ScreenLoc.X, 0, Canvas.ClipX-1);
		screenLoc.Y = FClamp(ScreenLoc.Y, 0, Canvas.ClipY-1);
		if ( screenLoc.Y != OldY)
		{
			// draw up/down arrow
			YLen = 28;
			if ( screenLoc.Y == 0 )
			{
				YStart += YLen;
			}
		}
		else
		{
			// draw horizontal arrow
			XLen = 28;
			if ( screenLoc.X == 0 )
			{
				XStart += XLen;
			}
		}
	}

	Canvas.SetPos(ScreenLoc.X - width * 0.5, ScreenLoc.Y - height * 0.5);
	Canvas.DrawColorizedTile(BeaconTexture, width, height, XStart, YStart, XLen, YLen, TeamColor);
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
	BeaconTexture=Texture2D'UI_HUD.HUD.UI_HUD_BaseA'
}
