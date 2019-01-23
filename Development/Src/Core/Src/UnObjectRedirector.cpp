/*=============================================================================
	UnRedirector.cpp: Object redirector implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	UObjectRedirector
-----------------------------------------------------------------------------*/

/**
 * Static constructor, called once during static initialization of global variables for native 
 * classes. Used to e.g. register object references for native- only classes required for realtime
 * garbage collection or to associate UProperties.
 */
void UObjectRedirector::StaticConstructor()
{
	UClass* TheClass = GetClass();
	TheClass->EmitObjectReference( STRUCT_OFFSET( UObjectRedirector, DestinationObject ) );
}

// If this object redirector is pointing to an object that won't be serialized anyway, set the RF_Transient flag
// so that this redirector is also removed from the package.
void UObjectRedirector::PreSave()
{
	if (DestinationObject == NULL
	||	DestinationObject->HasAnyFlags(RF_Transient)
	||	DestinationObject->IsIn(GetTransientPackage()) )
	{
		Modify();
		SetFlags(RF_Transient);

		if ( DestinationObject != NULL )
		{
			DestinationObject->Modify();
			DestinationObject->SetFlags(RF_Transient);
		}
	}
}

void UObjectRedirector::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	Ar << DestinationObject;
}

IMPLEMENT_CLASS(UObjectRedirector);

/**
 * Constructor for the callback device, will register itself with the GCallbackEvent
 *
 * @param InObjectPathNameToMatch Full pathname of the object refrence that is being compiled by script
 */
FScopedRedirectorCatcher::FScopedRedirectorCatcher(const FString& InObjectPathNameToMatch)
: ObjectPathNameToMatch(InObjectPathNameToMatch)
, bWasRedirectorFollowed(FALSE)
{
	// register itself on construction to see if the object is a redirector 
	GCallbackEvent->Register(CALLBACK_RedirectorFollowed, this);
}

/**
 * Destructor. Will unregister the callback
 */
FScopedRedirectorCatcher::~FScopedRedirectorCatcher()
{
	// register itself on construction to see if the object is a redirector 
	GCallbackEvent->Unregister(CALLBACK_RedirectorFollowed, this);
}


/**
 * Responds to CALLBACK_RedirectorFollowed. Records all followed redirections
 * so they can be cleaned later.
 *
 * @param InType Callback type (should only be CALLBACK_RedirectorFollowed)
 * @param InString Name of the package that pointed to the redirect
 * @param InObject The UObjectRedirector that was followed
 */
void FScopedRedirectorCatcher::Send( ECallbackEventType InType, const FString& InString, UObject* InObject)
{
	check(InType == CALLBACK_RedirectorFollowed);
	// this needs to be the redirector
	check(InObject->IsA(UObjectRedirector::StaticClass()));

	// if the path of the redirector was the same as the path to the object constant
	// being compiled, then the script code has a text reference to a redirector, 
	// which will cause FixupRedirects to break the reference
	if (InObject->GetPathName() == ObjectPathNameToMatch)
	{
		bWasRedirectorFollowed = TRUE;
	}
}
