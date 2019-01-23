/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"

static const FLOAT	WidgetSize = 0.15f; // Proportion of the viewport the widget should fill

/** Utility for calculating drag direction when you click on this widget. */
void HWidgetUtilProxy::CalcVectors(FSceneView* SceneView, const FViewportClick& Click, FVector& LocalManDir, FVector& WorldManDir, FLOAT& DragDirX, FLOAT& DragDirY)
{
	if(Axis == AXIS_X)
	{
		WorldManDir = WidgetMatrix.GetAxis(0);
		LocalManDir = FVector(1,0,0);
	}
	else if(Axis == AXIS_Y)
	{
		WorldManDir = WidgetMatrix.GetAxis(1);
		LocalManDir = FVector(0,1,0);
	}
	else
	{
		WorldManDir = WidgetMatrix.GetAxis(2);
		LocalManDir = FVector(0,0,1);
	}

	FVector WorldDragDir = WorldManDir;

	if(bRotWidget)
	{
		if( Abs(Click.GetDirection() | WorldManDir) > KINDA_SMALL_NUMBER ) // If click direction and circle plane are parallel.. can't resolve.
		{
			// First, find actual position we clicking on the circle in world space.
			const FVector ClickPosition = FLinePlaneIntersection(	Click.GetOrigin(),
																	Click.GetOrigin() + Click.GetDirection(),
																	WidgetMatrix.GetOrigin(),
																	WorldManDir );

			// Then find Radial direction vector (from center to widget to clicked position).
			FVector RadialDir = ( ClickPosition - WidgetMatrix.GetOrigin() );
			RadialDir.Normalize();

			// Then tangent in plane is just the cross product. Should always be unit length again because RadialDir and WorlManDir should be orthogonal.
			WorldDragDir = RadialDir ^ WorldManDir;
		}
	}

	// Transform world-space drag dir to screen space.
	FVector ScreenDir = SceneView->ViewMatrix.TransformNormal(WorldDragDir);
	ScreenDir.Z = 0.0f;

	if( ScreenDir.IsZero() )
	{
		DragDirX = 0.0f;
		DragDirY = 0.0f;
	}
	else
	{
		ScreenDir.Normalize();
		DragDirX = ScreenDir.X;
		DragDirY = ScreenDir.Y;
	}
}

/** 
 *	Utility function for drawing manipulation widget in a 3D viewport. 
 *	If we are hit-testing will create HWidgetUtilProxys for each axis, filling in InInfo1 and InInfo2 as passed in by user. 
 */
void FUnrealEdUtils::DrawWidget(const FSceneView* View,FPrimitiveDrawInterface* PDI, const FMatrix& WidgetMatrix, INT InInfo1, INT InInfo2, EAxis HighlightAxis, UBOOL bRotWidget)
{
	const FVector WidgetOrigin = WidgetMatrix.GetOrigin();

	// Calculate size to draw widget so it takes up the same screen space.
	const FLOAT ZoomFactor = Min<FLOAT>(View->ProjectionMatrix.M[0][0], View->ProjectionMatrix.M[1][1]);
	const FLOAT WidgetRadius = View->Project(WidgetOrigin).W * (WidgetSize / ZoomFactor);

	const UBOOL bHitTesting = PDI->IsHitTesting();

	// Choose its color. Highlight manipulated axis in yellow.
	FColor XColor(255, 0, 0);
	FColor YColor(0, 255, 0);
	FColor ZColor(0, 0, 255);

	if(HighlightAxis == AXIS_X)
		XColor = FColor(255, 255, 0);
	else if(HighlightAxis == AXIS_Y)
		YColor = FColor(255, 255, 0);
	else if(HighlightAxis == AXIS_Z)
		ZColor = FColor(255, 255, 0);

	const FVector XAxis = WidgetMatrix.GetAxis(0); 
	const FVector YAxis = WidgetMatrix.GetAxis(1); 
	const FVector ZAxis = WidgetMatrix.GetAxis(2);

	if(bRotWidget)
	{
		if(bHitTesting) PDI->SetHitProxy( new HWidgetUtilProxy(InInfo1, InInfo2, AXIS_X, WidgetMatrix, true) );
		DrawCircle(PDI,WidgetOrigin, YAxis, ZAxis, XColor, WidgetRadius, 24, SDPG_Foreground);
		if(bHitTesting) PDI->SetHitProxy( NULL );

		if(bHitTesting) PDI->SetHitProxy( new HWidgetUtilProxy(InInfo1, InInfo2, AXIS_Y, WidgetMatrix, true) );
		DrawCircle(PDI,WidgetOrigin, XAxis, ZAxis, YColor, WidgetRadius, 24, SDPG_Foreground);
		if(bHitTesting) PDI->SetHitProxy( NULL );

		if(bHitTesting) PDI->SetHitProxy( new HWidgetUtilProxy(InInfo1, InInfo2, AXIS_Z, WidgetMatrix, true) );
		DrawCircle(PDI,WidgetOrigin, XAxis, YAxis, ZColor, WidgetRadius, 24, SDPG_Foreground);
		if(bHitTesting) PDI->SetHitProxy( NULL );
	}
	else
	{
		FMatrix WidgetTM;

		// Draw the widget arrows.
		if(bHitTesting) PDI->SetHitProxy( new HWidgetUtilProxy(InInfo1, InInfo2, AXIS_X, WidgetMatrix, false) );
		WidgetTM = FMatrix(XAxis, YAxis, ZAxis, WidgetOrigin);
		DrawDirectionalArrow(PDI,WidgetTM, XColor, WidgetRadius, 1.f, SDPG_Foreground);
		if(bHitTesting) PDI->SetHitProxy( NULL );

		if(bHitTesting) PDI->SetHitProxy( new HWidgetUtilProxy(InInfo1, InInfo2, AXIS_Y, WidgetMatrix, false) );
		WidgetTM = FMatrix(YAxis, ZAxis, XAxis, WidgetOrigin);
		DrawDirectionalArrow(PDI,WidgetTM, YColor, WidgetRadius, 1.f, SDPG_Foreground);
		if(bHitTesting) PDI->SetHitProxy( NULL );

		if(bHitTesting) PDI->SetHitProxy( new HWidgetUtilProxy(InInfo1, InInfo2, AXIS_Z, WidgetMatrix, false) );
		WidgetTM = FMatrix(ZAxis, XAxis, YAxis, WidgetOrigin);
		DrawDirectionalArrow(PDI,WidgetTM, ZColor, WidgetRadius, 1.f, SDPG_Foreground);
		if(bHitTesting) PDI->SetHitProxy( NULL );
	}
}

/**
 * Localizes a window and all of its child windows.
 *
 * @param	InWin	The window to localize
 * @param	bOptional	TRUE to skip controls which do not have localized text (otherwise the <?int?bleh.bleh> string is assigned to the label)
 */
void FLocalizeWindow( wxWindow* InWin, UBOOL bOptional )
{
	FLocalizeWindowLabel( InWin, bOptional );
	FLocalizeChildren( InWin, bOptional );
}

/**
 * Localizes a window Label.
 *
 * @param	InWin		The window to localize.  Must be valid.
 * @param	bOptional	TRUE to skip controls which do not have localized text (otherwise the <?int?bleh.bleh> string is assigned to the label)
 */
void FLocalizeWindowLabel( wxWindow* InWin, UBOOL bOptional )
{
	check( InWin );
	const FString label = InWin->GetLabel().c_str();
	if( label.Len() )
	{
		FString WindowLabel = Localize( "UnrealEd", TCHAR_TO_ANSI( *label ), GPackage, NULL, bOptional );

		// if bOptional is TRUE, and this control's label isn't localized, WindowLabel should be empty
		if ( WindowLabel.Len() > 0 || !bOptional )
		{
			InWin->SetLabel( *WindowLabel );
		}
	}
}

/**
 * Localizes the child controls within a window, but not the window itself.
 *
 * @param	InWin		The window whose children to localize.  Must be valid.
 * @param	bOptional	TRUE to skip controls which do not have localized text (otherwise the <?int?bleh.bleh> string is assigned to the label)
 */
void FLocalizeChildren( wxWindow* InWin, UBOOL bOptional )
{
	wxWindow* child;

    for( wxWindowList::compatibility_iterator node = InWin->GetChildren().GetFirst() ; node ; node = node->GetNext() )
    {
        child = node->GetData();
		FLocalizeWindowLabel( child, bOptional );

		if( child->GetChildren().GetFirst() != NULL )
		{
			FLocalizeChildren( child, bOptional );
		}
    }
}

/**
 * Splash screen functions and static globals
 */

static HANDLE GSplashScreenThread = NULL;
static HBITMAP GSplashScreenBitmap = NULL;
static HWND GSplashScreenWnd = NULL; 
static TCHAR* GSplashScreenFileName;

/**
 * Window's proc for splash screen
 */
LRESULT CALLBACK SplashScreenWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
    PAINTSTRUCT ps;

	switch( message )
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawState(hdc, DSS_NORMAL, NULL, (LPARAM)GSplashScreenBitmap, 0, 0, 0, 0, 0, DST_BITMAP);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

/**
 * Splash screen thread entry function
 */
DWORD WINAPI StartSplashScreenThread( LPVOID unused )
{
    WNDCLASS wc;
	wc.style       = 0; 
	wc.lpfnWndProc = (WNDPROC) SplashScreenWindowProc; 
	wc.cbClsExtra  = 0; 
	wc.cbWndExtra  = 0; 
	wc.hInstance   = hInstance; 
	wc.hIcon       = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION); 
	wc.hCursor     = LoadCursor((HINSTANCE) NULL, IDC_ARROW); 
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = TEXT("SplashScreenClass"); 
 
	if(!RegisterClass(&wc)) 
	{
		return 0; 
    } 

	// Load splash screen image, display it and handle all window's messages
	GSplashScreenBitmap = (HBITMAP) LoadImage(hInstance, (LPCTSTR)GSplashScreenFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if(GSplashScreenBitmap)
	{
		BITMAP bm;
		GetObject(GSplashScreenBitmap, sizeof(bm), &bm);
		INT ScreenPosX = (GetSystemMetrics(SM_CXSCREEN) - bm.bmWidth) / 2;
		INT ScreenPosY = (GetSystemMetrics(SM_CYSCREEN) - bm.bmHeight) / 2;

		GSplashScreenWnd = CreateWindowEx(WS_EX_TOOLWINDOW, wc.lpszClassName, TEXT("SplashScreen"), WS_BORDER|WS_POPUP,
			ScreenPosX, ScreenPosY, bm.bmWidth, bm.bmHeight, (HWND) NULL, (HMENU) NULL, hInstance, (LPVOID) NULL); 
	 
		if (GSplashScreenWnd)
		{
			ShowWindow(GSplashScreenWnd, SW_SHOW); 
			UpdateWindow(GSplashScreenWnd); 
		 
			MSG message;
			while (GetMessage(&message, NULL, 0, 0))
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			}

			ShowWindow(GSplashScreenWnd, SW_HIDE); 
		}

		DeleteObject(GSplashScreenBitmap);
		GSplashScreenBitmap = NULL;
	}

	UnregisterClass(wc.lpszClassName, hInstance);
    return 0; 
}

/**
 * Displays a splash window with the specified image.  The function does not use wxWidgets.  The splash
 * screen variables are stored in static global variables (SplashScreen*).  If the image file doesn't exist,
 * the function does nothing.
 * 
 * @param SplashName		Image filename, including extension. The file should be located under the %appGameDir%/Splash/ directory.
 */
void appShowSplash( const TCHAR* SplashName )
{
	const FString Filepath = appGameDir() + SplashName;
	GSplashScreenFileName = (TCHAR*) appMalloc(MAX_PATH*sizeof(TCHAR));
	appStrncpy(GSplashScreenFileName, *Filepath, MAX_PATH);
	GSplashScreenThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartSplashScreenThread, (LPVOID)NULL, 0, NULL);
}

/**
 * Destroys the splash window that was previously shown by appShowSplash(). If the splash screen isn't active,
 * it will do nothing.
 */
void appHideSplash( )
{
	if(GSplashScreenThread)
	{
		if(GSplashScreenWnd)
		{
			// Send message to splash screen window to destroy itself
			SendMessage(GSplashScreenWnd, WM_DESTROY, 0, 0);
		}

		// Wait for splash screen thread to finish
		WaitForSingleObject(GSplashScreenThread, INFINITE);

		// Clean up
		CloseHandle(GSplashScreenThread);
		appFree(GSplashScreenFileName);
		GSplashScreenFileName = NULL; 
		GSplashScreenThread = NULL;
		GSplashScreenWnd = NULL;
	}
}


/*-----------------------------------------------------------------------------
	FWindowUtil.
-----------------------------------------------------------------------------*/

// Returns the width of InA as a percentage in relation to InB.
FLOAT FWindowUtil::GetWidthPct( const wxRect& InA, const wxRect& InB )
{
	check( (FLOAT)InB.GetWidth() );
	return InA.GetWidth() / (FLOAT)InB.GetWidth();
}

// Returns the height of InA as a percentage in relation to InB.
FLOAT FWindowUtil::GetHeightPct( const wxRect& InA, const wxRect& InB )
{
	check( (FLOAT)InB.GetHeight() );
	return InA.GetHeight() / (FLOAT)InB.GetHeight();
}

// Returns the real client area of this window, minus any toolbars and other docked controls.
wxRect FWindowUtil::GetClientRect( const wxWindow& InThis, const wxToolBar* InToolBar )
{
	wxRect rc = InThis.GetClientRect();

	if( InToolBar )
	{
		rc.y += InToolBar->GetClientRect().GetHeight();
		rc.height -= InToolBar->GetClientRect().GetHeight();
	}

	return rc;
}

// Loads the position/size and other information about InWindow from the INI file
// and applies them.

void FWindowUtil::LoadPosSize( const FString& InName, wxTopLevelWindow* InWindow, INT InX, INT InY, INT InW, INT InH )
{
	check( InWindow );

	FString Wk;
	GConfig->GetString( TEXT("WindowPosManager"), *InName, Wk, GEditorUserSettingsIni );

	TArray<FString> Args;
	wxRect rc(InX,InY,InW,InH);
	INT Maximized = 0;
	if( Wk.ParseIntoArray( &Args, TEXT(","), 0 ) == 5 )
	{
		// Break out the arguments

		INT X = appAtoi( *Args(0) );
		INT Y = appAtoi( *Args(1) );
		const INT W = appAtoi( *Args(2) );
		const INT H = appAtoi( *Args(3) );
		Maximized = appAtoi( *Args(4) );

		// Make sure that the window is going to be on the visible screen
		const INT Threshold = 20;
		const INT vleft = ::GetSystemMetrics( SM_XVIRTUALSCREEN )-Threshold;
		const INT vtop = ::GetSystemMetrics( SM_YVIRTUALSCREEN )-Threshold;
		const INT vright = ::GetSystemMetrics( SM_CXVIRTUALSCREEN )+Threshold;
		const INT vbottom = ::GetSystemMetrics( SM_CYVIRTUALSCREEN )+Threshold;

		

		if( X < vleft || X+W >= vright )		X = vleft;
		if( Y < vtop || Y+H >= vbottom )			Y = vtop;

		// Set the windows attributes

		rc.SetX( X );
		rc.SetY( Y );
		rc.SetWidth( W );
		rc.SetHeight( H );
	}

	InWindow->SetSize( rc );
	if(Maximized == 1)
	{
		InWindow->Maximize();
	}
}

// Saves the position/size and other relevant info about InWindow to the INI file.
void FWindowUtil::SavePosSize( const FString& InName, const wxTopLevelWindow* InWindow )
{
	check( InWindow );
	const wxRect rc = InWindow->GetRect();

	INT Maximized;

	if(InWindow->IsMaximized())
	{
		Maximized = 1;
	}
	else
	{
		Maximized = 0;
	}

	FString Wk = *FString::Printf( TEXT("%d,%d,%d,%d,%d"), rc.GetX(), rc.GetY(), rc.GetWidth(), rc.GetHeight(), Maximized );
	GConfig->SetString( TEXT("WindowPosManager"), *InName, *Wk, GEditorUserSettingsIni );
}

/*-----------------------------------------------------------------------------
	FTrackPopupMenu.
-----------------------------------------------------------------------------*/

FTrackPopupMenu::FTrackPopupMenu( wxWindow* InWindow, wxMenu* InMenu ):
Window( InWindow ),
Menu( InMenu )
{
	check( Window );
	check( Menu );
}

void FTrackPopupMenu::Show( INT InX, INT InY )
{
	wxPoint pt( InX, InY );

	// Display at the current mouse position?
	if( InX < 0 || InY < 0 )
	{
		pt = Window->ScreenToClient( wxGetMousePosition() );
	}

	Window->PopupMenu( Menu, pt );
}
