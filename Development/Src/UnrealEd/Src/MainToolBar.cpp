/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "MainToolBar.h"
#include "DlgTransform.h"

extern class WxUnrealEdApp* GApp;

BEGIN_EVENT_TABLE( WxMainToolBar, wxToolBar )
	EVT_TOOL_RCLICKED( ID_EDIT_TRANSLATE, WxMainToolBar::OnTransformButtonRightClick )
	EVT_TOOL_RCLICKED( ID_EDIT_ROTATE, WxMainToolBar::OnTransformButtonRightClick )
	EVT_TOOL_RCLICKED( ID_EDIT_SCALE, WxMainToolBar::OnTransformButtonRightClick )
	EVT_TOOL_RCLICKED( ID_EDIT_SCALE_NONUNIFORM, WxMainToolBar::OnTransformButtonRightClick )
END_EVENT_TABLE()

WxMainToolBar::WxMainToolBar( wxWindow* InParent, wxWindowID InID )
	: wxToolBar( InParent, InID, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxNO_BORDER | wxTB_3DBUTTONS )
{
	// Menus

	PasteSpecialMenu.Append( IDM_PASTE_ORIGINAL_LOCATION, *LocalizeUnrealEd("OriginalLocation") );
	PasteSpecialMenu.Append( IDM_PASTE_WORLD_ORIGIN, *LocalizeUnrealEd("WorldOrigin") );

	// Bitmaps

	NewB.Load( TEXT("New") );
	OpenB.Load( TEXT("Open") );
	SaveB.Load( TEXT("Save") );
	SaveAllB.Load( TEXT("SaveAll") );
	UndoB.Load( TEXT("Undo") );
	RedoB.Load( TEXT("Redo") );
	CutB.Load( TEXT("Cut") );
	CopyB.Load( TEXT("Copy") );
	PasteB.Load( TEXT("Paste") );
	SearchB.Load( TEXT("Search") );
	FullScreenB.Load( TEXT("FullScreen") );
	GenericB.Load( TEXT("ThumbnailView") );
	KismetB.Load( TEXT("Kismet") );
	MouseLockB.Load( TEXT("MouseLock") );
	ShowWidgetB.Load( TEXT("ShowWidget") );
	TranslateB.Load( TEXT("Translate") );
	RotateB.Load( TEXT("Rotate") );
	ScaleB.Load( TEXT("Scale") );
	ScaleNonUniformB.Load( TEXT("ScaleNonUniform") );
	BrushPolysB.Load( TEXT("BrushPolys") );
	PrefabLockB.Load( TEXT("LVT_Perspective") );
	DistributionToggleB.Load( TEXT("DistributionToggle") );
	CamSlowB.Load( TEXT("CamSlow") );
	CamNormalB.Load( TEXT("CamNormal") );
	CamFastB.Load( TEXT("CamFast") );
	ViewPushStartB.Load(TEXT("PushView_Start"));
	ViewPushStopB.Load(TEXT("PushView_Stop"));
	ViewPushSyncB.Load(TEXT("PushView_Sync"));
	BuildGeomB = new WxAlphaBitmap( TEXT("BuildGeometry"), 0 );
	BuildLightingB = new WxAlphaBitmap( TEXT("BuildLighting"), 0 );
	BuildPathsB = new WxAlphaBitmap( TEXT("BuildPaths"), 0 );
	BuildCoverNodesB = new WxAlphaBitmap( TEXT("BuildCoverNodes"), 0 );
	BuildAllB = new WxAlphaBitmap( TEXT("BuildAll"), 0 );
	// default PC play icon
	PlayB[B_PC] = new WxAlphaBitmap( TEXT("Play"), 0 );
	// Playstation play icon
	PlayB[B_PS3] = new WxAlphaBitmap( TEXT("PlayPS"), 0 );
	// Xbox play icon
	PlayB[B_XBox360] = new WxAlphaBitmap( TEXT("PlayXB"), 0 );

	// Create special windows

	MRUButton.Create( this, IDPB_MRU_DROPDOWN, &GApp->EditorFrame->DownArrowB, GApp->EditorFrame->GetMRUMenu(), wxPoint(0,0), wxSize(-1,21) );
	//MRUButton.SetToolTip( *LocalizeUnrealEd("ToolTip_20") );

	//PasteSpecialButton.Create( this, IDPB_PASTE_SPECIAL, &GApp->EditorFrame->DownArrowB, &PasteSpecialMenu, wxPoint(0,0), wxSize(-1,21) );
	//PasteSpecialButton.SetToolTip( *LocalizeUnrealEd("ToolTip_21") );

	// Far culling plane slider
	wxSlider* FarPlaneSlider = NULL;
	const int SliderMin = 0;
	const int SliderMax = 80;
	FarPlaneSlider = new wxSlider( this, ID_FarPlaneSlider, SliderMax, SliderMin, SliderMax, wxDefaultPosition, wxSize( 80, -1 ) );
	FarPlaneSlider->SetToolTip( *LocalizeUnrealEd("ToolTip_FarPlaneSlider") );

	// Coordinate systems
	CoordSystemCombo = new WxComboBox( this, IDCB_COORD_SYSTEM, TEXT(""), wxDefaultPosition, wxSize( 80, -1 ), 0, NULL, wxCB_READONLY );
	CoordSystemCombo->Append( *LocalizeUnrealEd("World") );
	CoordSystemCombo->Append( *LocalizeUnrealEd("Local") );
	CoordSystemCombo->SetSelection( GEditorModeTools().CoordSystem );
	CoordSystemCombo->SetToolTip( *LocalizeUnrealEd("ToolTip_22") );

	// Set up the ToolBar
	AddSeparator();
	AddTool( IDM_NEW, TEXT(""), NewB, *LocalizeUnrealEd("ToolTip_23") );
	AddTool( IDM_OPEN, TEXT(""), OpenB, *LocalizeUnrealEd("ToolTip_24") );
	AddControl( &MRUButton );
	AddTool( IDM_SAVE, TEXT(""), SaveB, *LocalizeUnrealEd("SaveCurrentLevel") );
	AddTool( IDM_SAVE_ALL_LEVELS, TEXT(""), SaveAllB, *LocalizeUnrealEd("ToolTip_SaveAllLevels") );
	AddSeparator();
	AddTool( IDM_UNDO, TEXT(""), UndoB, *LocalizeUnrealEd("ToolTip_26") );
	AddTool( IDM_REDO, TEXT(""), RedoB, *LocalizeUnrealEd("ToolTip_27") );
	AddSeparator();
	AddControl( FarPlaneSlider );
	AddSeparator();
	AddCheckTool( ID_CAMSPEED_SLOW, TEXT(""), CamSlowB, CamSlowB, *LocalizeUnrealEd("ToolTip_28") );
	AddCheckTool( ID_CAMSPEED_NORMAL, TEXT(""), CamNormalB, CamNormalB, *LocalizeUnrealEd("ToolTip_29") );
	AddCheckTool( ID_CAMSPEED_FAST, TEXT(""), CamFastB, CamFastB, *LocalizeUnrealEd("ToolTip_30") );
	AddSeparator();
	AddCheckTool( ID_EDIT_MOUSE_LOCK, TEXT(""), MouseLockB, MouseLockB, *LocalizeUnrealEd("ToolTip_MouseLock") );
	AddCheckTool( ID_EDIT_SHOW_WIDGET, TEXT(""), ShowWidgetB, ShowWidgetB, *LocalizeUnrealEd("ToolTip_31") );
	AddCheckTool( ID_EDIT_TRANSLATE, TEXT(""), TranslateB, TranslateB, *LocalizeUnrealEd("ToolTip_32") );
	AddCheckTool( ID_EDIT_ROTATE, TEXT(""), RotateB, RotateB, *LocalizeUnrealEd("ToolTip_33") );
	AddCheckTool( ID_EDIT_SCALE, TEXT(""), ScaleB, ScaleB, *LocalizeUnrealEd("ToolTip_34") );
	AddCheckTool( ID_EDIT_SCALE_NONUNIFORM, TEXT(""), ScaleNonUniformB, ScaleNonUniformB, *LocalizeUnrealEd("ToolTip_35") );
	AddSeparator();
	AddControl( CoordSystemCombo );
	AddSeparator();
	AddTool( IDM_SEARCH, TEXT(""), SearchB, *LocalizeUnrealEd("ToolTip_36") );
	AddSeparator();
	AddCheckTool( IDM_FULLSCREEN, TEXT(""), FullScreenB, FullScreenB, *LocalizeUnrealEd("ToolTip_37") );
	AddSeparator();
	AddTool( IDM_CUT, TEXT(""), CutB, *LocalizeUnrealEd("ToolTip_38") );
	AddTool( IDM_COPY, TEXT(""), CopyB, *LocalizeUnrealEd("ToolTip_39") );
	AddTool( IDM_PASTE, TEXT(""), PasteB, *LocalizeUnrealEd("ToolTip_40") );
	AddSeparator();
	AddTool( IDM_BROWSER_START, TEXT(""), GenericB, *LocalizeUnrealEd("ToolTip_41") );
	AddSeparator();
	AddTool( IDM_OPEN_KISMET, TEXT(""), KismetB, *LocalizeUnrealEd("ToolTip_42") );
	AddSeparator();
	AddCheckTool( IDM_BRUSHPOLYS, TEXT(""), BrushPolysB, BrushPolysB, *LocalizeUnrealEd("ToolTip_43") );
	AddSeparator();
	AddCheckTool( IDM_TogglePrefabsLocked, TEXT(""), PrefabLockB, PrefabLockB, *LocalizeUnrealEd("TogglePrefabLock") );
	AddSeparator();
	AddCheckTool( IDM_DISTRIBUTION_TOGGLE, TEXT(""), DistributionToggleB, DistributionToggleB, *LocalizeUnrealEd("ToolTip_DistributionToggle") );
	AddSeparator();
	AddTool( IDM_BUILD_GEOMETRY, TEXT(""), *BuildGeomB, *LocalizeUnrealEd("ToolTip_44") );
	AddTool( IDM_BUILD_LIGHTING, TEXT(""), *BuildLightingB, *LocalizeUnrealEd("ToolTip_45") );
	AddTool( IDM_BUILD_AI_PATHS, TEXT(""), *BuildPathsB, *LocalizeUnrealEd("ToolTip_46") );
	AddTool( IDM_BUILD_COVER, TEXT(""), *BuildCoverNodesB, *LocalizeUnrealEd("ToolTip_47") );
	AddTool( IDM_BUILD_ALL, TEXT(""), *BuildAllB, *LocalizeUnrealEd("ToolTip_48") );
	AddSeparator();

	// add a combo box for where to send object propagation messages to
	wxComboBox* PropagationCombo = new WxComboBox( this, IDCB_ObjectPropagation, TEXT(""), wxDefaultPosition, wxSize( 120, -1 ), 0, NULL, wxCB_READONLY );
	// Standard selection items
	PropagationCombo->Append(*LocalizeUnrealEd("NoPropagation")); // no propagation
	PropagationCombo->Append(*LocalizeUnrealEd("LocalStandalone")); // propagate to 127.0.0.1 (localhost)
	// allow for propagating to all loaded consoles
	for (FConsoleSupportIterator It; It; ++It)
	{
		PropagationCombo->Append(It->GetConsoleName());
	}
//	PropagationCombo->SetSelection( GEditorModeTools().CoordSystem );
	PropagationCombo->SetToolTip(*LocalizeUnrealEd("ToolTip_49"));
	AddControl(PropagationCombo);

	// start out with no propagation
	PropagationCombo->SetSelection(OPD_None);
	GEditor->SetObjectPropagationDestination(OPD_None);

	ViewPushStartStopButton = AddCheckTool( IDM_PUSHVIEW_StartStop, TEXT(""), ViewPushStartB, ViewPushStartB, *LocalizeUnrealEd("ToolTip_PushViewStart") );

	AddSeparator();

	// we always can play in the editor, put it's Play Icon in the toolbar
	AddTool(IDM_BuildPlayInEditor, TEXT(""), *PlayB[B_PC], *LocalizeUnrealEd("ToolTip_50"));
	// loop through all consoles (only support 20 consoles)
	INT ConsoleIndex = 0;
	for (FConsoleSupportIterator It; It && ConsoleIndex < 20; ++It, ConsoleIndex++)
	{
		// select console icon
		WxBitmap* PlayConsoleB = PlayB[B_PC];
		if( appStricmp( It->GetConsoleName(), TEXT("PS3") ) == 0 )
		{
			PlayConsoleB = PlayB[B_PS3];
		}
		else if( appStricmp( It->GetConsoleName(), TEXT("Xenon") ) == 0 )
		{
			PlayConsoleB = PlayB[B_XBox360];
		}

		// add a per-console Play On XXX button
		AddTool(
			IDM_BuildPlayConsole_START + ConsoleIndex, 
			TEXT(""),
			*PlayConsoleB,
			*FString::Printf(*LocalizeUnrealEd("ToolTip_50_F"), It->GetConsoleName())
			);
	}

	Realize();

	// The ViewPush button defulats to disabled because the object propagation destination is 'none'.
	// This must be called AFTER Realize()!
	EnablePushView( FALSE );
}

/** Updates the 'Push View' toggle's bitmap and hint text based on the input state. */
void WxMainToolBar::SetPushViewState(UBOOL bIsPushingView)
{
	if ( ViewPushStartStopButton )
	{
		ViewPushStartStopButton->SetNormalBitmap( bIsPushingView ? ViewPushStopB : ViewPushStartB );
		ViewPushStartStopButton->SetDisabledBitmap( bIsPushingView ? ViewPushStopB : ViewPushStartB );
		ViewPushStartStopButton->SetShortHelp( bIsPushingView ? *LocalizeUnrealEd("ToolTip_PushViewStop") : *LocalizeUnrealEd("ToolTip_PushViewStart") );
	}
}

/** Enables/disables the 'Push View' button. */
void WxMainToolBar::EnablePushView(UBOOL bEnabled)
{
	if ( ViewPushStartStopButton )
	{
		EnableTool( IDM_PUSHVIEW_StartStop, bEnabled ? true : false );
	}
}

/** Called when the trans/rot/scale widget toolbar buttons are right-clicked. */
void WxMainToolBar::OnTransformButtonRightClick(wxCommandEvent& In)
{
	WxDlgTransform::ETransformMode NewMode = WxDlgTransform::TM_Translate;
	switch( In.GetId() )
	{
		case ID_EDIT_ROTATE:
			NewMode = WxDlgTransform::TM_Rotate;
			break;
		case ID_EDIT_SCALE:
		case ID_EDIT_SCALE_NONUNIFORM:
			return;
			NewMode = WxDlgTransform::TM_Scale;
			break;
	}

	const bool bIsShown = GApp->DlgTransform->IsShown();
	const UBOOL bSameTransformMode = GApp->DlgTransform->GetTransformMode() == NewMode;
	if ( bIsShown && bSameTransformMode )
	{
		GApp->DlgTransform->Show( false );
	}
	else
	{
		GApp->DlgTransform->SetTransformMode( NewMode );
		GApp->DlgTransform->Show( true );
	}
}
