/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_DemoCameraShake extends SequenceAction;

var() DemoCamMod_ScreenShake.ScreenShakeStruct	CameraShake;

var() float		GlobalScale;
var() float		GlobalAmplitudeScale; 
var() float		GlobalFrequencyScale; 

defaultproperties
{
	ObjName="Camera Shake"
	ObjCategory="DemoGame"

	GlobalScale=1.f
	GlobalAmplitudeScale=1.f
	GlobalFrequencyScale=1.f

	InputLinks(0)=(LinkDesc="Timed")
	InputLinks(1)=(LinkDesc="Continuous")
	InputLinks(2)=(LinkDesc="Stop")
}
