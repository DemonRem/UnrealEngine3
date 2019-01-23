/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_TowCable extends UTVehicleWeapon
	hidedropdown
	native(Vehicle);

/** True if we are linked to a vehicle */
var bool bLinked;

var float LastLinkStartTime;

/** A quick reference to the hoverboard that owns this gun */
var UTVehicle_Hoverboard 	MyHoverboard;

/** Set natively on the server and replicated to the client.  This
	holds the reference of any potential lock */
var UTVehicle PotentialTowTruck;

/** Sound to play when the cable is recalled */
var SoundCue FireFailSound;

/** How far away from the hoverboard can a vehicle be to attach */
var float MaxAttachRange;

var float CrossScaler;
var float CrossScaleTime;

var UTVehicle LastPotentialTowTruck;
var name LastPointName;

var Texture2D CrossHairTexture;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

replication
{
	if (bNetDirty)
		MyHoverBoard, PotentialTowTruck;
}

/**
 * Cache a reference to the hoverboard
 */
simulated function PostBeginPlay()
{
	if ( Role == ROLE_Authority )
	{
		MyHoverboard = UTVehicle_Hoverboard(Instigator);
	}

	Super.PostBeginPlay();
}

simulated function EndFire(Byte FireModeNum)
{
	if(MyHoverboard != None && MyHoverboard.Role == ROLE_Authority)
	{
		if(FireModeNum == 0)
		{
			MyHoverboard.bGrab1 = FALSE;
		}
		else if(FireModeNum == 1)
		{
			MyHoverboard.bGrab2 = FALSE;
		}
	}

	if (Role == ROLE_Authority && IsLinked())
	{
		if(WorldInfo.TimeSeconds - LastLinkStartTime >= 1.5 * FireInterval[0])
		{
			MyHoverBoard.BreakTowLink();
		}
	}
}
/**
 * @Returns true if we are linked to another vehicle
 */
simulated function bool IsLinked()
{
	return ( MyHoverBoard != none && MyHoverboard.bInTow );
}


/**
 * If we are linked, kill the link, otherwise if we can link, make the link
 */

simulated function CustomFire()
{
}
simulated function BeginFire(Byte FireModeNum)
{
	if(MyHoverboard != None && MyHoverboard.Role == ROLE_Authority)
	{
		if(FireModeNum == 0)
		{
			MyHoverboard.bGrab1 = TRUE;
		}
		else if(FireModeNum == 1)
		{
			MyHoverboard.bGrab2 = TRUE;
		}
	}

	if ( IsLinked() )
	{
		if ( MyHoverBoard != none && MyHoverboard.bInTow )
		{
			MyHoverBoard.BreakTowLink();
		}
	}
	else if (PotentialTowTruck != none )
	{
		if(Role == ROLE_Authority)
		{
			CheckPossibleLink(PotentialTowTruck);
		}
	}
	else
	{
		PlaySound(FireFailSound);
	}
}

/**
 * Attempt to link to another vehicle
 */
function CheckPossibleLink(UTVehicle TowTruck)
{
	if (MyHoverBoard != None)
	{
		MyHoverBoard.LinkUp(TowTruck);
		if(MyHoverBoard.bInTow)
		{
			LastLinkStartTime = WorldInfo.TimeSeconds;
		}
	}
}


/**
 * Make sure we break the link before being destroyed
 */
simulated function Destroyed()
{
	if ( MyHoverBoard != none && MyHoverboard.bInTow )
	{
		MyHoverBoard.BreakTowLink();
	}

	Super.Destroyed();
}


simulated function DrawWeaponCrosshair( Hud HUD )
{
	return; // No crosshair for towcable.
}

simulated function ActiveRenderOverlays( HUD H )
{
	local string Msg;
	local name PointName;
	local vector V,ScreenLoc;
	local LinearColor TeamColor;
	local color c;
	local float ResScale;
	local float width,height;

	if ( PotentialTowTruck != none && PotentialTowTruck.Health > 0 && !IsLinked() )
	{
		PointName = PotentialTowTruck.GetHoverBoardAttachPoint(MyHoverBoard.Location);

		if (LastPotentialTowTruck != PotentialTowTruck || LastPointName != PointName )
		{
			ResetCrosshair(H.Canvas);
		}

		if (!PotentialTowTruck.Mesh.GetSocketWorldLocationAndRotation(PointName,V) )
		{
			V = PotentialTowTruck.Location + Vect(0,0,128);
		}

		ScreenLoc = H.Canvas.Project(v);

		class'UTHud'.static.GetTeamColor( MyVehicle.GetTeamNum(), TeamColor, c);
		TeamColor.A = 0.25 + (0.75 * (1.0 - (CrossScaleTime / 0.33)));

		ResScale = H.Canvas.ClipX / 1024;

		width = 56 * ResScale * CrossScaler;
		height = 56 * ResScale * CrossScaler;

		H.Canvas.SetPos(ScreenLoc.X - width * 0.5, ScreenLoc.Y - height * 0.5);
		H.Canvas.DrawColorizedTile(CrossHairTexture, width, height, 662,260,56,56, TeamColor);

    	msg = class'UTSPMAMessage'.Static.GetString(1);
    	H.LocalizedMessage(class'UTSPMAMessage',none,Msg,1,Class'UTSPMAMessage'.Default.PosY, 0.5,
    							Class'UTSPMAMessage'.Default.FontSize, Class'UTSPMAMessage'.Default.DrawColor);
	}

	LastPotentialTowTruck = PotentialTowTruck;
	LastPointName = PointName;

}

simulated function ResetCrosshair(Canvas Canvas)
{
	CrossScaler = Canvas.ClipX / 56;
	CrossScaleTime = 0.33;
}


defaultproperties
{
	WeaponFireTypes(0)=EWFT_Custom
	WeaponFireTypes(1)=EWFT_Custom

	WeaponFireSnd[0]=none
	WeaponFireSnd[1]=none
	FireInterval(0)=+0.6
	ShotCost(0)=0
	ShotCost(1)=0

	LockRange=1600.0

	FireFailSound=SoundCue'A_Vehicle_Hoverboard.Cue.A_Vehicle_Hoverboard_GrappleFailCue'

	// Let rider look anywhere
	MaxFinalAimAdjustment=3.2

	MaxAttachRange=1536

	CrossHairTexture=Texture2D'UI_HUD.HUD.UI_HUD_BaseA'
}
