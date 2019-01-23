/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTWalkerBody_Scavenger extends UTWalkerBody
	native(Vehicle)
	abstract;

var	RB_Handle			PawnGrabber[3];
var bool				bIsInBallMode;


var bool				bStartedBallMode;
var int					SpinRate;

/** Names of the anim nodes for playing the step anim for a leg.  Used to fill in FootStepAnimNode array. */
var protected const Name	BallAnimNodeName[NUM_WALKER_LEGS];
/** Refs to AnimNodes used for playing step animations */
var protected AnimNode		BallAnimNode[NUM_WALKER_LEGS];

var name RetractionBlend;
var name SphereCenterName;
cpptext
{
	virtual void AnimateLegs(FLOAT DeltaSeconds, UBOOL bIsFalling);
}

function PlayDying()
{
	if(!bIsInBallMode)
	{
		UTVehicle_Scavenger(WalkerVehicle).bIsInBallmode = true;
		UTVehicle_Scavenger(WalkerVehicle).BallModeTransition();
	}
}

function PostBeginPlay()
{
	local int Idx;

	Super.PostBeginPlay();

	for (Idx=0; Idx<NUM_WALKER_LEGS; ++Idx)
	{
		// cache refs to footstep anims
		BallAnimNode[Idx] = SkeletalMeshComponent.FindAnimNode(FootStepAnimNodeName[Idx]);
	}
}

simulated event Cloak(bool bIsEnabled)
{
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		if(bIsEnabled)
		{
			SetHidden(true); //SkeletalMeshComponent.setHidden(true);
		}
		else
		{
			SetHidden(false); //SkeletalMeshComponent.setHidden(false);
		}
	}
}

defaultproperties
{
	bHasCrouchMode=FALSE

	MinStepDist=100.f
	MaxLegReach=475.f				
	LegSpreadFactor=0.6f

	CustomGravityScale=0.f
	FootEmbedDistance=16.0

	LandedFootDistSq=2500.0f

	FootStepAnimNodeName[0]="Leg0 Step"
	FootStepAnimNodeName[1]="Leg1 Step"
	FootStepAnimNodeName[2]="Leg2 Step"

	FootPosVelAdjScale[WalkerLeg_Rear]=0.3f
	FootPosVelAdjScale[WalkerLeg_FrontLeft]=0.35f
	FootPosVelAdjScale[WalkerLeg_FrontRight]=0.35f

	FootStepStartLift=450.f
	FootStepEndLift=175.f

	StepStageTimes(0)=0.4f			// foot pickup and move forward
	StepStageTimes(1)=0.2f		// foot stab to ground at destination
	StepStageTimes(2)=0.7f			// wait for foot to reach dest before forcibly ending step

	BaseLegDirLocal[WalkerLeg_Rear]=(X=-1.f,Y=0.f,Z=0.f)
	BaseLegDirLocal[WalkerLeg_FrontLeft]=(X=0.5f,Y=-0.866025f,Z=0.f)
	BaseLegDirLocal[WalkerLeg_FrontRight]=(X=0.5f,Y=0.866025f,Z=0.f)
}
