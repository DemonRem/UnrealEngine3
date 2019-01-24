/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "NetworkManager.h"
#include "..\..\IpDrv\Inc\GameType.h"
#include "ByteStreamConverter.h"
#include "StringUtils.h"

#define DEFAULT_DEBUG_SERVER_PORT 13650
extern void DebugOutput(const wchar_t* Buffer);

FNetworkManager::FNetworkManager() 
: bInitialized(false)
, BroadcastSocket(NULL)
, LastPingTime(0)
, bDoSubnetSearch(false)
{
	GetSystemInfo(&SysInfo);
}

FNetworkManager::~FNetworkManager()
{
	Cleanup();
}

/**
 * Initalizes winsock and the FNetworkManager instance.
 */
void FNetworkManager::Initialize()
{
	if(bInitialized)
	{
		return;
	}

	DebugOutput(L"Initializing Network Manager...\n");

	// Init Windows Sockets
	WSADATA WSAData;
	WORD WSAVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(WSAVersionRequested, &WSAData) != 0)
	{
		return;
	}

	DebugOutput(L"WSA Sockets Initialized.\n");

	// make a broadcast UDP socket for use with auto-discovery
	BroadcastSocket = new FSocket(SF_IPv4, ST_Datagram, SP_UDP);
	if (!BroadcastSocket->CreateSocket())
	{
		DebugOutput(L"Unable to create UDP socket.\n");
		return;
	}

	// bind to all interfaces, using a port selected by the system
	BroadcastSocket->SetAddress("0.0.0.0");
	BroadcastSocket->SetPort(0);

	// bind the socket to the port/address
	if (!BroadcastSocket->Bind())
	{
		DebugOutput(L"Unable to bind UDP socket.\n");
		return;
	}
	BroadcastSocket->SetNonBlocking(true);
	BroadcastSocket->SetBroadcasting(true);

	// set up and save the broadcast address
	ZeroMemory(&BroadcastAddr, sizeof(BroadcastAddr));
	BroadcastAddr.sin_family = AF_INET;
	BroadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
	BroadcastAddr.sin_port = htons(DEFAULT_DEBUG_SERVER_PORT);

	// HACK: initializing to EPIC-SECURE subnet
	// SetSubnetSearch("10.1.33.0");

	bInitialized = true;
}

/**
 * Cleans up winsock and all of the resources allocated by the FNetworkManager instance.
 */
void FNetworkManager::Cleanup()
{
	if (bInitialized)
	{
		DebugOutput(L"Cleaning up UDP ping socket...\n");
		if (BroadcastSocket != NULL)
		{
			BroadcastSocket->Close();
			delete BroadcastSocket;
			BroadcastSocket = NULL;
		}

		DebugOutput(L"Cleaning up Network Manager...\n");

		TargetsLock.Lock();
		Targets.clear();
		TargetsLock.Unlock();

		WSACleanup();

		DebugOutput(L"Finished cleaning up Network Manager.\n");
		bInitialized = false;
	}
}

void FNetworkManager::SetSubnetSearch(const char* ip)
{	
	if (ip == NULL)
	{
		bDoSubnetSearch = false;
		return;
	}

	// initialize the SubnetSearchAddr
	ZeroMemory(&SubnetSearchAddr, sizeof(SubnetSearchAddr));
	SubnetSearchAddr.sin_family = AF_INET;
	SubnetSearchAddr.sin_addr.s_addr = inet_addr(ip);
	SubnetSearchAddr.sin_port = htons(DEFAULT_DEBUG_SERVER_PORT);

	// do the search as long as the IP parsed
	bDoSubnetSearch = (SubnetSearchAddr.sin_addr.s_addr != INADDR_NONE);
}
/**
 *	Attempts to determine available targets.
 */
void FNetworkManager::DetermineTargets()
{
	time_t Now = time(NULL);

	// do this no more than once per second
	if (Now != LastPingTime)
	{
		DebugOutput(L"Sending a UE3PING\n");

		// send a ping out on our UDP socket
		const static char PING[] = "UE3PING";
		BroadcastSocket->SetAddress(BroadcastAddr); // also sets port
		BroadcastSocket->SendTo(PING, sizeof(PING)-1);
		LastPingTime = Now;

		// spam the search subnet if any
		if (bDoSubnetSearch)
		{
			unsigned long addr = ntohl(SubnetSearchAddr.sin_addr.s_addr) & 0xFFFFFF00;
			for (int i = 1; i <= 254; i++)
			{
				SubnetSearchAddr.sin_addr.s_addr = htonl(addr | i);
				BroadcastSocket->SetAddress(SubnetSearchAddr); // also sets port
				BroadcastSocket->SendTo(PING, sizeof(PING)-1);
			}
		}
	}
	
	// Read the socket regularly for several seconds to see if anything responds
	for( int i = 0; i < 10; i ++ )
	{
		Sleep( 20 );
		UpdateTargets();
	}
}

void FNetworkManager::UpdateTargets()
{
	char Buffer[1600];
	bool ReadSomething = true;

	while (ReadSomething)
	{
		ReadSomething = false;

		// check the UDP socket for pongs
		sockaddr_in FromAddress;
		int BytesRead = BroadcastSocket->RecvFrom(Buffer, sizeof(Buffer)-1, FromAddress, SF_None);
		if (BytesRead >= 0)
		{
			ReadSomething = true;
			Buffer[BytesRead] = '\0'; // make sure we're terminated

			// make sure this is a PONG packet
			if (strncmp(Buffer, "UE3PONG", 7) == 0)
			{
				int DebugPort = 0;
				char CompName[512] = { 0 };
				char GameName[512] = { 0 };
				char GameType[512] = { 0 };
				char Platform[512] = { 0 };

				// parse the pong packet
				if (sscanf(Buffer, "UE3PONG\nDEBUGPORT=%d\nCNAME=%511s\nGAME=%511s\nGAMETYPE=%511s\nPLATFORM=%511s\n", 
					&DebugPort, &CompName, &GameName, &GameType, &Platform))
				{
					// If we aren't the Windows platform, skip to next entry
					if( strcmp( Platform, "PC" ) && strcmp( Platform, "PCServer" ) && strcmp( Platform, "PCConsole" ))
					{
						continue;
					}

					DebugOutput(L"Got a UE3PONG\n");

					// use the port specified in the PONG
					FromAddress.sin_port = htons((u_short)DebugPort);
					
					// find the target by IP
					TargetsLock.Lock();
					TargetPtr Ret;
					for(TargetMap::iterator Iter = Targets.begin(); Iter != Targets.end(); ++Iter)
					{
						if(memcmp(&(*Iter).second->GetRemoteAddress(), &FromAddress, sizeof(sockaddr_in)) == 0)
						{
							Ret = (*Iter).second;
							break;
						}
					}

					// add if necessary
					if (!Ret)
					{
						Ret = new CWindowsTarget(FromAddress);
						Targets[Ret.GetHandle()] = Ret;
					}

					// update with infp
					wchar_t WideBuf[1024] = { 0 };
					MultiByteToWideChar(CP_UTF8, 0, CompName, -1, WideBuf, sizeof(WideBuf)/sizeof(WideBuf[0]) - 1);
					Ret->Name = WideBuf;
					Ret->ComputerName = WideBuf;
					MultiByteToWideChar(CP_UTF8, 0, GameName, -1, WideBuf, sizeof(WideBuf)/sizeof(WideBuf[0]) - 1);
					Ret->GameName = WideBuf;
					Ret->Configuration = L"PC";
					MultiByteToWideChar(CP_UTF8, 0, GameType, -1, WideBuf, sizeof(WideBuf)/sizeof(WideBuf[0]) - 1);
					Ret->GameTypeName = WideBuf;
					if (strcmp(GameType, "Editor") == 0)
					{
						Ret->GameType = EGameType_Editor;
					}
					else if (strcmp(GameType, "Server") == 0)
					{
						Ret->GameType = EGameType_Server;
					}
					else if (strcmp(GameType, "Listen Server") == 0)
					{
						Ret->GameType = EGameType_ListenServer;
					}
					else if (strcmp(GameType, "Client") == 0)
					{
						Ret->GameType = EGameType_Client;
					}
					else
					{
						Ret->GameType = EGameType_Unknown;
					}
					Ret->PlatformType = EPlatformType_Windows;

					// unlock the mutex
					TargetsLock.Unlock();
				}
			}
		}
		else if (WSAGetLastError() == WSAECONNRESET)
		{
			// had to clear the error, just read again and it'll work
			ReadSomething = true;
		}
	}
}

/**
 * Retrieves a target with the specified IP Address.
 *
 * @param	Address		The address of the target to retrieve.
 * @return	NULL if the target could not be found, otherwise a valid reference pointer.
 */
TargetPtr FNetworkManager::GetTarget(const sockaddr_in &Address)
{
	TargetPtr Ret;

	TargetsLock.Lock();
	for(TargetMap::iterator Iter = Targets.begin(); Iter != Targets.end(); ++Iter)
	{
		if(memcmp(&(*Iter).second->GetRemoteAddress(), &Address, sizeof(sockaddr_in)) == 0)
		{
			Ret = (*Iter).second;
			break;
		}
	}
	TargetsLock.Unlock();

	return Ret;
}

/**
 * Retrieves a target with the specified IP Address.
 *
 * @param	Handle		The handle of the target to retrieve.
 * @return	NULL if the target could not be found, otherwise a valid reference pointer.
 */
TargetPtr FNetworkManager::GetTarget(const TARGETHANDLE Handle)
{
	TargetPtr ptr;

	TargetsLock.Lock();
	TargetMap::iterator Iter = Targets.find(Handle);
	if(Iter != Targets.end())
	{
		ptr = (*Iter).second;
	}
	TargetsLock.Unlock();

	return ptr;
}

/**
 * Removes the target with the specified address.
 *
 * @param	Handle		The handle of the target to be removed.
 */
void FNetworkManager::RemoveTarget(TARGETHANDLE Handle)
{
	TargetsLock.Lock();
	{
		TargetMap::iterator Iter = Targets.find(Handle);

		if(Iter != Targets.end())
		{
			Targets.erase(Iter);
		}
	}
	TargetsLock.Unlock();
}

/**
 *	Sends message using given UDP client.
 *	@param Handle			client to send message to
 *	@param MessageType		sent message type
 *	@param Message			actual text of the message
 *	@return true on success, false otherwise
 */
bool FNetworkManager::SendToConsole(const TARGETHANDLE Handle, const wchar_t* Message)
{
	TargetPtr Target = GetTarget(Handle);

	if(Target)
	{
		return Target->SendConsoleCommand(ToString(Message).c_str());
	}

	return false;
}

/**
 * Gets the default target.
 */
TargetPtr FNetworkManager::GetDefaultTarget()
{
	TargetPtr Ret;

	TargetsLock.Lock();
	TargetMap::iterator Iter = Targets.begin();

	if(Iter != Targets.end())
	{
		Ret = (*Iter).second;
	}

	TargetsLock.Unlock();

	return Ret;
}

/**
* Retrieves a handle to each available target.
*
* @param	OutTargetList			An array to copy all of the target handles into.
* @param	InOutTargetListSize		This variable needs to contain the size of OutTargetList. When the function returns it will contain the number of target handles copied into OutTargetList.
*/
void FNetworkManager::GetTargets(TARGETHANDLE *OutTargetList, int *InOutTargetListSize)
{
	TargetsLock.Lock();

	int TargetsCopied = 0;
	for(TargetMap::iterator Iter = Targets.begin(); Iter != Targets.end() && TargetsCopied < *InOutTargetListSize; ++Iter, ++TargetsCopied)
	{
		OutTargetList[TargetsCopied] = (*Iter).first;
	}

	TargetsLock.Unlock();

	*InOutTargetListSize = TargetsCopied;
}

/**
 * Connects to the target with the specified name.
 *
 * @param	TargetName		The name of the target to connect to.
 * @return	Handle to the target that was connected or else INVALID_TARGETHANDLE.
 */
TARGETHANDLE FNetworkManager::ConnectToTarget(const wchar_t* TargetName)
{
	// Find the target
	TargetPtr CurrentTarget;

	for(TargetMap::iterator Iter = Targets.begin(); Iter != Targets.end(); ++Iter)
	{
		if((*Iter).second->Name == TargetName)
		{
			CurrentTarget = (*Iter).second;
			break;
		}
	}

	if (CurrentTarget->Connect())
	{
		return CurrentTarget.GetHandle();
	}

	return INVALID_TARGETHANDLE;
}

/**
 * Connects to the target with the specified handle.
 *
 * @param	Handle		The handle of the target to connect to.
 */
bool FNetworkManager::ConnectToTarget(TARGETHANDLE Handle)
{
	TargetPtr Target = GetTarget(Handle);

	return Target->Connect();
}

/**
 * Returns the number of targets available.
 */
int FNetworkManager::GetNumberOfTargets()
{
	int Num = 0;

	TargetsLock.Lock();
	{
		Num = (int)Targets.size();
	}
	TargetsLock.Unlock();

	return Num;
}

/**
 * Exists for compatibility with UnrealConsole. Index is disregarded and CurrentTarget is disconnected if it contains a valid pointer.
 *
 * @param	Handle		Handle to the target to disconnect.
 */
void FNetworkManager::DisconnectTarget(const TARGETHANDLE Handle)
{
	TargetPtr Target = GetTarget(Handle);

	if(Target)
	{
		Target->Disconnect();
	}
}


