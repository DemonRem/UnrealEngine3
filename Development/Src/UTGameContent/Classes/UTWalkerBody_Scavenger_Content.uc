/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTWalkerBody_Scavenger_Content extends UTWalkerBody_Scavenger;

/** The beams attaching the legs to the shield */
var ParticleSystemComponent LegAttachBeams[NUM_WALKER_LEGS];
/** Names of the top leg bones.  LegAttachBeams will terminate here. */
var protected const name				TopLegSocketName[NUM_WALKER_LEGS];
/** Name of beam endpoint parameter in the particle system */
var protected const name				LegAttachBeamEndPointParamName;
/** radius around the body in which the arm attachments will rotate */
var float ShieldRadius;

function Tick(float DeltaTime)
{
	local int Idx;
	local vector ShieldLocation; // location of the shield relative to the beam
	local vector SocketLoc;
	super.Tick(DeltaTime);

	// set leg attach beam endpoints 
	for (Idx=0; Idx<NUM_WALKER_LEGS; ++Idx)
	{
		SkeletalMeshComponent.GetSocketWorldLocationAndRotation(TopLegSocketName[Idx],SocketLoc);
		ShieldLocation = WalkerVehicle.Location - SocketLoc; // line from leg to center
		ShieldLocation -= Normal(ShieldLocation)*ShieldRadius; // subtract off from the center to shield radius
		ShieldLocation += SocketLoc;
		LegAttachBeams[Idx].SetVectorParameter(LegAttachBeamEndPointParamName, ShieldLocation);
	}
}

function PostBeginPlay()
{
	local int Idx;

	super.PostBeginPlay();
	// attach leg attach beam emitters to the leg
	for (Idx=0; Idx<NUM_WALKER_LEGS; ++Idx)
	{
		SkeletalMeshComponent.AttachComponentToSocket(LegAttachBeams[Idx], TopLegSocketName[Idx]);
	}
}
defaultproperties
{
	DrawScale=0.7

	// @fixme, scavenger-specific effects needed?
	FootStepEffects[0]=(MaterialType=Dirt,Sound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FootstepCue',ParticleTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_FootImpact_Default')
	FootStepEffects[1]=(MaterialType=Snow,Sound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FootstepCue',ParticleTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_FootImpact_Default')
	FootStepEffects[2]=(MaterialType=Water,Sound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FootstepCue',ParticleTemplate=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_FootImpact_Default')

	FootWaterEffect=ParticleSystem'Envy_Level_Effects_2.Vehicle_Water_Effects.P_DarkWalker_Water_Splash'

	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'VH_Scavenger.Mesh.SK_VH_Scavenger_Legs'
		PhysicsAsset=PhysicsAsset'VH_Scavenger.Mesh.SK_VH_Scavenger_Legs_Physics'
		AnimSets(0)=AnimSet'VH_Scavenger.Anim.K_VH_Scavenger_Legs'
		AnimTreeTemplate=AnimTree'VH_Scavenger.Anim.AT_VH_Scavenger_Legs'
		bUpdateJointsFromAnimation=TRUE
	End Object

	Begin Object Class=ParticleSystemComponent Name=LegAttachPSC_0
		Template=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_LegLink'
		bAutoActivate=TRUE
	End Object
	LegAttachBeams[0]=LegAttachPSC_0

	Begin Object Class=ParticleSystemComponent Name=LegAttachPSC_1
		Template=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_LegLink'
		bAutoActivate=TRUE
	End Object
	LegAttachBeams[1]=LegAttachPSC_1

	Begin Object Class=ParticleSystemComponent Name=LegAttachPSC_2
		Template=ParticleSystem'VH_Scavenger.Effects.P_VH_Scavenger_LegLink'
		bAutoActivate=TRUE
	End Object
	LegAttachBeams[2]=LegAttachPSC_2
	TopLegSocketName[0]=Leg1Socket
	TopLegSocketName[1]=Leg2Socket
	TopLegSocketName[2]=Leg3Socket
	LegAttachBeamEndPointParamName=ScavengerLegEnd
	ShieldRadius = 50.0f;
}
