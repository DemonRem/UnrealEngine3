/*=============================================================================
	FFeedbackContextEditor.h: Feedback context tailored to UnrealEd

	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __FFEEDBACKCONTEXTEDITOR_H__
#define __FFEEDBACKCONTEXTEDITOR_H__

/**
 * A FFeedbackContext implementation for use in UnrealEd.
 */
class FFeedbackContextEditor : public FFeedbackContext
{
	UBOOL bIsPerformingMapCheck;
public:
	INT SlowTaskCount;

	FFeedbackContextEditor();

	void Serialize( const TCHAR* V, EName Event );

	/**
	 * Tells the editor that a slow task is beginning
	 * 
	 * @param Task			The description of the task beginning.
	 * @param StatusWindow	Whether to use a status window.
	 */
	void BeginSlowTask( const TCHAR* Task, UBOOL StatusWindow );

	/**
	 * Tells the editor that the slow task is done.
	 */
	void EndSlowTask();
	void SetContext( FContextSupplier* InSupplier );

	VARARG_BODY( UBOOL, YesNof, const TCHAR*, VARARG_NONE )
	{
		TCHAR TempStr[4096];
		GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr)-1, Fmt, Fmt );
		return appMsgf( AMT_YesNo, TempStr );
	}

	VARARG_BODY( UBOOL VARARGS, StatusUpdatef, const TCHAR*, VARARG_EXTRA(INT Numerator) VARARG_EXTRA(INT Denominator) );

	/**
	 * @return	TRUE if a map check is currently active.
	 */
	virtual UBOOL MapCheck_IsActive() const;
	virtual void MapCheck_Show();

	/**
	 * Same as MapCheck_Show, except it won't display the map check dialog if there are no errors in it.
	 */
	virtual void MapCheck_ShowConditionally();

	/**
	 * Hides the map check dialog.
	 */
	virtual void MapCheck_Hide();

	/**
	 * Clears out all errors/warnings.
	 */
	virtual void MapCheck_Clear();

	/**
	 * Called around bulk MapCheck_Add calls.
	 */
	virtual void MapCheck_BeginBulkAdd();

	/**
	 * Adds a message to the map check dialog, to be displayed when the dialog is shown.
	 *
	 * @param	InType					The	type of message.
	 * @param	InActor					Actor associated with the message; can be NULL.
	 * @param	InMessage				The message to display.
	 * @param	InRecommendedAction		[opt] The recommended course of action to take; default is MCACTION_NONE.
	 * @param	InUDNPage				UDN Page to visit if the user needs more info on the warning.  This will send the user to https://udn.epicgames.com/Three/MapErrors#InUDNPage.
	 */
	virtual void MapCheck_Add( MapCheckType InType, UObject* InActor, const TCHAR* InMessage, MapCheckAction InRecommendedAction=MCACTION_NONE, const TCHAR* InUDNPage=TEXT("") );

	/**
	 * Called around bulk MapCheck_Add calls.
	 */
	virtual void MapCheck_EndBulkAdd();
};

#endif // __FFEEDBACKCONTEXTEDITOR_H__
