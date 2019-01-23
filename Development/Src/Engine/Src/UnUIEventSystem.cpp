/*=============================================================================
	UnUIEventSystem.cpp: UI event and action implementations.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"

#include "EngineSequenceClasses.h"
#include "EngineUISequenceClasses.h"

#include "UnUIMarkupResolver.h"

IMPLEMENT_CLASS(UUISequence);
	IMPLEMENT_CLASS(UUIStateSequence);

IMPLEMENT_CLASS(UUIAction);
	IMPLEMENT_CLASS(UUIAction_ActivateLevelEvent);
	IMPLEMENT_CLASS(UUIAction_ChangeState);
		IMPLEMENT_CLASS(UUIAction_ActivateState);
		IMPLEMENT_CLASS(UUIAction_DeactivateState);
	IMPLEMENT_CLASS(UUIAction_Scene);
		IMPLEMENT_CLASS(UUIAction_OpenScene);
		IMPLEMENT_CLASS(UUIAction_CloseScene);
	IMPLEMENT_CLASS(UUIAction_DataStore);
		IMPLEMENT_CLASS(UUIAction_PublishValue);
		IMPLEMENT_CLASS(UUIAction_RefreshBindingValue);
		IMPLEMENT_CLASS(UUIAction_GetCellValue);
		IMPLEMENT_CLASS(UUIAction_DataStoreField);
			IMPLEMENT_CLASS(UUIAction_SetDatafieldValue);
			IMPLEMENT_CLASS(UUIAction_GetDatafieldValue);
			IMPLEMENT_CLASS(UUIAction_AddDataField);
	IMPLEMENT_CLASS(UUIAction_GetValue);
	IMPLEMENT_CLASS(UUIAction_GetListIndex);
	IMPLEMENT_CLASS(UUIAction_ShowLoginUI);
	IMPLEMENT_CLASS(UUIAction_ShowKeyboardUI);
	IMPLEMENT_CLASS(UUIAction_ShowDeviceSelectionUI);
	IMPLEMENT_CLASS(UUIAction_PlayUISoundCue);
	IMPLEMENT_CLASS(UUIAction_MoveListItem);
	IMPLEMENT_CLASS(UUIAction_JoinOnlineGame);
	IMPLEMENT_CLASS(UUIAction_CreateOnlineGame);
	IMPLEMENT_CLASS(UUIAction_FindOnlineGames);
	IMPLEMENT_CLASS(UUIAction_ShowGamercardForServerHost);
	IMPLEMENT_CLASS(UUIAction_RefreshStats);
	IMPLEMENT_CLASS(UUIAction_SaveProfileSettings);
	IMPLEMENT_CLASS(UUIAction_GetNATType);
	IMPLEMENT_CLASS(UUIAction_GetLoggedInPlayerCount);

IMPLEMENT_CLASS(UUIEvent);
	IMPLEMENT_CLASS(UUIEvent_MetaObject);
	IMPLEMENT_CLASS(UUIEvent_Scene);
	IMPLEMENT_CLASS(UUIEvent_State);
		IMPLEMENT_CLASS(UUIEvent_OnEnterState);
		IMPLEMENT_CLASS(UUIEvent_OnLeaveState);
	IMPLEMENT_CLASS(UUIEvent_ProcessInput);
	IMPLEMENT_CLASS(UUIEvent_OnClick);
	IMPLEMENT_CLASS(UUIEvent_OnDoubleClick);
	IMPLEMENT_CLASS(UUIEvent_ValueChanged);
		IMPLEMENT_CLASS(UUIEvent_SliderValueChanged);
		IMPLEMENT_CLASS(UUIEvent_ProgressBarValueChanged);
		IMPLEMENT_CLASS(UUIEvent_TextValueChanged);
		IMPLEMENT_CLASS(UUIEvent_CheckValueChanged);
		IMPLEMENT_CLASS(UUIEvent_ListIndexChanged);
		IMPLEMENT_CLASS(UUIEvent_ComboboxValueChanged);
		IMPLEMENT_CLASS(UUIEvent_OptionListValueChanged);
		IMPLEMENT_CLASS(UUIEvent_NumericOptionListValueChanged);
	IMPLEMENT_CLASS(UUIEvent_SubmitData);
		IMPLEMENT_CLASS(UUIEvent_SubmitTextData);
		IMPLEMENT_CLASS(UUIEvent_SubmitListData);
	IMPLEMENT_CLASS(UUIEvent_TabControl);

IMPLEMENT_CLASS(USeqVar_UIRange);

IMPLEMENT_CLASS(UUIEventContainer);

/* ==========================================================================================================
	UISequence
========================================================================================================== */
/* === UUISequence interface === */

/**
 * Adds a new sequence to this sequence's NestedSequences array, calling Modify() on this sequence.
 *
 * @param	NewNestedSequence	the sequence to add to the NestedSequence array
 *
 * @return	TRUE if the new sequence was successfully added to the NestedSequences list, or if was already in that list.
 */
UBOOL UUISequence::AddNestedSequence( USequence* NewNestedSequence )
{
	UBOOL bResult = FALSE;
	if ( NewNestedSequence != NULL )
	{
		if ( NestedSequences.ContainsItem(NewNestedSequence) )
		{
			bResult = TRUE;
		}
		else
		{
			Modify(TRUE);
			if ( AddSequenceObject(NewNestedSequence) )
			{
				NestedSequences.AddItem(NewNestedSequence);
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

/**
 * Removes the specified sequence from this sequences list of NestedSequences
 *
 * @param	SequenceToRemove	the sequence to remove
 *
 * @return	TRUE if the sequence was successfully removed from the NestedSequences list, or if it didn't exist in the list.
 */
UBOOL UUISequence::RemoveNestedSequence( USequence* SequenceToRemove )
{
	UBOOL bResult = FALSE;

	if ( SequenceToRemove != NULL )
	{
		INT SeqIndex = NestedSequences.FindItemIndex(SequenceToRemove);
		if ( SeqIndex != INDEX_NONE )
		{
			//@todo - what happens if the sequence has active ops pending??
			Modify(TRUE);
			NestedSequences.Remove(SeqIndex);
			SequenceObjects.RemoveItem(SequenceToRemove);
			bResult = TRUE;
		}
	}
	return bResult;
}

static const EObjectFlags UnreferencedObjectFlags = RF_NotForServer|RF_NotForClient;

/**
 * Determines whether this sequence contains any ops that can execute logic during the game.  Called when this sequence is saved
 * to determine whether it should be marked to be loaded in the game.  Marks any ops which aren't linked to other ops so that they
 * aren't loaded in the game.
 *
 * @return	returns TRUE if this sequence contains ops that execute logic, thus needs to be loaded in the game.
 */
UBOOL UUISequence::CalculateSequenceLoadFlags()
{
	debugfSlow(NAME_DevUI, TEXT("Calculating sequence load flags for sequence '%s'"), *GetFullName());

	// we must allow all sequences contained by prefabs to be loaded, even if don't contain ops.  This is because users can add ops to
	// instances of this sequence so it must always be able to load
	UUIScreenObject* OwningWidget = GetOwner();
	const UBOOL bIsPrefabSequence = IsAPrefabArchetype() || (OwningWidget != NULL && OwningWidget->IsA(UUIPrefab::StaticClass()));

	UBOOL bHasActiveOps = bIsPrefabSequence;
	UBOOL bHasActiveStateSequences = FALSE;

	// first, reset the load flags for all sequence objects
	ClearFlags(UnreferencedObjectFlags|RF_LoadForClient|RF_LoadForServer|RF_Marked);
	for ( INT ObjIndex = SequenceObjects.Num() - 1; ObjIndex >= 0; ObjIndex-- )
	{
		USequenceObject* SeqObj = SequenceObjects(ObjIndex);
		SeqObj->ClearFlags(UnreferencedObjectFlags|RF_LoadForServer|RF_LoadForClient|RF_Marked);
	}

	// tracks the non-standalone objects that are referenced by other ops; once we've marked 
	TArray<USequenceObject*> ReferencedObjects;
	for ( INT ObjIndex = SequenceObjects.Num() - 1; ObjIndex >= 0; ObjIndex-- )
	{
		USequenceObject* SeqObj = SequenceObjects(ObjIndex);
		if ( SeqObj != NULL )
		{
			if ( !SeqObj->HasAnyFlags(RF_Marked) )
			{
				// reset the load flags for this object
				if ( IsStandalone(SeqObj) )
				{
					SeqObj->SetFlags(RF_Marked);

					// events must be linked to other ops to be considered "active"
					USequenceEvent* Event = Cast<USequenceEvent>(SeqObj);
					if ( Event != NULL )
					{
						if ( !bIsPrefabSequence && !Event->HasLinkedOps() )
						{
							debugfSlow(NAME_DevUI, TEXT("\tMarking event %s to not load in game"), *SeqObj->GetFullName());
							Event->SetFlags(UnreferencedObjectFlags);
						}
						else
						{
							bHasActiveOps = TRUE;
						}
					}
					else
					{
						// if this is a nested sequence, tell it to calculate its load flags as well
						UUISequence* NestedSeq = Cast<UUISequence>(SeqObj);
						if ( NestedSeq != NULL )
						{
							UBOOL bNestedSequenceHasActiveOps = NestedSeq->CalculateSequenceLoadFlags();
							if ( !bHasActiveStateSequences && bNestedSequenceHasActiveOps )
							{
								// if we have state sequences in the list of SequenceObjects, it means that this sequence is a global sequence for a widget;
								// if any of those state sequences need to be loaded, we'll need to load the global sequence as well (we don't necessarily
								// need to load the global sequence if child widgets have active sequences, though)

								// (in the editor, we should never actually encounter this case, since state sequences aren't added to the SequenceObjects list
								bHasActiveStateSequences = Cast<UUIStateSequence>(NestedSeq) != NULL;
							}
						}
						else
						{
							// this is one of the other standalone types; this sequence will need to be loaded
							bHasActiveOps = TRUE;
						}
					}
				}
				else if ( bIsPrefabSequence )
				{
					SeqObj->SetFlags(RF_Marked);
				}
				// IsSequenceObjectRooted should set the RF_Marked flag on the sequence object
				else if ( !IsSequenceObjectRooted(SeqObj) )
				{
					// this op isn't a valid stand-alone op and isn't referenced by any other stand-alone ops ops, so don't load it during the game.
					debugfSlow(NAME_DevUI, TEXT("\tMarking sequence object %s to not load in game"), *SeqObj->GetFullName());
					SeqObj->SetFlags(UnreferencedObjectFlags);
				}

				checkSlow(SeqObj->HasAnyFlags(RF_Marked));
			}
		}
		else
		{
			SequenceObjects.Remove(ObjIndex);
		}
	}

	SetFlags(RF_Marked);
	UUIScreenObject* OwnerWidget = GetOwner();

	// sanity checks
	check(OwnerWidget);
	check(OwnerWidget->EventProvider);
	check(OwnerWidget->EventProvider->EventContainer);

	if ( OwnerWidget->EventProvider->EventContainer == this )
	{
		// if this is the widget's global sequence, check all state sequences for the widget
		for ( INT StateIndex = 0; StateIndex < OwnerWidget->InactiveStates.Num(); StateIndex++ )
		{
			UUIState* CurrentState = OwnerWidget->InactiveStates(StateIndex);
			if ( CurrentState != NULL )
			{
				// if the state has a sequence and the state's sequence wasn't part of our SequenceObjects array (this is normally the case in the editor),
				// give the state sequence the opportunity to mark its SequenceObjects now; [if it was contained in the SequenceObjects array, CalculateSequenceLoadFlags
				// has already been called on it
				if ( CurrentState->StateSequence != NULL && !SequenceObjects.ContainsItem(CurrentState->StateSequence) )
				{
					bHasActiveStateSequences = CurrentState->StateSequence->CalculateSequenceLoadFlags() || bHasActiveStateSequences;
				}
			}
		}

		// if this is a widget's global sequence and it has state sequences that need to be loaded, load the global sequence as well
		// and the global sequence for a scene always needs to be loaded
		bHasActiveOps = bHasActiveOps || bHasActiveStateSequences || Cast<UUIScene>(OwnerWidget) != NULL;
	}

	if ( !bHasActiveOps )
	{
		debugfSlow(NAME_DevUI, TEXT("\tMarking Sequence %s to not load in game"), *GetFullName());
		SetFlags(UnreferencedObjectFlags);
	}

	return bHasActiveOps;
}

/**
 * Determines whether this non-standalone object is referenced by a stand-alone sequence object.  This method assumes that all sequence objects
 * have previously had the RF_NotForClient|RF_NotForServer flags cleared on them prior to evaluating any sequence objects.
 *
 * @param	SeqObj	the sequence object to check
 *
 * @return	TRUE if SeqObj is a stand-alone sequence object or is referenced [directly or indirectly] by a standalone sequence object
 */
UBOOL UUISequence::IsSequenceObjectRooted( USequenceObject* SeqObj )
{
	UBOOL bResult = FALSE;

	if ( SeqObj != NULL )
	{
		if ( SeqObj->HasAnyFlags(RF_Marked) )
		{
			// if this object has already been evaluated and it is marked to be loaded in the game, then it was already determined to be rooted 
			bResult = !SeqObj->HasAllFlags(UnreferencedObjectFlags);
		}
		else if ( IsStandalone(SeqObj) )
		{
			// if this object is a standalone object, it is considered rooted, but we don't mark it as evaluated because CalculateSequenceLoadFlags
			// might need to execute other logic on this object
			bResult = TRUE;
		}
		else
		{
			// mark this object as having been evaluated
			SeqObj->SetFlags(RF_Marked);

			TArray<USequenceObject*> Referencers;
			if ( FindSequenceOpReferencers(SeqObj, &Referencers) )
			{
				// for each object referencing this sequence object, determine whether the reference should cause this sequence object to be loaded in game
				for ( INT ReferencerIndex = 0; ReferencerIndex < Referencers.Num(); ReferencerIndex++ )
				{
					USequenceObject* Referencer = Referencers(ReferencerIndex);
					if ( IsSequenceObjectRooted(Referencer) )
					{
						bResult = TRUE;
						break;
					}
				}
			}
		}
	}

	return bResult;
}

/**
 * Return the UIScreenObject that owns this sequence.
 */
UUIScreenObject* UUISequence::GetOwner() const
{
	UUIScreenObject* Result=NULL;

	for ( UObject* Owner = GetOuter(); Owner && Result == NULL; Owner = Owner->GetOuter() )
	{
		Result = Cast<UUIScreenObject>(Owner);
	}

	return Result;
}

/**
 * Retrieves the UIEvents contained by this container.
 *
 * @param	out_Events	will be filled with the UIEvent instances stored in by this container
 * @param	LimitClass	if specified, only events of the specified class (or child class) will be added to the array
 */
void UUISequence::GetUIEvents( TArray<UUIEvent*>& out_Events, UClass* LimitClass/*=NULL*/ )
{
	// the UIEvents array is only valid after InitializeSequence has been called, which is only called in the game
	if ( GIsGame )
	{
		if ( LimitClass == NULL )
		{
			out_Events += UIEvents;
		}
		else
		{
			check(LimitClass->IsChildOf(UUIEvent::StaticClass()));

			for ( INT EventIndex = 0; EventIndex < UIEvents.Num(); EventIndex++ )
			{
				UUIEvent* Event = UIEvents(EventIndex);
				if ( Event->IsA(LimitClass) )
				{
					out_Events.AddUniqueItem(Event);
				}
			}
		}
	}
	else
	{
		if ( LimitClass == NULL )
		{
			LimitClass = UUIEvent::StaticClass();
		}
		FindSeqObjectsByClass(LimitClass, (TArray<USequenceObject*>&)out_Events, FALSE);
	}
}

/* === USequence interface === */
/**
 * Initialize this kismet sequence.
 *  - Adds all UIEvents to the UIEvents list.
 */
void UUISequence::InitializeSequence()
{
	Super::InitializeSequence();

	// clear any existing entries
	UIEvents.Empty();

	// add all UIEvent objects contained in the SequenceObjects array to the UIEvents array for fast lookup
	FindSeqObjectsByClass( UUIEvent::StaticClass(), (TArray<USequenceObject*>&)UIEvents, FALSE );
}

/**
 * Conditionally creates the log file for this sequence.
 */
void UUISequence::CreateKismetLog()
{
#if !NO_LOGGING && !FINAL_RELEASE
	// Only create log file if we're not a nested USequence Object..,
	if( ParentSequence == NULL 
	// ... and kismet logging is enabled.
	&& GEngine->bEnableKismetLogging )
	{
		// Create the script log archive if necessary.
		if( LogFile == NULL )
		{
			UUIScreenObject* OwnerWidget = GetOwner();
			check(OwnerWidget);

			UUIScene* OwnerScene = OwnerWidget->GetScene();
			check(OwnerScene);

			FString Filename = FString::Printf(TEXT("%sUIKismet_%s.log"), *appGameLogDir(), *OwnerScene->GetTag().ToString());
			LogFile = new FOutputDeviceFile( *Filename );
			KISMET_LOG(TEXT("Opened UI Kismet log '%s'..."), *Filename);
		}
	}
#endif
}

/**
 * Adds a new SequenceObject to this containers's list of ops.
 *
 * @param	NewObj		the sequence object to add.
 * @param	bRecurse	if TRUE, recursively add any sequence objects attached to this one
 *
 * @return	TRUE if the object was successfully added to the sequence.
 *
 * @note: this implementation is necessary to fulfill the UIEventContainer interface contract
 */
UBOOL UUISequence::AddSequenceObject( USequenceObject* NewObj, UBOOL bRecurse/*=FALSE*/ )
{
	UBOOL bResult = Super::AddSequenceObject(NewObj, bRecurse);

	if ( bResult == TRUE )
	{
		UUIEvent* EventObject = Cast<UUIEvent>(NewObj);
		if ( EventObject != NULL && !EventObject->HasAnyFlags(RF_Transient) && !UIEvents.ContainsItem(EventObject) )
		{
			UIEvents.AddUniqueItem(EventObject);
		}
	}

	return bResult;
}

void UUISequence::execAddSequenceObject( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(USequenceObject,NewObj);
	P_FINISH;
	*(UBOOL*)Result=AddSequenceObject(NewObj);
}

/**
 * Removes the specified object from the SequenceObjects array, severing any links to that object.
 *
 * @param	ObjectToRemove	the SequenceObject to remove from this sequence.  All links to the object will be cleared.
 * @param	ModifiedObjects	a list of objects that have been modified the objects that have been
 */
void UUISequence::RemoveObject( USequenceObject* ObjectToRemove )
{
	Super::RemoveObject(ObjectToRemove);

	UUIEvent* EventToRemove = Cast<UUIEvent>(ObjectToRemove);
	if ( EventToRemove != NULL )
	{
		UIEvents.RemoveItem(EventToRemove);
	}
}

/**
 * Removes the specified SequenceObject from this container's list of ops.
 *
 * @param	ObjectToRemove	the sequence object to remove
 */
void UUISequence::RemoveSequenceObject( USequenceObject* ObjectToRemove )
{
	RemoveObject(ObjectToRemove);
}

/**
 * Removes the specified SequenceObjects from this container's list of ops.
 *
 * @param	ObjectsToRemove		the objects to remove from this sequence
 */
void UUISequence::RemoveSequenceObjects( const TArray<USequenceObject*>& ObjectsToRemove )
{
	RemoveObjects(ObjectsToRemove);
}

/* === USequenceObject interface === */
/**
 * Returns whether the specified SequenceObject can exist in this sequence without being linked to anything else (i.e. does not require
 * another sequence object to activate it).
 *
 * @param	SeqObj	the sequence object to check
 *
 * @return	TRUE if the sequecne object does not require a separate "stand-alone" sequence object to reference it, in order to be loaded in game.
 */
UBOOL UUISequence::IsStandalone( USequenceObject* SeqObj ) const
{
	return SeqObj != NULL && SeqObj->IsStandalone();
}

/* === UObject interface === */
/**
 * This version removes the RF_Public flag if it exists on a non-prefab sequence.
 */
void UUISequence::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerVersion() < VER_MATINEE_MOVEMENT_LOOKUP_TRACK_IMPLEMENTED )
	{
		if ( HasAnyFlags(RF_Public) )
		{
			UUIScreenObject* Owner = GetOwner();
			if ( Owner != NULL && !Owner->IsTemplate(RF_ClassDefaultObject|RF_ArchetypeObject) && !Owner->IsInUIPrefab() )
			{
				// this sequence shouldn't have been marked public
				ClearFlags(RF_Public);
			}
		}
	}
}

/**
 * Called after importing property values for this object (paste, duplicate or .t3d import)
 * Allow the object to perform any cleanup for properties which shouldn't be duplicated or
 * are unsupported by the script serialization.
 *
 * This version clears the value of ParentSequence since we want this to be set when the owning widget that was just
 * pasted is initialized.
 */
void UUISequence::PostEditImport()
{
	Super::PostEditImport();

	// we don't include subsequences when exporting widget sequences, so clear the ParentSequence value so
	// that when the owning widget is initialized, its sequence will be bound to the correct parent sequence
	ParentSequence = NULL;
}

/**
 * Determines whether this object is contained within a UPrefab.
 *
 * @param	OwnerPrefab		if specified, receives a pointer to the owning prefab.
 *
 * @return	TRUE if this object is contained within a UPrefab; FALSE if it IS a UPrefab or not contained within one.
 */
UBOOL UUISequence::IsAPrefabArchetype( UObject** OwnerPrefab/*=NULL*/ ) const
{
	UBOOL bResult = FALSE;

	UUIScreenObject* OwningWidget = GetOwner();
	if ( OwningWidget != NULL )
	{
		bResult = OwningWidget->IsAPrefabArchetype(OwnerPrefab);
	}
	else
	{
		bResult = Super::IsAPrefabArchetype(OwnerPrefab);
	}
	return bResult;
}

/**
 * @return	TRUE if the object is contained within a UIPrefabInstance.
 */
UBOOL UUISequence::IsInPrefabInstance() const
{
	UBOOL bResult = FALSE;

	UUIScreenObject* WidgetOwner = GetOwner();
	if ( WidgetOwner != NULL )
	{
		bResult = WidgetOwner->IsInPrefabInstance();
	}
	else
	{
		bResult = Super::IsInPrefabInstance();
	}

	return bResult;
}

/* ==========================================================================================================
	UUIStateSequence
========================================================================================================== */
/**
 * Returns the UIState that created this UIEventSequence.
 */
UUIState* UUIStateSequence::GetOwnerState() const
{
	UUIState* Result=NULL;

	for ( UObject* Owner = GetOuter(); Owner && Result == NULL; Owner = Owner->GetOuter() )
	{
		Result = Cast<UUIState>(Owner);
	}

	return Result;
}

/**
 * Returns the index [into the SequenceObjects array] of the UIEvent_MetaObject used to represent the owner state's
 * custom input actions in the UI kismet editor, or INDEX_NONE if it can't be found.
 */
INT UUIStateSequence::FindInputMetaObjectIndex() const
{
	INT Result = INDEX_NONE;

	for ( INT SeqObjIndex = 0; SeqObjIndex < SequenceObjects.Num(); SeqObjIndex++ )
	{
		USequenceObject* SeqObj = SequenceObjects(SeqObjIndex);
		if ( SeqObj->IsA(UUIEvent_MetaObject::StaticClass()) )
		{
			Result = SeqObjIndex;
			break;
		}
	}

	return Result;
}

/**
 * Creates a new UIEvent_InputMetaObject that will be used to display all of the input events for this state sequence, if one does not already exist
 * in this sequence's SequenceObjects array.  Initializes the input meta object with the data from the owning state's custom input actions.
 */
void UUIStateSequence::InitializeInputMetaObject()
{
	UUIEvent_MetaObject* MetaObject = NULL;

	INT ExistingMetaObjectIndex = FindInputMetaObjectIndex();
	if ( SequenceObjects.IsValidIndex(ExistingMetaObjectIndex) )
	{
		MetaObject = CastChecked<UUIEvent_MetaObject>(SequenceObjects(ExistingMetaObjectIndex));
	}
	else
	{
		FName MetaObjectName = *FString::Printf(TEXT("%s_MetaObject"), *GetName());
		MetaObject = ConstructObject<UUIEvent_MetaObject>( UUIEvent_MetaObject::StaticClass(), this,
			MetaObjectName, RF_Transient|GetMaskedFlags(RF_PropagateToSubObjects));

		MetaObject->OnCreated();
		AddSequenceObject(MetaObject);
	}

	check(MetaObject);
	MetaObject->RegisterCallbacks();
	MetaObject->ReadInputActionsFromState();
}

/**
 * Copies all values from the input meta object into the owning state's custom input actions, then removes the meta object from the sequence's
 * list of SequenceObjects.
 */
void UUIStateSequence::SaveInputMetaObject()
{
	UUIEvent_MetaObject* MetaObject = NULL;

	INT ExistingMetaObjectIndex = FindInputMetaObjectIndex();
	if ( SequenceObjects.IsValidIndex(ExistingMetaObjectIndex) )
	{
		MetaObject = CastChecked<UUIEvent_MetaObject>(SequenceObjects(ExistingMetaObjectIndex));
		MetaObject->SaveInputActionsToState();

		// temporarily remove the bDeletable flag so that it can actually be removed
		MetaObject->bDeletable = TRUE;
		RemoveSequenceObject(MetaObject);
	}
}

/**
 * Determines whether the specified sequence action is referenced by the owning state's StateInputActions array.
 */
UBOOL UUIStateSequence::IsStateInputAction( USequenceAction* SeqAction ) const
{
	UBOOL bResult = FALSE;
	if ( SeqAction != NULL )
	{
		UUIState* OwnerState = GetOwnerState();
		if ( OwnerState != NULL )
		{
			// if this state's sequence has actions assigned to raw input keys, make sure those actions can be loaded in the game.
			for ( INT InputKeyIndex = 0; InputKeyIndex < OwnerState->StateInputActions.Num(); InputKeyIndex++ )
			{
				const FInputKeyAction& InputAlias = OwnerState->StateInputActions(InputKeyIndex);
				if ( InputAlias.ActionsToExecute.FindItemIndex(SeqAction) != INDEX_NONE )
				{
					bResult = TRUE;
					break;
				}
			}
		}
	}
	return bResult;
}

/**
 * Adds the specified SequenceOp to the list of ActiveOps in the owning widget's global sequence
 *
 * @param	NewSequenceOp	the sequence op to add to the list
 * @param	bPushTop		if TRUE, adds the operation to the top of stack (meaning it will be executed first),
 *							rather than the bottom
 *
 * @return	TRUE if the sequence operation was successfully added to the list.
 */
UBOOL UUIStateSequence::QueueSequenceOp( USequenceOp* NewSequenceOp, UBOOL bPushTop/*=FALSE*/ )
{
	UBOOL bResult = FALSE;

	// we should always have a parent sequence
	if ( ParentSequence != NULL )
	{
		bResult = ParentSequence->QueueSequenceOp(NewSequenceOp, bPushTop);
	}
	else
	{
		// but in case we don't, handle it ourselves
		bResult = Super::QueueSequenceOp(NewSequenceOp, bPushTop);
	}

	return bResult;
}

/**
 * Returns whether the specified SequenceObject can exist in this sequence without being linked to anything else (i.e. does not require
 * another sequence object to activate it).  This version also considers actions which are referenced only through the state's StateInputActions array
 * to be standalone ops.
 *
 * @param	SeqObj	the sequence object to check
 *
 * @return	TRUE if the sequecne object does not require a separate "stand-alone" sequence object to reference it, in order to be loaded in game.
 */
UBOOL UUIStateSequence::IsStandalone( USequenceObject* SeqObj ) const
{
	UBOOL bResult = Super::IsStandalone(SeqObj);
	if ( !bResult )
	{
		bResult = IsStateInputAction( Cast<USequenceAction>(SeqObj) );
	}
	return bResult;
}

/**
 * Finds all sequence objects contained by this sequence which are linked to the specified sequence object.  This version
 * also checks the owning state's input array to determine whether the specified op is referenced.
 *
 * @param	SearchObject		the sequence object to search for link references to
 * @param	out_Referencers		if specified, receives the list of sequence objects contained by this sequence
 *								which are linked to the specified op
 *
 * @return	TRUE if at least one object in the sequence objects array is linked to the specified op.
 */
UBOOL UUIStateSequence::FindSequenceOpReferencers( USequenceObject* SearchObject, TArray<USequenceObject*>* out_Referencers/*=NULL*/ )
{
	UBOOL bResult = Super::FindSequenceOpReferencers(SearchObject, out_Referencers);

	// we only need to execute this code if we haven't found a reference by now, or if the caller wants to know which ops are referencing the SearchObject
	if ( !bResult || out_Referencers != NULL )
	{
		// see if we're referencing this object through our owner state's StateInputActions array
		// if this state's sequence has actions assigned to raw input keys, make sure those actions can be loaded in the game.
		USequenceAction* SearchAction = Cast<USequenceAction>(SearchObject);
		if ( SearchAction != NULL && IsStateInputAction(SearchAction) )
		{
			if ( out_Referencers != NULL )
			{
				out_Referencers->AddUniqueItem(this);
			}

			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Presave function. Gets called once before an object gets serialized for saving. This function is necessary
 * for save time computation as Serialize gets called three times per object from within UObject::SavePackage. 
 * @warning: Objects created from within PreSave will NOT have PreSave called on them!!!
 *
 * This version of the function pushes the input meta object's inputs created back to the parent state's event array.
 */
void UUIStateSequence::PreSave()
{
	SaveInputMetaObject();
	Super::PreSave();
}


/**
 * Called just before a property in this object's archetype is to be modified, prior to serializing this object into
 * the archetype propagation archive.
 *
 * Allows objects to perform special cleanup or preparation before being serialized into an FArchetypePropagationArc
 * against its archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.
 *
 * This version saves the data from the meta object into the owning state and removes the meta object from the SequenceObjects array.
 */
void UUIStateSequence::PreSerializeIntoPropagationArchive()
{
	Super::PreSerializeIntoPropagationArchive();

	// save the data from the input meta object and remove it from the SequenceObjects array.
	SaveInputMetaObject();
}

/**
 * Called just after a property in this object's archetype is modified, immediately after this object has been de-serialized
 * from the archetype propagation archive.
 *
 * Allows objects to perform reinitialization specific to being de-serialized from an FArchetypePropagationArc and
 * reinitialized against an archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.
 *
 * This version re-creates and re-initializes the meta object from the data in the owning state's list of input actions.
 */
void UUIStateSequence::PostSerializeFromPropagationArchive()
{
	// re-initialize the input meta object
	InitializeInputMetaObject();

	Super::PostSerializeFromPropagationArchive();
}

/* ==========================================================================================================
	USeqVar_UIRange
========================================================================================================== */
/**
 * Retrieve a list of UIRangeData values connected to this sequence op.
 *
 * @param	out_UIRanges	receieves the list of UIRangeData values
 * @param	inDesc			if specified, only UIRangeData values connected via a variable link that this name will be returned.
 */
void USequenceOp::GetUIRangeVars( TArray<FUIRangeData*>& out_UIRanges, const TCHAR* inDesc/*=NULL*/ )
{
	// search for all variables of the expected type
	for (INT Idx = 0; Idx < VariableLinks.Num(); Idx++)
	{
		FSeqVarLink& VarLink = VariableLinks(Idx);

		// if no desc requested, or matches requested desc
		if ( inDesc == NULL || *inDesc == 0 || VarLink.LinkDesc == inDesc )
		{
			if ( VarLink.SupportsVariableType(USeqVar_UIRange::StaticClass(),FALSE) )
			{
				// add the refs to out list
				for (INT LinkIdx = 0; LinkIdx < VarLink.LinkedVariables.Num(); LinkIdx++)
				{
					if ( VarLink.LinkedVariables(LinkIdx) != NULL)
					{
						FUIRangeData* UIRangeValue = VarLink.LinkedVariables(LinkIdx)->GetUIRangeRef();
						if (UIRangeValue != NULL)
						{
							out_UIRanges.AddItem(UIRangeValue);
						}
					}
				}
			}
		}
	}
}

/**
 * Returns a pointer to the UIRangeData value contained by this sequence variable
 */
FUIRangeData* USeqVar_UIRange::GetUIRangeRef()
{
	return &RangeValue;
}

/**
 * Returns a string representation of the value of this sequence variable
 */
FString USeqVar_UIRange::GetValueStr()
{
	return FString::Printf(
		TEXT("%.2f (%.2f/%.2f)"),
		RangeValue.GetCurrentValue(),
		RangeValue.MinValue,
		RangeValue.MaxValue
		);
}

/**
 * Determines whether the specified property can be linked to this sequence variable.
 */
UBOOL USeqVar_UIRange::SupportsProperty(UProperty *Property)
{
	UBOOL bResult = FALSE;

	UStructProperty* StructProp = Cast<UStructProperty>(Property,CLASS_IsAUStructProperty);
	if ( StructProp == NULL )
	{
		UArrayProperty* ArrayProp = Cast<UArrayProperty>(Property);
		if ( ArrayProp != NULL )
		{
			StructProp = Cast<UStructProperty>(ArrayProp->Inner,CLASS_IsAUStructProperty);
		}
	}

	if ( StructProp != NULL )
	{
		static UScriptStruct* UIRangeStruct = FindObject<UScriptStruct>(UUIRoot::StaticClass(), TEXT("UIRangeData"));
		checkf(UIRangeStruct!=NULL,TEXT("Unable to find UIRangeData struct within UIRoot!"));

		bResult = (StructProp->Struct->IsChildOf(UIRangeStruct));
	}

	return bResult;
}

/**
 * Copies the value stored by this SequenceVariable to the SequenceOp member variable that it's associated with.
 */
void USeqVar_UIRange::PublishValue(USequenceOp *Op, UProperty *Property, FSeqVarLink &VarLink)
{
	if (Op != NULL && Property != NULL && SupportsProperty(Property) )
	{
		TArray<FUIRangeData*> UIRangeVars;
		Op->GetUIRangeVars(UIRangeVars,*VarLink.LinkDesc);

		// dealing with a non-array or static array variable; the call to SupportsProperty at the top of this block guarantees
		// that we're dealing with a struct property of the correct type.
		if ( Property->IsA(UStructProperty::StaticClass()) )
		{
			// copy the value of the UIRangeDatas connected via the op's variable links to the address space of the member UIRangeData property
			// if dealing with a static array, attempt to copy as many values as possible; otherwise, we'll be copying only the first value
			for ( INT ArrayIdx = 0; ArrayIdx < Property->ArrayDim && ArrayIdx < UIRangeVars.Num(); ArrayIdx++ )
			{
				*(FUIRangeData*)((BYTE*)Op + Property->Offset + ArrayIdx * Property->ElementSize) = *UIRangeVars(ArrayIdx);
			}
		}
		else
		{
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Property);
			if ( ArrayProp != NULL )
			{
				// dealing with an array of UIRangeData values.... the call to SupportsProperty at the top of this block guarantees
				// that we're dealing with an array of the correct type.

				// grab the array
				INT ElementSize = ArrayProp->Inner->ElementSize;
				FArray* Array = (FArray*)((BYTE*)Op + ArrayProp->Offset);

				// resize it to fit the variable count
				Array->Empty(ElementSize, DEFAULT_ALIGNMENT, UIRangeVars.Num());
				Array->AddZeroed(UIRangeVars.Num(), ElementSize, DEFAULT_ALIGNMENT);
				for (INT Idx = 0; Idx < UIRangeVars.Num(); Idx++)
				{
					// assign to the array entry
					*(FUIRangeData*)((BYTE*)Array->GetData() + Idx * ElementSize) = *UIRangeVars(Idx);
				}
			}
		}
	}
}

/**
 * Copy the value from the member variable this VariableLink is associated with to this VariableLink's value.
 */
void USeqVar_UIRange::PopulateValue(USequenceOp *Op, UProperty *Property, FSeqVarLink &VarLink)
{
	if ( Op != NULL && Property != NULL && SupportsProperty(Property) )
	{
		TArray<FUIRangeData*> UIRangeVars;
		Op->GetUIRangeVars(UIRangeVars,*VarLink.LinkDesc);

		// dealing with a non-array or static array variable; the call to SupportsProperty at the top of this block guarantees
		// that we're dealing with a struct property of the correct type.
		if ( Property->IsA(UStructProperty::StaticClass()) )
		{
			INT ArrayIdx = 0;

			// copy the UIRangeData value from this seqvar's member property into the SeqVar_UIRange variables connected to the op
			// if dealing with a static array, attempt to copy as many values as possible; otherwise, we'll be copying only the first value
			for ( ;ArrayIdx < Property->ArrayDim && ArrayIdx < UIRangeVars.Num(); ArrayIdx++ )
			{
				*UIRangeVars(ArrayIdx) = *(FUIRangeData*)((BYTE*)Op + Property->Offset + ArrayIdx * Property->ElementSize);
			}

			// if we have more connected sequence variables than we have member values, then copy the value of the first element of our member property
			// into the remaining connected sequence varables
			FUIRangeData* DefaultRangeValue = (FUIRangeData*)((BYTE*)Op + Property->Offset);
			while ( ArrayIdx < UIRangeVars.Num() )
			{
				*UIRangeVars(ArrayIdx) = *DefaultRangeValue;
			}
		}
		else
		{
			UArrayProperty* ArrayProp = Cast<UArrayProperty>(Property);
			if ( ArrayProp != NULL )
			{
				// dealing with an array of UIRangeData values.... the call to SupportsProperty at the top of this block guarantees
				// that we're dealing with an array of the correct type.

				// grab the array
				INT ElementSize = ArrayProp->Inner->ElementSize;
				FArray* Array = (FArray*)((BYTE*)Op + ArrayProp->Offset);

				// write out as many entries as are attached
				for (INT ArrayIdx = 0; ArrayIdx < UIRangeVars.Num() && ArrayIdx < Array->Num(); ArrayIdx++)
				{
					*(UIRangeVars(ArrayIdx)) = *(FUIRangeData*)((BYTE*)Array->GetData() + ArrayIdx * ElementSize);
				}
			}
		}
	}
}

/* ==========================================================================================================
	UUIEvent
========================================================================================================== */
/**
 * Returns the widget that contains this UIAction.
 *
 * @return	a pointer to the widget that contains the sequence this action belongs to.
 */
UUIScreenObject* UUIEvent::GetOwner() const
{
	UUIScreenObject* Result = NULL;

	UUISequence* OwnerSequence = Cast<UUISequence>(ParentSequence);
	if ( OwnerSequence != NULL )
	{
		Result = OwnerSequence->GetOwner();
	}

	return Result;
}

/**
 * Returns the scene that contains this UIAction.
 *
 * @return	a pointer to the scene that contains this action's owner widget
 */
UUIScene* UUIEvent::GetOwnerScene() const
{
	UUIScene* Result = NULL;

	UUIScreenObject* OwnerWidget = GetOwner();
	if ( OwnerWidget != NULL )
	{
		Result = OwnerWidget->GetScene();
	}

	return Result;
}

/** Unrealscript stubs */
void UUIEvent::execCanBeActivated( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(ControllerIndex);
	P_GET_OBJECT(UUIScreenObject,InEventOwner);
	P_GET_OBJECT_OPTX(UObject,InEventActivator,NULL);
	P_GET_UBOOL_OPTX(bActivateImmediately,FALSE);
	P_GET_TARRAY_OPTX_REF(INT,IndicesToActivate,TArray<INT>());
	P_FINISH;
	*(UBOOL*)Result=CanBeActivated(ControllerIndex,InEventOwner,InEventActivator,bActivateImmediately,pIndicesToActivate);
}

void UUIEvent::execConditionalActivateUIEvent( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(ControllerIndex);
	P_GET_OBJECT(UUIScreenObject,InEventOwner);
	P_GET_OBJECT_OPTX(UObject,InEventActivator,NULL);
	P_GET_UBOOL_OPTX(bActivateImmediately,FALSE);
	P_GET_TARRAY_OPTX_REF(INT,IndicesToActivate,TArray<INT>());
	P_FINISH;
	*(UBOOL*)Result=ConditionalActivateUIEvent(ControllerIndex,InEventOwner,InEventActivator,bActivateImmediately,pIndicesToActivate);
}

void UUIEvent::execActivateUIEvent( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(ControllerIndex);
	P_GET_OBJECT(UUIScreenObject,InEventOwner);
	P_GET_OBJECT_OPTX(UObject,InEventActivator,NULL);
	P_GET_UBOOL_OPTX(bActivateImmediately,FALSE);
	P_GET_TARRAY_OPTX_REF(INT,IndicesToActivate,TArray<INT>());
	P_FINISH;
	*(UBOOL*)Result=ActivateUIEvent(ControllerIndex,InEventOwner,InEventActivator,bActivateImmediately,pIndicesToActivate);
}

/** USequenceEvent inteface */
UBOOL UUIEvent::RegisterEvent()
{
	return Super::RegisterEvent();
}

/**
 * This version of the DrawSeqObj function only draws the UIEvent if it is the UIEvent_MetaObject type.
 */
void UUIEvent::DrawSeqObj(FCanvas* Canvas, UBOOL bSelected, UBOOL bMouseOver, INT MouseOverConnType, INT MouseOverConnIndex, FLOAT MouseOverTime)
{
	/*
	UUIEvent_MetaObject* MetaObject = Cast<UUIEvent_MetaObject>(this);

	if(MetaObject != NULL)
	{
		Super::DrawSeqObj(Canvas, bSelected, bMouseOver, MouseOverConnType, MouseOverConnIndex, MouseOverTime);
	}
	*/

	Super::DrawSeqObj(Canvas, bSelected, bMouseOver, MouseOverConnType, MouseOverConnIndex, MouseOverTime);
}

/**
 * Determines whether this UIAction can be activated.
 *
 * @param	ControllerIndex			the index of the player that activated this event
 * @param	InEventOwner			the widget that contains this UIEvent
 * @param	InEventActivator		an optional object that can be used for various purposes in UIEvents
 * @param	bActivateImmediately	specify true to indicate that we'd like to know whether this event can be activated immediately
 * @param	IndicesToActivate		Indexes into this UIEvent's Output array to activate.  If not specified, all output links
 *									will be activated
 */
UBOOL UUIEvent::CanBeActivated( INT ControllerIndex, UUIScreenObject* InEventOwner, UObject* InEventActivator/*=NULL*/, UBOOL bActivateImmediately/*=FALSE*/, const TArray<INT>* IndicesToActivate/*=NULL*/ )
{
	UBOOL bResult = FALSE;

	UBOOL bValidRole = GWorld == NULL || (bClientSideOnly ? GWorld->IsClient() : GWorld->IsServer());
	if ( bValidRole && !IsPendingKill() && GIsGame )
	{
		KISMET_LOG(TEXT("%s check for activation, %s/%s, triggercount: %d/%d"),*GetName(),InEventOwner!=NULL?*InEventOwner->GetTag().ToString():TEXT("NULL"),InEventActivator!=NULL?*InEventActivator->GetName():TEXT("NULL"),TriggerCount,MaxTriggerCount);
		// if passed a valid widget,
		// and match max trigger count condition
		// and pass re-trigger delay condition
		if ( InEventOwner != NULL
		&&	(MaxTriggerCount == 0 || TriggerCount < MaxTriggerCount)
		&&	(TriggerCount == 0 || ReTriggerDelay == 0.f || GWorld == NULL || (GWorld->GetTimeSeconds() - ActivationTime) > ReTriggerDelay))
		{
			bResult = !DELEGATE_IS_SET(AllowEventActivation)
				||	delegateAllowEventActivation(ControllerIndex, InEventOwner, InEventActivator, bActivateImmediately, IndicesToActivate ? *IndicesToActivate : TArray<INT>());
		}
	}

	return bResult;
}

/**
 * Activates this event if CanBeActivated returns TRUE.
 *
 * @param	ControllerIndex			the index of the player that activated this event
 * @param	InEventOwner			the widget that contains this UIEvent
 * @param	InEventActivator		an optional object that can be used for various purposes in UIEvents
 * @param	bActivateImmediately	if TRUE, the event will be activated immediately, rather than deferring activation until the next tick
 * @param	IndicesToActivate		Indexes into this UIEvent's Output array to activate.  If not specified, all output links
 *									will be activated
 *
 * @return	TRUE if the event was activated successfully, FALSE if the event couldn't be activated.
 */
UBOOL UUIEvent::ConditionalActivateUIEvent( INT ControllerIndex, UUIScreenObject* InEventOwner, UObject* InEventActivator/*=NULL*/, UBOOL bActivateImmediately/*=FALSE*/, const TArray<INT>* IndicesToActivate/*=NULL*/ )
{
	UBOOL bResult = FALSE;

	bResult = CanBeActivated(ControllerIndex,InEventOwner,InEventActivator,bActivateImmediately,IndicesToActivate);
	if ( bResult )
	{
		bResult = ActivateUIEvent(ControllerIndex,InEventOwner,InEventActivator,bActivateImmediately,IndicesToActivate);
	}

	return bResult;
}

/**
 * Activates this UIEvent, adding it the list of active sequence operations in the owning widget's EventProvider
 *
 * @param	PlayerIndex				the index of the player that activated this event
 * @param	InEventOwner			the widget that contains this UIEvent
 * @param	InEventActivator		an optional object that can be used for various purposes in UIEvents
 * @param	bActivateImmediately	if TRUE, the event will be activated immediately, rather than deferring activation until the next tick
 * @param	IndicesToActivate		Indexes into this UIEvent's Output array to activate.  If not specified, all output links
 *									will be activated
 *
 * @return	TRUE if the event was activated successfully, FALSE if the event couldn't be activated.
 */
UBOOL UUIEvent::ActivateUIEvent( INT inPlayerIndex, UUIScreenObject* InEventOwner, UObject* InEventActivator/*=NULL*/, UBOOL bActivateImmediately/*=FALSE*/, const TArray<INT>* IndicesToActivate/*=NULL*/ )
{
	KISMET_LOG(TEXT("- Event %s activated with originator %s, instigator %s"),*GetName(),InEventOwner!=NULL?*InEventOwner->GetTag().ToString():TEXT("NULL"),InEventActivator!=NULL?*InEventActivator->GetName():TEXT("NULL"));

	// fill in any properties for this Event
	EventOwner = InEventOwner;
	EventActivator = InEventActivator;
	PlayerIndex = inPlayerIndex;

	ActivationTime = GWorld ? GWorld->GetTimeSeconds() : 0.f;

	// increment the trigger count
	TriggerCount++;

	// activate this Event
	bActive = TRUE;
	Activated();
	eventActivated();

	InitializeLinkedVariableValues();

	//@fixme ronp - notice that we do not call PopulateLinkedVariableValues here, but USequenceEvent::ActivateEvent does; why don't we?  do we need to?

	// if specific indices were supplied, only activate those output links
	if ( IndicesToActivate != NULL && IndicesToActivate->Num() > 0 )
	{
		for (INT Idx = 0; Idx < IndicesToActivate->Num(); Idx++)
		{
			INT LinkIndex = (*IndicesToActivate)(Idx);
			if ( OutputLinks.IsValidIndex(LinkIndex) )
			{
				OutputLinks(LinkIndex).ActivateOutputLink();
			}
		}
	}
	else
	{
		// otherwise, activate all output links by default
		for (INT LinkIndex = 0; LinkIndex < OutputLinks.Num(); LinkIndex++)
		{
			OutputLinks(LinkIndex).ActivateOutputLink();
		}
	}

	// check if we should log the object comment to the screen
	if ( GEngine->bOnScreenKismetWarnings && bOutputObjCommentToScreen && GWorld != NULL )
	{
		//@todo - change this so that its routed through the widget's reference to the player controller
		// iterate through the controller list
		for (AController *Controller = GWorld->GetFirstController(); Controller != NULL; Controller = Controller->NextController)
		{
			// if it's a player
			if (Controller->IsA(APlayerController::StaticClass()))
			{
				((APlayerController*)Controller)->eventClientMessage(ObjComment,NAME_None);
			}
		}
	}
	
	UBOOL bResult = FALSE;
	if ( bActivateImmediately )
	{
		// UUISequence always adds ops to the widget's global sequence so that ops that were activated from a particular
		// menu state are still executed even if that state is deactivated during the same tick.  So we need to make sure
		// we're manipulating the ActiveSequenceOps array of the widget's global sequence, not the one from the state sequence.
		USequence* ExecutorSequence = ParentSequence;
		while ( ExecutorSequence != NULL && ExecutorSequence->IsA(UUIStateSequence::StaticClass()) )
		{
			ExecutorSequence = ExecutorSequence->ParentSequence;
		}

		check(ExecutorSequence);

		// @todo - what should happen to any existing operations waiting in the queue?
		// Should they be skipped? Should they be activated first?  Should queued ops have
		// the chance to veto immediate activation or perhaps allow the op itself decide what
		// to do when a new op is going to be activated immediately and it's already in the queue...
		// For now, we'll skip them by removing existing ActiveSequenceOps, activating the new event,
		// then restoring the previous queued ops.
		TArray<USequenceOp*> CurrentActiveSequenceOps = ExecutorSequence->ActiveSequenceOps;
		ExecutorSequence->ActiveSequenceOps.Empty();

		if ( ExecutorSequence->QueueSequenceOp(this) )
		{
			ExecutorSequence->ExecuteActiveOps(0.f);
			bResult = TRUE;
			// @todo - notice that we call ExecuteActiveOps directly here, which skips calling UpdateOp on nested sequences
			// but this means that there is no way to have some op immediately activate another op in a child widget
		}

		// now we'll restore the previously queued ops
		ExecutorSequence->ActiveSequenceOps += CurrentActiveSequenceOps;
	}
	else
	{
		// add to the sequence's list of active ops
		bResult = ParentSequence->QueueSequenceOp(this);
	}

	return bResult;
}

/**
 * Allows the operation to initialize the values for any VariableLinks that need to be filled prior to executing this
 * op's logic.  This is a convenient hook for filling VariableLinks that aren't necessarily associated with an actual
 * member variable of this op, or for VariableLinks that are used in the execution of this ops logic.
 *
 * Initializes the value of the "Activator" VariableLink
 */
void UUIEvent::InitializeLinkedVariableValues()
{
	Super::InitializeLinkedVariableValues();

	// if this event has an object attached to the "Activator" variable link, copy the value of InActivator to that sequence variable now
	TArray<UObject**> ObjVars;
	GetObjectVars(ObjVars,TEXT("Activator"));
	for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
	{
		*(ObjVars(Idx)) = EventActivator;
	}


	// if this event has sequence variables attached to the "Player Index" variable links, copy the value of PlayerIndex to those variables now
	TArray<INT*> IntVars;
	GetIntVars(IntVars, TEXT("Player Index"));
	for ( INT Idx = 0; Idx < IntVars.Num(); Idx++ )
	{
		*(IntVars(Idx)) = PlayerIndex;
	}

	INT GamepadId = UUIInteraction::GetPlayerControllerId(PlayerIndex);

	// if this event has sequence variables attached to the "Gamepad Id" variable link, copy the value of the ControllerId to those variables now
	IntVars.Empty();
	GetIntVars(IntVars, TEXT("Gamepad Id"));
	for ( INT Idx = 0; Idx < IntVars.Num(); Idx++ )
	{
		*(IntVars(Idx)) = GamepadId;
	}
}

/* ==========================================================================================================
	UUIEvent_MetaObject
========================================================================================================== */
/** 
 * Sets up any callbacks that this object may need to respond to.
 */
void UUIEvent_MetaObject::RegisterCallbacks()
{
	GCallbackEvent->Register(CALLBACK_UIEditor_WidgetUIKeyBindingChanged, this);
}

/**
 * Removes all of the callbacks for this object.
 */
void UUIEvent_MetaObject::FinishDestroy()
{
	GCallbackEvent->UnregisterAll(this);

	Super::FinishDestroy();
}

/**
 * Gets the array of input actions from the parent state and creates output links corresponding to the input actions.
 */
void UUIEvent_MetaObject::ReadInputActionsFromState()
{
	UUIStateSequence* StateSequence = Cast<UUIStateSequence>(GetOuter());

	if(StateSequence != NULL)
	{
		InputLinks.Empty();
		VariableLinks.Empty();
		EventLinks.Empty();

		RebuildOutputLinks();
	}
}

/**
 * Saves the current connections to the parent state's input action array.
 *
 * @return	TRUE if the state's list of active input keys was modified.
 */
UBOOL UUIEvent_MetaObject::SaveInputActionsToState()
{
	UBOOL bModifiedInputKeys = FALSE;

	UUIState* OwnerState = GetOwnerState();
	if(OwnerState != NULL)
	{
		// Loop through all output links and add any new actions we encounter to the StateInputActions array of our parent UIState.
		INT NumLinks = OutputLinks.Num();
		INT NumActions = OwnerState->StateInputActions.Num();

		// Make sure the state array didnt change from under us.
		check(NumLinks == NumActions);

		for(INT LinkIdx = 0; LinkIdx < NumLinks; LinkIdx++)
		{
			FInputKeyAction& Action = OwnerState->StateInputActions(LinkIdx);
			FSeqOpOutputLink& OutputLink = OutputLinks(LinkIdx);

			//@todo: We are assuming only 1 connection per outputlink right now, this is WRONG, we need to add support for this later.
			if(OutputLink.Links.Num() > 0)
			{
				Modify();

				// determine whether we're about to actually modify something
				if ( !bModifiedInputKeys )
				{
					if ( Action.ActionsToExecute.Num() != OutputLink.Links.Num() )
					{
						bModifiedInputKeys = TRUE;
					}
					else
					{
						for(INT ActionIdx=0; ActionIdx<OutputLink.Links.Num(); ActionIdx++)
						{
							if ( Action.ActionsToExecute(ActionIdx) != OutputLink.Links(ActionIdx).LinkedOp )
							{
								bModifiedInputKeys = TRUE;
								break;
							}
						}
					}
				}

				Action.ActionsToExecute.Empty();
				Action.ActionsToExecute.AddZeroed(OutputLink.Links.Num());

				for(INT ActionIdx=0; ActionIdx<OutputLink.Links.Num(); ActionIdx++)
				{
					Action.ActionsToExecute(ActionIdx) = Cast<USequenceAction>(OutputLink.Links(ActionIdx).LinkedOp);
				}
			}
		}
	}

	return bModifiedInputKeys;
}

/** Rebuilds the list of output links using the items in the State's StateInputActions array. */
void UUIEvent_MetaObject::RebuildOutputLinks()
{

	UUIState* State = GetOwnerState();

	if(State != NULL)
	{


		// Loop through all of the elements in the input actions array and create a output link for each one.
		TArray<FInputKeyAction> &Actions = State->StateInputActions;

		OutputLinks.Empty();
		OutputLinks.InsertZeroed(0, Actions.Num());

		for(INT ActionIdx = 0; ActionIdx < Actions.Num(); ActionIdx++)
		{
			FInputKeyAction &Action = Actions(ActionIdx);
			FSeqOpOutputLink& OutputLink = OutputLinks(ActionIdx);

			GenerateActionDescription(Action, OutputLink.LinkDesc);

			// If there is an action connected, then add a link to the Sequence output link.
			const INT ActionCount = Action.ActionsToExecute.Num();
			if(ActionCount > 0)
			{
				OutputLink.Links.AddZeroed(ActionCount);

				for(INT ExecuteIdx=0; ExecuteIdx<ActionCount; ExecuteIdx++)
				{
					FSeqOpOutputInputLink &ActionLink = OutputLink.Links(ExecuteIdx);
					ActionLink.LinkedOp = Action.ActionsToExecute(ExecuteIdx);
					ActionLink.InputLinkIdx = 0;
				}
				
			}
		}
	}
}

void UUIEvent_MetaObject::GenerateActionDescription(const FInputKeyAction &InAction, FString& OutDescription)
{
	OutDescription = FString::Printf(TEXT("%s (%s)"), *InAction.InputKeyName.ToString(), *UUIRoot::GetInputEventText(InAction.InputKeyState));

	// See if this key is already bound to a default event, if it is, let the user know by adding ' - OVERRIDE' to the description.
	UUIInteraction* DefaultUIInteraction = UUIRoot::GetDefaultUIController();
	UUIState* State = GetOwnerState();
	checkf(State, TEXT("%s has no owner state while attempting to generate action description: %s"), *GetFullName(), *OutDescription);

	UUIScreenObject* OwnerWidget = State->GetOwner();
	FUIInputAliasClassMap* WidgetKeyMapping = DefaultUIInteraction->WidgetInputAliasLookupTable.FindRef(OwnerWidget->GetClass());
		
	if(WidgetKeyMapping != NULL)
	{
		FUIInputAliasMap* InputMap = WidgetKeyMapping->StateLookupTable.Find(State->GetClass());

		if(InputMap != NULL)
		{
			TArray<FUIInputAliasValue> InputKeyAliases;
			InputMap->InputAliasLookupTable.MultiFind(InAction.InputKeyName, InputKeyAliases);
			for ( INT AliasIndex = 0; AliasIndex < InputKeyAliases.Num(); AliasIndex++ )
			{
				const FUIInputAliasValue& InputAliasValue = InputKeyAliases(AliasIndex);

				// Make sure this action is enabled on this widget.
				if( !OwnerWidget->EventProvider->DisabledEventAliases.ContainsItem(InputAliasValue.InputAliasName) )
				{
					OutDescription += FString::Printf(TEXT("%s: %s"), *Localize( TEXT("UIEditor"), TEXT("UIEditor_OverrideSuffix"), TEXT("UnrealEd"), NULL, FALSE), *InputAliasValue.InputAliasName.ToString());
				}
			}
		}
	}
}

/** @return Returns the owner state of this event meta object. */
UUIState* UUIEvent_MetaObject::GetOwnerState()
{
	UUIState* State = NULL;
	UUIStateSequence* StateSequence = Cast<UUIStateSequence>(GetOuter());

	if(StateSequence != NULL)
	{
		State =  StateSequence->GetOwnerState();
	}

	return State;
}

/**
 * Responds to callback events and updates the metaobject as necessary.
 *
 * @param	InType		The type of callback event that was sent.
 */
void UUIEvent_MetaObject::Send(ECallbackEventType InType )
{
	switch(InType)
	{
	case CALLBACK_UIEditor_WidgetUIKeyBindingChanged:
		// Regenerate all of the output link descriptions, since we may be overriding a key now.
		RebuildOutputLinks();
		break;
	}
}

/* ==========================================================================================================
	UUIEvent_ProcessInput
========================================================================================================== */
void UUIEvent_ProcessInput::AddReferencedObjects( TArray<UObject*>& ObjectArray )
{
	for ( TMultiMap<FName,FInputKeyAction>::TIterator It(ActionMap); It; ++It )
	{
		const INT ExecuteCount = It.Value().ActionsToExecute.Num();

		for(INT ExecuteIdx=0; ExecuteIdx<ExecuteCount;ExecuteIdx++)
		{
			AddReferencedObject(ObjectArray,It.Value().ActionsToExecute(ExecuteIdx));
		}
	}
}

void UUIEvent_ProcessInput::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	// Archive the map of input keys to actions
	if ( Ar.IsLoading() && Ar.Ver() < VER_MADE_ACTIONMAP_TRANSIENT )
	{
		TMultiMap<FName,USequenceAction*> TempMap;
		Ar << TempMap;

		// convert into the new layout
		for ( TMultiMap<FName,USequenceAction*>::TIterator It(TempMap); It; ++It )
		{
			const FName& KeyName = It.Key();
			USequenceAction* Action = It.Value();

			TArray<FInputKeyAction> Actions;
			ActionMap.MultiFind(KeyName, Actions);

			FInputKeyAction NewAction(KeyName,IE_Released);

			if(Action)
			{
				NewAction.ActionsToExecute.AddZeroed(1);
				NewAction.ActionsToExecute(0) = Action;
			}
			
			if ( Actions.FindItemIndex(NewAction) == INDEX_NONE )
			{
				ActionMap.Add(KeyName,NewAction);
			}
		}
	}
	else if ( !Ar.IsPersistent() )
	{
		Ar << ActionMap;
	}
}


/* ==========================================================================================================
	UUIEvent_State
========================================================================================================== */
/**
 * Allows the operation to initialize the values for any VariableLinks that need to be filled prior to executing this
 * op's logic.  This is a convenient hook for filling VariableLinks that aren't necessarily associated with an actual
 * member variable of this op, or for VariableLinks that are used in the execution of this ops logic.
 *
 * Initializes the value of the "State" VariableLink
 */
void UUIEvent_State::InitializeLinkedVariableValues()
{
	Super::InitializeLinkedVariableValues();

	if ( EventOwner != NULL )
	{
		UUIState* StateActivator = Cast<UUIState>(EventActivator);
		if ( StateActivator != NULL )
		{
			// copy the value of the widget's current state into the "State" variable link
			TArray<UObject**> ObjVars;
			GetObjectVars(ObjVars,TEXT("State"));
			for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
			{
				*(ObjVars(Idx)) = StateActivator;
			}
		}
	}
}

/* ==========================================================================================================
	UUIAction
========================================================================================================== */
/**
 * Returns the widget that contains this UIAction.
 *
 * @return	a pointer to the widget that contains the sequence this action belongs to.
 */
UUIScreenObject* UUIAction::GetOwner() const
{
	UUIScreenObject* Result = NULL;

	UUISequence* OwnerSequence = Cast<UUISequence>(ParentSequence);
	if ( OwnerSequence != NULL )
	{
		Result = OwnerSequence->GetOwner();
	}

	return Result;
}

/**
 * Returns the scene that contains this UIAction.
 *
 * @return	a pointer to the scene that contains this action's owner widget
 */
UUIScene* UUIAction::GetOwnerScene() const
{
	UUIScene* Result = NULL;

	UUIScreenObject* OwnerWidget = GetOwner();
	if ( OwnerWidget != NULL )
	{
		Result = OwnerWidget->GetScene();
	}

	return Result;
}

/**
 * Allows the operation to initialize the values for any VariableLinks that need to be filled prior to executing this
 * op's logic.  This is a convenient hook for filling VariableLinks that aren't necessarily associated with an actual
 * member variable of this op, or for VariableLinks that are used in the execution of this ops logic.
 *
 * Initializes the value of the Player Index VariableLinks
 */
void UUIAction::InitializeLinkedVariableValues()
{
	Super::InitializeLinkedVariableValues();

	// if this event has sequence variables attached to the "Player Index" variable links, copy the value of PlayerIndex to those variables now
	TArray<INT*> IntVars;
	GetIntVars(IntVars, TEXT("Player Index"));
	for ( INT Idx = 0; Idx < IntVars.Num(); Idx++ )
	{
		*(IntVars(Idx)) = PlayerIndex;
	}

	// if this event has sequence variables attached to the "Gamepad Id" variable link, copy the value of the ControllerId to those variables now
	IntVars.Empty();
	GetIntVars(IntVars, TEXT("Gamepad Id"));
	for ( INT Idx = 0; Idx < IntVars.Num(); Idx++ )
	{
		*(IntVars(Idx)) = GamepadID;
	}
}

void UUIAction::Activated()
{
	// if we're configured to automatically attach to the owner widget and the designer didn't specify 
	// any specific widgets to attach to, insert the owning widget into the Targets array now.
	if ( bAutoTargetOwner && Targets.Num() == 0 )
	{
		UUIScreenObject* OwnerWidget = GetOwner();
		if ( OwnerWidget != NULL )
		{
			Targets.AddItem(OwnerWidget);
		}
	}

	Super::Activated();
}

void UUIAction::DeActivated()
{
	// if we're configured to automatically attach to the owner widget and our owner widget is
	// the only widget in the targets array, remove it now so that the next time we're activated,
	// we can bind to our owner again
	if ( bAutoTargetOwner && Targets.Num() == 1 )
	{
		UUIScreenObject* OwnerWidget = GetOwner();
		if ( OwnerWidget != NULL && OwnerWidget == Targets(0) )
		{
			Targets.Empty();
		}
	}

	Super::DeActivated();
}

/**
 * Notification that an input link on this sequence op has been given impulse by another op.
 *
 * This version also initializes the value of GamepadID.
 *
 * @param	ActivatorOp		the sequence op that applied impulse to this op's input link
 * @param	InputLinkIndex	the index [into this op's InputLinks array] for the input link that was given impulse
 */
void UUIAction::OnReceivedImpulse( USequenceOp* ActivatorOp, INT InputLinkIndex )
{
	Super::OnReceivedImpulse(ActivatorOp,InputLinkIndex);
	GamepadID = UUIInteraction::GetPlayerControllerId(PlayerIndex);
}

/* === UObject interface === */
/**
 * Called after this object has been completely de-serialized.  This version validates that this action has at least one
 * InputLink, and if not resets this action's InputLinks array to the default version
 */
void UUIAction::PostLoad()
{
	Super::PostLoad();

	// an action with no input links will not be rendered, so if this action has no input links, attempt to restore from defaults
	if ( InputLinks.Num() == 0 )
	{
		UUIAction* DefaultAction = GetArchetype<UUIAction>();
		checkSlow(DefaultAction);

		InputLinks = DefaultAction->InputLinks;
		debugf(TEXT("%s had empty InputLinks array - restoring InputLinks array from archetype with %i elements: %s"),
			*GetFullName(), InputLinks.Num(), *DefaultAction->GetFullName());

		if ( InputLinks.Num() == 0 && ParentSequence != NULL )
		{
			// if we still don't have any InputLinks, this action cannot be activated, so let's just remove it from the sequence
			debugf(TEXT("\tArchetype didn't contain any elements in InputLinks array - removing from parent sequence"));
			ParentSequence->RemoveObject(this);
		}
	}
}

/* ==========================================================================================================
	UUIAction_ActivateState
========================================================================================================== */
/**
 * Activates the state associated with this action for all targets.  If any of the targets cannot change states,
 * disables all output links.
 */
void UUIAction_ActivateState::Activated()
{
	bStateChangeFailed = FALSE;
	Super::Activated();

	// bCallHandler if false for UIAction_ChangeState by default, so if it's true, it means that this instance is overriding the default behavior and
	// the state change will have occurred in script
	if ( !bCallHandler )
	{
		for ( INT TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++ )
		{
			UUIScreenObject* Widget = Cast<UUIScreenObject>(Targets(TargetIndex));
			if ( Widget != NULL )
			{
				if ( TargetState != NULL )
				{
					// if ActivateState returns false because the state is already in the widget's state stack, don't
					// disable output links
					UBOOL bSuccess = Widget->ActivateState(TargetState,PlayerIndex) || Widget->StateStack.FindItemIndex(TargetState) != INDEX_NONE;
					bStateChangeFailed = !bSuccess || bStateChangeFailed;
				}
				else
				{
					// if ActivateState returns false because the state is already in the widget's state stack, don't
					// disable output links
					UBOOL bSuccess = Widget->ActivateStateByClass(StateType,PlayerIndex) || Widget->HasActiveStateOfClass(StateType,PlayerIndex);
					bStateChangeFailed = !bSuccess || bStateChangeFailed;
				}
			}
		}
	}

	// now activate the appropriate output link
	if ( bStateChangeFailed )
	{
		OutputLinks(1).ActivateOutputLink();
	}
	else
	{
		OutputLinks(0).ActivateOutputLink();
	}
}

/* ==========================================================================================================
	UUIAction_DeactivateState
========================================================================================================== */
/**
 * Activates the state associated with this action for all targets.  If any of the targets cannot change states,
 * disables all output links.
 */
void UUIAction_DeactivateState::Activated()
{
	bStateChangeFailed = FALSE;
	Super::Activated();

	// bCallHandler if false for UIAction_ChangeState by default, so if it's true, it means that this instance is overriding the default behavior and
	// the state change will have occurred in script
	if ( !bCallHandler )
	{
		for ( INT TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++ )
		{
			UUIScreenObject* Widget = Cast<UUIScreenObject>(Targets(TargetIndex));
			if ( Widget != NULL )
			{
				if ( TargetState != NULL )
				{
					// if DeactivateState returns false because the state doesn't exist in the widget's state stack,
					// don't disable the output links (we only want to disable output links if the widget refused to
					// deactivate the state)
					UBOOL bSuccess = Widget->DeactivateState(TargetState,PlayerIndex) || Widget->StateStack.FindItemIndex(TargetState) == INDEX_NONE;
					bStateChangeFailed = !bSuccess || bStateChangeFailed;
				}
				else
				{
					// if DeactivateState returns false because the state doesn't exist in the widget's state stack,
					// don't disable the output links (we only want to disable output links if the widget refused to
					// deactivate the state)
					UBOOL bSuccess = Widget->DeactivateStateByClass(StateType,PlayerIndex) || !Widget->HasActiveStateOfClass(StateType,PlayerIndex);
					bStateChangeFailed = !bSuccess || bStateChangeFailed;
				}
			}
		}
	}

	// now activate the appropriate output link
	if ( bStateChangeFailed )
	{
		OutputLinks(1).ActivateOutputLink();
	}
	else
	{
		OutputLinks(0).ActivateOutputLink();
	}
}

/* ==========================================================================================================
	UUIAction_Scene
========================================================================================================== */
/**
 * Allows the operation to initialize the values for any VariableLinks that need to be filled prior to executing this
 * op's logic.  This is a convenient hook for filling VariableLinks that aren't necessarily associated with an actual
 * member variable of this op, or for VariableLinks that are used in the execution of this ops logic.
 *
 * Initializes the value of the Scene linked variables
 */
void UUIAction_Scene::InitializeLinkedVariableValues()
{
	Super::InitializeLinkedVariableValues();

	// if the Scene was specified in the property window, initialize the value of linked variables
	if ( Scene != NULL )
	{
		// see if any Scene variables are attached
		TArray<UObject**> ObjectVars;
		GetObjectVars(ObjectVars,TEXT("Scene"));

		// if so, initialize the value of the linked variable with the current value of "Scene"
		for ( INT Idx = 0; Idx < ObjectVars.Num(); Idx++ )
		{
			*(ObjectVars(Idx)) = Scene;
		}
	}
}

/* ==========================================================================================================
	UUIAction_OpenScene
========================================================================================================== */
/**
 * Opens the scene specified by this action.
 *
 * @note: this action must be safe to execute from outside the scope of the UI system, since it can be used
 *			in level sequences.
 */
void UUIAction_OpenScene::Activated()
{
	Super::Activated();

	UBOOL bSceneOpenedSuccessfully = FALSE;
	UGameUISceneClient* SceneClient = UUIRoot::GetSceneClient();
	if ( SceneClient != NULL )
	{
		ULocalPlayer* PlayerOwner = NULL;
		UUIScreenObject* OwnerWidget = GetOwner();
		if ( OwnerWidget != NULL )
		{
			UUIScene* OwnerScene = OwnerWidget->GetScene();

			if( OwnerScene != NULL )
			{
				PlayerOwner = OwnerScene->GetPlayerOwner(PlayerIndex);
			}
		}
		else if( Scene != NULL )
		{
			PlayerOwner = Scene->GetPlayerOwner(PlayerIndex);
		}

		bSceneOpenedSuccessfully = SceneClient->OpenScene(Scene,PlayerOwner,&OpenedScene);
	}

	// activate the appropriate output link based on whether the operation was successful or not
	if ( OutputLinks.Num() > 0 )
	{
		if ( bSceneOpenedSuccessfully )
		{
			OutputLinks(0).ActivateOutputLink();
		}
		else if ( OutputLinks.Num() > 1 )
		{
			OutputLinks(1).ActivateOutputLink();
		}
	}
}

/**
 * Closes the scene specified by this action.
 */
void UUIAction_CloseScene::Activated()
{
	Super::Activated();

	UBOOL bSceneClosedSuccessfully = FALSE;
	UGameUISceneClient* SceneClient = UUIRoot::GetSceneClient();
	if ( SceneClient != NULL )
	{
		// if we didn't specify a scene, close the scene that this UIAction belongs to
		UUIScene* SceneToClose = Scene;
		if ( SceneToClose == NULL )
		{
			UUIScreenObject* OwnerWidget = GetOwner();
			if ( bAutoTargetOwner && OwnerWidget != NULL )
			{
                SceneToClose = OwnerWidget->GetScene();
			}
		}

		bSceneClosedSuccessfully = SceneClient->CloseScene(SceneToClose);
	}

	// activate the appropriate output link based on whether the operation was successful or not
	if ( OutputLinks.Num() > 0 )
	{
		if ( bSceneClosedSuccessfully )
		{
			OutputLinks(0).ActivateOutputLink();
		}
		else if ( OutputLinks.Num() > 1 )
		{
			OutputLinks(1).ActivateOutputLink();
		}
	}
}

/* ==========================================================================================================
	UUIAction_RefreshBindingValue
========================================================================================================== */
/**
 * If the owning widget implements the UIDataStoreSubscriber interface, calls the appropriate method for refreshing the
 * owning widget's value from the data store.
 */
void UUIAction_RefreshBindingValue::Activated()
{
	Super::Activated();

	for ( INT TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++ )
	{
		UUIScreenObject* Widget = Cast<UUIScreenObject>(Targets(TargetIndex));
		if ( Widget != NULL )
		{
			// if this child implements the UIDataStoreSubscriber interface, tell the child to load the value from the data store
			IUIDataStoreSubscriber* SubscriberChild = (IUIDataStoreSubscriber*)Widget->GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass());
			if ( SubscriberChild != NULL )
			{
				SubscriberChild->RefreshSubscriberValue(BindingIndex);
			}
		}
	}
}

/* ==========================================================================================================
	UUIAction_PublishValue
========================================================================================================== */
/**
 * If the owning widget implements the UIDataStorePublisher interface, calls the appropriate method for publishing the
 * owning widget's value to the data store.
 */
void UUIAction_PublishValue::Activated()
{
	Super::Activated();

	TArray<UUIDataStore*> DataStores;
	for ( INT TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++ )
	{
		// Try casting to a widget first, if it isn't a widget, try a scene.
		UUIObject* Widget = Cast<UUIObject>(Targets(TargetIndex));
		if ( Widget != NULL )
		{
			// if this child implements the UIDataStoreSubscriber interface, tell the child to load the value from the data store
			IUIDataStorePublisher* PublisherChild = (IUIDataStorePublisher*)Widget->GetInterfaceAddress(IUIDataStorePublisher::UClassType::StaticClass());
			if ( PublisherChild != NULL )
			{
				PublisherChild->SaveSubscriberValue(DataStores,BindingIndex);
			}
		}
		else
		{
			UUIScene* Scene = Cast<UUIScene>(Targets(TargetIndex));

			if(Scene)
			{
				Scene->SaveSceneDataValues(FALSE);
			}
		}
	}

	// now notify all data stores that we have finished publishing values
	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->OnCommit();
	}
}

/* ==========================================================================================================
	UUIAction_GetListIndex
========================================================================================================== */
/**
 * Copies the value of the current element for the UILists in the Targets array to the Selected Item variable link.
 */
void UUIAction_GetListIndex::Activated()
{
	Super::Activated();

	TArray<UUIList*> TargetLists;
	for ( INT TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++ )
	{
		UUIList* TargetList = Cast<UUIList>(Targets(TargetIndex));
		if ( TargetList != NULL )
		{
			TargetLists.AddItem(TargetList);
		}
	}

	TArray<INT*> IntVars;
	GetIntVars(IntVars,TEXT("Current Item"));

	if ( TargetLists.Num() != IntVars.Num() )
	{
		KISMET_LOG(TEXT("%s number of source lists doesn't match number of attached int vars. Lists:%d IntVars:%d"), *GetFullName(), TargetLists.Num(), IntVars.Num());
	}
	else
	{
		// go through the list of source lists, and copy the list's current item to the corresponding variable link; note that
		// it's possible for both the "Valid" and "Invalid" outputs to be activated, if multiple lists are attached
		// and one of them had an invalid index while the other had a valid index.
		for ( INT Index = 0; Index < TargetLists.Num(); Index++ )
		{
			UUIList* SourceList = TargetLists(Index);
			INT& IntValue = *IntVars(Index);

			IntValue = SourceList->GetCurrentItem();
			if ( IntValue == INDEX_NONE )
			{
				if ( OutputLinks.Num() > 1 )
				{
					// activate the "Invalid Index" output link
					OutputLinks(1).ActivateOutputLink();
				}
				else
				{
					KISMET_LOG(TEXT("%s missing \"Invalid\" OutputLink!"), *GetFullName());
				}
			}
			else
			{
				// activate the "Valid Index" output link
				OutputLinks(0).ActivateOutputLink();
			}
		}
	}
}

/* ==========================================================================================================
	UUIAction_DataStoreField
========================================================================================================== */
/**
 * Resolves the specified markup string into a data store reference and the data provider portion of the markup string.
 *
 * @param	out_ResolvedProvider	receives a pointer to the data provider that contains the field referenced by DataFieldMarkupString
 * @param	out_DataFieldName		receives the portion of DataFieldMarkupString that corresponds to the data field name
 * @param	out_DataStore			allows the caller to receive a pointer to the data store resolved from the markup string.
 *
 * @return	TRUE if the markup was successfully resolved; FALSE otherwise.
 */
UBOOL UUIAction_DataStoreField::ResolveMarkup( UUIDataProvider*& out_ResolvedProvider, FString& out_DataFieldName, UUIDataStore** out_ResolvedDataStore/*=NULL*/ )
{
	UBOOL bResult = FALSE;

	//@todo - support resolving data store values outside of the context of the UI (i.e. placing a GetDataStoreValue action in the level's sequence)
	if ( DataFieldMarkupString.Len() > 0 )
	{
		// first, determine which scene we should use for resolving the value of the data store markup
		if ( TargetScene == NULL )
		{
			// if no target scene was specified, see if we have any widgets in the Targets array
			UUIScreenObject* TargetWidget = NULL;
			for ( INT TargetIndex = 0; TargetWidget == NULL && TargetIndex < Targets.Num(); TargetIndex++ )
			{
				TargetWidget = Cast<UUIScreenObject>(Targets(TargetIndex));
			}

			if ( TargetWidget != NULL )
			{
				TargetScene = TargetWidget->GetScene();
			}
		}

		if ( TargetScene != NULL )
		{
			// now, resolve the data store markup
			FUIStringParser Parser;

			// scan in the markup string - we should end up with only one chunk
			Parser.ScanString(DataFieldMarkupString);

			const TIndirectArray<FTextChunk>* Chunks = Parser.GetTextChunks();
			if ( Chunks->Num() == 1 )
			{
				const FTextChunk* MarkupChunk = &((*Chunks)(0));
				if ( MarkupChunk->IsMarkup() )
				{
					FString DataStoreTag, DataStoreValue;

					// split-off the data store name from the data field path
					if ( MarkupChunk->GetDataStoreMarkup(DataStoreTag, DataStoreValue) )
					{
						// attempt to resolve the data store name
						UUIDataStore* ResolvedDataStore = TargetScene->ResolveDataStore(*DataStoreTag);
						if ( ResolvedDataStore != NULL )
						{
							INT CollectionArrayIndex = INDEX_NONE;

							// now resolve the data provider that contains the tag specified
							bResult = ResolvedDataStore->ParseDataStoreReference(DataStoreValue, out_ResolvedProvider, out_DataFieldName, CollectionArrayIndex);
							if ( bResult )
							{
								if ( out_ResolvedDataStore != NULL )
								{
									*out_ResolvedDataStore = ResolvedDataStore;
								}

								// if an array index was part of the data field name, it will have been stripped off by the call to ParseDataStoreReference
								// so we need to restore it by appending it back to the end of the field name
								if ( CollectionArrayIndex != INDEX_NONE )
								{
									out_DataFieldName += FString::Printf(TEXT(";%i"), CollectionArrayIndex);
								}
							}
						}
					}
				}
			}
		}
	}

	return bResult;
}

/* ==========================================================================================================
	UUIAction_SetDatafieldValue
========================================================================================================== */
/**
 * Resolves the datastore specified by DataFieldMarkupString, and copies the value from DataFieldStringValue/ImageValue
 * to the resolved data provider.
 */
void UUIAction_SetDatafieldValue::Activated()
{
	Super::Activated();

	UBOOL bSuccess = FALSE;

	UUIDataStore* DataStore = NULL;
	UUIDataProvider* FieldProvider = NULL;
	FString FieldName;

	// resolve the markup string to get the name of the data field and the provider that contains that data
	if ( ResolveMarkup(FieldProvider, FieldName, &DataStore) )
	{
		FUIProviderScriptFieldValue FieldValue(EC_EventParm);
		FieldValue.PropertyTag = *FieldName;
		FieldValue.PropertyType = DATATYPE_Property;

		if ( DataFieldRangeValue.HasValue() )
		{
			FieldValue.PropertyType = DATATYPE_RangeProperty;
		}
		else if ( DataFieldArrayValue.Num() > 0 )
		{
			FieldValue.PropertyType = DATATYPE_Collection;
		}

		FieldValue.StringValue = DataFieldStringValue;
		FieldValue.ImageValue = DataFieldImageValue;
		FieldValue.RangeValue = DataFieldRangeValue;

		//@todo ronp - support arrays?
		FieldValue.ArrayValue = DataFieldArrayValue;

		bSuccess = FieldProvider->SetDataStoreValue(FieldName, FieldValue);
		if ( DataStore != NULL && bCommitValueImmediately )
		{
			DataStore->OnCommit();
		}
	}

	if ( bSuccess == FALSE )
	{
		OutputLinks(0).ActivateOutputLink();
	}
	else
	{
		OutputLinks(1).ActivateOutputLink();
	}
}

/* ==========================================================================================================
	UUIAction_GetDatafieldValue
========================================================================================================== */
/**
 * Resolves the datastore specified by DataFieldMarkupString, and copies the value the resolved data provider to
 * DataFieldStringValue/ImageValue.
 */
void UUIAction_GetDatafieldValue::Activated()
{
	Super::Activated();

	UBOOL bSuccess = FALSE;

	UUIDataProvider* FieldProvider = NULL;
	FString FieldName;

	// resolve the markup string to get the name of the data field and the provider that contains that data
	if ( ResolveMarkup(FieldProvider, FieldName) )
	{
		FUIProviderFieldValue FieldValue(EC_EventParm);

		if ( FieldProvider->GetDataStoreValue(FieldName, FieldValue) )
		{
			bSuccess = TRUE;
			DataFieldStringValue = FieldValue.StringValue;
			DataFieldImageValue = FieldValue.ImageValue;
			DataFieldArrayValue = FieldValue.ArrayValue;
			DataFieldRangeValue = FieldValue.RangeValue;

			//@todo ronp - change the following code to use FieldValue.PropertyType to determine which OutputLink to activate, but this requires
			// that everyone sets PropertyType correctly
			if ( DataFieldStringValue.Len() > 0 )
			{
				OutputLinks(1).ActivateOutputLink();
			}

			if ( DataFieldImageValue != NULL )
			{
				OutputLinks(2).ActivateOutputLink();
			}

			//@todo ronp - support arrays?
			if ( DataFieldArrayValue.Num() > 0 )
			{
				OutputLinks(3).ActivateOutputLink();
			}

			if ( DataFieldRangeValue.HasValue() )
			{
				OutputLinks(4).ActivateOutputLink();
			}
		}
	}

	if ( bSuccess == FALSE )
	{
		OutputLinks(0).ActivateOutputLink();
	}
}

/* ==========================================================================================================
	UUIAction_GetDatafieldValue
========================================================================================================== */
/**
 * Resolves the datastore specified by DataFieldMarkupString, and adds a new data field to the resolved data provider
 * using the specified name.
 */
void UUIAction_AddDataField::Activated()
{
	Super::Activated();

	UBOOL bSuccess = FALSE;

	UUIDataProvider* FieldProvider = NULL;
	FString FieldName;

	// resolve the markup string to get the name of the data field and the provider that contains that data
	if ( ResolveMarkup(FieldProvider, FieldName) )
	{
		UUIDynamicFieldProvider* DynamicFieldProvider = Cast<UUIDynamicFieldProvider>(FieldProvider);
		if ( DynamicFieldProvider != NULL )
		{
			bSuccess = DynamicFieldProvider->AddField(*FieldName, FieldType, bPersistentField);
		}
	}

	OutputLinks(bSuccess).ActivateOutputLink();
}

/* ==========================================================================================================
	UUIAction_GetCellValue
========================================================================================================== */
/**
 * Builds a data store markup string which can be used to access the value of CellFieldName for the element specified
 * by CollectionIndex from the data store bound to the UIList objects contained in the targets array.
 */
void UUIAction_GetCellValue::Activated()
{
	Super::Activated();

	UBOOL bSuccess = FALSE;

	if ( CellFieldName != NAME_None )
	{
		// find the list that we'll be getting the cell field value for
		UUIList* TargetList = NULL;
		for ( INT TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++ )
		{
			TargetList = Cast<UUIList>(Targets(TargetIndex));
			if ( TargetList != NULL )
			{
				break;
			}
		}

		if ( TargetList != NULL && TargetList->DataProvider )
		{
			INT Index = CollectionIndex;

			// if no index was specified, use the list's currently selected item
			if ( Index == INDEX_NONE )
			{
				Index = TargetList->GetCurrentItem();
			}

			if ( Index != INDEX_NONE )
			{
				// get the UIListElementCellProvider for the specified element
				TScriptInterface<IUIListElementCellProvider> CellProvider = TargetList->DataProvider->GetElementCellValueProvider(TargetList->DataSource.DataStoreField, Index);
				if ( CellProvider )
				{
					FUIProviderFieldValue CellValue(EC_EventParm);
					if ( CellProvider->GetCellFieldValue(CellFieldName, Index, CellValue) == TRUE )
					{
						CellFieldStringValue = CellValue.StringValue;
						CellFieldImageValue = CellValue.ImageValue;
						CellFieldRangeValue = CellValue.RangeValue;

						// if the cell provider is a UIDataProvider, generate the markup string required to access this property
						UUIDataProvider* CellProviderObject = Cast<UUIDataProvider>(CellProvider.GetObject());
						if ( CellProviderObject != NULL )
						{
							// get the data field path to the collection data field that the list is bound to...
							FString DataSourceMarkup = CellProviderObject->BuildDataFieldPath(*TargetList->DataSource, TargetList->DataSource.DataStoreField);
							if ( DataSourceMarkup.Len() != 0 )
							{
								// then append the list index and the cell name
								CellFieldMarkup = FString::Printf(TEXT("<%s;%i.%s>"), *DataSourceMarkup, Index, *CellFieldName.ToString());
							}
							else
							{
								// if the CellProviderObject doesn't support the data field that the list is bound to, the CellProviderObject is an internal provider of
								// the data store that the list is bound to.  allow the data provider to generate its own markup by passing in the CellFieldName
								CellFieldMarkup = CellProviderObject->GenerateDataMarkupString(*TargetList->DataSource,CellFieldName);
							}
						}

						bSuccess = TRUE;
					}
				}
			}
		}
	}

	if ( bSuccess == TRUE )
	{
		OutputLinks(0).ActivateOutputLink();
	}
	else
	{
		OutputLinks(1).ActivateOutputLink();
	}
}

/* ==========================================================================================================
	UUIAction_ShowDeviceSelectionUI
========================================================================================================== */
void UUIAction_ShowDeviceSelectionUI::WriteToVariables()
{
	TArray<INT*> IntVars;

	GetIntVars(IntVars, TEXT("DeviceID"));

	for(INT VarIdx=0; VarIdx<IntVars.Num(); VarIdx++)
	{
		*(IntVars(VarIdx)) = DeviceID;
	}

	TArray<FString*> StringVars;

	GetStringVars(StringVars, TEXT("Device Name"));

	for(INT VarIdx=0; VarIdx<StringVars.Num(); VarIdx++)
	{
		*(StringVars(VarIdx)) = DeviceName;
	}
}



/* ==========================================================================================================
	UUIAction_GetPrivilegeLevel
========================================================================================================== */
IMPLEMENT_CLASS(UUIAction_GetPrivilegeLevel)

/**
 * Activated event handler.
 */
void UUIAction_GetPrivilegeLevel::Activated()
{


	EFeaturePrivilegeLevel Status = (EFeaturePrivilegeLevel)eventGetPrivilegeLevel(UUIInteraction::GetPlayerControllerId(PlayerID));

	switch(Status)
	{
	case FPL_EnabledFriendsOnly:
		OutputLinks(1).ActivateOutputLink();
		break;
	case FPL_Enabled:
		OutputLinks(2).ActivateOutputLink();
		break;
	default:
		OutputLinks(0).ActivateOutputLink();
		break;
	}

	Super::Activated();
}


/* ==========================================================================================================
	UUIAction_PlayUISoundCue
========================================================================================================== */
/**
 * Plays the UISoundCue indicated by SoundCueName.
 */
void UUIAction_PlayUISoundCue::Activated()
{
	Super::Activated();

	UBOOL bSuccess = FALSE;

	if ( SoundCueName.Len() > 0 )
	{
		UUIInteraction* UIController = UUIRoot::GetCurrentUIController();
		if ( UIController != NULL )
		{
			bSuccess = UIController->PlayUISound(*SoundCueName,PlayerIndex);
		}
	}

	if ( bSuccess == TRUE )
	{
		OutputLinks(0).ActivateOutputLink();
	}
	else
	{
		OutputLinks(1).ActivateOutputLink();
	}
}

/* ==========================================================================================================
	UUIAction_SetFocus
========================================================================================================== */
IMPLEMENT_CLASS(UUIAction_SetFocus)

/**
 * Calls SetFocus on the attached widgets.
 */
void UUIAction_SetFocus::Activated()
{
	Super::Activated();

	// find the list that we'll be getting the cell field value for
	UUIObject* TargetWidget = NULL;
	for ( INT TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++ )
	{
		TargetWidget = Cast<UUIObject>(Targets(TargetIndex));
		if ( TargetWidget != NULL )
		{
			TargetWidget->SetFocus(NULL);
		}
	}
}


/* ==========================================================================================================
	UUIAction_GetLastFocused
========================================================================================================== */
IMPLEMENT_CLASS(UUIAction_GetLastFocused)

/**
 * Gets the control that was previously focused in the specified parent and stores it in LastFocused.
 */
void UUIAction_GetLastFocused::Activated()
{
	UUIScreenObject* Object;
	LastFocused = NULL;

	// If the parent is null then we should use the owner scene for the control.
	if(Parent==NULL)
	{
		Object = GetOwner()->GetScene();
	}
	else
	{
		Object = Parent;
	}

	if(Object)
	{
		LastFocused = Object->GetLastFocusedControl(TRUE);
	}

	if(LastFocused != NULL)
	{
		OutputLinks(0).ActivateOutputLink();	//Success
	}
	else
	{
		OutputLinks(1).ActivateOutputLink();	//Failure
	}
}

/* ==========================================================================================================
	UUIAction_MoveListItem
========================================================================================================== */
/**
 * Executes the logic for this action.
 */
void UUIAction_MoveListItem::Activated()
{
	Super::Activated();

	UBOOL bSuccess = FALSE;

	// find the list that owns this action
	UUIList* TargetList = NULL;
	for ( INT TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++ )
	{
		TargetList = Cast<UUIList>(Targets(TargetIndex));
		if ( TargetList != NULL )
		{
			break;
		}
	}

	if ( TargetList != NULL )
	{
		INT IndexToMove = ElementIndex;

		// if no index was specified, use the list's currently selected item
		if ( IndexToMove == INDEX_NONE )
		{
			IndexToMove = TargetList->Index;
		}
		bSuccess = TargetList->MoveElementAtIndex(IndexToMove, MoveCount);
	}

	if ( bSuccess == TRUE )
	{
		OutputLinks(1).ActivateOutputLink();
	}
	else
	{
		OutputLinks(0).ActivateOutputLink();
	}
}

/**
 * Finds the datastore involved and tells that datastore to join the
 * online game with the selected index
 */
void UUIAction_JoinOnlineGame::Activated(void)
{	
	UUIList* TargetList = NULL;

	bIsDone = TRUE;

	// Find the first list object in the targets array
	for (INT Index = 0; Index < Targets.Num(); Index++)
	{
		TargetList = Cast<UUIList>(Targets(Index));
		if (TargetList != NULL)
		{
			break;
		}
	}

	// If the user set a list as a target
	if (TargetList != NULL)
	{
		const INT SelectedItem = TargetList->GetCurrentItem();
		if (SelectedItem != INDEX_NONE)
		{
			// Check to see that the list is bound to a game search datastore
			UUIDataStore_OnlineGameSearch* GameSearchDataStore = Cast<UUIDataStore_OnlineGameSearch>(TargetList->DataSource.ResolvedDataStore);
			if (GameSearchDataStore)
			{
				FOnlineGameSearchResult GameToJoin(EC_EventParm);
				// Get the game that was selected
				if (GameSearchDataStore->eventGetSearchResultFromIndex(SelectedItem,GameToJoin))
				{
					bIsDone = FALSE;

					// Pass it to script to join
					eventJoinOnlineGame(UUIInteraction::GetPlayerControllerId(PlayerIndex),
						GameToJoin,GWorld->GetWorldInfo());
				}
				else
				{
					debugf(TEXT("UUIAction_JoinOnlineGame::Activated - Invalid Search Index: %i"), SelectedItem);
				}
			}
			else
			{
				debugf(TEXT("UUIAction_JoinOnlineGame::Activated - GameSearchDataStore is NULL"));
			}
		}
		else
		{
			debugf(TEXT("UUIAction_JoinOnlineGame::Activated - Invalid List Index"), SelectedItem);
		}
	}
	else
	{
		debugf(TEXT("UUIAction_JoinOnlineGame::Activated - No list specified"));
	}
}


/**
 * Polls to see if the async action is done
 *
 * @param ignored
 *
 * @return TRUE if the operation has completed, FALSE otherwise
 */
UBOOL UUIAction_JoinOnlineGame::UpdateOp(FLOAT)
{
	if (bIsDone)
	{
		debugf(TEXT("JoinOnlineGame Result: %i"), bResult);

		if(bResult)
		{
			OutputLinks(1).ActivateOutputLink();
		}
		else
		{
			OutputLinks(0).ActivateOutputLink();
		}
	}
	return bIsDone;
}

/**
 * Finds the datastore involved and tells that datastore to create the
 * online game with the current settings held within it
 */
void UUIAction_CreateOnlineGame::Activated(void)
{
	UBOOL bSuccess = FALSE;

	// Get the scene client so we can find the datastore by name
	UGameUISceneClient* SceneClient = UUIRoot::GetSceneClient();
	if (SceneClient != NULL)
	{
		// Find the datastore by name and verify it's of the correct type
		UUIDataStore* DataStore = SceneClient->DataStoreManager->FindDataStore(DataStoreName);
		UUIDataStore_OnlineGameSettings* GameSettingsDataStore = Cast<UUIDataStore_OnlineGameSettings>(DataStore);
		// It's possible that the user entered the name of the wrong datastore
		if (GameSettingsDataStore != NULL)
		{
			// Now call the event to process the create
			bSuccess = GameSettingsDataStore->eventCreateGame(GWorld->GetWorldInfo(),
				UUIInteraction::GetPlayerControllerId(PlayerIndex));
		}
	}

	// Activate the output links based upon with the create call worked
	if (bSuccess == TRUE)
	{
		OutputLinks(1).ActivateOutputLink();
	}
	else
	{
		OutputLinks(0).ActivateOutputLink();
	}
}

/**
 * Finds the datastore involved and tells that datastore to search for online
 * games using the current settings
 */
void UUIAction_FindOnlineGames::Activated(void)
{
	UBOOL bSuccess = FALSE;
	// Get the scene client so we can find the datastore by name
	UGameUISceneClient* SceneClient = UUIRoot::GetSceneClient();
	if (SceneClient != NULL)
	{
		// Find the datastore by name and verify it's of the correct type
		UUIDataStore* DataStore = SceneClient->DataStoreManager->FindDataStore(DataStoreName);
		UUIDataStore_OnlineGameSearch* GameSearchDataStore = Cast<UUIDataStore_OnlineGameSearch>(DataStore);
		// It's possible that the user entered the name of the wrong datastore
		if (GameSearchDataStore != NULL)
		{
			// Now call the event to process the create
			bSuccess = GameSearchDataStore->eventSubmitGameSearch(UUIInteraction::GetPlayerControllerId(PlayerIndex));
		}
	}
	// Activate the output links based upon with the find call worked
	if (bSuccess == TRUE)
	{
		OutputLinks(1).ActivateOutputLink();
	}
	else
	{
		OutputLinks(0).ActivateOutputLink();
	}
}


/**
 * Finds the UI list connected to the targets array and then stores the number of items it contains to the output variable of the action.
 */
IMPLEMENT_CLASS(UUIAction_GetListItemCount)

void UUIAction_GetListItemCount::Activated(void)
{
	UUIList* TargetList = NULL;

	// Find the first list object in the targets array
	for (INT Index = 0; Index < Targets.Num(); Index++)
	{
		TargetList = Cast<UUIList>(Targets(Index));
		if (TargetList != NULL)
		{
			break;
		}
	}

	// If the user set a list as a target
	if (TargetList != NULL)
	{
		ItemCount = TargetList->GetItemCount();
	}
	else
	{
		ItemCount = 0;
	}
}
/**
 * Finds the UI list in question, gets the unique id of the player that is
 * hosting that server, and displays the gamercard for them
 */
void UUIAction_ShowGamercardForServerHost::Activated(void)
{
	UBOOL bSuccess = FALSE;
	UUIList* TargetList = NULL;
	// Find the first list object in the targets array
	for (INT Index = 0; Index < Targets.Num(); Index++)
	{
		TargetList = Cast<UUIList>(Targets(Index));
		if (TargetList != NULL)
		{
			break;
		}
	}
	// If the user set a list as a target
	if (TargetList != NULL)
	{
		const INT SelectedItem = TargetList->GetCurrentItem();

		if(SelectedItem!=INDEX_NONE)
		{
			// Check to see that the list is bound to a game search datastore
			UUIDataStore_OnlineGameSearch* GameSearchDataStore = Cast<UUIDataStore_OnlineGameSearch>(TargetList->DataSource.ResolvedDataStore);
			if (GameSearchDataStore)
			{
				// Tell the datastore to bring up the server's gamercard
				bSuccess = GameSearchDataStore->eventShowHostGamercard(UUIInteraction::GetPlayerControllerId(PlayerIndex),SelectedItem);
			}
		}
	}
	// Activate the output links based upon whether the call worked
	if (bSuccess == TRUE)
	{
		OutputLinks(1).ActivateOutputLink();
	}
	else
	{
		OutputLinks(0).ActivateOutputLink();
	}
}

/**
 * Finds the UI list in question and tells them to refresh the list
 */
void UUIAction_RefreshStats::Activated(void)
{
	Super::Activated();

	UBOOL bSuccess = FALSE;
	UUIList* TargetList = NULL;
	// Find the first list object in the targets array
	for (INT Index = 0; Index < Targets.Num(); Index++)
	{
		TargetList = Cast<UUIList>(Targets(Index));
		if (TargetList != NULL)
		{
			break;
		}
	}
	// If the user set a list as a target
	if (TargetList != NULL)
	{
		// Check to see that the list is bound to a stats datastore
		TargetList->DataSource.ResolveMarkup(TargetList);
		UUIDataStore_OnlineStats* StatsDataStore = Cast<UUIDataStore_OnlineStats>(TargetList->DataSource.ResolvedDataStore);
		if (StatsDataStore)
		{
			// Tell the datastore to refresh
			bSuccess = StatsDataStore->eventRefreshStats(UUIInteraction::GetPlayerControllerId(PlayerIndex));
		}
	}
	// Activate the output links based upon whether the call worked
	if (bSuccess == TRUE)
	{
		OutputLinks(1).ActivateOutputLink();
	}
	else
	{
		OutputLinks(0).ActivateOutputLink();
	}
}

/**
 * Finds the datastore and tells it to save its data
 */
void UUIAction_SaveProfileSettings::Activated(void)
{
	UBOOL bSuccess = FALSE;
	bIsDone = TRUE;

	// Get the scene client so we can find the datastore by name
	UGameUISceneClient* SceneClient = UUIRoot::GetSceneClient();
	if (SceneClient != NULL && GEngine->GamePlayers.IsValidIndex(PlayerIndex))
	{
		// Find the datastore by name and verify it's of the correct type
		UUIDataStore* DataStore = SceneClient->DataStoreManager->FindDataStore(TEXT("OnlinePlayerData"), GEngine->GamePlayers(PlayerIndex));
		if (DataStore)
		{
			UUIDataStore_OnlinePlayerData* OnlinePlayerDataStore = Cast<UUIDataStore_OnlinePlayerData>(DataStore);
			// It's possible that the user entered the name of the wrong datastore
			if (OnlinePlayerDataStore != NULL)
			{

				// Register our profile write complete delegate.
				eventRegisterDelegate();

				// Now call the event to process the profile save
				bSuccess = OnlinePlayerDataStore->eventSaveProfileData();

				if(!bSuccess)
				{
					eventClearDelegate();
				}
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("The data store not found for UIAction_SaveProfileSettings with PlayerIndex: %i"), PlayerIndex);
		}
	}
}

/**
 * Gets the current NAT type from the OnlineSubsystem and then activates the appropriate action.
 */
void UUIAction_GetNATType::Activated(void)
{
	BYTE NATType=0;
	INT OutputLinkIdx=0;

	if(bAlwaysOpen==TRUE)
	{
		OutputLinkIdx = 1;
	}
	else
	{
		if(eventGetNATType(NATType))
		{
			switch((ENATType)NATType)
			{
			case NAT_Unknown:
				OutputLinkIdx=0;
				break;
			case NAT_Open:
				OutputLinkIdx=1;
				break;
			case NAT_Moderate:
				OutputLinkIdx=2;
				break;
			case NAT_Strict:
				OutputLinkIdx=3;
				break;
			}
		}
	}

	OutputLinks(OutputLinkIdx).ActivateOutputLink();
}

/**
 * Checks to see how many players are currently signed in and stores them using VariableLinks.
 */
void UUIAction_GetLoggedInPlayerCount::Activated(void)
{
	eventGetLoginStatus();
	Super::Activated();
}

/**
 * Searches for an event in the current level with the specified EventTag and if found, activates it.
 */
void UUIAction_ActivateLevelEvent::Activated()
{
	APlayerController* PlayerOwner = NULL;

	// assign the player that activated this event as the Instigator for the remote event
	UUIScreenObject* Owner = GetOwner();
	if ( Owner != NULL )
	{
		if ( !GEngine->GamePlayers.IsValidIndex(PlayerIndex) )
		{
			PlayerIndex = 0;
		}

		ULocalPlayer* LocalPlayer = Owner->GetPlayerOwner(PlayerIndex);
		if ( LocalPlayer != NULL )
		{
			PlayerOwner = LocalPlayer->Actor;
		}
	}

	UBOOL bFoundRemoteEvents = FALSE;

	// now find the level's sequence
	AWorldInfo* WorldInfo = GetWorldInfo();
	if ( WorldInfo != NULL )
	{
		USequence* GameSequence = WorldInfo->GetGameSequence();
		if ( GameSequence != NULL )
		{
			// now find the root sequence
			USequence* RootSequence = GameSequence->GetRootSequence();
			if ( RootSequence != NULL )
			{
				TArray<USeqEvent_RemoteEvent*> RemoteEvents;
				RootSequence->FindSeqObjectsByClass(USeqEvent_RemoteEvent::StaticClass(), (TArray<USequenceObject*>&)RemoteEvents);
				
				// now iterate through the list of events, activating those events that have a matching event tag
				for ( INT EventIndex = 0; EventIndex < RemoteEvents.Num(); EventIndex++ )
				{
					USeqEvent_RemoteEvent* RemoteEvt = RemoteEvents(EventIndex);
					if ( RemoteEvt != NULL && RemoteEvt->EventName == EventName && RemoteEvt->bEnabled == TRUE )
					{
						// attempt to activate the remote event
						RemoteEvt->CheckActivate(WorldInfo, PlayerOwner);
						bFoundRemoteEvents = TRUE;
					}
				}
			}
		}
	}

	OutputLinks(bFoundRemoteEvents==TRUE ? 1 : 0).ActivateOutputLink();
}

/* ==========================================================================================================
	UITabControl related events and actions
========================================================================================================== */
/** USequenceOp interface */
/**
 * Allows the operation to initialize the values for any VariableLinks that need to be filled prior to executing this
 * op's logic.  This is a convenient hook for filling VariableLinks that aren't necessarily associated with an actual
 * member variable of this op, or for VariableLinks that are used in the execution of this ops logic.
 *
 * Initializes the value of the "OwnerTabControl" linked variable by copying the value of the activator tab page's tab
 * control into the linked variable.
 */
void UUIEvent_TabControl::InitializeLinkedVariableValues()
{
	Super::InitializeLinkedVariableValues();

	UUITabPage* TabPageActivator = Cast<UUITabPage>(EventActivator);
	if ( TabPageActivator == NULL )
	{
		UUITabButton* TabButtonActivator = Cast<UUITabButton>(EventActivator);
		if ( TabButtonActivator != NULL )
		{
			TabPageActivator = TabButtonActivator->TabPage;
		}
	}

	if ( TabPageActivator != NULL )
	{
		// initialize the Tab Page variable link
		TArray<UObject**> ObjVars;
		GetObjectVars(ObjVars,TEXT("Tab Page"));
		for (INT Idx = 0; Idx < ObjVars.Num(); Idx++)
		{
			*(ObjVars(Idx)) = TabPageActivator;
		}

		UUITabControl* TabControl = TabPageActivator->GetOwnerTabControl();
		if ( TabControl != NULL )
		{
			OwnerTabControl = TabControl;
		}
	}
}







// EOF

