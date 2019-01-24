/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

// ATLUE3Control.h : Declaration of the CATLUE3Control
#pragma once
#include "resource.h"       // main symbols
#include <atlctl.h>
#include "ATLUE3_i.h"

#include "../Common/PIBCommon.h"
#include <Winhttp.h>

#ifdef _INC_WINDOWSX
#undef SubclassWindow
#endif //_INC_WINDOWSX

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

#define DOWNLOAD_BUFFER_SIZE	4096

class FileDescAX : public FileDescCommon
{
public:
	// A reference count
	LONG ReferenceCount;
	// A lock used for multithreaded safety
	CRITICAL_SECTION Lock;
	// Set to true when the lock is initialised
	bool LockInitialised;
	// A handle for the current connection
	HINTERNET Connection;
	// A handle for the current request
	HINTERNET Request;
	// A handle for the thread signal
	HANDLE RequestFinished;
	// A buffer to download data into
	BYTE Buffer[DOWNLOAD_BUFFER_SIZE];

	FileDescAX( void ) : FileDescCommon()
	{
		ReferenceCount = 0;
		LockInitialised = false;
		Connection = INVALID_HANDLE_VALUE;
		Request = INVALID_HANDLE_VALUE;
		RequestFinished = INVALID_HANDLE_VALUE;
	}

	FileDescAX( INT ID, string RootFolder, string InName, DWORD InSize, string InMD5Sum );

	FileDescAX( const FileDescCommon* Copy, HINTERNET InConnection, HINTERNET InRequest );

	~FileDescAX( void );

	bool	LockRequestHandle( void );
	void	UnlockRequestHandle( void );

	void	Cancel( void );
};

/**
 * Main ATL control that handles the interface between Internet Explorer and UE3
 */
class ATL_NO_VTABLE CATLUE3Control :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IATLUE3Control, &IID_IATLUE3Control, &LIBID_ATLUE3Lib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IPersistStreamInitImpl<CATLUE3Control>,
	public IPersistPropertyBagImpl<CATLUE3Control>,
	public IOleControlImpl<CATLUE3Control>,
	public IOleObjectImpl<CATLUE3Control>,
	public IOleInPlaceActiveObjectImpl<CATLUE3Control>,
	public IViewObjectExImpl<CATLUE3Control>,
	public IOleInPlaceObjectWindowlessImpl<CATLUE3Control>,
#ifdef _WIN32_WCE // IObjectSafety is required on Windows CE for the control to be loaded correctly
	public IObjectSafetyImpl<CATLUE3Control, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
#endif
	public CComCoClass<CATLUE3Control, &CLSID_ATLUE3Control>,
	public CComControl<CATLUE3Control>,
	public PluginCommon
{
public:

DECLARE_OLEMISC_STATUS(	OLEMISC_RECOMPOSEONRESIZE |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_INSIDEOUT |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST
)

DECLARE_REGISTRY_RESOURCEID( IDR_ATLUE3CONTROL )


BEGIN_COM_MAP( CATLUE3Control )
	COM_INTERFACE_ENTRY( IATLUE3Control )
	COM_INTERFACE_ENTRY( IDispatch )
	COM_INTERFACE_ENTRY( IViewObjectEx )
	COM_INTERFACE_ENTRY( IViewObject2 )
	COM_INTERFACE_ENTRY( IViewObject )
	COM_INTERFACE_ENTRY( IOleInPlaceObjectWindowless )
	COM_INTERFACE_ENTRY( IOleInPlaceObject )
	COM_INTERFACE_ENTRY2( IOleWindow, IOleInPlaceObjectWindowless )
	COM_INTERFACE_ENTRY( IOleInPlaceActiveObject )
	COM_INTERFACE_ENTRY( IOleControl )
	COM_INTERFACE_ENTRY( IOleObject )
	COM_INTERFACE_ENTRY( IPersistStreamInit )
	COM_INTERFACE_ENTRY2( IPersist, IPersistStreamInit )
	COM_INTERFACE_ENTRY( IPersistPropertyBag )
#ifdef _WIN32_WCE // IObjectSafety is required on Windows CE for the control to be loaded correctly
	COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafety)
#endif
END_COM_MAP()

BEGIN_PROP_MAP( CATLUE3Control )
	PROP_DATA_ENTRY( "_cx", m_sizeExtent.cx, VT_UI4 )
	PROP_DATA_ENTRY( "_cy", m_sizeExtent.cy, VT_UI4 )
	PROP_DATA_ENTRY( "Debug", DebugMode, VT_UI1 )
END_PROP_MAP()


BEGIN_MSG_MAP( CATLUE3Control )
END_MSG_MAP()

// IViewObjectEx
	DECLARE_VIEW_STATUS( VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE )

// IATLUE3Control
						CATLUE3Control( void );

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT				FinalConstruct( void );

	void				FinalRelease( void );

	HRESULT				OnDrawAdvanced( ATL_DRAWINFO& di );
	HRESULT				InPlaceActivate(LONG iVerb, const RECT* prcPosRect = NULL);
	HRESULT __stdcall	InPlaceDeactivate( void );

	/** Helper functions */
	LRESULT				InitDLL( void );
	void				Destroy( void );

	/** Session is global to all URL requests */
	bool				InitSession( void );
	void				DestroySession( void );

	void				InitTickLoop( void );
	bool				InitConnection( void );
	void				GetFileList( void );
	void				WaitForFileList( void );
	void				RequestFile( void );
	void				WaitForFiles( void );

	/** WinHttp helpers */
	FileDescAX*			InitiateConnection( const wchar_t* Server, const wchar_t* URL, INTERNET_PORT Port );
	FileDescAX*			InitiateConnection( const wchar_t* Server, const char* URL, INTERNET_PORT Port );

	static VOID CALLBACK		AsyncCallback( HINTERNET Internet, DWORD_PTR Context, DWORD InternetStatus, LPVOID StatusInformation, DWORD StatusInformationLength );
	static bool					HeadersAvailable( FileDescAX* ReqFile );
	static bool					ReadComplete( FileDescAX* ReqFile, DWORD StatusInformationLength );

	HWND						ExplorerWindow;

private:
	HINTERNET					Session;							// Used for all connections
	FileDescAX*					FileList;
};

OBJECT_ENTRY_AUTO( __uuidof( ATLUE3Control ), CATLUE3Control )
