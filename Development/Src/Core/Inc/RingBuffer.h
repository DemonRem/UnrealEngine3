/*=============================================================================
	RingBuffer.h: Ring buffer definition.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * A ring buffer for use with two threads: a reading thread and a writing thread.
 */
class FRingBuffer
{
public:

	/**
	 * Minimal initialization constructor.
	 * @param BufferSize - The size of the data buffer to allocate.
	 * @param InAlingment - Alignment of each allocation unit (in bytes)
	 */
	FRingBuffer(UINT BufferSize, INT InAlignment = 1)
	{
		Data = new BYTE[BufferSize];
		ReadPointer = WritePointer = Data;
		EndPointer = DataEnd = Data + BufferSize;
		Alignment = InAlignment;
	}

	/**
	 * A reference to an allocated chunk of the ring buffer.
	 * Upon destruction of the context, the chunk is committed as written.
	 */
	class AllocationContext
	{
	public:

		/**
		 * Upon construction, AllocationContext allocates a chunk from the ring buffer.
		 * @param InRingBuffer - The ring buffer to allocate from.
		 * @param InAllocationSize - The size of the allocation to make.
		 */
		AllocationContext(FRingBuffer& InRingBuffer,UINT InAllocationSize):
			RingBuffer(InRingBuffer),
			AllocationSize(Align(InAllocationSize,InRingBuffer.Alignment))
		{
			// Only allow a single AllocationContext at a time for the ring buffer.
			check(!RingBuffer.bIsWriting);
			RingBuffer.bIsWriting = TRUE;

			// Check that the allocation will fit in the buffer.
			const UINT BufferSize = (UINT)(RingBuffer.DataEnd - InRingBuffer.Data);
			check(AllocationSize < BufferSize);

			while(1)
			{
				// Capture the current state of ReadPointer.
				BYTE* CurrentReadPointer = RingBuffer.ReadPointer;

				if(CurrentReadPointer > RingBuffer.WritePointer && RingBuffer.WritePointer + AllocationSize >= CurrentReadPointer)
				{
					// If the allocation won't fit in the buffer without overwriting yet-to-be-read data,
					// wait for the reader thread to catch up.
					continue;
				}

				if(RingBuffer.WritePointer + AllocationSize > RingBuffer.DataEnd)
				{
					if(CurrentReadPointer == RingBuffer.Data)
					{
						// Since ReadPointer == Data, don't set WritePointer=Data until ReadPointer>Data.  WritePointer==ReadPointer means
						// the buffer is empty.
						continue;
					}
					// If the allocation won't fit before the end of the buffer, move WritePointer to the beginning of the buffer.
					// Also set EndPointer so the reading thread knows the last byte of the buffer which has valid data.
					RingBuffer.EndPointer = RingBuffer.WritePointer;
					RingBuffer.WritePointer = RingBuffer.Data;
					continue;
				}

				// Use the memory referenced by WritePointer for the allocation.
				Allocation = RingBuffer.WritePointer;
				break;
			};
		}
		/**
		 * Commits the allocated chunk of memory to the ring buffer.
		 */
		void Commit()
		{
			if(Allocation)
			{
				// Use a memory barrier to ensure that data written to the ring buffer is visible to the reading thread before the WritePointer
				// update.
				appMemoryBarrier();

				RingBuffer.WritePointer += AllocationSize;

				// Reset the bIsWriting flag to allow other AllocationContexts to be created for the ring buffer.
				RingBuffer.bIsWriting = FALSE;

				// Clear the allocation pointer, to signal that it has been committed.
				Allocation = NULL;
			}
		}
		/**
		 * Upon destruction, the allocation is committed, if Commit hasn't been called manually.
		 */
		~AllocationContext()
		{
			Commit();
		}

		// Accessors.
		void* GetAllocation() const { return Allocation; }

	private:
		FRingBuffer& RingBuffer;
		void* Allocation;
		const UINT AllocationSize;
	};

	/**
	 * Checks if there is data to be read from the ring buffer, and if so accesses the pointer to the data to be read.
	 * @param OutReadPointer - When returning TRUE, this will hold the pointer to the data to read.
	 * @param MaxReadSize - When returning TRUE, this will hold the number of bytes available to read.
	 * @return TRUE if there is data to be read.
	 */
	UBOOL BeginRead(volatile void*& OutReadPointer,UINT& OutReadSize)
	{
		BYTE* CurrentWritePointer = WritePointer;
		BYTE* CurrentEndPointer = EndPointer;

		// If the read pointer has reached the end of readable data in the buffer, reset it to the beginning of the buffer.
		if(CurrentWritePointer <= CurrentEndPointer && ReadPointer == CurrentEndPointer)
		{
			if(CurrentWritePointer < CurrentEndPointer)
			{
				// Only wrap the ReadPointer to the beginning of the buffer once the WritePointer has been wrapped.
				// If it's wrapped as soon as it reaches EndPointer, then it might be wrapped before the WritePointer.
				ReadPointer = Data;
			}
			else
			{
				return FALSE;
			}
		}

		if(ReadPointer != CurrentWritePointer)
		{
			OutReadPointer = ReadPointer;
			if(CurrentWritePointer < ReadPointer)
			{
				// If WritePointer has been reset to the beginning of the buffer more recently than ReadPointer,
				// the buffer is readable from ReadPointer to EndPointer.
				OutReadSize = (UINT)(CurrentEndPointer - ReadPointer);
			}
			else
			{
				// Otherwise, the buffer is readable from ReadPointer to WritePointer.
				OutReadSize = (UINT)(CurrentWritePointer - ReadPointer);
			}
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	/**
	 * Frees the first ReadSize bytes available for reading via BeginRead to the writing thread.
	 * @param ReadSize - The number of bytes to free.
	 */
	void FinishRead(UINT ReadSize)
	{
		ReadPointer += Align( ReadSize, Alignment );
	}

private:

	/** The data buffer. */
	BYTE* Data;

	/** The first byte after end the of the data buffer. */
	BYTE* DataEnd;

	/** The next byte to be written to. */
	BYTE* volatile WritePointer;

	/** If WritePointer < ReadPointer, EndPointer is the first byte after the end of readable data in the buffer. */
	BYTE* volatile EndPointer;

	/** TRUE if there is an AllocationContext outstanding for this ring buffer. */
	UBOOL bIsWriting;

	/** The next byte to be read from. */
	BYTE* volatile ReadPointer;

	/** Alignment of each allocation unit (in bytes). */
	INT Alignment;
};
