/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#ifndef _WINDOWSTARGET_H_
#define _WINDOWSTARGET_H_

#include "Common.h"
#include "..\..\IpDrv\Inc\GameType.h"
#include "Socket.h"
#include "RefCountedTarget.h"

#define PACKET_BUFFER_SIZE 1024

/** Number of milliseconds between connection retries */
#define RECONNECT_RETRY_TIME 1000

void DebugOutput( const char* Buffer);

/** Available platform types; mirror of UnFile.h */
enum EPlatformType
{
	EPlatformType_Unknown = 0x00,
	EPlatformType_Windows = 0x01,
	EPlatformType_WindowsConsole = 0x02,     // Windows platform cooked as console without editor support
	EPlatformType_WindowsServer = 0x04,		// Windows platform dedicated server mode (like above but "lean and mean")
	EPlatformType_Xenon = 0x08,
	EPlatformType_PS3 = 0x10,
	EPlatformType_Linux = 0x20,
	EPlatformType_MacOSX = 0x40,
	EPlatformType_IPhone = 0x80,
	EPlatformType_Max
};

class CWindowsTarget;

/// Representation of a single UE3 instance running on PC
class CWindowsTarget : public FRefCountedTarget<CWindowsTarget>
{
public:
	/** User friendly name of the target. */
	wstring Name;
	/** Computer name. */
	wstring ComputerName;
	/** Game name. */
	wstring GameName;
	/** Configuration name. */
	wstring Configuration;
	/** Game type id. */
	EGameType GameType;
	/** Game type name. */
	wstring GameTypeName;
	/** Platform id. */
	EPlatformType PlatformType;
	/** Number of ticks since the last connection retry */
	DWORD LastConnectionTicks;
	/** Number of reconnect tries */
	INT ReconnectTries;

	/** The callback for TTY notifications. */
	TTYEventCallbackPtr TxtCallback;

	const sockaddr_in& GetRemoteAddress() const 
	{ 
		return RemoteAddress; 
	}

	/** TCP client used to communicate with the game. */
	FSocket TCPClient;

	/** True if something is currently connected to this target. */
	bool bConnected;
	/** TRUE if we got a socket error while trying to send. Will disconnect on next tick*/
	bool bHadSocketError;

	/** Constructor. */
	CWindowsTarget(const sockaddr_in& InRemoteAddress);	

	virtual ~CWindowsTarget();

	// ticks the connection, receiving and dispatching messages
	void Tick();

	bool Connect();
	void Reconnect();
	void Disconnect();

	// asynchronous send
	bool SendConsoleCommand(const char* Message);

	inline bool IsConnected() const 
	{ 
		return bConnected; 
	}

private:
	/** The remote address of the target */
	sockaddr_in RemoteAddress;

	/** Sends TTY text on the callback if it is valid. */
	void SendTTY(const char* Txt);

	char PacketBuffer[PACKET_BUFFER_SIZE+1];
};

#endif
