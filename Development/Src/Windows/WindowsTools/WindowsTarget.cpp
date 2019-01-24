/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "WindowsTarget.h"
#include "StringUtils.h"

///////////////////////CWindowsTarget/////////////////////////////////

CWindowsTarget::CWindowsTarget(const sockaddr_in& InRemoteAddress) 
: ReconnectTries(0)
, bHadSocketError(false)
, bConnected(false)
, TxtCallback(NULL)
{
	memcpy(&RemoteAddress, &InRemoteAddress, sizeof(RemoteAddress));
	ZeroMemory(PacketBuffer, sizeof(PacketBuffer));

	TCPClient.SetAttributes(SF_IPv4, ST_Stream, SP_TCP);
}

CWindowsTarget::~CWindowsTarget()
{
	Disconnect();
}

void CWindowsTarget::SendTTY(const char* Txt)
{
	if(TxtCallback)
	{
		wchar_t Buffer[PACKET_BUFFER_SIZE + 16] = L"";
		int Length = MultiByteToWideChar( CP_UTF8, 0, Txt, ( int )strlen( Txt ), Buffer, PACKET_BUFFER_SIZE );
		if (Length > 0)
		{
			Buffer[Length] = '\0';
			TxtCallback(Buffer);
		}
	}
}

bool CWindowsTarget::Connect()
{
	Disconnect();

	if (!TCPClient.CreateSocket())
	{
		return false;
	}

	TCPClient.SetAddress(RemoteAddress);
	if (!TCPClient.Connect())
	{
		return false;
	}
	TCPClient.SetNonBlocking(true);

	ReconnectTries = 0;
	bConnected = true;
	return true;
}

void CWindowsTarget::Reconnect()
{
	DWORD Ticks = GetTickCount();
	if( Ticks - LastConnectionTicks > RECONNECT_RETRY_TIME && ReconnectTries < 60 )
	{
		LastConnectionTicks = Ticks;
		ReconnectTries++;
		SendTTY("*** Reconnecting\n");
		Connect();
	}
}

void CWindowsTarget::Disconnect()
{
	bHadSocketError = false;
	bConnected = false;
	TCPClient.Close();
}

bool CWindowsTarget::SendConsoleCommand(const char* Message)
{
	int MessageLength = ( int )strlen( Message );
	if (MessageLength > 0 && bConnected && !bHadSocketError)
	{
		if (TCPClient.Send((char*)Message, MessageLength) == MessageLength)
		{
			return true;
		}
		bHadSocketError = true;
	}
	return false;
}

/** Periodic update */
void CWindowsTarget::Tick()
{
	if (!bConnected)
	{
		Reconnect();
		return;
	}

	// if we had a socket error, disconnect now
	if (bHadSocketError)
	{
		Disconnect();
		SendTTY("*** Socket Error While Sending\n");
		return;
	}

	// fill our buffer
	if (TCPClient.IsReadReady())
	{
		for (;;)
		{
			// read into buffer
			int BytesReceived = TCPClient.Recv(PacketBuffer, PACKET_BUFFER_SIZE);
			if (BytesReceived == 0)
			{
				// normal TCP close
				Disconnect();
				SendTTY("*** Socket Closed\n");
				return;
			}
			else if (BytesReceived < 0)
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// socket Error
					Disconnect();
					SendTTY("*** Socket Error While Receiving\n");
					return;
				}

				// no more data
				break;
			}

			// terminate the line
			PacketBuffer[BytesReceived] = '\0';
			SendTTY(PacketBuffer);
		}
	}
}
