/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __INTERPEDITOR_H__
#define __INTERPEDITOR_H__

#include "UnrealEd.h"
#include "CurveEd.h"

/*-----------------------------------------------------------------------------
	Editor-specific hit proxies.
-----------------------------------------------------------------------------*/

struct HInterpEdTrackBkg : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdTrackBkg,HHitProxy);
	HInterpEdTrackBkg(): HHitProxy(HPP_UI) {}
};

struct HInterpEdGroupTitle : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdGroupTitle,HHitProxy);

	class UInterpGroup* Group;

	HInterpEdGroupTitle(class UInterpGroup* InGroup) :
		HHitProxy(HPP_UI),
		Group(InGroup)
	{}
};

struct HInterpEdGroupCollapseBtn : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdGroupCollapseBtn,HHitProxy);

	class UInterpGroup* Group;

	HInterpEdGroupCollapseBtn(class UInterpGroup* InGroup) :
		HHitProxy(HPP_UI),
		Group(InGroup)
	{}
};

struct HInterpEdGroupLockCamBtn : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdGroupLockCamBtn,HHitProxy);

	class UInterpGroup* Group;

	HInterpEdGroupLockCamBtn(class UInterpGroup* InGroup) :
		HHitProxy(HPP_UI),
		Group(InGroup)
	{}
};

struct HInterpEdTrackTitle : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdTrackTitle,HHitProxy);

	class UInterpGroup* Group;
	INT TrackIndex;

	HInterpEdTrackTitle(class UInterpGroup* InGroup, INT InTrackIndex) :
		HHitProxy(HPP_UI),
		Group(InGroup),
		TrackIndex(InTrackIndex)
	{}
};

struct HInterpEdTrackGraphPropBtn : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdTrackGraphPropBtn,HHitProxy);

	class UInterpGroup* Group;
	INT TrackIndex;

	HInterpEdTrackGraphPropBtn(class UInterpGroup* InGroup, INT InTrackIndex) :
		HHitProxy(HPP_UI),
		Group(InGroup),
		TrackIndex(InTrackIndex)
	{}
};

struct HInterpEdTrackDisableTrackBtn : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdTrackDisableTrackBtn,HHitProxy);

	class UInterpGroup* Group;
	INT TrackIndex;

	HInterpEdTrackDisableTrackBtn(class UInterpGroup* InGroup, INT InTrackIndex) :
		HHitProxy(HPP_UI),
		Group(InGroup),
		TrackIndex(InTrackIndex)
	{}
};

enum EInterpEdEventDirection
{
	IED_Forward,
	IED_Backward
};

struct HInterpEdEventDirBtn : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdEventDirBtn,HHitProxy);

	class UInterpGroup* Group;
	INT TrackIndex;
	EInterpEdEventDirection Dir;

	HInterpEdEventDirBtn(class UInterpGroup* InGroup, INT InTrackIndex, EInterpEdEventDirection InDir) :
		HHitProxy(HPP_UI),
		Group(InGroup),
		TrackIndex(InTrackIndex),
		Dir(InDir)
	{}
};

struct HInterpEdTimelineBkg : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdTimelineBkg,HHitProxy);
	HInterpEdTimelineBkg(): HHitProxy(HPP_UI) {}
};

struct HInterpEdNavigator : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdNavigator,HHitProxy);
	HInterpEdNavigator(): HHitProxy(HPP_UI) {}
};

enum EInterpEdMarkerType
{
	ISM_SeqStart,
	ISM_SeqEnd,
	ISM_LoopStart,
	ISM_LoopEnd
};

struct HInterpEdMarker : public HHitProxy
{
	DECLARE_HIT_PROXY(HInterpEdMarker,HHitProxy);

	EInterpEdMarkerType Type;

	HInterpEdMarker(EInterpEdMarkerType InType) :
		HHitProxy(HPP_UI),
		Type(InType)
	{}
};

/** Hitproxy for when the user clicks on a filter tab in matinee. */
struct HInterpEdTab : public HHitProxy
{
	/** Pointer to the interp filter for this hit proxy. */
	class UInterpFilter* Filter;

	DECLARE_HIT_PROXY(HInterpEdTab,HHitProxy);
	HInterpEdTab(UInterpFilter* InFilter): HHitProxy(HPP_UI), Filter(InFilter) {}
};

/*-----------------------------------------------------------------------------
	UInterpEdTransBuffer / FInterpEdTransaction
-----------------------------------------------------------------------------*/

class UInterpEdTransBuffer : public UTransBuffer
{
	DECLARE_CLASS(UInterpEdTransBuffer,UTransBuffer,CLASS_Transient,UnrealEd)
	NO_DEFAULT_CONSTRUCTOR(UInterpEdTransBuffer)
public:

	UInterpEdTransBuffer(SIZE_T InMaxMemory)
		:	UTransBuffer( InMaxMemory )
	{}

	/**
	 * Begins a new undo transaction.  An undo transaction is defined as all actions
	 * which take place when the user selects "undo" a single time.
	 * If there is already an active transaction in progress, increments that transaction's
	 * action counter instead of beginning a new transaction.
	 * 
	 * @param	SessionName		the name for the undo session;  this is the text that 
	 *							will appear in the "Edit" menu next to the Undo item
	 *
	 * @return	Number of active actions when Begin() was called;  values greater than
	 *			0 indicate that there was already an existing undo transaction in progress.
	 */
	virtual INT Begin(const TCHAR* SessionName)
	{
		return 0;
	}

	/**
	 * Attempts to close an undo transaction.  Only successful if the transaction's action
	 * counter is 1.
	 * 
	 * @return	Number of active actions when End() was called; a value of 1 indicates that the
	 *			transaction was successfully closed
	 */
	virtual INT End()
	{
		return 1;
	}

	/**
	 * Cancels the current transaction, no longer capture actions to be placed in the undo buffer.
	 *
	 * @param	StartIndex	the value of ActiveIndex when the transaction to be cancelled was began. 
	 */
	virtual void Cancel(INT StartIndex = 0)
	{}

	virtual void BeginSpecial(const TCHAR* SessionName);
	virtual void EndSpecial();
};

class FInterpEdTransaction : public FTransaction
{
public:
	FInterpEdTransaction( const TCHAR* InTitle=NULL, UBOOL InFlip=0 )
	:	FTransaction(InTitle, InFlip)
	{}

	virtual void SaveObject( UObject* Object );
	virtual void SaveArray( UObject* Object, FArray* Array, INT Index, INT Count, INT Oper, INT ElementSize, STRUCT_AR Serializer, STRUCT_DTOR Destructor );
};

/*-----------------------------------------------------------------------------
	FInterpEdViewportClient
-----------------------------------------------------------------------------*/

class FInterpEdViewportClient : public FEditorLevelViewportClient
{
public:
	FInterpEdViewportClient( class WxInterpEd* InInterpEd );
	~FInterpEdViewportClient();

	void DrawTimeline(FViewport* Viewport,FCanvas* Canvas);
	void DrawMarkers(FViewport* Viewport,FCanvas* Canvas);
	void DrawGrid(FViewport* Viewport,FCanvas* Canvas, UBOOL bDrawTimeline);
	void DrawTabs(FViewport* Viewport,FCanvas* Canvas);
	FVector2D DrawTab(FViewport* Viewport, FCanvas* Canvas, INT &TabOffset, UInterpFilter* Filter);
	
	virtual void Draw(FViewport* Viewport,FCanvas* Canvas);

	virtual UBOOL InputKey(FViewport* Viewport,INT ControllerId,FName Key,EInputEvent Event,FLOAT AmountDepressed = 1.f,UBOOL bGamepad=FALSE);
	virtual void MouseMove(FViewport* Viewport, INT X, INT Y);
	virtual UBOOL InputAxis(FViewport* Viewport, INT ControllerId, FName Key, FLOAT Delta, FLOAT DeltaTime, UBOOL bGamepad=FALSE);
	virtual EMouseCursor GetCursor(FViewport* Viewport,INT X,INT Y);

	virtual void Tick(FLOAT DeltaSeconds);

	virtual void Serialize(FArchive& Ar);

	/** Exec handler */
	virtual void Exec(const TCHAR* Cmd);

	INT OldMouseX, OldMouseY;
	INT BoxStartX, BoxStartY;
	INT BoxEndX, BoxEndY;
	INT DistanceDragged;

	/** Used to accumulate velocity for autoscrolling when the user is dragging items near the edge of the viewport. */
	FVector2D ScrollAccum;

	class WxInterpEd* InterpEd;

	EInterpEdMarkerType	GrabbedMarkerType;

	FLOAT	ViewStartTime;
	FLOAT	ViewEndTime;

	/** The object and data we are currently dragging, if NULL we are not dragging any objects. */
	FInterpEdInputData DragData;
	FInterpEdInputInterface* DragObject;

	// Used to convert between seconds and size on the timeline
	INT		TrackViewSizeX;
	FLOAT	PixelsPerSec;
	FLOAT	NavPixelsPerSecond;

	FLOAT	UnsnappedMarkerPos;

	UBOOL	bDrawSnappingLine;
	FLOAT	SnappingLinePosition;

	UBOOL	bPanning;
	UBOOL   bMouseDown;
	UBOOL	bGrabbingHandle;
	UBOOL	bNavigating;
	UBOOL	bBoxSelecting;
	UBOOL	bTransactionBegun;
	UBOOL	bGrabbingMarker;
};

/*-----------------------------------------------------------------------------
	WxInterpEdVCHolder
-----------------------------------------------------------------------------*/

class WxInterpEdVCHolder : public wxWindow
{
public:
	WxInterpEdVCHolder( wxWindow* InParent, wxWindowID InID, class WxInterpEd* InInterpEd );
	virtual ~WxInterpEdVCHolder();

	/**
	 * Destroys the viewport held by this viewport holder, disassociating it from the engine, etc.  Rentrant.
	 */
	void DestroyViewport();

	wxScrollBar*	ScrollBar_Vert;

	void AdjustScrollBar(wxRect& rcClient);
	void OnSize( wxSizeEvent& In );

	FInterpEdViewportClient* InterpEdVC;

	DECLARE_EVENT_TABLE()
};


/*-----------------------------------------------------------------------------
	WxInterpEdToolBar
-----------------------------------------------------------------------------*/

class WxInterpEdToolBar : public wxToolBar
{
public:
	WxMaskedBitmap AddB, PlayB, LoopSectionB, StopB, UndoB, RedoB, CurveEdB, SnapB, FitSequenceB, FitLoopB, FitLoopSequenceB;
	WxMaskedBitmap Speed1B, Speed10B, Speed25B, Speed50B, Speed100B;
	wxComboBox* SnapCombo;

	WxInterpEdToolBar( wxWindow* InParent, wxWindowID InID );
	~WxInterpEdToolBar();
};

/*-----------------------------------------------------------------------------
	WxInterpEdMenuBar
-----------------------------------------------------------------------------*/

class WxInterpEdMenuBar : public wxMenuBar
{
public:
	wxMenu	*FileMenu, *EditMenu, *ViewMenu;

	WxInterpEdMenuBar(WxInterpEd* InEditor);
	~WxInterpEdMenuBar();
};

/*-----------------------------------------------------------------------------
	WxInterpEd
-----------------------------------------------------------------------------*/

static const FLOAT InterpEdSnapSizes[5] = {0.01f, 0.05f, 0.1f, 0.5f, 1.0f};
static const INT InterpEdFPSSnapSizes[5] = {15, 24, 30, 45, 60};

class WxInterpEd : public wxFrame, public FNotifyHook, public FSerializableObject, public FCurveEdNotifyInterface, public FDockingParent
{
public:
	WxInterpEd( wxWindow* InParent, wxWindowID InID, class USeqAct_Interp* InInterp  );
	virtual	~WxInterpEd();

	void OnSize( wxSizeEvent& In );
	virtual void OnClose( wxCloseEvent& In );

	// FNotify interface

	void NotifyDestroy( void* Src );
	void NotifyPreChange( void* Src, UProperty* PropertyAboutToChange );
	void NotifyPostChange( void* Src, UProperty* PropertyThatChanged );
	void NotifyExec( void* Src, const TCHAR* Cmd );

	// FCurveEdNotifyInterface
	virtual void PreEditCurve(TArray<UObject*> CurvesAboutToChange);
	virtual void PostEditCurve();
	virtual void MovedKey();
	virtual void DesireUndo();
	virtual void DesireRedo();

	// FSerializableObject
	void Serialize(FArchive& Ar);

	// Play controls

	/** 
	 * Starts playing the current sequence. 
	 * @param bPlayLoop		Whether or not we should play the looping section.
	 */
	void StartPlaying(UBOOL bPlayLoop);	
	void StopPlaying();					/** Stops playing the current sequence. */

	// Menu handlers
	void OnScroll(wxScrollEvent& In);
	void OnMenuAddKey( wxCommandEvent& In );
	void OnMenuPlay( wxCommandEvent& In );
	void OnMenuStop( wxCommandEvent& In );
	void OnChangePlaySpeed( wxCommandEvent& In );
	void OnMenuInsertSpace( wxCommandEvent& In );
	void OnMenuStretchSection( wxCommandEvent& In );
	void OnMenuDeleteSection( wxCommandEvent& In );
	void OnMenuSelectInSection( wxCommandEvent& In );
	void OnMenuDuplicateSelectedKeys( wxCommandEvent& In );
	void OnSavePathTime( wxCommandEvent& In );
	void OnJumpToPathTime( wxCommandEvent& In );
	void OnViewHide3DTracks( wxCommandEvent& In );
	void OnViewZoomToScrubPos( wxCommandEvent& In );
	void OnToggleCurveEd( wxCommandEvent& In );
	void OnGraphSplitChangePos( wxSplitterEvent& In );
	void OnToggleSnap( wxCommandEvent& In );
	void OnChangeSnapSize( wxCommandEvent& In );
	void OnViewFitSequence( wxCommandEvent& In );
	void OnViewFitLoop( wxCommandEvent& In );
	void OnViewFitLoopSequence( wxCommandEvent& In );

	void OnOpenBindKeysDialog( wxCommandEvent &In );

	void OnContextNewTrack( wxCommandEvent& In );
	void OnContextNewGroup( wxCommandEvent& In );
	void OnContextTrackRename( wxCommandEvent& In );
	void OnContextTrackDelete( wxCommandEvent& In );
	void OnContextTrackChangeFrame( wxCommandEvent& In );
	void OnContextGroupRename( wxCommandEvent& In );
	void OnContextGroupDelete( wxCommandEvent& In );
	void OnContextGroupCreateTab( wxCommandEvent& In );
	void OnContextGroupSendToTab( wxCommandEvent& In );
	void OnContextGroupRemoveFromTab( wxCommandEvent& In );
	void OnContextDeleteGroupTab( wxCommandEvent& In );
	void OnContextKeyInterpMode( wxCommandEvent& In );
	void OnContextRenameEventKey( wxCommandEvent& In );
	void OnContextSetKeyTime( wxCommandEvent& In );
	void OnContextSetValue( wxCommandEvent& In );

	/** Pops up a menu and lets you set the color for the selected key. Not all track types are supported. */
	void OnContextSetColor( wxCommandEvent& In );

	/** Pops up menu and lets the user set a group to use to lookup transform info for a movement keyframe. */
	void OnSetMoveKeyLookupGroup( wxCommandEvent& In );

	/** Clears the lookup group for a currently selected movement key. */
	void OnClearMoveKeyLookupGroup( wxCommandEvent& In );

	void OnSetAnimKeyLooping( wxCommandEvent& In );
	void OnSetAnimOffset( wxCommandEvent& In );
	void OnSetAnimPlayRate( wxCommandEvent& In );

	/** Handler for the toggle animation reverse menu item. */
	void OnToggleReverseAnim( wxCommandEvent& In );

	/** Handler for UI update requests for the toggle anim reverse menu item. */
	void OnToggleReverseAnim_UpdateUI( wxUpdateUIEvent& In );

	/** Handler for the save as camera animation menu item. */
	void OnContextSaveAsCameraAnimation( wxCommandEvent& In );

	void OnSetSoundVolume(wxCommandEvent& In);
	void OnSetSoundPitch(wxCommandEvent& In);
	void OnContextDirKeyTransitionTime( wxCommandEvent& In );
	void OnFlipToggleKey(wxCommandEvent& In);

	void OnMenuUndo( wxCommandEvent& In );
	void OnMenuRedo( wxCommandEvent& In );
	void OnMenuCut( wxCommandEvent& In );
	void OnMenuCopy( wxCommandEvent& In );
	void OnMenuPaste( wxCommandEvent& In );

	void OnMenuEdit_UpdateUI( wxUpdateUIEvent& In );

	void OnMenuImport( wxCommandEvent& In );
	void OnMenuExport( wxCommandEvent& In );
	void OnMenuReduceKeys( wxCommandEvent& In );

	// Selection
	void SetSelectedFilter(class UInterpFilter* InFilter);
	void SetActiveTrack(class UInterpGroup* InGroup, INT InTrackIndex);

	UBOOL KeyIsInSelection(class UInterpGroup* InGroup, INT InTrackIndex, INT InKeyIndex);
	void AddKeyToSelection(class UInterpGroup* InGroup, INT InTrackIndex, INT InKeyIndex, UBOOL bAutoWind);
	void RemoveKeyFromSelection(class UInterpGroup* InGroup, INT InTrackIndex, INT InKeyIndex);
	void ClearKeySelection();
	void CalcSelectedKeyRange(FLOAT& OutStartTime, FLOAT& OutEndTime);
	void SelectKeysInLoopSection();

	// Utils
	void DeleteSelectedKeys(UBOOL bDoTransaction=false);
	void DuplicateSelectedKeys();
	void BeginMoveSelectedKeys();
	void EndMoveSelectedKeys();
	void MoveSelectedKeys(FLOAT DeltaTime);
	void AddKey();
	void SplitAnimKey();
	void ReduceKeys();

	void ViewFitSequence();
	void ViewFitLoop();
	void ViewFitLoopSequence();
	void ViewFit(FLOAT StartTime, FLOAT EndTime);

	void ToggleCurveEd();

	void ChangeKeyInterpMode(EInterpCurveMode NewInterpMode=CIM_Unknown);

	/**
	 * Copies the currently selected group/group.
	 *
	 * @param bCut	Whether or not we should cut instead of simply copying the group/track.
	 */
	void CopySelectedGroupOrTrack(UBOOL bCut);

	/**
	 * Pastes the previously copied group/track.
	 */
	void PasteSelectedGroupOrTrack();

	/**
	 * @return Whether or not we can paste a group/track.
	 */
	UBOOL CanPasteGroupOrTrack();

	/** Deletes the currently active track. */
	void DeleteSelectedTrack();

	/** Deletes the currently active group. */
	void DeleteSelectedGroup();

	/**
	 * Duplicates the specified group
	 *
	 * @param GroupToDuplicate		Group we are going to duplicate.
	 */
	void DuplicateGroup(UInterpGroup* GroupToDuplicate);

	/**
	 * Adds a new track to the selected group.
	 *
	 * @param TrackClass		The class of the track we are adding.
	 * @param TrackToCopy		A optional track to copy instead of instatiating a new one.  If NULL, a new track will be instatiated.
	 */
	void AddTrackToSelectedGroup(UClass* TrackClass, UInterpTrack* TrackToCopy=NULL);

	/** 
	 * Crops the anim key in the currently selected track. 
	 *
	 * @param	bCropBeginning		Whether to crop the section before the position marker or after.
	 */
	void CropAnimKey(UBOOL bCropBeginning);

	void UpdateMatineeActionConnectors();
	void LockCamToGroup(class UInterpGroup* InGroup);
	class AActor* GetViewedActor();
	virtual void UpdateCameraToGroup();
	void UpdateCamColours();

	void SyncCurveEdView();
	void AddTrackToCurveEd(class UInterpGroup* InGroup, INT InTrackIndex);

	void SetInterpPosition(FLOAT NewPosition);
	void SelectActiveGroupParent();

	/** Increments the cursor or selected keys by 1 interval amount, as defined by the toolbar combo. */
	void IncrementSelection();

	/** Decrements the cursor or selected keys by 1 interval amount, as defined by the toolbar combo. */
	void DecrementSelection();

	void SelectNextKey();
	void SelectPreviousKey();
	void ZoomView(FLOAT ZoomAmount);
	void SetSnapEnabled(UBOOL bInSnapEnabled);
	FLOAT SnapTime(FLOAT InTime, UBOOL bIgnoreSelectedKeys);

	void BeginMoveMarker();
	void EndMoveMarker();
	void SetInterpEnd(FLOAT NewInterpLength);
	void MoveLoopMarker(FLOAT NewMarkerPos, UBOOL bIsStart);

	void BeginDrag3DHandle(UInterpGroup* Group, INT TrackIndex);
	void Move3DHandle(UInterpGroup* Group, INT TrackIndex, INT KeyIndex, UBOOL bArriving, const FVector& Delta);
	void EndDrag3DHandle();
	void MoveInitialPosition(const FVector& Delta, const FRotator& DeltaRot);

	void ActorModified();
	void ActorSelectionChange();
	void CamMoved(const FVector& NewCamLocation, const FRotator& NewCamRotation);
	UBOOL ProcessKeyPress(FName Key, UBOOL bCtrlDown, UBOOL bAltDown);

	void InterpEdUndo();
	void InterpEdRedo();

	void MoveActiveBy(INT MoveBy);
	void MoveActiveUp();
	void MoveActiveDown();

	void DrawTracks3D(const FSceneView* View, FPrimitiveDrawInterface* PDI);
	void DrawModeHUD(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas);

	void TickInterp(FLOAT DeltaSeconds);

	static void UpdateAttachedLocations(AActor* BaseActor);
	static void InitInterpTrackClasses();

	WxInterpEdToolBar* ToolBar;
	WxInterpEdMenuBar* MenuBar;

	wxSplitterWindow* GraphSplitterWnd; // Divides the graph from the track view.
	INT GraphSplitPos;

	WxPropertyWindow* PropertyWindow;
	FInterpEdViewportClient* InterpEdVC;
	WxCurveEditor* CurveEd;
	WxInterpEdVCHolder* TrackWindow;

	// Scroll bar
	INT	ThumbPos_Vert;

	UTexture2D*	BarGradText;
	FColor PosMarkerColor;
	FColor RegionFillColor;
	FColor RegionBorderColor;

	class USeqAct_Interp* Interp;
	class UInterpData* IData;

	// Only 1 track can be 'Active' at a time. This will be used for new keys etc.
	// You may have a Group selected but no Tracks (eg. empty group)
	class UInterpGroup* ActiveGroup;
	INT ActiveTrackIndex;

	// If we are connecting the camera to a particular group, this is it. If not, its NULL;
	class UInterpGroup* CamViewGroup;

	// Editor-specific Object, containing preferences and selection set to be serialised/undone.
	UInterpEdOptions* Opt;

	// Are we currently editing the value of a keyframe. This should only be true if there is one keyframe selected and the time is currently set to it.
	UBOOL bAdjustingKeyframe;

	// If we are looping 
	UBOOL bLoopingSection;

	// Currently moving a curve handle in the 3D viewport.
	UBOOL bDragging3DHandle;

	// Multiplier for preview playback of sequence
	FLOAT PlaybackSpeed;

	// Whether to draw the 3D version of any tracks.
	UBOOL bHide3DTrackView;

	/** Indicates if zoom should auto-center on the current scrub position. */
	UBOOL bZoomToScrubPos;

	/** Indicates if zoom should auto-center on the current scrub position. */
	UBOOL bShowCurveEd;

	// Snap settings.
	UBOOL bSnapEnabled;
	UBOOL bSnapToKeys;
	UBOOL bSnapToFrames;
	FLOAT SnapAmount;

	UTransactor* NormalTransactor;
	UInterpEdTransBuffer* InterpEdTrans;

	/** Set to TRUE in OnClose, at which point the editor is no longer ticked. */
	UBOOL	bClosed;

	/** If TRUE, the editor is modifying a CameraAnim, and functionality is tweaked appropriately */
	UBOOL	bEditingCameraAnim;

	// Static list of all InterpTrack subclasses.
	static TArray<UClass*>	InterpTrackClasses;
	static UBOOL			bInterpTrackClassesInitialized;

	/** Creates a popup context menu based on the item under the mouse cursor.
	* @param	Viewport	FViewport for the FInterpEdViewportClient.
	* @param	HitResult	HHitProxy returned by FViewport::GetHitProxy( ).
	* @return	A new wxMenu with context-appropriate menu options or NULL if there are no appropriate menu options.
	*/
	virtual wxMenu	*CreateContextMenu( FViewport *Viewport, const HHitProxy *HitResult );

protected:
	/**
	 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
	 *  @return A string representing a name to use for this docking parent.
	 */
	virtual const TCHAR* GetDockingParentName() const;

	/**
	 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
	 */
	virtual const INT GetDockingParentVersion() const;

	DECLARE_EVENT_TABLE()
};

/*-----------------------------------------------------------------------------
	WxMBInterpEdTabMenu
-----------------------------------------------------------------------------*/

class WxMBInterpEdTabMenu : public wxMenu
{
public:
	WxMBInterpEdTabMenu(WxInterpEd* InterpEd);
	~WxMBInterpEdTabMenu();
};


/*-----------------------------------------------------------------------------
	WxMBInterpEdGroupMenu
-----------------------------------------------------------------------------*/

class WxMBInterpEdGroupMenu : public wxMenu
{
public:
	WxMBInterpEdGroupMenu(WxInterpEd* InterpEd);
	~WxMBInterpEdGroupMenu();
};

/*-----------------------------------------------------------------------------
	WxMBInterpEdTrackMenu
-----------------------------------------------------------------------------*/

class WxMBInterpEdTrackMenu : public wxMenu
{
public:
	WxMBInterpEdTrackMenu(WxInterpEd* InterpEd);
	~WxMBInterpEdTrackMenu();
};

/*-----------------------------------------------------------------------------
	WxMBInterpEdBkgMenu
-----------------------------------------------------------------------------*/

class WxMBInterpEdBkgMenu : public wxMenu
{
public:
	WxMBInterpEdBkgMenu(WxInterpEd* InterpEd);
	~WxMBInterpEdBkgMenu();
};

/*-----------------------------------------------------------------------------
	WxMBInterpEdKeyMenu
-----------------------------------------------------------------------------*/

class WxMBInterpEdKeyMenu : public wxMenu
{
public:
	WxMBInterpEdKeyMenu(WxInterpEd* InterpEd);
	~WxMBInterpEdKeyMenu();
};


/*-----------------------------------------------------------------------------
	WxCameraAnimEd
-----------------------------------------------------------------------------*/

/** A specialized version of WxInterpEd used for CameraAnim editing.  Tangential features of Matinee are disabled. */
class WxCameraAnimEd : public WxInterpEd
{
public:
	WxCameraAnimEd( wxWindow* InParent, wxWindowID InID, class USeqAct_Interp* InInterp );
	virtual ~WxCameraAnimEd();

	virtual void	OnClose( wxCloseEvent& In );

	/** Creates a popup context menu based on the item under the mouse cursor.
	* @param	Viewport	FViewport for the FInterpEdViewportClient.
	* @param	HitResult	HHitProxy returned by FViewport::GetHitProxy( ).
	* @return	A new wxMenu with context-appropriate menu options or NULL if there are no appropriate menu options.
	*/
	virtual wxMenu	*CreateContextMenu( FViewport *Viewport, const HHitProxy *HitResult );

	virtual void UpdateCameraToGroup();
};


/*-----------------------------------------------------------------------------
	WxMBCameraAnimEdGroupMenu
-----------------------------------------------------------------------------*/

class WxMBCameraAnimEdGroupMenu : public wxMenu
{
public:
	WxMBCameraAnimEdGroupMenu(WxCameraAnimEd* CamAnimEd);
	~WxMBCameraAnimEdGroupMenu();
};

#endif // __INTERPEDITOR_H__
