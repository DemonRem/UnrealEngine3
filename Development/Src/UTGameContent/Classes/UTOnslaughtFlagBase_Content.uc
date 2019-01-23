/**
 * UTOnslaughtFlagBase.
 *
 * Onslaught levels with UTOnslaughtBunkers need to have a UTOnslaughtFlagBase placed near each powercore.
 * they may also have additional flag bases placed near nodes that the flag will return to instead if it is closer
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtFlagBase_Content extends UTOnslaughtFlagBase;

defaultproperties
{
	bStatic=false
	bStasis=false
	bHidden=false
	bAlwaysRelevant=true
	NetUpdateFrequency=1

	Components.Remove(Sprite)
	Components.Remove(Sprite2)
	GoodSprite=None
	BadSprite=None
	Begin Object Name=CollisionCylinder
		CollisionRadius=+0048.000000
		CollisionHeight=+0060.000000
		CollideActors=true
		BlockActors=false
		BlockZeroExtent=false
		BlockNonZeroExtent=false
	End Object

	Begin Object Class=ParticleSystemComponent Name=PSC
		Translation=(Z=-60)
	End Object
	Components.Add(PSC);
	BallEffect=PSC

	Begin Object Class=SkeletalMeshComponent Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'Pickups.PowerCellDispenser.Mesh.SK_Pickups_Orb_Dispenser-Optimized'
		AnimSets[0]=AnimSet'Pickups.PowerCellDispenser.Anims.Anim_Orb_Dispenser'
		AnimTreeTemplate=Pickups.PowerCellDispenser.Anims.AT_Pickups_PowerCellDispenser
		CollideActors=false
		BlockActors=false
		CastShadow=true
		bCastDynamicShadow=false
		Translation=(X=0.0,Y=0.0,Z=-64.0)
		bUseAsOccluder=FALSE
	End Object
 	Components.Add(SkeletalMeshComponent0)
 	Mesh=SkeletalMeshComponent0

	// @todo fixmesteve - replace these with approximate per poly collision solution for skeletal meshes when available
	Begin Object Class=CylinderComponent Name=FrameCollision
		CollisionRadius=+0040.000000
		CollisionHeight=+0060.000000
		Translation=(X=100)
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
	End Object
	Components.Add(FrameCollision)

	Begin Object Class=CylinderComponent Name=BaseCollision
		CollisionRadius=+0075.000000
		CollisionHeight=+006.000000
		Translation=(X=0,Z=-54)
		BlockNonZeroExtent=true
		BlockZeroExtent=true
		BlockActors=true
		CollideActors=true
	End Object
	Components.Add(BaseCollision)

	TeamEmitters(0)=ParticleSystem'Pickups.PowerCellDispenser.Effects.P_CellDispenser_Idle_Red'
	TeamEmitters(1)=ParticleSystem'Pickups.PowerCellDispenser.Effects.P_CellDispenser_Idle_Blue'
	BaseMaterials(0)=Pickups.PowerCellDispenser.Materials.M_Pickups_Orb_Dispenser_Red
	BaseMaterials(1)=Pickups.PowerCellDispenser.Materials.M_Pickups_Orb_Dispenser_Blue
	BallMaterials(0)=Pickups.PowerCellDispenser.Materials.M_Pickups_Orb_Red
	BallMaterials(1)=Pickups.PowerCellDispenser.Materials.M_Pickups_Orb_Blue

	CreateSound=SoundCue'A_Gameplay.ONS.A_Gameplay_ONS_OrbCreated'

	bEnabled=true
	FlagClass=class'UTOnslaughtFlag_Content'
}
