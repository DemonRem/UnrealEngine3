/*=============================================================================
	UnObj.cpp: Unreal object manager.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

#if PS3
#include "../../PS3/Inc/FMallocPS3.h"
#endif

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Object manager internal variables.
UBOOL						UObject::GObjInitialized						= 0;
UBOOL						UObject::GObjNoRegister							= 0;
INT							UObject::GObjBeginLoadCount						= 0;
INT							UObject::GObjRegisterCount						= 0;
INT							UObject::GImportCount							= 0;
/** Forced exports for EndLoad optimization.											*/
INT							UObject::GForcedExportCount						= 0;
UObject*					UObject::GAutoRegister							= NULL;
UPackage*					UObject::GObjTransientPkg						= NULL;
TCHAR						UObject::GObjCachedLanguage[32]					= TEXT("");
TCHAR						UObject::GLanguage[64]							= TEXT("int");
UObject*					UObject::GObjHash[OBJECT_HASH_BINS];
UObject*					UObject::GObjHashOuter[OBJECT_HASH_BINS];
TArray<UObject*>			UObject::GObjLoaded;
/** Objects that have been constructed during async loading phase.						*/
TArray<UObject*>			UObject::GObjConstructedDuringAsyncLoading;
TArray<UObject*>			UObject::GObjObjects;
TArray<INT>					UObject::GObjAvailable;
TArray<ULinkerLoad*>		UObject::GObjLoaders;
TArray<UObject*>			UObject::GObjRegistrants;
TArray<FAsyncPackage>		UObject::GObjAsyncPackages;
/** Whether incremental object purge is in progress										*/
UBOOL						UObject::GObjIncrementalPurgeIsInProgress		= FALSE;
/** Whether FinishDestroy has already been routed to all unreachable objects. */
UBOOL						UObject::GObjFinishDestroyHasBeenRoutedToAllObjects	= FALSE;
/** Whether we need to purge objects or not.											*/
UBOOL						UObject::GObjPurgeIsRequired					= FALSE;
/** Current object index for incremental purge.											*/
INT							UObject::GObjCurrentPurgeObjectIndex			= 0;
/** First index into objects array taken into account for GC.							*/
INT							UObject::GObjFirstGCIndex						= 0;
/** Index pointing to last object created in range disregarded for GC.					*/
INT							UObject::GObjLastNonGCIndex						= INDEX_NONE;
/** Size in bytes of pool for objects disregarded for GC.								*/
INT							UObject::GPermanentObjectPoolSize				= 0;
/** Begin of pool for objects disregarded for GC.										*/
BYTE*						UObject::GPermanentObjectPool					= NULL;
/** Current position in pool for objects disregarded for GC.							*/
BYTE*						UObject::GPermanentObjectPoolTail				= NULL;


DECLARE_CYCLE_STAT(TEXT("InitProperties"),STAT_InitProperties,STATGROUP_Object);
DECLARE_CYCLE_STAT(TEXT("ConstructObject"),STAT_ConstructObject,STATGROUP_Object);
DECLARE_CYCLE_STAT(TEXT("LoadConfig"),STAT_LoadConfig,STATGROUP_Object);
DECLARE_CYCLE_STAT(TEXT("LoadLocalized"),STAT_LoadLocalized,STATGROUP_Object);
DECLARE_CYCLE_STAT(TEXT("LoadObject"),STAT_LoadObject,STATGROUP_Object);
DECLARE_DWORD_COUNTER_STAT(TEXT("FindObject"),STAT_FindObject,STATGROUP_Object);
DECLARE_DWORD_COUNTER_STAT(TEXT("FindObjectFast"),STAT_FindObjectFast,STATGROUP_Object);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("NameTable Entries"),STAT_NameTableEntries,STATGROUP_Object);
DECLARE_MEMORY_STAT(TEXT("NameTable Memory Size"),STAT_NameTableMemorySize,STATGROUP_Object);

/*-----------------------------------------------------------------------------
	UObject constructors.
-----------------------------------------------------------------------------*/

UObject::UObject()
{}
UObject::UObject( const UObject& Src )
{
	check(&Src);
	if( Src.GetClass()!=GetClass() )
		appErrorf( TEXT("Attempt to copy-construct %s from %s"), *GetFullName(), *Src.GetFullName() );
}
UObject::UObject( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags )
:	Index			( INDEX_NONE											)
,	ObjectFlags		( InFlags | RF_Native | RF_RootSet | RF_DisregardForGC	)
,	HashNext		( NULL													)
,	HashOuterNext	( NULL													)
,	StateFrame		( NULL													)
,	_Linker			( NULL													)
,	_LinkerIndex	( (PTRINT)INDEX_NONE									)
,	Outer			( NULL													)
,	Name			( NAME_None												)
,	Class			( InClass												)
,	ObjectArchetype	( NULL													)
{
	// Make sure registration is allowed now.
	check(!GObjNoRegister);

	// Setup registration info, for processing now (if inited) or later (if starting up).
	check(sizeof(Outer       )>=sizeof(InPackageName));
	check(sizeof(_LinkerIndex)>=sizeof(GAutoRegister));
	check(sizeof(Name        )>=sizeof(DWORD        ));
	*(DWORD			*)&Name			= appPointerToDWORD((void*)InName);
	*(const TCHAR  **)&Outer        = InPackageName;
	*(UObject      **)&_LinkerIndex = GAutoRegister;
	GAutoRegister                   = this;

	// Call native registration from terminal constructor.
	if( GetInitialized() && GetClass()==StaticClass() )
		Register();
}
UObject::UObject( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags )
:	Index			( INDEX_NONE											)
,	ObjectFlags		( InFlags | RF_Native | RF_RootSet | RF_DisregardForGC	)
,	HashNext		( NULL													)
,	StateFrame		( NULL													)
,	_Linker			( NULL													)
,	_LinkerIndex	( (PTRINT)INDEX_NONE									)
,	Outer			( NULL													)
,	Name			( NAME_None												)
,	Class			( NULL													)
,	ObjectArchetype	( NULL													)
{
	// Setup registration info, for processing now (if inited) or later (if starting up).
	check(sizeof(Outer       )>=sizeof(InPackageName));
	check(sizeof(_LinkerIndex)>=sizeof(GAutoRegister));
	check(sizeof(Name        )>=sizeof(DWORD        ));
	*(DWORD			*)&Name			= appPointerToDWORD((void*)InName);
	*(const TCHAR  **)&Outer        = InPackageName;

	// If we are not initialized yet, auto register.
	if (!GObjInitialized)
	{
		*(UObject      **)&_LinkerIndex = GAutoRegister;
		GAutoRegister                   = this;
	}
}
UObject& UObject::operator=( const UObject& Src )
{
	check(&Src);
	if( Src.GetClass()!=GetClass() )
		appErrorf( TEXT("Attempt to assign %s from %s"), *GetFullName(), *Src.GetFullName() );
	return *this;
}

/*-----------------------------------------------------------------------------
	UObject class initializer.
-----------------------------------------------------------------------------*/

void UObject::StaticConstructor()
{
}

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UObject::InitializeIntrinsicPropertyValues()
{
}

/*-----------------------------------------------------------------------------
	UObject implementation.
-----------------------------------------------------------------------------*/

/**
 * @return		TRUE if the object is selected, FALSE otherwise.
 */
UBOOL UObject::IsSelected() const
{
	return HasAnyFlags(RF_EdSelected);
}

//
// Rename this object to a unique name.
//
UBOOL UObject::Rename( const TCHAR* InName, UObject* NewOuter, ERenameFlags Flags )
{
	// Check that we are not renaming a within object into an Outer of the wrong type.
	if( NewOuter && !NewOuter->IsA(GetClass()->ClassWithin) )	
	{
		appErrorf( TEXT("Cannot rename %s into Outer %s as it is not of type %s"), 
			*GetFullName(), 
			*NewOuter->GetFullName(), 
			*GetClass()->ClassWithin->GetName() );
	}

	// find an object with the same name and same class in the new outer
	if (InName)
	{
		UObject* ExistingObject = StaticFindObject(GetClass(), NewOuter, InName, TRUE);
		if (ExistingObject == this)
		{
			return TRUE;
		}
		else if (ExistingObject)
		{
			if (Flags & REN_Test)
			{
				return FALSE;
			}
			else
			{
				appErrorf(TEXT("Renaming an object (%s) on top of an existing object (%s) is not allowed"), *GetFullName(), *ExistingObject->GetFullName());
			}
		}
	}

	// if we are just testing, and there was no conflict, then return a success
	if (Flags & REN_Test)
	{
		return TRUE;
	}

	if (!(Flags & REN_ForceNoResetLoaders))
	{
		UObject::ResetLoaders( GetOuter() );
	}

	FName NewName = InName ? FName(InName) : MakeUniqueObjectName( NewOuter ? NewOuter : GetOuter(), GetClass() );

	// propagate any name changes
	GObjectPropagator->OnObjectRename(this, *NewName.ToString());

	UnhashObject();
	debugfSlow( TEXT("Renaming %s to %s"), *Name.ToString(), *NewName.ToString() );

	// Mark touched packages as dirty.
	if( Outer )
	{
		Outer->MarkPackageDirty();
	}

	if ( HasAnyFlags(RF_Public) )
	{
		const UBOOL bUniquePathChanged	= ((NewOuter != NULL && Outer != NewOuter) || (Name != NewName));
		const UBOOL bRootPackage		= GetClass() == UPackage::StaticClass() && GetOuter() == NULL;
		const UBOOL bRedirectionAllowed = !GIsGame;

		// We need to create a redirector if we changed the Outer or Name of an object that can be referenced from other packages
		// [i.e. has the RF_Public flag] so that references to this object are not broken.
		if ( bRootPackage == FALSE && bUniquePathChanged == TRUE && bRedirectionAllowed == TRUE )
	{
		// create a UObjectRedirector with the same name as the old object we are redirecting
		UObjectRedirector* Redir = (UObjectRedirector*)StaticConstructObject(UObjectRedirector::StaticClass(), Outer, Name, RF_Standalone | RF_Public);
		// point the redirector object to this object
		Redir->DestinationObject = this;

	}
	}

	if( NewOuter )
	{
		// objects that have been renamed cannot have refs to them serialized over the network, so clear NetIndex for this object and all objects inside it
		SetNetIndex(INDEX_NONE);
		GetOutermost()->ClearAllNetObjectsInside(this);

		NewOuter->MarkPackageDirty();

		// Replace outer.
		Outer = NewOuter;
	}
	Name = NewName;
	HashObject();

	PostRename();

	return TRUE;
}

//
// Shutdown after a critical error.
//
void UObject::ShutdownAfterError()
{
}

//
// Make sure the object is valid.
//
UBOOL UObject::IsValid()
{
	if( !this )
	{
		debugf( NAME_Warning, TEXT("NULL object") );
		return 0;
	}
	else if( !GObjObjects.IsValidIndex(GetIndex()) )
	{
		debugf( NAME_Warning, TEXT("Invalid object index %i"), GetIndex() );
		debugf( NAME_Warning, TEXT("This is: %s"), *GetFullName() );
		return 0;
	}
	else if( GObjObjects(GetIndex())==NULL )
	{
		debugf( NAME_Warning, TEXT("Empty slot") );
		debugf( NAME_Warning, TEXT("This is: %s"), *GetFullName() );
		return 0;
	}
	else if( GObjObjects(GetIndex())!=this )
	{
		debugf( NAME_Warning, TEXT("Other object in slot") );
		debugf( NAME_Warning, TEXT("This is: %s"), *GetFullName() );
		debugf( NAME_Warning, TEXT("Other is: %s"), *GObjObjects(GetIndex())->GetFullName() );
		return 0;
	}
	else return 1;
}

//
// Do any object-specific cleanup required
// immediately after loading an object, and immediately
// after any undo/redo.
//
void UObject::PostLoad()
{
	// Note that it has propagated.
	SetFlags( RF_DebugPostLoad );

	/*
	By this point, all default properties have been loaded from disk
	for this object's class and all of its parent classes.  It is now
	safe to import config and localized data for "special" objects:
	- per-object config objects (Class->ClassFlags & CLASS_PerObjectConfig)
	- subobjects/components (RF_PerObjectLocalized)
	*/
	if ( !GIsUCCMake )
	{
		if( GetClass()->HasAnyClassFlags(CLASS_PerObjectConfig) ||
			HasAnyFlags(RF_PerObjectLocalized) )
		{
			LoadConfig();
			LoadLocalized();
		}
	}
}

//
// Edit change notification.
//
void UObject::PreEditChange(UProperty* PropertyAboutToChange)
{
	Modify();
}

/**
 * Called when a property on this object has been modified externally
 *
 * @param PropertyThatChanged the property that was modified
 */
void UObject::PostEditChange(UProperty* PropertyThatChanged)
{
	//@todo Assert in game on consoles
	if (GIsEditor == 0 && GIsUCC == 0)
	{
		debugf( TEXT("DO NOT CALL PostEditChange() in game as it will be removed on consoles (%s)"), *GetName() );
	}

	// @todo: call this for auto-prop window updating
	GCallbackEvent->Send(CALLBACK_ObjectPropertyChanged, this);
}

/**
 * This alternate version of PreEditChange is called when properties inside structs are modified.  The property that was actually modified
 * is located at the tail of the list.  The head of the list of the UStructProperty member variable that contains the property that was modified.
 */
void UObject::PreEditChange( FEditPropertyChain& PropertyAboutToChange )
{
	if ( HasAnyFlags(RF_ClassDefaultObject|RF_ArchetypeObject) && PropertyAboutToChange.GetActiveMemberNode() == PropertyAboutToChange.GetHead() && !GIsGame)
	{
		// this object must now be included in the undo/redo buffer
		SetFlags(RF_Transactional);

		// If we have an active memory archive and we're modifying an archetype, save the
		// the property data for all instances and child classes of this archetype before the archetype's
		// property values are changed.  Once the archetype's values have been changed, we'll refresh each
		// instances values with the new values from the archetype, then reload the custom values for each
		// object that were stored in the memory archive (PostEditChange).
		if ( GMemoryArchive != NULL )
		{
			// first, get a list of all objects which will be affected by this change; 
			TArray<UObject*> Objects;
			GetArchetypeInstances(Objects);
			SaveInstancesIntoPropagationArchive(Objects);
		}
	}

	// now forward the notification to the UProperty* version of PreEditChange
	PreEditChange(PropertyAboutToChange.GetActiveNode()->GetValue());
}

/**
 * This alternate version of PostEditChange is called when properties inside structs are modified.  The property that was actually modified
 * is located at the tail of the list.  The head of the list of the UStructProperty member variable that contains the property that was modified.
 */
void UObject::PostEditChange( FEditPropertyChain& PropertyThatChanged )
{
	PostEditChange(PropertyThatChanged.GetActiveNode()->GetValue());

	if ( HasAnyFlags(RF_ClassDefaultObject|RF_ArchetypeObject) && PropertyThatChanged.GetActiveMemberNode() == PropertyThatChanged.GetHead() && !GIsGame )
	{
		// If we have an active memory archive and we're modifying an archetype class, reload
		// the property data for all instances and child classes of this archetype, then reimport
		// the property data for the modified properties of each object.
		if ( GMemoryArchive != NULL )
		{
			// first, get a list of all objects which will be affected by this change; 
			TArray<UObject*> Objects;
			GetArchetypeInstances(Objects);
			LoadInstancesFromPropagationArchive(Objects);
		}
	}
}

/**
 * Determines whether changes to archetypes of this class should be immediately propagated to instances of this
 * archetype.
 *
 * @param	PropagationManager	if specified, receives a pointer to the object which manages propagation for this archetype.
 *
 * @return	TRUE if this object's archetype propagation is managed by another object (usually the case when the object
 *			is part of a prefab or something); FALSE if this object is a standalone archetype.
 *
 */
UBOOL UObject::UsesManagedArchetypePropagation( UObject** PropagationManager/*=NULL*/ ) const
{
	return IsAPrefabArchetype(PropagationManager);
}

/**
 * Builds a list of objects which have this object in their archetype chain.
 *
 * @param	Instances	receives the list of objects which have this one in their archetype chain
 */
void UObject::GetArchetypeInstances( TArray<UObject*>& Instances )
{
	Instances.Empty();

	if ( HasAnyFlags(RF_ArchetypeObject|RF_ClassDefaultObject) )
	{
		// use an FObjectIterator because we need to evaluate CDOs as well.

		// if this object is the class default object, any object of the same class (or derived classes) could potentially be affected
		if ( !HasAnyFlags(RF_ArchetypeObject) )
		{
			for ( FObjectIterator It; It; ++It )
			{
				UObject* Obj = *It;

				// if this object is the correct type
				if ( Obj != this && Obj->IsA(GetClass()) )
				{
					Instances.AddItem(Obj);
				}
			}
		}
		else
		{
			// editing an archetype object - objects of child classes won't be affected
			for ( FObjectIterator It; It; ++It )
			{
				UObject* Obj = *It;

				// if this object is the correct type and its archetype is this object, add it to the list
				if ( Obj != this && Obj->IsA(GetClass()) && Obj->IsBasedOnArchetype(this) )
				{
					Instances.AddItem(Obj);
				}
			}
		}
	}
}

/**
 * Serializes all objects which have this object as their archetype into GMemoryArchive, then recursively calls this function
 * on each of those objects until the full list has been processed.
 * Called when a property value is about to be modified in an archetype object. 
 *
 * @param	AffectedObjects		the array of objects which have this object in their ObjectArchetype chain and will be affected by the change.
 *								Objects which have this object as their direct ObjectArchetype are removed from the list once they're processed.
 */
void UObject::SaveInstancesIntoPropagationArchive( TArray<UObject*>& AffectedObjects )
{
	check(GMemoryArchive || AffectedObjects.Num()==0);

	TArray<UObject*> Instances;

	for ( INT i = 0; i < AffectedObjects.Num(); i++ )
	{
		UObject* Obj = AffectedObjects(i);

		// in order to ensure that all objects are saved properly, only process the objects which have this object as their
		// ObjectArchetype since we are going to call Pre/PostEditChange on each object (which could potentially affect which data is serialized
		if ( Obj->GetArchetype() == this )
		{
			// add this object to the list that we're going to process
			Instances.AddItem(Obj);

			// remove this object from the input list so that when we pass the list to our instances they don't need to check those objects again.
			AffectedObjects.Remove(i--);
		}
	}

	for ( INT i = 0; i < Instances.Num(); i++ )
	{
		UObject* Obj = Instances(i);

		// this object must now be included in any undo/redo operations
		Obj->SetFlags(RF_Transactional);

		// This will call ClearComponents in the Actor case, so that we do not serialize more stuff than we need to.
		Obj->PreSerializeIntoPropagationArchive();

		// save the current property values for this object to the memory archive
		GMemoryArchive->SerializeObject(Obj);

		// notify the object that all changes are complete
		Obj->PostSerializeIntoPropagationArchive();

		// now recurse into this object, saving its instances
		//@todo ronp - should this be called *before* we call PostEditChange on Obj?
		Obj->SaveInstancesIntoPropagationArchive(AffectedObjects);
	}
}

/**
 * De-serializes all objects which have this object as their archetype from the GMemoryArchive, then recursively calls this function
 * on each of those objects until the full list has been processed.
 *
 * @param	AffectedObjects		the array of objects which have this object in their ObjectArchetype chain and will be affected by the change.
 *								Objects which have this object as their direct ObjectArchetype are removed from the list once they're processed.
 */
void UObject::LoadInstancesFromPropagationArchive( TArray<UObject*>& AffectedObjects )
{
	check(GMemoryArchive || AffectedObjects.Num()==0);
	TArray<UObject*> Instances;

	for ( INT i = 0; i < AffectedObjects.Num(); i++ )
	{
		UObject* Obj = AffectedObjects(i);

		// in order to ensure that all objects are re-initialized properly, only process the objects which have this object as their
		// ObjectArchetype
		if ( Obj->GetArchetype() == this )
		{
			// add this object to the list that we're going to process
			Instances.AddItem(Obj);

			// remove this object from the input list so that when we pass the list to our instances they don't need to check those objects again.
			AffectedObjects.Remove(i--);
		}
	}

	for ( INT i = 0; i < Instances.Num(); i++ )
	{
		UObject* Obj = Instances(i);

		// this object must now be included in any undo/redo operations
		Obj->SetFlags(RF_Transactional);

		// notify this object that it is about to be modified
		Obj->PreSerializeFromPropagationArchive();

		// refresh the object's property values
		GMemoryArchive->SerializeObject(Obj);

		// notify the object that all changes are complete
		Obj->PostSerializeFromPropagationArchive();

		// now recurse into this object, loading its instances
		//@todo ronp - should this be called *before* we call PostEditChange on Obj?
		Obj->LoadInstancesFromPropagationArchive(AffectedObjects);
	}
}

/**
 * Called just before a property in this object's archetype is to be modified, prior to serializing this object into
 * the archetype propagation archive.
 *
 * Allows objects to perform special cleanup or preparation before being serialized into an FArchetypePropagationArc
 * against its archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
 */
void UObject::PreSerializeIntoPropagationArchive()
{
	PreEditChange(NULL);
}

/**
 * Called just before a property in this object's archetype is to be modified, immediately after this object has been
 * serialized into the archetype propagation archive.
 *
 * Allows objects to perform special cleanup or preparation before being serialized into an FArchetypePropagationArc
 * against its archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
 */
void UObject::PostSerializeIntoPropagationArchive()
{
	PostEditChange(NULL);
}

/**
 * Called just after a property in this object's archetype is modified, prior to serializing this object from the archetype
 * propagation archive.
 *
 * Allows objects to perform reinitialization specific to being de-serialized from an FArchetypePropagationArc and
 * reinitialized against an archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
 */
void UObject::PreSerializeFromPropagationArchive()
{
	PreEditChange(NULL);
}

/**
 * Called just after a property in this object's archetype is modified, immediately after this object has been de-serialized
 * from the archetype propagation archive.
 *
 * Allows objects to perform reinitialization specific to being de-serialized from an FArchetypePropagationArc and
 * reinitialized against an archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
 */
void UObject::PostSerializeFromPropagationArchive()
{
	PostEditChange(NULL);
}

/** Called before applying a transaction to the object.  Default implementation simply calls PreEditChange. */
void UObject::PreEditUndo()
{
	PreEditChange(NULL);
}

/** Called after applying a transaction to the object.  Default implementation simply calls PostEditChange. */
void UObject::PostEditUndo()
{
	PostEditChange(NULL);
}

/**
 * Called before destroying the object.  This is called immediately upon deciding to destroy the object, to allow the object to begin an
 * asynchronous cleanup process.
 */
void UObject::BeginDestroy()
{
	// Unhash object, removing it from object hash so it cannot be found from now on.
	UnhashObject();

	// Remove from linker's export table.
	SetLinker( NULL, INDEX_NONE );

	// remove net mapping
	SetNetIndex(INDEX_NONE);

	// Sanity assertion to ensure ConditionalBeginDestroy is the only code calling us.
	if( !HasAnyFlags(RF_BeginDestroyed) )
	{
		appErrorf(
			TEXT("Trying to call UObject::BeginDestroy from outside of UObject::ConditionalBeginDestroy on object %s. Please fix up the calling code."),
			*GetName()
			);
	}

	// Set debug flag to ensure BeginDestroy has been routed back to UObject::BeginDestroy.
	SetFlags( RF_DebugBeginDestroyed );
}

/**
 * Called to finish destroying the object.  After UObject::FinishDestroy is called, the object's memory should no longer be accessed.
 *
 * note: because ExitProperties() is called here, Super::FinishDestroy() should always be called at the end of your child class's
 * FinishDestroy() method, rather than at the beginning.
 */
void UObject::FinishDestroy()
{
	if( !HasAnyFlags(RF_FinishDestroyed) )
	{
		appErrorf(
			TEXT("Trying to call UObject::FinishDestroy from outside of UObject::ConditionalFinishDestroy on object %s. Please fix up the calling code."),
			*GetName()
			);
	}

	check( _Linker == NULL );
	check( _LinkerIndex	== INDEX_NONE );

	SetFlags( RF_DebugFinishDestroyed );

	// Destroy properties.
	ExitProperties( (BYTE*)this, GetClass() );

	// Log message.
	if( GObjInitialized && !GIsCriticalError )
	{
		debugfSlow( NAME_DevKill, TEXT("Destroying %s"), *GetName() );
	}
}

/**
 * Changes the linker and linker index to the passed in one. A linker of NULL and linker index of INDEX_NONE
 * indicates that the object is without a linker.
 *
 * @param LinkerLoad	New LinkerLoad object to set
 * @param LinkerIndex	New LinkerIndex to set
 */
void UObject::SetLinker( ULinkerLoad* LinkerLoad, INT LinkerIndex )
{
	// Detach from existing linker.
	if( _Linker )
	{
		check(!HasAnyFlags(RF_NeedLoad|RF_NeedPostLoad));
		check(_Linker->ExportMap(_LinkerIndex)._Object!=NULL);
		check(_Linker->ExportMap(_LinkerIndex)._Object==this);
		_Linker->ExportMap(_LinkerIndex)._Object = NULL;
	}
	
	// Set new linker.
	_Linker      = LinkerLoad;
	_LinkerIndex = LinkerIndex;
}

/**
 * Returns the version of the linker for this object.
 *
 * @return	the version of the engine's package file when this object
 *			was last saved, or GPackageFileVersion (current version) if
 *			this object does not have a linker, which indicates that
 *			a) this object is a native only class,
 *			b) this object's linker has been detached, in which case it is already fully loaded
 */
INT UObject::GetLinkerVersion() const
{
	ULinkerLoad* Loader = _Linker;

	// No linker.
	if( Loader == NULL )
	{
		// the _Linker reference is never set for the top-most UPackage of a package (the linker root), so if this object
		// is the linker root, find our loader in the global list.
		if( GetOutermost() == this )
		{
			// Iterate over all loaders and try to find one that has this object as the linker root.
			for( INT i=0; i<GObjLoaders.Num(); i++ )
			{
				ULinkerLoad* LinkerLoad = GetLoader(i);
				// We found a match, return its version.
				if( LinkerLoad->LinkerRoot == this )
				{
					Loader = LinkerLoad;
					break;
				}
			}
		}
	}

	if ( Loader != NULL )
	{
		// We have a linker so we can return its version.
		return Loader->Ver();

	}
	else
	{
		// We don't have a linker associated as we e.g. might have been saved or had loaders reset, ...
		return GPackageFileVersion;
	}
}

/**
 * Returns the licensee version of the linker for this object.
 *
 * @return	the licensee version of the engine's package file when this object
 *			was last saved, or GPackageFileLicenseeVersion (current version) if
 *			this object does not have a linker, which indicates that
 *			a) this object is a native only class, or
 *			b) this object's linker has been detached, in which case it is already fully loaded
 */
INT UObject::GetLinkerLicenseeVersion() const
{
	ULinkerLoad* Loader = _Linker;

	// No linker.
	if( Loader == NULL )
	{
		// the _Linker reference is never set for the top-most UPackage of a package (the linker root), so if this object
		// is the linker root, find our loader in the global list.
		if( GetOutermost() == this )
		{
			// Iterate over all loaders and try to find one that has this object as the linker root.
			for( INT i=0; i<GObjLoaders.Num(); i++ )
			{
				ULinkerLoad* LinkerLoad = GetLoader(i);
				// We found a match, return its version.
				if( LinkerLoad->LinkerRoot == this )
				{
					Loader = LinkerLoad;
					break;
				}
			}
		}
	}

	if ( Loader != NULL )
	{
		// We have a linker so we can return its version.
		return Loader->LicenseeVer();

	}
	else
	{
		// We don't have a linker associated as we e.g. might have been saved or had loaders reset, ...
		return GPackageFileLicenseeVersion;
	}
}

/** sets the NetIndex associated with this object for network replication */
void UObject::SetNetIndex(INT InNetIndex)
{
	if (InNetIndex != NetIndex)
	{
		UPackage* Package = GetOutermost();
		// skip if package is not meant to be replicated
		if (!(Package->PackageFlags & PKG_ServerSideOnly))
		{
			if (NetIndex != INDEX_NONE)
			{
				// remove from old
				Package->RemoveNetObject(this);
			}
			NetIndex = InNetIndex;
			if (NetIndex != INDEX_NONE)
			{
				Package->AddNetObject(this);
			}
		}
	}
}

/**
 * Returns the fully qualified pathname for this object, in the format:
 * 'Outermost[.Outer].Name'
 *
 * @param	StopOuter	if specified, indicates that the output string should be relative to this object.  if StopOuter
 *						does not exist in this object's Outer chain, the result would be the same as passing NULL.
 *
 * @note	safe to call on NULL object pointers!
 */
FString UObject::GetPathName( const UObject* StopOuter/*=NULL*/ ) const
{
	FString Result;
	if( this!=StopOuter && this!=NULL )
	{
		if( Outer && Outer!=StopOuter )
		{
			Result += Outer->GetPathName( StopOuter ) + TEXT(".");
		}
		Result += GetName();
	}
	else
	{
		Result = TEXT("None");
	}
	return Result;
}

/**
 * Returns the fully qualified pathname for this object as well as the name of the class, in the format:
 * 'ClassName Outermost[.Outer].Name'.
 *
 * @param	StopOuter	if specified, indicates that the output string should be relative to this object.  if StopOuter
 *						does not exist in this object's Outer chain, the result would be the same as passing NULL.
 *
 * @note	safe to call on NULL object pointers!
 */
FString UObject::GetFullName( const UObject* StopOuter/*=NULL*/ ) const
{
	if( this )
	{
		return GetClass()->GetName() + TEXT(" ") + GetPathName( StopOuter );
	}
	else
	{
		return TEXT("None");
	}
}

/**
 * Walks up the chain of packages until it reaches the top level, which it ignores.
 *
 * @param	bStartWithOuter		whether to include this object's name in the returned string
 * @return	string containing the path name for this object, minus the outermost-package's name
 */
FString UObject::GetFullGroupName( UBOOL bStartWithOuter ) const
{
	const UObject* Obj = bStartWithOuter ? GetOuter() : this;
	return Obj ? Obj->GetPathName(GetOutermost()) : TEXT("");
}

/** 
 * Walks up the list of outers until it finds the highest one.
 *
 * @return outermost non NULL Outer.
 */
UPackage* UObject::GetOutermost() const
{
	UObject* Top;
	for( Top = (UObject*)this ; Top && Top->GetOuter() ; Top = Top->GetOuter() );
	return CastChecked<UPackage>(Top);
}

UBOOL UObject::ConditionalBeginDestroy()
{
	if( Index!=INDEX_NONE && !HasAnyFlags(RF_BeginDestroyed) )
	{
		SetFlags(RF_BeginDestroyed);
		ClearFlags(RF_DebugBeginDestroyed);
		BeginDestroy();
		if( !HasAnyFlags(RF_DebugBeginDestroyed) )
		{
			appErrorf( TEXT("%s failed to route BeginDestroy"), *GetFullName() );
		}
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}

UBOOL UObject::ConditionalFinishDestroy()
{
	if( Index!=INDEX_NONE && !HasAnyFlags(RF_FinishDestroyed) )
	{
		SetFlags(RF_FinishDestroyed);
		ClearFlags(RF_DebugFinishDestroyed);
		FinishDestroy();
		if( !HasAnyFlags(RF_DebugFinishDestroyed) )
		{
			appErrorf( TEXT("%s failed to route FinishDestroy"), *GetFullName() );
		}
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}

void UObject::ConditionalDestroy()
{
	// Check that the object hasn't been destroyed yet.
	if(!HasAnyFlags(RF_FinishDestroyed))
	{
		// Begin the asynchronous object cleanup.
		ConditionalBeginDestroy();

		// Wait for the object's asynchronous cleanup to finish.
		while(!IsReadyForFinishDestroy()) 
		{
			appSleep(0); // GEMINI_TODO: busy loop
		}; 

		// Finish destroying the object.
		ConditionalFinishDestroy();
	}
}

//
// Register if needed.
//
void UObject::ConditionalRegister()
{
	if( GetIndex()==INDEX_NONE )
	{
#if 0
		// Verify this object is on the list to register.
		INT i;
		for( i=0; i<GObjRegistrants.Num(); i++ )
			if( GObjRegistrants(i)==this )
				break;
		check(i!=GObjRegistrants.Num());
#endif

		// Register it.
		Register();
	}
}

//
// PostLoad if needed.
//
void UObject::ConditionalPostLoad()
{
	if( HasAnyFlags(RF_NeedPostLoad) )
	{
		check(GetLinker());

		ClearFlags( RF_NeedPostLoad | RF_DebugPostLoad );
		if ( ObjectArchetype != NULL )
		{
			//make sure our archetype executes ConditionalPostLoad first.
			ObjectArchetype->ConditionalPostLoad();
		}

		ConditionalPostLoadSubobjects();
		PostLoad();

		if( !HasAnyFlags(RF_DebugPostLoad) )
		{
			appErrorf( TEXT("%s failed to route PostLoad.  Please call Super::PostLoad() in your <className>::PostLoad() function. "), *GetFullName() );
		}
	}
}

/**
 * Instances components for objects being loaded from disk, if necessary.  Ensures that component references
 * between nested components are fixed up correctly.
 *
 * @param	OuterInstanceGraph	when calling this method on subobjects, specifies the instancing graph which contains all instanced
 *								subobjects and components for a subobject root.
 */
void UObject::ConditionalPostLoadSubobjects( FObjectInstancingGraph* OuterInstanceGraph/*=NULL*/ )
{
	// if this class contains instanced object properties and a new object property has been added since this object was saved,
	// this object won't receive its own unique instance of the object assigned to the new property, since we don't instance object during loading
	// so go over all instanced object properties and look for cases where the value for that property still matches the default value.
	if ( HasAnyFlags(RF_NeedPostLoadSubobjects) )
	{
		if ( IsTemplate(RF_ClassDefaultObject) )
		{
			// never instance and fixup subobject/components for CDOs and their subobjects - these are instanced during script compilation and are
			// serialized using shallow comparison (serialize if they're different objects), rather than deep comparison.  Therefore subobjects and components
			// inside of CDOs will always be loaded from disk and never need to be instanced at runtime.
			ClearFlags(RF_NeedPostLoadSubobjects);
			return;
		}

		// make sure our Outer has already called ConditionalPostLoadSubobjects
		if ( Outer != NULL && Outer->HasAnyFlags(RF_NeedPostLoadSubobjects) )
		{
			if ( Outer->HasAnyFlags(RF_NeedPostLoad) )
			{
				Outer->ConditionalPostLoad();
			}
			else
			{
				Outer->ConditionalPostLoadSubobjects();
			}
			if ( !HasAnyFlags(RF_NeedPostLoadSubobjects) )
			{
				// if calling ConditionalPostLoadSubobjects on our Outer resulted in ConditionalPostLoadSubobjects on this object, stop here
				return;
			}
		}

		// clear the flag so that we don't re-enter this method
		ClearFlags(RF_NeedPostLoadSubobjects);

		FObjectInstancingGraph CurrentInstanceGraph;
	
		FObjectInstancingGraph* InstanceGraph = OuterInstanceGraph;
		if ( InstanceGraph == NULL )
		{
			CurrentInstanceGraph.SetDestinationRoot(this,NULL);
			CurrentInstanceGraph.SetLoadingObject(TRUE);

			// if we weren't passed an instance graph to use, create a new one and use that
			InstanceGraph = &CurrentInstanceGraph;
		}

		if ( !GIsUCCMake )
		{
			// this will instance any subobjects that have been added to our archetype since this object was saved
			InstanceSubobjectTemplates(InstanceGraph);
		}

		if( Class->HasAnyClassFlags(CLASS_HasComponents) )
		{
			// this will be filled with the list of component instances which were serialized from disk
			TArray<UComponent*> SerializedComponents;
			// fill the array with the component contained by this object that were actually serialized to disk through property references
			CollectComponents(SerializedComponents, FALSE);

			if ( GetLinkerVersion() < VER_SINGLEPASS_COMPONENT_INSTANCING )
			{
				// InstancedComponents will not contain any components which were considered identical to their archetypes when this package was last saved;
				// that logic was buggy, so we have some components which weren't correctly serialized through a UComponentProperty reference, so we'll need to fix
				// those up
				const INT LinkerIdx = GetLinkerIndex();
				check(LinkerIdx >= 0 && LinkerIdx < GetLinker()->ExportMap.Num());
				const FObjectExport& Export = GetLinker()->ExportMap(LinkerIdx);

				for ( TMap<FName,INT>::TConstIterator It(Export.ComponentMap); It; ++It )
				{
					const INT ComponentInstanceExportMapIndex = It.Value();
					UComponent* Component = Cast<UComponent>(GetLinker()->CreateExport(ComponentInstanceExportMapIndex));
					if ( Component != NULL )
					{
						// Fix up components that aren't private.
						if ( !HasAnyFlags(RF_Public) && Component->HasAnyFlags(RF_Public) )
						{
							Component->ClearFlags( RF_Public );
							Component->MarkPackageDirty();
						}

						// add this component to the list of instanced components
						SerializedComponents.AddUniqueItem(Component);
					}
				}
			}

			// now, add all of the instanced components to the instance graph that will be used for instancing any components that have been added
			// to this object's archetype since this object was last saved
			for ( INT ComponentIndex = 0; ComponentIndex < SerializedComponents.Num(); ComponentIndex++ )
			{
				UComponent* PreviouslyInstancedComponent = SerializedComponents(ComponentIndex);
				InstanceGraph->AddComponentPair(Cast<UComponent>(PreviouslyInstancedComponent->GetArchetype()), PreviouslyInstancedComponent);
			}

			InstanceComponentTemplates(InstanceGraph);
		}
	}

}

//
// UObject destructor.
//warning: Called at shutdown.
//
UObject::~UObject()
{
	// Only in-place replace and garbage collection purge phase is allowed to delete UObjects.
	//@todo: enable again after linkers no longer throw from within constructor: check( GIsReplacingObject || GIsPurgingObject );

	// If not initialized, skip out.
	if( Index!=INDEX_NONE && GObjInitialized && !GIsCriticalError )
	{
		// Validate it.
		check(IsValid());

		// Destroy the object if necessary.
		ConditionalDestroy();

		GObjObjects(Index) = NULL;
		GObjAvailable.AddItem( Index );
	}

	// Free execution stack.
	if( StateFrame )
	{
		delete StateFrame;
	}
}

void UObject::operator delete( void* Object, size_t Size )
{
	check(GObjBeginLoadCount==0);
	appFree( Object );
}

/**
 * Note that the object has been modified.  If we are currently recording into the 
 * transaction buffer (undo/redo), save a copy of this object into the buffer and 
 * marks the package as needing to be saved.
 *
 * @param	bAlwaysMarkDirty	if TRUE, marks the package dirty even if we aren't
 *								currently recording an active undo/redo transaction
 */
void UObject::Modify( UBOOL bAlwaysMarkDirty/*=FALSE*/ )
{
	// PIE world objects should never end up in the transaction buffer as we cannot undo during gameplay.
	if ( (GetOutermost()->PackageFlags & PKG_PlayInEditor) == 0 )
	{
		// Perform transaction tracking if we have a transactor.
		if( GUndo 
		// Only operations on transactional objects can be undone.
		&&	HasAnyFlags(RF_Transactional) )
		{
			MarkPackageDirty();
			GUndo->SaveObject( this );
		}
		else if ( bAlwaysMarkDirty )
		{
			MarkPackageDirty();
		}
	}
}

/**
 * Serializes a pushed state, calculating the offset in the same manner as the normal
 * StateFrame->StateNode code offset is calculated in UObject::Serialize().
 */
FArchive& operator<<(FArchive& Ar,FStateFrame::FPushedState& PushedState)
{
	INT Offset = Ar.IsSaving() ? PushedState.Code - &PushedState.Node->Script(0) : INDEX_NONE;
	Ar << PushedState.State << PushedState.Node << Offset;
	if (Offset != INDEX_NONE)
	{
		PushedState.Code = &PushedState.Node->Script(Offset);
	}
	return Ar;
}

/** serializes NetIndex from the passed in archive; in a seperate function to share with default object serialization */
void UObject::SerializeNetIndex(FArchive& Ar)
{
	// do not serialize NetIndex when duplicating objects via serialization
	if (!(Ar.GetPortFlags() & PPF_Duplicate))
	{
		INT InNetIndex = NetIndex;
		if (Ar.Ver() >= VER_LINKERFREE_PACKAGEMAP)
		{
			Ar << InNetIndex;
			if( GUseSeekFreeLoading )
			{
				// use serialized net index for cooked packages
				SetNetIndex(InNetIndex);
			}
			// set net index from linker
			else if (_Linker != NULL && _LinkerIndex != INDEX_NONE)
			{
				SetNetIndex(_LinkerIndex);
			}
		}
		else if (_Linker != NULL && _LinkerIndex != INDEX_NONE)
		{
			SetNetIndex(_LinkerIndex);
		}
	}
}

//
// UObject serializer.
//
void UObject::Serialize( FArchive& Ar )
{
	SetFlags( RF_DebugSerialize );

	// Make sure this object's class's data is loaded.
	if( Class != UClass::StaticClass() )
	{
		Ar.Preload( Class );

		// if this object's class is intrinsic, its properties may not have been linked by this point, if a non-intrinsic
		// class has a reference to an intrinsic class in its script or defaultproperties....so make sure the class is linked
		// before any data is loaded
		if ( Ar.IsLoading() )
		{
			Class->ConditionalLink();
		}

		// make sure this object's template data is loaded - the only objects
		// this should actually affect are those that don't have any defaults
		// to serialize.  for objects with defaults that actually require loading
		// the class default object should be serialized in ULinkerLoad::Preload, before
		// we've hit this code.
		if ( !HasAnyFlags(RF_ClassDefaultObject) && Class->GetDefaultsCount() > 0 )
		{
			Ar.Preload(Class->GetDefaultObject());
		}
	}

	// Special info.
	if( (!Ar.IsLoading() && !Ar.IsSaving()) )
	{
		Ar << Name;
		// We don't want to have the following potentially be clobbered by GC code.
		Ar.AllowEliminatingReferences( FALSE );
		if(!Ar.IsIgnoringOuterRef())
		{
			Ar << Outer;
		}
		Ar.AllowEliminatingReferences( TRUE );
		if ( !Ar.IsIgnoringClassRef() )
		{
			Ar << Class;
		}
		Ar << _Linker;
		if( !Ar.IsIgnoringArchetypeRef() )
		{
			Ar.AllowEliminatingReferences(FALSE);
			Ar << ObjectArchetype;
			Ar.AllowEliminatingReferences(TRUE);
		}
	}
	// Special support for supporting undo/redo of renaming and changing Archetype.
	else if( Ar.IsTransacting() )
	{
		if(!Ar.IsIgnoringOuterRef())
		{
			if(Ar.IsLoading())
			{
				UObject* LoadOuter = Outer;
				FName LoadName = Name;
				Ar << LoadName << LoadOuter;

				// If the name we loaded is different from the current one,
				// unhash the object, change the name and hash it again.
				UBOOL bDifferentName = Name != NAME_None && LoadName != Name;
				UBOOL bDifferentOuter = LoadOuter != Outer;
				if ( bDifferentName == TRUE || bDifferentOuter == TRUE )
				{
					UnhashObject();
					Name = LoadName;
					Outer = LoadOuter;
					HashObject();
				}
			}
			else
			{
				Ar << Name << Outer;
			}
		}

		if( !Ar.IsIgnoringArchetypeRef() )
		{
			Ar << ObjectArchetype;
		}
	}

	// Execution stack.
	//!!how does the stack work in conjunction with transaction tracking?
	if( !Ar.IsTransacting() )
	{
		if( HasAnyFlags(RF_HasStack) )
		{
			if( !StateFrame )
			{
				StateFrame = new FStateFrame( this );
			}
			Ar << StateFrame->Node << StateFrame->StateNode;
			Ar << StateFrame->ProbeMask;
			Ar << StateFrame->LatentAction;
			Ar << StateFrame->StateStack;
			if( StateFrame->Node )
			{
				Ar.Preload( StateFrame->Node );
				if( Ar.IsSaving() && StateFrame->Code )
				{
					BYTE* Start = StateFrame->Node->Script.GetTypedData();
					BYTE* End	= StateFrame->Node->Script.GetTypedData() + StateFrame->Node->Script.Num();
					check(Start != End);
					check(StateFrame->Code >= Start);
					check(StateFrame->Code < End);
				}
				INT Offset = StateFrame->Code ? StateFrame->Code - &StateFrame->Node->Script(0) : INDEX_NONE;
				Ar << Offset;
				if( Offset!=INDEX_NONE )
				{
					if( Offset<0 || Offset>=StateFrame->Node->Script.Num() )
					{
						appErrorf( TEXT("%s: Offset mismatch: %i %i"), *GetFullName(), Offset, StateFrame->Node->Script.Num() );
					}
				}
				StateFrame->Code = Offset!=INDEX_NONE ? &StateFrame->Node->Script(Offset) : NULL;
			}
			else 
			{
				StateFrame->Code = NULL;
			}
		}
		else if( StateFrame )
		{
			delete StateFrame;
			StateFrame = NULL;
		}
	}

	// if this is a subobject, then we need to copy the source defaults from the template subobject living in
	// inside the class in the .u file
	if (this->IsA(UComponent::StaticClass()))
	{
		((UComponent*)this)->PreSerialize(Ar);
	}

	SerializeNetIndex(Ar);

	// Serialize object properties which are defined in the class.
	if( Class != UClass::StaticClass() )
	{
		SerializeScriptProperties(Ar);
	}

	// Keep track of object flags that are part of RF_UndoRedoMask when transacting.
	if( Ar.IsTransacting() )
	{
		if( Ar.IsLoading() )
		{
			EObjectFlags OriginalObjectFlags;
			Ar << OriginalObjectFlags;
			ClearFlags( RF_UndoRedoMask );
			SetFlags( OriginalObjectFlags & RF_UndoRedoMask );
		}
		else if( Ar.IsSaving() )
		{
			Ar << ObjectFlags;
		}
	}

	// Memory counting.
	SIZE_T Size = GetClass()->GetPropertiesSize();
	Ar.CountBytes( Size, Size );
}


/**
 * Serializes the unrealscript property data located at Data.  When saving, only saves those properties which differ from the corresponding
 * value in the specified 'DiffObject' (usually the object's archetype).
 *
 * @param	Ar				the archive to use for serialization
 * @param	DiffObject		the object to use for determining which properties need to be saved (delta serialization);
 *							if not specified, the ObjectArchetype is used
 * @param	DefaultsCount	maximum number of bytes to consider for delta serialization; any properties which have an Offset+ElementSize greater
 *							that this value will NOT use delta serialization when serializing data;
 *							if not specified, the result of DiffObject->GetClass()->GetPropertiesSize() will be used.
 */
void UObject::SerializeScriptProperties( FArchive& Ar, UObject* DiffObject/*=NULL*/, INT DiffCount/*=0*/ ) const
{
	Ar.MarkScriptSerializationStart(this);
	if( (Ar.IsLoading() || Ar.IsSaving()) && !Ar.IsTransacting() )
	{
		if ( DiffObject == NULL )
		{
			DiffObject = GetArchetype();
		}
		if( HasAnyFlags(RF_ClassDefaultObject) )
		{
			Ar.StartSerializingDefaults();
		}

		GetClass()->SerializeTaggedProperties( Ar, (BYTE*)this, HasAnyFlags(RF_ClassDefaultObject) ? Class->GetSuperClass() : Class, (BYTE*)DiffObject, DiffCount );
		
		if( HasAnyFlags(RF_ClassDefaultObject) )
		{
			Ar.StopSerializingDefaults();
		}
	}
	else if ( Ar.GetPortFlags() != 0 )
	{
		if ( DiffObject == NULL )
		{
			DiffObject = GetArchetype();
		}
		if ( DiffCount == 0 && DiffObject != NULL )
		{
			DiffCount = DiffObject->GetClass()->GetPropertiesSize();
		}

		GetClass()->SerializeBinEx( Ar, (BYTE*)this, (BYTE*)DiffObject, DiffCount );
	}
	else
	{
		GetClass()->SerializeBin( Ar, (BYTE*)this, 0 );
	}

	Ar.MarkScriptSerializationEnd(this);
}

/**
 * Finds the component that is contained within this object that has the specified component name.
 *
 * @param	ComponentName	the component name to search for
 * @param	bRecurse		if TRUE, also searches all objects contained within this object for the component specified
 *
 * @return	a pointer to a component contained within this object that has the specified component name, or
 *			NULL if no components were found within this object with the specified name.
 */
UComponent* UObject::FindComponent( FName ComponentName, UBOOL bRecurse/*=FALSE*/ )
{
	UComponent* Result = NULL;

	if ( GetClass()->HasAnyClassFlags(CLASS_HasComponents) )
	{
		TArray<UComponent*> ComponentReferences;

		UObject* ComponentRoot = this;

		/**
		 * Currently, a component is instanced only if the Owner [passing into InstanceComponents] is the Outer for the
		 * component that is being instanced.  The following loop allows components to be instanced the first time a reference
		 * is encountered, regardless of whether the current object is the component's Outer or not.
		 */
		while ( ComponentRoot->GetOuter() && ComponentRoot->GetOuter()->GetClass() != UPackage::StaticClass() )
		{
			ComponentRoot = ComponentRoot->GetOuter();
		}

		TArchiveObjectReferenceCollector<UComponent> ComponentCollector(&ComponentReferences, 
																		ComponentRoot,				// Required Outer
																		FALSE,						// bRequireDirectOuter
																		TRUE,						// bShouldIgnoreArchetypes
																		bRecurse					// bDeepSearch
																		);

		Serialize(ComponentCollector);
		for ( INT CompIndex = 0; CompIndex < ComponentReferences.Num(); CompIndex++ )
		{
			UComponent* Component = ComponentReferences(CompIndex);
			if ( Component->TemplateName == ComponentName )
			{
				Result = Component;
				break;
			}
		}

		if ( Result == NULL && HasAnyFlags(RF_ClassDefaultObject) )
		{
			// see if this component exists in the class's component map
			UComponent** TemplateComponent = GetClass()->ComponentNameToDefaultObjectMap.Find(ComponentName);
			if ( TemplateComponent != NULL )
			{
				Result = *TemplateComponent;
			}
		}
	}

	return Result;
}

/**
 * Uses the TArchiveObjectReferenceCollector to build a list of all components referenced by this object which have this object as the outer
 *
 * @param	out_ComponentMap			the map that should be populated with the components "owned" by this object
 * @param	bIncludeNestedComponents	controls whether components which are contained by this object, but do not have this object
 *										as its direct Outer should be included
 */
void UObject::CollectComponents( TMap<FName,UComponent*>& out_ComponentMap, UBOOL bIncludeNestedComponents/*=FALSE*/ )
{
	TArray<UComponent*> ComponentArray;
	CollectComponents(ComponentArray,bIncludeNestedComponents);

	out_ComponentMap.Empty();
	for ( INT ComponentIndex = 0; ComponentIndex < ComponentArray.Num(); ComponentIndex++ )
	{
		UComponent* Comp = ComponentArray(ComponentIndex);
		out_ComponentMap.Set(Comp->GetInstanceMapName(), Comp);
	}
}

/**
 * Uses the TArchiveObjectReferenceCollector to build a list of all components referenced by this object which have this object as the outer
 *
 * @param	out_ComponentArray			the array that should be populated with the components "owned" by this object
 * @param	bIncludeNestedComponents	controls whether components which are contained by this object, but do not have this object
 *										as its direct Outer should be included
 */
void UObject::CollectComponents( TArray<UComponent*>& out_ComponentArray, UBOOL bIncludeNestedComponents/*=FALSE*/ )
{
	out_ComponentArray.Empty();
	TArchiveObjectReferenceCollector<UComponent> ComponentCollector(
		&out_ComponentArray,		//	InObjectArray
		this,						//	LimitOuter
		!bIncludeNestedComponents,	//	bRequireDirectOuter
		TRUE,						//	bIgnoreArchetypes
		TRUE,						//	bSerializeRecursively
		FALSE						//	bShouldIgnoreTransient
		);
	Serialize( ComponentCollector );
}

/**
 * Wrapper for InternalDumpComponents which allows this function to be easily called from a debugger watch window.
 */
void UObject::DumpComponents()
{
	// make sure we don't have any side-effects by ensuring that all objects' ObjectFlags stay the same
	FScopedObjectFlagMarker Marker;
	for ( FObjectIterator It; It; ++It )
	{
		It->ClearFlags(RF_TagExp|RF_TagImp);
	}

	if ( appIsDebuggerPresent() )
	{
		// if we have a debugger attached, the watch window won't be able to display the full output if we attempt to log it as a single string
		// so pass in GLog instead so that each line is sent separately;  this causes the output to have an extra line break between each log statement,
		// but at least we'll be able to see the full output in the debugger's watch window
		debugf(TEXT("Components for '%s':"), *GetFullName());
		ExportProperties( NULL, *GLog, GetClass(), (BYTE*)this, 0, NULL, NULL, this, PPF_ComponentsOnly );
		debugf(TEXT("<--- DONE!"));
	}
	else
	{
		FStringOutputDevice Output;
			Output.Logf(TEXT("Components for '%s':\r\n"), *GetFullName());
			ExportProperties( NULL, Output, GetClass(), (BYTE*)this, 2, NULL, NULL, this, PPF_ComponentsOnly );
			Output.Logf(TEXT("<--- DONE!\r\n"));
		debugf(*Output);
	}
}

/**
 * Exports the property values for the specified object as text to the output device.
 *
 * @param	Context			Context from which the set of 'inner' objects is extracted.  If NULL, an object iterator will be used.
 * @param	Out				the output device to send the exported text to
 * @param	ObjectClass		the class of the object to dump properties for
 * @param	Object			the address of the object to dump properties for
 * @param	Indent			number of spaces to prepend to each line of output
 * @param	DiffClass		the class to use for comparing property values when delta export is desired.
 * @param	Diff			the address of the object to use for determining whether a property value should be exported.  If the value in Object matches the corresponding
 *							value in Diff, it is not exported.  Specify NULL to export all properties.
 * @param	Parent			the UObject corresponding to Object
 * @param	PortFlags		flags used for modifying the output and/or behavior of the export
 */
void UObject::ExportProperties
(
	const FExportObjectInnerContext* Context,
	FOutputDevice&	Out,
	UClass*			ObjectClass,
	BYTE*			Object,
	INT				Indent,
	UClass*			DiffClass,
	BYTE*			Diff,
	UObject*		Parent,
	DWORD			PortFlags
)
{
	FString ThisName = TEXT("(none)");
	check(ObjectClass!=NULL);

	// catch any legacy code that is still passing in a UClass as a parent...this is no longer valid since
	// class default objects are now real UObjects, therefore subobjects will never have a UClass as the Outer
	// (it would be the class's default object instead)
	if ( Parent->GetClass() == UClass::StaticClass() )
	{
		Parent = ((UClass*)Parent)->GetDefaultObject(TRUE);
	}

	for( UProperty* Property = ObjectClass->PropertyLink; Property; Property = Property->PropertyLinkNext )
	{
		if( Property->Port(PortFlags) )
		{
			ThisName = Property->GetName();
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
			UBOOL bExportObject = (Property->PropertyFlags & CPF_ExportObject) != 0 && Cast<UObjectProperty>(Property,CLASS_IsAUObjectProperty);
			const DWORD ExportFlags = PortFlags | PPF_Delimited;

			if ( ArrayProperty != NULL )
			{
				// Export dynamic array.
				UProperty* InnerProp = ArrayProperty->Inner;
				bExportObject = (Property->PropertyFlags & CPF_ExportObject) != 0 && Cast<UObjectProperty>(InnerProp,CLASS_IsAUObjectProperty);

				for( INT PropertyArrayIndex=0; PropertyArrayIndex<Property->ArrayDim; PropertyArrayIndex++ )
				{
					FArray* Arr = (FArray*)((BYTE*)Object + Property->Offset + PropertyArrayIndex*Property->ElementSize);
					FArray*	DiffArr = NULL;
					if( DiffClass && Property->Offset < DiffClass->GetPropertiesSize() )
					{
						DiffArr = (FArray*)(Diff + Property->Offset + PropertyArrayIndex*Property->ElementSize);
					}

					for( INT DynamicArrayIndex=0;DynamicArrayIndex<Arr->Num();DynamicArrayIndex++ )
					{
						FString	Value;

						// compare each element's value manually so that elements which match the NULL value for the array's inner property type
						// but aren't in the diff array are still exported
						BYTE* SourceData = (BYTE*)Arr->GetData() + DynamicArrayIndex * InnerProp->ElementSize;
						BYTE* DiffData = DiffArr && DynamicArrayIndex < DiffArr->Num()
							? (BYTE*)DiffArr->GetData() + DynamicArrayIndex * InnerProp->ElementSize
							: NULL;

						UBOOL bExportItem = DiffData == NULL || (DiffData != SourceData && !InnerProp->Identical(SourceData, DiffData, ExportFlags));
						if ( bExportItem )
						{
							InnerProp->ExportTextItem(Value, SourceData, DiffData, Parent, ExportFlags);
							if( bExportObject )
							{
								UObject* Obj = ((UObject**)Arr->GetData())[DynamicArrayIndex];
								if( Obj && !Obj->HasAnyFlags(RF_TagImp) )
								{
									// only export the BEGIN OBJECT block for a component if Parent is the component's Outer....when importing subobject definitions,
									// (i.e. BEGIN OBJECT), whichever BEGIN OBJECT block a component's BEGIN OBJECT block is located within is the object that will be
									// used as the Outer to create the component

									// Is this an array of components?
									if ( InnerProp->GetClass() == UComponentProperty::StaticClass() )
									{
										if ( Obj->GetOuter() == Parent )
										{
											// Don't export more than once.
											Obj->SetFlags( RF_TagImp );
											UExporter::ExportToOutputDevice( Context, Obj, NULL, Out, TEXT("T3D"), Indent, PortFlags );
										}
										else
										{
											// set the RF_TagExp flag so that the calling code knows we wanted to export this object
											Obj->SetFlags(RF_TagExp);
										}
									}
									else
									{
										// Don't export more than once.
										Obj->SetFlags( RF_TagImp );
										UExporter::ExportToOutputDevice( Context, Obj, NULL, Out, TEXT("T3D"), Indent, PortFlags );
									}
								}
							}

							Out.Logf( TEXT("%s%s(%i)=%s\r\n"), appSpc(Indent), *Property->GetName(), DynamicArrayIndex, *Value );
						}
					}
				}
			}
			else
			{
				for( INT PropertyArrayIndex=0; PropertyArrayIndex<Property->ArrayDim; PropertyArrayIndex++ )
				{
					FString	Value;
					// Export single element.

					BYTE* DiffData = (DiffClass && Property->Offset < DiffClass->GetPropertiesSize()) ? Diff : NULL;
					if( Property->ExportText( PropertyArrayIndex, Value, Object, DiffData, Parent, ExportFlags ) )
					{
						if ( bExportObject )
						{
							UObject* Obj = *(UObject **)((BYTE*)Object + Property->Offset + PropertyArrayIndex*Property->ElementSize);
							if( Obj && !Obj->HasAnyFlags(RF_TagImp) )
							{
								// only export the BEGIN OBJECT block for a component if Parent is the component's Outer....when importing subobject definitions,
								// (i.e. BEGIN OBJECT), whichever BEGIN OBJECT block a component's BEGIN OBJECT block is located within is the object that will be
								// used as the Outer to create the component
								if ( Property->GetClass() == UComponentProperty::StaticClass() )
								{
									if ( Obj->GetOuter() == Parent )
									{
										// Don't export more than once.
										Obj->SetFlags( RF_TagImp );
										UExporter::ExportToOutputDevice( Context, Obj, NULL, Out, TEXT("T3D"), Indent, PortFlags );
									}
									else
									{
										// set the RF_TagExp flag so that the calling code knows we wanted to export this object
										Obj->SetFlags(RF_TagExp);
									}
								}
								else
								{
									// Don't export more than once.
									Obj->SetFlags( RF_TagImp );
									UExporter::ExportToOutputDevice( Context, Obj, NULL, Out, TEXT("T3D"), Indent, PortFlags );
								}
							}
						}

						if( Property->ArrayDim == 1 )
						{
							Out.Logf( TEXT("%s%s=%s\r\n"), appSpc(Indent), *Property->GetName(), *Value );
						}
						else
						{
							Out.Logf( TEXT("%s%s(%i)=%s\r\n"), appSpc(Indent), *Property->GetName(), PropertyArrayIndex, *Value );
						}
					}
				}
			}
		}
	}
}

//
// Initialize script execution.
//
void UObject::InitExecution()
{
	check(GetClass()!=NULL);

	if( StateFrame )
		delete StateFrame;
	StateFrame = new FStateFrame( this );
	SetFlags( RF_HasStack );
}

//
// Command line.
//
UBOOL UObject::ScriptConsoleExec( const TCHAR* Str, FOutputDevice& Ar, UObject* Executor )
{
	// Find UnrealScript exec function.
	FString MsgStr;
	FName Message = NAME_None;
	UFunction* Function = NULL;
	if
	(	!ParseToken(Str,MsgStr,TRUE)
	||	(Message=FName(*MsgStr,FNAME_Find))==NAME_None
	||	(Function=FindFunction(Message))==NULL
	||	(Function->FunctionFlags & FUNC_Exec) == 0 )
	{
		return FALSE;
	}

	UProperty* LastParameter=NULL;

	// find the last parameter
	for ( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Function); It && (It->PropertyFlags&(CPF_Parm|CPF_ReturnParm)) == CPF_Parm; ++It )
	{
		LastParameter = *It;
	}

	UStrProperty* LastStringParameter = Cast<UStrProperty>(LastParameter);

	// Parse all function parameters.
	BYTE* Parms = (BYTE*)appAlloca(Function->ParmsSize);
	appMemzero( Parms, Function->ParmsSize );
	UBOOL Failed = 0;
	INT NumParamsEvaluated = 0;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Function); It && (It->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It, NumParamsEvaluated++ )
	{
		if( NumParamsEvaluated == 0 && Executor )
		{
			UObjectProperty* Op = Cast<UObjectProperty>(*It,CLASS_IsAUObjectProperty);
			if( Op && Executor->IsA(Op->PropertyClass) )
			{
				// First parameter is implicit reference to object executing the command.
				*(UObject**)(Parms + It->Offset) = Executor;
				continue;
			}
		}

		ParseNext( &Str );

		DWORD ExportFlags = PPF_Localized;
		// if this is the last parameter of the exec function and it's a string, make sure that it accepts the remainder of the passed in value
		if ( LastStringParameter != *It )
		{
			ExportFlags |= PPF_Delimited;
		}
		const TCHAR* PreviousStr = Str;
		const TCHAR* Result = It->ImportText( Str, Parms+It->Offset, ExportFlags, NULL );
		UBOOL bFailedImport = (Result == NULL || Result == PreviousStr);
		if( bFailedImport )
		{
			if( !(It->PropertyFlags & CPF_OptionalParm) )
			{
				Ar.Logf( *LocalizeError(TEXT("BadProperty"),TEXT("Core")), *Message.ToString(), *It->GetName() );
				Failed = 1;
			}
			break;
		}

		// move to the next parameter
		Str = Result;
	}

	if( !Failed )
	{
		ProcessEvent( Function, Parms );
	}

	//!!destructframe see also UObject::ProcessEvent
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Function); It && (It->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
	{
		It->DestroyValue( Parms + It->Offset );
	}

	// Success.
	return TRUE;
}

//
// Find an UnrealScript field.
//warning: Must be safe with class default metaobjects.
//
UField* UObject::FindObjectField( FName InName, UBOOL Global )
{
	// Search current state scope.
	if( StateFrame && StateFrame->StateNode && !Global )
		for( TFieldIterator<UField> It(StateFrame->StateNode); It; ++It )
			if( It->GetFName()==InName )
				return *It;

	// Search the global scope.
	for( TFieldIterator<UField> It(GetClass()); It; ++It )
		if( It->GetFName()==InName )
			return *It;

	// Not found.
	return NULL;
}
UFunction* UObject::FindFunction( FName InName, UBOOL Global )
{
	UFunction *Function = NULL;
	if( StateFrame != NULL && StateFrame->StateNode != NULL && !Global )
	{
		// search current/parent states
		UState *SearchState = StateFrame->StateNode;
		while( SearchState != NULL && Function == NULL)
		{
			Function	= SearchState->FuncMap.FindRef(InName);
			SearchState = SearchState->GetSuperState();
		}
	}
	if( Function == NULL )
	{
		// and search the global state
		UClass *SearchClass = GetClass();
		while( SearchClass != NULL && Function == NULL )
		{
			Function	= SearchClass->FuncMap.FindRef(InName);
			SearchClass = SearchClass->GetSuperClass();
		}
	}
    return Function;
}
UFunction* UObject::FindFunctionChecked( FName InName, UBOOL Global )
{
    UFunction* Result = FindFunction(InName,Global);
	if( !Result )
	{
		appErrorf( TEXT("Failed to find function %s in %s"), *InName.ToString(), *GetFullName() );
	}
	return Result;
}
UState* UObject::FindState( FName InName )
{
	UState* State = NULL;
	for (TFieldIterator<UState, CLASS_IsAUState> It(GetClass()); It && !State; ++It)
	{
		if (It->GetFName() == InName)
		{
			State = *It;
		}
	}
    return State;
}

/**
 * Determines whether the specified object should load values using PerObjectConfig rules
 */
static UBOOL UsesPerObjectConfig( UObject* SourceObject )
{
	checkSlow(SourceObject);
	return (SourceObject->GetClass()->HasAnyClassFlags(CLASS_PerObjectConfig) && !SourceObject->HasAnyFlags(RF_ClassDefaultObject));
}

/**
 * Returns the file to load ini values from for the specified object, taking into account PerObjectConfig-ness
 */
static FString GetConfigFilename( UObject* SourceObject )
{
	checkSlow(SourceObject);

	// if this is a PerObjectConfig object that is not contained by the transient package,
	// unless the PerObjectConfig class specified a different ini file.
	return (UsesPerObjectConfig(SourceObject) && SourceObject->GetOutermost() != UObject::GetTransientPackage())
		? (appGameConfigDir() + FString(GGameName) + *SourceObject->GetOuter()->GetName() + TEXT(".ini"))
		: SourceObject->GetClass()->GetConfigName();

}

/*-----------------------------------------------------------------------------
	UObject configuration.
-----------------------------------------------------------------------------*/
/**
 * Wrapper method for LoadConfig that is used when reloading the config data for objects at runtime which have already loaded their config data at least once.
 * Allows the objects the receive a callback that it's configuration data has been reloaded.
 */
void UObject::ReloadConfig( UClass* ConfigClass/*=NULL*/, const TCHAR* InFilename/*=NULL*/, DWORD PropagationFlags/*=LCPF_None*/, UProperty* PropertyToLoad/*=NULL*/ )
{
	LoadConfig(ConfigClass, InFilename, PropagationFlags|UE3::LCPF_ReloadingConfigData|UE3::LCPF_ReadParentSections, PropertyToLoad);
}

/**
 * Imports property values from an .ini file.
 *
 * @param	Class				the class to use for determining which section of the ini to retrieve text values from
 * @param	InFilename			indicates the filename to load values from; if not specified, uses ConfigClass's ClassConfigName
 * @param	PropagationFlags	indicates how this call to LoadConfig should be propagated; expects a bitmask of UE3::ELoadConfigPropagationFlags values.
 * @param	PropertyToLoad		if specified, only the ini value for the specified property will be imported.
 */
void UObject::LoadConfig( UClass* ConfigClass/*=NULL*/, const TCHAR* InFilename/*=NULL*/, DWORD PropagationFlags/*=LCPF_None*/, UProperty* PropertyToLoad/*=NULL*/ )
{
	SCOPE_CYCLE_COUNTER(STAT_LoadConfig);

	// OriginalClass is the class that LoadConfig() was originally called on
	static UClass* OriginalClass = NULL;

	if( !ConfigClass )
	{
		// if no class was specified in the call, this is the OriginalClass
		ConfigClass = GetClass();
		OriginalClass = ConfigClass;
	}

	if( !ConfigClass->HasAnyClassFlags(CLASS_Config) )
	{
		return;
	}

	UClass* ParentClass = ConfigClass->GetSuperClass();
	if ( ParentClass != NULL )
	{
		if ( ParentClass->HasAnyClassFlags(CLASS_Config) )
		{
			if ( (PropagationFlags&UE3::LCPF_ReadParentSections) != 0 )
			{
				// call LoadConfig on the parent class
				LoadConfig( ParentClass, NULL, PropagationFlags, PropertyToLoad );

				// if we are also notifying child classes or instances, stop here as this object's properties will be imported as a result of notifying the others
				if ( (PropagationFlags & (UE3::LCPF_PropagateToChildDefaultObjects|UE3::LCPF_PropagateToInstances)) != 0 )
				{
					return;
				}
			}
			else if ( (PropagationFlags&UE3::LCPF_PropagateToChildDefaultObjects) != 0 )
			{
				// not propagating the call upwards, but we are propagating the call to all child classes
				for (TObjectIterator<UClass> It; It; ++It)
				{
					if (It->IsChildOf(ConfigClass))
					{
						// mask out the PropgateToParent and PropagateToChildren values
						It->GetDefaultObject()->LoadConfig(*It, NULL, (PropagationFlags&(UE3::LCPF_PersistentFlags|UE3::LCPF_PropagateToInstances)), PropertyToLoad);
					}
				}

				// LoadConfig() was called on this object during iteration, so stop here 
				return;
			}
			else if ( (PropagationFlags&UE3::LCPF_PropagateToInstances) != 0 )
			{
				// call LoadConfig() on all instances of this class (except the CDO)
				// Do not propagate this call to parents, and do not propagate to children or instances (would be redundant) 
				for (TObjectIterator<UObject> It; It; ++It)
				{
					if (It->IsA(ConfigClass))
					{
						if ( !GIsEditor )
						{
							// make sure to pass in the class so that OriginalClass isn't reset
							It->LoadConfig(It->GetClass(), NULL, (PropagationFlags&UE3::LCPF_PersistentFlags), PropertyToLoad);
						}
						else
						{
							It->PreEditChange(NULL);

							// make sure to pass in the class so that OriginalClass isn't reset
							It->LoadConfig(It->GetClass(), NULL, (PropagationFlags&UE3::LCPF_PersistentFlags), PropertyToLoad);

							It->PostEditChange(NULL);
						}
					}
				}
			}
		}
		else if ( (PropagationFlags&UE3::LCPF_PropagateToChildDefaultObjects) != 0 )
		{
			// we're at the base-most config class
			for ( TObjectIterator<UClass> It; It; ++It )
			{
				if ( It->IsChildOf(ConfigClass) )
				{
					if ( !GIsEditor )
					{
						// make sure to pass in the class so that OriginalClass isn't reset
						It->GetDefaultObject()->LoadConfig( *It, NULL, (PropagationFlags&(UE3::LCPF_PersistentFlags|UE3::LCPF_PropagateToInstances)), PropertyToLoad );
					}
					else
					{
						It->PreEditChange(NULL);

						// make sure to pass in the class so that OriginalClass isn't reset
						It->GetDefaultObject()->LoadConfig( *It, NULL, (PropagationFlags&(UE3::LCPF_PersistentFlags|UE3::LCPF_PropagateToInstances)), PropertyToLoad );

						It->PostEditChange(NULL);
					}
				}
			}

			return;
		}
		else if ( (PropagationFlags&UE3::LCPF_PropagateToInstances) != 0 )
		{
			for ( TObjectIterator<UObject> It; It; ++It )
			{
				if ( It->GetClass() == ConfigClass )
				{
					if ( !GIsEditor )
					{
						// make sure to pass in the class so that OriginalClass isn't reset
						It->LoadConfig(It->GetClass(), NULL, (PropagationFlags&UE3::LCPF_PersistentFlags), PropertyToLoad);
					}
					else
					{
						It->PreEditChange(NULL);

						// make sure to pass in the class so that OriginalClass isn't reset
						It->LoadConfig(It->GetClass(), NULL, (PropagationFlags&UE3::LCPF_PersistentFlags), PropertyToLoad);
						It->PostEditChange(NULL);
					}
				}
			}
		}
	}

	const FString Filename
	// if a filename was specified, always load from that file
	=	InFilename
		? InFilename
		: GetConfigFilename(this);

	const UBOOL bPerObject = UsesPerObjectConfig(this);

	FString ClassSection;
	if ( bPerObject == TRUE )
	{
		ClassSection = FString::Printf(TEXT("%s %s"), *GetName(), *GetClass()->GetName());
	}

	// If any of my properties are class variables, then LoadConfig() would also be called for each one of those classes.
	// Since OrigClass is a static variable, if the value of a class variable is a class different from the current class, 
	// we'll lose our nice reference to the original class - and cause any variables which were declared after this class variable to fail 
	// the 'if (OriginalClass != Class)' check....better store it in a temporary place while we do the actual loading of our properties 
	UClass* MyOrigClass = OriginalClass;

	if ( PropertyToLoad == NULL )
	{
		debugfSlow(NAME_DevSave, TEXT("(%s) '%s' loading configuration from %s"), *ConfigClass->GetName(), *GetName(), *Filename);
	}
	else
	{
		debugfSlow(NAME_DevSave, TEXT("(%s) '%s' loading configuration for property %s from %s"), *ConfigClass->GetName(), *GetName(), *PropertyToLoad->GetName(), *Filename);
	}

// 	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(ConfigClass); It; ++It )
	for ( UProperty* Property = ConfigClass->ConfigLink; Property; Property = Property->ConfigLinkNext )
	{
		// if we're only supposed to load the value for a specific property, skip all others
		if ( PropertyToLoad != NULL && PropertyToLoad != Property )
		{
			continue;
		}

		const UBOOL bGlobalConfig = (Property->PropertyFlags&CPF_GlobalConfig) != 0;
		UClass* OwnerClass = Property->GetOwnerClass();

		UClass* BaseClass = bGlobalConfig ? OwnerClass : ConfigClass;
		if ( !bPerObject )
		{
			ClassSection = BaseClass->GetPathName();
		}

		FString Key = Property->GetName();
		debugfSlow(NAME_DevSave, TEXT("   Loading value for %s from [%s]"), *Key, *ClassSection);

		UArrayProperty* Array = Cast<UArrayProperty>( Property );
		if( Array )
		{
			TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate( *ClassSection, FALSE, TRUE, *Filename );
			if( Sec )
			{
				TArray<FString> List;
				Sec->MultiFind( Key, List );

				// Only override default properties if there is something to override them with.
				if ( List.Num() > 0 )
				{
					FArray*		Ptr  = (FArray*)((BYTE*)this + Property->Offset);
					const INT	Size = Array->Inner->ElementSize;
					Array->DestroyValue( Ptr );
					Ptr->AddZeroed( List.Num(), Size, DEFAULT_ALIGNMENT );

					UStructProperty* InnerStructProp = Cast<UStructProperty>(Array->Inner,CLASS_IsAUStructProperty);
					if ( InnerStructProp != NULL && InnerStructProp->Struct
					&&	InnerStructProp->Struct->GetDefaultsCount()
					&&	InnerStructProp->HasValue(InnerStructProp->Struct->GetDefaults()) )
					{
						for ( INT ArrayIndex = List.Num() - 1; ArrayIndex >= 0; ArrayIndex-- )
						{
							BYTE* ValueAddress = (BYTE*)Ptr->GetData() + ArrayIndex * Size;
							InnerStructProp->InitializeValue(ValueAddress);
						}
					}
					for( INT i=List.Num()-1,c=0; i>=0; i--,c++ )
					{
						Array->Inner->ImportText( *List(i), (BYTE*)Ptr->GetData() + c*Size, 0, this );
					}
				}
				else
				{
					// If nothing was found, try searching for indexed keys
					FArray*		Ptr  = (FArray*)((BYTE*)this + Property->Offset);
					const INT	Size = Array->Inner->ElementSize;

					UStructProperty* InnerStructProp = Cast<UStructProperty>(Array->Inner,CLASS_IsAUStructProperty);
					if ( InnerStructProp != NULL
					&&	(InnerStructProp->Struct == NULL
					||	!InnerStructProp->Struct->GetDefaultsCount()
					||	!InnerStructProp->HasValue(InnerStructProp->Struct->GetDefaults())) )
					{
						InnerStructProp = NULL;
					}

					INT Index = 0;
					FString* ElementValue = NULL;
					do
					{
						// Add array index number to end of key
						FString IndexedKey = FString::Printf(TEXT("%s[%i]"), *Key, Index);

						// Try to find value of key
						ElementValue  = Sec->Find( IndexedKey );

						// If found, import the element
						if ( ElementValue != NULL )
						{
							BYTE* ValueAddress = (BYTE*)Ptr->GetData() + Index * Size;

							if ( InnerStructProp != NULL )
							{
								// initialize struct defaults if applicable
								InnerStructProp->InitializeValue(ValueAddress);
							}

							Array->Inner->ImportText(**ElementValue, ValueAddress, 0, this);
						}

						Index++;
					} while( ElementValue || Index < Ptr->Num() );
				}
			}
		}
		else
		{
			UMapProperty* Map = Cast<UMapProperty>( Property );
			if( Map )
			{
				TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate( *ClassSection, 0, 1, *Filename );
				if( Sec )
				{
					TArray<FString> List;
					Sec->MultiFind( Key, List );
					//FArray* Ptr  = (FArray*)((BYTE*)this + Property->Offset);
					//INT     Size = Array->Inner->ElementSize;
					//Map->DestroyValue( Ptr );//!!won't do dction
					//Ptr->AddZeroed( List.Num(), Size, DEFAULT_ALIGNMENT );
					//for( INT i=List.Num()-1,c=0; i>=0; i--,c++ )
					//	Array->Inner->ImportText( *List(i), (BYTE*)Ptr->GetData() + c*Size, 0 );
				}
			}
			else
			{
				for( INT i=0; i<Property->ArrayDim; i++ )
				{
					if( Property->ArrayDim!=1 )
					{
						Key = FString::Printf(TEXT("%s[%i]"), *Property->GetName(), i);
					}

					FString Value;
					if( GConfig->GetString( *ClassSection, *Key, Value, *Filename ) )
					{
						Property->ImportText( *Value, (BYTE*)this + Property->Offset + i*Property->ElementSize, 0, this );
					}
				}
			}
		}
	}

	// if we are reloading config data after the initial class load, fire the callback now
	if ( (PropagationFlags&UE3::LCPF_ReloadingConfigData) != 0 )
	{
		PostReloadConfig(PropertyToLoad);
	}
}

//static
void UObject::LoadLocalizedDynamicArray(UArrayProperty *Prop, const TCHAR* IntName, const TCHAR* SectionName, const TCHAR* KeyPrefix, UObject* Parent, BYTE* Data)
{
	TMultiMap<FString,FString>* Sec = NULL;

	// Find the localization file containing the section.
	// This code should search the same files as Localize(...)
	const TCHAR* LangExt = UObject::GetLanguage();
	for( INT PathIndex=0; PathIndex<GSys->LocalizationPaths.Num(); PathIndex++ )
	{
		// Try specified language first and fall back to default (int) if not found.
		const FFilename FilenameLang	= FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("%s") PATH_SEPARATOR TEXT("%s.%s"), *GSys->LocalizationPaths(PathIndex), LangExt	  , IntName, LangExt	 );
		Sec = GConfig->GetSectionPrivate( SectionName, 0, 1, *FilenameLang );
		if ( Sec )
		{
			break;
		}
		const FFilename FilenameInt	= FString::Printf( TEXT("%s") PATH_SEPARATOR TEXT("%s") PATH_SEPARATOR TEXT("%s.%s"), *GSys->LocalizationPaths(PathIndex), TEXT("INT"), IntName, TEXT("INT") );
		Sec = GConfig->GetSectionPrivate( SectionName, 0, 1, *FilenameInt );
		if ( Sec )
		{
			break;
		}
	}

	// If the section was found
	if( Sec )
	{
		// Get array of properties
		FArray* Ptr  = (FArray*)(Data + Prop->Inner->Offset);

		TMap<INT,FString> LocValues;
		TArray<FString> List;

		// Find each entry in the section
		//Sec->MultiFind( FName(*KeyPrefix), List );
		Sec->MultiFind( KeyPrefix, List );

		if( List.Num() > 0 )
		{
			for ( INT i = List.Num() - 1, cnt = 0; i >= 0; i--, cnt++ )
			{
				LocValues.Set(cnt, *List(i));
			}
		}
		else
		{
			// If nothing was found, try searching for indexed keys
			INT Index = 0;
			FString* FoundKey = NULL;

			do
			{
				// Add array index number to end of key
				TCHAR IndexedKey[MAX_SPRINTF]=TEXT("");
				appSprintf(IndexedKey, TEXT("%s[%i]"), KeyPrefix, Index);

				// Try to find value of key
				FoundKey  = Sec->Find( IndexedKey );

				// If found
				if( FoundKey )
				{
					// add to map of loc values
					LocValues.Set(Index, **FoundKey);
				}

				Index++;
			} while( FoundKey || Index < Ptr->Num() );
		}

		// Get size of each element to help walk the list
		const INT Size = Prop->Inner->ElementSize;

		INT ImportedCount = 0;
		for ( TMap<INT,FString>::TIterator It(LocValues); It; ++It )
		{
			const INT ArrayIndex = It.Key();
			const FString& LocString = It.Value();

			// if this index is higher than the number of elements currently in the array (+1),
			// add empty elements to fill the gap
			if ( ArrayIndex >= Ptr->Num() )
			{
				const INT AddCount = (ArrayIndex - Ptr->Num()) + 1;
				Ptr->AddZeroed( AddCount, Size, DEFAULT_ALIGNMENT );
			}

			// keep track of the highest index we imported - we should remove all pre-existing elements that
			// are located higher than this index
			if ( ArrayIndex + 1 > ImportedCount )
			{
				ImportedCount = ArrayIndex + 1;
			}

			Prop->Inner->ImportText( *LocString, (BYTE*)Ptr->GetData() + ArrayIndex * Size, PPF_LocalizedOnly, Parent );
		}

		// this code would remove any pre-existing elements that had a higher index than the highest
		// index we found while importing localized text, but I haven't found any situations where
		// we actually want this to occur....
		//		if ( Ptr->Num() > 0 && ImportedCount > 0 && Ptr->Num() > ImportedCount )
		//			Ptr->Remove(ImportedCount, Ptr->Num() - ImportedCount, Size);
	}

#if !NO_LOGGING && !FINAL_RELEASE
	else
	{
		static UBOOL bShowMissingLoc = ParseParam(appCmdLine(), TEXT("SHOWMISSINGLOC"));
		if ( !bShowMissingLoc )
		{
			const UBOOL bAllowMissingLocData = 

				// static arrays generate lots of false positives, so just ignore them as they aren't that common anymore
				Prop->ArrayDim > 1 ||

				// if the class is abstract or deprecated then the localized data is probably defined in concrete child classes
				Parent->GetClass()->HasAnyClassFlags(CLASS_Abstract|CLASS_Deprecated) || 

				// localization for PerObjectConfig objects is usually done per-instance, so don't require it for the class's CDO
				(Parent->GetClass()->HasAnyClassFlags(CLASS_PerObjectConfig) && Parent->HasAnyFlags(RF_ClassDefaultObject)) ||

				// if this is an inherited property and the object being localized already has a value for this property, it means that the loc data was defined
				// in the parent's loc section and was inherited from the parent - no need to complain about missing loc data.
				(Prop->GetOwnerClass() != Parent->GetClass() && Prop->HasValue(Data,PPF_LocalizedOnly));

			if ( !bAllowMissingLocData )
			{
				debugfSuppressed(NAME_LocalizationWarning, TEXT("SECTION [%s] not found in FILE '%s' while attempting to load localized DYNAMIC ARRAY %s.%s"),
					SectionName, IntName, *Parent->GetPathName(), *Prop->GetName());
			}
		}
	}
#endif
}

//static
void UObject::LoadLocalizedStruct( UStruct* Struct, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, UObject* Parent, BYTE* Data )
{
	checkSlow(Struct);
	for ( UProperty* Prop = Struct->PropertyLink; Prop; Prop = Prop->PropertyLinkNext )
	{
		if ( !Prop->IsLocalized() )
		{
			continue;
		}

	    for( INT i = 0; i < Prop->ArrayDim; i++ )
	    {
    	    FString NewPrefix;

			// If a key prefix already exists, prepare to append more to it.
            if( KeyPrefix )
			{
                NewPrefix = FString::Printf( TEXT("%s."), KeyPrefix );
			}

	        if( Prop->ArrayDim > 1 )
			{
				// Key is an index into a static array.
                NewPrefix += FString::Printf( TEXT("%s[%d]"), *Prop->GetName(), i );
			}
	        else
			{
				// Only one entry -- just use the property name.
                NewPrefix += Prop->GetName();
			}

            BYTE* NewData = Data + (Prop->Offset) + (i * Prop->ElementSize );
            LoadLocalizedProp( Prop, IntName, SectionName, *NewPrefix, Parent, NewData );
	    }
	}
}

//static
void UObject::LoadLocalizedProp( UProperty* Prop, const TCHAR *IntName, const TCHAR *SectionName, const TCHAR *KeyPrefix, UObject* Parent, BYTE* Data )
{
	// Is the property a struct property?
	UStructProperty* StructProperty = ExactCast<UStructProperty>( Prop );
	if( StructProperty )
	{
		LoadLocalizedStruct(StructProperty->Struct, IntName, SectionName, KeyPrefix, Parent, Data );
		return;
	}

	// Is the property a dyanmic array?
	UArrayProperty* ArrayProperty = ExactCast<UArrayProperty>( Prop );
	if( ArrayProperty )
	{
		// Load each item in the array - this actually imports the text into the property
		LoadLocalizedDynamicArray( ArrayProperty, IntName, SectionName, KeyPrefix, Parent, Data );
		return;
	}

	FString LocalizedText = Localize( SectionName, KeyPrefix, IntName, NULL, TRUE );
	if( LocalizedText.Len() > 0 )
	{
		Prop->ImportText( *LocalizedText, Data, PPF_LocalizedOnly, Parent );
	}

#if !NO_LOGGING && !FINAL_RELEASE
	else
	{
		static UBOOL bShowMissingLoc = ParseParam(appCmdLine(), TEXT("SHOWMISSINGLOC"));
		if ( !bShowMissingLoc )
		{
			const UBOOL bAllowMissingLocData = 
				// static arrays generate lots of false positives, so just ignore them as they aren't that common anymore
				Prop->ArrayDim > 1 ||
				// if the class is abstract or deprecated then the localized data is probably defined in concrete child classes
				Parent->GetClass()->HasAnyClassFlags(CLASS_Abstract|CLASS_Deprecated) || 

				// localization for PerObjectConfig objects is usually done per-instance, so don't require it for the class's CDO
				(Parent->GetClass()->HasAnyClassFlags(CLASS_PerObjectConfig) && Parent->HasAnyFlags(RF_ClassDefaultObject)) ||

				// if the object being localized already has a value for this property, it means that the loc data was defined
				// in the object's archetype's loc section and was inherited - no need to complain about missing loc data.
				Prop->HasValue(Data,PPF_LocalizedOnly);

			if ( !bAllowMissingLocData )
			{
				debugfSuppressed(NAME_LocalizationWarning, TEXT("No localized value found for PROPERTY %s.%s in FILE '%s' SECTION [%s] KEY '%s'"), 
					*Parent->GetPathName(), *Prop->GetName(), IntName, SectionName, KeyPrefix);
			}
		}
	}
#endif
}

/**
 * Imports the localized property values for this object.
 *
 * @param	LocBase					the object to use for determing where to load the localized property from; defaults to 'this';  should always be
 *									either 'this' or an archetype of 'this'
 * @param	bLoadHierachecally		specify TRUE to have this object import the localized property data from its archetype's localization location first.
 */
void UObject::LoadLocalized( UObject* LocBase/*=NULL*/, UBOOL bLoadHierachecally/*=FALSE*/ )
{
	SCOPE_CYCLE_COUNTER(STAT_LoadLocalized);
	if ( LocBase == NULL )
	{
		LocBase = this;
	}
	else if ( LocBase != this )
	{
		checkfSlow( IsBasedOnArchetype(LocBase), TEXT("%s is not an archetype of %s"), *LocBase->GetFullName(), *GetFullName() );
	}

	// we want to load localized properties in the editor, but not when compiling scripts
	if( GIsUCCMake )
	{
		return;
	}

	UClass* LocClass = LocBase->GetClass();

	// if this class isn't localized, no data to load
	if ( !LocClass->HasAnyClassFlags(CLASS_Localized) )
	{
		return;
	}

	debugfSlow(NAME_Localization, TEXT("%sImporting localized property values for %s"), LocBase == this ? TEXT("") : TEXT(">>>  "), *GetFullName());

	if ( bLoadHierachecally == TRUE )
	{
		// if we're supposed to read localized data from our archetype's sections first
		LoadLocalized(LocBase->GetArchetype(), TRUE);
	}

	FString LocFilename, LocSectionName, LocPrefix;
	if ( GetLocalizationDataLocation(LocBase, LocFilename, LocSectionName, LocPrefix) )
	{
		LoadLocalizedStruct( LocClass, *LocFilename, *LocSectionName, LocPrefix.Len() > 0 ? *LocPrefix : NULL, this, (BYTE*)this );
	}
}

/**
 * Retrieves the location of the property values for this object's localized properties.
 *
 * @param	LocBase			the object to use for determing where to load the localized property from; should always be
 *							either 'this' or an archetype of 'this'
 * @param	LocFilename		[out] receives the filename which contains the loc data for this object.
 *							this value will contain the base filename only; no path or extension information
 * @param	LocSection		[out] receives the section name that contains the loc data for this object
 * @param	LocPrefix		[out] receives the prefix string to use when reading keynames from the loc section; usually only relevant when loading
 *							loading loc data for subobjects, and in that case will always be the name of the subobject template
 *
 * @return	TRUE if LocFilename and LocSection were filled with valid values; FALSE if this object's class isn't localized or the loc data location
 *			couldn't be found for some reason.
 */
UBOOL UObject::GetLocalizationDataLocation( UObject* LocBase, FString& LocFilename, FString& LocSection, FString& LocPrefix )
{
	UBOOL bResult = FALSE;

	if ( LocBase == NULL )
	{
		LocBase = this;
	}
	checkSlow(LocBase);

	UClass* LocClass = LocBase->GetClass();
	if ( LocClass->HasAnyClassFlags(CLASS_Localized) )
	{
		bResult = TRUE;

		if ( LocBase->HasAnyFlags(RF_ClassDefaultObject) )
		{
			// for class default objects, the filename is the package containing the class and the section is the name of the class
			LocFilename = LocClass->GetOutermost()->GetName();
			LocSection = LocClass->GetName();
			LocPrefix = TEXT("");
		}
		else
		{
			// for instances, we have three possibilities (all of which first inherit their localized data from their archetype):
			// 1. instance of a class marked PerObjectConfig - loads its own localized data from a loc section unique to that instance
			// 2. subobject of a class not marked PerObjectConfig - loads its own localized data from the loc section of its Outer's class using a key prefix
			// 3. instance of a class not marked PerObjectConfig - doesn't load its own localized data, reads shared loc data from its class's loc data location
			if ( LocBase->HasAnyFlags(RF_PerObjectLocalized) )
			{
				if ( LocClass->HasAnyClassFlags(CLASS_PerObjectConfig) )
				{
					LocPrefix = TEXT("");

					// case 1 - for POC objects, we need to do a little extra work
					// if the POC object is contained within a content package, the filename is the name of the package containing the instance
					// otherwise, filename is the name of the package containing the class
					// in both cases, the section is formatted like so: [InstanceName ClassName] so that it matches the way that PerObjectConfig .ini sections are named
					if ( LocBase->GetOutermost() != UObject::GetTransientPackage() )
					{
						// contained within a content package

						//@fixme ronp - hack for gears; keep using the older format since all loc files have already been created using this format
						static bool bUseOldLocFormat = !appStricmp(GGameName,TEXT("War"));
						if ( bUseOldLocFormat )
						{
							LocFilename = LocBase->GetOutermost()->GetName();
							LocSection = LocBase->GetOuter()->GetName();
							LocPrefix = LocBase->GetName();
						}
						else
						{
							LocFilename = LocBase->GetOutermost()->GetName();
							LocSection = LocBase->GetName() + TEXT(" ") + LocClass->GetName();
						}
					}
					else
					{
						// created at runtime (which hopefully means it's in the transient package)
						LocFilename = LocClass->GetOutermost()->GetName();
						LocSection = LocBase->GetName() + TEXT(" ") + LocClass->GetName();
					}
				}
				else
				{
					checkfSlow(LocBase->GetOuter()->GetOuter(), TEXT("%s marked PerObjectLocalized but is not a subobject or PerObjectConfig (while loading loc data for %s)"), 
						*LocBase->GetFullName(), *GetFullName());

					// case 2 - for instanced subobjects, the prefix is the subobject's name
					// if the subobject is contained within a content package, the filename is the name of the package, and the section is the name of the object's Outer
					// otherwise, the filename is its Outer's class's package name and the section is the Outer's class name
					if ( LocBase->GetOutermost() != UObject::GetTransientPackage() )
					{
						// instanced subobject which lives in a content package
						LocFilename = LocBase->GetOutermost()->GetName();
						LocSection = LocBase->GetOuter()->GetName();
					}
					else
					{
						// instanced subobject created at runtime
						LocFilename = LocBase->GetOuter()->GetClass()->GetOutermost()->GetName();
						LocSection = LocBase->GetOuter()->GetClass()->GetName();
					}
					LocPrefix = LocBase->GetName();
				}
			}
			else
			{
				// case 3 - for normal object instances, the filename is the package containing the class and the section is the name of the class
				LocFilename = LocClass->GetOutermost()->GetName();
				LocSection = LocClass->GetName();
				LocPrefix = TEXT("");
			}
		}
	}

	return bResult;
}

/**
 * Wrapper method for LoadLocalized that is used when reloading localization data for objects at runtime which have already loaded their localization data at least once.
 */
void UObject::ReloadLocalized()
{
	debugfSuppressed(NAME_Localization, TEXT("Reloading localization data for %s"), *GetFullName());
	LoadLocalized(this, TRUE);
}

//
// Save configuration.
//warning: Must be safe on class-default metaobjects.
//!!may benefit from hierarchical propagation, deleting keys that match superclass...not sure what's best yet.
//
void UObject::SaveConfig( QWORD Flags, const TCHAR* InFilename )
{
	if( !GetClass()->HasAnyClassFlags(CLASS_Config) )
	{
		return;
	}


	DWORD PropagationFlags = UE3::LCPF_None;

	const FString Filename
	// if a filename was specified, always load from that file
	=	InFilename
		? InFilename
		: GetConfigFilename(this);

	const UBOOL bPerObject = UsesPerObjectConfig(this);
	FString Section;
	if ( bPerObject == TRUE )
	{
		Section = FString::Printf(TEXT("%s %s"), *GetName(), *GetClass()->GetName());
	}

	for ( UProperty* Property = GetClass()->ConfigLink; Property; Property = Property->ConfigLinkNext )
	{
		if( (Property->PropertyFlags & Flags) == Flags )
		{
			UClass* BaseClass = GetClass();

			if ( (Property->PropertyFlags&CPF_GlobalConfig) != 0 )
			{
				// call LoadConfig() on child classes if any of the properties were global config
				PropagationFlags |= UE3::LCPF_PropagateToChildDefaultObjects;
				BaseClass = Property->GetOwnerClass();
				if ( BaseClass != GetClass() )
				{
					// call LoadConfig() on parent classes only if the global config property was declared in a parent class
					PropagationFlags |= UE3::LCPF_ReadParentSections;
				}
			}

			FString Key				= Property->GetName();
			if ( !bPerObject )
			{
				Section = BaseClass->GetPathName();
			}

			UArrayProperty* Array   = Cast<UArrayProperty>( Property );
			if( Array )
			{
				TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate( *Section, 1, 0, *Filename );
				check(Sec);
				Sec->Remove( *Key );
				FArray* Ptr		= (FArray*)((BYTE*)this + Property->Offset);
				const INT Size	= Array->Inner->ElementSize;
				for( INT i=0; i<Ptr->Num(); i++ )
				{
					FString	Buffer;
					BYTE*	Dest = (BYTE*)Ptr->GetData() + i*Size;
					Array->Inner->ExportTextItem( Buffer, Dest, Dest, this, 0 );
					Sec->Add( *Key, *Buffer );
				}
			}
			else
			{
				UMapProperty* Map = Cast<UMapProperty>( Property );
				if( Map )
				{
					TMultiMap<FString,FString>* Sec = GConfig->GetSectionPrivate( *Section, 1, 0, *Filename );
					check(Sec);
					Sec->Remove( *Key );
					//FArray* Ptr  = (FArray*)((BYTE*)this + Property->Offset);
					//INT     Size = Array->Inner->ElementSize;
					//for( INT i=0; i<Ptr->Num(); i++ )
					//{
					//	TCHAR Buffer[1024]="";
					//	BYTE* Dest = (BYTE*)Ptr->GetData() + i*Size;
					//	Array->Inner->ExportTextItem( Buffer, Dest, Dest, this, 0 );
					//	Sec->Add( Key, Buffer );
					//}
				}
				else
				{
					TCHAR TempKey[MAX_SPRINTF]=TEXT("");
					for( INT Index=0; Index<Property->ArrayDim; Index++ )
					{
						if( Property->ArrayDim!=1 )
						{
							appSprintf( TempKey, TEXT("%s[%i]"), *Property->GetName(), Index );
							Key = TempKey;
						}
						FString	Value;
						Property->ExportText( Index, Value, (BYTE*)this, (BYTE*)this, this, 0 );
						GConfig->SetString( *Section, *Key, *Value, *Filename );
					}
				}
			}
		}
	}

	GConfig->Flush( 0 );
	GetClass()->GetDefaultObject()->LoadConfig( NULL, *Filename, PropagationFlags );
}

/*-----------------------------------------------------------------------------
	Mo Functions.
-----------------------------------------------------------------------------*/

//
// Object accessor.
//
UObject* UObject::GetIndexedObject( INT Index )
{
	if( Index>=0 && Index<GObjObjects.Num() )
		return GObjObjects(Index);
	else
		return NULL;
}

/**
 * Private internal version of StaticFindObjectFast that allows using 0 exclusion flags.
 *
 * @param	ObjectClass		The to be found object's class
 * @param	ObjectPackage	The to be found object's outer
 * @param	ObjectName		The to be found object's class
 * @param	ExactClass		Whether to require an exact match with the passed in class
 * @param	AnyPackage		Whether to look in any package
 * @param	ExclusiveFlags	Ignores objects that contain any of the specified exclusive flags
 * @return	Returns a pointer to the found object or NULL if none could be found
 */
UObject* UObject::StaticFindObjectFastInternal( UClass* ObjectClass, UObject* ObjectPackage, FName ObjectName, UBOOL ExactClass, UBOOL AnyPackage, EObjectFlags ExclusiveFlags )
{
	INC_DWORD_STAT(STAT_FindObjectFast);
	// If they specified an outer use that during the hashing
	if (ObjectPackage != NULL)
	{
		// Find in the specified package using the outer hash
		INT iHash = GetObjectOuterHash(ObjectName,(DWORD)ObjectPackage);
		for( UObject* Hash = GObjHashOuter[iHash]; Hash != NULL; Hash = Hash->HashOuterNext )
		{
			/*
			InName: the object name to search for. Two possibilities.
				A = No dots. ie: 'S_Actor', a texture in Engine
				B = Dots. ie: 'Package.Name' or 'Package.Group.Name', or an even longer chain of outers. The first one needs to be relative to InObjectPackage.
				I'll define InName's package to be everything before the last period, or "" otherwise.
			InObjectPackage: the package or Outer to look for the object in. Can be ANY_PACKAGE, or NULL. Three possibilities:
				A = Non-null. Search for the object relative to this package.
				B = Null. We're looking for the object, as specified exactly in InName.
				C = ANY_PACKAGE. Search anywhere for the package (resrictions on functionality, see below)
			ObjectPackage: The package we need to be searching in. NULL means we don't care what package to search in
				InName.A &&  InObjectPackage.C ==> ObjectPackage = NULL
				InName.A && !InObjectPackage.C ==> ObjectPackage = InObjectPackage
				InName.B &&  InObjectPackage.C ==> ObjectPackage = InName's package
				InName.B && !InObjectPackage.C ==> ObjectPackage = InName's package, but as a subpackage of InObjectPackage
			*/
			if
			(	(Hash->GetFName()==ObjectName)
				/* Don't return objects that have any of the exclusive flags set */
			&&	!Hash->HasAnyFlags(ExclusiveFlags)
				/*If there is no package (no InObjectPackage specified, and InName's package is "")
				and the caller specified any_package, then accept it, regardless of its package.*/
			&&	((!ObjectPackage && AnyPackage)
				/*Or, if the object is directly within this package, (whether that package is non-NULL or not),
				then accept it immediately.*/
			||	(Hash->Outer == ObjectPackage))
			&&	(ObjectClass==NULL || (ExactClass ? Hash->GetClass()==ObjectClass : Hash->IsA(ObjectClass))) )
			{
				checkf( !Hash->HasAnyFlags(RF_Unreachable), TEXT("%s"), *Hash->GetFullName() );
				return Hash;
			}
		}
	}
	else
	{
		// Find in the specified package.
		INT iHash = GetObjectHash( ObjectName );
		for( UObject* Hash=GObjHash[iHash]; Hash!=NULL; Hash=Hash->HashNext )
		{
			/*
			InName: the object name to search for. Two possibilities.
				A = No dots. ie: 'S_Actor', a texture in Engine
				B = Dots. ie: 'Package.Name' or 'Package.Group.Name', or an even longer chain of outers. The first one needs to be relative to InObjectPackage.
				I'll define InName's package to be everything before the last period, or "" otherwise.
			InObjectPackage: the package or Outer to look for the object in. Can be ANY_PACKAGE, or NULL. Three possibilities:
				A = Non-null. Search for the object relative to this package.
				B = Null. We're looking for the object, as specified exactly in InName.
				C = ANY_PACKAGE. Search anywhere for the package (resrictions on functionality, see below)
			ObjectPackage: The package we need to be searching in. NULL means we don't care what package to search in
				InName.A &&  InObjectPackage.C ==> ObjectPackage = NULL
				InName.A && !InObjectPackage.C ==> ObjectPackage = InObjectPackage
				InName.B &&  InObjectPackage.C ==> ObjectPackage = InName's package
				InName.B && !InObjectPackage.C ==> ObjectPackage = InName's package, but as a subpackage of InObjectPackage
			*/
			if
			(	(Hash->GetFName()==ObjectName)
				/* Don't return objects that have any of the exclusive flags set */
			&&	!Hash->HasAnyFlags(ExclusiveFlags)
				/*If there is no package (no InObjectPackage specified, and InName's package is "")
				and the caller specified any_package, then accept it, regardless of its package.*/
			&&	(	(!ObjectPackage && AnyPackage)
				/*Or, if the object is directly within this package, (whether that package is non-NULL or not),
				then accept it immediately.*/
			||	(Hash->Outer == ObjectPackage) )
			&&	(ObjectClass==NULL || (ExactClass ? Hash->GetClass()==ObjectClass : Hash->IsA(ObjectClass))) )
			{
				checkf( !Hash->HasAnyFlags(RF_Unreachable), TEXT("%s"), *Hash->GetFullName() );
				return Hash;
			}
		}
	}
	// Not found.
	return NULL;
}

/**
 * Fast version of StaticFindObject that relies on the passed in FName being the object name
 * without any group/ package qualifiers.
 *
 * @param	ObjectClass		The to be found object's class
 * @param	ObjectPackage	The to be found object's outer
 * @param	ObjectName		The to be found object's class
 * @param	ExactClass		Whether to require an exact match with the passed in class
 * @param	AnyPackage		Whether to look in any package
 * @param	ExclusiveFlags	Ignores objects that contain any of the specified exclusive flags
 * @return	Returns a pointer to the found object or NULL if none could be found
 */
UObject* UObject::StaticFindObjectFast( UClass* ObjectClass, UObject* ObjectPackage, FName ObjectName, UBOOL ExactClass, UBOOL AnyPackage, EObjectFlags ExclusiveFlags )
{
	// We don't want to return any objects that are currently being background loaded unless we're using FindObject during async loading.
	ExclusiveFlags |= GIsAsyncLoading ? 0 : RF_AsyncLoading;
	return StaticFindObjectFastInternal( ObjectClass, ObjectPackage, ObjectName, ExactClass, AnyPackage, ExclusiveFlags );
}

//
// Find an optional object.
//
UObject* UObject::StaticFindObject( UClass* ObjectClass, UObject* InObjectPackage, const TCHAR* OrigInName, UBOOL ExactClass )
{
	INC_DWORD_STAT(STAT_FindObject);
	// Resolve the object and package name.
	UObject* ObjectPackage = InObjectPackage!=ANY_PACKAGE ? InObjectPackage : NULL;
	FString InName = OrigInName;
	if( !ResolveName( ObjectPackage, InName, 0, 0 ) )
	{
		return NULL;
	}

#if GEMINI_TODO
	// If object finding is fast enough, we don't want to do a name find (which it probably usually will be found) 
	// and also an object find, I believe. Woldn't hurt to verify at some point, if this function is a hotspot.

	// Make sure it's an existing name.
	FName ObjectName(*InName,FNAME_Find);
	if( ObjectName==NAME_None )
	{
		return NULL;
	}
#else
	FName ObjectName(*InName, FNAME_Add, TRUE);
#endif

	UObject* MatchingObject	= StaticFindObjectFast( ObjectClass, ObjectPackage, ObjectName, ExactClass, InObjectPackage==ANY_PACKAGE );
	return MatchingObject;
}

//
// Find an object; can't fail.
//
UObject* UObject::StaticFindObjectChecked( UClass* ObjectClass, UObject* ObjectParent, const TCHAR* InName, UBOOL ExactClass )
{
	UObject* Result = StaticFindObject( ObjectClass, ObjectParent, InName, ExactClass );
#if !FINAL_RELEASE
	if( !Result )
	{
		appErrorf( *LocalizeError(TEXT("ObjectNotFound"),TEXT("Core")), *ObjectClass->GetName(), ObjectParent==ANY_PACKAGE ? TEXT("Any") : ObjectParent ? *ObjectParent->GetName() : TEXT("None"), InName );
	}
#endif
	return Result;
}


/**
 * Wrapper for calling UClass::InstanceSubobjectTemplates
 */
void UObject::InstanceSubobjectTemplates( FObjectInstancingGraph* InstanceGraph )
{
	if ( Class->HasAnyClassFlags(CLASS_HasInstancedProps) 
		// don't instance subobjects when compiling script or when instancing is explicitly disabled
	&&	(GUglyHackFlags&HACK_DisableSubobjectInstancing) == 0 )
	{
		Class->InstanceSubobjectTemplates((BYTE*)this, (BYTE*)GetArchetype(), GetArchetype() ? GetArchetype()->GetClass()->GetPropertiesSize() : NULL, this, InstanceGraph);
	}
}

/**
 * Wrapper for calling UClass::InstanceComponentTemplates() for this object.
 */
void UObject::InstanceComponentTemplates( FObjectInstancingGraph* InstanceGraph )
{
	if ( Class->HasAnyClassFlags(CLASS_HasComponents) )
	{
		Class->InstanceComponentTemplates( (BYTE*)this, (BYTE*)GetArchetype(), GetArchetype() ? GetArchetype()->GetClass()->GetPropertiesSize() : NULL, this, InstanceGraph );
	}
}

/**
 * Wrapper function for InitProperties() - initialize this object using the
 * object specified.
 *
 * @param	SourceObject	the object to use for initializing this object
 * @param	InstanceGraph	contains the mappings of instanced objects and components to their templates
 */
void UObject::InitializeProperties( UObject* SourceObject/*=NULL*/, FObjectInstancingGraph* InstanceGraph/*=NULL*/ )
{
	if ( SourceObject == NULL )
	{
		SourceObject = GetArchetype();
	}

	check( SourceObject||Class==UObject::StaticClass() );
	checkSlow(Class==UObject::StaticClass()||IsA(SourceObject->Class));

	UClass* SourceClass = SourceObject ? SourceObject->GetClass() : NULL;

#if 0
	INT Size = GetClass()->GetPropertiesSize();
	INT DefaultsCount = SourceClass ? SourceClass->GetPropertiesSize() : 0;

	// since this is not a new UObject, it's likely that it already has existing values
	// for all of its UProperties.  We must destroy these existing values in a safe way.
	// Failure to do this can result in memory leaks in FArrays.
	ExitProperties( (BYTE*)this, GetClass() );

	InitProperties( (BYTE*)this, Size, SourceClass, (BYTE*)SourceObject, DefaultsCount, this, this, InstanceGraph );
#else

	// Recreate this object based on the new archetype - using StaticConstructObject rather than manually tearing down and re-initializing
	// the properties for this object ensures that any cleanup required when an object is reinitialized from defaults occurs properly
	// for example, when re-initializing UPrimitiveComponents, the component must notify the rendering thread that its data structures are
	// going to be re-initialized
	StaticConstructObject( GetClass(), GetOuter(), GetFName(), GetFlags(), SourceObject, GError, this, InstanceGraph );
#endif
}

/**
 * Sets the ObjectArchetype for this object, optionally reinitializing this object
 * from the new archetype.
 *
 * @param	NewArchetype	the object to change this object's ObjectArchetype to
 * @param	bReinitialize	TRUE if we should the property values should be reinitialized
 *							using the new archetype.
 * @param	InstanceGraph	contains the mappings of instanced objects and components to their templates; only relevant
 *							if bReinitialize is TRUE
 */
void UObject::SetArchetype( UObject* NewArchetype, UBOOL bReinitialize/*=FALSE*/, FObjectInstancingGraph* InstanceGraph/*=NULL*/ )
{
	check(NewArchetype);
	check(NewArchetype != this);

	ObjectArchetype = NewArchetype;
	if ( bReinitialize )
	{
		InitializeProperties(NULL, InstanceGraph);
	}
}

/**
 * Wrapper for InitProperties which calls ExitProperties first if this object has already had InitProperties called on it at least once.
 */
void UObject::SafeInitProperties( BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* DefaultData, INT DefaultsCount, UObject* DestObject/*=NULL*/, UObject* SubobjectRoot/*=NULL*/, FObjectInstancingGraph* InstanceGraph/*=NULL*/ )
{
	if ( HasAnyFlags(RF_InitializedProps) )
	{
		// since this is not a new UObject, it's likely that it already has existing values
		// for all of its UProperties.  We must destroy these existing values in a safe way.
		// Failure to do this can result in memory leaks in FArrays.
		ExitProperties( Data, GetClass() );
	}

	SetFlags(RF_InitializedProps);
	InitProperties(Data, DataCount, DefaultsClass, DefaultData, DefaultsCount, DestObject, SubobjectRoot, InstanceGraph);
}

/**
 * Binary initialize object properties to zero or defaults.
 *
 * @param	Data				the data that needs to be initialized
 * @param	DataCount			the size of the buffer pointed to by Data
 * @param	DefaultsClass		the class to use for initializing the data
 * @param	DefaultData			the buffer containing the source data for the initialization
 * @param	DefaultsCount		the size of the buffer pointed to by DefaultData
 * @param	DestObject			if non-NULL, corresponds to the object that is located at Data
 * @param	SubobjectRoot
 *						Only used to when duplicating or instancing objects; in a nested subobject chain, corresponds to the first object in DestObject's Outer chain that is not a subobject (including DestObject).
 *						A value of INVALID_OBJECT for this parameter indicates that we are calling StaticConstructObject to duplicate or instance a non-subobject (which will be the subobject root for any subobjects of the new object)
 *						A value of NULL indicates that we are not instancing or duplicating an object.
 * @param	InstanceGraph
 *						contains the mappings of instanced objects and components to their templates
 */
void UObject::InitProperties( BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* DefaultData, INT DefaultsCount, UObject* DestObject/*=NULL*/, UObject* SubobjectRoot/*=NULL*/, FObjectInstancingGraph* InstanceGraph/*=NULL*/ )
{
	SCOPE_CYCLE_COUNTER(STAT_InitProperties);
	check( !DefaultsClass || !DefaultsClass->GetMinAlignment() || Align(DataCount, DefaultsClass->GetMinAlignment()) >= sizeof(UObject) );

#if PS3
	// Inited needs to be the PropertiesSize of UObject (not sizeof(UObject) which may get padded out on some platforms)
	// , but we may not have it here yet. So, we get the properties size by taking the offset of the last property and 
	// adding the size of it.
	// @todo: Do this for all platforms (currently PS3 only so as not to require another full QA pass for PC/Xbox)
	INT Inited = STRUCT_OFFSET(UObject, ObjectArchetype) + sizeof(UObject*);
#else
	INT Inited = sizeof(UObject);
#endif

	// Find class defaults if no template was specified.
	//warning: At startup, DefaultsClass->Defaults.Num() will be zero for some native classes.
	if( !DefaultData && DefaultsClass && DefaultsClass->GetDefaultsCount() )
	{
		DefaultData   = DefaultsClass->GetDefaults();
		DefaultsCount = DefaultsClass->GetDefaultsCount();
	}

	// Copy defaults appended after the UObject variables.
	if( DefaultData && DefaultsCount > Inited )
	{
		checkSlow(DefaultsCount>=Inited);
		checkSlow(DefaultsCount<=DataCount);
		appMemcpy( Data+Inited, DefaultData+Inited, DefaultsCount-Inited );
		Inited = DefaultsCount;
	}

	// Zero-fill any remaining portion. (moved to StaticAllocateObject to support
	// reinitializing objects that have already been initialized from a stable state
	// at least once.
	//@fixme - 
	// probably need to make this a parameter, so that we can clear existing values
	// that for properties declared in the target class, when reinitializing objects
//	if( Inited < DataCount )
//		appMemzero( Data+Inited, DataCount-Inited );

	// if SubobjectRoot is INVALID_OBJECT, it means we are instancing or duplicating a root object (non-subobject).  In this case,
	// we'll want to use DestObject as the SubobjectRoot for instancing any components/subobjects contained by this object.
	if ( SubobjectRoot == INVALID_OBJECT )
	{
		SubobjectRoot = DestObject;
	}

	if( DefaultsClass && SubobjectRoot )
	{
		// This is a duplicate. The value for all transient or non-duplicatable properties should be copied
		// from the source class's defaults.
		checkSlow(DestObject);
		BYTE* ClassDefaults = DefaultsClass->GetDefaults();		
		UProperty* P=NULL;
		for( P=DestObject->GetClass()->TransientPropertyLink; P; P=P->TransientPropertyLinkNext )
		{		
			// Bools are packed bitfields which might contain both transient and non- transient members so we can't use memcpy.
			if( Cast<UBoolProperty>(P,CLASS_IsAUBoolProperty) )
			{
				P->CopyCompleteValue(Data + P->Offset, ClassDefaults + P->Offset, NULL);
			}
			else if( (P->PropertyFlags&CPF_NeedCtorLink) == 0 )
			{
				// if this property's value doesn't use dynamically allocated memory, just block copy
				appMemcpy( Data + P->Offset, ClassDefaults + P->Offset, P->ArrayDim * P->ElementSize );
			}
			else
			{
				// the heap memory allocated at this address is owned by the parent class, so
				// zero out the existing data so that the UProperty code doesn't attempt to
				// de-allocate it before allocating the memory for the new value
				appMemzero( Data + P->Offset, P->GetSize() );
				P->CopyCompleteValue( Data + P->Offset, ClassDefaults + P->Offset, SubobjectRoot, DestObject, InstanceGraph );
			}
		}
	}

	// Construct anything required.
	if( DefaultsClass && DefaultData )
	{
		for( UProperty* P=DefaultsClass->ConstructorLink; P; P=P->ConstructorLinkNext )
		{
			if( P->Offset < DefaultsCount )
			{
				// skip if SourceOwnerObject != NULL and this is a transient property - in this
				// situation, the new value for the property has already been copied from the class defaults
				// in the block of code above
				DWORD PropertyFlagMask = P->PropertyFlags&(CPF_Transient|CPF_DuplicateTransient);
				if ( SubobjectRoot == NULL || PropertyFlagMask == 0 ) //@todo xenon: fix up with May XDK
				{
					// the heap memory allocated at this address is owned the source object, so
					// zero out the existing data so that the UProperty code doesn't attempt to
					// de-allocate it before allocating the memory for the new value
					appMemzero( Data + P->Offset, P->GetSize() );//bad for bools, but not a real problem because they aren't constructed!!
					P->CopyCompleteValue( Data + P->Offset, DefaultData + P->Offset, SubobjectRoot ? SubobjectRoot : DestObject, DestObject, InstanceGraph );
				}
			}
		}
	}
}

//
// Destroy properties.
//
void UObject::ExitProperties( BYTE* Data, UClass* Class )
{
	UProperty* P=NULL;
	for( P=Class->ConstructorLink; P; P=P->ConstructorLinkNext )
	{
		// Only destroy values of properties which have been loaded completely.
		// This can be encountered by loading a package in ucc make which references a class which has already been compiled and saved.
		// The class is already in memory, so when the class is loaded from the package on disk, it reuses the memory address of the existing class with the same name.
		// Before reusing the memory address, it destructs the default properties of the existing class.
		// However, at this point, the properties may have been reallocated but not deserialized, leaving them in an invalid state.
		if(!P->HasAnyFlags(RF_NeedLoad))
		{
			P->DestroyValue( Data + P->Offset );
		}
		else
		{
			check(GIsUCC);
		}
	}
}

/**
 * Initializes the properties for this object based on the property values of the
 * specified class's default object
 *
 * @param	InClass		the class to use for initializing this object
 * @param	SetOuter	TRUE if the Outer for this object should be changed
 * @param	bPseudoObject	TRUE if 'this' does not point to a real UObject.  Used when
 *							treating an arbitrary block of memory as a UObject.  Specifying
 *							TRUE for this parameter has the following side-effects:
 *							- vtable for this UObject is copied from the specified class
 *							- sets the Class for this object to the specified class
 *							- sets the Index property of this UObject to INDEX_NONE
 */
void UObject::InitClassDefaultObject( UClass* InClass, UBOOL SetOuter, UBOOL bPseudoObject )
{
	if ( bPseudoObject )
	{
		// Init UObject portion.
		appMemset( this, 0, sizeof(UObject) );
		*(void**)this = *(void**)InClass;
		Class         = InClass;
		Index         = INDEX_NONE;
	}

	// Init post-UObject portion.
	if( SetOuter )
		Outer = InClass->GetOuter();
	SafeInitProperties( (BYTE*)this, InClass->GetPropertiesSize(), InClass->GetSuperClass(), NULL, 0, SetOuter ? this : NULL );
}

/**
 * Returns a pointer to this object safely converted to a pointer to the specified interface class.
 *
 * @param	InterfaceClass	the interface class to use for the returned type
 *
 * @return	a pointer that can be assigned to a variable of the interface type specified, or NULL if this object's
 *			class doesn't implement the interface indicated.  Will be the same value as 'this' if the interface class
 *			isn't native.
 */
void* UObject::GetInterfaceAddress( UClass* InterfaceClass )
{
	void* Result = NULL;

	if ( InterfaceClass != NULL && InterfaceClass->HasAnyClassFlags(CLASS_Interface) && InterfaceClass != UInterface::StaticClass() )
	{
		if ( !InterfaceClass->HasAnyClassFlags(CLASS_Native) )
		{
			if ( GetClass()->ImplementsInterface(InterfaceClass) )
			{
				// if it isn't a native interface, the address won't be different
				Result = this;
			}
		}
		else
		{
			for(UClass* CurrentClass=GetClass();CurrentClass;CurrentClass=CurrentClass->GetSuperClass())
			{
				for ( TMap<UClass*,UProperty*>::TIterator It(CurrentClass->Interfaces); It; ++It )
				{
					UClass* ImplementedInterfaceClass = It.Key();
					if ( ImplementedInterfaceClass == InterfaceClass || ImplementedInterfaceClass->IsChildOf(InterfaceClass) )
					{
						UProperty* VfTableProperty = It.Value();
						if ( VfTableProperty != NULL )
						{
							checkSlow(VfTableProperty->ArrayDim == 1);
							Result = (BYTE*)this + VfTableProperty->Offset;
							break;
						}
						else
						{
							// if it isn't a native interface, the address won't be different
							Result = this;
							break;
						}
					}
				}
			}
		}
	}

	return Result;
}

//
// Global property setting.
//
void UObject::GlobalSetProperty( const TCHAR* Value, UClass* Class, UProperty* Property, INT Offset, UBOOL bNotifyObjectOfChange )
{
	// Apply to existing objects of the class.
	for( FObjectIterator It; It; ++It )
	{	
		UObject* Object = *It;
		if( Object->IsA(Class) && !Object->IsPendingKill() )
		{
			if( !Object->HasAnyFlags(RF_ClassDefaultObject) && bNotifyObjectOfChange )
			{
				Object->PreEditChange(Property);
			}
			Property->ImportText( Value, (BYTE*)Object + Offset, PPF_Localized, Object );
			if( !Object->HasAnyFlags(RF_ClassDefaultObject) && bNotifyObjectOfChange )
			{
				Object->PostEditChange(Property);
			}
		}
	}

	// Apply to defaults.
	UObject* DefaultObject = Class->GetDefaultObject();
	check(DefaultObject != NULL);
	DefaultObject->SaveConfig();
}

/*-----------------------------------------------------------------------------
	Object registration.
-----------------------------------------------------------------------------*/

//
// Preregister an object.
//warning: Sometimes called at startup time.
//
void UObject::Register()
{
	check(GObjInitialized);

	// Get stashed registration info.
	const TCHAR* InOuter = *(const TCHAR**)&Outer;
	const TCHAR* InName  = (const TCHAR*)appDWORDToPointer(*(DWORD*)&Name);

	// Set object properties.
	Outer        = CreatePackage(NULL,InOuter);
	Name         = InName;
	_LinkerIndex = (PTRINT)INDEX_NONE;
	NetIndex = INDEX_NONE;

	// Validate the object.
	if( Outer==NULL )
		appErrorf( TEXT("Autoregistered object %s is unpackaged"), *GetFullName() );
	if( GetFName()==NAME_None )
		appErrorf( TEXT("Autoregistered object %s has invalid name"), *GetFullName() );
	if( StaticFindObject( NULL, GetOuter(), *GetName() ) )
		appErrorf( TEXT("Autoregistered object %s already exists"), *GetFullName() );

	// Add to the global object table.
	AddObject( INDEX_NONE );
}

//
// Handle language change.
//
void UObject::LanguageChange()
{
	LoadLocalized();
}

/*-----------------------------------------------------------------------------
	StaticInit & StaticExit.
-----------------------------------------------------------------------------*/

void SerTest( FArchive& Ar, DWORD& Value, DWORD Max )
{
	DWORD NewValue=0;
	for( DWORD Mask=1; NewValue+Mask<Max && Mask; Mask*=2 )
	{
		BYTE B = ((Value&Mask)!=0);
		Ar.SerializeBits( &B, 1 );
		if( B )
			NewValue |= Mask;
	}
	Value = NewValue;
}

//
// Init the object manager and allocate tables.
//
void UObject::StaticInit()
{
	GObjNoRegister = 1;

	// Checks.
	check(sizeof(BYTE)==1);
	check(sizeof(SBYTE)==1);
	check(sizeof(WORD)==2);
	check(sizeof(DWORD)==4);
	check(sizeof(QWORD)==8);
	check(sizeof(ANSICHAR)==1);
	check(sizeof(UNICHAR)==2);
	check(sizeof(SWORD)==2);
	check(sizeof(INT)==4);
	check(sizeof(SQWORD)==8);
	check(sizeof(UBOOL)==4);
	check(sizeof(FLOAT)==4);
	check(sizeof(DOUBLE)==8);
	check(GEngineMinNetVersion<=GEngineVersion);
	check(STRUCT_OFFSET(UObject,ObjectFlags)==Align(STRUCT_OFFSET(UObject,ObjectFlags),8));

	// Zero initialize and later on get value from .ini so it is overridable per game/ platform...
	
	INT MaxObjectsNotConsideredByGC	= 0;  // To set this look in your log:  ( e.g. Log: ##### objects alive at end of initial load. )  This is set from LaunchEnglineLoop after objects have been added to the root set. 
	INT SizeOfPermanentObjectPool	= 0;

	// ... unless we're running the Editor, in which case we skip this optimization.
	if( !GIsEditor )
	{
		GConfig->GetInt( TEXT("Core.System"), TEXT("MaxObjectsNotConsideredByGC"), MaxObjectsNotConsideredByGC, GEngineIni );
		GConfig->GetInt( TEXT("Core.System"), TEXT("SizeOfPermanentObjectPool"), SizeOfPermanentObjectPool, GEngineIni );
	}

	// GObjFirstGCIndex is the index at which the garbage collector will start for the mark phase.
	GObjFirstGCIndex			= MaxObjectsNotConsideredByGC;
	GPermanentObjectPoolSize	= SizeOfPermanentObjectPool;
	GPermanentObjectPool		= (BYTE*) appMalloc( GPermanentObjectPoolSize );
	GPermanentObjectPoolTail	= GPermanentObjectPool;

	// Presize array.
	check( GObjObjects.Num() == 0 );
	if( GObjFirstGCIndex )
	{
		GObjObjects.AddZeroed( GObjFirstGCIndex );
	}

	// Init hash.
	appMemzero(GObjHash,sizeof(UObject*) * OBJECT_HASH_BINS);
	appMemzero(GObjHashOuter,sizeof(UObject*) * OBJECT_HASH_BINS);

	// If statically linked, initialize registrants.
	INT Lookup = 0; // Dummy required by AUTO_INITIALIZE_REGISTRANTS_CORE
	extern void AutoInitializeRegistrantsCore( INT& Lookup );
	AutoInitializeRegistrantsCore( Lookup );

	// Note initialized.
	GObjInitialized = 1;

	// Add all autoregistered classes.
	ProcessRegistrants();

	// Allocate special packages.
	GObjTransientPkg = new( NULL, TEXT("Transient") )UPackage;
	GObjTransientPkg->AddToRoot();

	debugf( NAME_Init, TEXT("Object subsystem initialized") );
}

//
// Process all objects that have registered.
//
void UObject::ProcessRegistrants()
{
	GObjRegisterCount++;
	TArray<UObject*>	ObjRegistrants;
	// Make list of all objects to be registered.
	for( ; GAutoRegister; GAutoRegister=*(UObject **)&GAutoRegister->_LinkerIndex )
		ObjRegistrants.AddItem( GAutoRegister );
	for( INT i=0; i<ObjRegistrants.Num(); i++ )
	{
		ObjRegistrants(i)->ConditionalRegister();
		for( ; GAutoRegister; GAutoRegister=*(UObject **)&GAutoRegister->_LinkerIndex )
			ObjRegistrants.AddItem( GAutoRegister );
	}
	ObjRegistrants.Empty();
	check(!GAutoRegister);
	--GObjRegisterCount;
}

//
// Shut down the object manager.
//
void UObject::StaticExit()
{
	check(GObjLoaded.Num()==0);
	check(GObjRegistrants.Num()==0);
	check(!GAutoRegister);

	// Cleanup root.
	GObjTransientPkg->RemoveFromRoot();

	// Make sure any pending purge has finished before changing RF_Unreachable and don't use time limit.
	if( GObjIncrementalPurgeIsInProgress )
	{
		IncrementalPurgeGarbage( FALSE );
	}

	// We need to manually keep track of objects we mark unreachable as object iterators are going to
	// ignore them once marked.
	TArray<UObject*> UnreachableObjects;

	// Tag all objects as unreachable.
	for( FObjectIterator It; It; ++It )
	{
		UObject* Object = *It;
		// Mark as unreachable so purge phase will kill it.
		Object->SetFlags( RF_Unreachable );
		// Add to list of unreachable objects.
		UnreachableObjects.AddItem( Object );
	}

	// Route BeginDestroy. This needs to be a separate pass from marking as RF_Unreachable
	// as code might rely on RF_Unreachable to be set on all objects that are about to be
	// deleted. One example is ULinkerLoad detaching textures - the SetLinker call needs 
	// to not kick off texture streaming.
	for( INT i=0; i<UnreachableObjects.Num(); i++ )
	{
		UObject* Object = UnreachableObjects(i);
		// Begin the object's asynchronous destruction.
		Object->ConditionalBeginDestroy();
	}

	// Fully purge all objects, not using time limit.
	GExitPurge			= TRUE;
 	GObjPurgeIsRequired = TRUE;
	IncrementalPurgeGarbage( FALSE );

	// Empty arrays to prevent falsely-reported memory leaks.
	GObjLoaded			.Empty();
	GObjObjects			.Empty();
	GObjAvailable		.Empty();
	GObjLoaders			.Empty();
	GObjRegistrants		.Empty();
	GObjAsyncPackages	.Empty();

	GObjInitialized = 0;
	debugf( NAME_Exit, TEXT("Object subsystem successfully closed.") );
}
	
/*-----------------------------------------------------------------------------
	UObject Tick.
-----------------------------------------------------------------------------*/

/**
 * Static UObject tick function, used to verify certain key assumptions and to tick the async loading code.
 *
 * @warning: The streaming stats rely on this function not doing any work besides calling ProcessAsyncLoading.
 * @todo: Move stats code into core?
 *
 * @param DeltaTime	Time in seconds since last call
 */
void UObject::StaticTick( FLOAT DeltaTime )
{
	check(GObjBeginLoadCount==0);

	// Spend a bit of time (pre)loading packages - currently 5 ms.
	ProcessAsyncLoading( TRUE, 0.005f );

	// Check natives.
	extern int GNativeDuplicate;
	if( GNativeDuplicate )
	{
		appErrorf( TEXT("Duplicate native registered: %i"), GNativeDuplicate );
	}
	// Check for duplicates.
	extern int GCastDuplicate;
	if( GCastDuplicate )
	{
		appErrorf( TEXT("Duplicate cast registered: %i"), GCastDuplicate );
	}

#if STATS
	// Set name table stats.
	SET_DWORD_STAT( STAT_NameTableEntries, FName::GetMaxNames() );
	INT NameTableMemorySize = FName::GetNameEntryMemorySize() + FName::GetMaxNames() * sizeof(FNameEntry*);
	SET_DWORD_STAT( STAT_NameTableMemorySize, NameTableMemorySize );
#endif
}

/*-----------------------------------------------------------------------------
   Shutdown.
-----------------------------------------------------------------------------*/

//
// Make sure this object has been shut down.
//
void UObject::ConditionalShutdownAfterError()
{
	if( !HasAnyFlags(RF_ErrorShutdown) )
	{
		SetFlags( RF_ErrorShutdown );
#if !EXCEPTIONS_DISABLED
		try
#endif
		{
			ShutdownAfterError();
		}
#if !EXCEPTIONS_DISABLED
		catch( ... )
		{
			debugf( NAME_Exit, TEXT("Double fault in object ShutdownAfterError") );
		}
#endif
	}
}

//
// After a critical error, shutdown all objects which require
// mission-critical cleanup, such as restoring the video mode,
// releasing hardware resources.
//
void UObject::StaticShutdownAfterError()
{
#if PS3 // @todo hack - break here so GDB has a prayer of helping
	appDebugBreak();
#endif
	if( GObjInitialized )
	{
		static UBOOL Shutdown=0;
		if( Shutdown )
			return;
		Shutdown = 1;
		debugf( NAME_Exit, TEXT("Executing UObject::StaticShutdownAfterError") );
#if !EXCEPTIONS_DISABLED
		try
#endif
		{
			for( INT i=0; i<GObjObjects.Num(); i++ )
				if( GObjObjects(i) )
					GObjObjects(i)->ConditionalShutdownAfterError();
		}
#if !EXCEPTIONS_DISABLED
		catch( ... )
		{
			debugf( NAME_Exit, TEXT("Double fault in object manager ShutdownAfterError") );
		}
#endif
	}
}

//
// Bind package to DLL.
//warning: Must only find packages in the \Unreal\System directory!
//
void UObject::BindPackage( UPackage* Package )
{
	if( !Package->IsBound() && !Package->GetOuter())
	{
		Package->SetBound( TRUE );
		GObjNoRegister = 0;
		GObjNoRegister = 1;
		ProcessRegistrants();
	}
}

/*-----------------------------------------------------------------------------
   Command line.
-----------------------------------------------------------------------------*/
//
// Show the inheretance graph of all loaded classes.
//
static void ShowClasses( UClass* Class, FOutputDevice& Ar, int Indent )
{
	Ar.Logf( TEXT("%s%s"), appSpc(Indent), *Class->GetName() );
	for( TObjectIterator<UClass> It; It; ++It )
		if( It->GetSuperClass() == Class )
			ShowClasses( *It, Ar, Indent+2 );
}

struct FItem
{
	UClass*	Class;
	INT		Count;
	SIZE_T	Num, Max, Res;
	FItem( UClass* InClass=NULL )
	: Class(InClass), Count(0), Num(0), Max(0), Res(0)
	{}
	void Add( FArchiveCountMem& Ar, SIZE_T InRes )
	{Count++; Num+=Ar.GetNum(); Max+=Ar.GetMax(); Res+=InRes; }
};
struct FSubItem
{
	UObject* Object;
	SIZE_T Num, Max, Res;
	FSubItem( UObject* InObject, SIZE_T InNum, SIZE_T InMax, SIZE_T InRes )
	: Object( InObject ), Num( InNum ), Max( InMax ), Res( InRes )
	{}
};

static UBOOL bAlphaSort = FALSE;
static UBOOL bCountSort = FALSE;
IMPLEMENT_COMPARE_CONSTREF( FSubItem, UnObj, { return bAlphaSort ? appStricmp(*A.Object->GetPathName(),*B.Object->GetPathName()) : B.Max - A.Max; } );
IMPLEMENT_COMPARE_CONSTREF( FItem, UnObj, { return bAlphaSort ? appStricmp(*A.Class->GetName(),*B.Class->GetName()) : bCountSort ? B.Count - A.Count : B.Max - A.Max; } );

/** Utility for outputting as text to the supplied device object currently referencing this object. */
void UObject::OutputReferencers( FOutputDevice& Ar, UBOOL bIncludeTransients )
{
	Ar.Logf( TEXT("\n") );
	Ar.Logf( TEXT("Referencers of %s:\r\n"), *GetFullName() );
	for( FObjectIterator It; It; ++It )
	{
		UObject* Object = *It;

		if ( Object == this )
		{
			// this one is pretty easy  :)
			continue;
		}

		FArchiveFindCulprit ArFind(this,Object,false);
		TArray<UProperty*> Referencers;

		INT Count = ArFind.GetCount(Referencers);
		if ( Count > 0 )
		{
			Ar.Logf( TEXT("   %s (%i)\r\n"), *Object->GetFullName(), Count );
			for ( INT i = 0; i < Referencers.Num(); i++ )
			{
				UObject* Referencer = Referencers(i);
				Ar.Logf(TEXT("      %i) %s\r\n"), i, *Referencer->GetFullName());
			}
		}
	}
	Ar.Logf(TEXT("\r\n") );

#if !FINAL_RELEASE
	// this almost always causes a stack overflow on consoles - if you really want to see the shortest route info, increase your stack size locally
	Ar.Logf(TEXT("Shortest reachability from root to %s:\r\n"), *GetFullName() );
	TMap<UObject*,UProperty*> Rt = FArchiveTraceRoute::FindShortestRootPath(this,bIncludeTransients,GARBAGE_COLLECTION_KEEPFLAGS);

	FString RootPath = FArchiveTraceRoute::PrintRootPath(Rt, this);
	Ar.Log(*RootPath);

	Ar.Logf(TEXT("\r\n") );
#endif
}

/**
 * Maps object flag to human-readable string.
 */
class FObjectFlag
{
public:
	EObjectFlags	ObjectFlag;
	const TCHAR*	FlagName;
	FObjectFlag(EObjectFlags InObjectFlag, const TCHAR* InFlagName)
		:	ObjectFlag( InObjectFlag )
		,	FlagName( InFlagName )
	{}
};

/**
 * Initializes the singleton list of object flags.
 */
static TArray<FObjectFlag> PrivateInitObjectFlagList()
{
	TArray<FObjectFlag> ObjectFlagList;
#ifdef	DECLARE_OBJECT_FLAG
#error DECLARE_OBJECT_FLAG already defined
#else
#define DECLARE_OBJECT_FLAG( ObjectFlag ) ObjectFlagList.AddItem( FObjectFlag( RF_##ObjectFlag, TEXT(#ObjectFlag) ) );
	DECLARE_OBJECT_FLAG( InSingularFunc )
	DECLARE_OBJECT_FLAG( StateChanged )
	DECLARE_OBJECT_FLAG( DebugPostLoad	)
	DECLARE_OBJECT_FLAG( DebugSerialize )
	DECLARE_OBJECT_FLAG( DebugBeginDestroyed )
	DECLARE_OBJECT_FLAG( DebugFinishDestroyed )
	DECLARE_OBJECT_FLAG( EdSelected )
	DECLARE_OBJECT_FLAG( ZombieComponent )
	DECLARE_OBJECT_FLAG( Protected )
	DECLARE_OBJECT_FLAG( ClassDefaultObject )
	DECLARE_OBJECT_FLAG( ArchetypeObject )
	DECLARE_OBJECT_FLAG( ForceTagExp )
	DECLARE_OBJECT_FLAG( TokenStreamAssembled )
	DECLARE_OBJECT_FLAG( MisalignedObject )
	DECLARE_OBJECT_FLAG( Transactional )
	DECLARE_OBJECT_FLAG( Unreachable )
	DECLARE_OBJECT_FLAG( Public	)
	DECLARE_OBJECT_FLAG( TagImp	)
	DECLARE_OBJECT_FLAG( TagExp )
	DECLARE_OBJECT_FLAG( Obsolete )
	DECLARE_OBJECT_FLAG( TagGarbage )
	DECLARE_OBJECT_FLAG( DisregardForGC )
	DECLARE_OBJECT_FLAG( PerObjectLocalized )
	DECLARE_OBJECT_FLAG( NeedLoad )
	DECLARE_OBJECT_FLAG( AsyncLoading )
	DECLARE_OBJECT_FLAG( Suppress )
	DECLARE_OBJECT_FLAG( InEndState )
	DECLARE_OBJECT_FLAG( Transient )
	DECLARE_OBJECT_FLAG( Cooked )
	DECLARE_OBJECT_FLAG( LoadForClient )
	DECLARE_OBJECT_FLAG( LoadForServer )
	DECLARE_OBJECT_FLAG( LoadForEdit	)
	DECLARE_OBJECT_FLAG( Standalone )
	DECLARE_OBJECT_FLAG( NotForClient )
	DECLARE_OBJECT_FLAG( NotForServer )
	DECLARE_OBJECT_FLAG( NotForEdit )
	DECLARE_OBJECT_FLAG( RootSet )
	DECLARE_OBJECT_FLAG( BeginDestroyed )
	DECLARE_OBJECT_FLAG( FinishDestroyed )
	DECLARE_OBJECT_FLAG( NeedPostLoad )
	DECLARE_OBJECT_FLAG( HasStack )
	DECLARE_OBJECT_FLAG( Native )
	DECLARE_OBJECT_FLAG( Marked )
	DECLARE_OBJECT_FLAG( ErrorShutdown )
	DECLARE_OBJECT_FLAG( PendingKill	)
#undef DECLARE_OBJECT_FLAG
#endif
	return ObjectFlagList;
}
/**
 * Dumps object flags from the selected objects to debugf.
 */
static void PrivateDumpObjectFlags(UObject* Object, FOutputDevice& Ar)
{
	static TArray<FObjectFlag> SObjectFlagList = PrivateInitObjectFlagList();

	if ( Object )
	{
		FString Buf( FString::Printf( TEXT("%s:\t"), *Object->GetFullName() ) );
		for ( INT FlagIndex = 0 ; FlagIndex < SObjectFlagList.Num() ; ++FlagIndex )
		{
			const FObjectFlag& CurFlag = SObjectFlagList( FlagIndex );
			if ( Object->HasAnyFlags( CurFlag.ObjectFlag ) )
			{
				Buf += FString::Printf( TEXT("%s "), CurFlag.FlagName );
			}
		}
		Ar.Logf( TEXT("%s"), *Buf );
	}
}

/**
 * Recursively visits all object properties and dumps object flags.
 */
static void PrivateRecursiveDumpFlags(UStruct* Struct, BYTE* Data, FOutputDevice& Ar)
{
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Struct); It; ++It )
	{
		for( INT i=0; i<It->ArrayDim; i++ )
		{
			BYTE* Value = Data + It->Offset + i*It->ElementSize;
			if( Cast<UObjectProperty>(*It) )
			{
				UObject*& Obj = *(UObject**)Value;
				PrivateDumpObjectFlags( Obj, Ar );
			}
			else if( Cast<UStructProperty>(*It, CLASS_IsAUStructProperty) )
			{
				PrivateRecursiveDumpFlags( ((UStructProperty*)*It)->Struct, Value, Ar );
			}
		}
	}
}

/** 
 * Performs the work for "SET" and "SETNOPEC".
 *
 * @param	Str						reset of console command arguments
 * @param	Ar						output device to use for logging
 * @param	bNotifyObjectOfChange	whether to notify the object about to be changed via Pre/PostEditChange
 */
static void PerformSetCommand( const TCHAR* Str, FOutputDevice& Ar, UBOOL bNotifyObjectOfChange )
{
	// Set a class default variable.
	TCHAR ClassName[256], PropertyName[256];
	UClass* Class;
	UProperty* Property;
	if
	(	ParseToken( Str, ClassName, ARRAY_COUNT(ClassName), 1 )
	&&	(Class=FindObject<UClass>( ANY_PACKAGE, ClassName))!=NULL )
	{
		if
		(	ParseToken( Str, PropertyName, ARRAY_COUNT(PropertyName), 1 )
		&&	(Property=FindField<UProperty>( Class, PropertyName))!=NULL )
		{
			while( *Str==' ' )
			{
				Str++;
			}
			UObject::GlobalSetProperty( Str, Class, Property, Property->Offset, bNotifyObjectOfChange );
		}
		else
		{
			Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized property %s"), PropertyName );
		}
	}
	else 
	{
		Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized class %s"), ClassName );
	}
}

extern void FlushRenderingCommands();

UBOOL UObject::StaticExec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	const TCHAR *Str = Cmd;
	if( ParseCommand(&Str,TEXT("MEM")) )
	{
		const FString Token = ParseToken( Str, 0 );
		const UBOOL bSummaryOnly = ( Token != TEXT("STAT") );
		const UBOOL bDetailed	 = ( Token == TEXT("DETAILED") );	

		FlushAsyncLoading();
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS,TRUE);
		FlushRenderingCommands();

		if( !bSummaryOnly || bDetailed )
		{
			if( bSummaryOnly )
			{
				GMalloc->Exec( TEXT("DUMPALLOCS SUMMARYONLY"), Ar );
			}
			else
			{
				GMalloc->Exec( TEXT("DUMPALLOCS"), Ar );
			}
			Ar.Logf( TEXT("GMem allocation size [used/ unused] = [%i / %i] KByte"), GMem.GetByteCount() / 1024, GMem.GetUnusedByteCount() / 1024 );
		}

		// print out some mem stats
		GConfig->ShowMemoryUsage(Ar);


		Ar.Logf(TEXT(""));
#if PS3
		appPS3DumpDetailedMemoryInformation(Ar);
#else

		SIZE_T VirtualAllocationSize	= 0;
		SIZE_T PhysicalAllocationSize	= 0;
		GMalloc->GetAllocationInfo( VirtualAllocationSize, PhysicalAllocationSize );

		Ar.Logf( TEXT("Virtual  memory allocation size: %6.2f MByte (%i Bytes)"), VirtualAllocationSize  / 1024.f / 1024.f, VirtualAllocationSize );
		Ar.Logf( TEXT("Physical memory allocation size: %6.2f MByte (%i Bytes)"), PhysicalAllocationSize / 1024.f / 1024.f, PhysicalAllocationSize );
#endif
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("SETTRACKINGBASELINE")))
	{
		FlushAsyncLoading();
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS,TRUE);
		FlushRenderingCommands();

		return GMalloc->Exec(Cmd,Ar);
	}
	else if( ParseCommand(&Str,TEXT("DUMPNATIVES")) )
	{
#if _MSC_VER
		// ISO C++ forbids taking the address of a bound member function to form a pointer to member function.  Say `&UObject::execUndefined'.
		for( INT i=0; i<EX_Max; i++ )
			if( GNatives[i] == &execUndefined )
				debugf( TEXT("Native index %i is available"), i );
#endif
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("GET")) )
	{
		// Get a class default variable.
		TCHAR ClassName[256], PropertyName[256];
		UClass* Class;
		UProperty* Property;
		if
		(	ParseToken( Str, ClassName, ARRAY_COUNT(ClassName), 1 )
		&&	(Class=FindObject<UClass>( ANY_PACKAGE, ClassName))!=NULL )
		{
			if
			(	ParseToken( Str, PropertyName, ARRAY_COUNT(PropertyName), 1 )
			&&	(Property=FindField<UProperty>( Class, PropertyName))!=NULL )
			{
				FString	Temp;
				if( Class->GetDefaultsCount() )
					Property->ExportText( 0, Temp, Class->GetDefaults(), Class->GetDefaults(), Class, PPF_Localized );
				Ar.Log( *Temp );
			}
			else Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized property %s"), PropertyName );
		}
		else Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized class %s"), ClassName );
		return 1;
	}
	else if (ParseCommand(&Str, TEXT("GETALL")))
	{
		// iterate through all objects of the specified type and return the value of the specified property for each object
		TCHAR ClassName[256], PropertyName[256];
		UClass* Class;
		UProperty* Property;

		if ( ParseToken(Str,ClassName,ARRAY_COUNT(ClassName), 1) &&
			(Class=FindObject<UClass>( ANY_PACKAGE, ClassName)) != NULL )
		{
			if ( ParseToken(Str,PropertyName,ARRAY_COUNT(PropertyName),1) &&
				(Property=FindField<UProperty>(Class,PropertyName)) != NULL )
			{
				INT cnt = 0;
				UObject* LimitOuter = NULL;

				ParseObject<UObject>(Str,TEXT("OUTER="),LimitOuter,NULL);

				UBOOL bShowDefaultObjects = ParseCommand(&Str,TEXT("SHOWDEFAULTS"));
				UBOOL bShowPendingKills = ParseCommand(&Str, TEXT("SHOWPENDINGKILLS"));
				for ( FObjectIterator It; It; ++It )
				{
					UObject* CurrentObject = *It;

					if ( LimitOuter != NULL && !CurrentObject->IsIn(LimitOuter) )
					{
						continue;
					}
					
					if ( CurrentObject->IsTemplate(RF_ClassDefaultObject) && bShowDefaultObjects == FALSE )
					{
						continue;
					}

					if ( (bShowPendingKills || !CurrentObject->IsPendingKill()) && CurrentObject->IsA(Class) )
					{
						if ( Property->ArrayDim > 1 || Cast<UArrayProperty>(Property) != NULL )
						{
							BYTE* BaseData = (BYTE*)CurrentObject + Property->Offset;
							Ar.Logf(TEXT("%i) %s.%s ="), cnt++, *CurrentObject->GetFullName(), *Property->GetName());

							INT ElementCount = Property->ArrayDim;

							UProperty* ExportProperty = Property;
							if ( Property->ArrayDim == 1 )
							{
								FArray* Array = (FArray*)BaseData;
								UArrayProperty* ArrayProp = Cast<UArrayProperty>(Property);

								BaseData = (BYTE*)Array->GetData();
								ElementCount = Array->Num();
								ExportProperty = ArrayProp->Inner;
							}

							INT ElementSize = ExportProperty->ElementSize;
							for ( INT ArrayIndex = 0; ArrayIndex < ElementCount; ArrayIndex++ )
							{
								FString ResultStr;
								BYTE* ElementData = BaseData + ArrayIndex * ElementSize;
								ExportProperty->ExportTextItem(ResultStr, ElementData, NULL, CurrentObject, PPF_Localized);

								Ar.Logf(TEXT("\t%i: %s"), ArrayIndex, *ResultStr);
							}
						}
						else
						{
							BYTE* BaseData = (BYTE*)CurrentObject;
							FString ResultStr;
							for (INT i = 0; i < Property->ArrayDim; i++)
							{
								Property->ExportText(i, ResultStr, BaseData, BaseData, CurrentObject, PPF_Localized);
							}

							Ar.Logf(TEXT("%i) %s.%s = %s"), cnt++, *CurrentObject->GetFullName(), *Property->GetName(), *ResultStr);
						}
					}
				}
			}
			else Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized property %s"), PropertyName );
		}
		else Ar.Logf( NAME_ExecWarning, TEXT("Unrecognized class %s"), ClassName );
		return 1;
	}
	else if (ParseCommand(&Str, TEXT("GETALLSTATE")))
	{
		// iterate through all objects of the specified class and log the state they're in
		TCHAR ClassName[256];
		UClass* Class;

		if ( ParseToken(Str, ClassName, ARRAY_COUNT(ClassName), 1) &&
			(Class = FindObject<UClass>(ANY_PACKAGE, ClassName)) != NULL )
		{
			UBOOL bShowPendingKills = ParseCommand(&Str, TEXT("SHOWPENDINGKILLS"));
			INT cnt = 0;
			for (TObjectIterator<UObject> It; It; ++It)
			{
				if ((bShowPendingKills || !It->IsPendingKill()) && It->IsA(Class))
				{
					Ar.Logf( TEXT("%i) %s is in state %s"), cnt++, *It->GetFullName(),
							(It->GetStateFrame() && It->GetStateFrame()->StateNode) ?
								*It->GetStateFrame()->StateNode->GetName() :
								TEXT("None") );
				}
			}
		}
		else
		{
			Ar.Logf(NAME_ExecWarning, TEXT("Unrecognized class %s"), ClassName);
		}
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("SET")) )
	{
		PerformSetCommand( Str, Ar, TRUE );
		return 1;
	}
	else if( ParseCommand(&Str,TEXT("SETNOPEC")) )
	{
		PerformSetCommand( Str, Ar, FALSE );
		return 1;
	}
	else if ( ParseCommand(&Str,TEXT("VERIFYCOMPONENTS")) )
	{
		for ( TObjectIterator<UComponent> It; It; ++It )
		{
			UComponent* Component = *It;
			UComponent* ComponentTemplate = Cast<UComponent>(Component->GetArchetype());
			UObject* Owner = Component->GetOuter();
			UObject* OwnerTemplate = Owner->GetArchetype();
			UObject* TemplateOwner = ComponentTemplate->GetOuter();
			if ( !ComponentTemplate->HasAnyFlags(RF_ClassDefaultObject) )
			{
				if ( TemplateOwner != OwnerTemplate )
				{

					FString RealArchetypeName;
					if ( Component->TemplateName != NAME_None )
					{
						UComponent* RealArchetype = Owner->GetArchetype()->FindComponent(Component->TemplateName);
						if ( RealArchetype != NULL )
						{
							RealArchetypeName = RealArchetype->GetFullName();
						}
						else
						{
							RealArchetypeName = FString::Printf(TEXT("NULL: no matching components found in Owner Archetype %s"), *Owner->GetArchetype()->GetFullName());
						}
					}
					else
					{
						RealArchetypeName = TEXT("NULL");
					}

					warnf(TEXT("Possible corrupted component: '%s'	Archetype: '%s'	TemplateName: '%s'	ResolvedArchetype: '%s'"),
						*Component->GetFullName(), 
						*ComponentTemplate->GetPathName(),
						*Component->TemplateName.ToString(),
						*RealArchetypeName
						);
				}
			}
		}

		return 1;
	}
	else if( ParseCommand(&Str,TEXT("OBJ")) )
	{
		if( ParseCommand(&Str,TEXT("GARBAGE")) || ParseCommand(&Str,TEXT("GC")) )
		{
			// Purge unclaimed objects.
			CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("MARK")) )
		{
			debugf( TEXT("Marking objects") );
			for( FObjectIterator It; It; ++It )
				It->SetFlags( RF_Marked );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("MARKCHECK")) )
		{
			debugf( TEXT("Unmarked objects:") );
			for( FObjectIterator It; It; ++It )
				if( !It->HasAnyFlags(RF_Marked) )
					debugf( TEXT("%s"), *It->GetFullName() );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("REFS")) )
		{
			UClass* Class;
			UObject* Object;
			if
			(	ParseObject<UClass>( Str, TEXT("CLASS="), Class, ANY_PACKAGE )
			&&	ParseObject(Str,TEXT("NAME="), Class, Object, ANY_PACKAGE ) )
			{
				FStringOutputDevice TempAr;
				Object->OutputReferencers(TempAr,TRUE);

				TArray<FString> Lines;
				TempAr.ParseIntoArray(&Lines, LINE_TERMINATOR, 0);
				for ( INT i = 0; i < Lines.Num(); i++ )
				{
					Ar.Log(*Lines(i));
				}
			}
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("HASH")) )
		{
			// Hash info.
			FName::DisplayHash( Ar );
			INT ObjCount=0, HashCount=0;
			for( FObjectIterator It; It; ++It )
				ObjCount++;
			for( INT i=0; i<ARRAY_COUNT(GObjHash); i++ )
			{
				INT c=0;
				for( UObject* Hash=GObjHash[i]; Hash; Hash=Hash->HashNext )
					c++;
				if( c )
					HashCount++;
				//debugf( "%i: %i", i, c );
			}
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("HASHOUTER")) )
		{
			INT SlotsInUse = 0;
			INT TotalCollisions = 0;
			INT MinCollisions = 0;
			INT MaxCollisions = 0;
			INT MaxBin = 0;
			// Work through each slot and figure out how many collisions
			for (DWORD CurrSlot = 0; CurrSlot < OBJECT_HASH_BINS; CurrSlot++)
			{
				INT Collisions = 0;
				// Get the first item in the slot
				UObject* Hash = GObjHashOuter[CurrSlot];
				// Determine if this slot is being used
				if (Hash != NULL)
				{
					// This slot is in use
					SlotsInUse++;
					// Now count how many hash collisions there are
					while (Hash)
					{
						Hash = Hash->HashOuterNext;
						// If there is another item in the chain
						if (Hash != NULL)
						{
							// Then increment the number of collisions
							Collisions++;
						}
					}
					// Keep the global stats
					TotalCollisions += Collisions;
					if (Collisions > MaxCollisions)
					{
						MaxBin = CurrSlot;
					}
					MaxCollisions = Max<INT>(Collisions,MaxCollisions);
					MinCollisions = Min<INT>(Collisions,MinCollisions);
					// Now log the output
					Ar.Logf(TEXT("\tSlot %d has %d collisions"),CurrSlot,Collisions);
				}
			}
			// Dump how many slots were in use
			Ar.Logf(TEXT("Slots in use %d"),SlotsInUse);
			// Now dump how efficient the hash is
			Ar.Logf(TEXT("Collision Stats: Best Case (%d), Average Case (%d), Worst Case (%d)"),
				MinCollisions,appFloor(((FLOAT)TotalCollisions / (FLOAT)OBJECT_HASH_BINS)),
				MaxCollisions);
			// Dump the first 30 objects in the worst bin for inspection
			INT Count = 0;
			UObject* Hash = GObjHashOuter[MaxBin];
			while (Hash != NULL && Count < 30)
			{
				Ar.Logf(TEXT("Object is %s (%s)"),*Hash->GetName(),*Hash->GetFullName());
				Hash = Hash->HashOuterNext;
				Count++;
			}
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("HASHOBJ")) )
		{
			INT SlotsInUse = 0;
			INT TotalCollisions = 0;
			INT MinCollisions = 0;
			INT MaxCollisions = 0;
			INT MaxBin = 0;
			// Work through each slot and figure out how many collisions
			for (DWORD CurrSlot = 0; CurrSlot < OBJECT_HASH_BINS; CurrSlot++)
			{
				INT Collisions = 0;
				// Get the first item in the slot
				UObject* Hash = GObjHash[CurrSlot];
				// Determine if this slot is being used
				if (Hash != NULL)
				{
					// This slot is in use
					SlotsInUse++;
					// Now count how many hash collisions there are
					while (Hash)
					{
						Hash = Hash->HashNext;
						// If there is another item in the chain
						if (Hash != NULL)
						{
							// Then increment the number of collisions
							Collisions++;
						}
					}
					// Keep the global stats
					TotalCollisions += Collisions;
					if (Collisions > MaxCollisions)
					{
						MaxBin = CurrSlot;
					}
					MaxCollisions = Max<INT>(Collisions,MaxCollisions);
					MinCollisions = Min<INT>(Collisions,MinCollisions);
					// Now log the output
					Ar.Logf(TEXT("\tSlot %d has %d collisions"),CurrSlot,Collisions);
				}
			}
			// Dump how many slots were in use
			Ar.Logf(TEXT("Slots in use %d"),SlotsInUse);
			// Now dump how efficient the hash is
			Ar.Logf(TEXT("Collision Stats: Best Case (%d), Average Case (%d), Worst Case (%d)"),
				MinCollisions,appFloor(((FLOAT)TotalCollisions / (FLOAT)OBJECT_HASH_BINS)),
				MaxCollisions);
			// Dump the first 30 objects in the worst bin for inspection
			INT Count = 0;
			UObject* Hash = GObjHash[MaxBin];
			while (Hash != NULL && Count < 30)
			{
				Ar.Logf(TEXT("Object is %s (%s)"),*Hash->GetName(),*Hash->GetFullName());
				Hash = Hash->HashNext;
				Count++;
			}
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("CLASSES")) )
		{
			ShowClasses( StaticClass(), Ar, 0 );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("DEPENDENCIES")) )
		{
			UPackage* Pkg;
			if( ParseObject<UPackage>(Str,TEXT("PACKAGE="),Pkg,NULL) )
			{
				TArray<UObject*> Exclude;

				// check if we want to ignore references from any packages
				for( int i=0; i<16; i++ )
				{
					TCHAR Temp[MAX_SPRINTF]=TEXT("");
					appSprintf( Temp, TEXT("EXCLUDE%i="), i );
					FName F;
					if( Parse(Str,Temp,F) )
						Exclude.AddItem( CreatePackage(NULL,*F.ToString()) );
				}
				Ar.Logf( TEXT("Dependencies of %s:"), *Pkg->GetPathName() );

				UBOOL Dummy=0;

				// Should we recurse into inner packages?
				UBOOL bRecurse = ParseUBOOL(Str, TEXT("RECURSE"), Dummy);

				// Iterate through the object list
				for( FObjectIterator It; It; ++It )
				{
					// if this object is within the package specified, serialize the object
					// into a specialized archive which logs object names encountered during
					// serialization -- rjp
					if ( It->IsIn(Pkg) )
					{
						if ( It->GetOuter() == Pkg )
						{
							FArchiveShowReferences ArShowReferences( Ar, Pkg, *It, Exclude );
						}
						else if ( bRecurse )
						{
							// Two options -
							// a) this object is a function or something (which we don't care about)
							// b) this object is inside a group inside the specified package (which we do care about)
							UObject* CurrentObject = *It;
							UObject* CurrentOuter = It->GetOuter();
							while ( CurrentObject && CurrentOuter )
							{
								// this object is a UPackage (a group inside a package)
								// abort
								if ( CurrentObject->GetClass() == UPackage::StaticClass() )
									break;

								// see if this object's outer is a UPackage
								if ( CurrentOuter->GetClass() == UPackage::StaticClass() )
								{
									// if this object's outer is our original package, the original object (It)
									// wasn't inside a group, it just wasn't at the base level of the package
									// (its Outer wasn't the Pkg, it was something else e.g. a function, state, etc.)
									/// ....just skip it
									if ( CurrentOuter == Pkg )
										break;

									// otherwise, we've successfully found an object that was in the package we
									// were searching, but would have been hidden within a group - let's log it
									FArchiveShowReferences ArShowReferences( Ar, CurrentOuter, CurrentObject, Exclude );
									break;
								}

								CurrentObject = CurrentOuter;
								CurrentOuter = CurrentObject->GetOuter();
							}
						}
					}
				}
			}
			else
				debugf(TEXT("Package wasn't found."));
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("BULK")) )
		{
			FUntypedBulkData::DumpBulkDataUsage( Ar );
			return TRUE;
		}
		else if( ParseCommand(&Str,TEXT("LIST")) )
		{
			Ar.Log( TEXT("Objects:") );
			Ar.Log( TEXT("") );
			UClass*   CheckType     = NULL;
			UClass*   MetaClass		= NULL;
			///MSSTART DAN PRICE MICROSOFT Mar 12th, 2007 //If -File param is passed then export the object data to a file
			UBOOL bExportToFile = ParseParam(Str,TEXT("FILE"));
			///MSEND


			// allow checking for any Outer, not just a UPackage
			UObject* CheckOuter = NULL;
			UPackage* InsidePackage = NULL;
			UObject* InsideObject = NULL;
			ParseObject<UClass>(Str, TEXT("CLASS="  ), CheckType, ANY_PACKAGE );
			ParseObject<UObject>(Str, TEXT("OUTER="), CheckOuter, ANY_PACKAGE);

			ParseObject<UPackage>(Str, TEXT("PACKAGE="), InsidePackage, NULL);
			if ( InsidePackage == NULL )
			{
				ParseObject<UObject>( Str, TEXT("INSIDE=" ), InsideObject, NULL );
			}
			INT Depth = -1;
			Parse(Str, TEXT("DEPTH="), Depth);

			TArray<FItem> List;
			TArray<FSubItem> Objects;
			FItem Total;

			// support specifying metaclasses when listing class objects
			if ( CheckType && CheckType == UClass::StaticClass() )
			{
				ParseObject<UClass>  ( Str, TEXT("TYPE="   ), MetaClass,     ANY_PACKAGE );
			}

			// if we specified a parameter in the command, but no objects of that parameter were found,
			// don't be dumb and list all objects
			if ((CheckType		||	!appStrfind(Str,TEXT("CLASS=")))
			&&	(MetaClass		||	!appStrfind(Str,TEXT("TYPE=")))
			&&	(CheckOuter		||	!appStrfind(Str,TEXT("OUTER=")))
			&&	(InsidePackage	||	!appStrfind(Str,TEXT("PACKAGE="))) 
			&&	(InsideObject	||	!appStrfind(Str,TEXT("INSIDE="))))
			{
				UBOOL bOnlyListGCObjects			= ParseParam( Str, TEXT("GCONLY") );
				UBOOL bShouldIncludeDefaultObjects	= ParseParam( Str, TEXT("INCLUDEDEFAULTS") );
				UBOOL bOnlyListDefaultObjects		= ParseParam( Str, TEXT("DEFAULTSONLY") );

				for( FObjectIterator It; It; ++It )
				{
					if ( It->HasAllFlags(RF_ClassDefaultObject) )
					{
						if( !bShouldIncludeDefaultObjects )
						{
							continue;
						}
					}
					else if( bOnlyListDefaultObjects )
					{
						continue;
					}

					if ( bOnlyListGCObjects && It->HasAnyFlags( RF_DisregardForGC ) )
					{
						continue;
					}

					if ( CheckType && !It->IsA(CheckType) )
					{
						continue;
					}

					if ( CheckOuter && It->GetOuter() != CheckOuter )
					{
						continue;
					}

					if ( InsidePackage && !It->IsIn(InsidePackage) )
					{
						continue;
					}

					if ( InsideObject && !It->IsIn(InsideObject) )
					{
						continue;
					}

					if ( MetaClass )
					{
						UClass* ClassObj = Cast<UClass>(*It);
						if ( ClassObj && !ClassObj->IsChildOf(MetaClass) )
						{
							continue;
						}
					}

					FArchiveCountMem Count( *It );
					SIZE_T ResourceSize = It->GetResourceSize();
					INT i;

					// which class are we going to file this object under? by default, it's class
					UClass* ClassToUse = It->GetClass();
					// if we specified a depth to use, then put this object into the class Depth away from Object
					if (Depth != -1)
					{
						UClass* Travel = ClassToUse;
						// go up the class hierarchy chain, using a trail pointer Depth away
						for (INT Up = 0; Up < Depth && Travel != UObject::StaticClass(); Up++)
						{
							Travel = (UClass*)Travel->SuperField;
						}
						// when travel is a UObject, ClassToUse will be pointing to a class Depth away
						while (Travel != UObject::StaticClass())
						{
							Travel = (UClass*)Travel->SuperField;
							ClassToUse = (UClass*)ClassToUse->SuperField;
						}
					}

					for( i=0; i<List.Num(); i++ )
					{
						if( List(i).Class == ClassToUse )
						{
							break;
						}
					}
					if( i==List.Num() )
					{
						i = List.AddItem(FItem( ClassToUse ));
					}
					if( CheckType || CheckOuter || InsidePackage )
					{
						new(Objects)FSubItem( *It, Count.GetNum(), Count.GetMax(), ResourceSize );
					}
					List(i).Add( Count, ResourceSize );
					Total.Add( Count, ResourceSize );
				}
			}

			bAlphaSort = ParseParam( Str, TEXT("ALPHASORT") );
			bCountSort = ParseParam( Str, TEXT("COUNTSORT") );

			if( Objects.Num() )
			{
				Sort<USE_COMPARE_CONSTREF(FSubItem,UnObj)>( &Objects(0), Objects.Num() );
#if PS3 && UNICODE
				Ar.Logf( TEXT("%60ls % 10ls % 10ls % 10ls"), TEXT("Object"), TEXT("NumBytes"), TEXT("MaxBytes"), TEXT("ResKBytes") );
#else
				Ar.Logf( TEXT("%60s % 10s % 10s % 10s"), TEXT("Object"), TEXT("NumBytes"), TEXT("MaxBytes"), TEXT("ResKBytes") );
#endif
				for( INT i=0; i<Objects.Num(); i++ )
				{
					///MSSTART DAN PRICE MICROSOFT Mar 12th, 2007 export object data to a file
					if(bExportToFile)
					{
						FString Path = TEXT(".\\ObjExport");
						const FString Filename = Path * *Objects(i).Object->GetName() + TEXT(".t3d");
						Ar.Logf( TEXT("%s"),*Filename);
						UExporter::ExportToFile(Objects(i).Object, NULL, *Filename, 1, 0);
					}					
					//MSEND
#if PS3 && UNICODE
					Ar.Logf( TEXT("%60ls % 10i % 10i % 10i"), *Objects(i).Object->GetFullName(), Objects(i).Num, Objects(i).Max, Objects(i).Object->GetResourceSize() / 1024 );
#else
					Ar.Logf( TEXT("%60s % 10i % 10i % 10i"), *Objects(i).Object->GetFullName(), Objects(i).Num, Objects(i).Max, Objects(i).Object->GetResourceSize() / 1024 );
#endif
				}
				Ar.Log( TEXT("") );
			}
			if( List.Num() )
			{
				Sort<USE_COMPARE_CONSTREF(FItem,UnObj)>( &List(0), List.Num() );
#if PS3 && UNICODE
				Ar.Logf(TEXT(" %60ls % 6ls % 10ls % 10ls % 10ls"), TEXT("Class"), TEXT("Count"), TEXT("NumBytes"), TEXT("MaxBytes"), TEXT("ResBytes") );
#else
				Ar.Logf(TEXT(" %60s % 6s % 10s % 10s % 10s"), TEXT("Class"), TEXT("Count"), TEXT("NumBytes"), TEXT("MaxBytes"), TEXT("ResBytes") );
#endif
				for( INT i=0; i<List.Num(); i++ )
				{
#if PS3 && UNICODE
					Ar.Logf(TEXT(" %60ls % 6i % 10iK % 10iK % 10iK"), *List(i).Class->GetName(), List(i).Count, List(i).Num/1024, List(i).Max/1024, List(i).Res/1024 );
#else
					Ar.Logf(TEXT(" %60s % 6i % 10iK % 10iK % 10iK"), *List(i).Class->GetName(), List(i).Count, List(i).Num/1024, List(i).Max/1024, List(i).Res/1024 );
#endif
				}
				Ar.Log( TEXT("") );
			}
			Ar.Logf( TEXT("%i Objects (%.3fM / %.3fM / %.3fM)"), Total.Count, (FLOAT)Total.Num/1024.0/1024.0, (FLOAT)Total.Max/1024.0/1024.0, (FLOAT)Total.Res/1024.0/1024.0 );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("LINKERS")) )
		{
			Ar.Logf( TEXT("Linkers:") );
			for( INT i=0; i<GObjLoaders.Num(); i++ )
			{
				ULinkerLoad* Linker = GObjLoaders(i);
				INT NameSize = 0;
				for( INT j=0; j<Linker->NameMap.Num(); j++ )
					if( Linker->NameMap(j) != NAME_None )
						NameSize += sizeof(FNameEntry) - (NAME_SIZE - appStrlen(*Linker->NameMap(j).ToString()) - 1) * sizeof(TCHAR);
				Ar.Logf
				(
					TEXT("%s (%s): Names=%i (%iK/%iK) Imports=%i (%iK) Exports=%i (%iK) Gen=%i Bulk=%i"),
					*Linker->Filename,
					*Linker->LinkerRoot->GetFullName(),
					Linker->NameMap.Num(),
					Linker->NameMap.Num() * sizeof(FName) / 1024,
					NameSize / 1024,
					Linker->ImportMap.Num(),
					Linker->ImportMap.Num() * sizeof(FObjectImport) / 1024,
					Linker->ExportMap.Num(),
					Linker->ExportMap.Num() * sizeof(FObjectExport) / 1024,
					Linker->Summary.Generations.Num(),
					Linker->BulkDataLoaders.Num()
				);
			}
			return 1;
		}
		else if ( ParseCommand(&Str,TEXT("FLAGS")) )
		{
			// Dump all object flags for objects rooted at the named object.
			TCHAR ObjectName[NAME_SIZE];
			UObject* Obj = NULL;
			if ( ParseToken(Str,ObjectName,ARRAY_COUNT(ObjectName), 1) )
			{
				Obj = FindObject<UObject>(ANY_PACKAGE,ObjectName);
			}

			if ( Obj )
			{
				PrivateDumpObjectFlags( Obj, Ar );
				PrivateRecursiveDumpFlags( Obj->GetClass(), (BYTE*)Obj, Ar );
			}

			return TRUE;
		}
		else if ( ParseCommand(&Str,TEXT("COMPONENTS")) )
		{
			UObject* Obj=NULL;
			FString ObjectName;

			if ( ParseToken(Str,ObjectName,TRUE) )
			{
				Obj = FindObject<UObject>(ANY_PACKAGE,*ObjectName);

				if ( Obj != NULL )
				{
					Ar.Log(TEXT(""));
					Obj->DumpComponents();
					Ar.Log(TEXT(""));
				}
				else
				{
					Ar.Logf(TEXT("No objects found named '%s'"), *ObjectName);
				}
			}
			else
			{
				Ar.Logf(TEXT("Syntax: OBJ COMPONENTS <Name Of Object>"));
			}
			return TRUE;
		}
		else if ( ParseCommand(&Str,TEXT("DUMP")) )
		{
			// Dump all variable values for the specified object
			// supports specifying categories to hide or show
			// OBJ DUMP playercontroller0 hide="actor,object,lighting,movement"     OR
			// OBJ DUMP playercontroller0 show="playercontroller,controller"        OR
			// OBJ DUMP class=playercontroller name=playercontroller0 show=object OR
			// OBJ DUMP playercontroller0 recurse=true
			TCHAR ObjectName[NAME_SIZE];
			UObject* Obj = NULL;
			UClass* Cls = NULL;

			TArray<FString> HiddenCategories, ShowingCategories;

			if ( !ParseObject<UClass>( Str, TEXT("CLASS="), Cls, ANY_PACKAGE ) || !ParseObject(Str,TEXT("NAME="), Cls, Obj, ANY_PACKAGE) )
			{
				if ( ParseToken(Str,ObjectName,ARRAY_COUNT(ObjectName), 1) )
				{
					Obj = FindObject<UObject>(ANY_PACKAGE,ObjectName);
				}
			}

			if ( Obj )
			{
				if ( Cast<UClass>(Obj) != NULL )
				{
					Obj = Cast<UClass>(Obj)->GetDefaultObject();
				}

				FString Value;

				Ar.Logf(TEXT(""));

				const UBOOL bRecurse = Parse(Str, TEXT("RECURSE=TRUE"), Value);
				Ar.Logf(TEXT("*** Property dump for object %s'%s' ***"), bRecurse ? TEXT("(Recursive) ") : TEXT(""), *Obj->GetFullName() );

				if ( bRecurse )
				{
					const FExportObjectInnerContext Context;
					ExportProperties( &Context, Ar, Obj->GetClass(), (BYTE*)Obj, 0, Obj->GetArchetype()->GetClass(), (BYTE*)Obj->GetArchetype(), Obj, PPF_Localized );
				}
				else
				{
					//@todo: add support to Parse() for specifying characters that should be ignored
					if ( Parse(Str, TEXT("HIDE="), Value/*, TEXT(",")*/) )
					{
						Value.ParseIntoArray(&HiddenCategories,TEXT(","),1);
					}
					else if ( Parse(Str, TEXT("SHOW="), Value/*, TEXT(",")*/) )
					{
						Value.ParseIntoArray(&ShowingCategories,TEXT(","),1);
					}

					for ( TFieldIterator<UProperty,CLASS_IsAUProperty> It(Obj->GetClass()); It; ++It )
					{
						Value.Empty();
						if ( HiddenCategories.Num() )
						{
							INT i;
							for ( i = 0; i < HiddenCategories.Num(); i++ )
							{
								if ( It->Category != NAME_None && HiddenCategories(i) == It->Category.ToString() )
								{
									break;
								}

								if ( HiddenCategories(i) == *It->GetOwnerClass()->GetName() )
								{
									break;
								}
							}

							if ( i < HiddenCategories.Num() )
							{
								continue;
							}
						}

						else if ( ShowingCategories.Num() )
						{
							INT i;
							for ( i = 0; i < ShowingCategories.Num(); i++ )
							{
								if ( It->Category != NAME_None && ShowingCategories(i) == It->Category.ToString() )
								{
									break;
								}

								if ( ShowingCategories(i) == *It->GetOwnerClass()->GetName() )
								{
									break;
								}
							}

							if ( i == ShowingCategories.Num() )
							{
								continue;
							}
						}

						if ( It->ArrayDim > 1 )
						{
							for ( INT i = 0; i < It->ArrayDim; i++ )
							{
								Value.Empty();
								It->ExportText(i, Value, (BYTE*)Obj, (BYTE*)Obj, Obj, PPF_Localized);
								Ar.Logf(TEXT("  %s[%i]=%s"), *It->GetName(), i, *Value);
							}
						}
						else
						{
							if ( It->IsA(UArrayProperty::StaticClass() ) )
							{
								UArrayProperty* ArrayProp = Cast<UArrayProperty>(*It);
								FArray* Array       = (FArray*)((BYTE*)Obj + ArrayProp->Offset);
								const INT ElementSize = ArrayProp->Inner->ElementSize;
								for( INT i=0; i<Array->Num(); i++ )
								{
									Value.Empty();
									BYTE* Data = (BYTE*)Array->GetData() + i * ElementSize;
									ArrayProp->Inner->ExportTextItem( Value, Data, Data, Obj, PPF_Localized );
									Ar.Logf(TEXT("  %s(%i)=%s"), *ArrayProp->GetName(), i, *Value);
								}
							}

							else
							{
								It->ExportText(0, Value, (BYTE*)Obj, (BYTE*)Obj, Obj, PPF_Localized);
								Ar.Logf(TEXT("  %s=%s"), *It->GetName(), *Value);
							}
						}
					}
				}
			}
			else
			{
				Ar.Logf(NAME_ExecWarning, TEXT("No objects found using command '%s'"), Cmd);
			}

			return 1;
		}
		else if (ParseCommand(&Str, TEXT("NET")))
		{
			TCHAR PackageName[1024];
			if (ParseToken(Str, PackageName, ARRAY_COUNT(PackageName), TRUE))
			{
				UPackage* Package = FindObject<UPackage>(NULL, PackageName, TRUE);
				if (Package != NULL)
				{
					INT NetObjects = Package->GetGenerationNetObjectCount().Last();
					INT LoadedNetObjects = Package->GetCurrentNumNetObjects();
					Ar.Logf(TEXT("Package '%s' has %i total net objects (%i loaded)"), PackageName, NetObjects, LoadedNetObjects);
					for (INT i = 0; i < NetObjects; i++)
					{
						UObject* Obj = Package->GetNetObjectAtIndex(i);
						if (Obj != NULL)
						{
							Ar.Logf(TEXT(" - %i: %s"), i, *Obj->GetFullName());
						}
					}
				}
				else
				{
					Ar.Logf(NAME_ExecWarning, TEXT("OBJ NET: failed to find package '%s'"), PackageName);
				}
			}
			else
			{
				Ar.Logf(NAME_ExecWarning, TEXT("OBJ NET: missing package name"));
			}
			return 1;
		}
		else return 0;
	}
	else if( ParseCommand(&Str,TEXT("PROFILESCRIPT")) || ParseCommand(&Str,TEXT("SCRIPTPROFILER")) )
	{
		if( ParseCommand(&Str,TEXT("START")) )
		{
			delete GScriptCallGraph;
			GScriptCallGraph = new FScriptCallGraph( 16 * 1024 * 1024 );
			debugf( TEXT( " ... collecting script profile data." ) );
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("STOP")) )
		{
			if( GScriptCallGraph )
			{
				GScriptCallGraph->EmitEnd();

				// Create unique filename based on time.
				INT Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
				appSystemTime( Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec );

#if XBOX
				FString	Filename = FString::Printf(TEXT("%sProfiling\\%sGame-Xenon-%i.%02i.%02i-%02i.%02i.%02i.uprof"), *appGameDir(), GGameName, Year, Month, Day, Hour, Min, Sec );
#elif PS3
				FString	Filename = FString::Printf(TEXT("%sProfiling\\%sGame-PS3-%i.%02i.%02i-%02i.%02i.%02i.uprof"), *appGameDir(), GGameName, Year, Month, Day, Hour, Min, Sec );
#else
				FString	Filename = FString::Printf(TEXT("%sProfiling\\%sGame-%i.%02i.%02i-%02i.%02i.%02i.uprof"), *appGameDir(), GGameName, Year, Month, Day, Hour, Min, Sec );
#endif
				debugf( TEXT( " ... attempting write of profile data to \'%s\'" ), *Filename );

				// Create the directory in case it doesn't exist yet.
				GFileManager->MakeDirectory( *(appGameDir() + TEXT("Profiling\\")) );

				// Create a file writer and serialize the script profiling data to it.
				FArchive* Archive = GFileManager->CreateFileWriter( *Filename );
				if( Archive )
				{
					GScriptCallGraph->Serialize( *Archive );
					delete Archive;
				}

				delete GScriptCallGraph;
				GScriptCallGraph = NULL;

				debugf( TEXT( " ... profile data written and cleaned up." ), *Filename );
			}
			return 1;
		}
		else if( ParseCommand(&Str,TEXT("RESET"))  )
		{
			if( GScriptCallGraph )
			{
				GScriptCallGraph->Reset();
			}
			return 1;
		}
		return 0;
	}
	else if (ParseCommand(&Str,TEXT("SUPPRESS")))
	{
		FString EventNameString = ParseToken(Str,0);
		FName	EventName		= FName(*EventNameString);
		if( EventName == NAME_All )
		{
			// We don't want to bloat GSys->Suppress with the entire list of FNames so we only set the
			// suppression flag for all currently active names and leave GSys->Suppress be.
			INT MaxNames = FName::GetMaxNames();			
			for( INT NameIndex=0; NameIndex<MaxNames; NameIndex++ )
			{
				FNameEntry* NameEntry = FName::GetEntry(NameIndex);
				if( NameEntry )
				{
					NameEntry->SetFlags(RF_Suppress);
				}
			}
		}
		else if( EventName != NAME_None )
		{
			EventName.SetFlags(RF_Suppress);
			GSys->Suppress.AddUniqueItem( EventName );
			debugf(TEXT("Suppressed event %s"),*EventNameString);
		}
		return 1;
	}
	else if (ParseCommand(&Str,TEXT("UNSUPPRESS")))
	{
		FString	EventNameString = ParseToken(Str,0);
		FName	EventName		= FName(*EventNameString);
		if( EventName == NAME_All )
		{
			// Iterate over all names, clearing suppression flag.
			INT MaxNames = FName::GetMaxNames();			
			for( INT NameIndex=0; NameIndex<MaxNames; NameIndex++ )
			{
				FNameEntry* NameEntry = FName::GetEntry(NameIndex);
				if( NameEntry )
				{
					NameEntry->ClearFlags(RF_Suppress);
				}
			}
			// Empty serialized array of suppressed names.
			GSys->Suppress.Empty();
		}
		else if( EventName != NAME_None )
		{
			EventName.ClearFlags(RF_Suppress);
			GSys->Suppress.RemoveItem( EventName );
			debugf(TEXT("Unsuppressed event %s"),*EventNameString);
		}
		return 1;
	}
	else if ( ParseCommand(&Str, TEXT("STRUCTPERFDATA")) )
	{
#if TRACK_SERIALIZATION_PERFORMANCE || LOOKING_FOR_PERF_ISSUES
		if ( ParseCommand(&Str, TEXT("DUMP")) )
		{
			if ( GObjectSerializationPerfTracker != NULL )
			{
				Ar.Logf(NAME_PerfEvent, TEXT("Global Performance Data:"));
				GObjectSerializationPerfTracker->DumpPerformanceData(&Ar);
			}

			for( INT LinkerIdx = 0; LinkerIdx < GObjLoaders.Num(); LinkerIdx++ )
			{
				ULinkerLoad* Loader = GObjLoaders(LinkerIdx);
				if ( Loader != NULL && Loader->LinkerRoot != NULL && Loader->SerializationPerfTracker.HasPerformanceData() )
				{
					Ar.Logf(NAME_PerfEvent, TEXT("============================================================"));
					Ar.Logf(NAME_PerfEvent, TEXT("Performance data for %s:"), *Loader->Filename);
					Loader->SerializationPerfTracker.DumpPerformanceData(&Ar);
					Ar.Logf(NAME_PerfEvent, TEXT("============================================================"));
				}
			}

			if ( GClassSerializationPerfTracker != NULL )
			{
				Ar.Log(NAME_PerfEvent, TEXT("============================================================"));
				Ar.Log(NAME_PerfEvent, TEXT("Class Serialization Performance Data:"));
				GClassSerializationPerfTracker->DumpPerformanceData(&Ar);
				Ar.Log(NAME_PerfEvent, TEXT("============================================================"));
			}
		}
		else if ( ParseCommand(&Str,TEXT("RESET")) )
		{
			if ( GObjectSerializationPerfTracker != NULL )
			{
				Ar.Logf(NAME_PerfEvent, TEXT("Clearing Global Performance Data..."));
				GObjectSerializationPerfTracker->ClearEvents();
			}

			for( INT LinkerIdx = 0; LinkerIdx < GObjLoaders.Num(); LinkerIdx++ )
			{
				ULinkerLoad* Loader = GObjLoaders(LinkerIdx);
				if ( Loader != NULL && Loader->LinkerRoot != NULL && Loader->SerializationPerfTracker.HasPerformanceData() )
				{
					Ar.Logf(NAME_PerfEvent, TEXT("Clearing performance data for %s:"), *Loader->Filename);
					Loader->SerializationPerfTracker.ClearEvents();
				}
			}

			if ( GClassSerializationPerfTracker != NULL )
			{
				GClassSerializationPerfTracker->ClearEvents();
			}
		}
#else
		Ar.Logf(NAME_PerfEvent, TEXT("Serialization performance tracking not enabled - #define TRACK_SERIALIZATION_PERFORMANCE or LOOKING_FOR_PERF_ISSUES to 1 to enable"));
#endif

		return TRUE;
	}
	else
	{
		return FALSE; // Not executed
	}
}

/*-----------------------------------------------------------------------------
   File loading.
-----------------------------------------------------------------------------*/

//
// Safe load error-handling.
//
void UObject::SafeLoadError( UObject* Outer, DWORD LoadFlags, const TCHAR* Error, const TCHAR* Fmt, ... )
{
	// Variable arguments setup.
	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Fmt, Fmt );

	if( ParseParam( appCmdLine(), TEXT("TREATLOADWARNINGSASERRORS") ) == TRUE )
	{
		warnf( NAME_Error, TEXT("%s"), TempStr ); 
	}
	else
	{
		if( !(LoadFlags & LOAD_Quiet)  )
		{ 
			debugf( NAME_Warning, TEXT("%s"), TempStr ); 
		}

		if( LoadFlags & LOAD_Throw )
		{
			appThrowf( TEXT("%s"), Error   ); 
		}

		if( !(LoadFlags & LOAD_NoWarn) )
		{ 
			warnf( TEXT("%s"), TempStr ); 
		}
	}
}

//
// Find or create the linker for a package.
//
ULinkerLoad* UObject::GetPackageLinker
(
	UPackage*		InOuter,
	const TCHAR*	InFilename,
	DWORD			LoadFlags,
	UPackageMap*	Sandbox,
	FGuid*			CompatibleGuid
)
{
	check(GObjBeginLoadCount);

	// See if there is already a linker for this package.
	ULinkerLoad* Result = NULL;
	if( InOuter )
	{
		for( INT i=0; i<GObjLoaders.Num() && !Result; i++ )
		{
			if( GetLoader(i)->LinkerRoot == InOuter )
			{
				Result = GetLoader( i );
			}
		}
	}

	// Try to load the linker.
#if !EXCEPTIONS_DISABLED
	try
#endif
	{
		// See if the linker is already loaded.
		FString NewFilename;
		if( Result )
		{
			// Linker already found.
			NewFilename = TEXT("");
		}
		else
		if( !InFilename )
		{
			// Resolve filename from package name.
			if( !InOuter )
			{
#if EXCEPTIONS_DISABLED
				// try to recover from this instead of throwing, it seems recoverable just by doing this
				return NULL;
#endif
				appThrowf( *LocalizeError(TEXT("PackageResolveFailed"),TEXT("Core")) );
			}
			if( !GPackageFileCache->FindPackageFile( *InOuter->GetName(), CompatibleGuid, NewFilename ) )
			{
				// See about looking in the dll.
				if( (LoadFlags & LOAD_AllowDll) && InOuter->IsA(UPackage::StaticClass()) && ((UPackage*)InOuter)->IsBound() )
				{
					return NULL;
				}
				appThrowf( *LocalizeError(TEXT("PackageNotFound"),TEXT("Core")), *InOuter->GetName() );
			}
		}
		else
		{
			// Verify that the file exists.
			if( !GPackageFileCache->FindPackageFile( InFilename, CompatibleGuid, NewFilename ) )
			{
#if EXCEPTIONS_DISABLED
				// try to recover from this instead of throwing, it seems recoverable just by doing this
				return NULL;
#endif
				appThrowf( *LocalizeError(TEXT("FileNotFound"),TEXT("Core")), InFilename );
			}

			// Resolve package name from filename.
			TCHAR Tmp[256], *T=Tmp;
			appStrncpy( Tmp, InFilename, ARRAY_COUNT(Tmp) );
			while( 1 )
			{
				if( appStrstr(T,PATH_SEPARATOR) )
				{
					T = appStrstr(T,PATH_SEPARATOR)+appStrlen(PATH_SEPARATOR);
				}
				else if( appStrstr(T,TEXT("/")) )
				{
					T = appStrstr(T,TEXT("/"))+1;
				}
				else if( appStrstr(T,TEXT(":")) )
				{
					T = appStrstr(T,TEXT(":"))+1;
				}
				else
				{
					break;
				}
			}
			if( appStrstr(T,TEXT(".")) )
			{
				*appStrstr(T,TEXT(".")) = 0;
			}
			UPackage* FilenamePkg = CreatePackage( NULL, T );

			// If no package specified, use package from file.
			if( InOuter==NULL )
			{
				if( !FilenamePkg )
				{
					appThrowf( *LocalizeError(TEXT("FilenameToPackage"),TEXT("Core")), InFilename );
				}
				InOuter = FilenamePkg;
				for( INT i=0; i<GObjLoaders.Num() && !Result; i++ )
				{
					if( GetLoader(i)->LinkerRoot == InOuter )
					{
						Result = GetLoader(i);
					}
				}
			}
			else if( InOuter != FilenamePkg )//!!should be tested and validated in new UnrealEd
			{
				// Loading a new file into an existing package, so reset the loader.
				debugf( TEXT("New File, Existing Package (%s, %s)"), *InOuter->GetFullName(), *FilenamePkg->GetFullName() );
				ResetLoaders( InOuter );
			}
		}

		// Make sure the package is accessible in the sandbox.
		if( Sandbox && !Sandbox->SupportsPackage(InOuter) )
		{
			appThrowf( *LocalizeError(TEXT("Sandbox"),TEXT("Core")), *InOuter->GetName() );
		}

		// Create new linker.
		if( !Result )
		{
			// we will already have found the filename above
			check(NewFilename.Len() > 0);
			Result = ULinkerLoad::CreateLinker( InOuter, *NewFilename, LoadFlags );
		}

		// Verify compatibility.
		if( CompatibleGuid && Result->Summary.Guid!=*CompatibleGuid )
		{
			// This should never fire, because FindPackageFile should never return an incompatible file
			appThrowf( *LocalizeError(TEXT("PackageVersion"),TEXT("Core")), *InOuter->GetName() );
		}
	}
#if !EXCEPTIONS_DISABLED
	catch( const TCHAR* Error )
	{
		// If we're in the editor (and not running from UCC) we don't want this to be a fatal error.
		if( GIsEditor && !GIsUCC )
		{
			FString Filename = InFilename ? InFilename : InOuter ? InOuter->GetPathName() : TEXT("NULL");

			// if we don't want to be warned, skip the load warning
			if (!(LoadFlags & LOAD_NoWarn))
			{
				// If the filename is already in the error, no need to display it twice
				if (appStrstr(Error, *Filename))
				{
					EdLoadErrorf(FEdLoadError::TYPE_FILE, Error);
				}
				else
				{
					// If the error doesn't contain the file name, tack it on the end. This might be something like "..\Content\Foo.upk (Out of Memory)"
					EdLoadErrorf(FEdLoadError::TYPE_FILE, TEXT("%s (%s)"), *Filename, Error);
				}
				SET_WARN_COLOR(COLOR_RED);
				warnf( *LocalizeError(TEXT("FailedLoad"),TEXT("Core")), *Filename, Error);
				CLEAR_WARN_COLOR();
			}
		}
		else
		{
			// @see ResavePackagesCommandlet
			if( ParseParam(appCmdLine(),TEXT("SavePackagesThatHaveFailedLoads")) == TRUE )
			{
				warnf( NAME_Error, *LocalizeError(TEXT("FailedLoad"),TEXT("Core")), InFilename ? InFilename : InOuter ? *InOuter->GetName() : TEXT("NULL"), Error );
			}
			else
			{
				// Don't propagate error up so we can gracefully handle missing packages in the game as well.
				LoadFlags &= ~LOAD_Throw;
				SafeLoadError( InOuter, LoadFlags, Error, *LocalizeError(TEXT("FailedLoad"),TEXT("Core")), InFilename ? InFilename : InOuter ? *InOuter->GetName() : TEXT("NULL"), Error );
			}
		}
	}
#endif

	// Success.
	return Result;
}

/**
 * Find an existing package by name
 * @param InOuter		The Outer object to search inside
 * @param PackageName	The name of the package to find
 *
 * @return The package if it exists
 */
UPackage* UObject::FindPackage( UObject* InOuter, const TCHAR* PackageName )
{
	FString InName;
	if( PackageName )
	{
		InName = PackageName;
	}
	else
	{
		InName = MakeUniqueObjectName( InOuter, UPackage::StaticClass() ).ToString();
	}
	ResolveName( InOuter, InName, 1, 0 );

	UPackage* Result = NULL;
	if ( InName != TEXT("None") )
	{
		Result = FindObject<UPackage>( InOuter, *InName );
	}
	else
	{
		appErrorf( *LocalizeError(TEXT("PackageNamedNone"), TEXT("Core")) );
	}
	return Result;
}

/**
 * Find an existing package by name or create it if it doesn't exist
 * @param InOuter		The Outer object to search inside
 * @param PackageName	The name of the package to find
 *
 * @return The existing package or a newly created one
 */
UPackage* UObject::CreatePackage( UObject* InOuter, const TCHAR* PackageName )
{
	FString InName;
	if( PackageName )
	{
		InName = PackageName;
	}
	else
	{
		InName = MakeUniqueObjectName( InOuter, UPackage::StaticClass() ).ToString();
	}
	ResolveName( InOuter, InName, 1, 0 );

	UPackage* Result = NULL;
	if ( InName.Len() == 0 )
	{
		appErrorf( *LocalizeError(TEXT("EmptyPackageName"), TEXT("Core")) );
	}
	if ( InName != TEXT("None") )
	{
		Result = FindObject<UPackage>( InOuter, *InName );
		if( Result == NULL )
		{
			Result = new( InOuter, FName(*InName, FNAME_Add, TRUE), RF_Public )UPackage;
			// default is for packages to be downloadable
			Result->PackageFlags |= PKG_AllowDownload;
		}
	}
	else
	{
		appErrorf( *LocalizeError(TEXT("PackageNamedNone"), TEXT("Core")) );
	}
	return Result;
}

//
// Resolve a package and name.
//
UBOOL UObject::ResolveName( UObject*& InPackage, FString& InName, UBOOL Create, UBOOL Throw )
{
	const TCHAR* IniFilename = NULL;

	// See if the name is specified in the .ini file.
	if( appStrnicmp( *InName, TEXT("engine-ini:"), appStrlen(TEXT("engine-ini:")) )==0 )
	{
		IniFilename = GEngineIni;
	}
	else if( appStrnicmp( *InName, TEXT("game-ini:"), appStrlen(TEXT("game-ini:")) )==0 )
	{
		IniFilename = GGameIni;
	}
	else if( appStrnicmp( *InName, TEXT("input-ini:"), appStrlen(TEXT("input-ini:")) )==0 )
	{
		IniFilename = GInputIni;
	}
	else if( appStrnicmp( *InName, TEXT("editor-ini:"), appStrlen(TEXT("editor-ini:")) )==0 )
	{
		IniFilename = GEditorIni;
	}


	if( IniFilename && InName.InStr(TEXT("."))!=-1 )
	{
		// Get .ini key and section.
		FString Section = InName.Mid(1+InName.InStr(TEXT(":")));
		INT i = Section.InStr(TEXT("."), 1);
		FString Key;
		if( i != -1)
		{
			Key = Section.Mid(i+1);
			Section = Section.Left(i);
		}

		// Look up name.
		FString Result;
		if( !GConfig->GetString( *Section, *Key, Result, IniFilename ) )
		{
			if( Throw == TRUE )
			{
				FString ErrorMsg = FString::Printf( *LocalizeError(TEXT("ConfigNotFound"),TEXT("Core")), *InName, *Section, *Key );
				//GConfig->Dump( *GLog );
				appThrowf( *FString::Printf( TEXT( " %s %s " ), *ErrorMsg, IniFilename ) );
			}
			return FALSE;
		}
		InName = Result;
	}

	// Strip off the object class.
	INT NameStartIndex = InName.InStr(TEXT("\'"));
	if(NameStartIndex != INDEX_NONE)
	{
		INT NameEndIndex = InName.InStr(TEXT("\'"),TRUE);
		if(NameEndIndex > NameStartIndex)
		{
			InName = InName.Mid(NameStartIndex + 1,NameEndIndex - NameStartIndex - 1);
		}
	}

	// Handle specified packages.
	INT i;

	// if you're attempting to find an object in any package using a dotted name that isn't fully
	// qualified (such as ObjectName.SubobjectName - notice no package name there), you normally call
	// StaticFindObject and pass in ANY_PACKAGE as the value for InPackage.  When StaticFindObject calls ResolveName,
	// it passes NULL as the value for InPackage, rather than ANY_PACKAGE.  As a result, unless the first chunk of the
	// dotted name (i.e. ObjectName from the above example) is a UPackage, the object will not be found.  So here we attempt
	// to detect when this has happened - if we aren't attempting to create a package, and a UPackage with the specified
	// name couldn't be found, pass in ANY_PACKAGE as the value for InPackage to the call to FindObject<UObject>().
	while( (i=InName.InStr(TEXT("."))) != -1 )
	{
		FString PartialName = InName.Left(i);
		if( Create )
		{
			InPackage = CreatePackage( InPackage, *PartialName );
		}
		else
		{
			UObject* NewPackage = FindObject<UPackage>( InPackage, *PartialName );
			if( !NewPackage )
			{
				NewPackage = FindObject<UObject>( InPackage == NULL ? ANY_PACKAGE : InPackage, *PartialName );
				if( !NewPackage )
					return FALSE;
			}
			InPackage = NewPackage;
		}
		InName = InName.Mid(i+1);
	}
	return TRUE;
}

/**
 * Find or load an object by string name with optional outer and filename specifications.
 * These are optional because the InName can contain all of the necessary information.
 *
 * @param ObjectClass	The class (or a superclass) of the object to be loaded.
 * @param InOuter		An optional object to narrow where to find/load the object from
 * @param InName		String name of the object. If it's not fully qualified, InOuter and/or Filename will be needed
 * @param Filename		An optional file to load from (or find in the file's package object)
 * @param LoadFlags		Flags controlling how to handle loading from disk
 * @param Sandbox		A list of packages to restrict the search for the object
 * @param bAllowObjectReconciliation	Whether to allow the object to be found via FindObject in the case of seek free loading
 *
 * @return The object that was loaded or found. NULL for a failure.
 */
UObject* UObject::StaticLoadObject(UClass* ObjectClass, UObject* InOuter, const TCHAR* InName, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox, UBOOL bAllowObjectReconciliation )
{
	SCOPE_CYCLE_COUNTER(STAT_LoadObject);
	check(ObjectClass);
	check(InName);

	FString		StrName				= InName;
	UObject*	Result				=NULL;
	// Keep track of whether BeginLoad has been called in case we throw an exception.
	UBOOL		bNeedsEndLoadCalled = FALSE;

#if !EXCEPTIONS_DISABLED
	try
#endif
	{
		// break up the name into packages, returning the innermost name and its outer
		ResolveName(InOuter, StrName, TRUE, TRUE);
		FName ObjName(*StrName, FNAME_Add, TRUE);
		if ( InOuter != NULL )
		{
			if( bAllowObjectReconciliation && (GIsGame && !GIsEditor && !GIsUCC) )
			{
				Result = StaticFindObjectFast(ObjectClass, InOuter, ObjName);
			}

			if( !Result )
			{
				if( GUseSeekFreeLoading )
				{
					debugf(NAME_Warning,TEXT("StaticLoadObject for %s %s %s couldn't find object in memory!"),
						*ObjectClass->GetName(),
						*InOuter->GetName(),
						InName);
				}
				else
				{
					BeginLoad();
					bNeedsEndLoadCalled = TRUE;

					// the outermost will be the top-level package that is the root package for the file
					UPackage* TopOuter = InOuter->GetOutermost();

					// find or create a linker object which contains the object being loaded
					ULinkerLoad* Linker = NULL;
					if (!(LoadFlags & LOAD_DisallowFiles))
					{
						Linker = GetPackageLinker(TopOuter, Filename, LoadFlags | LOAD_Throw | LOAD_AllowDll, Sandbox, NULL);
					}
					// if we have a linker for the outermost package, use it to get the object from its export table
					if (Linker)
					{
						// handling an error case
						UBOOL bSkipCreate = FALSE;

						// make sure the InOuter has been loaded off disk so that it has a _LinkerIndex, which
						// Linker->Create _needs_ to be able to find the proper object (two objects with the same
						// name in different groups are only differentiated by their Outer's LinkerIndex!)
						if (InOuter != TopOuter && InOuter->GetLinkerIndex() == INDEX_NONE)
						{
							UObject* LoadedOuter = StaticLoadObject(InOuter->GetClass(), NULL, *InOuter->GetPathName(), Filename, LoadFlags, Sandbox, FALSE);
							// if the outer failed to load, or was otherwise a different object than what ResolveName made, 
							// they are using an invalid group name
							if (LoadedOuter != InOuter || LoadedOuter->GetLinkerIndex() == INDEX_NONE)
							{
								warnf(NAME_Warning, TEXT("The group '%s' does not exist in package %s [loading %s]"), *InOuter->GetPathName(), *TopOuter->GetName(), InName);

								// we can't call create without a properly loaded outer
								bSkipCreate = TRUE;
							}
						}
						// if no error yet, create it
						if (!bSkipCreate)
						{
							// InOuter should now be properly loaded with a linkerindex, so we can use it to create the actual desired object
							Result = Linker->Create(ObjectClass, ObjName, InOuter, LoadFlags, 0);
						}
					}

					// if we haven't created it yet, try to find it (which will always be the case if LOAD_DisallowFiles is on)
					if (!Result)
					{
						Result = StaticFindObjectFast(ObjectClass, InOuter, ObjName);
					}

					bNeedsEndLoadCalled = FALSE;
					EndLoad();
				}
			}
		}
		// if we haven't created or found the object, error
		if (!Result)
		{
#if EXCEPTIONS_DISABLED
			return NULL;
#else
			appThrowf(*LocalizeError(TEXT("ObjectNotFound"),TEXT("Core")), *ObjectClass->GetName(), InOuter ? *InOuter->GetPathName() : TEXT("None"), *StrName);
#endif
		}
	}
#if !EXCEPTIONS_DISABLED
	catch( const TCHAR* Error )
	{
		if( bNeedsEndLoadCalled )
		{
			EndLoad();
		}
		SafeLoadError(InOuter, LoadFlags, Error, *LocalizeError(TEXT("FailedLoadObject"),TEXT("Core")), *ObjectClass->GetName(), InOuter ? *InOuter->GetPathName() : TEXT("None"), *StrName, Error);
	}
#endif
	return Result;
}

//
// Load a class.
//
UClass* UObject::StaticLoadClass( UClass* BaseClass, UObject* InOuter, const TCHAR* InName, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox )
{
	check(BaseClass);
#if !EXCEPTIONS_DISABLED
	try
#endif
	{
		UClass* Class = LoadObject<UClass>( InOuter, InName, Filename, LoadFlags | LOAD_Throw, Sandbox );
		if( Class && !Class->IsChildOf(BaseClass) )
			appThrowf( *LocalizeError(TEXT("LoadClassMismatch"),TEXT("Core")), *Class->GetFullName(), *BaseClass->GetFullName() );
		return Class;
	}
#if !EXCEPTIONS_DISABLED
	catch( const TCHAR* Error )
	{
		// Failed.
		SafeLoadError( InOuter, LoadFlags, Error, Error );
		return NULL;
	}
#endif
}

/**
 * Loads a package and all contained objects that match context flags.
 *
 * @param	InOuter		Package to load new package into (usually NULL or ULevel->GetOuter())
 * @param	Filename	Name of file on disk
 * @param	LoadFlags	Flags controlling loading behavior
 * @return	Loaded package if successful, NULL otherwise
 */
UPackage* UObject::LoadPackage( UPackage* InOuter, const TCHAR* Filename, DWORD LoadFlags )
{
	UPackage* Result = NULL;

    if( *Filename == '\0' )
        return NULL;

	// Try to load.
	BeginLoad();
#if !EXCEPTIONS_DISABLED
	try
#endif
	{
		// Keep track of start time.
		DOUBLE StartTime = appSeconds();

		// Create a new linker object which goes off and tries load the file.
		ULinkerLoad* Linker = GetPackageLinker( InOuter, Filename ? Filename : *InOuter->GetName(), LoadFlags | LOAD_Throw, NULL, NULL );
        if( !Linker )
		{
			EndLoad();
            return( NULL );
		}

		if( !(LoadFlags & LOAD_Verify) )
		{
			Linker->LoadAllObjects();
		}
		Result = Linker->LinkerRoot;
		EndLoad();

		// Only set time it took to load package if the above EndLoad is the "outermost" EndLoad.
		if( Result && GObjBeginLoadCount == 0 && !(LoadFlags & LOAD_Verify) )
		{
			Result->SetLoadTime( appSeconds() - StartTime );
		}
	}
#if !EXCEPTIONS_DISABLED
	catch( const TCHAR* Error )
	{
		EndLoad();
		SafeLoadError( InOuter, LoadFlags, Error, *LocalizeError(TEXT("FailedLoadPackage"),TEXT("Core")), Error );
		Result = NULL;
	}
#endif
	if( GUseSeekFreeLoading && Result )
	{
		// We no longer need the linker. Passing in NULL would reset all loaders so we need to check for that.
		UObject::ResetLoaders( Result );
	}
	return Cast<UPackage>(Result);
}

//
// Verify a linker.
//
void UObject::VerifyLinker( ULinkerLoad* Linker )
{
	Linker->Verify();
}

/**
 * Dissociates all linker import and forced export object references. This currently needs to 
 * happen as the referred objects might be destroyed at any time.
 */
void UObject::DissociateImportsAndForcedExports()
{
	if( GImportCount )
	{
		for( INT LoaderIndex=0; LoaderIndex<GObjLoaders.Num(); LoaderIndex++ )
		{
			ULinkerLoad* Linker = GetLoader(LoaderIndex);
			//@todo optimization: only dissociate imports for loaders that had imports created
			//@todo optimization: since the last time this function was called.
			for( INT ImportIndex=0; ImportIndex<Linker->ImportMap.Num(); ImportIndex++ )
			{
				FObjectImport& Import = Linker->ImportMap(ImportIndex);
				if( Import.XObject && !Import.XObject->HasAnyFlags(RF_Native) )
				{
					Import.XObject = NULL;
				}
				Import.SourceLinker = NULL;
				// when the SourceLinker is reset, the SourceIndex must also be reset, or recreating
				// an import that points to a redirector will fail to find the redirector
				Import.SourceIndex = INDEX_NONE;
			}
		}
	}
	GImportCount = 0;

	if( GForcedExportCount )
	{
		for( INT LoaderIndex=0; LoaderIndex<GObjLoaders.Num(); LoaderIndex++ )
		{
			ULinkerLoad* Linker = GetLoader(LoaderIndex);
			//@todo optimization: only dissociate exports for loaders that had forced exports created
			//@todo optimization: since the last time this function was called.
			for( INT ExportIndex=0; ExportIndex<Linker->ExportMap.Num(); ExportIndex++ )
			{
				FObjectExport& Export = Linker->ExportMap(ExportIndex);
				if( Export._Object && Export.HasAnyFlags(EF_ForcedExport) )
				{
					Export._Object->SetLinker( NULL, INDEX_NONE );
					Export._Object = NULL;
				}
			}
		}
	}
	GForcedExportCount = 0;
}

//
// Begin loading packages.
//warning: Objects may not be destroyed between BeginLoad/EndLoad calls.
//
void UObject::BeginLoad()
{
	if( ++GObjBeginLoadCount == 1 )
	{
		// Make sure we're finishing up all pending async loads.
		FlushAsyncLoading();

		// Validate clean load state.
		check(GObjLoaded.Num()==0);
		check(!GAutoRegister);
	}
}

// Sort objects by linker name and file offset
IMPLEMENT_COMPARE_POINTER( UObject, UnObj,
{
	ULinker* LinkerA = A->GetLinker();
	ULinker* LinkerB = B->GetLinker();

	if( LinkerA && LinkerB )
	{
		if( LinkerA->GetFName() == LinkerB->GetFName() )
		{
			FObjectExport& ExportA = LinkerA->ExportMap( A->GetLinkerIndex() );
			FObjectExport& ExportB = LinkerB->ExportMap( B->GetLinkerIndex() );
			return ExportA.SerialOffset - ExportB.SerialOffset;
		}
		else
		{
			return LinkerA->GetFName().GetIndex() - LinkerB->GetFName().GetIndex();
		}
	}
	if( LinkerA == LinkerB )
	{
		return 0;
	}
	else
	{
		return LinkerA ? -1 : 1;
	}
}
)

//
// End loading packages.
//
void UObject::EndLoad()
{
	check(GObjBeginLoadCount>0);
	while( --GObjBeginLoadCount == 0 && (GObjLoaded.Num() || GImportCount || GForcedExportCount) )
	{
		// Make sure we're not recursively calling EndLoad as e.g. loading a config file could cause
		// BeginLoad/EndLoad to be called.
		GObjBeginLoadCount++;
#if !EXCEPTIONS_DISABLED
		try
#endif
		{
			// Temporary list of loaded objects as GObjLoaded might expand during iteration.
			TArray<UObject*> ObjLoaded;

			while( GObjLoaded.Num() )
			{
				// Accumulate till GObjLoaded no longer increases.
				ObjLoaded += GObjLoaded;
				GObjLoaded.Empty();

				// Sort by Filename and Offset.
				Sort<USE_COMPARE_POINTER(UObject,UnObj)>( &ObjLoaded(0), ObjLoaded.Num() );

				// Finish loading everything.
				debugfSlow( NAME_DevLoad, TEXT("Loading objects...") );
				for( INT i=0; i<ObjLoaded.Num(); i++ )
				{
					// Preload.
					UObject* Obj = ObjLoaded(i);
					if( Obj->HasAnyFlags(RF_NeedLoad) )
					{
						check(Obj->GetLinker());
						Obj->GetLinker()->Preload( Obj );
					}
				}

				// Start over again as new objects have been loaded that need to have "Preload" called on them before
				// we can safely PostLoad them.
				if(GObjLoaded.Num())
				{
					continue;
				}

				// set this so that we can perform certain operations in which are only safe once all objects have been de-serialized.
				GIsRoutingPostLoad = TRUE;

				// Postload objects.
				for( INT i=0; i<ObjLoaded.Num(); i++ )
				{
					UObject* Obj = ObjLoaded(i);
					check(Obj);
					Obj->ConditionalPostLoad();
				}

				GIsRoutingPostLoad = FALSE;

				// Empty array before next iteration as we finished postloading all objects.
				ObjLoaded.Empty( GObjLoaded.Num() );
			}

			// Dissociate all linker import and forced export object references, since they
			// may be destroyed, causing their pointers to become invalid.
			DissociateImportsAndForcedExports();
		}
#if !EXCEPTIONS_DISABLED
		catch( const TCHAR* Error )
		{
			// Any errors here are fatal.
			GError->Logf( Error );
		}
#endif
	}
}

//
// Empty the loaders.
//
void UObject::ResetLoaders( UObject* InPkg )
{
	// Make sure we're not in the middle of loading something in the background.
	FlushAsyncLoading();

	// Top level package to reset loaders for.
	UObject*		TopLevelPackage = InPkg ? InPkg->GetOutermost() : NULL;
	// Linker to reset/ detach.
	ULinkerLoad*	LinkerToReset	= NULL;

	// Find loader/ linker associated with toplevel package. We do this upfront as Detach resets LinkerRoot.
	if( TopLevelPackage )
	{
		for( INT i=GObjLoaders.Num()-1; i>=0; i-- )
		{
			ULinkerLoad* Linker = GetLoader(i);
			if( Linker->LinkerRoot == TopLevelPackage )
			{
				LinkerToReset = Linker;
				break;
			}
		}
	}

	// Need to iterate in reverse order as Detach removes entries from GObjLoaders array.
	if ( TopLevelPackage == NULL || LinkerToReset != NULL )
	{
		for( INT i=GObjLoaders.Num()-1; i>=0; i-- )
		{
			ULinkerLoad* Linker = GetLoader(i);
			// Detach linker if it has matching linker root or we are detaching all linkers.
			if( !TopLevelPackage || Linker->LinkerRoot==TopLevelPackage )
			{
				// Detach linker, also removes from array and sets LinkerRoot to NULL.
				Linker->Detach( TRUE );
			}
			// Detach LinkerToReset from other linker's import table.
			else
			{
				for( INT j=0; j<Linker->ImportMap.Num(); j++ )
				{
					if( Linker->ImportMap(j).SourceLinker == LinkerToReset )
					{
						Linker->ImportMap(j).SourceLinker	= NULL;
						Linker->ImportMap(j).SourceIndex	= INDEX_NONE;
					}
				}
			}
		}
	}
}

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

//
// Return whether statics are initialized.
//
UBOOL UObject::GetInitialized()
{
	return UObject::GObjInitialized;
}

//
// Return the static transient package.
//
UPackage* UObject::GetTransientPackage()
{
	return UObject::GObjTransientPkg;
}

//
// Return the ith loader.
//?
ULinkerLoad* UObject::GetLoader( INT i )
{
	return GObjLoaders(i);
}

//
// Add an object to the root set. This prevents the object and all
// its descendants from being deleted during garbage collection.
//
void UObject::AddToRoot()
{
	SetFlags( RF_RootSet );
}

//
// Remove an object from the root set.
//
void UObject::RemoveFromRoot()
{
	ClearFlags( RF_RootSet );
}

/*-----------------------------------------------------------------------------
	Object name functions.
-----------------------------------------------------------------------------*/

/**
 * Create a unique name by combining a base name and an arbitrary number string.
 * The object name returned is guaranteed not to exist.
 *
 * @param	Parent		the outer for the object that needs to be named
 * @param	Class		the class for the object
 * @param	BaseName	optional base name to use when generating the unique object name; if not specified, the class's name is used
 *
 * @return	name is the form BaseName_##, where ## is the number of objects of this
 *			type that have been created since the last time the class was garbage collected.
 */
FName UObject::MakeUniqueObjectName( UObject* Parent, UClass* Class, FName BaseName/*=NAME_None*/ )
{
	check(Class);
	if ( BaseName == NAME_None )
	{
		BaseName = Class->GetFName();
	}

	// cache the class's name's index for faster name creation later
	EName BaseNameIndex = (EName)BaseName.GetIndex();
	FName TestName;
	do
	{
		// create the next name in the sequence for this class
		TestName = FName(BaseNameIndex, ++Class->ClassUnique);
	} 
	while( StaticFindObjectFastInternal( NULL, Parent, TestName, FALSE, Parent==ANY_PACKAGE, 0 ) );

	return TestName;
}

/*-----------------------------------------------------------------------------
	Object hashing.
-----------------------------------------------------------------------------*/

//
// Add an object to the hash table.
//
void UObject::HashObject()
{
	INT iHash       = GetObjectHash( Name );
	HashNext        = GObjHash[iHash];
	GObjHash[iHash] = this;
	// Now hash using the outer
	iHash			= GetObjectOuterHash(Name,(DWORD)Outer);
	HashOuterNext	= GObjHashOuter[iHash];
	GObjHashOuter[iHash] = this;
}

//
// Remove an object from the hash table.
//
void UObject::UnhashObject()
{
	INT       iHash   = GetObjectHash( Name );
	UObject** Hash    = &GObjHash[iHash];
#if _DEBUG
	INT       Removed = 0;
#endif
	while( *Hash != NULL )
	{
		if( *Hash != this )
		{
			Hash = &(*Hash)->HashNext;
 		}
		else
		{
			*Hash = (*Hash)->HashNext;
#if _DEBUG
			// Verify that we find one and just one object in debug builds.		
			Removed++;
#else
			break;
#endif
		}
	}
#if _DEBUG
	checkSlow(Removed != 0);
	checkSlow(Removed == 1);
#endif
	// Remove the object from the outer hash.
	// NOTE: It relies on the outer being untouched and treats it as an int to avoid potential crashes during GC
	iHash   = GetObjectOuterHash(Name,(DWORD)Outer);
	Hash    = &GObjHashOuter[iHash];
#if _DEBUG
	Removed = 0;
#endif
	// Search through the hash bin and remove the item
	while( *Hash != NULL )
	{
		if( *Hash != this )
		{
			Hash = &(*Hash)->HashOuterNext;
 		}
		else
		{
			*Hash = (*Hash)->HashOuterNext;
#if _DEBUG
			Removed++;
#else
			return;
#endif
		}
	}
#if _DEBUG
	checkSlow(Removed != 0);
	checkSlow(Removed == 1);
#endif
}

/*-----------------------------------------------------------------------------
	Creating and allocating data for new objects.
-----------------------------------------------------------------------------*/

//
// Add an object to the table.
//
void UObject::AddObject( INT InIndex )
{
	// Find an available index.
	if( InIndex==INDEX_NONE )
	{
		// Special non- garbage collectable range.
		if( HasAnyFlags( RF_DisregardForGC ) && (++GObjLastNonGCIndex < GObjFirstGCIndex) )
		{
			// "Last" was pre-incremented above. This is done to allow keeping track of how many objects we requested
			// to be disregarded, regardless of actual allocated pool size.
			InIndex = GObjLastNonGCIndex;
		}
		// Regular pool/ range.
		else
		{
			if( GObjAvailable.Num() )
			{
				InIndex = GObjAvailable.Pop();
				check(GObjObjects(InIndex)==NULL);
			}
			else
			{
				InIndex = GObjObjects.Add();
			}
		}
	}

	// Clear object flag signalling disregard for GC if object was allocated in garbage collectable range.
	if( InIndex >= GObjFirstGCIndex )
	{
		// Object is allocated in regular pool so we need to clear RF_DisregardForGC if it was set.
		ClearFlags( RF_DisregardForGC );
	}

	// Make sure only objects in disregarded index range have the object flag set.
	check( !HasAnyFlags( RF_DisregardForGC ) || (InIndex < GObjFirstGCIndex) );
	// Make sure that objects disregarded for GC are part of root set.
	check( !HasAnyFlags( RF_DisregardForGC ) || HasAnyFlags( RF_RootSet ) );

	// Add to global table.
	GObjObjects(InIndex) = this;
	Index = InIndex;
	HashObject();
}

/**
 * Create a new instance of an object or replace an existing object.  If both an Outer and Name are specified, and there is an object already in memory with the same Class, Outer, and Name, the
 * existing object will be destructed, and the new object will be created in its place.
 * 
 * @param	Class		the class of the object to create
 * @param	InOuter		the object to create this object within (the Outer property for the new object will be set to the value specified here).
 * @param	Name		the name to give the new object. If no value (NAME_None) is specified, the object will be given a unique name in the form of ClassName_#.
 * @param	SetFlags	the ObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
 * @param	Template	if specified, the property values from this object will be copied to the new object, and the new object's ObjectArchetype value will be set to this object.
 *						If NULL, the class default object is used instead.
 * @param	Error		the output device to use for logging errors
 * @param	Ptr			the address to use for allocating the new object.  If specified, new object will be created in the same memory block as the existing object.
 * @param	SubobjectRoot
 *						Only used to when duplicating or instancing objects; in a nested subobject chain, corresponds to the first object that is not a subobject.
 *						A value of INVALID_OBJECT for this parameter indicates that we are calling StaticConstructObject to duplicate or instance a non-subobject (which will be the subobject root for any subobjects of the new object)
 *						A value of NULL indicates that we are not instancing or duplicating an object.
 * @param	InstanceGraph
 *						contains the mappings of instanced objects and components to their templates
 *
 * @return	a pointer to a fully initialized object of the specified class.
 */
UObject* UObject::StaticAllocateObject
(
	UClass*			InClass,
	UObject*		InOuter,
	FName			InName,
	EObjectFlags	InFlags,
	UObject*		InTemplate,
	FOutputDevice*	Error,
	UObject*		Ptr,
	UObject*		SubobjectRoot,
	FObjectInstancingGraph* InstanceGraph
)
{
	check(Error);
	check(!InClass || InClass->ClassWithin);
	check(!InClass || InClass->ClassConstructor);

	// Validation checks.
	if( !InClass )
	{
		Error->Logf( TEXT("Empty class for object %s"), *InName.ToString() );
		return NULL;
	}
	if( InClass->GetIndex()==INDEX_NONE && GObjRegisterCount==0 )
	{
		Error->Logf( TEXT("Unregistered class for %s"), *InName.ToString() );
		return NULL;
	}
	// for abstract classes that are being loaded NOT in the editor we want to error.  If they are in the editor we do not want to have an error
	if( ( (InClass->ClassFlags & CLASS_Abstract) != 0 )
		&& ( (InFlags&RF_ClassDefaultObject) == 0 )
		&& ( GIsEditor != TRUE )
		)
	{
		Error->Logf( *LocalizeError(TEXT("Abstract"),TEXT("Core")), *InName.ToString(), *InClass->GetName() );
		return NULL;
	}
	// if we are trying instantiate an abstract class in the editor we want to just mark as deprecated and let it be nulled out
	if( ( (InClass->ClassFlags & CLASS_Abstract) != 0 )
		&& ( (InFlags&RF_ClassDefaultObject) == 0 )
		&& ( GIsEditor == TRUE )
		)
	{
		warnf( NAME_Error, TEXT("Class which was marked abstract was trying to be loaded.  It has been marked as Deprecated so it can correctly be nulled out on save. %s %s"), *InName.ToString(), *InClass->GetName() );
		InClass->ClassFlags |= CLASS_Deprecated;
	}

	if( InOuter == NULL )
	{
		if ( InClass!=UPackage::StaticClass() )
		{
			Error->Logf( *LocalizeError(TEXT("NotPackaged"),TEXT("Core")), *InClass->GetName(), *InName.ToString() );
			return NULL;
		}
		else if ( InName == NAME_None )
		{
			Error->Logf( *LocalizeError(TEXT("PackageNamedNone"), TEXT("Core")) );
			return NULL;
		}
	}
	if( InOuter && !InOuter->IsA(InClass->ClassWithin) && (InFlags&RF_ClassDefaultObject) == 0 )
	{
		Error->Logf( *LocalizeError(TEXT("NotWithin"),TEXT("Core")), *InClass->GetName(), *InName.ToString(), *InOuter->GetClass()->GetName(), *InClass->ClassWithin->GetName() );
		return NULL;
	}
	if ( (InFlags & RF_ClassDefaultObject) != 0 )
	{
		TCHAR Result[NAME_SIZE] = DEFAULT_OBJECT_PREFIX;
//		FName CurrentClassName = InClass->GetFName();
		FString ClassName = InClass->Index == INDEX_NONE
			? *(TCHAR* const*)&InClass->Name
			: InClass->Name.ToString();

		appStrncat(Result, *ClassName, NAME_SIZE);
		InName = FName(Result);
	}
	else if ( InClass->IsMisaligned() )
	{
		appThrowf(*LocalizeError(TEXT("NativeRebuildRequired"), TEXT("Core")), *InClass->GetName() );
	}

	UObject* Obj = NULL;
	// Compose name, if unnamed.
	if( InName==NAME_None )
	{
		InName = MakeUniqueObjectName( InOuter, InClass );
	}
	else
	{
		// See if object already exists.
		Obj = StaticFindObjectFastInternal( InClass, InOuter, InName );//oldver: Should use NULL instead of InClass to prevent conflicts by name rather than name-class.
	}

	UClass*			Cls							= ExactCast<UClass>( Obj );
	INT				Index						= INDEX_NONE;
	UClass*			ClassWithin					= NULL;
	UObject*		DefaultObject				= NULL;
	UObject*		ObjectArchetype				= InTemplate;
	DWORD			ClassFlags					= 0;
	BYTE			PlatformFlags				= 0;
	void			(*ClassConstructor)(void*)	= NULL;
	ULinkerLoad*	Linker						= NULL;
	INT				LinkerIndex					= INDEX_NONE;
	INT				OldNetIndex					= INDEX_NONE;

	/*
	remember the StaticClassConstructor for the class so that we can restore it after we've call
	appMemzero() on the object's memory address, which results in the non-intrinsic classes losing
	its ClassStaticConstructor.

	@fixme rjp - there was a reason I wasn't restoring this before, but not sure why now. If you find
	a bug that you can track down to restoring the StaticClassConstructor, please document why.  :)
	*/
	void			(UObject::*StaticClassConstructor)() = NULL;

	void			(UObject::*StaticClassInitializer)() = NULL;
	if( !Obj )
	{
		// Figure out size, alignment and aligned size of object.
		INT Size		= InClass->GetPropertiesSize();
		// Enforce 8 byte alignment to ensure ObjectFlags are never accessed misaligned.
		INT Alignment	= Max( 8, InClass->GetMinAlignment() );
		INT AlignedSize = Align( Size, Alignment );

		// Reuse existing pointer, in-place replace.
		if( Ptr )
		{
			Obj = Ptr;
		}
		// Use object memory pool for objects disregarded by GC (initially loaded ones). This allows identifying their
		// GC status by simply looking at their address.
		else if( (InFlags & RF_DisregardForGC) && (Align(GPermanentObjectPoolTail,Alignment) + Size) <= (GPermanentObjectPool + GPermanentObjectPoolSize) )
		{
			// Align current tail pointer and use it for object. 
			BYTE* AlignedPtr = Align( GPermanentObjectPoolTail, Alignment );
			Obj = (UObject*) AlignedPtr;
			// Update tail pointer.
			GPermanentObjectPoolTail = AlignedPtr + Size;
		}
		// Allocate new memory of the appropriate size and alignment.
		else
		{
			Obj = (UObject*)appMalloc( AlignedSize );
		}

		// InClass->Index == INDEX_NONE when InClass hasn't been fully initialized yet (during static registration)
		// thus its Outer and Name won't be valid yet (@see UObject::Register)
		if ( InClass->Index != INDEX_NONE && ObjectArchetype == NULL && InClass != UObject::StaticClass() )
		{
			ObjectArchetype = InClass->GetDefaultObject();
		}
	}
	else
	{
		// Replace an existing object without affecting the original's address or index.
		check(!Ptr || Ptr==Obj);
		check(!GIsAsyncLoading);
		check(!Obj->HasAnyFlags(RF_Unreachable|RF_AsyncLoading));
		debugfSlow( NAME_DevReplace, TEXT("Replacing %s"), *Obj->GetName() );

		// Can only replace if class is identical.
		if( Obj->GetClass() != InClass )
		{	
			appErrorf( *LocalizeError(TEXT("NoReplace"),TEXT("Core")), *Obj->GetFullName(), *InClass->GetName() );
		}

		// Remember linker, flags, index, and native class info.
		Linker		= Obj->_Linker;
		LinkerIndex = Obj->_LinkerIndex;
		InFlags		|= Obj->GetMaskedFlags(RF_Keep);
		Index		= Obj->Index;
		OldNetIndex = Obj->NetIndex;

		// if this is the class default object, it means that InClass is native and the CDO was created during static registration.
		// Propagate the load flags but don't replace it - the CDO will be initialized by InitClassDefaultObject
		if ( Obj->HasAnyFlags(RF_ClassDefaultObject) )
		{
			Obj->ObjectFlags |= InFlags;
			Obj->ClearFlags(RF_NeedPostLoad|RF_NeedPostLoadSubobjects);
			return Obj;
		}

		// if this object was found and we were searching for the class default object
		// it should be marked as such
		check((InFlags&RF_ClassDefaultObject)==0);
		checkf(!Obj->HasAnyFlags(RF_NeedLoad), *LocalizeError(TEXT("ReplaceNotFullyLoaded_f"),TEXT("Core")), *Obj->GetFullName() );
		if ( Obj->HasAnyFlags(RF_NeedPostLoad) )
		{
			// this is only safe while we are routing PostLoad to all loaded objects - i.e. it is unsafe to do if we are still in the Preload
			// portion of UObject::EndLoad
			checkf(GIsRoutingPostLoad,*LocalizeError(TEXT("ReplaceNoPostLoad_f"),TEXT("Core")),*Obj->GetFullName());

			// Clear these flags so that we don't trigger the assertion in UObject::SetLinker.  We don't call ConditionalPostLoad (which would take of
			// all this for us) because we don't want the object to do all of the subobject/component instancing stuff because we're about to destruct
			// this object
			Obj->ClearFlags(RF_NeedPostLoad|RF_DebugPostLoad);

			// It is assumed that if an object is serialized, it will always have PostLoad called on it.  Based on this assumption, some
			// classes might be doing two-part operatiosn where the first part happens in Serialize and the second part happens in PostLoad.
			// So if we're about to replace the object and it has been loaded but still needs PostLoad called, do that now.
			Obj->PostLoad();
		}

		if ( ObjectArchetype == NULL )
		{
			ObjectArchetype = Obj->ObjectArchetype
				? Obj->ObjectArchetype
				: Obj->GetClass()->GetDefaultObject();
		}

		if( Cls )
		{
			ClassWithin		 = Cls->ClassWithin;
			ClassFlags       = Cls->ClassFlags & CLASS_Abstract;
			PlatformFlags    = Cls->ClassPlatformFlags;
			ClassConstructor = Cls->ClassConstructor;
			StaticClassConstructor = Cls->ClassStaticConstructor;
			StaticClassInitializer = Cls->ClassStaticInitializer;
			
			DefaultObject	 = Cls->GetDefaultsCount() ? Cls->GetDefaultObject() : NULL;
		}

		// Destroy the object.
		GIsReplacingObject = TRUE;
		Obj->~UObject();
		GIsReplacingObject = FALSE;
		check(GObjAvailable.Num() && GObjAvailable.Last()==Index);
		GObjAvailable.Pop();
	}

	if( (InFlags&RF_ClassDefaultObject) == 0 )
	{
		// If class is transient, objects must be transient.
		if ( (InClass->ClassFlags & CLASS_Transient) != 0 )
		{
            InFlags |= RF_Transient;
		}
	}
	else
	{
		// never call PostLoad on class default objects
		InFlags &= ~(RF_NeedPostLoad|RF_NeedPostLoadSubobjects);
	}

	// If the class is marked PerObjectConfig, mark the object as needing per-instance localization.
	if ( (InClass->ClassFlags & CLASS_PerObjectConfig) != 0 )
	{
		InFlags |= RF_PerObjectLocalized;
	}

	appMemzero( Obj, InClass->GetPropertiesSize() );

	// Set the base properties.
	Obj->Index			 = INDEX_NONE;
	Obj->HashNext		 = NULL;
	Obj->StateFrame      = NULL;
	Obj->_Linker		 = Linker;
	Obj->_LinkerIndex	 = LinkerIndex;
	Obj->Outer			 = InOuter;
	Obj->ObjectFlags	 = InFlags;
	Obj->Name			 = InName;
	Obj->Class			 = InClass;
	Obj->ObjectArchetype = ObjectArchetype;

	// Reassociate the object with it's linker.
	if(Linker)
	{
		check(Linker->ExportMap(LinkerIndex)._Object == NULL);
		Linker->ExportMap(LinkerIndex)._Object = Obj;

		FString ObjectName;
		if ( InOuter != NULL )
		{
			ObjectName = InOuter->GetPathName() + TEXT(".");
		}

		ObjectName += InName.ToString();
		debugf(TEXT("Reassociating %s with %s serving %s"),*ObjectName,*Linker->GetName(),*Linker->LinkerRoot->GetPathName());
	}

	// Init the properties.
	UClass* BaseClass = Obj->HasAnyFlags(RF_ClassDefaultObject) ? InClass->GetSuperClass() : InClass;
	if ( BaseClass == NULL )
	{
		check(InClass==UObject::StaticClass());
		BaseClass = InClass;
	}

	INT DefaultsCount = ObjectArchetype
		? ObjectArchetype->GetClass()->GetPropertiesSize()
		: BaseClass->GetPropertiesSize();

	if ( InstanceGraph != NULL )
	{
		if ( InstanceGraph->IsInitialized() )
		{
			InstanceGraph->AddObjectPair(Obj, ObjectArchetype);
		}
		else
		{
			InstanceGraph->SetDestinationRoot(Obj);
		}
	}

	Obj->SafeInitProperties( (BYTE*)Obj, InClass->GetPropertiesSize(), BaseClass, (BYTE*)ObjectArchetype, DefaultsCount, Obj->HasAnyFlags(RF_NeedLoad) ? NULL : Obj, SubobjectRoot, InstanceGraph );

	// reset NetIndex after InitProperties so that the value from the template is ignored
	Obj->NetIndex = INDEX_NONE;
	Obj->SetNetIndex(OldNetIndex);

	// Add to global table.
	Obj->AddObject( Index );
	check(Obj->IsValid());

	// Restore class information if replacing native class.
	if( Cls )
	{
		Cls->ClassWithin				= ClassWithin;
		Cls->ClassFlags					|= ClassFlags;
		Cls->ClassPlatformFlags			|= PlatformFlags;
		Cls->ClassConstructor			= ClassConstructor;
		Cls->ClassStaticConstructor		= StaticClassConstructor;
		Cls->ClassStaticInitializer		= StaticClassInitializer;
		Cls->ClassDefaultObject			= DefaultObject;
	}

	// Mark objects created during async loading process (e.g. from within PostLoad or CreateExport) as async loaded so they 
	// cannot be found. This requires also keeping track of them so we can remove the async loading flag later one when we 
	// finished routing PostLoad to all objects.
	if( GIsAsyncLoading )
	{
		Obj->SetFlags( RF_AsyncLoading );
		GObjConstructedDuringAsyncLoading.AddItem(Obj);
	}

	// Success.
	return Obj;
}

/**
 * Create a new instance of an object.  The returned object will be fully initialized.  If InFlags contains RF_NeedsLoad (indicating that the object still needs to load its object data from disk), components
 * are not instanced (this will instead occur in PostLoad()).  The different between StaticConstructObject and StaticAllocateObject is that StaticConstructObject will also call the class constructor on the object
 * and instance any components.
 * 
 * @param	Class		the class of the object to create
 * @param	InOuter		the object to create this object within (the Outer property for the new object will be set to the value specified here).
 * @param	Name		the name to give the new object. If no value (NAME_None) is specified, the object will be given a unique name in the form of ClassName_#.
 * @param	SetFlags	the ObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
 * @param	Template	if specified, the property values from this object will be copied to the new object, and the new object's ObjectArchetype value will be set to this object.
 *						If NULL, the class default object is used instead.
 * @param	Error		the output device to use for logging errors
 * @param	SubobjectRoot
 *						Only used to when duplicating or instancing objects; in a nested subobject chain, corresponds to the first object that is not a subobject.
 *						A value of INVALID_OBJECT for this parameter indicates that we are calling StaticConstructObject to duplicate or instance a non-subobject (which will be the subobject root for any subobjects of the new object)
 *						A value of NULL indicates that we are not instancing or duplicating an object.
 * @param	InstanceGraph
 *						contains the mappings of instanced objects and components to their templates
 *
 * @return	a pointer to a fully initialized object of the specified class.
 */
UObject* UObject::StaticConstructObject
(
	UClass*			InClass,
	UObject*		InOuter								/*=GetTransientPackage()*/,
	FName			InName								/*=NAME_None*/,
	EObjectFlags	InFlags								/*=0*/,
	UObject*		InTemplate							/*=NULL*/,
	FOutputDevice*	Error								/*=GError*/,
	UObject*		SubobjectRoot						/*=NULL*/,
	FObjectInstancingGraph* InInstanceGraph				/*=NULL*/
)
{
	SCOPE_CYCLE_COUNTER(STAT_ConstructObject);
	check(Error);

	FObjectInstancingGraph* InstanceGraph = InInstanceGraph;
	if ( InstanceGraph == NULL && InClass->HasAnyClassFlags(CLASS_HasComponents) )
	{
		InstanceGraph = new FObjectInstancingGraph;
	}

	// Allocate the object.
	UObject* Result = StaticAllocateObject( InClass, InOuter, InName, InFlags, InTemplate, Error, NULL, SubobjectRoot, InstanceGraph );

	if( Result )
	{
		const UBOOL bCurrentValue = GIsAffectingClassDefaultObject;
		GIsAffectingClassDefaultObject = (InFlags&RF_ClassDefaultObject) != 0;

		// call the base UObject class constructor if the class is misaligned (i.e. a native class that is currently being recompiled)
		if ( !InClass->IsMisaligned() )
		{
			(*InClass->ClassConstructor)( Result );
		}
		else
		{
			(*UObject::StaticClass()->ClassConstructor)( Result );
		}

		GIsAffectingClassDefaultObject = bCurrentValue;

		// if InFlags & RF_NeedLoad, it means that this object is being created either
		// by the linker, as a result of a call to CreateExport(), or that this object's
		// archetype has not yet been completely initialized (most likely not yet serialized)
		// In this case, components will be safely instanced once the archetype for the Result
		// object has been completely initialized.
		if( (InFlags & RF_NeedLoad) == 0 )
		{
			if (
				// if we're not running make
				!GIsUCCMake &&													

				// and InClass is PerObjectConfig
				InClass->HasAnyClassFlags(CLASS_PerObjectConfig) &&
				
				// and we're not creating a CDO or Archetype object
				(InFlags&(RF_ClassDefaultObject|RF_ArchetypeObject)) == 0 )		
			{
				Result->LoadConfig();
				Result->LoadLocalized();
			}

			if ( InClass->HasAnyClassFlags(CLASS_HasComponents) )
			{
				const UBOOL bIsDefaultObject = Result->HasAnyFlags(RF_ClassDefaultObject);

				if ( bIsDefaultObject == FALSE )
				{
					// if the caller hasn't disabled component instancing, instance the components for the new object now.
					if ( InstanceGraph->IsComponentInstancingEnabled() )
					{
						InClass->InstanceComponentTemplates((BYTE*)Result, (BYTE*)Result->GetArchetype(),
							Result->GetArchetype() ? Result->GetArchetype()->GetClass()->GetPropertiesSize() : NULL,
							Result, InstanceGraph);
					}
				}
			}
		}
	}

	if ( InInstanceGraph == NULL && InstanceGraph != NULL )
	{
		delete InstanceGraph;
		InstanceGraph = NULL;
	}

	return Result;
}

/*-----------------------------------------------------------------------------
   Duplicating Objects.
-----------------------------------------------------------------------------*/

/**
 * Constructor - zero initializes all members
 */
FObjectDuplicationParameters::FObjectDuplicationParameters( UObject* InSourceObject, UObject* InDestOuter )
: SourceObject(InSourceObject), DestOuter(InDestOuter), DestName(NAME_None)
, FlagMask(RF_AllFlags), ApplyFlags(0), DestClass(NULL), bMigrateArchetypes(FALSE), CreatedObjects(NULL)
{
	checkSlow(SourceObject);
	checkSlow(DestOuter);
	checkSlow(SourceObject->IsValid());
	checkSlow(DestOuter->IsValid());
	DestClass = SourceObject->GetClass();
}

/**
 * Creates a copy of SourceObject using the Outer and Name specified, as well as copies of all objects contained by SourceObject.  
 * Any objects referenced by SourceOuter or RootObject and contained by SourceOuter are also copied, maintaining their name relative to SourceOuter.  Any
 * references to objects that are duplicated are automatically replaced with the copy of the object.
 *
 * @param	SourceObject	the object to duplicate
 * @param	RootObject		should always be the same value as SourceObject (unused)
 * @param	DestOuter		the object to use as the Outer for the copy of SourceObject
 * @param	DestName		the name to use for the copy of SourceObject
 * @param	FlagMask		a bitmask of EObjectFlags that should be propagated to the object copies.  The resulting object copies will only have the object flags
 *							specified copied from their source object.
 * @param	DestClass		optional class to specify for the destination object. MUST BE SERIALIZATION COMPATIBLE WITH SOURCE OBJECT!!!
 *
 * @return	the duplicate of SourceObject.
 *
 * @note: this version is deprecated in favor of StaticDuplicateObjectEx
 */
UObject* UObject::StaticDuplicateObject(UObject* SourceObject,UObject* RootObject,UObject* DestOuter,const TCHAR* DestName,EObjectFlags FlagMask/*=~0*/,UClass* DestClass/*=NULL*/, UBOOL bMigrateArchetypes/*=FALSE*/)
{
	FObjectDuplicationParameters Parameters(SourceObject, DestOuter);
	if ( DestName && appStrcmp(DestName,TEXT("")) )
	{
		Parameters.DestName = FName(DestName, FNAME_Add, TRUE);
	}

	if ( DestClass == NULL )
	{
		Parameters.DestClass = SourceObject->GetClass();
	}
	else
	{
		Parameters.DestClass = DestClass;
	}
	Parameters.FlagMask = FlagMask;
	Parameters.bMigrateArchetypes = bMigrateArchetypes;

	return StaticDuplicateObjectEx(Parameters);
}

UObject* UObject::StaticDuplicateObjectEx( FObjectDuplicationParameters& Parameters )
{
	// make sure the two classes are the same size, as this hopefully will mean they are serialization
	// compatible. It's not a guarantee, but will help find errors
	check(Parameters.DestClass->GetPropertiesSize() == Parameters.SourceObject->GetClass()->GetPropertiesSize());
	FObjectInstancingGraph InstanceGraph;

	// disable object and component instancing while we're duplicating objects, as we're going to instance components manually a little further below
	InstanceGraph.EnableObjectInstancing(FALSE);
	InstanceGraph.EnableComponentInstancing(FALSE);

	// we set this flag so that the component instancing code doesn't think we're creating a new archetype, because when creating a new archetype,
	// the ObjectArchetype for instanced components is set to the ObjectArchetype of the source component, which in the case of duplication (or loading)
	// will be changing the archetype's ObjectArchetype to the wrong object (typically the CDO or something)
	InstanceGraph.SetLoadingObject(TRUE);

	UObject*								DupObject = UObject::StaticConstructObject(
															Parameters.DestClass,Parameters.DestOuter,Parameters.DestName,
															Parameters.ApplyFlags|Parameters.SourceObject->GetMaskedFlags(Parameters.FlagMask),
															Parameters.SourceObject->GetArchetype(),
															GError, INVALID_OBJECT, &InstanceGraph);

	TArray<BYTE>							ObjectData;
	TMap<UObject*,FDuplicatedObjectInfo*>	DuplicatedObjects;

	// if seed objects were specified, add those to the DuplicatedObjects map now
	if ( Parameters.DuplicationSeed.Num() > 0 )
	{
		for ( TMap<UObject*,UObject*>::TIterator It(Parameters.DuplicationSeed); It; ++It )
		{
			UObject* Src = It.Key();
			UObject* Dup = It.Value();
			checkSlow(Src);
			checkSlow(Dup);

			// create the DuplicateObjectInfo for this object
			FDuplicatedObjectInfo* Info = DuplicatedObjects.Set(Src,new FDuplicatedObjectInfo());
			Info->DupObject = Dup;
		}
	}

	FDuplicateDataWriter					Writer(DuplicatedObjects,ObjectData,Parameters.SourceObject,DupObject,Parameters.FlagMask,Parameters.ApplyFlags,&InstanceGraph);
	TArray<UObject*>						SerializedObjects;
	//UObject*								DupRoot = Writer.GetDuplicatedObject(Parameters.SourceObject);

	InstanceGraph.SetDestinationRoot(DupObject, DupObject->GetArchetype());
	while(Writer.UnserializedObjects.Num())
	{
		UObject*	Object = Writer.UnserializedObjects.Pop();
		Object->Serialize(Writer);
		SerializedObjects.AddItem(Object);
	};

	FDuplicateDataReader	Reader(DuplicatedObjects,ObjectData);
	for(INT ObjectIndex = 0;ObjectIndex < SerializedObjects.Num();ObjectIndex++)
	{
		FDuplicatedObjectInfo*	ObjectInfo = DuplicatedObjects.FindRef(SerializedObjects(ObjectIndex));
		check(ObjectInfo);
		if ( !SerializedObjects(ObjectIndex)->HasAnyFlags(RF_ClassDefaultObject) )
		{
			ObjectInfo->DupObject->Serialize(Reader);
		}
		else
		{
			// if the source object was a CDO, then transient property values were serialized by the FDuplicateDataWriter
			// and in order to read those properties out correctly, we'll need to enable defaults serialization on the
			// reader as well.
			Reader.StartSerializingDefaults();
			ObjectInfo->DupObject->Serialize(Reader);
			Reader.StopSerializingDefaults();
		}
	}

/*
	@note ronp: this should be completely unecessary since all subobject and component references should have been handled by the duplication archives.
	for(TMap<UObject*,FDuplicatedObjectInfo*>::TIterator It(DuplicatedObjects);It;++It)
	{
		UObject* OrigObject = It.Key();
		FDuplicatedObjectInfo* DupObjectInfo = It.Value();
		UObject* Duplicate = DupObjectInfo->DupObject;

		// don't include any objects which were included in the duplication seed map in the instance graph, as the "duplicate" of these objects
		// may not necessarily be the object that is supposed to be its archetype (the caller can populate the duplication seed map with any objects they wish)
		// and the DuplicationSeed is only used for preserving inter-object references, not for object graphs in SCO
		if ( Parameters.DuplicationSeed.Find(OrigObject) == NULL )
		{
			UComponent* DuplicateComponent = Cast<UComponent>(Duplicate);
			if ( DuplicateComponent != NULL )
			{
				InstanceGraph.AddComponentPair(DuplicateComponent->GetArchetype<UComponent>(), DuplicateComponent);
			}
			else
			{
				InstanceGraph.AddObjectPair(Duplicate);
			}
		}
	}
*/

	InstanceGraph.EnableComponentInstancing(TRUE);
	InstanceGraph.EnableObjectInstancing(TRUE);

	for(TMap<UObject*,FDuplicatedObjectInfo*>::TIterator It(DuplicatedObjects);It;++It)
	{
		UObject* OrigObject = It.Key();
		FDuplicatedObjectInfo*	DupObjectInfo = It.Value();

		// don't include any objects which were included in the duplication seed map in the instance graph, as the "duplicate" of these objects
		// may not necessarily be the object that is supposed to be its archetype (the caller can populate the duplication seed map with any objects they wish)
		// and the DuplicationSeed is only used for preserving inter-object references, not for object graphs in SCO and we don't want to call PostDuplicate/PostLoad
		// on them as they weren't actually duplicated
		if ( Parameters.DuplicationSeed.Find(OrigObject) == NULL )
		{
			UObject* DupObjectArchetype = DupObjectInfo->DupObject->GetArchetype();

			// the InstanceGraph's ComponentMap may not contain the correct arc->inst mappings
			// if there were subobjects of the same class inside DupObject, so make sure that the mappings
			// used for this call are the right ones.
			//@note ronp - this should be completely unnecessary since all component references should have been fixed up during
			// duplication
/*
			DupObjectInfo->DupObject->GetClass()->InstanceComponentTemplates(
				(BYTE*)DupObjectInfo->DupObject,
				(BYTE*)DupObjectArchetype,
				DupObjectArchetype ? DupObjectArchetype->GetClass()->GetPropertiesSize() : NULL,
				DupObjectInfo->DupObject,&InstanceGraph);
*/

			DupObjectInfo->DupObject->PostDuplicate();
			DupObjectInfo->DupObject->PostLoad();
		}
	}

	if ( Parameters.bMigrateArchetypes )
	{
		for(TMap<UObject*,FDuplicatedObjectInfo*>::TIterator It(DuplicatedObjects);It;++It)
		{
			UObject* OrigObject = It.Key();
			FDuplicatedObjectInfo* DupInfo = It.Value();

			// don't change the archetypes for any objects which were included in the duplication seed map, as the "duplicate" of these objects
			// may not necessarily be the object that is supposed to be its archetype (the caller can populate the duplication seed map with any objects they wish)
			if ( Parameters.DuplicationSeed.Find(OrigObject) == NULL )
			{
				DupInfo->DupObject->SetArchetype(OrigObject);
				UComponent* ComponentDup = Cast<UComponent>(DupInfo->DupObject);
				if ( ComponentDup != NULL && ComponentDup->TemplateName == NAME_None && OrigObject->IsTemplate() )
				{
					ComponentDup->TemplateName = CastChecked<UComponent>(OrigObject)->TemplateName;
				}
			}
		}
	}

	// if the caller wanted to know which objects were created, do that now
	if ( Parameters.CreatedObjects != NULL )
	{
		// note that we do not clear the map first - this is to allow callers to incrementally build a collection
		// of duplicated objects through multiple calls to StaticDuplicateObject

		// now add each pair of duplicated objects;
		// NOTE: we don't check whether the entry was added from the DuplicationSeed map, so this map
		// will contain those objects as well.
		for ( TMap<UObject*,FDuplicatedObjectInfo*>::TIterator It(DuplicatedObjects); It; ++It )
		{
			UObject* Src = It.Key();
			UObject* Dst = It.Value()->DupObject;

			// don't include any objects which were in the DuplicationSeed map, as CreatedObjects should only contain the list
			// of objects actually created during this call to SDO
			if ( Parameters.DuplicationSeed.Find(Src) == NULL )
			{
				Parameters.CreatedObjects->Set(Src, Dst);
			}
		}
	}

	// cleanup our DuplicatedObjects map
	for ( TMap<UObject*,FDuplicatedObjectInfo*>::TIterator It(DuplicatedObjects); It; ++It )
	{
		delete It.Value();
	}

	//return DupRoot;
	return DupObject;
}

/**
 * Creates a new archetype based on this UObject.  The archetype's property values will match
 * the current values of this UObject.
 *
 * @param	ArchetypeName			the name for the new class
 * @param	ArchetypeOuter			the outer to create the new class in (package?)
 * @param	AlternateArchetype		if specified, is set as the ObjectArchetype for the newly created archetype, after the new archetype
 *									is initialized against "this".  Should only be specified in cases where you need the new archetype to
 *									inherit the property values of this object, but don't want this object to be the new archetype's ObjectArchetype.
 * @param	InstanceGraph			contains the mappings of instanced objects and components to their templates
 *
 * @return	a pointer to a UObject which has values identical to this object
 */
UObject* UObject::CreateArchetype( const TCHAR* ArchetypeName, UObject* ArchetypeOuter, UObject* AlternateArchetype/*=NULL*/, FObjectInstancingGraph* InstanceGraph/*=NULL*/ )
{
	check(ArchetypeName);
	check(ArchetypeOuter);

	EObjectFlags ArchetypeObjectFlags = RF_Public | RF_ArchetypeObject;
	
	// Archetypes residing directly in packages need to be marked RF_Standalone though archetypes that are part of prefabs
	// should not have this flag in order for the prefabs to be deletable via the generic browser.
	if( ArchetypeOuter->IsA(UPackage::StaticClass()) )
	{
		ArchetypeObjectFlags |= RF_Standalone;
	}

	UObject* ArchetypeObject = StaticConstructObject(GetClass(), ArchetypeOuter, ArchetypeName, ArchetypeObjectFlags, this, GError, INVALID_OBJECT, InstanceGraph);
	check(ArchetypeObject);

	UObject* NewArchetype = AlternateArchetype == NULL
		? GetArchetype()
		: AlternateArchetype;

	check(NewArchetype);
	// make sure the alternate archetype has the same class
	check(NewArchetype->GetClass()==GetClass());

	if ( NewArchetype != ArchetypeObject )
	{
		ArchetypeObject->SetArchetype(NewArchetype);
	}
	return ArchetypeObject;
}

/**
 *	Update the ObjectArchetype of this UObject based on this UObject's properties.
 */
void UObject::UpdateArchetype()
{
	DWORD OldUglyHackFlags = GUglyHackFlags;
	GUglyHackFlags |= HACK_UpdateArchetypeFromInstance;

	FObjectInstancingGraph InstanceGraph(ObjectArchetype, this);

	// we're going to recreate this object's archetype inplace which will reinitialize the archetype against this object, replacing the values for all its component properties
	// to point to components owned by this object.  

	//@todo components: determine if this is actually necessary...
	// what we're basically doing here is generating a list of all subojects contained by this instance, which we'll later iterate through and call InstanceComponents on
	// the idea being that nested subobjects instanced as a result of calling CreateArchetype() won't have the correct components....need to verify that this is indeed the case.
	TArray<UObject*> SubobjectInstances;
	FArchiveObjectReferenceCollector SubobjectCollector(
		&SubobjectInstances,		//	InObjectArray
		this,						//	LimitOuter
		FALSE,						//	bRequireDirectOuter
		TRUE,						//	bIgnoreArchetypes
		TRUE,						//	bSerializeRecursively
		FALSE						//	bShouldIgnoreTransient
		);
	Serialize( SubobjectCollector );

	UObject* NewArchetype = CreateArchetype( *ObjectArchetype->GetName(), ObjectArchetype->GetOuter(), ObjectArchetype->GetArchetype(), &InstanceGraph);

	// now instance components for the new archetype
	//@todo components: seems like this should happen already because StaticConstructObject() calls InstanceComponents...don't think I need this, but it certainly doesn't
	// hurt anything for it to be here.
	NewArchetype->GetClass()->InstanceComponentTemplates( (BYTE*)NewArchetype, (BYTE*)this, GetClass()->GetPropertiesSize(), NewArchetype, &InstanceGraph );

	// now instance components for subobjects and components contained by our archetype
	TArray<UObject*> ArchetypeSubobjects;
	InstanceGraph.RetrieveObjectInstances(NewArchetype, ArchetypeSubobjects, TRUE);
	for ( INT ObjIndex = 0; ObjIndex < ArchetypeSubobjects.Num(); ObjIndex++ )
	{
		UObject* Subobject = ArchetypeSubobjects(ObjIndex);
		UObject* Instance = InstanceGraph.GetDestinationObject(Subobject,TRUE);

		Subobject->GetClass()->InstanceComponentTemplates((BYTE*)Subobject, (BYTE*)Instance, Instance->GetClass()->GetPropertiesSize(), Subobject, &InstanceGraph );
	}

	check(NewArchetype == ObjectArchetype);

	GUglyHackFlags = OldUglyHackFlags;
}

/*-----------------------------------------------------------------------------
	Importing and exporting.
-----------------------------------------------------------------------------*/

//
// Import an object from a file.
//
void UObject::ParseParms( const TCHAR* Parms )
{
	if( !Parms )
		return;
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It(GetClass()); It; ++It )
	{
		if( It->GetOuter()!=UObject::StaticClass() )
		{
			FString Value;
			if( Parse(Parms,*(FString(*It->GetName())+TEXT("=")),Value) )
				It->ImportText( *Value, (BYTE*)this + It->Offset, PPF_Localized, this );
		}
	}
}

// Marks the package containing this object as needing to be saved.
void UObject::MarkPackageDirty( UBOOL InDirty ) const
{
	// since transient objects will never be saved into a package, there is no need to mark a package dirty
	// if we're transient
	if ( !HasAnyFlags(RF_Transient) )
	{
		UPackage* Package = Cast<UPackage>(GetOutermost());

		if( Package != NULL )
		{
			Package->SetDirtyFlag(InDirty);
		}
	}
}

/*----------------------------------------------------------------------------
	Description functions.
----------------------------------------------------------------------------*/

/** 
 * Returns detailed info to populate listview columns (defaults to the one line description)
 */
FString UObject::GetDetailedDescription( INT InIndex )
{
	FString Description = TEXT( "" );
	if( InIndex == 0 )
	{
		Description = GetDesc();
	}
	return( Description );
}

/*----------------------------------------------------------------------------
	Language functions.
----------------------------------------------------------------------------*/

const TCHAR* UObject::GetLanguage()
{
	return GLanguage;
}
void UObject::SetLanguage( const TCHAR* LangExt )
{
	if( appStricmp(LangExt,GLanguage) != 0 )
	{
		appStrcpy( GLanguage, LangExt );

		appStrcpy( GNone,  *LocalizeGeneral( TEXT("None"),  TEXT("Core")) );
		appStrcpy( GTrue,  *LocalizeGeneral( TEXT("True"),  TEXT("Core")) );
		appStrcpy( GFalse, *LocalizeGeneral( TEXT("False"), TEXT("Core")) );
		appStrcpy( GYes,   *LocalizeGeneral( TEXT("Yes"),   TEXT("Core")) );
		appStrcpy( GNo,    *LocalizeGeneral( TEXT("No"),    TEXT("Core")) );

		for( FObjectIterator It; It; ++It )
		{
			It->LanguageChange();
		}
	}
}

IMPLEMENT_CLASS(UObject);
