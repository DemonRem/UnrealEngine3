/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __EDITORFRAME_H__
#define __EDITORFRAME_H__

class WxViewportsContainer;

class WxEditorFrame : public wxFrame, public FCallbackEventDevice
{
	// Used for dynamic creation of the window. This must be declared for any
	// subclasses of WxEditorFrame
	DECLARE_DYNAMIC_CLASS(WxEditorFrame);

private:
	class WxMainMenu* MainMenuBar;

protected:
	/** A cached version of the level filename to incorporate into the editor caption. */
	FString LevelFilename;

	/**
	 * Returns the localized caption for this frame window. It looks in the
	 * editor's INI file to determine which localization file to use
	 */
	virtual FString GetLocalizedCaption();

public:
	class WxButtonBar* ButtonBar;

	WxModeBar* ModeBar;

	/** Holds all open level editing viewports. */
	WxViewportsContainer* ViewportContainer;

	/** The status bars the editor uses. */
	WxStatusBar* StatusBars[ SB_Max ];

	/** Window position, size and maximized state, persisted in the .ini file */
	wxPoint	FramePos;
	wxSize	FrameSize;
	UBOOL	bFrameMaximized;

private:
	class WxMainToolBar* MainToolBar;

	///////////////////////////////////////////////////////////
	// Viewport configuration vars

public:
	/** All the possible viewport configurations. */
	TArray<FViewportConfig_Template*> ViewportConfigTemplates;

	/** The viewport configuration data currently in use. */
	FViewportConfig_Data* ViewportConfigData;

	/** Returns whether or not we should link the vertical splitters for the top and bottom viewports */
	UBOOL GetViewportsResizeTogether() const
	{
		return bViewportResizeTogether;
	}

private:
	/** Locks the vertical splitter for the top and bottom viewports together */
	UBOOL bViewportResizeTogether;


	// Common menus that are used in more than one place
	class WxDragGridMenu* DragGridMenu;
	class WxRotationGridMenu* RotationGridMenu;
	class WxAutoSaveIntervalMenu* AutoSaveIntervalMenu;
	class WxScaleGridMenu* ScaleGridMenu;

	TMap<INT,UOptionsProxy*>* OptionProxies;

public:
	// Bitmaps which are used all over the editor

	WxBitmap WhitePlaceholder, WizardB;
	WxMaskedBitmap DownArrowB, MaterialEditor_RGBAB, MaterialEditor_RB, MaterialEditor_GB, MaterialEditor_BB,
		MaterialEditor_AB, MaterialEditor_ControlPanelFillB, MaterialEditor_ControlPanelCapB,
		LeftHandle, RightHandle, RightHandleOn, ArrowUp, ArrowDown, ArrowRight;

	/**
	 * Default constructor. Construction is a 2 phase process: class creation
	 * followed by a call to Create(). This is required for dynamically determining
	 * which editor frame class to create for the editor's main frame.
	 */
	WxEditorFrame();

	/**
	 * Part 2 of the 2 phase creation process. First it loads the localized caption.
	 * Then it creates the window with that caption. And finally finishes the
	 * window initialization
	 */
	virtual void Create();

	~WxEditorFrame();

	void UpdateUI();
	virtual void SetUp();
	void SetStatusBar( EStatusBar InStatusBar );
	void SetViewportConfig( EViewportConfig InConfig );
	FGetInfoRet GetInfo( INT Item );

	/**
	* Puts all of the AVolume classes into the passed in array and sorts them by class name.
	*
	* @param	VolumeClasses		Array to populate with AVolume classes.
	*/
	void GetSortedVolumeClasses( TArray< UClass* >* VolumeClasses );

	// Option proxies.
	/**
	 * Clears out existing option proxies.  Must be called every level change so that option proxies
	 * can be set up for the new world.
	 */
	void ClearOptionProxies();

	/**
	 * Initializes option proxies with the standard classes.
	 */
	void OptionProxyInit();

	UOptionsProxy** FindOptionProxy(INT Key);

	/**
	 * @return	A pointer to the rotation grid menu.
	 */
	class WxRotationGridMenu* GetRotationGridMenu();

	/**
	* @return	A pointer to the autosave interval menu
	*/
	class WxAutoSaveIntervalMenu* GetAutoSaveIntervalMenu();

	/**
	 * @return	A pointer to the drag grid menu.
	 */
	class WxDragGridMenu* GetDragGridMenu();


	/**
	* @return	A pointer to the scale grid menu.
	*/
	class WxScaleGridMenu* GetScaleGridMenu();

	wxMenu* GetMRUMenu();
	FMRUList* GetMRUFiles();

	/**
	 * Process events the editor frame is registered to.
	 *
	 * @param	InEvent		The event that was fired.
	 */
	virtual void Send(ECallbackEventType InEvent);

	/**
	 * Updates the application's title bar caption.
	 *
	 * @param	LevelFilename		[opt] The level filename to use as editor caption.  Ignored if NULL.
	 */
	void RefreshCaption(const FFilename* InLevelFilename);

	//////////////////////////////////////////////////////////////
	// WxEvents

	void OnSize( wxSizeEvent& InEvent );
	void OnMove( wxMoveEvent& InEvent );
	void OnSplitterChanging( wxSplitterEvent& InEvent );
	void OnSplitterDblClk( wxSplitterEvent& InEvent );

private:
	/**
	 * Called when the app is trying to quit.
	 */
	void OnClose( wxCloseEvent& InEvent );

	/**
	 * Called when the application is minimized.
	 */
	void OnIconize( wxIconizeEvent& InEvent );

	/**
	* Called when the application is maximized.
	*/
	void OnMaximize( wxMaximizeEvent& InEvent );

	/**
	 * Restore's all minimized children.
	 */
	void RestoreAllChildren();

	void MenuFileNew( wxCommandEvent& In );
	void MenuFileOpen( wxCommandEvent& In );
	void MenuFileSave( wxCommandEvent& In );
	void MenuFileSaveAllLevels( wxCommandEvent& In );
	void MenuFileSaveAs( wxCommandEvent& In );
	void MenuFileSaveAll( wxCommandEvent& In );
	void MenuFileForceSaveAll( wxCommandEvent& In );
	void MenuFileImportNew( wxCommandEvent& In );
	void MenuFileImportMerge( wxCommandEvent& In );
	void MenuFileExportAll( wxCommandEvent& In );
	void MenuFileExportSelected( wxCommandEvent& In );
	void MenuFileCreateArchetype( wxCommandEvent& In );
	void MenuFileMRU( wxCommandEvent& In );
	void MenuFileExit( wxCommandEvent& In );
	void MenuEditUndo( wxCommandEvent& In );
	void MenuEditRedo( wxCommandEvent& In );
	void MenuFarPlaneScrollChanged( wxCommandEvent& In );
	void MenuEditMouseLock( wxCommandEvent& In );
	void MenuEditShowWidget( wxCommandEvent& In );
	void MenuEditTranslate( wxCommandEvent& In );
	void MenuEditRotate( wxCommandEvent& In );
	void MenuEditScale( wxCommandEvent& In );
	void MenuEditScaleNonUniform( wxCommandEvent& In );
	void CoordSystemSelChanged( wxCommandEvent& In );
	void MenuEditSearch( wxCommandEvent& In );
	void MenuEditCut( wxCommandEvent& In );
	void MenuEditCopy( wxCommandEvent& In );
	void MenuEditDuplicate( wxCommandEvent& In );
	void MenuEditDelete( wxCommandEvent& In );
	void MenuEditSelectNone( wxCommandEvent& In );
	void MenuEditSelectAll( wxCommandEvent& In );
	void MenuEditSelectByProperty( wxCommandEvent& In );
	void MenuEditSelectInvert( wxCommandEvent& In );
	void MenuViewFullScreen( wxCommandEvent& In );
	void MenuViewBrushPolys( wxCommandEvent& In );
	void MenuViewDistributionToggle( wxCommandEvent& In );
	void MenuViewCamSpeedSlow( wxCommandEvent& In );
	void MenuViewCamSpeedNormal( wxCommandEvent& In );
	void MenuViewCamSpeedFast( wxCommandEvent& In );
	void MenuEditPasteOriginalLocation( wxCommandEvent& In );
	void MenuEditPasteWorldOrigin( wxCommandEvent& In );
	void MenuEditPasteHere( wxCommandEvent& In );
	void MenuDragGrid( wxCommandEvent& In );
	void MenuRotationGrid( wxCommandEvent& In );
	void MenuAutoSaveInterval( wxCommandEvent& In );
	void MenuScaleGrid( wxCommandEvent& In );
	void MenuViewportConfig( wxCommandEvent& In );
	void MenuViewportResizeTogether( wxCommandEvent& In );
	void MenuViewShowBrowser( wxCommandEvent& In );
	void MenuActorProperties( wxCommandEvent& In );
	void MenuSyncGenericBrowser( wxCommandEvent& In );
	void MenuMakeSelectedActorsLevelCurrent( wxCommandEvent& In );
	void MenuMoveSelectedActorsToCurrentLevel( wxCommandEvent& In );
	void MenuSelectLevelInLevelBrowser( wxCommandEvent& In );
	void MenuSelectLevelOnlyInLevelBrowser( wxCommandEvent& In );
	void MenuDeselectLevelInLevelBrowser( wxCommandEvent& In );
	void MenuSurfaceProperties( wxCommandEvent& In );
	void MenuWorldProperties( wxCommandEvent& In );
	void MenuBrushCSG( wxCommandEvent& In );
	void MenuBrushAddSpecial( wxCommandEvent& In );
	void MenuBuildPlayInEditor( wxCommandEvent& In );
	void MenuBuildPlayOnConsole( wxCommandEvent& In );
	void MenuConsoleSpecific( wxCommandEvent& In );
	void UpdateUIConsoleSpecific( wxUpdateUIEvent& In );
	void MenuBuild( wxCommandEvent& In );
	void MenuRedrawAllViewports( wxCommandEvent& In );
	void MenuAlignWall( wxCommandEvent& In );
	void MenuToolCheckErrors( wxCommandEvent& In );
	void MenuReviewPaths( wxCommandEvent& In );
	void MenuRotateActors( wxCommandEvent& In );
	void MenuResetParticleEmitters( wxCommandEvent& In );
	void MenuSelectAllSurfs( wxCommandEvent& In );
	void MenuBrushAdd( wxCommandEvent& In );
	void MenuBrushSubtract( wxCommandEvent& In );
	void MenuBrushIntersect( wxCommandEvent& In );
	void MenuBrushDeintersect( wxCommandEvent& In );
	void MenuBrushOpen( wxCommandEvent& In );
	void MenuBrushSaveAs( wxCommandEvent& In );
	void MenuBrushImport( wxCommandEvent& In );
	void MenuBrushExport( wxCommandEvent& In );
	void MenuReplaceSkelMeshActors( wxCommandEvent& In );
	void MenuCleanBSPMaterials( wxCommandEvent& In );
	void MenuWizardNewTerrain( wxCommandEvent& In );	
	void MenuAboutBox( wxCommandEvent& In );	
	void MenuOnlineHelp( wxCommandEvent& In );
	void MenuTipOfTheDay( wxCommandEvent& In );
	void MenuBackdropPopupAddClassHere( wxCommandEvent& In );
	void MenuBackdropPopupAddLastSelectedClassHere( wxCommandEvent& In );
	void MenuSurfPopupApplyMaterial( wxCommandEvent& In );
	void MenuSurfPopupAlignPlanarAuto( wxCommandEvent& In );
	void MenuSurfPopupAlignPlanarWall( wxCommandEvent& In );
	void MenuSurfPopupAlignPlanarFloor( wxCommandEvent& In );
	void MenuSurfPopupAlignBox( wxCommandEvent& In );
	void MenuSurfPopupAlignFace( wxCommandEvent& In );
	void MenuSurfPopupUnalign( wxCommandEvent& In );
	void MenuSurfPopupSelectMatchingGroups( wxCommandEvent& In );
	void MenuSurfPopupSelectMatchingItems( wxCommandEvent& In );
	void MenuSurfPopupSelectMatchingBrush( wxCommandEvent& In );
	void MenuSurfPopupSelectMatchingTexture( wxCommandEvent& In );
	void MenuSurfPopupSelectAllAdjacents( wxCommandEvent& In );
	void MenuSurfPopupSelectAdjacentCoplanars( wxCommandEvent& In );
	void MenuSurfPopupSelectAdjacentWalls( wxCommandEvent& In );
	void MenuSurfPopupSelectAdjacentFloors( wxCommandEvent& In );
	void MenuSurfPopupSelectAdjacentSlants( wxCommandEvent& In );
	void MenuSurfPopupSelectReverse( wxCommandEvent& In );
	void MenuSurfPopupSelectMemorize( wxCommandEvent& In );
	void MenuSurfPopupRecall( wxCommandEvent& In );
	void MenuSurfPopupOr( wxCommandEvent& In );
	void MenuSurfPopupAnd( wxCommandEvent& In );
	void MenuSurfPopupXor( wxCommandEvent& In );
	void MenuActorPopup( wxCommandEvent& In );
	void MenuActorPopupCopy( wxCommandEvent& In );
	void MenuActorPopupPasteOriginal( wxCommandEvent& In );
	void MenuActorPopupPasteHere( wxCommandEvent& In );
	void MenuActorPopupPasteOrigin( wxCommandEvent& In );
	void MenuActorPopupSelectAllClass( wxCommandEvent& In );
	void MenuActorPopupSelectMatchingStaticMeshesThisClass( wxCommandEvent& In );
	void MenuActorPopupSelectMatchingStaticMeshesAllClasses( wxCommandEvent& In );	
	void MenuActorPopupSelectMatchingSpeedTrees( wxCommandEvent& In );
	void MenuActorPopupToggleDynamicChannel( wxCommandEvent& In );
	void MenuActorPopupSelectAllLights( wxCommandEvent& In );
	void MenuActorPopupSelectAllLightsWithSameClassification( wxCommandEvent& In );
	void MenuActorPopupSelectKismetUnreferenced( wxCommandEvent& In );
	void MenuActorPopupSelectKismetReferenced( wxCommandEvent& In );
	void MenuActorPopupAlignCameras( wxCommandEvent& In );
	void MenuActorPopupLockMovement( wxCommandEvent& In );
	void MenuActorPopupSnapViewToCamera( wxCommandEvent& In );
	void MenuActorPopupMerge( wxCommandEvent& In );
	void MenuActorPopupSeparate( wxCommandEvent& In );
	void MenuActorPopupToFirst( wxCommandEvent& In );
	void MenuActorPopupToLast( wxCommandEvent& In );
	void MenuActorPopupToBrush( wxCommandEvent& In );
	void MenuActorPopupFromBrush( wxCommandEvent& In );
	void MenuActorPopupMakeAdd( wxCommandEvent& In );
	void MenuActorPopupMakeSubtract( wxCommandEvent& In );
	void MenuActorSelectShow( wxCommandEvent& In );
	void MenuActorSelectHide( wxCommandEvent& In );
	void MenuActorSelectInvert( wxCommandEvent& In );
	void MenuTogglePrefabsLocked( wxCommandEvent& In );
	void MenuActorShowAll( wxCommandEvent& In );
	void MenuSnapToFloor( wxCommandEvent& In );
	void MenuMoveToGrid( wxCommandEvent& In );
	void MenuAlignToFloor( wxCommandEvent& In );
	void MenuSaveBrushAsCollision( wxCommandEvent& In );
	void MenuConvertActors( wxCommandEvent& In );
	void MenuSetLightDataBasedOnClassification( wxCommandEvent& In );
	void MenuQuantizeVertices( wxCommandEvent& In );
	void MenuConvertToStaticMesh( wxCommandEvent& In );
	void MenuOpenKismet( wxCommandEvent& In );
	void MenuActorFindInKismet( wxCommandEvent& In );
	void MenuActorPivotReset( wxCommandEvent& In );
	void MenuActorPivotMoveHere( wxCommandEvent& In );
	void MenuActorPivotMoveHereSnapped( wxCommandEvent& In );
	void MenuActorPivotMoveCenterOfSelection( wxCommandEvent& In );
	void MenuActorMirrorX( wxCommandEvent& In );
	void MenuActorMirrorY( wxCommandEvent& In );
	void MenuActorMirrorZ( wxCommandEvent& In );
	void OnAddVolumeClass( wxCommandEvent& In );
	void MenuCreateMaterialInstanceConstant( wxCommandEvent &In );
	void MenuCreateMaterialInstanceTimeVarying( wxCommandEvent &In );
	void MenuAssignMaterial( wxCommandEvent &In );
	void MenuActorPopupMakeSolid( wxCommandEvent& In );
	void MenuActorPopupMakeSemiSolid( wxCommandEvent& In );
	void MenuActorPopupMakeNonSolid( wxCommandEvent& In );
	void MenuActorPopupBrushSelectAdd( wxCommandEvent& In );
	void MenuActorPopupBrushSelectSubtract( wxCommandEvent& In );
	void MenuActorPopupBrushSelectSemiSolid( wxCommandEvent& In );
	void MenuActorPopupBrushSelectNonSolid( wxCommandEvent& In );

	void MenuActorPopupPathPosition( wxCommandEvent& In );
	void MenuActorPopupPathProscribe( wxCommandEvent& In );
	void MenuActorPopupPathForce( wxCommandEvent& In );
	void MenuActorPopupPathAssignNavPointsToRoute( wxCommandEvent& In );
	void MenuActorPopupPathAssignLinksToCoverGroup( wxCommandEvent& In );
	void MenuActorPopupPathClearProscribed( wxCommandEvent& In );
	void MenuActorPopupPathClearForced( wxCommandEvent& In );
	void MenuActorPopupPathStitchCover( wxCommandEvent& In );

	void MenuEmitterAutoPopulate(wxCommandEvent& In);
	void MenuEmitterReset(wxCommandEvent& In);

	void CreateArchetype(wxCommandEvent& In);
	void CreatePrefab(wxCommandEvent& In);
	void AddPrefab(wxCommandEvent& In);
	void SelectPrefabActors(wxCommandEvent& In);
	void UpdatePrefabFromInstance(wxCommandEvent& In);
	void ResetInstanceFromPrefab(wxCommandEvent& In);
	void PrefabInstanceToNormalActors(wxCommandEvent& In);
	void PrefabInstanceOpenSequence(wxCommandEvent& In);

	void MenuPlayFromHereInEditor( wxCommandEvent& In );
	void MenuPlayFromHereOnConsole( wxCommandEvent& In );

	void MenuUseActorFactory( wxCommandEvent& In );
	void MenuUseActorFactoryAdv( wxCommandEvent& In );

	void OnDockingChange( class WxDockEvent& In );

	void UI_MenuEditUndo( wxUpdateUIEvent& In );
	void UI_MenuEditRedo( wxUpdateUIEvent& In );
	void UI_MenuEditMouseLock( wxUpdateUIEvent& In );
	void UI_MenuEditShowWidget( wxUpdateUIEvent& In );
	void UI_MenuEditTranslate( wxUpdateUIEvent& In );
	void UI_MenuEditRotate( wxUpdateUIEvent& In );
	void UI_MenuEditScale( wxUpdateUIEvent& In );
	void UI_MenuEditScaleNonUniform( wxUpdateUIEvent& In );
	void UI_MenuViewportConfig( wxUpdateUIEvent& In );
	void UI_MenuDragGrid( wxUpdateUIEvent& In );
	void UI_MenuRotationGrid( wxUpdateUIEvent& In );
	void UI_MenuScaleGrid( wxUpdateUIEvent& In );
	void UI_MenuViewResizeViewportsTogether( wxUpdateUIEvent& In );
	void UI_MenuViewFullScreen( wxUpdateUIEvent& In );
	void UI_MenuViewBrushPolys( wxUpdateUIEvent& In );
	void UI_MenuTogglePrefabLock( wxUpdateUIEvent& In );
	void UI_MenuViewDistributionToggle( wxUpdateUIEvent& In );
	void UI_MenuViewCamSpeedSlow( wxUpdateUIEvent& In );
	void UI_MenuViewCamSpeedNormal( wxUpdateUIEvent& In );
	void UI_MenuViewCamSpeedFast( wxUpdateUIEvent& In );
	void UI_ContextMenuMakeCurrentLevel( wxUpdateUIEvent& In );

	void CoverEdit_ToggleEnabled( wxCommandEvent& In );
	void CoverEdit_ToggleType( wxCommandEvent& In );
	void CoverEdit_ToggleCoverslip( wxCommandEvent& In );
	void CoverEdit_ToggleSwatTurn( wxCommandEvent& In );
	void CoverEdit_ToggleMantle( wxCommandEvent& In );
	void CoverEdit_TogglePopup( wxCommandEvent& In );
	void CoverEdit_ToggleClimbUp( wxCommandEvent& In );

	void ObjectPropagationSelChanged( wxCommandEvent& In );
	void PushViewStartStop(wxCommandEvent& In);
	void PushViewSync(wxCommandEvent& In);

	DECLARE_EVENT_TABLE();
};

#endif // __EDITORFRAME_H__
