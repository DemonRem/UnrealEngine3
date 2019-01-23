/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTSeqAct_ChangeNodeStatus extends SequenceAction;

/** the team that should have control of the node, if it is set to Constructing or Built state */
var() int OwnerTeam;

defaultproperties
{
	ObjName="Change Node Status"
	ObjCategory="Objective"

	InputLinks[0]=(LinkDesc="Constructing")
	InputLinks[1]=(LinkDesc="Built")
	InputLinks[2]=(LinkDesc="Neutral")

	VariableLinks[1]=(ExpectedType=class'SeqVar_Int',LinkDesc="Owner Team",PropertyName=OwnerTeam)
}
