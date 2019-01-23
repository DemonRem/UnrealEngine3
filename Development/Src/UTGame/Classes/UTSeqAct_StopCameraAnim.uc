/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * UT-specific CameraAnim action to provide for stopping a playing cameraanim.
 */

class UTSeqAct_StopCameraAnim extends SequenceAction;

/** True to stop immediately, regardless of BlendOutTime specified when the anim was played. */
var()	bool		bStopImmediately;

defaultproperties
{
	ObjClassVersion=1
	
	ObjName="Stop Camera Animation"
	ObjCategory="Camera"
}
