/**
 * DemoPawn
 * Demo pawn demonstrating animating character.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class DemoPawn extends GamePawn
	config(Game);

var(Mesh) SkeletalMeshComponent 	HeadMesh;

simulated event PostBeginPlay()
{
	super.PostBeginPlay();

	if ( HeadMesh != None )
		HeadMesh.SetShadowParent(Mesh);
	SetBaseEyeheight();
}

simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	return false;
}

simulated function SetBaseEyeheight()
{
	local string MapName;

	// hack for old larger scale demo levels (PhysicsExpanded, EffectsDemo, CityStreet
	MapName = WorldInfo.GetLocalURL();

	// strip the UEDPIE_ from the filename, if it exists (meaning this is a Play in Editor game)
	if (Left(MapName, 7) ~= "UEDPIE_")
	{
		MapName = Right(MapName, Len(MapName) - 7);
	}
	if ( !bIsCrouched && ((Left(MapName,15) ~= "PhysicsExpanded") || (Left(MapName,11) ~= "EffectsDemo") || (Left(MapName,10) ~= "CityStreet"))  )
	{
		BaseEyeheight = 96;
	}
	else
	{
		super.SetBaseEyeheight();
	}
}


defaultproperties
{
	Components.Remove(Sprite)

	BaseEyeHeight=38.0
	EyeHeight=38.0

	Begin Object Class=SkeletalMeshComponent Name=DemoPawnSkeletalMeshComponent
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		bOwnerNoSee=true

		// these should match UTPawn's DefaultMesh and UTFamilyInfo_Ironguard_Male defaults
		SkeletalMesh=SkeletalMesh'CH_IronGuard_Male.Mesh.SK_CH_IronGuard_MaleA'
		AnimTreeTemplate=AnimTree'CH_AnimHuman_Tree.AT_CH_Human'
		PhysicsAsset=PhysicsAsset'CH_AnimHuman.Mesh.SK_CH_BaseMale_Physics'
		AnimSets(0)=AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale'
		BlockRigidBody=true
		CastShadow=true
		Translation=(X=0.0,Y=0.0,Z=8.0)
	End Object
	Mesh=DemoPawnSkeletalMeshComponent
	Components.Add(DemoPawnSkeletalMeshComponent)

// 	Begin Object class=SkeletalMeshComponent name=smHeadMesh
// 		bOwnerNoSee=true
// 		AlwaysLoadOnClient=true
// 		AlwaysLoadOnServer=true
// 		CastShadow=true
// 		ParentAnimComponent=DemoPawnSkeletalMeshComponent
// 		SkeletalMesh=SkeletalMesh'CH_Immortals.Mesh.SK_CH_Imm_Malcolm_Head'
// 	End Object
// 	HeadMesh=smHeadMesh
// 	Components.Add(smHeadMesh)

	Begin Object Name=CollisionCylinder
		CollisionRadius=+0021.000000
		CollisionHeight=+0044.000000
	End Object
	CylinderComponent=CollisionCylinder

	bStasis=false
}
