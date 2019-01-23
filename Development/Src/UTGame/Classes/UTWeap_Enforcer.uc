/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_Enforcer extends UTWeapon
	HideDropDown;

enum EDualMode
{
	EDM_SingleWeapon,
	EDM_DualEquipping,
	EDM_Dual,
	EDM_Max
};

/** if true, we are a dual enforcer */
var repnotify EDualMode	DualMode;

/** The Left Handed Mesh */
var() editinline UTSkeletalMeshComponent LeftMesh;

var protected UTSkeletalMeshComponent OverlayLeftMesh;
/** timeseconds when last fired */
var float LastFireTime;

/** if the last shot was fired from the left enforcer*/
var bool bLastFiredLeft;

/** How many shots have been fired in burst */
var int BurstCnt;

/** How many shots max in burst before going to cool down */
var int BurstMax;

/** How long to cool down */
var float BurstCoolDownTime;

/** Holds the name of the last firing state.  Used by reload to know where to go */
var name LastFiringState;

/** If true this weapon won't play the reload anim */
var bool bLoaded;

/** Name of the animation to play when reloading */
var name WeaponReloadAnim;
var name ArmReloadAnim;
var name WeaponDualEqipAnim;
var name ArmDualEquipAnim;
/** sound played when reloading */
var SoundCue WeaponReloadSnd;
/** Amount of time for the reload anim */
var float ReloadTime;

/** Amount of time it takes to equip the left gun */
var float DualEquiptime;

/** Used internally to track which gun to fire */
var int ShotCount;

/** If we become a dual while firing, force return to the active state in order to properly re-initialize */
var bool bForceReturnToActive;

var bool bFullDualCoolDown;

var MaterialInstanceConstant WeaponMaterialInstance;
var MaterialInstanceConstant LeftWeaponMaterialInstance;
/** Left versions of everything */

/** Muzzle flash PSC and Templates*/
var ParticleSystemComponent	EnforcerMuzzleFlashPSC[2];

/** dynamic light */
var	UTExplosionLight		EnforcerMuzzleFlashLight[2];

/** Name of the animation when bursting */
var name BurstFireAnimName;
var name ArmBurstFireAnimName;

var AnimSet LeftArmAnimSet;

/** Damage type used with dual enforcers (for unique kill message) */
var class<UTDamageType> DualEnforcerDamageType;

replication
{
	// Server->Client
	if( Role==ROLE_Authority )
		DualMode;
}

/*********************************************************************************************
 * Everything else
 *********************************************************************************************/

/**
 * Set the Overlay Meshes
 */

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer && LeftMesh != None)
	{
		OverlayLeftMesh = new(outer) LeftMesh.Class;
		if (OverlayLeftMesh != None)
		{
			OverlayLeftMesh.SetScale(1.00);
			OverlayLeftMesh.SetOwnerNoSee(LeftMesh.bOwnerNoSee);
			OverlayLeftMesh.SetDepthPriorityGroup(SDPG_Foreground);
			OverlayLeftMesh.SetSkeletalMesh(LeftMesh.SkeletalMesh);
			OverlayLeftMesh.AnimSets = LeftMesh.AnimSets;
			OverlayLeftMesh.SetAnimTreeTemplate(LeftMesh.AnimTreeTemplate);
			OverlayLeftMesh.SetParentAnimComponent(LeftMesh);
			OverlayLeftMesh.SetFOV(LeftMesh.FOV);
		}
		else
		{
			`Warn("Could not create Weapon Overlay mesh for" @ self @ LeftMesh);
		}
	}
}

/**
  * Adjust weapon equip and fire timings so they match between PC and console
  * This is important so the sounds match up.
  */
simulated function AdjustWeaponTimingForConsole()
{
	Super.AdjustWeaponTimingForConsole();

	DualEquipTime = DualEquipTime/1.1;
}

/**
 * Setup the overlay skins
 */
simulated function SetSkin(Material NewMaterial)
{
	local int i,Cnt;
	local UTPawn P;
	Super.SetSkin(NewMaterial);

	if ( LeftMesh != none && NewMaterial == None )	// Clear the materials
	{
		if ( default.LeftMesh.Materials.Length > 0 )
		{
			Cnt = Default.Mesh.Materials.Length;
			for (i=0;i<Cnt;i++)
			{
				LeftMesh.SetMaterial( i, Default.Mesh.GetMaterial(i) );
			}
		}
		else if (LeftMesh.Materials.Length > 0)
		{
			Cnt = LeftMesh.Materials.Length;
			for ( i=0; i < Cnt; i++ )
			{
				LeftMesh.SetMaterial(i,none);
			}
		}
	}
	else
	{
		if ( default.LeftMesh.Materials.Length > 0 || Leftmesh.GetNumElements() > 0 )
		{
			Cnt = default.LeftMesh.Materials.Length > 0 ? default.LeftMesh.Materials.Length : LeftMesh.GetNumElements();
			for ( i=0; i < Cnt; i++ )
			{
				LeftMesh.SetMaterial(i,NewMaterial);
			}
		}
	}
	if(NewMaterial==none)
	{
		P = UTPawn(Instigator);

			WeaponMaterialInstance = MaterialInstanceConstant(Mesh.Materials[0]);
			if(WeaponMaterialInstance == none)
			{
				WeaponMaterialInstance = Mesh.CreateAndSetMaterialInstanceConstant(0);
			}
			LeftWeaponMaterialInstance = MaterialInstanceConstant(LeftMesh.Materials[0]);
			if(LeftWeaponMaterialInstance == none)
			{
				LeftWeaponMaterialInstance = LeftMesh.CreateAndSetMaterialInstanceConstant(0);
			}
		if(P != none)
		{
			if(P.GetTeamNum() == 1) // blue
			{
				WeaponMaterialInstance.SetVectorParameterValue('Enforcer_Diffuse',MakeLinearColor(0,0.1,2,1));
				WeaponMaterialInstance.SetVectorParameterValue('Enforcer_Spec',MakeLinearColor(0,0,7,1));
				LeftWeaponMaterialInstance.SetVectorParameterValue('Enforcer_Diffuse',MakeLinearColor(0,0.1,2,1));
				LeftWeaponMaterialInstance.SetVectorParameterValue('Enforcer_Spec',MakeLinearColor(0,0,7,1));
			}
			else
			{
				WeaponMaterialInstance.SetVectorParameterValue('Enforcer_Diffuse',MakeLinearColor(1.0,0.0,0.0,1.0));
				WeaponMaterialInstance.SetVectorParameterValue('Enforcer_Spec',MakeLinearColor(7.0,0.0,0.0,1.0));
				LeftWeaponMaterialInstance.SetVectorParameterValue('Enforcer_Diffuse',MakeLinearColor(1.0,0.0,0.0,1.0));
				LeftWeaponMaterialInstance.SetVectorParameterValue('Enforcer_Spec',MakeLinearColor(7.0,0.0,0.0,1.0));
			}
		}
	}
}

/**
 * Extend support to allow for multiple muzzle flashes
 */
simulated function AttachMuzzleFlash()
{
	local int i;
	if (MuzzleFlashPSCTemplate != none)
	{
		EnforcerMuzzleFlashPSC[0]  = new(Outer) class'UTParticleSystemComponent';
		SkeletalMeshComponent(Mesh).AttachComponentToSocket(EnforcerMuzzleFlashPSC[0], MuzzleFlashSocket);

		EnforcerMuzzleFlashPSC[1]  = new(Outer) class'UTParticleSystemComponent';
		LeftMesh.AttachComponentToSocket(EnforcerMuzzleFlashPSC[1], MuzzleFlashSocket);

		for (i=0;i<2;i++)
		{
			EnforcerMuzzleFlashPSC[i].SetDepthPriorityGroup(SDPG_Foreground);
			EnforcerMuzzleFlashPSC[i].DeactivateSystem();
			EnforcerMuzzleFlashPSC[i].SetColorParameter('MuzzleFlashColor', MuzzleFlashColor);
		}
	}
}

/**
 * Detach weapon from skeletal mesh
 *
 * @param	SkeletalMeshComponent weapon is attached to.
 */
simulated function DetachMuzzleFlash()
{
	if (EnforcerMuzzleFlashPSC[0] != None)
	{
		SkeletalMeshComponent(Mesh).DetachComponent(EnforcerMuzzleFlashPSC[0]);
	}
	if (EnforcerMuzzleFlashPSC[1] != None)
	{
		LeftMesh.DetachComponent(EnforcerMuzzleFlashPSC[1]);
	}
}

/**
 * This function is called from the pawn when the visibility of the weapon changes
 */
simulated function ChangeVisibility(bool bIsVisible)
{
	local UTPawn UTP;

	Super.ChangeVisibility(bIsVisible);

	if (DualMode != EDM_SingleWeapon)
	{
		if (LeftMesh != None)
		{
			LeftMesh.SetHidden(!bIsVisible);
		}
		UTP = UTPawn(Instigator);
		if (UTP != None && UTP.ArmsMesh[1] != None)
		{
			UTP.AttachComponent(UTP.ArmsMesh[1]);
			UTP.ArmsMesh[1].SetHidden(!bIsVisible);
			if (UTP.ArmsOverlay[1] != None)
			{
				UTP.ArmsOverlay[1].SetHidden(!bIsVisible);
			}
		}
		if (OverlayLeftMesh != none)
		{
			OverlayLeftMesh.SetHidden(!bIsVisible);
		}
	}
}


/**
 * When the Pawn's WeaponOverlay flag changes, handle it here
 */
simulated function SetWeaponOverlayFlags(UTPawn OwnerPawn)
{
	local MaterialInterface InstanceToUse;
	local byte Flags;
	local int i;
	local UTGameReplicationInfo GRI;

	Super.SetWeaponOverlayFlags(OwnerPawn);

	if ( DualMode != EDM_SingleWeapon )
	{
		GRI = UTGameReplicationInfo(WorldInfo.GRI);
		if (GRI != None)
		{
			Flags = OwnerPawn.WeaponOverlayFlags;
			for (i = 0; i < GRI.WeaponOverlays.length; i++)
			{
				if (GRI.WeaponOverlays[i] != None && bool(Flags & (1 << i)))
				{
					InstanceToUse = GRI.WeaponOverlays[i];
					break;
				}
			}
		}

		if (InstanceToUse != none)
		{
			if ( OverlayLeftMesh != none )
			{
				for (i=0;i<OverlayLeftMesh.GetNumElements(); i++)
				{
					OverlayLeftMesh.SetMaterial(i, InstanceToUse);
				}

				if (!OverlayLeftMesh.bAttached)
				{
					AttachComponent(OverlayLeftMesh);
					OverlayLeftMesh.SetHidden(false);
				}
			}
		}
		else if (OverlayLeftMesh != none && OverlayLeftMesh.bAttached)
		{
			OverlayLeftMesh.SetHidden(true);
			DetachComponent(OverlayLeftMesh);
		}
	}
}


/**
 * Turns the MuzzleFlashlight off
 */
simulated event MuzzleFlashOff(int Index)
{
	if ( EnforcerMuzzleFlashPSC[Index] != none )
	{
		EnforcerMuzzleFlashPSC[Index].DeactivateSystem();
	}
}

simulated event MuzzleFlashTimerLeft()
{
	MuzzleFlashOff(1);
}

simulated event MuzzleFlashTimerRight()
{
	MuzzleFlashOff(0);
}


/**
 * Causes the muzzle flashlight to turn on
 */
simulated event CauseMuzzleFlashLight()
{
	local int Index;

	if ( WorldInfo.bDropDetail )
		return;

	Index = UseLeftBarrel() ? 1 : 0;

	if ( EnforcerMuzzleFlashLight[Index] != None )
	{
		EnforcerMuzzleFlashLight[Index].ResetLight();
	}
	else if ( MuzzleFlashLightClass != None )
	{
		EnforcerMuzzleFlashLight[Index] = new(Outer) MuzzleFlashLightClass;
		if ( Index == 1 )
			LeftMesh.AttachComponentToSocket(EnforcerMuzzleFlashLight[Index],MuzzleFlashSocket);
		else
			SkeletalMeshComponent(Mesh).AttachComponentToSocket(EnforcerMuzzleFlashLight[Index],MuzzleFlashSocket);
	}
}

/**
 * Causes the muzzle flashlight to turn on and setup a time to
 * turn it back off again.
 */
simulated event CauseMuzzleFlash()
{
	local int Index;

	CauseMuzzleFlashLight();

	Index = UseLeftBarrel() ? 1 : 0;

	if (EnforcerMuzzleFlashPSC[Index] != none)
	{
		EnforcerMuzzleFlashPSC[Index].SetTemplate(MuzzleFlashPSCTemplate);
		EnforcerMuzzleFlashPSC[Index].SetVectorParameter('MFlashScale',Vect(0.5,0.5,0.5));
		EnforcerMuzzleFlashPSC[Index].ActivateSystem();
	}

	// Set when to turn it off.
	if (Index > 0 )
	{
		SetTimer(MuzzleFlashDuration,false,'MuzzleFlashTimerLeft');
	}
	else
	{
		SetTimer(MuzzleFlashDuration,false,'MuzzleFlashTimerRight');
	}

}

simulated event StopMuzzleFlash()
{
	ClearTimer('MuzzleFlashTimerLeft');
	ClearTimer('MuzzleFlashTimerRight');
	MuzzleFlashOff(0);
	MuzzleFlashOff(1);
}

/**
 * On a remote client, watch for a change in bDualMode and if it changes, signal this has become a dual
 * weapon.
 */
simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'DualMode')
	{
		BecomeDual();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/**
 * Become a dual firing weapon.  Right now this isn't important unless the instigator is the local client.
 * If it is, then show the second mesh and play any needed "Equipping" animations
 */
simulated function BecomeDual()
{
	local UTPawn P;

	if ( DualMode == EDM_DualEquipping )
	{
		MaxAmmoCount = 2 * Default.MaxAmmoCount;
		AttachComponent(LeftMesh);
		P = UTPawn(Instigator);
		SetWeaponOverlayFlags(P);
		SetTimer(DualEquiptime, false, 'DualEquipDone');
		if (P != None)
		{
			if (!Mesh.HiddenGame)
			{
				LeftMesh.SetHidden(false);
				if (WorldInfo.NetMode != NM_DedicatedServer && P.ArmsMesh[1] != None && Instigator.Weapon == self)
				{
					bUsesOffhand = true;
					P.AttachComponent(P.ArmsMesh[1]);
					P.ArmsMesh[1].SetHidden(false);
				}
			}

			P.bDualWielding = true;
			if (P.CurrentWeaponAttachment != None)
			{
				P.CurrentWeaponAttachment.SetDualWielding(true);
			}
		}
		PlayWeaponAnimation( WeaponDualEqipAnim, DualEquiptime,, LeftMesh );
		PlayArmAnimation( WeaponDualEqipAnim, DualEquiptime, true,, LeftMesh );
	}
}
simulated function AttachWeaponTo(SkeletalMeshComponent MeshCpnt, optional Name SocketName )
{
	local UTPawn UTP;

	UTP = UTPawn(Instigator);
	if (UTP != None)
	{
		if (DualMode != EDM_SingleWeapon)
		{
			UTP.bDualWielding = true;
			if (UTP.CurrentWeaponAttachment != None)
			{
				UTP.CurrentWeaponAttachment.SetDualWielding(true);
			}
			UTP.AttachComponent(UTP.ArmsMesh[1]);
		}
		if(UTP.IsFirstPerson())
		{
			if (LeftArmAnimSet != None)
			{
				UTP.ArmsMesh[1].SetHidden(LeftMesh.HiddenGame);
				UTP.ArmsMesh[1].AnimSets[1] = LeftArmAnimSet;
			}
			LeftMesh.SetLightEnvironment(UTP.LightEnvironment);
			UTP.ArmsMesh[1].SetLightEnvironment(UTP.LightEnvironment);
		}
	}
	Super.AttachWeaponTo(MeshCpnt, SocketName);
}


simulated function DetachWeapon()
{
	local UTPawn UTP;
	UTP = UTPawn(Instigator);
	if(UTP != none && UTP.IsLocallyControlled())
	{
		UTP.DetachComponent(UTP.ArmsMesh[1]);
		UTP.ArmsMesh[1].SetLightEnvironment(None);
		UTP.ArmsMesh[1].setHidden(true);
	}

	LeftMesh.SetLightEnvironment(None);

	super.DetachWeapon();
}


/**
 * The equip animation is done, notify the weapon and become usuable
 */
simulated function DualEquipDone()
{
	ShotCount = 0;
	DualMode = EDM_Dual;
	InstantHitDamageTypes[0] = DualEnforcerDamageType;
	InstantHitDamageTypes[1] = DualEnforcerDamageType;
	BecomeDual();
}

/**
 * We override GetHand so that we are always right handed unless hidden
 */
simulated function UTPawn.EWeaponHand GetHand()
{
	local UTPawn.EWeaponHand Handed;

	Handed = Super.GetHand();

	if ( Handed == HAND_Hidden )
		return Handed;
	else
		return HAND_Right;
}

/**
 * Detect that we are trying to pickup another enforcer and switch in to dual mode.
 */
function bool DenyPickupQuery(class<Inventory> ItemClass, Actor Pickup)
{
	if (ItemClass==Class && DualMode != EDM_Dual)
	{
		DualMode = EDM_DualEquipping;
		BecomeDual();			// Handle any animations/effects locally.
	}
	return super.DenyPickupQuery(ItemClass, Pickup);
}

/**
 * Returns the rate at which the weapon fires.  If in dual mode, 1/2 the rate
 *
 * @See Weapon.GetFireInterval()
 */
simulated function float GetFireInterval(byte FireModeNum)
{
	local float FI;

	FI = Super.GetFireInterval(FireModeNum);
	if (FireModeNum == 0 && DualMode == EDM_Dual)
	{
		FI *= 0.5;
	}

	return FI;
}

simulated function InstantFire()
{
	Super.InstantFire();
	LastFireTime = WorldInfo.TimeSeconds;
}

simulated function vector GetEffectLocation()
{
	local vector SocketLocation;
	local Rotator SocketRotation;
	if(bLastFiredLeft)
	{
		if (LeftMesh!=none && EffectSockets[CurrentFireMode]!='')
		{
			if (!LeftMesh.GetSocketWorldLocationAndrotation(EffectSockets[CurrentFireMode], SocketLocation, SocketRotation))
				SocketLocation = Location;
		}
		else if (LeftMesh!=none)
			SocketLocation = LeftMesh.Bounds.Origin + (vect(45,0,0) >> Rotation);
		else
			SocketLocation = Location;

 		return SocketLocation;
	}
	else
	{
		return super.GetEffectLocation();
	}
}


/**
 * Adds any fire spread offset to the passed in rotator
 * @param BaseAim the base aim direction
 * @return the adjusted aim direction
 */
simulated function rotator AddSpread(rotator BaseAim)
{
	local float SpreadMod;

	SpreadMod = PendingFire(1) ? 1.3 : 1.0;
	if ( WorldInfo.TimeSeconds - LastFireTime > 0.6 )
	{
		Spread[CurrentFireMode] = Default.Spread[CurrentFireMode];
	}
	else
	{
		if ( DualMode == EDM_Dual )
		{
			Spread[0] = FMin(Spread[0]+0.025,0.1);
			Spread[1] = FMin(Spread[1]+0.05,0.15);
		}
		else
		{
			Spread[0] = FMin(Spread[0]+0.015,0.075);
			Spread[1] = FMin(Spread[1]+0.05,0.15);
		}

		Spread[0] *= SpreadMod;
		Spread[1] *= SpreadMod;
	}

	return Super.AddSpread(BaseAim);
}

function byte BestMode()
{
	local UTBot B;

	B = UTBot(Instigator.Controller);
	if (B == None || B.Enemy == None)
	{
		return 0;
	}

	// use burst fire at short range
	return ( VSize(B.Enemy.Location - Instigator.Location) < 500.0 ) ? 1 : 0;
}

simulated function float GetEquipTime()
{
	return bLoaded ? EquipTime : ReloadTime;
}

/**
 * By default, we track each shot so that the 2nd weapon will work.  Bursting overrides this function
 */

simulated function TrackShotCount()
{
	ShotCount++;
}

/**
 * Determine which gun to use
 */
simulated function bool UseLeftBarrel()
{
	return ( DualMode == EDM_Dual && (ShotCount % 2 > 0 ) );
}

/**
 * Returns the name of the firing animation
 */

simulated function name GetFireAnim(int FireModeNum)
{
	return WeaponFireAnim[FireModeNum];
}

/**
 * We override PlayFireEffects to reoffset the animation rate.  When in dual mode, all firing speeds
 * are 1/2 but we want the animations to play as normal.
 *
 * @See PlayFireEffects
 */

simulated function PlayFireEffects( byte FireModeNum, optional vector HitLocation )
{
	local float Rate;
	// Play Weapon fire animation

	Rate = GetFireInterval(FireModeNum);

	// If we are in dual mode, we want to back it out to the normal speed

	if (DualMode == EDM_Dual)
	{
		Rate *= 2;
	}

	if ( FireModeNum < WeaponFireAnim.Length && WeaponFireAnim[FireModeNum] != '' )
	{
		PlayWeaponAnimation( GetFireAnim(FireModeNum), Rate,, UseLeftBarrel() ? LeftMesh : None );
		PlayArmAnimation( GetFireAnim(FireModeNum), Rate, UseLeftBarrel(),, UseLeftBarrel() ? LeftMesh : None );
		bLastFiredLeft=UseLeftBarrel();
	}

	// Start muzzle flash effect
	CauseMuzzleFlash();

	ShakeView();

	TrackShotCount();
}

/**
 * Look to see if bLoaded (ie: has been brought up once) is true and decide which animation to play
 */

simulated function PlayWeaponEquip()
{
	local name Anim;
	local SoundCue EquipSound;

	// Play the animation for the weapon being put down
	if (bLoaded)
	{
		Anim = WeaponEquipAnim;
		EquipSound = WeaponEquipSnd;
	}
	else
	{
		Anim = WeaponReloadAnim;
		EquipSound = WeaponReloadSnd;
	}

	if ( Anim != '' )
	{
		PlayWeaponAnimation( Anim, GetEquipTime() );
		PlayArmAnimation( Anim, GetEquipTime() );
		if ( DualMode == EDM_Dual )
		{
			PlayWeaponAnimation( Anim, GetEquipTime(),,LeftMesh);
			PlayArmAnimation( Anim, GetEquipTime(),true,,LeftMesh);
		}
	}

	// play any assoicated sound
	if (EquipSound != None)
	{
		WeaponPlaySound(EquipSound);
	}
}

/**
 * Added support for handling the 2nd gun
 */

simulated function PlayWeaponPutDown()
{
	// Play the animation for the weapon being put down

	if ( WeaponPutDownAnim != '' )
	{
		PlayWeaponAnimation( WeaponPutDownAnim, PutDownTime );
		PlayArmAnimation( WeaponPutDownAnim, PutDownTime );
		if ( DualMode == EDM_Dual )
		{
			PlayWeaponAnimation( WeaponPutDownAnim, PutDownTime,,LeftMesh);
			PlayArmAnimation( WeaponPutDownAnim, PutDownTime,true,,LeftMesh);
		}
	}

	// play any associated sound
	if ( WeaponPutDownSnd != None )
		WeaponPlaySound( WeaponPutDownSnd );
}

simulated state WeaponEquipping
{
	simulated function WeaponEquipped()
	{
		if( bWeaponPutDown )
		{
			// if switched to another weapon, put down right away
			PutDownWeapon();
			return;
		}
		else
		{
			GotoState('Active');
		}
	}
	simulated function EndState(Name NextStateName)
	{
		Super.EndState(NextStateName);
		bLoaded = true;
	}
}

simulated state WeaponFiring
{

	simulated function BecomeDual()
	{
		Global.BecomeDual();
		if ( DualMode == EDM_Dual )
		{
			bForceReturnToActive=true;
		}
	}

	/**
	 * We poll the PendingFire in RefireCheckTimer() to insure the switch to Burst Firing
	 * respects the current shot timings
	 */

	simulated function RefireCheckTimer()
	{
		// if switching to another weapon, abort firing and put down right away
		if( bWeaponPutDown )
		{
			PutDownWeapon();
			return;
		}

		if (bForceReturnToActive)
		{
			GotoState('Active');
			return;
		}

		if ( PendingFire(1) )
		{

			SendToFiringState(1);
			return;
		}

		Super.RefireCheckTimer();
	}
}

simulated state WeaponBursting extends WeaponFiring
{
	/**
	 * We only play the animation once.
	 *
	 * @See Weapon.PlayWeaponAnimation
	 */

	simulated function PlayWeaponAnimation( Name Sequence, float fDesiredDuration, optional bool bLoop, optional SkeletalMeshComponent SkelMesh)
	{
		if (BurstCnt != 1)
		{
			return;
		}

		fDesiredDuration = 0.8667; // @FIXME - Match anims to shot speed

		Global.PlayWeaponAnimation(Sequence, fDesiredDuration, bLoop, SkelMesh);
	}

	simulated function PlayArmAnimation( Name Sequence, float fDesiredDuration, optional bool bOffHand, optional bool bLoop, optional SkeletalMeshComponent SkelMesh)
	{
		if (BurstCnt != 1)
		{
			return;
		}

		fDesiredDuration = 0.8667; // @FIXME - Match anims to shot speed

		Global.PlayArmAnimation(Sequence, fDesiredDuration, bOffHand, bLoop, SkelMesh);
	}


	simulated function name GetFireAnim(int FireModeNum)
	{
		return BurstFireAnimName;
	}

	/**
	 * CoolDown tracks the shots
	 */
	simulated function TrackShotCount();

	simulated function bool TryPutDown()
	{
		bWeaponPutDown = true;
		return true;
	}

	simulated function RefireCheckTimer()
	{
		// if out of ammo, then call weapon empty notification
		if( !HasAnyAmmo() )
		{
			WeaponEmpty();
		}
		else
		{
			// Check to see if we have shot our burst
			if (BurstCnt == BurstMax || !HasAmmo(CurrentFireMode))
			{
				GotoState('WeaponBurstCoolDown');
			}
			else
			{
				// If weapon should keep on firing, then do not leave state and fire again.
				BurstCnt++;
				FireAmmunition();
			}
		}
	}

	simulated function BeginState(name PrevStateName)
	{
		BurstCnt = 1;
		Super.BeginState(PrevStateName);
	}

	simulated function EndState(name NextStateName)
	{
		Super.EndState(NextStateName);
		Cleartimer('RefireCheckTimer');
	}
}

simulated State WeaponBurstCoolDown
{
	simulated function EndState(name NextStateName)
	{
		super.EndState(NextStateName);
		ClearTimer('WeaponReady');
		TrackShotCount();
	}

	simulated function BeginState(name PrevStateName)
	{

		local float CoolDowntime;

		if ( UTPawn(Owner) == None || UTPawn(Owner).FireRateMultiplier==1.0 )
		{
			CoolDownTime = BurstCoolDowntime;
		}
		else
		{
			CoolDownTime = BurstCoolDowntime*0.3125;
		}

		if (DualMode == EDM_Dual)
		{
	    if ( !bFullDualCoolDown )
	    {
		CoolDownTime = FireInterval[1];
	    }
	    bFullDualCoolDown = !bFullDualCoolDown;
		}

		SetTimer(CoolDowntime,false,'WeaponReady');

	}

	simulated function WeaponReady()
	{
		// if switching to another weapon, abort firing and put down right away
		if( bWeaponPutDown )
		{
			PutDownWeapon();
		}
		else if ( PendingFire(1) )
		{
			SendToFiringState(1);
		}
		else
		{
	    if ( bFullDualCoolDown )
	    {
		SetTimer(BurstCoolDownTime - FireInterval[1], false, 'FullyCooled');
	    }
			GotoState('Active');
		}
	}
}

simulated function FullyCooled()
{
    bFullDualCoolDown = false;
}

defaultproperties
{
	WeaponColor=(R=255,G=255,B=255,A=255)
	FireInterval(0)=+0.36

	BurstMax=3
	BurstCoolDownTime=0.97
	FireInterval(1)=+0.12

	WeaponFireTypes(0)=EWFT_InstantHit
	WeaponFireTypes(1)=EWFT_InstantHit
	FiringStatesArray(1)=WeaponBursting
	ShotCost(1)=1
	Spread(1)=0.03
	PlayerViewOffset=(X=0.0,Y=0.0,Z=0.0)
	MaxPitchLag=600
	MaxYawLag=800
	RotChgSpeed=3.0
	ReturnChgSpeed=3.0

	// First Enforcer
	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object class=AnimNodeSequence Name=MeshSequenceB
	End Object

	// Weapon SkeletalMesh
	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_Enforcers.Mesh.SK_WP_Enforcers_1P'
		PhysicsAsset=None
		AnimSets(0)=AnimSet'WP_Enforcers.Anims.K_WP_Enforcers_1P_Base'
		Materials(0)=Material'WP_Enforcers.Materials.M_WP_Enforcers_01'
		Animations=MeshSequenceA
		//Scale=1.1
		FOV=55.0
	End Object

	ArmsAnimSet=AnimSet'WP_Enforcers.Anims.K_WP_Enforcers_1P_Arms'
	LeftArmAnimSet=AnimSet'WP_Enforcers.Anims.K_WP_Enforcers_1P_Arms'

	// Weapon SkeletalMesh
	Begin Object Class=UTSkeletalMeshComponent Name=FirstPersonMeshLeft
		SkeletalMesh=SkeletalMesh'WP_Enforcers.Mesh.SK_WP_Enforcers_1P'
		PhysicsAsset=None
		AnimSets(0)=AnimSet'WP_Enforcers.Anims.K_WP_Enforcers_1P_Base'
		Materials(0)=Material'WP_Enforcers.Materials.M_WP_Enforcers_01'
		Animations=MeshSequenceA
		//Scale=1.1
		Scale3D=(Y=-1.0)
		FOV=55.0
		DepthPriorityGroup=SDPG_Foreground
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bOnlyOwnerSee=true
		HiddenGame=true
	End Object

	LeftMesh=FirstPersonMeshLeft

	AttachmentClass=class'UTGame.UTAttachment_Enforcer'

	// Pickup staticmesh
	Begin Object Name=PickupMesh
		SkeletalMesh=SkeletalMesh'WP_Enforcers.Mesh.SK_WP_Enforcer_3P'
		Translation=(X=0.0,Y=0.0,Z=-20.0)
		Scale=0.5
	End Object

	InstantHitDamage(0)=20
	InstantHitDamage(1)=20

	InstantHitDamageTypes(0)=class'UTDmgType_Enforcer'
	InstantHitDamageTypes(1)=class'UTDmgType_Enforcer'
	DualEnforcerDamageType=class'UTDmgType_DualEnforcer'

	InstantHitMomentum(0)=1000
	InstantHitMomentum(1)=1000

	WeaponFireSnd[0]=SoundCue'A_Weapon_Enforcer.Cue.A_Weapon_Enforcer_Fire_Cue'
	WeaponFireSnd[1]=SoundCue'A_Weapon_Enforcer.Cue.A_Weapon_Enforcer_Fire_Cue'

	WeaponPutDownSnd=SoundCue'A_Weapon_Enforcer.Cue.A_Weapon_Enforcer_Lower_Cue'
	WeaponEquipSnd=SoundCue'A_Weapon_Enforcer.Cue.A_Weapon_Enforcer_Raise_Cue'
	//WeaponReloadSnd=SoundCue'A_Weapon_Enforcer.Cue.A_Weapon_Enforcer_ReloadComposite_Cue' // We do this with an anim notify ATM.

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Enforcer_Cue'

	AimingHelpRadius[0]=20.0
	AimingHelpRadius[1]=12.0

	MaxDesireability=0.4
	AIRating=+0.4
	CurrentRating=0.4
	BotRefireRate=0.99
	bInstantHit=true
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0
	InventoryGroup=2
	GroupWeight=0.5

	AmmoCount=50
	LockerAmmoCount=50
	MaxAmmoCount=100

	FireShake(0)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=1.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=2,RotMag=(X=50.0,Y=50.0,Z=50.0),RotRate=(X=10000.0,Y=10000.0,Z=10000.0),RotTime=2)

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_Enforcers.Effects.P_WP_Enforcers_MuzzleFlash'
	MuzzleFlashDuration=0.33
	MuzzleFlashLightClass=class'UTGame.UTEnforcerMuzzleFlashLight'

	IconX=335
	IconY=81
	IconWidth=43
	IconHeight=39

	WeaponIdleAnims(0)=WeaponIdle
	ArmIdleAnims(0)=WeaponIdle

	EquipTime=+0.2
	PutDownTime=+0.2

	//WeaponReloadAnim=WeaponEquipFirst
	WeaponReloadAnim=Weaponequipempty
	ArmReloadAnim=WeaponEquipFirst

	WeaponDualEqipAnim=WeaponEquiplFancy
	ArmDualEquipAnim=WeaponEquiplFancy

	BurstFireAnimName=WeaponFireBurst
	ArmBurstFireAnimName=WeaponFireBurst

	ReloadTime=2.0
	DualEquiptime=1.0

	DualMode=EDM_SingleWeapon
	JumpDamping=1.5
	CrossHairCoordinates=(U=128,V=0,UL=64,VL=64)

	QuickPickGroup=1
	QuickPickWeight=0.2
}
