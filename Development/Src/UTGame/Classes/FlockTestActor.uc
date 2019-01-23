/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class FlockTestActor extends Actor
	native
	placeable;

var()	FlockTest_Spawner	Spawner;



var		NavigationPoint	TargetNav;

var() enum EAgentMoveState
{
	EAMS_Move,
	EAMS_Idle
} AgentState;

var		float	NextChangeTargetTime;
var		float	EndActionTime;
var		float	NextActionTime;
var		float	VelDamping;

var		rotator		ToTargetRot;
var		bool	bRotateToTargetRot;
var		bool	bHadNearbyTarget;

var()	SkeletalMeshComponent			SkeletalMeshComponent;
var()	SkeletalMeshComponent			AttachmentComponent;

var		AnimNodeBlend					SpeedBlendNode;
var		AnimNodeBlend					ActionBlendNode;
var		AnimNodeSequence				ActionSeqNode;
var		AnimNodeSequence				WalkSeqNode;
var		AnimNodeSequence				RunSeqNode;

var() const editconst LightEnvironmentComponent LightEnvironment;

cpptext
{
	virtual void performPhysics(FLOAT DeltaTime);
	void SetAgentMoveState(BYTE NewState);
	void DoAction(UBOOL bAtTarget, const FVector& TargetLoc);
}

defaultproperties
{
	AgentState=EAMS_Move
	bDestroyInPainVolume=true

	Begin Object Class=DynamicLightEnvironmentComponent Name=MyLightEnvironment
		bEnabled=False
	End Object
	Components.Add(MyLightEnvironment)
	LightEnvironment=MyLightEnvironment

	Begin Object Class=SkeletalMeshComponent Name=SkeletalMeshComponent0
		CollideActors=true
		BlockActors=false
		BlockZeroExtent=true
		BlockNonZeroExtent=false
		BlockRigidBody=false
		LightEnvironment=MyLightEnvironment
		RBChannel=RBCC_GameplayPhysics
		bCastDynamicShadow=false
		RBCollideWithChannels=(Default=TRUE,GameplayPhysics=TRUE,EffectPhysics=TRUE)
	End Object
	CollisionComponent=SkeletalMeshComponent0
	SkeletalMeshComponent=SkeletalMeshComponent0
	Components.Add(SkeletalMeshComponent0)

	Begin Object Class=SkeletalMeshComponent Name=AttachmentComponent0
		CollideActors=false
		bCastDynamicShadow=false
	End Object
	AttachmentComponent=AttachmentComponent0

	Physics=PHYS_Flying
	bStatic=false
	bCollideActors=true
	bBlockActors=false
	bWorldGeometry=false
	bCollideWorld=false
	bProjTarget=true
	bUpdateSimulatedPosition=false
	bNoEncroachCheck=true

	RemoteRole=ROLE_None
	bNoDelete=false
}