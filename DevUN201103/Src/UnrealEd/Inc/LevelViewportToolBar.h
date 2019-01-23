/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __LEVELVIEWPORTTOOLBAR_H__
#define __LEVELVIEWPORTTOOLBAR_H__

struct FEditorLevelViewportClient;
class wxComboBox;
class wxCommandEvent;
class wxWindow;

class WxLevelViewportToolBar : public WxToolBar
{
public:
	WxLevelViewportToolBar(wxWindow* InParent, wxWindowID InID, FEditorLevelViewportClient* InViewportClient);

	/**Appends new Matinee Recording options to the standard toolbar*/
	void AppendMatineeRecordOptions(WxInterpEd* InInterpEd);
	/**returns TRUE if this toolbar was configured for matinee recording*/
	UBOOL IsShowingMatineeRecord(void) const { return MatineeWindow != NULL; }

	void UpdateUI();

	/**
	 * Returns the level viewport toolbar height, in pixels.
	 */
	INT GetToolbarHeight();

private:
	WxMaskedBitmap LockSelectedToCameraB;
	WxMaskedBitmap LockViewportB;
	WxMaskedBitmap RealTimeB;
	WxMaskedBitmap SquintButtonB;
	WxMaskedBitmap StreamingPrevisB;
	WxMaskedBitmap PostProcessPrevisB;
	WxMaskedBitmap ShowFlagsB;
	WxMaskedBitmap BrushWireframeB;
	WxMaskedBitmap WireframeB;
	WxMaskedBitmap UnlitB;
	WxMaskedBitmap LitB;
	WxMaskedBitmap DetailLightingB;
	WxMaskedBitmap LightingOnlyB;
	WxMaskedBitmap LightComplexityB;
	WxMaskedBitmap TextureDensityB;
	WxMaskedBitmap ShaderComplexityB;
	WxMaskedBitmap LightMapDensityB;
	WxMaskedBitmap LitLightmapDensityB;
	WxMaskedBitmap GameViewB;
	WxMaskedBitmap ViewportTypesB[IDM_VIEWPORT_TYPE_END - IDM_VIEWPORT_TYPE_START];
	WxMaskedBitmap TearOffNewFloatingViewportB;
	WxMaskedBitmap CamSpeedsB[ID_CAMSPEED_END - ID_CAMSPEED_START];
	WxBitmap PlayInViewportStartB;
	WxBitmap PlayInViewportStopB;

	/**Icons to place on the toolbar to start and stop custom track recording*/
	WxBitmap MatineeStartRecordB;
	WxBitmap MatineeCancelRecordB;

	WxBitmapStateButton* PlayInViewportButton;

	INT LastCameraSpeedID;

	FEditorLevelViewportClient* ViewportClient;

	void SetViewModeUI(INT ViewModeID);
	void UpdateToolBarButtonEnabledStates();

	void OnRealTime( wxCommandEvent& In );
	
	/** Updates UI state for the various viewport option menu items */
	void UpdateUI_ViewportOptionsMenu( wxUpdateUIEvent& In );

	void OnMoveUnlit( wxCommandEvent& In );

	void OnLevelStreamingVolumePreVis( wxCommandEvent& In );
	void OnPostProcessVolumePreVis( wxCommandEvent& In );
	void OnViewportLocked( wxCommandEvent& In );

	void OnBrushWireframe( wxCommandEvent& In );
	void OnWireframe( wxCommandEvent& In );
	void OnUnlit( wxCommandEvent& In );
	void OnLit( wxCommandEvent& In );
	void OnDetailLighting( wxCommandEvent& In );
	void OnLightingOnly( wxCommandEvent& In );
	void OnLightComplexity( wxCommandEvent& In );
	void OnTextureDensity( wxCommandEvent& In );
	void OnShaderComplexity( wxCommandEvent& In );
	void OnLightMapDensity( wxCommandEvent& In );
	void OnLitLightmapDensity( wxCommandEvent& In );
	void OnReflections( wxCommandEvent& In );

	/**
	 * Sets the show flags to the default game show flags if checked.
	 *
	 * @param	In	wxWidget event generated if the GameView button is checked or unchecked. 
	 */
	void OnGameView( wxCommandEvent& In );

	/**
	 * Sets the show flags to the default game show flags if bChecked is TRUE.
	 *
	 * This makes the viewport display only items that would be seen in-game (such as no editor icons).
	 *
	 * @param	bChecked	If TRUE, sets the show flags to SHOW_DefaultGame; If FALSE, sets it to SHOW_DefaultEditor.
	 */
	void OnGameViewFlagsChanged(UBOOL bIsChecked);

	/**
	 * Determines if the show flags are set specifically to game view. 
	 *
	 * @return	UBOOL	TRUE if the show flags are set specifically to be the game view.
	 */
	UBOOL AreGameViewFlagsSet() const;

	void OnPerspective( wxCommandEvent& In );
	void OnTop( wxCommandEvent& In );
	void OnFront( wxCommandEvent& In );
	void OnSide( wxCommandEvent& In );

	/** Called when you left click on the switch viewport type button.  Cycles the viewport modes*/
	void OnViewportTypeLeftClick( wxCommandEvent& In );

	/** Called when you right click on the switch viewport type button. Shows a menu allowing you to select a different type.*/
	void OnViewportTypeRightClick( wxCommandEvent& In );

	void OnLockSelectedToCamera( wxCommandEvent& In );
	void OnMakeParentViewport( wxCommandEvent& In );
	void OnTearOffNewFloatingViewport( wxCommandEvent& In );
	void OnToggleAllowMatineePreview( wxCommandEvent& In );

	/**Callback when toggling recording of matinee camera movement*/
	void OnToggleRecordInterpValues( wxCommandEvent& In );
	/**UI Adjustment for recording of matinee camera movement*/
	void UpdateUI_RecordInterpValues (wxUpdateUIEvent& In);

	/**Callback when changing the number of camera takes that will be used when sampling the camera for matinee*/
	void OnRecordModeChange ( wxCommandEvent& In );

	/** Called from the window event handler to launch Play-In-Editor for this viewport */
	void OnPlayInViewport( wxCommandEvent& In );

	/** Update the UI for the play in viewport button */
	void UpdateUI_OnPlayInViewport( wxUpdateUIEvent& In );

	void OnSceneViewModeSelChange( wxCommandEvent& In );
	
	/**
	 * Displays the options menu for this viewport
	 */
	void OnOptionsMenu( wxCommandEvent& In );
	void OnShowFlagsShortcut( wxCommandEvent& In );

	void OnShowDefaults(wxCommandEvent& In);
	void OnToggleShowFlag( wxCommandEvent& In );

	void OnChangeCollisionMode( wxCommandEvent& In );
	void OnSquintModeChange( wxCommandEvent& In );

	/**
	 * Sets the show flag to SHOW_DefaultGame if the "Game View Show Flag" is checked. Otherwise, this function sets 
	 * it to SHOW_DefaultEditor. In addition, it syncs the result to the "Game View" button on the toolbar.
	 *
	 * @param	In	wxWidget event the is called when the user toggles the "Game View Show Flag" option in the Show Flags menu.
	 */
	void OnShowGameView( wxCommandEvent& In );
	
	/**
	 * Toggles FPS display in real-time viewports
	 *
	 * @param	In	wxWidget event the is called when the user toggles the "Show FPS" option
	 */
	void OnShowFPS( wxCommandEvent& In );

	/**
	 * Toggles stats display in real-time viewports
	 *
	 * @param	In	wxWidget event the is called when the user toggles the "Show Stats" option
	 */
	void OnShowStats( wxCommandEvent& In );

	void OnPerViewGroups( wxCommandEvent& In );
	void OnPerViewGroupsHideAll( wxCommandEvent& In );
	void OnPerViewGroupsHideNone( wxCommandEvent& In );
	void OnTogglePerViewGroup( wxCommandEvent& In );

	/**
	 * Called in response to the camera speed button being left-clicked. Increments the camera speed setting by one, looping 
	 * back to slow if clicked while on the fast setting.
	 *
	 * @param	In	Event generated by wxWidgets in response to the button left-click
	 */
	void OnCamSpeedButtonLeftClick( wxCommandEvent& In );

	/**
	 * Called in response to the camera speed button being right-clicked. Produces a pop-out menu for the user to select
	 * their desired speed setting with.
	 *
	 * @param	In	Event generated by wxWidgets in response to the button right-click
	 */
	void OnCamSpeedButtonRightClick( wxCommandEvent& In );

	/**
	 * Called in response to receiving a custom event type designed to update the UI button for the camera speed.
	 *
	 * @param	In	Event which contains information pertaining to the currently set camera speed so that the UI can update if necessary
	 */
	void OnCamSpeedUpdateEvent( wxCommandEvent& In );


	/** Called when a user selects a volume actor visibility menu item. **/
	void OnChangeVolumeVisibility( wxCommandEvent& In );

	/** Called when a user selects show or hide all from the volume visibility menu. **/
	void OnToggleAllVolumeActors( wxCommandEvent& In );


	DECLARE_EVENT_TABLE()

	/**Matinee Editor for direct interfacing*/
	WxInterpEd* MatineeWindow;
};

/**
 * A specialized toolbar for the right-aligned viewport maximize button
 */
class WxLevelViewportMaximizeToolBar : public WxToolBar
{
public:
	WxLevelViewportMaximizeToolBar(wxWindow* InParent, wxWindowID InID, FEditorLevelViewportClient* InViewportClient);

	INT GetDialogOptionsWidth();
	INT GetDialogOptionsHeight();

private:
	UBOOL bIsMaximized;
	WxMaskedBitmap MaximizeB;
	WxMaskedBitmap RestoreB;
	
	FEditorLevelViewportClient* ViewportClient;

	/** Maximizes the level viewport */
	void OnMaximizeViewport( wxCommandEvent& In );

	/** Updates UI state for the 'Maximize Viewport' toolbar button */
	void UpdateMaximizeViewportUI( wxUpdateUIEvent& In );

	DECLARE_EVENT_TABLE()
};

#endif // __LEVELVIEWPORTTOOLBAR_H__
