/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "BusyCursor.h"

static INT GScopedBusyCursorReferenceCounter = 0;

FScopedBusyCursor::FScopedBusyCursor()
{
	// Reference count so FScopedBusyCursor instances can nest.
	if ( GScopedBusyCursorReferenceCounter == 0 )
	{
		wxBeginBusyCursor( wxHOURGLASS_CURSOR );
	}
	GScopedBusyCursorReferenceCounter++;
}

FScopedBusyCursor::~FScopedBusyCursor()
{
	// Reference count so FScopedBusyCursor instances can nest.
	--GScopedBusyCursorReferenceCounter;
	if ( GScopedBusyCursorReferenceCounter == 0 )
	{
		wxEndBusyCursor();
	}
}
