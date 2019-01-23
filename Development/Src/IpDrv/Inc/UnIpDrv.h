/*=============================================================================
	IpDrvPrivate.h: Unreal TCP/IP driver.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef UNIPDRV_H
#define UNIPDRV_H

// include for all definitions
#include "Engine.h"

#if _MSC_VER
	#pragma warning( disable : 4201 )
#endif

// Set the socket api name depending on platform
#if _MSC_VER
#if XBOX || WITH_PANORAMA
	#define SOCKET_API TEXT("LiveSock")
#else
	#define SOCKET_API TEXT("WinSock")
#endif
#elif PS3
	#define SOCKET_API TEXT("PS3Sockets")
#else
	#define SOCKET_API TEXT("Sockets")
#endif

// WinSock includes.
#if _MSC_VER
	typedef int socklen_t;
#ifndef XBOX

#pragma pack(push,8)
#if !WITH_PANORAMA
	#include <winsock.h>
#else
	#include <WinSock2.h>
#endif
#pragma pack(pop)

	#include <conio.h>
#endif
#endif

// BSD socket includes.
#if !_MSC_VER

	// Temporarily disable our clock() override (for unistd.h --> ... -> time.h)
	#ifdef clock
		#undef clock
	#endif

	#include <stdio.h>
	#include <unistd.h>
	#include <sys/types.h>
#if PS3
	#include <common/include/sdk_version.h>
	#if CELL_SDK_VERSION < 0x080004
		#include <network/sys/ansi.h>
		#include <network/sys/socket.h>
		#include <network/netinet/in.h>
		#include <network/arpa/inet.h>
		#include <network/netdb.h>
		#include <network/netex/errno.h>
		#include <network/netex/net.h>
		#include <network/sys/time.h>
		#include <network/sys/select.h>
//		#include <sys/uio.h>
//		#include <sys/ioctl.h>
//		#include <sys/time.h>
//		#include <pthread.h>
	#else
		#include <sys/ansi.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <netdb.h>
		#include <netex/errno.h>
		#include <netex/net.h>
		#include <sys/time.h>
		#include <sys/select.h>
	#endif
#else
	#include <errno.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <sys/uio.h>
	#include <sys/ioctl.h>
	#include <sys/time.h>
	#include <pthread.h>
#endif
	// Handle glibc < 2.1.3
	#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0x4000
	#endif

	#ifdef clock
		#undef clock
	#endif
	#define clock(Timer)   {Timer -= appCycles();}

#endif	// !_MSC_VER


/*-----------------------------------------------------------------------------
	Includes..
-----------------------------------------------------------------------------*/

#include "UnNet.h"
// Base socket interfaces
#include "UnSocket.h"
// Platform specific implementations
#if PS3
#include "UnSocketPS3.h"
#else
#include "UnSocketWin.h"
#endif

/*-----------------------------------------------------------------------------
	Definitions.
-----------------------------------------------------------------------------*/

// Globals.
extern UBOOL GIpDrvInitialized;

/** Network communication between UE3 executable and the "Unreal Console" tool. */
extern class FDebugServer*				GDebugChannel;

/*-----------------------------------------------------------------------------
	More Includes.
-----------------------------------------------------------------------------*/

#include "InternetLink.h"
#include "FRemotePropagator.h"
#include "FDebugServer.h"
#include "DebugServerDefs.h"

/*-----------------------------------------------------------------------------
	Host resolution thread.
-----------------------------------------------------------------------------*/

//
// Class for creating a background thread to resolve a host.
//
class FResolveInfo : public FQueuedWork
{
public:
	// Variables.
	FInternetIpAddr		Addr;
	ANSICHAR	HostName[256];
	/** This value is atomicly updated in a thread safe way */
	TCHAR*		Error;
	/**
	 * Holds the error string in a separate buffer so that one thread can write
	 * to it without worrying about another thread getting partial reads
	 */
	TCHAR		ErrorBuffer[MAX_SPRINTF];
	/** Error code returned by GetHostByName. */
	INT			ErrorCode;
	UBOOL		bWasResolved;
	/** Tells the worker thread whether it should abandon it's work or not */
	UBOOL bShouldAbandon;

	FResolveInfo(const TCHAR* InHostName);

	~FResolveInfo()
	{
	}

//FQueuedWork interface

	/**
	 * Resolves the specified host name
	 */
	virtual void DoWork(void);

	/**
	 * Tells the thread to quit trying to resolve
	 */
	virtual void Abandon(void)
	{
		appInterlockedExchange(&bShouldAbandon,TRUE);
	}

	/**
	 * Tells the consumer thread that the resolve is done
	 */ 
	virtual void Dispose(void);

	// Accessors

	UBOOL Resolved()
	{
		return bWasResolved;
	}
	const TCHAR* GetError()
	{
		return Error;
	}
	FInternetIpAddr GetAddr()
	{
		return Addr;
	}
	const FString GetHostName()
	{
		return FString(HostName);
	}
};

/*-----------------------------------------------------------------------------
	Bind to next available port.
-----------------------------------------------------------------------------*/

//
// Bind to next available port.
//
inline INT bindnextport( FSocket* Socket, FInternetIpAddr& Addr, INT portcount, INT portinc )
{
	for( INT i=0; i<portcount; i++ )
	{
		if( Socket->Bind(Addr) == TRUE )
		{
			if (Addr.GetPort() != 0)
			{
				return Addr.GetPort();
			}
			else
			{
				return Socket->GetPortNo();
			}
		}
		if( Addr.GetPort() == 0 )
			break;
		Addr.SetPort(Addr.GetPort() + portinc);
	}
	return 0;
}

//
// Get local IP to bind to
//
inline FInternetIpAddr getlocalbindaddr( FOutputDevice& Out )
{
	FInternetIpAddr BindAddr;
	// If we can bind to all addresses, return 0.0.0.0
	if (GSocketSubsystem->GetLocalHostAddr(Out,BindAddr) == TRUE)
	{
		BindAddr.SetAnyAddress();
	}
	return BindAddr;

}


/*-----------------------------------------------------------------------------
	Public includes.
-----------------------------------------------------------------------------*/

#include "VoiceInterface.h"
// Common code shared across all platforms for the online subsystem
#include "OnlineSubsystemUtilities.h"

#include "IpDrvClasses.h"

#include "UnTcpNetDriver.h"

// Per platform driver/connection implementations
#if PS3
#endif

#include "UnStatsNotifyProviders_UDP.h"

#endif // UNIPDRV_H

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

