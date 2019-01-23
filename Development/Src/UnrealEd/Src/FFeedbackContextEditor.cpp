/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "UnrealEd.h"
#include "EngineSequenceClasses.h"
#include "FFeedbackContextEditor.h"
#include "DlgMapCheck.h"

FFeedbackContextEditor::FFeedbackContextEditor()
: bIsPerformingMapCheck(FALSE), SlowTaskCount(0)
{}

void FFeedbackContextEditor::Serialize( const TCHAR* V, EName Event )
{
	if( !GLog->IsRedirectingTo( this ) )
	{
		GLog->Serialize( V, Event );
	}

	if ( Event == NAME_Error )
	{
		appMsgf( AMT_OK, V );
	}
}

/**
 * Tells the editor that a slow task is beginning
 * 
 * @param Task			The description of the task beginning.
 * @param StatusWindow	Whether to use a status window.
 */
void FFeedbackContextEditor::BeginSlowTask( const TCHAR* Task, UBOOL StatusWindow )
{
	if( GApp )
	{
		GIsSlowTask = ++SlowTaskCount>0;

		// Show a wait cursor for long running tasks
		if (SlowTaskCount == 1)
		{
			::wxBeginBusyCursor();
			GApp->EditorFrame->SetStatusBar( SB_Progress );
		}

		WxStatusBarProgress* sbp = static_cast<WxStatusBarProgress*>(GApp->EditorFrame->StatusBars[ SB_Progress ]);
		if( sbp->GetFieldsCount() > WxStatusBarProgress::FIELD_Description )
		{
			sbp->SetStatusText( Task != NULL ? Task : TEXT(""), WxStatusBarProgress::FIELD_Description );
		}
		sbp->SetProgress( 0 );
	}
}

/**
 * Tells the editor that the slow task is done.
 */
void FFeedbackContextEditor::EndSlowTask()
{
	if( GApp )
	{
		check(SlowTaskCount>0);
		GIsSlowTask = --SlowTaskCount>0;
		// Restore the cursor now that the long running task is done
		if (SlowTaskCount == 0)
		{
			GApp->EditorFrame->SetStatusBar( SB_Standard );
			::wxEndBusyCursor();
		}
	}
}

void FFeedbackContextEditor::SetContext( FContextSupplier* InSupplier )
{
}

VARARG_BODY( UBOOL VARARGS, FFeedbackContextEditor::StatusUpdatef, const TCHAR*, VARARG_EXTRA(INT Numerator) VARARG_EXTRA(INT Denominator) )
{
	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Fmt, Fmt );

	if( GIsSlowTask )
	{
		GApp->StatusUpdateProgress(TempStr, Numerator, Denominator);
	}
	return 1;
}

/**
 * @return	TRUE if a map check is currently active.
 */
UBOOL FFeedbackContextEditor::MapCheck_IsActive() const
{
	return bIsPerformingMapCheck;
}

void FFeedbackContextEditor::MapCheck_Show()
{
	GApp->DlgMapCheck->Show( true );
}

/**
 * Same as MapCheck_Show, except it won't display the map check dialog if there are no errors in it.
 */
void FFeedbackContextEditor::MapCheck_ShowConditionally()
{
	GApp->DlgMapCheck->ShowConditionally();
}

/**
 * Hides the map check dialog.
 */
void FFeedbackContextEditor::MapCheck_Hide()
{
	GApp->DlgMapCheck->Show( false );
}

/**
 * Clears out all errors/warnings.
 */
void FFeedbackContextEditor::MapCheck_Clear()
{
	GApp->DlgMapCheck->ClearMessageList();
}

/**
 * Called around bulk MapCheck_Add calls.
 */
void FFeedbackContextEditor::MapCheck_BeginBulkAdd()
{
	GApp->DlgMapCheck->FreezeMessageList();
	bIsPerformingMapCheck = TRUE;
}

/**
 * Called around bulk MapCheck_Add calls.
 */
void FFeedbackContextEditor::MapCheck_EndBulkAdd()
{
	bIsPerformingMapCheck = FALSE;
	GApp->DlgMapCheck->ThawMessageList();
}

/**
 * Adds a message to the map check dialog, to be displayed when the dialog is shown.
 *
 * @param	InType					The	type of message.
 * @param	InActor					Actor associated with the message; can be NULL.
 * @param	InMessage				The message to display.
 * @param	InRecommendedAction		[opt] The recommended course of action to take; default is MCACTION_NONE.
 * @param	InUDNPage				[opt] UDN Page to visit if the user needs more info on the warning.  This will send the user to https://udn.epicgames.com/Three/MapErrors#InUDNPage. 
 */
void FFeedbackContextEditor::MapCheck_Add(MapCheckType InType, UObject* InActor, const TCHAR* InMessage, MapCheckAction InRecommendedAction, const TCHAR* InUDNPage)
{
	AActor* Actor	= Cast<AActor>( InActor );
	WxDlgMapCheck::AddItem( InType, Actor, InMessage, InRecommendedAction, InUDNPage );
}
