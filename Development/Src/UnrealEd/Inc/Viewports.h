/*=============================================================================
	Viewports.h: The viewport windows used by the editor
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __VIEWPORTS_H__
#define __VIEWPORTS_H__

class WxViewportsContainer;

enum EViewportConfig
{ 
	VC_None			= -1,
	VC_2_2_Split,
	VC_1_2_Split,
	VC_1_1_Split,
	VC_Max
};


/**
 * The default values for a viewport within an FViewportConfig_Template.
 */
class FViewportConfig_Viewport
{
public:
	FViewportConfig_Viewport();
	virtual ~FViewportConfig_Viewport();

	ELevelViewportType ViewportType;
	EShowFlags ShowFlags;

	/** If FALSE, this viewport template is not being used within its owner config template. */
	UBOOL bEnabled;
};

/**
 * A template for a viewport configuration.  Holds the baseline layouts and flags for a config.
 */
class FViewportConfig_Template
{
public:
	FViewportConfig_Template();
	virtual ~FViewportConfig_Template();
	
	void Set( EViewportConfig InViewportConfig );

	/** The description for this configuration. */
	FString Desc;

	/** The viewport config this template represents. */
	EViewportConfig ViewportConfig;

	/** The viewport templates within this config template. */
	FViewportConfig_Viewport ViewportTemplates[4];
};

/**
 * An instance of a FViewportConfig_Template.  There is only one of these
 * in use at any given time and it represents the current editor viewport
 * layout.  This contains more information than the template (i.e. splitter 
 * and camera locations).
 */
class FVCD_Viewport : public FViewportConfig_Viewport
{
public:
	FVCD_Viewport();
	virtual ~FVCD_Viewport();

	EShowFlags ShowFlags;

	/** Stores the sash position from the INI until we can use it in FViewportConfig_Data::Apply. */
	FLOAT SashPos;

	/** The window that holds the viewport itself. */
	class WxLevelViewportWindow* ViewportWindow;

	inline FVCD_Viewport operator=( const FViewportConfig_Viewport &Other )
	{
		ViewportType = Other.ViewportType;
		ShowFlags = Other.ShowFlags;
		bEnabled = Other.bEnabled;

		return *this;
	}
};

class FViewportConfig_Data
{
public:
	FViewportConfig_Data();
	virtual ~FViewportConfig_Data();

	void	SetTemplate( EViewportConfig InTemplate );
	void	Apply( WxViewportsContainer* InParent );
	void	ResizeProportionally( FLOAT ScaleX, FLOAT ScaleY, UBOOL bRedraw=TRUE );
	void	ToggleMaximize( FViewport* Viewport );
	void	Layout();
	void	Save();
	void	Load( FViewportConfig_Data* InData );
	void	SaveToINI();
	UBOOL	LoadFromINI();
	UBOOL	IsViewportMaximized();

	/** Helper functions */
	INT		FindSplitter( wxWindow* ContainedWindow, INT *WhichWindow=NULL );

	/** The splitters windows that make this config possible. */
	TArray<wxSplitterWindow*> SplitterWindows;

	/** Maximized viewport (-1 if none) */
	INT				MaximizedViewport;

	/** The top level sizer for the viewports. */
	wxBoxSizer*		Sizer;

	/** The template this instance is based on. */
	EViewportConfig	Template;

	/** The overriding viewport data for this instance. */
	FVCD_Viewport	Viewports[4];

	inline FViewportConfig_Data operator=( const FViewportConfig_Template &Other )
	{
		for( INT x = 0 ; x < 4 ; ++x )
		{
			Viewports[x] = Other.ViewportTemplates[x];
		}

		return *this;
	}
};

/**
 * Contains a level editing viewport.
 */
class WxLevelViewportWindow : public wxWindow, public FEditorLevelViewportClient
{
public:
	class WxLevelViewportToolBar* ToolBar;

	WxLevelViewportWindow();
	~WxLevelViewportWindow();

	void SetUp( INT InViewportNum, ELevelViewportType InViewportType, EShowFlags InShowFlags );
	virtual UBOOL InputAxis(FViewport* Viewport,INT ControllerId,FName Key,FLOAT Delta,FLOAT DeltaTime);

private:
	void OnSize( wxSizeEvent& InEvent );
	void OnSetFocus( wxFocusEvent& In );

	DECLARE_EVENT_TABLE()
};

/**
 * A panel window that holds a viewport inside of it.  This class is used
 * to ease the use of splitters and other things that want wxWindows, not
 * UViewports.
 */
class WxViewportHolder : public wxPanel
{
public:
	WxViewportHolder(wxWindow* InParent, wxWindowID InID, bool InWantScrollBar, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL);
	virtual ~WxViewportHolder();

	/** The viewport living inside of this holder. */
	FViewport* Viewport;

	/** An optional scroll bar. */
	wxScrollBar* ScrollBar;

	/** Vars for controlling the scrollbar. */
	INT SBPos, SBRange;

	/** if TRUE, will call CloseViewport() when this window is destroyed */
	UBOOL bAutoDestroy;

	void SetViewport( FViewport* InViewport );
	void UpdateScrollBar( INT InPos, INT InRange );
	INT GetScrollThumbPos();
	void SetScrollThumbPos( INT InPos );

	/** 
	 * Scrolls the viewport up or down to a specified location. Also supports
	 * relative position scrolling
	 *
	 * @param Pos the position to scroll to or the amount to scroll by
	 * @param bIsDelta whether to apply a relative scroll or not
	 */
	void ScrollViewport(INT Pos,UBOOL bIsDelta = TRUE)
	{
		if (ScrollBar != NULL)
		{
			// If this is true, scroll relative to the current scroll location
			if (bIsDelta == TRUE)
			{
				Pos += GetScrollThumbPos();
			}
			// Set the new scroll position and invalidate
			SetScrollThumbPos(Pos);
			Viewport->Invalidate();
		}
	}

private:
	void OnSize( wxSizeEvent& InEvent );

	DECLARE_EVENT_TABLE()
};

#endif // __VIEWPORTS_H__
