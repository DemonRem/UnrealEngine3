/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UIState_UTObjStates extends UIState
	abstract
	native(UI);

/**
 * UTObjStates are exclusive so if one get's activated, deactivate all others
 */
event bool ActivateState( UIScreenObject Target, int PlayerIndex )
{
	local int i;
	local bool bResult;
	local UIStatE_UTObjStates TestState;

	bResult = false;
	if ( Target != None )
	{
		for (i=0; i < Target.StateStack.Length; i++)
		{
			TestState = UIState_UTObjStates( Target.StateStack[i] );
			if (  TestState != none && TestState != self )
			{
				Target.DeactivateState( TestState, PlayerIndex);
			}
		}
		bResult = true;
	}

	return bResult;
}


defaultproperties
{
}
