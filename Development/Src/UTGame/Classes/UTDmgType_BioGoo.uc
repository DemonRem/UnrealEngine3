/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDmgType_BioGoo extends UTDamageType
	abstract;

static function DoCustomDamageEffects(UTPawn ThePawn, class<UTDamageType> TheDamageType, const out TraceHitInfo HitInfo, vector HitLocation)
{
	local int i, Num;
	local MaterialInstanceConstant OldMIC, NewMIC;

	if (ThePawn.CurrFamilyInfo != None && ThePawn.CurrFamilyInfo.default.BioDeathMICParent != None)
	{
		Num = ThePawn.Mesh.GetNumElements();
		for (i = 0; i < Num; i++)
		{
			OldMIC = MaterialInstanceConstant(ThePawn.Mesh.GetMaterial(i));
			if (OldMIC != None)
			{
				// duplicate the material (copying its parameter values)
				// then set the parent to the bio material
				NewMIC = new(ThePawn) OldMIC.Class(OldMic);
				NewMIC.SetParent(ThePawn.CurrFamilyInfo.default.BioDeathMICParent);
				ThePawn.Mesh.SetMaterial(i, NewMIC);
			}
		}
		ThePawn.BioBurnAway.ActivateSystem();
		ThePawn.bKilledByBio = true;
		ThePawn.Mesh.CastShadow = FALSE;
		ThePawn.Mesh.bCastDynamicShadow = FALSE;
	}
}

defaultproperties
{
	DamageWeaponClass=class'UTWeap_BioRifle'
	DamageWeaponFireMode=0

	DamageBodyMatColor=(R=0,G=50)
	DamageOverlayTime=0.2
	DeathOverlayTime=0.4
	bUseTearOffMomentum=false
	VehicleDamageScaling=0.9
	VehicleMomentumScaling=0.2

	bUseDamageBasedDeathEffects=true
	//bUseDeathCameraEffectVictim=true
	//DeathCameraEffectVictim=class'UTEmitCameraEffect_BioGoop'  // use for single shot big goop UTEmitCameraEffect_BioGoopDark

	DamageCameraAnim=CameraAnim'Camera_FX.BioRifle.C_WP_Bio_Hit'
}
