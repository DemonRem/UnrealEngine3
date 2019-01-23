/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_RocketLauncher extends  UTLoadedWeapon;

var enum ERocketFireMode
{
	RFM_Spread,
	RFM_Spiral,
	RFM_Grenades,
	RFM_Max
} LoadedFireMode;

/** Class of the rocket to use when seeking */
var class<UTProjectile> SeekingRocketClass;

/** Class of the rocket to use when multiple rockets loaded */
var class<UTProjectile> LoadedRocketClass;

/** Class of the grenade */
var class<UTProjectile> GrenadeClass;

/** How much spread on Grenades */
var int GrenadeSpreadDist;

/** The sound to play when alt-fire mode is changed */
var SoundCue AltFireModeChangeSound;

/** The sound to play when a rocket is loaded */
var SoundCue RocketLoadedSound;
/** sound to play when firing grenades */
var SoundCue GrenadeFireSound;

var ParticleSystemComponent AltMuzzleFlashPSC[2];

/**
 * Turns the MuzzleFlashlight off
 */
simulated event MuzzleFlashTimer()
{
	local int i;

	Super.MuzzleFlashTimer();

	for (i=0;i<2;i++)
	{
		if ( AltMuzzleFlashPSC[i] != none )
		{
			AltMuzzleFlashPSC[i].DeactivateSystem();
		}
	}
}

simulated event StopMuzzleFlash()
{
	local int i;
	Super.StopMuzzleFlash();

	for (i=0;i<2;i++)
	{
		if ( AltMuzzleFlashPSC[i] != none )
		{
			AltMuzzleFlashPSC[i].DeactivateSystem();
		}
	}
}


/**
 * Causes the muzzle flashlight to turn on
 */
simulated event CauseMuzzleFlashLight()
{
	local UTExplosionLight NewMuzzleFlashLight;

	super.CauseMuzzleFlashLight();

	if ( WorldInfo.bDropDetail )
		return;

	if ( LoadedShotCount == 3 )
	{
		NewMuzzleFlashLight = new(Outer) MuzzleFlashLightClass;
		SkeletalMeshComponent(Mesh).AttachComponentToSocket(NewMuzzleFlashLight,MuzzleFlashSocketList[2]);
	}
}

simulated function AttachMuzzleFlash()
{
	local SkeletalMeshComponent SKMesh;
	local int i;

	Super.AttachMuzzleFlash();

	// Attach the Muzzle Flash
	SKMesh = SkeletalMeshComponent(Mesh);
	if (  SKMesh != none )
	{
		if (MuzzleFlashPSCTemplate != none)
		{
			for (i=0;i<2;i++)
			{
				AltMuzzleFlashPSC[i]  = new(Outer) class'UTParticleSystemComponent';
				SKMesh.AttachComponentToSocket(AltMuzzleFlashPSC[i], MuzzleFlashSocketList[i+1]);
				AltMuzzleFlashPSC[i].SetDepthPriorityGroup(SDPG_Foreground);
				AltMuzzleFlashPSC[i].DeactivateSystem();
			}
		}
	}

}
/**
 * Remove/Detach the muzzle flash components
 */
simulated function DetachMuzzleFlash()
{
	local SkeletalMeshComponent SKMesh;
	local int i;

	Super.DetachMuzzleFlash();

	SKMesh = SkeletalMeshComponent(Mesh);
	if (  SKMesh != none )
	{
		for (i=0;i<2;i++)
		{
			if (AltMuzzleFlashPSC[i] != none)
			{
				SKMesh.DetachComponent( AltMuzzleFlashPSC[i] );
			}
		}
	}
}

simulated event CauseMuzzleFlash()
{
	local int i;

	if (CurrentFireMode == 1 && LoadedFireMode == RFM_Grenades)
	{
		return;
	}

	Super.CauseMuzzleFlash();
	if(LoadedShotCount == 1 && CurrentFireMode == 1)
	{
		AltMuzzleFlashPSC[1].ActivateSystem();
	}
	else
	{
		for (i=0;i<LoadedShotCount;i++)
		{
			if (AltMuzzleFlashPSC[i] != none)
			{
				if ( AltMuzzleFlashPSC[i].bWasDeactivated || !AltMuzzleFlashPSC[i].bIsActive)
				{
					AltMuzzleFlashPSC[i].SetTemplate(MuzzleFlashPSCTemplate);
					SetMuzzleFlashParams(AltMuzzleFlashPSC[i]);
					AltMuzzleFlashPSC[i].ActivateSystem();
				}
			}
		}
	}
}

/*********************************************************************************************
 * Hud/Crosshairs
 *********************************************************************************************/

simulated function DrawWeaponCrosshair( Hud HUD )
{
	local UTHUD		H;

	Super.DrawWeaponCrosshair(Hud);

	if (bLockedOnTarget)
	{
		H = UTHUD(HUD);
		if ( H == None )
			return;

		H.Canvas.SetDrawColor(255,128,128);
		H.Canvas.SetPos(H.Canvas.ClipX/2-5,H.Canvas.CLipY/2-5);
		H.Canvas.DrawBox(10,10);
	}
}

simulated function DrawLFMData(HUD Hud)
{
	local UTHUD		H;
	local float midx,midy,xl,yl;
	local string s;

	H = UTHUD(HUD);
	if ( H == None )
		return;

	if (LoadedShotCount > 0)
	{
		if ( LoadedFireMode != RFM_Grenades )
		{

			if (LoadedShotCount < MaxLoadCount)
			{
				H.Canvas.SetDrawColor(255,255,255,100);
			}
			else
			{
				H.Canvas.SetDrawColor(255,64,64,255);
			}
		}
		else
		{
			if (LoadedShotCount < MaxLoadCount)
			{
				H.Canvas.SetDrawColor(64,255,64,100);
			}
			else
			{
				H.Canvas.SetDrawColor(128,255,128,255);
			}
		}

		midx = H.Canvas.ClipX / 2;
		midy = H.Canvas.ClipY / 2;

		H.Canvas.SetPos(MidX-10,MidY-20);
		H.Canvas.DrawTile(H.HudTexture, 21,10,395,210,21,10);


		if (LoadedShotCount>1)
		{
			H.Canvas.SetPos(MidX-20,MidY-5);
			H.Canvas.DrawTile(H.HudTexture, 11,21,415,185,-11,21);
		}

		if (LoadedShotCount>2)
		{
			H.Canvas.SetPos(MidX+9,MidY-5);
			H.Canvas.DrawTile(H.HudTexture, 11,21,404,185,11,21);
		}
		if (LoadedFireMode != RFM_Spread)
		{
			S = (LoadedFireMode == RFM_Spiral) ? "[Spiral]" : "[Grenades]";

			H.Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(0);
			H.Canvas.StrLen(S,xl,yl);
			H.Canvas.SetDrawColor(255,255,255,255);
			H.Canvas.SetPos( midx - xl/2, midy + 20);
			H.Canvas.DrawText(s);
		}
	}

}

/*********************************************************************************************
 * Utility Functions.
 *********************************************************************************************/


/**
 * @Returns 	The amount of spread
 */

simulated function int GetSpreadDist()
{
	if ( LoadedFireMode == RFM_Grenades )
	{
		return GrenadeSpreadDist;
	}
	else
	{
		return SpreadDist;
	}
}


/**
 * Fire off a load of rockets.
 *
 * Network: Server Only
 */
function FireLoad()
{
	local int i,j,k;
	local vector SpreadVector;
	local rotator Aim;
	local float theta;
   	local vector Firelocation, RealStartLoc, X,Y,Z;
	local Projectile	SpawnedProjectile;
	local UTProj_LoadedRocket FiredRockets[4];
	local bool bCurl;
	local byte FlockIndex;
	SetCurrentFireMode(1);
	IncrementFlashCount();

	// this is the location where the projectile is spawned
	RealStartLoc = GetPhysicalFireStartLoc();

	Aim = GetAdjustedAim( RealStartLoc );			// get fire aim direction

	GetViewAxes(X,Y,Z);

	UpdateFiredStats(LoadedShotCount);

	for (i = 0; i < LoadedShotCount; i++)
	{
		if (LoadedFireMode == RFM_Grenades || LoadedFireMode == RFM_Spread)
		{
			// Give them some gradual spread.
			theta = GetSpreadDist() * PI / 32768.0 * (i - float(LoadedShotCount - 1) / 2.0);
			SpreadVector.X = Cos(theta);
			SpreadVector.Y = Sin(theta);
			SpreadVector.Z = 0.0;

			SpawnedProjectile = Spawn(GetProjectileClass(),,, RealStartLoc, Rotator(SpreadVector >> Aim));
			if ( SpawnedProjectile != None )
			{
				if (LoadedFireMode == RFM_Grenades)
				{
					UTProjectile(SpawnedProjectile).TossZ += (frand() * 200 - 100);
				}
				SpawnedProjectile.Init(SpreadVector >> Aim);
				UTProjectile(SpawnedProjectile).InitStats(self);
			}
		}
		else
		{
			Firelocation = RealStartLoc - 2 * ((Sin(i * 2 * PI / MaxLoadCount) * 8 - 7) * Y - (Cos(i * 2 * PI / MaxLoadCount) * 8 - 7) * Z) - X * 8 * FRand();
			SpawnedProjectile = Spawn(GetProjectileClass(),,, FireLocation, Aim);
			if ( SpawnedProjectile != None )
			{
				SpawnedProjectile.Init(vector(Aim));
				UTProjectile(SpawnedProjectile).InitStats(Self);
				FiredRockets[i] = UTProj_LoadedRocket(SpawnedProjectile);
			}
		}

		if (LoadedFireMode != RFM_Grenades && bLockedOnTarget && UTProj_SeekingRocket(SpawnedProjectile) != None)
		{
			UTProj_SeekingRocket(SpawnedProjectile).Seeking = LockedTarget;
		}
	}

	// Initialize the rockets so they flock towards each other
	if ( !bLockedOnTarget && (LoadedFireMode == RFM_Spiral) )
	{
		FlockIndex++;
		bCurl = false;

		// To get crazy flying, we tell each projectile in the flock about the others.
		for ( i = 0; i < LoadedShotCount; i++ )
		{
			if ( FiredRockets[i] != None )
			{
				FiredRockets[i].bCurl = bCurl;
				FiredRockets[i].FlockIndex = FlockIndex;

				j=0;
				for ( k=0; k<LoadedShotCount; k++ )
				{
					if ( (i != k) && (FiredRockets[k] != None) )
					{
						FiredRockets[i].Flock[j] = FiredRockets[k];
						j++;
					}
				}
				bCurl = !bCurl;
				if ( WorldInfo.NetMode != NM_DedicatedServer )
				{
					FiredRockets[i].SetTimer(0.1, true, 'FlockTimer');
				}
			}
		}
	}
}

/**
 * If we are locked on, we need to set the Seeking projectiles LockedTarget.
 */

simulated function Projectile ProjectileFire()
{
	local Projectile SpawnedProjectile;

	SpawnedProjectile = super.ProjectileFire();
	if (bLockedOnTarget && UTProj_SeekingRocket(SpawnedProjectile) != None)
	{
		UTProj_SeekingRocket(SpawnedProjectile).Seeking = LockedTarget;
	}

	return SpawnedProjectile;
}

/**
 * We override GetProjectileClass to swap in a Seeking Rocket if we are locked on.
 */

function class<Projectile> GetProjectileClass()
{
	if (CurrentFireMode == 1 && LoadedFireMode == RFM_Grenades)
	{
		return GrenadeClass;
	}
	else if (bLockedOnTarget)
	{
		return SeekingRocketClass;
	}
	else if ( LoadedShotCount > 1 )
	{
		return LoadedRocketClass;
	}
	else
	{
		return WeaponProjectiles[CurrentFireMode];
	}
}

simulated function PlayFiringSound()
{
	if (CurrentFireMode == 1 && LoadedFireMode == RFM_Grenades)
	{
		MakeNoise(1.0);
		WeaponPlaySound(GrenadeFireSound);
	}
	else
	{
		Super.PlayFiringSound();
	}
}

simulated state WeaponLoadAmmo
{

	simulated function DrawWeaponCrosshair( Hud HUD )
	{
		Global.DrawWeaponCrosshair(Hud);
		DrawLFMData(HUD);
	}

	/**
	 * Initialize the alt fire state
	 */
	simulated function BeginState(Name PreviousStateName)
	{
		LoadedFireMode = RFM_Spread;
		Super.BeginState(PreviousStateName);

	}

	/**
	 * We override BeginFire to detect a normal fire press and switch in to flocked mode
	 */
   	simulated function BeginFire(byte FireModeNum)
	{
		if (FireModeNum == 0)
		{
			LoadedFireMode = ERocketFireMode((int(LoadedFireMode) + 1) % RFM_Max);
			WeaponPlaySound(AltFireModeChangeSound);
		}

		global.BeginFire(FireModeNum);
	}

}

simulated State WeaponWaitingForFire
{
	simulated function DrawWeaponCrosshair( Hud HUD )
	{
		Global.DrawWeaponCrosshair(Hud);
		DrawLFMData(HUD);
	}

	/**
	 * We override BeginFire to detect a normal fire press and switch in to flocked mode
	 */
   	simulated function BeginFire(byte FireModeNum)
	{
		if (FireModeNum == 0)
		{
			LoadedFireMode = ERocketFireMode((int(LoadedFireMode) + 1) % RFM_Max);
			WeaponPlaySound(AltFireModeChangeSound);
		}
		global.BeginFire(FireModeNum);
	}
}

simulated function PlayLoadedSound()
{
	WeaponPlaySound(RocketLoadedSound);
}

simulated State WeaponPlayingFire
{
	simulated function EndState(name NextStateName)
	{
		Super.EndState(NextStateName);

		// Reset the flocked flag
		LoadedFireMode = RFM_Spread;
	}
}

defaultproperties
{
	WeaponColor=(R=255,G=0,B=0,A=255)
	FireInterval(0)=+1.0
	FireInterval(1)=+1.05
	PlayerViewOffset=(X=0.0,Y=0.0,Z=0.0)

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_RocketLauncher.Mesh.SK_WP_RocketLauncher_1P'
		PhysicsAsset=None
		AnimSets(0)=AnimSet'WP_RocketLauncher.Anims.K_WP_RocketLauncher_1P_Base'
		Animations=MeshSequenceA
		Translation=(X=0,Y=0,Z=0)
		Rotation=(Yaw=0)
		scale=1.0
		FOV=60.0
	End Object
	AttachmentClass=class'UTGame.UTAttachment_RocketLauncher'

	// Pickup staticmesh
	Begin Object Name=PickupMesh
		SkeletalMesh=SkeletalMesh'WP_RocketLauncher.Mesh.SK_WP_RocketLauncher_3P'
	End Object

	WeaponLoadSnd=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Load_Cue'
	WeaponLoadedSnd=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Load_Cue'
//	WeaponLoadedSnd=SoundCue'A_Pickups.Ammo.Cue.A_Pickup_Ammo_Sniper_Cue'
	WeaponFireSnd[0]=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Fire_Cue'
	WeaponFireSnd[1]=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Fire_Cue'
	WeaponEquipSnd=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Raise_Cue'

	LockAcquiredSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_SeekLock_Cue'
	LockLostSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_SeekLost_Cue'

	WeaponProjectiles(0)=class'UTProj_Rocket'
	WeaponProjectiles(1)=class'UTProj_Rocket'
	LoadedRocketClass=class'UTProj_LoadedRocket'

	GrenadeClass=class'UTProj_Grenade'

	FireOffset=(X=20,Y=12,Z=-5)

	MaxDesireability=0.78
	AIRating=+0.78
	CurrentRating=+0.78
	BotRefireRate=0.99
	bInstantHit=false
	bSplashJump=true
	bSplashDamage=true
	bRecommendSplashDamage=true
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=1
	InventoryGroup=8
	GroupWeight=0.5

	WeaponIdleAnims(0)=WeaponIdle

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Rocket_Cue'

	AmmoCount=9
	LockerAmmoCount=18
	MaxAmmoCount=30

	SeekingRocketClass=class'UTProj_SeekingRocket'

	FireShake(0)=(OffsetMag=(X=-5.0,Y=-5.00,Z=-5.00),OffsetRate=(X=-1000.0,Y=-1000.0,Z=-1000.0),OffsetTime=2,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=-5.0,Y=-5.00,Z=-5.00),OffsetRate=(X=-1000.0,Y=-1000.0,Z=-1000.0),OffsetTime=2,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)

	AltFireQueueTimes(0)=0.40
	AltFireQueueTimes(1)=0.96
	AltFireQueueTimes(2)=0.96
	AltFireLaunchTimes(0)= 0.51
	AltFireLaunchTimes(1)= 0.51
	AltFireLaunchTimes(2)= 0.51
	AltFireEndTimes(0)=0.44
	AltFireEndTimes(1)=0.44
	AltFireEndTimes(2)=0.44

	GracePeriod=0.96

	MuzzleFlashSocket=MuzzleFlashSocketA
	MuzzleFlashSocketList(0)=MuzzleFlashSocketA
	MuzzleFlashSocketList(1)=MuzzleFlashSocketB
	MuzzleFlashSocketList(2)=MuzzleFlashSocketC

	MuzzleFlashPSCTemplate=WP_RocketLauncher.Effects.P_WP_RockerLauncher_Muzzle_Flash
	MuzzleFlashDuration=0.33
	MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight'

	WeaponLockType=WEAPLOCK_Default

	LockChecktime=0.15
	LockAcquireTime=1.1
	LockTolerance=0.25
	LockablePawnClass=class'Engine.Pawn'

	IconX=460
	IconY=34
	IconWidth=51
	IconHeight=38

	AltFireSndQue(0)=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_AltFireQueue1_Cue'
	AltFireSndQue(1)=SoundCue'A_Weapon.RocketLauncher.Cue.A_Weapon_RL_AltFireQueue2_Cue'
	AltFireSndQue(2)=SoundCue'A_Weapon.RocketLauncher.Cue.A_Weapon_RL_AltFireQueue3_Cue'

	EquipTime=+0.6

	GrenadeSpreadDist=300

	AltFireModeChangeSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_AltModeChange_Cue'
	RocketLoadedSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_RocketLoaded_Cue'
	GrenadeFireSound=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_GrenadeFire_Cue'

	JumpDamping=0.75

	LockerRotation=(pitch=0,yaw=0,roll=-16384)
	IconCoordinates=(U=273,V=210,UL=84,VL=50)
	CrossHairCoordinates=(U=128,V=64,UL=64,VL=64)
	WeaponPutDownSnd=SoundCue'A_Weapon_RocketLauncher.Cue.A_Weapon_RL_Lower_Cue'

	QuickPickGroup=0
	QuickPickWeight=0.9

}
