/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTAttachment_Translocator extends UTWeaponAttachment;

defaultproperties
{
	// Weapon SkeletalMesh
	Begin Object Class=SkeletalMeshComponent Name=StaticMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_Translocator.Mesh.SK_WP_Translocator_3P'
		bOwnerNoSee=true
		bOnlyOwnerSee=false
		CollideActors=false
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CullDistance=4000
		bUseAsOccluder=FALSE
	End Object
    Mesh=StaticMeshComponent0
    WeaponClass=class'UTWeap_Translocator_Content'
}
