/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSeqAct_TutorialMsg extends SequenceAction
	native;


/** The Message to display.  You can enter it directly here or use markup */
var() string TutorialMessage;
var() surface TutorialImage;	// TODO: Add this to the custom scene

var transient UTUIScene_MessageBox MsgBox;
var transient bool bFinished;

cpptext
{
	virtual UBOOL UpdateOp(FLOAT deltaTime);
}

event Activated()
{
 	local GameUISceneClient SC;
 	local UIScene S;

	bFinished = false;
 	SC = class'UIRoot'.static.GetSceneClient();
 	if (SC != none )
 	{
		SC.OpenScene(class'UTUIScene'.Default.MessageBoxScene,,S);
		MsgBox = UTUIScene_MessageBox(S);
		if ( MsgBox != none )
		{
			MsgBox.Display(TutorialMessage, "Temp Msg", MBClosed);
		}
 	}
}

function MBClosed(int SelectedOption, int PIndex)
{
 	local GameUISceneClient SC;

 	SC = class'UIRoot'.static.GetSceneClient();
	if ( SC != none )
	{
		SC.CloseScene(MsgBox);
	}

	bFinished = true;
	OutputLinks[0].bHasImpulse = true;
}


/**
 * Determines whether this class should be displayed in the list of available ops in the level kismet editor.
 *
 * @return	TRUE if this sequence object should be available for use in the level kismet editor
 */
event bool IsValidLevelSequenceObject()
{
	return true;
}

DefaultProperties
{
	bAutoActivateOutputLinks=false
	bCallHandler=false
	OutputLinks(0)=(LinkDesc="Success")
	ObjName="Set Tutorial Message"
	ObjCategory="Tutorials"
	bFinished=false;
	bLatentExecution=true;
}
