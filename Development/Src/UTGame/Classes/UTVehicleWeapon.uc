/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleWeapon extends UTWeapon
	abstract
	native(Vehicle)
	nativereplication
	dependson(UTPhysicalMaterialProperty);

/** Holds a link in to the Seats array in MyVehicle that represents this weapon */
var int SeatIndex;

/** Holds a link to the parent vehicle */
var RepNotify UTVehicle	MyVehicle;

/** Triggers that should be activated when a weapon fires */
var array<name>	FireTriggerTags, AltFireTriggerTags;

/** impact effects by material type */
var array<MaterialImpactEffect> ImpactEffects, AltImpactEffects;

/** default impact effect to use if a material specific one isn't found */
var MaterialImpactEffect DefaultImpactEffect, DefaultAltImpactEffect;

/** sound that is played when the bullets go whizzing past your head */
var SoundCue BulletWhip;

struct native ColorOverTime
{
	/** the color to lerp to */
	var color TargetColor;
	/** time at which the color is exactly TargetColor */
	var float Time;
};
/** color flash interpolation when aim becomes correct - assumed to be in order of increasing time */
var array<ColorOverTime> GoodAimColors;
/** last time aim was incorrect, used for looking up GoodAimColor */
var float LastIncorrectAimTime;

var color BadAimColor;

/** cached max range of the weapon used for aiming traces */
var float AimTraceRange;

/** actors that the aiming trace should ignore */
var array<Actor> AimingTraceIgnoredActors;

/** This value is used to cap the maximum amount of "automatic" adjustment that will be made to a shot
    so that it will travel at the crosshair.  If the angle between the barrel aim and the player aim is
    less than this angle, it will be adjusted to fire at the crosshair.  The value is in radians */
var float MaxFinalAimAdjustment;

var bool bPlaySoundFromSocket;

/** used for client to tell server when zooming, as that is clientside
 * but on console it affects the controls so the server needs to know
 */
var bool bCurrentlyZoomed;

/**
 * If the weapon is attached to a socket that doesn't pitch with
 * player view, and should fire at the aimed pitch, then this should be enabled.
 */
var bool bIgnoreSocketPitchRotation;
/**
 * Same as above, but only allows for downward direction, for vehicles with 'bomber' like behavior.
 */
var bool bIgnoreDownwardPitch;
/** Vehicle class used for drawing kill icons */
var class<UTVehicle> VehicleClass;


cpptext
{
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
}

replication
{
	if (Role == ROLE_Authority && bNetInitial && bNetOwner)
		SeatIndex, MyVehicle;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	AimTraceRange = MaxRange();
}

simulated event ReplicatedEvent(name VarName)
{
	local UTVehicle_Deployable DeployableVehicle;

	if (VarName == 'MyVehicle')
	{
		DeployableVehicle = UTVehicle_Deployable(MyVehicle);
		if (DeployableVehicle != None && DeployableVehicle.IsDeployed())
		{
			NotifyVehicleDeployed();
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/** checks if the weapon is actually capable of hitting the desired target, including trace test (used by crosshair)
 * if false because trace failed, RealAimPoint is set to what the trace hit
 */
simulated function bool CanHitDesiredTarget(vector SocketLocation, rotator SocketRotation, vector DesiredAimPoint, Actor TargetActor, out vector RealAimPoint)
{
	local int i;
	local array<Actor> IgnoredActors;
	local Actor HitActor;
	local vector HitLocation, HitNormal;
	local bool bResult;

	if ((Normal(DesiredAimPoint - SocketLocation) dot Normal(RealAimPoint - SocketLocation)) >= GetMaxFinalAimAdjustment())
	{
		// turn off bProjTarget on Actors we should ignore for the aiming trace
		for (i = 0; i < AimingTraceIgnoredActors.length; i++)
		{
			if (AimingTraceIgnoredActors[i] != None && AimingTraceIgnoredActors[i].bProjTarget)
			{
				AimingTraceIgnoredActors[i].bProjTarget = false;
				IgnoredActors[IgnoredActors.length] = AimingTraceIgnoredActors[i];
			}
		}
		// perform the trace
		HitActor = GetTraceOwner().Trace(HitLocation, HitNormal, DesiredAimPoint, SocketLocation, true,,, TRACEFLAG_Bullet);
		if (HitActor == None || HitActor == TargetActor)
		{
			bResult = true;
		}
		else
		{
			RealAimPoint = HitLocation;
		}
		// restore bProjTarget on Actors we turned it off for
		for (i = 0; i < IgnoredActors.length; i++)
		{
			IgnoredActors[i].bProjTarget = true;
		}
	}

	return bResult;
}


simulated static function DrawKillIcon(Canvas Canvas, float ScreenX, float ScreenY, float HUDScaleX, float HUDScaleY)
{
	if ( default.VehicleClass != None )
	{
		default.VehicleClass.static.DrawKillIcon(Canvas, ScreenX, ScreenY, HUDScaleX, HUDScaleY);
	}
}

simulated function DrawWeaponCrosshair( Hud HUD )
{
	local vector SocketLocation, DesiredAimPoint, RealAimPoint;
	local rotator SocketRotation;
	local Actor TargetActor;
	local bool bAimIsCorrect;
	local int i;
	local float TimeSinceIncorrectAim, Pct;
	local color LastColor;

	DesiredAimPoint = GetDesiredAimPoint(TargetActor);
	GetFireStartLocationAndRotation(SocketLocation, SocketRotation);
	RealAimPoint = SocketLocation + Vector(SocketRotation) * GetTraceRange();

	bAimIsCorrect = CanHitDesiredTarget(SocketLocation, SocketRotation, DesiredAimPoint, TargetActor, RealAimPoint);

	// draw the center crosshair, then figure out where to draw the crosshair that shows the vehicle's actual aim
	if (bAimIsCorrect)
	{
		// if recently aim became correct, play color flash
		TimeSinceIncorrectAim = WorldInfo.TimeSeconds - LastIncorrectAimTime;
		if (TimeSinceIncorrectAim < GoodAimColors[GoodAimColors.length - 1].Time)
		{
			for (i = 0; i < GoodAimColors.length; i++)
			{
				if (TimeSinceIncorrectAim <= GoodAimColors[i].Time)
				{
					if (i == 0)
					{
						Pct = TimeSinceIncorrectAim / GoodAimColors[i].Time;
						LastColor = BadAimColor;
					}
					else
					{
						Pct = (TimeSinceIncorrectAim - GoodAimColors[i - 1].Time) / (GoodAimColors[i].Time - GoodAimColors[i - 1].Time);
						LastColor = GoodAimColors[i - 1].TargetColor;
					}
					CrosshairColor = (LastColor * (1.0 - Pct)) + (GoodAimColors[i].TargetColor * Pct);
					break;
				}
			}
		}
		else
		{
			CrosshairColor = GoodAimColors[GoodAimColors.length - 1].TargetColor;
		}
		CrosshairColor.A = 255;
		Super.DrawWeaponCrosshair(HUD);

		Hud.Canvas.SetPos(HUD.Canvas.ClipX * 0.5 - 10.0, HUD.Canvas.ClipY * 0.5 - 10.0);
	}
	else
	{
		LastIncorrectAimTime = WorldInfo.TimeSeconds;
		CrosshairColor = BadAimColor;
		Super.DrawWeaponCrosshair(HUD);

		RealAimPoint = Hud.Canvas.Project(RealAimPoint);
		if (RealAimPoint.X < 12 || RealAimPoint.X > Hud.Canvas.ClipX-12)
		{
			RealAimPoint.X = Clamp(RealAimPoint.X,12,Hud.Canvas.ClipX-12);
		}
		if (RealAimPoint.Y < 12 || RealAimPoint.Y > Hud.Canvas.ClipY-12)
		{
			RealAimPoint.Y = Clamp(RealAimPoint.Y,12,Hud.Canvas.ClipY-12);
		}
		Hud.Canvas.SetPos(RealAimPoint.X - 10.0, RealAimPoint.Y - 10.0);
	}
	Hud.Canvas.DrawTile(Texture2D'UI_HUD.HUD.UTCrossHairs', 21, 21, 63, 257, 20, 20);
}

/** GetDesiredAimPoint - Returns the desired aim given the current controller
 * @param TargetActor (out) - if specified, set to the actor being aimed at
 * @return The location the controller is aiming at
 */
simulated event vector GetDesiredAimPoint(optional out Actor TargetActor)
{
	local vector CameraLocation, HitLocation, HitNormal, DesiredAimPoint;
	local rotator CameraRotation;
	local Controller C;
	local PlayerController PC;

	C = MyVehicle.Seats[SeatIndex].SeatPawn.Controller;

	PC = PlayerController(C);
	if (PC != None)
	{
		PC.GetPlayerViewPoint(CameraLocation, CameraRotation);
		DesiredAimPoint = CameraLocation + Vector(CameraRotation) * GetTraceRange();
		TargetActor = GetTraceOwner().Trace(HitLocation, HitNormal, DesiredAimPoint, CameraLocation);
		if (TargetActor != None)
		{
			DesiredAimPoint = HitLocation;
		}
	}
	else if ( C != None )
	{
		DesiredAimPoint = C.FocalPoint;
		TargetActor = C.Focus;
	}
	return DesiredAimPoint;
}

/** returns the location and rotation that the weapon's fire starts at */
simulated function GetFireStartLocationAndRotation(out vector StartLocation, out rotator StartRotation)
{
	if ( MyVehicle.Seats[SeatIndex].GunSocket.Length>0 )
	{
		MyVehicle.GetBarrelLocationAndRotation(SeatIndex, StartLocation, StartRotation);
	}
	else
	{
		StartLocation = MyVehicle.Location;
		StartRotation = MyVehicle.Rotation;
	}
}

/**
 * IsAimCorrect - Returns true if the turret associated with a given seat is aiming correctly
 *
 * @return TRUE if we can hit where the controller is aiming
 */
simulated function bool IsAimCorrect()
{
	local vector SocketLocation, DesiredAimPoint, RealAimPoint;
	local rotator SocketRotation;

	DesiredAimPoint = GetDesiredAimPoint();

	GetFireStartLocationAndRotation(SocketLocation, SocketRotation);

	RealAimPoint = SocketLocation + Vector(SocketRotation) * GetTraceRange();
	return ((Normal(DesiredAimPoint - SocketLocation) dot Normal(RealAimPoint - SocketLocation)) >= GetMaxFinalAimAdjustment());
}


simulated static function Name GetFireTriggerTag(int BarrelIndex, int FireMode)
{
	if (FireMode==0)
	{
		if (default.FireTriggerTags.Length > BarrelIndex)
		{
			return default.FireTriggerTags[BarrelIndex];
		}
	}
	else
	{
		if (default.AltFireTriggerTags.Length > BarrelIndex)
		{
			return default.AltFireTriggerTags[BarrelIndex];
		}
	}
	return '';
}

/**
 * Returns interval in seconds between each shot, for the firing state of FireModeNum firing mode.
 *
 * @param	FireModeNum	fire mode
 * @return	Period in seconds of firing mode
 */
simulated function float GetFireInterval( byte FireModeNum )
{
	local Vehicle V;
	local UTPawn UTP;

	V = Vehicle(Instigator);
	if (V != None)
	{
		UTP = UTPawn(V.Driver);
		if (UTP != None)
		{
			return (FireInterval[FireModeNum] * UTP.FireRateMultiplier);
		}
	}

	return Super.GetFireInterval(FireModeNum);
}

/** returns the impact effect that should be used for hits on the given actor and physical material */
simulated static function MaterialImpactEffect GetImpactEffect(Actor HitActor, PhysicalMaterial HitMaterial, byte FireModeNum)
{
	local int i;
	local UTPhysicalMaterialProperty PhysicalProperty;

	if (HitMaterial != None)
	{
		PhysicalProperty = UTPhysicalMaterialProperty(HitMaterial.GetPhysicalMaterialProperty(class'UTPhysicalMaterialProperty'));
	}
	if (FireModeNum > 0)
	{
		if (PhysicalProperty != None && PhysicalProperty.MaterialType != 'None')
		{
			i = default.AltImpactEffects.Find('MaterialType', PhysicalProperty.MaterialType);
			if (i != -1)
			{
				return default.AltImpactEffects[i];
			}

		}
		return default.DefaultAltImpactEffect;
	}
	else
	{
		if (PhysicalProperty != None && PhysicalProperty.MaterialType != 'None')
		{
			i = default.ImpactEffects.Find('MaterialType', PhysicalProperty.MaterialType);
			if (i != -1)
			{
				return default.ImpactEffects[i];
			}
		}
		return default.DefaultImpactEffect;
	}
}

simulated function SetHand(UTPawn.EWeaponHand NewWeaponHand);
simulated function UTPawn.EWeaponHand GetHand();
simulated function AttachWeaponTo( SkeletalMeshComponent MeshCpnt, optional Name SocketName );
simulated function DetachWeapon();

simulated function Activate()
{
	// don't reactivate if already firing
	if (!IsFiring())
	{
		GotoState('Active');
	}
}

simulated function PutDownWeapon()
{
	GotoState('Inactive');
}

simulated function Vector GetPhysicalFireStartLoc(optional vector AimDir)
{
	if ( MyVehicle != none )
		return MyVehicle.GetPhysicalFireStartLoc(self);
	else
		return Location;
}

simulated function BeginFire(byte FireModeNum)
{
	local UTVehicle V;

	// allow the vehicle to override the call
	V = UTVehicle(Instigator);
	if (V == None || (!V.bIsDisabled && !V.OverrideBeginFire(FireModeNum)))
	{
		Super.BeginFire(FireModeNum);
	}
}

simulated function EndFire(byte FireModeNum)
{
	local UTVehicle V;

	// allow the vehicle to override the call
	V = UTVehicle(Instigator);
	if (V == None || !V.OverrideEndFire(FireModeNum))
	{
		Super.EndFire(FireModeNum);
	}
}

simulated function Rotator GetAdjustedAim( vector StartFireLoc )
{
	// Start the chain, see Pawn.GetAdjustedAimFor()
	// @note we don't add in spread here because UTVehicle::GetWeaponAim() assumes
	// that a return value of Instigator.Rotation or Instigator.Controller.Rotation means 'no adjustment', which spread interferes with
	return Instigator.GetAdjustedAimFor( Self, StartFireLoc );
}

/**
 * Create the projectile, but also increment the flash count for remote client effects.
 */
simulated function Projectile ProjectileFire()
{
	local Projectile SpawnedProjectile;

	IncrementFlashCount();

	if (Role==ROLE_Authority)
	{
		SpawnedProjectile = Spawn(GetProjectileClass(),,, MyVehicle.GetPhysicalFireStartLoc(self));

		if ( SpawnedProjectile != None )
		{
			SpawnedProjectile.Init( vector(AddSpread(MyVehicle.GetWeaponAim(self))) );

			// return it up the line
			UTProjectile(SpawnedProjectile).InitStats(self);
		}
	}
	return SpawnedProjectile;
}

/**
* Overriden to use vehicle starttrace/endtrace locations
* @returns position of trace start for instantfire()
*/
simulated function vector InstantFireStartTrace()
{
	return MyVehicle.GetPhysicalFireStartLoc(self);
}

/**
* @returns end trace position for instantfire()
*/
simulated function vector InstantFireEndTrace(vector StartTrace)
{
	return StartTrace + vector(AddSpread(MyVehicle.GetWeaponAim(self))) * GetTraceRange();;
}

simulated function Actor GetTraceOwner()
{
	return (MyVehicle != None) ? MyVehicle : Super.GetTraceOwner();
}

simulated function float GetMaxFinalAimAdjustment()
{
	return MaxFinalAimAdjustment;
}

/** notification that MyVehicle has been deployed/undeployed, since that often changes how its weapon works */
simulated function NotifyVehicleDeployed();
simulated function NotifyVehicleUndeployed();

simulated function WeaponPlaySound( SoundCue Sound, optional float NoiseLoudness )
{
	local int Barrel;
	local name Pivot;
	local vector Loc;
	local rotator Rot;
	if(bPlaySoundFromSocket && MyVehicle != none && MyVehicle.Mesh != none)
	{
		if( Sound == None || Instigator == None )
		{
			return;
		}
		Barrel = MyVehicle.GetBarrelIndex(SeatIndex);
		Pivot = MyVehicle.Seats[SeatIndex].GunSocket[Barrel];
		MyVehicle.Mesh.GetSocketWorldLocationAndRotation(Pivot, Loc, Rot);
		Instigator.PlaySound(Sound, false, true,false,Loc);
	}
	else
		super.WeaponPlaySound(Sound,NoiseLoudness);
}

/**
 * If you want to be able to hide a crosshair for this weapon, override this function and return false;
 */
simulated function bool ShowCrosshair()
{
	return true;
}

simulated function EZoomState GetZoomedState()
{
	// override on server to use what the client told us
	if (Role == ROLE_Authority && Instigator != None && !Instigator.IsLocallyControlled())
	{
		return bCurrentlyZoomed ? ZST_Zoomed : ZST_NotZoomed;
	}
	else
	{
		return Super.GetZoomedState();
	}
}

reliable server function ServerSetZoom(bool bNowZoomed)
{
	bCurrentlyZoomed = bNowZoomed;
}

simulated function StartZoom(UTPlayerController PC)
{
	Super.StartZoom(PC);

	ServerSetZoom(true);
}

simulated function EndZoom(UTPlayerController PC)
{
	Super.EndZoom(PC);

	ServerSetZoom(false);
}

defaultproperties
{
	Components.Remove(FirstPersonMesh)

	TickGroup=TG_PostAsyncWork
	InventoryGroup=100
	GroupWeight=0.5
	GoodAimColors[0]=(Time=0.25,TargetColor=(R=255,G=255,B=64,A=255))
	GoodAimColors[1]=(Time=0.70,TargetColor=(R=0,G=255,B=64,A=255))
	BadAimColor=(R=255,G=64,B=0,A=255)
	bExportMenuData=false

	ShotCost[0]=0
	ShotCost[1]=0

	// ~ 5 Degrees
	MaxFinalAimAdjustment=0.995;
}
