/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_Nuke extends SequenceAction
	deprecated
	native(Sequence);

cpptext
{
	virtual void Activated();
}

/** DO NOT USE */

defaultproperties
{
	ObjName="Nuke"
	ObjCategory="Actor"
}
