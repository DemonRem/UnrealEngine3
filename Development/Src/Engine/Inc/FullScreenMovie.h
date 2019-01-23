/*=============================================================================
	FullscreenMovie.h: Fullscreen movie playback support
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _FULLSCREENMOVIE_H_
#define _FULLSCREENMOVIE_H_

/**
 * Movie loading flags
 */
enum EMovieFlags
{
	// streamed directly from disk
	MF_TypeStreamed  = 0x00,
	// preloaded from disk to memory
	MF_TypePreloaded = 0x01,
	// supplied memory buffer
	MF_TypeSupplied  = 0x02,
	// mask for above types
	MF_TypeMask      = 0x03,

	MF_LoopPlayback  = 0x80,
};	

/**
* Movie playback modes
*/
enum EMovieMode
{
	// stream and loop from disk, minimal memory usage
	MM_LoopFromStream				= MF_TypeStreamed | MF_LoopPlayback,
	// stream from disk, minimal memory usage
	MM_PlayOnceFromStream			= MF_TypeStreamed,
	// load movie into RAM and loop from there
	MM_LoopFromMemory				= MF_TypePreloaded | MF_LoopPlayback,
	// load movie into RAM and play once
	MM_PlayOnceFromMemory			= MF_TypePreloaded,
	// loop from previously loaded buffer
	MM_LoopFromSuppliedMemory		= MF_TypeSupplied | MF_LoopPlayback,
	// play once from previously loaded buffer
	MM_PlayOnceFromSuppliedMemory	= MF_TypeSupplied,

	MM_Uninitialized				= 0xFFFFFFFF,
};

/**
* Abstract base class for full-screen movie player
*/
class FFullScreenMovieSupport : public FTickableObject, public FViewportClient
{
public:

	/**
	* Constructor
	*/
	FFullScreenMovieSupport(UBOOL bIsRenderingThreadObject=TRUE)
		: FTickableObject(bIsRenderingThreadObject)
	{}

	/** 
	* Destructor
	*/
	virtual ~FFullScreenMovieSupport() {};

	// FFullScreenMovieSupport interface

	/**
	* Kick off a movie play from the game thread
	*
	* @param InMovieMode How to play the movie (usually MM_PlayOnceFromStream or MM_LoopFromMemory).
	* @param MovieFilename Path of the movie to play in its entirety
	* @param StartFrame Optional frame number to start on
	*/
	virtual void GameThreadPlayMovie(EMovieMode InMovieMode, const TCHAR* MovieFilename, INT StartFrame=0) = 0;

	/**
	* Stops the currently playing movie
	*
	* @param DelayInSeconds Will delay the stopping of the movie for this many seconds. If zero, this function will wait until the movie stops before returning.
	* @param bWaitForMovie if TRUE then wait until the movie finish event triggers
	* @param bForceStop if TRUE then non-skippable movies and startup movies are forced to stop
	*/
	virtual void GameThreadStopMovie(FLOAT DelayInSeconds=0.0f,UBOOL bWaitForMovie=TRUE,UBOOL bForceStop=FALSE) = 0;

	/**
	* Block game thread until movie is complete (must have been started
	* with GameThreadPlayMovie or it may never return)
	*/
	virtual void GameThreadWaitForMovie() = 0;

	/**
	* Checks to see if the movie has finished playing. Will return immediately
	*
	* @param MovieFilename MovieFilename to check against (should match the name passed to GameThreadPlayMovie). Empty string will match any movie
	* 
	* @return TRUE if the named movie has finished playing
	*/
	virtual UBOOL GameThreadIsMovieFinished(const TCHAR* MovieFilename) = 0;

	/**
	* Checks to see if the movie is playing. Will return immediately
	*
	* @param MovieFilename MovieFilename to check against (should match the name passed to GameThreadPlayMovie). Empty string will match any movie
	* 
	* @return TRUE if the named movie is playing
	*/
	virtual UBOOL GameThreadIsMoviePlaying(const TCHAR* MovieFilename) = 0;

	/**
	* Get the name of the most recent movie played
	*
	* @return Name of the movie that was most recently played, or empty string if a movie hadn't been played
	*/
	virtual FString GameThreadGetLastMovieName() = 0;

	/**
	* Kicks off a thread to control the startup movie sequence
	*/
	virtual void GameThreadInitiateStartupSequence() = 0;

	/**
	* Returns the current frame number of the movie (not thred synchronized in anyway, but it's okay 
	* if it's a little off
	*/
	virtual INT GameThreadGetCurrentFrame() = 0;

	/**
	* Releases any dynamic resources. This is needed for flushing resources during device reset on d3d.
	*/
	virtual void ReleaseDynamicResources() = 0;
};

/*-----------------------------------------------------------------------------
Extern vars
-----------------------------------------------------------------------------*/

/** global for full screen movie player - see appInitFullScreenMoviePlayer */
extern FFullScreenMovieSupport* GFullScreenMovie;

#endif //_FULLSCREENMOVIE_H_
