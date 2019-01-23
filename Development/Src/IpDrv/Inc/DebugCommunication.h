/*=============================================================================
	DebugCommunication.h:
	Allowing network communication between UE3 executable and the
	"Unreal Remote" tool running on a PC.
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef DEBUGCOMMUNICATION_HEADER
#define DEBUGCOMMUNICATION_HEADER


#include "UnIpDrv.h"

/**
 * Used for passing buffered data between one sender thread and one
 * receiver thread.
 *
 * Can also be used with multiple sender threads by wrapping calls to
 * BeginPush/EndPush/KickBuffer within a synchronization lock.
 */
class FMultiThreadRingBuffer
{
public:
	struct BufferData
	{
		BYTE *	Buffer;
		INT		Size;
		INT		NewIndex;	// Used internally for the new read/write index
	};

	FMultiThreadRingBuffer( INT InBufferSize );
	~FMultiThreadRingBuffer();

	UBOOL			BeginPush( BufferData &Data, INT Size );
	void			EndPush( BufferData &Data, UBOOL bForcePush=FALSE );
	UBOOL			Peek( BufferData &Entry  );
	void			Pop( BufferData &Entry );
	void			WaitForData( DWORD MilliSeconds = INFINITE );
	void			KickBuffer( );

protected:
	BYTE*			RingBuffer;
	INT				BufferSize;
	volatile INT	ReadIndex;			// Where the Reader starts reading from
	volatile INT	WriteIndex;			// Where the Writer has written into (available to the Reader)
	INT				CurrentWriteIndex;	// Where the Writer is currently writing into
	FEvent*			WorkEvent;
};

class FDebugCommunication
{
	enum { DEFAULTBUFFERSIZE=16384, MAX_COMMANDS=16, MAX_COMMANDLENGTH=128 };
	class FDebugReceiver : public FRunnable
	{
	public:
		FDebugReceiver(INT InPort, INT InTrafficPort);

		virtual UBOOL	Init( );
		virtual DWORD	Run( );
		virtual void	Stop( );
		virtual void	Exit( );

	protected:
		void			ProcessPacket( FIpAddr SrcAddr, BYTE* Data, INT Count );
		FSocket*		Socket;
		FIpAddr			IPAddress;
		INT				Port;
		INT				TrafficPort;
		FEvent*			TimeToDie;
	};

	class FDebugSender : public FRunnable
	{
	public:
		FDebugSender( INT InPort, INT BufferSize );
		~FDebugSender( );

		void			KickBuffer( );
		void			SendChannelData( const ANSICHAR* Channel, const BYTE* Buffer, INT Size );
		virtual UBOOL	Init( );
		virtual DWORD	Run( );
		virtual void	Stop( );
		virtual void	Exit( );

	protected:
		FMultiThreadRingBuffer	RingBuffer;
		FCriticalSection*		AccessSync;
		FSocket*		Socket;
		INT				Port;
		UBOOL			bIsTimeToDie;
	};

public:
	FDebugCommunication( INT InBufferSize=DEFAULTBUFFERSIZE );
	~FDebugCommunication();

	UBOOL		Startup();
	void		Shutdown();
	void		SendText( const ANSICHAR* Text );
	void		SendChannelText( const ANSICHAR* Channel, const ANSICHAR* Text );
	void		SendChannelData( const ANSICHAR* Channel, const BYTE* Data, INT Len );
	void		Tick( );
	UBOOL		IsConnected( );

protected:

	// Callbacks from the Receiver and Sender classes.
	FIpAddr		GetClientAddr( );
	FIpAddr		OnAddClient( FIpAddr InClientAddress );
	UBOOL		OnRemoveClient( FIpAddr InClientAddress );
	void		OnReceiveText( ANSICHAR *Message, INT Length );

	FDebugReceiver*		ReceiverRunnable;
	FRunnableThread*	ReceiverThread;
	FDebugSender*		SenderRunnable;
	FRunnableThread*	SenderThread;
	FCriticalSection*	ConnectionSync;		// Sync access to client connection (ClientAddress)
	FCriticalSection*	CommandSync;		// Sync access to the Commands
	FIpAddr				ClientAddress;
	TCHAR				Commands[MAX_COMMANDS][MAX_COMMANDLENGTH];
	INT					NumCommands;
	INT					BufferSize;

	friend class FDebugReceiver;
	friend class FDebugSender;
};

#endif	// DEBUGCOMMUNICATION_HEADER
