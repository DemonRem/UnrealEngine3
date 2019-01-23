/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __PROPERTYWINDOWMANAGER_H__
#define __PROPERTYWINDOWMANAGER_H__

#include "Bitmaps.h"

// Forward declarations.
class WxPropertyWindow;


/**
 * Allocates and initializes GPropertyWindowManager.  Called by WxLaunchApp::OnInit() to initialize.  Not rentrant.
 */
void InitPropertySubSystem();

/**
 * A container for all property windows.  Manages property window serialization.
 */
class UPropertyWindowManager : public UObject
{
	DECLARE_CLASS(UPropertyWindowManager,UObject,CLASS_Transient|CLASS_Intrinsic,UnrealEd);

	UPropertyWindowManager();

	/**
	 * Loads common bitmaps.  Safely reentrant.
	 *
	 * @return		TRUE on success, FALSE on fail.
	 */
	UBOOL Initialize();

	/**
	 * Property window management.  Registers a property window with the manager.
	 *
	 * @param	InPropertyWindow		The property window to add to managing control.  Must be non-NULL.
	 */
	void RegisterWindow(WxPropertyWindow* InPropertyWindow);

	/**
	 * Property window management.  Unregisters a property window from the manager.
	 *
	 * @param	InPropertyWindow		The property window to remove from managing control.
	 */
	void UnregisterWindow(WxPropertyWindow* InPropertyWindow);

	/**
	 * Serializes all managed property windows to the specified archive.
	 *
	 * @param		Ar		The archive to read/write.
	 */
	virtual void Serialize(FArchive& Ar);

	/**
	 * Callback used to allow object register its direct object references that are not already covered by
	 * the token stream.
	 *
	 * @param ObjectArray	array to add referenced objects to via AddReferencedObject
	 */
	void AddReferencedObjects(TArray<UObject*>& ObjectArray);

	/**
	 * Dissociates all set objects and hides windows.
	 */
	void ClearReferencesAndHide();

	/** The list of active property windows. */
	TArray<WxPropertyWindow*> PropertyWindows;

public:
	// @todo DB: create a solution for dynamically binding bitmaps.

	// Bitmaps used by all property windows.
	WxMaskedBitmap	ArrowDownB;
	WxMaskedBitmap	ArrowRightB;
	WxMaskedBitmap	CheckBoxOnB;
	WxMaskedBitmap	CheckBoxOffB;
	WxMaskedBitmap	CheckBoxUnknownB;
	WxMaskedBitmap	Prop_AddNewItemB;
	WxMaskedBitmap	Prop_RemoveAllItemsFromArrayB;
	WxMaskedBitmap	Prop_InsertNewItemHereB;
	WxMaskedBitmap	Prop_DeleteItemB;
	WxMaskedBitmap	Prop_ShowGenericBrowserB;
	WxMaskedBitmap	Prop_UseMouseToPickColorB;
	WxMaskedBitmap	Prop_ClearAllTextB;
	WxMaskedBitmap	Prop_UseMouseToPickActorB;
	WxMaskedBitmap	Prop_UseCurrentBrowserSelectionB;
	WxMaskedBitmap	Prop_NewObjectB;
	WxMaskedBitmap	Prop_DuplicateB;
	WxMaskedBitmap	Prop_ResetToDefaultB;

private:
	/** Indicates whether or not the manager has been initialized ie Initialize() was successfully called. */
	UBOOL			bInitialized;
};

#endif // __PROPERTYWINDOWMANAGER_H__
