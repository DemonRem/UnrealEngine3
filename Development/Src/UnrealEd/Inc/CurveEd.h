/*=============================================================================
	CurveEd.h: FInterpCurve editor
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.

=============================================================================*/

#ifndef __CURVEED_H__
#define __CURVEED_H__

#include "UnrealEd.h"

// Forward declarations.
class WxCurveEdPresetDlg;
class WxCurveEditor;

struct HCurveEdLabelProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(HCurveEdLabelProxy,HHitProxy);

	INT CurveIndex;

	HCurveEdLabelProxy(INT InCurveIndex) :
		HHitProxy(HPP_UI),
		CurveIndex(InCurveIndex)
	{}
};

struct HCurveEdHideCurveProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(HCurveEdHideCurveProxy,HHitProxy);
	
	INT CurveIndex;

	HCurveEdHideCurveProxy(INT InCurveIndex) :
		HHitProxy(HPP_UI),
		CurveIndex(InCurveIndex)
	{}
};

struct HCurveEdKeyProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(HCurveEdKeyProxy,HHitProxy);

	INT CurveIndex;
	INT SubIndex;
	INT KeyIndex;

	HCurveEdKeyProxy(INT InCurveIndex, INT InSubIndex, INT InKeyIndex) :
		HHitProxy(HPP_UI),
		CurveIndex(InCurveIndex),
		SubIndex(InSubIndex),
		KeyIndex(InKeyIndex)
	{}
};

struct HCurveEdKeyHandleProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(HCurveEdKeyHandleProxy,HHitProxy);

	INT CurveIndex;
	INT SubIndex;
	INT KeyIndex;
	UBOOL bArriving;

	HCurveEdKeyHandleProxy(INT InCurveIndex, INT InSubIndex, INT InKeyIndex, UBOOL bInArriving) :
		HHitProxy(HPP_UI),
		CurveIndex(InCurveIndex),
		SubIndex(InSubIndex),
		KeyIndex(InKeyIndex),
		bArriving(bInArriving)
	{}
};

struct HCurveEdLineProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(HCurveEdLineProxy,HHitProxy);

	INT CurveIndex;
	INT SubIndex;

	HCurveEdLineProxy(INT InCurveIndex, INT InSubIndex) :
		HHitProxy(HPP_UI),
		CurveIndex(InCurveIndex),
		SubIndex(InSubIndex)
	{}
};

struct HCurveEdLabelBkgProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(HCurveEdLabelBkgProxy,HHitProxy);
	HCurveEdLabelBkgProxy(): HHitProxy(HPP_UI) {}
};

struct HCurveEdHideSubCurveProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(HCurveEdHideSubCurveProxy,HHitProxy);
	
	INT CurveIndex;
	INT SubCurveIndex;

	HCurveEdHideSubCurveProxy(INT InCurveIndex, INT InSubCurveIndex) :
		HHitProxy(HPP_UI),
		CurveIndex(InCurveIndex),
		SubCurveIndex(InSubCurveIndex)
	{}
};

/*-----------------------------------------------------------------------------
	FCurveEdViewportClient
-----------------------------------------------------------------------------*/

class FCurveEdViewportClient : public FEditorLevelViewportClient
{
public:

	WxCurveEditor* CurveEd;

	INT			OldMouseX, OldMouseY;
	UBOOL		bPanning;
	UBOOL		bZooming;
	UBOOL		bMouseDown;
	UBOOL		bDraggingHandle;
	UBOOL		bBegunMoving;
	UBOOL		bBoxSelecting;
	UBOOL		bKeyAdded;
	INT			DistanceDragged;

	INT			BoxStartX, BoxStartY;
	INT			BoxEndX, BoxEndY;

	FCurveEdViewportClient(WxCurveEditor* InCurveEd);
	~FCurveEdViewportClient();

	virtual void Draw(FViewport* Viewport,FCanvas* Canvas);

	virtual UBOOL InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed = 1.f,UBOOL bGamepad=FALSE);
	virtual void MouseMove(FViewport* Viewport, INT X, INT Y);
	virtual UBOOL InputAxis(FViewport* Viewport, INT ControllerId, FName Key, FLOAT Delta, FLOAT DeltaTime, UBOOL bGamepad=FALSE);

	virtual void Serialize(FArchive& Ar) { Ar << Input; }

	/** Exec handler */
	virtual void Exec(const TCHAR* Cmd);
};

/*-----------------------------------------------------------------------------
	WxCurveEditor
-----------------------------------------------------------------------------*/

struct FCurveEdSelKey
{
	INT		CurveIndex;
	INT		SubIndex;
	INT		KeyIndex;
	FLOAT	UnsnappedIn;
	FLOAT	UnsnappedOut;

	FCurveEdSelKey(INT InCurveIndex, INT InSubIndex, INT InKeyIndex)
	{
		CurveIndex = InCurveIndex;
		SubIndex = InSubIndex;
		KeyIndex = InKeyIndex;
	}

	UBOOL operator==(const FCurveEdSelKey& Other) const
	{
		if( CurveIndex == Other.CurveIndex &&
			SubIndex == Other.SubIndex &&
			KeyIndex == Other.KeyIndex )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

enum ECurveEdMode
{
	CEM_Pan,
	CEM_Zoom
};

class FCurveEdNotifyInterface
{
public:
	virtual void PreEditCurve(TArray<UObject*> CurvesAboutToChange) {}
	virtual void PostEditCurve() {}

	virtual void MovedKey() {}

	virtual void DesireUndo() {}
	virtual void DesireRedo() {}
};

class WxCurveEdPresetDlg;

class WxCurveEditor : public wxWindow
{
public:
	FCurveEdViewportClient*		CurveEdVC;

	UInterpCurveEdSetup*		EdSetup;

	UCurveEdOptions*			EditorOptions;

	FLOAT StartIn, EndIn, StartOut, EndOut;
	FLOAT CurveViewX, CurveViewY;
	FLOAT PixelsPerIn, PixelsPerOut;
	FLOAT MaxViewRange, MinViewRange;
	FLinearColor BackgroundColor;
	FLinearColor LabelColor;
	FLinearColor SelectedLabelColor;
	FLinearColor GridColor;
	FLinearColor GridTextColor;
	FLinearColor LabelBlockBkgColor;
	FLinearColor SelectedKeyColor;

	INT MouseOverCurveIndex;
	INT MouseOverSubIndex;
	INT MouseOverKeyIndex;

	TArray<FCurveEdSelKey>		SelectedKeys;

	INT							HandleCurveIndex;
	INT							HandleSubIndex;
	INT							HandleKeyIndex;
	UBOOL						bHandleArriving;

	class WxCurveEdToolBar*		ToolBar;
	INT							RightClickCurveIndex;

	ECurveEdMode				EdMode;

	UBOOL bShowPositionMarker;
	FLOAT MarkerPosition;
	FColor MarkerColor;

	UBOOL bShowEndMarker;
	FLOAT EndMarkerPosition;

	UBOOL bShowRegionMarker;
	FLOAT RegionStart;
	FLOAT RegionEnd;
	FColor RegionFillColor;

	UBOOL bSnapEnabled;
	FLOAT InSnapAmount;
	UBOOL bSnapToFrames;

	FCurveEdNotifyInterface*	NotifyObject;

	FIntPoint					LabelOrigin2D;
	wxScrollBar*				ScrollBar_Vert;
	INT							ThumbPos_Vert;
	INT							EntryCount;

	TArray<UClass*>						CurveEdPresets;

	UDistributionFloatConstantCurve*	FloatCC;
	UDistributionFloatUniformCurve*		FloatUC;
	UDistributionVectorConstantCurve*	VectorCC;
	UDistributionVectorUniformCurve*	VectorUC;
	UBOOL								bMinMaxValid;
	UBOOL								bFloatDist;
	FCurveEdInterface*					Distribution;
	UDistributionFloat*					FloatDist;
	UDistributionVector*				VectorDist;

	enum
	{
		CURVEED_MAX_CURVES	= 6
	};

	TArray<FPresetGeneratedPoint>		GeneratedPoints[CURVEED_MAX_CURVES];
	TArray<FLOAT>						RequiredKeyInTimes[CURVEED_MAX_CURVES];
	TArray<FLOAT>						CompleteKeyInTimes;
	TArray<FPresetGeneratedPoint>		CopiedCurves[CURVEED_MAX_CURVES];

	WxCurveEdPresetDlg*					PresetDialog;

public:
	WxCurveEditor( wxWindow* InParent, wxWindowID InID, class UInterpCurveEdSetup* InEdSetup );
	~WxCurveEditor();

	void CurveChanged();
	void UpdateDisplay();

	void SetPositionMarker(UBOOL bEnabled, FLOAT InPosition, const FColor& InMarkerColor);
	void SetEndMarker(UBOOL bEnabled, FLOAT InEndPosition);
	void SetRegionMarker(UBOOL bEnabled, FLOAT InRegionStart, FLOAT InRegionEnd, const FColor& InRegionFillColor);

	void SetInSnap(UBOOL bEnabled, FLOAT SnapAmount, UBOOL bInSnapToFrames);

	void SetNotifyObject(FCurveEdNotifyInterface* NewNotifyObject);

	void SetCurveSelected(UObject* InCurve, UBOOL bSelected);
	void ClearAllSelectedCurves();

	UBOOL	PresetDialog_OnOK();

	void ChangeInterpMode(EInterpCurveMode NewInterpMode=CIM_Unknown);

private:

	void OnSize( wxSizeEvent& In );
	void OnFitHorz( wxCommandEvent& In );
	void OnFitVert( wxCommandEvent& In );
	void OnContextCurveRemove( wxCommandEvent& In );
	void OnChangeMode( wxCommandEvent& In );
	void OnSetKey( wxCommandEvent& In );
	void OnSetKeyColor( wxCommandEvent& In );
	void OnChangeInterpMode( wxCommandEvent& In );
	void OnTabCreate(wxCommandEvent& In);
	void OnChangeTab(wxCommandEvent& In);
	void OnTabDelete(wxCommandEvent& In);
	void OnPresetCurves(wxCommandEvent& In);
	void OnSavePresetCurves(wxCommandEvent& In);
	void OnScroll(wxScrollEvent& In);
	void OnMouseWheel(wxMouseEvent& In);

	void UpdateScrollBar(INT Vert);
	void AdjustScrollBar(wxRect& rcClient);
	void FillTabCombo();

	// --
	INT		DetermineValidPresetFlags(FCurveEdEntry* Entry);
	UBOOL	GeneratePresetClassList(UBOOL bIsSaving = FALSE);
	UBOOL	SetupDistributionVariables(FCurveEdEntry* Entry);
	UBOOL	GetSetupDataAndRequiredKeyInPoints(WxCurveEdPresetDlg* PresetDlg);
	UBOOL	GenerateCompleteKeyInList();
	UBOOL	GeneratePresetSamples(WxCurveEdPresetDlg* PresetDlg);
	INT		DetermineSubCurveIndex(INT CurveIndex);

	// --

	INT AddNewKeypoint( INT InCurveIndex, INT InSubIndex, const FIntPoint& ScreenPos );
	void SetCurveView(FLOAT StartIn, FLOAT EndIn, FLOAT StartOut, FLOAT EndOut);
	void MoveSelectedKeys( FLOAT DeltaIn, FLOAT DeltaOut );
	void MoveCurveHandle(const FVector2D& NewHandleVal);

	void AddKeyToSelection(INT InCurveIndex, INT InSubIndex, INT InKeyIndex);
	void RemoveKeyFromSelection(INT InCurveIndex, INT InSubIndex, INT InKeyIndex);
	void ClearKeySelection();
	UBOOL KeyIsInSelection(INT InCurveIndex, INT InSubIndex, INT InKeyIndex);

	FLOAT SnapIn(FLOAT InValue);

	void BeginMoveSelectedKeys();
	void EndMoveSelectedKeys();

	void DoubleClickedKey(INT InCurveIndex, INT InSubIndex, INT InKeyIndex);

	void ToggleCurveHidden(INT InCurveIndex);
	void ToggleSubCurveHidden(INT InCurveIndex, INT InSubCurveIndex);
	
	void UpdateInterpModeButtons();

	void DeleteSelectedKeys();

	FIntPoint CalcScreenPos(const FVector2D& Val);
	FVector2D CalcValuePoint(const FIntPoint& Pos);

	void DrawCurveEditor(FViewport* Viewport, FCanvas* Canvas);
	void DrawEntry(FViewport* Viewport, FCanvas* Canvas, const FCurveEdEntry& Entry, INT CurveIndex);
	void DrawGrid(FViewport* Viewport, FCanvas* Canvas);

	friend class FCurveEdViewportClient;
	friend class WxMBCurveKeyMenu;

	DECLARE_EVENT_TABLE()
};

/*-----------------------------------------------------------------------------
	WxCurveEdToolBar
-----------------------------------------------------------------------------*/

class WxCurveEdToolBar : public wxToolBar
{
public:
	WxCurveEdToolBar( wxWindow* InParent, wxWindowID InID );
	~WxCurveEdToolBar();

	void SetCurveMode(EInterpCurveMode NewMode);
	void SetButtonsEnabled(UBOOL bEnabled);
	void SetEditMode(ECurveEdMode NewMode);

	WxMaskedBitmap FitHorzB, FitVertB, PanB, ZoomB, ModeAutoB, ModeUserB, ModeBreakB, ModeLinearB, ModeConstantB;
	WxMaskedBitmap TabCreateB, TabDeleteB;
	wxComboBox* TabCombo;

	DECLARE_EVENT_TABLE()
};

/*-----------------------------------------------------------------------------
	WxMBCurveLabelMenu
-----------------------------------------------------------------------------*/

class WxMBCurveLabelMenu : public wxMenu
{
public:
	WxMBCurveLabelMenu(WxCurveEditor* CurveEd);
	~WxMBCurveLabelMenu();
};


/*-----------------------------------------------------------------------------
	WxMBCurveKeyMenu
-----------------------------------------------------------------------------*/

class WxMBCurveKeyMenu : public wxMenu
{
public:
	WxMBCurveKeyMenu(WxCurveEditor* CurveEd);
	~WxMBCurveKeyMenu();
};

#endif // __CURVEED_H__
