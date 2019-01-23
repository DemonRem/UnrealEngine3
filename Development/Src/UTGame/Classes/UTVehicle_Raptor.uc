/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTVehicle_Raptor extends UTAirVehicle
	native(Vehicle);

/** bForwardMode==true if vehicle is thrusting forward.  Used by wing controls. */
var bool bForwardMode;

var name TurretPivotSocketName;

var particlesystem TeamMF[2];
cpptext
{
	virtual FVector GetSeatPivotPoint(int SeatIndex);
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

simulated function ForceSingleWingUp(bool bRight)
{
	local UTSkelControl_RaptorWing WingControl;

	WingControl = UTSkelControl_RaptorWing( mesh.FindSkelControl('WingControl') );
	if (WingControl != none)
	{
		WingControl.ForceSingleWingUp(bRight, 3000);
	}
}

simulated function ForceWingsUp()
{
	local UTSkelControl_RaptorWing WingControl;

	WingControl = UTSkelControl_RaptorWing( mesh.FindSkelControl('WingControl') );
	if (WingControl != none)
	{
		WingControl.ForceWingsUp(3000);
	}
}

simulated function TeamChanged()
{
	super.Teamchanged();
	VehicleEffects[0].EffectTemplate = TeamMF[Team%2];
	if(VehicleEffects[0].EffectRef != none)
	{
		VehicleEffects[0].EffectRef.DeactivateSystem();
		VehicleEffects[0].EffectRef.KillParticlesForced();
		VehicleEffects[0].EffectRef.SetTemplate(TeamMF[Team%2]);
	}
	VehicleEffects[1].EffectTemplate = TeamMF[Team%2];
	if(VehicleEffects[1].EffectRef != none)
	{
		VehicleEffects[1].EffectRef.DeactivateSystem();
		VehicleEffects[1].EffectRef.KillParticlesForced();
		VehicleEffects[1].EffectRef.SetTemplate(TeamMF[Team%2]);
	}
}
defaultproperties
{
	AirSpeed=2500.0
	GroundSpeed=2000
	Health=300
	MomentumMult=2.0

	UprightLiftStrength=30.0
	UprightTorqueStrength=30.0
	PushForce=50000.0

	bStayUpright=true
	StayUprightRollResistAngle=5.0
	StayUprightPitchResistAngle=5.0
	StayUprightStiffness=400
	StayUprightDamping=20

	SpawnRadius=180.0

	Begin Object Class=UTVehicleSimChopper Name=SimObject
		MaxThrustForce=750.0
		MaxReverseForce=100.0
		LongDamping=0.7
		MaxStrafeForce=450.0
		LatDamping=0.7
		MaxRiseForce=500.0
		UpDamping=0.7
		TurnTorqueFactor=4500.0
		TurnTorqueMax=240000.0
		TurnDamping=0.4
		MaxYawRate=1.5
		PitchTorqueFactor=450.0
		PitchTorqueMax=60.0
		PitchDamping=0.3
		RollTorqueTurnFactor=450.0
		RollTorqueStrafeFactor=50.0
		RollTorqueMax=50.0
		RollDamping=0.1
		MaxRandForce=25.0
		RandForceInterval=0.5
		StopThreshold=100
		bFullThrustOnDirectionChange=true
	End Object
	SimObj=SimObject
	Components.Add(SimObject)

	BaseEyeheight=50
	Eyeheight=50
	bRotateCameraUnderVehicle=false
	CameraLag=0.05

	MaxDesireability=0.6
}

