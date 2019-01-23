/*=============================================================================
	RenderingThread.cpp: Rendering thread implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

/** The size of the rendering command buffer, in bytes. */
#define RENDERING_COMMAND_BUFFER_SIZE	(256*1024)

/** comment in this line to display the average amount of data per second processed by the command buffer */
//#define RENDERING_COMMAND_BUFFER_STATS	1

/** The rendering command queue. */
FRingBuffer GRenderCommandBuffer(RENDERING_COMMAND_BUFFER_SIZE, 16);

/**
 * Whether the renderer is running in a separate thread.
 * If this is false, then all rendering commands will be executed immediately instead of being enqueued in the rendering command buffer.
 */
UBOOL GIsThreadedRendering = FALSE;

/**
 * Whether the rendering thread should be created or not.
 * Currently set by command line parameter and by the ToggleRenderingThread console command.
 */
UBOOL GUseThreadedRendering = FALSE;

/** If the rendering thread has been terminated by an unhandled exception, this contains the error message. */
FString GRenderingThreadError;

UBOOL GIsRenderingThreadHealthy = TRUE;

UBOOL GGameThreadWantsToSuspendRendering = FALSE;

/**
 * Tick all rendering thread tickable objects
 */
static void TickRenderingTickables()
{
	static DOUBLE LastTickTime = appSeconds();

	// calc how long has passed since last tick
	DOUBLE CurTime = appSeconds();
	FLOAT DeltaSeconds = CurTime - LastTickTime;

	UINT TickedObjects = 0;

	// tick any rendering thread tickables
	for (INT ObjectIndex = 0; ObjectIndex < FTickableObject::RenderingThreadTickableObjects.Num(); ObjectIndex++)
	{
		FTickableObject* TickableObject = FTickableObject::RenderingThreadTickableObjects(ObjectIndex);
		// make sure it wants to be ticked
		if (TickableObject->IsTickable())
		{
			if (GGameThreadWantsToSuspendRendering)
			{
				RHIResumeRendering();
			}
			TickableObject->Tick(DeltaSeconds);
			TickedObjects++;
		}
	}
	// update the last time we ticked
	LastTickTime = CurTime;

	if (TickedObjects == 0 && GGameThreadWantsToSuspendRendering)
	{
        RHISuspendRendering();
	}
}

/** The rendering thread main loop */
void RenderingThreadMain()
{
#if RENDERING_COMMAND_BUFFER_STATS
	static UINT		RenderingCommandByteCount = 0;
	static DOUBLE	Then = 0.0;
#endif

	volatile void* ReadPointer = NULL;
	UINT NumReadBytes = 0;
	ReadPointer = GRenderCommandBuffer.BeginRead(ReadPointer,NumReadBytes) ? ReadPointer : NULL;

	while(GIsThreadedRendering)
	{
		// Command processing loop.
		while ( ReadPointer )
		{
			SCOPE_CYCLE_COUNTER(STAT_RenderingBusyTime);
			FRenderCommand* Command = (FRenderCommand*)ReadPointer;
			UINT CommandSize = Command->Execute();
			Command->~FRenderCommand();
			GRenderCommandBuffer.FinishRead(CommandSize);
#if RENDERING_COMMAND_BUFFER_STATS
			RenderingCommandByteCount += CommandSize;
#endif
			ReadPointer = GRenderCommandBuffer.BeginRead(ReadPointer,NumReadBytes) ? ReadPointer : NULL;
		}

		// Idle loop:
		{
			SCOPE_CYCLE_COUNTER(STAT_RenderingIdleTime);
			while ( GIsThreadedRendering && !GRenderCommandBuffer.BeginRead(ReadPointer,NumReadBytes) )
			{
				if (GHandleDirtyDiscError)
				{
					appHandleIOFailure( NULL );
				}

#if ALLOW_TRACEDUMP
				// Suspend rendering thread while we're trace dumping to avoid running out of memory due to holding
				// an IRQ for too long.
				extern UBOOL GShouldTraceGameTick;
				while( GShouldTraceGameTick )
				{
					appSleep( 1 );
				}
#endif

				// Yield CPU time if there aren't any rendering commands.
				appSleep(0);

				// tick tickable objects when there are no commands to process, so if there are
				// no commands for a long time, we don't want to starve the tickables
				TickRenderingTickables();

#if RENDERING_COMMAND_BUFFER_STATS
				DOUBLE Now = appSeconds();
				if( Now - Then > 2.0 )
				{
					debugf( TEXT( "RenderCommands: %7d bytes per second" ), UINT( RenderingCommandByteCount / ( Now - Then ) ) );
					Then = Now;
					RenderingCommandByteCount = 0;
				}
#endif
			}
		}
	};

	// Advance and reset the rendering thread stats before returning from the thread.
	// This is necessary to prevent the stats for the last frame of the thread from persisting.
	STAT(GStatManager.AdvanceFrameForThread());
}

/** The rendering thread runnable object. */
class FRenderingThread : public FRunnable
{
public:

	// FRunnable interface.
	virtual UBOOL Init(void) { return TRUE; }
	virtual void Exit(void) {}
	virtual void Stop(void) {}
	virtual DWORD Run(void)
	{
#if _MSC_VER && !XBOX
		extern INT CreateMiniDump( LPEXCEPTION_POINTERS ExceptionInfo );
		if(!appIsDebuggerPresent())
		{
			__try
			{
				RenderingThreadMain();
			}
			__except( CreateMiniDump( GetExceptionInformation() ) )
			{
				GRenderingThreadError = GErrorHist;

				// Use a memory barrier to ensure that the game thread sees the write to GRenderingThreadError before
				// the write to GIsRenderingThreadHealthy.
				appMemoryBarrier();

				GIsRenderingThreadHealthy = FALSE;
			}
		}
		else
#endif
		{
			RenderingThreadMain();
		}

		return 0;
	}
};

/** Thread used for rendering */
FRunnableThread* GRenderingThread = NULL;
FRunnable* GRenderingThreadRunnable = NULL;

void StartRenderingThread()
{
	check(!GIsThreadedRendering && GUseThreadedRendering);

	// Turn on the threaded rendering flag.
	GIsThreadedRendering = TRUE;

	// Create the rendering thread.
	GRenderingThreadRunnable = new FRenderingThread();

	EThreadPriority RenderingThreadPrio = TPri_Normal;
#if PS3
	// below normal, so streaming doesn't get blocked
	RenderingThreadPrio = TPri_BelowNormal;
#endif
	GRenderingThread = GThreadFactory->CreateThread(GRenderingThreadRunnable, TEXT("RenderingThread"), 0, 0, 0, RenderingThreadPrio);

#if XBOX
	if( GIsRHIInitialized )
	{
		// transfer ownership of D3D device to rendering thread
#if USE_XeD3D_RHI
		extern IDirect3DDevice9* GDirect3DDevice;
		// transfer ownership of D3D device to main thread		
		DWORD RenderingThreadID = GRenderingThread->GetThreadID();
		GDirect3DDevice->SetThreadOwnership(RenderingThreadID);        
#endif // USE_XeD3D_RHI
	}
	// Assign the rendering thread to the specified hwthread
	GRenderingThread->SetProcessorAffinity(RENDERING_HWTHREAD);
#endif
	
}

void StopRenderingThread()
{
	if(GIsThreadedRendering)
	{
		// Get the list of objects which need to be cleaned up when the rendering thread is done with them.
		FPendingCleanupObjects* PendingCleanupObjects = GetPendingCleanupObjects();

#if XBOX && USE_XeD3D_RHI
		if( GIsRHIInitialized )
		{
			// transfer ownership of D3D device to main thread
			DWORD GameThreadID = appGetCurrentThreadId();
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
				TransferRenderThreadOwnership,
				DWORD,GameThreadID,GameThreadID,
			{
				// release ownership of D3D device from rendering thread
				extern IDirect3DDevice9* GDirect3DDevice;			
				GDirect3DDevice->SetThreadOwnership(GameThreadID);
			});
		}
#endif
		// Wait for the rendering thread to finish executing all enqueued commands.
		FlushRenderingCommands();

		// Turn off the threaded rendering flag.
		GIsThreadedRendering = FALSE;

		// Wait for the rendering thread to return.
		GRenderingThread->WaitForCompletion();

		// Destroy the rendering thread objects.
		GThreadFactory->Destroy(GRenderingThread);
		GRenderingThread = NULL;
		delete GRenderingThreadRunnable;
		GRenderingThreadRunnable = NULL;

		// Delete the pending cleanup objects which were in use by the rendering thread.
		delete PendingCleanupObjects;
	}
}

void CheckRenderingThreadHealth()
{
	if(!GIsRenderingThreadHealthy)
	{
		GErrorHist[0] = 0;
		GIsCriticalError = FALSE;
		GError->Logf(TEXT("Rendering thread exception:\r\n%s"),*GRenderingThreadError);
	}
}

UBOOL IsInRenderingThread()
{
	return !GRenderingThread || (appGetCurrentThreadId() == GRenderingThread->GetThreadID());
}

UBOOL IsInGameThread()
{
	return !GRenderingThread || (appGetCurrentThreadId() != GRenderingThread->GetThreadID());
}

void FRenderCommandFence::BeginFence()
{
	appInterlockedIncrement((INT*)&NumPendingFences);

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FenceCommand,
		FRenderCommandFence*,Fence,this,
		{
			// Use a memory barrier to ensure that memory writes made by the rendering thread are visible to the game thread before the
			// NumPendingFences decrement.
			appMemoryBarrier();

			appInterlockedDecrement((INT*)&Fence->NumPendingFences);
		});
}

UINT FRenderCommandFence::GetNumPendingFences() const
{
	CheckRenderingThreadHealth();
	return NumPendingFences;
}

/**
 * Waits for pending fence commands to retire.
 * @param NumFencesLeft	- Maximum number of fence commands to leave in queue
 */
void FRenderCommandFence::Wait( UINT NumFencesLeft/*=0*/ ) const
{
	check(IsInGameThread());

	SCOPE_CYCLE_COUNTER(STAT_GameIdleTime);
	while(NumPendingFences > NumFencesLeft)
	{
		// Check that the rendering thread is still running.
		CheckRenderingThreadHealth();

		// Yield CPU time while waiting.
		appSleep(0);
	};
}

/**
 * Waits for all deferred deleted objects to be cleaned up. Should  only be used from the game thread.
 */
void FlushDeferredDeletion()
{
#if XBOX && USE_XeD3D_RHI
	// make sure all textures are freed up after GCing
	FlushRenderingCommands();
	extern void DeleteUnusedXeResources();
	for( INT i=0; i<2; i++ )
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND( DeleteResources, { DeleteUnusedXeResources(); } );
	}
	FlushRenderingCommands();
#elif PS3 && USE_PS3_RHI
	// make sure all textures are freed up after GCing
	FlushRenderingCommands();
	extern void DeleteUnusedPS3Resources();
	for( INT i=0; i<2; i++ )
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND( DeleteResources, { DeleteUnusedPS3Resources(); } );
	}
	FlushRenderingCommands();
#endif
}

/**
 * Waits for the rendering thread to finish executing all pending rendering commands.  Should only be used from the game thread.
 */
void FlushRenderingCommands()
{
	// Find the objects which may be cleaned up once the rendering thread command queue has been flushed.
	FPendingCleanupObjects* PendingCleanupObjects = GetPendingCleanupObjects();

	// Issue a fence command to the rendering thread and wait for it to complete.
	FRenderCommandFence Fence;
	Fence.BeginFence();
	Fence.Wait();

	// Delete the objects which were enqueued for deferred cleanup before the command queue flush.
	delete PendingCleanupObjects;
}

/** The set of deferred cleanup objects which are pending cleanup. */
FPendingCleanupObjects* GPendingCleanupObjects = NULL;

FPendingCleanupObjects::~FPendingCleanupObjects()
{
	for(INT ObjectIndex = 0;ObjectIndex < Num();ObjectIndex++)
	{
		(*this)(ObjectIndex)->FinishCleanup();
	}
}

void BeginCleanup(FDeferredCleanupInterface* CleanupObject)
{
	if(GIsThreadedRendering)
	{
		// If no pending cleanup set exists, create a new one.
		if(!GPendingCleanupObjects)
		{
			GPendingCleanupObjects = new FPendingCleanupObjects();
		}

		// Add the specified object to the pending cleanup set.
		GPendingCleanupObjects->AddItem(CleanupObject);
	}
	else
	{
		CleanupObject->FinishCleanup();
	}
}

FPendingCleanupObjects* GetPendingCleanupObjects()
{
	FPendingCleanupObjects* OldPendingCleanupObjects = GPendingCleanupObjects;
	GPendingCleanupObjects = NULL;

	return OldPendingCleanupObjects;
}
