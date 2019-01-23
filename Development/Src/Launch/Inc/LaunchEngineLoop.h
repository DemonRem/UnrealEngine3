/*=============================================================================
	LaunchEngineLoop.h: Unreal launcher.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __HEADER_LAUNCHENGINELOOP_H
#define __HEADER_LAUNCHENGINELOOP_H

/**
 * This class implementes the main engine loop.	
 */
class FEngineLoop
{
protected:
	/** Dynamically expanding array of frame times in milliseconds (if GIsBenchmarking is set)*/
	TArray<FLOAT>	FrameTimes;
	/** Total of time spent ticking engine.  */
	DOUBLE			TotalTickTime;
	/** Maximum number of seconds engine should be ticked. */
	DOUBLE			MaxTickTime;
	QWORD			MaxFrameCounter;
	DWORD			LastFrameCycles;

	/** A fence count which tracks the rendering thread's progress on previous game thread frames. */
	FRenderCommandFence PendingFrameFence;

	/** The objects which need to be cleaned up when the rendering thread finishes the previous frame. */
	FPendingCleanupObjects* PendingCleanupObjects;

public:
	/**
	* Pre-Initialize the main loop - parse command line, sets up GIsEditor, etc.
	*
	* @param	CmdLine	command line
	* @return	Returns the error level, 0 if successful and > 0 if there were errors
	*/ 
	INT PreInit( const TCHAR* CmdLine );
	/**
	 * Initialize the main loop - the rest of the initialization.
	 *
	 * @return	Returns the error level, 0 if successful and > 0 if there were errors
	 */ 
	INT Init( );
	/** 
	 * Performs shut down 
	 */
	void Exit();
	/**
	 * Advances main loop 
	 */
	void Tick();
};

/**
 * Global engine loop object. This is needed so wxWindows can access it.
 */
extern FEngineLoop GEngineLoop;

#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
