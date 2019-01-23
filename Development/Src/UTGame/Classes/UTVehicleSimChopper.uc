/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleSimChopper extends SVehicleSimBase
	native(Vehicle);

var()	float				MaxThrustForce;
var()	float				MaxReverseForce;
var()	float				LongDamping;

var()	float				MaxStrafeForce;
var()	float				LatDamping;

var()	float				MaxRiseForce;
var()	float				UpDamping;

var()	float				TurnTorqueFactor;
var()	float				TurnTorqueMax;
var()	float				TurnDamping;
var()	float				MaxYawRate;

var()	float				PitchTorqueFactor;
var()	float				PitchTorqueMax;
var()	float				PitchDamping;

var()	float				RollTorqueTurnFactor;
var()	float				RollTorqueStrafeFactor;
var()	float				RollTorqueMax;
var()	float				RollDamping;

var()	float				StopThreshold;

var()   float               MaxRandForce;
var()   float               RandForceInterval;

var()   bool                bAllowZThrust;

/** If true, use full forward thrust force while changing directions, even for reverse or strafing */
var()	bool				bFullThrustOnDirectionChange;

var()	bool				bShouldCutThrustMaxOnImpact;
var		bool				bRecentlyHit;
/** If true, strafing will increase the turn rate */
var		bool	bStrafeAffectsTurnDamping;

var		float	StrafeTurnDamping;

var		float				TargetHeading;
var		float				TargetPitch;
var		float				PitchViewCorrelation;
var     bool                bHeadingInitialized;

var     vector              RandForce;
var     vector              RandTorque;
var     float               AccumulatedTime;

/** If bStabilizeStops=true, forces are applied to attempt to keep the vehicle from moving when it is stopped */
var()		bool	bStabilizeStops;

/** Stabilization force multiplier when bStabilizeStops=true */
var()		float	StabilizationForceMultiplier;

/** modified based on whether velocity is decreasing acceptably due to stabilization */
var			float	CurrentStabilizationMultiplier;

/** OldVelocity saved to monitor stabilization */
var			vector	OldVelocity;

cpptext
{
	virtual void ProcessCarInput(ASVehicle* Vehicle);
	virtual void UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime);
	virtual FVector StabilizationForce(ASVehicle* Vehicle, FLOAT DeltaTime, UBOOL bShouldStabilize);
	virtual FVector StabilizationTorque(ASVehicle* Vehicle, FLOAT DeltaTime, UBOOL bShouldStabilize);
	float GetEngineOutput(ASVehicle* Vehicle);
	virtual void GetRotationAxes(ASVehicle* Vehicle, FVector &DirX, FVector &DirY, FVector &DirZ);
}


defaultproperties
{
	StabilizationForceMultiplier=1.0
	CurrentStabilizationMultiplier=1.0
	PitchViewCorrelation=0.0
}
