/*=============================================================================
	WindowsSupport/UDPClient.cpp: Simple UDP client.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "Common.h"
#include "UDPClient.h"

unsigned int CUDPClient::BroadcastIP = inet_addr("255.255.255.255");

CUDPClient::CUDPClient() :
	Socket(INVALID_SOCKET),
	IP(0),
	Port(0)
{
}

CUDPClient::~CUDPClient()
{
	DestroySocket();
}

void CUDPClient::SetIP(unsigned int InIP)
{
	IP = InIP;
}

unsigned int CUDPClient::GetIP() const
{
	return IP;
}

void CUDPClient::SetPort(int InPort)
{
	Port = InPort;		
}

bool CUDPClient::SetBroadcasting(bool Enable)
{
	if (Socket == INVALID_SOCKET) return false;

	if (Enable)
		IP = BroadcastIP;

	INT EnableBroadcast = Enable ? TRUE : FALSE;
	const int Result = setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (const char*) &EnableBroadcast, sizeof(EnableBroadcast));
	return Result == 0;
}

bool CUDPClient::SetNonBlocking(bool Enable)
{
	if (Socket == INVALID_SOCKET) return false;

	u_long EnableNonBlocking = Enable ? TRUE : FALSE;
	const int Result = ioctlsocket(Socket, FIONBIO, &EnableNonBlocking);
	return Result == 0;
}

bool CUDPClient::CreateSocket()
{
	DestroySocket();

	Socket = socket(AF_INET, SOCK_DGRAM, 0);
	return Socket != INVALID_SOCKET;
}

void CUDPClient::DestroySocket()
{
	if (Socket != INVALID_SOCKET)
	{
		closesocket(Socket);
		Socket = NULL;
	}
}

bool CUDPClient::Bind()
{
	if (Socket == INVALID_SOCKET) return false;

	sockaddr_in MyAddress;
	memset(&MyAddress, 0, sizeof(MyAddress));
	MyAddress.sin_family = AF_INET;
	MyAddress.sin_port = htons((u_short) Port);
	MyAddress.sin_addr.s_addr = INADDR_ANY;

	const int Result = bind(Socket, (struct sockaddr*) &MyAddress, sizeof(sockaddr_in));
	return Result == 0;
}

int CUDPClient::Receive(char* Buffer, int BufferSize, sockaddr_in& SenderAddress)
{
	if (Socket == INVALID_SOCKET) return 0;

	int AddressSize = sizeof(struct sockaddr);
	return recvfrom(Socket, Buffer, BufferSize, 0, (struct sockaddr*) &SenderAddress, &AddressSize);
}

int CUDPClient::Send(char* Buffer, int BytesCount)
{
	if (Socket == INVALID_SOCKET) return 0;

	sockaddr_in TargetAddress;
	memset(&TargetAddress, 0, sizeof(TargetAddress));
	TargetAddress.sin_family = AF_INET;
	TargetAddress.sin_port = htons((u_short) Port);
	TargetAddress.sin_addr.s_addr = IP;

	return sendto(
		Socket,
		Buffer,
		BytesCount,
		0,
		(struct sockaddr*) &TargetAddress,
		sizeof(TargetAddress));
}
