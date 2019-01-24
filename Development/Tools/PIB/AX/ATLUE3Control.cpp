/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "stdafx.h"
#include "ATLUE3Control.h"

CATLUE3Control::CATLUE3Control( void )
	: PluginCommon()
{
	Session = INVALID_HANDLE_VALUE;
	ExplorerWindow = NULL;
	FileList = NULL;
}

HRESULT CATLUE3Control::FinalConstruct( void )
{
	return( S_OK );
}

void CATLUE3Control::FinalRelease( void )
{
}

DWORD __stdcall TickLoop( void* Parameter )
{
	CATLUE3Control* Control = ( CATLUE3Control* )Parameter;

	while( Control->PluginStage != Stage_Exiting )
	{
		switch( Control->PluginStage )
		{
		case Stage_GetFileList:
			Control->InitSession();
			Control->GetFileList();
			break;

		case Stage_WaitingForFileList:
			Control->WaitForFileList();
			break;

		case Stage_GetFiles:
			Control->RequestFile();
			Control->WaitForFiles();
			break;

		case Stage_WaitForFiles:
		case Stage_WaitForAllFiles:
			Control->WaitForFiles();
			break;

		case Stage_InitDLL:
			Control->DestroySession();
			InvalidateRect( Control->PIBWindow, NULL, FALSE );
			break;

		case Stage_Running:
			// Tick the game thread
			while( Control->PluginStage != Stage_Exiting )
			{
				InvalidateRect( Control->ExplorerWindow, NULL, FALSE );
				Sleep( 15 );
			}
			break;
		}
	}

	return( 1 );
}

void CATLUE3Control::Destroy( void )
{
	PluginCommon::Destroy();
}

HRESULT CATLUE3Control::InitDLL( void )
{
	Width = m_rcPos.right - m_rcPos.left;
	Height = m_rcPos.bottom - m_rcPos.top;

	PluginCommon::InitDLL();

	return( E_FAIL );
}

void CATLUE3Control::InitTickLoop( void )
{
	PluginCommon::InitTickLoop( &TickLoop, this );

	PIBWindow = Create( ExplorerWindow, NULL, NULL, WS_CHILD | WS_VISIBLE, 0, 0U, NULL );
}

HRESULT CATLUE3Control::InPlaceDeactivate( void )
{
	PluginStage = Stage_Exiting;

	WaitForSingleObject( ThreadHandle, INFINITE );

	Destroy();

	return( IOleInPlaceObjectWindowlessImpl<CATLUE3Control>::InPlaceDeactivate() );
}

HRESULT CATLUE3Control::InPlaceActivate( LONG iVerb, const RECT* prcPosRect )
{
	if( CComControlBase::InPlaceActivate( iVerb, prcPosRect ) == S_OK )
	{
		if( m_spInPlaceSite->GetWindow( &ExplorerWindow ) == S_OK )
		{
			InitTickLoop();
			return( S_OK );
		}
	}

	return( E_FAIL );
}

HRESULT CATLUE3Control::OnDrawAdvanced( ATL_DRAWINFO& )
{
	assert( GetCurrentThreadId() == PluginThreadId );

	switch( PluginStage )
	{
	case Stage_InitDLL:
		InitDLL();
		break;

	case Stage_Running:
		IPIB->Tick();
		break;

	default:
		break;
	}

	// Return as handled
	return( S_OK );
}

// CATLUE3Control
