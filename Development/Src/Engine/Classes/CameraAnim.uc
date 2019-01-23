
/**
 *	CameraAnim: defines a pre-packaged animation to be played on a camera.
 * 	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class CameraAnim extends Object
	notplaceable
	native;

/** The InterpGroup that holds our actual interpolation data. */
var InterpGroup		CameraInterpGroup;

/** Length, in seconds. */
var float			AnimLength;

cpptext
{
	UBOOL CreateFromInterpGroup(class UInterpGroup* SrcGroup, class USeqAct_Interp* Interp);
};

defaultproperties
{
	AnimLength=3.f
}
