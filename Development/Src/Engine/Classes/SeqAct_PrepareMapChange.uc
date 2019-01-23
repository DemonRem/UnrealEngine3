/**
 * SeqAct_PrepareMapChange
 *
 * Kismet action exposing kicking off async map changes
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_PrepareMapChange extends SeqAct_Latent
	native(Sequence);

/** The main level that should be transitioned to.										*/
var() name			MainLevelName;

/** Additional secondary levels that should be pre-loaded before the switcheroo.		*/
var() array<name>	InitiallyLoadedSecondaryLevelNames;

/** If this is TRUE, then a much larger time slice will be given to the loading code (useful for loading during a movie, etc) */
var() bool			bIsHighPriority;

cpptext
{
	/**
	 * Called when this sequence action is being activated. Kicks off async background loading.
	 */
	void Activated();

	void DeActivated();

	/**
	 * Called from parent sequence via ExecuteActiveOps, returns TRUE to indicate this
	 * action has completed, which in this case means the engine is ready to have
	 * CommitMapChange called.
	 *
	 * @return TRUE if action has completed, FALSE otherwise
	 */
	UBOOL UpdateOp(FLOAT DeltaTime);
};

defaultproperties
{
	ObjName="Prepare Map Change"

	ObjCategory="Level"
	VariableLinks.Empty
	OutputLinks.Empty
	InputLinks(0)=(LinkDesc="PrepareLoad")
	OutputLinks(0)=(LinkDesc="Finished")
}
