/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "Socket.h"
#include <MSWSock.h>

FSocket::FSocket() : Socket(INVALID_SOCKET)
{
	ZeroMemory(&Address, sizeof(Address));
	ZeroMemory(&ProtocolInfo, sizeof(ProtocolInfo));
}

FSocket::FSocket(ESocketFamily SocketFamily, ESocketType SocketType, ESocketProtocol SocketProtocol) : Socket(INVALID_SOCKET)
{
	ZeroMemory(&Address, sizeof(Address));
	ZeroMemory(&ProtocolInfo, sizeof(ProtocolInfo));

	SetAttributes(SocketFamily, SocketType, SocketProtocol);
}

FSocket::~FSocket()
{
	Close();
}

/**
 * Wrapper for the winsock recv().
 *
 * @param	Buffer		Buffer to save the recv'd data into.
 * @param	BufLength	The size of Buffer.
 * @param	Flags		Flags controlling the operation.
 */
int FSocket::Recv(char* Buffer, int BufLength, ESocketFlags Flags)
{
	return recv(Socket, Buffer, BufLength, (int)Flags);
}

/**
 * Wrapper for the winsock recvfrom().
 *
 * @param	Buffer		Buffer to save the recv'd data into.
 * @param	BufLength	The size of Buffer.
 * @param	FromAddress	The address the data was recv'd from.
 * @param	Flags		Flags controlling the operation.
 */
int FSocket::RecvFrom(char* Buffer, int BufLength, sockaddr_in &FromAddress, ESocketFlags Flags)
{
	int FromLen = sizeof(sockaddr_in);
	return recvfrom(Socket, Buffer, BufLength, (int)Flags, (sockaddr*)&FromAddress, &FromLen);
}

/**
 * Begins an asynchronous recv() operation.
 *
 * @param	Buffers		Array of buffers for the incoming data.
 * @param	BufferCount	The number of buffers.
 * @param	BytesRecvd	Receives the number of bytes written to the buffers.
 * @param	EventArgs	Information about the event.
 * @param	Flags		Flags controlling the operation.
 * @return	True if the operation succeeded.
 */
bool FSocket::RecvAsync(LPWSABUF Buffers, DWORD BufferCount, DWORD &BytesRecvd, WSAOVERLAPPED *EventArgs, ESocketFlags Flags)
{
	bool ret = WSARecv(Socket, Buffers, BufferCount, &BytesRecvd, (LPDWORD)&Flags, EventArgs, NULL) != SOCKET_ERROR;
	
	if(!ret && WSAGetLastError() == WSA_IO_PENDING)
	{
		ret = true;
	}

	return ret;
}

/**
 * Wrapper for the winsock send().
 *
 * @param	Buffer		Buffer to save the recv'd data into.
 * @param	BufLength	The size of Buffer.
 * @param	Flags		Flags controlling the operation.
 * @return	SOCKET_ERROR if the operation failed, otherwise the number of bytes sent.
 */
int FSocket::Send(const char* Buffer, int BufLength, ESocketFlags Flags)
{
	return send(Socket, Buffer, BufLength, (int)Flags);
}

/**
 * Wrapper for the winsock sendto().
 *
 * @param	Buffer		Buffer to save the recv'd data into.
 * @param	BufLength	The size of Buffer.
 * @param	Flags		Flags controlling the operation.
 * @return	SOCKET_ERROR if the operation failed, otherwise the number of bytes sent.
 */
int FSocket::SendTo(const char* Buffer, int BufLength, ESocketFlags Flags)
{
	return sendto(Socket, Buffer, BufLength, (int)Flags, (sockaddr*)&Address, sizeof(Address));
}

/**
 * Creates a new internal socket.
 *
 * @return	True if the operation succeeds.
 */
bool FSocket::CreateSocket()
{
	Close();

	return (Socket = WSASocket(ProtocolInfo.iAddressFamily, ProtocolInfo.iSocketType, ProtocolInfo.iProtocol, NULL, NULL, WSA_FLAG_OVERLAPPED)) != INVALID_SOCKET;
}

/**
 * Sets attributes used when creating a new socket via CreateSocket().
 *
 * @param	SocketFamily	The family of the socket.
 * @param	SocketType		The type of socket to be created.
 * @param	SocketProtocol	The protocol of the socket.
 */
void FSocket::SetAttributes(ESocketFamily SocketFamily, ESocketType SocketType, ESocketProtocol SocketProtocol)
{
	ProtocolInfo.iAddressFamily = (int)SocketFamily;
	ProtocolInfo.iSocketType = (int)SocketType;
	ProtocolInfo.iProtocol = (int)SocketProtocol;
	Address.sin_family = (short)SocketFamily;
}

/**
 * Binds the socket to the current address.
 *
 * @return	True if the operation succeeds.
 */
bool FSocket::Bind()
{
	return bind(Socket, (sockaddr*)&Address, sizeof(Address)) != SOCKET_ERROR;
}

/**
 * Wraps the winsock connect() by connecting to the current address.
 *
 * @return	True if the operation succeeds.
 */
bool FSocket::Connect()
{
	int Error = connect(Socket, (sockaddr*)&Address, sizeof(Address));
	return Error == 0;
}

/**
 * Sets the blocking state of the socket.
 *
 * @param	IsNonBlocking	True if the socket is set to non-blocking.
 * @return	True if the operation succeeds.
 */
bool FSocket::SetNonBlocking(bool IsNonBlocking)
{
	u_long arg = IsNonBlocking ? 1 : 0;
	return ioctlsocket(Socket, FIONBIO, &arg) == 0;
}

/**
 * Sets the broadcasting state of the socket.
 *
 * @param	bEnable	True if the socket is set to broadcast.
 * @return	True if the operation succeeds.
 */
bool FSocket::SetBroadcasting(bool bEnable)
{
	INT bEnableBroadcast = bEnable ? TRUE : FALSE;
	return setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, (const char*)&bEnableBroadcast, sizeof(bEnableBroadcast)) == 0;
}

/**
 * Sets whether a socket can be bound to an address in use
 *
 * @param bAllowReuse whether to allow reuse or not
 *
 * @return TRUE if the call succeeded, FALSE otherwise
 */
bool FSocket::SetReuseAddr(bool bEnable)
{
	INT bAllowReuse = bEnable ? TRUE : FALSE;
	return setsockopt(Socket,SOL_SOCKET,SO_REUSEADDR,(char*)&bAllowReuse,sizeof(bAllowReuse)) == 0;
}

/**
 * Closes the socket.
 */
void FSocket::Close()
{
	if(IsValid())
	{
		closesocket(Socket);
		Socket = INVALID_SOCKET;
	}
}

/**
 * @return TRUE if the other end of socket has gone away
 */
bool FSocket::IsBroken()
{
	// Check and return without waiting
	TIMEVAL Time = {0,0};
	fd_set SocketSet;
	// Set up the socket sets we are interested in (just this one)
	FD_ZERO(&SocketSet);
	FD_SET(Socket,&SocketSet);
	// Check the status of the bits for errors
	INT SelectStatus = select(0,NULL,NULL,&SocketSet,&Time);
	if (SelectStatus == 0)
	{
		FD_ZERO(&SocketSet);
		FD_SET(Socket,&SocketSet);
		// Now check to see if it's connected (writable means connected)
		INT WriteSelectStatus = select(0,NULL,&SocketSet,NULL,&Time);
		INT ReadSelectStatus = 1;//select(0,&SocketSet,NULL,NULL,&Time);
		if (WriteSelectStatus > 0 && ReadSelectStatus > 0)
		{
			return FALSE;
		}
	}
	// any other cases are busted socket
	return TRUE;
}

/**
 * @return TRUE if there's an error or read ready to go
 */
bool FSocket::IsReadReady()
{
	// Check and return without waiting
	TIMEVAL Time = {0,0};
	fd_set ReadSet;
	FD_ZERO(&ReadSet);
	FD_SET(Socket,&ReadSet);
	fd_set ErrorSet;
	FD_ZERO(&ErrorSet);
	FD_SET(Socket,&ErrorSet);

	// Check the status of the bits for errors
	INT SelectStatus = select(0,&ReadSet,NULL,&ErrorSet,&Time);
	if (SelectStatus == 0)
	{
		// nothing to see here, move along
		return FALSE;
	}

	// either an error or a read (or select failed)
	// in any case, do a recv and see if we get an error
	return TRUE;
}
