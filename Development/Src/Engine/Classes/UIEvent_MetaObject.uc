/**
 * This object is a event that draw connections for all other UI Event objects, it allows the user to bind/remove input events.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * @note: native because C++ code activates this event
 */
class UIEvent_MetaObject extends UIEvent
	native(inherit)
	transient
	inherits(FCallbackEventDevice);

cpptext
{

	/**
	 * Sets up any callbacks that this object may need to respond to.
	 */
	void RegisterCallbacks();

	/**
	 * Removes all of the callbacks for this object.
	 */
	void FinishDestroy();

	/**
	 * Gets the array of input actions from the parent state and creates output links corresponding to the input actions.
	 */
	void ReadInputActionsFromState();

	/**
	 * Saves the current connections to the parent state's input action array.
	 *
	 * @return	TRUE if the state's list of active input keys was modified.
	 */
	UBOOL SaveInputActionsToState();

	/** Get the name of the class used to help out when handling events in UnrealEd.
	 * @return	String name of the helper class.
	 */
	virtual const FString GetEdHelperClassName() const
	{
		return FString( TEXT("UnrealEd.UIEvent_MetaObjectHelper") );
	}

	/** Rebuilds the list of output links using the items in the State's StateInputActions array. */
	void RebuildOutputLinks();

	/**
	 * Generates a description for the action specified in the form: KeyName (InputEventType)
	 *
	 * @param InAction			The action to generate a description for.
	 * @param OutDescription	Storage string for the generated action.
	 */
	void GenerateActionDescription(const FInputKeyAction &InAction, FString& OutDescription);

	/** @return Returns the owner state of this event meta object. */
	UUIState* GetOwnerState();

	/**
	 * Responds to callback events and updates the metaobject as necessary.
	 *
	 * @param	InType		The type of callback event that was sent.
	 */
	void Send( ECallbackEventType InType );
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
	return false;
}

DefaultProperties
{
	ObjName="State Input Events"
	bDeletable=false
}

