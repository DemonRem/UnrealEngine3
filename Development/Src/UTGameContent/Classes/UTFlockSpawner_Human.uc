/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTFlockSpawner_Human extends FlockTest_Spawner
	placeable;

defaultproperties
{
	FlockAnimTree=AnimTree'CH_AnimHuman_Tree.AT_CH_Flock_Human'
	FlockMesh=SkeletalMesh'CH_IronGuard.Mesh.SK_CH_IronGuard_MaleA'
	FlockAnimSets.Add(AnimSet'CH_AnimHuman.Anims.K_AnimHuman_BaseMale')

	WalkAnimName="crouch_fwd_rif"
	RunAnimName="run_fwd_rif"

	ActionAnimNames.Add("idle_ready_rif")

	TargetActionAnimNames.Add("fire_straight_rif")
}