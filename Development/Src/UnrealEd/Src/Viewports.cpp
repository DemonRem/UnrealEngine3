/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "UnrealEd.h"
#include "LevelViewportToolBar.h"
#include "ViewportsContainer.h"

/*-----------------------------------------------------------------------------
	FViewportConfig_Viewport.
-----------------------------------------------------------------------------*/

FViewportConfig_Viewport::FViewportConfig_Viewport()
{
	ViewportType = LVT_Perspective;
	ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_BrushWireframe | SHOW_ModeWidgets;
	bEnabled = 0;
}

FViewportConfig_Viewport::~FViewportConfig_Viewport()
{
}

/*-----------------------------------------------------------------------------
	FViewportConfig_Template.
-----------------------------------------------------------------------------*/

FViewportConfig_Template::FViewportConfig_Template()
{
}

FViewportConfig_Template::~FViewportConfig_Template()
{
}

void FViewportConfig_Template::Set( EViewportConfig InViewportConfig )
{
	ViewportConfig = InViewportConfig;

	switch( ViewportConfig )
	{
		case VC_2_2_Split:

			Desc = *LocalizeUnrealEd("2x2Split");

			ViewportTemplates[0].bEnabled = 1;
			ViewportTemplates[0].ViewportType = LVT_OrthoXY;
			ViewportTemplates[0].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_BrushWireframe;

			ViewportTemplates[1].bEnabled = 1;
			ViewportTemplates[1].ViewportType = LVT_OrthoXZ;
			ViewportTemplates[1].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_BrushWireframe;

			ViewportTemplates[2].bEnabled = 1;
			ViewportTemplates[2].ViewportType = LVT_OrthoYZ;
			ViewportTemplates[2].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_BrushWireframe;

			ViewportTemplates[3].bEnabled = 1;
			ViewportTemplates[3].ViewportType = LVT_Perspective;
			ViewportTemplates[3].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_Lit;

			break;

		case VC_1_2_Split:

			Desc = *LocalizeUnrealEd("1x2Split");

			ViewportTemplates[0].bEnabled = 1;
			ViewportTemplates[0].ViewportType = LVT_Perspective;
			ViewportTemplates[0].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_Lit;

			ViewportTemplates[1].bEnabled = 1;
			ViewportTemplates[1].ViewportType = LVT_OrthoXY;
			ViewportTemplates[1].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_BrushWireframe;

			ViewportTemplates[2].bEnabled = 1;
			ViewportTemplates[2].ViewportType = LVT_OrthoXZ;
			ViewportTemplates[2].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_BrushWireframe;

			break;

		case VC_1_1_Split:

			Desc = *LocalizeUnrealEd("1x1Split");

			ViewportTemplates[0].bEnabled = 1;
			ViewportTemplates[0].ViewportType = LVT_Perspective;
			ViewportTemplates[0].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_Lit;

			ViewportTemplates[1].bEnabled = 1;
			ViewportTemplates[1].ViewportType = LVT_OrthoXY;
			ViewportTemplates[1].ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_BrushWireframe;

			break;

		default:
			check(0);	// Unknown viewport config
			break;
	}
}

/*-----------------------------------------------------------------------------
	FVCD_Viewport.
-----------------------------------------------------------------------------*/

FVCD_Viewport::FVCD_Viewport()
{
	ViewportWindow = NULL;
	ShowFlags = (SHOW_DefaultEditor&~SHOW_ViewMode_Mask) | SHOW_ViewMode_BrushWireframe | SHOW_ModeWidgets;

	SashPos = 0;
}

FVCD_Viewport::~FVCD_Viewport()
{
}

/*-----------------------------------------------------------------------------
	FViewportConfig_Data.
-----------------------------------------------------------------------------*/

FViewportConfig_Data::FViewportConfig_Data()
{
	MaximizedViewport = -1;
}

FViewportConfig_Data::~FViewportConfig_Data()
{
}

/**
 * Tells this viewport configuration to create its windows and apply its settings.
 */
void FViewportConfig_Data::Apply( WxViewportsContainer* InParent )
{
	MaximizedViewport = -1;
	SplitterWindows.Empty();

	for( INT x = 0 ; x < 4 ; ++x )
	{
		if( Viewports[x].bEnabled && Viewports[x].ViewportWindow != NULL )
		{
			Viewports[x].ViewportWindow->DestroyChildren();
			Viewports[x].ViewportWindow->Destroy();
			Viewports[x].ViewportWindow = NULL;
		}
	}

	wxRect rc = InParent->GetClientRect();

	// Set up the splitters and viewports as per the template defaults.

	wxSplitterWindow *MainSplitter = NULL;
	INT SashPos = 0;
	FString Key;

	switch( Template )
	{
		case VC_2_2_Split:
		{
			wxSplitterWindow *TopSplitter, *BottomSplitter;

			MainSplitter = new wxSplitterWindow( InParent, ID_SPLITTERWINDOW, wxPoint(0,0), wxSize(rc.GetWidth(), rc.GetHeight()), wxSP_3D );
			TopSplitter = new wxSplitterWindow( MainSplitter, ID_SPLITTERWINDOW+1, wxPoint(0,0), wxSize(rc.GetWidth(), rc.GetHeight()/2), wxSP_3D );
			BottomSplitter = new wxSplitterWindow( MainSplitter, ID_SPLITTERWINDOW+2, wxPoint(0,0), wxSize(rc.GetWidth(), rc.GetHeight()/2), wxSP_3D );

			SplitterWindows.AddItem( MainSplitter );
			SplitterWindows.AddItem( TopSplitter );
			SplitterWindows.AddItem( BottomSplitter );

			// Connect splitter events so we can link the top and bottom splitters if the ViewportResizeTogether option is set.
			InParent->ConnectSplitterEvents( TopSplitter, BottomSplitter );

			for( INT x = 0 ; x < 4 ; ++x )
			{
				if( Viewports[x].bEnabled )
				{
					Viewports[x].ViewportWindow = new WxLevelViewportWindow;
					Viewports[x].ViewportWindow->Create( MainSplitter, -1 );
					Viewports[x].ViewportWindow->SetUp( x, Viewports[x].ViewportType, Viewports[x].ShowFlags );
					Viewports[x].ViewportWindow->SetTitle( wxT("LevelViewport") );
				}
			}

			Viewports[0].ViewportWindow->Reparent( TopSplitter );
			Viewports[1].ViewportWindow->Reparent( TopSplitter );
			Viewports[2].ViewportWindow->Reparent( BottomSplitter );
			Viewports[3].ViewportWindow->Reparent( BottomSplitter );

			MainSplitter->SetTitle( wxT("MainSplitter") );
			TopSplitter->SetTitle( wxT("TopSplitter") );
			BottomSplitter->SetTitle( wxT("BottomSplitter") );

			GConfig->GetInt( TEXT("ViewportConfig"), TEXT("Splitter0"), SashPos, GEditorIni );
			MainSplitter->SplitHorizontally( TopSplitter, BottomSplitter, SashPos );
			GConfig->GetInt( TEXT("ViewportConfig"), TEXT("Splitter1"), SashPos, GEditorIni );
			TopSplitter->SplitVertically( Viewports[0].ViewportWindow, Viewports[1].ViewportWindow, SashPos );
			GConfig->GetInt( TEXT("ViewportConfig"), TEXT("Splitter2"), SashPos, GEditorIni );
			BottomSplitter->SplitVertically( Viewports[3].ViewportWindow, Viewports[2].ViewportWindow, SashPos );
		}
		break;

		case VC_1_2_Split:
		{
			wxSplitterWindow *RightSplitter;

			MainSplitter = new wxSplitterWindow( InParent, ID_SPLITTERWINDOW, wxPoint(0,0), wxSize(rc.GetWidth(), rc.GetHeight()), wxSP_3D );
			RightSplitter = new wxSplitterWindow( MainSplitter, ID_SPLITTERWINDOW+1, wxPoint(0,0), wxSize(rc.GetWidth(), rc.GetHeight()/2), wxSP_3D );

			SplitterWindows.AddItem( MainSplitter );
			SplitterWindows.AddItem( RightSplitter );

			// Disconnect Splitter Events
			InParent->DisconnectSplitterEvents();

			for( INT x = 0 ; x < 4 ; ++x )
			{
				if( Viewports[x].bEnabled )
				{
					Viewports[x].ViewportWindow = new WxLevelViewportWindow;
					Viewports[x].ViewportWindow->Create( MainSplitter, -1 );
					Viewports[x].ViewportWindow->SetUp( x, Viewports[x].ViewportType, Viewports[x].ShowFlags );
					Viewports[x].ViewportWindow->SetTitle( wxT("LevelViewport") );
				}
			}

			Viewports[0].ViewportWindow->Reparent( MainSplitter );
			Viewports[1].ViewportWindow->Reparent( RightSplitter );
			Viewports[2].ViewportWindow->Reparent( RightSplitter );

			MainSplitter->SetTitle( wxT("MainSplitter") );
			RightSplitter->SetTitle( wxT("RightSplitter") );

			GConfig->GetInt( TEXT("ViewportConfig"), TEXT("Splitter0"), SashPos, GEditorIni );
			MainSplitter->SplitVertically( Viewports[0].ViewportWindow, RightSplitter, SashPos );
			GConfig->GetInt( TEXT("ViewportConfig"), TEXT("Splitter1"), SashPos, GEditorIni );
			RightSplitter->SplitHorizontally( Viewports[1].ViewportWindow, Viewports[2].ViewportWindow, SashPos );
		}
		break;

		case VC_1_1_Split:
		{
			MainSplitter = new wxSplitterWindow( InParent, ID_SPLITTERWINDOW, wxPoint(0,0), wxSize(rc.GetWidth(), rc.GetHeight()), wxSP_3D );

			SplitterWindows.AddItem( MainSplitter );

			// Disconnect Splitter Events
			InParent->DisconnectSplitterEvents();

			for( INT x = 0 ; x < 4 ; ++x )
			{
				if( Viewports[x].bEnabled )
				{
					Viewports[x].ViewportWindow = new WxLevelViewportWindow;
					Viewports[x].ViewportWindow->Create( MainSplitter, -1 );
					Viewports[x].ViewportWindow->SetUp( x, Viewports[x].ViewportType, Viewports[x].ShowFlags );
					Viewports[x].ViewportWindow->SetTitle( wxT("LevelViewport") );
				}
			}

			Viewports[0].ViewportWindow->Reparent( MainSplitter );
			Viewports[1].ViewportWindow->Reparent( MainSplitter );

			MainSplitter->SetTitle( wxT("MainSplitter") );

			GConfig->GetInt( TEXT("ViewportConfig"), TEXT("Splitter0"), SashPos, GEditorIni );
			MainSplitter->SplitHorizontally( Viewports[0].ViewportWindow, Viewports[1].ViewportWindow, SashPos );
		}
		break;
	}

	// Make sure the splitters will resize with the editor
	wxBoxSizer* WkSizer = new wxBoxSizer( wxHORIZONTAL );
	WkSizer->Add( MainSplitter, 1, wxEXPAND | wxALL, 0 );

	Sizer = new wxBoxSizer( wxVERTICAL );
	Sizer->Add( WkSizer, 1, wxEXPAND | wxALL, 0 );
	InParent->SetSizer( Sizer );
	
	// Apply the custom settings contained in this instance.

	for( INT x = 0 ; x < 4 ; ++x )
	{
		if( Viewports[x].bEnabled )
		{
			check(Viewports[x].ViewportWindow);
			Viewports[x].ViewportWindow->ViewportType = Viewports[x].ViewportType;
			Viewports[x].ViewportWindow->ShowFlags = Viewports[x].ShowFlags;
			Viewports[x].ViewportWindow->ToolBar->UpdateUI();
		}
	}
}

/*
 * Scales the splitter windows proportionally.
 * Used when switching between windowed and maximized mode.
 */
void FViewportConfig_Data::ResizeProportionally( FLOAT ScaleX, FLOAT ScaleY, UBOOL bRedraw/*=TRUE*/ )
{
	if ( MaximizedViewport >= 0 )
	{
		Layout();
	}
	else
	{
		for( INT x = 0 ; x < SplitterWindows.Num() ; ++x )
		{
			wxSplitterWindow* Splitter = SplitterWindows(x);
			wxSize WindowSize = Splitter->GetSize();
			FLOAT Scale, OldSize;
			if (Splitter->GetSplitMode() == wxSPLIT_HORIZONTAL)
			{
				Scale = ScaleY;
				OldSize = WindowSize.y;
			}
			else
			{
				Scale = ScaleX;
				OldSize = WindowSize.x;
			}
			FLOAT NewSize = FLOAT(OldSize) * Scale;
			FLOAT Proportion = FLOAT(Splitter->GetSashPosition()) / OldSize;
			INT Sash = appTrunc( Proportion * NewSize );
			Splitter->SetSashPosition( Sash, bRedraw == TRUE );
		}
	}
}

/**
 * Sets up this instance with the data from a template.
 */
void FViewportConfig_Data::SetTemplate( EViewportConfig InTemplate )
{
	Template = InTemplate;

	// Find the template

	FViewportConfig_Template* vct = NULL;
	for( INT x = 0 ; x < GApp->EditorFrame->ViewportConfigTemplates.Num() ; ++x )
	{
		FViewportConfig_Template* vctWk = GApp->EditorFrame->ViewportConfigTemplates(x);
		if( vctWk->ViewportConfig == InTemplate )
		{
			vct = vctWk;
			break;
		}
	}

	check( vct );	// If NULL, the template config type is unknown

	// Copy the templated data into our local vars

	*this = *vct;
}

/**
 * Updates custom data elements.
 */

void FViewportConfig_Data::Save()
{
	for( INT x = 0 ; x < 4 ; ++x )
	{
		FVCD_Viewport* viewport = &Viewports[x];

		if( viewport->bEnabled )
		{
			viewport->ViewportType = (ELevelViewportType)viewport->ViewportWindow->ViewportType;

			if(viewport->ViewportWindow->LastSpecialMode != 0)
			{
				viewport->ShowFlags = viewport->ViewportWindow->LastShowFlags;
			}
			else
			{
				viewport->ShowFlags = viewport->ViewportWindow->ShowFlags;
			}
		}
	}
}

/**
 * Loads the custom data elements from InData to this instance.
 *
 * @param	InData	The instance to load the data from.
 */

void FViewportConfig_Data::Load( FViewportConfig_Data* InData )
{
	for( INT x = 0 ; x < 4 ; ++x )
	{
		FVCD_Viewport* Src = &InData->Viewports[x];

		if( Src->bEnabled )
		{
			// Find a matching viewport to copy the data into

			for( INT y = 0 ; y < 4 ; ++y )
			{
				FVCD_Viewport* Dst = &Viewports[y];

				if( Dst->bEnabled && Dst->ViewportType == Src->ViewportType )
				{
					Dst->ViewportType = Src->ViewportType;
					Dst->ShowFlags = Src->ShowFlags;
				}
			}
		}
	}
}

/**
 * Saves out the current viewport configuration to the editors INI file.
 */

void FViewportConfig_Data::SaveToINI()
{
	Save();

	GConfig->EmptySection( TEXT("ViewportConfig"), GEditorIni );
	GConfig->SetInt( TEXT("ViewportConfig"), TEXT("Template"), Template, GEditorIni );

	if ( !IsViewportMaximized() )
	{
		for( INT x = 0 ; x < SplitterWindows.Num() ; ++x )
		{
			FString Key = FString::Printf( TEXT("Splitter%d"), x );
			GConfig->SetInt( TEXT("ViewportConfig"), *Key, SplitterWindows(x)->GetSashPosition(), GEditorIni );
		}
	}

	for( INT x = 0 ; x < 4 ; ++x )
	{
		FVCD_Viewport* viewport = &Viewports[x];

		FString Key = FString::Printf( TEXT("Viewport%d"), x );

		GConfig->SetBool( TEXT("ViewportConfig"), *(Key+TEXT("_Enabled")), viewport->bEnabled, GEditorIni );

		if( viewport->bEnabled )
		{
			GConfig->SetInt( TEXT("ViewportConfig"), *(Key+TEXT("_ViewportType")), viewport->ViewportType, GEditorIni );

			INT LowerFlags = viewport->ShowFlags & 0xFFFFFFFF;
			INT UpperFlags = viewport->ShowFlags >> 32;
			GConfig->SetInt( TEXT("ViewportConfig"), *(Key+TEXT("_ShowFlagsLower")), LowerFlags, GEditorIni );
			GConfig->SetInt( TEXT("ViewportConfig"), *(Key+TEXT("_ShowFlagsUpper")), UpperFlags, GEditorIni );
		}
	}
}

/**
 * Attempts to load the viewport configuration from the editors INI file.  If unsuccessful,
 * it returns 0 to the caller.
 */
UBOOL FViewportConfig_Data::LoadFromINI()
{
	INT Wk = VC_None;
	GConfig->GetInt( TEXT("ViewportConfig"), TEXT("Template"), Wk, GEditorIni );

	if( Wk == VC_None )
	{
		return 0;
	}

	Template = (EViewportConfig)Wk;
	GApp->EditorFrame->ViewportConfigData->SetTemplate( Template );

	for( INT x = 0 ; x < 4 ; ++x )
	{
		FVCD_Viewport* viewport = &Viewports[x];

		FString Key = FString::Printf( TEXT("Viewport%d"), x );

		GConfig->GetBool( TEXT("ViewportConfig"), *(Key+TEXT("_Enabled")), viewport->bEnabled, GEditorIni );

		if( viewport->bEnabled )
		{
			GConfig->GetInt( TEXT("ViewportConfig"), *(Key+TEXT("_ViewportType")), Wk, GEditorIni );
			viewport->ViewportType = (ELevelViewportType)Wk;

			DWORD UpperFlags, LowerFlags;
			GConfig->GetInt( TEXT("ViewportConfig"), *(Key+TEXT("_ShowFlagsUpper")), (INT&) UpperFlags, GEditorIni );
			GConfig->GetInt( TEXT("ViewportConfig"), *(Key+TEXT("_ShowFlagsLower")), (INT&) LowerFlags, GEditorIni );
			viewport->ShowFlags = ((QWORD)LowerFlags) | (((QWORD)UpperFlags) << 32);
			viewport->ShowFlags |= SHOW_ModeWidgets;

			// Remove postprocess on non-perspective viewports
			if(viewport->ShowFlags & SHOW_PostProcess && viewport->ViewportType != LVT_Perspective)
				viewport->ShowFlags &= ~SHOW_PostProcess;

			// Always clear the StreamingBounds flag at start-up
			viewport->ShowFlags &= ~SHOW_StreamingBounds;
		}
	}

	GApp->EditorFrame->ViewportConfigData->Load( this );
	GApp->EditorFrame->ViewportConfigData->Apply( GApp->EditorFrame->ViewportContainer );

	return 1;
}

/**
 * Either resizes all viewports so that the specified Viewport is fills the entire editor window,
 * or restores all viewports to their previous sizes.
 * WxEditorFrame will lock all splitter drag bars when a viewport is maximized.
 * This function is called by clicking a button on the viewport toolbar.
 */
void FViewportConfig_Data::ToggleMaximize( FViewport* Viewport )
{
	for( INT x = 0; x < 4 && Viewports[x].ViewportWindow; ++x )
	{
		if ( Viewport == Viewports[x].ViewportWindow->Viewport )
		{
			// Already maximized?
			if ( MaximizedViewport == x )
			{
				// Restore all viewports:

				// Restore all sash positions:
				for( INT n = 0; n < SplitterWindows.Num(); ++n )
				{
					INT SashPos;
					FString Key = FString::Printf( TEXT("Splitter%d"), n );
					if ( GConfig->GetInt( TEXT("ViewportConfig"), *Key, SashPos, GEditorIni ) )
					{
						SplitterWindows(n)->SetSashPosition( SashPos );
					}
				}
				MaximizedViewport = -1;
			}
			else
			{
				// Maximize this viewport:

				// Save sash positions if no other viewport was maximized (should never happen, but anyway)
				if ( MaximizedViewport < 0 )
				{
					for( INT n = 0; n < SplitterWindows.Num(); ++n )
					{
						FString Key = FString::Printf( TEXT("Splitter%d"), n);
						INT SashPos = SplitterWindows(n)->GetSashPosition();
						GConfig->SetInt( TEXT("ViewportConfig"), *Key, SashPos, GEditorIni );
					}
				}

				MaximizedViewport = x;
				Layout();
			}
			break;
		}
	}
}

/**
 * Lets the ViewportConfig layout the splitter windows.
 * If a viewport is maximized, set up all sash positions so that the maximized viewport covers the entire window.
 */
void FViewportConfig_Data::Layout()
{
	if ( IsViewportMaximized() )
	{
		INT TreePath[3] = {0,0,0};	// Maximum 3 splitters
		check( SplitterWindows.Num() <= 3 );
		wxWindow* ContainedWindow = Viewports[MaximizedViewport].ViewportWindow;
		INT WhichWindow;
		INT SplitterIndex = FindSplitter( ContainedWindow, &WhichWindow );
		while( SplitterIndex >= 0 )
		{
			TreePath[SplitterIndex] = WhichWindow;
			ContainedWindow = SplitterWindows(SplitterIndex);
			SplitterIndex = FindSplitter( ContainedWindow, &WhichWindow );
		}
		for( INT n=0; n < SplitterWindows.Num(); ++n )
		{
			wxSplitterWindow* Splitter = SplitterWindows(n);
			wxSize Size = Splitter->GetClientSize();
			INT MaxPos = (Splitter->GetSplitMode() == wxSPLIT_HORIZONTAL) ? Size.y : Size.x;
			if ( TreePath[n] == 1 )
			{
				Splitter->SetSashPosition(MaxPos, FALSE);
			}
			else if ( TreePath[n] == 2 )
			{
				Splitter->SetSashPosition(-MaxPos, FALSE);
			}
			else
			{
				Splitter->SetSashPosition(0, FALSE);
			}
			Splitter->UpdateSize();
		}
	}
}

/**
 * Returns TRUE if a viewport is maximized, otherwise FALSE.
 */
UBOOL FViewportConfig_Data::IsViewportMaximized()
{
	return (MaximizedViewport >= 0);
}

/**
 * Finds which SplitterWindow contains the specified window.
 * Returns the index in the SplitterWindow array, or -1 if not found.
 * It also returns which window it was:
 *    WindowID == 1 if it was the first window (GetWindow1)
 *    WindowID == 2 if it was the second window (GetWindow2)
 * WindowID may be NULL.
 */
INT FViewportConfig_Data::FindSplitter( wxWindow* ContainedWindow, INT *WhichWindow/*=NULL*/ )
{
	// First, find the ViewportWindow:
	INT i;
	wxSplitterWindow* Splitter;
	INT Found=0;
	for( i=0; i < SplitterWindows.Num(); ++i )
	{
		Splitter = SplitterWindows(i);
		if ( ContainedWindow == Splitter->GetWindow1() )
		{
			Found = 1;
			break;
		}
		if ( ContainedWindow == Splitter->GetWindow2() )
		{
			Found = 2;
			break;
		}
	}
	if ( !Found )
	{
		return -1;
	}
	if ( WhichWindow )
	{
		*WhichWindow = Found;
	}
	return i;
}


/*-----------------------------------------------------------------------------
	WxLevelViewportWindow.
-----------------------------------------------------------------------------*/

BEGIN_EVENT_TABLE( WxLevelViewportWindow, wxWindow )
	EVT_SIZE( WxLevelViewportWindow::OnSize )
	EVT_SET_FOCUS( WxLevelViewportWindow::OnSetFocus )
END_EVENT_TABLE()

WxLevelViewportWindow::WxLevelViewportWindow()
	: FEditorLevelViewportClient()
{
	ToolBar = NULL;
	Viewport = NULL;
	bVariableFarPlane = TRUE;
}

WxLevelViewportWindow::~WxLevelViewportWindow()
{
	GEngine->Client->CloseViewport(Viewport);
	Viewport = NULL;
}

void WxLevelViewportWindow::SetUp( INT InViewportNum, ELevelViewportType InViewportType, EShowFlags InShowFlags )
{
	// Set viewport parameters first.  These may be queried by the level viewport toolbar, etc.
	ViewportType = InViewportType;
	ShowFlags = InShowFlags;
	LastShowFlags = InShowFlags;

	// ToolBar
	ToolBar = new WxLevelViewportToolBar( this, -1, this );
	ToolBar->SetTitle( wxT("ToolBar") );

	// Viewport
	Viewport = GEngine->Client->CreateWindowChildViewport( (FViewportClient*)this, (HWND)GetHandle() );
	Viewport->CaptureJoystickInput(false);
	::SetWindowText( (HWND)Viewport->GetWindow(), TEXT("Viewport") );

	wxSizeEvent DummyEvent;
	OnSize( DummyEvent );

	ToolBar->UpdateUI();
}

UBOOL WxLevelViewportWindow::InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime)
{
	return FEditorLevelViewportClient::InputAxis( Viewport, ControllerId, Key, Delta, DeltaTime );
}

void WxLevelViewportWindow::OnSize( wxSizeEvent& InEvent )
{
	if( ToolBar )
	{
		wxRect rc = GetClientRect();
		rc.y += WxLevelViewportToolBar::GetToolbarHeight();
		rc.height -= WxLevelViewportToolBar::GetToolbarHeight();

		ToolBar->SetSize( rc.GetWidth(), WxLevelViewportToolBar::GetToolbarHeight() );
		::SetWindowPos( (HWND)Viewport->GetWindow(), HWND_TOP, rc.GetLeft()+1, rc.GetTop()+1, rc.GetWidth()-2, rc.GetHeight()-2, SWP_SHOWWINDOW );
	}
	InEvent.Skip();
}

void WxLevelViewportWindow::OnSetFocus(wxFocusEvent& In)
{
	if ( Viewport )
	{
		::SetFocus( (HWND) Viewport->GetWindow() );
	}
}

/*-----------------------------------------------------------------------------
	WxViewportHolder
-----------------------------------------------------------------------------*/

BEGIN_EVENT_TABLE( WxViewportHolder, wxPanel )
	EVT_SIZE(WxViewportHolder::OnSize)
END_EVENT_TABLE()

WxViewportHolder::WxViewportHolder( wxWindow* InParent, wxWindowID InID, bool InWantScrollBar, const wxPoint& pos, const wxSize& size, long style)
	: wxPanel( InParent, InID, pos, size, style ), bAutoDestroy(FALSE)
{
	Viewport = NULL;
	ScrollBar = NULL;
	SBPos = SBRange = 0;

	if( InWantScrollBar )
		ScrollBar = new wxScrollBar( this, ID_BROWSER_SCROLL_BAR, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL );
}

WxViewportHolder::~WxViewportHolder()
{
	if ( bAutoDestroy )
	{
		GEngine->Client->CloseViewport(Viewport);
		SetViewport(NULL);
	}
}

void WxViewportHolder::SetViewport( FViewport* InViewport )
{
	Viewport = InViewport;
}

void WxViewportHolder::OnSize( wxSizeEvent& InEvent )
{
	if( Viewport )
	{
		wxRect rc = GetClientRect();
		wxRect rcSB;
		if( ScrollBar )
			rcSB = ScrollBar->GetClientRect();

		SetWindowPos( (HWND)Viewport->GetWindow(), HWND_TOP, rc.GetLeft(), rc.GetTop(), rc.GetWidth()-rcSB.GetWidth(), rc.GetHeight(), SWP_SHOWWINDOW );

		if( ScrollBar )
			ScrollBar->SetSize( rc.GetLeft()+rc.GetWidth()-rcSB.GetWidth(), rc.GetTop(), rcSB.GetWidth(), rc.GetHeight() );
	}

	InEvent.Skip();
}

// Updates the scrollbar so it looks right and is in the right position

void WxViewportHolder::UpdateScrollBar( INT InPos, INT InRange )
{
	if( ScrollBar )
		ScrollBar->SetScrollbar( InPos, Viewport->GetSizeY(), InRange, Viewport->GetSizeY() );
}

INT WxViewportHolder::GetScrollThumbPos()
{
	return ( ScrollBar ? ScrollBar->GetThumbPosition() : 0 );
}

void WxViewportHolder::SetScrollThumbPos( INT InPos )
{
	if( ScrollBar )
		ScrollBar->SetThumbPosition( InPos );
}
