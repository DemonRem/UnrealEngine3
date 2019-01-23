/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAttachment_ImpactHammer extends UTWeaponAttachment;

var name IdleAnim;
var name ChargedAnim;

var MaterialInstanceConstant BloodMIC;

simulated function FireModeUpdated(byte FireModeNum, bool bViaReplication)
{
	if(FireModeNum < 2)
	{
		StartCharging();
	}
	else if(FireModeNum == 3) // we're being told we got a hit
	{
		AddBlood();
	}
	super.FireModeUpdated(FireModeNum, bViaReplication);
}

simulated function StartCharging()
{
	Play3pAnimation(ChargedAnim,1.0f,true);
}

simulated event ThirdPersonFireEffects(vector HitLocation)
{
	Super.ThirdPersonFireEffects(HitLocation);
	FireModeUpdated(2,false); // force the impact hammer into a neutral state
	Play3pAnimation(IdleAnim,1.0f,true);
}

simulated function Play3pAnimation(name Sequence, float fDesiredDuration, optional bool bLoop)
{
	// Check we have access to mesh and animations
	if (Mesh != None && Mesh.Animations != None)
	{
		// @todo - this should call GetWeaponAnimNodeSeq, move 'duration' code into AnimNodeSequence and use that.
		Mesh.PlayAnim(Sequence, fDesiredDuration, bLoop);
	}
}

simulated function SetSkin(Material NewMaterial)
{
	super.SetSkin(NewMaterial);
	BloodMIC = Mesh.CreateAndSetMaterialInstanceConstant(0);
}

simulated function PlayHitPawnEffects(vector HitLocation, pawn HitPawn)
{
	local Pawn HitUTPawn; // stores is there is an obvious utpawn to hit (rather than a vehicle)
	if (UTPawn(Owner).FiringMode == 0)
	{
		HitUTPawn = UTPawn(HitPawn);
		if ( HitUTPawn == None )
		{
			HitUTPawn = UTVehicle_Hoverboard(HitPawn);
		}
		if ( (HitUTPawn != None) && !WorldInfo.GRI.OnSameTeam(HitPawn, Instigator) )
		{
			AddBlood();
		}
	}
}
simulated function AddBlood()
{
	local float Damage1,Damage2,Damage3;
	if(BloodMic != none)
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
	}
}

defaultproperties
{
	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	// Weapon SkeletalMesh
	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_ImpactHammer.Mesh.SK_WP_Impact_3P'
		Animations=MeshSequenceA
		AnimSets(0)=AnimSet'WP_ImpactHammer.Anims.K_WP_Impact_3P'
		bForceRefPose=false
	End Object

	IdleAnim=WeaponIdle
	ChargedAnim=WeaponChargedIdle

	ImpactEffects(0)=(MaterialType=Metal,Sound=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_FireImpactMetal_Cue',ParticleTemplate=none,DecalMaterial=none,DecalWidth=16.0,DecalHeight=16.0)
	ImpactEffects(1)=(MaterialType=Stone,Sound=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_FireImpactMetal_Cue',ParticleTemplate=none,DecalMaterial=none,DecalWidth=16.0,DecalHeight=16.0)
	ImpactEffects(2)=(MaterialType=Wood,Sound=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_FireImpactMetal_Cue',ParticleTemplate=none,DecalMaterial=none,DecalWidth=16.0,DecalHeight=16.0)
	ImpactEffects(3)=(MaterialType=Metal,Sound=SoundCue'A_Weapon_ImpactHammer.ImpactHammer.A_Weapon_ImpactHammer_FireImpactMetal_Cue',ParticleTemplate=none,DecalMaterial=none,DecalWidth=16.0,DecalHeight=16.0)

	WeaponClass=class'UTWeap_ImpactHammer'
	MuzzleFlashPSCTemplate=ParticleSystem'WP_ImpactHammer.Particles.P_WP_ImpactHammer_Primary_Hit'
	MuzzleFlashAltPSCTemplate=ParticleSystem'WP_ImpactHammer.Particles.P_WP_ImpactHammer_Secondary_Hit'

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashDuration=0.33;


}
