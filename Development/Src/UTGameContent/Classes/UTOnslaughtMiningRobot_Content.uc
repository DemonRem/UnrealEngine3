/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtMiningRobot_Content extends UTOnslaughtMiningRobot
	notplaceable;


defaultproperties
{
	Physics=PHYS_Walking
	GroundSpeed=200.0
	ControllerClass=class'UTGame.UTOnslaughtMiningRobotController'
	ExplosionTemplate=ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketExplosion'
	ExplosionSound=SoundCue'A_Weapon.RocketLauncher.Cue.A_Weapon_RL_Impact_Cue'
	OreQuantity=50
	JumpZ=100.0
	bCanJump=false
	MaxStepHeight=26.0

	Components.Remove(Sprite)

	Begin Object Name=CollisionCylinder
		CollisionRadius=+0021.000000
		CollisionHeight=+0035.000000
	End Object

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent0
		StaticMesh=StaticMesh'GP_Conquest.SM.Mesh.S_GP_Conq_SM_Bot'
		CollideActors=false
		BlockActors=false
		CastShadow=true
		BlockRigidBody=false
		HiddenGame=false
		Translation=(X=0.0,Y=0.0,Z=0.0)
		Scale3D=(X=1.0,Y=1.0,Z=1.0)
		bUseAsOccluder=FALSE
	End Object
 	Components.Add(StaticMeshComponent0)

	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent1
		StaticMesh=StaticMesh'GP_Conquest.SM.Mesh.S_GP_Con_Crystal01'
		CollideActors=true
		BlockActors=true
		CastShadow=true
		HiddenGame=false
		Translation=(X=25.0,Y=0.0,Z=0.0)
		Scale3D=(X=0.1,Y=0.1,Z=0.1)
		bUseAsOccluder=FALSE
	End Object
	CarriedOre=StaticMeshComponent1
 	Components.Add(StaticMeshComponent1)

}
