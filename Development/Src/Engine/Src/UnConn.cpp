/*=============================================================================
	UnConn.h: Unreal connection base class.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"

/*-----------------------------------------------------------------------------
	UNetConnection implementation.
-----------------------------------------------------------------------------*/

UNetConnection::UNetConnection()
:	Driver				( NULL )
,	State				( USOCK_Invalid )
,	PackageMap			( NULL )

,	ProtocolVersion		( MIN_PROTOCOL_VERSION )
,	MaxPacket			( 0 )
,	PacketOverhead		( 0 )
,	InternalAck			( FALSE )
,	Challenge			( 0 )
,	NegotiatedVer		( GEngineNegotiationVersion )
,	UserFlags			( 0 )

,	QueuedBytes			( 0 )
,	TickCount			( 0 )

,	AllowMerge			( FALSE )
,	TimeSensitive		( FALSE )
,	LastOutBunch		( NULL )

,	StatPeriod			( 1.f  )
,	BestLag				( 9999 )
,	AvgLag				( 9999 )

,	LagAcc				( 9999 )
,	BestLagAcc			( 9999 )
,	LagCount			( 0 )
,	LastTime			( 0 )
,	FrameTime			( 0 )
,	CumulativeTime		( 0 )
,	AverageFrameTime	( 0 )
,	CountedFrames		( 0 )

,	Out					( 0 )
,	InPacketId			( -1 )
,	OutPacketId			( 0 ) // must be initialized as OutAckPacketId + 1 so loss of first packet can be detected
,	OutAckPacketId		( -1 )
{
}

/**
 * Static constructor called once per class during static initialization via IMPLEMENT_CLASS
 * macro. Used to e.g. emit object reference tokens for realtime garbage collection or expose
 * properties for native- only classes.
 */
void UNetConnection::StaticConstructor()
{
	UClass* TheClass = GetClass();
	TheClass->EmitObjectReference( STRUCT_OFFSET( UNetConnection, PackageMap ) );
	TheClass->EmitFixedArrayBegin( STRUCT_OFFSET( UNetConnection, Channels ), sizeof(UChannel*), MAX_CHANNELS );
	TheClass->EmitObjectReference( STRUCT_OFFSET( UNetConnection, Channels ) );
	TheClass->EmitFixedArrayEnd();
	TheClass->EmitObjectReference( STRUCT_OFFSET( UNetConnection, Download ) );
	TheClass->EmitObjectArrayReference( STRUCT_OFFSET( UNetConnection, Children ) );
}

/**
 * Static constructor called once per class during static initialization via IMPLEMENT_CLASS
 * macro. Used to e.g. emit object reference tokens for realtime garbage collection or expose
 * properties for native- only classes.
 */
void UChildConnection::StaticConstructor()
{
	GetClass()->EmitObjectReference(STRUCT_OFFSET(UChildConnection, Parent));
}

/**
 * Initializes a connection with the passed in settings
 *
 * @param InDriver the net driver associated with this connection
 * @param InSocket the socket associated with this connection
 * @param InRemoteAddr the remote address for this connection
 * @param InState the connection state to start with for this connection
 * @param InOpenedLocally whether the connection was a client/server
 * @param InURL the URL to init with
 * @param InMaxPacket the max packet size that will be used for sending
 * @param InPacketOverhead the packet overhead for this connection type
 */
void UNetConnection::InitConnection(UNetDriver* InDriver,class FSocket* InSocket,const class FInternetIpAddr& InRemoteAddr,EConnectionState InState,UBOOL InOpenedLocally,const FURL& InURL,INT InMaxPacket,INT InPacketOverhead)
{
	// Use the passed in values
	MaxPacket = InMaxPacket;
	PacketOverhead = InPacketOverhead;
	check(MaxPacket && PacketOverhead);

#if DO_ENABLE_NET_TEST
	// Copy the command line settings from the net driver
	UpdatePacketSimulationSettings();
#endif

	// Other parameters.
	CurrentNetSpeed = URL.HasOption(TEXT("LAN")) ? GetDefault<UPlayer>()->ConfiguredLanSpeed : GetDefault<UPlayer>()->ConfiguredInternetSpeed;

	if ( CurrentNetSpeed == 0 )
	{
		CurrentNetSpeed = 2600;
	}
	else
	{
		CurrentNetSpeed = ::Max<INT>(CurrentNetSpeed, 1800);
	}

	// Create package map.
	if( GUseSeekFreePackageMap )
	{
		PackageMap = new(this)UPackageMapSeekFree(this);
	}
	else
	{
		PackageMap = new(this)UPackageMapLevel(this);
	}
}

/**
 * Intializes a "addressless" connection with the passed in settings
 *
 * @param InDriver the net driver associated with this connection
 * @param InState the connection state to start with for this connection
 * @param InURL the URL to init with
 * @param InConnectionSpeed Optional connection speed override
 */
void UNetConnection::InitConnection(UNetDriver* InDriver, EConnectionState InState, const FURL& InURL, INT InConnectionSpeed)
{
	Driver = InDriver;
	// We won't be sending any packets, so use a default size
	MaxPacket = 512;
	PacketOverhead = 0;
	State = InState;

#if DO_ENABLE_NET_TEST
	// Copy the command line settings from the net driver
	UpdatePacketSimulationSettings();
#endif

	// Get the 
	if (InConnectionSpeed)
	{
		CurrentNetSpeed = InConnectionSpeed;
	}
	else
	{

		CurrentNetSpeed =  URL.HasOption(TEXT("LAN")) ? GetDefault<UPlayer>()->ConfiguredLanSpeed : GetDefault<UPlayer>()->ConfiguredInternetSpeed;
		if ( CurrentNetSpeed == 0 )
		{
			CurrentNetSpeed = 2600;
		}
		else
		{
			CurrentNetSpeed = ::Max<INT>(CurrentNetSpeed, 1800);
		}
	}

	// Create package map.
	if( GUseSeekFreePackageMap )
	{
		PackageMap = new(this)UPackageMapSeekFree(this);
	}
	else
	{
		PackageMap = new(this)UPackageMapLevel(this);
	}
}

void UNetConnection::Serialize( FArchive& Ar )
{
	UObject::Serialize( Ar );
	Ar << PackageMap;
	for( INT i=0; i<MAX_CHANNELS; i++ )
		Ar << Channels[i];
	Ar << Download;

}

/** closes the connection (including sending a close notify across the network) */
void UNetConnection::Close()
{
	if (Driver != NULL)
	{
//@todo joeg/matto -- Wrap this whole function in this if?
		if (State != USOCK_Closed)
		{
			debugf(NAME_NetComeGo, TEXT("Close %s %s %s"), *GetName(), *LowLevelGetRemoteAddress(), appTimestamp());
		}
		if (Channels[0] != NULL)
		{
			Channels[0]->Close();
		}
		State = USOCK_Closed;
		FlushNet();
	}
}

/** closes the control channel, cleans up structures, and prepares for deletion */
void UNetConnection::CleanUp()
{
	// Remove UChildConnection(s)
	for (INT i = 0; i < Children.Num(); i++)
	{
		Children(i)->CleanUp();
	}
	Children.Empty();

	Close();

	if (Driver != NULL)
	{
		// Remove from driver.
		if (Driver->ServerConnection)
		{
			check(Driver->ServerConnection == this);
			Driver->ServerConnection = NULL;
		}
		else
		{
			check(Driver->ServerConnection == NULL);
			verify(Driver->ClientConnections.RemoveItem(this) == 1);
		}
	}

	// Kill all channels.
	for (INT i = OpenChannels.Num() - 1; i >= 0; i--)
	{
		UChannel* OpenChannel = OpenChannels(i);
		if (OpenChannel != NULL)
		{
			OpenChannel->ConditionalCleanUp();
		}
	}

	PackageMap = NULL;

	// Kill download object.
	if (Download != NULL)
	{
		Download->CleanUp();
	}

	// Destroy the connection's actor.
	if (GIsRunning && Actor != NULL)
	{
		Actor->Player = NULL;
		if (GWorld != NULL)
		{
			GWorld->DestroyActor(Actor, 1);
		}
		Actor = NULL;
	}

	Driver = NULL;
}

void UChildConnection::CleanUp()
{
	// Destroy the connection's actor.
	if (GIsRunning && Actor != NULL)
	{
		Actor->Player = NULL;
		if (GWorld != NULL)
		{
			GWorld->DestroyActor(Actor, 1);
		}
		Actor = NULL;
	}
	PackageMap = NULL;
	Driver = NULL;
}

void UNetConnection::FinishDestroy()
{
	if ( !HasAnyFlags(RF_ClassDefaultObject) )
	{
		CleanUp();
	}

	Super::FinishDestroy();
}

UBOOL UNetConnection::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	return 0;
}
void UNetConnection::AssertValid()
{
	// Make sure this connection is in a reasonable state.
	check(ProtocolVersion>=MIN_PROTOCOL_VERSION);
	check(ProtocolVersion<=MAX_PROTOCOL_VERSION);
	check(State==USOCK_Closed || State==USOCK_Pending || State==USOCK_Open);

}
void UNetConnection::SendPackageMap()
{
	// Send package map to the remote.
	for( TArray<FPackageInfo>::TIterator It(PackageMap->List); It; ++It )
	{
		SendPackageInfo(*It);
	}

	for( INT i=0; i<Driver->DownloadManagers.Num(); i++ )
	{
		UClass* DownloadClass = StaticLoadClass( UDownload::StaticClass(), NULL, *Driver->DownloadManagers(i), NULL, LOAD_NoWarn, NULL );
		if( DownloadClass )
		{
			UObject* DefaultObject = DownloadClass->GetDefaultObject();
			check(DefaultObject);

			FString Params = *CastChecked<UDownload>(DefaultObject)->DownloadParams;
			UBOOL Compression = CastChecked<UDownload>(DefaultObject)->UseCompression;
			if( *(*Params) )
			{
				Logf
				( 
					TEXT("DLMGR CLASS=%s PARAMS=%s COMPRESSION=%d"), 
					*DownloadClass->GetPathName(),
					*Params,
					Compression
				);
			}
		}
	}
}

/** parses the passed in string and fills the given package info struct with that data
 * @param Text pointer to the string
 * @param Info (out) FPackageInfo that receives the parsed data
 */
void UNetConnection::ParsePackageInfo(const TCHAR* Text, FPackageInfo& Info)
{
	Parse(Text, TEXT("GUID=" ), Info.Guid);
	Parse(Text, TEXT("GEN=" ), Info.RemoteGeneration);
	Parse(Text, TEXT("FLAGS="), Info.PackageFlags);
	Parse(Text, TEXT("PKG="), Info.PackageName);
	Parse(Text, TEXT("EXT="), Info.Extension);
	Parse(Text, TEXT("BASEPKG="), Info.ForcedExportBasePackageName);
}

/** sends text describing the given package info to the client via Logf() */
void UNetConnection::SendPackageInfo(const FPackageInfo& Info)
{
	Logf
	(
		TEXT("USES GUID=%s PKG=%s EXT=%s FLAGS=%i GEN=%i BASEPKG=%s"),
		*Info.Guid.String(),
		*Info.PackageName.ToString(),
		*Info.Extension,
		Info.PackageFlags,
		Info.LocalGeneration,
		(Info.Parent != NULL) ? *Info.Parent->GetForcedExportBasePackageName().ToString() : TEXT("None")
	);
}

/** adds a package to this connection's PackageMap and synchronizes it with the client
 * @param Package the package to add
 */
void UNetConnection::AddNetPackage(UPackage* Package)
{
	if (!GWorld->IsServer())
	{
		debugf(NAME_Error, TEXT("UNetConnection::AddNetPackage() called on client"));
		appErrorfSlow(TEXT("UNetConnection::AddNetPackage() called on client"));
	}
	// if we haven't been welcomed, do nothing as that process will cause the current package map to be sent
	else if (PackageMap != NULL && bWelcomed)
	{
		// we're no longer removing this package, so make sure it's not in the list
		PendingRemovePackageGUIDs.RemoveItem(Package->GetGuid());

		INT Index = PackageMap->AddPackage(Package);
		SendPackageInfo(PackageMap->List(Index));
	}
}

/** removes a package from this connection's PackageMap and synchronizes it with the client
 * @param Package the package to remove
 */
void UNetConnection::RemoveNetPackage(UPackage* Package)
{
	if (!GWorld->IsServer())
	{
		debugf(NAME_Error, TEXT("UNetConnection::RemoveNetPackage() called on client"));
		appErrorfSlow(TEXT("UNetConnection::RemoveNetPackage() called on client"));
	}
	else if (PackageMap != NULL)
	{
		// we DO NOT delete the entry in client PackageMaps so that indices are not reshuffled during play as that can potentially result in mismatches
		if (!PackageMap->RemovePackageOnlyIfSynced(Package))
		{
			// if the client hasn't acknowledged this package yet, we need to wait for it to do so
			// otherwise we get mismatches if the ack is already in transit at the time we send this
			// as we'll receive the ack but without the package reference be unable to figure out how many indices we need to reserve
			//@todo: package acks need a timeout/failsafe so we're not waiting forever and leaving UPackage objects around unnecessarily
			PendingRemovePackageGUIDs.AddItem(Package->GetGuid());
		}
		Logf(TEXT("UNLOAD GUID=%s"), *Package->GetGuid().String());
	}
}

/** returns whether the client has initialized the level required for the given object
 * @return TRUE if the client has initialized the level the object is in or the object is not in a level, FALSE otherwise
 */
UBOOL UNetConnection::ClientHasInitializedLevelFor(UObject* TestObject)
{
	checkSlow(GWorld->IsServer());

	// get the level for the object
	ULevel* Level = NULL;
	for (UObject* Obj = TestObject; Obj != NULL; Obj = Obj->GetOuter())
	{
		Level = Cast<ULevel>(Obj);
		if (Level != NULL)
		{
			break;
		}
	}

	return (Level == NULL || Level == GWorld->PersistentLevel || ClientVisibleLevelNames.ContainsItem(Level->GetOutermost()->GetFName()));
}

/**
 * Resets the FBitWriter to its default state
 */
void UNetConnection::InitOut()
{
	// Initialize the one outgoing buffer.
	if (MaxPacket * 8 == Out.GetMaxBits())
	{
		// Reset all of our values to their initial state without a malloc/free
		Out.Reset();
	}
	else
	{
		// First time initialization needs to allocate the buffer
		Out = FBitWriter(MaxPacket * 8);
	}
}

void UNetConnection::ReceivedRawPacket( void* InData, INT Count )
{
	BYTE* Data = (BYTE*)InData;

	// Handle an incoming raw packet from the driver.
	debugfSlow( NAME_DevNetTraffic, TEXT("%03i: Received %i"), appTrunc(appSeconds()*1000)%1000, Count );
	Driver->InBytes += Count + PacketOverhead;
	Driver->InPackets++;
	if( Count>0 )
	{
		BYTE LastByte = Data[Count-1];
		if( LastByte )
		{
			INT BitSize = Count*8-1;
			while( !(LastByte & 0x80) )
			{
				LastByte *= 2;
				BitSize--;
			}
			FBitReader Reader( Data, BitSize );
			ReceivedPacket( Reader );
		}
		else appErrorfSlow( TEXT("Packet missing trailing 1") );
	}
	else appErrorfSlow( TEXT("Received zero-size packet") );

}
void UNetConnection::FlushNet()
{
	// Update info.
	check(!Out.IsError());
	LastEnd = FBitWriterMark();
	TimeSensitive = 0;

	// If there is any pending data to send, send it.
	if( Out.GetNumBits() || Driver->Time-LastSendTime>Driver->KeepAliveTime )
	{
		// If sending keepalive packet, still generate header.
		if( Out.GetNumBits()==0 )
			PreSend( 0 );

		// Make sure packet size is byte-aligned.
		Out.WriteBit( 1 );
		check(!Out.IsError());
		while( Out.GetNumBits() & 7 )
			Out.WriteBit( 0 );
		check(!Out.IsError());

		// Send now.
#if DO_ENABLE_NET_TEST
		if( PacketSimulationSettings.PktOrder )
		{
			DelayedPacket& B = *(new(Delayed)DelayedPacket);
			B.Data.Add( Out.GetNumBytes() );
			appMemcpy( &B.Data(0), Out.GetData(), Out.GetNumBytes() );

			for( INT i=Delayed.Num()-1; i>=0; i-- )
			{
				if( appFrand()>0.50 )
				{
					if( !PacketSimulationSettings.PktLoss || appFrand()*100.f > PacketSimulationSettings.PktLoss )
					{
						LowLevelSend( (char*)&Delayed(i).Data(0), Delayed(i).Data.Num() );
					}
					Delayed.Remove( i );
				}
			}
		}
		else if( PacketSimulationSettings.PktLag )
		{
			if( !PacketSimulationSettings.PktLoss || appFrand()*100.f > PacketSimulationSettings.PktLoss )
			{
				DelayedPacket& B = *(new(Delayed)DelayedPacket);
				B.Data.Add( Out.GetNumBytes() );
				appMemcpy( &B.Data(0), Out.GetData(), Out.GetNumBytes() );
				B.SendTime = appSeconds() + (DOUBLE(PacketSimulationSettings.PktLag)  + 2.0f * (appFrand() - 0.5f) * DOUBLE(PacketSimulationSettings.PktLagVariance))/ 1000.f;
			}
		}
		else if( !PacketSimulationSettings.PktLoss || appFrand()*100.f >= PacketSimulationSettings.PktLoss )
		{
#endif
			LowLevelSend( Out.GetData(), Out.GetNumBytes() );
#if DO_ENABLE_NET_TEST
			if( PacketSimulationSettings.PktDup && appFrand()*100.f < PacketSimulationSettings.PktDup )
			{
				LowLevelSend( (char*)Out.GetData(), Out.GetNumBytes() );
			}
		}
#endif

		// Update stuff.
		INT Index = OutPacketId & (ARRAY_COUNT(OutLagPacketId)-1);
		OutLagPacketId [Index] = OutPacketId;
		OutLagTime     [Index] = Driver->Time;
		OutPacketId++;
		Driver->OutPackets++;
		LastSendTime = Driver->Time;
		QueuedBytes += Out.GetNumBytes() + PacketOverhead;
		Driver->OutBytes += Out.GetNumBytes() + PacketOverhead;
		InitOut();
	}

	// Move acks around.
	for( INT i=0; i<QueuedAcks.Num(); i++ )
		ResendAcks.AddItem(QueuedAcks(i));
	QueuedAcks.Empty(32);

}
void UNetConnection::Serialize( const TCHAR* Data, EName MsgType )
{
	// Send data to the control channel.
	if( Channels[0] && !Channels[0]->Closing )
		((UControlChannel*)Channels[0])->Serialize( Data, MsgType );

}
INT UNetConnection::IsNetReady( UBOOL Saturate )
{
	// Return whether we can send more data without saturation the connection.
	if( Saturate )
		QueuedBytes = -Out.GetNumBytes();
	return QueuedBytes+Out.GetNumBytes() <= 0;

}
void UNetConnection::ReadInput( FLOAT DeltaSeconds )
{}
IMPLEMENT_CLASS(UNetConnection);
IMPLEMENT_CLASS(UChildConnection);

/*-----------------------------------------------------------------------------
	Packet reception.
-----------------------------------------------------------------------------*/

//
// Packet was negatively acknowledged.
//
void UNetConnection::ReceivedNak( INT NakPacketId )
{
	// Make note of the nak.
	for( INT i=OpenChannels.Num()-1; i>=0; i-- )
	{
		UChannel* Channel = OpenChannels(i);
		Channel->ReceivedNak( NakPacketId );
		if( Channel->OpenPacketId==NakPacketId )
			Channel->ReceivedAcks(); //warning: May destroy Channel.
	}
}

//
// Handle a packet we just received.
//
void UNetConnection::ReceivedPacket( FBitReader& Reader )
{
	AssertValid();

	// Handle PacketId.
	if( Reader.IsError() )
	{
		appErrorfSlow( TEXT("Packet too small") );
		return;
	}

	// Update receive time to avoid timeout.
	LastReceiveTime = Driver->Time;

	// Check packet ordering.
	INT PacketId = MakeRelative(Reader.ReadInt(MAX_PACKETID),InPacketId,MAX_PACKETID);
	if( PacketId > InPacketId )
	{
		Driver->InPacketsLost += PacketId - InPacketId - 1;
		InPacketId = PacketId;
	}
	else
	{
		Driver->InOutOfOrderPackets++;
	}
	//debugf(TEXT("RcvdPacket: %i %i"),appTrunc(appSeconds()*1000)%1000,PacketId);


	// Disassemble and dispatch all bunches in the packet.
	while( !Reader.AtEnd() && State!=USOCK_Closed )
	{
		// Parse the bunch.
		INT StartPos = Reader.GetPosBits();
		UBOOL IsAck = Reader.ReadBit();
		if( Reader.IsError() )
		{
			appErrorfSlow( TEXT("Bunch missing ack flag") );
			// Acknowledge the packet.
			SendAck( PacketId );
			return;
		}

		// Process the bunch.
		if( IsAck )
		{
			// This is an acknowledgement.
			INT AckPacketId = MakeRelative(Reader.ReadInt(MAX_PACKETID),OutAckPacketId,MAX_PACKETID);
			if( Reader.IsError() )
			{
				appErrorfSlow( TEXT("Bunch missing ack") );
				// Acknowledge the packet.
				SendAck( PacketId );
				return;
			}

			// Resend any old reliable packets that the receiver hasn't acknowledged.
			if( AckPacketId>OutAckPacketId )
			{
				for( INT NakPacketId=OutAckPacketId+1; NakPacketId<AckPacketId; NakPacketId++,Driver->OutPacketsLost++ )
				{
					debugfSlow( NAME_DevNetTraffic, TEXT("   Received virtual nak %i (%.1f)"), NakPacketId, (Reader.GetPosBits()-StartPos)/8.f );
					ReceivedNak( NakPacketId );
				}
				OutAckPacketId = AckPacketId;
			}
			else if( AckPacketId<OutAckPacketId )
			{
				//warning: Double-ack logic makes this unmeasurable.
				//OutOrdAcc++;
			}
			// Update lag.
			INT Index = AckPacketId & (ARRAY_COUNT(OutLagPacketId)-1);
			if( OutLagPacketId[Index]==AckPacketId )
			{
				FLOAT NewLag = Driver->Time - OutLagTime[Index] - (FrameTime/2.f);
				LagAcc += NewLag;
					LagCount++;
			}

			// Forward the ack to the channel.
			debugfSlow( NAME_DevNetTraffic, TEXT("   Received ack %i (%.1f)"), AckPacketId, (Reader.GetPosBits()-StartPos)/8.f );
			for( INT i=OpenChannels.Num()-1; i>=0; i-- )
			{
				UChannel* Channel = OpenChannels(i);
				for( FOutBunch* Out=Channel->OutRec; Out; Out=Out->Next )
				{
					if( Out->PacketId==AckPacketId )
					{
						Out->ReceivedAck = 1;
						if( Out->bOpen )
							Channel->OpenAcked = 1;
					}
				}
				if( Channel->OpenPacketId==AckPacketId ) // Necessary for unreliable "bNetTemporary" channels.
					Channel->OpenAcked = 1;
				Channel->ReceivedAcks(); //warning: May destroy Channel.
			}
		}
		else
		{
			// Parse the incoming data.
			FInBunch Bunch( this );
			INT IncomingStartPos = Reader.GetPosBits();
			BYTE bControl      = Reader.ReadBit();
			Bunch.PacketId     = PacketId;
			Bunch.bOpen        = bControl ? Reader.ReadBit() : 0;
			Bunch.bClose       = bControl ? Reader.ReadBit() : 0;
			Bunch.bReliable    = Reader.ReadBit();
			Bunch.ChIndex      = Reader.ReadInt( MAX_CHANNELS );
			Bunch.ChSequence   = Bunch.bReliable ? MakeRelative(Reader.ReadInt(MAX_CHSEQUENCE),InReliable[Bunch.ChIndex],MAX_CHSEQUENCE) : 0;
			Bunch.ChType       = (Bunch.bReliable||Bunch.bOpen) ? Reader.ReadInt(CHTYPE_MAX) : CHTYPE_None;
			INT BunchDataBits  = Reader.ReadInt( UNetConnection::MaxPacket*8 );
			INT HeaderPos      = Reader.GetPosBits();
			if( Reader.IsError() )
			{
				appErrorfSlow( TEXT("Bunch header overflowed") );
				// Acknowledge the packet.
				SendAck( PacketId );
				return;
			}
			Bunch.SetData( Reader, BunchDataBits );
			if( Reader.IsError() )
			{
				// Bunch claims it's larger than the enclosing packet.
				appErrorfSlow( TEXT("Bunch data overflowed (%i %i+%i/%i)"), IncomingStartPos, HeaderPos, BunchDataBits, Reader.GetNumBits() );
				// Acknowledge the packet.
				SendAck( PacketId );
				return;
			}
			if( Bunch.bReliable )
				debugfSlow( NAME_DevNetTraffic, TEXT("   Reliable Bunch, Channel %i Sequence %i: Size %.1f+%.1f"), Bunch.ChIndex, Bunch.ChSequence, (HeaderPos-IncomingStartPos)/8.f, (Reader.GetPosBits()-HeaderPos)/8.f );
			else
				debugfSlow( NAME_DevNetTraffic, TEXT("   Unreliable Bunch, Channel %i: Size %.1f+%.1f"), Bunch.ChIndex, (HeaderPos-IncomingStartPos)/8.f, (Reader.GetPosBits()-HeaderPos)/8.f );
			// Can't handle other channels until control channel exists.
			if( !Channels[Bunch.ChIndex] && !Channels[0] && (Bunch.ChIndex!=0 || Bunch.ChType!=CHTYPE_Control) )
			{
				appErrorfSlow( NAME_DevNetTraffic, TEXT("Received bunch before connected") );
				// Acknowledge the packet.
				SendAck( PacketId );
				return;
			}

			// Receiving data.
			UChannel* Channel = Channels[Bunch.ChIndex];

			// Ignore if reliable packet has already been processed.
			if( Bunch.bReliable && Bunch.ChSequence<=InReliable[Bunch.ChIndex] )
			{
				debugfSlow( NAME_DevNetTraffic, TEXT("      Received outdated bunch (Current Sequence %i)"), InReliable[Bunch.ChIndex] );
				continue;
			}

			// If unreliable but not one-shot open+close "bNetTemporary" packet, discard it.
			if( !Bunch.bReliable && (!Bunch.bOpen || !Bunch.bClose) && (!Channel || Channel->OpenPacketId==INDEX_NONE) )
			{
				debugfSlow( NAME_DevNetTraffic, TEXT("      Received unreliable bunch before open (Current Sequence %i)"), InReliable[Bunch.ChIndex] );
				continue;
			}

			// Create channel if necessary.
			if( !Channel )
			{
				// Validate channel type.
				if( !UChannel::IsKnownChannelType(Bunch.ChType) )
				{
					// Unknown type.
					appErrorfSlow( TEXT("Connection unknown channel type (%i)"), Bunch.ChType );
					// Acknowledge the packet.
					SendAck( PacketId );
					return;
				}

				// Reliable (either open or later), so create new channel.
				debugfSlow( NAME_DevNetTraffic, TEXT("      Bunch Create %i: ChType %i"), Bunch.ChIndex, Bunch.ChType );
				Channel = CreateChannel( (EChannelType)Bunch.ChType, FALSE, Bunch.ChIndex );

				// Notify the server of the new channel.
				if( !Driver->Notify->NotifyAcceptingChannel( Channel ) )
				{
					// Channel refused, so close it, flush it, and delete it.
					FOutBunch CloseBunch( Channel, 1 );
					check(!CloseBunch.IsError());
					check(CloseBunch.bClose);
					CloseBunch.bReliable = 1;
					Channel->SendBunch( &CloseBunch, 0 );
					FlushNet();
					Channel->ConditionalCleanUp();
					if( Bunch.ChIndex==0 )
					{
						debugfSlow( NAME_DevNetTraffic, TEXT("Channel 0 create failed") );
						State = USOCK_Closed;
					}
					continue;
				}
			}
			if( Bunch.bOpen )
			{
				Channel->OpenAcked = 1;
				Channel->OpenPacketId = PacketId;
			}

			// Dispatch the raw, unsequenced bunch to the channel.
			Channel->ReceivedRawBunch( Bunch ); //warning: May destroy channel.
			Driver->InBunches++;

			// Disconnect if we received a corrupted packet from the client (eg server crash attempt).
			if( !Driver->ServerConnection && Bunch.IsCriticalError() )
			{
				debugfSuppressed( NAME_DevNetTraffic, TEXT("Received corrupted packet data from client %s.  Disconnecting."), *LowLevelGetRemoteAddress() );
				State = USOCK_Closed;
			}
		}
	}

	// Acknowledge the packet.
	SendAck( PacketId );
}

/*-----------------------------------------------------------------------------
	All raw sending functions.
-----------------------------------------------------------------------------*/

//
// Called before sending anything.
//
void UNetConnection::PreSend( INT SizeBits )
{
	// Flush if not enough space.
	if( Out.GetNumBits() + SizeBits + MAX_PACKET_TRAILER_BITS > MaxPacket*8 )
		FlushNet();

	// If start of packet, send packet id.
	if( Out.GetNumBits()==0 )
	{
		Out.WriteInt( OutPacketId, MAX_PACKETID );
		check(Out.GetNumBits()<=MAX_PACKET_HEADER_BITS);
	}

	// Make sure there's enough space now.
	if( Out.GetNumBits() + SizeBits + MAX_PACKET_TRAILER_BITS > MaxPacket*8 )
		appErrorf( TEXT("PreSend overflowed: %i+%i>%i"), Out.GetNumBits(), SizeBits, MaxPacket*8 );

}

//
// Called after sending anything.
//
void UNetConnection::PostSend()
{
	// If absolutely filled now, flush so that MaxSend() doesn't return zero unnecessarily.
	check(Out.GetNumBits()<=MaxPacket*8);
	if( Out.GetNumBits()==MaxPacket*8 )
		FlushNet();

}

//
// Resend any pending acks.
//
void UNetConnection::PurgeAcks()
{
	for( INT i=0; i<ResendAcks.Num(); i++ )
		SendAck( ResendAcks(i), 0 );
	ResendAcks.Empty(32);
}

//
// Send an acknowledgement.
//
void UNetConnection::SendAck( INT AckPacketId, UBOOL FirstTime )
{
	if( !InternalAck )
	{
		if( FirstTime )
		{
			PurgeAcks();
			QueuedAcks.AddItem(AckPacketId);
		}
		PreSend( appCeilLogTwo(MAX_PACKETID)+1 );
		Out.WriteBit( 1 );
		Out.WriteInt( AckPacketId, MAX_PACKETID );
		AllowMerge = FALSE;
		PostSend();
	}
}

//
// Send a raw bunch.
//
INT UNetConnection::SendRawBunch( FOutBunch& Bunch, UBOOL InAllowMerge )
{
	check(!Bunch.ReceivedAck);
	check(!Bunch.IsError());
	Driver->OutBunches++;
	TimeSensitive = 1;

	// Build header.
	FBitWriter Header( MAX_BUNCH_HEADER_BITS );
	Header.WriteBit( 0 );
	Header.WriteBit( Bunch.bOpen || Bunch.bClose );
	if( Bunch.bOpen || Bunch.bClose )
	{
		Header.WriteBit( Bunch.bOpen );
		Header.WriteBit( Bunch.bClose );
	}
	Header.WriteBit( Bunch.bReliable );
	Header.WriteInt( Bunch.ChIndex, MAX_CHANNELS );
	if( Bunch.bReliable )
		Header.WriteInt( Bunch.ChSequence, MAX_CHSEQUENCE );
	if( Bunch.bReliable || Bunch.bOpen )
		Header.WriteInt( Bunch.ChType, CHTYPE_MAX );
	Header.WriteInt( Bunch.GetNumBits(), UNetConnection::MaxPacket*8 );
	check(!Header.IsError());

	// If this data doesn't fit in the current packet, flush it.
	PreSend( Header.GetNumBits() + Bunch.GetNumBits() );

	// Remember start position.
	AllowMerge      = InAllowMerge;
	Bunch.PacketId  = OutPacketId;
	Bunch.Time      = Driver->Time;

	// Remember start position, and write data.
	LastStart = FBitWriterMark( Out );
	Out.SerializeBits( Header.GetData(), Header.GetNumBits() );
	Out.SerializeBits( Bunch .GetData(), Bunch .GetNumBits() );

	// Finished.
	PostSend();

	return Bunch.PacketId;
}

/*-----------------------------------------------------------------------------
	Channel creation.
-----------------------------------------------------------------------------*/

//
// Create a channel.
//
UChannel* UNetConnection::CreateChannel( EChannelType ChType, UBOOL bOpenedLocally, INT ChIndex )
{
	check(UChannel::IsKnownChannelType(ChType));
	AssertValid();

	// If no channel index was specified, find the first available.
	if( ChIndex==INDEX_NONE )
	{
		INT FirstChannel = 1;
		if ( ChType == CHTYPE_Control )
			FirstChannel = 0;
		for( ChIndex=FirstChannel; ChIndex<MAX_CHANNELS; ChIndex++ )
			if( !Channels[ChIndex] )
				break;
		if( ChIndex==MAX_CHANNELS )
			return NULL;
	}

	// Make sure channel is valid.
	check(ChIndex<MAX_CHANNELS);
	check(Channels[ChIndex]==NULL);

	// Create channel.
	UChannel* Channel = ConstructObject<UChannel>( UChannel::ChannelClasses[ChType] );
	Channel->Init( this, ChIndex, bOpenedLocally );
	Channels[ChIndex] = Channel;
	OpenChannels.AddItem(Channel);
	//debugf( "Created channel %i of type %i", ChIndex, ChType);

	return Channel;
}

/*-----------------------------------------------------------------------------
	Connection polling.
-----------------------------------------------------------------------------*/

//
// Poll the connection.
// If it is timed out, close it.
//
void UNetConnection::Tick()
{
	AssertValid();

	// Lag simulation.
#if DO_ENABLE_NET_TEST
	if( PacketSimulationSettings.PktLag )
	{
		for( INT i=0; i < Delayed.Num(); i++ )
		{
			if( appSeconds() > Delayed(i).SendTime )
			{
				LowLevelSend( (char*)&Delayed(i).Data(0), Delayed(i).Data.Num() );
				Delayed.Remove( i );
				i--;
			}
		}
	}
#endif

	// Get frame time.
	DOUBLE CurrentTime = appSeconds();
	FrameTime = CurrentTime - LastTime;
	LastTime = CurrentTime;
	CumulativeTime += FrameTime;
	CountedFrames++;
	if(CumulativeTime > 1.f)
	{
		AverageFrameTime = CumulativeTime / CountedFrames;
		CumulativeTime = 0;
		CountedFrames = 0;
	}

	// Pretend everything was acked, for 100% reliable connections or demo recording.
	if( InternalAck )
	{
		LastReceiveTime = Driver->Time;
		for( INT i=OpenChannels.Num()-1; i>=0; i-- )
		{
			UChannel* It = OpenChannels(i);
			for( FOutBunch* Out=It->OutRec; Out; Out=Out->Next )
				Out->ReceivedAck = 1;
			It->OpenAcked = 1;
			It->ReceivedAcks();
		}
	}

	// Update stats.
	if( Driver->Time - StatUpdateTime > StatPeriod )
	{
		// Update stats.
		FLOAT RealTime	= Driver->Time - StatUpdateTime;
		if( LagCount )
			AvgLag = LagAcc/LagCount;
		BestLag = AvgLag;

		if( Actor )
		{
			FLOAT PktLoss = ::Max(Driver->InPacketsLost, Driver->OutPacketsLost) * 0.01f;
			FLOAT ModifiedLag = BestLag + 1.2 * PktLoss;
			if ( Actor->myHUD )
				Actor->myHUD->bShowBadConnectionAlert = !InternalAck && ((ModifiedLag>0.8 || CurrentNetSpeed * (1 - PktLoss)<2000) && ActorChannels.FindRef(Actor) || Driver->InPackets < 2); 
			if ( Actor->PlayerReplicationInfo )
				Actor->PlayerReplicationInfo->PacketLoss = appTrunc(::Max(Driver->InPacketsLost, Driver->OutPacketsLost));
		}

		// Init counters.
		LagAcc			= 0;
		StatUpdateTime  = Driver->Time;
		BestLagAcc		= 9999;
		LagCount        = 0;
	}

	// Compute time passed since last update.
	FLOAT DeltaTime     = Driver->Time - LastTickTime;
	LastTickTime        = Driver->Time;

	// Handle timeouts.
	FLOAT Timeout = Driver->InitialConnectTimeout;
	if ( (State!=USOCK_Pending) && Actor && (Actor->bPendingDestroy || Actor->bShortConnectTimeOut) )
		Timeout = Actor->bPendingDestroy ? 2.f : Driver->ConnectionTimeout;
	if( Driver->Time - LastReceiveTime > Timeout )
	{
		// Timeout.
		if( State != USOCK_Closed )
			debugf( NAME_DevNet, TEXT("Connection timed out after %f seconds (%f)"), Timeout, Driver->Time - LastReceiveTime );
		Close();
	}
	else
	{
		// Tick the channels.
		for( INT i=OpenChannels.Num()-1; i>=0; i-- )
			OpenChannels(i)->Tick();

		// If channel 0 has closed, mark the conection as closed.
		if( Channels[0]==NULL && (OutReliable[0]!=0 || InReliable[0]!=0) )
			State = USOCK_Closed;
	}

	// Flush.
	PurgeAcks();
	if( TimeSensitive || Driver->Time-LastSendTime>Driver->KeepAliveTime )
		FlushNet();

	if( Download )
		Download->Tick();

	// Update queued byte count.
	// this should be at the end so that the cap is applied *after* sending (and adjusting QueuedBytes for) any remaining data for this tick
	FLOAT DeltaBytes = CurrentNetSpeed * DeltaTime;
	QueuedBytes -= appTrunc(DeltaBytes);
	FLOAT AllowedLag = 2.f * DeltaBytes;
	if (QueuedBytes < -AllowedLag)
	{
		QueuedBytes = appTrunc(-AllowedLag);
	}
}

/*---------------------------------------------------------------------------------------
	Client Player Connection.
---------------------------------------------------------------------------------------*/

void UNetConnection::HandleClientPlayer( APlayerController *PC )
{
	// Hook up the Viewport to the new player actor.
	ULocalPlayer*	LocalPlayer = NULL;
	for(FPlayerIterator It(Cast<UGameEngine>(GEngine));It;++It)
	{
		LocalPlayer = *It;
		break;
	}

	// Detach old player.
	check(LocalPlayer);
	if( LocalPlayer->Actor )
	{
		LocalPlayer->Actor->eventServerDestroy(); //@note: will be executed locally if this was the placeholder PC while waiting for connection to be established
		LocalPlayer->Actor->Player = NULL;
		LocalPlayer->Actor = NULL;
	}

	LocalPlayer->CurrentNetSpeed = CurrentNetSpeed;

	// Init the new playerpawn.
	PC->Role = ROLE_AutonomousProxy;
	PC->SetPlayer(LocalPlayer);
	debugf(TEXT("%s setplayer %s"),*PC->GetName(),*LocalPlayer->GetName());
	State = USOCK_Open;
	Actor = PC;

	// if we have splitscreen viewports, ask the server to join them as well
	UBOOL bSkippedFirst = FALSE;
	for (FPlayerIterator It(Cast<UGameEngine>(GEngine)); It; ++It)
	{
		if (*It != LocalPlayer)
		{
			// send server command for new child connection
			It->SendSplitJoin();
		}
	}
}

void UChildConnection::HandleClientPlayer(APlayerController* PC)
{
	// find the first player that doesn't already have a connection
	ULocalPlayer* NewPlayer = NULL;
	BYTE CurrentIndex = 0;
	for (FPlayerIterator It(Cast<UGameEngine>(GEngine)); It; ++It, CurrentIndex++)
	{
		if (CurrentIndex == PC->NetPlayerIndex)
		{
			NewPlayer = *It;
			break;
		}
	}

	if (NewPlayer == NULL)
	{
#if !FINAL_RELEASE
		debugf(NAME_Error, TEXT("Failed to find LocalPlayer for received PlayerController '%s' with index %d. PlayerControllers:"), *PC->GetName(), INT(PC->NetPlayerIndex));
		for (FDynamicActorIterator It; It; ++It)
		{
			if (It->GetAPlayerController() && It->Role < ROLE_Authority)
			{
				debugf(TEXT(" - %s"), *It->GetFullName());
			}
		}
		appErrorf(TEXT("Failed to find LocalPlayer for received PlayerController"));
#else
		return; // avoid crash
#endif
	}

	// Detach old player.
	check(NewPlayer);
	if (NewPlayer->Actor != NULL)
	{
		NewPlayer->Actor->eventServerDestroy(); //@note: will be executed locally if this was the placeholder PC while waiting for connection to be established
		NewPlayer->Actor->Player = NULL;
		NewPlayer->Actor = NULL;
	}

	NewPlayer->CurrentNetSpeed = CurrentNetSpeed;

	// Init the new playerpawn.
	PC->Role = ROLE_AutonomousProxy;
	PC->SetPlayer(NewPlayer);
	debugf(TEXT("%s setplayer %s"), *PC->GetName(), *NewPlayer->GetName());
	Actor = PC;
}

/*---------------------------------------------------------------------------------------
	File transfer.
---------------------------------------------------------------------------------------*/
//
// Initiate downloading a file to the cache directory.
// The transfer will eventually succeed or fail, and the
// NotifyReceivedFile will be called with the results.
//
void UNetConnection::ReceiveFile( INT PackageIndex )
{
	check(PackageMap->List.IsValidIndex(PackageIndex));
	if( DownloadInfo.Num() == 0 )
	{
		DownloadInfo.AddZeroed();
		DownloadInfo(0).Class = UChannelDownload::StaticClass();
		DownloadInfo(0).ClassName = TEXT("Engine.UChannelDownload");
		DownloadInfo(0).Params = TEXT("");
		DownloadInfo(0).Compression = 0;
	}
	Download = ConstructObject<UDownload>( DownloadInfo(0).Class );	
	Download->ReceiveFile( this, PackageIndex, *DownloadInfo(0).Params, DownloadInfo(0).Compression );
}

#if DO_ENABLE_NET_TEST
/**
 * Copies the simulation settings from the net driver to this object
 */
void UNetConnection::UpdatePacketSimulationSettings(void)
{
	PacketSimulationSettings.PktLoss = Driver->PacketSimulationSettings.PktLoss;
	PacketSimulationSettings.PktOrder = Driver->PacketSimulationSettings.PktOrder;
	PacketSimulationSettings.PktDup = Driver->PacketSimulationSettings.PktDup;
	PacketSimulationSettings.PktLag = Driver->PacketSimulationSettings.PktLag;
	PacketSimulationSettings.PktLagVariance = Driver->PacketSimulationSettings.PktLagVariance;
}
#endif

/**
 * Called to determine if a voice packet should be replicated to this
 * connection or any of its child connections
 *
 * @param Sender - the sender of the voice packet
 *
 * @return true if it should be sent on this connection, false otherwise
 */
UBOOL UNetConnection::ShouldReplicateVoicePacketFrom(const FUniqueNetId& Sender)
{
	// Check with the owning player controller first
	if (Actor && Actor->IsPlayerMuted(Sender))
	{
		// The parent wants to mute, but see if any child connections want the data
		for (INT Index = 0; Index < Children.Num(); Index++)
		{
			if (Children(Index)->ShouldReplicateVoicePacketFrom(Sender))
			{
				// A child wants it so replicate the packet
				return TRUE;
			}
		}
		// No child wanted it so discard
		return FALSE;
	}
	// The parent connection wanted it so no need to check the children
	return TRUE;
}
