/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "LevelViewportToolBar.h"
#include "UnTerrain.h"
#include "GroupUtils.h"
#include "DlgDensityRenderingOptions.h"

#include "InterpEditor.h"

/** Simple enum for the play in viewport buttons */
enum EPIEButtons
{
	PIE_Play,
	PIE_Stop
};

BEGIN_EVENT_TABLE( WxLevelViewportToolBar, WxToolBar )
	EVT_TOOL( IDM_REALTIME, WxLevelViewportToolBar::OnRealTime )
	EVT_UPDATE_UI( IDM_REALTIME, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )

	EVT_TOOL( IDM_MOVEUNLIT, WxLevelViewportToolBar::OnMoveUnlit )
	EVT_UPDATE_UI( IDM_MOVEUNLIT, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )

	EVT_TOOL( IDM_LevelStreamingVolumePreVis, WxLevelViewportToolBar::OnLevelStreamingVolumePreVis )
	EVT_TOOL( IDM_PostProcessVolumePreVis, WxLevelViewportToolBar::OnPostProcessVolumePreVis )
	EVT_TOOL( IDM_SQUINTBLURMODE, WxLevelViewportToolBar::OnSquintModeChange )
	EVT_TOOL( IDM_VIEWPORTLOCKED, WxLevelViewportToolBar::OnViewportLocked )

	EVT_TOOL( IDM_BRUSHWIREFRAME, WxLevelViewportToolBar::OnBrushWireframe )
	EVT_TOOL( IDM_WIREFRAME, WxLevelViewportToolBar::OnWireframe )
	EVT_TOOL( IDM_UNLIT, WxLevelViewportToolBar::OnUnlit )
	EVT_TOOL( IDM_LIT, WxLevelViewportToolBar::OnLit )
	EVT_TOOL( IDM_DETAILLIGHTING, WxLevelViewportToolBar::OnDetailLighting )
	EVT_TOOL( IDM_LIGHTINGONLY, WxLevelViewportToolBar::OnLightingOnly )
	EVT_TOOL( IDM_LIGHTCOMPLEXITY, WxLevelViewportToolBar::OnLightComplexity )
	EVT_TOOL( IDM_TEXTUREDENSITY, WxLevelViewportToolBar::OnTextureDensity )
	EVT_TOOL( IDM_SHADERCOMPLEXITY, WxLevelViewportToolBar::OnShaderComplexity )
	EVT_TOOL( IDM_LIGHTMAPDENSITY, WxLevelViewportToolBar::OnLightMapDensity )
	EVT_TOOL( IDM_LITLIGHTMAPDENSITY, WxLevelViewportToolBar::OnLitLightmapDensity )
	EVT_TOOL( IDM_REFLECTIONS, WxLevelViewportToolBar::OnReflections )

	EVT_UPDATE_UI( IDM_BRUSHWIREFRAME, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_WIREFRAME, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_UNLIT, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_LIT, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_DETAILLIGHTING, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_LIGHTINGONLY, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_LIGHTCOMPLEXITY, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_TEXTUREDENSITY, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_SHADERCOMPLEXITY, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_LIGHTMAPDENSITY, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_LITLIGHTMAPDENSITY, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_REFLECTIONS, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )

	EVT_TOOL( IDM_PERSPECTIVE, WxLevelViewportToolBar::OnPerspective )
	EVT_TOOL( IDM_TOP, WxLevelViewportToolBar::OnTop )
	EVT_TOOL( IDM_FRONT, WxLevelViewportToolBar::OnFront )
	EVT_TOOL( IDM_SIDE, WxLevelViewportToolBar::OnSide )
	EVT_UPDATE_UI( IDM_PERSPECTIVE, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_TOP, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_FRONT, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_UPDATE_UI( IDM_SIDE, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )

	EVT_TOOL( IDM_TearOffNewFloatingViewport, WxLevelViewportToolBar::OnTearOffNewFloatingViewport )
	EVT_TOOL( IDM_AllowMatineePreview, WxLevelViewportToolBar::OnToggleAllowMatineePreview )
	EVT_UPDATE_UI( IDM_AllowMatineePreview, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )
	EVT_TOOL( IDM_LOCK_SELECTED_TO_CAMERA, WxLevelViewportToolBar::OnLockSelectedToCamera )
	EVT_BUTTON( IDM_LevelViewportToolbar_PlayInViewport, WxLevelViewportToolBar::OnPlayInViewport )
	EVT_UPDATE_UI( IDM_LevelViewportToolbar_PlayInViewport, WxLevelViewportToolBar::UpdateUI_OnPlayInViewport )

	EVT_BUTTON( IDM_VIEWPORT_MATINEE_TOGGLE_RECORD_TRACKS, WxLevelViewportToolBar::OnToggleRecordInterpValues )
	EVT_UPDATE_UI( IDM_VIEWPORT_MATINEE_TOGGLE_RECORD_TRACKS, WxLevelViewportToolBar::UpdateUI_RecordInterpValues )
	EVT_COMBOBOX( IDM_VIEWPORT_MATINEE_RECORD_MODE, WxLevelViewportToolBar::OnRecordModeChange)

	EVT_MENU( IDM_MakeOcclusionParent, WxLevelViewportToolBar::OnMakeParentViewport )
	EVT_UPDATE_UI( IDM_MakeOcclusionParent, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )

	EVT_MENU( IDM_LevelViewportOptionsMenu, WxLevelViewportToolBar::OnOptionsMenu )
	EVT_MENU( IDM_LevelViewportShowFlagsShortcut, WxLevelViewportToolBar::OnShowFlagsShortcut )
	EVT_MENU( IDM_VIEWPORT_SHOW_DEFAULTS, WxLevelViewportToolBar::OnShowDefaults )

	EVT_MENU( IDM_ViewportOptions_ShowFPS, WxLevelViewportToolBar::OnShowFPS )
	EVT_UPDATE_UI( IDM_ViewportOptions_ShowFPS, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )

	EVT_MENU( IDM_ViewportOptions_ShowStats, WxLevelViewportToolBar::OnShowStats )
	EVT_UPDATE_UI( IDM_ViewportOptions_ShowStats, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )

	EVT_MENU( IDM_VIEWPORT_SHOW_GAMEVIEW, WxLevelViewportToolBar::OnGameView )
	EVT_UPDATE_UI( IDM_VIEWPORT_SHOW_GAMEVIEW, WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu )

	EVT_MENU( IDM_VIEWPORT_SHOW_COLLISION_ZEROEXTENT, WxLevelViewportToolBar::OnChangeCollisionMode )
	EVT_MENU( IDM_VIEWPORT_SHOW_COLLISION_NONZEROEXTENT, WxLevelViewportToolBar::OnChangeCollisionMode )
	EVT_MENU( IDM_VIEWPORT_SHOW_COLLISION_RIGIDBODY, WxLevelViewportToolBar::OnChangeCollisionMode )
	EVT_MENU( IDM_VIEWPORT_SHOW_COLLISION_NONE, WxLevelViewportToolBar::OnChangeCollisionMode )
	EVT_MENU( IDM_PerViewGroups_HideAll, WxLevelViewportToolBar::OnPerViewGroupsHideAll )
	EVT_MENU( IDM_PerViewGroups_HideNone, WxLevelViewportToolBar::OnPerViewGroupsHideNone )
	EVT_MENU( IDM_VIEWPORT_TYPE_CYCLE_BUTTON, WxLevelViewportToolBar::OnViewportTypeLeftClick )
	EVT_TOOL_RCLICKED( IDM_VIEWPORT_TYPE_CYCLE_BUTTON, WxLevelViewportToolBar::OnViewportTypeRightClick )
	EVT_MENU( ID_CAMSPEED_CYCLE_BUTTON, WxLevelViewportToolBar::OnCamSpeedButtonLeftClick )
	EVT_TOOL_RCLICKED( ID_CAMSPEED_CYCLE_BUTTON, WxLevelViewportToolBar::OnCamSpeedButtonRightClick )
	EVT_COMMAND_RANGE( ID_CAMSPEED_SLOW, ID_CAMSPEED_FAST, EVT_CAMSPEED_UPDATE, WxLevelViewportToolBar::OnCamSpeedUpdateEvent )
	EVT_MENU_RANGE (IDM_PerViewGroups_Start, IDM_PerViewGroups_End, WxLevelViewportToolBar::OnTogglePerViewGroup )
	EVT_MENU_RANGE (IDM_VolumeClasses_START, IDM_VolumeClasses_END, WxLevelViewportToolBar::OnChangeVolumeVisibility )
	EVT_MENU( IDM_VolumeActorVisibilityShowAll, WxLevelViewportToolBar::OnToggleAllVolumeActors )
	EVT_MENU( IDM_VolumeActorVisibilityHideAll, WxLevelViewportToolBar::OnToggleAllVolumeActors )
	EVT_MENU_RANGE( IDM_VIEWPORT_SHOWFLAGS_START, IDM_VIEWPORT_SHOWFLAGS_END, WxLevelViewportToolBar::OnToggleShowFlag )
END_EVENT_TABLE()

/** Converts a Resource ID  into its equivalent ELevelViewport type.
 *
 * @param ViewportID	The resource ID for the viewport type
 * @return	The ELevelViewport type for the passed in resource ID.
 */
static ELevelViewportType ViewportTypeFromResourceID( INT ViewportID )
{
	ELevelViewportType CurrentViewportType;
	switch (ViewportID)
	{
	case IDM_TOP:
		CurrentViewportType = LVT_OrthoXY;
		break;
	case IDM_SIDE:
		CurrentViewportType = LVT_OrthoXZ;
		break;
	case IDM_FRONT:
		CurrentViewportType = LVT_OrthoYZ;
		break;
	case IDM_PERSPECTIVE:
		CurrentViewportType = LVT_Perspective;
		break;
	default:
		CurrentViewportType = LVT_None;
		break;
	}

	return CurrentViewportType;
}

/** Converts a ELevelViewportType into its equivalent resource ID type.
 *
 * @param InType	The ELevelViewport type to convert
 * @return	The resource ID  for the passed in type.
 */
static INT ResourceIDFromViewportType( ELevelViewportType InType )
{
	INT CurrentViewportType;
	switch (InType)
	{
	case LVT_OrthoXY:
		CurrentViewportType = IDM_TOP;
		break;
	case LVT_OrthoXZ:
		CurrentViewportType = IDM_SIDE;
		break;
	case LVT_OrthoYZ:
		CurrentViewportType = IDM_FRONT;
		break;
	case LVT_Perspective:
		CurrentViewportType = IDM_PERSPECTIVE;
		break;
	default:
		CurrentViewportType = -1;
		break;
	}

	return CurrentViewportType;
}

WxLevelViewportToolBar::WxLevelViewportToolBar( wxWindow* InParent, wxWindowID InID, FEditorLevelViewportClient* InViewportClient )
	: WxToolBar( InParent, InID, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxTB_FLAT | wxTB_3DBUTTONS )
	,	LastCameraSpeedID( ID_CAMSPEED_START )
	,	ViewportClient( InViewportClient )
	,	MatineeWindow(NULL)
{
	// Bitmaps
	LockSelectedToCameraB.Load( TEXT("LVT_LockSelectedToCamera") );
	LockViewportB.Load( TEXT("LVT_LockViewport") );
	RealTimeB.Load( TEXT("Realtime") );
	StreamingPrevisB.Load( TEXT("LVT_StreamingPrevis") );
	PostProcessPrevisB.Load( TEXT("LVT_PPPrevis") );
	ShowFlagsB.Load( TEXT("Showflags") );
	GameViewB.Load( TEXT("LVT_GameView" ) );
	SquintButtonB.Load( TEXT("Squint") );
	CamSpeedsB[0].Load( TEXT("CamSlow") );
	CamSpeedsB[1].Load( TEXT("CamNormal") );
	CamSpeedsB[2].Load( TEXT("CamFast") );

	//SetToolBitmapSize( wxSize( 16, 16 ) );

	BrushWireframeB.Load( TEXT("LVT_BrushWire") );
	WireframeB.Load( TEXT("LVT_Wire") );
	UnlitB.Load( TEXT("LVT_Unlit") );
	LitB.Load( TEXT("LVT_Lit") );
	DetailLightingB.Load( TEXT("LVT_DetailLighting") );
	LightingOnlyB.Load( TEXT("LVT_LightingOnly") );
	LightComplexityB.Load( TEXT("LVT_LightingComplexity") );
	TextureDensityB.Load( TEXT("LVT_TextureDensity") );
	ShaderComplexityB.Load( TEXT("LVT_ShaderComplexity") );
	LightMapDensityB.Load( TEXT("LVT_LightMapDensity" ) );
	LitLightmapDensityB.Load( TEXT("LVT_LitLightmapDensity") );

	// Load the viewport type bitmaps
	ViewportTypesB[IDM_PERSPECTIVE - IDM_VIEWPORT_TYPE_START].Load( TEXT("LVT_Perspective") );
	ViewportTypesB[IDM_TOP - IDM_VIEWPORT_TYPE_START].Load( TEXT("LVT_Top") );
	ViewportTypesB[IDM_FRONT - IDM_VIEWPORT_TYPE_START].Load( TEXT("LVT_Front") );
	ViewportTypesB[IDM_SIDE - IDM_VIEWPORT_TYPE_START].Load( TEXT("LVT_Side") );
	

	TearOffNewFloatingViewportB.Load( TEXT( "LVT_TearOffNewFloatingViewport" ) );

	// Play-In-Viewport button
	PlayInViewportStartB.Load( TEXT("PlayInViewportPlay.png") );
	PlayInViewportStopB.Load( TEXT("PlayInEditorStop.png") );

	// Set up the ToolBar
	//AddSeparator();
	AddTool( IDM_LevelViewportOptionsMenu, TEXT(""), ShowFlagsB, *LocalizeUnrealEd("LevelViewportToolBar_OptionsButton_ToolTip") );
	if (GEditor->GetUserSettings().bEnableShowFlagsShortcut)
	{
		AddTool( IDM_LevelViewportShowFlagsShortcut, TEXT(""), ShowFlagsB, *LocalizeUnrealEd("LevelViewportToolBar_OptionsButton_ToolTip") );
	}
	AddSeparator();
	AddTool( IDM_VIEWPORT_TYPE_CYCLE_BUTTON, TEXT(""), ViewportTypesB[ResourceIDFromViewportType(ViewportClient->ViewportType) - IDM_VIEWPORT_TYPE_START], *LocalizeUnrealEd("ToolTip_ViewportTypeSetting") );
	AddCheckTool( IDM_REALTIME, TEXT(""), RealTimeB, RealTimeB, *LocalizeUnrealEd("LevelViewportToolBar_RealTime") );
	AddSeparator();
	AddCheckTool( IDM_BRUSHWIREFRAME, TEXT(""), BrushWireframeB, BrushWireframeB, *LocalizeUnrealEd("LevelViewportToolbar_BrushWireframe") );
	AddCheckTool( IDM_WIREFRAME, TEXT(""), WireframeB, WireframeB, *LocalizeUnrealEd("LevelViewportToolbar_Wireframe") );
	AddCheckTool( IDM_UNLIT, TEXT(""), UnlitB, UnlitB, *LocalizeUnrealEd("LevelViewportToolbar_Unlit") );
	AddCheckTool( IDM_LIT, TEXT(""), LitB, LitB, *LocalizeUnrealEd("LevelViewportToolbar_Lit") );
	AddCheckTool( IDM_DETAILLIGHTING, TEXT(""), DetailLightingB, DetailLightingB, *LocalizeUnrealEd("LevelViewportToolbar_DetailLighting") );
	AddCheckTool( IDM_LIGHTINGONLY, TEXT(""), LightingOnlyB, LightingOnlyB, *LocalizeUnrealEd("LevelViewportToolbar_LightingOnly") );
	AddCheckTool( IDM_LIGHTCOMPLEXITY, TEXT(""), LightComplexityB, LightComplexityB, *LocalizeUnrealEd("LevelViewportToolbar_LightComplexity") );
	AddCheckTool( IDM_TEXTUREDENSITY, TEXT(""), TextureDensityB, TextureDensityB, *LocalizeUnrealEd("LevelViewportToolbar_TextureDensity") );
	AddCheckTool( IDM_SHADERCOMPLEXITY, TEXT(""), ShaderComplexityB, ShaderComplexityB, *LocalizeUnrealEd("LevelViewportToolbar_ShaderComplexity") );
	AddCheckTool( IDM_LIGHTMAPDENSITY, TEXT(""), LightMapDensityB, LightMapDensityB, *LocalizeUnrealEd("LevelViewportToolbar_LightMapDensity") );
	AddCheckTool( IDM_LITLIGHTMAPDENSITY, TEXT(""), LitLightmapDensityB, LitLightmapDensityB, *LocalizeUnrealEd("LevelViewportToolbar_LitLightmapDensity") );
	AddSeparator();
	AddCheckTool( IDM_VIEWPORT_SHOW_GAMEVIEW, TEXT(""), GameViewB, GameViewB, *LocalizeUnrealEd("LevelViewportToolBar_ToggleGameView_ToolTip") );
	AddSeparator();
	AddCheckTool( IDM_VIEWPORTLOCKED, TEXT(""), LockViewportB, LockViewportB, *LocalizeUnrealEd("ToolTip_ViewportLocked") );
	AddCheckTool( IDM_LOCK_SELECTED_TO_CAMERA, TEXT(""), LockSelectedToCameraB, LockSelectedToCameraB, *LocalizeUnrealEd("ToolTip_159") );
	AddSeparator();
	AddCheckTool( IDM_LevelStreamingVolumePreVis, TEXT(""), StreamingPrevisB, StreamingPrevisB, *LocalizeUnrealEd("ToolTip_LevelStreamingVolumePrevis") );
	AddCheckTool( IDM_PostProcessVolumePreVis, TEXT(""), PostProcessPrevisB, PostProcessPrevisB, *LocalizeUnrealEd("ToolTip_PostProcessVolumePrevis") );
#if !UDK
	AddCheckTool( IDM_SQUINTBLURMODE, TEXT(""), SquintButtonB, SquintButtonB, *LocalizeUnrealEd("TollTip_SquintMode") );
#endif
	AddSeparator();
	
	AddTool( ID_CAMSPEED_CYCLE_BUTTON, TEXT(""), CamSpeedsB[LastCameraSpeedID - ID_CAMSPEED_START], *LocalizeUnrealEd("ToolTip_CameraSpeedSetting") );

	AddSeparator();
	
	// Set up the play in viewport button
	PlayInViewportButton = new WxBitmapStateButton( this, this, IDM_LevelViewportToolbar_PlayInViewport, wxDefaultPosition, wxSize( 35, 20 ), FALSE );
	PlayInViewportButton->AddState( PIE_Play, &PlayInViewportStartB );
	PlayInViewportButton->AddState( PIE_Stop, &PlayInViewportStopB );
	PlayInViewportButton->SetToolTip( *LocalizeUnrealEd("LevelViewportToolbar_PlayInViewportStart") );
	PlayInViewportButton->SetCurrentState( PIE_Play );
	AddControl( PlayInViewportButton );

	if( !ViewportClient->IsFloatingViewport() )
	{
		AddSeparator();

		// Only add the 'tear off' button for non-floating viewports
		AddTool( IDM_TearOffNewFloatingViewport, TEXT( "" ), TearOffNewFloatingViewportB, *LocalizeUnrealEd( "ViewportToolbar_TearOffNewFloatingViewport" ) );
	}

	Realize();
	// UpdateUI must be called *after* Realize.
	UpdateUI();
}

void WxLevelViewportToolBar::AppendMatineeRecordOptions(WxInterpEd* InInterpEd)
{
	check(InInterpEd);
	check(MatineeWindow==NULL);

	//Save matinee window for interface
	MatineeWindow = InInterpEd;

	AddSeparator();

	//add record toggle button
	MatineeStartRecordB.Load( TEXT("Record.png") );
	MatineeCancelRecordB.Load( TEXT("Stop.png") );

	WxBitmapCheckButton* RecordModeButton = new WxBitmapCheckButton(this, this, IDM_VIEWPORT_MATINEE_TOGGLE_RECORD_TRACKS, &MatineeStartRecordB, &MatineeCancelRecordB, wxDefaultPosition, wxDefaultSize);
	RecordModeButton->SetToolTip(*LocalizeUnrealEd("InterpEd_ToggleMatineeRecord_Tooltip"));
	AddControl( RecordModeButton );

	//add number of "takes" drop down
	WxComboBox* RecordModeCombo = new WxComboBox( this, IDM_VIEWPORT_MATINEE_RECORD_MODE, TEXT(""), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	RecordModeCombo->Append( *LocalizeUnrealEd("InterpEd_RecordMode_NewCameraMode"));
	RecordModeCombo->Append( *LocalizeUnrealEd("InterpEd_RecordMode_NewCameraAttachedMode"));
	RecordModeCombo->Append( *LocalizeUnrealEd("InterpEd_RecordMode_DuplicateTracksMode"));
	RecordModeCombo->Append( *LocalizeUnrealEd("InterpEd_RecordMode_ReplaceTracksMode"));
	RecordModeCombo->SetSelection( 0 );
	RecordModeCombo->SetToolTip( *LocalizeUnrealEd("InterpEd_MatineeRecordMode_Tooltip") );

	AddSeparator();
	AddControl( RecordModeCombo );

	//since we're adding new options we have to re-realize the toolbar
	Realize();
	// UpdateUI must be called *after* Realize.
	UpdateUI();
}


void WxLevelViewportToolBar::UpdateUI()
{
	ToggleTool( IDM_VIEWPORT_SHOW_GAMEVIEW, AreGameViewFlagsSet() ? true : false );
	ToggleTool( IDM_REALTIME, ViewportClient->IsRealtime()==TRUE );

	UpdateToolBarButtonEnabledStates();

	INT ViewModeID = -1;
	EShowFlags ViewModeShowFlags = ViewportClient->ShowFlags & SHOW_ViewMode_Mask;

	if( ViewModeShowFlags == SHOW_ViewMode_BrushWireframe )
	{
		ViewModeID = 0;
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_Wireframe )
	{
		ViewModeID = 1;
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_Unlit )
	{
		ViewModeID = 2;
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_Lit)
	{
		if (ViewportClient->bOverrideDiffuseAndSpecular)
		{
			ViewModeID = 4;
		}
		else
		{
			if (ViewportClient->bShowReflectionsOnly)
			{
				ViewModeID = 11;
			}
			else
			{
				ViewModeID = 3;
			}
		}
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_LightingOnly)
	{
		ViewModeID = 5;
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_LightComplexity)
	{
		ViewModeID = 6;
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_TextureDensity)
	{
		ViewModeID = 7;
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_ShaderComplexity)
	{
		ViewModeID = 8;
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_LightMapDensity)
	{
		ViewModeID = 9;
	}
	else if( ViewModeShowFlags == SHOW_ViewMode_LitLightmapDensity)
	{
		ViewModeID = 10;
	}
	SetViewModeUI( ViewModeID );
}


/** Updates UI state for the various viewport option menu items */
void WxLevelViewportToolBar::UpdateUI_ViewportOptionsMenu( wxUpdateUIEvent& In )
{
	const bool bIsPerspectiveView = ViewportClient->ViewportType == LVT_Perspective;
	switch( In.GetId() )
	{
		case IDM_REALTIME:
			In.Check( ViewportClient->IsRealtime() ? true : false );
			break;

		// ...

		case IDM_BRUSHWIREFRAME: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_BrushWireframe );
			break;

		case IDM_WIREFRAME: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_Wireframe );
			break;

		case IDM_UNLIT: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_Unlit );
			break;

		case IDM_LIT: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_Lit && !ViewportClient->bOverrideDiffuseAndSpecular );
			break;

		case IDM_DETAILLIGHTING: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_Lit && ViewportClient->bOverrideDiffuseAndSpecular );
			break;

		case IDM_LIGHTINGONLY: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_LightingOnly );
			break;

		case IDM_LIGHTCOMPLEXITY: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_LightComplexity );
			break;

		case IDM_TEXTUREDENSITY: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_TextureDensity );
			break;

		case IDM_SHADERCOMPLEXITY: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_ShaderComplexity );
			break;

		case IDM_LIGHTMAPDENSITY: 
			In.Check( ( ViewportClient->ShowFlags & SHOW_ViewMode_Mask ) == SHOW_ViewMode_LightMapDensity );
			break;

		case IDM_LITLIGHTMAPDENSITY:
			In.Check( (ViewportClient->ShowFlags & SHOW_ViewMode_Mask) == SHOW_ViewMode_LitLightmapDensity );
			break;

		case IDM_REFLECTIONS:
			In.Check( (ViewportClient->ShowFlags & SHOW_ViewMode_Mask) == SHOW_ViewMode_Lit && ViewportClient->bShowReflectionsOnly );
			break;

		// ...

		case IDM_PERSPECTIVE:
			In.Check( ViewportClient->ViewportType == LVT_Perspective );
			break;

		case IDM_TOP:
			In.Check( ViewportClient->ViewportType == LVT_OrthoXY );
			break;

		case IDM_FRONT:
			In.Check( ViewportClient->ViewportType == LVT_OrthoYZ );
			break;

		case IDM_SIDE:
			In.Check( ViewportClient->ViewportType == LVT_OrthoXZ );
			break;

		// ...

		case IDM_AllowMatineePreview:
			In.Enable( bIsPerspectiveView );
			In.Check( bIsPerspectiveView && ViewportClient->AllowMatineePreview() );
			break;

		case IDM_MOVEUNLIT:
			In.Check( ViewportClient->bMoveUnlit == TRUE );
			break;

		case IDM_VIEWPORT_SHOW_GAMEVIEW:
			In.Check( AreGameViewFlagsSet() ? true : false );
			break;

		case IDM_MakeOcclusionParent:
			In.Enable( bIsPerspectiveView );
			In.Check( ViewportClient->ViewState->IsViewParent()==TRUE );
			break;


		// ...

		case IDM_ViewportOptions_ShowFPS:
			In.Check( ViewportClient->ShouldShowFPS() == TRUE );
			break;

		case IDM_ViewportOptions_ShowStats:
			In.Check( ViewportClient->ShouldShowStats() == TRUE );
			break;
	}
}



void WxLevelViewportToolBar::OnSquintModeChange( wxCommandEvent& In )
{
	ViewportClient->SetSquintMode( In.IsChecked() );
	ViewportClient->Viewport->Invalidate();
}

void WxLevelViewportToolBar::OnRealTime( wxCommandEvent& In )
{
	ViewportClient->SetRealtime( In.IsChecked() );
	ViewportClient->Invalidate( FALSE );
}


void WxLevelViewportToolBar::OnMoveUnlit( wxCommandEvent& In )
{
	ViewportClient->bMoveUnlit = In.IsChecked();
	ViewportClient->Invalidate( FALSE );
}



void WxLevelViewportToolBar::OnToggleAllowMatineePreview( wxCommandEvent& In )
{
	ViewportClient->SetAllowMatineePreview( In.IsChecked() );
	ViewportClient->Invalidate( FALSE );
}


/**Callback when toggling recording of matinee camera movement*/
void WxLevelViewportToolBar::OnToggleRecordInterpValues( wxCommandEvent& In )
{
	MatineeWindow->ToggleRecordInterpValues();
}

/**UI Adjustment for recording of matinee camera movement*/
void WxLevelViewportToolBar::UpdateUI_RecordInterpValues (wxUpdateUIEvent& In)
{
	wxObject* EventObject = In.GetEventObject();
	WxBitmapCheckButton* CheckButton = wxDynamicCast(EventObject, WxBitmapCheckButton);
	if (CheckButton)
	{
		//the standard logic (ON) is for scrict intersection
		INT TestState = MatineeWindow->IsRecordingInterpValues() ? WxBitmapCheckButton::STATE_On : WxBitmapCheckButton::STATE_Off;
		if (TestState != CheckButton->GetCurrentState()->ID)
		{
			CheckButton->SetCurrentState(TestState);
		}
	}
	In.Check( MatineeWindow->IsRecordingInterpValues() ? true : false );
}

/**Callback when changing the number of camera takes that will be used when sampling the camera for matinee*/
void WxLevelViewportToolBar::OnRecordModeChange ( wxCommandEvent& In )
{
	MatineeWindow->SetRecordMode(In.GetInt());
}

/** Called from the window event handler to launch Play-In-Editor for this viewport */
void WxLevelViewportToolBar::OnPlayInViewport( wxCommandEvent& In )
{
	// If there isn't already a PIE session in progress, start a new one in the viewport
	if ( PlayInViewportButton->GetCurrentState()->ID == PIE_Play )
	{
		if ( GApp && GApp->EditorFrame && GApp->EditorFrame->ViewportConfigData )
		{
			FVector* StartLocation = NULL;
			FRotator* StartRotation = NULL;

			// If this is a perspective viewport, then we'll Play From Here
			if( ViewportClient->ViewportType == LVT_Perspective )
			{
				// Start PIE from the camera's location and orientation!
				StartLocation = &ViewportClient->ViewLocation;
				StartRotation = &ViewportClient->ViewRotation;
			}

			// Figure out which viewport index we are
			INT MyViewportIndex = -1;
			for( INT CurViewportIndex = 0; CurViewportIndex < GApp->EditorFrame->ViewportConfigData->GetViewportCount(); ++CurViewportIndex )
			{
				FVCD_Viewport& CurViewport = GApp->EditorFrame->ViewportConfigData->AccessViewport( CurViewportIndex );
				if( CurViewport.bEnabled && CurViewport.ViewportWindow->Viewport == ViewportClient->Viewport )
				{
					MyViewportIndex = CurViewportIndex;
					break;
				}
			}

			// Queue a PIE session for this viewport
			GUnrealEd->PlayMap( StartLocation, StartRotation, -1, MyViewportIndex );
		}
	}
	// Stop the current PIE session
	else
	{
		GUnrealEd->EndPlayMap();
	}
}

/** Update the UI for the play in viewport button */
void WxLevelViewportToolBar::UpdateUI_OnPlayInViewport( wxUpdateUIEvent& In )
{
	// If the state of PIE is different from what the PIE button currently shows, update the state and tooltip of the button
	const INT CurState = ( GEditor->PlayWorld != NULL ) ? PIE_Stop : PIE_Play;
	if ( PlayInViewportButton->GetCurrentState()->ID != CurState )
	{
		PlayInViewportButton->SetCurrentState( CurState );
		FString NewToolTip = ( CurState == PIE_Play ) ? LocalizeUnrealEd("LevelViewportToolbar_PlayInViewportStart") : LocalizeUnrealEd("LevelViewportToolbar_PlayInViewportStop");
		PlayInViewportButton->SetToolTip( *NewToolTip );
	}
}



void WxLevelViewportToolBar::OnLevelStreamingVolumePreVis( wxCommandEvent& In )
{
	// Level streaming volume previews is only possible with perspective level viewports.
	check( ViewportClient->IsPerspective() );
	ViewportClient->bLevelStreamingVolumePrevis = In.IsChecked();
	
	// Redraw all viewports, as streaming volume previs draws a camera actor in the ortho viewports as well
	GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
}

void WxLevelViewportToolBar::OnPostProcessVolumePreVis( wxCommandEvent& In )
{
	// Post process volume previews is only possible with perspective level viewports.
	check( ViewportClient->IsPerspective() );
	ViewportClient->bPostProcessVolumePrevis = In.IsChecked();
	ViewportClient->Invalidate( FALSE );
}

void WxLevelViewportToolBar::OnViewportLocked( wxCommandEvent& In )
{
	ViewportClient->bViewportLocked = In.IsChecked();
}

void WxLevelViewportToolBar::SetViewModeUI(INT ViewModeID)
{
	ToggleTool( IDM_BRUSHWIREFRAME, false );
	ToggleTool( IDM_WIREFRAME, false );
	ToggleTool( IDM_UNLIT, false );
	ToggleTool( IDM_LIT, false );
	ToggleTool( IDM_DETAILLIGHTING, false );
	ToggleTool( IDM_LIGHTINGONLY, false );
	ToggleTool( IDM_LIGHTCOMPLEXITY, false );
	ToggleTool( IDM_TEXTUREDENSITY, false );
	ToggleTool( IDM_SHADERCOMPLEXITY, false );
	ToggleTool( IDM_LIGHTMAPDENSITY, false );
	ToggleTool( IDM_LITLIGHTMAPDENSITY, false );
	ToggleTool( IDM_REFLECTIONS, false );
	ViewportClient->bOverrideDiffuseAndSpecular = FALSE;
	ViewportClient->bShowReflectionsOnly = FALSE;
	switch( ViewModeID )
	{
	case 0:		ToggleTool( IDM_BRUSHWIREFRAME, true );		break;
	case 1:		ToggleTool( IDM_WIREFRAME, true );			break;
	case 2:		ToggleTool( IDM_UNLIT, true );				break;
	case 3:		ToggleTool( IDM_LIT, true );				break;
	case 4:		ToggleTool( IDM_DETAILLIGHTING, true );		
		ViewportClient->bOverrideDiffuseAndSpecular = TRUE;
		break;
	case 5:		ToggleTool( IDM_LIGHTINGONLY, true );		break;
	case 6:		ToggleTool( IDM_LIGHTCOMPLEXITY, true );	break;
	case 7:		ToggleTool( IDM_TEXTUREDENSITY, true );		break;
	case 8:		ToggleTool( IDM_SHADERCOMPLEXITY, true );	break;
	case 9:		ToggleTool( IDM_LIGHTMAPDENSITY, true );	break;
	case 10:	ToggleTool( IDM_LITLIGHTMAPDENSITY, true );	break;
	case 11:	ToggleTool( IDM_REFLECTIONS, true );
		ViewportClient->bShowReflectionsOnly = TRUE;
		break;
	default:
		break;
	}
}

void WxLevelViewportToolBar::UpdateToolBarButtonEnabledStates()
{
	// Level streaming and post process volume previs is only possible with perspective level viewports.
	const bool bIsPerspectiveView = ViewportClient->ViewportType == LVT_Perspective;
	EnableTool( IDM_LevelStreamingVolumePreVis, bIsPerspectiveView );
	EnableTool( IDM_PostProcessVolumePreVis, bIsPerspectiveView );

	SetToolNormalBitmap( IDM_VIEWPORT_TYPE_CYCLE_BUTTON, ViewportTypesB[ ResourceIDFromViewportType( ViewportClient->ViewportType ) - IDM_VIEWPORT_TYPE_START ] );
	if ( bIsPerspectiveView )
	{
		ToggleTool( IDM_LevelStreamingVolumePreVis, ViewportClient->bLevelStreamingVolumePrevis==TRUE );
		ToggleTool( IDM_PostProcessVolumePreVis, ViewportClient->bPostProcessVolumePrevis==TRUE );
	}
}

void WxLevelViewportToolBar::OnBrushWireframe( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_BrushWireframe;
	SetViewModeUI( 0 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnWireframe( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_Wireframe;
	SetViewModeUI( 1 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnUnlit( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_Unlit;
	SetViewModeUI( 2 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnLit( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_Lit;
	SetViewModeUI( 3 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnDetailLighting( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_Lit;
	SetViewModeUI( 4 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnLightingOnly( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_LightingOnly;
	SetViewModeUI( 5 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnLightComplexity( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_LightComplexity;
	SetViewModeUI( 6 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnTextureDensity( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_TextureDensity;
	SetViewModeUI( 7 );
	ViewportClient->Invalidate();

	WxDlgDensityRenderingOptions* DensityRenderingOptions = GApp->GetDlgDensityRenderingOptions();
	check(DensityRenderingOptions);
	DensityRenderingOptions->Show(false);
}

void WxLevelViewportToolBar::OnShaderComplexity( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_ShaderComplexity;;
	SetViewModeUI( 8 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnLightMapDensity( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_LightMapDensity;
	SetViewModeUI( 9 );
	ViewportClient->Invalidate();

	WxDlgDensityRenderingOptions* DensityRenderingOptions = GApp->GetDlgDensityRenderingOptions();
	check(DensityRenderingOptions);
	DensityRenderingOptions->Show(true);
}

void WxLevelViewportToolBar::OnLitLightmapDensity( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_LitLightmapDensity;
	SetViewModeUI( 10 );
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnReflections( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags |= SHOW_ViewMode_Lit;
	SetViewModeUI( 11 );
	ViewportClient->Invalidate();
}

/**
 * Sets the show flags to the default game show flags if checked.
 *
 * @param	In	wxWidget event generated if the GameView button is checked or unchecked. 
 */
void WxLevelViewportToolBar::OnGameView( wxCommandEvent& In )
{
	OnGameViewFlagsChanged(In.IsChecked());

	// Make sure menu/toolbar button state is in sync
	UpdateUI();
}

/**
 * Sets the show flags to the default game show flags if bChecked is TRUE.
 *
 * This makes the viewport display only items that would be seen in-game (such as no editor icons).
 *
 * @param	bChecked	If TRUE, sets the show flags to SHOW_DefaultGame; If FALSE, sets it to SHOW_DefaultEditor.
 */
void WxLevelViewportToolBar::OnGameViewFlagsChanged(UBOOL bChecked)
{
	// Save the view mode because in order to completely set the game view show flag, we 
	// need to wipe any existing flags. We don't want to wipe the view mode flags, though.
	EShowFlags CurrentViewFlags = ViewportClient->ShowFlags & SHOW_ViewMode_Mask;

	// If the Game View Show Flags are not checked, then we must revert back to the default editor flags.
	ViewportClient->ShowFlags = bChecked ? SHOW_DefaultGame : SHOW_DefaultEditor;

	// Now, we can re-add the view mode flags
	ViewportClient->ShowFlags |= CurrentViewFlags;

	ViewportClient->Invalidate();
}

/**
 * Determines if the show flags are set specifically to game view. 
 *
 * @return	UBOOL	TRUE if the show flags are set specifically to be the game view.
 */
UBOOL WxLevelViewportToolBar::AreGameViewFlagsSet() const
{
	// The game view is set only if all of the flags of SHOW_DefaultGame
	// are set on the current show flags, not just some of them. This is
	// why I check for equality instead of just the bitwise AND operation. 
	return ( SHOW_DefaultGame == (ViewportClient->ShowFlags & SHOW_DefaultGame) );
}

void WxLevelViewportToolBar::OnPerspective( wxCommandEvent& In )
{
	ViewportClient->ViewportType = LVT_Perspective;
	UpdateToolBarButtonEnabledStates();
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnTop( wxCommandEvent& In )
{
	ViewportClient->ViewportType = LVT_OrthoXY;
	UpdateToolBarButtonEnabledStates();
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnFront( wxCommandEvent& In )
{
	ViewportClient->ViewportType = LVT_OrthoYZ;
	UpdateToolBarButtonEnabledStates();
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnSide( wxCommandEvent& In )
{
	ViewportClient->ViewportType = LVT_OrthoXZ;
	UpdateToolBarButtonEnabledStates();
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnViewportTypeLeftClick( wxCommandEvent& In )
{
	// Get the resource ID of the current viewport type and increase it by 1.
	INT NewType = ResourceIDFromViewportType(ViewportClient->ViewportType) + 1;
	if ( NewType >= IDM_VIEWPORT_TYPE_END )
	{
		// Start over
		NewType = IDM_VIEWPORT_TYPE_START;
	}

	// Set the viewport type to the new type.
	ViewportClient->ViewportType = ViewportTypeFromResourceID( NewType );

	//Update the bitmap
	SetToolNormalBitmap( IDM_VIEWPORT_TYPE_CYCLE_BUTTON, ViewportTypesB[NewType - IDM_VIEWPORT_TYPE_START] );
	
	// update the window 
	ViewportClient->Invalidate();
}

void WxLevelViewportToolBar::OnViewportTypeRightClick( wxCommandEvent& In )
{
	wxMenu* RightClickMenu = new wxMenu;
	
	// Construct a right-click menu with the current viewport type already selected
	if ( RightClickMenu )
	{
		RightClickMenu->AppendCheckItem( IDM_PERSPECTIVE, *LocalizeUnrealEd( "Perspective" ) );
		RightClickMenu->AppendCheckItem( IDM_TOP, *LocalizeUnrealEd( "Top" ) );
		RightClickMenu->AppendCheckItem( IDM_FRONT, *LocalizeUnrealEd( "Front" ) );
		RightClickMenu->AppendCheckItem( IDM_SIDE, *LocalizeUnrealEd( "Side" ) );
		RightClickMenu->Check( ResourceIDFromViewportType(ViewportClient->ViewportType), TRUE );

		FTrackPopupMenu TrackPopUpMenu( this, RightClickMenu );
		TrackPopUpMenu.Show();
		delete RightClickMenu;
	}
}

void WxLevelViewportToolBar::OnTearOffNewFloatingViewport( wxCommandEvent& In )
{
	if ( GApp && GApp->EditorFrame && GApp->EditorFrame->ViewportConfigData )
	{
		FViewportConfig_Data *ViewportConfig = GApp->EditorFrame->ViewportConfigData;

		// The 'torn off copy' will have the same dimensions and settings as the original window
		FFloatingViewportParams ViewportParams;
		ViewportParams.ParentWxWindow = GApp->EditorFrame;
		ViewportParams.ViewportType = ViewportClient->ViewportType;
		ViewportParams.ShowFlags = ViewportClient->ShowFlags;
		ViewportParams.Width = ViewportClient->Viewport->GetSizeX();
		ViewportParams.Height = ViewportClient->Viewport->GetSizeY();


		// Create the new floating viewport
		INT NewViewportIndex = INDEX_NONE;
		UBOOL bResultValue = ViewportConfig->OpenNewFloatingViewport(ViewportParams, NewViewportIndex);

		if( bResultValue )
		{
			// OK, now copy various settings from our viewport into the newly created viewport
			FVCD_Viewport& NewViewport = ViewportConfig->AccessViewport( NewViewportIndex );
			WxLevelViewportWindow* NewViewportWin = NewViewport.ViewportWindow;
			if( NewViewportWin != NULL )
			{
				NewViewportWin->CopyLayoutFromViewport( *ViewportClient );
				NewViewportWin->Invalidate();
				NewViewportWin->ToolBar->UpdateUI();
			}
		}
		else
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd( "OpenNewFloatingViewport_Error" ) );
		}
	}
}



void WxLevelViewportToolBar::OnLockSelectedToCamera( wxCommandEvent& In )
{
	if ( GApp && GApp->EditorFrame && GApp->EditorFrame->ViewportConfigData )
	{
		FViewportConfig_Data *ViewportConfig = GApp->EditorFrame->ViewportConfigData;
		ViewportClient->bLockSelectedToCamera = In.IsChecked();

		if( !ViewportClient->bLockSelectedToCamera )
		{
			ViewportClient->ViewRotation.Pitch = 0;
			ViewportClient->ViewRotation.Roll = 0;
		}
	}
}

void WxLevelViewportToolBar::OnMakeParentViewport( wxCommandEvent& In )
{
	if ( GApp && GApp->EditorFrame && GApp->EditorFrame->ViewportConfigData )
	{
		// Allow only perspective views to be view parents
		const UBOOL bIsPerspectiveView = ViewportClient->IsPerspective();
		if ( bIsPerspectiveView )
		{
			FlushRenderingCommands();
			const UBOOL bIsViewParent = ViewportClient->ViewState->IsViewParent();

			// First, clear all existing view parent status.
			for( INT ViewportIndex = 0 ; ViewportIndex < GApp->EditorFrame->ViewportConfigData->GetViewportCount() ; ++ViewportIndex )
			{
				FVCD_Viewport& CurViewport = GApp->EditorFrame->ViewportConfigData->AccessViewport( ViewportIndex );
				if( CurViewport.bEnabled )
				{
					CurViewport.ViewportWindow->ViewState->SetViewParent( NULL );
					CurViewport.ViewportWindow->Invalidate();
				}
			}

			// If the view was not a parent, we're toggling occlusion parenting 'on' for this viewport.
			if ( !bIsViewParent )
			{
				for( INT ViewportIndex = 0 ; ViewportIndex < GApp->EditorFrame->ViewportConfigData->GetViewportCount() ; ++ViewportIndex )
				{
					FVCD_Viewport& CurViewport = GApp->EditorFrame->ViewportConfigData->AccessViewport( ViewportIndex );
					if( CurViewport.bEnabled )
					{
						CurViewport.ViewportWindow->ViewState->SetViewParent( ViewportClient->ViewState );
					}
				}
			}

			// Finally, update level viewport toolbar UI.
			for( INT ViewportIndex = 0 ; ViewportIndex < GApp->EditorFrame->ViewportConfigData->GetViewportCount() ; ++ViewportIndex )
			{
				FVCD_Viewport& CurViewport = GApp->EditorFrame->ViewportConfigData->AccessViewport( ViewportIndex );
				if( CurViewport.bEnabled )
				{
					CurViewport.ViewportWindow->ToolBar->UpdateUI();
				}
			}
		}
	}
}

void WxLevelViewportToolBar::OnSceneViewModeSelChange( wxCommandEvent& In )
{
	ViewportClient->ShowFlags &= ~SHOW_ViewMode_Mask;
	switch( In.GetInt() )
	{
		case 0: ViewportClient->ShowFlags |= SHOW_ViewMode_BrushWireframe; break;
		case 1: ViewportClient->ShowFlags |= SHOW_ViewMode_Wireframe; break;
		case 2: ViewportClient->ShowFlags |= SHOW_ViewMode_Unlit; break;
		default:
		case 3: ViewportClient->ShowFlags |= SHOW_ViewMode_Lit; break;
		case 4:	ViewportClient->ShowFlags |= SHOW_ViewMode_LightingOnly; break;
		case 5:	ViewportClient->ShowFlags |= SHOW_ViewMode_LightComplexity; break;
		case 6:	ViewportClient->ShowFlags |= SHOW_ViewMode_TextureDensity; break;
		case 7:	ViewportClient->ShowFlags |= SHOW_ViewMode_ShaderComplexity; break;
		case 8: ViewportClient->ShowFlags |= SHOW_ViewMode_LightMapDensity; break;
			break;
	}
	SetViewModeUI( In.GetInt() );
	ViewportClient->Invalidate();
}


/** for FShowFlagData */
enum EShowFlagGroup
{
	SFG_Normal,
	SFG_Advanced,
	SFG_PostProcess
};
class FShowFlagData
{
public:

	/** The wxWidgets resource ID for responding to events */
	INT				ID;
	/** The display name of this show flag (source for LocalizeUnrealEd) */
	FString			LocalizedName;
	/** The show flag mask */
	EShowFlags		Mask;
	/** Which group the flags should show up */
	EShowFlagGroup	Group;

	FShowFlagData(INT InID, const char *InUnLocalizedName, const EShowFlags& InMask, EShowFlagGroup InGroup = SFG_Normal)
		:	ID( InID )
		,	LocalizedName( *LocalizeUnrealEd(InUnLocalizedName) )
		,	Mask( InMask )
		,	Group( InGroup )
	{}
};

IMPLEMENT_COMPARE_CONSTREF( FShowFlagData, LevelViewportToolBar, { return appStricmp(*A.LocalizedName, *B.LocalizedName); } )


static TArray<FShowFlagData>& GetShowFlagMenuItems( )
{
	static TArray<FShowFlagData> OutShowFlags;

	static UBOOL bFirst = TRUE; 
	if(bFirst)
	{
		// do this only once
		bFirst = FALSE;

		INT Id = IDM_VIEWPORT_SHOWFLAGS_START;

		// Present the user with an alphabetical listing of available show flags.
		OutShowFlags.AddItem( FShowFlagData( Id++, "BSPSF", SHOW_BSP ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "BSPSplitSF", SHOW_BSPSplit, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "ActorTagsSF", SHOW_ActorTags, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "BoundsSF", SHOW_Bounds, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "AudioRadiusSF", SHOW_AudioRadius, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "BuilderBrushSF", SHOW_BuilderBrush ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "CameraFrustumsSF", SHOW_CamFrustums, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "CollisionSF", SHOW_Collision, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "DecalsSF", SHOW_Decals ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "DecalInfoSF", SHOW_DecalInfo, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "FogSF", SHOW_Fog, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "GridSF", SHOW_Grid ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "HitProxiesSF", SHOW_HitProxies, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "KismetReferencesSF", SHOW_KismetRefs, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "LargeVerticesSF", SHOW_LargeVertices, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "LensFlareSF", SHOW_LensFlares ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "LevelColorationSF", SHOW_LevelColoration, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "PropertyColorationSF", SHOW_PropertyColoration, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "LightInfluencesSF", SHOW_LightInfluences, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "LightRadiusSF", SHOW_LightRadius, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "LODSF", SHOW_LOD, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "MeshEdgesSF", SHOW_MeshEdges, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "ModeWidgetsSF", SHOW_ModeWidgets ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "NavigationNodesSF", SHOW_NavigationNodes, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "PathsSF", SHOW_Paths ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "ViewportShowFlagsMenu_VertexColorsSF", SHOW_VertexColors, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "PostProcessSF", SHOW_PostProcess, SFG_PostProcess ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "SelectionSF", SHOW_Selection, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "ShadowFrustumsSF", SHOW_ShadowFrustums, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "SpritesSF", SHOW_Sprites, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "SplinesSF", SHOW_Splines, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "ConstraintsSF", SHOW_Constraints, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "DynamicShadowsSF", SHOW_DynamicShadows, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "ParticlesSF", SHOW_Particles ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "StaticMeshesSF", SHOW_StaticMeshes ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "InstancedStaticMeshesSF", SHOW_InstancedStaticMeshes, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "SkeletalMeshesSF", SHOW_SkeletalMeshes ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "SpeedTreesSF", SHOW_SpeedTrees ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "TerrainSF", SHOW_Terrain ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "TerrainPatchesSF", SHOW_TerrainPatches, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "UnlitTranslucencySF", SHOW_UnlitTranslucency, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "VolumesSF", SHOW_Volumes ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "SceneCaptureUpdatesSF", SHOW_SceneCaptureUpdates, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "SentinelStatsSF", SHOW_SentinelStats, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "MotionBlurSF", SHOW_MotionBlur, SFG_PostProcess ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "ImageGrainSF", SHOW_ImageGrain, SFG_PostProcess ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "DepthOfFieldSF", SHOW_DepthOfField, SFG_PostProcess ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "ImageReflectionsSF", SHOW_ImageReflections, SFG_PostProcess ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "SubsurfaceScatteringSF", SHOW_SubsurfaceScattering, SFG_PostProcess ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "LightFunctionsSF", SHOW_LightFunctions, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "TesselationSF", SHOW_Tesselation, SFG_Advanced ) );
		OutShowFlags.AddItem( FShowFlagData( Id++, "VisualizeDOFLayersSF", SHOW_VisualizeDOFLayers, SFG_PostProcess ) );

		// Sort the show flags alphabetically by string.
		Sort<USE_COMPARE_CONSTREF(FShowFlagData,LevelViewportToolBar)>( OutShowFlags.GetTypedData(), OutShowFlags.Num() );

		check(Id < IDM_VIEWPORT_SHOWFLAGS_END);
	}

	return OutShowFlags;
}

/**
 * Displays the options menu for this viewport
 */
void WxLevelViewportToolBar::OnOptionsMenu( wxCommandEvent& In )
{
	wxMenu* OptionsMenu = new wxMenu();

	const TArray<FShowFlagData>& ShowFlags = GetShowFlagMenuItems();

	// Show flags menu
	wxMenu* ShowMenu = new wxMenu();
	{
		ShowMenu->Append(IDM_VIEWPORT_SHOW_DEFAULTS, *LocalizeUnrealEd("LevelViewportOptions_DefaultShowFlags"));

		ShowMenu->AppendSeparator();

		{
			wxMenu* CollisionMenu = new wxMenu();

			CollisionMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_COLLISION_ZEROEXTENT, *LocalizeUnrealEd("ZeroExtent") );
			CollisionMenu->Check(IDM_VIEWPORT_SHOW_COLLISION_ZEROEXTENT, (ViewportClient->ShowFlags & SHOW_CollisionZeroExtent) ? true : false);

			CollisionMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_COLLISION_NONZEROEXTENT, *LocalizeUnrealEd("NonZeroExtent") );
			CollisionMenu->Check(IDM_VIEWPORT_SHOW_COLLISION_NONZEROEXTENT, (ViewportClient->ShowFlags & SHOW_CollisionNonZeroExtent) ? true : false);

			CollisionMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_COLLISION_RIGIDBODY, *LocalizeUnrealEd("RigidBody") );
			CollisionMenu->Check(IDM_VIEWPORT_SHOW_COLLISION_RIGIDBODY, (ViewportClient->ShowFlags & SHOW_CollisionRigidBody) ? true : false);

			CollisionMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_COLLISION_NONE, *LocalizeUnrealEd("Normal") );
			CollisionMenu->Check(IDM_VIEWPORT_SHOW_COLLISION_NONE, !(ViewportClient->ShowFlags & SHOW_Collision_Any));

			ShowMenu->Append( IDM_VIEWPORT_SHOW_COLLISIONMENU, *LocalizeUnrealEd("CollisionModes"), CollisionMenu );
		}

		ShowMenu->AppendSeparator();

		wxMenu* AdvancedShowMenu = new wxMenu();
		wxMenu* PostProcessShowMenu = new wxMenu();

		for ( INT i = 0 ; i < ShowFlags.Num() ; ++i )
		{
			const FShowFlagData& ShowFlagData = ShowFlags(i);
			
			wxMenu* LocalMenu = ShowMenu;

			if( ShowFlagData.Group == SFG_Normal )
			{
				// is already set
			}
			else if( ShowFlagData.Group == SFG_Advanced )
			{
				LocalMenu = AdvancedShowMenu;
			}
			else if( ShowFlagData.Group == SFG_PostProcess )
			{
				LocalMenu = PostProcessShowMenu;
			}
			else
			{
				checkSlow(0);
			}

			LocalMenu->AppendCheckItem( ShowFlagData.ID, *ShowFlagData.LocalizedName );
			const UBOOL bShowFlagEnabled = (ViewportClient->ShowFlags & ShowFlagData.Mask) ? TRUE : FALSE;
			LocalMenu->Check( ShowFlagData.ID, bShowFlagEnabled == TRUE );
		}

		ShowMenu->AppendSeparator();
		ShowMenu->Append( IDM_VIEWPORT_SHOW_ADVANCED_MENU, *LocalizeUnrealEd("ShowMenu_AdvancedFlags"), AdvancedShowMenu );
		ShowMenu->Append( IDM_VIEWPORT_SHOW_POSTPROCESS_MENU, *LocalizeUnrealEd("ShowMenu_PostProcessFlags"), PostProcessShowMenu );
	}

	OptionsMenu->Append( wxID_ANY, *LocalizeUnrealEd( "ViewportOptionsMenu_Show" ), ShowMenu );

	// Show Volumes menu
	{
		wxMenu* VolumeMenu = new wxMenu();

		// Get sorted array of volume classes then create a menu item for each one
		TArray< UClass* > VolumeClasses;

		VolumeMenu->AppendCheckItem( IDM_VolumeActorVisibilityShowAll, *LocalizeUnrealEd( TEXT("ShowAll") ) );
		VolumeMenu->AppendCheckItem( IDM_VolumeActorVisibilityHideAll, *LocalizeUnrealEd( TEXT("HideAll") ) );

		VolumeMenu->AppendSeparator();

		GApp->EditorFrame->GetSortedVolumeClasses( &VolumeClasses );

		INT ID = IDM_VolumeClasses_START;

		// The index to insert menu items
		const INT MenuItemIdx = VolumeMenu->GetMenuItemCount();

		for( INT VolumeIdx = 0; VolumeIdx < VolumeClasses.Num(); VolumeIdx++ )
		{
			// Insert instead of append so that volume classes will be in A-Z order and not Z-A order
			VolumeMenu->InsertCheckItem( MenuItemIdx, ID, *VolumeClasses( VolumeIdx )->GetName() );
			// The menu item should be checked if the bit for this volume class is set
			VolumeMenu->Check( ID, ViewportClient->VolumeActorVisibility(VolumeIdx) != FALSE );
			++ID;
		}

		OptionsMenu->Append( wxID_ANY, *LocalizeUnrealEd( TEXT("VolumeActorVisibility") ), VolumeMenu );
	}

	// Per-group visibility
	{
		// get all the known groups
		TArray<FName> AllGroups;
		FGroupUtils::GetAllGroups(AllGroups);

		// Create a new menu and add the flags in alphabetical order.
		wxMenu* Menu = new wxMenu();

		// add the hide all/none items
		Menu->Append( IDM_PerViewGroups_HideAll, *LocalizeUnrealEd("PerViewGroupHideAll"));
		Menu->Append( IDM_PerViewGroups_HideNone, *LocalizeUnrealEd("PerViewGroupHideNone"));

		UBOOL bNeedSeparator = TRUE;

		INT MenuID = IDM_PerViewGroups_Start;
		// get all the groups
		for (INT GroupIndex = 0; GroupIndex < AllGroups.Num(); GroupIndex++, MenuID++)
		{
			if( bNeedSeparator )
			{
				Menu->AppendSeparator();
				bNeedSeparator = FALSE;
			}

			// add an item for ecah group
			Menu->AppendCheckItem( MenuID, *AllGroups(GroupIndex).ToString());

			// check it if it's visible
			Menu->Check( MenuID, (ViewportClient->ViewHiddenGroups.FindItemIndex(AllGroups(GroupIndex)) != INDEX_NONE));
		}

		OptionsMenu->Append( wxID_ANY, *LocalizeUnrealEd("PerViewGroupHiding"), Menu );

	}

	OptionsMenu->AppendSeparator();

	// Viewport type
	{
		wxMenu* ViewportTypeMenu = new wxMenu();
		ViewportTypeMenu->AppendCheckItem( IDM_PERSPECTIVE, *LocalizeUnrealEd( "Perspective" ) );
		ViewportTypeMenu->AppendCheckItem( IDM_TOP, *LocalizeUnrealEd( "Top" ) );
		ViewportTypeMenu->AppendCheckItem( IDM_FRONT, *LocalizeUnrealEd( "Front" ) );
		ViewportTypeMenu->AppendCheckItem( IDM_SIDE, *LocalizeUnrealEd( "Side" ) );

		OptionsMenu->Append( wxID_ANY, *LocalizeUnrealEd( "ViewportOptionsMenu_Type" ), ViewportTypeMenu );
	}

	// Realtime
	OptionsMenu->AppendCheckItem( IDM_REALTIME, *LocalizeUnrealEd( "ViewportOptionsMenu_ToggleRealTime" ) );

	OptionsMenu->AppendCheckItem( IDM_ViewportOptions_ShowFPS, *LocalizeUnrealEd( "ViewportOptions_ShowFPS" ), *LocalizeUnrealEd( "ViewportOptions_ShowFPS_ToolTip" ) );
	OptionsMenu->Check( IDM_ViewportOptions_ShowFPS, ViewportClient->ShouldShowFPS() == TRUE );

	OptionsMenu->AppendCheckItem( IDM_ViewportOptions_ShowStats, *LocalizeUnrealEd( "ViewportOptions_ShowStats" ), *LocalizeUnrealEd( "ViewportOptions_ShowStats_ToolTip" ) );
	OptionsMenu->Check( IDM_ViewportOptions_ShowStats, ViewportClient->ShouldShowStats() == TRUE );


	// Shading mode
	{
		OptionsMenu->AppendSeparator();

		OptionsMenu->AppendCheckItem( IDM_BRUSHWIREFRAME, *LocalizeUnrealEd("LevelViewportOptions_BrushWireframe") );
		OptionsMenu->AppendCheckItem( IDM_WIREFRAME, *LocalizeUnrealEd("LevelViewportOptions_Wireframe") );
		OptionsMenu->AppendCheckItem( IDM_UNLIT, *LocalizeUnrealEd("LevelViewportOptions_Unlit") );
		OptionsMenu->AppendCheckItem( IDM_LIT, *LocalizeUnrealEd("LevelViewportOptions_Lit") );
		OptionsMenu->AppendCheckItem( IDM_DETAILLIGHTING, *LocalizeUnrealEd("LevelViewportOptions_DetailLighting") );
		OptionsMenu->AppendCheckItem( IDM_LIGHTINGONLY, *LocalizeUnrealEd("LevelViewportOptions_LightingOnly") );
		OptionsMenu->AppendCheckItem( IDM_LIGHTCOMPLEXITY, *LocalizeUnrealEd("LevelViewportOptions_LightComplexity") );
		OptionsMenu->AppendCheckItem( IDM_TEXTUREDENSITY, *LocalizeUnrealEd("LevelViewportOptions_TextureDensity") );
		OptionsMenu->AppendCheckItem( IDM_SHADERCOMPLEXITY, *LocalizeUnrealEd("LevelViewportOptions_ShaderComplexity") );
		OptionsMenu->AppendCheckItem( IDM_LIGHTMAPDENSITY, *LocalizeUnrealEd("LevelViewportOptions_LightMapDensity") );
		OptionsMenu->AppendCheckItem( IDM_LITLIGHTMAPDENSITY, *LocalizeUnrealEd("LevelViewportOptions_LitLightmapDensity") );
		OptionsMenu->AppendCheckItem( IDM_REFLECTIONS, *LocalizeUnrealEd("LevelViewportOptions_Reflections") );

		OptionsMenu->AppendSeparator();
	}

	// Game View
	OptionsMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_GAMEVIEW, *LocalizeUnrealEd("ViewportOptionsMenu_ToggleGameView"));
	OptionsMenu->Check(IDM_VIEWPORT_SHOW_GAMEVIEW, AreGameViewFlagsSet() ? true : false );

	OptionsMenu->AppendSeparator();

	{
		OptionsMenu->AppendCheckItem( IDM_AllowMatineePreview, *LocalizeUnrealEd( "ViewportOptionsMenu_AllowMatineePreview" ) );
		OptionsMenu->AppendCheckItem( IDM_MOVEUNLIT, *LocalizeUnrealEd( "ViewportOptionsMenu_UnlitMovement" ) );

		OptionsMenu->AppendCheckItem( IDM_MakeOcclusionParent, *LocalizeUnrealEd( "ViewportOptions_MakeViewOcclusionParent" ), *LocalizeUnrealEd( "ViewportOptions_MakeViewOcclusionParent_ToolTip" ) );
	}

	// Display the options menu
	FTrackPopupMenu tpm( this, OptionsMenu );
	tpm.Show();
	delete OptionsMenu;
}

void WxLevelViewportToolBar::OnShowFlagsShortcut( wxCommandEvent& In )
{
	wxMenu* ShowMenu = new wxMenu();

	const TArray<FShowFlagData>& ShowFlags = GetShowFlagMenuItems();

	{
		ShowMenu->Append(IDM_VIEWPORT_SHOW_DEFAULTS, *LocalizeUnrealEd("LevelViewportOptions_DefaultShowFlags"));

		ShowMenu->AppendSeparator();

		{
			wxMenu* CollisionMenu = new wxMenu();

			CollisionMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_COLLISION_ZEROEXTENT, *LocalizeUnrealEd("ZeroExtent") );
			CollisionMenu->Check(IDM_VIEWPORT_SHOW_COLLISION_ZEROEXTENT, (ViewportClient->ShowFlags & SHOW_CollisionZeroExtent) ? true : false);

			CollisionMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_COLLISION_NONZEROEXTENT, *LocalizeUnrealEd("NonZeroExtent") );
			CollisionMenu->Check(IDM_VIEWPORT_SHOW_COLLISION_NONZEROEXTENT, (ViewportClient->ShowFlags & SHOW_CollisionNonZeroExtent) ? true : false);

			CollisionMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_COLLISION_RIGIDBODY, *LocalizeUnrealEd("RigidBody") );
			CollisionMenu->Check(IDM_VIEWPORT_SHOW_COLLISION_RIGIDBODY, (ViewportClient->ShowFlags & SHOW_CollisionRigidBody) ? true : false);

			CollisionMenu->AppendCheckItem(IDM_VIEWPORT_SHOW_COLLISION_NONE, *LocalizeUnrealEd("Normal") );
			CollisionMenu->Check(IDM_VIEWPORT_SHOW_COLLISION_NONE, !(ViewportClient->ShowFlags & SHOW_Collision_Any));

			ShowMenu->Append( IDM_VIEWPORT_SHOW_COLLISIONMENU, *LocalizeUnrealEd("CollisionModes"), CollisionMenu );
		}

		ShowMenu->AppendSeparator();

		for ( INT i = 0 ; i < ShowFlags.Num() ; ++i )
		{
			const FShowFlagData& ShowFlagData = ShowFlags(i);
			const UBOOL bShowFlagEnabled = (ViewportClient->ShowFlags & ShowFlagData.Mask) ? TRUE : FALSE;

			ShowMenu->AppendCheckItem( ShowFlagData.ID, *ShowFlagData.LocalizedName );
			ShowMenu->Check( ShowFlagData.ID, bShowFlagEnabled == TRUE );
		}
	}

	// Display the options menu
	FTrackPopupMenu tpm( this, ShowMenu );
	tpm.Show();
	delete ShowMenu;
}

void WxLevelViewportToolBar::OnShowDefaults( wxCommandEvent& In )
{
	// Setting show flags to the defaults should not stomp on the current viewmode settings.
	const EShowFlags ViewModeShowFlags = ViewportClient->ShowFlags &= SHOW_ViewMode_Mask;
	ViewportClient->ShowFlags = SHOW_DefaultEditor;
	ViewportClient->ShowFlags |= ViewModeShowFlags;

	UpdateUI();

	ViewportClient->Invalidate();
}

/**
 * Sets the show flag to SHOW_DefaultGame if the "Game View Show Flag" is checked. Otherwise, this function sets 
 * it to SHOW_DefaultEditor. In addition, it syncs the result to the "Game View" button on the toolbar.
 *
 * @param	In	wxWidget event the is called when the user toggles the "Game View Show Flag" option in the Show Flags menu.
 */
void WxLevelViewportToolBar::OnShowGameView( wxCommandEvent& In )
{
	OnGameViewFlagsChanged( In.IsChecked() );

	// We need to update the button the toolbar that associates to the game view 
	// flag. Otherwise, the button will be out of sync with the viewport.
	ToggleTool( IDM_VIEWPORT_SHOW_GAMEVIEW, AreGameViewFlagsSet() ? true : false );

}


/**
 * Toggles FPS display in real-time viewports
 *
 * @param	In	wxWidget event the is called when the user toggles the "Show FPS" option
 */
void WxLevelViewportToolBar::OnShowFPS( wxCommandEvent& In )
{
	ViewportClient->SetShowFPS( In.IsChecked() );

	if( In.IsChecked() )
	{
		// Also make sure that real-time mode is enabled
		ViewportClient->SetRealtime( In.IsChecked() );
		ViewportClient->Invalidate( FALSE );
	}
}




/**
 * Toggles stats display in real-time viewports
 *
 * @param	In	wxWidget event the is called when the user toggles the "Show Stats" option
 */
void WxLevelViewportToolBar::OnShowStats( wxCommandEvent& In )
{
	ViewportClient->SetShowStats( In.IsChecked() );

	if( In.IsChecked() )
	{
		// Also make sure that real-time mode is enabled
		ViewportClient->SetRealtime( In.IsChecked() );
		ViewportClient->Invalidate( FALSE );
	}
}

void WxLevelViewportToolBar::OnToggleShowFlag( wxCommandEvent& In )
{
	INT Id = In.GetId();

	// can be optimized
	const TArray<FShowFlagData>& ShowFlags = GetShowFlagMenuItems();

	for ( INT i = 0 ; i < ShowFlags.Num() ; ++i )
	{
		const FShowFlagData& ShowFlagData = ShowFlags(i);

		if(Id == ShowFlagData.ID)
		{
			ViewportClient->ShowFlags ^= ShowFlagData.Mask;
			ViewportClient->Invalidate();
			return;
		}
	}

	// GetShowFlagMenuItems doesn't know this control ID
	checkSlow(0);
}

/** Handle choosing one of the collision view mode options */
void WxLevelViewportToolBar::OnChangeCollisionMode(wxCommandEvent& In)
{
	INT Id = In.GetId();
	
	// Turn off all collision flags
	ViewportClient->ShowFlags &= ~SHOW_Collision_Any;

	// Then set the one we want
	if(Id == IDM_VIEWPORT_SHOW_COLLISION_ZEROEXTENT)
	{
		ViewportClient->ShowFlags |= SHOW_CollisionZeroExtent;
	}
	else if(Id == IDM_VIEWPORT_SHOW_COLLISION_NONZEROEXTENT)
	{
		ViewportClient->ShowFlags |= SHOW_CollisionNonZeroExtent;
	}
	else if(Id == IDM_VIEWPORT_SHOW_COLLISION_RIGIDBODY)
	{
		ViewportClient->ShowFlags |= SHOW_CollisionRigidBody;
	}

	ViewportClient->Invalidate();
}


void WxLevelViewportToolBar::OnPerViewGroupsHideAll( wxCommandEvent& In )
{
	// get all the known groups
	TArray<FName> AllGroups;
	FGroupUtils::GetAllGroups(AllGroups);

	// hide them all
	ViewportClient->ViewHiddenGroups = AllGroups;

	// update actor visiblity for this view
	FGroupUtils::UpdatePerViewVisibility(ViewportClient->ViewIndex);

	ViewportClient->Invalidate(); 
}

void WxLevelViewportToolBar::OnPerViewGroupsHideNone( wxCommandEvent& In )
{
	// clear all hidden groups
	ViewportClient->ViewHiddenGroups.Empty();
	
	// update actor visiblity for this view
	FGroupUtils::UpdatePerViewVisibility(ViewportClient->ViewIndex);

	ViewportClient->Invalidate(); 
}

void WxLevelViewportToolBar::OnTogglePerViewGroup( wxCommandEvent& In )
{
	// get all the known groups
	TArray<FName> AllGroups;
	FGroupUtils::GetAllGroups(AllGroups);

	// get the name of the chosen one (In.GetString() returns None or something)
	FName GroupName = AllGroups(In.GetId() - IDM_PerViewGroups_Start);

	INT HiddenIndex = ViewportClient->ViewHiddenGroups.FindItemIndex(GroupName);
	if (HiddenIndex == INDEX_NONE)
	{
		ViewportClient->ViewHiddenGroups.AddItem(GroupName);
	}
	else
	{
		ViewportClient->ViewHiddenGroups.Remove(HiddenIndex);
	}

	// update actor visibility for this view
	FGroupUtils::UpdatePerViewVisibility(ViewportClient->ViewIndex, GroupName);

	ViewportClient->Invalidate(); 
}

/**
 * Called in response to the camera speed button being left-clicked. Increments the camera speed setting by one, looping 
 * back to slow if clicked while on the fast setting.
 *
 * @param	In	Event generated by wxWidgets in response to the button left-click
 */
void WxLevelViewportToolBar::OnCamSpeedButtonLeftClick( wxCommandEvent& In )
{
	INT SpeedIdToSet = LastCameraSpeedID + 1;
	
	// Cycle the camera speed back to the start if it was already at its maximum setting
	if ( SpeedIdToSet >= ID_CAMSPEED_END )
	{
		SpeedIdToSet = ID_CAMSPEED_START;
	}
	
	// Generate an event to notify the parent window that the camera speed has been requested to change
	wxCommandEvent CamSpeedEvent( wxEVT_COMMAND_MENU_SELECTED, SpeedIdToSet );
	GetEventHandler()->ProcessEvent( CamSpeedEvent );
}

/**
 * Called in response to the camera speed button being right-clicked. Produces a pop-out menu for the user to select
 * their desired speed setting with.
 *
 * @param	In	Event generated by wxWidgets in response to the button right-click
 */
void WxLevelViewportToolBar::OnCamSpeedButtonRightClick( wxCommandEvent& In )
{
	wxMenu* RightClickMenu = new wxMenu;
	
	// Construct a right-click menu with the current camera speed already selected
	if ( RightClickMenu )
	{
		RightClickMenu->AppendCheckItem( ID_CAMSPEED_SLOW, *LocalizeUnrealEd( "ToolTip_28" ) );
		RightClickMenu->AppendCheckItem( ID_CAMSPEED_NORMAL, *LocalizeUnrealEd( "ToolTip_29" ) );
		RightClickMenu->AppendCheckItem( ID_CAMSPEED_FAST, *LocalizeUnrealEd( "ToolTip_30" ) );
		RightClickMenu->Check( LastCameraSpeedID, TRUE );

		FTrackPopupMenu TrackPopUpMenu( this, RightClickMenu );
		TrackPopUpMenu.Show();
		delete RightClickMenu;
	}
}

/**
 * Called in response to receiving a custom event type designed to update the UI button for the camera speed.
 *
 * @param	In	Event which contains information pertaining to the currently set camera speed so that the UI can update if necessary
 */
void WxLevelViewportToolBar::OnCamSpeedUpdateEvent( wxCommandEvent& In )
{
	INT InId = In.GetId();
	check( InId >= ID_CAMSPEED_START && InId < ID_CAMSPEED_END );

	// Ensure that the UI is displaying the correct icon for the currently selected camera speed
	if ( LastCameraSpeedID != InId )
	{
		LastCameraSpeedID = InId;
		SetToolNormalBitmap( ID_CAMSPEED_CYCLE_BUTTON, CamSpeedsB[LastCameraSpeedID - ID_CAMSPEED_START] );
	}
}

/** Called when a user selects a volume actor visibility menu item. **/
void WxLevelViewportToolBar::OnChangeVolumeVisibility( wxCommandEvent& In )
{
	INT VolumeID = In.GetId() - IDM_VolumeClasses_START;

	// Get a sorted list of volume classes.
	TArray< UClass *> VolumeClasses;
	GApp->EditorFrame->GetSortedVolumeClasses( &VolumeClasses );

	// Get the corresponding volume class for the clicked menu item.
	UClass *SelectedVolumeClass = VolumeClasses( VolumeID );
	// Toggle the flag corresponding to this actors ID.  The VolumeActorVisibility bitfield stores the visible/hidden state for all volume types
	ViewportClient->VolumeActorVisibility( VolumeID ) = !ViewportClient->VolumeActorVisibility( VolumeID );
	// Update the found actors visibility based on the new bitfield
	GUnrealEd->UpdateVolumeActorVisibility( SelectedVolumeClass, ViewportClient );
}

/** Called when a user selects show or hide all from the volume visibility menu. **/
void WxLevelViewportToolBar::OnToggleAllVolumeActors( wxCommandEvent& In )
{
	// Reinitialize the volume actor visibility flags to the new state.  All volumes should be visible if "Show All" was selected and hidden if it was not selected.
	const UBOOL NewState =  In.GetId() == IDM_VolumeActorVisibilityShowAll;
	ViewportClient->VolumeActorVisibility.Init( NewState, ViewportClient->VolumeActorVisibility.Num() );
	// Update visibility based on the new state
	// All volume actor types should be taken since the user clicked on show or hide all to get here
	GUnrealEd->UpdateVolumeActorVisibility( NULL, ViewportClient );
}

/**
 * Returns the level viewport toolbar height, in pixels.
 */
INT WxLevelViewportToolBar::GetToolbarHeight()
{
	wxSize ClientSize = GetClientSize();
	return ClientSize.GetHeight();
}

/*-----------------------------------------------------------------------------
	WxLevelViewportMaximizeToolBar
-----------------------------------------------------------------------------*/

BEGIN_EVENT_TABLE( WxLevelViewportMaximizeToolBar, WxToolBar )
	EVT_TOOL( IDM_MAXIMIZE_VIEWPORT, WxLevelViewportMaximizeToolBar::OnMaximizeViewport )
	EVT_UPDATE_UI( IDM_MAXIMIZE_VIEWPORT, WxLevelViewportMaximizeToolBar::UpdateMaximizeViewportUI )
END_EVENT_TABLE()

WxLevelViewportMaximizeToolBar::WxLevelViewportMaximizeToolBar( wxWindow* InParent, wxWindowID InID, FEditorLevelViewportClient* InViewportClient )
: WxToolBar( InParent, InID, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxTB_FLAT | wxTB_3DBUTTONS )
,	bIsMaximized( FALSE )
,	ViewportClient( InViewportClient )
{
	MaximizeB.Load( TEXT("LVT_Maximize") );
	RestoreB.Load( TEXT("LVT_Restore") );

	AddTool( IDM_MAXIMIZE_VIEWPORT, TEXT(""), MaximizeB, *LocalizeUnrealEd("Maximize_Viewport") );

	Realize();
}

/** Maximizes the level viewport */
void WxLevelViewportMaximizeToolBar::OnMaximizeViewport( wxCommandEvent& In )
{
	if ( GApp && GApp->EditorFrame && GApp->EditorFrame->ViewportConfigData )
	{
		FViewportConfig_Data *ViewportConfig = GApp->EditorFrame->ViewportConfigData;
		ViewportConfig->ToggleMaximize( ViewportClient->Viewport );
		ViewportClient->Invalidate();
	}
}


/** Updates UI state for the 'Maximize Viewport' toolbar button */
void WxLevelViewportMaximizeToolBar::UpdateMaximizeViewportUI( wxUpdateUIEvent& In )
{
	UBOOL bIsCurrentlyMaximized = FALSE;

	if ( GApp && GApp->EditorFrame && GApp->EditorFrame->ViewportConfigData )
	{
		if( GApp->EditorFrame->ViewportConfigData->IsViewportMaximized() )
		{
			const INT MaximizedViewportIndex = GApp->EditorFrame->ViewportConfigData->MaximizedViewport;
			if( MaximizedViewportIndex >= 0 && MaximizedViewportIndex < 4 )
			{
				if( GApp->EditorFrame->ViewportConfigData->Viewports[ MaximizedViewportIndex ].ViewportWindow == ViewportClient )
				{
					// This viewport is maximized, so check the button!
					bIsCurrentlyMaximized = TRUE;
				}
			}
		}
	}

	// if the viewport maximized state has changed, we need to change the maximize button's icon and tooltip
	if( bIsMaximized != bIsCurrentlyMaximized )
	{
		bIsMaximized = bIsCurrentlyMaximized;

		wxToolBarToolBase* MaximizeTool = FindById( IDM_MAXIMIZE_VIEWPORT );
		if( MaximizeTool )
		{
			wxBitmap* NewBitmap = bIsMaximized? &RestoreB: &MaximizeB;
			FString NewToolTip = bIsMaximized? LocalizeUnrealEd("Restore_Viewport"): LocalizeUnrealEd("Maximize_Viewport");
			INT PositionX = 0;
			INT PositionY = 0;
			INT Width = GetDialogOptionsWidth();
			INT Height = GetDialogOptionsHeight();

			GetPosition(&PositionX, &PositionY);

			MaximizeTool->SetNormalBitmap( *NewBitmap );
			MaximizeTool->SetShortHelp( *NewToolTip );
			Realize();
			// this is a bit of a hack, but calling Realize() resets the size and position of the toolbar
			Move(PositionX, PositionY);
			SetSize(Width, Height);
		}
	}
}

/** Returns the level viewport toolbar width, in pixels. */
INT WxLevelViewportMaximizeToolBar::GetDialogOptionsWidth()
{
	wxSize ClientSize = GetClientSize();
	return ClientSize.GetWidth();
}

/** Returns the level viewport toolbar height, in pixels. */
INT WxLevelViewportMaximizeToolBar::GetDialogOptionsHeight()
{
	wxSize ClientSize = GetClientSize();
	return ClientSize.GetHeight();
}
