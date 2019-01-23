/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTAnimBlendByCollision extends AnimNodeBlendPerBone
	native(Animation);

/** Name of the socket on the parent mesh to perform the trace from */
var() name 	TraceSocket;

/** How far forward should we trace */
var() float TraceDistance;

/** The final distance will be adjusted by this amount */
var() float TraceAdjustment;

/** If true, this node will override the collision setting and force the blend */
var bool bForceBlend;

/** If true, this node will pulse it's blend one time */
var bool bPulseBlend;

/** How long after going to blend do we go back */
var float PulseDelay;

/** Holds a reference to the last actor hit during the trace */
var transient actor  LastHitActor;
var transient vector LastHitLocation;

event SetForceBlend(bool bForce, optional float Rate)
{
	bForceBlend = bForce;
	SetBlendTarget(1.0, (Rate <= 0.0) ? 0.65 : Rate);
}

event Pulse(float Delay)
{
	bPulseBlend = true;
	PulseDelay = Delay;
	SetBlendTarget(1.0, 0.33);
}

cpptext
{
	virtual	void TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight );
}

defaultproperties
{
	Children(0)=(Name="Not Colliding",Weight=1.0)
	Children(1)=(Name="Colliding")
	bFixNumChildren=true
	TraceAdjustment=300;
	PulseDelay=0.33
}
