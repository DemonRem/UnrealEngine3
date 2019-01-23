/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTWalkerBody_DarkWalker extends UTWalkerBody;

/** Light attached to the energy ball. */
var() protected PointLightComponent EnergyBallLight;

/** Holds energy ball's material so we can modify parameters. */
var protected transient MaterialInstanceConstant EnergyBallMatInst;

/** Current percentage the energy ball is powered.  Range [0..1]. */
var private transient float CurrentEnergyBallPowerPct;
/** Goal energy ball power percentage (to interpolate towards).  Range [0..1]. */
var private transient float GoalEnergyBallPowerPct;

/** InterpSpeed (for FInterpTo) used for interpolating the energy ball power up and down. */
var() protected const float EnergyBallPowerInterpSpeed;

/** Color for energy ball light in powered-on state. */
var() protected const color EnergyBallLightColor_PoweredOn;
/** Color for energy ball light in powered-off state. */
var() protected const color EnergyBallLightColor_PoweredOff;

/** Brightness for energy ball light in powered-on state. */
var() protected const float EnergyBallLightBrightness_PoweredOn;
/** Brightness for energy ball light in powered-off state. */
var() protected const float EnergyBallLightBrightness_PoweredOff;

/** Made a parameter because having an = in a name literal is verboten for some reason. */
var protected const Name	EnergyBallMaterialParameterName;


/** Emitters for beams connecting powerball to shoulders */
var protected ParticleSystemComponent	LegAttachBeams[NUM_WALKER_LEGS];
/** Name of beam endpoint parameter in the particle system */
var protected const name				LegAttachBeamEndPointParamName;

/** Names of the top leg bones.  LegAttachBeams will terminate here. */
var protected const name				TopLegBoneName[NUM_WALKER_LEGS];


function PostBeginPlay()
{
	local int Idx;

	super.PostBeginPlay();

	EnergyBallMatInst = SkeletalMeshComponent.CreateMaterialInstance(1);
	SetEnergyBallPowerPercent(0.f);

	// attach powerball light to the ball
	SkeletalMeshComponent.AttachComponent(EnergyBallLight, BodyBoneName);

	// attach leg attach beam emitters to the ball
	for (Idx=0; Idx<NUM_WALKER_LEGS; ++Idx)
	{
		SkeletalMeshComponent.AttachComponent(LegAttachBeams[Idx], BodyBoneName);
	}
}

final protected function SetEnergyBallPowerPercent(float Pct)
{
	local float NewBrightness;
	local color NewColor;

	// store it
	CurrentEnergyBallPowerPct = Pct;

	// set light color and brightness
	NewColor = EnergyBallLightColor_PoweredOff + (EnergyBallLightColor_PoweredOn - EnergyBallLightColor_PoweredOff) * CurrentEnergyBallPowerPct;
	NewBrightness = EnergyBallLightBrightness_PoweredOff + (EnergyBallLightBrightness_PoweredOn - EnergyBallLightBrightness_PoweredOff) * CurrentEnergyBallPowerPct;
	EnergyBallLight.SetLightProperties(NewBrightness, NewColor);

	// set material param
	EnergyBallMatInst.SetScalarParameterValue(EnergyBallMaterialParameterName, CurrentEnergyBallPowerPct);
}

function Tick(float DeltaTime)
{
	local int Idx;
	local float NewPowerPct, NewBrightness;

	super.Tick(DeltaTime);

	// ball is powered on when driven, powered off otherwise
	GoalEnergyBallPowerPct = (WalkerVehicle.bDriving && !WalkerVehicle.bDeadVehicle) ? 1.f : 0.f;

	if (GoalEnergyBallPowerPct != CurrentEnergyBallPowerPct)
	{
		NewPowerPct = FInterpTo(CurrentEnergyBallPowerPct, GoalEnergyBallPowerPct, DeltaTime, EnergyBallPowerInterpSpeed);
		SetEnergyBallPowerPercent(NewPowerPct);
	}
	else if (WalkerVehicle.bDeadVehicle)
	{
		// this will fade light to zero after it gets to the zero-energy color
		NewBrightness = FInterpTo(EnergyBallLight.Brightness, 0.f, DeltaTime, EnergyBallPowerInterpSpeed);
		EnergyBallLight.SetLightProperties(NewBrightness);
	}

	// set leg attach beam endpoints 
	for (Idx=0; Idx<NUM_WALKER_LEGS; ++Idx)
	{
		LegAttachBeams[Idx].SetVectorParameter(LegAttachBeamEndPointParamName, SkeletalMeshComponent.GetBoneLocation(TopLegBoneName[Idx]));
	}
}

defaultproperties
{
	bHasCrouchMode=true
	FootStepEffects[0]=(MaterialType=Dirt,Sound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FootstepCue',ParticleTemplate=ParticleSystem'VH_Darkwalker.Effects.P_VH_DarkWalker_FootImpact_Dust')
	FootStepEffects[1]=(MaterialType=Snow,Sound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FootstepCue',ParticleTemplate=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_FootImpact_Snow')
	FootStepEffects[2]=(MaterialType=Water,Sound=SoundCue'A_Vehicle_DarkWalker.Cue.A_Vehicle_DarkWalker_FootstepCue',ParticleTemplate=ParticleSystem'Envy_Level_Effects_2.Vehicle_Water_Effects.P_DarkWalker_Water_Splash')

	FootWaterEffect=ParticleSystem'Envy_Level_Effects_2.Vehicle_Water_Effects.P_DarkWalker_Water_Splash'

	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'VH_DarkWalker.Mesh.SK_VH_DarkWalker_Legs'
		PhysicsAsset=PhysicsAsset'VH_DarkWalker.Mesh.SK_VH_DarkWalker_Legs_Physics_NewLegs'
		AnimSets(0)=AnimSet'VH_DarkWalker.Anims.K_VH_DarkWalker_Legs'
		AnimTreeTemplate=AnimTree'VH_DarkWalker.Anims.AT_VH_DarkWalker_Legs'
		bUpdateJointsFromAnimation=TRUE
	End Object

	MinStepDist=120.0
	MaxLegReach=750.0
	LegSpreadFactor=0.6

	CustomGravityScale=0.f
	FootEmbedDistance=32.0

	LandedFootDistSq=2500.0

	FootStepAnimNodeName[0]="Leg0 Step"
	FootStepAnimNodeName[1]="Leg1 Step"
	FootStepAnimNodeName[2]="Leg2 Step"

	//point light
	Begin Object Class=PointLightComponent Name=Light0
		Radius=300.f
		CastShadows=FALSE
		bForceDynamicLight=FALSE
		bEnabled=TRUE
		FalloffExponent=4.f
		LightingChannels=(BSP=FALSE,Static=FALSE,Dynamic=TRUE,bInitialized=TRUE)
	End Object
	EnergyBallLight=Light0

	EnergyBallLightColor_PoweredOn=(R=250,G=231,B=126)
	EnergyBallLightColor_PoweredOff=(R=150,G=50,B=10)

	EnergyBallLightBrightness_PoweredOn=8.f
	EnergyBallLightBrightness_PoweredOff=6.f

	EnergyBallMaterialParameterName="EnergyCore-power-1=on,2=off"

	EnergyBallPowerInterpSpeed=1.5f


	Begin Object Class=ParticleSystemComponent Name=LegAttachPSC_0
		Template=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_PowerBall_Idle'
		bAutoActivate=TRUE
	End Object
	LegAttachBeams[0]=LegAttachPSC_0

	Begin Object Class=ParticleSystemComponent Name=LegAttachPSC_1
		Template=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_PowerBall_Idle'
		bAutoActivate=TRUE
	End Object
	LegAttachBeams[1]=LegAttachPSC_1

	Begin Object Class=ParticleSystemComponent Name=LegAttachPSC_2
		Template=ParticleSystem'VH_DarkWalker.Effects.P_VH_DarkWalker_PowerBall_Idle'
		bAutoActivate=TRUE
	End Object
	LegAttachBeams[2]=LegAttachPSC_2

	LegAttachBeamEndPointParamName=DarkwalkerLegEnd

	TopLegBoneName[0]=Leg1_Bone1
	TopLegBoneName[1]=Leg2_Bone1
	TopLegBoneName[2]=Leg3_Bone1
}
