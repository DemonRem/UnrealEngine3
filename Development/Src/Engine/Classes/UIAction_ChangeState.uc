/**
 * Base class for actions that change widget states.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIAction_ChangeState extends UIAction
	abstract
	native(inherit);

/**
 * If not target state is specified, then the first State in the target widget's InactiveStates array
 * that has this class will be activated
 */
var()	class<UIState>		StateType;

/**
 * (Optional)
 * the state that should be activated by this action.  Useful when a widget contains more
 * than one instance of a particular state class in its InactiveStates array (such as a focused state, and
 * a specialized version of the focused state)
 */
var()			UIState		TargetState;

/**
 * Determines whether this action was successfully executed.
 * Should be set by the handler function for this action.
 */
var		bool				bStateChangeFailed;

DefaultProperties
{
	ObjClassVersion=5
	bCallHandler=false
	OutputLinks(0)=(LinkDesc="Successful")
	OutputLinks(1)=(LinkDesc="Failed")
//	VariableLinks.Add(ExpectedType=class'SeqVar_Object',LinkDesc="Target State")

	bAutoActivateOutputLinks=false
	bAutoTargetOwner=true
}
