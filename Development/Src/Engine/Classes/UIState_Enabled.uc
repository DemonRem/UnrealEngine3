/**
 * Default widget state.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIState_Enabled extends UIState
	native(inherit);

cpptext
{
	/**
	 * Notification that Target has made this state its active state.
	 *
	 * @param	Target			the widget that activated this state.
	 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated this call
	 * @param	bPushState		TRUE if this state needs to be added to the state stack for the owning widget; FALSE if this state was already
	 *							in the state stack of the owning widget and is being activated for additional split-screen players.
	 */
	virtual void OnActivate( UUIScreenObject* Target, INT PlayerIndex, UBOOL bPushState );
}


/**
 * Deactivate this state for the specified target.
 * This version removes also deactivates all states which have "false" for the value of bRequiresEnabledState.
 *
 * @param	Target			the widget that is deactivating this state.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated this call
 *
 * @return	TRUE to allow this state to be deactivated for the specified Target.
 */
event bool DeactivateState( UIScreenObject Target, int PlayerIndex )
{
	local int StateIndex, EnabledIndex;
	local array<UIState> StateList;

	local bool bResult;

	bResult = Super.DeactivateState(Target,PlayerIndex);

	// if nothing in our super class's DeactivateState wants to prevent this state from being deactivated
	if ( Target != None && bResult )
	{
		// All states except the disabled state require that the enabled state be in the StateStack
		// it might make sense to generalize this functionality
		if ( Target.HasActiveStateOfClass(class,PlayerIndex,EnabledIndex) )
		{
			StateList = Target.StateStack;

			// deactivate all states higher than this one in the state stack
			for ( StateIndex = StateList.Length - 1; StateIndex > EnabledIndex; StateIndex-- )
			{
				// attempt to deactivate it...if it can't be deactivated, we can't deactivate the enabled state
				if ( !StateList[StateIndex].DeactivateState(Target,PlayerIndex) )
				{
					bResult = false;
					break;
				}
			}
		}
	}

	return bResult;
}

DefaultProperties
{
	StackPriority=5
}
