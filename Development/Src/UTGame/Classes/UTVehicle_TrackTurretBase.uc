/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_TrackTurretBase extends UTVehicle
	native(Vehicle)
	abstract;

var AudioComponent TurretMoveStart;
var AudioComponent TurretMoveLoop;
var AudioComponent TurretMoveStop;

/** Is true if this turret is in motion */
var bool bInMotion;

cpptext
{
	virtual void TickSpecial(FLOAT DeltaSeconds);
	virtual void PostNetReceiveBase(AActor* NewBase);
	virtual UBOOL IgnoreBlockingBy(const AActor* Other) const;
}

/**
 * When an icon for this vehicle is needed on the hud, this function is called
 */
simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, LinearColor FinalColor)
{
	local Rotator VehicleRotation;

	if ( HUDMaterialInstance == None )
	{
		HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		HUDMaterialInstance.SetParent(MP.HUDIcons);
	}
	HUDMaterialInstance.SetVectorParameterValue('HUDColor', FinalColor);
	VehicleRotation = (Controller != None) ? Controller.Rotation : Rotation;
	MP.DrawRotatedMaterialTile(Canvas, HUDMaterialInstance, HUDLocation, VehicleRotation.Yaw + 32767, MapSize, MapSize*Canvas.ClipY/Canvas.ClipX,IconXStart, IconYStart, IconXWidth, IconYWidth);
}

/**
 * Notify Kismet that the turret has died
 */
function bool Died(Controller Killer, class<DamageType> DamageType, vector HitLocation)
{
	local UTVehicleFactory_TrackTurretBase PFacWhileDying;

	PFacWhileDying = UTVehicleFactory_TrackTurretBase(ParentFactory); // cache this before super.died sets it to none
	if (Super.Died(Killer, DamageType, HitLocation))
	{
		if (Role == ROLE_Authority && PFacWhileDying != none)
		{

			SetBase( None );
			SetHardAttach(false);
			PFacWhileDying.TriggerEventClass(class'UTSeqEvent_TurretStatusChanged', PFacWhileDying, 1);
			PFacWhileDying.TurretDeathReset();
		}
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Notify Kismet that the drive left
 */
simulated function DriverLeft()
{
	Super.DriverLeft();

	if (Role == ROLE_Authority)
	{
		ParentFactory.TriggerEventClass(class'UTSeqEvent_TurretStatusChanged', ParentFactory, 3);
	}

	// Set the move back to start time
	ResetTime = WorldInfo.TimeSeconds + 30;
	if (ParentFactory != none)
	{
		UTVehicleFactory_TrackTurretBase(ParentFactory).ForceTurretStop();
	}
}

event CheckReset()
{
	local controller C;

	// If we are occupied, don't reset
	if ( Occupied() )
	{
		ResetTime = WorldInfo.TimeSeconds - 1;
		return;
	}

	// If someone is close, don't reset

	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if ( (C.Pawn != None) && WorldInfo.GRI.OnSameTeam(C,self) && (VSize(Location - C.Pawn.Location) < 512) && FastTrace(C.Pawn.Location + C.Pawn.GetCollisionHeight() * vect(0,0,1), Location + GetCollisionHeight() * vect(0,0,1)) )
		{
			ResetTime = WorldInfo.TimeSeconds + 10;
			return;
		}
	}

	UTVehicleFactory_TrackTurretBase(ParentFactory).ResetTurret();
}

function SetTeamNum(byte T)
{
	if (Controller != None && Controller.GetTeamNum() != T)
	{
		DriverLeave(true);
	}

	// restore health and return to initial position when changing hands
	if (T != GetTeamNum())
	{
		Health = default.Health;
		if (UTVehicleFactory_TrackTurretBase(ParentFactory) != None)
		{
			UTVehicleFactory_TrackTurretBase(ParentFactory).ResetTurret();
		}
	}

	Super.SetTeamNum(T);
}

/**
 * Notify Kismet that someone has entered
 */
function bool DriverEnter(Pawn P)
{
	if (Super.DriverEnter(P))
	{
		ParentFactory.TriggerEventClass(class'UTSeqEvent_TurretStatusChanged', ParentFactory, 2);
		return true;
	}
	else
	{
		return false;
	}
}

/**
  * Returns rotation used for determining valid exit positions
  */
function rotator ExitRotation()
{
	return (Controller != None) ? Controller.Rotation : Rotation;
}

defaultproperties
{
	Health=450
	bHardAttach=true
	bDefensive=true
	bEnteringUnlocks=false
	RespawnTime=45.0

	BaseEyeheight=0
	Eyeheight=0
	CameraLag=0.05

	// ~ -85deg.

	ViewPitchMax=15473
	ViewPitchMin=-15473

	LookForwardDist=150
	bCameraNeverHidesVehicle=true
	bIgnoreEncroachers=True
	bSeparateTurretFocus=true

	bCollideWorld=false
	Physics=PHYS_None

	ExplosionDamage=0 // so players getting kicked out due to node control change don't get blown up
}
