/*=============================================================================
	DlgMapCheck.h: UnrealEd dialog for displaying map errors.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __DLGMAPCHECK_H__
#define __DLGMAPCHECK_H__

#include "TrackableWindow.h"


/** Struct that holds information about a entry in the ErrorWarningList. */
struct FCheckErrorWarningInfo
{
	/** The actor and/or object that is related to this error/warning.  Can be NULL if not relevant. */
	UObject* Object;

	/** What type of error/warning this is */
	MapCheckType Type;

	/** The message we want to display to the LD. */
	FString Message;

	/** The UDN page where help can be found for this error/warning. */
	FString UDNPage;

	/** Recommended way to deal with this error/warning. */
	MapCheckAction RecommendedAction;

	/** UDN Page that describes the warning in detail. */
	FString	UDNHelpString;

	/** The time taken to execute (for lighting builds). */
	DOUBLE ExecutionTime;

	UBOOL operator==(const FCheckErrorWarningInfo& Other) const
	{
		return (
			(Object == Other.Object) &&
			(Type == Other.Type) &&
			(Message == Other.Message) &&
			(UDNPage == Other.UDNPage) &&
			(RecommendedAction == Other.RecommendedAction) &&
			(ExecutionTime == Other.ExecutionTime)
			);
	}
};

/**
 * Dialog that displays map errors and allows the user to focus on and delete
 * any actors associated with those errors.
 */
class WxDlgMapCheck : public WxTrackableDialog, public FSerializableObject
{
public:
	WxDlgMapCheck(wxWindow* InParent);
	virtual ~WxDlgMapCheck();

	/**
	 *	Initialize the dialog box.
	 */
	virtual void Initialize();

	/**
	 * This function is called when the window has been selected from within the ctrl + tab dialog.
	 */
	virtual void OnSelected();

	/**
	 * Shows the dialog only if there are messages to display.
	 */
	void ShowConditionally();

	/**
	 * Clears out the list of messages appearing in the window.
	 */
	virtual void ClearMessageList();

	/**
	 * Freezes the message list.
	 */
	void FreezeMessageList();

	/**
	 * Thaws the message list.
	 */
	void ThawMessageList();

	/**
	 * Returns TRUE if the dialog has any map errors, FALSE if not
	 *
	 * @return TRUE if the dialog has any map errors, FALSE if not
	 */
	UBOOL HasAnyErrors() const;

	/**
	 * Adds a message to the map check dialog, to be displayed when the dialog is shown.
	 *
	 * @param	InType					The type of message.
	 * @param	InActor					Actor associated with the message; can be NULL.
	 * @param	InMessage				The message to display.
	 * @param	InRecommendedAction		The recommended course of action to take.
	 * @param	InUDNPage				UDN Page to visit if the user needs more info on the warning.  This will send the user to https://udn.epicgames.com/Three/MapErrors#InUDNPage. 
	 */
	static void AddItem(MapCheckType InType, UObject* InActor, const TCHAR* InMessage, MapCheckAction InRecommendedAction, const TCHAR* InUDNPage=TEXT(""));

	virtual void Serialize(FArchive& Ar);

	/**
	* Loads the list control with the contents of the GErrorWarningInfoList array.
	*/
	void LoadListCtrl();

	virtual bool Show( bool show = true );

protected:
	UBOOL							bSortResultsByCheckType;
	wxListCtrl*						ErrorWarningList;
	wxImageList*					ImageList;
	TArray<UObject*>				ReferencedObjects;
	TArray<FCheckErrorWarningInfo>*	ErrorWarningInfoList;
	FString							DlgPosSizeName;

	wxButton*	CloseButton;
	wxButton*	RefreshButton;
	wxButton*	GotoButton;
	wxButton*	DeleteButton;
	wxButton*	PropertiesButton;
	wxButton*	HelpButton;
	wxButton*	CopyToClipboardButton;

	/**
	 * Removes all items from the map check dialog that pertain to the specified object.
	 *
	 * @param	Object		The object to match when removing items.
	 */
	void RemoveObjectItems(UObject* Object);

	/** Event handler for when the close button is clicked on. */
	void OnClose(wxCommandEvent& In);

	/** Event handler for when the refresh button is clicked on. */
	virtual void OnRefresh(wxCommandEvent& In);

	/** Event handler for when the goto button is clicked on. */
	virtual void OnGoTo(wxCommandEvent& In);

	/** Event handler for when the delete button is clicked on. */
	virtual void OnDelete(wxCommandEvent& In);

	/** Event handler for when the properties button is clicked on. */
	virtual void OnGotoProperties(wxCommandEvent& In);

	/** Event handler for when the "Show Help" button is clicked on. */
	void OnShowHelpPage(wxCommandEvent& In);

	/** 
	 * Event handler for when the "Copy to Clipboard" button is clicked on. Copies the contents
	 * of the dialog to the clipboard. If no items are selected, the entire contents are copied.
	 * If any items are selected, only selected items are copied.
	 */
	void OnCopyToClipboard(wxCommandEvent& In);

	/** Event handler for when a message is clicked on. */
	virtual void OnItemActivated(wxListEvent& In);

	/** Event handler for when wx wants to update UI elements. */
	void OnUpdateUI(wxUpdateUIEvent& In);

public:
	/** Set up the columns for the ErrorWarningList control. */
	virtual void SetupErrorWarningListColumns();

	/** 
	 *	Get the level/package name for the given object.
	 *
	 *	@param	InObject	The object to retrieve the level/package name for.
	 *
	 *	@return	FString		The name of the level/package.
	 */
	virtual FString GetLevelOrPackageName(UObject* InObject);

protected:
	/** If bSortResultsByCheckType==TRUE, then sort the list. */
	virtual void SortErrorWarningInfoList();

protected:
	DECLARE_EVENT_TABLE()
};

static TArray<FCheckErrorWarningInfo> GErrorWarningInfoList;

#endif // __DLGMAPCHECK_H__
