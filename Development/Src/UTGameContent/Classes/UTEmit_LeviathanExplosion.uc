/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTEmit_LeviathanExplosion extends UTReplicatedEmitter;

var float Damage;
var float DamageRadius;
var float MomentumTransfer;
var Class<DamageType> MyDamageType;
var class<UTExplosionLight> ExplosionLightClass;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( WorldInfo.NetMode != NM_DedicatedServer && !WorldInfo.bDropDetail )
	{
		AttachComponent(new(Outer) ExplosionLightClass);
	}
}

function Explode()
{
	GotoState('Dying');
}

state Dying
{
	function BeginState(Name PreviousStateName)
	{
		InitialState = 'Dying';
	}

Begin:
    HurtRadius(Damage, DamageRadius*0.125, MyDamageType, MomentumTransfer, Location);
    Sleep(0.5);
    HurtRadius(Damage, DamageRadius*0.300, MyDamageType, MomentumTransfer, Location);
    Sleep(0.2);
    HurtRadius(Damage, DamageRadius*0.475, MyDamageType, MomentumTransfer, Location);
    Sleep(0.2);
    HurtRadius(Damage, DamageRadius*0.650, MyDamageType, MomentumTransfer, Location);
    Sleep(0.2);
    HurtRadius(Damage, DamageRadius*0.825, MyDamageType, MomentumTransfer, Location);
    Sleep(0.2);
    HurtRadius(Damage, DamageRadius*1.000, MyDamageType, MomentumTransfer, Location);
}

defaultproperties
{
	TickGroup=TG_PreAsyncWork
	ServerLifeSpan=3.0
	LifeSpan=0.0
	EmitterTemplate=ParticleSystem'VH_Leviathan.Effects.P_VH_Leviathan_BigBeam_Explode'
	ExplosionLightClass=UTHugeExplosionLight

	Damage=250.0
	DamageRadius=2000.0
	MomentumTransfer=250000
	MyDamageType=class'UTDmgType_Redeemer'

	Begin Object Class=AudioComponent Name=ExplosionAC
		SoundCue=SoundCue'A_Vehicle_Leviathan.SoundCues.A_Vehicle_Leviathan_BigBang'
		bAutoPlay=true
		bAutoDestroy=true
	End Object
	Components.Add(ExplosionAC);
	bDestroyOnSystemFinish=true
}
