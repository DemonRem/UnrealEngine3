/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Leviathan extends UTVehicle_Deployable
	native(Vehicle)
	abstract;

// Define all of the variables needed for the turrets

var repnotify	vector 				LFTurretFlashLocation;
var repnotify	byte				LFTurretFlashCount;
var repnotify 	rotator 			LFTurretWeaponRotation;

var repnotify	vector 				RFTurretFlashLocation;
var repnotify	byte				RFTurretFlashCount;
var repnotify 	rotator 			RFTurretWeaponRotation;

var repnotify	vector 				LRTurretFlashLocation;
var repnotify	byte				LRTurretFlashCount;
var repnotify 	rotator 			LRTurretWeaponRotation;

var repnotify	vector 				RRTurretFlashLocation;
var repnotify	byte				RRTurretFlashCount;
var repnotify 	rotator 			RRTurretWeaponRotation;

/** These are the turret Shock Beams */

var ParticleSystem 			BeamTemplate;
var name					BeamEndpointVarName;

/** This is the primary beam emitter */

var ParticleSystem 			BigBeamTemplate;
var ParticleSystemComponent BigBeamEmitter;
var name					BigBeamEndpointVarName;
var name					BigBeamSocket;

// Shields

var UTVehicleShield Shield[4];
var class<UTVehicleShield> ShieldClass;
var repnotify byte ShieldStatus;
var byte OldVisStatus;
var	byte OldFlashStatus;

var(Collision) CylinderComponent TurretCollision[4];

var repnotify 	byte TurretStatus;
var				byte OldTurretDeathStatus, OldTurretVisStatus;

var 			int  LFTurretHealth;
var 			int  RFTurretHealth;
var 			int  LRTurretHealth;
var 			int  RRTurretHealth;

var				int	 MaxTurretHealth;

var SoundCue BigBeamFireSound;

var()			float MaxHitCheckDist;

var UTSkelControl_TurretConstrained	CachedTurrets[4];

var name MainTurretPivot, DriverTurretPivot;

var(test) int StingerTurretTurnRate;

var SoundCue TurretExplosionSound;
var SoundCue TurretActivate;
var SoundCue TurretDeactivate;

/** PRI of player in 2nd passenger turret */
var UTPlayerReplicationInfo PassengerPRITwo;

/** PRI of player in 2nd passenger turret */
var UTPlayerReplicationInfo PassengerPRIThree;

/** PRI of player in 2nd passenger turret */
var UTPlayerReplicationInfo PassengerPRIFour;

var vector ExtraPassengerTeamBeaconOffset[3];

/** particle effect played when a turret is destroyed */
var ParticleSystem TurretExplosionTemplate;

replication
{
	if (bNetDirty)
		LFTurretFlashLocation, RFTurretFlashLocation,
		LRTurretFlashLocation, RRTurretFlashLocation,
		LFTurretHealth, RFTurretHealth, LRTurretHealth, RRTurretHealth,
		ShieldStatus, TurretStatus, PassengerPRITwo, PassengerPRIThree, PassengerPRIFour;
	if (!IsSeatControllerReplicationViewer(1))
		LFTurretFlashCount, LFTurretWeaponRotation;
	if (!IsSeatControllerReplicationViewer(2))
		RFTurretFlashCount, RFTurretWeaponRotation;
	if (!IsSeatControllerReplicationViewer(3))
		LRTurretFlashCount, LRTurretWeaponRotation;
	if (!IsSeatControllerReplicationViewer(4))
		RRTurretFlashCount, RRTurretWeaponRotation;
}

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
	virtual void ApplyWeaponRotation(INT SeatIndex, FRotator NewRotation);
	virtual FVector GetSeatPivotPoint(INT SeatIndex);
}

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	// Add me to the global game objectives list

	if ( UTGameReplicationInfo(WorldInfo.GRI) != none )
	{
		UTGameReplicationInfo(WorldInfo.GRI).AddGameObjective(self);
	}
}

simulated function Destroyed()
{
	Super.Destroyed();

	// Remove me from the global game objectives list
	if ( UTGameReplicationInfo(WorldInfo.GRI) != none )
	{
		UTGameReplicationInfo(WorldInfo.GRI).RemoveGameObjective(self);
	}
}

/**
 * FindAutoExit() Tries to find exit position on either side of vehicle, in back, or in front
 * returns true if driver successfully exited.
 *
 * @param	ExitingDriver	The Pawn that is leaving the vehicle
 */
function bool FindAutoExit(Pawn ExitingDriver)
{
	local vector OutDir;

	// if in turret, try to get out beside the turret
	if ( (ExitingDriver != None) && (ExitingDriver != Driver) )
	{
		OutDir = ExitingDriver.Location - Location;
		OutDir.Z = 0;

		if ( TryExitPos(ExitingDriver, ExitingDriver.Location + 120*Normal(OutDir) - vect(0,0,120)) )
		{
			return true;
		}
	}
	return Super.FindAutoExit(ExitingDriver);
}

simulated function PlayerReplicationInfo GetSeatPRI(int SeatNum)
{
	if ( Role == ROLE_Authority )
	{
		return Seats[SeatNum].SeatPawn.PlayerReplicationInfo;
	}
	else
	{
		Switch(SeatNum)
		{
			case 0:
				return PlayerReplicationInfo;
			case 1:
				return PassengerPRI;
			case 2:
				return PassengerPRITwo;
			case 3:
				return PassengerPRIThree;
			case 4:
				return PassengerPRIFour;
		}
	}
	return PlayerReplicationInfo;
}

/**
 * Temp for now
 */
simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, LinearColor FinalColor)
{
	local float xl,yl;

	Super.RenderMapIcon(MP,Canvas, PlayerOwner,FinalColor);

	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(1);
	Canvas.StrLen("L",xl,yl);
	Canvas.SetPos(HudLocation.X - (xl/2), HudLocation.Y - (yl/2));
	Canvas.SetDrawColor(255,255,0,255);
	Canvas.DrawText("L");
}

simulated function UTVehicleShield SpawnShield(vector Offset, name SocketName)
{
	local UTVehicleShield NewShield;

	if (ShieldClass != None)
	{
		NewShield = Spawn(ShieldClass, self);
		StaticMeshComponent(NewShield.CollisionComponent).SetTranslation(Offset);
		StaticMeshComponent(NewShield.CollisionComponent).SetScale( 0.75 );
		NewShield.SetBase(self,, Mesh, SocketName);
		Mesh.AttachComponent(NewShield.ShieldEffectComponent, SocketName);
	}

	return NewShield;
}

/** PoweredUp()
returns true if pawn has game play advantages, as defined by specific game implementation
*/
function bool PoweredUp()
{
	return true;
}

simulated function RenderPassengerBeacons(PlayerController PC, Canvas Canvas, LinearColor TeamColor, Color TextColor, UTWeapon Weap)
{
	if ( PassengerPRI != None )
	{
		PostRenderPassengerBeacon(PC, Canvas, TeamColor, TextColor, Weap, PassengerPRI, PassengerTeamBeaconOffset);
	}
	if ( PassengerPRITwo != None )
	{
		PostRenderPassengerBeacon(PC, Canvas, TeamColor, TextColor, Weap, PassengerPRITwo, ExtraPassengerTeamBeaconOffset[0]);
	}
	if ( PassengerPRIThree != None )
	{
		PostRenderPassengerBeacon(PC, Canvas, TeamColor, TextColor, Weap, PassengerPRIThree, ExtraPassengerTeamBeaconOffset[1]);
	}
	if ( PassengerPRIFour != None )
	{
		PostRenderPassengerBeacon(PC, Canvas, TeamColor, TextColor, Weap, PassengerPRIFour, ExtraPassengerTeamBeaconOffset[2]);
	}
}


function SetSeatStoragePawn(int SeatIndex, Pawn PawnToSit)
{
	Super.SetSeatStoragePawn(SeatIndex, PawnToSit);

	if ( Role == ROLE_Authority )
	{
		if ( SeatIndex == 2 )
		{
			PassengerPRITwo = (PawnToSit == None) ? None : Seats[SeatIndex].SeatPawn.PlayerReplicationInfo;
		}
		else if ( SeatIndex == 3 )
		{
			PassengerPRIThree = (PawnToSit == None) ? None : Seats[SeatIndex].SeatPawn.PlayerReplicationInfo;
		}
		else if ( SeatIndex == 4 )
		{
			PassengerPRIFour = (PawnToSit == None) ? None : Seats[SeatIndex].SeatPawn.PlayerReplicationInfo;
		}
	}
}

simulated function WeaponRotationChanged(int SeatIndex)
{
	local rotator DriverRot, MainRot, WeapRot;

	if (SeatIndex == 0)
	{
		if (DeployedState != EDS_Deployed || !Weapon.IsFiring())
		{
			WeapRot = SeatWeaponRotation(SeatIndex,,true);

			if (DeployedState == EDS_Undeployed)
			{
				DriverRot = WeapRot;
				MainRot = Rotation;
			}
			else if (DeployedState == EDS_Deployed)
			{
				MainRot = WeapRot;
				DriverRot = Rotation;
			}
			else
			{
				MainRot = Rotation;
				DriverRot = Rotation;
			}

			CachedTurrets[0].DesiredBoneRotation = DriverRot;
			CachedTurrets[1].DesiredBoneRotation = DriverRot;
			CachedTurrets[2].DesiredBoneRotation = MainRot;
			CachedTurrets[3].DesiredBoneRotation = MainRot;
		}
		else if (DeployedState == EDS_Deployed)
		{
			CachedTurrets[0].DesiredBoneRotation = Rotation;
			CachedTurrets[1].DesiredBoneRotation = Rotation;
		}
	}
	else
	{
		Super.WeaponRotationchanged(SeatIndex);
	}
}


simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'ShieldStatus')
	{
		ShieldStatusChanged();
	}
	else if (VarName == 'TurretStatus')
	{
		TurretStatusChanged();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

function bool PassengerEnter(Pawn P, int SeatIndex)
{
	if ( !Super.PassengerEnter(P, SeatIndex) )
		return false;

	TurretStatus = TurretStatus | (1 << (SeatIndex-1));
	TurretStatusChanged();
	return true;
}

function PassengerLeave(int SeatIndex)
{
	Super.PassengerLeave(SeatIndex);
	TurretStatus = TurretStatus & ( 0xFF ^ (1<<(SeatIndex-1)) );
	TurretStatusChanged();
}

function bool CanDeploy()
{
	local int i;

	// Check current speed
	if (VSize(Velocity) > MaxDeploySpeed)
	{
		ReceiveLocalizedMessage(class'UTSPMAMessage',0);
		return false;
	}
	else if (IsFiring())
	{
		return false;
	}
	else
	{
		// Make sure all 4 wheels are on the ground if required
		if (bRequireAllWheelsOnGround)
		{
			for (i=0;i<Wheels.Length-1;i++) // -1 because the very last wheel is a 'fake' wheel.
			{
				if ( !Wheels[i].bWheelOnGround )
				{
					ReceiveLocalizedMessage(class'UTSPMAMessage',3);
					return false;
				}
			}
		}
		return true;
	}
}

simulated function PlayTurretAnim(int ShieldIndex, string BaseName)
{
	local string Prefix;
	local AnimNodeSequence Player;
	local int SeatIndex;

	SeatIndex = ShieldIndex + 1;

	switch	(SeatIndex)
	{
		case 1 : PreFix = "Lt_Front_"; break;
		case 2 : PreFix = "Rt_Front_"; break;
		case 3 : PreFix = "Lt_Rear_"; break;
		case 4 : PreFix = "Rt_Rear_"; break;
	}



	Player = AnimNodeSequence( Mesh.Animations.FindAnimNode( name(PreFix$"Player") ) );

	Player.SetAnim( name(Prefix$BaseName) );
	Player.PlayAnim();
}

simulated function TurretStatusChanged()
{
	local int i;
	local byte DeathStatus, VisStatus;
	local SkelControlSingleBone SkelControl;
	local vector ExpVect;

	DeathStatus = (TurretStatus & 0xF0) >> 4;
	VisStatus   = TurretStatus & 0x0F;

	for (i=0;i<4;i++)
	{
		if ( (VisStatus & (1<<i)) != (OldTurretVisStatus & (1<<i) ) )
		{
			if ( (VisStatus & (1<<i)) >0 )
			{
				PlayTurretAnim(i,"turret_deplying");
				PlaySound(TurretActivate, true);
			}
			else
			{
				PlaySound(TurretDeactivate, true);
				PlayTurretAnim(i,"turret_undeplyed");
			}
		}
	}

	for (i=0;i<4;i++)
	{
		if ( (DeathStatus & (1<<i)) != (OldTurretDeathStatus & (1<<i) ) )
		{
			if ( (DeathStatus & (1<<i)) > 0 )
			{
				switch (i)
				{
					case 0:
						SkelControl = SkelControlSingleBone(Mesh.FindSkelControl('LF_TurretScale'));
						break;
					case 1:
						SkelControl = SkelControlSingleBone(Mesh.FindSkelControl('RF_TurretScale'));
						break;
					case 2:
						SkelControl = SkelControlSingleBone(Mesh.FindSkelControl('LR_TurretScale'));
						break;
					case 3:
						SkelControl = SkelControlSingleBone(Mesh.FindSkelControl('RR_TurretScale'));
						break;
				}

				VehicleEvent(Name("Damage"$i$"Smoke") );

				VehicleEffects[i].EffectRef.SetFloatParameter('smokeamount',0.9);
				VehicleEffects[i].EffectRef.SetFloatParameter('fireamount',0.9);

				if (EffectIsRelevant(Location, false))
				{
					Mesh.GetSocketWorldLocationAndRotation(VehicleEffects[i].EffectSocket, ExpVect);
					WorldInfo.MyEmitterPool.SpawnEmitter(TurretExplosionTemplate, ExpVect);
				}

				// Spawn Smoke and explosion effect

				if (SkelControl != none && SkelControl.BoneScale > 0)
				{
					SkelControl.BoneScale = 0;
				}

				// Play the Explosion Sound

				PlaySound(TurretExplosionSound, true);

			}
		}
	}

	OldTurretDeathStatus = DeathStatus;
	OldTurretVisStatus = VisStatus;
}

simulated function bool OverrideBeginFire(byte FireModeNum)
{
	if (FireModeNum == 1)
	{
		if (Role == ROLE_Authority)
		{
			ServerToggleDeploy();
		}
		return true;
	}
	else
	{
		return false;
	}
}

simulated function VehicleWeaponFired( bool bViaReplication, vector HitLocation, int SeatIndex )
{
	if (SeatIndex == 0 && DeployedState == EDS_Deployed )
	{
		if (BigBeamEmitter == None)
		{
			BigBeamEmitter = new(Outer) class'UTParticleSystemComponent';
			BigBeamEmitter.SetAbsolute(false, true, false);
			BigBeamEmitter.SetTemplate(BigBeamTemplate);
			Mesh.AttachComponentToSocket(BigBeamEmitter, BigBeamSocket);
		}
		BigBeamEmitter.SetRotation(rotator(HitLocation - BigBeamEmitter.GetPosition()));
		BigBeamEmitter.ActivateSystem();
		BigBeamEmitter.SetVectorParameter(BigBeamEndpointVarName, HitLocation);
		PlaySound(BigBeamFireSound);
		CauseMuzzleFlashLight(0);
	}
	else
	{
		Super.VehicleWeaponFired(bViaReplication, HitLocation, SeatIndex);
	}

}

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local ParticleSystemComponent Beam;

	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	// Handle Beam Effects for the shock beam
	if (!IsZero(HitLocation))
	{
		Beam = WorldInfo.MyEmitterPool.SpawnEmitter(BeamTemplate, GetEffectLocation(SeatIndex));
		Beam.SetVectorParameter(BeamEndpointVarName, HitLocation);
	}
}

function FlashShield(int SeatIndex)
{
	SeatIndex += 4;
	ShieldStatus = ShieldStatus ^ (1 << SeatIndex);
	ShieldStatusChanged();
}

function SetShieldActive(int SeatIndex, bool bActive)
{
	local int ShieldIndex;

	ShieldIndex = SeatIndex - 1;
	if (bActive)
	{
		ShieldStatus = ShieldStatus | (1<<ShieldIndex);
	}
	else
	{
		ShieldStatus = ShieldStatus & ( 0xFF ^ (1<<ShieldIndex) );
	}
	ShieldStatusChanged();
}

simulated function ShieldStatusChanged()
{
	local byte FlashStatus;
	local byte VisStatus;
	local byte Mask;
	local int i;

	FlashStatus    = (ShieldStatus & 0xf0) >> 4;
	VisStatus      = (ShieldStatus & 0x0f);

	Mask = 0x01;
	for (i=0;i<4;i++)
	{
		if ( (FlashStatus & Mask) != (OldFlashStatus & Mask) )
		{
			TriggerShieldEffect(i);
		}

		if (Shield[i] != none && (VisStatus & Mask) != (OldVisStatus & Mask) )
		{
			Shield[i].SetActive( bool( VisStatus & Mask ) );
		}

		Mask = Mask << 1;
	}

	OldFlashStatus = FlashStatus;
	OldVisStatus = VisStatus;

}

simulated function TriggerShieldEffect(int SeatIndex)
{
	// TODO: The paladin sheild effect doesn't have a "hit shimmer" yet.
}


event int CheckActiveTurret(vector HitLocation, float MaxDist)
{
	local int i;
	local float Dist, MinDist;
	local int MinIndex;

	for (i=0;i<4;i++)
	{
		Dist = VSize(HitLocation - TurretCollision[i].GetPosition());
		if (i==0 || Dist < MinDist)
		{
			MinDist = Dist;
			MinIndex = i;
		}
	}

	if ( TurretAlive(MinIndex) && MaxDist <= 0.0 || MinDist < MaxDist)
	{
		return MinIndex;
	}

	return -1;
}

/** kills the turret if it is dead */
function CheckTurretDead(int TurretIndex, int CurHealth, Controller InstigatedBy, class<DamageType> DamageType, vector HitLocation)
{
	if (CurHealth <= 0)
	{
		Seats[TurretIndex+1].SeatPawn.Died(InstigatedBy, DamageType, HitLocation);
		TurretStatus = TurretStatus | (1 << (TurretIndex+4));
		TurretStatusChanged();
	}
}

function int TotalTurretHealth()
{
	return LFTurretHealth + RFTurretHealth + LRTurretHealth + RRTurretHealth;
}

function bool TurretAlive(int TurretIndex)
{
	local int THealth;
	switch (TurretIndex)
	{
		case 0:	THealth = LFTurretHealth; break;
		case 1:	THealth = RFTurretHealth; break;
		case 2:	THealth = LRTurretHealth; break;
		case 3:	THealth = RRTurretHealth; break;
	}

	return (THealth > 0);
}

function bool SeatAvailable(int SeatIndex)
{
	if ( SeatIndex<1 || TurretAlive(SeatIndex-1) )
	{
		return Super.SeatAvailable(SeatIndex);
	}

	return false;
}

function bool ChangeSeat(Controller ControllerToMove, int RequestedSeat)
{
	if (RequestedSeat < 1 || TurretAlive(RequestedSeat - 1))
	{
		return Super.ChangeSeat(ControllerToMove, RequestedSeat);
	}
	else
	{
		if ( PlayerController(ControllerToMove) != None )
		{
			PlayerController(ControllerToMove).ClientPlaySound(VehicleLockedSound);
		}
		return false;
	}
}

function bool TurretTakeDamage(int TurretIndex, int Damage, Controller InstigatedBy, class<DamageType> DamageType, vector HitLocation)
{
	switch (TurretIndex)
	{
		case 0 :
			LFTurretHealth = Max( (LFTurretHealth - Damage), 0);
			CheckTurretDead(0, LFTurretHealth, InstigatedBy, DamageType, HitLocation);
			break;
		case 1 :
			RFTurretHealth = Max( (RFTurretHealth - Damage), 0);
			CheckTurretDead(1, RFTurretHealth, InstigatedBy, DamageType, HitLocation);
			break;
		case 2 :
			LRTurretHealth = Max( (LRTurretHealth - Damage), 0);
			CheckTurretDead(2, LRTurretHealth, InstigatedBy, DamageType, HitLocation);
			break;
		case 3 :
			RRTurretHealth = Max( (RRTurretHealth - Damage), 0);
			CheckTurretDead(3, RRTurretHealth, InstigatedBy, DamageType, HitLocation);
			break;
	}

	return ( TotalTurretHealth() <=0);
}

function TryToFindShieldHit(vector EndTrace, vector StartTrace, out TraceHitInfo HitInfo)
{
	local vector ShieldHitLocation, ShieldHitNormal;
	local int i;

	// Look for collision against an active shield;

	for (i=0;i<4;i++)
	{
		if (Shield[i] != None)
		{
			TraceComponent(ShieldHitLocation, ShieldHitNormal, Shield[i].CollisionComponent, EndTrace, StartTrace,, HitInfo);
			if (HitInfo.HitComponent != none)
			{
				return;
			}
		}
	}
}

simulated event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local int ShieldHitIndex, TurretHitIndex, i, TurretDamage;
	local UTVWeap_LeviathanTurretBase VWeap;
	local vector TurretMomentum;

	if ( Role < ROLE_Authority )
		return;

	if (InstigatedBy == Controller)
	{
		Damage = 0;
	}

	if (HitInfo.HitComponent == None)	// Attempt to find it
	{
		TryToFindshieldHit(HitLocation, (HitLocation - 2000.f * Normal(Momentum)),  HitInfo);
	}

	// check to see if shield was hit
	ShieldHitIndex = -1;
	for ( i=0; i<4; i++ )
	{
		if ( TurretAlive(i) )
		{
			if (Shield[i] != None)
			{
				if ( HitInfo.HitComponent == Shield[i].CollisionComponent )
				{
					ShieldHitIndex = i;
					break;
				}
			}
		}
	}

	if ( ShieldHitIndex<0 )
	{
		// damage turret
		TurretHitIndex = CheckActiveTurret(HitLocation, MaxHitCheckDist);
		TurretDamage = Damage;
		TurretMomentum = Momentum;
		WorldInfo.Game.ReduceDamage(TurretDamage, self, instigatedBy, HitLocation, TurretMomentum, DamageType);
		AdjustDamage(TurretDamage, TurretMomentum, instigatedBy, HitLocation, DamageType, HitInfo);
		if (TurretHitIndex >= 0)
		{
			TurretTakeDamage(TurretHitIndex, TurretDamage, InstigatedBy, DamageType, HitLocation);
		}

		// damage main vehicle
		Super.TakeDamage(Damage, InstigatedBy, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
	}
	else if ( !WorldInfo.GRI.OnSameTeam(self, InstigatedBy) )
	{
		VWeap = UTVWeap_LeviathanTurretBase( Seats[ShieldHitIndex+1].Gun );

		if (VWeap != none)
		{
			VWeap.NotifyShieldHit(Damage);
			FlashShield( ShieldHitIndex );
		}
	}
}

simulated function PlaySpawnEffect()
{
	local PlayerController PC;

	super.PlaySpawnEffect();
	foreach WorldInfo.AllControllers(class'PlayerController', PC)
	{
		if(PC.GetTeamNum() == GetTeamNum())
		{
			PC.ReceiveLocalizedMessage( class'UTVehicleMessage', 3);
		}
	}
}
simulated function TakeRadiusDamage
(
	Controller			InstigatedBy,
	float				BaseDamage,
	float				DamageRadius,
	class<DamageType>	DamageType,
	float				Momentum,
	vector				HurtOrigin,
	bool				bFullDamage,
	Actor DamageCauser
)
{
	local int TurretIndex;
	local int Dist;
	local Float DmgPct;

	if ( Role < ROLE_Authority )
		return;

	TurretIndex = CheckActiveTurret(HurtOrigin, MaxHitCheckDist + DamageRadius);
	if ( TurretIndex >= 0 )
	{

		Dist = vSize( HurtOrigin - TurretCollision[TurretIndex].GetPosition() ) - MaxHitCheckDist;
		if ( Dist > 0 )
		{
			DmgPct = FMax(0,1 - Dist/DamageRadius);
		}
		else
		{
			DmgPct = 1.0;
		}

		TakeDamage(BaseDamage*DmgPct, InstigatedBy, HurtOrigin, Momentum * Normal( HurtOrigin - TurretCollision[TurretIndex].GetPosition() ), DamageType,, DamageCauser);
	}
	else
	{
		Super.TakeRadiusDamage(InstigatedBy, BaseDamage, DamageRadius, DamageType, Momentum, HurtOrigin, bFullDamage, DamageCauser);
	}

}


simulated function int GetHealth(int SeatIndex)
{
	switch (SeatIndex)
	{
		case 0 : return Health; break;
		case 1 : return LFTurretHealth; break;
		case 2 : return RFTurretHealth; break;
		case 3 : return LRTurretHealth; break;
		case 4 : return RRTurretHealth; break;
	}

	return 0;
}



/*
simulated function VehicleCalcCamera(float DeltaTime, int SeatIndex, out vector out_CamLoc, out rotator out_CamRot, out vector CamStart, optional bool bPivotOnly)
{
	if (SeatIndex >= 1 && !UTPawn(Seats[SeatIndex].SeatPawn.Driver).bFixedView)
	{
		Mesh.GetSocketWorldLocationAndRotation( Seats[SeatIndex].CameraTag, out_CamLoc);
		out_CamRot = Seats[SeatIndex].SeatPawn.GetViewRotation();
		out_CamLoc +=  Vector(out_CamRot) * Seats[SeatIndex].CameraOffset;
	CamStart = out_CamLoc;
	}
	else
	{
		Super.VehicleCalcCamera(Deltatime, SeatIndex, out_CamLoc, out_CamRot, CamStart, bPivotOnly);
	}
}
*/
simulated native function vector GetTargetLocation(optional Actor RequestedBy);

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
					const CollisionImpactData RigidCollisionData, int ContactIndex )
{
	local UTVehicle OtherVehicle;
	local Controller InstigatorController;
	local float Angle;
	local vector CollisionVelocity;

	// if we hit a vehicle, and our velocity is in its general direction, crush it
	OtherVehicle = UTVehicle(OtherComponent.Owner);
	if (OtherVehicle != None)
	{
		CollisionVelocity = Mesh.GetRootBodyInstance().PreviousVelocity;
		Angle = Normal(CollisionVelocity) dot Normal(OtherVehicle.Location - Location);
		if (Angle > 0.0)
		{
			if (Controller != None)
			{
				InstigatorController = Controller;
			}
			else if (Instigator != None)
			{
				InstigatorController = Instigator.Controller;
			}

			OtherVehicle.TakeDamage(VSize(CollisionVelocity) * Angle, InstigatorController, RigidCollisionData.ContactInfos[0].ContactPosition, CollisionVelocity * Mass, class'UTDmgType_VehicleCollision',, self);
		}
	}

	Super.RigidBodyCollision(HitComponent, OtherComponent, RigidCollisionData, ContactIndex);
}

state Deployed
{
	function CheckStability()
	{
		local int i;
		local vector WheelLoc, MeshDir;

		for (i = 0; i < Wheels.Length - 1; i++) // -1 because the very last wheel is a 'fake' wheel.
		{
			MeshDir.X = Mesh.LocalToWorld.XPlane.X;
			MeshDir.Y = Mesh.LocalToWorld.XPlane.Y;
			MeshDir.Z = Mesh.LocalToWorld.XPlane.Z;
			WheelLoc = Mesh.GetPosition() + (Wheels[i].WheelPosition >> rotator(MeshDir));
			if (FastTrace(WheelLoc - vect(0,0,1) * (Wheels[i].WheelRadius + Wheels[i].SuspensionTravel), WheelLoc))
			{
				SetPhysics(PHYS_RigidBody);
				GotoState('UnDeploying');
				return;
			}
		}
	}
}

simulated function SetVehicleDeployed()
{
	Super.SetVehicleDeployed();
	CachedTurrets[2].bFixedWhenFiring = true;
	CachedTurrets[3].bFixedWhenFiring = true;
}

simulated function SetVehicleUndeployed()
{
	Super.SetVehicleUndeployed();
	CachedTurrets[2].bFixedWhenFiring = false;
	CachedTurrets[3].bFixedWhenFiring = false;
}
defaultproperties
{
	Health=6500
	StolenAnnouncementIndex=5

	COMOffset=(x=-20.0,y=0.0,z=-200.0)
	UprightLiftStrength=280.0
	UprightTime=1.25
	UprightTorqueStrength=500.0
	bCanFlip=false
	GroundSpeed=600
	AirSpeed=850
	ObjectiveGetOutDist=2000.0
	MaxDesireability=2.0
	bSeparateTurretFocus=true
	bUseLookSteer=true

	Begin Object Name=MyLightEnvironment
		NumVolumeVisibilitySamples=4
	End Object

	Begin Object Name=SVehicleMesh
		RBCollideWithChannels=(Default=TRUE,GameplayPhysics=TRUE,EffectPhysics=TRUE,Vehicle=TRUE,Untitled1=TRUE)
	End Object

	Begin Object Class=UTVehicleSimCar Name=SimObject
		WheelSuspensionStiffness=50.0
		WheelSuspensionDamping=75.0
		WheelSuspensionBias=0.7

		ChassisTorqueScale=0.2
		LSDFactor=1.0
		WheelInertia=1.0

		MaxSteerAngleCurve=(Points=((InVal=0,OutVal=30.0),(InVal=1500.0,OutVal=20.0)))
		SteerSpeed=100

		MaxBrakeTorque=8.0
		StopThreshold=500

		TorqueVSpeedCurve=(Points=((InVal=-600.0,OutVal=0.0),(InVal=-200.0,OutVal=10.0),(InVal=0.0,OutVal=16.0),(InVal=540.0,OutVal=10.0),(InVal=650.0,OutVal=0.0)))
		EngineRPMCurve=(Points=((InVal=-500.0,OutVal=2500.0),(InVal=0.0,OutVal=500.0),(InVal=599.0,OutVal=5000.0),(InVal=600.0,OutVal=3000.0),(InVal=949.0,OutVal=5000.0),(InVal=950.0,OutVal=3000.0),(InVal=1100.0,OutVal=5000.0)))
		EngineBrakeFactor=0.02
		FrontalCollisionGripFactor=0.18
	End Object
	SimObj=SimObject
	Components.Add(SimObject)


	CameraScaleMin=0.75
	CameraScaleMax=1.25

	SpawnRadius=425.0

	BaseEyeheight=0
	Eyeheight=0
	bLightArmor=false

	LFTurretHealth=1000
	RFTurretHealth=1000
	LRTurretHealth=1000
	RRTurretHealth=1000
	MaxTurretHealth=1000

	MaxHitCheckDist=140

	RespawnTime=120.0
	InitialSpawnDelay=+30.0
	bKeyVehicle=true
	StingerTurretTurnRate=+8192

	bStickDeflectionThrottle=true
	MaxDeploySpeed=300

	bEnteringUnlocks=false
	bAlwaysRelevant=true

	HUDExtent=500.0
}
