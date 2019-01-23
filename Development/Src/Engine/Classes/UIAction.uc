/**
 * Abstract base class for UI actions.
 * Actions perform tasks for widgets, in response to some external event.  Actions are created by programmers and are
 * bound to widget events by designers using the UI editor.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIAction extends SequenceAction
	native(UISequence)
	abstract
	placeable;

/**
 * The ControllerId of the LocalPlayer corresponding to the 'PlayerIndex' element of the Engine.GamePlayers array.
 */
var	transient	noimport	int		GamepadID;

/**
 * Controls whether this action is automatically executed on the owning widget.  If true, this action will add the owning
 * widget to the Targets array when it's activated, provided the Targets array is empty.
 */
var()		bool		bAutoTargetOwner;

cpptext
{
	/* === USequenceOp interface */
	/**
	 * Allows the operation to initialize the values for any VariableLinks that need to be filled prior to executing this
	 * op's logic.  This is a convenient hook for filling VariableLinks that aren't necessarily associated with an actual
	 * member variable of this op, or for VariableLinks that are used in the execution of this ops logic.
	 *
	 * Initializes the value of the Player Index VariableLinks
	 */
	virtual void InitializeLinkedVariableValues();

	virtual void Activated();
	virtual void DeActivated();

	/**
	 * Notification that an input link on this sequence op has been given impulse by another op.
	 *
	 * This version also initializes the value of GamepadID.
	 *
	 * @param	ActivatorOp		the sequence op that applied impulse to this op's input link
	 * @param	InputLinkIndex	the index [into this op's InputLinks array] for the input link that was given impulse
	 */
	virtual void OnReceivedImpulse( class USequenceOp* ActivatorOp, INT InputLinkIndex );

	/* === USequenceObject interface === */
	/** Get the name of the class used to help out when handling events in UnrealEd.
	 * @return	String name of the helper class.
	 */
	virtual const FString GetEdHelperClassName() const
	{
		return FString( TEXT("UnrealEd.UISequenceObjectHelper") );
	}

	/* === UObject interface === */
	/**
	 * Called after this object has been completely de-serialized.  This version validates that this action has at least one
	 * InputLink, and if not resets this action's InputLinks array to the default version
	 */
	virtual void PostLoad();
}

/**
 * Returns the widget that contains this UIAction.
 */
native final function UIScreenObject GetOwner() const;

/**
 * Returns the scene that contains this UIAction.
 */
native final function UIScene GetOwnerScene() const;

/**
 * Determines whether this class should be displayed in the list of available ops in the level kismet editor.
 *
 * @return	TRUE if this sequence object should be available for use in the level kismet editor
 */
event bool IsValidLevelSequenceObject()
{
	return false;
}

/**
 * Determines whether this class should be displayed in the list of available ops in the UI's kismet editor.
 *
 * @param	TargetObject	the widget that this SequenceObject would be attached to.
 *
 * @return	TRUE if this sequence object should be available for use in the UI kismet editor
 */
event bool IsValidUISequenceObject( optional UIScreenObject TargetObject )
{
	return true;
}


defaultproperties
{
	ObjCategory="UI"
	ObjClassVersion=4

	GamepadID=-1

	// the index for the player that activated this event
	VariableLinks.Add((ExpectedType=class'SeqVar_Int',LinkDesc="Player Index",bWriteable=true,bHidden=true))

	// the gamepad id for the player that activated this event
	VariableLinks.Add((ExpectedType=class'SeqVar_Int',LinkDesc="Gamepad Id",PropertyName=GamepadID,bWriteable=true,bHidden=true))
}
