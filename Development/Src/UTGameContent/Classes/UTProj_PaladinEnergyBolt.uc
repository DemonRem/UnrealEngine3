/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTProj_PaladinEnergyBolt extends UTProjectile;

var ParticleSystem ProximityExplosionTemplate;
var float ProximityExplosionDamage;
var float ProximityExplosionRadius;
var float ProximityExplosionMomentum;
var SoundCue ProximityExplosionSound;

var repnotify bool bProximityExploded;

replication
{
	if (bNetInitial)
		bProximityExploded;
}

/** check if we hit our Instigator's shield and if so do the special proximity explosion */
simulated function bool CheckShieldHit(Actor HitActor)
{
	local UTVehicle_Paladin Paladin;
	local vector CompHitLocation, CompHitNormal, Dir;

	Paladin = UTVehicle_Paladin(Instigator);
	if (Paladin != None && Paladin.bShieldActive && Paladin.ShockShield != None && (HitActor == Paladin || HitActor == Paladin.ShockShield))
	{
		Dir = Normal(Velocity);
		if (TraceComponent(CompHitLocation, CompHitNormal, Paladin.ShockShield.CollisionComponent, Location + (Dir * 5000.f), Location - (Dir * 5000.f)))
		{
			ProximityExplode();
			return true;
		}
	}
	return false;
}

simulated function ProcessTouch(Actor Other, vector HitLocation, vector HitNormal)
{
	if (!CheckShieldHit(Other))
	{
		Super.ProcessTouch(Other, HitLocation, HitNormal);
	}
}

simulated function HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	if (!CheckShieldHit(Wall))
	{
		Super.HitWall(HitNormal, Wall, WallComp);
	}
}

simulated function ProximityExplode()
{
	Instigator.HurtRadius(ProximityExplosionDamage, ProximityExplosionRadius, class'UTDmgType_PaladinProximityExplosion', ProximityExplosionMomentum, Instigator.Location, Instigator);

	// play explosion effects
	PlaySound(ProximityExplosionSound, true);
	if (WorldInfo.NetMode != NM_DedicatedServer && ProximityExplosionTemplate != None && Instigator.EffectIsRelevant(Instigator.Location, false, MaxEffectDistance))
	{
		ProjExplosion = Instigator.Spawn(class'UTEmitter');
		if (ProjExplosion != None)
		{
			ProjExplosion.SetTemplate(ProximityExplosionTemplate, true);
			// FIXME: temp hack
			ProjExplosion.SetDrawScale(3.25);
		}
	}

	// we use a delay to make sure clients recieve the impact, since this can happen in the same tick as we are spawned
	if (WorldInfo.NetMode != NM_DedicatedServer && WorldInfo.NetMode != NM_ListenServer)
	{
		Destroy();
	}
	else
	{
		bProximityExploded = true;
		LifeSpan = 0.2;
		SetHidden(true);
		SetPhysics(PHYS_None);
		SetCollision(false);
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bProximityExploded')
	{
		ProximityExplode();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

defaultproperties
{
	Components.Remove(ProjectileMesh)

	MyDamageType=class'UTDmgType_PaladinEnergyBolt'
	Speed=9000
	MaxSpeed=9000
	Damage=200
	DamageRadius=450
	MomentumTransfer=200000
	ProjFlightTemplate=ParticleSystem'VH_Paladin.Effects.P_VH_Paladin_PrimaryProj'
	ProjExplosionTemplate=ParticleSystem'WP_ShockRifle.Particles.P_WP_ShockRifle_Explo'
	ExplosionSound=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_FireImpact'
	ExplosionLightClass=class'UTGame.UTShockComboExplosionLight'

	ProximityExplosionDamage=200
	ProximityExplosionRadius=900
	ProximityExplosionMomentum=150000
	ProximityExplosionTemplate=ParticleSystem'VH_Paladin.Particles.P_VH_Paladin_ProximityExplosion'
	ProximityExplosionSound=SoundCue'A_Vehicle_Paladin.SoundCues.A_Vehicle_Paladin_ComboExplosion'
}
