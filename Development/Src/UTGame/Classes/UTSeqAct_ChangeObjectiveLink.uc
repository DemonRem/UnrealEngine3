/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTSeqAct_ChangeObjectiveLink extends SequenceAction;

/** the two objectives to add/remove their link together */
var() UTOnslaughtNodeObjective Objective1, Objective2;
/** if adding a link, whether that link should be one way */
var() bool bOneWay;

event Activated()
{

	// verify that valid objectives were selected
	if ( Objective1 == None || Objective2 == None )
	{
		ScriptLog("WARNING: Unable to link invalid objectives" @ Objective1 @ "and" @ Objective2);
	}
	else if (InputLinks[0].bHasImpulse)
	{
		// link the objectives together
		Objective1.AddLink(Objective2);
	}
	else if (InputLinks[1].bHasImpulse)
	{
		// remove the link between the objectives
		Objective1.RemoveLink(Objective2);
	}
}

defaultproperties
{
	bCallHandler=false
	ObjName="Change Objective Link"
	ObjCategory="Objective"
	InputLinks(0)=(LinkDesc="Add")
	InputLinks(1)=(LinkDesc="Remove")
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Objective 1",PropertyName=Objective1)
	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Objective 2",PropertyName=Objective2)
}
