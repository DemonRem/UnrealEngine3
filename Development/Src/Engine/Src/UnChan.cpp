/*=============================================================================
	UnChan.cpp: Unreal datachannel implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"

static inline void SerializeCompRotator( FArchive& Ar, FRotator& R )
{
	BYTE Pitch(R.Pitch>>8), Yaw(R.Yaw>>8), Roll(R.Roll>>8), B;
	B = (Pitch!=0);
	Ar.SerializeBits( &B, 1 );
	if( B )
		Ar << Pitch;
	else
		Pitch = 0;
	B = (Yaw!=0);
	Ar.SerializeBits( &B, 1 );
	if( B )
		Ar << Yaw;
	else
		Yaw = 0;
	B = (Roll!=0);
	Ar.SerializeBits( &B, 1 );
	if( B )
		Ar << Roll;
	else
		Roll = 0;
	if( Ar.IsLoading() )
		R = FRotator(Pitch<<8,Yaw<<8,Roll<<8);
}

static inline void SerializeCompVector( FArchive& Ar, FVector& V )
{
	INT X(appRound(V.X)), Y(appRound(V.Y)), Z(appRound(V.Z));
	DWORD Bits = Clamp<DWORD>( appCeilLogTwo(1+Max(Max(Abs(X),Abs(Y)),Abs(Z))), 1, 20 )-1;
	Ar.SerializeInt( Bits, 20 );
	INT   Bias = 1<<(Bits+1);
	DWORD Max  = 1<<(Bits+2);
	DWORD DX(X+Bias), DY(Y+Bias), DZ(Z+Bias);
	Ar.SerializeInt( DX, Max );
	Ar.SerializeInt( DY, Max );
	Ar.SerializeInt( DZ, Max );
	if( Ar.IsLoading() )
		V = FVector((INT)DX-Bias,(INT)DY-Bias,(INT)DZ-Bias);
}

// passing NULL for UActorChannel will skip recent property update
static inline void SerializeCompressedInitial( FArchive& Bunch, FVector& Location, FRotator& Rotation, UBOOL bSerializeRotation, UActorChannel* Ch )
{
    // read/write compressed location
    SerializeCompVector( Bunch, Location );
    if( Ch && Ch->Recent.Num() )
        ((AActor*)&Ch->Recent(0))->Location = Location;

    // optionally read/write compressed rotation
    if( bSerializeRotation )
    {
        SerializeCompRotator( Bunch, Rotation );
	    if( Ch && Ch->Recent.Num() )
            ((AActor*)&Ch->Recent(0))->Rotation = Rotation;
    }
}

/*-----------------------------------------------------------------------------
	UChannel implementation.
-----------------------------------------------------------------------------*/

//
// Initialize the base channel.
//
UChannel::UChannel()
{}
void UChannel::Init( UNetConnection* InConnection, INT InChIndex, UBOOL InOpenedLocally )
{
	// if child connection then use its parent
	if (InConnection->GetUChildConnection() != NULL)
	{
		Connection = ((UChildConnection*)InConnection)->Parent;
	}
	else
	{
		Connection = InConnection;
	}
	ChIndex			= InChIndex;
	OpenedLocally	= InOpenedLocally;
	OpenPacketId	= INDEX_NONE;
	NegotiatedVer	= InConnection->NegotiatedVer;
}

//
// Set the closing flag.
//
void UChannel::SetClosingFlag()
{
	Closing = 1;
}

//
// Close the base channel.
//
void UChannel::Close()
{
	check(Connection->Channels[ChIndex]==this);
	if
	(	!Closing
	&&	(Connection->State==USOCK_Open || Connection->State==USOCK_Pending) )
	{
		// Send a close notify, and wait for ack.
		FOutBunch CloseBunch( this, 1 );
		check(!CloseBunch.IsError());
		check(CloseBunch.bClose);
		CloseBunch.bReliable = 1;
		SendBunch( &CloseBunch, 0 );
	}
}

/** cleans up channel structures and NULLs references to the channel */
void UChannel::CleanUp()
{
	checkSlow(Connection != NULL);
	checkSlow(Connection->Channels[ChIndex] == this);

	// if this is the control channel, make sure we properly killed the connection
	if (ChIndex == 0 && !Closing)
	{
		Connection->Close();
	}

	// remember sequence number of first non-acked outgoing reliable bunch for this slot
	if (OutRec != NULL)
	{
		Connection->PendingOutRec[ChIndex] = OutRec->ChSequence;
		//debugf(TEXT("%i save pending out bunch %i"),ChIndex,Connection->PendingOutRec[ChIndex]);
	}
	// Free any pending incoming and outgoing bunches.
	for (FOutBunch* Out = OutRec, *NextOut; Out != NULL; Out = NextOut)
	{
		NextOut = Out->Next;
		delete Out;
	}
	for (FInBunch* In = InRec, *NextIn; In != NULL; In = NextIn)
	{
		NextIn = In->Next;
		delete In;
	}

	// Remove from connection's channel table.
	verifySlow(Connection->OpenChannels.RemoveItem(this) == 1);
	Connection->Channels[ChIndex] = NULL;
	Connection = NULL;
}

//
// Base channel destructor.
//
void UChannel::BeginDestroy()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		ConditionalCleanUp();
	}
	
	Super::BeginDestroy();
}

//
// Handle an acknowledgement on this channel.
//
void UChannel::ReceivedAcks()
{
	check(Connection->Channels[ChIndex]==this);

	/*
	// Verify in sequence.
	for( FOutBunch* Out=OutRec; Out && Out->Next; Out=Out->Next )
		check(Out->Next->ChSequence>Out->ChSequence);
	*/

	// Release all acknowledged outgoing queued bunches.
	UBOOL DoClose = 0;
	while( OutRec && OutRec->ReceivedAck )
	{
		DoClose |= OutRec->bClose;
		FOutBunch* Release = OutRec;
		OutRec = OutRec->Next;
		delete Release;
		NumOutRec--;
	}

	// If a close has been acknowledged in sequence, we're done.
	if( DoClose || (OpenTemporary && OpenAcked) )
	{
		check(!OutRec);
		ConditionalCleanUp();
	}

}

//
// Return the maximum amount of data that can be sent in this bunch without overflow.
//
INT UChannel::MaxSendBytes()
{
	INT ResultBits
	=	Connection->MaxPacket*8
	-	(Connection->Out.GetNumBits() ? 0 : MAX_PACKET_HEADER_BITS)
	-	Connection->Out.GetNumBits()
	-	MAX_PACKET_TRAILER_BITS
	-	MAX_BUNCH_HEADER_BITS;
	return Max( 0, ResultBits/8 );
}

//
// Handle time passing on this channel.
//
void UChannel::Tick()
{
	checkSlow(Connection->Channels[ChIndex]==this);
	if( ChIndex==0 && !OpenAcked )
	{
		INT Count = 0;
		for( FOutBunch* Out=OutRec; Out; Out=Out->Next )
			if( !Out->ReceivedAck )
				Count++;
		if ( Count > 8 )
			return;
		// Resend any pending packets if we didn't get the appropriate acks.
		for( FOutBunch* Out=OutRec; Out; Out=Out->Next )
		{
			if( !Out->ReceivedAck )
			{
				FLOAT Wait = Connection->Driver->Time-Out->Time;
				checkSlow(Wait>=0.f);
				if( Wait>1.f )
				{
					debugfSlow( NAME_DevNetTraffic, TEXT("Channel %i ack timeout; resending %i..."), ChIndex, Out->ChSequence );
					check(Out->bReliable);
					Connection->SendRawBunch( *Out, 0 );
				}
			}
		}
	}
}

//
// Make sure the incoming buffer is in sequence and there are no duplicates.
//
void UChannel::AssertInSequenced()
{
#if DO_CHECK
	// Verify that buffer is in order with no duplicates.
	for( FInBunch* In=InRec; In && In->Next; In=In->Next )
		check(In->Next->ChSequence>In->ChSequence);
#endif
}

//
// Process a properly-sequenced bunch.
//
UBOOL UChannel::ReceivedSequencedBunch( FInBunch& Bunch )
{
	// Note this bunch's retirement.
	if( Bunch.bReliable )
		Connection->InReliable[ChIndex] = Bunch.ChSequence;

	// Handle a regular bunch.
	if( !Closing )
		ReceivedBunch( Bunch );

	// We have fully received the bunch, so process it.
	if( Bunch.bClose )
	{
		// Handle a close-notify.
		if( InRec )
			appErrorfSlow( TEXT("Close Anomaly %i / %i"), Bunch.ChSequence, InRec->ChSequence );
		debugfSuppressed( NAME_DevNetTraffic, TEXT("      Channel %i got close-notify"), ChIndex );
		ConditionalCleanUp();
		return 1;
	}
	return 0;
}

//
// Process a raw, possibly out-of-sequence bunch: either queue it or dispatch it.
// The bunch is sure not to be discarded.
//
void UChannel::ReceivedRawBunch( FInBunch& Bunch )
{
	check(Connection->Channels[ChIndex]==this);
	if
	(	Bunch.bReliable
	&&	Bunch.ChSequence!=Connection->InReliable[ChIndex]+1 )
	{
		// If this bunch has a dependency on a previous unreceived bunch, buffer it.
		checkSlow(!Bunch.bOpen);

		// Verify that UConnection::ReceivedPacket has passed us a valid bunch.
		check(Bunch.ChSequence>Connection->InReliable[ChIndex]);

		// Find the place for this item, sorted in sequence.
		debugfSlow( NAME_DevNetTraffic, TEXT("      Queuing bunch with unreceived dependency") );
		FInBunch** InPtr;
		for( InPtr=&InRec; *InPtr; InPtr=&(*InPtr)->Next )
		{
			if( Bunch.ChSequence==(*InPtr)->ChSequence )
			{
				// Already queued.
				return;
			}
			else if( Bunch.ChSequence<(*InPtr)->ChSequence )
			{
				// Stick before this one.
				break;
			}
		}
		FInBunch* New = new FInBunch(Bunch);
		New->Next     = *InPtr;
		*InPtr        = New;
		NumInRec++;
		checkSlow(NumInRec<=RELIABLE_BUFFER);
		//AssertInSequenced();
	}
	else
	{
		// Receive it in sequence.
		UBOOL bDeleted = ReceivedSequencedBunch( Bunch );
		if( bDeleted )
		{
			return;
		}
		// Dispatch any waiting bunches.
		while( InRec )
		{
			if( InRec->ChSequence!=Connection->InReliable[ChIndex]+1 )
				break;
			debugfSlow( NAME_DevNetTraffic, TEXT("      Unleashing queued bunch") );
			FInBunch* Release = InRec;
			InRec = InRec->Next;
			NumInRec--;
			bDeleted = ReceivedSequencedBunch( *Release );
			delete Release;
			if (bDeleted)
			{
				return;
			}
			//AssertInSequenced();
		}
	}
}

//
// Send a bunch if it's not overflowed, and queue it if it's reliable.
//
INT UChannel::SendBunch( FOutBunch* Bunch, UBOOL Merge )
{
	check(!Closing);
	check(Connection->Channels[ChIndex]==this);
	check(!Bunch->IsError());

	// Set bunch flags.
	if( OpenPacketId==INDEX_NONE && OpenedLocally )
	{
		Bunch->bOpen = 1;
		OpenTemporary = !Bunch->bReliable;
	}

	// If channel was opened temporarily, we are never allowed to send reliable packets on it.
	if( OpenTemporary )
		check(!Bunch->bReliable);

	// Contemplate merging.
	FOutBunch* OutBunch = NULL;
	if
	(	Merge
	&&	Connection->LastOut.ChIndex==Bunch->ChIndex
	&&	Connection->AllowMerge
	&&	Connection->LastEnd.GetNumBits()
	&&	Connection->LastEnd.GetNumBits()==Connection->Out.GetNumBits()
	&&	Connection->Out.GetNumBytes()+Bunch->GetNumBytes()+(MAX_BUNCH_HEADER_BITS+MAX_PACKET_TRAILER_BITS+7)/8<=Connection->MaxPacket )
	{
		// Merge.
		check(!Connection->LastOut.IsError());
		Connection->LastOut.SerializeBits( Bunch->GetData(), Bunch->GetNumBits() );
		Connection->LastOut.bReliable |= Bunch->bReliable;
		Connection->LastOut.bOpen     |= Bunch->bOpen;
		Connection->LastOut.bClose    |= Bunch->bClose;
		OutBunch                       = Connection->LastOutBunch;
		Bunch                          = &Connection->LastOut;
		check(!Bunch->IsError());
		Connection->LastStart.Pop( Connection->Out );
		Connection->Driver->OutBunches--;
	}

	// Find outgoing bunch index.
	if( Bunch->bReliable )
	{
		// Find spot, which was guaranteed available by FOutBunch constructor.
		if( OutBunch==NULL )
		{
			check(NumOutRec<RELIABLE_BUFFER-1+Bunch->bClose);
			Bunch->Next	= NULL;
			Bunch->ChSequence = ++Connection->OutReliable[ChIndex];
			NumOutRec++;
			OutBunch = new FOutBunch(*Bunch);
			FOutBunch** OutLink;
			for( OutLink=&OutRec; *OutLink; OutLink=&(*OutLink)->Next );
			*OutLink = OutBunch;
		}
		else
		{
			Bunch->Next = OutBunch->Next;
			*OutBunch = *Bunch;
		}
		Connection->LastOutBunch = OutBunch;
	}
	else
	{
		OutBunch = Bunch;
		Connection->LastOutBunch = NULL;//warning: Complex code, don't mess with this!
	}

	// Send the raw bunch.
	OutBunch->ReceivedAck = 0;
	INT PacketId = Connection->SendRawBunch(*OutBunch, Merge);
	if( OpenPacketId==INDEX_NONE && OpenedLocally )
		OpenPacketId = PacketId;
	if( OutBunch->bClose )
		SetClosingFlag();

	// Update channel sequence count.
	Connection->LastOut = *OutBunch;
	Connection->LastEnd	= FBitWriterMark(Connection->Out);

	return PacketId;
}

//
// Describe the channel.
//
FString UChannel::Describe()
{
	return FString(TEXT("State=")) + (Closing ? TEXT("closing") : TEXT("open") );
}

//
// Return whether this channel is ready for sending.
//
INT UChannel::IsNetReady( UBOOL Saturate )
{
	// If saturation allowed, ignore queued byte count.
	if( NumOutRec>=RELIABLE_BUFFER-1 )
		return 0;
	return Connection->IsNetReady( Saturate );

}

//
// Returns whether the specified channel type exists.
//
UBOOL UChannel::IsKnownChannelType( INT Type )
{
	return Type>=0 && Type<CHTYPE_MAX && ChannelClasses[Type];
}

//
// Negative acknowledgement processing.
//
void UChannel::ReceivedNak( INT NakPacketId )
{
	for( FOutBunch* Out=OutRec; Out; Out=Out->Next )
	{
		// Retransmit reliable bunches in the lost packet.
		if( Out->PacketId==NakPacketId && !Out->ReceivedAck )
		{
			check(Out->bReliable);
			debugfSlow( NAME_DevNetTraffic, TEXT("      Channel %i nak; resending %i..."), Out->ChIndex, Out->ChSequence );
			Connection->SendRawBunch( *Out, 0 );
		}
	}
}

// UChannel statics.
UClass* UChannel::ChannelClasses[CHTYPE_MAX]={0,0,0,0,0,0,0,0};
IMPLEMENT_CLASS(UChannel)

/*-----------------------------------------------------------------------------
	UControlChannel implementation.
-----------------------------------------------------------------------------*/

//
// Initialize the text channel.
//
UControlChannel::UControlChannel()
{}
void UControlChannel::Init( UNetConnection* InConnection, INT InChannelIndex, UBOOL InOpenedLocally )
{
	Super::Init( InConnection, InChannelIndex, InOpenedLocally );
	// If we are opened as a server connection, do the endian checking
	// The client assumes that the data will always have the correct byte order
	if (InOpenedLocally == FALSE)
	{
		// Mark this channel as needing endianess determination
		bNeedsEndianInspection = TRUE;
	}
}

/**
 * Inspects the packet for endianess information. Validates this information
 * against what the client sent. If anything seems wrong, the connection is
 * closed
 *
 * @param Bunch the packet to inspect
 *
 * @return TRUE if the packet is good, FALSE otherwise (closes socket)
 */
UBOOL UControlChannel::CheckEndianess(FInBunch& Bunch)
{
	// Assume the packet is bogus and the connection needs closing
	UBOOL bConnectionOk = FALSE;
	// Get pointers to the raw packet data
	BYTE* RawData = Bunch.GetData();
	const BYTE* HelloString = RawData + 4;
	// Check for a packet that is big enough to look at (4 length bytes + "HELLO P=n")
	if (Bunch.GetNumBytes() >= 13)
	{
		if (HelloString[0] == 'H' &&
			HelloString[1] == 'E' &&
			HelloString[2] == 'L' &&
			HelloString[3] == 'L' &&
			HelloString[4] == 'O' &&
			HelloString[5] == ' ' &&
			HelloString[6] == 'P' &&
			HelloString[7] == '=')
		{
			// Get platform id
			UE3::EPlatformType OtherPlatform = (UE3::EPlatformType)(HelloString[8] - '0');
			// Check whether the other platform needs byte swapping by
			// using the value sent in the packet. Note: we still validate it
			if (appNetworkNeedsByteSwapping(OtherPlatform))
			{
#if NO_BYTE_ORDER_SERIALIZE && !WANTS_XBOX_BYTE_SWAPPING
				// The xbox doesn't swap ever
				debugf(NAME_Error,TEXT("Refusing PC client"));
				return FALSE;
#else
				// Client has opposite endianess so swap this bunch
				// and mark the connection as needing byte swapping
				Bunch.SetByteSwapping(TRUE);
				Connection->bNeedsByteSwapping = TRUE;
#endif
			}
			else
			{
				// Disable all swapping
				Bunch.SetByteSwapping(FALSE);
				Connection->bNeedsByteSwapping = FALSE;
			}
			// We parsed everything so keep the connection open
			bConnectionOk = TRUE;
			bNeedsEndianInspection = FALSE;
		}
	}
	return bConnectionOk;
}

//
// Handle an incoming bunch.
//
void UControlChannel::ReceivedBunch( FInBunch& Bunch )
{
	check(!Closing);
	// If this is a new client connection inspect the raw packet for endianess
	if (bNeedsEndianInspection == TRUE)
	{
		// Check for an error in the processing and close the connection if so
		if (CheckEndianess(Bunch) == FALSE)
		{
			// Send close bunch and shutdown this connection
			Connection->Close();
			return;
		}
	}
	// Process the packet
	do
	{
		FString Text;
		Bunch << Text;
		if( Bunch.IsError() )
		{
			break;
		}
		
		Connection->Driver->Notify->NotifyReceivedText(Connection, *Text);
		// if the connection got closed, we don't care about the rest
	} while (Connection != NULL && Connection->State != USOCK_Closed);
}

/** adds the given string to the QueuedMessages list. Closes the connection if MAX_QUEUED_CONTROL_MESSAGES is exceeded */
void UControlChannel::QueueMessage(const TCHAR* Data)
{
	if (QueuedMessages.Num() >= MAX_QUEUED_CONTROL_MESSAGES)
	{
		// we're out of room in our extra buffer as well, so kill the connection
		debugf(NAME_DevNet, TEXT("Overflowed control channel message queue, disconnecting client"));
		Connection->Close();
	}
	else
	{
		new(QueuedMessages) FString(Data);
	}
}

//
// Text channel FArchive interface.
//
void UControlChannel::Serialize( const TCHAR* Data, EName MsgType )
{
	// if we already have queued messages, we need to queue subsequent ones to guarantee proper ordering
	if (QueuedMessages.Num() > 0)
	{
		QueueMessage(Data);
	}
	else
	{
		FOutBunch Bunch(this, 0);
		if (!Bunch.IsError())
		{
			Bunch.bReliable = 1;
			FString Text = Data;
			Bunch << Text;
			if (!Bunch.IsError())
			{
				SendBunch(&Bunch, 1);
			}
			else
			{
				// an error here most likely indicates an unfixable error, such as the text using more than the maximum packet size
				// so there is no point in queueing it as it will just fail again
				appErrorfSlow(TEXT("Control channel bunch overflowed"));
				debugf(NAME_Error, TEXT("Control channel bunch overflowed"));
				Connection->Close();
			}
		}
		else
		{
			// no more room for this bunch, probably because we overflowed the RELIABLE_BUFFER
			// queue the message in our additional buffer until it can be sent
			QueueMessage(Data);
		}
	}
}

void UControlChannel::Tick()
{
	Super::Tick();

	// attempt to send queued messages
	while (QueuedMessages.Num() > 0 && !Closing)
	{
		FOutBunch Bunch(this, 0);
		if (Bunch.IsError())
		{
			break;
		}
		else
		{
			Bunch.bReliable = 1;
			Bunch << QueuedMessages(0);
			if (!Bunch.IsError())
			{
				SendBunch(&Bunch, 1);
				QueuedMessages.Remove(0, 1);
			}
			else
			{
				// an error here most likely indicates an unfixable error, such as the text using more than the maximum packet size
				// so there is no point in queueing it as it will just fail again
				appErrorfSlow(TEXT("Control channel bunch overflowed"));
				debugf(NAME_Error, TEXT("Control channel bunch overflowed"));
				Connection->Close();
				break;
			}
		}
	}
}

//
// Describe the text channel.
//
FString UControlChannel::Describe()
{
	return FString(TEXT("Text ")) + UChannel::Describe();
}

IMPLEMENT_CLASS(UControlChannel);

/*-----------------------------------------------------------------------------
	UActorChannel.
-----------------------------------------------------------------------------*/

//
// Initialize this actor channel.
//
UActorChannel::UActorChannel()
{}
void UActorChannel::Init( UNetConnection* InConnection, INT InChannelIndex, UBOOL InOpenedLocally )
{
	Super::Init( InConnection, InChannelIndex, InOpenedLocally );
	World			= Connection->Driver->Notify->NotifyGetWorld();
	RelevantTime	= Connection->Driver->Time;
	LastUpdateTime	= Connection->Driver->Time - Connection->Driver->SpawnPrioritySeconds;
	LastFullUpdateTime = -1.f;
	ActorDirty = TRUE;
	bActorMustStayDirty = FALSE;
	bActorStillInitial = FALSE;
}

//
// Set the closing flag.
//
void UActorChannel::SetClosingFlag()
{
	if( Actor )
		Connection->ActorChannels.Remove( Actor );
	UChannel::SetClosingFlag();
}

//
// Close it.
//
void UActorChannel::Close()
{
	UChannel::Close();
	if (Actor != NULL)
	{
		// SetClosingFlag() might have already done this, but we need to make sure as that won't get called if the connection itself has already been closed
		Connection->ActorChannels.Remove(Actor);

		if (!Actor->bStatic && !Actor->bNoDelete && bClearRecentActorRefs)
		{
			// if transient actor lost relevance, clear references to it from other channels' Recent data to mirror what happens on the other side of the connection
			// so that if it becomes relevant again, we know we need to resend those references
			for (TMap<AActor*, UActorChannel*>::TIterator It(Connection->ActorChannels); It; ++It)
			{
				UActorChannel* Chan = It.Value();
				if (Chan != NULL && Chan->Actor != NULL && !Chan->Closing)
				{
					for (INT j = 0; j < Chan->ReplicatedActorProperties.Num(); j++)
					{
						AActor** ActorRef = (AActor**)((BYTE*)Chan->Recent.GetData() + Chan->ReplicatedActorProperties(j)->Offset);
						if (*ActorRef == Actor)
						{
							*ActorRef = NULL;
							Chan->ActorDirty = TRUE;
							debugfSlow( NAME_DevNetTraffic, TEXT("Clearing Recent ref to irrelevant Actor %s from channel %i (property %s, Actor %s)"), Chan->ChIndex, *Chan->Actor->GetName(),
										*Chan->ReplicatedActorProperties(j)->GetPathName(), *Actor->GetName() );
						}
					}
				}
			}
		}

		Actor = NULL;
	}
}

void UActorChannel::StaticConstructor()
{
	UClass* TheClass = GetClass();
	TheClass->EmitObjectReference(STRUCT_OFFSET(UActorChannel,Actor));
	TheClass->EmitObjectReference(STRUCT_OFFSET(UActorChannel,ActorClass));
	TheClass->EmitObjectArrayReference(STRUCT_OFFSET(UActorChannel, ReplicatedActorProperties));
}

/** cleans up channel structures and NULLs references to the channel */
void UActorChannel::CleanUp()
{
	// Remove from hash and stuff.
	SetClosingFlag();

	// Destroy Recent properties.
	if (Recent.Num() > 0)
	{
		checkSlow(ActorClass != NULL);
		UObject::ExitProperties(&Recent(0), ActorClass);
	}

	// If we're the client, destroy this actor.
	if (Connection->Driver->ServerConnection != NULL)
	{
		check(Actor == NULL || Actor->IsValid());
		checkSlow(Connection != NULL);
		checkSlow(Connection->IsValid());
		checkSlow(Connection->Driver != NULL);
		checkSlow(Connection->Driver->IsValid());
		if (Actor != NULL)
		{
			if (Actor->bTearOff)
			{
				Actor->Role = ROLE_Authority;
				Actor->RemoteRole = ROLE_None;
			}
			else if (!Actor->bNetTemporary && GWorld != NULL && !GIsRequestingExit)
			{
				// if actor should be destroyed and isn't already, then destroy it
				GWorld->DestroyActor(Actor, 1);
			}
		}
	}
	else if (Actor && !OpenAcked)
	{
		// Resend temporary actors if nak'd.
		Connection->SentTemporaries.RemoveItem(Actor);
	}

	Super::CleanUp();
}

//
// Negative acknowledgements.
//
void UActorChannel::ReceivedNak( INT NakPacketId )
{
	UChannel::ReceivedNak(NakPacketId);
	if( ActorClass )
		for( INT i=Retirement.Num()-1; i>=0; i-- )
			if( Retirement(i).OutPacketId==NakPacketId && !Retirement(i).Reliable )
				Dirty.AddUniqueItem(i);
    ActorDirty = true; 
}

//
// Allocate replication tables for the actor channel.
//
void UActorChannel::SetChannelActor( AActor* InActor )
{
	check(!Closing);
	check(Actor==NULL);

	// Set stuff.
	Actor                      = InActor;
	ActorClass                 = Actor->GetClass();
	FClassNetCache* ClassCache = Connection->PackageMap->GetClassNetCache( ActorClass );

	if ( Connection->PendingOutRec[ChIndex] > 0 )
	{
		// send empty reliable bunches to synchronize both sides
		// debugf(TEXT("%i Actor %s WILL BE sending %i vs first %i"), ChIndex, *Actor->GetName(), Connection->PendingOutRec[ChIndex],Connection->OutReliable[ChIndex]);
		INT RealOutReliable = Connection->OutReliable[ChIndex];
		Connection->OutReliable[ChIndex] = Connection->PendingOutRec[ChIndex] - 1;
		while ( Connection->PendingOutRec[ChIndex] <= RealOutReliable )
		{
			// debugf(TEXT("%i SYNCHRONIZING by sending %i"), ChIndex, Connection->PendingOutRec[ChIndex]);

			FOutBunch Bunch( this, 0 );
			if( !Bunch.IsError() )
			{
				Bunch.bReliable = true;
				SendBunch( &Bunch, 0 );
				Connection->PendingOutRec[ChIndex]++;
			}
		}

		Connection->OutReliable[ChIndex] = RealOutReliable;
		Connection->PendingOutRec[ChIndex] = 0;
	}


	// Add to map.
	Connection->ActorChannels.Set( Actor, this );

	// Allocate replication condition evaluation cache.
	RepEval.AddZeroed( ClassCache->GetRepConditionCount() );

	// Init recent properties.
	if( !InActor->bNetTemporary )
	{
		// Allocate recent property list.
		INT Size = ActorClass->GetDefaultsCount();
		Recent.Add( Size );
		UObject::InitProperties( &Recent(0), Size, ActorClass, NULL, 0 );

		// Init config properties, to force replicate them.
		for( UProperty* It=ActorClass->ConfigLink; It; It=It->ConfigLinkNext )
		{
			if( It->PropertyFlags & CPF_NeedCtorLink )
				It->DestroyValue( &Recent(It->Offset) );
            UBoolProperty* BoolProperty = Cast<UBoolProperty>(It,CLASS_IsAUBoolProperty);
			if( !BoolProperty )
				appMemzero( &Recent(It->Offset), It->GetSize() );
			else
				*(DWORD*)&Recent(It->Offset) &= ~BoolProperty->BitMask;
		}
	}

	// Allocate retirement list.
	Retirement.Empty( ActorClass->ClassReps.Num() );
	while( Retirement.Num()<ActorClass->ClassReps.Num() )
		new(Retirement)FPropertyRetirement;

	// figure out list of replicated actor properties
	for (UProperty* Prop = ActorClass->RefLink; Prop != NULL; Prop = Prop->NextRef)
	{
		if (Prop->PropertyFlags & CPF_Net)
		{
			UObjectProperty* ObjectProp = Cast<UObjectProperty>(Prop, CLASS_IsAUObjectProperty);
			if (ObjectProp != NULL && ObjectProp->PropertyClass != NULL && ObjectProp->PropertyClass->IsChildOf(AActor::StaticClass()))
			{
				ReplicatedActorProperties.AddItem(ObjectProp);
			}
		}
	}
}

//
// Handle receiving a bunch of data on this actor channel.
//
void UActorChannel::ReceivedBunch( FInBunch& Bunch )
{
	check(!Closing);
	if ( Broken || bTornOff )
		return;

	// Initialize client if first time through.
	INT bJustSpawned = 0;
	FClassNetCache* ClassCache = NULL;
	if( Actor==NULL )
	{
		if( !Bunch.bOpen )
		{
			debugfSuppressed(NAME_DevNetTraffic, TEXT("New actor channel received non-open packet: %i/%i/%i"),Bunch.bOpen,Bunch.bClose,Bunch.bReliable);
			return;
		}

		// Read class.
		UObject* Object;
		Bunch << Object;
		AActor* InActor = Cast<AActor>( Object );
		if (InActor == NULL)
		{
			// We are unsynchronized. Instead of crashing, let's try to recover.
			warnf(TEXT("Received invalid actor class on channel %i"), ChIndex);
			Broken = 1;
			return;
		}
		if (InActor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			// Transient actor.
			FVector Location;
			FRotator Rotation(0,0,0);
			SerializeCompressedInitial(Bunch, Location, Rotation, InActor->bNetInitialRotation, NULL);

			InActor = GWorld->SpawnActor(InActor->GetClass(), NAME_None, Location, Rotation, InActor, 1, 1, NULL, NULL, 1); 
			bJustSpawned = 1;
			if ( !InActor )
			{
				debugf(TEXT("Couldn't spawn %s replicated from server"), *ActorClass->GetName());
			}
			else if ( InActor->bDeleteMe )
			{
				debugf(TEXT("Client received and deleted instantly %s"),*InActor->GetName());
			}
			check(InActor);
		}
		debugfSuppressed( NAME_DevNetTraffic, TEXT("      Spawn %s:"), *InActor->GetFullName() );
		SetChannelActor( InActor );

		// if it's a PlayerController, attempt to match it to a local viewport
		APlayerController* PC = Actor->GetAPlayerController();
		if (PC != NULL)
		{
			Bunch << PC->NetPlayerIndex;
			if (Connection->Driver != NULL && Connection == Connection->Driver->ServerConnection)
			{
				if (PC->NetPlayerIndex == 0)
				{
					// main connection PlayerController
					Connection->HandleClientPlayer(PC); 
				}
				else
				{
					INT ChildIndex = INT(PC->NetPlayerIndex) - 1;
					if (Connection->Children.IsValidIndex(ChildIndex))
					{
						// received a new PlayerController for an already existing child
						Connection->Children(ChildIndex)->HandleClientPlayer(PC);
					}
					else
					{
						// create a split connection on the client
						UChildConnection* Child = Connection->Driver->CreateChild(Connection); 
						Child->HandleClientPlayer(PC);
						debugf(NAME_DevNet, TEXT("Client received PlayerController=%s. Num child connections=%i."), *Actor->GetName(), Connection->Children.Num());
					}
				}
			}
		}
	}
	else
	{
		debugfSuppressed( NAME_DevNetTraffic, TEXT("      Actor %s:"), *Actor->GetFullName() );
	}
	ClassCache = Connection->PackageMap->GetClassNetCache(ActorClass);
	checkSlow(ClassCache != NULL);

	// Owned by connection's player?
	Actor->bNetOwner = 0;
	APlayerController* Top = Actor->GetTopPlayerController();
	if(Top)
	{
		UPlayer* Player = Top ? Top->Player : NULL;
		// Set quickie replication variables.
		if( Connection->Driver->ServerConnection )
		{
			// We are the client.
			if( Player && Player->IsA( ULocalPlayer::StaticClass() ) )
				Actor->bNetOwner = 1;
		}
		else
		{
			// We are the server.
			if (Player == Connection || (Player != NULL && Player->IsA(UChildConnection::StaticClass()) && ((UChildConnection*)Player)->Parent == Connection))
			{
				Actor->bNetOwner = 1;
			}
		}
	}

	// Handle the data stream.
	INT             RepIndex   = Bunch.ReadInt( ClassCache->GetMaxIndex() );
	FFieldNetCache* FieldCache = Bunch.IsError() ? NULL : ClassCache->GetFromIndex( RepIndex );
	const UBOOL bIsServer = (Connection->Driver->ServerConnection == NULL);
	while (FieldCache || bJustSpawned)
	{
		// Save current properties.
		//debugf(TEXT("Rep %s: %i"),FieldCache->Field->GetFullName(),RepIndex);
		UBOOL bHasFieldCache = (FieldCache != NULL);
		if (bHasFieldCache && !bIsServer)
		{
			Actor->PreNetReceive();
		}

		// Receive properties from the net.
		TArray<UProperty*> RepNotifies;
		UProperty* It;
		while( FieldCache && (It=Cast<UProperty>(FieldCache->Field,CLASS_IsAUProperty))!=NULL )
		{
			// Receive array index.
			BYTE Element=0;
			if( It->ArrayDim != 1 )
				Bunch << Element;

			// Pointer to destiation.
			BYTE* DestActor  = (BYTE*)Actor;
			BYTE* DestRecent = Recent.Num() ? &Recent(0) : NULL;

			// Server shouldn't receive properties.
			if (bIsServer)
			{
				debugfSlow( TEXT("Server received unwanted property value %s in %s"), *It->GetName(), *Actor->GetFullName() );
					DestActor  = NULL;
					DestRecent = NULL;
			}

			// Check property ordering.
			FPropertyRetirement& Retire = Retirement( It->RepIndex + Element );
			if( Bunch.PacketId>=Retire.InPacketId ) //!! problem with reliable pkts containing dynamic references, being retransmitted, and overriding newer versions. Want "OriginalPacketId" for retransmissions?
			{
				// Receive this new property.
				Retire.InPacketId = Bunch.PacketId;
			}
			else
			{
				// Skip this property, because it's out-of-date.
				debugfSlow( NAME_DevNetTraffic, TEXT("Received out-of-date %s"), *It->GetName() );
				DestActor  = NULL;
				DestRecent = NULL;
			}

			// Receive property.
			FMemMark Mark(GMem);
			INT   Offset = It->Offset + Element*It->ElementSize;
			BYTE* Data   = DestActor ? (DestActor + Offset) : NewZeroed<BYTE>(GMem,It->ElementSize);
			It->NetSerializeItem( Bunch, Connection->PackageMap, Data );
			if( DestRecent )
				It->CopySingleValue( DestRecent + Offset, Data );
			Mark.Pop();
			// Successfully received it.
			debugfSuppressed( NAME_DevNetTraffic, TEXT("         %s"), *It->GetName() );

			// Notify the actor if this var is RepNotify

			if ( It->PropertyFlags & CPF_RepNotify )
			{
				RepNotifies.AddItem(It);
			}

			// Get next.
			RepIndex   = Bunch.ReadInt( ClassCache->GetMaxIndex() );
			FieldCache = Bunch.IsError() ? NULL : ClassCache->GetFromIndex( RepIndex );
		}
		// Process important changed properties.
		if (!bIsServer)
		{
			if ( bHasFieldCache )
			{
				Actor->PostNetReceive();
				if (Actor == NULL)
				{
					// PostNetReceive() destroyed Actor
					return;
				}
				for (INT i = 0; i < RepNotifies.Num(); i++)
				{
					//debugf( TEXT("Calling Actor->eventReplicatedEvent with %s"), RepNotifies(i)->GetName());
					Actor->eventReplicatedEvent(RepNotifies(i)->GetFName());
					if (Actor == NULL)
					{
						// script event destroyed Actor
						return;
					}
				}
			}
			bJustSpawned = 0;
		}

		// Handle function calls.
		if( FieldCache && Cast<UFunction>(FieldCache->Field,CLASS_IsAUFunction) )
		{
			FName Message = FieldCache->Field->GetFName();
			UFunction* Function = Actor->FindFunction( Message );
			check(Function);

			debugfSuppressed( NAME_DevNetTraffic, TEXT("      Received RPC: %s"), *Message.ToString() );

			// Get the parameters.
			FMemMark Mark(GMem);
			BYTE* Parms = new(GMem,MEM_Zeroed,Function->ParmsSize)BYTE;
			for( TFieldIterator<UProperty,CLASS_IsAUProperty> Itr(Function); Itr && (Itr->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++Itr )
			{
				if( Connection->PackageMap->SupportsObject(*Itr) )
				{
					if( Cast<UBoolProperty>(*Itr,CLASS_IsAUBoolProperty) || Bunch.ReadBit() ) 
					{
						for (INT i = 0; i < Itr->ArrayDim; i++)
						{
							Itr->NetSerializeItem(Bunch, Connection->PackageMap, Parms + Itr->Offset + (i * Itr->ElementSize));
						}
					}
				}
			}

			// Call the function.
			Actor->ProcessEvent( Function, Parms );
//			debugfSuppressed( NAME_DevNetTraffic, TEXT("      Call RPC: %s"), *Function->GetName() );

			// Destroy the parameters.
			//warning: highly dependent on UObject::ProcessEvent freeing of parms!
			{for( UProperty* Destruct=Function->ConstructorLink; Destruct; Destruct=Destruct->ConstructorLinkNext )
				if( Destruct->Offset < Function->ParmsSize )
					Destruct->DestroyValue( Parms + Destruct->Offset );}
			Mark.Pop();

			if (Actor == NULL)
			{
				// replicated function destroyed Actor
				return;
			}

			// Next.
			RepIndex   = Bunch.ReadInt( ClassCache->GetMaxIndex() );
			FieldCache = Bunch.IsError() ? NULL : ClassCache->GetFromIndex( RepIndex );
		}
		else if( FieldCache )
		{
			appErrorfSlow( TEXT("Invalid replicated field %i in %s"), RepIndex, *Actor->GetFullName() );
			return;
		}
	}
	// Tear off an actor on the client-side
	if ( Actor && Actor->bTearOff && GWorld->GetNetMode() == NM_Client )
	{
		Actor->Role = ROLE_Authority;
		Actor->RemoteRole = ROLE_None;
		bTornOff = true;
		Connection->ActorChannels.Remove( Actor );
		Actor->eventTornOff();
		Actor = NULL;
	}
}

//
// Replicate this channel's actor differences.
//
void UActorChannel::ReplicateActor()
{
	checkSlow(Actor);
	checkSlow(!Closing);

	// triggering replication of an Actor while already in the middle of replication can result in invalid data being sent and is therefore illegal
	if (bIsReplicatingActor)
	{
		FString Error(FString::Printf(TEXT("Attempt to replicate '%s' while already replicating that Actor!"), *Actor->GetName()));
		debugf(*Error);
		appErrorfSlow(*Error);
		return;
	}

	bIsReplicatingActor = TRUE;

	// Create an outgoing bunch, and skip this actor if the channel is saturated.
	FOutBunch Bunch( this, 0 );
	if( Bunch.IsError() )
		return;

	// Send initial stuff.
	if( OpenPacketId!=INDEX_NONE )
	{
		Actor->bNetInitial = 0;
		if( !SpawnAcked && OpenAcked )
		{
			// After receiving ack to the spawn, force refresh of all subsequent unreliable packets, which could
			// have been lost due to ordering problems. Note: We could avoid this by doing it in FActorChannel::ReceivedAck,
			// and avoid dirtying properties whose acks were received *after* the spawn-ack (tricky ordering issues though).
			SpawnAcked = 1;
			for( INT i=Retirement.Num()-1; i>=0; i-- )
				if( Retirement(i).OutPacketId!=INDEX_NONE && !Retirement(i).Reliable )
					Dirty.AddUniqueItem(i);
		}
	}
	else
	{
		Actor->bNetInitial = 1;
		Bunch.bClose    =  Actor->bNetTemporary;
		Bunch.bReliable = !Actor->bNetTemporary;
	}
	// Get class network info cache.
	FClassNetCache* ClassCache = Connection->PackageMap->GetClassNetCache(Actor->GetClass());
	check(ClassCache);

	// Owned by connection's player?
	Actor->bNetOwner = 0;
	APlayerController* Top = Actor->GetTopPlayerController();
	UPlayer* Player = Top ? Top->Player : NULL;

	// Set quickie replication variables.
	UBOOL bDemoOwner = 0;
	if (Actor->bDemoRecording)
	{
		Actor->bNetOwner = 1;
		bDemoOwner = (Actor->WorldInfo->NetMode == NM_Client) ? (Cast<ULocalPlayer>(Player) != NULL) : Actor->bDemoOwner;
	}
	else
	{
		Actor->bNetOwner = Connection->Driver->ServerConnection ? Cast<ULocalPlayer>(Player)!=NULL : Player==Connection;
		// use child connection's parent
		if (Connection->Driver->ServerConnection == NULL &&	Player != NULL && Player->IsA(UChildConnection::StaticClass()) &&
			((UChildConnection*)Player)->Parent == Connection )
		{
			Actor->bNetOwner = 1;
		}
	}
#if CLIENT_DEMO
	Actor->bRepClientDemo = Actor->bNetOwner && Top && Top->bClientDemo;
#endif

	// If initial, send init data.
	if( Actor->bNetInitial && OpenedLocally )
	{
		if (Actor->bStatic || Actor->bNoDelete)
		{
			// Persistent actor.
#if PS3_SNC
			// convert the actor to an object to avoid multiple matches to the operator in SNC 
			UObject* ActorObject = Actor;
			Bunch << ActorObject;
#else
			Bunch << Actor;
#endif
		}
		else
		{
			// Transient actor.
			
			// check for conditions that would result in the client being unable to properly receive this Actor
			if (Actor->GetArchetype()->GetClass() != Actor->GetClass())
			{
				debugf( NAME_Warning, TEXT("Attempt to replicate %s with archetype class (%s) different from Actor class (%s)"),
						*Actor->GetName(), *Actor->GetArchetype()->GetClass()->GetName(), *Actor->GetClass()->GetName() );
			}

			// serialize it
			UObject* Archetype = Actor->GetArchetype();
			Bunch << Archetype;
			SerializeCompressedInitial( Bunch, Actor->Location, Actor->Rotation, Actor->bNetInitialRotation, this );

			// serialize PlayerIndex as part of the initial bunch for PlayerControllers so they can be matched to the correct clientside viewport
			APlayerController* PC = Actor->GetAPlayerController();
			if (PC != NULL)
			{
				Bunch << PC->NetPlayerIndex;
			}
		}
	}

	// Save out the actor's RemoteRole, and downgrade it if necessary.
	BYTE ActualRemoteRole=Actor->RemoteRole;
	if (Actor->RemoteRole==ROLE_AutonomousProxy && (((Actor->Instigator == NULL || !Actor->Instigator->bNetOwner) && !Actor->bNetOwner) || (Actor->bDemoRecording && !bDemoOwner)))
	{
		Actor->RemoteRole=ROLE_SimulatedProxy;
	}

	Actor->bNetDirty = ActorDirty || Actor->bNetInitial;
	Actor->bNetInitial = Actor->bNetInitial || bActorStillInitial; // for replication purposes, bNetInitial stays true until all properties sent
	bActorMustStayDirty = false;
	bActorStillInitial = false;

	debugfSuppressed(NAME_DevNetTraffic, TEXT("Replicate %s, bNetDirty: %d, bNetInitial: %d, bNetOwner: %d"), *Actor->GetName(), Actor->bNetDirty, Actor->bNetInitial, Actor->bNetOwner );

	// Get memory for retirement list.
	FMemMark MemMark(GMem);
	appMemzero( &RepEval(0), RepEval.Num() );
	INT* Reps = New<INT>( GMem, Retirement.Num() ), *LastRep;
	UBOOL		FilledUp = 0;

	// Figure out which properties to replicate.
	BYTE*   CompareBin = Recent.Num() ? &Recent(0) : ActorClass->GetDefaults();
	INT     iCount     = ClassCache->RepProperties.Num();
	LastRep            = Actor->GetOptimizedRepList( CompareBin, &Retirement(0), Reps, Connection->PackageMap,this );
	if ( Actor->bNetDirty )
	{
		//if ( iCount > 0 ) debugf(TEXT("%s iCount %d"),Actor->GetName(), iCount);
		if ( Actor->DelayScriptReplication(LastFullUpdateTime) )
		{
			debugfSuppressed(NAME_DevNetTraffic, TEXT(" DelayScriptReplication() and setting bActorMustStayDirty.") );
			bActorMustStayDirty = true;
		}
		else
		{
			for( INT iField=0; iField<iCount; iField++  )
			{
				FFieldNetCache* FieldCache = ClassCache->RepProperties(iField);
				UProperty* It = (UProperty*)(FieldCache->Field);  
				BYTE& Eval = RepEval(FieldCache->ConditionIndex);
				if( Eval!=2 )
				{
					UObjectProperty* Op = Cast<UObjectProperty>(It,CLASS_IsAUObjectProperty);
					for( INT Index=0; Index<It->ArrayDim; Index++ )
					{
						// Evaluate need to send the property.
						INT Offset = It->Offset + Index*It->ElementSize;
						BYTE* Src = (BYTE*)Actor + Offset;
						if( Op && !Connection->PackageMap->CanSerializeObject(*(UObject**)Src) )
						{
							if (!(*(UObject**)Src)->IsPendingKill())
							{
								if( !(Eval & 2) )
								{
									DWORD Val=0;
									FFrame( Actor, It->RepOwner->GetOwnerClass(), It->RepOwner->RepOffset, NULL ).Step( Actor, &Val );
									Eval = Val | 2;
								}
								if( Eval & 1 )
								{
									debugfSuppressed(NAME_DevNetTraffic,TEXT("MUST STAY DIRTY Because of %s"),*(*(UObject**)Src)->GetName());
									bActorMustStayDirty = true;
								}
							}
							Src = NULL;
						}
						if( !It->Identical(CompareBin+Offset,Src) )
						{
							if( !(Eval & 2) )
							{
								DWORD Val=0;
								FFrame( Actor, It->RepOwner->GetOwnerClass(), It->RepOwner->RepOffset, NULL ).Step( Actor, &Val );
								Eval = Val | 2;
							}
							if( Eval & 1 )
								*LastRep++ = It->RepIndex+Index;
						}
					}
				}
			}
		}
	}
	if (Actor->bNetInitial)
	{
		// add forced initial replicated properties to dirty list
		const TArray<UProperty*>* PropArray = Connection->Driver->ForcedInitialReplicationMap.Find(Actor);
		if (PropArray != NULL)
		{
			for (INT i = 0; i < PropArray->Num(); i++)
			{
				for (INT j = 0; j < (*PropArray)(i)->ArrayDim; j++)
				{
					Dirty.AddItem((*PropArray)(i)->RepIndex + j);
				}
			}
		}
	}
	check(!Bunch.IsError());

	// Add dirty properties to list.
	for( INT i=Dirty.Num()-1; i>=0; i-- )
	{
		INT D=Dirty(i);
		INT* R;
		for( R=Reps; R<LastRep; R++ )
			if( *R==D )
				break;
		if( R==LastRep )
			*LastRep++=D;
	}
	TArray<INT>  StillDirty;

	// Replicate those properties.
	for( INT* iPtr=Reps; iPtr<LastRep; iPtr++ )
	{
		// Get info.
		FRepRecord* Rep    = &ActorClass->ClassReps(*iPtr);
		UProperty*	It     = Rep->Property;
		INT         Index  = Rep->Index;
		INT         Offset = It->Offset + Index*It->ElementSize;

		// Figure out field to replicate.
		FFieldNetCache* FieldCache
		=	It->GetFName()==NAME_Role
		?	ClassCache->GetFromField(Connection->Driver->RemoteRoleProperty)
		:	It->GetFName()==NAME_RemoteRole
		?	ClassCache->GetFromField(Connection->Driver->RoleProperty)
		:	ClassCache->GetFromField(It);
		checkSlow(FieldCache);

		// Send property name and optional array index.
		FBitWriterMark WriterMark( Bunch );
		Bunch.WriteInt( FieldCache->FieldNetIndex, ClassCache->GetMaxIndex() );
		if( It->ArrayDim != 1 )
		{
			BYTE Element = Index;
			Bunch << Element;
		}

		// Send property.
		UBOOL Mapped = It->NetSerializeItem( Bunch, Connection->PackageMap, (BYTE*)Actor + Offset );
		debugfSuppressed(NAME_DevNetTraffic,TEXT("   Send %s %i"),*It->GetName(),Mapped);
		if( !Bunch.IsError() )
		{
			// Update recent value.
			if( Recent.Num() )
			{
				if( Mapped )
					It->CopySingleValue( &Recent(Offset), (BYTE*)Actor + Offset );
				else
					StillDirty.AddUniqueItem(*iPtr);
			}
		}
		else
		{
			// Stop the changes because we overflowed.
			WriterMark.Pop( Bunch );
			LastRep  = iPtr;
			//debugfSuppressed(NAME_DevNetTraffic,TEXT("FILLED UP"));
			FilledUp = 1;
			break;
		}
	}
	// If not empty, send and mark as updated.
	if( Bunch.GetNumBits() )
	{
		INT PacketId = SendBunch( &Bunch, 1 );
		for( INT* Rep=Reps; Rep<LastRep; Rep++ )
		{
			Dirty.RemoveItem(*Rep);
			FPropertyRetirement& Retire = Retirement(*Rep);
			Retire.OutPacketId = PacketId;
			Retire.Reliable    = Bunch.bReliable;
		}
		if( Actor->bNetTemporary )
		{
			Connection->SentTemporaries.AddItem( Actor );
		}
	}
	for ( INT i=0; i<StillDirty.Num(); i++ )
		Dirty.AddUniqueItem(StillDirty(i));

	// If we evaluated everything, mark LastUpdateTime, even if nothing changed.
	if ( FilledUp )
	{
		debugfSlow(NAME_DevNetTraffic,TEXT("Filled packet up before finishing %s still initial %d"),*Actor->GetName(),bActorStillInitial);
	}
	else
	{
		LastUpdateTime = Connection->Driver->Time;
	}

	bActorStillInitial = Actor->bNetInitial && (FilledUp || (!Actor->bNetTemporary && bActorMustStayDirty));
	ActorDirty = bActorMustStayDirty || FilledUp;

	// Reset temporary net info.
	Actor->bNetOwner  = 0;
	Actor->RemoteRole = ActualRemoteRole;
	Actor->bNetDirty = 0;

	MemMark.Pop();

	bIsReplicatingActor = FALSE;
}

//
// Describe the actor channel.
//
FString UActorChannel::Describe()
{
	if( Closing || !Actor )
		return FString(TEXT("Actor=None ")) + UChannel::Describe();
	else
		return FString::Printf(TEXT("Actor=%s (Role=%i RemoteRole=%i) "), *Actor->GetFullName(), Actor->Role, Actor->RemoteRole) + UChannel::Describe();
}

IMPLEMENT_CLASS(UActorChannel);

/*-----------------------------------------------------------------------------
	UFileChannel implementation.
-----------------------------------------------------------------------------*/

UFileChannel::UFileChannel()
{
	Download = NULL;
}
void UFileChannel::Init( UNetConnection* InConnection, INT InChannelIndex, UBOOL InOpenedLocally )
{
	Super::Init( InConnection, InChannelIndex, InOpenedLocally );
}
void UFileChannel::ReceivedBunch( FInBunch& Bunch )
{
	check(!Closing);
	if( OpenedLocally )
	{
		// Receiving a file sent from the other side.  If Bunch.GetNumBytes()==0, it means the server refused to send the file.
		Download->ReceiveData( Bunch.GetData(), Bunch.GetNumBytes() );
	}
	else
	{
		if( !Connection->Driver->AllowDownloads )
		{
			// Refuse the download by sending a 0 bunch.
			debugf( NAME_DevNet, *LocalizeError(TEXT("NetInvalid"),TEXT("Engine")) );
			FOutBunch Bunch( this, 1 );
			SendBunch( &Bunch, 0 );
			return;
		}
		if( SendFileAr )
		{
			FString Cmd;
			Bunch << Cmd;
			if( !Bunch.IsError() && Cmd==TEXT("SKIP") )
			{
				// User cancelled optional file download.
				// Remove it from the package map
				debugf( TEXT("User skipped download of '%s'"), SrcFilename );
				Connection->PackageMap->List.Remove( PackageIndex );
				return;
			}
		}
		else
		{
			// Request to send a file.
			FGuid Guid;
			Bunch << Guid;
			if( !Bunch.IsError() )
			{
				for( INT i=0; i<Connection->PackageMap->List.Num(); i++ )
				{
					FPackageInfo& Info = Connection->PackageMap->List(i);
					if (Info.Guid == Guid && Info.PackageName != NAME_None)
					{
						FString ServerPackage;
						if (GPackageFileCache->FindPackageFile(*Info.PackageName.ToString(), NULL, ServerPackage))
						{
							if ( Connection->Driver->MaxDownloadSize > 0 &&
								GFileManager->FileSize(*ServerPackage) > Connection->Driver->MaxDownloadSize )
							{
								break;							
							}

							// find the path to the source package file
							appStrncpy(SrcFilename, *ServerPackage, ARRAY_COUNT(SrcFilename));
							if( Connection->Driver->Notify->NotifySendingFile( Connection, Guid ) )
							{
								SendFileAr = GFileManager->CreateFileReader( SrcFilename );
								if( SendFileAr )
								{
									// Accepted! Now initiate file sending.
									debugf( NAME_DevNet, *LocalizeProgress(TEXT("NetSend"),TEXT("Engine")), SrcFilename );
									PackageIndex = i;
									return;
								}
							}
						}
						else
						{
//							appErrorf(TEXT("Server failed to find a package file that it reported was in its PackageMap [%s]"), *Info.PackageName.ToString());
							debugf(TEXT("ERROR: Server failed to find a package file that it reported was in its PackageMap [%s]"), *Info.PackageName.ToString());
						}
					}
				}
			}
		}

		// Illegal request; refuse it by closing the channel.
		debugf( NAME_DevNet, *LocalizeError(TEXT("NetInvalid"),TEXT("Engine")) );
		FOutBunch Bunch( this, 1 );
		SendBunch( &Bunch, 0 );
	}
}
void UFileChannel::Tick()
{
	UChannel::Tick();
	Connection->TimeSensitive = 1;
	INT Size;
	//TIM: IsNetReady(1) causes the client's bandwidth to be saturated. Good for clients, very bad
	// for bandwidth-limited servers. IsNetReady(0) caps the clients bandwidth.
	static UBOOL LanPlay = ParseParam(appCmdLine(),TEXT("lanplay"));
	while( !OpenedLocally && SendFileAr && IsNetReady(LanPlay) && (Size=MaxSendBytes())!=0 )
	{
		// Sending.
		// get the filesize (we can't use the PackageInfo's size, because this may be a streaming texture pacakge
		// and so the size will be zero
		INT FileSize = SendFileAr->TotalSize();

		INT Remaining = (sizeof(INT) + FileSize) - SentData;
		FOutBunch Bunch( this, Size>=Remaining );
		Size = Min( Size, Remaining );
		BYTE* Buffer = (BYTE*)appAlloca( Size );

		// first chunk gets total size prepended to it
		if (SentData == 0)
		{
			// put it in the buffer
			appMemcpy(Buffer, &FileSize, sizeof(INT));

			// read a little less then Size of the source data
			SendFileAr->Serialize(Buffer + sizeof(INT), Size - sizeof(INT));
		}
		else
		{
			// normal case, just read Size amount
			SendFileAr->Serialize(Buffer, Size);
		}

		if( SendFileAr->IsError() )
		{
			//!!
		}
		SentData += Size;
		Bunch.Serialize( Buffer, Size );
		Bunch.bReliable = 1;
		check(!Bunch.IsError());
		SendBunch( &Bunch, 0 );
		Connection->FlushNet();
		if( Bunch.bClose )
		{
			// Finished.
			delete SendFileAr;
			SendFileAr = NULL;
		}
	}
}

/** cleans up channel structures and NULLs references to the channel */
void UFileChannel::CleanUp()
{
	// Close the file.
	if (SendFileAr != NULL)
	{
		delete SendFileAr;
		SendFileAr = NULL;
	}

	// Notify that the receive succeeded or failed.
	if (OpenedLocally && Download != NULL)
	{
		Download->DownloadDone();
		Download->CleanUp();
	}

	Super::CleanUp();
}

FString UFileChannel::Describe()
{
	FPackageInfo& Info = Connection->PackageMap->List( PackageIndex );
	return FString::Printf
	(
		TEXT("File='%s', %s=%i "),
		OpenedLocally ? (Download?Download->TempFilename:TEXT("")): SrcFilename,
		OpenedLocally ? TEXT("Received") : TEXT("Sent"),
		OpenedLocally ? (Download?Download->Transfered:0): SentData
	) + UChannel::Describe();
}
IMPLEMENT_CLASS(UFileChannel)
