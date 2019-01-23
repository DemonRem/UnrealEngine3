/**
 * Container object for all sequence objects, also responsible
 * for execution of objects.  Can contain nested Sequence objects
 * as well.
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class Sequence extends SequenceOp
	native(Sequence);

cpptext
{
	virtual void PostLoad();

	void CheckForErrors();

	/**
	 * Adds a new SequenceObject to this sequence's list of ops
	 *
	 * @param	NewObj		the sequence object to add.
	 * @param	bRecurse	if TRUE, recursively add any sequence objects attached to this one
	 *
	 * @return	TRUE if the object was successfully added to the sequence.
	 */
	virtual UBOOL AddSequenceObject( USequenceObject* NewObj, UBOOL bRecurse=FALSE );

	/**
	 * Removes the specified object from the SequenceObjects array, severing any links to that object.
	 *
	 * @param	ObjectToRemove	the SequenceObject to remove from this sequence.  All links to the object will be cleared.
	 * @param	ModifiedObjects	a list of objects that have been modified the objects that have been
	 */
	virtual void RemoveObject( USequenceObject* ObjectToRemove );

	/**
	 * Removes the specified objects from this Sequence's SequenceObjects array, severing any links to these objects.
	 *
	 * @param	ObjectsToRemove	the sequence objects to remove from this sequence.  All links to these objects will be cleared,
	 *							and the objects will be removed from all SequenceObject arrays.
	 */
	void RemoveObjects( const TArray<USequenceObject*>& ObjectsToRemove);

	/**
	 * Adds the specified SequenceOp to this sequence's list of ActiveOps.
	 *
	 * @param	NewSequenceOp	the sequence op to add to the list
	 * @param	bPushTop		if TRUE, adds the operation to the top of stack (meaning it will be executed first),
	 *							rather than the bottom
	 *
	 * @return	TRUE if the sequence operation was successfully added to the list.
	 */
	virtual UBOOL QueueSequenceOp( USequenceOp* NewSequenceOp, UBOOL bPushTop=FALSE );

	UBOOL ExecuteActiveOps(FLOAT DeltaTime, INT MaxSteps = 0);
	UBOOL UpdateOp(FLOAT DeltaTime);

	VARARG_DECL(void, void, {}, ScriptLogf, VARARG_NONE, const TCHAR*, VARARG_NONE, VARARG_NONE);
	VARARG_DECL(void, void, {}, ScriptWarnf, VARARG_NONE, const TCHAR*, VARARG_NONE, VARARG_NONE);

	virtual void Activated();

	virtual void UpdateObject()
	{
		// do nothing
	}

	virtual void OnCreated()
	{
		Super::OnCreated();
		// update our connectors
		UpdateConnectors();
	}

	virtual void OnExport()
	{
		Super::OnExport();
		for (INT idx = 0; idx < SequenceObjects.Num(); idx++)
		{
			SequenceObjects(idx)->OnExport();
		}
		for (INT idx = 0; idx < OutputLinks.Num(); idx++)
		{
			OutputLinks(idx).Links.Empty();
		}
		for (INT idx = 0; idx < VariableLinks.Num(); idx++)
		{
			VariableLinks(idx).LinkedVariables.Empty();
		}
		for (INT idx = 0; idx < EventLinks.Num(); idx++)
		{
			EventLinks(idx).LinkedEvents.Empty();
		}
	}

	virtual void UpdateConnectors();
	void UpdateNamedVarStatus();
	void UpdateInterpActionConnectors();

	/**
	 * Initialize this kismet sequence.
	 *  - Creates the kismet script log (if this sequence has no parent sequence)
	 *  - Registers all events with the objects that they're associated with.
	 *  - Resolves all "named" and "external" variable links contained by this sequence.
	 */
	virtual void InitializeSequence();

	/**
	 * Conditionally creates the log file for this sequence.
	 */
	virtual void CreateKismetLog();

	/**
	 * Called from level startup.  Initializes the sequence and activates any level-startup
	 * events contained within the sequence.
	 */
	virtual void BeginPlay();
	virtual void FinishDestroy();
	/**
	 * Activates LevelStartup and/or LevelBeginning events in this sequence
	 *
	 * @param bShouldActivateLevelStartupEvents If TRUE, will activate all LevelStartup events
	 * @param bShouldActivateLevelBeginningEvents If TRUE, will activate all LevelBeginning events
	 * @param bShouldActivateLevelLoadedEvents If TRUE, will activate all LevelLoadedAndVisible events
	 */
	virtual void NotifyMatchStarted(UBOOL bShouldActivateLevelStartupEvents=TRUE, UBOOL bShouldActivateLevelBeginningEvents=TRUE, UBOOL bShouldActivateLevelLoadedEvents=FALSE);

	void FindSeqObjectsByClass(UClass* DesiredClass, TArray<USequenceObject*>& OutputObjects, UBOOL bRecursive = TRUE) const;
	void FindSeqObjectsByName(const FString& Name, UBOOL bCheckComment, TArray<USequenceObject*>& OutputObjects, UBOOL bRecursive = TRUE) const;
	void FindSeqObjectsByObjectName(FName Name, TArray<USequenceObject*>& OutputObjects, UBOOL bRecursive = TRUE) const;
	void FindNamedVariables(FName VarName, UBOOL bFindUses, TArray<USequenceVariable*>& OutputVars, UBOOL bRecursive = TRUE) const;

	/**
	 * Finds all sequence objects contained by this sequence which are linked to the specified sequence object.
	 *
	 * @param	SearchObject		the sequence object to search for link references to
	 * @param	out_Referencers		if specified, receieves the list of sequence objects contained by this sequence
	 *								which are linked to the specified op
	 *
	 * @return	TRUE if at least one object in the sequence objects array is linked to the specified op.
	 */
	virtual UBOOL FindSequenceOpReferencers( USequenceObject* SearchObject, TArray<USequenceObject*>* out_Referencers=NULL );

	/**
	 * Returns a list of output links from this sequence's ops which reference the specified op.
	 *
	 * @param	SeqOp	the sequence object to search for output links to
	 * @param	Links	[out] receives the list of output links which reference the specified op.
	 */
	void FindLinksToSeqOp(USequenceOp* SeqOp, TArray<FSeqOpOutputLink*> &Links);

	USequence* GetPrefabsSequence();

	/**
	 * Determine if this sequence (or any of its subsequences) references a certain object.
	 *
	 * @param	InObject	the object to search for references to
	 * @param	pReferencer	if specified, will be set to the SequenceObject that is referencing the search object.
	 *
	 * @return TRUE if this sequence references the specified object.
	 */
	UBOOL ReferencesObject( const UObject* InObject, USequenceObject** pReferencer=NULL ) const;

	/**
	 * Determines whether the specified SequenceObject is contained in the SequenceObjects array of this sequence.
	 *
	 * @param	SearchObject	the sequence object to look for
	 * @param	bRecursive		specify FALSE to limit the search to this sequence only (do not search in sub-sequences as well)
	 *
	 * @return	TRUE if the specified sequence object was found in the SequenceObjects array of this sequence or one of its sub-sequences
	 */
	UBOOL ContainsSequenceObject( USequenceObject* SearchObject, UBOOL bRecursive=TRUE ) const;

	/**
	 * Returns whether this SequenceObject can exist in a sequence without being linked to anything else (i.e. does not require
	 * another sequence object to activate it)
	 */
	virtual UBOOL IsStandalone() const { return TRUE; }

	void ClearNameUsage(FName InName, ERenameFlags RenameFlags=REN_None);

	/** Iterate over all SequenceObjects in this Sequence, making sure that their ParentSequence pointer points back to this Sequence. */
	void CheckParentSequencePointers();

	/** Draws the this sequence. */
	virtual void DrawSequence(FCanvas* Canvas, TArray<USequenceObject*>& SelectedSeqObjs, USequenceObject* MouseOverSeqObj, INT MouseOverConnType, INT MouseOverConnIndex, FLOAT MouseOverTime, UBOOL bCurves);

	/**
	 * @return		The ULevel this sequence occurs in.
	 */
	ULevel* GetLevel() const;

	UBOOL IsEnabled() const;

	/** called when the level that contains this sequence object is being removed/unloaded */
	virtual void CleanUp();
};

/** Dedicated file log for tracking all script execution */
var const pointer LogFile;

/** List of all scripting objects contained in this sequence */
var const export array<SequenceObject> SequenceObjects;

/** List of all currently active sequence objects (events, latent actions, etc) */
var const array<SequenceOp> ActiveSequenceOps;

/**
 *	List of any nested sequences, to recursively execute in UpdateOp
 *	Do not rely on this in the editor - it is really built and accuracte only when play begins.
 */
var transient const array<Sequence> NestedSequences;

/** List of events that failed to register on first pass */
var const array<SequenceEvent> UnregisteredEvents;

/**
 * Used to save an op to activate and the impulse index.
 */
struct native ActivateOp
{
	/** the sequecne op that last activated the sequence op referenced by 'Op' */
	var	SequenceOp	ActivatorOp;
	/** Op pending activation */
	var SequenceOp Op;
	/** Input link idx to activate on Op */
	var int InputIdx;
	/** Remaining delay (for use with DelayedActivatedOps) */
	var float RemainingDelay;
};

/** List of impulses that are currently delayed */
var const array<ActivateOp> DelayedActivatedOps;

/** Is this sequence currently enabled? */
var() private{private} bool bEnabled;


/** Default position of origin when opening this sequence in Kismet. */
var	int		DefaultViewX;
var	int		DefaultViewY;
var float	DefaultViewZoom;

/**
 * Fills supplied array with all sequence objects of the specified type.
 */
native noexport final function FindSeqObjectsByClass( class<SequenceObject> DesiredClass, bool bRecursive, out array<SequenceObject> OutputObjects ) const;

/* Reset() - reset to initial state - used when restarting level without reloading */
function Reset()
{
	local int i;
	local SequenceOp Op;

	// pass to the SequenceOps we contain
	for (i = 0; i < SequenceObjects.length; i++)
	{
		Op = SequenceOp(SequenceObjects[i]);
		if (Op != None)
		{
			Op.Reset();
		}
	}
}

native final function SetEnabled(bool bInEnabled);

defaultproperties
{
	ObjName="Sequence"
	InputLinks.Empty
	OutputLinks.Empty
	VariableLinks.Empty

	DefaultViewZoom=1.0
	bEnabled=TRUE
}
