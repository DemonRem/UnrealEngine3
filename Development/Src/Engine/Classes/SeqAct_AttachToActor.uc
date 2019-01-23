/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_AttachToActor extends SequenceAction;

/** if true, then attachments will be detached. */
var() bool		bDetach;

/** Should hard attach to the actor */
var() bool		bHardAttach;

/** Bone Name to use for attachment */
var() Name		BoneName;

/** true if attachment should be set relatively to the target, using an offset */
var() bool		bUseRelativeOffset;

/** offset to use when attaching */
var() vector	RelativeOffset;

/** Use relative rotation offset */
var() bool		bUseRelativeRotation;

/** relative rotation */
var()	Rotator	RelativeRotation;

defaultproperties
{
	ObjName="Attach to Actor"
	ObjCategory="Actor"
	ObjClassVersion=2

	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Attachment")

	bHardAttach=TRUE
}
