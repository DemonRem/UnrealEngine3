/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_SetMusicTrack extends SequenceAction
	dependson(SeqVar_MusicTrackBank)
	native(Sequence);


/** This is the track bank that should be used if there is one not attached **/
var() name TrackBankName;



cpptext
{
	virtual void Activated();
};


defaultproperties
{
	ObjName="Set Music Track"
	ObjCategory="Sound"
	ObjColor=(R=255,G=0,B=255,A=255)


	InputLinks(0)=(LinkDesc="Set Music Track")

	OutputLinks(0)=(LinkDesc="Out")


	VariableLinks(0)=(ExpectedType=class'SeqVar_MusicTrackBank',LinkDesc="MusicTrackBank",bWriteable=true,MinVars=1,MaxVars=1)

	VariableLinks(1)=(ExpectedType=class'SeqVar_MusicTrack',LinkDesc="MusicTrack",MinVars=1,MaxVars=1)

}
