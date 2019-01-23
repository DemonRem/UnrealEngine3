/*=============================================================================
	WindowsSupport/UDPClient.h: Simple UDP client.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#pragma once

/**
 *	Simple UDP client.
 */
class CUDPClient
{
public:
	/// An IP used to broadcast messages
	static unsigned int BroadcastIP;

protected:
	SOCKET Socket; //!< Socket used
	unsigned int IP; //!< IP we're connecting to
	int Port; //!< Port we're connecting to

public:
	CUDPClient();
	~CUDPClient();

	void SetIP(unsigned int InIP);
	unsigned int GetIP() const;
	void SetPort(int InPort);

	bool CreateSocket();
	void DestroySocket();

	bool SetBroadcasting(bool Enable);
	bool SetNonBlocking(bool Enable);

	bool Bind();

	int Receive(char* Buffer, int BufferSize, sockaddr_in& SenderAddress);
	int Send(char* Buffer, int BytesCount);
};
