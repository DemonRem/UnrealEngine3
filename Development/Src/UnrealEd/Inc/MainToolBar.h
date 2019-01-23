/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __MAINTOOLBAR_H__
#define __MAINTOOLBAR_H__

class wxComboBox;
class wxWindow;
class WxAlphaBitmap;

/**
 * The toolbar that sits at the top of the main editor frame.
 */
class WxMainToolBar : public wxToolBar
{
public:
	WxMainToolBar(wxWindow* InParent, wxWindowID InID);

	enum EPlatformButtons { B_PC=0, B_PS3, B_XBox360, B_MAX };

	/** Updates the 'Push View' toggle's bitmap and hint text based on the input state. */
	void SetPushViewState(UBOOL bIsPushingView);

	/** Enables/disables the 'Push View' button. */
	void EnablePushView(UBOOL bEnabled);

private:
	WxMaskedBitmap NewB, OpenB, SaveB, SaveAllB, UndoB, RedoB, CutB, CopyB, PasteB, SearchB, FullScreenB, GenericB, KismetB, TranslateB,
		ShowWidgetB, RotateB, ScaleB, ScaleNonUniformB, MouseLockB, BrushPolysB, PrefabLockB, CamSlowB, CamNormalB, CamFastB, ViewPushStartB, ViewPushStopB, ViewPushSyncB,
		DistributionToggleB;
	wxToolBarToolBase* ViewPushStartStopButton;
	WxAlphaBitmap *BuildGeomB, *BuildLightingB, *BuildPathsB, *BuildCoverNodesB, *BuildAllB, *PlayB[B_MAX];
	WxMenuButton MRUButton, PasteSpecialButton;
	wxMenu PasteSpecialMenu;

	DECLARE_EVENT_TABLE();

	/** Called when the trans/rot/scale widget toolbar buttons are right-clicked. */
	void OnTransformButtonRightClick(wxCommandEvent& In);

public:
	wxComboBox* CoordSystemCombo;
};

#endif // __MAINTOOLBAR_H__
