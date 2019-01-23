/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "UnrealEd.h"

/**
 * Notifies all observers that are registered for this event type
 * that the event has fired. This version stores/restores the GWorld
 * object before passing on the message.
 *
 * @param InType the event that was fired
 * @param InViewport the viewport associated with this event
 * @param InMessage the message for this event
 */
void FCallbackEventDeviceEditor::Send(ECallbackEventType InType,
	FViewport* InViewport,UINT InMessage)
{
	check( InViewport );
	switch( InType )
	{
		case CALLBACK_PreWindowsMessage:
		{
			// Make sure the proper GWorld is set before handling the windows message
			if( GEditor->GameViewport && GEditor->GameViewport->Viewport == InViewport )
			{
				// remember the current GWorld that will be restored in the PostWindowsMessage callback
				SavedGWorld = GWorld;
				SetPlayInEditorWorld( GEditor->PlayWorld );
			}
			else
			{
				SavedGWorld = NULL;
			}
			// Let any other registered parties do their thing
			FCallbackEventObserver::Send(InType,InViewport,InMessage);
			break;
		}
		case CALLBACK_PostWindowsMessage:
		{
			// Let any other registered parties do their thing
			FCallbackEventObserver::Send(InType,InViewport,InMessage);
			// Restore the proper GWorld.
			if( SavedGWorld )
			{
				RestoreEditorWorld( SavedGWorld );
			}			
			break;
		}

		default:
		{
			FCallbackEventObserver::Send(InType,InViewport,InMessage);
			break;
		}
	}
}

/**
 * Notifies all observers that are registered for this event type
 * that the event has fired. This version handles updating FEdMode's
 * mode bars
 *
 * @param InType the event that was fired
 * @param InEdMode the FEdMode that is changing
 *
 * @todo figure out which FEdModes need this and register them directly
 */
void FCallbackEventDeviceEditor::Send(ECallbackEventType InType,FEdMode* InEdMode)
{
	// Let others get the event
	FCallbackEventObserver::Send(InType,InEdMode);
	if (InEdMode && InEdMode->GetModeBar())
	{
		if (InType == CALLBACK_EditorModeEnter)
		{
			InEdMode->GetModeBar()->Refresh();
		}
		else if (InType == CALLBACK_EditorModeExit)
		{
			InEdMode->GetModeBar()->SaveData();
		}
	}
}

UBOOL FCallbackQueryDeviceEditor::Query( ECallbackQueryType InType, const FString& InString )
{
	switch (InType)
	{
		case CALLBACK_LoadObjectsOnTop:
			return GEditor && (FFilename(InString).GetBaseFilename() == FFilename(GEditor->UserOpenedFile).GetBaseFilename());

		default:
			check(0);	// Unknown callback
	}

	return FALSE;
}

UBOOL FCallbackQueryDeviceEditor::Query( ECallbackQueryType InType, UObject* QueryObject )
{
	switch ( InType )
	{
		case CALLBACK_AllowPackageSave:
			{
				UPackage* Pkg = Cast<UPackage>(QueryObject);
				if ( GUnrealEd != NULL && Pkg != NULL )
				{
					return GUnrealEd->CanSavePackage(Pkg);
				}

				break;
			}

		default:
			checkf(0, TEXT("Unknown query callback type encountered by FCallbackQueryDeviceEditor::Query: %d"), InType);
	}

	return FALSE;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
