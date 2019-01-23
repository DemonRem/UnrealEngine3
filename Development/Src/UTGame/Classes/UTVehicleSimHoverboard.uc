/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleSimHoverboard extends SVehicleSimBase
    native(Vehicle);

var()	float				MaxThrustForce;
var()	float				MaxReverseForce;
var()	float				MaxReverseVelocity;
var()	float				LongDamping;

var()	float				MaxStrafeForce;
var()	float				LatDamping;

var()	float				MaxRiseForce;

var()	float				TurnTorqueFactor;
var()	float				SpinTurnTorqueScale;
var()	float				MaxTurnTorque;
var()	InterpCurveFloat	TurnDampingSpeedFunc;

var()	float				StopThreshold;

// Internal
var     bool                bHeadingInitialized;
var     float               TargetHeading;

cpptext
{
    virtual void ProcessCarInput(ASVehicle* Vehicle);
	virtual void UpdateVehicle(ASVehicle* Vehicle, FLOAT DeltaTime);
	float GetEngineOutput(ASVehicle* Vehicle);
}
