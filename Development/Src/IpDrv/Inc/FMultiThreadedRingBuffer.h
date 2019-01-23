/*=============================================================================
	FMultiThreadedRingBuffer.h:	Ring buffer ready to use in a multi-threaded environment.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __MULTI_THREADED_RING_BUFFER_HEADER__
#define __MULTI_THREADED_RING_BUFFER_HEADER__

#include "UnIpDrv.h"

/**
 * Used for passing buffered data between one sender thread and one
 * receiver thread. Secure wrt. multi-threaded access.
 */
class FMultiThreadedRingBuffer
{
public:

	/// Data packet representation
	struct BufferData
	{
		BYTE* Buffer; //!< Data pointer
		INT Size; //!< Data size
	};

	FMultiThreadedRingBuffer( INT InBufferSize, INT InMaxPacketSize );
	~FMultiThreadedRingBuffer();

	UBOOL				BeginPush( BufferData &Data, INT Size );
	void				EndPush( BufferData &Data, UBOOL bForcePush=FALSE );
	UBOOL				Peek( BufferData &Entry  );
	void				Pop( BufferData &Entry );
	void				WaitForData( DWORD MilliSeconds = INFINITE );
	void				KickBuffer( );

protected:
	void				ReadEntry(BufferData &Entry);

	INT					MaxPacketSize;		// Maximal allowed packet size
	INT					NumPackets;			// No. packets in buffer
	BYTE*				RingBuffer;			// The actual data buffer
	INT					BufferSize;			// Buffer size
	INT					ReadIndex;			// Current reading index
	INT					WriteIndex;			// Current writing index
	FEvent*				WorkEvent;			// An event used to wake up 'data retriever'
	FCriticalSection*	BufferMutex;		// Buffer synchronization
};

#endif	// __MULTI_THREADED_RING_BUFFER_HEADER__
