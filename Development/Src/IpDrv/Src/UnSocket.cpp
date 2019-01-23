/*============================================================================
	UnSocket.cpp: Common interface for WinSock and BSD sockets.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
============================================================================*/

#include "UnIpDrv.h"

#if _MSC_VER
#include "UnSocketWin.h"
#endif

/** The global socket subsystem pointer */
FSocketSubsystem* GSocketSubsystem = NULL;

/*----------------------------------------------------------------------------
	Resolve.
----------------------------------------------------------------------------*/

FResolveInfo::FResolveInfo(const TCHAR* InHostName) :
	Error(NULL),
	ErrorCode(0),
    bWasResolved(FALSE),
	bShouldAbandon(FALSE)
{
	ErrorBuffer[0] = '\0';
	appMemcpy(HostName, TCHAR_TO_ANSI(InHostName), appStrlen(InHostName) + 1);
	if (GThreadPool != NULL)
	{
		// Queue this to our worker thread(s) for resolving
		GThreadPool->AddQueuedWork(this);
	}
}

/**
 * Called by a thread from the thread pool. Does host name resolution asynchronously
 */
void FResolveInfo::DoWork(void)
{
	// PC/Xe code
#if _MSC_VER
	Addr.SetIp(0);
	// Make up to 3 attempts to resolve it
   	for (INT AttemptCount = 0;
		// Stop if we've hit the limit of checks, been told to or found it
		bShouldAbandon == FALSE && AttemptCount < 3;
		AttemptCount++)
	{
		ErrorCode = GSocketSubsystem->GetHostByName(HostName,Addr);
		if (IsSocketError(ErrorCode))
		{
			if (ErrorCode == SE_HOST_NOT_FOUND || ErrorCode == SE_NO_DATA)
			{
				// Force a failure
				AttemptCount = 3;
			}
		}
	}
#endif
}

/**
 * Tells the consumer thread that the resolve is done
 */ 
void FResolveInfo::Dispose(void)
{
	if (IsSocketError(ErrorCode))
	{
		appSprintf(ErrorBuffer, TEXT("Can't find host %s (%s)"), ANSI_TO_TCHAR(HostName),GSocketSubsystem->GetSocketError(ErrorCode));
		// Allow other threads to see this buffer in an atomic operation
		appInterlockedExchange((INT*)&Error,(INT)(PTRINT)ErrorBuffer);
	}

	// Atomicly update our "done" flag
	appInterlockedExchange((INT*)&bWasResolved,TRUE);
}

//
// FSocketData functions
//

FString FSocketData::GetString( UBOOL bAppendPort )
{
	return Addr.ToString(bAppendPort);
}

void FSocketData::UpdateFromSocket(void)
{
	if (Socket != NULL)
	{
		Addr = Socket->GetAddress();
		Addr.GetPort(Port);
	}
}

//
// FIpAddr functions
//

FIpAddr::FIpAddr(const FInternetIpAddr& SockAddr)
{
	FIpAddr New = SockAddr.GetAddress();
	Addr = New.Addr;
	Port = New.Port;
}

FString FIpAddr::GetString( UBOOL bAppendPort )
{
	// Get the individual bytes
	INT A = (Addr >> 24) & 0xFF;
	INT B = (Addr >> 16) & 0xFF;
	INT C = (Addr >> 8) & 0xFF;
	INT D = Addr & 0xFF;
	if (bAppendPort)
	{
		return FString::Printf(TEXT("%i.%i.%i.%i:%i"),A,B,C,D,Port);
	}
	else
	{
		return FString::Printf(TEXT("%i.%i.%i.%i"),A,B,C,D);
	}
}

/**
 * Builds an internet address from this host ip address object
 */
FInternetIpAddr FIpAddr::GetSocketAddress(void)
{
	FInternetIpAddr Result;
	Result.SetIp(Addr);
	Result.SetPort(Port);
	return Result;
}

