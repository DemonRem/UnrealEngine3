/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTPlayerInput extends GamePlayerInput within UTPlayerController
	native;

var float LastDuckTime;
var bool  bHoldDuck;
var Actor.EDoubleClickDir ForcedDoubleClick;



// Accessor delegates
delegate bool OnInputKey(int ControllerID, name Key, EInputEvent Event, float AmountDepressed, bool bGamepad );
delegate bool OnInputAxis(int ControllerID, name Key, float Delta, float DeltaTime);
delegate bool OnInputChar(int ControllerID, string Unicode);

event bool InputAxis(int ControllerId,name Key,float Delta,float DeltaTime,optional bool bGamepad=FALSE)
{
	if ( OnInputAxis(ControllerID, Key, Delta, DeltaTime) )
	{
		return true;
	}
	else
	{
		return Super.InputAxis(ControllerID, Key, Delta, DeltaTime);
	}
}

event bool InputKey(int ControllerID, name Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = FALSE)
{
	if ( OnInputKey(ControllerID, Key, Event, AmountDepressed, bGamepad) )
	{
		return true;
	}
	else
	{
		return Super.InputKey(ControllerID, Key, Event, AmountDepressed, bGamepad);
	}
}

event bool InputChar(int ControllerID, string Unicode)
{
	if ( OnInputChar(ControllerID, Unicode) )
	{
		return true;
	}
	else
	{
		return Super.InputChar(ControllerId, Unicode);
	}
}


simulated exec function Duck()
{
	if ( UTPawn(Pawn)!= none )
	{
		if (bHoldDuck)
		{
			bHoldDuck=false;
			bDuck=0;
			return;
		}

		bDuck=1;

		if ( WorldInfo.TimeSeconds - LastDuckTime < DoubleClickTime )
		{
			bHoldDuck = true;
		}

		LastDuckTime = WorldInfo.TimeSeconds;
	}
}

simulated exec function UnDuck()
{
	if ( UTPawn(Pawn)!= none && !bHoldDuck )
	{
		bDuck=0;
	}
}

exec function Jump()
{
	local UTPawn P;

	// jump cancels feign death
	P = UTPawn(Pawn);
	if (P != None && P.bFeigningDeath)
	{
		P.FeignDeath();
	}
	else
	{
	 	if (bDuck>0)
	 	{
	 		bDuck = 0;
	 		bHoldDuck = false;
	 	}
		Super.Jump();
	}
}

event PlayerInput( float DeltaTime )
{
	local UTPawn UTP;
	local UTWeapon UTWeap;

	local vector Dir;
	local float Dist, Ang;
	local int Quad;

	if ( QuickPickScene != none && QuickPickScene.IsVisible() )
	{

		Dir.X = aTurn;
		Dir.Y = aLookup;

		Dist = vsize(Dir);
		Ang = (static.GetHeadingAngle(Dir) * 57.2957795) + 22.5;	// Convert to Degrees then offset

		if (Ang < 0 )
		{
			Ang = 360 + Ang;
		}

		if ( Dist > 0.6 )
		{
			Quad = Int( Abs(Ang) / 45.0 );
		}
		else
		{
			Quad = -1;
		}

		QuickPick(Quad);

		aLookup = 0.0;
		aTurn = 0.0;
	}

	Super.PlayerInput(Deltatime);

	UTP = UTPawn(Pawn);
	if (UTP != none && UTP.Weapon != none)
	{
		UTWeap = UTWeapon(Pawn.Weapon);
		if (UTWeap != none)
		{
			UTWeap.ThrottleLook(aTurn, aLookup );
		}
	}

//	`log(""@aTurn@aLookup);

}


defaultproperties
{
	bEnableFOVScaling=true
}
