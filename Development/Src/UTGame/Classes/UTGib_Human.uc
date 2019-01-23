/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTGib_Human extends UTGib
	abstract;


simulated function LeaveADecal( vector HitLoc, vector HitNorm )
{
	local DecalComponent DC;
	local DecalLifetimeDataAge LifetimePolicy;

	local Actor TraceActor;
	local vector out_HitLocation;
	local vector out_HitNormal;
	local vector TraceDest;
	local vector TraceStart;
	local vector TraceExtent;
	local TraceHitInfo HitInfo;
	local float RandRotation;

	// these should be randomized
	TraceStart = HitLoc + ( Vect(0,0,15));  
	TraceDest =  HitLoc - ( Vect(0,0,15));

	RandRotation = FRand() * 360;

	TraceActor = Trace( out_HitLocation, out_HitNormal, TraceDest, TraceStart, false, TraceExtent, HitInfo, TRACEFLAG_PhysicsVolumes );

	if( TraceActor != None )
	{
		LifetimePolicy = new(TraceActor.Outer) class'DecalLifetimeDataAge';
		LifetimePolicy.LifeSpan = 10.0f;

		DC = new(self) class'DecalComponent';
		DC.Width = 200;
		DC.Height = 200;
		DC.Thickness = 10;
		DC.bNoClip = FALSE;


		MIC_Decal = new(outer) class'MaterialInstanceTimeVarying';
		// T_FX.DecalMaterials.M_FX_BloodDecal_Large01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Large02
		// T_FX.DecalMaterials.M_FX_BloodDecal_Medium01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Medium02
		// T_FX.DecalMaterials.M_FX_BloodDecal_Small01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Small02

		//MIC_Decal.SetParent( MaterialInstanceConstant'T_FX.DecalMaterials.M_FX_BloodDecal_Small01' );
		MIC_Decal.SetParent( MaterialInstanceTimeVarying'CH_Gibs.Decals.BloodSplatter' );

		DC.DecalMaterial = MIC_Decal;

		TraceActor.AddDecal( DC.DecalMaterial, out_HitLocation, rotator(-out_HitNormal), RandRotation, DC.Width, DC.Height, DC.Thickness, DC.bNoClip, HitInfo.HitComponent, LifetimePolicy, TRUE, FALSE, HitInfo.BoneName, HitInfo.Item, HitInfo.LevelIndex );

		//SetTimer( 3.0f, FALSE, 'StartDissolving' );
		MaterialInstanceTimeVarying(MIC_Decal).SetScalarStartTime( 'DissolveAmount',  3.0f );
	}
}




defaultproperties
{
	HitSound=SoundCue'A_Gameplay.A_Gameplay_GibSmallCue'
}
