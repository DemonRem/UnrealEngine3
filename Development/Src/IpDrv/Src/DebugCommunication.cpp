/*=============================================================================
	DebugCommunication.cpp:
	Allowing network communication between UE3 executable and the
	"Unreal Remote" tool running on a PC.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnIpDrv.h"

//#define DEBUG_DEBUGCOMM 1

#if DEBUG_DEBUGCOMM
struct History
{
	INT		ReadIndex;
	INT		WriteIndex;
	INT		PacketSize;
	INT		NewIndex;
	FLOAT	Timestamp;
};
FLOAT GTimeStart = 0.0f;
#define MAXHISTORY 16000
History WriteHistory[MAXHISTORY];
History ReadHistory[MAXHISTORY];
INT CurrRead = 0;
INT CurrWrite = 0;
#endif

// make sure the packets we send will fit into any UDP packet
#define MAX_PACKET_SIZE 1200

/*----------------------------------------------------------------------------
	class FDebugReceiver
----------------------------------------------------------------------------*/

FDebugCommunication::FDebugReceiver::FDebugReceiver(INT InPort, INT InTrafficPort)
:	Socket(NULL)
,	Port(InPort)
,	TrafficPort(InTrafficPort)
,	TimeToDie(NULL)
{
}

UBOOL FDebugCommunication::FDebugReceiver::Init( )
{
	// Create the event used to tell our threads to bail
	TimeToDie = GSynchronizeFactory->CreateSynchEvent(TRUE);
	UBOOL bOk = TimeToDie != NULL;
	if ( bOk )
	{
		FInternetIpAddr ListenAddr;
		ListenAddr.SetPort( Port );
		ListenAddr.SetIp( getlocalbindaddr(*GWarn) );
		// Now create and set up our sockets (force UDP even when speciliazed protocols exist)
		Socket = GSocketSubsystem->CreateDGramSocket(TRUE);
		bOk = (Socket != NULL);
		if ( bOk )
		{
			Socket->SetReuseAddr();
			Socket->SetNonBlocking();
			Socket->SetRecvErr();
			// Bind to our listen port
			if (Socket->Bind(ListenAddr))
			{
				INT SizeSet;
				// Make the send buffer large so we don't overflow it
				bOk = Socket->SetSendBufferSize(0x20000,SizeSet);
			}
		}
	}
	return bOk;
}

DWORD FDebugCommunication::FDebugReceiver::Run( )
{
	BYTE PacketData[512];
	// Check every 1/2 second for a client request, while the death event
	// isn't signaled
	while (TimeToDie->Wait(500) == FALSE)
	{
		// Default to no data being read
		INT BytesRead = 0;
		if (Socket != NULL)
		{
			// Read from the socket and process if some was read
			FInternetIpAddr SockAddr;
			Socket->RecvFrom(PacketData,512,BytesRead,SockAddr);
			while ( BytesRead > 0 )
			{
				ProcessPacket(SockAddr,PacketData,BytesRead);
				Socket->RecvFrom(PacketData,512,BytesRead,SockAddr);
			}
		}
	}
	return 0;
}

void FDebugCommunication::FDebugReceiver::Stop( )
{
	TimeToDie->Trigger();
}

void FDebugCommunication::FDebugReceiver::Exit( )
{
	GSynchronizeFactory->Destroy(TimeToDie);
	TimeToDie = NULL;
	if (Socket != NULL)
	{
		GSocketSubsystem->DestroySocket(Socket);
		Socket = NULL;
	}
}

void FDebugCommunication::FDebugReceiver::ProcessPacket( FIpAddr SrcAddr, BYTE* Data, INT Count )
{
#if STATS
	// Check for a "server announce" request
	if (Count == 2 && Data[0] == 'S' && Data[1] == 'A')
	{
		debugf(NAME_DevStats,TEXT("Sending server announce to %s"),*SrcAddr.GetString(TRUE));

		// Build the packet containing the information the client needs
		// Format is SR, computer name, game name, game type, platform type
		FNboSerializeBuffer QueryResponsePacket;
		QueryResponsePacket << 'S' << 'R' << appComputerName() << GGameName << (BYTE)appGetStatGameType() << (BYTE)appGetPlatformType() << TrafficPort;

		// Send our info to the client (responding on admin port + 1)
		SrcAddr.Port = Port + 1;
		INT BytesSent;
		Socket->SendTo(QueryResponsePacket,QueryResponsePacket.GetByteCount(),BytesSent,SrcAddr.GetSocketAddress());
	}
	// Check for a client connect request
	else if (Count == 2 && Data[0] == 'C' && Data[1] == 'C')
	{
		debugf(NAME_DevStats,TEXT("Received client connect from %s"),*SrcAddr.GetString(TRUE));
		// Add this client
		SrcAddr.Port = TrafficPort;
		FIpAddr PrevClient = GDebugComm->OnAddClient(SrcAddr);

		// Notify previous client about disconnect (if it's not the same client).
		if ( PrevClient.Addr && PrevClient.Port && PrevClient.Addr != SrcAddr.Addr )
		{
			// Format: 'SD'
			INT BytesSent;
			FNboSerializeBuffer QueryResponsePacket;
			QueryResponsePacket << 'S' << 'D';
			Socket->SendTo(QueryResponsePacket,QueryResponsePacket.GetByteCount(),BytesSent,PrevClient.GetSocketAddress());
		}
	}
	// Check for a client disconnect
	else if (Count == 2 && Data[0] == 'C' && Data[1] == 'D')
	{
		debugf(NAME_DevStats,TEXT("Received client disconnect from %s"),*SrcAddr.GetString(TRUE));
		// Remove this client
		SrcAddr.Port = TrafficPort;
		GDebugComm->OnRemoveClient(SrcAddr);
	}
	// Check for client text being sent to us
	else if (Count >= 6 && Data[0] == 'C' && Data[1] == 'T' )
	{
		SrcAddr.Port = TrafficPort;
		FIpAddr CurrentClient = GDebugComm->GetClientAddr();
		if ( CurrentClient == SrcAddr )
		{
			//@TODO - Do correct platform byteswapping
			INT Len = (DWORD(Data[2]) << 24) | (DWORD(Data[3]) << 16) | (DWORD(Data[4]) << 8) | DWORD(Data[5]);
			GDebugComm->OnReceiveText( (ANSICHAR*)(Data+6), Len );
		}
	}
	else
	{
		// Log the unknown request type
		debugf(NAME_DevStats,TEXT("Unknown request of size %d from %s"),Count,*SrcAddr.GetString(TRUE));
	}
#endif
}


/*----------------------------------------------------------------------------
	class FDebugSender
----------------------------------------------------------------------------*/

FDebugCommunication::FDebugSender::FDebugSender( INT InPort, INT BufferSize )
:	RingBuffer(BufferSize)
,	Socket(NULL)
,	Port(InPort)
,	bIsTimeToDie(FALSE)
{
	AccessSync = GSynchronizeFactory->CreateCriticalSection();
}

FDebugCommunication::FDebugSender::~FDebugSender()
{
	GSynchronizeFactory->Destroy( AccessSync );
}

void FDebugCommunication::FDebugSender::KickBuffer()
{
	FScopeLock ScopeLock(AccessSync);
	RingBuffer.KickBuffer();
}

void FDebugCommunication::FDebugSender::SendChannelData( const ANSICHAR* Channel, const BYTE* Buffer, INT Size )
{
	//@TODO Proper byte-swapping when necessary!!! -Smedis

	FMultiThreadRingBuffer::BufferData Data;
	INT ChannelLen = strlen( Channel );
	UBOOL bSuccess;
	do 
	{
		// NOTE: We must keep the lock over the entire situation, no matter if we succeed or not.
		{
			FScopeLock ScopeLock(AccessSync);
			bSuccess = RingBuffer.BeginPush( Data, 10+ChannelLen+Size );
			if ( bSuccess )
			{
				// ST = "Server Transmission" being sent out.
				*Data.Buffer++ = 'S';
				*Data.Buffer++ = 'T';

				// Channel name in Network format.
				*((INT*&)Data.Buffer)++ = ChannelLen;
				appMemcpy( Data.Buffer, Channel, ChannelLen );
				Data.Buffer += ChannelLen;

				// Buffer size and buffer data.
				*((INT*&)Data.Buffer)++ = Size;
				appMemcpy( Data.Buffer, Buffer, Size );
				RingBuffer.EndPush( Data );
			}
		}

		// Sleep a little bit while waiting for more buffer space
		if ( !bSuccess )
		{
			appSleep( 0.001f );		// 1 millisecond
		}
	} while (!bSuccess);
}

UBOOL FDebugCommunication::FDebugSender::Init()
{
	// Create the socket used for sending (force UDP)
	FInternetIpAddr ListenAddr;
	ListenAddr.SetIp( getlocalbindaddr(*GWarn) );
	ListenAddr.SetPort( Port );
	Socket = GSocketSubsystem->CreateDGramSocket(TRUE);
	return (Socket != NULL);
}

void FDebugCommunication::FDebugSender::Stop()
{
	FScopeLock ScopeLock(AccessSync);
	appInterlockedIncrement((INT*)&bIsTimeToDie);
	RingBuffer.KickBuffer();
}

void FDebugCommunication::FDebugSender::Exit()
{
	FScopeLock ScopeLock(AccessSync);
	if (Socket != NULL)
	{
		GSocketSubsystem->DestroySocket(Socket);
		Socket = NULL;
	}
}

DWORD FDebugCommunication::FDebugSender::Run()
{
	while (bIsTimeToDie == FALSE)
	{
		// Wait at most 30 ms before trying to send again.
		RingBuffer.WaitForData( 30 );

		// Get current client
		FIpAddr IPAddress = GDebugComm->GetClientAddr();

		INT BytesSent;
		FMultiThreadRingBuffer::BufferData Data;
		while ( RingBuffer.Peek( Data ) )
		{
			if ( IPAddress.Addr && IPAddress.Port )
			{
				Socket->SendTo( Data.Buffer, Data.Size, BytesSent, IPAddress.GetSocketAddress() );
			}
			RingBuffer.Pop( Data );
		}
	}
	return 0;
}


/*----------------------------------------------------------------------------
	class FDebugCommunication
----------------------------------------------------------------------------*/

FDebugCommunication::FDebugCommunication( INT InBufferSize/*=DEFAULTBUFFERSIZE*/ )
:	ReceiverRunnable(NULL)
,	ReceiverThread(NULL)
,	SenderRunnable(NULL)
,	SenderThread(NULL)
,	ConnectionSync(NULL)
,	CommandSync(NULL)
,	NumCommands(0)
,	BufferSize(InBufferSize)
{
	ClientAddress.Addr = 0;
	ClientAddress.Port = 0;

#if DEBUG_DEBUGCOMM
	GTimeStart = appSeconds();
#endif
}

FDebugCommunication::~FDebugCommunication()
{
	Shutdown();
}

UBOOL FDebugCommunication::Startup()
{
	ConnectionSync = GSynchronizeFactory->CreateCriticalSection();
	CommandSync = GSynchronizeFactory->CreateCriticalSection();

	// Read the port that is to be used for listening for client broadcasts
	INT ListenPort = 13500;
	GConfig->GetInt(TEXT("DebugCommunication.UDP"),TEXT("ListenPort"),ListenPort,GEngineIni);

	// Read the port that is to be used for listening for client broadcasts
	INT TrafficPort = 13502;
	GConfig->GetInt(TEXT("DebugCommunication.UDP"),TEXT("TrafficPort"),TrafficPort,GEngineIni);

	if ( ListenPort+1 == TrafficPort )
	{
		TrafficPort++;
		debugf(TEXT("Port Collision: Changed DebugCommunication traffic port to %d"), TrafficPort);
	}

	ReceiverRunnable = new FDebugReceiver( ListenPort, TrafficPort );
	ReceiverThread = GThreadFactory->CreateThread(ReceiverRunnable,TEXT("DebugReceiver"),FALSE,FALSE,12*1024);
	SenderRunnable = new FDebugSender( TrafficPort, BufferSize );
	SenderThread = GThreadFactory->CreateThread(SenderRunnable,TEXT("DebugSender"),FALSE,FALSE,12*1024,TPri_AboveNormal);
	return TRUE;
}

void FDebugCommunication::Shutdown()
{
	// Tell the threads to stop running
	if (ReceiverRunnable != NULL)
	{
		ReceiverRunnable->Stop();
		ReceiverThread->WaitForCompletion();
		GThreadFactory->Destroy(ReceiverThread);
		delete ReceiverRunnable;
		ReceiverRunnable = NULL;
		ReceiverThread = NULL;
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
	if ( ConnectionSync )
	{
		GSynchronizeFactory->Destroy(ConnectionSync);
		ConnectionSync = NULL;
	}
	if ( CommandSync )
	{
		GSynchronizeFactory->Destroy(CommandSync);
		CommandSync = NULL;
	}

	ClientAddress.Addr = 0;
	ClientAddress.Port = 0;
}

void FDebugCommunication::SendText( const ANSICHAR* Text )
{
	SendChannelText( "LOG", Text );
}

void FDebugCommunication::SendChannelText( const ANSICHAR* Channel, const ANSICHAR* Text )
{
	if ( SenderRunnable && IsConnected() )
	{
		// NOTE: We send without null-termination. The receiver gets the length as a number.
		SenderRunnable->SendChannelData( Channel, (const BYTE*) Text, strlen(Text) );
	}
}

void FDebugCommunication::SendChannelData( const ANSICHAR* Channel, const BYTE* Data, INT Len )
{
	if ( SenderRunnable && IsConnected() )
	{
		SenderRunnable->SendChannelData( Channel, Data, Len );
	}
}

void FDebugCommunication::Tick( )
{
	FScopeLock ScopeLock(CommandSync);
	for ( INT Index=0; Index < NumCommands; ++Index )
	{
		new(GEngine->DeferredCommands) FString(Commands[Index]);
	}
	NumCommands = 0;	// Reset the command buffer

	if ( SenderRunnable )
	{
		SenderRunnable->KickBuffer();
	}
}

UBOOL FDebugCommunication::IsConnected( )
{
	FScopeLock ScopeLock(ConnectionSync);
	FIpAddr Addr = ClientAddress;
	return (Addr.Addr && Addr.Port);
}

FIpAddr	FDebugCommunication::GetClientAddr( )
{
	FScopeLock ScopeLock(ConnectionSync);
	FIpAddr Addr = ClientAddress;
	return Addr;
}

FIpAddr	FDebugCommunication::OnAddClient( FIpAddr InClientAddress )
{
	FScopeLock ScopeLock(ConnectionSync);
	FIpAddr PrevClient = ClientAddress;
	ClientAddress = InClientAddress;
	return PrevClient;
}

UBOOL FDebugCommunication::OnRemoveClient( FIpAddr InClientAddress )
{
	FScopeLock ScopeLock(ConnectionSync);
	UBOOL bWasRemoved = FALSE;
	if ( ClientAddress == InClientAddress )
	{
		ClientAddress.Addr = 0;
		ClientAddress.Port = 0;
		bWasRemoved = TRUE;
	}
	return bWasRemoved;
}

void FDebugCommunication::OnReceiveText( ANSICHAR *Message, INT Length )
{
	FScopeLock ScopeLock(CommandSync);
	if ( NumCommands < MAX_COMMANDS )
	{
		// Truncate if necessary
		if ( Length > (MAX_COMMANDLENGTH - 1) )
		{
			Length = MAX_COMMANDLENGTH - 1;
		}
		appStrncpy( Commands[NumCommands], ANSI_TO_TCHAR(Message), Length + 1 );
		Commands[NumCommands][ Length ] = 0;	
		NumCommands++;
	}
}


/*----------------------------------------------------------------------------
	class FMultiThreadRingBuffer
----------------------------------------------------------------------------*/

FMultiThreadRingBuffer::FMultiThreadRingBuffer( INT InBufferSize )
:	BufferSize( InBufferSize )
,	ReadIndex(0)
,	WriteIndex(0)
,	CurrentWriteIndex(4)	// Account for 4-byte Packet Marker
{
	RingBuffer = new BYTE[ InBufferSize + 4 ];	// Allow an extra Packet Marker at the end
	*((DWORD*)RingBuffer) = 0;
	WorkEvent = GSynchronizeFactory->CreateSynchEvent();
}

FMultiThreadRingBuffer::~FMultiThreadRingBuffer()
{
	GSynchronizeFactory->Destroy(WorkEvent);
	delete [] RingBuffer;
}

UBOOL FMultiThreadRingBuffer::BeginPush( BufferData &Data, INT Size )
{
	// if current size + new size is going to be bigger than a UDP packet, then kick what's there now
	if ((CurrentWriteIndex - WriteIndex) + Size > MAX_PACKET_SIZE)
	{
		KickBuffer();
	}

	// The Message must fit at the beginning or at the end of the buffer.
	check( Size < (CurrentWriteIndex-5) || Size < (BufferSize-CurrentWriteIndex) );

	INT SafeReadIndex = ReadIndex;			// Copy the current value (ReadIndex is updated by the work thread)
	INT WriteStart = CurrentWriteIndex;

	// Verify integrity of the ringbuffer
	check( (WriteIndex < SafeReadIndex && CurrentWriteIndex < SafeReadIndex) ||
		   (WriteIndex >= SafeReadIndex && (CurrentWriteIndex >= WriteIndex || CurrentWriteIndex < SafeReadIndex)) );

	// Are we starting a new packet?
	if ( CurrentWriteIndex == WriteIndex )
	{
		// Add 4 bytes for Packet Marker.
		WriteStart += 4;
	}

	// Are we looking for space until the end of the buffer?
	UBOOL bFindSpaceAtEnd = (SafeReadIndex <= CurrentWriteIndex);

	INT SizeAvailable = bFindSpaceAtEnd ? (BufferSize - WriteStart) : (SafeReadIndex - WriteStart - 1);

	// Not enough space?
	if ( Size > SizeAvailable )
	{
		// Are we looking for space until the end of the buffer?
		if ( bFindSpaceAtEnd )
		{
			// Case 1:  [____R....W____]
			// If so, there's not enough space between CurrentWriteIndex and the end of the buffer.
			// Check the beginning of the buffer instead.
			WriteStart = 4;									// Account for new Packet Marker in the first 4 bytes
			SizeAvailable = SafeReadIndex - WriteStart - 1;	// Don't pass the Reader!

			// Still not enough space?
			if ( Size > SizeAvailable )
			{
				// Wait for Reader to free up more space in the beginning of the buffer.
				KickBuffer();
				return FALSE;
			}
		}
		else
		{
			// Case 2:  [....W____R....]   ==>   [....W_______R.]
			// We are looking for space between WriteIndex and ReadIndex.
			// We can only wait for the Reader to free up more space.
			KickBuffer();
			return FALSE;
		}
	}

	Data.Buffer		= RingBuffer + WriteStart;
	Data.Size		= Size;
	Data.NewIndex	= WriteStart + Size;
	return TRUE;
}

void FMultiThreadRingBuffer::EndPush( BufferData &Data, UBOOL bForcePush/*=FALSE*/ )
{
	INT PacketSize	= Data.NewIndex - WriteIndex;
	check( CurrentWriteIndex >= WriteIndex );

	INT PacketMarkerIndex = WriteIndex;

	// Did we wrap around?
	if ( PacketSize < 0 )
	{
		// Calculate the packet size for the data at the end of the buffer.
		PacketSize = CurrentWriteIndex - WriteIndex;

		// Write Packet Marker (high bit tells the Reader it's the last Packet - wrap around).
		*((DWORD*) (RingBuffer + PacketMarkerIndex)) = DWORD(PacketSize) | 0x80000000;

		// Calculate the packet size for the data at the beginning of the buffer.
		PacketSize = Data.NewIndex;

		// Start a new Packet at the beginning of the buffer.
		PacketMarkerIndex = 0;

		// There should be data there, besides the Packet Marker.
		check(PacketSize > 4);

		// Force push.
		bForcePush = TRUE;
	}

	// If we don't have any data besides Packet Marker, don't do anything.
	if ( PacketSize <= 4 )
	{
		return;
	}

	// Enough data to push to the Reader?
	if ( PacketSize > 1024 || bForcePush )
	{
#if DEBUG_DEBUGCOMM
		CurrWrite = (CurrWrite+1)%MAXHISTORY;
		WriteHistory[CurrWrite].ReadIndex = ReadIndex;
		WriteHistory[CurrWrite].WriteIndex = WriteIndex;
		WriteHistory[CurrWrite].PacketSize = PacketSize;
		WriteHistory[CurrWrite].NewIndex = Data.NewIndex;
		WriteHistory[CurrWrite].Timestamp = (appSeconds()-GTimeStart) * 1000.0f;
#endif
		// Write Packet Marker (just the size in this case).
		*((DWORD*) (RingBuffer + PacketMarkerIndex)) = DWORD(PacketSize);

		// Pass the data to the Reader.
		appInterlockedExchange( &WriteIndex, Data.NewIndex );
	}

	// Set where we will write to next.
	CurrentWriteIndex = Data.NewIndex;
}

void FMultiThreadRingBuffer::KickBuffer( )
{
	// Do we have data to push?
	if ( CurrentWriteIndex != WriteIndex )
	{
		BufferData Data;
		Data.NewIndex = CurrentWriteIndex;
		EndPush( Data, TRUE );
	}

	WorkEvent->Trigger();
}

UBOOL FMultiThreadRingBuffer::Peek( BufferData &Entry  )
{
	INT SafeWriteIndex	= WriteIndex;	// Copy the current value (WriteIndex is updated by user thread)

	// Is there data to read?
	if ( ReadIndex != SafeWriteIndex )
	{
		DWORD PacketMarker = *((DWORD*) (RingBuffer + ReadIndex));
		INT PacketSize = INT(PacketMarker & 0x7fffffff);

		// DEBUG: check for reasonable sizes
		check(PacketSize >= 0 && PacketSize < 4096);

		Entry.Buffer	= RingBuffer + ReadIndex + 4;	// Skip the Packet Marker
		Entry.Size		= PacketSize - 4;
		Entry.NewIndex	= ReadIndex + PacketSize;

		// Should we wrap around?
		if ( PacketMarker & 0x80000000 )
		{
			Entry.NewIndex = 0;
		}

		// Empty packet?
		if ( Entry.Size <= 0 )
		{
#if DEBUG_DEBUGCOMM
			CurrRead = (CurrRead+1)%MAXHISTORY;
			ReadHistory[CurrRead].ReadIndex = ReadIndex;
			ReadHistory[CurrRead].WriteIndex = WriteIndex;
			ReadHistory[CurrRead].PacketSize = PacketSize;
			ReadHistory[CurrRead].NewIndex = Entry.NewIndex;
			ReadHistory[CurrRead].Timestamp = (appSeconds()-GTimeStart) * 1000.0f;
#endif

			// Just update the ReadIndex; don't return any data
			appInterlockedExchange( &ReadIndex, Entry.NewIndex );
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

void FMultiThreadRingBuffer::Pop( BufferData &Entry )
{
#if DEBUG_DEBUGCOMM
	CurrRead = (CurrRead+1)%MAXHISTORY;
	ReadHistory[CurrRead].ReadIndex = ReadIndex;
	ReadHistory[CurrRead].WriteIndex = WriteIndex;
	ReadHistory[CurrRead].PacketSize = *((DWORD*) (RingBuffer + ReadIndex));;
	ReadHistory[CurrRead].NewIndex = Entry.NewIndex;
	ReadHistory[CurrRead].Timestamp = (appSeconds()-GTimeStart) * 1000.0f;
#endif

	appInterlockedExchange( &ReadIndex, Entry.NewIndex );
}

void FMultiThreadRingBuffer::WaitForData( DWORD MilliSeconds /*= INFINITE*/ )
{
	WorkEvent->Wait( MilliSeconds );
}

