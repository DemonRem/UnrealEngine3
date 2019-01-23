/*=============================================================================
TrackAllocSections.cpp: Utils for tracking alloc by scoped sections
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"
#include "TrackAllocSections.h"

/*----------------------------------------------------------------------------
Globals
----------------------------------------------------------------------------*/

/** 
* Global state to keep track of current section name. 
* Used by FMallocProxySimpleTrack and set by FScopeAllocSection
*/
FGlobalAllocSectionState GAllocSectionState;

/*-----------------------------------------------------------------------------
FGlobalAllocSectionState
-----------------------------------------------------------------------------*/

/** 
* Ctor. Init the TLS entry 
*/
FGlobalAllocSectionState::FGlobalAllocSectionState()
:	NextAvailInstance(0)
{
	// Allocate the TLS slot and zero its contents
	PerThreadSectionDataTLS = appAllocTlsSlot();
	appSetTlsValue(PerThreadSectionDataTLS,NULL);
}

/** 
* Dtor. Free any memory allocated for the per thread global section name 
*/
FGlobalAllocSectionState::~FGlobalAllocSectionState()
{
	appFreeTlsSlot(PerThreadSectionDataTLS);
}

/**
* Gets the global section name via the thread's TLS entry. Allocates a new
* section string if one doesn't exist for the current TLS entry.
* @return ptr to the global section name for the running thread 
*/
TCHAR* FGlobalAllocSectionState::GetCurrentSectionName()
{
	FAllocSectionData* Entry = (FAllocSectionData*)appGetTlsValue(PerThreadSectionDataTLS);
	if( !Entry )
	{
		INT CurAvailInstance = NextAvailInstance;
		do 
		{	
			// update the cur available instance atomically
			CurAvailInstance = NextAvailInstance;			
		} while( appInterlockedCompareExchange(&NextAvailInstance,CurAvailInstance+1,CurAvailInstance) != CurAvailInstance );

		check(CurAvailInstance < MAX_THREAD_DATA_INSTANCES);
		Entry = &DataInstances[CurAvailInstance];
		appSetTlsValue(PerThreadSectionDataTLS,(void*)Entry);		
	}
	return Entry->SectionName;
}

FGlobalAllocSectionState::FAllocSectionData::FAllocSectionData(const TCHAR* InSectionName)
{
	appStrncpy(SectionName,InSectionName,MAX_SECTION_NAME_SIZE);
}

/*-----------------------------------------------------------------------------
FScopeAllocSection
-----------------------------------------------------------------------------*/

/** Ctor, sets the section name. */
FScopeAllocSection::FScopeAllocSection( TCHAR* InSection )
{
	appStrncpy(OldSection, GAllocSectionState.GetCurrentSectionName(), FGlobalAllocSectionState::MAX_SECTION_NAME_SIZE);
	appStrncpy(GAllocSectionState.GetCurrentSectionName(), InSection, FGlobalAllocSectionState::MAX_SECTION_NAME_SIZE);
}

/** Dtor, restores old section name. */
FScopeAllocSection::~FScopeAllocSection()
{
	appStrncpy(GAllocSectionState.GetCurrentSectionName(), OldSection, FGlobalAllocSectionState::MAX_SECTION_NAME_SIZE);
}



