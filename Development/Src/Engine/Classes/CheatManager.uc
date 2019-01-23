//=============================================================================
// CheatManager
// Object within playercontroller that manages "cheat" commands
// only spawned in single player mode
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================

class CheatManager extends Object within PlayerController
	native;

exec function ListDynamicActors()
{
`if(`notdefined(FINAL_RELEASE))
	local Actor A;
	local int i;

	ForEach DynamicActors(class'Actor',A)
	{
		i++;
		`log(i@A);
	}
	`log("Num dynamic actors: "$i);
`endif
}

exec function FreezeFrame(float delay)
{
	WorldInfo.Game.SetPause(Outer,Outer.CanUnpause);
	WorldInfo.PauseDelay = WorldInfo.TimeSeconds + delay;
}

exec function WriteToLog( string Param )
{
	`log("NOW! "$Param);
}

exec function KillViewedActor()
{
	if ( ViewTarget != None )
	{
		if ( (Pawn(ViewTarget) != None) && (Pawn(ViewTarget).Controller != None) )
			Pawn(ViewTarget).Controller.Destroy();
		ViewTarget.Destroy();
		SetViewTarget(None);
	}
}

/* Teleport()
Teleport to surface player is looking at
*/
exec function Teleport()
{
	local Actor		HitActor;
	local vector	HitNormal, HitLocation;
	local vector	ViewLocation;
	local rotator	ViewRotation;

	GetPlayerViewPoint( ViewLocation, ViewRotation );

	HitActor = Trace(HitLocation, HitNormal, ViewLocation + 1000000 * vector(ViewRotation), ViewLocation, true);
	if ( HitActor != None)
		HitLocation += HitNormal * 4.0;

	ViewTarget.SetLocation( HitLocation );
}

/*
Scale the player's size to be F * default size
*/
exec function ChangeSize( float F )
{
	Pawn.CylinderComponent.SetCylinderSize( Pawn.Default.CylinderComponent.CollisionRadius * F, Pawn.Default.CylinderComponent.CollisionHeight * F );
	Pawn.SetDrawScale(F);
	Pawn.SetLocation(Pawn.Location);
}

/* Stop interpolation
*/
exec function EndPath()
{
}

exec function Amphibious()
{
	Pawn.UnderwaterTime = +999999.0;
}

exec function Fly()
{
	if ( (Pawn != None) && Pawn.CheatFly() )
	{
		ClientMessage("You feel much lighter");
		bCheatFlying = true;
		Outer.GotoState('PlayerFlying');
	}
}

exec function Walk()
{
	bCheatFlying = false;
	if (Pawn != None && Pawn.CheatWalk())
	{
		Restart(false);
	}
}

exec function Ghost()
{
	if ( (Pawn != None) && Pawn.CheatGhost() )
	{
		bCheatFlying = true;
		Outer.GotoState('PlayerFlying');
	}
	else
	{
		bCollideWorld = false;
	}

	ClientMessage("You feel ethereal");
}

/* AllAmmo
	Sets maximum ammo on all weapons
*/
exec function AllAmmo();

exec function God()
{
	if ( bGodMode )
	{
		bGodMode = false;
		ClientMessage("God mode off");
		return;
	}

	bGodMode = true;
	ClientMessage("God Mode on");
}

exec function SloMo( float T )
{
	WorldInfo.Game.SetGameSpeed(T);
}

exec function SetJumpZ( float F )
{
	Pawn.JumpZ = F;
}

exec function SetGravity( float F )
{
	WorldInfo.WorldGravityZ = F;
}

exec function SetSpeed( float F )
{
	Pawn.GroundSpeed = Pawn.Default.GroundSpeed * f;
	Pawn.WaterSpeed = Pawn.Default.WaterSpeed * f;
}

exec function KillAll(class<actor> aClass)
{
	local Actor A;
`if(`notdefined(FINAL_RELEASE))
	local PlayerController PC;

	foreach WorldInfo.AllControllers(class'PlayerController', PC)
	{
		PC.ClientMessage("Killed all "$string(aClass));
	}
`endif

	if ( ClassIsChildOf(aClass, class'AIController') )
	{
		WorldInfo.Game.KillBots();
		return;
	}
	if ( ClassIsChildOf(aClass, class'Pawn') )
	{
		KillAllPawns(class<Pawn>(aClass));
		return;
	}
	ForEach DynamicActors(class 'Actor', A)
		if ( ClassIsChildOf(A.class, aClass) )
			A.Destroy();
}

// Kill non-player pawns and their controllers
function KillAllPawns(class<Pawn> aClass)
{
	local Pawn P;

	WorldInfo.Game.KillBots();
	ForEach DynamicActors(class'Pawn', P)
		if ( ClassIsChildOf(P.Class, aClass)
			&& !P.IsPlayerPawn() )
		{
			if ( P.Controller != None )
				P.Controller.Destroy();
			P.Destroy();
		}
}

exec function KillPawns()
{
	KillAllPawns(class'Pawn');
}

/**
 * Possess a pawn of the requested class
 */
exec function Avatar( name ClassName )
{
	local Pawn			P, TargetPawn, FirstPawn, OldPawn;
	local bool			bPickNextPawn;

	Foreach DynamicActors(class'Pawn', P)
	{
		if( P == Pawn )
		{
			bPickNextPawn = TRUE;
		}
		else if( P.IsA(ClassName) )
		{
			if( FirstPawn == None )
			{
				FirstPawn = P;
			}

			if( bPickNextPawn )
			{
				TargetPawn = P;
				break;
			}
		}
	}

	// if we went through the list without choosing a pawn, pick first available choice (loop)
	if( TargetPawn == None )
	{
		TargetPawn = FirstPawn;
	}

	if( TargetPawn != None )
	{
		// detach TargetPawn from its controller and kill its controller.
		TargetPawn.DetachFromController( TRUE );

		// detach player from current pawn and possess targetpawn
		if( Pawn != None )
		{
			OldPawn = Pawn;
			Pawn.DetachFromController();
		}

		Possess(TargetPawn, FALSE);

		// Spawn default controller for our ex-pawn (AI)
		if( OldPawn != None )
		{
			OldPawn.SpawnDefaultController();
		}
	}
	else
	{
		`log("Avatar: Couldn't find any Pawn to possess of class '" $ ClassName $ "'");
	}
}

exec function Summon( string ClassName )
{
	local class<actor> NewClass;
	local vector SpawnLoc;

	`log( "Fabricate " $ ClassName );
	NewClass = class<actor>( DynamicLoadObject( ClassName, class'Class' ) );
	if( NewClass!=None )
	{
		if ( Pawn != None )
			SpawnLoc = Pawn.Location;
		else
			SpawnLoc = Location;
		Spawn( NewClass,,,SpawnLoc + 72 * Vector(Rotation) + vect(0,0,1) * 15 );
	}
}

/**
 * Give a specified weapon to the Pawn.
 * If weapon is not carried by player, then it is created.
 * Weapon given is returned as the function's return parmater.
 */
exec function Weapon GiveWeapon( String WeaponClassStr )
{
	Local Weapon		Weap;
	local class<Weapon> WeaponClass;

	WeaponClass = class<Weapon>(DynamicLoadObject(WeaponClassStr, class'Class'));
	Weap		= Weapon(Pawn.FindInventoryType(WeaponClass));
	if( Weap != None )
	{
		return Weap;
	}
	return Weapon(Pawn.CreateInventory( WeaponClass ));
}

exec function PlayersOnly()
{
	WorldInfo.bPlayersOnly = !WorldInfo.bPlayersOnly;
}

// ***********************************************************
// Navigation Aids (for testing)

// remember spot for path testing (display path using ShowDebug)
exec function RememberSpot()
{
	if ( Pawn != None )
		Destination = Pawn.Location;
	else
		Destination = Location;
}

// ***********************************************************
// Changing viewtarget

exec function ViewSelf(optional bool bQuiet)
{
	Outer.ResetCameraMode();
	if ( Pawn != None )
		SetViewTarget(Pawn);
	else
		SetViewtarget(outer);
	if (!bQuiet )
		ClientMessage(OwnCamera, 'Event');

	FixFOV();
}

exec function ViewPlayer( string S )
{
	local Controller P;

	foreach WorldInfo.AllControllers(class'Controller', P)
	{
		if ( P.bIsPlayer && (P.PlayerReplicationInfo.PlayerName ~= S) )
		{
			break;
		}
	}

	if ( P.Pawn != None )
	{
		ClientMessage(ViewingFrom@P.PlayerReplicationInfo.PlayerName, 'Event');
		SetViewTarget(P.Pawn);
	}
}

exec function ViewActor( name ActorName)
{
	local Actor A;

	ForEach AllActors(class'Actor', A)
		if ( A.Name == ActorName )
		{
			SetViewTarget(A);
	    SetCameraMode('ThirdPerson');
			return;
		}
}

exec function ViewFlag()
{
	local AIController C;

	foreach WorldInfo.AllControllers(class'AIController', C)
	{
		if (C.PlayerReplicationInfo != None && C.PlayerReplicationInfo.bHasFlag)
		{
			SetViewTarget(C.Pawn);
			return;
		}
	}
}

exec function ViewBot()
{
	local actor first;
	local bool bFound;
	local AIController C;

	foreach WorldInfo.AllControllers(class'AIController', C)
	{
		if (C.Pawn != None && C.PlayerReplicationInfo != None)
		{
			if (bFound || first == None)
			{
				first = C;
				if (bFound)
				{
					break;
				}
			}
			if (C.PlayerReplicationInfo == RealViewTarget)
			{
				bFound = true;
			}
		}
	}

	if ( first != None )
	{
		`log("view "$first);
		SetViewTarget(first);
		SetCameraMode( 'ThirdPerson' );
		FixFOV();
	}
	else
		ViewSelf(true);
}

exec function ViewClass( class<actor> aClass )
{
	local actor other, first;
	local bool bFound;

	first = None;

	ForEach AllActors( aClass, other )
	{
		if ( bFound || (first == None) )
		{
			first = other;
			if ( bFound )
				break;
		}
		if ( other == ViewTarget )
			bFound = true;
	}

	if ( first != None )
	{
		if ( Pawn(first) != None )
			ClientMessage(ViewingFrom@First.GetHumanReadableName(), 'Event');
		else
			ClientMessage(ViewingFrom@first, 'Event');
		SetViewTarget(first);
		FixFOV();
	}
	else
		ViewSelf(false);
}

exec function Loaded()
{
	if( WorldInfo.Netmode!=NM_Standalone )
		return;

    AllWeapons();
    AllAmmo();
}

/* AllWeapons
	Give player all available weapons
*/
exec function AllWeapons()
{
	// subclass me
}

/** streaming level debugging */

function SetLevelStreamingStatus(name PackageName, bool bShouldBeLoaded, bool bShouldBeVisible)
{
	local PlayerController PC;
	local int i;

	if (PackageName != 'All')
	{
		foreach WorldInfo.AllControllers(class'PlayerController', PC)
		{
			PC.ClientUpdateLevelStreamingStatus(PackageName, bShouldBeLoaded, bShouldBeVisible, FALSE );
		}
	}
	else
	{
		foreach WorldInfo.AllControllers(class'PlayerController', PC)
		{
			for (i = 0; i < WorldInfo.StreamingLevels.length; i++)
			{
				PC.ClientUpdateLevelStreamingStatus(WorldInfo.StreamingLevels[i].PackageName, bShouldBeLoaded, bShouldBeVisible, FALSE );
			}
		}
	}
}

exec function StreamLevelIn(name PackageName)
{
	SetLevelStreamingStatus(PackageName, true, true);
}

exec function OnlyLoadLevel(name PackageName)
{
	SetLevelStreamingStatus(PackageName, true, false);
}

exec function StreamLevelOut(name PackageName)
{
	SetLevelStreamingStatus(PackageName, false, false);
}

/**
 * Toggle between debug camera/player camera without locking gameplay and with locking
 * local player controller input.
 */
exec function ToggleDebugCamera()
{
	local PlayerController PC;
	local DebugCameraController DCC;

	foreach WorldInfo.AllControllers(class'PlayerController', PC)
	{
		if ( PC.bIsPlayer && PC.IsLocalPlayerController() )
		{
			DCC = DebugCameraController(PC);
			if( DCC!=none && DCC.OryginalControllerRef==none )
			{
				//dcc are disabled, so we are looking for normal player controller
				continue;
			}
			break;
		}
	}

	if( DCC!=none && DCC.OryginalControllerRef!=none )
	{
		DCC.DisableDebugCamera();
	}
	else if( PC!=none )
	{
		PC.EnableDebugCamera();
	}
}

defaultproperties
{
}
