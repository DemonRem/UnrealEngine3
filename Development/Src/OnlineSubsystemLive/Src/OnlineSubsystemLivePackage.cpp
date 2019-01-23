/**
 * Copyright © 2006 Epic Games, Inc. All Rights Reserved.
 */

#include "OnlineSubsystemLive.h"

#define STATIC_LINKING_MOJO 1

// Register things.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) FName ONLINESUBSYSTEMLIVE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "OnlineSubsystemLiveClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

// Register natives.
#define NATIVES_ONLY
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name)
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#include "OnlineSubsystemLiveClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NATIVES_ONLY
#undef NAMES_ONLY

/**
 * Initialize registrants, basically calling StaticClass() to create the class and also 
 * populating the lookup table.
 *
 * @param	Lookup	current index into lookup table
 */
void AutoInitializeRegistrantsOnlineSubsystemLive( INT& Lookup )
{
	AUTO_INITIALIZE_REGISTRANTS_ONLINESUBSYSTEMLIVE;
}

/**
 * Auto generates names.
 */
void AutoGenerateNamesOnlineSubsystemLive()
{
	#define NAMES_ONLY
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
    #define AUTOGENERATE_NAME(name) ONLINESUBSYSTEMLIVE_##name = FName(TEXT(#name));
	#include "OnlineSubsystemLiveClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
}

