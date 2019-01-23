/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class FlockTest_Spawner extends Actor
	native
	abstract;

// Spawner stuff
var()	bool	bActive;
var()	float	SpawnRate;
var()	int		SpawnNum;
var()	float	Radius;

var		float	Remainder;
var		int		NumSpawned;

// Agent force stuff
var()	float	AwareRadius;


var()	float	ToCentroidStrength;

var()	float	AvoidOtherStrength;
var()	float	AvoidOtherRadius;

var()	float	MatchVelStrength;

var()	float	ToTargetStrength;
var()	float	ChangeTargetInterval;

var()	float	ToPathStrength;
var()	float	FollowPathStrength;

var()	vector	CrowdAcc;

var()	float	MinVelDamping;
var()	float	MaxVelDamping;

// Agent animation stuff
var(Action)		RawDistributionFloat	ActionDuration;
var(Action)		RawDistributionFloat	ActionInterval;
var(Action)		RawDistributionFloat	TargetActionInterval;
var(Action)		array<name>				ActionAnimNames;
var(Action)		array<name>				TargetActionAnimNames;
var(Action)		float					ActionBlendTime;
var(Action)		float					ReActionDelay;
var(Action)		float					RotateToTargetSpeed;

var()	float	SpeedBlendStart;
var()	float	SpeedBlendEnd;

var()	float	WalkVelThresh;

var()	float	AnimVelRate;
var()	float	MaxYawRate;

var()	array<SkeletalMesh>	FlockMeshes;
var()	array<AnimSet>	FlockAnimSets;
var()	name			WalkAnimName;
var()	name			RunAnimName;
var()	AnimTree		FlockAnimTree;

var(AttachMesh)		array<SkeletalMesh> AttachmentMeshes;
var(AttachMesh)		name				AttachmentSocket;

cpptext
{
	void UpdateAgent(class AFlockTestActor* Agent, FLOAT DeltaTime);
}

event Tick(float DeltaSeconds)
{
	local rotator	Rot;
	local vector	SpawnPos;
	local FlockTestActor	Agent;
	local SkeletalMesh	AttachMesh;

	if(NumSpawned >= SpawnNum || !bActive)
	{
		return;
	}

	Remainder += (DeltaSeconds * SpawnRate);

	while(Remainder > 1.0 && NumSpawned < SpawnNum)
	{
		Rot = RotRand(false);
		Rot.Pitch = 0;

		SpawnPos = Location + ((vect(1,0,0) * FRand() * Radius) >> Rot);
		Agent = Spawn( class'UTGame.FlockTestActor',,,SpawnPos);

		Agent.SkeletalMeshComponent.AnimSets = FlockAnimSets;
		Agent.SkeletalMeshComponent.SetSkeletalMesh( FlockMeshes[Rand(FlockMeshes.length)] );
		Agent.SkeletalMeshComponent.SetAnimTreeTemplate(FlockAnimTree);

		AttachMesh = AttachmentMeshes[Rand(AttachmentMeshes.length)];
		if(AttachMesh != None)
		{
			Agent.AttachmentComponent.SetSkeletalMesh(AttachMesh);
			Agent.SkeletalMeshComponent.AttachComponentToSocket(Agent.AttachmentComponent, AttachmentSocket);
		}

		// Cache pointers to anim nodes
		Agent.SpeedBlendNode = AnimNodeBlend(Agent.SkeletalMeshComponent.Animations.FindAnimNode('SpeedBlendNode'));
		Agent.ActionBlendNode = AnimNodeBlend(Agent.SkeletalMeshComponent.Animations.FindAnimNode('ActionBlendNode'));
		Agent.ActionSeqNode = AnimNodeSequence(Agent.SkeletalMeshComponent.Animations.FindAnimNode('ActionSeqNode'));
		Agent.WalkSeqNode = AnimNodeSequence(Agent.SkeletalMeshComponent.Animations.FindAnimNode('WalkSeqNode'));
		Agent.RunSeqNode = AnimNodeSequence(Agent.SkeletalMeshComponent.Animations.FindAnimNode('RunSeqNode'));

		Agent.WalkSeqNode.SetAnim(WalkAnimName);
		Agent.RunSeqNode.SetAnim(RunAnimName);

		Agent.VelDamping = MinVelDamping + (FRand() * (MaxVelDamping - MinVelDamping));
		Agent.Spawner = self;

		Remainder -= 1.0;
		NumSpawned++;
	}
}


simulated function OnToggle(SeqAct_Toggle action)
{
	if (!bStatic)
	{
		if (action.InputLinks[0].bHasImpulse)
		{
			// turn on
			bActive = true;
		}
		else if (action.InputLinks[1].bHasImpulse)
		{
			// turn off
			bActive = false;
		}
		else if (action.InputLinks[2].bHasImpulse)
		{
			// toggle
			bActive = !bActive;
		}
	}
}

defaultproperties
{
	Radius=200
	SpawnRate=10
	SpawnNum=100
	bActive=true

	ToCentroidStrength=0.0

	AvoidOtherStrength=1500.0
	AvoidOtherRadius=100.0

	MatchVelStrength=0.6

	AwareRadius=200.0

	ChangeTargetInterval=30

	FollowPathStrength=30.0
	ToPathStrength=200.0
	ToTargetStrength=150.0

	Begin Object Class=DistributionFloatUniform Name=DistributionActionDuration
		Min=0.8
		Max=1.2
	End Object
	ActionDuration=(Distribution=DistributionActionDuration)

	Begin Object Class=DistributionFloatUniform Name=DistributionTargetActionInterval
		Min=1.0
		Max=5.0
	End Object
	TargetActionInterval=(Distribution=DistributionTargetActionInterval)

	Begin Object Class=DistributionFloatUniform Name=DistributionActionInterval
		Min=10.0
		Max=20.0
	End Object
	ActionInterval=(Distribution=DistributionActionInterval)

	MinVelDamping=0.001
	MaxVelDamping=0.003

	ActionBlendTime=0.1
	ReActionDelay=1.0
	RotateToTargetSpeed=0.1

	WalkVelThresh=10.0

	SpeedBlendStart=150.0
	SpeedBlendEnd=180.0

	AnimVelRate=0.007
	MaxYawRate=40000.0

	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EnvyEditorResources.BlueDefense'
		HiddenGame=true
		HiddenEditor=false
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)
}