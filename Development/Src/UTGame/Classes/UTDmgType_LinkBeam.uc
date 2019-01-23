/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_LinkBeam extends UTDamageType
	abstract;

var ParticleSystem PS_AttachToGib;

static function DoCustomDamageEffects(UTPawn ThePawn, class<UTDamageType> TheDamageType, const out TraceHitInfo HitInfo, vector HitLocation)
{
	CreateDeathSkeleton( ThePawn, TheDamageType, HitInfo, HitLocation );
	CreateDeathGoreChunks( ThePawn, TheDamageType, HitInfo, HitLocation );
}

/** allows special effects when gibs are spawned via DoCustomDamageEffects() instead of the normal way */
simulated static function SpawnExtraGibEffects(UTGib TheGib)
{
	if (!TheGib.WorldInfo.bDropDetail && FRand() < 0.70f)
	{
		TheGib.PSC_GibEffect = new(TheGib) class'UTParticleSystemComponent';
		TheGib.PSC_GibEffect.SetTemplate(default.PS_AttachToGib);
		TheGib.AttachComponent(TheGib.PSC_GibEffect);
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_LinkGun'
	DamageWeaponFireMode=1

	DamageBodyMatColor=(R=100,G=100,B=100)
	DamageOverlayTime=0.5
	DeathOverlayTime=1.0

	bCausesBlood=false
	bLeaveBodyEffect=true
	bUseDamageBasedDeathEffects=true
	VehicleDamageScaling=0.9
	VehicleMomentumScaling=0.1

	KDamageImpulse=100

	PS_AttachToGib=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Death_Gib_Effect'
	DamageCameraAnim=CameraAnim'Camera_FX.LinkGun.C_WP_Link_Beam_Hit'
}
