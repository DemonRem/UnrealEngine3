/**
 * UnThreadingLinux.cpp -- Contains all Linux platform specific definitions
 * of interfaces and concrete classes for multithreading support in the Unreal
 * engine.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#ifdef __GNUC__

#include "CorePrivate.h"
#include "UnThreadingLinux.h"

/** The global synchonization object factory.	*/
FSynchronizeFactory*	GSynchronizeFactory = NULL;
/** The global thread factory.					*/
FThreadFactory*			GThreadFactory		= NULL;

/** Default constructor, initializing Value to 0 */
FThreadSafeCounter::FThreadSafeCounter()
{
	Value = 0;
}

/**
 * Constructor, initializing counter to passed in value.
 *
 * @param InValue	Value to initialize counter to
 */
FThreadSafeCounter::FThreadSafeCounter( LONG InValue )
{
	Value = InValue;
}

/**
 * Increment and return new value.	
 *
 * @return the incremented value
 */
LONG FThreadSafeCounter::Increment()
{
	return ++Value;//@todo gcc
}

/**
 * Decrement and return new value.
 *
 * @return the decremented value
 */
LONG FThreadSafeCounter::Decrement()
{
	return --Value;//@todo gcc
}

/**
 * Return the current value.
 *
 * @return the current value
 */
LONG FThreadSafeCounter::GetValue() const
{
	return Value;
}

/**
 * Constructor that initializes the aggregated critical section
 */
FCriticalSection::FCriticalSection(void)
{
}

/**
 * Locks the critical section
 */
void FCriticalSection::Lock(void)
{
}

/**
 * Releases the lock on the critical seciton
 */
void FCriticalSection::Unlock(void)
{
}

/**
 * Destructor cleaning up the critical section
 */
FCriticalSection::~FCriticalSection(void)
{
}

/**
 * Constructor that zeroes the handle
 */
FEventLinux::FEventLinux(void)
{
}

/**
 * Cleans up the event handle if valid
 */
FEventLinux::~FEventLinux(void)
{
}

/**
 * Waits for the event to be signaled before returning
 */
void FEventLinux::Lock(void)
{
}

/**
 * Triggers the event so any waiting threads are allowed access
 */
void FEventLinux::Unlock(void)
{
}

/**
 * Creates the event. Manually reset events stay triggered until reset.
 * Named events share the same underlying event.
 *
 * @param bIsManualReset Whether the event requires manual reseting or not
 * @param InName Whether to use a commonly shared event or not. If so this
 * is the name of the event to share.
 *
 * @return Returns TRUE if the event was created, FALSE otherwise
 */
UBOOL FEventLinux::Create(UBOOL bIsManualReset,const TCHAR* InName)
{
	return FALSE;
}

/**
 * Triggers the event so any waiting threads are activated
 */
void FEventLinux::Trigger(void)
{
}

/**
 * Resets the event to an untriggered (waitable) state
 */
void FEventLinux::Reset(void)
{
}

/**
 * Triggers the event and resets the triggered state NOTE: This behaves
 * differently for auto-reset versus manual reset events. All threads
 * are released for manual reset events and only one is for auto reset
 */
void FEventLinux::Pulse(void)
{
}

/**
 * Waits for the event to be triggered
 *
 * @param WaitTime Time in milliseconds to wait before abandoning the event
 * (DWORD)-1 is treated as wait infinite
 *
 * @return TRUE if the event was signaled, FALSE if the wait timed out
 */
UBOOL FEventLinux::Wait(DWORD WaitTime)
{
	return FALSE;
}

/**
 * Zeroes its members
 */
FSynchronizeFactoryLinux::FSynchronizeFactoryLinux(void)
{
}

/**
 * Creates a new critical section
 *
 * @return The new critical section object or NULL otherwise
 */
FCriticalSection* FSynchronizeFactoryLinux::CreateCriticalSection(void)
{
	return new FCriticalSection();
}

/**
 * Creates a new event
 *
 * @param bIsManualReset Whether the event requires manual reseting or not
 * @param InName Whether to use a commonly shared event or not. If so this
 * is the name of the event to share.
 *
 * @return Returns the new event object if successful, NULL otherwise
 */
FEvent* FSynchronizeFactoryLinux::CreateSynchEvent(UBOOL bIsManualReset,
	const TCHAR* InName)
{
	return NULL;
}

/**
 * Cleans up the specified synchronization object using the correct heap
 *
 * @param InSynchObj The synchronization object to destroy
 */
void FSynchronizeFactoryLinux::Destroy(FSynchronize* InSynchObj)
{
}

/**
 * Zeros any members
 */
FQueuedThreadLinux::FQueuedThreadLinux(void)
{
	DoWorkEvent			= NULL;
	TimeToDie			= FALSE;
	QueuedWork			= NULL;
	QueuedWorkSynch		= NULL;
	OwningThreadPool	= NULL;
	ThreadID			= 0;
}

/**
 * Deletes any allocated resources. Kills the thread if it is running.
 */
FQueuedThreadLinux::~FQueuedThreadLinux(void)
{
}

/**
 * The real thread entry point. It waits for work events to be queued. Once
 * an event is queued, it executes it and goes back to waiting.
 */
void FQueuedThreadLinux::Run(void)
{
}

/**
 * Creates the thread with the specified stack size and creates the various
 * events to be able to communicate with it.
 *
 * @param InPool The thread pool interface used to place this thread
 * back into the pool of available threads when its work is done
 * @param InStackSize The size of the stack to create. 0 means use the
 * current thread's stack size
 *
 * @return True if the thread and all of its initialization was successful, false otherwise
 */
UBOOL FQueuedThreadLinux::Create(FQueuedThreadPool* InPool,DWORD InStackSize)
{
	return FALSE;
}

/**
 * Tells the thread to exit. If the caller needs to know when the thread
 * has exited, it should use the bShouldWait value and tell it how long
 * to wait before deciding that it is deadlocked and needs to be destroyed.
 * NOTE: having a thread forcibly destroyed can cause leaks in TLS, etc.
 *
 * @param bShouldWait If true, the call will wait for the thread to exit
 * @param MaxWaitTime The amount of time to wait before killing it. It
 * defaults to inifinite.
 * @param bShouldDeleteSelf Whether to delete ourselves upon completion
 *
 * @return True if the thread exited gracefully, false otherwise
 */
UBOOL FQueuedThreadLinux::Kill(UBOOL bShouldWait,DWORD MaxWaitTime,UBOOL bShouldDeleteSelf)
{
	return FALSE;
}

/**
 * Tells the thread there is work to be done. Upon completion, the thread
 * is responsible for adding itself back into the available pool.
 *
 * @param InQueuedWork The queued work to perform
 */
void FQueuedThreadLinux::DoWork(FQueuedWork* InQueuedWork)
{
}

/**
 * Cleans up any threads that were allocated in the pool
 */
FQueuedThreadPoolLinux::~FQueuedThreadPoolLinux(void)
{
}

/**
 * Creates the thread pool with the specified number of threads
 *
 * @param InNumQueuedThreads Specifies the number of threads to use in the pool
 * @param StackSize The size of stack the threads in the pool need (32K default)
 *
 * @return Whether the pool creation was successful or not
 */
UBOOL FQueuedThreadPoolLinux::Create(DWORD InNumQueuedThreads,DWORD StackSize)
{
	return FALSE;
}

/**
 * Zeroes members
 */
FRunnableThreadLinux::FRunnableThreadLinux(void)
{
}

/**
 * Cleans up any resources
 */
FRunnableThreadLinux::~FRunnableThreadLinux(void)
{
}

/**
 * Creates the thread with the specified stack size and thread priority.
 *
 * @param InRunnable The runnable object to execute
 * @param bAutoDeleteSelf Whether to delete this object on exit
 * @param bAutoDeleteRunnable Whether to delete the runnable object on exit
 * @param InStackSize The size of the stack to create. 0 means use the
 * current thread's stack size
 * @param InThreadPri Tells the thread whether it needs to adjust its
 * priority or not. Defaults to normal priority
 *
 * @return True if the thread and all of its initialization was successful, false otherwise
 */
UBOOL FRunnableThreadLinux::Create(FRunnable* InRunnable,
	UBOOL bAutoDeleteSelf,UBOOL bAutoDeleteRunnable,DWORD InStackSize,
	EThreadPriority InThreadPri)
{
	return FALSE;
}

/**
 * Tells the thread to either pause execution or resume depending on the
 * passed in value.
 *
 * @param bShouldPause Whether to pause the thread (true) or resume (false)
 */
void FRunnableThreadLinux::Suspend(UBOOL bShouldPause)
{
}

/**
 * Tells the thread to exit. If the caller needs to know when the thread
 * has exited, it should use the bShouldWait value and tell it how long
 * to wait before deciding that it is deadlocked and needs to be destroyed.
 * NOTE: having a thread forcibly destroyed can cause leaks in TLS, etc.
 *
 * @param bShouldWait If true, the call will wait for the thread to exit
 * @param MaxWaitTime The amount of time to wait before killing it.
 * Defaults to inifinite.
 *
 * @return True if the thread exited gracefull, false otherwise
 */
UBOOL FRunnableThreadLinux::Kill(UBOOL bShouldWait,DWORD MaxWaitTime)
{
	return FALSE;
}

/**
 * The real thread entry point. It calls the Init/Run/Exit methods on
 * the runnable object
 *
 * @return The exit code of the thread
 */
DWORD FRunnableThreadLinux::Run(void)
{
	return 0;
}

/**
 * Changes the thread priority of the currently running thread
 *
 * @param NewPriority The thread priority to change to
 */
void FRunnableThreadLinux::SetThreadPriority(EThreadPriority NewPriority)
{
}

/**
 * Tells the OS the preferred CPU to run the thread on.
 *
 * @param ProcessorNum The preferred processor for executing the thread on
 */
void FRunnableThreadLinux::SetProcessorAffinity(DWORD ProcessorNum)
{
}

/**
 * Halts the caller until this thread is has completed its work.
 */
void FRunnableThreadLinux::WaitForCompletion(void)
{
}

/**
 * Creates the thread with the specified stack size and thread priority.
 *
 * @param InRunnable The runnable object to execute
 * @param bAutoDeleteSelf Whether to delete this object on exit
 * @param bAutoDeleteRunnable Whether to delete the runnable object on exit
 * @param InStackSize The size of the stack to create. 0 means use the
 * current thread's stack size
 * @param InThreadPri Tells the thread whether it needs to adjust its
 * priority or not. Defaults to normal priority
 *
 * @return The newly created thread or NULL if it failed
 */
FRunnableThread* FThreadFactoryLinux::CreateThread(FRunnable* InRunnable,
	UBOOL bAutoDeleteSelf,UBOOL bAutoDeleteRunnable,DWORD InStackSize,
	EThreadPriority InThreadPri)
{
	return NULL;
}

/**
 * Cleans up the specified thread object using the correct heap
 *
 * @param InThread The thread object to destroy
 */
void FThreadFactoryLinux::Destroy(FRunnableThread* InThread)
{
}

#endif
