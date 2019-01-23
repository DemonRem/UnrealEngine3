/*=============================================================================
TrackAllocSections.h: Utils for tracking alloc by scoped sections
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _TRACK_ALLOC_SECTIONS
#define _TRACK_ALLOC_SECTIONS

/** Global state to keep track of current section name. */
extern class FGlobalAllocSectionState	GAllocSectionState;

/** 
* Keeps track of the current alloc section name. 
* Used by FMallocProxySimpleTrack and set by FScopeAllocSection 
*/
class FGlobalAllocSectionState
{
public:
	enum { MAX_SECTION_NAME_SIZE=256, MAX_THREAD_DATA_INSTANCES=20 };

	/** 
	* Ctor. Init the TLS entry 
	*/
	FGlobalAllocSectionState();
	/** 
	* Dtor. Free any memory allocated for the per thread global section name 
	*/
	~FGlobalAllocSectionState();
	/**
	* Gets the global section name via the thread's TLS entry. Allocates a new
	* section string if one doesn't exist for the current TLS entry.
	* @return ptr to the global section name for the running thread 
	*/
	TCHAR* GetCurrentSectionName();
private:
	class FAllocSectionData
	{
	public:
		TCHAR SectionName[MAX_SECTION_NAME_SIZE];
		FAllocSectionData(const TCHAR* InSectionName=TEXT("None"));
	};
	/** each thread needs its own instance set accessed via a TLS entry */
	FAllocSectionData DataInstances[MAX_THREAD_DATA_INSTANCES];
	/** keeps track of the next entry toin DataInstances */
	INT NextAvailInstance;
	/** TLS index for the per thread entry of the alloc section name data */
	DWORD PerThreadSectionDataTLS;
};

/** Utility class for setting GAllocSectionName while this object is in scope. */
class FScopeAllocSection
{
public:
	/** Ctor, sets the section name. */
	FScopeAllocSection( TCHAR* InSection );
	/** Dtor, restores old section name. */
	~FScopeAllocSection();
private:
	/** Previous section name - to restore when this object goes out of scope. */
	TCHAR OldSection[FGlobalAllocSectionState::MAX_SECTION_NAME_SIZE];
};

#endif //#if _TRACK_ALLOC_SECTIONS



