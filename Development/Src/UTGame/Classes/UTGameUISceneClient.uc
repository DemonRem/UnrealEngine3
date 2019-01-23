/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTGameUISceneClient extends GameUISceneClient
	dependson(UTSeqObj_SPMission)
	native(UI);

/** Debug values */

var(Debug)  bool bShowRenderTimes;
var float PreRenderTime;
var float RenderTime;
var float TickTime;
var float AnimTime;
var float AvgTime;
var float AvgRenderTime;
var float FrameCount;
var float StringRenderTime;



/** Holds a copy of the current mission info */
var transient EMissionData CurrentMissionData;

cpptext
{
	/**
	 * Render all the active scenes
	 */
	virtual void RenderScenes( FCanvas* Canvas );

	virtual void Render_Scene( FCanvas* Canvas, UUIScene* Scene );
	virtual void Tick(FLOAT DeltaTime);
}

/**
 * Sets the current mission data
 */

function SetCurrentMission(EMissionData NewMissionData)
{
	CurrentMissionData = NewMissionData;
}

/** @return TRUE if there are any scenes currently accepting input, FALSE otherwise. */
native function bool IsUIAcceptingInput();


defaultproperties
{

	Begin Object Class=UIAnimationSeq Name=seqRotPitchNeg90
		SeqName=RotPitchNeg90
		SeqDuration=0.6
		Tracks(0)=(TrackType=EAT_Rotation,KeyFrames=((TimeMark=0.0,Data=(DestAsRotator=(Pitch=16384))),(TimeMArk=1.0,Data=(DestAsRotator=(Pitch=0)))))
	End Object
	AnimSequencePool.Add(seqRotPitchNeg90)

	Begin Object Class=UIAnimationSeq Name=seqRotPitch90
		SeqName=RotPitch90
		SeqDuration=0.6
		Tracks(0)=(TrackType=EAT_Rotation,KeyFrames=((TimeMark=0.0,Data=(DestAsRotator=(Pitch=0))),(TimeMArk=1.0,Data=(DestAsRotator=(Pitch=16384)))))
	End Object
	AnimSequencePool.Add(seqRotPitch90)

	Begin Object Class=UIAnimationSeq Name=seqFadeIn
		SeqName=FadeIn
		SeqDuration=1.0
		Tracks(0)=(TrackType=EAT_Opacity,KeyFrames=((TimeMark=0.0,Data=(DestAsFloat=0.0)),(TimeMark=1.0,Data=(DestAsFloat=1.0))))
	End Object
	AnimSequencePool.Add(seqFadeIn)

	Begin Object Class=UIAnimationSeq Name=seqFadeOut
		SeqName=FadeOut
		SeqDuration=1.0
		Tracks(0)=(TrackType=EAT_Opacity,KeyFrames=((TimeMark=0.0,Data=(DestAsFloat=1.0)),(TimeMark=1.0,Data=(DestAsFloat=0.0))))
	End Object
	AnimSequencePool.Add(seqFadeOut)

}
