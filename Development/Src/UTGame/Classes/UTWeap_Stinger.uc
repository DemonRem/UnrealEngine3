/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_Stinger extends UTWeapon;

var(Weapon) float WindUpTime[2];
var(Weapon) float WindDownTime[2];

var SoundCue WeaponLoadSnd;
var SoundCue WeaponSpinUpSnd[2];
var SoundCue WeaponSpinDownSnd[2];
var SoundCue WeaponWarmUpShotSnd;
var SoundCue WeaponFireStart;

var AudioComponent SpinningBarrelSound;
var SoundCue SpinningBarrelCue;

var name WeaponSpinUpAnims[2];
var name WeaponSpinDownAnims[2];
var name WeaponFireAnims[2];

/** spin up, down, and fire animations to match above for the arm mesh */
var name ArmSpinUpAnims[2];
var name ArmSpinDownAnims[2];
var name ArmFireAnims[2];

var float WindUpShotTimer;

var ParticleSystemComponent PrimaryMuzzleFlashPSC;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	// Atttach the muzzle flash
	SkeletalMeshComponent(Mesh).AttachComponentToSocket(PrimaryMuzzleFLashPSC, MuzzleFlashSocket);
}


/** Needed so when the function is called on this object it will be found and then because we are in a state that version will be used **/
simulated function FireWarmupShot();

/** Needed so when the function is called on this object it will be found and then because we are in a state that version will be used **/
simulated function WeaponWarmedUp();

/*********************************************************************************************
 * state WindUp
 * The Minigun will first enter this state during the firing sequence.  It winds up the barrel
 *
 * Because we don't have animations or sounds, it just delays a bit then fires.
 *********************************************************************************************/

simulated state WeaponWindUp
{
	simulated function bool RefireCheck()
	{
		// if switching to another weapon, abort firing and put down right away
		if( bWeaponPutDown )
		{
			GotoState('WeaponWindDown');
			return false;
		}

		// If weapon should keep on firing, then do not leave state and fire again.
		if( ShouldRefire() )
		{
			return true;
		}

		// if out of ammo, then call weapon empty notification
		if( !HasAnyAmmo() )
		{
			GotoState('Active');
			WeaponEmpty();
			return false;
		}
		else
		{
			GotoState('WeaponWindDown');
			return false;
		}
	}

	simulated function WeaponWarmedUp()
	{
		if ( RefireCheck() )
			GotoState('WeaponFiring');
	}

	simulated function bool IsFiring()
	{
		return true;
	}

	simulated function FireWarmupShot()
	{
		local float time;

		if ( RefireCheck() )
   		{
	   		FireAmmunition();
			WeaponPlaySound(WeaponWarmUpShotSnd);
   			Time = WindUpShotTimer * 0.5;
   			WindUpShotTimer -= Time;
   			SetTimer(Time,false,'FireWarmupShot');
	   	}
	}

	simulated function BeginState(name PreviousStateName)
	{
		local float Time;

		MuzzleFlashPSCTemplate = default.MuzzleFlashPSCTemplate;

   		Super.BeginState(PreviousStateName);

		PlayWeaponAnimation( WeaponSpinUpAnims[CurrentFireMode], 1.0, false );
		PlayArmAnimation( ArmSpinUpAnims[CurrentFireMode], 1.0);
   		WeaponPlaySound(WeaponSpinUpSnd[CurrentFireMode]);

	   	FireWarmupShot();
		WindUpShotTimer = WindUpTime[CurrentFireMode];
		SetTimer(WindUpShotTimer,False,'WeaponWarmedUp');

		Time = WindUpShotTimer * 0.25;
		WindUpShotTimer -= Time;
		SetTimer(Time,false,'FireWarmupShot');
	}

	simulated function EndState(name NextStateName)
	{
		Super.EndState(NextStateName);
		ClearTimer('FireWarmupShot');	// Catchall just in case.
		ClearTimer('WeaponWarmedUp');
	}
}

/*********************************************************************************************
 * State WindDown
 * The Minigun enteres this state when it stops firing.  It slows the barrels down and when
 * done, goes active/down/etc.
 *
 * Because we don't have animations or sounds, it just delays a bit then exits
 *********************************************************************************************/

simulated state WeaponWindDown
{
	simulated function bool IsFiring()
	{
		return true;
	}

	simulated function Timer()
	{
		if ( bWeaponPutDown )
		{
			// if switched to another weapon, put down right away
			PutDownWeapon();
		}
		else
		{
			// Return to the active state
			GotoState('Active');
		}
	}
Begin:
	PlayWeaponAnimation( WeaponSpinDownAnims[CurrentFireMode], 1.5, false );
	PlayArmAnimation(ArmSpinDownAnims[CurrentFireMode], 1.5);
	WeaponPlaySound(WeaponSpinDownSnd[CurrentFireMode]);
	SetTimer(WindDownTime[CurrentFireMode],false);
}

simulated function PlayFiringSound()
{
	if(CurrentFireMode == 1)
	{
		WeaponPlaySound(WeaponFireSnd[CurrentFireMode]);
	}
	MakeNoise(1.0);
}

/*********************************************************************************************
 * State WeaponFiring
 * See UTWeapon.WeaponFiring
 *********************************************************************************************/

simulated state WeaponFiring
{
	/**
	 * Called when the weapon is done firing, handles what to do next.
	 * We override the default here so as to go to the WindDown state.
	 */

	simulated function RefireCheckTimer()
	{
		// if switching to another weapon, abort firing and put down right away
		if( bWeaponPutDown )
		{
			GotoState('WeaponWindDown');
			return;
		}

		// If weapon should keep on firing, then do not leave state and fire again.
		if( ShouldRefire() )
		{
			FireAmmunition();
			return;
		}

		// if out of ammo, then call weapon empty notification
		if( !HasAnyAmmo() )
		{
			GotoState('Active');
			WeaponEmpty();
		}
		else
		{
			GotoState('WeaponWindDown');
		}
	}

	simulated function BeginState(Name PreviousStateName)
	{
		local UTPawn POwner;

		MuzzleFlashPSCTemplate = none;

		super.BeginState(PreviousStateName);

		POwner = UTPawn(Instigator);
		if (POwner != None && CurrentFireMode == 0)
		{
			POwner.SetWeaponAmbientSound(WeaponFireSnd[CurrentFireMode]);
			WeaponPlaySound(WeaponFireStart);
		}

		if (CurrentFireMode == 0)
		{
			PrimaryMuzzleFlashPSC.ActivateSystem();
			PlayWeaponAnimation(WeaponFireAnims[0],1.0, true); //GetFireInterval(0),true);
			PlayArmAnimation(ArmFireAnims[0],1.0,false,true);
		}
		else
		{
			PlayWeaponAnimation(WeaponFireAnims[1],1.0, true); //,GetFireInterval(1),true);
			PlayArmAnimation(ArmFireAnims[1],1.0,false,true);
		}

		if(SpinningBarrelSound == none)
		{
			SpinningBarrelsound = CreateAudioComponent(SpinningBarrelCue, false, true);
		}
		if(SpinningBarrelSound != none)
			SpinningBarrelSound.FadeIn(0.2f,1.0f);

	}

	simulated function EndState(Name NextStateName)
	{
		local UTPawn POwner;

		super.EndState(NextStateName);

		POwner = UTPawn(Instigator);
		if (POwner != None)
		{
			POwner.SetWeaponAmbientSound(None);
		}
		if(SpinningBarrelSound != none)
		{
				SpinningBarrelSound.FadeOut(0.2f,0.0f);
				SpinningBarrelSound=none;
		}

		PrimaryMuzzleFlashPSC.DeActivateSystem();
	}

	simulated function bool IsFiring()
	{
		return true;
	}



Begin:
	TimeWeaponFiring(CurrentFireMode);
}



//-----------------------------------------------------------------
// AI Interface

function float GetAIRating()
{
	local UTBot B;

	B = UTBot(Instigator.Controller);
	if ( (B== None) || (B.Enemy == None) )
		return AIRating;

	if ( !B.LineOfSightTo(B.Enemy) )
		return AIRating - 0.15;

	return AIRating * FMin(Pawn(Owner).GetDamageScaling(), 1.5);
}

/* BestMode()
choose between regular or alt-fire
*/
function byte BestMode()
{
	local float EnemyDist;
	local UTBot B;

	if ( IsFiring() )
		return CurrentFireMode;

	B = UTBot(Instigator.Controller);
	if ( (B == None) || (B.Enemy == None) )
		return 0;

	EnemyDist = VSize(B.Enemy.Location - Instigator.Location);
	if ( EnemyDist < 2000 )
		return 0;
	return 1;
}

defaultproperties
{
	WeaponColor=(R=255,G=255,B=0,A=255)
	PlayerViewOffset=(X=0,Y=2.0,Z=0.0)

	// Muzzle Flashes

	Begin Object Class=ParticleSystemComponent Name=MuzzleFlashComponent
		bAutoActivate=FALSE
		Template=particleSystem'WP_Stinger.Particles.P_Stinger_MF_Primary'
		DepthPriorityGroup=SDPG_Foreground
		SecondsBeforeInactive=1.0f
	End Object
	PrimaryMuzzleFlashPSC=MuzzleFlashComponent

	// Weapon SkeletalMesh / Anims

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_Stinger.Mesh.SK_WP_Stinger_1P'
		AnimSets(0)=AnimSet'WP_Stinger.Anims.K_WP_Stinger_1P_Base'
		Animations=MeshSequenceA
		Translation=(X=0.0,Y=0.0,Z=0.0)
		Scale=1.0
		FOV=65
	End Object
	AttachmentClass=class'UTAttachment_Stinger'

	// Pickup staticmesh
	Begin Object Name=PickupMesh
		SkeletalMesh=SkeletalMesh'WP_Stinger.Mesh.SK_WP_Stinger_3P'
	End Object

	EffectSockets=(MuzzleFlashSocket,MuzzleFlashSocket)

	FiringStatesArray(0)=WeaponWindUp
	WeaponFireTypes(0)=EWFT_InstantHit
	FireInterval(0)=+0.1
	WindUpTime(0)=1.0
	WindDownTime(0)=0.27
	Spread(0)=0.0675
	InstantHitDamage(0)=14
	InstantHitDamageTypes(0)=class'UTDmgType_StingerBullet'

	FiringStatesArray(1)=WeaponFiring
	WeaponFireTypes(1)=EWFT_Projectile
	WeaponProjectiles(1)=class'UTProj_StingerShard'
	FireInterval(1)=+0.28
	WindUpTime(1)=0.33
	WindDownTime(1)=0.33

	FireOffset=(X=19,Y=10,Z=-10)

	WeaponSpinUpSnd[0]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindStartCue'
	WeaponSpinUpSnd[1]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindStartCue'
	WeaponSpinDownSnd[0]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindStopCue'
	WeaponSpinDownSnd[1]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindStopCue'
	WeaponWarmUpShotSnd=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireCue'
	WeaponFireSnd[0]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireLoopCue'
	WeaponFireSnd[1]=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_FireAltCue'
	WeaponPutDownSnd=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_LowerCue'
	WeaponEquipSnd=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_RaiseCue'
	
	SpinningBarrelCue=SoundCue'A_Weapon_Stinger.Weapons.A_Weapon_Stinger_BarrelWindLoopCue'

	MaxDesireability=0.73
	AIRating=+0.71
	CurrentRating=+0.71
	BotRefireRate=0.99
	bInstantHit=true
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0
	InventoryGroup=6
	GroupWeight=0.52
	AimError=850

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Stinger_Cue'

	AmmoCount=100
	LockerAmmoCount=150
	MaxAmmoCount=300

	FireShake(0)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=-4.0,Y=0.0,Z=-4.0),OffsetRate=(X=1000.0,Y=0.0,Z=1000.0),OffsetTime=2,RotMag=(X=100.0,Y=0.0,Z=0.0),RotRate=(X=1000.0,Y=0.0,Z=0.0),RotTime=2)

	IconX=273
	IconY=413
	IconWidth=84
	IconHeight=50

	EquipTime=+0.6
	AimingHelpRadius[0]=12.0
	AimingHelpRadius[1]=12.0

	MuzzleFlashLightClass=class'UTStingerMuzzleFlashLight'
	CrossHairCoordinates=(U=448,V=0,UL=64,VL=64)
	LockerRotation=(Pitch=0,Roll=-16384)

	IconCoordinates=(U=273,V=413,UL=84,VL=50)

	ArmsAnimSet=AnimSet'WP_Stinger.Anims.K_WP_Stinger_1P_Arms'

	WeaponFireAnim(0)=none
	WeaponFireAnim(1)=none
	ArmFireAnim(0)=none
	ArmFireAnim(1)=none

	WeaponSpinUpAnims(0)=WeaponRampUp
	WeaponSpinUpAnims(1)=WeaponRampUp
	WeaponSpinDownAnims(0)=WeaponRampDown
	WeaponSpinDownAnims(1)=WeaponRampDown
	WeaponFireAnims(0)=WeaponFire
	WeaponFireAnims(1)=WeaponFire-Secondary

	ArmSpinUpAnims(0)=WeaponRampUp
	ArmSpinUpAnims(1)=WeaponRampUp
	ArmSpinDownAnims(0)=WeaponRampDown
	ArmSpinDownAnims(1)=WeaponRampDown
	ArmFireAnims(0)=WeaponFire
	ArmFireAnims(1)=WeaponFire-Secondary

	MuzzleFlashPSCTemplate=WP_Stinger.Particles.P_Stinger_MF_Primary_WarmUP
	MuzzleFlashAltPSCTemplate=WP_Stinger.Particles.P_Stinger_MF_Alt_Fire

	QuickPickGroup=4
	QuickPickWeight=0.9
}
