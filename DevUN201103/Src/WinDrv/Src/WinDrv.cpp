/*=============================================================================
	WinDrv.cpp: Unreal Windows viewport and platform driver.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "WinDrvPrivate.h"

/**
 * Initialize registrants, basically calling StaticClass() to create the class and also 
 * populating the lookup table.
 *
 * @param	Lookup	current index into lookup table
 */
void AutoInitializeRegistrantsWinDrv( INT& Lookup )
{
	UWindowsClient::StaticClass();
	UXnaForceFeedbackManager::StaticClass();

	// facebook class is solely in a .cpp file
	extern void AutoInitializeRegistrantsWinFacebook( INT& Lookup );
	AutoInitializeRegistrantsWinFacebook( Lookup );
}

/**
 * Auto generates names.
 */
void AutoRegisterNamesWinDrv()
{
}
