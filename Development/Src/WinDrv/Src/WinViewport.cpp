/*=============================================================================
	WinViewport.cpp: FWindowsViewport code.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "WinDrv.h"
#include "EngineUserInterfaceClasses.h"

#define WM_MOUSEWHEEL 0x020A

// From atlwin.h:
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))
#endif

/** If TRUE, FWindowsViewport::UpdateModifierState() will enqueue rather than process immediately. */
UBOOL GShouldEnqueueModifierStateUpdates = FALSE;

#if WITH_IME
#pragma comment( lib, "Imm32.lib" )

UBOOL CheckIMESupport( HWND Window )
{
	HIMC Imc = ImmGetContext( Window );
	if( !Imc )
	{
		Imc = ImmCreateContext();
		if( Imc )	
		{
			ImmAssociateContext( Window, Imc ); 
		}
	}
	else
	{
		ImmReleaseContext( Window, Imc );
	}

	return( Imc != NULL );
}

void DestroyIME( HWND Window )
{
	HIMC Imc = ImmGetContext( Window );
	if( Imc )
	{
		ImmAssociateContext( Window, NULL );
		ImmDestroyContext( Imc );
	}
}
#endif

//
//	FWindowsViewport::FWindowsViewport
//
FWindowsViewport::FWindowsViewport(UWindowsClient* InClient,FViewportClient* InViewportClient,const TCHAR* InName,UINT InSizeX,UINT InSizeY,UBOOL InFullscreen,HWND InParentWindow,INT InPosX, INT InPosY)
	:	FViewport(InViewportClient)
	,	bUpdateModifierStateEnqueued( FALSE )
	,	PreventCapture(FALSE)
{
	Client					= InClient;

	Name					= InName;
	Window					= NULL;
	ParentWindow			= InParentWindow;

	bCapturingMouseInput	= 0;
	bCapturingJoystickInput = 1;
	bLockingMouseToWindow	= 0;
#if WITH_IME
	bSupportsIME			= TRUE;
	CurrentIMESize			= 0;
#endif

	Minimized				= 0;
	Maximized				= 0;
	Resizing				= 0;
	bPerformingSize			= FALSE;
	
	LastMouseXEventTime		= 0;
	LastMouseYEventTime		= 0;

	Client->Viewports.AddItem(this);

	// if win positions are default, attempt to read from cmdline
	if (InPosX == -1)
	{
		Parse(appCmdLine(),TEXT("PosX="),InPosX);
	}
	if (InPosY == -1)
	{
		Parse(appCmdLine(),TEXT("PosY="),InPosY);
	}

	// Creates the viewport window.
	Resize(InSizeX,InSizeY,InFullscreen,InPosX,InPosY);

	// Set as active window.
	::SetActiveWindow(Window);

	// Set initial key state.
	for(UINT KeyIndex = 0;KeyIndex < 256;KeyIndex++)
	{
		if ( Client->KeyMapVirtualToName.HasKey(KeyIndex) )
		{
			KeyStates[KeyIndex] = ::GetKeyState(KeyIndex) & 0x8000;
		}
	}
}

//
//	FWindowsViewport::Destroy
//

void FWindowsViewport::Destroy()
{
	ViewportClient = NULL;

#if WITH_IME
	DestroyIME( Window );
#endif

	// Release the viewport RHI.
	UpdateViewportRHI(TRUE,0,0,FALSE);

	// Release the mouse capture.
	CaptureMouseInput(FALSE);

	// Destroy the viewport's window.
	DestroyWindow(Window);
}

//
//	FWindowsViewport::Resize
//
void FWindowsViewport::Resize(UINT NewSizeX,UINT NewSizeY,UBOOL NewFullscreen,INT InPosX, INT InPosY)
{
	if( bPerformingSize )
	{
		debugf(NAME_Error,TEXT("bPerformingSize == 1, FWindowsViewport::Resize"));
		appDebugBreak();
		return;
	}

	// This makes sure that the Play From Here window in the editor can't go fullscreen
	if (NewFullscreen && appStricmp(*Name, PLAYWORLD_VIEWPORT_NAME) == 0)
	{
		return;
	}

	bPerformingSize			= TRUE;

	// Make sure we're using a supported fullscreen resolution.
	if ( NewFullscreen && !ParentWindow )
	{
#if USE_D3D_RHI
		FD3DViewport::GetSupportedResolution( NewSizeX, NewSizeY );
#endif // USE_D3D_RHI
	}

	// Figure out physical window size we must specify to get appropriate client area.
	DWORD	WindowStyle;
	INT		PhysWidth		= NewSizeX;
	INT		PhysHeight		= NewSizeY;

	if( ParentWindow )
	{
		WindowStyle			= WS_CHILD | WS_CLIPSIBLINGS;
		NewFullscreen		= 0;
	}
	else
	{
		if(NewFullscreen)
		{
			WindowStyle		= WS_POPUP;
		}
		else
		{
			WindowStyle		= WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION;
		}

		RECT  WindowRect;
		WindowRect.left		= 100;
		WindowRect.top		= 100;
		WindowRect.right	= NewSizeX + 100;
		WindowRect.bottom	= NewSizeY + 100;

		::AdjustWindowRect(&WindowRect,WindowStyle,0);

		PhysWidth			= WindowRect.right  - WindowRect.left;
		PhysHeight			= WindowRect.bottom - WindowRect.top;
	}

	if( Window == NULL )
	{
		// Obtain width and height of primary monitor.
		INT ScreenWidth  = ::GetSystemMetrics( SM_CXSCREEN );
		INT ScreenHeight = ::GetSystemMetrics( SM_CYSCREEN );
		
		int WindowPosX, WindowPosY;
		if (InPosX == -1)
		{
			if (!Parse(appCmdLine(), TEXT("WindowPosX="), WindowPosX))
			{
				WindowPosX = (ScreenWidth - PhysWidth) / 6;
			}
		}
		else
		{
			WindowPosX = InPosX;
		}

		if (InPosY == -1)
		{
			if (!Parse(appCmdLine(), TEXT("WindowPosY="), WindowPosY))
			{
				WindowPosY = (ScreenHeight - PhysHeight) / 6;
			}
		}
		else
		{
			WindowPosY = InPosY;
		}

		// Create the window
		Window = CreateWindowExX( WS_EX_APPWINDOW, *Client->WindowClassName, *Name, WindowStyle, WindowPosX, WindowPosY, PhysWidth, PhysHeight, ParentWindow, NULL, hInstance, this );
		verify( Window );
	}
	else
	{
		SetWindowLongX(Window, GWL_STYLE, WindowStyle);
	}

	// Initialize the viewport's render device.
	if( NewSizeX && NewSizeY )
	{
		UpdateViewportRHI(FALSE,NewSizeX,NewSizeY,NewFullscreen);
	}
	// #19088: Based on certain startup patterns, there can be a case when all viewports are destroyed, which in turn frees up the D3D device (which results in badness).
	// There are plans to fix the initialization code, but until then hack the known case when a viewport is deleted due to being resized to zero width or height.
	// (D3D does not handle the zero width or zero height case)
	else if( NewSizeX && !NewSizeY )
	{
		NewSizeY = 1;
		UpdateViewportRHI(FALSE,NewSizeX,NewSizeY,NewFullscreen);
	}
	else if( !NewSizeX && NewSizeY )
	{
		NewSizeX = 1;
		UpdateViewportRHI(FALSE,NewSizeX,NewSizeY,NewFullscreen);
	}
	// End hack
	else
	{
		UpdateViewportRHI(TRUE,NewSizeX,NewSizeY,NewFullscreen);
	}

	if( !ParentWindow && !NewFullscreen )
	{
		// Resize viewport window.

		RECT WindowRect;
		::GetWindowRect( Window, &WindowRect );

		RECT ScreenRect;
		ScreenRect.left		= ::GetSystemMetrics( SM_XVIRTUALSCREEN );
		ScreenRect.top		= ::GetSystemMetrics( SM_YVIRTUALSCREEN );
		ScreenRect.right	= ::GetSystemMetrics( SM_CXVIRTUALSCREEN );
		ScreenRect.bottom	= ::GetSystemMetrics( SM_CYVIRTUALSCREEN );

		if( WindowRect.left >= ScreenRect.right-4 || WindowRect.left < ScreenRect.left - 4 )
		{
			WindowRect.left = ScreenRect.left;
		}
		if( WindowRect.top >= ScreenRect.bottom-4 || WindowRect.top < ScreenRect.top - 4 )
		{
			WindowRect.top = ScreenRect.top;
		}

		::SetWindowPos( Window, HWND_TOP, WindowRect.left, WindowRect.top, PhysWidth, PhysHeight, SWP_NOSENDCHANGING | SWP_NOZORDER );
	}

	// Show the viewport.

	::ShowWindow( Window, SW_SHOW );
	::UpdateWindow( Window );

	bPerformingSize = FALSE;
}

//
//	FWindowsViewport::ShutdownAfterError - Minimalist shutdown.
//
void FWindowsViewport::ShutdownAfterError()
{
	if(Window)
	{
		::DestroyWindow(Window);
		Window = NULL;
	}
}

//
//	FWindowsViewport::CaptureInput
//
UBOOL FWindowsViewport::CaptureMouseInput(UBOOL Capture,UBOOL bLockMouse/*=TRUE*/)
{
	// Focus issues with Viewports:
	// This code used to check ForegroundWindow too. It also checked only against Window.
	// For some reason, ::GetParent(Window) is always NULL, so in order to make the frame window
	// visibly get focus (blue title bar), I need to call ::SetFocus(ParentWindow) before calling
	// this function. Mouse, keyboard, gamepads, getting focus and losing focus must still work
	// if FocusWindow is either Window or ParentWindow (note FocusWindow and/or ParentWindow may
	// be NULL).
	//
	// Issues that have been identified and been fixed:
	// 1.  Mouse dragging in editor
	// 2.  Zooming with both mouse buttons in editor
	// 3.  Keyboard input in game
	// 4.  Focus revert back to Generic Browser instead of new window when double-clicking an item
	// 5.  Unable to drag around game window
	// 6.  Mouse clipping still active after game lost focus
	// 7.  Mouse input still active after game window lost focus and log window got focus.
	// 8.  Keyboard input in viewports in editor (shortcuts like spacebar, W, etc)
	// 9.  Gamepad input still active after game lost focus to another window
	// 10. Annyoing beep when pressing un-bound key in Editor viewport

	HWND FocusWindow = IsFullscreen() || GIsGame ? ::GetForegroundWindow() : ::GetFocus();
	Capture = Capture && FocusWindow == Window;
	bLockMouse = bLockMouse || IsFullscreen();

	// Make REALLY sure we're in correct mouse lock mode.
	LockMouseToWindow( bLockingMouseToWindow && Capture && bLockMouse );

	if( Capture && !bCapturingMouseInput )
	{
		// turning on mouse capture
		bCapturingMouseInput = 1;

		// Hide mouse cursor.
		if ( bLockMouse )
		{
			while( ::ShowCursor(FALSE)>=0 );

			// Store current mouse position and capture mouse.
			::GetCursorPos(&PreCaptureMousePos);
		}
		else
		{
			PreCaptureMousePos.x = -1;
			PreCaptureMousePos.y = -1;
		}

		::SetCapture(Window);

		// Clip mouse to window.
		if ( bLockMouse )
		{
			LockMouseToWindow(TRUE);
		}

		// Clear mouse input buffer.
		UWindowsClient::DirectInput8Mouse->Unacquire();
		UWindowsClient::DirectInput8Mouse->SetCooperativeLevel(GetForegroundWindow(),DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
		if( SUCCEEDED( UWindowsClient::DirectInput8Mouse->Acquire() ) )
		{
			DIDEVICEOBJECTDATA	Event;
			DWORD				Elements = 1;
			while( SUCCEEDED( UWindowsClient::DirectInput8Mouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),&Event,&Elements,0) ) && (Elements > 0) );
		}
	}
	else if(!Capture && bCapturingMouseInput)
	{
		// turning off mouse capture
		bCapturingMouseInput = 0;

		// No longer confine mouse to window if we're not in fullscreen.
		if ( !IsFullscreen() || FocusWindow != Window )
		{
			LockMouseToWindow(FALSE);
		}

		// Restore old mouse position and release capture.
		if ( PreCaptureMousePos.x != -1 && PreCaptureMousePos.y != -1 )
		{
			::SetCursorPos(PreCaptureMousePos.x,PreCaptureMousePos.y);
		}
		::ReleaseCapture();

		// Show mouse cursor.
		while( ::ShowCursor(TRUE)<0 );
	}

	return bCapturingMouseInput;
}

void FWindowsViewport::LockMouseToWindow(UBOOL bLock)
{
	if(bLock && !bLockingMouseToWindow)
	{
		// Store the old cursor clip rectangle.
		::GetClipCursor(&PreCaptureCursorClipRect);

		RECT R;
		::GetClientRect( Window, &R );
		::MapWindowPoints( Window, NULL, (POINT*)&R, 2 );
		::ClipCursor(&R);

		bLockingMouseToWindow = TRUE;
	}
	else if(!bLock && bLockingMouseToWindow)
	{
		::ClipCursor(&PreCaptureCursorClipRect);

		bLockingMouseToWindow = FALSE;
	}
	else
	{
		// Make REALLY sure Windows is set up the way bLockingMouseToWindow thinks.
		RECT WindowClipRect, ActualClipRect;
		::GetClientRect( Window, &WindowClipRect );
		::MapWindowPoints( Window, NULL, (POINT*)&WindowClipRect, 2 );
		::GetClipCursor(&ActualClipRect);
		UBOOL bIsLocked =
			(WindowClipRect.left == ActualClipRect.left) &&
			(WindowClipRect.top == ActualClipRect.top) &&
			(WindowClipRect.right == ActualClipRect.right) &&
			(WindowClipRect.bottom == ActualClipRect.bottom);
		if ( bIsLocked != bLockingMouseToWindow )
		{
			RECT& R = bLockingMouseToWindow ? WindowClipRect : PreCaptureCursorClipRect;
			::ClipCursor( &R );
		}
	}
}

//
//	FWindowsViewport::CaptureJoystickInput
//
UBOOL FWindowsViewport::CaptureJoystickInput(UBOOL Capture)
{
	bCapturingJoystickInput	= Capture;

	return bCapturingJoystickInput;
}

//
//	FWindowsViewport::KeyState
//
UBOOL FWindowsViewport::KeyState(FName Key) const
{
	BYTE* VirtualKey = Client->KeyMapNameToVirtual.Find(Key);

	if( VirtualKey )
	{
		return ::GetKeyState(*VirtualKey) & 0x8000 ? TRUE : FALSE;
	}
	else
	{
		return 0;
	}
}

//
//	FWindowsViewport::GetMouseX
//
INT FWindowsViewport::GetMouseX()
{
	POINT P;
	::GetCursorPos( &P );
	::ScreenToClient( Window, &P );
	return P.x;
}

//
//	FWindowsViewport::GetMouseY
//
INT FWindowsViewport::GetMouseY()
{
	POINT P;
	::GetCursorPos( &P );
	::ScreenToClient( Window, &P );
	return P.y;
}

void FWindowsViewport::GetMousePos( FIntPoint& MousePosition )
{
	POINT P;
	::GetCursorPos( &P );
	::ScreenToClient( Window, &P );
	MousePosition.X = P.x;
	MousePosition.Y = P.y;
}

void FWindowsViewport::SetMouse(INT x, INT y)
{
	POINT	P;
	P.x = x;
	P.y = y;
	::ClientToScreen(Window, &P);
	::SetCursorPos(P.x, P.y);
}

//
//	FWindowsViewport::InvalidateDisplay
//

void FWindowsViewport::InvalidateDisplay()
{
	::InvalidateRect(Window,NULL,0);
}

FViewportFrame* FWindowsViewport::GetViewportFrame()
{
	return ParentWindow ? NULL : this;
}

/**
 * Helper structure for keeping track of polled mouse events.
 */
struct FMouseEvent
{
	/**
	 * Constructor, initializing all member variables to passed in ones.
	 *
	 * @param InTimeStamp	Time stamp of event
	 * @param InDelta		Movement delta of event
	 */
	FMouseEvent( DWORD InTimeStamp, INT InDelta )
	:	TimeStamp( InTimeStamp )
	,	Delta( InDelta )
	{}
	/** Time stamp of event in ms (arbitrarily based) */
	DWORD	TimeStamp;
	/** Movement delta of event */
	INT		Delta;	
};


//
//	FWindowsViewport::ProcessInput
//
void FWindowsViewport::ProcessInput( FLOAT DeltaTime )
{
	if( !ViewportClient )
	{
		return;
	}

	HWND FocusWindow = ::GetFocus();

#if WITH_PANORAMA
	extern UBOOL appIsPanoramaGuideOpen(void);
	/** If the Live guide is open, don't consume it's input */
	if (appIsPanoramaGuideOpen())
	{
		// Also pause any waveforms being played
		if ((bCapturingJoystickInput || ViewportClient->RequiresUncapturedAxisInput()) && FocusWindow == Window )
		{
			XINPUT_VIBRATION Feedback = {0};
			for (INT JoystickIndex = 0; JoystickIndex < UWindowsClient::Joysticks.Num(); JoystickIndex++)
			{
				// This will turn off the vibration
				XInputSetState(JoystickIndex,&Feedback);
			}
		}
		return;
	}
#endif

	// Process mouse input.

	if( bCapturingMouseInput || ViewportClient->RequiresUncapturedAxisInput() )
	{
		// Focus issues with Viewports: Force mouse input
		UWindowsClient::DirectInput8Mouse->Unacquire();
		UWindowsClient::DirectInput8Mouse->SetCooperativeLevel(GetForegroundWindow(),DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
		if( SUCCEEDED( UWindowsClient::DirectInput8Mouse->Acquire() ) )
		{
			UWindowsClient::DirectInput8Mouse->Poll();

			TArray<FMouseEvent>	MouseXEvents;
			TArray<FMouseEvent>	MouseYEvents;
			DIDEVICEOBJECTDATA	Event;
			DWORD				Elements = 1;
			while( SUCCEEDED( UWindowsClient::DirectInput8Mouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),&Event,&Elements,0) ) && (Elements > 0) )
			{
				switch( Event.dwOfs ) 
				{
				case DIMOFS_X:
					{
						MouseXEvents.AddItem( FMouseEvent( Event.dwTimeStamp, (INT)Event.dwData ) );
						bMouseHasMoved = 1;
						break;
					}
				case DIMOFS_Y: 
					{
						MouseYEvents.AddItem( FMouseEvent( Event.dwTimeStamp, -(INT)Event.dwData ) );
						bMouseHasMoved = 1;
						break; 
					}
				case DIMOFS_Z:
					if ( bCapturingMouseInput )
					{
						if( ((INT)Event.dwData) < 0 )
						{
							ViewportClient->InputKey(this,0,KEY_MouseScrollDown,IE_Pressed);
							ViewportClient->InputKey(this,0,KEY_MouseScrollDown,IE_Released);
						}
						else if( ((INT)Event.dwData) > 0)
						{
							ViewportClient->InputKey(this,0,KEY_MouseScrollUp,IE_Pressed);
							ViewportClient->InputKey(this,0,KEY_MouseScrollUp,IE_Released);
						}
						break;
					}
				default:
					break;
				}
			}

#if MERGE_SAME_TIMESTAMPS
			// Merge duplicate entries.
			for( INT EventIndex=1; EventIndex<MouseXEvents.Num(); EventIndex++ )
			{
				FMouseEvent& PreviousMouseEvent = MouseXEvents( EventIndex - 1 );
				FMouseEvent& CurrentMouseEvent	= MouseXEvents( EventIndex );
				// Merge entries with duplicate timestamps to allow proper delta time calculation.
				if( PreviousMouseEvent.TimeStamp == CurrentMouseEvent.TimeStamp )
				{			
					PreviousMouseEvent.Delta += CurrentMouseEvent.Delta;
					MouseXEvents.Remove( EventIndex-- );
				}
			}
			for( INT EventIndex=1; EventIndex<MouseYEvents.Num(); EventIndex++ )
			{
				FMouseEvent& PreviousMouseEvent = MouseYEvents( EventIndex - 1 );
				FMouseEvent& CurrentMouseEvent	= MouseYEvents( EventIndex );
				// Merge entries with duplicate timestamps to allow proper delta time calculation.
				if( PreviousMouseEvent.TimeStamp == CurrentMouseEvent.TimeStamp )
				{			
					PreviousMouseEvent.Delta += CurrentMouseEvent.Delta;
					MouseYEvents.Remove( EventIndex-- );
				}
			}
#endif

			// Route mouse events to input.
			for( INT EventIndex=0; EventIndex<MouseXEvents.Num(); EventIndex++ )
			{
				FMouseEvent Event		= MouseXEvents( EventIndex );
				DWORD		DeltaTicks	= Event.TimeStamp - LastMouseXEventTime;
				ViewportClient->InputAxis( this, 0, KEY_MouseX, Event.Delta, DeltaTicks / 1000.f );
				LastMouseXEventTime = Event.TimeStamp;
			}
			for( INT EventIndex=0; EventIndex<MouseYEvents.Num(); EventIndex++ )
			{
				FMouseEvent Event		= MouseYEvents( EventIndex );
				DWORD		DeltaTicks	= Event.TimeStamp - LastMouseYEventTime;
				ViewportClient->InputAxis( this, 0, KEY_MouseY, Event.Delta, DeltaTicks / 1000.f );
				LastMouseYEventTime = Event.TimeStamp;
			}
		}
	}

	if( (bCapturingJoystickInput || ViewportClient->RequiresUncapturedAxisInput()) && FocusWindow == Window )
	{
		for(INT JoystickIndex = 0;JoystickIndex < UWindowsClient::Joysticks.Num();JoystickIndex++)
		{
			DIJOYSTATE2 State;
			XINPUT_STATE XIstate;
			FJoystickInfo& JoystickInfo = UWindowsClient::Joysticks(JoystickIndex);
			UBOOL bIsConnected = FALSE;

			ZeroMemory( &XIstate, sizeof(XIstate) );
			ZeroMemory( &State, sizeof(State) );

			if ( JoystickInfo.DirectInput8Joystick )
			{
				// Focus issues with Viewports: Force gamepad input
				JoystickInfo.DirectInput8Joystick->Unacquire();
				JoystickInfo.DirectInput8Joystick->SetCooperativeLevel(GetForegroundWindow(),DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
				bIsConnected = ( SUCCEEDED(JoystickInfo.DirectInput8Joystick->Acquire()) && SUCCEEDED(JoystickInfo.DirectInput8Joystick->Poll()) );
				if( bIsConnected && SUCCEEDED(JoystickInfo.DirectInput8Joystick->GetDeviceState(sizeof(DIJOYSTATE2), &State)) )
				{
					// Store state
					JoystickInfo.PreviousState = State;
				}
				else
				{
					// if failed to get device state, use previous one.
					// this seems to happen quite frequently unfortunately :(
					State = JoystickInfo.PreviousState;
				}
			}
			else if ( JoystickIndex < JOYSTICK_NUM_XINPUT_CONTROLLERS )
			{
				// Simply get the state of the controller from XInput.
				bIsConnected = ( XInputGetState( JoystickIndex, &XIstate ) == ERROR_SUCCESS ) ? TRUE : FALSE;
			}

			if ( bIsConnected )
			{
				// see the UWindowsClient::UWindowsClient calls below for which slots in this array map to which names
				// 1 means pressed, 0 means not pressed
				UBOOL CurrentStates[MAX_JOYSTICK_BUTTONS] = {0};

				if( JoystickInfo.JoystickType == JOYSTICK_Xbox_Type_S )
				{
					// record our current button pressed state
					for (INT ButtonIndex = 0; ButtonIndex < 12; ButtonIndex++)
					{
						CurrentStates[ButtonIndex] = State.rgbButtons[ButtonIndex];
					}
					CurrentStates[12] = State.rgdwPOV[0] == 0;
					CurrentStates[13] = State.rgdwPOV[0] == 18000;
					CurrentStates[14] = State.rgdwPOV[0] == 27000;
					CurrentStates[15] = State.rgdwPOV[0] == 9000;

					// Axis, convert range 0..65536 set up in EnumAxesCallback to +/- 1.
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_LeftX	, (State.lX  - 32768.f) / 32768.f, DeltaTime, TRUE );
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_LeftY	, -(State.lY  - 32768.f) / 32768.f, DeltaTime, TRUE );
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_RightX	, (State.lRx - 32768.f) / 32768.f, DeltaTime, TRUE );
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_RightY	, (State.lRy - 32768.f) / 32768.f, DeltaTime, TRUE );
				}
				// Support X360 controller
				else if( JoystickInfo.JoystickType == JOYSTICK_X360 )
				{
					CurrentStates[Client->X360ToXboxControllerMapping[0]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_A;
					CurrentStates[Client->X360ToXboxControllerMapping[1]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_B;
					CurrentStates[Client->X360ToXboxControllerMapping[2]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_X;
					CurrentStates[Client->X360ToXboxControllerMapping[3]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_Y;
					CurrentStates[Client->X360ToXboxControllerMapping[4]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
					CurrentStates[Client->X360ToXboxControllerMapping[5]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
					CurrentStates[Client->X360ToXboxControllerMapping[6]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;
					CurrentStates[Client->X360ToXboxControllerMapping[7]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_START;
					CurrentStates[Client->X360ToXboxControllerMapping[8]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
					CurrentStates[Client->X360ToXboxControllerMapping[9]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;

					CurrentStates[Client->X360ToXboxControllerMapping[10]] = XIstate.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
					CurrentStates[Client->X360ToXboxControllerMapping[11]] = XIstate.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

					// record our current button pressed state
					CurrentStates[Client->X360ToXboxControllerMapping[12]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
					CurrentStates[Client->X360ToXboxControllerMapping[13]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
					CurrentStates[Client->X360ToXboxControllerMapping[14]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
					CurrentStates[Client->X360ToXboxControllerMapping[15]] = XIstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

					// Axis, convert range -32768..+32767 set up in EnumAxesCallback to +/- 1.
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_LeftX	, XIstate.Gamepad.sThumbLX / 32768.f, DeltaTime, TRUE );
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_LeftY	, XIstate.Gamepad.sThumbLY / 32768.f, DeltaTime, TRUE );
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_RightX, XIstate.Gamepad.sThumbRX / 32768.f, DeltaTime, TRUE );
					// this needs to be inverted as the XboxTypeS axis are flipped from the "norm"
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_RightY, -XIstate.Gamepad.sThumbRY / 32768.f, DeltaTime, TRUE );

					// Now update any waveform data
					UForceFeedbackManager* Manager = ViewportClient->GetForceFeedbackManager(JoystickIndex);
					if (Manager != NULL)
					{
						Manager->ApplyForceFeedback(JoystickIndex,DeltaTime);
					}
				}
				else if( JoystickInfo.JoystickType == JOYSTICK_PS2_Old_Converter || JoystickInfo.JoystickType == JOYSTICK_PS2_New_Converter )
				{
					// PS2 controller has to be mapped funny, since we use Xbox button mapping below
					for (INT ButtonIndex = 0; ButtonIndex < 12; ButtonIndex++)
					{
						CurrentStates[Client->PS2ToXboxControllerMapping[ButtonIndex]] = State.rgbButtons[ButtonIndex];
					}

					CurrentStates[12] = State.rgdwPOV[0] == 0;
					CurrentStates[13] = State.rgdwPOV[0] == 18000;
					CurrentStates[14] = State.rgdwPOV[0] == 27000;
					CurrentStates[15] = State.rgdwPOV[0] == 9000;

					// Axis, convert range 0..65536 set up in EnumAxesCallback to +/- 1.
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_LeftX			, (State.lX  - 32768.f) / 32768.f, DeltaTime, TRUE );
					ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_LeftY			, (State.lY  - 32768.f) / 32768.f, DeltaTime, TRUE );
					if( JoystickInfo.JoystickType == JOYSTICK_PS2_Old_Converter )
					{
						ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_RightX, (State.lRz - 32768.f) / 32768.f, DeltaTime, TRUE );
						ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_RightY, (State.lZ  - 32768.f) / 32768.f, DeltaTime, TRUE );
					}
					else
					{
						ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_RightX, (State.lZ  - 32768.f) / 32768.f, DeltaTime, TRUE );
						ViewportClient->InputAxis( this, JoystickInfo.ControllerId, KEY_XboxTypeS_RightY, (State.lRz - 32768.f) / 32768.f, DeltaTime, TRUE );
					}
				}

				const DOUBLE CurrentTime = appSeconds();

				// Buttons (treated as digital buttons).
				for (INT ButtonIndex = 0; ButtonIndex < MAX_JOYSTICK_BUTTONS; ButtonIndex++)
				{
					if (CurrentStates[ButtonIndex] != JoystickInfo.JoyStates[ButtonIndex])
					{
						ViewportClient->InputKey(this, JoystickInfo.ControllerId, Client->JoyNames[ButtonIndex], CurrentStates[ButtonIndex] ? IE_Pressed : IE_Released, 1.f, TRUE );
						if ( CurrentStates[ButtonIndex] != 0 )
						{
							// this button was pressed - set the button's NextRepeatTime to the InitialButtonRepeatDelay
							JoystickInfo.NextRepeatTime[ButtonIndex] = CurrentTime + Client->InitialButtonRepeatDelay;
						}
					}
					else if ( CurrentStates[ButtonIndex] != 0 && JoystickInfo.NextRepeatTime[ButtonIndex] <= CurrentTime )
					{
						// it is time to generate an IE_Repeat event for this joystick button
						ViewportClient->InputKey(this, JoystickInfo.ControllerId, Client->JoyNames[ButtonIndex], IE_Repeat, 1.f, TRUE );

						// set the button's NextRepeatTime to the ButtonRepeatDelay
						JoystickInfo.NextRepeatTime[ButtonIndex] = CurrentTime + Client->ButtonRepeatDelay;
					}
				}

				// update last frames state
				appMemcpy(JoystickInfo.JoyStates, CurrentStates, sizeof(JoystickInfo.JoyStates));
			}
		}
	}

	// Process keyboard input.

	for( UINT KeyIndex = 0;KeyIndex < 256;KeyIndex++ )
	{
		FName* KeyName = Client->KeyMapVirtualToName.Find(KeyIndex);

		if(  KeyName )
		{
			UBOOL NewKeyState = ::GetKeyState(KeyIndex) & 0x8000;

			// wxWindows does not let you tell the left and right shift apart, so the problem is pressing the right shift
			// results in a left shift event. Then this code looks to see if left shift is down, and it isn't, so it calls
			// a Released event straight away. This code means that both shift have to be up to generate a Release event.
			// This isn't perfect, but means you can use the right shift as a modifier in the editor etc.
			if(KeyIndex == VK_RSHIFT)
			{
				NewKeyState = NewKeyState || ::GetKeyState(VK_LSHIFT) & 0x8000;
			}
			else if(KeyIndex == VK_LSHIFT)
			{
				NewKeyState = NewKeyState || ::GetKeyState(VK_RSHIFT) & 0x8000;
			}


			if (*KeyName != KEY_LeftMouseButton 
			&&  *KeyName != KEY_RightMouseButton 
			&&  *KeyName != KEY_MiddleMouseButton 
			&&	*KeyName != KEY_ThumbMouseButton)
			{
				if( !NewKeyState && KeyStates[KeyIndex] )
				{
					ViewportClient->InputKey(this,0,*KeyName,IE_Released);
					KeyStates[KeyIndex] = 0;
				}
			}
			else if ( GIsGame && KeyStates[KeyIndex] )
			{
				//@todo. The Repeat event should really be sent in both editor and game,
				// but the editor viewport input handlers need to be updated accordingly.
				// This would involve changing the lock mouse to windows functionality 
				// (which does not check the button for a press event, but simply 
				// captures if the mouse button state is true - which is the case for
				// repeats). It would also require verifying each function makes no 
				// assumptions about what event is passed in.
				ViewportClient->InputKey(this, 0, *KeyName, IE_Repeat);
			}
		}
	}

#if WITH_IME
	// Tick/init the input method editor
	bSupportsIME = CheckIMESupport( Window );
#endif
}

//
//	FWindowsViewport::HandlePossibleSizeChange
//
void FWindowsViewport::HandlePossibleSizeChange()
{
	RECT WindowClientRect;
	::GetClientRect( Window, &WindowClientRect );

	UINT NewSizeX = WindowClientRect.right - WindowClientRect.left;
	UINT NewSizeY = WindowClientRect.bottom - WindowClientRect.top;

	if(!IsFullscreen() && (NewSizeX != GetSizeX() || NewSizeY != GetSizeY()))
	{
		Resize( NewSizeX, NewSizeY, 0 );

		if(ViewportClient)
			ViewportClient->ReceivedFocus(this);
	}
}

//
//	FWindowsViewport::ViewportWndProc - Main viewport window function.
//
LONG FWindowsViewport::ViewportWndProc( UINT Message, WPARAM wParam, LPARAM lParam )
{
	// Process any enqueued UpdateModifierState requests.
	if ( bUpdateModifierStateEnqueued )
	{
		UpdateModifierState();
	}

	if( !Client->ProcessWindowsMessages || Client->Viewports.FindItemIndex(this) == INDEX_NONE )
	{
		return DefWindowProcX(Window,Message,wParam,lParam);
	}

	// Helper class to aid in sending callbacks, but still let each message return in their case statements
	class FCallbackHelper
	{
		FViewport* Viewport;
	public:
		FCallbackHelper(FViewport* InViewport, UINT InMessage) 
		:	Viewport( InViewport )
		{ 
			GCallbackEvent->Send( CALLBACK_PreWindowsMessage, Viewport, InMessage ); 
		}
		~FCallbackHelper() 
		{ 
			GCallbackEvent->Send( CALLBACK_PostWindowsMessage, Viewport, 0 ); 
		}
	};

	// Message handler.
	switch(Message)
	{
		case WM_DESTROY:
			Window = NULL;
			return 0;

		case WM_CLOSE:
			if( ViewportClient )
				ViewportClient->CloseRequested(this);
			return 0;

		case WM_PAINT:
			if(ViewportClient)
				ViewportClient->RedrawRequested(this);
			break;

		case WM_MOUSEACTIVATE:
			if(LOWORD(lParam) != HTCLIENT)
			{
				// The activation was the result of a mouse click outside of the client area of the window.
				// Prevent the mouse from being captured to allow the user to drag the window around.
				PreventCapture = 1;
			}
			return MA_ACTIVATE;

		// Focus issues with Viewports: Viewports are owned by a wxPanel window (to wrap this plain Windows code into wxWidgets I guess),
		// which makes all these messages become processed by IsDialogMessage(). As part of that, we need to tell IsDialogMessage that
		// we want all WM_CHARS and don't let Windows beep for unprocessed chars in WM_KEYDOWN.
		// I never figured out why bound keys don't beep in WM_KEYDOWN (like spacebar) and un-bound keys beep... :(   -Smedis
		case WM_GETDLGCODE:
			{
				return DLGC_WANTALLKEYS;
			}

		case WM_SETFOCUS:
			UWindowsClient::DirectInput8Mouse->Unacquire();
			UWindowsClient::DirectInput8Mouse->SetCooperativeLevel(Window,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
			for(INT JoystickIndex = 0;JoystickIndex < UWindowsClient::Joysticks.Num();JoystickIndex++)
			{
				FJoystickInfo& JoystickInfo = UWindowsClient::Joysticks(JoystickIndex);
				if ( JoystickInfo.DirectInput8Joystick )
				{
					JoystickInfo.DirectInput8Joystick->Unacquire();
					JoystickInfo.DirectInput8Joystick->SetCooperativeLevel(Window,DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
				}
			}
			if(ViewportClient && !PreventCapture)
				ViewportClient->ReceivedFocus(this);
			UpdateModifierState();
			return 0;

		case WM_KILLFOCUS:
			if(ViewportClient)
				ViewportClient->LostFocus(this);
			return 0;

		case WM_DISPLAYCHANGE:
			if(bCapturingMouseInput)
			{
				CaptureMouseInput(FALSE);
				CaptureMouseInput(TRUE);
			}
			return 0;

		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 160;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 120; 
			break;

		case WM_MOUSEMOVE:
			if( !bCapturingMouseInput )
			{
				// the destruction of this will send the PostWindowsMessage callback
				FCallbackHelper CallbackHelper(this, Message);

				INT	X = GET_X_LPARAM(lParam),
					Y = GET_Y_LPARAM(lParam);

				if(ViewportClient)
					ViewportClient->MouseMove(this,X,Y);
				bMouseHasMoved = 1;
			}
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			{
				// the destruction of this will send the PostWindowsMessage callback
				FCallbackHelper CallbackHelper(this, Message);

				// Distinguish between left/right control/shift/alt.
				UBOOL ExtendedKey = (lParam & (1 << 24));
				UINT KeyCode;
				switch(wParam)
				{
					case VK_CONTROL:	KeyCode = (ExtendedKey ? VK_RCONTROL : VK_LCONTROL); break;
					case VK_SHIFT:		KeyCode = (ExtendedKey ? VK_RSHIFT : VK_LSHIFT); break;
					case VK_MENU:		KeyCode = (ExtendedKey ? VK_RMENU : VK_LMENU); break;
					default:			KeyCode = wParam; break;
				};

				// Get key code.
				FName* Key = Client->KeyMapVirtualToName.Find(KeyCode);

				if(!Key)
					break;

				// Update the cached key state.
				UBOOL OldKeyState = KeyStates[KeyCode];
				KeyStates[KeyCode] = TRUE;

				// Send key to input system.
 				if( *Key==KEY_Enter && (::GetKeyState(VK_MENU)&0x8000) )
				{
					Resize(GetSizeX(),GetSizeY(),!IsFullscreen());

					// enable mouse capture
					//@fixme - but this is undesired if the UI is active, since it will have released mouse capture.
					// currently this is hidden by the fact that UGameEngine::Tick() calls UpdateMouseCapture every frame
					// to guarantee that the mouse capture state is up to date, but if that call is removed for optimization,
					// this call to CaptureMouseInput() will need to be updated to be aware of other factors
					CaptureMouseInput(TRUE);
				}
				else if( ViewportClient )
				{
					if(!ViewportClient->InputKey(this,0,*Key,OldKeyState ? IE_Repeat : IE_Pressed))
					{
						// Pass the key event up the window heirarchy.
						return 1;
					}
				}
			}
			return 0;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			{
				// the destruction of this will send the PostWindowsMessage callback
				FCallbackHelper CallbackHelper(this, Message);

				// Distinguish between left/right control/shift/alt.
				UBOOL ExtendedKey = (lParam & (1 << 24));
				UINT KeyCode;
				switch(wParam)
				{
					case VK_CONTROL:	KeyCode = ExtendedKey ? VK_RCONTROL : VK_LCONTROL; break;
					case VK_SHIFT:		KeyCode = ExtendedKey ? VK_RSHIFT : VK_LSHIFT; break;
					case VK_MENU:		KeyCode = ExtendedKey ? VK_RMENU : VK_LMENU; break;
					default:			KeyCode = wParam; break;
				};

				// Get key code.
				FName* Key = Client->KeyMapVirtualToName.Find(KeyCode);

				if( !Key )
				{
					break;
				}

				// Update the cached key state.
				KeyStates[KeyCode] = FALSE;

				// Send key to input system.
				if( ViewportClient )
				{
					if(!ViewportClient->InputKey(this,0,*Key,IE_Released))
					{
						// Pass the key event up the window heirarchy.
						return 1;
					}
				}
			}
			return 0;

		case WM_CHAR:
			{
				// the destruction of this will send the PostWindowsMessage callback
				FCallbackHelper CallbackHelper(this, Message);

				TCHAR Character = GUnicodeOS ? wParam : TCHAR(wParam);
				if(ViewportClient)
				{
					ViewportClient->InputChar(this,0,Character);
				}
			}
			return 0;

#if WITH_IME
		case WM_IME_COMPOSITION:
			{
				HIMC Imc = ImmGetContext( Window );
				if( !Imc )
				{
					appErrorf( TEXT( "No IME context" ) );
				}

				if( lParam & GCS_RESULTSTR )
				{
					// Get the size of the result string.
					INT Size = ImmGetCompositionString( Imc, GCS_RESULTSTR, NULL, 0 );

					TCHAR* String = new TCHAR[Size + 1];
					appMemzero( String, sizeof( TCHAR ) * ( Size + 1 ) );

					// Get the result strings that is generated by IME.
					Size = ImmGetCompositionString( Imc, GCS_RESULTSTR, String, Size );
					Size /= sizeof( TCHAR );

					for( INT i = 0; i < CurrentIMESize; i++ )
					{
						ViewportClient->InputKey( this, 0, KEY_BackSpace, IE_Pressed );
						ViewportClient->InputKey( this, 0, KEY_BackSpace, IE_Released );
					}

					for( INT i = 0; i < Size; i++ )
					{
						INT Key = String[i];
						if( Key )
						{
							ViewportClient->InputChar( this, 0, Key );
						}
					}

					delete [] String;

					ImmReleaseContext( Window, Imc );

					CurrentIMESize = 0;
				}
				else if( lParam & GCS_COMPSTR ) 
				{
					// Get the size of the result string.
					INT Size = ImmGetCompositionString( Imc, GCS_COMPSTR, NULL, 0 );

					TCHAR* String = new TCHAR[Size + 1];
					appMemzero( String, sizeof( TCHAR ) * ( Size + 1 ) );

					// Get the result strings that is generated by IME.
					Size = ImmGetCompositionString( Imc, GCS_COMPSTR, String, Size );
					Size /= sizeof( TCHAR );

					for( INT i = 0; i < CurrentIMESize; i++ )
					{
						ViewportClient->InputKey( this, 0, KEY_BackSpace, IE_Pressed );
						ViewportClient->InputKey( this, 0, KEY_BackSpace, IE_Released );
					}

					for( INT i = 0; i < Size; i++ )
					{
						INT Key = String[i];
						if( Key )
						{
							ViewportClient->InputChar( this, 0, Key );
						}
					}

					delete [] String;

					ImmReleaseContext( Window, Imc );

					CurrentIMESize = Size;
				}
				else
				{
					return DefWindowProcX( Window, Message, wParam, lParam );
				}

				return 0;
			}
#endif
		case WM_ERASEBKGND:
			return 1;

		case WM_SETCURSOR:
			if( IsFullscreen() )
			{
				SetCursor( NULL );
				return 1;
			}
			else
			{
				RECT	ClientRect;
				INT		MouseX		= GetMouseX(),
						MouseY		= GetMouseY();
				::GetClientRect( Window, &ClientRect );
			
				EMouseCursor	Cursor = MC_Arrow;
				if(ViewportClient)
				{
					Cursor = ViewportClient->GetCursor(this,MouseX,MouseY);
				}


				if ( Cursor == MC_None )
				{
					SetCursor(NULL);
					return 1;
				}
				else if(Cursor == MC_NoChange)
				{
					// Intentionally do not set the cursor.
					return 1;
				}
				else
				{
					LPCTSTR	CursorResource = IDC_ARROW;
					switch(Cursor)
					{
					case MC_Arrow:				CursorResource = IDC_ARROW; break;
					case MC_Cross:				CursorResource = IDC_CROSS; break;
					case MC_SizeAll:			CursorResource = IDC_SIZEALL; break;
					case MC_SizeUpRightDownLeft:CursorResource = IDC_SIZENESW; break;
					case MC_SizeUpLeftDownRight:CursorResource = IDC_SIZENWSE; break;
					case MC_SizeLeftRight:		CursorResource = IDC_SIZEWE; break;
					case MC_SizeUpDown:			CursorResource = IDC_SIZENS; break;
					case MC_Hand:				CursorResource = IDC_HAND; break;
					};

					::SetCursor(::LoadCursor(NULL,CursorResource));
						return 1;
				}
			}
			break;

		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
			{		
				// the destruction of this will send the PostWindowsMessage callback
				FCallbackHelper CallbackHelper(this, Message);

				FName Key;
				if( Message == WM_LBUTTONDBLCLK )
					Key = KEY_LeftMouseButton;
				else
				if( Message == WM_MBUTTONDBLCLK )
					Key = KEY_MiddleMouseButton;
				else 
				if( Message == WM_RBUTTONDBLCLK )
					Key = KEY_RightMouseButton;
				else
				if( Message == WM_XBUTTONDBLCLK )
				{
					Key = KEY_ThumbMouseButton;
				}
				
				if(ViewportClient)
				{
					::SetFocus( Window );			// Focus issues with Viewports: Force focus on Window
					ViewportClient->InputKey(this,0,Key,IE_DoubleClick);
				}
			}			
			bMouseHasMoved = 0;
			return 0;
	
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			{
				// the destruction of this will send the PostWindowsMessage callback
				FCallbackHelper CallbackHelper(this, Message);

				FName Key;
				if( Message == WM_LBUTTONDOWN )
				{
					Key = KEY_LeftMouseButton;
				}
				else if( Message == WM_MBUTTONDOWN )
				{
					Key = KEY_MiddleMouseButton;
				}
				else if( Message == WM_RBUTTONDOWN )
				{
					Key = KEY_RightMouseButton;
				}
				else if( Message == WM_XBUTTONDOWN )
				{
					Key = KEY_ThumbMouseButton;
				}
			
				if ( Client )
				{
					BYTE* KeyIndex = Client->KeyMapNameToVirtual.Find(Key);
					if ( KeyIndex )
					{
						KeyStates[*KeyIndex] = TRUE;
					}
				}
			
				if(ViewportClient)
				{
					::SetFocus( Window );			// Focus issues with Viewports: Force focus on Window
					ViewportClient->InputKey(this,0,Key,IE_Pressed);
				}
			}
			bMouseHasMoved = 0;
			return 0;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
			{
				// the destruction of this will send the PostWindowsMessage callback
				FCallbackHelper CallbackHelper(this, Message);

				// allow mouse capture to resume after resizing
				PreventCapture = 0;

				FName Key;
				if( Message == WM_LBUTTONUP )
					Key = KEY_LeftMouseButton;
				else if( Message == WM_MBUTTONUP )
					Key = KEY_MiddleMouseButton;
				else if( Message == WM_RBUTTONUP )
					Key = KEY_RightMouseButton;
				else if( Message == WM_XBUTTONUP )
					Key = KEY_ThumbMouseButton;

				if ( Client )
				{
					BYTE* KeyIndex = Client->KeyMapNameToVirtual.Find(Key);
					if ( KeyIndex )
					{
						KeyStates[*KeyIndex] = FALSE;
					}
				}

				if(ViewportClient)
				{
					ViewportClient->InputKey(this,0,Key,IE_Released);
			}
			}
			return 0;

		case WM_MOUSEWHEEL:
			if(!bCapturingMouseInput)
			{
				// the destruction of this will send the PostWindowsMessage callback
				FCallbackHelper CallbackHelper(this, Message);

				if(ViewportClient)
				{
					SWORD zDelta = HIWORD(wParam);
					if(zDelta > 0)
					{
						ViewportClient->InputKey(this, 0, KEY_MouseScrollUp, IE_Pressed);
						ViewportClient->InputKey(this, 0, KEY_MouseScrollUp, IE_Released);
					}
					else if(zDelta < 0)
					{
						ViewportClient->InputKey(this, 0, KEY_MouseScrollDown, IE_Pressed);
						ViewportClient->InputKey(this, 0, KEY_MouseScrollDown, IE_Released);
					}
				}
			}
			break;

		case WM_ENTERSIZEMOVE:
			Resizing = 1;
			CaptureMouseInput(FALSE);
			break;

		case WM_EXITSIZEMOVE:
			Resizing = 0;
			HandlePossibleSizeChange();
			break;

		case WM_SIZING:
			// Flush the rendering command queue to ensure that there aren't pending viewport draw commands for the old viewport size.
			FlushRenderingCommands();
			break;

		case WM_SIZE:
			if ( !bPerformingSize )
			{
				if(bCapturingMouseInput)
				{
					CaptureMouseInput(FALSE);
					CaptureMouseInput(TRUE);
				}
				// Show mouse cursor if we're being minimized.
				if( SIZE_MINIMIZED == wParam )
				{
					CaptureMouseInput( FALSE );
					Minimized = true;
					Maximized = false;
				}
				else if( SIZE_MAXIMIZED == wParam )
				{
					Minimized = false;
					Maximized = true;
					HandlePossibleSizeChange();
				}
				else if( SIZE_RESTORED == wParam )
				{
					if( Maximized )
					{
						Maximized = false;
						HandlePossibleSizeChange();
					}
					else if( Minimized)
					{
						Minimized = false;
						HandlePossibleSizeChange();
					}
					else
					{
						// Game:
						// If we're neither maximized nor minimized, the window size 
						// is changing by the user dragging the window edges.  In this 
						// case, we don't reset the device yet -- we wait until the 
						// user stops dragging, and a WM_EXITSIZEMOVE message comes.
						if(!Resizing)
							HandlePossibleSizeChange();
					}
				}
			}
			break;

		case WM_SYSCOMMAND:
			// Prevent moving/sizing and power loss in fullscreen mode.
			switch( wParam )
			{
				case SC_SCREENSAVE:
					return 1;

				case SC_MOVE:
				case SC_SIZE:
				case SC_MAXIMIZE:
				case SC_KEYMENU:
				case SC_MONITORPOWER:
					if( IsFullscreen() )
						return 1;
					break;
			}
			break;

		case WM_NCHITTEST:
			// Prevent the user from selecting the menu in fullscreen mode.
			if( IsFullscreen() )
				return HTCLIENT;
			break;

		case WM_POWER:
			if( PWR_SUSPENDREQUEST == wParam )
			{
				debugf( NAME_Log, TEXT("Received WM_POWER suspend") );
				return PWR_FAIL;
			}
			else
				debugf( NAME_Log, TEXT("Received WM_POWER") );
			break;

		default:
			break;
	}
	return DefWindowProcX( Window, Message, wParam, lParam );
}

/*
 * Resends the state of the modifier keys (Ctrl, Shift, Alt).
 * Used when receiving focus, otherwise these keypresses may
 * be lost to some other process in the system.
 */
void FWindowsViewport::UpdateModifierState()
{
	if ( GShouldEnqueueModifierStateUpdates )
	{
		bUpdateModifierStateEnqueued = TRUE;
	}
	else
	{
		// Clear any enqueued UpdateModifierState requests.
		bUpdateModifierStateEnqueued = FALSE;
		UpdateModifierState( VK_LCONTROL );
		UpdateModifierState( VK_RCONTROL );
		UpdateModifierState( VK_LSHIFT );
		UpdateModifierState( VK_RSHIFT );
		UpdateModifierState( VK_LMENU );
		UpdateModifierState( VK_RMENU );
	}
}

/*
 * Resends the state of the specified key to the viewport client.
 * It would've been nice to call InputKey only if the current state differs from what 
 * FEditorLevelViewportClient::Input thinks, but I can't access that here... :/
 */
void FWindowsViewport::UpdateModifierState( INT VirtKey )
{
	if ( !ViewportClient || !Client )
	{
		return;
	}

	FName* Key = Client->KeyMapVirtualToName.Find( VirtKey );
	if (!Key)
	{
		return;
	}

	KeyStates[VirtKey] = FALSE;
	INT bDown = (::GetKeyState(VirtKey) & 0x8000) ? TRUE : FALSE;
	ViewportClient->InputKey(this, 0, *Key, bDown ? IE_Pressed : IE_Released );
}





