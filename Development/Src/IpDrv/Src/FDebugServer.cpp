/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This file contains the implementation of the debug server.
 */

#include "UnIpDrv.h"
#include "FDebugServer.h"
#include "DebugServerDefs.h"

/*------------------------------ FDebugServer::FListenerRunnable ----------------------------------*/

UBOOL FDebugServer::FListenerRunnable::OutputDeviceAdded = FALSE;

/**
 * Copies the specified values to use for the listener thread
 *
 * @param InPort the port to listen for client requests on
 * @param InTrafficPort the port that will send on
 * @param InClients the list that clients will be added to/removed from
 */
FDebugServer::FListenerRunnable::FListenerRunnable(
	INT InPort,
	INT InTrafficPort,
	FClientList* InClients,
	FDebugServer* InDebugServer) :
	ListenPort(InPort),
	TrafficPort(InTrafficPort),
	TimeToDie(NULL),
	ClientList(InClients),
	ListenSocket(NULL),
	DebugServer(InDebugServer)
{
}

/**
 * Binds to the specified port
 *
 * @return True if initialization was successful, false otherwise
 */
UBOOL FDebugServer::FListenerRunnable::Init()
{
	// Create the event used to tell our threads to bail
	TimeToDie = GSynchronizeFactory->CreateSynchEvent(TRUE);
	UBOOL bOk = TimeToDie != NULL;
	if (bOk)
	{
		ListenAddr.SetPort(ListenPort);
		ListenAddr.SetIp(getlocalbindaddr(*GWarn));
		// Now create and set up our sockets (force UDP even when speciliazed
		// protocols exist)
		ListenSocket = GSocketSubsystem->CreateDGramSocket(TRUE);
		if (ListenSocket != NULL)
		{
			ListenSocket->SetReuseAddr();
			ListenSocket->SetNonBlocking();
			ListenSocket->SetRecvErr();
			// Bind to our listen port
			if (ListenSocket->Bind(ListenAddr))
			{
				INT SizeSet;
				// Make the send buffer large so we don't overflow it
				bOk = ListenSocket->SetSendBufferSize(0x20000,SizeSet);
			}
		}
	}
	return bOk;
}

/**
 * Sends an announcement message on start up and periodically thereafter
 *
 * @return The exit code of the runnable object
 */
DWORD FDebugServer::FListenerRunnable::Run()
{
	BYTE PacketData[512];
	// Check every 1/2 second for a client request, while the death event
	// isn't signaled
	while (TimeToDie->Wait(500) == FALSE)
	{
		// Default to no data being read
		INT BytesRead = 0;
		if (ListenSocket != NULL)
		{
			FInternetIpAddr SockAddr;
			// Read from the socket and process if some was read
			ListenSocket->RecvFrom(PacketData,512,BytesRead,SockAddr);
			if (BytesRead > 0)
			{
				// Process this data
				ProcessPacket(SockAddr,PacketData,BytesRead);
			}
		}
	}
	return 0;
}

/**
 * Receives data from a given call to Poll(). For the beacon, we
 * just send an announcement directly to the source address
 *
 * @param SrcAddr the source address of the request
 * @param Data the packet data
 * @param Count the number of bytes in the packet
 */
void FDebugServer::FListenerRunnable::ProcessPacket(FIpAddr SrcAddr,
	BYTE* Data,INT Count)
{
	// Check for a "server announce" request
	if (Count == 2 && Data[0] == 'S' && Data[1] == 'A')
	{
		debugfSuppressed(NAME_DevStats,TEXT("Sending server announce to %s"),
			*SrcAddr.GetString(TRUE));
		// Build the packet containing the information the client needs
		FNboSerializeToBuffer QueryResponsePacket(DEFAULT_MAX_UDP_PACKET_SIZE);
		// Format is SR, computer name, game name, game type, platform type
		QueryResponsePacket << 'S' << 'R' << appComputerName() << GGameName <<
			(BYTE)appGetGameType() << (BYTE)appGetPlatformType() <<
			TrafficPort;
// @todo joeg - add a "requires password" option
		// Respond on the next port from what we are listening to
		SrcAddr.Port = ListenPort + 1;
		INT BytesSent;
		// Send our info to the client
		ListenSocket->SendTo(QueryResponsePacket,QueryResponsePacket.GetByteCount(),
			BytesSent,SrcAddr.GetSocketAddress());
	}

	// Check for a client connect request
	else if (Count == 2 && Data[0] == 'C' && Data[1] == 'C')
	{
		debugfSuppressed(NAME_DevStats,TEXT("Received client connect from %s"),
			*SrcAddr.GetString(TRUE));
		
		// Make sure debug output was added to log
		if (!OutputDeviceAdded)
		{
			// See if output device was added - it depends on whether the below conditions were met
			// (as specified in UnMisc.cpp, line 3715 as of 05-06-2007)
			OutputDeviceAdded = CONSOLE || appIsDebuggerPresent();

			// If output device wasn't added, do it now
			if (!OutputDeviceAdded)
			{
				GLog->AddOutputDevice( new FOutputDeviceDebug() );
				OutputDeviceAdded = TRUE;
			}
		}

		// Add this address to our update list
		if (ClientList->AddClient(SrcAddr))
			// Notify listener
			OnClientConnected(SrcAddr);
	}

	// Check for a client disconnect
	else if (Count == 2 && Data[0] == 'C' && Data[1] == 'D')
	{
		debugfSuppressed(NAME_DevStats,TEXT("Received client disconnect from %s"),
			*SrcAddr.GetString(TRUE));
		// Remove this address from our update list
		ClientList->RemoveClient(SrcAddr);
		// Notify listener
		OnClientDisconnected(SrcAddr);
	}

	// Check for client text being sent to us
	else if (Count >= 6 && Data[0] == 'C' && Data[1] == 'T' )
	{
		//@TODO - Do correct platform byteswapping
		INT Len = (DWORD(Data[2]) << 24) | (DWORD(Data[3]) << 16) | (DWORD(Data[4]) << 8) | DWORD(Data[5]);
		DebugServer->OnReceiveText(SrcAddr, (ANSICHAR*)(Data+6), Len );
	}

	else
	{
		// Log the unknown request type
		debugfSuppressed(NAME_DevStats,TEXT("Unknown request of size %d from %s"),Count,
			*SrcAddr.GetString(TRUE));
	}
}

void FDebugServer::OnReceiveText(const FIpAddr& SrcAddr, const ANSICHAR* Characters, INT Length)
{
	FScopeLock ScopeLock(CommandSync);
	if ( NumCommands < MAX_COMMANDS )
	{
		// Truncate if necessary
		if ( Length > (MAX_COMMANDLENGTH - 1) )
		{
			Length = MAX_COMMANDLENGTH - 1;
		}
		appStrncpy( Commands[NumCommands], ANSI_TO_TCHAR(Characters), Length + 1 );
		Commands[NumCommands][ Length ] = 0;	
		NumCommands++;
	}
}

/*------------------------------ FDebugServer::FSenderRunnable ----------------------------------*/

/**
 * Copies the specified values to use for the send thread
 *
 * @param InSync the critical section to use
 * @param InClients the list that clients will be added to/removed from
 */
FDebugServer::FSenderRunnable::FSenderRunnable(
	FCriticalSection* InSync,FClientList* InClients,FMultiThreadedRingBuffer* InLogBuffer) :
	bIsTimeToDie(FALSE),
	WorkEvent(NULL),
	AccessSync(InSync),
	ClientList(InClients),
	SenderSocket(NULL),
	LogBuffer(InLogBuffer)
{
}

/**
 * Binds to the specified port
 *
 * @return True if initialization was successful, false otherwise
 */
UBOOL FDebugServer::FSenderRunnable::Init(void)
{
	UBOOL bSuccess = FALSE;
	// Create the event used to tell our threads to bail
	WorkEvent = GSynchronizeFactory->CreateSynchEvent();
	// Create the socket used for sending (force UDP)
	SenderSocket = GSocketSubsystem->CreateDGramSocket(TRUE);
	if (SenderSocket != NULL)
	{
		// Set it to broadcast mode
		bSuccess = SenderSocket->SetBroadcast();
	}

	return WorkEvent != NULL && bSuccess;
}

/**
 * Sends an announcement message on start up and periodically thereafter
 *
 * @return The exit code of the runnable object
 */
DWORD FDebugServer::FSenderRunnable::Run(void)
{
	do
	{
		// Wait for there to be work to do
		WorkEvent->Wait();

		// Do the server specific job
		Tick();
	}
	while (bIsTimeToDie == FALSE);
	return 0;
}

void FDebugServer::FSenderRunnable::Tick()
{
	// See if there's new log to be sent
	FMultiThreadedRingBuffer::BufferData Data;
	while (LogBuffer->Peek(Data))
	{
		// Send log to all clients
		FClientConnection* Client = ClientList->BeginIterator();
		while (Client)
		{
			INT BytesSent = 0;
			SenderSocket->SendTo( Data.Buffer, Data.Size, BytesSent, Client->Address.GetSocketAddress() );
			
			Client = ClientList->GetNext();
		}
		ClientList->EndIterator();

		// Remove log data from buffer
		LogBuffer->Pop(Data);
	}
}

/**
 * Cleans up any allocated resources
 */
void FDebugServer::FSenderRunnable::Exit(void)
{
	GSynchronizeFactory->Destroy(WorkEvent);
	WorkEvent = NULL;
	GSynchronizeFactory->Destroy(AccessSync);
	AccessSync = NULL;
	// Clean up socket
	if (SenderSocket != NULL)
	{
		GSocketSubsystem->DestroySocket(SenderSocket);
		SenderSocket = NULL;
	}
}

void FDebugServer::FSenderRunnable::WakeUp()
{
	WorkEvent->Trigger();
}

/*------------------------------ FDebugServer ----------------------------------*/

FDebugServer::FDebugServer() :
	ListenerThread(NULL),
	ListenerRunnable(NULL),
	ClientList(NULL),
	SenderThread(NULL),
	SenderRunnable(NULL),
	LogBuffer(NULL),
	NumCommands(0),
	CommandSync(NULL)
{
}

/**
 * Initializes the threads that handle the network layer
 */
UBOOL FDebugServer::Init(INT ListenPort, INT TrafficPort, INT BufferSize, INT InMaxUDPPacketSize)
{
	// Create cyclic buffer for messages that are to be sent
	MaxUDPPacketSize = InMaxUDPPacketSize;
	LogBuffer = new FMultiThreadedRingBuffer(BufferSize, MaxUDPPacketSize);

	// Create command synchronizer
	CommandSync = GSynchronizeFactory->CreateCriticalSection();

	// Validate that the listen / response(equal to listen + 1) / traffic ports don't collide
	if (ListenPort + 1 == TrafficPort)
	{
		TrafficPort++;
		debugf(TEXT("FDebugServer::Init(): Port Collision: Changed traffic port to %d"),TrafficPort);
	}

	// Create the object that will manage our client connections
	ClientList = ::new FClientList(TrafficPort);

	// Create the listener object
	ListenerRunnable = CreateListenerRunnable(ListenPort, TrafficPort, ClientList);
	// Now create the thread that will do the work
	ListenerThread = GThreadFactory->CreateThread(ListenerRunnable,TEXT("DebugServerListener"),FALSE,FALSE,
		8 * 1024);
	if (ListenerThread != NULL)
	{
#if XBOX
		// See UnXenon.h
		//ListenerThread->SetProcessorAffinity(STATS_LISTENER_HWTHREAD);
#endif
		// Created externally so that it can be locked before the sender thread has
		// gone through its initialization
		FCriticalSection* AccessSync = GSynchronizeFactory->CreateCriticalSection();
		if (AccessSync != NULL)
		{
			// Now create the sender thread
			SenderRunnable = CreateSenderRunnable(AccessSync,ClientList);
			// Now create the thread that will do the work
			SenderThread = GThreadFactory->CreateThread(SenderRunnable,TEXT("DebugServerSender"),FALSE,FALSE,
				12 * 1024);
			if (SenderThread != NULL)
			{
#if XBOX
				// See UnXenon.h
				SenderThread->SetProcessorAffinity(STATS_SENDER_HWTHREAD);
#endif
			}
			else
			{
				debugf(NAME_Error,TEXT("Failed to create FDebugServer send thread"));
			}
		}
		else
		{
			debugf(NAME_Error,TEXT("Failed to create FDebugServer send thread"));
		}
	}
	else
	{
		debugf(NAME_Error,TEXT("Failed to create FDebugServer listener thread"));
	}
	return ListenerThread != NULL && SenderThread != NULL;
}

/**
 * Shuts down the network threads
 */
void FDebugServer::Destroy(void)
{
	// Tell the threads to stop running
	if (ListenerRunnable != NULL)
	{
		ListenerRunnable->Stop();
		ListenerThread->WaitForCompletion();
		GThreadFactory->Destroy(ListenerThread);
		delete ListenerRunnable;
		ListenerRunnable = NULL;
		ListenerThread = NULL;
	}
	if (SenderRunnable != NULL)
	{
		SenderRunnable->Stop();
		SenderThread->WaitForCompletion();
		GThreadFactory->Destroy(SenderThread);
		delete SenderRunnable;
		SenderRunnable = NULL;
		SenderThread = NULL;
	}
	// Destroy synch objects
	if (CommandSync != NULL)
	{
		GSynchronizeFactory->Destroy(CommandSync);
		CommandSync = NULL;
	}
	// Delete the clients class
	delete ClientList;
}

void FDebugServer::Tick()
{
	// Execute all received commands
	FScopeLock ScopeLock(CommandSync);
	for ( INT Index=0; Index < NumCommands; ++Index )
	{
		new(GEngine->DeferredCommands) FString(Commands[Index]);
	}
	NumCommands = 0;

	// Wake up sender
	if (SenderRunnable)
		SenderRunnable->WakeUp();
}

FDebugServer::FSenderRunnable* FDebugServer::CreateSenderRunnable(FCriticalSection* InSync, FClientList* InClients)
{
	return new FDebugServer::FSenderRunnable(InSync, InClients, LogBuffer);
}

FDebugServer::FListenerRunnable* FDebugServer::CreateListenerRunnable(INT InPort, INT InTrafficPort, FClientList* InClients)
{
	return new FDebugServer::FListenerRunnable(InPort, InTrafficPort, InClients, this);
}

void FDebugServer::SendText(const ANSICHAR* Channel, const ANSICHAR* Text)
{
	// Do not send text if there's no client connected
	if (!ClientList || ClientList->GetConnectionCount() == 0) return;

	// Handle case when packet size is greater than max allowed UDP packet size
	const INT PacketHeaderSize = 2 + sizeof(ANSICHAR) * 2 + sizeof(INT) + strlen(Channel) + sizeof(INT);
	const INT TextLength = strlen(Text);
	const INT PacketSize = PacketHeaderSize + TextLength;

	const INT TextPieceLength = MaxUDPPacketSize - PacketHeaderSize;

	INT NumMessages = TextLength / TextPieceLength;
	if (NumMessages * TextPieceLength < TextLength)
		NumMessages++;

	for (INT i = 0; i < NumMessages; i++)
	{
		const INT CurrentPieceLength = (i < NumMessages - 1) ?
			TextPieceLength :
			(TextLength - (NumMessages - 1) * TextPieceLength);
		SendText(Channel, Text + i * TextPieceLength, CurrentPieceLength);
	}
}

void FDebugServer::SendText(const ANSICHAR* Channel, const ANSICHAR* Text, const INT Length)
{
	// Create packet
	FNboSerializeToBuffer Packet(MaxUDPPacketSize);
	const ANSICHAR* MessageHeader = ToString(EDebugServerMessageType_ServerTransmission);
	Packet << MessageHeader[0] << MessageHeader[1] << Channel;
	Packet.AddString(Text, Length);

	// Attempt to obtain space in logs ring buffer for our packet
	INT AttemptsCounter = 10;
	FMultiThreadedRingBuffer::BufferData Data;
	while (!LogBuffer->BeginPush(Data, Packet.GetByteCount()))
	{
		AttemptsCounter--;
		if (AttemptsCounter == 0)
			return; // Drop the message

		appSleep(0.001f);
	}

	// Copy the data to ring buffer
	memcpy(Data.Buffer, Packet.GetRawBuffer(0), Packet.GetByteCount());

	// Finalize operation
	LogBuffer->EndPush(Data);

	// Let's awake the sender thread
	if (SenderRunnable)
		SenderRunnable->WakeUp();
}
