/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * A custom action to demonstrate condensing a complex Kismet script into a single action.
 */
class SeqAct_DrumHit extends SeqAct_Latent
	native(Sequence);

cpptext
{
	// override default activated behavior
	UBOOL UpdateOp(FLOAT DeltaTime);
}

/** Sound to play on activation */
var() SoundCue HitSound;

/** Light brightness range */
var() editinline DistributionFloatUniform BrightnessRange;

/** Light brightness interpolation rates */
var() float BrightnessRampUpRate;
var() float BrightnessRampDownRate;

/** Which direction are we currently interpolating brightness? */
var transient float PulseDirection;

/** Current interpolation value */
var transient float LightBrightness;

/** Cached list of lights to interpolate */
var transient array<Light> CachedLights;

defaultproperties
{
	ObjName="Drum Hit"
	ObjCategory="DemoGame"

	// First input to only play the sound
	InputLinks(0)=(LinkDesc="Sound Only")
	// Second input to activate everything
	InputLinks(1)=(LinkDesc="All")

	// Actor to play sound on
	VariableLinks(0)=(LinkDesc="Sound Source",ExpectedType=class'SeqVar_Object')
	// Lights to pulse on activation
	VariableLinks(1)=(LinkDesc="Light",ExpectedType=class'SeqVar_Object')
	// Emitters to toggle on activation
	VariableLinks(2)=(LinkDesc="Emitter",ExpectedType=class'SeqVar_Object')

	// set the default brightness range
	Begin Object class=DistributionFloatUniform Name=LightBrightnessDistribution
		Min=0.f
		Max=1.f
	End Object
	BrightnessRange=LightBrightnessDistribution

	// default brightness interpolation rate
	BrightnessRampUpRate=0.1f
	BrightnessRampDownRate=0.5f
}
