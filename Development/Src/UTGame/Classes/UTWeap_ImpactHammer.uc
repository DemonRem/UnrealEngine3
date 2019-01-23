/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_ImpactHammer extends UTWeapon
	HideDropDown
	native;

var float MinDamage;
var float MaxDamage;
var float MinForce;
var float MaxForce;
var float MinSelfDamage;
var float SelfForceScale;
var float SelfDamageScale;

var int PowerLevel;

/** This holds when the hammer began charging */
var float ChargeTime;

/** The charging animations*/
var name ChargeAnim;
var name ChargeIdleAnim;

/** How long does it take for a full charge */
var float MaxChargeTime;

/** Each shot will have at least this amount of charge */
var float MinChargeTime;

/** The sound that plays while charging */
var SoundCue WeaponChargeSnd;

/** Sound played when you take damage from your own hammer */
var SoundCue ImpactJumpSound;

/** This is the actor that was hit automatically */
var actor AutoHitActor;

/** auto fire range */
var float AutoFireRange;

/** Max damage done to vehicle by EMP (alt-fire) */
var float EMPDamage;

/** currently charging hammer */
var bool bIsCurrentlyCharging;

var particlesystem ChargeEffect[2];

var MaterialInstanceConstant BloodMIC;

var ParticleSystem AltHitEffect;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

simulated function SetSkin(Material NewMaterial)
{
	super.SetSkin(NewMaterial);
	BloodMIC = Mesh.CreateAndSetMaterialInstanceConstant(0);
}

function GivenTo(Pawn ThisPawn, optional bool bDoNotActivate)
{
	Super.GivenTo(ThisPawn, bDoNotActivate);

	if (UTBot(ThisPawn.Controller) != None)
	{
		// tell the bot it has an impact hammer and how much boost it can get from it
		UTBot(ThisPawn.Controller).ImpactJumpZ = MinForce / ThisPawn.Mass;
	}
}

simulated function rotator GetAdjustedAim(vector StartFireLoc)
{
	local UTBot B;

	// if it's a bot trying to impact jump, make it shoot straight down
	B = UTBot(Instigator.Controller);
	if (B != None && B.bPlannedJump && B.ImpactTarget != None)
	{
		return rotator(vect(0,0,-1));
	}

	return Super.GetAdjustedAim(StartFireLoc);
}

function float GetAIRating()
{
	local UTBot B;
	local float EnemyDist;

	B = UTBot(Instigator.Controller);
	if (B == None)
	{
		return AIRating;
	}
	// super desireable for bot waiting to impact jump
	if (B.bPreparingMove && B.ImpactTarget != None)
	{
		return 9.f;
	}

	if (B.Enemy == None)
	{
		return AIRating;
	}

	EnemyDist = VSize(B.Enemy.Location - Instigator.Location);
	if (B.Stopped() && EnemyDist > 100.f)
	{
		return 0.1;
	}
	if (EnemyDist < 750.f && B.Skill <= 2 && UTWeap_ImpactHammer(B.Enemy.Weapon) != None)
	{
		return FClamp(300.f / (EnemyDist + 1.f), 0.6, 0.75);
	}
	if (EnemyDist > 400)
	{
		return 0.1;
	}
	if (Instigator.Weapon != self && EnemyDist < 120)
	{
		return 0.25;
	}
	return FMin(0.6, 90.f / (EnemyDist + 1.f));
}

reliable client function ClientAutoFire()
{
	if (Role < ROLE_Authority)
	{
		ImpactFire();
	}
}

simulated function ImpactFire()
{
	FireAmmunition();
	//StopMuzzleFlash();
	MuzzleFlashAltPSCTemplate = default.MuzzleFlashAltPSCTemplate;
	MuzzleFlashPSCTemplate = default.MuzzleFlashPSCTemplate;
	bMuzzleFlashPSCLoops = false;
	CauseMuzzleFlash();
	GotoState('WeaponRecharge');
}

simulated function bool HasAnyAmmo()
{
	return true;
}

/**  figure out how close P is to aiming at the center of Target
	@return the cosine of the angle of P's aim
*/
function float CalcAim(Pawn P, Pawn Target)
{
	local float Aim, EffectiveSkill;
	local UTBot B;

	Aim = vector(P.GetViewRotation()) dot Normal(Target.Location - P.Location);
	B = UTBot(P.Controller);
	if (B != None)
	{
		EffectiveSkill = B.Skill;
		if (B.FavoriteWeapon == Class)
		{
			EffectiveSkill += 3.0;
		}
		// if the bot just happens to be looking away from the target, use the real angle, otherwise make one up based on the bot's skill
		Aim = FMin(Aim, FMin(1.0, 1.0 - 0.30 + (0.02 * EffectiveSkill) + (0.10 * FRand())));
	}

	return Aim;
}

simulated function InstantFire()
{
	super.InstantFire();
	SetFlashLocation(Location);
}

simulated function PlayImpactEffect(byte FiringMode, ImpactInfo Impact)
{
	local float Damage1,Damage2,Damage3;
	local Pawn HitPawn;

	if (FiringMode == 1 && Impact.HitActor != None)
	{
		WorldInfo.MyEmitterPool.SpawnEmitter(AltHitEffect, Impact.HitLocation, rotator(Impact.HitLocation - Instigator.Location));
	}
	else if( FiringMode == 0 )
	{
		HitPawn = UTPawn(Impact.HitActor);
		if ( HitPawn == None )
		{
			HitPawn = UTVehicle_Hoverboard(Impact.HitActor);
		}
		if ( (HitPawn != None) && !WorldInfo.GRI.OnSameTeam(HitPawn, Instigator) )
		{
			BloodMIC.GetScalarParameterValue('Damage1',Damage1);
			BloodMIC.GetScalarParameterValue('Damage2',Damage2);
			BloodMIC.GetScalarParameterValue('Damage3',Damage3);
			Damage1 = Damage1+frand()*2.0;
			Damage2 = Damage2+frand()*1.0;
			Damage3 = Damage3+frand()*0.75;
			BloodMIC.SetScalarParameterValue('Damage1',Damage1);
			BloodMIC.SetScalarParameterValue('Damage2',Damage2);
			BloodMIC.SetScalarParameterValue('Damage3',Damage3);
			SetCurrentFireMode(3); // replicate a hit out to weapon attachments
		}
	}
}

simulated function ProcessInstantHit( byte FiringMode, ImpactInfo Impact )
{
	local float Damage, SelfDamage, Force, Scale, Aim, EnemyAim;
	local UTInventoryManager UTInvManager;
	local UTPawn P;
	local UTVehicle_Hoverboard H;

	local class<UTDamageType> UTDT;

	if(WorldInfo.NetMode != NM_DEDICATEDSERVER && Impact.HitActor != none)
	{
		PlayImpactEffect(FiringMode, Impact);
	}

	if (Role == Role_Authority )
	{
		// If we auto-hit something, guarantee the strike
		if ( AutoHitActor != none )
		{
			Impact.HitActor = AutoHitActor;
			AutoHitActor = none;
		}

		if ( Impact.HitActor != None && Instigator != None && Instigator.Health > 0 )
		{
			// if we hit something on the server, then deal damage to it.

			// Track Stats
			if(  Pawn(Impact.HitActor) != none || Objective(Impact.HitActor) != none )
			{
				UpdateHitStats(true);
			}

		    Scale = (FClamp(WorldInfo.TimeSeconds - ChargeTime, MinChargeTime, MaxChargeTime) - MinChargeTime) / (MaxChargeTime - MinChargeTime); // result 0 to 1

			if ( FiringMode == 0 )
			{
				Damage = MinDamage + Scale * (MaxDamage - MinDamage);
				Force = MinForce + Scale * (MaxForce - MinForce);
				P = UTPawn(Impact.HitActor);
				if ( P == None )
				{
					H = UTVehicle_Hoverboard(Impact.HitActor);
					if ( H != None )
					{
						P = UTPawn(H.Driver);
						if ( P == None )
						{
							return;
						}
					}
				}
				if (P != None && P != Instigator)
				{
					if ( VSize(Impact.HitLocation - Instigator.GetWeaponStartTraceLocation()) > AutoFireRange )
					{
						// no damage if out of close range
						Damage = 0;
					}
					// if the other pawn is also trying to hammer us, allow the player with the better aim to win
					if (P.Weapon != None && P.Weapon.Class == Class && P.Weapon.IsFiring() && P.Weapon.CurrentFireMode == CurrentFireMode)
					{
						EnemyAim = CalcAim(P, Instigator);
						Aim = CalcAim(Instigator, P);
						if (EnemyAim > Aim)
						{
							// cause the enemy hammer to release and damage our pawn
							P.Weapon.StopFire(CurrentFireMode);
							// if our pawn died, bail
							if (Instigator == None || Instigator.Health <= 0)
							{
								return;
							}
						}
					}
					P.TakeDamage(Damage, Instigator.Controller, Impact.HitLocation, Force * Impact.RayDir, InstantHitDamageTypes[0], Impact.HitInfo, self);

					if( (P.Health <= 0) && (UTPlayerController(Instigator.Controller) != None) )
					{
						UTDT = class<UTDamageType>(InstantHitDamageTypes[0]);

						if (UTDT.default.DeathCameraEffectInstigator != None)
						{
							UTPlayerController(Instigator.Controller).ClientSpawnCameraEffect(P, UTDT.default.DeathCameraEffectInstigator);
						}
					}

				}
				else
				{
					SelfDamage = MinSelfDamage + (SelfDamageScale * Damage);
					Impact.HitActor.TakeDamage(0, Instigator.Controller, Impact.HitLocation, Force * Impact.RayDir, InstantHitDamageTypes[0], Impact.HitInfo, self);
					Instigator.TakeDamage(SelfDamage, Instigator.Controller, Location, SelfForceScale * Force * Impact.RayDir, InstantHitDamageTypes[0], Impact.HitInfo, self);
					WeaponPlaySound(ImpactJumpSound);
				}
			}
			else
			{
				// EMP pulse
				if ( Vehicle(Impact.HitActor) != None )
				{
					Impact.HitActor.TakeDamage(Scale * EMPDamage, Instigator.Controller, Impact.HitActor.location, -(Impact.HitActor.velocity * 0.2), InstantHitDamageTypes[1], Impact.HitInfo, self);
				}
				else if ( UTPawn(Impact.HitActor) != None )
				{
						UTInvManager = UTInventoryManager(UTPawn(Impact.HitActor).InvManager);
						if(UTInvManager != none)
						{
							if (UTInvManager.DisruptInventory())
							{
								Force = MinForce + Scale * (MaxForce - MinForce);
								Impact.HitActor.TakeDamage(0,Instigator.Controller,Impact.HitLocation, Force*Impact.RayDir, InstantHitDamageTypes[1], Impact.HitInfo, self);
							}
						}
				}
				else if ( UTDeployedActor(Impact.HitActor) != None )
				{
					UTDeployedActor(Impact.HitActor).HitByEMP();
				}
			}
			PowerLevel = 0;
		}
	}
}

// always have hammer and always have EMPPulse.
simulated function bool HasAmmo( byte FireModeNum, optional int Amount )
{
	return true;
}

simulated event float GetPowerPerc()
{
	return 0;
}

simulated state Active
{
	simulated function BeginState(name PreviousStateName)
	{
		local UTBot B;

		Super.BeginState(PreviousStateName);

		// if it's a bot waiting to impact jump, tell it to do that now
		B = UTBot(Instigator.Controller);
		if (B != None && B.bPreparingMove && B.ImpactTarget != None)
		{
			B.ImpactJump();
		}
	}
}

event ImpactAutoFire()
{
	local Vector		StartTrace, EndTrace;
	local vector		HitLocation, HitNormal;
	local actor			HitActor;

	// define trace end points
	StartTrace	= Instigator.GetWeaponStartTraceLocation();
	EndTrace	= StartTrace + Vector(GetAdjustedAim( StartTrace )) * AutoFireRange;

	// Perform trace to retrieve hit info
	HitActor = Trace(HitLocation, HitNormal, EndTrace, StartTrace, true,,,TRACEFLAG_Blocking);
	if ( (HitActor != None) && !HitActor.bWorldGeometry )
	{
		AutoHitActor = None;
		if ( CurrentFireMode == 0 )
		{
			// auto fire against non-vehicles and translocators
			if ( ((Pawn(HitActor) != None) && ((Vehicle(HitActor) == None) || HitActor.IsA('UTVehicle_Hoverboard'))) || (UTProj_TransDisc(HitActor) != None) )
			{
				AutoHitActor = HitActor;
				ClientAutoFire();
				ImpactFire();
			}
		}
		else
		{
			// auto fire against vehicles, UTPawns, and deployables
			if ( (Vehicle(HitActor) != None) || (UTDeployedActor(HitActor) != None) || UTPawn(HitActor) != none )
			{
				AutoHitActor = HitActor;
				ClientAutoFire();
				ImpactFire();
			}
		}
	}
}

/*********************************************************************************************
 * State WeaponLoadAmmo
 * In this state, ammo will continue to load up until MAXLOADCOUNT has been reached.  It's
 * similar to the firing state
 *********************************************************************************************/
simulated state WeaponChargeUp extends Active
{

	simulated event float GetPowerPerc()
	{
		local float p;
		p = (WorldInfo.TimeSeconds - ChargeTime) / MaxChargeTime;
		p = FClamp(P,0.0,1.0);
		return p;
	}

	simulated event bool IsFiring()
	{
		return true;
	}

	simulated function EndFire(byte FireModeNum)
	{
		// Pass along to the global to handle everything
		Global.EndFire(FireModeNum);
		ImpactFire();
	}

	/**
	 * Initialize the alt fire state
	 */
	simulated function BeginState(Name PreviousStateName)
	{
		local UTPawn POwner;
		local UTAttachment_ImpactHammer Attach;

		ChargeTime = WorldInfo.TimeSeconds;
		PlayWeaponAnimation(ChargeAnim, MaxChargeTime);
		PlayArmAnimation(ChargeAnim, MaxChargeTime);
		WeaponIdleAnims[0] = ChargeIdleAnim;
		ArmIdleAnims[0] = ChargeIdleAnim;
		POwner = UTPawn(Instigator);
		bIsCurrentlyCharging = true;
		if (POwner != None)
		{
			POwner.SetWeaponAmbientSound(WeaponChargeSnd);
			Attach = UTAttachment_ImpactHammer(POwner.CurrentWeaponAttachment);
			if(Attach != none)
			{
				Attach.StartCharging();
			}
		}
		//ChargeEffect[CurrentFireMode<2?CurrentFireMode:byte(0)].ActivateSystem();
		MuzzleFlashAltPSCTemplate = ChargeEffect[1];
		MuzzleFlashPSCTemplate = ChargeEffect[0];
		bMuzzleFlashPSCLoops = true;
		CauseMuzzleFlash();
	}

	/**
	 * Clear Timers / Sounds
	 */
	simulated function EndState(Name NextStateName)
	{
		local UTPawn POwner;

		POwner = UTPawn(Instigator);
		if (POwner != None)
		{
			POwner.SetWeaponAmbientSound(None);
		}

		//ClearFlashCount();
		//ClearFlashLocation();
		bIsCurrentlyCharging = false;
		WeaponIdleAnims[0] = default.WeaponIdleAnims[0];
		ArmIdleAnims[0] = default.ArmIdleAnims[0];
		ClearTimer('RefireCheckTimer');
	}

	/**
	 * When we are in the firing state, don't allow for a pickup to switch the weapon
	 */
	simulated function bool DenyClientWeaponSet()
	{
		return true;
	}

	simulated function bool TryPutdown()
	{
		bWeaponPutDown = true;
		return true;
	}
}

simulated state WeaponRecharge
{
	/**
	 * When we are in the firing state, don't allow for a pickup to switch the weapon
	 */
	simulated function bool DenyClientWeaponSet()
	{
		return true;
	}

	simulated function bool TryPutdown()
	{
		bWeaponPutDown = true;
		return true;
	}

	simulated function BeginState(Name PreviousStateName)
	{
		SetTimer(FireInterval[0], false, 'Recharged'); // can't use CurrentFireMode because we change it to other values to replicate effects
		Super.BeginState(PreviousStateName);
	}

	simulated function EndState(name NextStateName)
	{
		Super.EndState(NextStateName);
		ClearTimer('Recharged');
		ClearFlashCount();
		ClearFlashLocation();
		SetCurrentFireMode(2);
	}

	simulated function Recharged()
	{
		GotoState('Active');
	}
}

simulated function StopFireEffects(byte FireModeNum);

defaultproperties
{
	WeaponColor=(R=255,G=255,B=128,A=255)
	PlayerViewOffset=(X=0,Y=0.0,Z=0)

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_ImpactHammer.Mesh.SK_WP_Impact_1P'
		Materials(0)=Material'WP_ImpactHammer.Materials.M_WP_ImpactHammer_Base'
		PhysicsAsset=None
		AnimSets(0)=AnimSet'WP_ImpactHammer.Anims.K_WP_Impact_1P_Base'
		Animations=MeshSequenceA
		FOV=75
	End Object
	AttachmentClass=class'UTGame.UTAttachment_ImpactHammer'

	WeaponChargeSnd=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_FireLoop_Cue'
	WeaponFireSnd[0]=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_AltFire_Cue'
	WeaponFireSnd[1]=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_AltImpact_Cue'
	WeaponPutDownSnd=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_Lower_Cue'
	WeaponEquipSnd=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_Raise_Cue'

	WeaponFireTypes(0)=EWFT_InstantHit
	WeaponFireTypes(1)=EWFT_InstantHit
	WeaponFireTypes(2)=EWFT_None
	WeaponFireTypes(3)=EWFT_None
	Spread(2)=0.0

	FiringStatesArray(0)=WeaponChargeUp
	FiringStatesArray(1)=WeaponChargeUp

	WeaponRange=110.0
	AutoFireRange=110.0

	FireInterval(0)=1.1//+0.1
	FireInterval(1)=1.1//+0.1

	FireOffset=(X=25)

	InstantHitDamageTypes(0)=class'UTDmgType_ImpactHammer'
	InstantHitDamage(0)=10
	InstantHitDamageTypes(1)=class'UTDmgType_EMP'
	InstantHitDamage(1)=10
	InstantHitDamageTypes(2)=none
	InstantHitDamageTypes(3)=class'UTDmgType_ImpactHammer' // 3 is when bloody from firetype 0

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_ImpactHammer.Particles.P_WP_ImpactHammer_Primary_Hit'
	MuzzleFlashAltPSCTemplate=ParticleSystem'WP_ImpactHammer.Particles.P_WP_ImpactHammer_Secondary_Hit'
	bMuzzleFlashPSCLoops=true
	MuzzleFlashDuration=0.33
	ChargeEffect[0]=ParticleSystem'WP_ImpactHammer.Particles.P_WP_ImpactHammer_Charge_Primary'
	ChargeEffect[1]=ParticleSystem'WP_ImpactHammer.Particles.P_WP_Impact_Charge_Secondary'


	AIRating=+0.35
	CurrentRating=+0.45
	BotRefireRate=0.99
	bInstantHit=true
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0
	bCanThrow=false
	bMeleeWeapon=true

	InventoryGroup=1
	GroupWeight=0.7

	ShotCost(0)=0
	ShotCost(1)=1

	FireCameraAnim[0]=CameraAnim'Camera_FX.ImpactHammer.C_WP_ImpactHammer_Primary_Fire_Shake'
	FireCameraAnim[1]=CameraAnim'Camera_FX.ImpactHammer.C_WP_ImpactHammer_Alt_Fire_Shake'

 	WeaponFireAnim(0)=WeaponFire
 	WeaponFireAnim(1)=WeaponFire
 	WeaponPutDownAnim=WeaponPutDown
	ArmsPutDownAnim=WeaponPutDown
	WeaponEquipAnim=WeaponEquip
	ArmsEquipAnim=WeaponEquip
	WeaponIdleAnims(0)=WeaponIdle
	ArmIdleAnims(0)=WeaponIdle

	ChargeAnim=weaponcharge
	ChargeIdleAnim=weaponchargedidle

	MaxDamage=140.0
	MinDamage=20.0
	MinForce=40000.0
	MaxForce=100000.0
	MinSelfDamage=8
	SelfForceScale=-1.2
	SelfDamageScale=0.3

	MaxChargeTime=2.5
	MinChargeTime=1.0

	CrossHairCoordinates=(U=64,V=0,UL=64,VL=64)
	IconCoordinates=(U=134,V=367,UL=84,VL=50)

	AmmoCount=0;
	MaxAmmoCount=5;
	EMPDamage=150.0

	AmmoDisplayType=EAWDS_BarGraph
	ArmsAnimSet=AnimSet'WP_ImpactHammer.Anims.K_WP_Impact_1P_Arms'
	ImpactJumpSound=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_FireBodyThrow_Cue'

	AltHitEffect=ParticleSystem'WP_ImpactHammer.Particles.P_WP_ImpactHammer_Secondary_Hit_Impact'
}
