/*=============================================================================
	UnUIObjects.cpp: UI system component implementations.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
	
	@todo - when widgets are deleted, need to remove its sequence from its parent's sequence
	@todo - when widgets are reparented, need to update the ParentSequence for the widget's sequence
=============================================================================*/

#include "EnginePrivate.h"

#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "UnUIMarkupResolver.h"

#include "EngineMaterialClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineUISequenceClasses.h"

IMPLEMENT_CLASS(UUIComponent);
	IMPLEMENT_CLASS(UUIComp_Event);
	IMPLEMENT_CLASS(UUIComp_ListComponentBase);
		IMPLEMENT_CLASS(UUIComp_ListPresenter);
			IMPLEMENT_CLASS(UUIComp_ListPresenterCascade);
				IMPLEMENT_CLASS(UUIComp_ContextMenuListPresenter);
			IMPLEMENT_CLASS(UUIComp_ListPresenterTree);
		IMPLEMENT_CLASS(UUIComp_ListElementSorter);

	IMPLEMENT_CLASS(UUIComp_DrawComponents);

		IMPLEMENT_CLASS(UUIComp_DrawString);
			IMPLEMENT_CLASS(UUIComp_DrawStringEditbox);
			IMPLEMENT_CLASS(UUIComp_DrawStringSlider);
			IMPLEMENT_CLASS(UUIComp_DrawCaption);

		IMPLEMENT_CLASS(UUIComp_DrawImage);

	IMPLEMENT_CLASS(UUIComp_AutoAlignment);


DECLARE_CYCLE_STAT(TEXT("GetStringFormatParms Time"),STAT_UIGetStringFormatParms,STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("ApplyStringFormatting Time"),STAT_UIApplyStringFormatting,STATGROUP_UI);
//DECLARE_CYCLE_STAT(TEXT("ResolvePosition Time (String)"),STAT_UIResolvePosition_String,STATGROUP_UI);
//DECLARE_CYCLE_STAT(TEXT("ResolvePosition Time (List)"),STAT_UIResolvePosition_List,STATGROUP_UI);
//DECLARE_CYCLE_STAT(TEXT("ResolvePosition Time (AutoAlignment)"),STAT_UIResolvePosition_AutoAlignment,STATGROUP_UI);


inline DWORD GetTypeHash(const IUIEventContainer* ContainerInstance)
{
	return PointerHash(ContainerInstance);
}

/* ==========================================================================================================
	UUIComponent
========================================================================================================== */
/**
 * Builds a list of objects which have this object in their archetype chain.
 *
 * All archetype propagation for UIScreenObjects is handled by the UIPrefab/UIPrefabInstance code, so this version just
 * skips the iteration unless we're a CDO.
 *
 * @param	Instances	receives the list of objects which have this one in their archetype chain
 */
void UUIComponent::GetArchetypeInstances( TArray<UObject*>& Instances )
{
	if ( HasAnyFlags(RF_ClassDefaultObject) )
	{
		Super::GetArchetypeInstances(Instances);
	}
}

/**
 * Serializes all objects which have this object as their archetype into GMemoryArchive, then recursively calls this function
 * on each of those objects until the full list has been processed.
 * Called when a property value is about to be modified in an archetype object.
 *
 * Since archetype propagation for UIScreenObjects is handled by the UIPrefab code, this version simply routes the call 
 * to the owning UIPrefab so that it can handle the propagation at the appropriate time.
 *
 * @param	AffectedObjects		ignored
 */
void UUIComponent::SaveInstancesIntoPropagationArchive( TArray<UObject*>& AffectedObjects )
{
	UUIPrefab* OwnerPrefab = NULL;
	if ( GetOuterUUIScreenObject()->IsInUIPrefab(&OwnerPrefab) || GetOuterUUIScreenObject()->IsA(UUIPrefab::StaticClass()) )
	{
		checkSlow(OwnerPrefab);
		OwnerPrefab->SaveInstancesIntoPropagationArchive(AffectedObjects);
	}

	// otherwise, we just swallow this call - UIScreenObjects should never execute the UObject version
}

/**
 * De-serializes all objects which have this object as their archetype from the GMemoryArchive, then recursively calls this function
 * on each of those objects until the full list has been processed.
 *
 * Since archetype propagation for UIScreenObjects is handled by the UIPrefab code, this version simply routes the call 
 * to the owning UIPrefab so that it can handle the propagation at the appropriate time.
 *
 * @param	AffectedObjects		the array of objects which have this object in their ObjectArchetype chain and will be affected by the change.
 *								Objects which have this object as their direct ObjectArchetype are removed from the list once they're processed.
 */
void UUIComponent::LoadInstancesFromPropagationArchive( TArray<UObject*>& AffectedObjects )
{
	UUIPrefab* OwnerPrefab = NULL;
	if ( GetOuterUUIScreenObject()->IsInUIPrefab(&OwnerPrefab) || GetOuterUUIScreenObject()->IsA(UUIPrefab::StaticClass()) )
	{
		checkSlow(OwnerPrefab);
		OwnerPrefab->LoadInstancesFromPropagationArchive(AffectedObjects);
	}

	// otherwise, we just swallow this call - UIScreenObjects should never execute the UObject version
}

/**
 * Called just after a property in this object's archetype is modified, immediately after this object has been de-serialized
 * from the archetype propagation archive.
 *
 * Allows objects to perform reinitialization specific to being de-serialized from an FArchetypePropagationArc and
 * reinitialized against an archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
 *
 * This version requests a full scene update.
 */
void UUIComponent::PostSerializeFromPropagationArchive()
{
	GetOuterUUIScreenObject()->RequestSceneUpdate(TRUE, TRUE, TRUE, TRUE);
	GetOuterUUIScreenObject()->RequestFormattingUpdate();

	Super::PostSerializeFromPropagationArchive();
}

/**
 * Determines whether this object is contained within a UIPrefab.
 *
 * @param	OwnerPrefab		if specified, receives a pointer to the owning prefab.
 *
 * @return	TRUE if this object is contained within a UIPrefab; FALSE if this object IS a UIPrefab or is not
 *			contained within a UIPrefab.
 */
UBOOL UUIComponent::IsAPrefabArchetype( UObject** OwnerPrefab/*=NULL*/ ) const
{
	UBOOL bResult = FALSE;
	
	UUIScreenObject* WidgetOwner = GetOuterUUIScreenObject();
	if ( WidgetOwner != NULL )
	{
		bResult = WidgetOwner->IsAPrefabArchetype(OwnerPrefab);
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
UBOOL UUIComponent::IsInPrefabInstance() const
{
	UBOOL bResult = FALSE;

	UUIScreenObject* WidgetOwner = GetOuterUUIScreenObject();
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
	UUIComp_Event
========================================================================================================== */
/**
 * Returns the widget associated with this event provider.
 */
UUIScreenObject* UUIComp_Event::GetOwner() const
{
	return GetOuterUUIScreenObject();
}

/**
 * Called when the screen object that owns this UIComp_Event is created.  Creates the UISequence which will contain
 * the events for this widget, instances any objects assigned to the DefaultEvents array and adds those instances
 * to the sequence container.
 */
void UUIComp_Event::Created()
{
	SetFlags(RF_Transactional|GetOuter()->GetMaskedFlags(RF_Transient|RF_ArchetypeObject));
	if ( EventContainer == NULL )
	{
		Modify(TRUE);
		EventContainer = CreateEventContainer();

		// can't link the parent sequence here because our owning widget might not have been initialized yet
		// (which means it's Owner pointer won't be set yet)
	}
	check(EventContainer);
}

/**
 * Determines which sequences should be instanced for the widget that owns this event component.  Note that this method
 * does not care whether the sequences have ALREADY been instanced - it just determines whether a sequence should be instanced
 * in the case where the corresponding sequence container has a NULL sequence.
 *
 * @param	out_EventsToInstance	will receive the list of indexes of the event templates which have linked ops, thus need to be instanced
 *
 * @return	TRUE if the global sequence for this component should be instanced.
 */
UBOOL UUIComp_Event::ShouldInstanceSequence( TArray<INT>& out_EventsToInstance )
{
	UBOOL bResult = FALSE;
	
	out_EventsToInstance.Empty();
	if ( GIsGame == TRUE )
	{
		UUIScreenObject* Owner = GetOwner();
		if ( Owner != NULL )
		{
			if ( Owner->IsA(UUIScene::StaticClass()) )
			{
				// always instance the global sequence for UIScenes, as the scene's sequence acts as the top-level parent for all sequences in the scene
				bResult = TRUE;
			}

			/* if a widget doesn't already have a sequence when its initialized during the game, it means that either:
				a) the widget was created at runtime during gameplay
				b) the widget was created in the UI editor, but the widget's sequence didn't contain any active ops when the scene was saved,
					thus the widget's sequence was marked as RF_NotForClient|RF_NotForServer and so wasn't loaded
			
			Thus, there is no need to instance a widget's sequence if none of the event templates have linked ops.  Since there's no way to
			add new sequence ops during the game, only actions linked to the default event templates would be executed by widgets that don't already
			have a sequence when they are initialized during the game
			*/
			for ( INT DefaultIndex = 0; DefaultIndex < DefaultEvents.Num(); DefaultIndex++ )
			{
				FDefaultEventSpecification& EventSpec = DefaultEvents(DefaultIndex);
				if ( ShouldInstanceDefaultEvent(DefaultIndex) )
				{
					// this event template has ops linked to it, so we'll need to instance the sequence
					out_EventsToInstance.AddItem(DefaultIndex);
					bResult = TRUE;
				}
			}
		}
	}
	else
	{
		// always instance sequences in the editor
		bResult = TRUE;
		for ( INT DefaultIndex = 0; DefaultIndex < DefaultEvents.Num(); DefaultIndex++ )
		{
			out_EventsToInstance.AddItem(DefaultIndex);
		}
	}

	return bResult;
}

/**
 * Determines whether the specified event template should be instanced when this event component is initializing its sequence.
 *
 * @param	DefaultIndex	index into the DefaultEvents array for the event to check
 *
 * @return	returns TRUE if the event located at the specified index is valid for instancing; FALSE otherwise.
 *			Note that this function does not care whether the event has ALREADY been instanced or not - just whether
 *			it is valid to instance that event.
 */
UBOOL UUIComp_Event::ShouldInstanceDefaultEvent( INT DefaultIndex )
{
	UBOOL bResult = FALSE;

	if ( DefaultEvents.IsValidIndex(DefaultIndex) )
	{
		// in the game, only instance the event template if it has linked ops or this event type should always be instanced
		// (indicated by returning TRUE for the UIEvent.ShouldAlwaysInstance event)
		if ( GIsGame == TRUE )
		{
			UUIEvent* EventTemplate = DefaultEvents(DefaultIndex).EventTemplate;
			if ( EventTemplate != NULL )
			{
				bResult = EventTemplate->HasLinkedOps();
				if ( bResult == FALSE )
				{
					bResult = EventTemplate->eventShouldAlwaysInstance();
				}
			}
		}
		else
		{
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Creates the sequence for this event component
 *
 * @param	SequenceName	optionally specify the name for the sequence container....used by the T3D import code to
 *							make sure that the new sequence can be resolved by other objects which reference it
 *
 * @return	a pointer to a new UISequence which has this component as its Outer
 */
UUISequence* UUIComp_Event::CreateEventContainer( FName SequenceName/*=NAME_None*/ ) const
{
	// if this event component is owned by a widget instanced from a UIPrefab, find the correct archetype before
	// creating the component
	UUISequence* SequenceArchetype = NULL;
	EObjectFlags Flags = RF_Transactional|GetMaskedFlags(RF_Transient|RF_ArchetypeObject);

	UUIScreenObject* Owner = GetOwner();
	check(Owner);

	if ( Owner->IsInPrefabInstance() )
	{
		SequenceArchetype = GetArchetype<UUIComp_Event>()->EventContainer;
	}
	else if ( Owner->HasAnyFlags(RF_ArchetypeObject) || Owner->IsInUIPrefab() || Owner->IsA(UUIPrefab::StaticClass()) )
	{
		Flags |= RF_Public;
	}
	
	UUISequence* Result;
	if ( GIsGame )
	{
		Result = ConstructObject<UUISequence>(UUISequence::StaticClass(), const_cast<UUIComp_Event*>(this), SequenceName, 
			Flags, SequenceArchetype);
	}
	else
	{
		// if we're in the editor, create the sequence in the transient package with a transient name then set the correct outer/name
		// after calling Modify().  This is so that the same name can be used the next time, after undoing the creation of this sequence.
		Result = ConstructObject<UUISequence>(UUISequence::StaticClass(), GetTransientPackage(), NAME_None, 
			Flags, SequenceArchetype);

		Result->Modify();

		// remove RF_Public before renaming to ensure that we don't create a redirect
		const EObjectFlags OriginalFlags = Result->GetFlags();
		Result->ClearFlags(RF_Public);
		Result->Rename( SequenceName != NAME_None ? *SequenceName.ToString() : *Result->GetName(), const_cast<UUIComp_Event*>(this), REN_ForceNoResetLoaders );
		Result->SetFlags(OriginalFlags);
	}
	Result->ObjName = TEXT("Global_Sequence");

	return Result;
}

/**
 * Creates a UIEvent_ProcessInput object for routing input events to actions.
 */
void UUIComp_Event::CreateInputProcessor()
{
	check(EventContainer);
	if ( InputProcessor == NULL )
	{
		InputProcessor = ConstructObject<UUIEvent_ProcessInput>(UUIEvent_ProcessInput::StaticClass(), EventContainer, NAME_None, 
			RF_Transient|EventContainer->GetMaskedFlags(RF_ArchetypeObject|RF_Public));
	}
}


/**
 * Initializes the sequence associated with this event component.  Assigns the parent sequence for the EventContainer
 * to the UISequence associated with the widget that owns this component's owner widget.  Only called during the game.
 */
void UUIComp_Event::InitializeEventProvider( UBOOL bInitializeSequence/*=GIsGame*/ )
{
	TArray<INT> EventsToInstance;
	if ( ShouldInstanceSequence(EventsToInstance) && EventContainer == NULL )
	{
		Created();
	}

	UUIScreenObject* Owner = GetOwner();
	if ( Owner != NULL )
	{
		if ( EventContainer != NULL )
		{
			// link up this sequence to our parent widget's sequence
			SetParentSequence();

			// this maps the DefaultStates array to the UIState instance of that class living in the owning widget's InactiveStates array
			TMap<UClass*,UUIState*> StateInstanceMap;

			// first, get all of the UIEvent containers that could possibly contain events instanced from the DefaultEvents array
			Owner->GetInstancedStates(StateInstanceMap);

			// make sure that all states have a valid sequence
			for ( INT StateIndex = 0; StateIndex < Owner->InactiveStates.Num(); StateIndex++ )
			{
				UUIState* State = Owner->InactiveStates(StateIndex);
				if ( State && State->StateSequence == NULL )
				{
					UClass* StateClass = State->GetClass();

					// if the state's class isn't found in the StateInstanceMap, it indicates that the designer added this state in the UI editor.
					UBOOL bShouldInstanceStateSequence = !GIsGame || StateInstanceMap.Find(StateClass) == NULL;
					if ( !bShouldInstanceStateSequence )
					{
						for ( INT EventIndex = 0; EventIndex < EventsToInstance.Num(); EventIndex++ )
						{
							FDefaultEventSpecification& EventSpec = DefaultEvents(EventsToInstance(EventIndex));
							if ( EventSpec.EventState == StateClass )
							{
								bShouldInstanceStateSequence = TRUE;
								break;
							}
						}
					}

					// only instance the sequences for a state if there is an event template that is scoped to that state which has linked ops
					if ( bShouldInstanceStateSequence == TRUE )
					{
						State->Created();
					}
				}
			}

			InstanceEventTemplates(StateInstanceMap, EventsToInstance);
			if ( bInitializeSequence )
			{
				CreateInputProcessor();
				EventContainer->InitializeSequence();

				// now initialize all of the state sequences
				for ( INT StateIndex = 0; StateIndex < Owner->InactiveStates.Num(); StateIndex++ )
				{
					UUIState* InactiveState = Owner->InactiveStates(StateIndex);
					if ( InactiveState != NULL && InactiveState->StateSequence != NULL )
					{
						InactiveState->StateSequence->InitializeSequence();
					}
				}
			}
		}
		else
		{
			debugfSlow(NAME_DevUI, TEXT("Not instancing global sequence for '%s'"), *GetFullName());
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Invalid owner for event provider '%s'"), *GetFullName());
	}
}

/**
 * Assigns the parent sequence for this widget's global sequence to the sequence owned by the first widget in the parent chain that has a valid global sequence.
 */
void UUIComp_Event::SetParentSequence()
{
	// we shouldn't be here unless we actually have a global sequence
	check(EventContainer);

	UUIScreenObject* Owner = GetOwner();
	UUIScreenObject* OwnerParent = Owner->GetParent();

	UUISequence* ParentSequence = NULL;

	// the only type of widget that should not have a parent is a UIScene, so if the widget that owns this event
	// provider doesn't have a parent widget and it isn't a UIScene, complain
	if ( OwnerParent == NULL )
	{
		if ( Owner->GetScene() != Owner && !Owner->IsA(UUIPrefab::StaticClass()) )
		{
			debugf(NAME_Error,TEXT("%s: Invalid parent widget for Owner: %s"), *GetFullName(), *Owner->GetPathName());
		}
	}
	else
	{
		for ( ; OwnerParent; OwnerParent = OwnerParent->GetParent() )
		{
			if ( OwnerParent->EventProvider != NULL )
			{
				if ( OwnerParent->EventProvider->EventContainer != NULL )
				{
					ParentSequence = OwnerParent->EventProvider->EventContainer;
					break;
				}
			}
			else
			{
				debugf(NAME_Error, TEXT("%s: NULL EventProvider for parent widget '%s'"), *GetFullName(), *OwnerParent->GetFullName());
			}
		}

		// since the scene should always have a valid sequence, we are guaranteed to find a valid ParentSequence
		checkf(ParentSequence, TEXT("%s: No valid parent sequence found for widget '%s' (%s)"), *GetFullName(), *Owner->GetWidgetPathName(), *Owner->GetFullName());
	}

	if ( EventContainer->ParentSequence != ParentSequence )
	{
		//@fixme - currently we'll cleanup ParentSequence references, but the UI editor should prevent this from happening in the first place
		if ( EventContainer->ParentSequence != NULL )
		{
			debugfSuppressed(NAME_DevUI, TEXT("%s: Correcting ParentSequence reference - old:%s  new:%s"), *GetFullName(), *EventContainer->ParentSequence->GetFullName(), *ParentSequence->GetFullName());
			CleanupEventProvider();
		}

		EventContainer->ParentSequence = ParentSequence;

		//@fixme ronp - for now, simply add this sequence as a subsequence of the parent sequence...later, figure out
		// out a better way to update sequences in individual widgets without calling UpdateOp() throughout the entire scene graph
		if ( ParentSequence != NULL )
		{
			if ( ParentSequence->AddSequenceObject(EventContainer) )
			{
				ParentSequence->AddNestedSequence(EventContainer);
			}
		}
	}
}

/**
 * Creates instances for any newly attached actions, variables, etc. that were declared in the class defaultproperties which don't exist in the sequence.
 *
 * @param	StateInstanceMap	maps the DefaultStates array to the UIState instance of that class living in the owning widget's InactiveStates array
 * @param	EventsToInstance	the indexes for the elements of the DefaultEvents array which should be instanced.
 */
void UUIComp_Event::InstanceEventTemplates( TMap<UClass*,UUIState*>& StateInstanceMap, const TArray<INT>& EventsToInstance )
{
	if ( DefaultEvents.Num() > 0 )
	{
		UUIScreenObject* OwnerWidget = GetOwner();

		// now, build a map of event containers to existing events
		TMultiMap<IUIEventContainer*,UUIEvent*> ExistingEventMap;
		GetInstancedEvents(StateInstanceMap,ExistingEventMap);

		// now go through the list of event templates
		for ( INT DefaultIndex = 0; DefaultIndex < DefaultEvents.Num(); DefaultIndex++ )
		{
			FDefaultEventSpecification& EventSpec = DefaultEvents(DefaultIndex);
			UUIEvent* DefaultEvent = EventSpec.EventTemplate;
			IUIEventContainer* TargetContainer = NULL;

			TArray<UUIEvent*> ExistingEvents;
			if ( EventSpec.EventState != NULL )
			{
				UUIState* StateInstance = StateInstanceMap.FindRef(EventSpec.EventState);
				if ( StateInstance != NULL )
				{
					TargetContainer = StateInstance;
					ExistingEventMap.MultiFind(StateInstance, ExistingEvents);
				}
				else
				{
					// if there is no instance for the UIState associated with this event, it means that either:
					// a) this is an inherited event which this widget doesn't support
					// b) this widget used to support this state, but no longer does

					// in either case, the appropriate response is to just skip instancing this event.
					continue;
				}
			}
			else
			{
				TargetContainer = EventContainer;
				ExistingEventMap.MultiFind(EventContainer, ExistingEvents);
			}

			check(TargetContainer);

			UUIEvent_ProcessInput* DefaultInputProcessor = Cast<UUIEvent_ProcessInput>(DefaultEvent);
			if ( DefaultInputProcessor != NULL )
			{
				// then, if this sequence container contains instances of this event, remove them now
				TArray<UUIEvent*> InstancedEvents;
				if ( ContainsObjectOfClass<UUIEvent>(ExistingEvents, DefaultInputProcessor->GetClass(), TRUE, &InstancedEvents) )
				{
					TargetContainer->RemoveSequenceObjects( *((TArray<USequenceObject*>*)&InstancedEvents) );
				}

				continue;
			}

			const UBOOL bUIPrefabInstance = OwnerWidget->IsInPrefabInstance() || OwnerWidget->IsA(UUIPrefabInstance::StaticClass());
			const UBOOL bUIPrefab = OwnerWidget->IsInUIPrefab() || OwnerWidget->IsA(UUIPrefab::StaticClass());

			// for each of our DefaultEvents, check to see if we have any instances of that UIEvent class in our list of instanced events
			TArray<UUIEvent*> InstancedEvents;
			if ( ContainsObjectOfClass<UUIEvent>(ExistingEvents, DefaultEvent->GetClass(), TRUE, &InstancedEvents) )
			{
				for ( INT InstanceIndex = 0; InstanceIndex < InstancedEvents.Num(); InstanceIndex++ )
				{
					UUIEvent* EventInstance = InstancedEvents(InstanceIndex);
					if ( EventInstance->GetArchetype() != DefaultEvent && !bUIPrefabInstance && !bUIPrefab )
					{
						KISMET_LOG_REF(EventContainer)(TEXT("Replacing archetype for instanced event: %s #### Original: %s #### New: '%s'"), *EventInstance->GetFullName(), *EventInstance->GetArchetype()->GetPathName(), *DefaultEvent->GetPathName());
					
						// if the event instance's template isn't the event template from defaultproperties, then we'll assume that means
						// that this event template was added to this UIComp_Event after it had been placed in a map, and a designer had
						// already manually added an instance of this event class to this widget's sequence
						EventInstance->SetArchetype(DefaultEvent,TRUE);
					}

					// instance any ops attached to this event which have been added to the class since the last time this package was saved
					InitializeInstancedOp(TargetContainer,EventInstance);
				}
			}
			else if ( EventsToInstance.FindItemIndex(DefaultIndex) != INDEX_NONE )
			{
				//@todo ronp uiprefabs: this doesn't work correctly for instances of a UIPrefab
				// the widget instance doesn't contain any events of this UIEvent class in its list of events, which we'll assume means that
				// this event template was added to the UIComp_Event template for this widget after this package was saved
				UUIEvent* EventInstance = InstanceDefaultEvent(TargetContainer,DefaultIndex);
				InitializeInstancedOp(TargetContainer,EventInstance);
			}
			else
			{
				debugfSlow(NAME_DevUI, TEXT("Not instancing default event template '%s' in '%s' for '%s'"), *DefaultEvent->GetFullName(), *TargetContainer->GetUObjectInterfaceUIEventContainer()->GetFullName(), *GetFullName());
			}
		}
	}
}



/**
 * Creates an UIEvent instance using the DefaultEvent template located at the index specified.
 *
 * @param	TargetContainer		the UIEventContainer that will contain the newly instanced ops
 * @param	DefaultIndex	index into the DefaultEvents array for the template to use when creating the event
 *
 * @return	a pointer to the UIEvent instance that was creatd, or NULL if it couldn't be created for some reason
 */
UUIEvent* UUIComp_Event::InstanceDefaultEvent( IUIEventContainer* TargetContainer, INT DefaultIndex )
{
	UUIEvent* EventInstance = NULL;
	if ( DefaultEvents.IsValidIndex(DefaultIndex) )
	{
		UUIEvent* EventTemplate = DefaultEvents(DefaultIndex).EventTemplate;
		UObject* InstanceOuter = TargetContainer->GetUObjectInterfaceUIEventContainer();

		// the outer for a default event should always be the widget that owns this component, not the sequence
		// otherwise, when I create the instance on the next line, the template will be overwritten since I use the same name
		// as the EventTemplate when constructing the object.  Having the same name and Outer would result in two objects with
		// identical paths.
		// so here we just verify that the event template's Outer is in fact a UIScreenObject
		UUIScreenObject* TemplateOuter = Cast<UUIScreenObject>(EventTemplate->GetOuter());
		check(TemplateOuter);

		EventInstance = ConstructObject<UUIEvent>(EventTemplate->GetClass(), InstanceOuter, EventTemplate->GetFName(),
			RF_Transactional|GetMaskedFlags(RF_Transient|RF_ArchetypeObject|RF_Public), EventTemplate, INVALID_OBJECT);
		EventInstance->OnCreated();
	}

	return EventInstance;
}

/**
 * Used for initializing sequence operations which have been instanced from event templates assigned to the
 * DefaultEvents array.  Iterates through the op's input links, output links, and variable links, instancing
 * any linked sequence objects which are contained within a class default object.
 *
 * @param	OpInstance	the SequenceOp to initialize.  This should either be a UIEvent created during
 *						UUIComp_Event::Created() or some other sequence op referenced by an script-declared
 * @todo native interfaces - with native interface support, the compiler could enforce the constraint that only UI related
 *							sequence objects should ever be passed as the value for OpInstance
 */
void UUIComp_Event::InitializeInstancedOp( IUIEventContainer* TargetContainer, USequenceOp* OpInstance )
{
	if ( TargetContainer->AddSequenceObject(OpInstance) )
	{
		UObject* InstanceOuter = TargetContainer->GetUObjectInterfaceUIEventContainer();

		UUIEvent* EventInstance = Cast<UUIEvent>(OpInstance);
		if ( EventInstance != NULL )
		{
			// set the EventOwner for the event to the widget that owns this component
			EventInstance->EventOwner = GetOwner();
		}

		// first, instance any ops referenced in the OutputLinks for OpInstance
		for ( INT LinkIndex = 0; LinkIndex < OpInstance->OutputLinks.Num(); LinkIndex++ )
		{
			FSeqOpOutputLink& OutputLink = OpInstance->OutputLinks(LinkIndex);
			for ( INT OpIndex = 0; OpIndex < OutputLink.Links.Num(); OpIndex++ )
			{
				FSeqOpOutputInputLink& ConnectedLink = OutputLink.Links(OpIndex);
				if ( ConnectedLink.LinkedOp && (ConnectedLink.LinkedOp->IsTemplate(RF_ClassDefaultObject)
					|| (!IsTemplate(RF_ArchetypeObject) && ConnectedLink.LinkedOp->IsTemplate(RF_ArchetypeObject))) )
				{
					// instance a unique copy of this op template
					USequenceOp* LinkedOpInstance = ConstructObject<USequenceOp>(ConnectedLink.LinkedOp->GetClass(),InstanceOuter,ConnectedLink.LinkedOp->GetFName(),
						RF_Transactional|GetMaskedFlags(RF_Transient|RF_ArchetypeObject|RF_Public),ConnectedLink.LinkedOp,INVALID_OBJECT);
					LinkedOpInstance->OnCreated();

					// now replace the reference with the instance
					ConnectedLink.LinkedOp = LinkedOpInstance;

					// recurse into this op to instance any templated ops it's referring to
					InitializeInstancedOp(TargetContainer, LinkedOpInstance);
				}
			}
		}

		// next, instance any ops referenced in the VariableLinks for OpInstance
		for ( INT LinkIndex = 0; LinkIndex < OpInstance->VariableLinks.Num(); LinkIndex++ )
		{
			FSeqVarLink& VarLink = OpInstance->VariableLinks(LinkIndex);
			for ( INT OpIndex = 0; OpIndex < VarLink.LinkedVariables.Num(); OpIndex++ )
			{
				USequenceVariable*& ConnectedVar = VarLink.LinkedVariables(OpIndex);
				if ( ConnectedVar && (ConnectedVar->IsTemplate(RF_ClassDefaultObject)
					|| (!IsTemplate(RF_ArchetypeObject) && ConnectedVar->IsTemplate(RF_ArchetypeObject))) )
				{
					// instance a unique copy of this op template
					USequenceVariable* ConnectedVarInstance = ConstructObject<USequenceVariable>(ConnectedVar->GetClass(),InstanceOuter,ConnectedVar->GetFName(),
						RF_Transactional|GetMaskedFlags(RF_Transient|RF_ArchetypeObject|RF_Public),ConnectedVar,INVALID_OBJECT);
					ConnectedVarInstance->OnCreated();

					// now replace the reference with the instance
					ConnectedVar = ConnectedVarInstance;
				}
			}
		}

		// finally, instance any ops referenced in the EventLinks for OpInstance
		for ( INT LinkIndex = 0; LinkIndex < OpInstance->EventLinks.Num(); LinkIndex++ )
		{
			FSeqEventLink& EventLink = OpInstance->EventLinks(LinkIndex);
			for ( INT OpIndex = 0; OpIndex < EventLink.LinkedEvents.Num(); OpIndex++ )
			{
				USequenceEvent*& LinkedEvent = EventLink.LinkedEvents(OpIndex);
				if ( LinkedEvent && (LinkedEvent->IsTemplate(RF_ClassDefaultObject)
					|| (!IsTemplate(RF_ArchetypeObject) && LinkedEvent->IsTemplate(RF_ArchetypeObject))) )
				{
					// instance a unique copy of this op template
					USequenceEvent* LinkedEventInstance = ConstructObject<USequenceEvent>(LinkedEvent->GetClass(),InstanceOuter,LinkedEvent->GetFName(),
						RF_Transactional|GetMaskedFlags(RF_Transient|RF_ArchetypeObject|RF_Public),LinkedEvent,INVALID_OBJECT);
					LinkedEventInstance->OnCreated();

					// now replace the reference with the instance
					LinkedEvent = LinkedEventInstance;

					// recurse into this op to instance any templated ops it's referring to
					InitializeInstancedOp(TargetContainer, LinkedEvent);
				}
			}
		}
	}
}

/**
 * Generates a list of UIEvent instances that have been previously created and added to either the widget's sequence
 * or one of its states.
 *
 * @param	ExistingEventMap	Will be filled with the list of previously instanced UIEvents, mapped to
 *								their corresponding containers
 */
void UUIComp_Event::GetInstancedEvents( TMap<UClass*,UUIState*>& StateInstanceMap, TMultiMap<IUIEventContainer*,UUIEvent*>& out_ExistingEventMap )
{
	// first, go through the DefaultEvents array and get scoped event instances from the owning widget's UIStates
	for ( INT DefaultIndex = 0; DefaultIndex < DefaultEvents.Num(); DefaultIndex++ )
	{
		FDefaultEventSpecification& EventSpec = DefaultEvents(DefaultIndex);
		if ( EventSpec.EventState != NULL )
		{
			UUIState** pStateInstance = StateInstanceMap.Find(EventSpec.EventState);
			if ( pStateInstance != NULL )
			{
				UUIState* StateInstance = *pStateInstance;
				if ( !out_ExistingEventMap.HasKey(StateInstance) && StateInstance->StateSequence != NULL )
				{
					// we must use FindSeqObjectsByClass, rather than GetUIEvents, as the optimized UIEvents
					// list is not necessarily initialized by this point
					TArray<UUIEvent*> EventList;
					StateInstance->StateSequence->FindSeqObjectsByClass(UUIEvent::StaticClass(),(TArray<USequenceObject*>&)EventList, FALSE);

					for ( INT InstanceIndex = 0; InstanceIndex < EventList.Num(); InstanceIndex++ )
					{
						out_ExistingEventMap.Add(StateInstance,EventList(InstanceIndex));
					}
				}
			}
			else
			{
				StateInstanceMap.Set(EventSpec.EventState,NULL);
			}
		}
	}

	if ( EventContainer != NULL )
	{
		// then add the unscoped event instances from the widget's sequence; we must use FindSeqObjectsByClass, rather than GetUIEvents,
		// as the optimized UIEvents list is not necessarily initialized by this point
		TArray<UUIEvent*> EventList;
		EventContainer->FindSeqObjectsByClass(UUIEvent::StaticClass(),(TArray<USequenceObject*>&)EventList, FALSE);

		for ( INT InstanceIndex = 0; InstanceIndex < EventList.Num(); InstanceIndex++ )
		{
			out_ExistingEventMap.Add(EventContainer,EventList(InstanceIndex));
		}
	}
}

/**
 * Cleans up any references to objects contained in other widgets.  Called when the owning widget is removed from the scene.
 */
void UUIComp_Event::CleanupEventProvider()
{
	if ( EventContainer != NULL && EventContainer->ParentSequence != NULL )
	{
		// store the current parent sequence before calling RemoveObjects, as RemoveObjects may clear the ParentSequence pointer
		USequence* OldParentSequence = EventContainer->ParentSequence;

		// clear the references from the owning widget's parent Sequence to this widget's sequences
		OldParentSequence->RemoveObject(EventContainer);
		OldParentSequence->NestedSequences.RemoveItem(EventContainer);
	}
}

/**
 * Adds the input events for the specified state to the owning scene's InputEventSubscribers
 *
 * @param	InputEventOwner		the state that contains the input keys that should be registered with the scene
 * @param	PlayerIndex			the index of the player to register the input keys for
 */
void UUIComp_Event::RegisterInputEvents( UUIState* InputEventOwner, INT PlayerIndex )
{
	if ( InputEventOwner != NULL )
	{
		UUIScreenObject* Owner = GetOwner();
		if ( Owner != NULL )
		{
			UUIScene* OwnerScene = Owner->GetScene();
			check(OwnerScene);

			for ( INT EventIndex = 0; EventIndex < InputEventOwner->StateInputActions.Num(); EventIndex++ )
			{
				FInputKeyAction& InputKeyAction = InputEventOwner->StateInputActions(EventIndex);

				// add it to the input processor
				if ( InputProcessor != NULL )
				{
					InputProcessor->ActionMap.Add(InputKeyAction.InputKeyName, InputKeyAction);
				}

				// let the scene know that we're now responding to this input key event
				OwnerScene->SubscribeInputEvent(InputKeyAction.InputKeyName,Owner, PlayerIndex);
			}

			// Register all of the input keys corresponding to the input aliases assigned to this state in the owning widget
			UUIInteraction* Interaction = UUIRoot::GetCurrentUIController();
			FUIInputAliasClassMap* WidgetKeyMapping = Interaction->WidgetInputAliasLookupTable.FindRef(Owner->GetClass());

			if(WidgetKeyMapping != NULL)
			{
				TArray<const FUIInputAliasStateMap*>* StateMapPtr = WidgetKeyMapping->StateReverseLookupTable.Find(InputEventOwner->GetClass());

				// Loop through each alias supported by this state and add its bound keys to the subscribed input array only if
				// the alias isn't disabled by this widget.
				if(StateMapPtr != NULL)
				{
					for ( INT ClassIndex = 0; ClassIndex < StateMapPtr->Num(); ClassIndex++ )
					{
						const FUIInputAliasStateMap* StateMap = (*StateMapPtr)(ClassIndex);
						const INT AliasCount = StateMap->StateInputAliases.Num();
						
						for(INT AliasIdx = 0; AliasIdx < AliasCount; AliasIdx++)
						{
							const FUIInputActionAlias& Alias = StateMap->StateInputAliases(AliasIdx);
							const FName& AliasName = Alias.InputAliasName;

							if( DisabledEventAliases.ContainsItem(AliasName) == FALSE )
							{
								const INT NumKeys = Alias.LinkedInputKeys.Num();
								for(INT KeyIdx = 0; KeyIdx < NumKeys; KeyIdx++)
								{
									OwnerScene->SubscribeInputEvent(Alias.LinkedInputKeys(KeyIdx).InputKeyName, Owner, PlayerIndex);
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
 * Removes the input events for the specified state from the owning scene's InputEventSubscribers
 *
 * @param	InputEventOwner		the state that contains the input keys that should be removed from the scene
 * @param	PlayerIndex			the index of the player to unregister input keys for
 */
void UUIComp_Event::UnregisterInputEvents( UUIState* InputEventOwner, INT PlayerIndex )
{
	if ( InputEventOwner != NULL )
	{
		UUIScreenObject* Owner = GetOwner();
		if ( Owner != NULL )
		{
			UUIScene* OwnerScene = Owner->GetScene();
			check(OwnerScene);

			for ( INT EventIndex = 0; EventIndex < InputEventOwner->StateInputActions.Num(); EventIndex++ )
			{
				FInputKeyAction& InputKeyAction = InputEventOwner->StateInputActions(EventIndex);

				// add it to the input processor
				if ( InputProcessor != NULL )
				{
					InputProcessor->ActionMap.RemovePair(InputKeyAction.InputKeyName, InputKeyAction);
				}

				// if there are no more actions assigned to this input key,
				if ( InputProcessor == NULL || !InputProcessor->ActionMap.HasKey(InputKeyAction.InputKeyName) )
				{
					// let the scene know that we're no longer responding to this input key event
					OwnerScene->UnsubscribeInputEvent(InputKeyAction.InputKeyName,Owner, PlayerIndex);
				}
			}

			// Unregister all of the default input keys for this widget state.
			UUIInteraction* Interaction = GEngine->GameViewport->UIController;
			FUIInputAliasClassMap* WidgetKeyMapping = Interaction->WidgetInputAliasLookupTable.FindRef(Owner->GetClass());

			if(WidgetKeyMapping != NULL)
			{
				FUIInputAliasMap* StateMap = WidgetKeyMapping->StateLookupTable.Find(InputEventOwner->GetClass());
				if(StateMap != NULL)
				{
					TMultiMap<FName,FUIInputAliasValue>& InputKeyAliasLookupMaps = StateMap->InputAliasLookupTable;
					for( TMultiMap<FName,FUIInputAliasValue>::TConstIterator It(InputKeyAliasLookupMaps); It; ++It )
					{
						const FName& InputKeyName = It.Key();
						OwnerScene->UnsubscribeInputEvent(InputKeyName, Owner, PlayerIndex);
					}
				}
			}
		}
	}
}

/**
 * Adds the specified sub-sequence to the widget's list of nested sequences.
 *
 * @param	StateSequence	the sequence to add.  This should be a sequence owned by one of the UIStates in this
 *							widget's InactiveStates array.
 *
 * @return	TRUE if the sequence was successfully added to [or if it already existed] the widget's sequence
 */
UBOOL UUIComp_Event::PushStateSequence( UUIStateSequence* StateSequence )
{
	UBOOL bResult = FALSE;
	if ( StateSequence != NULL )
	{
		// if we have a valid state sequence, we must have a global sequence
		checkSlow(EventContainer);
		bResult = EventContainer->AddNestedSequence(StateSequence);
	}
	else
	{
		// simulate success in the game if we don't have a state sequence
		bResult = GIsGame;
	}

	return bResult;
}

/**
 * Removes the specified sub-sequence from the widget's list of nested sequences.
 *
 * @param	StateSequence	the sequence to remove.  This should be a sequence owned by one of the UIStates in this
 *							widget's InactiveStates array.
 *
 * @return	TRUE if the sequence was successfully removed [or wasn't in the list] from the widget's sequence
 */
UBOOL UUIComp_Event::PopStateSequence( UUIStateSequence* StateSequence )
{
	UBOOL bResult = FALSE;
	if ( StateSequence != NULL )
	{
		check(EventContainer);
		bResult = EventContainer->RemoveNestedSequence(StateSequence);
	}
	else
	{
		// simulate success in the game if we don't have a state sequence
		bResult = GIsGame;
	}

	return bResult;
}

void UUIComp_Event::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerVersion() < VER_FIXED_UISCROLLBARS && !IsTemplate(RF_ClassDefaultObject) )
	{
		DefaultEvents = GetArchetype<UUIComp_Event>()->DefaultEvents;
	}
}

/* ==========================================================================================================
	UIComp_DrawComponents
========================================================================================================== */

UBOOL UUIComp_DrawComponents::UpdateFade(FLOAT& NewOpacity)
{
	// Apply fading if needed

	if ( FadeType != EFT_None )
	{
		AWorldInfo* WorldInfo = GWorld->GetWorldInfo();

		FLOAT DeltaTime = WorldInfo->TimeSeconds - LastRenderTime;
		LastRenderTime = WorldInfo->TimeSeconds;

		DeltaTime *= WorldInfo->TimeDilation;

		if ( FadeType == EFT_Fading )
		{
			if (FadeTime > 0 )
			{
				FadeAlpha += ( FadeTarget - FadeAlpha ) * ( DeltaTime / FadeTime );
				FadeTime -= DeltaTime;
			}
	
			if (FadeTime <= 0.0f)
			{
				FadeType = EFT_None;
				FadeTime = 0.0f;
				FadeAlpha = FadeTarget;

				// FIXME: Add a UI Event as well

				if ( DELEGATE_IS_SET(OnFadeComplete) )
				{
					delegateOnFadeComplete(this);
				}
			}

			NewOpacity = FadeAlpha;
		}
		else if ( FadeType == EFT_Pulsing )
		{
			FadeTime += DeltaTime * FadeRate;
			FLOAT FadeRange = FadeAlpha - FadeTarget;
			FadeRange *= Abs( appSin(FadeTime) );

			NewOpacity = FadeTarget + FadeRange;;
		}
		
		// Clamp the opacity to 0.0 - 1.0 
		NewOpacity = Clamp<FLOAT>(NewOpacity, 0.f, 1.0f);
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Call this function to cause this UIComp_DrawString to fade to a given alpha target.
 */

void UUIComp_DrawComponents::Fade(FLOAT FromAlpha, FLOAT ToAlpha, FLOAT TargetFadeTime)
{
	FadeType 	= EFT_Fading;
	FadeAlpha 	= FromAlpha;
	FadeTarget 	= ToAlpha;
	FadeTime 	= TargetFadeTime;

	// Prime Last Render time
	LastRenderTime = GWorld->GetWorldInfo()->TimeSeconds;
	
}
/**
 * Call this function to cause this UIComp_DrawString to pulse between 0 and a given alpha target.
 */

void UUIComp_DrawComponents::Pulse(FLOAT MaxAlpha, FLOAT MinAlpha, FLOAT PulseRate)
{
	FadeType 	= EFT_Pulsing;
	FadeAlpha 	= MaxAlpha;
	FadeTarget	= MinAlpha;
	FadeRate 	= PulseRate;
	FadeTime 	= 0.0;

	// Prime Last Render time
	LastRenderTime = GWorld->GetWorldInfo()->TimeSeconds;

}

void UUIComp_DrawComponents::ResetFade()
{
	FadeType = EFT_None;
}



/* ==========================================================================================================
	UUIComp_DrawImage
========================================================================================================== */
/**
 * Changes the image for this component, creating the wrapper UITexture if necessary.
 *
 * @param	NewImage		the new texture or material to use in this component
 */
void UUIComp_DrawImage::SetImage( USurface* NewImage )
{
	const UBOOL bInitializeStyleData=(ImageRef==NULL);
	if ( ImageRef == NULL )
	{
		Modify();
		ImageRef = ConstructObject<UUITexture>(UUITexture::StaticClass(), this, NAME_None, RF_Transactional|GetMaskedFlags(RF_Transient|RF_ArchetypeObject|RF_Public));
	}

	ImageRef->Modify();
	ImageRef->ImageTexture = NewImage;

	if ( bInitializeStyleData && HasValidStyleReference() )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables image coordinate customization and changes the component's override coordinates to the value specified.
 *
 * @param	NewCoordinates	the UV coordinates to use for rendering this component's image
 */
void UUIComp_DrawImage::SetCoordinates( FTextureCoordinates NewCoordinates )
{
	if ( StyleCustomization.SetCustomCoordinates(NewCoordinates) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables image color customization and changes the component's override color to the value specified.
 *
 * @param	NewColor	the color to use for rendering this component's image
 */
void UUIComp_DrawImage::SetColor( FLinearColor NewColor )
{
	if ( StyleCustomization.SetCustomDrawColor(NewColor) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables opacity customization and changes the component's override opacity to the value specified.
 *
 * @param	NewOpacity	the Opacity to use for rendering this component's string
 */
void UUIComp_DrawImage::SetOpacity( FLOAT NewOpacity )
{
	if ( StyleCustomization.SetCustomOpacity(NewOpacity) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables image formatting customization and changes the component's formatting override data to the value specified.
 *
 * @param	Orientation		indicates which orientation to modify
 * @param	NewFormattingData	the new value to use for rendering this component's image.
 */
void UUIComp_DrawImage::SetFormatting( BYTE Orientation, FUIImageAdjustmentData NewFormattingData )
{
	if ( StyleCustomization.SetCustomFormatting((EUIOrientation)Orientation, NewFormattingData) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables image coordinate customization allowing the image to use the values from the applied style.
 */
void UUIComp_DrawImage::DisableCustomCoordinates()
{
	if ( StyleCustomization.EnableCustomCoordinates(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables image color customization allowing the image to use the values from the applied style.
 */
void UUIComp_DrawImage::DisableCustomColor()
{
	if ( StyleCustomization.EnableCustomDrawColor(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables font color customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawImage::DisableCustomOpacity()
{
	if ( StyleCustomization.EnableCustomOpacity(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables image formatting customization allowing the image to use the values from the applied style.
 */
void UUIComp_DrawImage::DisableCustomFormatting()
{
	if ( StyleCustomization.EnableCustomFormatting(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Returns the texture or material assigned to this component.
 */
USurface* UUIComp_DrawImage::GetImage() const
{
	return ImageRef != NULL ? ImageRef->GetSurface() : NULL;
}

/**
 * Returns TRUE if this component's UIStyleReference can be resolved into a valid UIStyle.
 *
 * @param	CurrentlyActiveSkin		the currently active skin; used for resolving the style reference's default style if it doesn't yet have a valid style id.
 */
UBOOL UUIComp_DrawImage::HasValidStyleReference( UUISkin* CurrentlyActiveSkin/*=NULL*/ )
{
	UBOOL bHasValidStyleReference = ImageStyle.GetResolvedStyle() != NULL;
	if ( !bHasValidStyleReference && CurrentlyActiveSkin != NULL )
	{
		bHasValidStyleReference = ImageStyle.AssignedStyleID.IsValid() || ImageStyle.GetDefaultStyleTag(CurrentlyActiveSkin) != NAME_None;
	}

	return bHasValidStyleReference;
}

/**
 * Returns the image style data being used by this image rendering component.  If the component's ImageStyle is not set, the style data
 * will be pulled from the owning widget's primary style.
 *
 * @param	DesiredMenuState	the menu state for the style data to retrieve; if not speicified, uses the owning widget's current menu state.
 * @param	SourceSkin			the skin to use for resolving this component's image style; only relevant when the component's image style is invalid
 *								(or if TRUE is passed for bClearExistingValue). If the image style is invalid and a value is not specified, returned value
 *								will be NULL.
 * @param	bClearExistingValue	used to force the component's image style to be re-resolved from the specified skin; if TRUE, you must supply a valid value for
 *								SourceSkin.
 *
 * @return	the image style data used to render this component's image for the specified menu state.
 */
UUIStyle_Image* UUIComp_DrawImage::GetAppliedImageStyle( UUIState* DesiredMenuState/*=NULL*/, UUISkin* SourceSkin/*=NULL*/, UBOOL bClearExistingValue/*=FALSE*/ )
{
	UUIStyle_Image* Result = NULL;
	UUIObject* OwnerWidget = GetOuterUUIObject();

	// if no menu state was specified, use the owning widget's current menu state
	if ( DesiredMenuState == NULL )
	{
		DesiredMenuState = OwnerWidget->GetCurrentState();
	}
	checkf(DesiredMenuState,TEXT("Cannot find a valid menu state for owning widget: %s (%i active states, %i inactive states)"),
		*OwnerWidget->GetPathName(), OwnerWidget->StateStack.Num(), OwnerWidget->InactiveStates.Num());

	// if this component's style reference hasn't been assigned a value, use the owning widget's PrimaryStyle
	const UBOOL bHasValidStyleReference = HasValidStyleReference(SourceSkin);
	if ( !bHasValidStyleReference )
	{
		checkf(OwnerWidget->bSupportsPrimaryStyle,TEXT("Unable to resolve component style reference (%s) for %s.%s and owner widget no longer supports the PrimaryStyle"),
			*ImageStyle.DefaultStyleTag.ToString(), *OwnerWidget->GetWidgetPathName(), *GetName());
	}

	const UBOOL bIsStyleManaged = OwnerWidget->IsPrivateBehaviorSet(UCONST_PRIVATE_ManagedStyle);
	FUIStyleReference& StyleToResolve = bHasValidStyleReference ? ImageStyle : OwnerWidget->PrimaryStyle;
	if ( bHasValidStyleReference && bClearExistingValue == TRUE && !bIsStyleManaged )
	{
		StyleToResolve.InvalidateResolvedStyle();
	}

	// get the UIStyle corresponding to the UIStyleReference that we're going to pull from
	// GetResolvedStyle() is guaranteed to return a valid style if a valid UISkin is passed in, unless the style reference is completely invalid

	// If the owner widget's style is being managed by someone else, then StyleToResolve's AssignedStyleID will always be zero, so never pass in
	// a valid skin reference.  This style should have already been resolved by whoever is managing the style, and if the resolved style doesn't live
	// in the currently active skin (for example, it's being inherited from a base skin package), GetResolvedStyle will clear the reference and reset
	// the ResolvedStyle back to the default style for this style reference.
	UUIStyle* ResolvedStyle = StyleToResolve.GetResolvedStyle(bIsStyleManaged ? NULL : SourceSkin);
	checkf(ResolvedStyle, TEXT("Unable to resolve style reference (%s)' for '%s.%s'"), *StyleToResolve.DefaultStyleTag.ToString(), *OwnerWidget->GetWidgetPathName(), *GetName());

	// rertrieve the style data for this menu state
	UUIStyle_Data* StyleData = ResolvedStyle->GetStyleForState(DesiredMenuState);
	check(StyleData);

	// if the specified style data corresponds to a combo style, retrieve the image style data from it
	Result = Cast<UUIStyle_Image>(StyleData);
	if ( Result == NULL )
	{
		UUIStyle_Combo* ComboStyleData = Cast<UUIStyle_Combo>(StyleData);
		if ( ComboStyleData != NULL )
		{
			Result = Cast<UUIStyle_Image>(ComboStyleData->ImageStyle.GetStyleData());
		}
	}

	return Result;
}
void UUIComp_DrawImage::execGetAppliedImageStyle( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT_OPTX(UUIState,DesiredMenuState,NULL);
	P_FINISH;
	*(UUIStyle_Image**)Result=GetAppliedImageStyle(DesiredMenuState);
}

/**
 * Applies the current style data (including any style data customization which might be enabled) to the component's image.
 */
void UUIComp_DrawImage::RefreshAppliedStyleData()
{
	if ( ImageRef == NULL )
	{
		// we have no image if we've never been assigned a value...if this is the case, create one
		// so that the style's DefaultImage will be rendererd
		SetImage(NULL);
	}
	else
	{
		// get the style data that should be applied to the image
		UUIStyle_Image* ImageStyleData = GetAppliedImageStyle();

		// ImageStyleData will be NULL if this component has never resolved its style
		if ( ImageStyleData != NULL )
		{
			// apply this component's per-instance image style settings
			FUICombinedStyleData FinalStyleData(ImageStyleData);
			CustomizeAppliedStyle(FinalStyleData);

			// apply the style data to the image
			ImageRef->SetImageStyle(FinalStyleData);
		}
	}
}

/**
 * Initializes the combinedstyledata using the component's current image sytle, then applies any per-instance values 
 * which are intended to override values in the style.
 *
 * @param	CustomizedStyleData		struct which receives the per-instance style data configured for this component;
 *									should be initialized using an image style prior to calling this function.
 */
void UUIComp_DrawImage::CustomizeAppliedStyle( FUICombinedStyleData& CustomizedStyleData ) const
{
	StyleCustomization.CustomizeDrawColor(CustomizedStyleData.ImageColor);
	StyleCustomization.CustomizeCoordinates(CustomizedStyleData.AtlasCoords);
	StyleCustomization.CustomizeFormatting(UIORIENT_Horizontal, CustomizedStyleData.AdjustmentType[UIORIENT_Horizontal]);
	StyleCustomization.CustomizeFormatting(UIORIENT_Vertical, CustomizedStyleData.AdjustmentType[UIORIENT_Vertical]);
}

/**
 * Renders the image.  The owning widget is responsible for applying any transformations to the canvas
 * prior to rendering this component.
 *
 * @param	Canvas		the canvas to render the image to
 * @param	Parameters	the bounds for the region that this texture can render to.
 */
void UUIComp_DrawImage::RenderComponent( FCanvas* Canvas, FRenderParameters Parameters )
{
	if ( ImageRef != NULL && Canvas != NULL )
	{
		// Fading
		FLOAT FadeOpacity;

		if ( UpdateFade(FadeOpacity) )
		{
			SetOpacity( FadeOpacity);
		}

		//@todo ronp - there is a discrepancy here in how the coordinates are set.  In widgets which have been updated to use the draw image
		// component, the style's AtlasCoords should always be used for rendering the image, but for widgets which are still using a UITexture
		// directly, they still have seperate coordinate variables
		StyleCustomization.CustomizeCoordinates(Parameters.DrawCoords);
		ImageRef->Render_Texture(Canvas, Parameters);
	}
}

/* === UIStyleResolver interface === */
/**
 * Returns the tag assigned to this UIStyleResolver by the owning widget
 */
FName UUIComp_DrawImage::GetStyleResolverTag()
{
	return StyleResolverTag;
}

/**
 * Changes the tag assigned to the UIStyleResolver to the specified value.
 *
 * @return	TRUE if the name was changed successfully; FALSE otherwise.
 */
UBOOL UUIComp_DrawImage::SetStyleResolverTag( FName NewResolverTag )
{
	UBOOL bResult = NewResolverTag != StyleResolverTag;

	StyleResolverTag = NewResolverTag;

	return bResult;
}

/**
 * Resolves the image style for this image rendering component.
 *
 * @param	ActiveSkin			the skin the use for resolving the style reference.
 * @param	bClearExistingValue	if TRUE, style references will be invalidated first.
 * @param	CurrentMenuState	the menu state to use for resolving the style data; if not specified, uses the current
 *								menu state of the owning widget.
 * @param	StyleProperty		if specified, only the style reference corresponding to the specified property
 *								will be resolved; otherwise, all style references will be resolved.
 */
UBOOL UUIComp_DrawImage::NotifyResolveStyle( UUISkin* ActiveSkin, UBOOL bClearExistingValue, UUIState* CurrentMenuState/*=NULL*/, const FName StylePropertyName/*=NAME_None*/ )
{
	UBOOL bResult = FALSE;
	if ( StylePropertyName == NAME_None || StylePropertyName == TEXT("ImageStyle") )
	{
		if ( ImageRef == NULL )
		{
			// we have no image if we've never been assigned a value...if this is the case, create one
			// so that the style's DefaultImage will be rendererd
			SetImage(NULL);
		}

		if ( ImageRef != NULL )
		{
			// get the style data that should be applied to the image
			UUIStyle_Image* ImageStyleData = GetAppliedImageStyle(CurrentMenuState, ActiveSkin, bClearExistingValue);
			check(ImageStyleData);

			// apply this component's per-instance image style settings
			FUICombinedStyleData FinalStyleData(ImageStyleData);
			CustomizeAppliedStyle(FinalStyleData);

			// apply the style data to the image
			ImageRef->SetImageStyle(FinalStyleData);
			bResult = TRUE;
		}
	}

	return bResult;
}

/* === CustomPropertyItemHandler interface === */
/**
 * Determines whether the specified property value matches the current value of the property.  Called after the user
 * has changed the value of a property handled by a custom property window item.  Is used to determine whether Pre/PostEditChange
 * should be called for the selected objects.
 *
 * @param	InProperty			the property whose value is being checked.
 * @param	NewPropertyValue	the value to compare against the current value of the property.
 *
 * @return	TRUE if NewPropertyValue matches the current value of the property specified, indicating that no effective changes
 *			were actually made.
 */
UBOOL UUIComp_DrawImage::IsCustomPropertyValueIdentical( UProperty* InProperty, const UPropertyValue& NewPropertyValue )
{
	UBOOL bResult = FALSE;

	if ( InProperty->GetOuter() == UUIComp_DrawImage::StaticClass() )
	{
		if ( InProperty->GetFName() == TEXT("ImageRef") )
		{
			bResult = NewPropertyValue.ObjectValue == GetImage();
		}
	}

	return bResult;
}

/**
 * Callback which allows classes to override the default behavior for setting property values in the editor.
 *
 * @param	InProperty		the property that is being edited
 * @param	PropertyValue	the value to assign to the property
 *
 * @return	TRUE if the property was handled by this object and the property value was successfully applied to the
 *			object's data.
 */
UBOOL UUIComp_DrawImage::EditorSetPropertyValue( UProperty* InProperty, const UPropertyValue& PropertyValue )
{
	UBOOL bResult = FALSE;
	if ( InProperty->GetOuter() == UUIComp_DrawImage::StaticClass() )
	{
		if ( InProperty->GetFName() == TEXT("ImageRef") )
		{
			SetImage( Cast<USurface>(PropertyValue.ObjectValue) );
			bResult = TRUE;
		}
	}

	return bResult;
}

/* === UObject interface === */
/**
 * Called when a property value has been changed in the editor.
 */
void UUIComp_DrawImage::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("StyleCustomization") )
			{
				RefreshAppliedStyleData();
			}
		}
	}
	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.
 *
 * This version migrates the ImageCoordinates value over to the StyleCustomization member.
 */
void UUIComp_DrawImage::PostLoad()
{
	Super::PostLoad();

	// make sure that changes to this component are included in undo/redo
	SetFlags(RF_Transactional);
	if ( ImageRef != NULL )
	{
		ImageRef->SetFlags(RF_Transactional);
	}

	if ( !GIsUCCMake && !ImageCoordinates.IsZero() )
	{
		StyleCustomization.SetCustomCoordinates(ImageCoordinates);
	}
}

/* ==========================================================================================================
	UUIComp_DrawString
========================================================================================================== */
/**
 * Initializes this component, creating the UIString needed for rendering text.
 *
 * @param	InSubscriberOwner	if this component is owned by a widget that implements the IUIDataStoreSubscriber interface,
 *								the TScriptInterface containing the interface data for the owner.
 */
void UUIComp_DrawString::InitializeComponent( TScriptInterface<IUIDataStoreSubscriber>* InSubscriberOwner/*=NULL*/ )
{
	if ( InSubscriberOwner != NULL )
	{
		SubscriberOwner = *InSubscriberOwner;
	}

	if ( ValueString == NULL )
	{
		UUIObject* Owner = GetOuterUUIObject();

		if ( StringClass == NULL )
		{
			debugf(NAME_Warning, TEXT("No StringClass configured for '%s' - using class 'Engine.UIString'"), *GetFullName());
			StringClass = UUIString::StaticClass();
		}

		ValueString = ConstructObject<UUIString>(StringClass, Owner, NAME_None, GetMaskedFlags(RF_Transient|RF_ArchetypeObject|RF_Public) );
	}
}

/**
 * Marks the Position for any faces dependent on the specified face, in this component's owning widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUIComp_DrawString::InvalidatePositionDependencies( BYTE Face )
{
	//@todo ronp - is anything needed here?  do we need to do anything beyond what we're doing in UUIObject?
}

/**
 * Adds the specified face to the owning scene's DockingStack for the owning widget.  Takes wrap behavior and
 * autosizing into account, ensuring that all widget faces are added to the scene's docking stack in the appropriate
 * order.
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUIComp_DrawString::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	UUIObject* Owner = GetOuterUUIObject();

	UBOOL bResult = TRUE;
	FUIDockingNode NewNode(Owner,Face);

	// determine if this node already exists in the docking stack
	INT StackPosition = DockingStack.FindItemIndex(NewNode);

	// if this node doesn't already exist
	if ( StackPosition == INDEX_NONE )
	{
		// for convenience
		FLOAT* RenderBounds = Owner->RenderBounds;
		FUIScreenValue_Bounds& Position = Owner->Position;
		FUIDockingSet& DockTargets = Owner->DockTargets;

		if ( DockTargets.bLinking[Face] != 0 )
		{
			if ( GIsGame )
			{
				UUIScreenObject* DockTarget = DockTargets.GetDockTarget(Face);
				if ( DockTarget == NULL && DockTargets.IsDocked(Face) )
				{
					DockTarget = Owner->GetScene();
				}
				debugf(TEXT("Circular docking relationship detected!  Widget:%s    Face:%s    Target:%s    TargetFace:%s"),
					*Owner->GetWidgetPathName(),
					*UUIRoot::GetDockFaceText(Face),
					DockTarget ? *DockTarget->GetWidgetPathName() : TEXT("NULL"),
					*UUIRoot::GetDockFaceText(DockTargets.GetDockFace(Face))
					);

				Owner->GetScene()->eventLogDockingStack();
			}
			return FALSE;
		}
		DockTargets.bLinking[Face] = 1;

		const UBOOL bDocked[UIFACE_MAX] =
		{
			DockTargets.IsDocked(UIFACE_Left),	DockTargets.IsDocked(UIFACE_Top),
			DockTargets.IsDocked(UIFACE_Right),	DockTargets.IsDocked(UIFACE_Bottom),
		};

		// first, determine which faces are considered the "first" of the set
		EUIWidgetFace H1, H2, V1, V2;
		// do the horizontal; the left face is always considered H1 unless the right face is docked and the left isn't.
		if ( bDocked[UIFACE_Right] && !bDocked[UIFACE_Left] )
			{ H1 = UIFACE_Right, H2 = UIFACE_Left; }
		else
			{ H1 = UIFACE_Left, H2 = UIFACE_Right; }

		// do the vertical - same deal
		if ( bDocked[UIFACE_Bottom] && !bDocked[UIFACE_Top] )
			{ V1 = UIFACE_Bottom, V2 = UIFACE_Top; }
		else
			{ V1 = UIFACE_Top, V2 = UIFACE_Bottom; }

		const UBOOL bAutoSizeHorz = IsAutoSizeEnabled(UIORIENT_Horizontal);
		const UBOOL bAutoSizeVert = IsAutoSizeEnabled(UIORIENT_Vertical);
		const UBOOL bLockedHorz = bDocked[H1] && bDocked[H2];
		const UBOOL bLockedVert = bDocked[V1] && bDocked[V2];

		if ( bAutoSizeHorz && bAutoSizeVert )
		{
			if ( Face == V2 && !bLockedVert )
			{
				bResult=AddDockingNode(DockingStack, V1)
					&&	AddDockingNode(DockingStack, H1)
					&&	bResult;
			}
			if ( Face == H2 && !bLockedHorz )
			{
				bResult=AddDockingNode(DockingStack, V1)
					&&	AddDockingNode(DockingStack, H1)
					&&	bResult;
			}
		}
		else if ( bAutoSizeHorz )
		{
			if ( Face == H2 && !bLockedHorz )
			{
				bResult=AddDockingNode(DockingStack, V2)
					&&	AddDockingNode(DockingStack, V1)
					&&	AddDockingNode(DockingStack, H1)
					&&	bResult;
			}

			else if ( Face == V2 && !bLockedVert && !bDocked[Face] )
			{
				bResult=AddDockingNode(DockingStack, V1) && bResult;
			}
		}
		else if ( bAutoSizeVert )
		{
			if ( Face == V2 && !bLockedVert )
			{
				bResult=AddDockingNode(DockingStack, H2)
					&&	AddDockingNode(DockingStack, H1)
					&&	AddDockingNode(DockingStack, V1)
					&&	bResult;
			}
			
			else if ( Face == H2 && !bLockedHorz && !bDocked[Face] )
			{
				bResult=AddDockingNode(DockingStack, H1) && bResult;
			}
		}
		else
		{
			if ( Face == V2 && !bDocked[Face] )
			{
				bResult=AddDockingNode(DockingStack, V1) && bResult;
			}
			else if ( Face == V1 && !bDocked[Face] )
			{
				bResult=AddDockingNode(DockingStack, H2) && bResult;
			}
			else if ( Face == H2 && !bDocked[Face] )
			{
				bResult=AddDockingNode(DockingStack, H1) && bResult;
			}
		}

		if ( bDocked[Face] )
		{
			UUIObject* DockTarget = DockTargets.GetDockTarget(Face);
			if ( DockTarget != NULL )
			{
				// if there is a TargetWidget for this face, that widget's face must be evaluated
				// before this one, so that face must go into the DockingStack first
				bResult = DockTarget->AddDockingNode(DockingStack, DockTargets.GetDockFace(Face)) && bResult;
			}
		}

		// add this face to the DockingStack
		check(NewNode.Face != UIFACE_MAX);
		new(DockingStack) FUIDockingNode(NewNode);

		DockTargets.bLinking[Face] = 0;
	}

	return bResult;
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value, and triggers the methods which apply
 * formatting data to the string.
 *
 * @param	Face	the face that should be resolved
 */
void UUIComp_DrawString::ResolveFacePosition( EUIWidgetFace Face )
{
	//SCOPE_CYCLE_COUNTER(STAT_UIResolvePosition_String);
	if ( bReapplyFormatting && ValueString != NULL )
	{
		//@todo - this needs more detailed documentation
// 		ValueString->StringClipMode = WrapMode;
// 		ValueString->StringClipAlignment = ClipAlignment;

		FRenderParameters Parameters;
		if ( GetStringFormatParameters(Face, Parameters) )
		{
			// format the string
			ApplyStringFormatting(Parameters, bIgnoreMarkup);
			
			// update the bounds of the owning widget if configured to do so
			UpdateOwnerBounds(Parameters);
		}
	}
}

/**
 * Initializes the render parameters that will be used for formatting the string.
 *
 * @param	Face			the face that was being resolved
 * @param	out_Parameters	[out] the formatting parameters to use for formatting the string.
 *
 * @return	TRUE if the formatting data is ready to be applied to the string, taking into account the autosize settings.
 */
UBOOL UUIComp_DrawString::GetStringFormatParameters( EUIWidgetFace Face, FRenderParameters& out_Parameters ) const
{
	SCOPE_CYCLE_COUNTER(STAT_UIGetStringFormatParms);
	UBOOL bFormattingConditionsMet = FALSE;
	UUIObject* Owner = GetOuterUUIObject();


	// if the owner widget hasn't been fully initialized, then we can't apply formatting
	if ( !Owner->IsInitialized() )
	{
		return FALSE;
	}

	// for convenience
	FLOAT* RenderBounds = Owner->RenderBounds;
	FUIScreenValue_Bounds& Position = Owner->Position;
	FUIDockingSet& Docking = Owner->DockTargets;

	const UBOOL bAutoSizeHorz = IsAutoSizeEnabled(UIORIENT_Horizontal);
	const UBOOL bAutoSizeVert = IsAutoSizeEnabled(UIORIENT_Vertical);

	const UBOOL bDocked[UIFACE_MAX] =
	{
		Docking.IsDocked(UIFACE_Left), Docking.IsDocked(UIFACE_Top),
		Docking.IsDocked(UIFACE_Right), Docking.IsDocked(UIFACE_Bottom),
	};

	// first, figure out the order in which parallel faces need to be resolved.
	EUIWidgetFace H1, H2, V1, V2;
	// do the horizontal; the left face is always considered H1 unless the right face is docked and the left isn't.
	if ( bDocked[UIFACE_Right] && !bDocked[UIFACE_Left] )
		{ H1 = UIFACE_Right, H2 = UIFACE_Left; }
	else
		{ H1 = UIFACE_Left, H2 = UIFACE_Right; }

	// do the vertical - same deal
	if ( bDocked[UIFACE_Bottom] && !bDocked[UIFACE_Top] )
		{ V1 = UIFACE_Bottom, V2 = UIFACE_Top; }
	else
		{ V1 = UIFACE_Top, V2 = UIFACE_Bottom; }

	FUICombinedStyleData StringStyleData;
	const_cast<UUIComp_DrawString*>(this)->GetFinalStringStyle(StringStyleData);

	// OK, the next thing we need to do is figure out if it's time to call ApplyFormatting
	// the conditions for this are different for wrapped strings than they are for others
	//@todo ronp - we could move this to a separate function
	if ( StringStyleData.TextClipMode == CLIP_Wrap )
	{
		if ( bAutoSizeHorz && bAutoSizeVert )
		{
			// if both horizontal and vertical auto-sizing are enabled, the position of the second face for each orientation
			// will be set using the extents of the formatted string; therefore only the first face from each orientation must
			// be resolved before we can AF
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(V1);
		}
		else if ( bAutoSizeHorz )
		{
			// if only horizontal auto-sizing is enabled, the position of the second horizontal face will be set according to the length
			// of the string; therefore, the other three faces must be resolved before AF can be called.
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(V1) && Owner->HasPositionBeenResolved(V2);
		}
		else if ( bAutoSizeVert )
		{
			// if only vertical auto-sizing is enabled, the width of the string will be used to determine where the string should be wrapped, and
			// the position of the second vertical face will be set according to the height of the post-wrapped string.
			// Therefore, the other three faces must be resolved before calling AF
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(H2) && Owner->HasPositionBeenResolved(V1);
		}
		else
		{
			// if auto-sizing is disabled, we don't dynamically adjust face positions using the post-formatted string's extents;
			// in order to wrap the string, we'll need to know the width of the string
			bFormattingConditionsMet = Owner->GetNumResolvedFaces() == UIFACE_MAX;
		}

		if ( bAutoSizeHorz && bDocked[UIFACE_Left] && bDocked[UIFACE_Right] )
		{
			bFormattingConditionsMet = bFormattingConditionsMet && Owner->HasPositionBeenResolved(H2);
		}
		if ( bAutoSizeVert && bDocked[UIFACE_Top] && bDocked[UIFACE_Bottom] )
		{
			bFormattingConditionsMet = bFormattingConditionsMet && Owner->HasPositionBeenResolved(V2);
		}
	}
	else
	{
		if ( bAutoSizeHorz && bAutoSizeVert )
		{
			// if both horizontal and vertical auto-sizing are enabled, the position of the second face for each orientation
			// will be set using the extents of the formatted string; therefore only the first face from each orientation must
			// be resolved before we can AF
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(V1);
		}
		else if ( bAutoSizeHorz )
		{
			// if only horizontal auto-sizing is enabled, the position of the second horizontal face will be set according to the length
			// of the string; therefore, only the first horizontal face must be resolved before we can AF
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(V1) && Owner->HasPositionBeenResolved(V2);
		}
		else if ( bAutoSizeVert )
		{
			// If only vertical auto-sizing is enabled, the width of the string will be used to determine where the string should be clipped, and
			// the position of the second vertical face will be set according to the resolved height of the string.
			// therefore, the other three faces must be resolved before calling AF
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(H2) && Owner->HasPositionBeenResolved(V1);
		}
		else
		{
			// if auto-sizing is disabled, we don't dynamically adjust face positions using the post-formatted string's extents;
			// but we need the width of the string in order to know where the string should be clipped
			if ( StringStyleData.TextClipMode != CLIP_None )
			{
				bFormattingConditionsMet = Owner->GetNumResolvedFaces() == UIFACE_MAX;
			}
			else
			{
				// if our wrap mode is none (allow strings to overrun the bounding region), then just wait until all faces have been resolved so that
				// we can pass in the correct bounding region to AF.
				bFormattingConditionsMet = Owner->GetNumResolvedFaces() == UIFACE_MAX;
			}
		}

		if ( bAutoSizeHorz && bDocked[UIFACE_Left] && bDocked[UIFACE_Right] )
		{
			bFormattingConditionsMet = bFormattingConditionsMet && Owner->HasPositionBeenResolved(H2);
		}
		if ( bAutoSizeVert && bDocked[UIFACE_Top] && bDocked[UIFACE_Bottom] )
		{
			bFormattingConditionsMet = bFormattingConditionsMet && Owner->HasPositionBeenResolved(V2);
		}
	}

	// if we're not ready to format the string, stop here
	if ( bFormattingConditionsMet )
	{
		//@todo ronp - move this into a method, like GetViewportDimension(out_ViewportOrigin, out_ViewportSize)
		FVector2D ViewportOrigin, ViewportSize;
		if ( !Owner->GetViewportOrigin(ViewportOrigin) )
		{
			ViewportOrigin.X = 0.f;
			ViewportOrigin.Y = 0.f;
		}
		if ( !Owner->GetViewportSize(ViewportSize) )
		{
			ViewportSize.X = UCONST_DEFAULT_SIZE_X;
			ViewportSize.Y = UCONST_DEFAULT_SIZE_Y;
		}

		// calculate the amount of padding to apply to the available bounding region (will return zero if autosizing is not enabled for that orientation)
		const FLOAT LeftPadding		= AutoSizeParameters[UIORIENT_Horizontal].GetPaddingValue( UIAUTOSIZEREGION_Minimum, UIORIENT_Horizontal, UIEXTENTEVAL_Pixels, Owner );
		const FLOAT RightPadding	= AutoSizeParameters[UIORIENT_Horizontal].GetPaddingValue( UIAUTOSIZEREGION_Maximum, UIORIENT_Horizontal, UIEXTENTEVAL_Pixels, Owner );
		const FLOAT TopPadding		= AutoSizeParameters[UIORIENT_Vertical].GetPaddingValue( UIAUTOSIZEREGION_Minimum, UIORIENT_Vertical, UIEXTENTEVAL_Pixels, Owner );
		const FLOAT BottomPadding	= AutoSizeParameters[UIORIENT_Vertical].GetPaddingValue( UIAUTOSIZEREGION_Maximum, UIORIENT_Vertical, UIEXTENTEVAL_Pixels, Owner );

		out_Parameters.DrawX = RenderBounds[UIFACE_Left]	+ LeftPadding;
		out_Parameters.DrawY = RenderBounds[UIFACE_Top]		+ TopPadding;

		FLOAT BoundingRegionWidth = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
		FLOAT BoundingRegionHeight = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];
		
		if ( bAutoSizeHorz )
		{
			// if we're configured to auto-size horizontally, rather than passing in the current width of the widget, we pass in the maximum
			// possible horizontal size

			// if the widget is docked on both horizontal faces, then auto-sizing is irrelevant because the widget's size is determined by the positions
			// of the faces that the widget is docked to
			if ( !bDocked[H1] || !bDocked[H2] )
			{
				if ( !bDocked[UIFACE_Right] )
				{
					// If the widget is not docked or is docked only on its left side, then the horizontal bounding region used 
					// is the distance from the right edge of the screen to the left face of the widget.
					out_Parameters.DrawX = RenderBounds[UIFACE_Left];
					BoundingRegionWidth = ViewportSize.X - RenderBounds[UIFACE_Left];
				}
				else
				{
					// If the widget is docked on the right face, then the horizontal bounding region used is the distance from the left
					// edge of the screen to the right face of the widget.
					out_Parameters.DrawX = ViewportOrigin.X;
					BoundingRegionWidth = RenderBounds[UIFACE_Right] - ViewportOrigin.X;
				}
			}
			
			// if the user has specified an auto-size constraint range, adjust the available bounding region
			const FLOAT MinRegionWidth = AutoSizeParameters[UIORIENT_Horizontal].GetMinValue( UIEXTENTEVAL_Pixels, UIORIENT_Horizontal, Owner );
			const FLOAT MaxRegionWidth = AutoSizeParameters[UIORIENT_Horizontal].GetMaxValue( UIEXTENTEVAL_Pixels, UIORIENT_Horizontal, Owner );
			if ( MinRegionWidth > KINDA_SMALL_NUMBER )
			{
				BoundingRegionWidth = Max(BoundingRegionWidth, MinRegionWidth);
			}
			if ( MaxRegionWidth > KINDA_SMALL_NUMBER )
			{
				BoundingRegionWidth = Min(BoundingRegionWidth, MaxRegionWidth);
			}
		}

		if ( bAutoSizeVert )
		{
			// if we're configured to auto-size vertically, rather than passing in the current height of the widget, we pass in the maximum
			// possible vertical size

			// if the widget is docked on both vertical faces, then auto-sizing is irrelevant because the widget's size is determined by the positions
			// of the faces that the widget is docked to
			if ( !bDocked[V1] || !bDocked[V2] )
			{
				if ( !bDocked[UIFACE_Bottom] )
				{
					// If the widget is not docked or is docked only on its top face, then the vertical bounding region used 
					// is the distance from the bottom edge of the screen to the top face of the widget.
					out_Parameters.DrawY = RenderBounds[UIFACE_Top];
					BoundingRegionHeight = ViewportSize.Y - RenderBounds[UIFACE_Top];
				}
				else
				{
					// If the widget is docked on the bottom face, then the vertical bounding region used is the distance from the top
					// edge of the screen to the bottom face of the widget.
					out_Parameters.DrawY = ViewportOrigin.Y;
					BoundingRegionHeight = RenderBounds[UIFACE_Bottom] - ViewportOrigin.Y;
				}
			}

			// if the user has specified an auto-size constraint range, adjust the available bounding region
			const FLOAT MinRegionWidth = AutoSizeParameters[UIORIENT_Vertical].GetMinValue( UIEXTENTEVAL_Pixels, UIORIENT_Vertical, Owner );
			const FLOAT MaxRegionWidth = AutoSizeParameters[UIORIENT_Vertical].GetMaxValue( UIEXTENTEVAL_Pixels, UIORIENT_Vertical, Owner );
			if ( MinRegionWidth > KINDA_SMALL_NUMBER )
			{
				BoundingRegionHeight = Max(BoundingRegionHeight, MinRegionWidth);
			}
			if ( MaxRegionWidth > KINDA_SMALL_NUMBER )
			{
				BoundingRegionHeight = Min(BoundingRegionHeight, MaxRegionWidth);
			}
		}

		// Subtract any auto-size padding from the bounding region sizes
		BoundingRegionWidth -= (LeftPadding + RightPadding);
		BoundingRegionHeight -= (TopPadding + BottomPadding);

		// UUIString::ApplyFormatting sets ClipX = DrawX + DrawXL, so DrawXL must be relative to DrawX.  It sets ClipY to DrawYL, so DrawYL must be absolute.
		out_Parameters.DrawXL = BoundingRegionWidth;
		out_Parameters.DrawYL = out_Parameters.DrawY + BoundingRegionHeight;

		out_Parameters.Scaling[UIORIENT_Horizontal] = ValueString->StringStyleData.TextScale[UIORIENT_Horizontal];
		out_Parameters.Scaling[UIORIENT_Vertical] = ValueString->StringStyleData.TextScale[UIORIENT_Vertical];
	}

	return bFormattingConditionsMet;
}

/**
 * Adjusts the owning widget's bounds according to the wrapping mode and autosize behaviors.
 */
void UUIComp_DrawString::UpdateOwnerBounds( FRenderParameters& Parameters )
{
	UUIObject* Owner = GetOuterUUIObject();

	FLOAT* RenderBounds = Owner->RenderBounds;
	FUIScreenValue_Bounds& Position = Owner->Position;
	FUIDockingSet& DockTargets = Owner->DockTargets;

	// Now that we've formatted the string, adjust the bounds of the owning widget if applicable (i.e. if auto-sizing is enabled).
	// The order in which the widget's faces were resolved was determined by the logic in AddDockingNode, and GetStringFormatParameters
	// should only return TRUE only after the faces which are NOT going to be adjusted here have all been resolved.

	// So, at this point the only faces that the RenderBounds of the widget should be out-of-date for are those faces which we are going
	// to adjust.  Therefore, we can use RenderBounds directly to determine offsets and such, rather than the [slower] GetPosition methods.
	const UBOOL bDocked[UIFACE_MAX] =
	{
		DockTargets.IsDocked(UIFACE_Left), DockTargets.IsDocked(UIFACE_Top),
		DockTargets.IsDocked(UIFACE_Right), DockTargets.IsDocked(UIFACE_Bottom),
	};
	const UBOOL bLockedHorz = bDocked[UIFACE_Left] && bDocked[UIFACE_Right];
	const UBOOL bLockedVert = bDocked[UIFACE_Top] && bDocked[UIFACE_Bottom];

	if ( IsAutoSizeEnabled(UIORIENT_Vertical) )
	{
		FLOAT TargetHeight = ValueString->StringExtent.Y;


		// The bounding region we used for formatting the string was reduced by the configured auto-size padding, but this padding value only applies to
		// the region availble for the string.  Before we adjust the bounds of the owning widget, we need to re-add the padding values back into the
		// size of the formatted string
		const FLOAT TopPadding		= AutoSizeParameters[UIORIENT_Vertical].GetPaddingValue( UIAUTOSIZEREGION_Minimum, UIORIENT_Vertical, UIEXTENTEVAL_Pixels, Owner );
		const FLOAT BottomPadding	= AutoSizeParameters[UIORIENT_Vertical].GetPaddingValue( UIAUTOSIZEREGION_Maximum, UIORIENT_Vertical, UIEXTENTEVAL_Pixels, Owner );
		TargetHeight += TopPadding + BottomPadding;

		const FLOAT MinTargetHeight = AutoSizeParameters[UIORIENT_Vertical].GetMinValue( UIEXTENTEVAL_Pixels, UIORIENT_Vertical, Owner );
		const FLOAT MaxTargetHeight = AutoSizeParameters[UIORIENT_Vertical].GetMaxValue( UIEXTENTEVAL_Pixels, UIORIENT_Vertical, Owner );
		if ( MinTargetHeight > KINDA_SMALL_NUMBER )
		{
			TargetHeight = Max(TargetHeight, MinTargetHeight);
		}
		if ( MaxTargetHeight > KINDA_SMALL_NUMBER )
		{
			TargetHeight = Min(TargetHeight, MaxTargetHeight);
		}

		// if the widget is docked on both faces, no adjustments can be made
		if ( !bDocked[UIFACE_Top] || !bDocked[UIFACE_Bottom] )
		{
			if ( !bDocked[UIFACE_Bottom] )
			{
				// If the widget is not docked or is docked only on its top face, then the bottom face of the widget needs to be adjusted
				Owner->Position.SetPositionValue(Owner, TargetHeight, UIFACE_Bottom, EVALPOS_PixelOwner, FALSE);
				Owner->RenderBounds[UIFACE_Bottom] = Owner->RenderBounds[UIFACE_Top] + TargetHeight;
				Owner->Position.ValidatePosition(UIFACE_Bottom);
				Owner->DockTargets.bResolved[UIFACE_Bottom] = TRUE;
			}
			else
			{
				// otherwise, if the widget is docked only on its bottom face, then the top face needs to be adjusted.
				const FLOAT NewPositionY = Owner->RenderBounds[UIFACE_Bottom] - TargetHeight;

				Owner->Position.SetPositionValue(Owner, NewPositionY, UIFACE_Top, EVALPOS_PixelViewport, FALSE);
				Owner->RenderBounds[UIFACE_Top] = NewPositionY;
				Owner->Position.ValidatePosition(UIFACE_Top);
				Owner->DockTargets.bResolved[UIFACE_Top] = TRUE;
			}
		}
	}

	if ( IsAutoSizeEnabled(UIORIENT_Horizontal) )
	{
		FLOAT TargetWidth = ValueString->StringExtent.X;

		// The bounding region we used for formatting the string was reduced by the configured auto-size padding, but this padding value only applies to
		// the region availble for the string.  Before we adjust the bounds of the owning widget, we need to re-add the padding values back into the
		// size of the formatted string
		const FLOAT LeftPadding		= AutoSizeParameters[UIORIENT_Horizontal].GetPaddingValue( UIAUTOSIZEREGION_Minimum, UIORIENT_Horizontal, UIEXTENTEVAL_Pixels, Owner );
		const FLOAT RightPadding	= AutoSizeParameters[UIORIENT_Horizontal].GetPaddingValue( UIAUTOSIZEREGION_Maximum, UIORIENT_Horizontal, UIEXTENTEVAL_Pixels, Owner );
		TargetWidth += LeftPadding + RightPadding;

		// now verify that the auto-size region falls within the constraints of the auto-size min/max
		const FLOAT MinTargetWidth = AutoSizeParameters[UIORIENT_Horizontal].GetMinValue( UIEXTENTEVAL_Pixels, UIORIENT_Horizontal, Owner );
		const FLOAT MaxTargetWidth = AutoSizeParameters[UIORIENT_Horizontal].GetMaxValue( UIEXTENTEVAL_Pixels, UIORIENT_Horizontal, Owner );
		if ( MinTargetWidth > KINDA_SMALL_NUMBER )
		{
			TargetWidth = Max(TargetWidth, MinTargetWidth);
		}
		if ( MaxTargetWidth > KINDA_SMALL_NUMBER )
		{
			TargetWidth = Min(TargetWidth, MaxTargetWidth);
		}

		// if the widget is docked on both faces, no adjustments can be made
		if ( !bDocked[UIFACE_Left] || !bDocked[UIFACE_Right] )
		{
			if ( !bDocked[UIFACE_Right] )
			{
				// If the widget is not docked or is docked only on its left face, then the right face of the widget needs to be adjusted
				Owner->Position.SetPositionValue(Owner, TargetWidth, UIFACE_Right, EVALPOS_PixelOwner, FALSE);
				Owner->RenderBounds[UIFACE_Right] = Owner->RenderBounds[UIFACE_Left] + TargetWidth;
				Owner->Position.ValidatePosition(UIFACE_Right);
				Owner->DockTargets.bResolved[UIFACE_Right] = TRUE;
			}
			else
			{
				// otherwise, if the widget is docked only on its right face, then the left face needs to be adjusted.
				const FLOAT NewPositionX = Owner->RenderBounds[UIFACE_Right] - TargetWidth;
				Owner->Position.SetPositionValue(Owner, NewPositionX, UIFACE_Left, EVALPOS_PixelViewport, FALSE);
				Owner->RenderBounds[UIFACE_Left] = NewPositionX;
				Owner->Position.ValidatePosition(UIFACE_Left);
				Owner->DockTargets.bResolved[UIFACE_Left] = TRUE;
			}
		}
	}
}

/**
 * Returns TRUE if this component's UIStyleReference can be resolved into a valid UIStyle.
 *
 * @param	CurrentlyActiveSkin		the currently active skin; used for resolving the style reference's default style if it doesn't yet have a valid style id.
 */
UBOOL UUIComp_DrawString::HasValidStyleReference( UUISkin* CurrentlyActiveSkin/*=NULL*/ )
{
	UBOOL bHasValidStyleReference = StringStyle.GetResolvedStyle() != NULL;
	if ( !bHasValidStyleReference && CurrentlyActiveSkin != NULL )
	{
		bHasValidStyleReference = StringStyle.AssignedStyleID.IsValid() || StringStyle.GetDefaultStyleTag(CurrentlyActiveSkin) != NAME_None;
	}

	return bHasValidStyleReference;
}

/**
 * Returns the combo style data being used by this string rendering component.  If the component's StringStyle is not set, the style data
 * will be pulled from the owning widget's PrimaryStyle, if possible.
 *
 * @param	DesiredMenuState	the menu state for the style data to retrieve; if not specified, uses the owning widget's current menu state.
 * @param	SourceSkin			the skin to use for resolving this component's combo style; only relevant when the component's combo style is invalid
 *								(or if TRUE is passed for bClearExistingValue). If the combo style is invalid and a value is not specified, returned value
 *								will be NULL.
 * @param	bClearExistingValue	used to force the component's combo style to be re-resolved from the specified skin; if TRUE, you must supply a valid value for
 *								SourceSkin.
 *
 * @return	the combo style data used to render this component's string for the specified menu state.
 */
UUIStyle_Combo* UUIComp_DrawString::GetAppliedStringStyle( UUIState* DesiredMenuState/*=NULL*/, UUISkin* SourceSkin/*=NULL*/, UBOOL bClearExistingValue/*=FALSE*/ )
{
	UUIStyle_Combo* Result = NULL;
	UUIObject* OwnerWidget = GetOuterUUIObject();

	// if no menu state was specified, use the owning widget's current menu state
	if ( DesiredMenuState == NULL )
	{
		DesiredMenuState = OwnerWidget->GetCurrentState();
	}
	check(DesiredMenuState);

	// if this component's style reference hasn't been assigned a value, use the owning widget's PrimaryStyle
	const UBOOL bHasValidStyleReference = HasValidStyleReference(SourceSkin);
	if ( !bHasValidStyleReference )
	{
		checkf(OwnerWidget->bSupportsPrimaryStyle,TEXT("Unable to resolve component style reference (%s) for %s.%s and owner widget no longer supports the PrimaryStyle"),
			*StringStyle.DefaultStyleTag.ToString(), *OwnerWidget->GetWidgetPathName(), *GetName());
	}

	const UBOOL bIsStyleManaged = OwnerWidget->IsPrivateBehaviorSet(UCONST_PRIVATE_ManagedStyle);
	FUIStyleReference& StyleToResolve = bHasValidStyleReference ? StringStyle : OwnerWidget->PrimaryStyle;
	if ( bHasValidStyleReference && bClearExistingValue == TRUE && !bIsStyleManaged )
	{
		StyleToResolve.InvalidateResolvedStyle();
	}

	// get the UIStyle corresponding to the UIStyleReference that we're going to pull from
	// GetResolvedStyle() is guaranteed to return a valid style if a valid UISkin is passed in, unless the style reference is completely invalid

	// If the owner widget's style is being managed by someone else, then StyleToResolve's AssignedStyleID will always be zero, so never pass in
	// a valid skin reference.  This style should have already been resolved by whoever is managing the style, and if the resolved style doesn't live
	// in the currently active skin (for example, it's being inherited from a base skin package), GetResolvedStyle will clear the reference and reset
	// the ResolvedStyle back to the default style for this style reference.
	UUIStyle* ResolvedStyle = StyleToResolve.GetResolvedStyle(bIsStyleManaged ? NULL : SourceSkin);
	checkf(ResolvedStyle, TEXT("Unable to resolve style reference (%s)' for '%s.%s'"), *StyleToResolve.DefaultStyleTag.ToString(), *OwnerWidget->GetWidgetPathName(), *GetName());

	// retrieve the style data for this menu state
	UUIStyle_Data* StyleData = ResolvedStyle->GetStyleForState(DesiredMenuState);
	check(StyleData);

	Result = Cast<UUIStyle_Combo>(StyleData);
	return Result;
}
void UUIComp_DrawString::execGetAppliedStringStyle( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT_OPTX(UUIState,DesiredMenuState,NULL);
	P_FINISH;
	*(UUIStyle_Combo**)Result=GetAppliedStringStyle(DesiredMenuState);
}

/**
 * Gets the style data that will be used when rendering this component's string, including all style overrides or customizations enabled for this instance.
 *
 * @param	FinalStyleData	will be filled in with the style and formatting values that will be applied to this component's string
 *
 * @return	TRUE if the input value was filled in; FALSE if the component's style is still invalid or couldn't set the output value for any reason.
 */
UBOOL UUIComp_DrawString::GetFinalStringStyle( FUICombinedStyleData& FinalStyleData )
{
	UBOOL bResult = FALSE;

	UUIStyle_Combo* ComboStyleData = GetAppliedStringStyle();
	if ( ComboStyleData != NULL )
	{
		// apply this component's per-instance style settings
		FinalStyleData = FUICombinedStyleData(ComboStyleData);
		CustomizeAppliedStyle(FinalStyleData);

		bResult = TRUE;
	}

	return bResult;
}

/* === UIStyleResolver interface === */
/**
 * Returns the tag assigned to this UIStyleResolver by the owning widget
 */
FName UUIComp_DrawString::GetStyleResolverTag()
{
	return StyleResolverTag;
}

/**
 * Changes the tag assigned to the UIStyleResolver to the specified value.
 *
 * @return	TRUE if the name was changed successfully; FALSE otherwise.
 */
UBOOL UUIComp_DrawString::SetStyleResolverTag( FName NewResolverTag )
{
	UBOOL bResult = NewResolverTag != StyleResolverTag;

	StyleResolverTag = NewResolverTag;

	return bResult;
}

/**
 * Resolves the style for this string rendering component.
 *
 * @param	ActiveSkin			the skin the use for resolving the style reference.
 * @param	bClearExistingValue	if TRUE, style references will be invalidated first.
 * @param	CurrentMenuState	the menu state to use for resolving the style data; if not specified, uses the current
 *								menu state of the owning widget.
 * @param	StyleProperty		if specified, only the style reference corresponding to the specified property
 *								will be resolved; otherwise, all style references will be resolved.
 */
UBOOL UUIComp_DrawString::NotifyResolveStyle( UUISkin* ActiveSkin, UBOOL bClearExistingValue, UUIState* CurrentMenuState/*=NULL*/, const FName StylePropertyName/*=NAME_None*/ )
{
	UBOOL bResult = FALSE;
	if ( StylePropertyName == NAME_None || StylePropertyName == TEXT("StringStyle") )
	{
 		UUIStyle_Combo* ComboStyleData = GetAppliedStringStyle(CurrentMenuState, ActiveSkin, bClearExistingValue);
		check(ComboStyleData);

		if ( ValueString != NULL )
		{
			// apply this component's per-instance style settings
			FUICombinedStyleData FinalStyleData(ComboStyleData);
			CustomizeAppliedStyle(FinalStyleData);

			// apply the style data to the string
			if ( ValueString->SetStringStyle(FinalStyleData) )
			{
				ReapplyFormatting();
			}
		}
	}

	return bResult;
}

/**
 * Changes the style for this UIString.
 *
 * this version is called from widgets when their styles are applied - this version will go away once all widgets which use string components
 * are updated to have their string components automatically resolve their styles by adding the string component to the widget's StyleResolvers array.
 *
 * @param	NewStringStyle	the new style to use for rendering the string
 */
void UUIComp_DrawString::SetStringStyle( UUIStyle_Combo* NewComboStyle )
{
	// for now, just apply the style to the string and don't worry about updating our style reference
	if ( ValueString != NULL )
	{
		// we should never allow a NULL style to be assigned to the internal string, as this will trip the assertions that UIString uses to
		// validate its internal state
		checkf(NewComboStyle, TEXT("Attempted to assign a NULL style as the default string style for '%s'"), *GetFullName());

		if ( ValueString->SetStringStyle(NewComboStyle) )
		{
			ReapplyFormatting();
		}
	}
}

/**
 * Initializes the CustomizedStyleData using the string current style, then applies any per-instance values
 * which are intended to override values in the style.
 *
 * @param	CustomizedStyleData		struct which receives the per-instance style data configured for this component;
 *									should be initialized using a combo style prior to calling this function.
 */
void UUIComp_DrawString::CustomizeAppliedStyle( FUICombinedStyleData& CustomizedStyleData ) const
{
	TextStyleCustomization.CustomizeDrawColor(CustomizedStyleData.TextColor);
	TextStyleCustomization.CustomizeDrawFont(CustomizedStyleData.DrawFont);
	TextStyleCustomization.CustomizeAttributes(CustomizedStyleData.TextAttributes);
	TextStyleCustomization.CustomizeScale(UIORIENT_Horizontal, CustomizedStyleData.TextScale[UIORIENT_Horizontal]);
	TextStyleCustomization.CustomizeScale(UIORIENT_Vertical, CustomizedStyleData.TextScale[UIORIENT_Vertical]);
	TextStyleCustomization.CustomizeAutoScaling(CustomizedStyleData.TextAutoScaling);

	EUIAlignment Temp;
	if ( TextStyleCustomization.CustomizeAlignment(UIORIENT_Horizontal, Temp) )
	{
		CustomizedStyleData.TextAlignment[UIORIENT_Horizontal] = Temp;
	}
	if ( TextStyleCustomization.CustomizeAlignment(UIORIENT_Vertical, Temp) )
	{
		CustomizedStyleData.TextAlignment[UIORIENT_Vertical] = Temp;
	}
	if ( TextStyleCustomization.CustomizeClipAlignment(Temp) )
	{
		CustomizedStyleData.TextClipAlignment = Temp;
	}

	ETextClipMode ClipMode;
	if ( TextStyleCustomization.CustomizeClipMode(ClipMode) )
	{
		CustomizedStyleData.TextClipMode = ClipMode;
	}

//	TextStyleCustomization.CustomizeAlignment(UIORIENT_Horizontal, (EUIAlignment&)CustomizedStyleData.TextAlignment[UIORIENT_Horizontal]);
//	TextStyleCustomization.CustomizeAlignment(UIORIENT_Vertical, (EUIAlignment&)CustomizedStyleData.TextAlignment[UIORIENT_Vertical]);

	//@todo ronp - we'll also need to allow customization of the image portion of this component combo style
}

/**
 * Applies the current style data (including any style data customization which might be enabled) to the string.
 */
void UUIComp_DrawString::RefreshAppliedStyleData()
{
	if ( ValueString != NULL )
	{
		UUIStyle_Combo* ComboStyleData = GetAppliedStringStyle();
		check(ComboStyleData);

		// apply this component's per-instance style settings
		FUICombinedStyleData FinalStyleData(ComboStyleData);
		CustomizeAppliedStyle(FinalStyleData);

		// apply the style data to the string
		if ( ValueString->SetStringStyle(FinalStyleData) )
		{
			ReapplyFormatting();
		}
	}
}

/**
 * Enables font color customization and changes the component's override color to the value specified.
 *
 * @param	NewColor	the color to use for rendering this component's string
 */
void UUIComp_DrawString::SetColor( FLinearColor NewColor )
{
	if ( TextStyleCustomization.SetCustomDrawColor(NewColor) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables opacity customization and changes the component's override opacity to the value specified.
 *
 * @param	NewOpacity	the Opacity to use for rendering this component's string
 */
void UUIComp_DrawString::SetOpacity( FLOAT NewOpacity )
{
	if ( TextStyleCustomization.SetCustomOpacity(NewOpacity) )
	{
		RefreshAppliedStyleData();
	}
}


/**
 * Enables font customization and changes the component's override font to the value specified.
 *
 * @param	NewFont	the font to use for rendering this component's text
 */
void UUIComp_DrawString::SetFont( UFont* NewFont )
{
	if ( TextStyleCustomization.SetCustomDrawFont(NewFont) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables text attribute customization and changes the component's override attributes to the value specified.
 *
 * @param	NewAttributes	the attributes to use for rendering this component's text
 */
void UUIComp_DrawString::SetAttributes( FUITextAttributes NewAttributes )
{
	if ( TextStyleCustomization.SetCustomAttributes(NewAttributes) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables text alignment customization and sets the component's custom alignment value to the value specified.
 *
 * @param	Orientation		indicates which orientation to modify
 * @param	NewAlignment	the new alignment to use for rendering this component's text
 */
void UUIComp_DrawString::SetAlignment( /*EUIOrientation*/ BYTE Orientation, /*EUIAlignment*/ BYTE NewAlignment )
{
	if ( Orientation < UIORIENT_MAX && NewAlignment < UIALIGN_MAX )
	{
		if ( TextStyleCustomization.SetCustomAlignment((EUIOrientation)Orientation, (EUIAlignment)NewAlignment) )
		{
			RefreshAppliedStyleData();
		}
	}
}

/**
 * Enables clip mode customization and sets the component's custom clip mode value to the value specified.
 *
 * @param	NewClipMode	the new wrapping mode for this string.
 */
void UUIComp_DrawString::SetWrapMode( /*ETextClipMode*/BYTE NewClipMode )
{
	if ( NewClipMode < CLIP_MAX && TextStyleCustomization.SetCustomClipMode(static_cast<ETextClipMode>(NewClipMode)) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables clip alignment customization and sets the component's custom clip alignment value to the value specified.
 *
 * @param	NewClipAlignment	the new clip alignment to use mode for this string.
 */
void UUIComp_DrawString::SetClipAlignment( /*EUIAlignment*/BYTE NewClipAlignment )
{
	if ( NewClipAlignment < UIALIGN_MAX && TextStyleCustomization.SetCustomClipAlignment(static_cast<EUIAlignment>(NewClipAlignment)) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Enables autoscale customization and changes the component's override autoscalemode to the value specified.
 *
 * @param	NewAutoScaleMode	the autoscale mode to use for formatting this component's text
 * @param	NewMinScaleValue	the minimum scaling value to apply to the text.  if not specified (or a negative value
 *								is specified), the min scaling value will not be changed.
 */
void UUIComp_DrawString::SetAutoScaling( /*ETextAutoScaleMode*/BYTE NewAutoScaleMode, FLOAT NewMinScaleValue/*=-1.f*/ )
{
	if ( NewAutoScaleMode < UIAUTOSCALE_MAX )
	{
		if ( NewMinScaleValue < 0 )
		{
			NewMinScaleValue = TextStyleCustomization.AutoScaling.MinScale;
		}
		if ( TextStyleCustomization.SetCustomAutoScaling(static_cast<ETextAutoScaleMode>(NewAutoScaleMode), NewMinScaleValue) )
		{
			RefreshAppliedStyleData();
		}
	}
}

/**
 * Enables text scale customization and sets the component's custom scale value to the value specified.
 *
 * @param	Orientation		indicates which orientation to modify
 * @param	NewScale		the new scale to use for rendering this component's text
 */
void UUIComp_DrawString::SetScale( /*EUIOrientation*/BYTE Orientation, FLOAT NewScale )
{
	if ( Orientation < UIORIENT_MAX && TextStyleCustomization.SetCustomScale(static_cast<EUIOrientation>(Orientation),NewScale) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables font color customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomColor()
{
	if ( TextStyleCustomization.EnableCustomDrawColor(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables font color customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomOpacity()
{
	if ( TextStyleCustomization.EnableCustomOpacity(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables font customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomFont()
{
	if ( TextStyleCustomization.EnableCustomDrawFont(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables text attribute customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomAttributes()
{
	if ( TextStyleCustomization.EnableCustomAttributes(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables text alignment customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomAlignment()
{
	if ( TextStyleCustomization.EnableCustomAlignment(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables text clip mode customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomClipMode()
{
	if ( TextStyleCustomization.EnableCustomClipMode(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables clip alignment customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomClipAlignment()
{
	if ( TextStyleCustomization.EnableCustomClipAlignment(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables text autoscale mode customization, allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomAutoScaling()
{
	if ( TextStyleCustomization.EnableCustomAutoScaleMode(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Disables text scale customization allowing the string to use the values from the applied style.
 */
void UUIComp_DrawString::DisableCustomScale()
{
	if ( TextStyleCustomization.EnableCustomScale(FALSE) )
	{
		RefreshAppliedStyleData();
	}
}

/**
 * Wrapper for quickly grabbing the current wrap mode for this component.
 */
/*ETextClipMode*/BYTE UUIComp_DrawString::GetWrapMode() const
{
	ETextClipMode Result = CLIP_None;

	FUICombinedStyleData StyleData;
	if ( const_cast<UUIComp_DrawString*>(this)->GetFinalStringStyle(StyleData) )
	{
		Result = static_cast<ETextClipMode>(StyleData.TextClipMode);
	}
	return Result;
}

/**
 * Render the UIString
 *
 * @param	Canvas	the FCanvas to use for rendering this string
 */
void UUIComp_DrawString::Render_String( FCanvas* Canvas )
{
	checkfSlow(ValueString, TEXT("NULL ValueString in DrawStringComponent (did you forget to call InitializeComponent from %s::Initialize()?)"), *GetOuter()->GetName());

	// if in release mode and we don't have a ValueString, just return
	if ( ValueString == NULL )
	{
		return;
	}

	UUIObject* Owner = GetOuterUUIObject();
	FLOAT* RenderBounds = Owner->RenderBounds;

	if ( ValueString->Nodes.Num() && ValueString->StringExtent.X == 0 && ValueString->StringExtent.Y == 0 && ValueString->GetValue(TRUE).Len() > 0 )
	{
		ReapplyFormatting();
		return;
	}

	// Fading
	FLOAT FadeOpacity;;
	
	if (  UpdateFade(FadeOpacity) )
	{
		SetOpacity( FadeOpacity);
	}

	// @note: DefaultStringStyle's TextStyle should ALWAYS be valid, and if text style is invalid as a result of programmer error, one of the
	// other assertions sprinkled throughout the UIString class should have been triggered first...
	// so if the below assertion is triggered, it is usually an indication of a problem outside of the UI system, and is generally not a good
	// idea to simply disable it. (the last time this assertion was triggered, it turned out to be caused by a VC compiler bug, for example).
	FUICombinedStyleData FinalStyleData;
	GetFinalStringStyle(FinalStyleData);

	checkf(FinalStyleData.DrawFont, TEXT("NULL DrawFont in StringStyle for '%s' of %s"), *GetFullName(), *Owner->GetWidgetPathName());


	// Account for left face autosize padding.
	const FLOAT LeftPadding		= AutoSizeParameters[UIORIENT_Horizontal].GetPaddingValue( UIAUTOSIZEREGION_Minimum, UIORIENT_Horizontal, UIEXTENTEVAL_Pixels, Owner );
	const FLOAT RightPadding	= AutoSizeParameters[UIORIENT_Horizontal].GetPaddingValue( UIAUTOSIZEREGION_Maximum, UIORIENT_Horizontal, UIEXTENTEVAL_Pixels, Owner );
	const FLOAT TopPadding		= AutoSizeParameters[UIORIENT_Vertical].GetPaddingValue( UIAUTOSIZEREGION_Minimum, UIORIENT_Vertical, UIEXTENTEVAL_Pixels, Owner );
	const FLOAT BottomPadding	= AutoSizeParameters[UIORIENT_Vertical].GetPaddingValue( UIAUTOSIZEREGION_Maximum, UIORIENT_Vertical, UIEXTENTEVAL_Pixels, Owner );

	FRenderParameters Parameters;
	FLOAT LabelX = RenderBounds[UIFACE_Left] + LeftPadding;
	FLOAT LabelY = RenderBounds[UIFACE_Top] + TopPadding;
	FLOAT LabelX2 = RenderBounds[UIFACE_Right] - RightPadding;
	FLOAT LabelY2 = RenderBounds[UIFACE_Bottom] - BottomPadding;

	FLOAT LabelWidth = LabelX2 - LabelX;
	FLOAT LabelHeight = LabelY2 - LabelY;

	const BYTE VerticalAlign = FinalStyleData.TextAlignment[UIORIENT_Vertical];
	const BYTE HorizontalAlign = FinalStyleData.TextAlignment[UIORIENT_Horizontal];

	// here's where we calculate the bounding region that should be visible to our internal UIString.  Since each node of the UIString
	// may have its own alignment value, we must apply the overall alignment (as dictated by the label's style) to the bounding region
	// that we pass to the UIString's render function.  The UIString is aligned within the label's bounding region, then the UIString further
	// divides the available region into smaller chunks which are used to apply any per-node alignment.
	switch( HorizontalAlign )
	{
	case UIALIGN_Left:
		// the entire label's bounding region is available to the string
		Parameters.DrawX = LabelX;
		Parameters.DrawXL = LabelWidth;
		break;

	case UIALIGN_Center:
		// reduce the bounding region to the width of the longest line of the UIString (the value of StringExtent.X)
		Parameters.DrawX = LabelX + (LabelWidth * 0.5f - ValueString->StringExtent.X * 0.5f);
		Parameters.DrawXL = ValueString->StringExtent.X;
		break;

	case UIALIGN_Right:
		// reduce the bounding region to the width of the longest line of the UIString (the value of StringExtent.X)
		Parameters.DrawX = LabelX2 - ValueString->StringExtent.X;
		Parameters.DrawXL = LabelX2 - Parameters.DrawX;
		break;
	}

	switch( VerticalAlign )
	{
	case UIALIGN_Left:
		// the entire label's bounding region is available to the string
		Parameters.DrawY = LabelY;
		Parameters.DrawYL = LabelHeight;
		break;

	case UIALIGN_Center:
		// reduce the bounding region to the combined height of all lines in the UIString (the value of StringExtent.Y)
		Parameters.DrawY = LabelY + (LabelHeight * 0.5f - ValueString->StringExtent.Y * 0.5f);
		Parameters.DrawYL = ValueString->StringExtent.Y;
		break;

	case UIALIGN_Right:
		// reduce the bounding region to the combined height of all lines in the UIString (the value of StringExtent.Y)
		Parameters.DrawY = LabelY2 - ValueString->StringExtent.Y;
		Parameters.DrawYL = LabelY2 - Parameters.DrawY;
		break;
	}

	// Set the parameters to pass to UUIString.
	Parameters.TextAlignment[UIORIENT_Horizontal] = HorizontalAlign;
	Parameters.TextAlignment[UIORIENT_Vertical] = VerticalAlign;
	Parameters.Scaling[UIORIENT_Horizontal] = FinalStyleData.TextScale[UIORIENT_Horizontal];
	Parameters.Scaling[UIORIENT_Vertical] = FinalStyleData.TextScale[UIORIENT_Vertical];

	// Removed to permit font scaling.
	/*
	// clamp the regions to the render boundaries of this label
	Parameters.DrawX = Max( LabelX, Parameters.DrawX);
	Parameters.DrawY = Max( LabelY, Parameters.DrawY);
	Parameters.DrawXL = Min(Parameters.DrawXL, LabelWidth);
	Parameters.DrawYL = Min(Parameters.DrawYL, LabelHeight);
	*/

	InternalRender_String(Canvas, Parameters);
}

/**
 * Renders the string using the parameters specified.
 *
 * @param	Canvas	the canvas to use for rendering this string
 */
void UUIComp_DrawString::InternalRender_String( FCanvas* Canvas, FRenderParameters& Parameters )
{
	checkfSlow(ValueString, TEXT("NULL ValueString in string render component (did you forget to call InitializeComponent from %s::Initialize()?)"), *GetOuter()->GetName());

	if ( ValueString != NULL )
	{
		// simply render the string
		ValueString->Render_String(Canvas, Parameters);
	}
}

/**
 * Handles unregistering the "RefreshSubscriberValue" callbacks for the data stores that were previously resolved by this
 * component's UIString and registering the callbacks for the data stores currently resolved by the UIString.  Requires that
 * the SubscriberOwner value be set.
 *
 * @param	RemovedDataStores	the list of data stores that were previously bound to this component's string.  SubscriberOwner
 *								will be removed from each data store's "RefreshSubscriberValue" callback list.
 * @param	AddedDataStores		the list of data stores that are now bound to this component's string.  SubscriberOwner
 *								will be registered with each data store's "RefreshSubscriberValue" callback list.
 */
void UUIComp_DrawString::UpdateSubscriberCallbacks( TArray<UUIDataStore*> RemovedDataStores, TArray<UUIDataStore*> AddedDataStores )
{
	if ( SubscriberOwner )
	{
		// first, unsubscribe from the old data stores
		for ( INT DataStoreIndex = 0; DataStoreIndex < RemovedDataStores.Num(); DataStoreIndex++ )
		{
			UUIDataStore* DataStore = RemovedDataStores(DataStoreIndex);
			DataStore->eventSubscriberDetached(SubscriberOwner);
		}

		// now, register with the new data stores
		for ( INT DataStoreIndex = 0; DataStoreIndex < AddedDataStores.Num(); DataStoreIndex++ )
		{
			UUIDataStore* DataStore = AddedDataStores(DataStoreIndex);
			DataStore->eventSubscriberAttached(SubscriberOwner);
		}
	}
}

/**
 * Retrieves a list of all data stores resolved by ValueString.
 *
 * @param	StringDataStores	receives the list of data stores that have been resolved by the UIString.  Appends all
 *								entries to the end of the array and does not clear the array first.
 */
void UUIComp_DrawString::GetResolvedDataStores( TArray<UUIDataStore*>& StringDataStores )
{
	if ( ValueString != NULL )
	{
		ValueString->GetResolvedDataStores(StringDataStores);
	}
}

/**
 * Retrieve the value of the string.
 *
 * @param	bReturnProcessedText	Determines whether the processed or raw version of the value string is returned.
 *									The raw value will contain any markup; the processed string will be text only.
 *									Any image tokens are converted to their text counterpart.
 *
 * @return	the complete text value contained by the UIString, in either the processed or unprocessed state.
 */
FString UUIComp_DrawString::GetValue(UBOOL bReturnProcessedText/* =TRUE */) const
{
	checkf(ValueString, TEXT("NULL ValueString in DrawStringComponent (did you forget to call InitializeComponent from %s::Initialize()?)"), *GetOuter()->GetName());
	return ValueString != NULL
		? ValueString->GetValue(bReturnProcessedText)
		: TEXT("");
}

/**
 * Changes the value of the text at runtime.
 *
 * @param	NewText		the new text that should be displayed
 */
void UUIComp_DrawString::SetValue( const FString& NewText )
{
	checkf(ValueString, TEXT("NULL ValueString in DrawStringComponent (did you forget to call InitializeComponent from %s::Initialize()?)"), *GetOuter()->GetName());

	if ( ValueString != NULL )
	{
		TArray<UUIDataStore*> OldDataStores, NewDataStores;
		GetResolvedDataStores(OldDataStores);

		ValueString->SetValue(NewText, bIgnoreMarkup);

		GetResolvedDataStores(NewDataStores);

		UpdateSubscriberCallbacks(OldDataStores, NewDataStores);
		ReapplyFormatting();
	}
}

/**
 * Clears and regenerates all nodes in the string by reparsing the source string.
 */
void UUIComp_DrawString::RefreshValue()
{
	SetValue(GetValue(FALSE));
}

/**
 * Refreshes the UIString's extents and appearance based on the string's current style and formatting options.
 *
 * @param	bDockingStackChanged	if TRUE, the scene will rebuild its DockingStack at the beginning
 *									the next frame
 * @param	bPositionsChanged		if TRUE, the scene will update the positions for all its widgets
 *									at the beginning of the next frame
 * @param	bNavLinksOutdated		if TRUE, the scene will update the navigation links for all widgets
 *									at the beginning of the next frame
 * @param	bWidgetStylesChanged	if TRUE, the scene will refresh the widgets reapplying their current styles
 */
void UUIComp_DrawString::ReapplyFormatting( UBOOL bDockingStackChanged/*=FALSE*/, UBOOL bPositionsChanged/*=TRUE*/, UBOOL bNavLinksOutdated/*=FALSE*/, UBOOL bWidgetStylesChanged/*=FALSE*/ )
{
	bReapplyFormatting = TRUE;

	// the text has changed, so we need to update our owner widget's render bounds
	// DO NOT change this to call RefreshPosition on the owning widget, as some implementations of RefreshPosition call ReapplyFormatting
	// on the component!
	GetOuterUUIObject()->RequestSceneUpdate(bDockingStackChanged,bPositionsChanged,bNavLinksOutdated,bWidgetStylesChanged);
}

/**
 * Wrapper for calling ApplyFormatting on the string.  Resets the value of bReapplyFormatting.
 *
 * @param	Parameters		@see UUIString::ApplyFormatting())
 * @param	bIgnoreMarkup	@see UUIString::ApplyFormatting())
 */
void UUIComp_DrawString::ApplyStringFormatting( FRenderParameters& Parameters, UBOOL bIgnoreMarkup )
{
	SCOPE_CYCLE_COUNTER(STAT_UIApplyStringFormatting);
	bReapplyFormatting = FALSE;

	checkfSlow(ValueString, TEXT("NULL ValueString in DrawStringComponent (did you forget to call InitializeComponent from %s::Initialize()?)"), *GetOuter()->GetName());
	if ( ValueString != NULL )
	{
		ValueString->ApplyFormatting(Parameters, bIgnoreMarkup);
	}
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUIComp_DrawString::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("AutoSizeParameters") || PropertyName == TEXT("bIgnoreMarkup") )
			{
				// this recalculates the extents for each of the nodes contained by the string
				ReapplyFormatting(TRUE,TRUE,TRUE);
			}
			else if ( PropertyName == TEXT("bRefreshString") )
			{
				bRefreshString = FALSE;
				RefreshAppliedStyleData();
				RefreshValue();
			}
			else if ( PropertyName == TEXT("TextStyleCustomization") )
			{
				// this represents the inner-most property that the user modified
				UProperty* ModifiedProperty = PropertyThatChanged.GetTail()->GetValue();
				if ( (ModifiedProperty->PropertyFlags&CPF_Edit) != 0 )
				{
					RefreshAppliedStyleData();
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated text alignment
 * properties over to the style customization property.
 */
void UUIComp_DrawString::PostLoad()
{
	Super::PostLoad();

#if !CONSOLE
	if ( !GIsUCCMake )
	{
		const INT LinkerVersion = GetLinkerVersion();
		if ( LinkerVersion < VER_ADDED_CLIPMODE_TO_TEXTSTYLES )
		{
			// wrap mode
			{
				if ( WrapMode > CLIP_MAX )
				{
					TextStyleCustomization.SetCustomClipMode(CLIP_None);

					// UByteProperty::SerializeItem will set WrapMode to CLIP_MAX + 1 if the previous value was CLIP_Scaled
					// and CLIP_MAX + 2 if the previous value was CLIP_ScaledBestFit
					if ( WrapMode == CLIP_MAX + 1 )
					{
						// UIAUTOSCALE_Normal == CLIP_Scaled
						TextStyleCustomization.SetCustomAutoScaling(UIAUTOSCALE_Normal, 0.f);
					}
					else if ( WrapMode == CLIP_MAX + 2 )
					{
						// UIAUTOSCALE_Justified == CLIP_ScaledBestFit
						TextStyleCustomization.SetCustomAutoScaling(UIAUTOSCALE_Justified, 0.f);
					}
				}
				else if ( WrapMode != CLIP_MAX )
				{
					TextStyleCustomization.SetCustomClipMode(static_cast<ETextClipMode>(WrapMode));
				}
			}

			// clip alignment
			{
				if ( ClipAlignment < UIALIGN_MAX && ClipAlignment != UIALIGN_Default )
				{
					TextStyleCustomization.SetCustomClipAlignment(static_cast<EUIAlignment>(ClipAlignment));
				}
			}

			// text alignment
			if ( LinkerVersion < VER_UPDATED_SOUND_NODE_WAVE )
			{
				if ( TextAlignmentHorizontal < UIALIGN_MAX && TextAlignmentHorizontal != UIALIGN_Default )
				{
					TextStyleCustomization.SetCustomAlignment(UIORIENT_Horizontal, (EUIAlignment)TextAlignmentHorizontal);
				}

				if ( TextAlignmentVertical < UIALIGN_MAX && TextAlignmentVertical != UIALIGN_Default )
				{
					TextStyleCustomization.SetCustomAlignment(UIORIENT_Vertical, (EUIAlignment)TextAlignmentVertical);
				}
			}
		}
	}
#endif
}

/* ==========================================================================================================
	UUIComp_DrawStringEditbox
========================================================================================================== */
#define EDITBOXCOMPONENT_IMPLEMENTS_DSSUBSCRIBER 0

/* === UUIComp_DrawStringEditbox interface === */
/**
 * Requests a new UIStringNode_Image from the Images data store.  Initializes the image node
 * and assigns it to the value of CaretNode.
 *
 * @param	bRecreateExisting	specifies what should happen if this component already has a valid CaretNode.  If
 *								TRUE, the existing caret is deleted and a new one is created.
 */
void UUIComp_DrawStringEditbox::ResolveCaretImageNode( UBOOL bRecreateExisting/*=FALSE*/ )
{
	FUIStringNode_Image* NewCaretNode=NULL;

	if ( CaretNode == NULL || bRecreateExisting == TRUE )
	{
		FString CaretMarkup;

		// generate a string that can be used by the images data store to create an image node
		if ( GenerateCaretMarkup(CaretMarkup) )
		{
			UUIScreenObject* OwnerWidget = GetOuterUUIObject();
			UUIScene* OwnerScene = OwnerWidget->GetScene();
			if ( OwnerScene != NULL )
			{
				UUIDataStore* ImageDataStore = OwnerScene->ResolveDataStore(NAME_Images);
				if ( ImageDataStore != NULL )
				{
					FUIProviderFieldValue CaretValue(EC_EventParm);
					if ( ImageDataStore->GetDataStoreValue(CaretMarkup, CaretValue) )
					{
						FUIStringNode* StringNode = UUIDataProvider::CreateStringNode(CaretMarkup, CaretValue);

						// the "Images" data store will only ever generate UIStringNode_Images, so this always evaluate to true
						if ( StringNode != NULL )
						{
							check(StringNode->IsImageNode());
							NewCaretNode = static_cast<FUIStringNode_Image*>(StringNode);
							NewCaretNode->NodeDataStore = ImageDataStore;

#if EDITBOXCOMPONENT_IMPLEMENTS_DSSUBSCRIBER
							ImageDataStore->eventSubscriberAttached(this);
#endif

							//@todo - any other initialization required for the caret image node
						}
					}
				}
			}
		}
		else
		{
			DeleteCaretNode();
		}
	}

	if ( NewCaretNode != NULL )
	{
		// if we have a previously existing node, delete it now
		DeleteCaretNode();
		CaretNode = NewCaretNode;
	}
}

/**
 * Inserts the markup necessary to render the caret at the appropriate position in SourceText.
 *
 * @param	out_CaretMarkupString	a string containing markup code necessary for the caret image to be resolved into a UIStringNode_Image
 *
 * @return	TRUE if out_ProcessedString contains valid caret markup text.
 *			FALSE if this component is configured to not render a caret, or the caret reference is invalid.
 */
UBOOL UUIComp_DrawStringEditbox::GenerateCaretMarkup( FString& out_CaretMarkupString )
{
	UBOOL bResult = FALSE;

	// if this component is configured to render a caret and we have a valid caret type
	if ( StringCaret.bDisplayCaret && StringCaret.CaretType < UIPEN_MAX )
	{
		// get a reference to the configured caret image
		UMaterial* CaretMaterial = GEngine->DefaultUICaretMaterial;
		if ( CaretMaterial != NULL )
		{
			// try to use the configured scene client client class
			UClass* SceneClientClass = UUIRoot::GetDefaultUIController()->SceneClientClass;
			if ( SceneClientClass == NULL )
			{
				// if none is set, fallback to the default engine version
				SceneClientClass = UGameUISceneClient::StaticClass();
			}

			// figure out which texture should be used as the source texture for the caret material instance
			UTexture* CaretImage = SceneClientClass->GetDefaultObject<UGameUISceneClient>()->DefaultUITexture[StringCaret.CaretType];
			if ( CaretImage != NULL )
			{
				UMaterialInstanceConstant* CaretInstance = Cast<UMaterialInstanceConstant>(StringCaret.CaretMaterial);
				if ( StringCaret.CaretMaterial == NULL )
				{
					CaretInstance = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass());
				}

				CaretInstance->SetParent(CaretMaterial);
				CaretInstance->SetTextureParameterValue(TEXT("SourceTexture"), CaretImage);
				StringCaret.CaretMaterial = CaretInstance;

				// generate the markup that will resolve to the caret image we want to render
				out_CaretMarkupString = FString::Printf(TEXT("%s;XL=%f"), *StringCaret.CaretMaterial->GetPathName(), StringCaret.CaretWidth );
				bResult = TRUE;
			}
			else
			{
				static UEnum* PenColorEnum = FindObject<UEnum>(UUIRoot::StaticClass(), TEXT("EUIDefaultPenColor"), TRUE);
				checkSlow(PenColorEnum);

				debugfSuppressed(NAME_Warning, TEXT("%s::GenerateCaretMarkup failed: %s.DefaultUITexture[%s] is NULL (%i)"),
					*GetPathName(), *SceneClientClass->GetName(), *PenColorEnum->GetEnum(StringCaret.CaretType).ToString(), StringCaret.CaretType);
			}
		}
		else
		{
			debugfSuppressed(NAME_Warning, TEXT("%s::GenerateCaretMarkup failed: Engine.DefaultUICaretMaterial is NULL"), *GetPathName());
		}
	}

	return bResult;
}

/**
 * Retrieves the image style data associated with the caret's configured style from the currently active
 * skin and applies that style data to the caret's UITexture.
 *
 * @param	CurrentWidgetStyle		the current state of the widget that owns this draw string component.  Used
 *									for choosing which style data set [from the caret's style] to use for rendering.
 *									If not specified, the current state of the widget that owns this component will be used.
 */
void UUIComp_DrawStringEditbox::ApplyCaretStyle( UUIState* CurrentWidgetState/*=NULL*/ )
{
	if ( CurrentWidgetState == NULL )
	{
		// if no state was specified, use the owning widget's current state
		CurrentWidgetState = GetOuterUUIEditBox()->GetCurrentState();
	}

	if ( CurrentWidgetState != NULL )
	{
		// if the string has a valid caret stylename
		if ( StringCaret.CaretStyle != NAME_None )
		{
			// get the currently active skin
			UUISkin* ActiveSkin = GetOuterUUIObject()->GetActiveSkin();
			if ( ActiveSkin != NULL )
			{
				// lookup the style specified in the currently active skin
				UUIStyle* CaretStyle = ActiveSkin->FindStyle(StringCaret.CaretStyle);
				if ( CaretStyle != NULL )
				{
					// create the caret stringnode, if necessary
					ResolveCaretImageNode();
					if ( CaretNode != NULL )
					{
						// initialize the node's style with the caret style from the currently active skin
						//@todo ronp - should we add a UIImageStyleOverride to allow overriding the caret style's values?
						UUIStyle_Image* CaretStyleData = Cast<UUIStyle_Image>(CaretStyle->GetStyleForState(CurrentWidgetState));
						if ( CaretStyleData != NULL )
						{
							CaretNode->InitializeStyle(CaretStyleData);
						}

						check( CaretNode->RenderedImage==NULL || CaretNode->RenderedImage->HasValidStyleData() );
					}
				}
				else
				{
					debugf(NAME_Warning, TEXT("%s::ApplyCaretStyle: Caret style (%s) not found in currently active skin '%s'!"), *GetPathName(), *StringCaret.CaretStyle.ToString(), *ActiveSkin->GetPathName());
				}
			}
		}
		else
		{
			debugf(NAME_Warning, TEXT("%s::ApplyCaretStyle: no caret style configured (UIComp_DrawStringEditbox.StringCaret.CaretStyle)!"), *GetPathName());
		}
	}
	else
	{
		debugf(NAME_Warning, TEXT("%s::ApplyCaretStyle: NULL WidgetState specified!"), *GetPathName());
	}
}

/**
 * Moves the caret to a new position in the text
 *
 * @param	NewCaretPosition	the location to put the caret.  Should be a non-zero integer between 0 and the length
 *								of UserText.  Values outside the valid range will be clamed.
 *
 * @return	TRUE if the string's new caret position is different than the string's previous caret position.
 */
UBOOL UUIComp_DrawStringEditbox::SetCaretPosition( INT NewCaretPosition )
{
	UBOOL bCaretPositionChanged = FALSE;

	// save the previous value; even if NewCaretPosition is a different value, it will be clamped if it is not in the valid range,
	// so we compare the old value to the new value after clamping to see if it really changed
	INT PreviousCaretPosition = StringCaret.CaretPosition;
	StringCaret.CaretPosition = Clamp(NewCaretPosition, 0, UserText.Len());

	if ( StringCaret.CaretPosition != PreviousCaretPosition )
	{
		// The RenderText in the editbox string's node now contains only the visible text; in order for the full value
		// to be properly formatted we need to reset the editbox's render text to the full string (only relevant when UserText
		// is too long to be displayed in the editbox).
		//@todo ronp - optimization: only call Super::SetValue() when the new caret position would make the visible text change
		Super::SetValue(UserText);

		ReapplyFormatting();
		bCaretPositionChanged = TRUE;
	}

	return bCaretPositionChanged;
}

/**
 * Updates the value of FirstCharacterPosition with the location of the first character of the string that is now visible.
 */
void UUIComp_DrawStringEditbox::UpdateFirstVisibleCharacter()
{
	checkfSlow(ValueString, TEXT("NULL ValueString in DrawStringComponent (did you forget to call InitializeComponent from %s::Initialize()?)"), *GetOuter()->GetName());

	// if the caret is left of the first visible character, the first visible character should now be the one next to the caret
	if ( StringCaret.CaretPosition <= FirstCharacterPosition )
	{
		FirstCharacterPosition = Max(0, StringCaret.CaretPosition - 1);
	}
	else
	{
		FUICombinedStyleData CombinedStyleData;
		verify(GetFinalStringStyle(CombinedStyleData));
		check(CombinedStyleData.DrawFont);

		// get the current bounding region for the owning widget
		UUIEditBox* OwnerEditbox = GetOuterUUIEditBox();
		FLOAT* RenderBounds = OwnerEditbox->RenderBounds;
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			RenderBounds[UIFACE_Right], RenderBounds[UIFACE_Bottom],
			CombinedStyleData.DrawFont);
		Parameters.Scaling = CombinedStyleData.TextScale;
		Parameters.TextAlignment[UIORIENT_Horizontal] = CombinedStyleData.TextAlignment[UIORIENT_Horizontal];
		Parameters.TextAlignment[UIORIENT_Vertical] = CombinedStyleData.TextAlignment[UIORIENT_Vertical];

		// subtract the width of the caret from the available bounding region so that the caret doesn't end up 
		// rendering outside the bounds of the editbox
		Parameters.DrawXL -= StringCaret.CaretWidth;

		//@fixme - this doesn't work correctly for editboxes that aren't left aligned (i.e. ClipAlignment is UIALIGN_Right)

		// get a string containing all characters up to the caret position.  if we're in password mode, we need to use
		// the value being rendered (the asterisks), not the input value (UserText)
		FString CaretBoundString = GetDisplayString().Left(StringCaret.CaretPosition);
		FString CaretClippedString;

		// clip that string to the bounding region specified...this will remove the characters that are outside the
		// bounding region
		UUIString::ClipString(Parameters, *CaretBoundString, CaretClippedString, static_cast<EUIAlignment>(CombinedStyleData.TextClipAlignment));
		if ( CaretClippedString.Len() < StringCaret.CaretPosition - FirstCharacterPosition )
		{
			// FirstCharacterPosition is no longer valid, as it is pointing to a character outside the text
			// that will be visible in the editbox
			FirstCharacterPosition = StringCaret.CaretPosition - CaretClippedString.Len();
		}
	}
}

/**
 * Calculates the total width of the characters that precede the FirstCharacterPosition.
 *
 * @param	Parameters	@see UUIString::StringSize() (intentionally passed by value)
 *
 * @return	the width (in pixels) of a sub-string containing all characters up to FirstCharacterPosition.
 */
FLOAT UUIComp_DrawStringEditbox::CalculateFirstVisibleOffset( FRenderParameters Parameters ) const
{
	UUIString::StringSize( Parameters, *GetDisplayString().Left(Clamp(FirstCharacterPosition, 0, UserText.Len())) );
	return Parameters.DrawXL;
}

/**
 * Calculates the total width of the characters that precede the CaretPosition.
 *
 * @param	Parameters	@see UUIString::StringSize() (intentionally passed by value)
 *
 * @return	the width (in pixels) of a sub-string containing all characters up to StringCaret.CaretPosition.
 */
FLOAT UUIComp_DrawStringEditbox::CalculateCaretOffset( FRenderParameters Parameters ) const
{
	// start at the first visible character
	INT StartPosition = Clamp(FirstCharacterPosition, 0, UserText.Len());

	// calculate up the caret position
	INT EndPosition = Clamp(StringCaret.CaretPosition, StartPosition, UserText.Len());

	// get a string containing only the characters from the first position up to the caret
	// if we're in password mode, we need to use the value being rendered (the asterisks), not the input value (UserText)
	FString VisibleString = GetDisplayString().Mid(StartPosition, EndPosition - StartPosition);


	// calculate the width of that string
	UUIString::StringSize( Parameters, *VisibleString );
	return Parameters.DrawXL;
}

/**
 * Deletes the existing caret node if one exists, and unregisters this component from the images data store.
 */
void UUIComp_DrawStringEditbox::DeleteCaretNode()
{
	if ( CaretNode != NULL )
	{
#if EDITBOXCOMPONENT_IMPLEMENTS_DSSUBSCRIBER
		if ( CaretNode->NodeDataStore != NULL )
		{
			CaretNode->NodeDataStore->eventSubscriberDetached(this);
		}
#endif
		delete CaretNode;
		CaretNode = NULL;
	}
}

/**
 * Changes the value of UserText to the specified text.
 *
 * SetUserText should be used for modifying the "input text"; that is, the text that would potentially be published to
 * the data store this editbox is bound to.
 * SetValue should be used to change the raw string that will be parsed by the underlying UIString.  UserText will be
 * set to the resolved value of the parsed string.
 *
 * @param	NewText		the new text that should be displayed
 *
 * @return	TRUE if the value changed.
 */
UBOOL UUIComp_DrawStringEditbox::SetUserText( const FString& NewValue )
{
	UBOOL bResult = FALSE;

	if ( UserText != NewValue )
	{
		UserText = NewValue;

		Super::SetValue(UserText);

		// make sure the caret position is still valid
		SetCaretPosition(StringCaret.CaretPosition);

		// even if the caret position didn't change, we may still need to adjust the first visible character
		ReapplyFormatting();

		bResult = TRUE;
	}

	return bResult;
}

/**
 * Returns the length of UserText
 */
INT UUIComp_DrawStringEditbox::GetUserTextLength() const
{
	return UserText.Len();
}

/**
 * @return	the string being rendered in the editbox; equal to UserText unless the editbox is in password mode.
 */
FString UUIComp_DrawStringEditbox::GetDisplayString() const
{
	if ( !GetOuterUUIEditBox()->bPasswordMode )
	{
		return UserText;
	}
	
	// if we're in password mode, we need to use the value being rendered (the asterisks), not the input value (UserText)
	return Super::GetValue(TRUE);
}

/* === UUIComp_DrawString interface === */
/**
 * Initializes this component, creating the UIString needed for rendering text.
 *
 * @param	InSubscriberOwner	if this component is owned by a widget that implements the IUIDataStoreSubscriber interface,
 *								the TScriptInterface containing the interface data for the owner.
 */
void UUIComp_DrawStringEditbox::InitializeComponent( TScriptInterface<IUIDataStoreSubscriber>* InSubscriberOwner/*=NULL*/ )
{
	Super::InitializeComponent(InSubscriberOwner);

	ResolveCaretImageNode();
}

/**
 * Changes the text that will be parsed by the UIString, and updates UserText to the resolved value.
 *
 * @param	NewText		the new text that should be displayed
 */
void UUIComp_DrawStringEditbox::SetValue( const FString& NewText )
{
	// our internal string will update the value of UserText (UUIEditboxString::SetValue()), so
	// store a copy of it so we can determine whether it has changed
	const FString PreviousValue = UserText;

	// apply the new value to the UIString
	Super::SetValue(NewText);

	// bTextChanged might be FALSE if NewText was the same as the editbox's current data store binding
	// and the data store's value changed, so we'll also check whether the resolved value changed as well
	if ( UserText != PreviousValue )
	{
		// make sure the caret position is still valid
		SetCaretPosition(StringCaret.CaretPosition);

		// even if the caret position didn't change, we may still need to adjust the first visible character
		ReapplyFormatting();
	}
}

/**
 * Retrieve the text value of this editbox.
 *
 * @param	bReturnInputText	specify TRUE to return the value of UserText; FALSE to return the raw text stored
 *								in the UIString's node's SourceText
 *
 * @return	either the raw value of this editbox string component or the text that the user entered
 */
FString UUIComp_DrawStringEditbox::GetValue( UBOOL bReturnInputText/*=TRUE*/ ) const
{
	if ( bReturnInputText )
	{
		return UserText;
	}

	//@NOTE: NEVER call GetValue(TRUE) on the owning editbox, since that results in calling this method.
	return GetOuterUUIEditBox()->GetValue(FALSE);
}

/**
 * Retrieves a list of all data stores resolved by ValueString.
 *
 * @param	StringDataStores	receives the list of data stores that have been resolved by the UIString.  Appends all
 *								entries to the end of the array and does not clear the array first.
 */
void UUIComp_DrawStringEditbox::GetResolvedDataStores( TArray<UUIDataStore*>& StringDataStores )
{
	Super::GetResolvedDataStores(StringDataStores);

#if EDITBOXCOMPONENT_IMPLEMENTS_DSSUBSCRIBER
	if ( CaretNode != NULL && CaretNode->NodeDataStore != NULL )
	{
		StringDataStores.AddUniqueItem(CaretNode->NodeDataStore);
	}
#endif
}

/**
 * Refreshes the UIString's extents and appearance based on the string's current style and formatting options.
 *
 * @param	bDockingStackChanged	if TRUE, the scene will rebuild its DockingStack at the beginning
 *									the next frame
 * @param	bPositionsChanged		if TRUE, the scene will update the positions for all its widgets
 *									at the beginning of the next frame
 * @param	bNavLinksOutdated		if TRUE, the scene will update the navigation links for all widgets
 *									at the beginning of the next frame
 * @param	bWidgetStylesChanged	if TRUE, the scene will refresh the widgets reapplying their current styles
 */
void UUIComp_DrawStringEditbox::ReapplyFormatting( UBOOL bDockingStackChanged/*=FALSE*/, UBOOL bPositionsChanged/*=TRUE*/, UBOOL bNavLinksOutdated/*=FALSE*/, UBOOL bWidgetStylesChanged/*=FALSE*/ )
{
	bRecalculateFirstCharacter = TRUE;
	Super::ReapplyFormatting(bDockingStackChanged,bPositionsChanged,bNavLinksOutdated,bWidgetStylesChanged);
}

/**
 * Resolves the combo style for this string rendering component.
 *
 * This version also resolves the caret image style.
 *
 * @param	ActiveSkin			the skin the use for resolving the style reference.
 * @param	bClearExistingValue	if TRUE, style references will be invalidated first.
 * @param	CurrentMenuState	the menu state to use for resolving the style data; if not specified, uses the current
 *								menu state of the owning widget.
 * @param	StyleProperty		if specified, only the style reference corresponding to the specified property
 *								will be resolved; otherwise, all style references will be resolved.
 */
UBOOL UUIComp_DrawStringEditbox::NotifyResolveStyle( UUISkin* ActiveSkin, UBOOL bClearExistingValue, UUIState* CurrentMenuState/*=NULL*/, const FName StylePropertyName/*=NAME_None*/ )
{
	UBOOL bResult = Super::NotifyResolveStyle(ActiveSkin, bClearExistingValue, CurrentMenuState, StylePropertyName);
	SetValue(UserText);
	ApplyCaretStyle(CurrentMenuState);

	return bResult;
}


/**
 * Wrapper for calling ApplyFormatting on the string.  Resets the value of bReapplyFormatting and bRecalculateFirstCharacter.
 *
 * @param	Parameters		@see UUIString::ApplyFormatting())
 * @param	bIgnoreMarkup	@see UUIString::ApplyFormatting())
 */
void UUIComp_DrawStringEditbox::ApplyStringFormatting( FRenderParameters& Parameters, UBOOL bIgnoreMarkup )
{
	if ( bRecalculateFirstCharacter == TRUE )
	{
		bRecalculateFirstCharacter = FALSE;
		UpdateFirstVisibleCharacter();
	}

	Super::ApplyStringFormatting(Parameters, bIgnoreMarkup);
}

/**
 * Renders the string and caret using the parameters specified.
 *
 * @param	Canvas	the canvas to use for rendering this string
 */
void UUIComp_DrawStringEditbox::InternalRender_String( FCanvas* Canvas, FRenderParameters& Parameters )
{
	Super::InternalRender_String(Canvas, Parameters);

	if ( CaretNode != NULL && StringCaret.bDisplayCaret )
	{
		UUIEditBox* EditboxOwner = GetOuterUUIEditBox();
		if ( !EditboxOwner->bReadOnly )
		{
			UBOOL bIsFocused = FALSE;
			const INT NumPlayers = UUIInteraction::GetPlayerCount();
			for ( INT PlayerIndex=0; PlayerIndex < NumPlayers; PlayerIndex++ )
			{
				if ( EditboxOwner->IsFocused(PlayerIndex) )
				{
					bIsFocused = TRUE;
					break;
				}
			}

			if ( bIsFocused )
			{
				// now render the caret
				Parameters.DrawX += CaretOffset;
				Parameters.DrawXL = StringCaret.CaretWidth;
				Parameters.ImageExtent = CaretNode->Extent;
				CaretNode->Render_Node(Canvas, Parameters);
			}
		}
	}
}

/* === UObject interface === */
void UUIComp_DrawStringEditbox::AddReferencedObjects( TArray<UObject*>& ObjectArray )
{
	Super::AddReferencedObjects(ObjectArray);
	if ( CaretNode != NULL )
	{
		AddReferencedObject(ObjectArray,CaretNode->RenderedImage);
	}
}

/**
 * I/O function
 */
void UUIComp_DrawStringEditbox::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	if ( CaretNode != NULL && !Ar.IsPersistent() )
	{
		Ar << *CaretNode;
	}
}

void UUIComp_DrawStringEditbox::FinishDestroy()
{
	if ( CaretNode != NULL )
	{
		delete CaretNode;
		CaretNode = NULL;
	}

	Super::FinishDestroy();
}

/**
 * Called when a property value from a member struct or array has been changed in the editor.
 */
void UUIComp_DrawStringEditbox::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* MemberProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( MemberProperty != NULL )
		{
			FName PropertyName = MemberProperty->GetFName();
			if ( PropertyName == TEXT("StringCaret") )
			{
				// reapply the caret's style to the caret image, creating a new one if necessary
				ApplyCaretStyle();

				// set the flag to indicate that the editbox needs to be updated
				ReapplyFormatting();

				//@todo - is it necessary to rebuild the docking stack?
				GetOuterUUIObject()->RefreshPosition();
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/* ==========================================================================================================
	UUIComp_ListPresenter
========================================================================================================== */
/**
 * Changes whether this list renders colum headers or not.  Only applicable if the owning list's CellLinkType is LINKED_Columns
 */
void UUIComp_ListPresenter::EnableColumnHeaderRendering( UBOOL bShouldRenderColHeaders/*=TRUE*/ )
{
	if ( bShouldRenderColHeaders != bDisplayColumnHeaders )
	{
		bShouldRenderColHeaders = bDisplayColumnHeaders;
		ReapplyFormatting();
	}
}

/**
 * Returns whether this list should render column headers
 */
UBOOL UUIComp_ListPresenter::ShouldRenderColumnHeaders() const
{
	if ( bDisplayColumnHeaders )
	{
		UUIList* OwnerList = GetOuterUUIList();
		return OwnerList->CellLinkType == LINKED_Columns;
	}

	return FALSE;
}

/**
 * Returns whether the list's bounds will be adjusted for the specified orientation considering the list's configured
 * autosize and cell link type values.
 *
 * @param	Orientation		the orientation to check auto-sizing for
 */
UBOOL UUIComp_ListPresenter::ShouldAdjustListBounds( BYTE Orientation ) const
{
	UBOOL bResult = FALSE;
	UUIList* Owner = GetOuterUUIList();
	if ( Owner->CellLinkType != LINKED_None )
	{
		if ( Orientation == UIORIENT_Horizontal )
		{
			bResult = Owner->ColumnAutoSizeMode == CELLAUTOSIZE_AdjustList;
		}
		else if ( Orientation == UIORIENT_Vertical )
		{
			bResult = Owner->RowAutoSizeMode == CELLAUTOSIZE_AdjustList;
		}
	}
	return bResult;
}

/**
 * Returns whether element size is determined by the elements themselves.  For lists with linked columns, returns whether
 * the item height is autosized; for lists with linked rows, returns whether item width is autosized.
 */
UBOOL UUIComp_ListPresenter::IsElementAutoSizingEnabled() const
{
	return GetOuterUUIList()->IsElementAutoSizingEnabled();
}

/**
 * Returns the object that provides the cell schema for this component's owner list (usually the class default object for
 * the class of the owning list's list element provider)
 */
TScriptInterface<IUIListElementCellProvider> UUIComp_ListPresenter::GetCellSchemaProvider() const
{
	TScriptInterface<IUIListElementCellProvider> CellProvider;

	UUIList* OwnerList = GetOuterUUIList();
	if ( OwnerList->DataProvider )
	{
		// get the schema provider, which will probably be the default object for the class that is implementing this UIListElementProvider interface
		CellProvider = OwnerList->DataProvider->GetElementCellSchemaProvider(OwnerList->DataSource.DataStoreField);
	}
	return CellProvider;
}


/**
 * Find the index of the list item which corresponds to the data element specified.
 *
 * @param	DataSourceIndex		the index into the list element provider's data source collection for the element to find.
 *
 * @return	the index [into the ListItems array] for the element which corresponds to the data element specified, or INDEX_NONE
 * if none where found or DataSourceIndex is invalid.
 */
INT UUIComp_ListPresenter::FindElementIndex( INT DataSourceIndex ) const
{
	INT Result = INDEX_NONE;

	if ( DataSourceIndex >= 0 )
	{
		for ( INT ElementIndex = 0; ElementIndex < ListItems.Num(); ElementIndex++ )
		{
			const FUIListItem& Item = ListItems(ElementIndex);
			if (Item.DataSource.DataSourceIndex == DataSourceIndex
			&&	Item.DataSource.DataSourceProvider )
			{
				Result = ElementIndex;
				break;
			}
		}
	}

	return Result;
}

/**
 * Changes the data binding for the specified cell index.
 *
 * @param	CellDataBinding		a name corresponding to a tag from the UIListElementProvider currently bound to this list.
 * @param	ColumnHeader		the string that should be displayed in the column header for this cell.
 * @param	BindingIndex		the column or row to bind this data field to.  If BindingIndex is greater than the number
 *								schema cells, empty schema cells will be added to meet the number required to place the data 
 *								at BindingIndex.
 *								If a value of INDEX_NONE is specified, the cell binding will only occur if there are no other
 *								schema cells bound to that data field.  In this case, a new schema cell will be appended and
 *								it will be bound to the data field specified.
 */
UBOOL UUIComp_ListPresenter::SetCellBinding( FName CellDataBinding, const FString& ColumnHeader, INT BindingIndex )
{
	UBOOL bResult = FALSE;

	TScriptInterface<IUIListElementCellProvider> SchemaProvider = GetCellSchemaProvider();
	if ( SchemaProvider && CellDataBinding != NAME_None && BindingIndex >= INDEX_NONE )
	{
		UUIList* OwnerList = GetOuterUUIList();

		if ( BindingIndex == INDEX_NONE )
		{
			BindingIndex = ElementSchema.Cells.Num();
		}

		if ( BindingIndex >= ElementSchema.Cells.Num() )
		{
			// create empty cells to fill the space between the current number of schema cells and the cell index we want to bind
			for ( INT CellIndex = ElementSchema.Cells.Num(); CellIndex <= BindingIndex; CellIndex++ )
			{
				FUIListElementCellTemplate* EmptyCell = new(ElementSchema.Cells,CellIndex) FUIListElementCellTemplate(EC_EventParm);
				if ( CellIndex < BindingIndex )
				{
					EmptyCell->OnCellCreated(OwnerList);
					EmptyCell->AssignBinding(TScriptInterface<IUIListElementCellProvider>(), NAME_None, TEXT(""));
				}
			}

			ReapplyFormatting();
		}

		FUIListElementCellTemplate& Cell = ElementSchema.Cells(BindingIndex);
		Cell.OnCellCreated(OwnerList);
		Cell.AssignBinding(SchemaProvider, CellDataBinding, ColumnHeader);
		bResult = TRUE;
	}
	
	return bResult;
}

/**
 * Inserts a new schema cell at the specified index and assigns the data binding.
 *
 * @param	InsertIndex			the column/row to insert the schema cell; must be a valid index.
 * @param	CellDataBinding		a name corresponding to a tag from the UIListElementProvider currently bound to this list.
 * @param	ColumnHeader	the string that should be displayed in the column header for this cell.
 *
 * @return	TRUE if the schema cell was successfully inserted into the list
 */
UBOOL UUIComp_ListPresenter::InsertSchemaCell( INT InsertIndex, FName CellDataBinding, const FString& ColumnHeader )
{
	UBOOL bResult = FALSE;

	UUIList* OwnerList = GetOuterUUIList();
	if ( CellDataBinding != NAME_None && InsertIndex >= 0 && InsertIndex <= ElementSchema.Cells.Num() )
	{
		TScriptInterface<IUIListElementCellProvider> SchemaProvider = GetCellSchemaProvider();
		if ( SchemaProvider )
		{
			FUIListElementCellTemplate* Cell = new(ElementSchema.Cells,InsertIndex) FUIListElementCellTemplate(EC_EventParm);
			if ( Cell != NULL )
			{
				Cell->OnCellCreated(OwnerList);
				Cell->AssignBinding(SchemaProvider, Cell->CellDataField, Cell->ColumnHeaderText);
				ReapplyFormatting();
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

/**
 * Removes all schema cells which are bound to the specified data field.
 *
 * @return	TRUE if one or more schema cells were successfully removed.
 */
UBOOL UUIComp_ListPresenter::ClearCellBinding( FName CellDataBinding )
{
	UBOOL bResult = FALSE;

	INT NumberOfCellsRemoved = 0;
	for ( INT CellIndex = ElementSchema.Cells.Num() - 1; CellIndex >= 0; CellIndex-- )
	{
		FUIListElementCellTemplate& SchemaCell = ElementSchema.Cells(CellIndex);

		if ( SchemaCell.CellDataField == CellDataBinding )
		{
			Modify();
			ElementSchema.Cells.Remove(CellIndex);
			NumberOfCellsRemoved++;
			ReapplyFormatting();
		}
	}

	bResult = (NumberOfCellsRemoved > 0);

	return bResult;
}

/**
 * Removes schema cells at the location specified.  If the list's columns are linked, this index should correspond to
 * the column that should be removed; if the list's rows are linked, this index should correspond to the row that should
 * be removed.
 *
 * @return	TRUE if the schema cell at BindingIndex was successfully removed.
 */
UBOOL UUIComp_ListPresenter::ClearCellBinding( INT BindingIndex )
{
	UBOOL bResult = FALSE;

	if ( ElementSchema.Cells.IsValidIndex(BindingIndex) )
	{
		Modify();
		ElementSchema.Cells.Remove(BindingIndex);
		ReapplyFormatting();
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Resolves the element schema provider based on the owning list's data source binding, and repopulates the element schema based on
 * the available data fields in that element schema provider.
 */
void UUIComp_ListPresenter::RefreshElementSchema()
{
	// get the schema provider, which will probably be the default object for the class that is implementing this UIListElementProvider interface
	TScriptInterface<IUIListElementCellProvider> CellProvider = GetCellSchemaProvider();
	if ( CellProvider )
	{
		UUIList* OwnerList = GetOuterUUIList();

		// retrieve the list of data field tags that this list element provider supports
		TMap<FName,FString> AvailableCellTags;
		CellProvider->GetElementCellTags(AvailableCellTags);

		if(!GIsGame)
		{
			// verify that the new schema provider contains the tags currently assigned to the schema cells
			for ( INT CellIndex = ElementSchema.Cells.Num() - 1; CellIndex >= 0; CellIndex-- )
			{
				FName& DataBinding = ElementSchema.Cells(CellIndex).CellDataField;
				if ( AvailableCellTags.Find(DataBinding) == NULL )
				{
					Modify();

					// the data field that this cell is currently bound to does not exist in the new schema provider, so remove it
					ElementSchema.Cells.Remove(CellIndex);
				}
			}
		}

		// if we currently don't have any cells in the schema, add them all
		if ( ElementSchema.Cells.Num() == 0 )
		{
			Modify();
			if ( OwnerList->CellLinkType != LINKED_None )
			{
				ElementSchema.Cells.Empty(AvailableCellTags.Num());
				ElementSchema.Cells.AddZeroed(AvailableCellTags.Num());
			}
			else
			{
				ElementSchema.Cells.AddZeroed(1);
			}

			TArray<FName> CellTags;
			TArray<FString> ColumnHeaders;
			AvailableCellTags.GenerateKeyArray(CellTags);
			AvailableCellTags.GenerateValueArray(ColumnHeaders);
			for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
			{
				FUIListElementCellTemplate& Cell = ElementSchema.Cells(CellIndex);
				Cell.CellDataField = CellTags(CellIndex);
				Cell.ColumnHeaderText = ColumnHeaders(CellIndex);
			}
		}

		for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
		{
			FUIListElementCellTemplate& Cell = ElementSchema.Cells(CellIndex);

			Cell.OnCellCreated(OwnerList);
			Cell.AssignBinding(CellProvider, Cell.CellDataField, Cell.ColumnHeaderText);
		}
	}
	else if ( ElementSchema.Cells.Num() > 0 )
	{
		Modify();
		ElementSchema.Cells.Empty();
		ReapplyFormatting();
	}
}

/**
 * Called when a new element is added to the list that owns this component.  Creates a UIElementCellList for the specified element.
 *
 * @param	InsertIndex			an index in the range of 0 - Items.Num() to use for inserting the element.  If the value is
 *								not a valid index, the element will be added to the end of the list.
 * @param	ElementValue		the index [into the data provider's collection] for the element that is being inserted into the list.
 *
 * @return	the index where the new element was inserted, or INDEX_NONE if the element wasn't added to the list.
 */
INT UUIComp_ListPresenter::InsertElement( INT InsertIndex, INT ElementValue )
{
	INT ActualInsertIndex = InsertIndex;

	UUIList* OwnerList = GetOuterUUIList();
	if ( OwnerList->DataProvider )
	{
		if ( !ListItems.IsValidIndex(InsertIndex) )
		{
			InsertIndex = ListItems.Num();
		}

		// get the schema provider, which will probably be the default object for the class that is implementing this UIListElementProvider interface
		TScriptInterface<IUIListElementCellProvider> CellProvider = OwnerList->DataProvider->GetElementCellValueProvider(OwnerList->DataSource.DataStoreField, ElementValue);
		if ( CellProvider )
		{
			// now that we're clear to actually add the element, first update the list's array of items
			// (FUIListElementCell::ApplyCellStyleData will need to find the index in the list's Items array for this data element)
			OwnerList->Items.InsertItem(ElementValue, InsertIndex);

			//@fixme - check that CellProvider considers ElementValue a valid index....otherwise we'll have entries for elements in the list, but there won't be any actual data rendering
			FUIListItemDataBinding DataBinding(CellProvider, OwnerList->DataSource.DataStoreField, ElementValue);
			FUIListItem* NewElement = new(ListItems,InsertIndex) FUIListItem(DataBinding);

			// add enough cells to accommodate the number of data cells associated with this element
			NewElement->Cells.Empty(ElementSchema.Cells.Num());
			for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
			{
				FUIListElementCellTemplate& CellSource = ElementSchema.Cells(CellIndex);
				FUIListElementCell* NewCell = new(NewElement->Cells,CellIndex) FUIListElementCell(EC_EventParm);

				NewCell->OnCellCreated(NewElement,OwnerList);
				CellSource.InitializeCell(DataBinding, *NewCell);
			}

			ReapplyFormatting();
			ActualInsertIndex = InsertIndex;
		}
	}

	return ActualInsertIndex;
}

/**
 * Called when an element is removed from the list that owns this component.  Removes the UIElementCellList located at the
 * specified index.
 *
 * @param	RemovalIndex	the index for the element that should be removed from the list
 *
 * @return	the index [into the ListItems array] for the element that was removed, or INDEX_NONE if RemovalIndex was invalid
 *			or that element couldn't be removed from this list.
 */
INT UUIComp_ListPresenter::RemoveElement( INT RemovalIndex )
{
	INT Result = INDEX_NONE;
	if ( ListItems.IsValidIndex(RemovalIndex) )
	{
		ListItems.Remove(RemovalIndex);

		//@todo - more stuff

		Result = RemovalIndex;
		ReapplyFormatting();
	}

	return Result;
}

/**
 * Swaps the values at the specified indexes, reversing their positions in the ListItems array.
 *
 * @param	IndexA	the index into the ListItems array for the first element to swap
 * @param	IndexB	the index into the ListItems array for the second element to swap
 *
 * @param	TRUE if the swap was successful
 */
UBOOL UUIComp_ListPresenter::SwapElements( INT IndexA, INT IndexB )
{
	UBOOL bResult = FALSE;

	if ( ListItems.IsValidIndex(IndexA) && ListItems.IsValidIndex(IndexB) )
	{
		ListItems.SwapItems(IndexA,IndexB);
		bResult = TRUE;
	}

	return bResult;
}

/**
 * Allows the list presenter to override the menu state that is used for rendering a specific element in the list.  Used for those
 * lists which need to render some elements using the disabled state, for example.
 *
 * @param	ElementIndex		the index into the Elements array for the element to retrieve the menu state for.
 * @param	out_OverrideState	receives the value of the menu state that should be used for rendering this element. if a specific
 *								menu state is desired for the specified element, this value should be set to a child of UIState corresponding
 *								to the menu state that should be used;  only used if the return value for this method is TRUE.
 *
 * @return	TRUE if the list presenter assigned a value to out_OverrideState, indicating that the element should be rendered using that menu
 *			state, regardless of which menu state the list is currently in.  FALSE if the list presenter doesn't want to override the menu
 *			state for this element.
 */
UBOOL UUIComp_ListPresenter::GetOverrideMenuState( INT ElementIndex, UClass*& out_OverrideState )
{
	UBOOL bResult = FALSE;

	if ( ListItems.IsValidIndex(ElementIndex) )
	{
		UUIList* OwnerList = GetOuterUUIList();

		// query the bound data store to see if this element is disabled - if so, use the disabled menu state
		TScriptInterface<IUIListElementProvider>& ListElementProvider = OwnerList->DataProvider;

		const FName DataSourceField = ListItems(ElementIndex).DataSource.DataSourceTag;
		const INT DataSourceIndex = ListItems(ElementIndex).DataSource.DataSourceIndex;

		if ( !ListElementProvider->IsElementEnabled(DataSourceField,DataSourceIndex) )
		{
			out_OverrideState = UUIState_Disabled::StaticClass();
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Assigns the style for the cell specified and refreshes the cell's resolved style.
 *
 * @param	NewStyle		the new style to assign to this widget
 * @param	ElementState	the list element state to set the element style for
 * @param	CellIndex		indicates the column (if columns are linked) or row (if rows are linked) to apply the style to
 *
 * @return	TRUE if the style was successfully applied to the cell.
 */
UBOOL UUIComp_ListPresenter::SetCustomCellStyle( UUIStyle* NewStyle, EUIListElementState ElementState, INT CellIndex )
{
	UBOOL bStyleChanged = FALSE;

	if ( ElementSchema.Cells.IsValidIndex(CellIndex) && ElementState < ELEMENT_MAX )
	{
		FUIStyleReference* StyleRef = &ElementSchema.Cells(CellIndex).CellStyle[ElementState];
		INT ElementIndex = 0;

		while ( StyleRef != NULL )
		{
			if ( NewStyle != NULL )
			{
				// make sure the new style has a valid ID
				if ( NewStyle->StyleID.IsValid() )
				{
					bStyleChanged = StyleRef->SetStyle(NewStyle) || bStyleChanged;

					// if this widget's style is managed by someone else (usually its owner widget), do not mark the package dirty
					// and clear the assigned style ID so that this widget does not maintain a permanent mapping to that style
					if ( GetOuterUUIList()->IsPrivateBehaviorSet(UCONST_PRIVATE_ManagedStyle) )
					{
						FSTYLE_ID ClearStyleID(EC_EventParm);
						StyleRef->SetStyleID(ClearStyleID);
					}
					else
					{
						// otherwise, mark the package dirty so that we save this style assignment
						Modify(bStyleChanged);
					}
				}
			}
			else
			{
				// if NewStyle is NULL, it indicates that we want to resolve the reference using the reference's default style.
				// The most common reason for doing this is when a new widget is placed in the scene - since we don't know the
				// STYLE_ID of the widget's default style, we need to perform the initial lookup by name.
				bStyleChanged = StyleRef->SetStyle(NULL) || bStyleChanged;
			}

			StyleRef = NULL;
			if ( ElementIndex < ListItems.Num() )
			{
				// grab the appropriate cell from this list element
				FUIListItem& Element = ListItems(ElementIndex);
				StyleRef = &Element.Cells(CellIndex).CellStyle[ElementState];

				ElementIndex++;
			}
		}
	}

	if ( bStyleChanged == TRUE )
	{
		// if the style was applied successfully, resolve the style now to ensure that the new style data is applied to new elements
		ElementSchema.Cells(CellIndex).ResolveCellStyles(ElementState);

		// now update existing elements with this new style
		for ( INT ElementIndex = 0; ElementIndex < ListItems.Num(); ElementIndex++ )
		{
			// grab the appropriate cell from this list element
			FUIListItem& Element = ListItems(ElementIndex);

			// then update the cached style references contained in the cell that was updated
			Element.Cells(CellIndex).ResolveCellStyles(ElementState);
		}
	}

	return bStyleChanged;
}

/**
 * Applies the resolved style data for the column header style to the schema cells' strings.  This function is called anytime
 * the header style data that is applied to the schema cells is no longer valid, such as when the owning list's menu state is changed.
 *
 * @param	ResolvedStyle			the style resolved by the style reference
 */
void UUIComp_ListPresenter::ApplyColumnHeaderStyle( UUIStyle* ResolvedStyle )
{
	// update the column header style in the schema cells
	for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
	{
		FUIListElementCellTemplate& CellTemplate = ElementSchema.Cells(CellIndex);
		if ( CellTemplate.OwnerList != NULL )
		{
			CellTemplate.ApplyHeaderStyleData(ResolvedStyle);
		}
	}

	if ( ElementSchema.Cells.Num() > 0 && ShouldRenderColumnHeaders() )
	{
		ReapplyFormatting();
	}
}

/**
 * Notification that the list's style has been changed.  Updates the cached cell styles for all elements for the specified
 * list element state.
 *
 * @param	ElementState	the list element state to update the element style for
 */
void UUIComp_ListPresenter::OnListStyleChanged( EUIListElementState ElementState )
{
	// update the styles in the schema cells
	for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
	{
		FUIListElementCellTemplate& CellTemplate = ElementSchema.Cells(CellIndex);
		if ( CellTemplate.OwnerList != NULL )
		{
			CellTemplate.ResolveCellStyles(ElementState);
		}
	}

	// for each element of this list
	for ( INT ElementIndex = 0; ElementIndex < ListItems.Num(); ElementIndex++ )
	{
		// grab the list of cells associated with this element
		FUIListItem& Element = ListItems(ElementIndex);

		for ( INT CellIndex = 0; CellIndex < Element.Cells.Num(); CellIndex++ )
		{
			// then update the cached style references contained in each cell
			FUIListElementCell& Cell = Element.Cells(CellIndex);
			Cell.ResolveCellStyles(ElementState);
		}
	}

	// we changed cell styles which may have affected the evaluated size of the list or elements, so
	// we'll need to refresh all formatting for the list.
	if ( GetOuterUUIList()->IsVisible() || ListItems.Num() > 0 || ElementSchema.Cells.Num() > 0 )
	{
		ReapplyFormatting();
	}
}

/**
 * Notification that the list's menu state has changed.  Reapplies the specified cell style for all elements based on the
 * new menu state.
 *
 * @param	ElementState	the list element state to update the element style for
 */
void UUIComp_ListPresenter::OnListMenuStateChanged( EUIListElementState ElementState )
{
	// update the styles in the schema cells
	if ( ElementState < ELEMENT_MAX )
	{
		for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
		{
			FUIListElementCellTemplate& CellTemplate = ElementSchema.Cells(CellIndex);
			if ( CellTemplate.OwnerList != NULL )
			{
				CellTemplate.ApplyCellStyleData(ElementState);
			}
		}

		for ( INT ElementIndex = 0; ElementIndex < ListItems.Num(); ElementIndex++ )
		{
			FUIListItem& Element = ListItems(ElementIndex);
			for ( INT CellIndex = 0; CellIndex < Element.Cells.Num(); CellIndex++ )
			{
				FUIListElementCell& Cell = Element.Cells(CellIndex);

				Cell.ApplyCellStyleData(ElementState);
			}
		}

		if ( ElementSchema.Cells.Num() > 0 || ListItems.Num() > 0 || GetOuterUUIList()->IsVisible() )
		{
			ReapplyFormatting();
		}
	}
}

/** 
 * Returns the maximum number of rows that can be displayed in the list, given its current size and configuration.
 */
INT UUIComp_ListPresenter::GetMaxNumVisibleRows() const
{
	return GetOuterUUIList()->GetMaxNumVisibleRows();
}

/**
 * Returns the maximum number of columns that can be displayed in the list, given its current size and configuration.
 */
INT UUIComp_ListPresenter::GetMaxNumVisibleColumns() const
{
	return GetOuterUUIList()->GetMaxNumVisibleColumns();
}

/** 
 * Returns the total number of rows in this list.
 */
INT UUIComp_ListPresenter::GetTotalRowCount() const
{
	return GetOuterUUIList()->GetTotalRowCount();
}

/**
 * Returns the total number of columns in this list.
 */
INT UUIComp_ListPresenter::GetTotalColumnCount() const
{
	return GetOuterUUIList()->GetTotalColumnCount();
}

/**
 * Changes the cell state for the specified element.
 *
 * @param	ElementIndex	the index of the element to change states for
 * @param	NewElementState	the new state to place the element in
 *
 * @return	TRUE if the new state was successfully applied to the new element, FALSE otherwise.
 */
UBOOL UUIComp_ListPresenter::SetElementState( INT ElementIndex, EUIListElementState NewElementState )
{
	UBOOL bResult = FALSE;
	
	if ( ListItems.IsValidIndex(ElementIndex) )
	{
		FUIListItem& Element = ListItems(ElementIndex);
		if ( Element.SetElementState(NewElementState) )
		{
			ReapplyFormatting();
			bResult = TRUE;
		}	
	}

	return bResult;
}

/**
 * Notifies the owning widget that the formatting and render parameters for the list need to be updated.
 */
void UUIComp_ListPresenter::ReapplyFormatting()
{
	bReapplyFormatting = TRUE;

	GetOuterUUIList()->RequestSceneUpdate(FALSE,TRUE,FALSE);
}

/**
 * Determines the maximum number of elements which can be rendered given the owning list's bounding region.
 */
void UUIComp_ListPresenter::CalculateVisibleElements( FRenderParameters& Parameters )
{
	UUIList* Owner = GetOuterUUIList();

	switch ( Owner->CellLinkType )
	{
	case LINKED_Rows:
		{
			const FLOAT ColumnWidth = Owner->GetColumnWidth();
			if ( ColumnWidth > 0 )
			{
				const FLOAT ListWidth = Parameters.DrawXL;
				INT TotalVisibleItems = appFloor(ListWidth / ColumnWidth);

				// now we have to adjust the number for spacing.  Until we know how many items would be visible, we can't take spacing into account
				// so we need to do that step now
				const FLOAT ColumnSpacing = Owner->CellSpacing.GetValue(Owner);
				FLOAT TotalItemWidth = (ColumnWidth * TotalVisibleItems) + (ColumnSpacing * (TotalVisibleItems - 1));
				if ( TotalItemWidth > appCeil(ListWidth) )
				{
					FLOAT ColAndSpace = ListWidth + ColumnSpacing;
					FLOAT Difference = TotalItemWidth - ListWidth;
					TotalVisibleItems -= appCeil(Difference / ColAndSpace);
				}

				Owner->MaxVisibleItems = Min(Owner->GetItemCount(), TotalVisibleItems);
			}
			else
			{
				Owner->MaxVisibleItems = 0;
			}
		}
		break;

	case LINKED_Columns:
		{
			const FLOAT RowHeight = Owner->GetRowHeight();
			if ( RowHeight > 0 )
			{
				const FLOAT ListHeight = Parameters.DrawYL - (ShouldRenderColumnHeaders() ? RowHeight : 0.f);
				INT TotalVisibleItems = appFloor(ListHeight / RowHeight);

				// now we have to adjust the number for spacing.  Until we know how many items would be visible, we can't take spacing into account
				// so we need to do that step now
				const FLOAT RowSpacing = Owner->CellSpacing.GetValue(Owner);
				FLOAT TotalItemHeight = (RowHeight * TotalVisibleItems) + (RowSpacing * (TotalVisibleItems - 1));
				if ( TotalItemHeight > appCeil(ListHeight) )
				{
					FLOAT RowAndSpace = RowHeight + RowSpacing;
					FLOAT Difference = TotalItemHeight - ListHeight;
					TotalVisibleItems -= appCeil(Difference / RowAndSpace);
				}

				Owner->MaxVisibleItems = Min(Owner->GetItemCount(), TotalVisibleItems);
			}
			else
			{
				Owner->MaxVisibleItems = 0;
			}
		}
		break;

	default:
		{
			const FLOAT ColumnWidth = Owner->GetColumnWidth();
			const FLOAT RowHeight = Owner->GetRowHeight();

			const FLOAT ListWidth = Parameters.DrawXL;
			const FLOAT ListHeight = Parameters.DrawYL - (ShouldRenderColumnHeaders() ? RowHeight : 0.f);

			//@todo ronp - if we add auto-sizing support for non-linked lists, 
			// 1. should the list be resized to fit (ColumnCount*ColumnWidth) x (RowCount*RowHeight)
			//Owner->MaxVisibleItems = Owner->ColumnCount * Owner->RowCount;


			// 2. should the MaxVisibleItems be calculated based on the size of the list.
			INT NumVisibleRows, NumVisibleColumns;
			{
				// calculate the visible rows
				if ( RowHeight > 0 )
				{
					NumVisibleRows = appFloor(ListHeight / RowHeight);

					// now we have to adjust the number for spacing.  Until we know how many items would be visible, we can't take spacing into account
					// so we need to do that step now
					const FLOAT RowSpacing = Owner->CellSpacing.GetValue(Owner);
					FLOAT TotalItemHeight = (RowHeight * NumVisibleRows) + (RowSpacing * (NumVisibleRows - 1));
					if ( TotalItemHeight > appCeil(ListHeight) )
					{
						FLOAT RowAndSpace = RowHeight + RowSpacing;
						FLOAT Difference = TotalItemHeight - ListHeight;
						NumVisibleRows -= appCeil(Difference / RowAndSpace);
					}
				}
				else
				{
					NumVisibleRows = 0;
				}
			}

			{
				// now the columns
				if ( ColumnWidth > 0 )
				{
					NumVisibleColumns = appFloor(ListWidth / ColumnWidth);

					// now we have to adjust the number for spacing.  Until we know how many items would be visible, we can't take spacing into account
					// so we need to do that step now
					const FLOAT ColumnSpacing = Owner->CellSpacing.GetValue(Owner);
					FLOAT TotalItemWidth = (ColumnWidth * NumVisibleColumns) + (ColumnSpacing * (NumVisibleColumns - 1));
					if ( TotalItemWidth > appCeil(ListWidth) )
					{
						FLOAT ColAndSpace = ListWidth + ColumnSpacing;
						FLOAT Difference = TotalItemWidth - ListWidth;
						NumVisibleColumns -= appCeil(Difference / ColAndSpace);
					}
				}
				else
				{
					NumVisibleColumns = 0;
				}
			}

			Owner->MaxVisibleItems = NumVisibleColumns * NumVisibleRows;
		}
		break;
	}
}

/** Converts an array of FUIListElementCells into a array of pointers to FUIListElementCells */
static TArray<FUIListElementCell*> ConvertElementCellArray( TArray<FUIListElementCell>& ElementCells )
{
	TArray<FUIListElementCell*> Result;
	Result.Empty(ElementCells.Num());
	for ( INT CellIndex = 0; CellIndex < ElementCells.Num(); CellIndex++ )
	{
		Result.AddItem(&ElementCells(CellIndex));
	}
	return Result;
}
/** Converts an array of FUIListElementCellTemplates into a array of pointers to FUIListElementCells */
static TArray<FUIListElementCell*> ConvertElementCellArray( TArray<FUIListElementCellTemplate>& SchemaCells )
{
	TArray<FUIListElementCell*> Result;
	Result.Empty(SchemaCells.Num());
	for ( INT CellIndex = 0; CellIndex < SchemaCells.Num(); CellIndex++ )
	{
		Result.AddItem(&SchemaCells(CellIndex));
	}
	return Result;
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Adjusts the owning widget's bounds
 * according to the wrapping mode and autosize behaviors.
 *
 * @param	Face	the face that should be resolved
 */
void UUIComp_ListPresenter::ResolveFacePosition( EUIWidgetFace Face )
{
	//SCOPE_CYCLE_COUNTER(STAT_UIResolvePosition_List);
	if ( bReapplyFormatting )
	{
		FRenderParameters Parameters;
		if ( GetListRenderParameters(Face, Parameters) )
		{
			// format the list elements
			ApplyListFormatting(Parameters);

			// update the bounds of the owning widget if configured to do so
			UpdateOwnerBounds(Parameters);
		}
	}
}

/**
 * Initializes the render parameters that will be used for formatting the list elements.
 *
 * @param	Face			the face that was being resolved
 * @param	out_Parameters	[out] the formatting parameters to use when calling ApplyFormatting.
 *
 * @return	TRUE if the formatting data is ready to be applied to the list elements, taking into account the autosize settings.
 */
UBOOL UUIComp_ListPresenter::GetListRenderParameters( EUIWidgetFace Face, FRenderParameters& out_Parameters ) const
{
	UBOOL bFormattingConditionsMet = FALSE;
	UUIList* Owner = GetOuterUUIList();

	// if the list hasn't been fully initialized, then we can't apply formatting
	if ( !Owner->IsInitialized() )
	{
		return FALSE;
	}

	// for convenience
	FLOAT* RenderBounds = Owner->RenderBounds;
	FUIScreenValue_Bounds& Position = Owner->Position;
	FUIDockingSet& Docking = Owner->DockTargets;

	const UBOOL bAutoSizeHorz = ShouldAdjustListBounds(UIORIENT_Horizontal);
	const UBOOL bAutoSizeVert = ShouldAdjustListBounds(UIORIENT_Vertical);
	const UBOOL bDocked[UIFACE_MAX] =
	{
		Docking.IsDocked(UIFACE_Left), Docking.IsDocked(UIFACE_Top),
		Docking.IsDocked(UIFACE_Right), Docking.IsDocked(UIFACE_Bottom),
	};

	// first, figure out the order in which parallel faces need to be resolved.
	EUIWidgetFace H1, H2, V1, V2;
	// do the horizontal; the left face is always considered H1 unless the right face is docked and the left isn't.
	if ( bDocked[UIFACE_Right] && !bDocked[UIFACE_Left] )
		{ H1 = UIFACE_Right, H2 = UIFACE_Left; }
	else
		{ H1 = UIFACE_Left, H2 = UIFACE_Right; }

	// do the vertical - same deal
	if ( bDocked[UIFACE_Bottom] && !bDocked[UIFACE_Top] )
		{ V1 = UIFACE_Bottom, V2 = UIFACE_Top; }
	else
		{ V1 = UIFACE_Top, V2 = UIFACE_Bottom; }

	if ( bAutoSizeHorz && bAutoSizeVert )
	{
		// if both horizontal and vertical auto-sizing are enabled, the position of the second face for each orientation
		// will be set using the extents of the formatted string; therefore only the first face from each orientation must
		// be resolved before we can AF
		if ( bDocked[H1] && bDocked[H2] && bDocked[V1] && bDocked[V2] )
		{
			bFormattingConditionsMet = Owner->GetNumResolvedFaces() == UIFACE_MAX;
		}
		else
		{
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(V1);
		}
	}
	else if ( bAutoSizeHorz )
	{
		// if horizontal auto-expansion is enabled, the position of the second horizontal face will be set according to the combined
		// widths of all columns; therefore, only the first horizontal face must be resolved before we can AF.  Of course, if we're
		// docked on both sides, we can't do any expansion anyway
		if ( bDocked[H1] && bDocked[H2] )
		{
			bFormattingConditionsMet = Owner->GetNumResolvedFaces() == UIFACE_MAX;
		}
		else
		{
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(V1) && Owner->HasPositionBeenResolved(V2);
		}
	}
	else if ( bAutoSizeVert )
	{
		// If vertical auto-expansion is enabled, the position of the second vertical face will be set according to the resolved height of the string.
		// therefore, the other three faces must be resolved before calling AF.  Of course, if we're docked on both sides, we can't do any expansion anyway
		if ( bDocked[V1] && bDocked[V2] )
		{
			bFormattingConditionsMet = Owner->GetNumResolvedFaces() == UIFACE_MAX;
		}
		else
		{
			bFormattingConditionsMet = Owner->HasPositionBeenResolved(H1) && Owner->HasPositionBeenResolved(H2) && Owner->HasPositionBeenResolved(V1);
		}
	}
	else
	{
		// if auto-sizing is disabled , then just wait until all faces have been resolved so that we can pass in the correct bounding region to AF.
		bFormattingConditionsMet = Owner->GetNumResolvedFaces() == UIFACE_MAX;
	}

	// if we're not ready to format the list, stop here
	if ( bFormattingConditionsMet )
	{
		//@todo ronp - move this into a method, like GetViewportDimension(out_ViewportOrigin, out_ViewportSize)
		FVector2D ViewportOrigin, ViewportSize;
		if ( !Owner->GetViewportOrigin(ViewportOrigin) )
		{
			ViewportOrigin.X = 0.f;
			ViewportOrigin.Y = 0.f;
		}
		if ( !Owner->GetViewportSize(ViewportSize) )
		{
			ViewportSize.X = UCONST_DEFAULT_SIZE_X;
			ViewportSize.Y = UCONST_DEFAULT_SIZE_Y;
		}

		out_Parameters.DrawX = RenderBounds[UIFACE_Left];
		out_Parameters.DrawY = RenderBounds[UIFACE_Top];

		FLOAT BoundingRegionWidth = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
		FLOAT BoundingRegionHeight = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];

		if ( bAutoSizeHorz )
		{
			// if we're configured to auto-size horizontally, rather than passing in the current width of the widget, we pass in the maximum
			// possible horizontal size

			// if the widget is docked on both horizontal faces, then auto-sizing is irrelevant because the widget's size is determined by the positions
			// of the faces that the widget is docked to
			if ( !bDocked[H1] || !bDocked[H2] )
			{
				if ( !bDocked[UIFACE_Right] )
				{
					// If the widget is not docked or is docked only on its left side, then the horizontal bounding region used 
					// is the distance from the right edge of the screen to the left face of the widget.
					out_Parameters.DrawX = RenderBounds[UIFACE_Left];
					BoundingRegionWidth = ViewportSize.X - RenderBounds[UIFACE_Left];
				}
				else
				{
					// If the widget is docked on the right face, then the horizontal bounding region used is the distance from the left
					// edge of the screen to the right face of the widget.
					out_Parameters.DrawX = ViewportOrigin.X;
					BoundingRegionWidth = RenderBounds[UIFACE_Right] - ViewportOrigin.X;
				}
			}
		}

		if ( bAutoSizeVert )
		{
			// if we're configured to auto-size vertically, rather than passing in the current height of the widget, we pass in the maximum
			// possible vertical size

			// if the widget is docked on both vertical faces, then auto-sizing is irrelevant because the widget's size is determined by the positions
			// of the faces that the widget is docked to
			if ( !bDocked[V1] || !bDocked[V2] )
			{
				if ( !bDocked[UIFACE_Bottom] )
				{
					// If the widget is not docked or is docked only on its top face, then the vertical bounding region used 
					// is the distance from the bottom edge of the screen to the top face of the widget.
					out_Parameters.DrawY = RenderBounds[UIFACE_Top];
					BoundingRegionHeight = ViewportSize.Y - RenderBounds[UIFACE_Top];
				}
				else
				{
					// If the widget is docked on the bottom face, then the vertical bounding region used is the distance from the top
					// edge of the screen to the bottom face of the widget.
					out_Parameters.DrawY = ViewportOrigin.Y;
					BoundingRegionHeight = RenderBounds[UIFACE_Bottom] - ViewportOrigin.Y;
				}
			}
		}

		// subtract out any additional offsets
		if ( Owner->VerticalScrollbar != NULL && Owner->VerticalScrollbar->IsVisible() )
		{
			BoundingRegionWidth -= Owner->VerticalScrollbar->GetScrollZoneWidth();
		}

		// ApplyListFormatting treats DrawXL/DrawYL as width and height
		out_Parameters.DrawXL = BoundingRegionWidth;
		out_Parameters.DrawYL = BoundingRegionHeight;
	}

	return bFormattingConditionsMet;
}

/**
 * Adjusts the owning widget's bounds according to the autosize settings.
 */
void UUIComp_ListPresenter::UpdateOwnerBounds( FRenderParameters& Parameters )
{
	UUIList* Owner = GetOuterUUIList();

	// for convenience
	FLOAT* RenderBounds = Owner->RenderBounds;
	FUIScreenValue_Bounds& Position = Owner->Position;
	FUIDockingSet& DockTargets = Owner->DockTargets;

	// Now that we've formatted the list, adjust the bounds of the owning widget if applicable (i.e. if auto-sizing is enabled).
	// The order in which the widget's faces were resolved was determined by the logic in AddDockingNode, and GetListFormatParameters
	// should only return TRUE only after the faces which are NOT going to be adjusted here have all been resolved.

	// So, at this point the only faces that the RenderBounds of the widget should be out-of-date for are those faces which we are going
	// to adjust.  Therefore, we can use RenderBounds directly to determine offsets and such, rather than the [slower] GetPosition methods.
	const UBOOL bDocked[UIFACE_MAX] =
	{
		DockTargets.IsDocked(UIFACE_Left), DockTargets.IsDocked(UIFACE_Top),
		DockTargets.IsDocked(UIFACE_Right), DockTargets.IsDocked(UIFACE_Bottom),
	};
	const UBOOL bLockedHorz = bDocked[UIFACE_Left] && bDocked[UIFACE_Right];
	const UBOOL bLockedVert = bDocked[UIFACE_Top] && bDocked[UIFACE_Bottom];

	const UBOOL bAutoSizeHorz = ShouldAdjustListBounds(UIORIENT_Horizontal);
	const UBOOL bAutoSizeVert = ShouldAdjustListBounds(UIORIENT_Vertical);

	if ( ShouldAdjustListBounds(UIORIENT_Vertical) )
	{
		FLOAT TargetHeight = 0.f;
		const INT NumRows = GetTotalRowCount() + ((Owner->CellLinkType != LINKED_Rows && ShouldRenderColumnHeaders()) ? 1 : 0);
		const FLOAT Spacing = Owner->CellLinkType != LINKED_Rows ? Owner->CellSpacing.GetValue(Owner) : 0.f;
		for ( INT CellIndex = 0; CellIndex < NumRows; CellIndex++ )
		{
			TargetHeight += Owner->GetRowHeight(CellIndex);
			if ( CellIndex < NumRows - 1 )
			{
				TargetHeight += Spacing;
			}
		}

		// if the widget is docked on both faces, no adjustments can be made
		if ( !bDocked[UIFACE_Top] || !bDocked[UIFACE_Bottom] )
		{
			if ( !bDocked[UIFACE_Bottom] )
			{
				// If the widget is not docked or is docked only on its top face, then the bottom face of the widget needs to be adjusted
				Owner->Position.SetPositionValue(Owner, TargetHeight, UIFACE_Bottom, EVALPOS_PixelOwner, FALSE);
				Owner->RenderBounds[UIFACE_Bottom] = Owner->RenderBounds[UIFACE_Top] + TargetHeight;
				Owner->Position.ValidatePosition(UIFACE_Bottom);
				Owner->DockTargets.bResolved[UIFACE_Bottom] = TRUE;
			}
			else
			{
				// otherwise, if the widget is docked only on its bottom face, then the top face needs to be adjusted.
				const FLOAT NewPositionY = Owner->RenderBounds[UIFACE_Bottom] - TargetHeight;

				Owner->Position.SetPositionValue(Owner, NewPositionY, UIFACE_Top, EVALPOS_PixelViewport, FALSE);
				Owner->RenderBounds[UIFACE_Top] = NewPositionY;
				Owner->Position.ValidatePosition(UIFACE_Top);
				Owner->DockTargets.bResolved[UIFACE_Top] = TRUE;
			}
		}
	}

	if ( ShouldAdjustListBounds(UIORIENT_Horizontal) )
	{
		FLOAT TargetWidth = 0.f;

		const INT NumCols = GetTotalColumnCount() + ((Owner->CellLinkType != LINKED_Columns && ShouldRenderColumnHeaders()) ? 1 : 0);
		const FLOAT Spacing = Owner->CellLinkType != LINKED_Columns ? Owner->CellSpacing.GetValue(Owner) : 0.f;
		for ( INT CellIndex = 0; CellIndex < NumCols; CellIndex++ )
		{
			TargetWidth += Owner->GetColumnWidth(CellIndex);
			if ( CellIndex < NumCols - 1 )
			{
				TargetWidth += Spacing;
			}
		}

		// if the widget is docked on both faces, no adjustments can be made
		if ( !bDocked[UIFACE_Left] || !bDocked[UIFACE_Right] )
		{
			if ( !bDocked[UIFACE_Right] )
			{
				// If the widget is not docked or is docked only on its left face, then the right face of the widget needs to be adjusted
				Owner->Position.SetPositionValue(Owner, TargetWidth, UIFACE_Right, EVALPOS_PixelOwner, FALSE);
				Owner->RenderBounds[UIFACE_Right] = Owner->RenderBounds[UIFACE_Left] + TargetWidth;
				Owner->Position.ValidatePosition(UIFACE_Right);
				Owner->DockTargets.bResolved[UIFACE_Right] = TRUE;
			}
			else
			{
				// otherwise, if the widget is docked only on its right face, then the left face needs to be adjusted.
				const FLOAT NewPositionX = Owner->RenderBounds[UIFACE_Right] - TargetWidth;
				Owner->Position.SetPositionValue(Owner, NewPositionX, UIFACE_Left, EVALPOS_PixelViewport, FALSE);
				Owner->RenderBounds[UIFACE_Left] = NewPositionX;
				Owner->Position.ValidatePosition(UIFACE_Left);
				Owner->DockTargets.bResolved[UIFACE_Left] = TRUE;
			}
		}
	}
}

/**
 * Calculates the maximum number of visible elements and calls ApplyElementFormatting for all elements.  Only called
 * once all faces of the owning list have been resolved into actual pixel values.
 *
 * @param	Parameters		@see UUIString::ApplyFormatting())
 */
void UUIComp_ListPresenter::ApplyListFormatting( FRenderParameters& Parameters )
{
	bReapplyFormatting = FALSE;
	UUIList* Owner = GetOuterUUIList();

	// we'll need to keep track of the original values in case the scrollbar's visiblity changes since we'll need to reformat the schema cells 
	const FLOAT OriginalDrawX=Parameters.DrawX;
	const FLOAT OriginalDrawY=Parameters.DrawY;
	FormatSchemaCells(Parameters);

	// update the maximum visible items
	CalculateVisibleElements(Parameters);

	// no sense in initializing and/or updating the scrollbar until we know how may elements are visible
	if ( Owner->bInitializeScrollbars )
	{
		const UBOOL bPreviouslyVisible = Owner->VerticalScrollbar != NULL && Owner->VerticalScrollbar->IsVisible();
		Owner->InitializeScrollbars();
		const UBOOL bCurrentlyVisible = Owner->VerticalScrollbar != NULL && Owner->VerticalScrollbar->IsVisible();

		// if the scrollbar's visibility changed, we'll need to reformat the schema ells as their current sizes are outdated
		// and depending on the list's options, the schema cell's sizes are used for other calculations later on
		if ( bPreviouslyVisible != bCurrentlyVisible )
		{
			Parameters.DrawX = OriginalDrawX;
			Parameters.DrawY = OriginalDrawY;
			FormatSchemaCells(Parameters);
		}
	}

	const INT NumItems = Owner->GetItemCount();
	for ( INT ListIndex = Owner->TopIndex; ListIndex < Owner->TopIndex + Owner->MaxVisibleItems; ListIndex++ )
	{
		INT ItemIndex = ListIndex;

		// make sure we don't attempt to update formatting for elements past the end of the list
		if ( !ListItems.IsValidIndex(ItemIndex) )
		{
			break;
		}

		ApplyElementFormatting(ItemIndex, Parameters);
	}
}

/**
 * Updates the formatting parameters for all cells of the specified element.
 *
 * @param	ElementIndex	the list element to apply formatting for.
 * @param	Parameters		@see UUIString::ApplyFormatting())
 */
void UUIComp_ListPresenter::ApplyElementFormatting( INT ElementIndex, FRenderParameters& Parameters )
{
	if ( ListItems.IsValidIndex(ElementIndex) )
	{
		ApplyCellFormatting(ConvertElementCellArray(ListItems(ElementIndex).Cells), Parameters);

		UUIList* Owner = GetOuterUUIList();
		if ( Owner->CellLinkType == LINKED_None )
		{
			const INT MaxColumns = GetMaxNumVisibleColumns();

			// if this is the last element in the row, move to the next row
			// (ElementIndex%MaxColumns will be zero for element 0, but we don't want to move to the next row
			// unless there is only 1 column)
			if ( ElementIndex % MaxColumns == 0 && (ElementIndex > 0 || MaxColumns == 1) )
			{
				Parameters.DrawX = Owner->RenderBounds[UIFACE_Left];
				Parameters.DrawY += Owner->GetRowHeight(ElementIndex);
				if ( ElementIndex < Owner->GetItemCount() - MaxColumns )
				{
					Parameters.DrawY += Owner->CellSpacing.GetValue(Owner);
				}
			}
			else
			{
				// otherwise, move to the next column
				Parameters.DrawX += Owner->GetColumnWidth(ElementIndex) + Owner->CellSpacing.GetValue(Owner);
			}
		}
	}
}
/**
 * Wrapper for applying formatting to the schema cells.
 */
void UUIComp_ListPresenter::FormatSchemaCells( FRenderParameters& Parameters )
{
	UUIList* OwnerList = GetOuterUUIList();
	const ECellLinkType CellLinkType = (ECellLinkType)OwnerList->CellLinkType;
	if ( CellLinkType != LINKED_None )
	{
		const FLOAT OriginalDrawX = Parameters.DrawX, OriginalDrawY = Parameters.DrawY;

		// apply formatting to the column header
		ApplyCellFormatting(ConvertElementCellArray(ElementSchema.Cells), Parameters);

		// now set the starting Position for each cell
		FLOAT CellPosition = CellLinkType == LINKED_Columns ? OriginalDrawX : OriginalDrawY;
		for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
		{
			FUIListElementCellTemplate& SchemaCell = ElementSchema.Cells(CellIndex);
			SchemaCell.CellPosition = CellPosition;
			CellPosition += (CellLinkType == LINKED_Columns ? OwnerList->GetColumnWidth(CellIndex) : OwnerList->GetRowHeight(CellIndex));
		}

		// ApplyCellFormatting will increment the drawing position by the width or height of the cells it formatted,
		// but if we aren't rendering the column headers, we don't want this.
		if ( !ShouldRenderColumnHeaders() )
		{
			Parameters.DrawX = OriginalDrawX;
			Parameters.DrawY = OriginalDrawY;
		}
	}
}

/**
 * Updates the formatting parameters for all cells of the specified element.
 *
 * @param	Cells			the list of cells to render
 * @param	Parameters		@see UUIString::ApplyFormatting())
 */
void UUIComp_ListPresenter::ApplyCellFormatting( TArray<FUIListElementCell*> Cells, FRenderParameters& Parameters )
{
	UUIList* Owner = GetOuterUUIList();

	const FLOAT ClipX = Parameters.DrawX + Parameters.DrawXL;
	const FLOAT ClipY = Parameters.DrawY + Parameters.DrawYL;

	if ( Cells.Num() > 0 )
	{
		// intentionally a copy
		FRenderParameters CellParameters = Parameters;

		FLOAT CellWidth=0.f, CellHeight=0.f;
		const FLOAT CellPadding = Owner->CellPadding.GetValue(Owner);

		// for each element cell
		for ( INT CellIndex = 0; CellIndex < Cells.Num(); CellIndex++ )
		{
			FUIListElementCell& ElementCell = *Cells(CellIndex);
			CellWidth = Owner->GetColumnWidth(CellIndex);
			CellHeight = Owner->GetRowHeight(CellIndex);

			CellParameters.DrawXL = Min(CellWidth, ClipX - CellParameters.DrawX);
			CellParameters.DrawYL = CellHeight;

			FLOAT NextDrawX, NextDrawY;
			if ( Owner->CellLinkType == LINKED_Columns )
			{
				// check to see whether we have enough room to render the next element
				if ( ClipX - CellParameters.DrawX < appFloor(CellWidth) )
				{
					if (ElementCell.ValueString == NULL

					// if the cell's string is configured to clip, it's ok to render it
					||	ElementCell.ValueString->StringStyleData.TextClipMode == CLIP_None
					||	ElementCell.ValueString->StringStyleData.TextClipMode == CLIP_Wrap )
					{
						// we ran out of room, stop here....
						break;
					}
				}

				NextDrawX = CellParameters.DrawX + CellWidth;
				NextDrawY = CellParameters.DrawY;
			}
			else if ( Owner->CellLinkType == LINKED_Rows )
			{
				// if the list doesn't have enough space to render this entire cell, don't render any of it; otherwise,
				// it would fall outside the bounds of the list.....unless the cell clips its own string...
				if ( ClipY - CellParameters.DrawY < appFloor(CellHeight) )
				{
					// reduce the cell's bounding region by the space available in the list
					CellParameters.DrawYL = ClipY - CellParameters.DrawY;
				}

				NextDrawX = CellParameters.DrawX;
				NextDrawY = CellParameters.DrawY + CellHeight;
			}
			else
			{
				NextDrawX = CellParameters.DrawX + CellWidth;
				NextDrawY = CellParameters.DrawY + CellHeight;
			}

			if ( ElementCell.ValueString != NULL )
			{
				// apply any CellPadding to the area used for rendering the string
				CellParameters.DrawX += CellPadding * 0.5f;
				CellParameters.DrawY += CellPadding * 0.5f;
				CellParameters.DrawXL -= CellPadding;
				CellParameters.DrawYL -= CellPadding;

				ElementCell.ValueString->ApplyFormatting(CellParameters, FALSE);
			}

			if ( Owner->CellLinkType == LINKED_Columns )
			{
				// ApplyFormatting will set CellParameters.DrawX to the size of the string rendered in this cell
				// but this is not the location we'll start rendering the next cell so fix that up now
				CellParameters.DrawX = NextDrawX;

				// UUIString::ApplyFormatting modifies the value of DrawY, so we'll need to restore it.
				CellParameters.DrawY = Parameters.DrawY;

				if ( CellParameters.DrawX == ClipX || appFloor(CellParameters.DrawX) > ClipX )
				{
					// none of this cell falls inside the bounding region
					break;
				}
			}
			else if ( Owner->CellLinkType == LINKED_Rows )
			{
				// ApplyFormatting will set CellParameters.DrawX to the size of the string rendered in this cell
				// but this is not the location we'll start rendering the next cell so fix that up now
				CellParameters.DrawX = Parameters.DrawX;

				// UUIString::ApplyFormatting modifies the value of DrawY, so we'll need to restore it.
				CellParameters.DrawY = NextDrawY;

				if ( CellParameters.DrawY == ClipY || appFloor(CellParameters.DrawY) > ClipY )
				{
					// none of this cell falls inside the bounding region
					break;
				}
			}
		}

		// now advance the draw location appropriately
		switch( Owner->CellLinkType )
		{
		case LINKED_Rows:
			Parameters.DrawX += CellWidth + Owner->CellSpacing.GetValue(Owner);
			break;

		case LINKED_Columns:
			Parameters.DrawY += CellHeight + Owner->CellSpacing.GetValue(Owner);
			break;
		}
	}
	else
	{
		// @todo - should we decrease Parameters.DrawYL by Owner->CellHeight, creating a blank line?  probably not...
	}
}

/**
 * Renders the visible elements in this list
 *
 * @param	RI		the render interface to use for rendering the elements
 */
void UUIComp_ListPresenter::Render_List( FCanvas* Canvas )
{
	// get a reference to the owner list
	UUIList* Owner = GetOuterUUIList();
	const FLOAT* RenderBounds = Owner->RenderBounds;

	FRenderParameters Parameters;
	Parameters.DrawX = RenderBounds[UIFACE_Left];
	Parameters.DrawY = RenderBounds[UIFACE_Top];
	Parameters.DrawXL = RenderBounds[UIFACE_Right];
	Parameters.DrawYL = RenderBounds[UIFACE_Bottom];
	if ( ShouldRenderColumnHeaders() )
	{
		// intentionally a copy
		FRenderParameters ColumnHeaderParameters = Parameters;
		switch ( Owner->CellLinkType )
		{
		case LINKED_None:
			ColumnHeaderParameters.DrawXL = Parameters.DrawX + Owner->GetColumnWidth();
			ColumnHeaderParameters.DrawYL = Parameters.DrawY + Owner->GetRowHeight();
			break;

		case LINKED_Rows:
			ColumnHeaderParameters.DrawXL = Parameters.DrawX + Owner->GetColumnWidth();
			break;

		case LINKED_Columns:
			ColumnHeaderParameters.DrawYL = Parameters.DrawY + Owner->GetRowHeight();
			break;
		}

		// render the column header (this only works for 
		Render_Cells(Canvas, ConvertElementCellArray(ElementSchema.Cells), ColumnHeaderParameters);
		Parameters.DrawY += ColumnHeaderParameters.DrawYL;
	}

	//Horizontal extent must take into account the list's vertical scrollbar width if it is visible
	if ( Owner->VerticalScrollbar != NULL && Owner->VerticalScrollbar->IsVisible() )
	{
		Parameters.DrawXL -= Owner->VerticalScrollbar->GetScrollZoneWidth();
	}

	const INT NumItems = Owner->GetItemCount();
	if ( NumItems > 0 && Owner->MaxVisibleItems > 0 )
	{
		for ( INT ListIndex = Owner->TopIndex; ListIndex < Owner->TopIndex + Owner->MaxVisibleItems; ListIndex++ )
		{
			INT ItemIndex = ListIndex;
			if ( Owner->WrapType == LISTWRAP_Smooth )
			{
				ItemIndex = ListIndex % NumItems;
			}

			// make sure we don't attempt to render elements past the end of the list
			if ( !ListItems.IsValidIndex(ItemIndex) )
			{
				break;
			}

			Render_ListElement(Canvas, ItemIndex, Parameters);
		}
	}
}

/**
 * Renders the list element specified.
 *
 * @param	Canvas			the canvas to use for rendering
 * @param	ElementIndex	the index for the list element to render
 * @param	Parameters		Used for various purposes:
 *							DrawX:		[in]	specifies the pixel location of the start of the horizontal bounding region that should be used for
 *												rendering this element
 *										[out]	unused
 *							DrawY:		[in]	specifies the pixel Y location of the bounding region that should be used for rendering this list element.
 *										[out]	Will be set to the Y position of the rendering "pen" after rendering this element.  This is the Y position for rendering
 *												the next element should be rendered
 *							DrawXL:		[in]	specifies the pixel location of the end of the horizontal bounding region that should be used for rendering this element.
 *										[out]	unused
 *							DrawYL:		[in]	specifies the height of the bounding region, in pixels.  If this value is not large enough to render the specified element,
 *												the element will not be rendered.
 *										[out]	Will be reduced by the height of the element that was rendered. Thus represents the "remaining" height available for rendering.
 *							DrawFont:	[in]	specifies the font to use for retrieving the size of the characters in the string
 *							Scale:		[in]	specifies the amount of scaling to apply when rendering the element
 */
void UUIComp_ListPresenter::Render_ListElement( FCanvas* Canvas, INT ElementIndex, FRenderParameters& Parameters )
{
	if ( ListItems.IsValidIndex(ElementIndex) )
	{
		UUIList* Owner = GetOuterUUIList();

		// intentionally a copy
		FRenderParameters CellParameters = Parameters;

		// calculate the dimensions that will apply to all cells
		const FLOAT DefaultCellWidth = Owner->GetColumnWidth(ElementIndex);
		const FLOAT DefaultCellHeight = Owner->GetRowHeight(ElementIndex);

		//@todo ronp - might need to do a more complex overlay rendering if we ever need to have e.g. a row selected but one cell in the row shouldn't have the overlay...
		// Render any overlays

		// look up the list of element cells associated with the specified list item
		TArray<FUIListElementCell>& Cells = ListItems(ElementIndex).Cells;
		if ( Cells.Num() > 0 )
		{
			EUIListElementState CellState = static_cast<EUIListElementState>(ListItems(ElementIndex).ElementState);

			// Render configured item overlay/background
			if ( ListItemOverlay[CellState] != NULL )
			{
				FRenderParameters OverlayParameters = Parameters;

				// calculate the dimensions to use for any outlines that need to be rendered for this element
				switch ( Owner->CellLinkType )
				{
				case LINKED_None:
					OverlayParameters.DrawXL = DefaultCellWidth;
					OverlayParameters.DrawYL = DefaultCellHeight;
					break;

				case LINKED_Rows:
					OverlayParameters.DrawXL = DefaultCellWidth;
					// UITexture::Render_Texture expects the YL to be a height value, not a coordinate
					OverlayParameters.DrawYL -= OverlayParameters.DrawY;
					break;

				case LINKED_Columns:
					// UITexture::Render_Texture expects the XL to be a width value, not a coordinate
					OverlayParameters.DrawXL -= OverlayParameters.DrawX;
					OverlayParameters.DrawYL = DefaultCellHeight;
					break;
				}

				OverlayParameters.DrawCoords = ListItemOverlayCoordinates[CellState];
				ListItemOverlay[CellState]->Render_Texture(Canvas, OverlayParameters);
			}
		}

		Render_Cells(Canvas, ConvertElementCellArray(Cells), CellParameters);

		// now advance the draw location appropriately
		switch( Owner->CellLinkType )
		{
		case LINKED_None:
			{
				const INT MaxColumns = GetMaxNumVisibleColumns();


				// if this is the last element in the row, move to the next row
				// (ElementIndex%MaxColumns will be zero for element 0, but we don't want to move to the next row
				// unless there is only 1 column)
				if ( ElementIndex % MaxColumns == 0 &&	(ElementIndex > 0 || MaxColumns == 1) )
				{
					Parameters.DrawX = Owner->RenderBounds[UIFACE_Left];
					Parameters.DrawY += CellParameters.DrawYL;
					if ( ElementIndex < Owner->GetItemCount() - MaxColumns )
					{
						Parameters.DrawY += Owner->CellSpacing.GetValue(Owner);
					}
				}
				else
				{
					Parameters.DrawX += CellParameters.DrawXL + Owner->CellSpacing.GetValue(Owner);
				}
			}
			break;

		case LINKED_Rows:
			Parameters.DrawX += CellParameters.DrawXL + Owner->CellSpacing.GetValue(Owner);
		    break;

		case LINKED_Columns:
			Parameters.DrawY += CellParameters.DrawYL + Owner->CellSpacing.GetValue(Owner);
		    break;
		}
	}
}


/**
 * Renders the list element cells specified.
 *
 * @param	Canvas			the canvas to use for rendering
 * @param	Cells			the list of cells to render
 * @param	Parameters		Used for various purposes:
 *							DrawX:		[in]	specifies the location of the start of the horizontal bounding region that should be used for
 *												rendering the cells, in absolute screen pixels
 *										[out]	unused
 *							DrawY:		[in]	specifies the location of the start of the vertical bounding region that should be used for rendering
 *												the cells, in absolute screen pixels
 *										[out]	Will be set to the Y position of the rendering "pen" after rendering all cells.
 *							DrawXL:		[in]	specifies the location of the end of the horizontal bounding region that should be used for rendering this element, in absolute screen pixels
 *										[out]	unused
 *							DrawYL:		[in]	specifies the height of the bounding region, in absolute screen pixels.  If this value is not large enough to render the cells, they will not be
 *												rendered
 *										[out]	Will be reduced by the height of the cells that were rendered. Thus represents the "remaining" height available for rendering.
 *							DrawFont:	[in]	specifies the font to use for retrieving the size of the characters in the string
 *							Scale:		[in]	specifies the amount of scaling to apply when rendering the cells
 */
void UUIComp_ListPresenter::Render_Cells( FCanvas* Canvas, const TArray<FUIListElementCell*> Cells, FRenderParameters& CellParameters )
{
	UUIList* Owner = GetOuterUUIList();

	const FLOAT ClipX = CellParameters.DrawXL;
	const FLOAT ClipY = CellParameters.DrawYL;
	const FLOAT CellPadding = Owner->CellPadding.GetValue(Owner);

	// for each element cell
	for ( INT CellIndex = 0; CellIndex < Cells.Num(); CellIndex++ )
	{
		const FUIListElementCell& ElementCell = *Cells(CellIndex);

		const FLOAT CellWidth = Owner->GetColumnWidth(CellIndex);
		const FLOAT CellHeight = Owner->GetRowHeight(CellIndex);

		CellParameters.DrawXL = CellWidth;
		CellParameters.DrawYL = CellHeight;

		FLOAT NextDrawX, NextDrawY;
		if ( Owner->CellLinkType == LINKED_Columns )
		{
			// if the list doesn't have enough space to render this entire cell, don't render any of it; otherwise,
			// it would fall outside the bounds of the list.....unless the cell clips its own string...
			if ( ClipX - CellParameters.DrawX < appFloor(CellWidth) )
			{
				// reduce the cell's bounding region by the space available in the list
				CellParameters.DrawXL = ClipX - CellParameters.DrawX;

				if (ElementCell.ValueString == NULL

				// if the cell's string is configured to clip, it's ok to render it
				||	ElementCell.ValueString->StringStyleData.TextClipMode == CLIP_None
				||	ElementCell.ValueString->StringStyleData.TextClipMode == CLIP_Wrap )
				{
					// otherwise, stop here
					break;
				}
			}

			NextDrawX = CellParameters.DrawX + CellWidth;
			NextDrawY = CellParameters.DrawY;
		}
		else if ( Owner->CellLinkType == LINKED_Rows )
		{
			// if the list doesn't have enough space to render this entire cell, don't render any of it; otherwise,
			// it would fall outside the bounds of the list.....unless the cell clips its own string...
			if ( ClipY - CellParameters.DrawY < appFloor(CellHeight) )
			{
				// reduce the cell's bounding region by the space available in the list
				CellParameters.DrawYL = ClipY - CellParameters.DrawY;
			}

			NextDrawX = CellParameters.DrawX;
			NextDrawY = CellParameters.DrawY + CellHeight;
		}
		else
		{
			NextDrawX = CellParameters.DrawX + CellWidth;
			NextDrawY = CellParameters.DrawY + CellHeight;
		}

		//@todo ronp - Use the UIListElement interface to allow the list element to modify the rendering parameters, or completely override the render

		if ( ElementCell.ValueString != NULL )
		{
			// apply any CellPadding to the area used for rendering the string
			CellParameters.DrawX += CellPadding * 0.5f;
			CellParameters.DrawY += CellPadding * 0.5f;
			CellParameters.DrawXL -= CellPadding;
			CellParameters.DrawYL -= CellPadding;

			ElementCell.ValueString->Render_String(Canvas, CellParameters);
		}

		if ( Owner->bDebugShowBounds )
		{
			// render a box which displays the outline for this cell
			FVector2D StartLoc(CellParameters.DrawX, CellParameters.DrawY);
			FVector2D EndLoc(Min(CellParameters.DrawX + CellWidth, ClipX), Min(CellParameters.DrawY + CellHeight, ClipY));
			DrawBox2D(Canvas, StartLoc, EndLoc, FColor(0,255,255));	// draw an aqua box to show the bounds of this label
		}

		CellParameters.DrawX = NextDrawX;
		CellParameters.DrawY = NextDrawY;

		if ( Owner->CellLinkType == LINKED_Columns )
		{
			if ( CellParameters.DrawX == ClipX || appFloor(CellParameters.DrawX) > ClipX )
			{
				break;
			}
		}
		else if ( Owner->CellLinkType == LINKED_Rows )
		{
			if (CellParameters.DrawY == ClipY ||  appFloor(CellParameters.DrawY) > ClipY )
			{
				break;
			}
		}
	}
}

/**
 * Adds the specified face to the owning scene's DockingStack for the owning widget.  Takes list configuration into account,
 * ensuring that all widget faces are added to the scene's docking stack in the appropriate order.
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUIComp_ListPresenter::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	UBOOL bResult = TRUE;

	checkSlow(Face<UIFACE_MAX);

	UUIList* OwnerList = GetOuterUUIList();
	FUIDockingNode NewNode(OwnerList,Face);

	// determine if this node already exists in the docking stack
	INT StackPosition = DockingStack.FindItemIndex(NewNode);

	// if this node doesn't already exist
	if ( StackPosition == INDEX_NONE )
	{
		FUIDockingSet& DockTargets = OwnerList->DockTargets;
		if ( DockTargets.bLinking[Face] != 0 )
		{
			if ( GIsGame )
			{
				UUIScreenObject* DockTarget = DockTargets.GetDockTarget(Face);
				if ( DockTarget == NULL && DockTargets.IsDocked(Face) )
				{
					DockTarget = OwnerList->GetScene();
				}
				debugf(TEXT("Circular docking relationship detected!  Widget:%s    Face:%s    Target:%s    TargetFace:%s"),
					*OwnerList->GetWidgetPathName(),
					*UUIRoot::GetDockFaceText(Face),
					DockTarget ? *DockTarget->GetWidgetPathName() : TEXT("NULL"),
					*UUIRoot::GetDockFaceText(DockTargets.GetDockFace(Face))
					);

				OwnerList->GetScene()->eventLogDockingStack();
			}
			return FALSE;
		}

		DockTargets.bLinking[Face] = 1;

		//@todo - what if the target widget has been removed from the scene

		// if this face isn't docked to another widget,
		if ( DockTargets.GetDockTarget(Face) == NULL )
		{
			// if this face isn't docked to the scene
			if ( !DockTargets.IsDocked(Face) )
			{
				// add any intrinsic dependencies
				//@fixme - this is only necessary if our ScaleType for this face is not EVALPOS_PixelViewport
				if ( Face == UIFACE_Right )
				{
					// if this is the right face, the left face needs to be evaluated first
					bResult = AddDockingNode(DockingStack, UIFACE_Left) && bResult;
				}
				else if ( Face == UIFACE_Bottom )
				{
					// if this is the bottom face, the top face needs to be evaluated first
					bResult = AddDockingNode(DockingStack, UIFACE_Top) && bResult;
				}
				else
				{
					EUIWidgetFace OppositeFace = UUIRoot::GetOppositeFace(Face);
					if ( DockTargets.IsDocked(OppositeFace) )
					{
						bResult = AddDockingNode(DockingStack, OppositeFace) && bResult;
					}
				}
			}
		}
		else
		{
			// if there is a TargetWidget for this face, that widget's face must be evaluated
			// before this one, so that face must go into the DockingStack first
			bResult = DockTargets.GetDockTarget(Face)->AddDockingNode(DockingStack, DockTargets.GetDockFace(Face)) && bResult;
		}

		check(NewNode.Face != UIFACE_MAX);

		// add this face to the DockingStack
		new(DockingStack) FUIDockingNode(NewNode);
		DockTargets.bLinking[Face] = 0;
	}

	return bResult;
}

/* === UObject interface === */
/**
 * Called when a property value has been changed in the editor.  When the data source for the cell schema is changed,
 * refreshes the list's data.
 */
void UUIComp_ListPresenter::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	if ( PropertyThatChanged.Num() > 0 )
	{
		UProperty* OutermostProperty = PropertyThatChanged.GetActiveMemberNode()->GetValue();
		if ( OutermostProperty != NULL )
		{
			FName OuterPropertyName = OutermostProperty->GetFName();
			if ( OuterPropertyName == TEXT("ElementSchema") )
			{
				GetOuterUUIList()->RefreshListData(TRUE);
			}
			else if ( OuterPropertyName == TEXT("bDisplayColumnHeaders") )
			{
				ReapplyFormatting();
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Called when a member property value has been changed in the editor.
 */
void UUIComp_ListPresenter::PostEditChange( UProperty* PropertyThatChanged )
{
	if ( PropertyThatChanged != NULL )
	{
		FName PropertyName = PropertyThatChanged->GetFName();
		if ( PropertyName == TEXT("ListItemOverlay") )
		{
			UUIList* OwnerList = GetOuterUUIList();
			UUIState* CurrentState = OwnerList->GetCurrentState();

			// if the value of the ListItemOverlay image has been changed, make sure that it has a valid style
			// we don't know which element of the ListItemOverlay array has been changed, so just update them all
			for ( INT CellStateIndex = 0; CellStateIndex < ELEMENT_MAX; CellStateIndex++ )
			{
				if ( ListItemOverlay[CellStateIndex] != NULL )
				{
					UUIStyle_Image* OverlayStyle = Cast<UUIStyle_Image>(OwnerList->ItemOverlayStyle[CellStateIndex].GetStyleData(CurrentState));
					if ( OverlayStyle != NULL )
					{
						ListItemOverlay[CellStateIndex]->SetImageStyle(OverlayStyle);
					}
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

/**
 * Copies the value of the deprecated SelectionOverlay/Coordinates into the appropriate element of the ItemOverlay array.
 */
void UUIComp_ListPresenter::PostLoad()
{
	Super::PostLoad();

	if ( !GIsUCCMake )
	{
		// SelectionOverlay has been deprecated
		if ( SelectionOverlay != NULL )
		{
			ListItemOverlay[ELEMENT_Selected] = SelectionOverlay;
			ListItemOverlayCoordinates[ELEMENT_Selected] = SelectionOverlayCoordinates;
		}

		if ( GIsEditor && !GIsGame )
		{
			// fixup CellStyles that didn't have a RequiredStyleClass set (fixed around 08/09/06)
			for ( INT StateIndex = 0; StateIndex < ELEMENT_MAX; StateIndex++ )
			{
				for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
				{
					if ( ElementSchema.Cells(CellIndex).CellStyle[StateIndex].RequiredStyleClass == NULL )
					{
						ElementSchema.Cells(CellIndex).CellStyle[StateIndex].RequiredStyleClass = UUIStyle_Combo::StaticClass();
					}
				}
			}

			for ( INT ItemIndex = 0; ItemIndex < ListItems.Num(); ItemIndex++ )
			{
				FUIListItem& Item = ListItems(ItemIndex);
				for ( INT CellIndex = 0; CellIndex < Item.Cells.Num(); CellIndex++ )
				{
					for ( INT StateIndex = 0; StateIndex < ELEMENT_MAX; StateIndex++ )
					{
						if ( Item.Cells(CellIndex).CellStyle[StateIndex].RequiredStyleClass == NULL )
						{
							Item.Cells(CellIndex).CellStyle[StateIndex].RequiredStyleClass = UUIStyle_Combo::StaticClass();
						}
					}
				}
			}
		}
	}
}


/* ==========================================================================================================
	UUIComp_ContextMenuListPresenter
========================================================================================================== */
/**
 * Resolves the element schema provider based on the owning list's data source binding, and repopulates the element schema based on
 * the available data fields in that element schema provider.
 */
void UUIComp_ContextMenuListPresenter::RefreshElementSchema()
{
	// get the schema provider, which will probably be the default object for the class that is implementing this UIListElementProvider interface
	TScriptInterface<IUIListElementCellProvider> CellProvider = GetCellSchemaProvider();
	if ( CellProvider )
	{
		UUIContextMenu* ContextMenu = GetOuterUUIContextMenu();

		// retrieve the list of data field tags that this list element provider supports
		TMap<FName,FString> AvailableCellTags;
		CellProvider->GetElementCellTags(AvailableCellTags);
		if ( AvailableCellTags.Num() > 0 && AvailableCellTags.HasKey(TEXT("ContextMenuItems")) )
		{
			// replace the ContextMenuItems entry with the owning widget's ID so that the list items display
			// correctly
			AvailableCellTags.Remove(TEXT("ContextMenuItems"));
			AvailableCellTags.Set(ContextMenu->DataSource.DataStoreField, TEXT("ContextMenuItems"));
			if(!GIsGame)
			{
				// verify that the new schema provider contains the tags currently assigned to the schema cells
				for ( INT CellIndex = ElementSchema.Cells.Num() - 1; CellIndex >= 0; CellIndex-- )
				{
					FName& DataBinding = ElementSchema.Cells(CellIndex).CellDataField;
					if ( AvailableCellTags.Find(DataBinding) == NULL )
					{
						Modify();

						// the data field that this cell is currently bound to does not exist in the new schema provider, so remove it
						ElementSchema.Cells.Remove(CellIndex);
					}
				}
			}

			// if we currently don't have any cells in the schema, add them all
			if ( ElementSchema.Cells.Num() == 0 )
			{
				Modify();
				ElementSchema.Cells.Empty(AvailableCellTags.Num());
				ElementSchema.Cells.AddZeroed(AvailableCellTags.Num());

				TArray<FName> CellTags;
				TArray<FString> ColumnHeaders;
				AvailableCellTags.GenerateKeyArray(CellTags);
				AvailableCellTags.GenerateValueArray(ColumnHeaders);
				for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
				{
					FUIListElementCellTemplate& Cell = ElementSchema.Cells(CellIndex);
					Cell.CellDataField = CellTags(CellIndex);
					Cell.ColumnHeaderText = ColumnHeaders(CellIndex);
				}
			}

			for ( INT CellIndex = 0; CellIndex < ElementSchema.Cells.Num(); CellIndex++ )
			{
				FUIListElementCellTemplate& Cell = ElementSchema.Cells(CellIndex);

				Cell.OnCellCreated(ContextMenu);
				Cell.AssignBinding(CellProvider, Cell.CellDataField, Cell.ColumnHeaderText);
			}
		}
	}
	else if ( ElementSchema.Cells.Num() > 0 )
	{
		Modify();
		ElementSchema.Cells.Empty();
		ReapplyFormatting();
	}
}

/* ==========================================================================================================
	UUIComp_ListElementSorter
========================================================================================================== */
/** sorting classes */
namespace
{
	class FListElementComparison_ResetSorting
	{
	public:
		// Comparison method
		static inline INT Compare( const FUIListItem& A, const FUIListItem& B )
		{
			const INT IndexA = DataSourceIndices->FindItemIndex(B.DataSource.DataSourceIndex);
			const INT IndexB = DataSourceIndices->FindItemIndex(B.DataSource.DataSourceIndex);
			return IndexB - IndexA;
		}

		static TArray<INT>* DataSourceIndices;
	};

	class FListElementComparison_StandardSort
	{
	public:
		static FUIListSortingParameters* SortParameters;

		// Comparison method
		static inline INT Compare( const FUIListItem& A, const FUIListItem& B )
		{
			check(SortParameters);
			checkSlow(A.Cells.IsValidIndex(SortParameters->PrimaryIndex));
			checkSlow(A.Cells(SortParameters->PrimaryIndex).ValueString);
			checkSlow(B.Cells.IsValidIndex(SortParameters->PrimaryIndex));
			checkSlow(B.Cells(SortParameters->PrimaryIndex).ValueString);

			FString StringA = A.Cells(SortParameters->PrimaryIndex).ValueString->GetValue(TRUE);
			FString StringB = B.Cells(SortParameters->PrimaryIndex).ValueString->GetValue(TRUE);
			INT Result = SortParameters->bCaseSensitive ? appStrcmp(*StringA, *StringB) : appStricmp(*StringA, *StringB);
			if ( SortParameters->bReversePrimarySorting )
			{
				Result *= -1;
			}

			if ( Result == 0 && SortParameters->SecondaryIndex != INDEX_NONE )
			{
				checkSlow(A.Cells(SortParameters->SecondaryIndex).ValueString);
				checkSlow(B.Cells(SortParameters->SecondaryIndex).ValueString);

				StringA = A.Cells(SortParameters->SecondaryIndex).ValueString->GetValue(TRUE);
				StringB = B.Cells(SortParameters->SecondaryIndex).ValueString->GetValue(TRUE);
				Result = SortParameters->bCaseSensitive ? appStrcmp(*StringA, *StringB) : appStricmp(*StringA, *StringB);
				if ( SortParameters->bReverseSecondarySorting )
				{
					Result *= -1;
				}
			}

			return Result;
		}
	};

	// initialization
	TArray<INT>* FListElementComparison_ResetSorting::DataSourceIndices = NULL;
	FUIListSortingParameters* FListElementComparison_StandardSort::SortParameters = NULL;
}

/**
 * Resets the PrimarySortColumn and SecondarySortColumn to the Initial* values.
 *
 * @param	bResort	specify TRUE to re-sort the list's elements after resetting the sort columns.
 */
void UUIComp_ListElementSorter::ResetSortColumns( UBOOL bResort/*=TRUE*/ )
{
	UUIComp_ListPresenter* DataPresenter = GetOuterUUIList()->CellDataComponent;
	if ( DataPresenter )
	{
		if ( !DataPresenter->ElementSchema.Cells.IsValidIndex(InitialSortColumn) )
		{
			InitialSortColumn = INDEX_NONE;
		}
		if ( !DataPresenter->ElementSchema.Cells.IsValidIndex(InitialSecondarySortColumn) )
		{
			InitialSecondarySortColumn = INDEX_NONE;
		}
	}

	PrimarySortColumn = InitialSortColumn;
	SecondarySortColumn = InitialSecondarySortColumn;
	bReversePrimarySorting = bReverseSecondarySorting = FALSE;
	if ( bResort )
	{
		ResortItems();
	}
}

/**
 * Sorts the owning list's items.
 *
 * @param	ColumnIndex		the column (when CellLinkType is LINKED_Columns) or row (when CellLinkType is LINKED_Rows) to
 *							use for sorting.  Specify INDEX_NONE to clear sorting.
 * @param	bSecondarySort	specify TRUE to set ColumnIndex as the SecondarySortColumn.  If FALSE, resets the value of SecondarySortColumn
 * @param	bCaseSensitive	specify TRUE to perform case-sensitive comparison
 *
 * @return	TRUE if the items were sorted successfully.
 */
UBOOL UUIComp_ListElementSorter::SortItems( INT ColumnIndex, UBOOL bSecondarySort/*=FALSE*/, UBOOL bCaseSensitive/*=FALSE*/ )
{
	UBOOL bResult = FALSE;

	UUIList* OwnerList = GetOuterUUIList();
	UUIComp_ListPresenter* ListPresenter = OwnerList->CellDataComponent;
	if ( ListPresenter != NULL )
	{
		UBOOL bResetSorting = FALSE;

		if ( !bAllowCompoundSorting )
		{
			// don't allow the user to assign secondary sort columns if compound sorting is disabled
			bSecondarySort = FALSE;
		}

		// if an invalid sort column was specified
		if ( !ListPresenter->ElementSchema.Cells.IsValidIndex(ColumnIndex) )
		{
			// if bSecondarySort is TRUE but we have a valid PrimarySortColumn, just clear the SecondarySortColumn and resort
			if ( bSecondarySort && PrimarySortColumn != INDEX_NONE )
			{
				if ( SecondarySortColumn != INDEX_NONE )
				{
					SecondarySortColumn = INDEX_NONE;
				}
				else
				{
					// SecondarySortColumn was already cleared - no need to do anything
					bResult = TRUE;
				}
			}
			else
			{
				// either we don't have a valid PrimarySortColumn or bSecondarySort was false, so reset the sorting
				bResetSorting = TRUE;
			}
		}

		if ( !bResetSorting )
		{
			if ( !bResult )
			{
				// must already have a valid PrimarySortColumn in order to set a secondary sort column
				if ( bSecondarySort && PrimarySortColumn != INDEX_NONE )
				{
					if ( ColumnIndex == SecondarySortColumn )
					{
						bReverseSecondarySorting = !bReverseSecondarySorting;
					}
					else
					{
						SecondarySortColumn = ColumnIndex;
						bReverseSecondarySorting = FALSE;
					}
				}
				else
				{
					if ( PrimarySortColumn == ColumnIndex )
					{
						bReversePrimarySorting = !bReversePrimarySorting;
					}
					else
					{
						PrimarySortColumn = ColumnIndex;
						bReversePrimarySorting = FALSE;
					}
					SecondarySortColumn = INDEX_NONE;
					bReverseSecondarySorting = FALSE;
				}

				if ( OwnerList->GetItemCount() > 0 && ListPresenter->ElementSchema.Cells.IsValidIndex(PrimarySortColumn) )
				{
					// first, provide the list element provider with a chance to perform custom sorting
					FUIListSortingParameters SortParams(PrimarySortColumn, SecondarySortColumn, bReversePrimarySorting, bReverseSecondarySorting, bCaseSensitive);
					if ( OwnerList->DataProvider && OwnerList->DataProvider->SortListElements(OwnerList->DataSource.DataStoreField,
						(TArray<const FUIListItem>&)ListPresenter->ListItems, SortParams) )
					{
						bResult = TRUE;
					}
					else
					{
						// default sorting behavior - alphabetically sort the values of the desired cells
						{
							FListElementComparison_StandardSort::SortParameters = &SortParams;
							Sort<FUIListItem,FListElementComparison_StandardSort>(&ListPresenter->ListItems(0), ListPresenter->ListItems.Num());
							FListElementComparison_StandardSort::SortParameters = NULL;
						}

						// notify the list
						OwnerList->NotifyListElementsSorted();
						bResult = TRUE;
					}
				}
			}
		}
		else
		{
			// if the user configured an initial sort column, just reset the sort column to it and perform normal sorting
			if ( ListPresenter->ElementSchema.Cells.IsValidIndex(InitialSortColumn) )
			{
				ResetSortColumns(TRUE);
				bResult = TRUE;
			}
			else
			{
				// otherwise, remove all sorting and make the element order match the order that they came from the data provider.
				ResetSortColumns(FALSE);
				if ( OwnerList->DataProvider )
				{
					//Sort<USE_COMPARE_CONSTREF(FSubItem,UnObj)>( &Objects(0), Objects.Num() );
					TArray<INT> ListElements;
					if ( OwnerList->DataProvider->GetListElements(OwnerList->DataSource.DataStoreField, ListElements) )
					{
						{
							FListElementComparison_ResetSorting::DataSourceIndices = NULL;
							Sort<FUIListItem,FListElementComparison_ResetSorting>(&ListPresenter->ListItems(0), ListPresenter->ListItems.Num());
							FListElementComparison_ResetSorting::DataSourceIndices = NULL;
						}

						// notify the list
						OwnerList->NotifyListElementsSorted();
						bResult = TRUE;
					}
				}
			}
		}
	}

	return bResult;
}


/**
 * Sorts the owning list's items without modifying any sorting parameters.
 *
 * @param	bCaseSensitive	specify TRUE to perform case-sensitive comparison
 *
 * @return	TRUE if the items were sorted successfully.
 */
UBOOL UUIComp_ListElementSorter::ResortItems( UBOOL bCaseSensitive/*=FALSE*/ )
{
	UBOOL bResult = FALSE;

	UUIList* OwnerList = GetOuterUUIList();
	UUIComp_ListPresenter* ListPresenter = OwnerList->CellDataComponent;
	if ( OwnerList->GetItemCount() > 0 && ListPresenter != NULL && ListPresenter->ElementSchema.Cells.IsValidIndex(PrimarySortColumn) )
	{

		// first, provide the list element provider with a chance to perform custom sorting
		FUIListSortingParameters SortParams(PrimarySortColumn, SecondarySortColumn, bReversePrimarySorting, bReverseSecondarySorting, bCaseSensitive);
		if ( OwnerList->DataProvider && OwnerList->DataProvider->SortListElements(OwnerList->DataSource.DataStoreField,
			(TArray<const FUIListItem>&)ListPresenter->ListItems, SortParams) )
		{
			bResult = TRUE;
		}
		else
		{
			// default sorting behavior - alphabetically sort the values of the desired cells
			{
				FListElementComparison_StandardSort::SortParameters = &SortParams;
				Sort<FUIListItem,FListElementComparison_StandardSort>(&ListPresenter->ListItems(0), ListPresenter->ListItems.Num());
				FListElementComparison_StandardSort::SortParameters = NULL;
			}

			bResult = TRUE;
		}
	}

	if ( bResult )
	{
		// notify the list
		OwnerList->NotifyListElementsSorted();
	}

	return bResult;
}

/* ==========================================================================================================
	UUIComp_AutoAlignment
========================================================================================================== */

void UUIComp_AutoAlignment::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	if ( Ar.IsLoading() && Ar.Ver() < VER_REMOVED_REDUNDANT_ENUMS )
	{
		BYTE OldVertAlignment = Vertical;
		BYTE OldHorzAlignment = Horizontal;

		// old values originally used a different enum which placed the "none" value at the beginning of the enum list, so we'll need
		// to convert the value so that it matches the corresponding value in the standard ui alignment enum
		if ( OldVertAlignment == 0 )
		{
			OldVertAlignment = UIALIGN_MAX;
		}
		if ( OldHorzAlignment == 0 )
		{
			OldHorzAlignment = UIALIGN_MAX;
		}

		HorzAlignment = OldHorzAlignment - 1;
		VertAlignment = OldVertAlignment - 1;
	}
}


/**
 * Evaluates the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 * This ResolveFacePosition method takes into account the autoalignment setting of this component
 * and repositions the child widgets without modifying their underlying size.  If widget possesses any 
 * docking links on the faces which are supposed to be autoaligned, the docking links will be broken
 *
 * @param	Face	the face that should be resolved
 */
void UUIComp_AutoAlignment::ResolveFacePosition( EUIWidgetFace Face )
{
	//SCOPE_CYCLE_COUNTER(STAT_UIResolvePosition_AutoAlignment);
	// Auto-position the children widgets after right face has been evaluated, so we know our horizontal extent 
	UUIObject* OwnerWidget = GetOuterUUIObject();
	if( HorzAlignment != UIALIGN_Default
	&&	(OwnerWidget->HasPositionBeenResolved(UIFACE_Left) && OwnerWidget->HasPositionBeenResolved(UIFACE_Right)) )
	{
		AlignWidgetsHorizontally(OwnerWidget, (EUIAlignment)HorzAlignment);
	}

	//Auto-position the children widgets after bottom face has been evaluated, so we know our vertical extent
	if( VertAlignment != UIALIGN_Default
	&&	(OwnerWidget->HasPositionBeenResolved(UIFACE_Top) && OwnerWidget->HasPositionBeenResolved(UIFACE_Bottom)) )
	{
		AlignWidgetsVertically(OwnerWidget, (EUIAlignment)VertAlignment);		
	}
}

/**
 *	Updates the horizontal position of child widgets according to the specified alignment setting 
 *
 * @param	ContainerWidget			The widget to whose bounds the widgets will be aligned
 * @param	HorizontalAlignment		The horizontal alignment setting
 */
void UUIComp_AutoAlignment::AlignWidgetsHorizontally(UUIObject* ContainerWidget, EUIAlignment HorizontalAlignment)
{
	check(ContainerWidget);

	// Obtain value of left and right faces of the owner widget in absolute pixels
	const FLOAT ContainerLeftFace = ContainerWidget->GetPosition(UIFACE_Left,  EVALPOS_PixelViewport, TRUE );
	const FLOAT ContainerRightFace = ContainerWidget->GetPosition(UIFACE_Right,  EVALPOS_PixelViewport, TRUE );

	if(ContainerRightFace>ContainerLeftFace)
	{
		TArray<UUIObject*> ChildWidgets = ContainerWidget->GetChildren(FALSE);
		for ( INT ChildIndex = 0; ChildIndex < ChildWidgets.Num(); ChildIndex++ )
		{
			UUIObject* Child = ChildWidgets(ChildIndex);
			Child->Modify();

			// If any docking links exist break them now
			if(Child->DockTargets.IsDocked(UIFACE_Left))
			{
				Child->DockTargets.SetDockTarget(UIFACE_Left, NULL, UIFACE_MAX); 
			}
			if(Child->DockTargets.IsDocked(UIFACE_Right))
			{
				Child->DockTargets.SetDockTarget(UIFACE_Right, NULL, UIFACE_MAX); 
			}

			const FLOAT ChildLeftFace = Child->GetPosition(UIFACE_Left, EVALPOS_PixelViewport, TRUE);
			const FLOAT ChildRightFace = Child->GetPosition(UIFACE_Right, EVALPOS_PixelViewport, TRUE);
			FLOAT ChildNewLeftFace = ChildLeftFace;
			FLOAT ChildNewRightFace = ChildRightFace;

			if(ChildRightFace>ChildLeftFace)
			{
				if(HorizontalAlignment == UIALIGN_Left)
				{
					// Shift widgets position so its left face is the same as the owner container
					FLOAT ChildExtent = ChildRightFace - ChildLeftFace;

					ChildNewLeftFace = ContainerLeftFace;
					ChildNewRightFace = ContainerLeftFace + ChildExtent;
				}
				else if(HorizontalAlignment == UIALIGN_Center)
				{
					// Calculate the center point of the owner widget to which child widgets will be aligned
					FLOAT HorizontalCenter = ContainerLeftFace + (ContainerRightFace - ContainerLeftFace)/2;
					// Child widget's horizontal extent divided in half, value in absolute pixels
					FLOAT HalfExtent = (ChildRightFace - ChildLeftFace)/2;

					ChildNewLeftFace = HorizontalCenter - HalfExtent;
					ChildNewRightFace = HorizontalCenter + HalfExtent;
				}
				else if(HorizontalAlignment == UIALIGN_Right)
				{
					// Shift widgets position so its right face is the same as the owner container
					FLOAT ChildExtent = ChildRightFace - ChildLeftFace;

					ChildNewLeftFace = ContainerRightFace - ChildExtent;
					ChildNewRightFace = ContainerRightFace;
				}

				// Modify Child position while preserving its current size, the original scale type of the child face 
				// is not modified by this call, the absolute pixel value will be automatically converted into child face scale type
				Child->SetPosition(ChildNewLeftFace, static_cast<BYTE>(UIFACE_Left), EVALPOS_PixelViewport, TRUE);
				Child->SetPosition(ChildNewRightFace, static_cast<BYTE>(UIFACE_Right), EVALPOS_PixelViewport, TRUE);
			}
		}
	}
}


/**
 *	Updates the vertical position of child widgets according to the specified alignment setting 
 *
 * @param	ContainerWidget			The widget to whose bounds the widgets will be aligned
 * @param	VerticalAlignment		The vertical alignment setting
 */
void UUIComp_AutoAlignment::AlignWidgetsVertically(UUIObject* ContainerWidget, EUIAlignment VerticalAlignment)
{
	check(ContainerWidget);

	// Obtain value of top and bottom faces of the owner widget in absolute pixels
	const FLOAT ContainerTopFace = ContainerWidget->GetPosition(UIFACE_Top,  EVALPOS_PixelViewport, TRUE );
	const FLOAT ContainerBottomFace = ContainerWidget->GetPosition(UIFACE_Bottom,  EVALPOS_PixelViewport, TRUE );

	if(ContainerBottomFace>ContainerTopFace)
	{
		TArray<UUIObject*> ChildWidgets = ContainerWidget->GetChildren(FALSE);
		for ( INT ChildIndex = 0; ChildIndex < ChildWidgets.Num(); ChildIndex++ )
		{
			UUIObject* Child = ChildWidgets(ChildIndex);
			Child->Modify();

			// If any docking links exist break them now
			if(Child->DockTargets.IsDocked(UIFACE_Top))
			{
				Child->DockTargets.SetDockTarget(UIFACE_Top, NULL, UIFACE_MAX); 
			}
			if(Child->DockTargets.IsDocked(UIFACE_Bottom))
			{
				Child->DockTargets.SetDockTarget(UIFACE_Bottom, NULL, UIFACE_MAX); 
			}

			const FLOAT ChildTopFace = Child->GetPosition(UIFACE_Top, EVALPOS_PixelViewport, TRUE);
			const FLOAT ChildBottomFace = Child->GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport, TRUE);
			FLOAT ChildNewTopFace = ChildTopFace;
			FLOAT ChildNewBottomFace = ChildBottomFace;

			if(ChildBottomFace>ChildTopFace)
			{
				if(VerticalAlignment == UIALIGN_Left)
				{
					// Shift widgets position so its top face is the same as the owner container
					FLOAT ChildExtent = ChildBottomFace - ChildTopFace;

					ChildNewTopFace = ContainerTopFace;
					ChildNewBottomFace = ContainerTopFace + ChildExtent;
				}
				else if(VerticalAlignment == UIALIGN_Center)
				{
					// Calculate the center point of the owner widget to which child widgets will be aligned
					FLOAT VerticalCenter = ContainerTopFace + (ContainerBottomFace - ContainerTopFace)/2;
					// Child widget's vertical extent divided in half, value in absolute pixels
					FLOAT HalfExtent = (ChildBottomFace - ChildTopFace)/2;

					ChildNewTopFace = VerticalCenter - HalfExtent;
					ChildNewBottomFace = VerticalCenter + HalfExtent;
				}
				else if(VerticalAlignment == UIALIGN_Right)
				{
					// Shift widgets position so its bottom face is the same as the owner container
					FLOAT ChildExtent = ChildBottomFace - ChildTopFace;

					ChildNewTopFace = ContainerBottomFace - ChildExtent;
					ChildNewBottomFace = ContainerBottomFace;
				}

				// Modify Child position while preserving its current size, the original scale type of the child face 
				// is not modified by this call, the absolute pixel value will be automatically converted into child face scale type
				Child->SetPosition(ChildNewTopFace, static_cast<BYTE>(UIFACE_Top), EVALPOS_PixelViewport, TRUE);
				Child->SetPosition(ChildNewBottomFace, static_cast<BYTE>(UIFACE_Bottom), EVALPOS_PixelViewport, TRUE);
			}
		}
	}
}

/**
 * Called when a property value has been changed in the editor.
 */
void UUIComp_AutoAlignment::PostEditChange( UProperty* PropertyThatChanged )
{
	if ( PropertyThatChanged != NULL )
	{
		FName PropertyName = PropertyThatChanged->GetFName();
		if ( PropertyName == TEXT("VertAlignment") || PropertyName == TEXT("HorzAlignment") )
		{
			// make the scene build its docking stack
			GetOuterUUIObject()->RequestSceneUpdate(TRUE,TRUE);
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}
