/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_CameraLookAt extends SequenceAction
	native(Sequence);

/** Should this affect the camera? */
var()	bool		bAffectCamera;
/** If FALSE, focus only if point roughly in view; if TRUE, focus no matter where player is looking */
var()	bool		bAlwaysFocus;
/** If TRUE, camera adjusts to keep player in view */
var()	bool		bAdjustCamera;
/** If TRUE, ignore world trace to find a good spot */
var()	bool		bIgnoreTrace;
/** Speed range of interpolation to focus camera */
var()	Vector2d	InterpSpeedRange;
/** How tight the focus should be */
var()	Vector2d	InFocusFOV;
/** Name of bone to focus on if available */
var() 	Name		FocusBoneName;
/** Should this turn the character's head? */
var()	bool		bAffectHead;
/** Set this player in god mode?  Only works if bAffectCamera == TRUE */
var() 	bool		bToggleGodMode;
/** Leave the camera focused on the actor? */
var()	bool		bLeaveCameraRotation;
/** Text to display while camera is focused */
var()	String		TextDisplay;
/** Don't allow input */
var()	bool		bDisableInput;
/** The total amount of time the camera lookat will happen */
var()	float		TotalTime;
/** Whether this event used a timer or not */
var		bool		bUsedTimer;
/** TRUE to validate visibility of lookat target before doing any camera changes */
var()	bool		bCheckLineOfSight;

var transient float RemainingTime;

cpptext
{
	void Activated();
	UBOOL UpdateOp(FLOAT DeltaTime);
	void DeActivated();
};

defaultproperties
{
	ObjName="Look At"
	ObjCategory="Camera"

	bAffectCamera=TRUE
	InterpSpeedRange=(X=3,Y=3)
	InFocusFOV=(X=1,Y=1)

	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Focus")

	OutputLinks(0)=(LinkDesc="Out")		// left in for legacy reasons... don't want to break levels.
	OutputLinks(1)=(LinkDesc="Finished")
	OutputLinks(2)=(LinkDesc="Succeeded")		// fires if lookat actually happens
	OutputLinks(3)=(LinkDesc="Failed")			// fires if lookat fails (eg bCheckLineOfSight=true but trace fails)

	bDisableInput=TRUE
	TotalTime=0.0f
	bLatentExecution=TRUE
	bUsedTimer=FALSE
	ObjClassVersion=3
}
