/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "SceneManager.h"
#include "..\..\Launch\Resources\resource.h"
#include "UnTerrain.h"
#include "EngineSequenceClasses.h"
#include "Properties.h"

/** IDs for Scene Manager window elements. */
#define SM_ID_PANEL 10009
#define SM_ID_SHOWBRUSHES 10004
#define SM_ID_TYPEFILTER 10005
#define SM_ID_NAMEFILTER 10006
#define SM_ID_GRID 10007
// PCF Begin
#define SM_ID_LEVELLIST 10008
// PCF End

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxMBSceneManager
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Scene Manager menu bar.
 */
class WxMBSceneManager : public wxMenuBar
{
public:
	WxMBSceneManager()
	{
		// View menu
		wxMenu* ViewMenu = new wxMenu();
		ViewMenu->Append( IDM_RefreshBrowser, *LocalizeUnrealEd("RefreshWithHotkey"), TEXT("") );
		Append( ViewMenu, *LocalizeUnrealEd("View") );

		// Edit menu
		wxMenu* EditMenu = new wxMenu();
		EditMenu->Append( IDMN_SCENEMANAGER_DELETE, *LocalizeUnrealEd("Delete"), TEXT("") );
		Append( EditMenu, *LocalizeUnrealEd("Edit") );

		WxBrowser::AddDockingMenu( this );
	}
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxSceneManager
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// PCF Begin
/* Deprecated - only SM_PROPS_WIDTH is important, rest is automatic
#define SM_SPLITTER_WIDTH	1000
#define SM_SPLITTER_HEIGHT	400
#define SM_SPLITTER_SASH	500
#define SM_GRID_WIDTH		600
#define SM_GRID_HEIGHT		SM_SPLITTER_HEIGHT - 40
*/
// PCF End

#define SM_PROPS_WIDTH		300
// PCF Begin
#define SM_LEVELLIST_WIDTH	150
#define SM_LEVELLIST_HEIGHT	96
// PCF End

/** Enums for scene manager columns. */
enum
{
	SM_ACTOR,
	SM_TAG,
	SM_TYPE,
	SM_GROUPS,
	SM_ATTACHMENT,
	SM_KISMET,
	SM_LOCATION,
	SM_PACKAGE,
	SM_LAYER,
	//SM_MEMORY,
	//SM_POLYGONS,
	SM_NUM
};

/** Column headings. */
static const char *GGridHeadings[SM_NUM] =
{ 
	"Actor",
	"Tag",
	"Type",
	"Groups",
	"AttachmentBase",
	"Kismet",
	"Location",
	"Package",
	"Layer",
	//"Memory",
	//"Polygons",
};

/** Compare function implementations for column sorting. */
IMPLEMENT_COMPARE_POINTER( AActor, SM_ACTOR, \
{ \
	return appStricmp( *A->GetName(), *B->GetName() ); \
} )

IMPLEMENT_COMPARE_POINTER( AActor, SM_TAG, \
{ \
	UBOOL Comp = appStricmp( *(A->Tag.ToString()), *(B->Tag.ToString()) ); \
	return (Comp == 0) ? \
			appStricmp( *A->GetName(), *B->GetName() ) : \
			Comp; \
} )

IMPLEMENT_COMPARE_POINTER( AActor, SM_TYPE, \
{ \
	UBOOL Comp = appStricmp( *A->GetClass()->GetName(), *B->GetClass()->GetName() ); \
	return (Comp == 0)? \
			appStricmp( *A->GetName(), *B->GetName() ) : \
			Comp; \
} )

IMPLEMENT_COMPARE_POINTER( AActor, SM_GROUPS, \
{ \
	UBOOL Comp = appStricmp( *(A->Group.ToString()), *(B->Group.ToString()) ); \
	return (Comp == 0) ? \
			appStricmp( *A->GetName(), *B->GetName() ) : \
			Comp; \
} )

IMPLEMENT_COMPARE_POINTER( AActor, SM_ATTACHMENT, \
{ \
	FString _CompStr_A = (A->Base) ? A->Base->GetName() : TEXT(""); \
	FString _CompStr_B = (B->Base) ? B->Base->GetName() : TEXT(""); \
	UBOOL Comp = appStricmp( *_CompStr_A, *_CompStr_B ); \
	return (Comp == 0)? \
			appStricmp( *A->GetName(), *B->GetName() ) : \
			Comp; \
} )

IMPLEMENT_COMPARE_POINTER( AActor, SM_LAYER, \
{ \
	const TCHAR * _CompStr_A = (A->Layer) ? *(A->Layer->Desc) : TEXT(""); \
	const TCHAR * _CompStr_B = (B->Layer) ? *(B->Layer->Desc) : TEXT(""); \
	UBOOL Comp = appStricmp( _CompStr_A, _CompStr_B ); \
	return (Comp == 0)? \
			appStricmp( *A->GetName(), *B->GetName() ) : \
			Comp; \
} )

IMPLEMENT_COMPARE_POINTER( AActor, SM_KISMET, \
{ \
	USequence* RootSeqA = GWorld->GetGameSequence( A->GetLevel() ); \
	USequence* RootSeqB = GWorld->GetGameSequence( B->GetLevel() ); \
	const TCHAR * _CompStr_A = ( RootSeqA && RootSeqA->ReferencesObject(A) ) ? TEXT("TRUE") : TEXT(""); \
	const TCHAR * _CompStr_B = ( RootSeqB && RootSeqB->ReferencesObject(B) ) ? TEXT("TRUE") : TEXT(""); \
	UBOOL Comp = appStricmp( _CompStr_A, _CompStr_B ); \
	return (Comp == 0)? \
			appStricmp( *A->GetName(), *B->GetName() ) : \
			Comp; \
} )

BEGIN_EVENT_TABLE( WxSceneManager, WxBrowser )
	// Window events
	// PCF Begin
	// Deprecated - now controls' positioning is automatic
	//EVT_SIZE( WxSceneManager::OnSize )
	// PCF End

	// Menu events
	EVT_MENU( IDMN_SCENEMANAGER_DELETE, WxSceneManager::OnDelete )
	EVT_MENU( IDM_RefreshBrowser, WxSceneManager::OnRefresh )

	// Top Toolbar events
    EVT_CHECKBOX( IDMN_SCENEMANAGER_AUTOFOCUS, WxSceneManager::OnAutoFocus )
    EVT_BUTTON( IDMN_SCENEMANAGER_AUTOFOCUS, WxSceneManager::OnAutoFocus )
    EVT_BUTTON( IDMN_SCENEMANAGER_FOCUS, WxSceneManager::OnFocus )
    EVT_BUTTON( IDMN_SCENEMANAGER_DELETE, WxSceneManager::OnDelete )
	EVT_BUTTON( IDM_RefreshBrowser, WxSceneManager::OnRefresh )

	// Filtering ToolBar events
    EVT_COMBOBOX( SM_ID_TYPEFILTER, WxSceneManager::OnTypeFilterSelected )
    EVT_CHECKBOX( SM_ID_SHOWBRUSHES, WxSceneManager::OnShowBrushes )
    EVT_TEXT_ENTER( SM_ID_NAMEFILTER, WxSceneManager::OnNameFilterChanged )	

	// Splitter window and grid events
	// PCF Begin
	// Deprecated
	// EVT_SPLITTER_SASH_POS_CHANGED( IDMN_SCENEMANAGER_SPLITTER, WxSceneManager::OnSashPositionChange )
    // EVT_GRID_CMD_SELECT_CELL( SM_ID_GRID, WxSceneManager::OnGridSelectCell )
	// PCF End
    
	EVT_GRID_CMD_RANGE_SELECT( SM_ID_GRID, WxSceneManager::OnGridRangeSelect )
    EVT_GRID_LABEL_LEFT_CLICK( WxSceneManager::OnLabelLeftClick )

	// PCF Begin
	// Level List Events
	EVT_LISTBOX( SM_ID_LEVELLIST, WxSceneManager::OnLevelSelected )
	// PCF End
END_EVENT_TABLE()


// PCF Begin
/*-----------------------------------------------------------------------------
 WxDelGrid
-----------------------------------------------------------------------------*/
BEGIN_EVENT_TABLE(WxDelGrid, wxGrid)
	EVT_CHAR(WxDelGrid::OnChar)
END_EVENT_TABLE()

WxDelGrid::WxDelGrid(wxWindow *parent, wxWindowID id, const wxPoint& pos,
							 const wxSize& size, long style) : 
					wxGrid(parent, id, pos, size, style),
					Parent(NULL) 
{
}

/** Keypress event handler. */
void WxDelGrid::OnChar(wxKeyEvent &Event)
{
	if ( Event.GetKeyCode() == WXK_DELETE )
	{
		if ( Parent )
			Parent->DeleteSelectedActors();
	}
	else
	{
		Event.Skip();
	}
}
/** save grid position */
void WxDelGrid::SaveGridPosition()
{
	GetViewStart(&PrevSX,&PrevSY);
	PrevCX = GetGridCursorRow();
    PrevCY = GetGridCursorCol();
}

/** restore grid position */
void WxDelGrid::RestoreGridPosition()
{
	// restore grid position
	Scroll(PrevSX,PrevSY);
	SetGridCursor(PrevCX,PrevCY);
}
// PCF End



WxSceneManager::WxSceneManager()
	:	bAreWindowsInitialized( FALSE )
    // PCF Begin
	// Deprecated
	//,	SplitterWindow(NULL)
	//,	LastRow(-1)
	// PCF End
	,	PropertyWindow(NULL)
	,	TypeFilter_Combo(NULL)
	,	TypeFilter_Selection(NULL)
	,	ShowBrushes_Check(NULL)
	,	Grid(NULL)
	,	SortColumn(0)
	,	bAutoFocus( FALSE )
	,	bUpdateOnActivated( FALSE )
	// PCF Begin
	,	Level_List(NULL)
	// PCF End
{
	Panel = NULL;
	MenuBar = NULL;

	GCallbackEvent->Register(CALLBACK_RefreshEditor_SceneManager,this);
	GCallbackEvent->Register(CALLBACK_MapChange,this);
}

/**
* Forwards the call to our base class to create the window relationship.
* Creates any internally used windows after that
*
* @param DockID the unique id to associate with this dockable window
* @param FriendlyName the friendly name to assign to this window
* @param Parent the parent of this window (should be a Notebook)
*/
void WxSceneManager::Create(INT DockID,const TCHAR* FriendlyName,wxWindow* Parent)
{
	WxBrowser::Create(DockID,FriendlyName,Parent);

	// Main Panel
	Panel = new wxPanel( this, SM_ID_PANEL);
	CreateControls();  
	Panel->Fit();
    Panel->GetSizer()->SetSizeHints(Panel);

	MenuBar = new WxMBSceneManager();

	// PCF Begin
	// Deprecated
	// SetGridSize();
	// SetPropsSize();
	// PCF End

	Update();

	FLocalizeWindow( this );
}

// PCF Begin
// Deprecated
/*
void WxSceneManager::CreateControls()
{  
	// Top level panel
    wxPanel* itemPanel1 = Panel;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
	SetAutoLayout(TRUE);

    itemPanel1->SetSizer(itemBoxSizer2);

	// Top "Toolbar"
	{
		wxImage		TempImage;
		wxBitmap*	BitMap;

		wxBoxSizer* toolbar1Sizer = new wxBoxSizer(wxHORIZONTAL);
		itemBoxSizer2->Add(toolbar1Sizer, 0, wxGROW|wxALL, 5);

		// AutoFocus CheckBox
		wxStaticText* itemStaticTextAutoFocus = new wxStaticText( itemPanel1, wxID_STATIC, TEXT("AutoFocus"), 
			wxDefaultPosition, wxDefaultSize, 0 );
		toolbar1Sizer->Add(itemStaticTextAutoFocus, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

		wxCheckBox* itemCheckBox6 = new wxCheckBox( itemPanel1, IDMN_SCENEMANAGER_AUTOFOCUS, TEXT(""),
			wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
		itemCheckBox6->SetValue( bAutoFocus == TRUE );
		toolbar1Sizer->Add(itemCheckBox6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

		// Focus Button
		wxBitmap itemBitmapButton5Bitmap(wxNullBitmap);
		wxBitmapButton *FocusButton = new wxBitmapButton( itemPanel1, IDMN_SCENEMANAGER_FOCUS, 
			itemBitmapButton5Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
		toolbar1Sizer->Add(FocusButton, 0,wxALIGN_CENTER_VERTICAL|wxALL,2);
		TempImage.LoadFile(*FString::Printf(TEXT("%swxres\\SceneManager_Focus.bmp"), appBaseDir()), wxBITMAP_TYPE_BMP);
		BitMap = new wxBitmap(TempImage);
		FocusButton->SetBitmapLabel(*BitMap);
		FocusButton->SetToolTip(*LocalizeUnrealEd("SceneManager_Focus"));

		// Refresh Button
		wxBitmap itemBitmapButton6Bitmap(wxNullBitmap);
		wxBitmapButton *RefreshButton = new wxBitmapButton( itemPanel1, IDM_RefreshBrowser, 
			itemBitmapButton6Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
		toolbar1Sizer->Add(RefreshButton, 0,wxALIGN_CENTER_VERTICAL|wxALL,2);
		TempImage.LoadFile(*FString::Printf(TEXT("%swxres\\SceneManager_Refresh.bmp"), appBaseDir()), wxBITMAP_TYPE_BMP);
		BitMap = new wxBitmap(TempImage);
		RefreshButton->SetBitmapLabel(*BitMap);
		RefreshButton->SetToolTip(*LocalizeUnrealEd("Refresh"));

		// Delete Button
		wxBitmap itemBitmapButton7Bitmap(wxNullBitmap);
		wxBitmapButton *DeleteButton = new wxBitmapButton( itemPanel1, IDMN_SCENEMANAGER_DELETE, 
			itemBitmapButton7Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
		toolbar1Sizer->Add(DeleteButton, 0,wxALIGN_CENTER_VERTICAL|wxALL,2);
		TempImage.LoadFile(*FString::Printf(TEXT("%swxres\\SceneManager_Delete.bmp"), appBaseDir()), wxBITMAP_TYPE_BMP);
		BitMap = new wxBitmap(TempImage);
		DeleteButton->SetBitmapLabel(*BitMap);
		DeleteButton->SetToolTip(*LocalizeUnrealEd("Delete"));
	}

	// Filtering "Toolbar"
	wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    wxComboBox* itemComboBox4 = new wxComboBox( Panel, SM_ID_TYPEFILTER, TEXT(""), 
		wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
    itemBoxSizer3->Add(itemComboBox4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText5 = new wxStaticText( itemPanel1, wxID_STATIC, *LocalizeUnrealEd("SceneManager_FilterQ"), 
		wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* itemTextCtrl6 = new wxTextCtrl( itemPanel1, SM_ID_NAMEFILTER, TEXT(""), 
		wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemTextCtrl6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

	wxStaticText* itemStaticText77 = new wxStaticText( itemPanel1, wxID_STATIC, TEXT("ShowBrushes"),
		wxDefaultPosition, wxDefaultSize, 0 );
	itemBoxSizer3->Add(itemStaticText77, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

	wxCheckBox* itemCheckBox7 = new wxCheckBox( itemPanel1, SM_ID_SHOWBRUSHES, TEXT(""),
		wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemBoxSizer3->Add(itemCheckBox7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

	// Splitter window
    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxALIGN_LEFT|wxALL, 0);

    wxSplitterWindow* itemSplitterWindow8 = new wxSplitterWindow( itemPanel1, IDMN_SCENEMANAGER_SPLITTER, 
		wxDefaultPosition, wxSize(SM_SPLITTER_WIDTH, SM_SPLITTER_HEIGHT), wxSP_3D|wxSP_3DBORDER|wxSP_FULLSASH );
	itemSplitterWindow8->SetMinimumPaneSize(20);

	// Grid 
    wxPanel* itemPanel9 = new wxPanel( itemSplitterWindow8, -1, 
		wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxGrid* itemGrid10 = new wxGrid( itemPanel9, SM_ID_GRID, 
		wxDefaultPosition, wxSize(SM_GRID_WIDTH, SM_GRID_HEIGHT), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
	itemGrid10->EnableEditing(false);
	itemGrid10->SetDefaultColSize(70);
    itemGrid10->SetDefaultRowSize(16);
    itemGrid10->SetColLabelSize(25);
    itemGrid10->SetRowLabelSize(50);
	Grid = itemGrid10;

	// Assign column headings
	wxArrayString colHeadings;
	for ( UINT i = 0; i < SM_NUM; i++)
	{
		colHeadings.Add(*LocalizeUnrealEd(GGridHeadings[i]));
	}

	// Initialize grid headings
    Grid->CreateGrid(0, colHeadings.Count(), wxGrid::wxGridSelectCells);
	for ( UINT i=0; i < colHeadings.Count(); i++)
	{
		Grid->SetColLabelValue(i, colHeadings[i]);
	}

	// Property panel
	PropertyWindow = new WxPropertyWindow;
	PropertyWindow->Create( (wxWindow*)itemSplitterWindow8, NULL );

	// Splitter window contains Grid on LHS and Property panel on RHS
	wxRect rc = GetClientRect();
	itemSplitterWindow8->SplitVertically(itemPanel9, PropertyWindow,  rc.GetWidth() - SM_PROPS_WIDTH);
    itemBoxSizer7->Add(itemSplitterWindow8, 1, wxGROW|wxALL, 5);

	SplitterWindow = itemSplitterWindow8;
	TypeFilter_Combo = itemComboBox4;
	TypeFilter_Selection = 0;
	SortColumn = 0;
	bAutoFocus = FALSE;
	ShowBrushes_Check = itemCheckBox7;

	PropertyWindow->Raise();

	Refresh();
	Grid->SelectRow(0, true);

	bAreWindowsInitialized = TRUE;
}
*/

//  PCF version accordance with wxWidgets standards
void WxSceneManager::CreateControls()
{  
	// Main Sizer
    wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	Panel->SetSizer(MainSizer);

	// Toolbar Sizers
	wxBoxSizer* MainToolbarSizer = new wxBoxSizer(wxHORIZONTAL);
	MainSizer->Add(MainToolbarSizer, 0, wxGROW|wxALL, 2);

	wxBoxSizer* ToolbarLeftSizer = new wxBoxSizer(wxVERTICAL);
	MainToolbarSizer->Add(ToolbarLeftSizer, 0, wxGROW|wxALL, 2);

	wxBoxSizer* ToolbarRightSizer = new wxBoxSizer(wxVERTICAL);
	MainToolbarSizer->Add(ToolbarRightSizer, 0, wxGROW|wxALL, 2);
	
	// Top Left Toolbar (Level Selector)
	{
		Level_List = new wxListBox( Panel, SM_ID_LEVELLIST, wxDefaultPosition,
			wxSize(SM_LEVELLIST_WIDTH, SM_LEVELLIST_HEIGHT), 0, NULL, wxLB_MULTIPLE ); 
		ToolbarLeftSizer->Add( Level_List, 0, wxALL, 2 );
	}

	// Top Upper Toolbar (Focus, Refresh, Delete)
	{
		wxImage		TempImage;
		wxBitmap*	BitMap;

		// Tooolbar Sizer
		wxBoxSizer* TooolbarSizer = new wxBoxSizer(wxHORIZONTAL);
		ToolbarRightSizer->Add(TooolbarSizer, 0, wxGROW|wxALL, 5);

		// AutoFocus Text
		wxStaticText* AutoFocusText = new wxStaticText( Panel, wxID_STATIC, TEXT("AutoFocus"), 
			wxDefaultPosition, wxDefaultSize, 0 );
		TooolbarSizer->Add(AutoFocusText, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

		// AutoFocus CheckBox
		wxCheckBox* AutoFocusCheckBox = new wxCheckBox( Panel, IDMN_SCENEMANAGER_AUTOFOCUS, TEXT(""),
			wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
		AutoFocusCheckBox->SetValue( bAutoFocus == TRUE );
		TooolbarSizer->Add(AutoFocusCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

		// Focus Button
		wxBitmap FocusBitmap(wxNullBitmap);
		wxBitmapButton* FocusButton = new wxBitmapButton( Panel, IDMN_SCENEMANAGER_FOCUS, 
			FocusBitmap, wxDefaultPosition, wxSize(24,24), wxBU_AUTODRAW|wxBU_EXACTFIT );
		TooolbarSizer->Add(FocusButton, 0,wxALIGN_CENTER_VERTICAL|wxALL,2);
		TempImage.LoadFile(*FString::Printf(TEXT("%swxres\\SceneManager_Focus.bmp"), appBaseDir()), wxBITMAP_TYPE_BMP);
		BitMap = new wxBitmap(TempImage);
		FocusButton->SetBitmapLabel(*BitMap);
		FocusButton->SetToolTip(*LocalizeUnrealEd("SceneManager_Focus"));

		// Refresh Button
		wxBitmap RefreshBitmap(wxNullBitmap);
		wxBitmapButton* RefreshButton = new wxBitmapButton( Panel, IDM_RefreshBrowser, 
			RefreshBitmap, wxDefaultPosition, wxSize(24,24), wxBU_AUTODRAW|wxBU_EXACTFIT );
		TooolbarSizer->Add(RefreshButton, 0,wxALIGN_CENTER_VERTICAL|wxALL,2);
		TempImage.LoadFile(*FString::Printf(TEXT("%swxres\\SceneManager_Refresh.bmp"), appBaseDir()), wxBITMAP_TYPE_BMP);
		BitMap = new wxBitmap(TempImage);
		RefreshButton->SetBitmapLabel(*BitMap);
		RefreshButton->SetToolTip(*LocalizeUnrealEd("Refresh"));

		// Delete Button
		wxBitmap DeleteBitmap(wxNullBitmap);
		wxBitmapButton* DeleteButton = new wxBitmapButton( Panel, IDMN_SCENEMANAGER_DELETE, 
			DeleteBitmap, wxDefaultPosition, wxSize(24,24), wxBU_AUTODRAW|wxBU_EXACTFIT );
		TooolbarSizer->Add(DeleteButton, 0,wxALIGN_CENTER_VERTICAL|wxALL,2);
		TempImage.LoadFile(*FString::Printf(TEXT("%swxres\\SceneManager_Delete.bmp"), appBaseDir()), wxBITMAP_TYPE_BMP);
		BitMap = new wxBitmap(TempImage);
		DeleteButton->SetBitmapLabel(*BitMap);
		DeleteButton->SetToolTip(*LocalizeUnrealEd("Delete"));
	}


	// Top Bottom Toolbar (Filter, Show Brushes)
	{
		// Toolbar Sizer
		wxBoxSizer* TooolbarSizer = new wxBoxSizer(wxHORIZONTAL);
		ToolbarRightSizer->Add(TooolbarSizer, 0, wxALL, 0);

		// Filter Combo
		wxComboBox* FilterCombo = new wxComboBox( Panel, SM_ID_TYPEFILTER, TEXT(""), 
			wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
		TooolbarSizer->Add(FilterCombo, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

		// Filter Text
		wxStaticText* FilterText = new wxStaticText( Panel, wxID_STATIC, *LocalizeUnrealEd("SceneManager_FilterQ"), 
			wxDefaultPosition, wxDefaultSize, 0 );
		TooolbarSizer->Add(FilterText, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);
	 
		// Filter Edit
		wxTextCtrl* FilterEdit = new wxTextCtrl( Panel, SM_ID_NAMEFILTER, TEXT(""), 
			wxDefaultPosition, wxDefaultSize, 0 );
		TooolbarSizer->Add(FilterEdit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

		// Show Brushes Text
		wxStaticText* ShowBText = new wxStaticText( Panel, wxID_STATIC, TEXT("ShowBrushes"),
			wxDefaultPosition, wxDefaultSize, 0 );
		TooolbarSizer->Add(ShowBText, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

		// Show Brushes Checkbox
		wxCheckBox* ShowBCheckbox = new wxCheckBox( Panel, SM_ID_SHOWBRUSHES, TEXT(""),
			wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
		TooolbarSizer->Add(ShowBCheckbox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

		ShowBrushes_Check = ShowBCheckbox;
		TypeFilter_Combo = FilterCombo;
		TypeFilter_Selection = 0;
	}

	// Grid & Property Panel: Sizer & Splitter
	wxBoxSizer* GridAndPropertiesSizer = new wxBoxSizer( wxHORIZONTAL );
	wxSplitterWindow* SplitterWindow = new wxSplitterWindow( Panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
	SplitterWindow->SetMinimumPaneSize( 100 );
	GridAndPropertiesSizer->Add( SplitterWindow, 1, wxEXPAND, 5 );	
	MainSizer->Add( GridAndPropertiesSizer, 1, wxEXPAND, 5 );
	
	// Grid
	wxPanel* GridPanel;
	{
		GridPanel = new wxPanel( SplitterWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL  );
		wxBoxSizer* GridSizer = new wxBoxSizer( wxVERTICAL );
		
		GridSizer->SetMinSize(wxSize( 100,100 )); 
		Grid = new WxDelGrid( GridPanel, SM_ID_GRID, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS );
		Grid->Parent = this; // set parent for callbacks
		GridSizer->Add( Grid, 1, wxALL|wxEXPAND, 5 );
		Grid->EnableEditing(false);
		Grid->SetDefaultColSize(70);
		Grid->SetDefaultRowSize(18);
		Grid->SetColLabelSize(25);
		Grid->SetRowLabelSize(40);
		Grid->EnableDragRowSize(false);
		Grid->EnableCellEditControl(false);
		//Grid->EnableDragGridSize(false);
		//Grid->SetCellHighlightPenWidth(0);

		// Assign column headings
		wxArrayString colHeadings;
		for ( UINT i = 0; i < SM_NUM; i++)
		{
			colHeadings.Add(*LocalizeUnrealEd(GGridHeadings[i]));
		}

		// Initialize grid headings
		Grid->CreateGrid(0, colHeadings.Count(), wxGrid::wxGridSelectRows);
		for ( UINT i=0; i < colHeadings.Count(); i++)
		{
			Grid->SetColLabelValue(i, colHeadings[i]);
		}

		Grid->SetColSize(0,135);

		GridPanel->SetSizer( GridSizer );
		GridPanel->Layout();
		GridSizer->Fit( GridPanel );
		
	}

	// Property Panel
	wxPanel* PropertyPanel;
	{
		PropertyPanel = new wxPanel( SplitterWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
		wxBoxSizer* PropertySizer = new wxBoxSizer( wxVERTICAL );	
		PropertySizer->SetMinSize(wxSize( 100,100 )); 
		
		PropertyWindow = new WxPropertyWindow;
		PropertyWindow->Create( PropertyPanel, NULL );	
		PropertySizer->Add( PropertyWindow, 1, wxALL|wxEXPAND, 5 );

		PropertyPanel->SetSizer( PropertySizer );
		PropertyPanel->Layout();
		PropertySizer->Fit( PropertyPanel );
	}

	SplitterWindow->SetSize(0,0,1000,500); // temporary fake size for correct sash position (wxWidgets bug)
	SplitterWindow->SplitVertically( GridPanel, PropertyPanel, -SM_PROPS_WIDTH );
	SplitterWindow->SetSashGravity(1);
	
	this->Layout();

	SortColumn = 0;
	bAutoFocus = FALSE; 	

	Refresh();
	bAreWindowsInitialized = TRUE;
}

// PCF End

void WxSceneManager::PopulateCombo(const TArray<ULevel*>& Levels)
{
	// Do nothing if browser isn't initialized.
	if( bAreWindowsInitialized )
	{

		TArray<FString> ClassList;
		
		for ( INT LevelIndex = 0 ; LevelIndex < Levels.Num() ; ++ LevelIndex )
		{
			ULevel* Level = Levels( LevelIndex );
			for( INT ActorIndex = 0 ; ActorIndex < Level->Actors.Num() ; ++ActorIndex )
			{
				AActor* Actor = Level->Actors(ActorIndex);
				if( Actor )
				{
					ClassList.AddUniqueItem( *Actor->GetClass()->GetName() );
				}
			}
		}

		// Populate TypeFilter_Combo.
		TypeFilter_Combo->Clear();
		TypeFilter_Combo->Append( TEXT("") );

		for ( INT ClassIndex = 0 ; ClassIndex < ClassList.Num() ; ++ClassIndex )
		{
			TypeFilter_Combo->Append( *ClassList(ClassIndex) );
		}

		TypeFilter_Combo->SetSelection( TypeFilter_Selection );
	}
}

void WxSceneManager::PopulateGrid(const TArray<ULevel*>& Levels)
{
	// No wxWidget operations if browser isn't initialized
	if( !bAreWindowsInitialized )
	{
		return;
	}

	// PCF Begin
	// Deprecated - this method didn't work in wxWidget
	// Save selected rows
	// wxArrayInt selRows = Grid->GetSelectedRows();

	// save grid position
	Grid->SaveGridPosition();
	// PCF End

	// Create Actors array
	GridActors.Empty();

	const UBOOL bShowBrushes = ShowBrushes_Check->IsChecked();
	const wxString ClassFilterString( TypeFilter_Combo->GetStringSelection() );
	const UBOOL bClassFilterEmpty = ClassFilterString.IsSameAs( TEXT("") ) ? TRUE : FALSE;
	const FString NameFilterToUpper( NameFilter.ToUpper() );

	//DOUBLE UpdateStartTime1 = appSeconds();
	for ( INT LevelIndex = 0 ; LevelIndex < Levels.Num() ; ++LevelIndex )
	{
		ULevel* Level = Levels( LevelIndex );
		TTransArray<AActor*>& Actors = Level->Actors;
		for( INT ActorIndex = 0 ; ActorIndex < Actors.Num() ; ++ActorIndex )
		{
			AActor* Actor = Actors(ActorIndex);
			if ( Actor )
			{
				if ( bClassFilterEmpty || ClassFilterString.IsSameAs(*Actor->GetClass()->GetName()) )
				{
					// Disallow brushes?
					if ( !bShowBrushes && Actor->IsABrush() )
					{
						continue;
					}

					/* Show only Placeable Objects
					if( (Actor->GetClass()->ClassFlags & CLASS_Hidden) || 
						(!ShowBrushes_Check->IsChecked() && !(Actor->GetClass()->ClassFlags & CLASS_Placeable)) )
						continue;
					*/

					// Check to see if we need to filter based on name.
					if ( NameFilter.Len() > 0 )
					{
						if ( !appStrstr(*FString( *Actor->GetName() ).ToUpper(), *NameFilterToUpper) )
						{
							// Names don't match, advance to the next one.
							continue;
						}
					}

					GridActors.AddItem(Actor);
				}
			}
		}
	}
	//debugf( TEXT("%s Update() - %i ms"), TEXT("GridActors"), appTrunc((appSeconds() - UpdateStartTime1) * 1000) );

	//DOUBLE UpdateStartTime2 = appSeconds();	
	// Sort actors array
	switch (SortColumn)
	{
	case SM_ACTOR: 
		Sort<USE_COMPARE_POINTER(AActor, SM_ACTOR)>( &GridActors(0), GridActors.Num() );
		break;
	case SM_TAG:
		Sort<USE_COMPARE_POINTER(AActor, SM_TAG)>( &GridActors(0), GridActors.Num() );
		break;
	case SM_TYPE:
		Sort<USE_COMPARE_POINTER(AActor, SM_TYPE)>( &GridActors(0), GridActors.Num() );
		break;
	case SM_GROUPS:
		Sort<USE_COMPARE_POINTER(AActor, SM_GROUPS)>( &GridActors(0), GridActors.Num() );
		break;
	case SM_ATTACHMENT:
		Sort<USE_COMPARE_POINTER(AActor, SM_ATTACHMENT)>( &GridActors(0), GridActors.Num() );
		break;
	case SM_LAYER:
		Sort<USE_COMPARE_POINTER(AActor, SM_LAYER)>( &GridActors(0), GridActors.Num() );
		break;
	case SM_KISMET: break;
		Sort<USE_COMPARE_POINTER(AActor, SM_KISMET)>( &GridActors(0), GridActors.Num() );
		break;
	case SM_LOCATION: break;
	case SM_PACKAGE: break;
	//case SM_MEMORY: break;
	//case SM_POLYGONS: break;
	}
	//debugf( TEXT("%s Update() - %i ms"), TEXT("Sort"), appTrunc((appSeconds() - UpdateStartTime2) * 1000) );


	//DOUBLE UpdateStartTime20 = appSeconds();	
	// Delete existing grid rows
	if ( Grid->GetNumberRows() > 0 )
	{
		Grid->DeleteRows( 0, Grid->GetNumberRows() );
	}

	// Create Grid
	Grid->AppendRows( GridActors.Num() );
	//debugf( TEXT("%s Update() - %i ms"), TEXT("Del and Create"), appTrunc((appSeconds() - UpdateStartTime20) * 1000) );

	//DOUBLE UpdateStartTime3 = appSeconds();
	for( INT ActorIndex = 0 ; ActorIndex < GridActors.Num() ; ++ActorIndex )
	{
		AActor *Actor = GridActors(ActorIndex);
		Grid->SetCellValue( ActorIndex, SM_ACTOR, *Actor->GetName() );
		Grid->SetCellValue( ActorIndex, SM_TAG, *(Actor->Tag.ToString()) );
		Grid->SetCellValue( ActorIndex, SM_TYPE, *Actor->GetClass()->GetName() );
		Grid->SetCellValue( ActorIndex, SM_GROUPS, *(Actor->Group.ToString()) );
		if (Actor->Layer)
		{
			Grid->SetCellValue( ActorIndex, SM_LAYER, *(Actor->Layer->Desc) );
		}

		if (Actor->Base)
		{
			Grid->SetCellValue( ActorIndex, SM_ATTACHMENT, *Actor->Base->GetName() );
		}

		const FString str = FString::Printf( TEXT("%.3f, %.3f, %.3f"), Actor->Location.X, Actor->Location.Y, Actor->Location.Z);
		Grid->SetCellValue( ActorIndex, SM_LOCATION, *str );

		// Get the kismet sequence for the level the actor belongs to.
		USequence* RootSeq = GWorld->GetGameSequence( Actor->GetLevel() );
		if ( RootSeq && RootSeq->ReferencesObject(Actor) )
		{
			Grid->SetCellValue( ActorIndex, SM_KISMET, TEXT("TRUE") );
		}

		//if ( Actor->GetOuter() && Actor->GetOuter()->GetOuter() && Actor->GetOuter()->GetOuter()->GetOuter() )
		{
			//const UPackage* pkg = (UPackage*)Actor->GetOuter()->GetOuter()->GetOuter();
			const UPackage* Package = Cast<UPackage>( Actor->GetOutermost() );
			if ( Package )
			{
				Grid->SetCellValue( ActorIndex, SM_PACKAGE, *Package->GetName() );
			}
		}
	}
	//debugf( TEXT("%s Update() - %i ms"), TEXT("CellValues"), appTrunc((appSeconds() - UpdateStartTime3) * 1000) );
	//DOUBLE UpdateStartTime4 = appSeconds();		
	
	// PCF Begin
	/* Deprecated
	Grid->AutoSize();
	SetGridSize();
	//debugf( TEXT("%s Update() - %i ms"), TEXT("Autosize"), appTrunc((appSeconds() - UpdateStartTime4) * 1000) );
	// Restore selected rows
	for ( UINT i = 0 ; i < selRows.Count() ; ++i )
	{
		Grid->SelectRow( selRows[i], true );
	}

	LastRow = (selRows.Count() > 0)? selRows[0] : -1;
	*/

	// restore grid position
	Grid->RestoreGridPosition();
	// PCF End

}

void WxSceneManager::GetSelectedActors(TArray<UObject*>& OutSelectedActors)
{
	if( bAreWindowsInitialized && GridActors.Num() > 0 )
	{
		// Collect selected rows
		wxArrayInt SelectedRows = Grid->GetSelectedRows();
		for( UINT RowIndex = 0 ; RowIndex < SelectedRows.Count() ; ++RowIndex )
		{	
			if ( SelectedRows[RowIndex] < GridActors.Num() )
			{
				OutSelectedActors.AddItem( GridActors(SelectedRows[RowIndex]) );
			}
		}

		// Collect rows in a selected block of cells
		const wxGridCellCoordsArray TopLeft = Grid->GetSelectionBlockTopLeft();
		const wxGridCellCoordsArray BottomRight = Grid->GetSelectionBlockBottomRight();

		if ( TopLeft.Count() && BottomRight.Count() )
		{
			for( INT i = TopLeft[0].GetRow() ; i <= BottomRight[0].GetRow() ; ++i )
			{	
				if ( i < GridActors.Num() )
				{
					OutSelectedActors.AddItem( GridActors(i) );
				}
			}
		}

		// Collect selected cells
		const wxGridCellCoordsArray Cells = Grid->GetSelectedCells();
		for( UINT c = 0 ; c < Cells.Count() ; ++c )
		{		
			const INT RowIndex = Cells[c].GetRow();
			if ( RowIndex < GridActors.Num() )
			{
				OutSelectedActors.AddItem( GridActors(RowIndex) );
			}
		}
	}
}

void WxSceneManager::PopulatePropPanel()
{
	if( bAreWindowsInitialized )
	{
		// Check if the set of selected actors has changed.
		TArray<UObject*> NewSelectedActors;
		GetSelectedActors( NewSelectedActors );

		UBOOL bActorsChanged = FALSE;

		if ( NewSelectedActors.Num() != SelectedActors.Num() )
		{
			bActorsChanged = TRUE;
		}
		else
		{
			for( INT i = 0 ; i < SelectedActors.Num() ; i++ )
			{
				if ( SelectedActors(i) != NewSelectedActors(i) )
				{
					bActorsChanged = TRUE;
					break;
				}
			}
		}

		if ( bActorsChanged && NewSelectedActors.Num() > 0 )
		{
			PropertyWindow->SetObjectArray( NewSelectedActors, 1,1,1 );
			PropertyWindow->Show( TRUE );
		}

		SelectedActors = NewSelectedActors;
	}
}

// PCF Begin
/* Deprecated - this is automatic in wxWidget
void WxSceneManager::SetGridSize()
{
	if( bAreWindowsInitialized )
	{
		const wxRect rcSplitter = SplitterWindow->GetClientRect();
		wxRect rcGrid = Grid->GetClientRect();

		rcGrid.width = SplitterWindow->GetSashPosition();
		rcGrid.height = rcSplitter.height;
		Grid->SetSize( rcGrid );
	}
}

void WxSceneManager::SetPropsSize()
{
	if( bAreWindowsInitialized )

	{
		const wxRect rcSplitter = SplitterWindow->GetClientRect();
		const INT SplitPos = SplitterWindow->GetSashPosition();
		wxRect rcProp = PropertyWindow->GetClientRect();

		rcProp.width = rcSplitter.width - SplitPos;
		rcProp.height = rcSplitter.height;
		PropertyWindow->SetSize( rcProp );
	}
}





void WxSceneManager::OnSize( wxSizeEvent& In )
{
	if( bAreWindowsInitialized )
	{
		const INT ToolBarHeight = 45;
		const wxRect rc = GetClientRect();

		// Set splitter window dimensions.
		wxRect rcSplitter = SplitterWindow->GetClientRect();
		rcSplitter.width = rc.width - 10;
		rcSplitter.height = rc.height - ToolBarHeight * 2;
		SplitterWindow->SetSize( rcSplitter );

		// Set grid size.
		SetGridSize();

		// Set panel size.
		Panel->SetSize( 0, 0, rc.GetWidth(), rc.GetHeight() );
	}
}

void WxSceneManager::OnSashPositionChange(wxSplitterEvent& In)
{
	SetGridSize();
	SetPropsSize();
}
*/

/**
*  Populate level list based on world info and mark ActiveLevels as selected
*/
void WxSceneManager::PopulateLevelList()
{
	Level_List->Clear();
	if (GWorld)
	{
		INT item = 0;

		// add persistent level
		AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
		Level_List->Append( *WorldInfo->GetLevel()->GetName() );
		
		// select level if is active
		if ( ActiveLevels.FindItem(WorldInfo->GetLevel(), item) )
		{
			Level_List->SetSelection(0);
		}

		// add streaming levels
		for( INT LevelIndex = 0 ; LevelIndex < WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
		{
			ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels(LevelIndex);
			if( StreamingLevel && StreamingLevel->LoadedLevel )
			{
				Level_List->Append(*StreamingLevel->PackageName.ToString());				
		
				// select level if is active			
				if ( ActiveLevels.FindItem(StreamingLevel->LoadedLevel, item) )
				{
					Level_List->SetSelection(LevelIndex+1);
				}
			}
		}
		
	}
}
// PCF End


void WxSceneManager::Update()
{
	BeginUpdate();

	Grid->ClearSelection();

	if ( IsShown() )
	{	
		//DOUBLE UpdateStartTime0 = appSeconds();
		// PCF Begin
		PopulateLevelList();
		// PCF End
		PopulateCombo( ActiveLevels );
		//debugf( TEXT("%s Update() - %i ms"), TEXT("PropCombo"), appTrunc((appSeconds() - UpdateStartTime0) * 1000) );
		//DOUBLE UpdateStartTime1 = appSeconds();
		PopulateGrid( ActiveLevels );
		//debugf( TEXT("%s Update() - %i ms"), TEXT("Grid"), appTrunc((appSeconds() - UpdateStartTime1) * 1000) );
		PopulatePropPanel();
		bUpdateOnActivated = FALSE;
	}
	else
	{
		bUpdateOnActivated = TRUE;
	}

	EndUpdate();
}

void WxSceneManager::Activated()
{
	WxBrowser::Activated();
	if ( bUpdateOnActivated )
	{
		Update();
	}
}
// PCF Begin
/* Deprecated - OnGridRangeSelect supporting this
void WxSceneManager::OnGridSelectCell( wxGridEvent& event )
{
	if( bAreWindowsInitialized )
	{
		if ( event.Selecting() )
		{
			Grid->SelectBlock( event.GetRow(), event.GetCol(), event.GetRow(), event.GetCol() );
			PopulatePropPanel();
		}
	}
}
*/

void WxSceneManager::OnGridRangeSelect( wxGridRangeSelectEvent& event )
{
	/* Deprecated 
	if ( event.ShiftDown() )
		{
			if ( LastRow != -1 )
			{
				Grid->SelectBlock( LastRow, event.GetLeftCol(), event.GetBottomRow(), event.GetRightCol() );
			}
		}
		else
		*/
	if ( bAutoFocus )
	{
		FocusOnSelected();
	}
	PopulatePropPanel();
}
// PCF End

void WxSceneManager::OnLabelLeftClick( wxGridEvent& event )
{
	if ( event.GetCol() > -1 && 
		 SortColumn != event.GetCol() &&
		 event.GetCol() != SM_LOCATION )
	{
		SortColumn = event.GetCol();
		PopulateGrid( ActiveLevels );
	}

	if ( event.GetRow() > -1 )
	{
		event.Skip();
	}
}

void WxSceneManager::OnCellChange( wxGridEvent& event )
{
}

/** Handler for IDM_RefreshBrowser events; updates the browser contents. */
void WxSceneManager::OnRefresh( wxCommandEvent& In )
{
	// PCF Begin
	// backup selection
	// http://wxforum.shadonet.com/viewtopic.php?t=5618 // GetSelectedRows bug
	wxGridCellCoordsArray tl=Grid->GetSelectionBlockTopLeft();
	wxGridCellCoordsArray br=Grid->GetSelectionBlockBottomRight();

	Update();
	
	// restore selection
	for (UINT i=0; i<tl.GetCount(); i++)
		Grid->SelectBlock(tl[i],br[i],true);
	// PCF End
}

void WxSceneManager::OnFileOpen( wxCommandEvent& In )
{
	WxFileDialog OpenFileDialog( this, 
		*LocalizeUnrealEd("OpenPackage"), 
		*appScriptOutputDir(),
		TEXT(""),
		TEXT("Class Packages (*.u)|*.u|All Files|*.*\0\0"),
		wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY | wxMULTIPLE,
		wxDefaultPosition);
	/*
	if( OpenFileDialog.ShowModal() == wxID_OK )
	{
		wxArrayString	OpenFilePaths;
		OpenFileDialog.GetPaths(OpenFilePaths);

		for(UINT FileIndex = 0;FileIndex < OpenFilePaths.Count();FileIndex++)
		{
			UObject::LoadPackage( NULL, *FString(OpenFilePaths[FileIndex]), 0 );
		}
	}

	GCallback->Send( CALLBACK_RefreshEditor, ERefreshEditor_AllBrowsers );
	Update();
	*/
}

void WxSceneManager::FocusOnSelected()
{
	TArray<UObject*> TempSelectedActors;
	GetSelectedActors( TempSelectedActors );

	if ( TempSelectedActors.Num() > 0 )
	{
		GEditor->SelectNone( FALSE, TRUE );
		TArray<AActor*> Actors;

		for ( INT SelectedActorIndex = 0 ; SelectedActorIndex < TempSelectedActors.Num() ; ++SelectedActorIndex )
		{
			AActor* Actor = (AActor *) TempSelectedActors( SelectedActorIndex );

			if( Actor )
			{
				GEditor->SelectActor( Actor, TRUE, NULL, TRUE );
				Actors.AddItem( Actor );
			}
		}

		// If there were any non-null actors in the temp list of actors, then focus on them.
		if ( Actors.Num() )
		{
			GUnrealEd->MoveViewportCamerasToActor( Actors , FALSE );
		}
	}
}

/**
 * Sets the set of levels to visualize the scene manager.
 */
void WxSceneManager::SetActiveLevels(const TArray<ULevel*>& InActiveLevels)
{
	ActiveLevels = InActiveLevels;
	Update();
}

void WxSceneManager::OnAutoFocus( wxCommandEvent& In )
{
	bAutoFocus = !bAutoFocus;
}

void WxSceneManager::OnFocus( wxCommandEvent& In )
{
	FocusOnSelected();
}

// PCF Begin
void WxSceneManager::DeleteSelectedActors()
{
	FocusOnSelected();
	GEditor->Exec( TEXT("DELETE") );
	/* Deprecated - update is automatic in callback after exectuion "OnGridRangeSelect"
	  PopulateGrid( ActiveLevels );
	  PopulatePropPanel();
	*/
}

void WxSceneManager::OnDelete( wxCommandEvent& In )
{
	DeleteSelectedActors();
}
// PCF End

void WxSceneManager::OnTypeFilterSelected( wxCommandEvent& event )
{
	TypeFilter_Selection = event.m_commandInt;
	Grid->ClearSelection();
	PopulateGrid( ActiveLevels );
}

void WxSceneManager::OnShowBrushes( wxCommandEvent& event )
{
	Grid->ClearSelection();
	PopulateGrid( ActiveLevels );
}

void WxSceneManager::OnNameFilterChanged( wxCommandEvent& In )
{
	NameFilter = In.GetString();
	PopulateGrid( ActiveLevels );
}

// PCF Begin
void WxSceneManager::OnLevelSelected( wxCommandEvent& event )
{
	// build new level list based on selection
	GetSelectedLevelsFromList(ActiveLevels);

	// refresh controls
	PopulateCombo( ActiveLevels );
	PopulateGrid( ActiveLevels );
	PopulatePropPanel();
}

/** 
* Build levels array based on names of selected levels from list.
*/
void WxSceneManager::GetSelectedLevelsFromList(TArray<ULevel*>& InLevels)
{
	InLevels.Empty();

	if (GWorld)
	{
		wxArrayInt Selection;
		Level_List->GetSelections(Selection);
		for (UINT i=0; i<Selection.Count(); i++)
		{
			INT idx = Selection.Item(i);
			wxString LevelName = Level_List->GetString(idx);				
			
			// is persistent level?
			AWorldInfo*	WorldInfo = GWorld->GetWorldInfo();
			if (appStrcmp(*WorldInfo->GetLevel()->GetName(),LevelName) == 0)
			{
				InLevels.AddItem(WorldInfo->GetLevel());
				continue;
			}
			
			// is streaming level?
			for( INT LevelIndex = 0 ; LevelIndex < WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
			{
				ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels( LevelIndex );
				if( StreamingLevel && StreamingLevel->LoadedLevel )
				{
					if (appStrcmp(*StreamingLevel->PackageName.ToString(), LevelName) == 0)
					{
						InLevels.AddItem(StreamingLevel->LoadedLevel);
						break;
					}
				}
			}

		}
	}

}

// PCF End

void WxSceneManager::Send(ECallbackEventType Event)
{
	PropertyWindow->GetRoot()->RemoveAllObjects();
	PropertyWindow->Show( FALSE );
	GridActors.Empty();
	SelectedActors.Empty();		

	// PCF Begin
	// ActiveLevels.Empty(); deprecated - moved to GetSelectedLevelsFromList
	// restore ActiveLevels array based on selected levels from Level_List
	 GetSelectedLevelsFromList(ActiveLevels);

	// clear list when GWorld is empty (after map loading)
	if (!GWorld)
	{
		Level_List->Clear();
	}
	// PCF End

	Update();
}

/**
 * Adds entries to the browser's accelerator key table.  Derived classes should call up to their parents.
 */
void WxSceneManager::AddAcceleratorTableEntries(TArray<wxAcceleratorEntry>& Entries)
{
	// PCF
	/* Deprecated - moved to internal handler in WxDelGrid
	WxBrowser::AddAcceleratorTableEntries( Entries );
	Entries.AddItem( wxAcceleratorEntry( wxACCEL_NORMAL, WXK_DELETE, IDMN_SCENEMANAGER_DELETE ) );
	*/
}

/**
 * Since this class holds onto an object reference, it needs to be serialized
 * so that the objects aren't GCed out from underneath us.
 *
 * @param	Ar			The archive to serialize with.
 */
void WxSceneManager::Serialize(FArchive& Ar)
{
	Ar << GridActors;
	Ar << ActiveLevels;
}
