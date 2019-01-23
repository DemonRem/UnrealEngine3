/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __LEVELVIEWPORTTOOLBAR_H__
#define __LEVELVIEWPORTTOOLBAR_H__

struct FEditorLevelViewportClient;
class wxComboBox;
class wxCommandEvent;
class wxWindow;

class WxLevelViewportToolBar : public wxToolBar
{
public:
	WxLevelViewportToolBar(wxWindow* InParent, wxWindowID InID, FEditorLevelViewportClient* InViewportClient);

	void UpdateUI();

	/**
	 * Returns the level viewport toolbar height, in pixels.
	 */
	static INT GetToolbarHeight();

private:
	WxMaskedBitmap MakeViewOcclusionParentB;
	WxMaskedBitmap LockSelectedToCameraB;
	WxMaskedBitmap LockViewportB;
	WxMaskedBitmap RealTimeB;
	WxMaskedBitmap SquintButtonB;
	WxMaskedBitmap StreamingPrevisB;
	WxMaskedBitmap PostProcessPrevisB;
	WxMaskedBitmap UnlitMovementB;
	WxMaskedBitmap ShowFlagsB;
	FEditorLevelViewportClient* ViewportClient;

	typedef WxBitmap ViewModeBitmap;
	ViewModeBitmap BrushWireframeB;
	ViewModeBitmap WireframeB;
	ViewModeBitmap UnlitB;
	ViewModeBitmap LitB;
	ViewModeBitmap LightingOnlyB;
	ViewModeBitmap LightComplexityB;
	ViewModeBitmap TextureDensityB;
	ViewModeBitmap ShaderComplexityB;

	typedef WxMaskedBitmap ViewportBitmap;
	//typedef WxAlphaBitmap ViewportBitmap;
	ViewportBitmap PerspectiveB;
	ViewportBitmap TopB;
	ViewportBitmap FrontB;
	ViewportBitmap SideB;
	ViewportBitmap MaximizeB;

	void SetViewModeUI(INT ViewModeID);
	void SetViewportTypeUI(INT ViewportTypeID);

	void OnRealTime( wxCommandEvent& In );
	void OnMoveUnlit( wxCommandEvent& In );
	void OnLevelStreamingVolumePreVis( wxCommandEvent& In );
	void OnPostProcessVolumePreVis( wxCommandEvent& In );
	void OnViewportLocked( wxCommandEvent& In );

	void OnBrushWireframe( wxCommandEvent& In );
	void OnWireframe( wxCommandEvent& In );
	void OnUnlit( wxCommandEvent& In );
	void OnLit( wxCommandEvent& In );
	void OnLightingOnly( wxCommandEvent& In );
	void OnLightComplexity( wxCommandEvent& In );
	void OnTextureDensity( wxCommandEvent& In );
	void OnShaderComplexity( wxCommandEvent& In );

	void OnPerspective( wxCommandEvent& In );
	void OnTop( wxCommandEvent& In );
	void OnFront( wxCommandEvent& In );
	void OnSide( wxCommandEvent& In );
	void OnMaximizeViewport( wxCommandEvent& In );
	void OnLockSelectedToCamera( wxCommandEvent& In );
	void OnMakeParentViewport( wxCommandEvent& In );

	void OnViewportTypeSelChange( wxCommandEvent& In );
	void OnSceneViewModeSelChange( wxCommandEvent& In );
	void OnShowFlags( wxCommandEvent& In );
	void OnShowDefaults(wxCommandEvent& In);
	void OnShowStaticMeshes( wxCommandEvent& In );
	void OnShowSkeletalMeshes( wxCommandEvent& In );
	void OnShowSpeedTrees( wxCommandEvent& In );
	void OnShowTerrain( wxCommandEvent& In );
	void OnShowFoliage( wxCommandEvent& In );
	void OnShowBSP( wxCommandEvent& In );
	void OnShowBSPSplit( wxCommandEvent& In );
	void OnShowCollision( wxCommandEvent& In );
	void OnShowGrid( wxCommandEvent& In );
	void OnShowBounds( wxCommandEvent& In );
	void OnShowPaths( wxCommandEvent& In );
	void OnShowNavigationNodes( wxCommandEvent& In );
	void OnShowMeshEdges( wxCommandEvent& In );
	void OnShowModeWidgets( wxCommandEvent& In );
	void OnShowLargeVertices( wxCommandEvent& In );
	void OnShowZoneColors( wxCommandEvent& In );
	void OnShowPortals( wxCommandEvent& In );
	void OnShowHitProxies( wxCommandEvent& In );
	void OnShowShadowFrustums( wxCommandEvent& In );
	void OnShowKismetRefs( wxCommandEvent& In );
	void OnShowVolumes( wxCommandEvent& In );
	void OnShowFog( wxCommandEvent& In );
	void OnShowCamFrustums( wxCommandEvent& In );
	void OnShowParticles( wxCommandEvent& In );
	void OnShowLightInfluences( wxCommandEvent& In );
	void OnShowSelection( wxCommandEvent& In );
	void OnShowBuilderBrush( wxCommandEvent& In );
	void OnShowActorTags( wxCommandEvent& In );
	void OnShowMissingCollision( wxCommandEvent& In );
	void OnShowDecals( wxCommandEvent& In );
	void OnShowDecalInfo( wxCommandEvent& In );
	void OnShowLightRadius( wxCommandEvent& In );
	void OnShowAudioRadius( wxCommandEvent& In );
	void OnShowTerrainCollision( wxCommandEvent& In );
	void OnShowTerrainPatches( wxCommandEvent& In );
	void OnShowPostProcess( wxCommandEvent& In );
	void OnShowUnlitTranslucency( wxCommandEvent& In );
	void OnShowSceneCaptureUpdates( wxCommandEvent& In );
	void OnShowLevelColoration( wxCommandEvent& In );
	void OnShowPropertyColoration( wxCommandEvent& In );
	void OnShowSprites( wxCommandEvent& In );
	void OnShowConstraints( wxCommandEvent& In );
	void OnShowDynamicShadows( wxCommandEvent& In );
	void OnChangeCollisionMode( wxCommandEvent& In );
	void OnSquintModeChange( wxCommandEvent& In );
	void OnShowLensFlares( wxCommandEvent& In );

	DECLARE_EVENT_TABLE()
};

#endif // __LEVELVIEWPORTTOOLBAR_H__
