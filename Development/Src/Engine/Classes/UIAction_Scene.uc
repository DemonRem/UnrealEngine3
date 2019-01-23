/**
 * Base class for all actions that manipulate scenes.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIAction_Scene extends UIAction
	abstract
	native(UISequence);

cpptext
{
	/** USequenceOp interface */
	/**
	 * Allows the operation to initialize the values for any VariableLinks that need to be filled prior to executing this
	 * op's logic.  This is a convenient hook for filling VariableLinks that aren't necessarily associated with an actual
	 * member variable of this op, or for VariableLinks that are used in the execution of this ops logic.
	 *
	 * Initializes the value of the Scene linked variables
	 */
	virtual void InitializeLinkedVariableValues();
}

/** the scene that this action will manipulate */
var()	UIScene		Scene;

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
	OutputLinks(1)=(LinkDesc="Failed")

	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Scene",PropertyName=Scene)
}
