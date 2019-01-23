/*=============================================================================
	UnUIStrings.cpp: UI structs, utility, and helper class implementations for rendering and formatting strings
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// engine classes
#include "EnginePrivate.h"

// widgets and supporting UI classes
#include "EngineUserInterfaceClasses.h"
#include "EngineUIPrivateClasses.h"
#include "UnUIMarkupResolver.h"
#include "Localization.h"

/**
 * Allows access to UObject::GetLanguage() from modules which aren't linked against Core
 */
const TCHAR* appGetCurrentLanguage()
{
	return UObject::GetLanguage();
}

DECLARE_CYCLE_STAT(TEXT("ParseString Time"),STAT_UIParseString,STATGROUP_UI);

/* ==========================================================================================================
	FUIDataStoreBinding
========================================================================================================== */
/**
 * Unregisters any bound data stores and clears all references.
 */
UBOOL FUIDataStoreBinding::ClearDataBinding()
{
	UBOOL bResult = Subscriber || (ResolvedDataStore != NULL);

	UnregisterSubscriberCallback();
	ResolvedDataStore = NULL;
	return bResult;
}

/**
 * Registers the current subscriber with ResolvedDataStore's list of RefreshSubscriberNotifies
 */
void FUIDataStoreBinding::RegisterSubscriberCallback()
{
	if ( Subscriber && ResolvedDataStore != NULL )
	{
		ResolvedDataStore->eventSubscriberAttached(Subscriber);
	}
}

/**
 * Removes the current subscriber from ResolvedDataStore's list of RefreshSubscriberNotifies.
 */
void FUIDataStoreBinding::UnregisterSubscriberCallback()
{
	if ( Subscriber && ResolvedDataStore != NULL )
	{
		// if we have an existing value for both subscriber and ResolvedDataStore, unregister our current subscriber
		// from the data store's list of refresh notifies
		ResolvedDataStore->eventSubscriberDetached(Subscriber);
	}
}

/**
 * Determines whether the specified data field can be assigned to this data store binding.
 *
 * @param	DataField	the data field to verify.
 *
 * @return	TRUE if DataField's FieldType is compatible with the RequiredFieldType for this data binding.
 */
UBOOL FUIDataStoreBinding::IsValidDataField( const FUIDataProviderField& DataField ) const
{
	return IsValidDataField((EUIDataProviderFieldType)DataField.FieldType);
}

/**
 * Determines whether the specified field type is valid for this data store binding.
 *
 * @param	FieldType	the data field type to check
 *
 * @return	TRUE if FieldType is compatible with the RequiredFieldType for this data binding.
 */
UBOOL FUIDataStoreBinding::IsValidDataField( EUIDataProviderFieldType FieldType ) const
{
	UBOOL bResult = FALSE;

	if ( RequiredFieldType != DATATYPE_MAX && RequiredFieldType != DATATYPE_Provider )
	{
		if ( RequiredFieldType == DATATYPE_Collection )
		{
			bResult = (FieldType == DATATYPE_Collection || FieldType == DATATYPE_ProviderCollection);
		}
		else
		{
			bResult = FieldType == RequiredFieldType;
		}
	}

	return bResult;
}

/**
 * Resolves the value of MarkupString into a data store reference, and fills in the values for all members of this struct
 *
 * @param	InSubscriber	the subscriber that contains this data store binding
 *
 * @return	TRUE if the markup was successfully resolved.
 */
UBOOL FUIDataStoreBinding::ResolveMarkup( TScriptInterface<IUIDataStoreSubscriber> InSubscriber )
{
	UBOOL bResult = FALSE;

	return bResult;
}

	
/**
 * Retrieves the value for this data store binding from the ResolvedDataStore.
 *
 * @param	out_ResolvedValue	will contain the value of the data store binding.
 *
 * @return	TRUE if the value for this data store binding was successfully retrieved from the data store.
 */
UBOOL FUIDataStoreBinding::GetBindingValue( FUIProviderFieldValue& out_ResolvedValue ) const
{
	UBOOL bResult = FALSE;

	if ( ResolvedDataStore != NULL && DataStoreField != NAME_None )
	{
		bResult = ResolvedDataStore->GetDataStoreValue(DataStoreField.ToString(), out_ResolvedValue);
	}

	return bResult;
}

/**
 * Publishes the value for this data store binding to the ResolvedDataStore.
 *
 * @param	NewValue	contains the value that should be published to the data store
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
UBOOL FUIDataStoreBinding::SetBindingValue( const FUIProviderScriptFieldValue& NewValue ) const
{
	UBOOL bResult = FALSE;

	if ( ResolvedDataStore != NULL && DataStoreField != NAME_None )
	{
		bResult = ResolvedDataStore->SetDataStoreValue(DataStoreField.ToString(), NewValue);
	}

	return bResult;
}


