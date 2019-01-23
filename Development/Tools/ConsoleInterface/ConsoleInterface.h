// ConsoleInterface.h

#pragma once

#define MANAGED_CPP_MAGIC __nogc

#include "..\..\Src\Engine\Inc\UnConsoleTools.h"

using namespace System;
using namespace System::Collections;

namespace ConsoleInterface
{

/** Contains per-platform DLL data */
public struct FCachedConsoleData
{
	FCachedConsoleData()
		: DLLHandle(NULL)
		, ConsoleSupport(NULL)
	{
	}
	HMODULE DLLHandle;
	FConsoleSupport* ConsoleSupport;
};
	
public __gc class DLLInterface
{

public:
	/**
	 * Constructor
	 */
	DLLInterface();

	/**
	 * Destructor
	 */
	~DLLInterface();

	/**
	 * @return the number of known platforms supported by this DLL
	 */
	int GetNumPlatforms();

	/**
	 * @ return The string name of the requested platform
	 */
	String* GetPlatformName(int Index);

	/**
	 * @return the index that corresponds to the given platform name
	 */
	int GetPlatformIndex(String* GetPlatformName);

	/**
	 * Enables a platform. Previously activated platform will be disabled.
	 *
	 * @param PlatformName Hardcoded platform name (PS3, Xenon, etc)
	 *
	 * @return true if successful
	 */
	bool ActivatePlatform(String* PlatformName);

	/**
	 * Convert a platform name into one of the known platform names, or returns what
	 * was specified if unknown (also handles prettifying PC)
	 * @param PlatformName Input platform name
	 * @return Known standard platform name
	 */
	String* GetFriendlyPlatformName(String* PlatformName);



	/////////////////////////////////////
	// MAIN INTERFACE
	/////////////////////////////////////
	/**
	 * @return the number of known xbox targets
	 */
	int GetNumTargets();

	/**
	 * Get the name of the specified target
	 * @param Index Index of the targt to query
	 * @return Name of the target, or NULL if the Index is out of bounds
	 */
	String* GetTargetName(int Index);

	/**
	 * Gets the Game IP address (not debug IP) for the given target
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 *
	 * @return IP address as an unsigned int
	 */
	unsigned int GetGameIPAddress(int TargetIndex);

	/**
	 * @return true if this platform needs to have files copied from PC->target (required to implement)
	 */
	bool PlatformNeedsToCopyFiles();

	/**
	 * Open an internal connection to a target. This is used so that each operation is "atomic" in 
	 * that connection failures can quickly be 'remembered' for one "sync", etc operation, so
	 * that a connection is attempted for each file in the sync operation...
	 * For the next operation, the connection can try to be opened again.
	 *
	 * @param TargetName Name of target
	 *
	 * @return -1 for failure, or the index used in the other functions (MakeDirectory, etc)
	 */
	int ConnectToTarget(String* TargetName);

	/**
	 * Called after an atomic operation to close any open connections
	 */
	void DisconnectFromTarget(int TargetIndex);

	/**
	 * Creates a directory
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param SourceFilename Platform-independent directory name (ie UnrealEngine3\Binaries)
	 *
	 * @return true if successful
	 */
	bool MakeDirectory(int TargetIndex, String* DirectoryName);

	/**
	 * Determines if the given file needs to be copied
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param SourceFilename Path of the source file on PC
	 * @param DestFilename Platform-independent destination filename (ie, no xe:\\ for Xbox, etc)
	 *
	 * @return true if successful
	 */
	bool NeedsToSendFile(int TargetIndex, String* SourceFilename, String* DestFilename);

	/**
	 * Copies a single file from PC to target
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param SourceFilename Path of the source file on PC
	 * @param DestFilename Platform-independent destination filename (ie, no xe:\\ for Xbox, etc)
	 *
	 * @return true if successful
	 */
	bool SendFile(int TargetIndex, String* SourceFilename, String* DestFilename);

	/**
	 * Sets the name of the layout file for the DVD, so that GetDVDFileStartSector can be used
	 * 
	 * @param DVDLayoutFile Name of the layout file
	 *
	 * @return true if successful
	 */
	bool SetDVDLayoutFile(String* DVDLayoutFile);

	/**
	 * Gets the starting sector of a file on the DVD (or whatever optical medium)
	 *
	 * @param DVDFilename Path to the file on the DVD
	 * @param SectorHigh High 32 bits of the sector location
	 * @param SectorLow Low 32 bits of the sector location
	 * 
	 * @return true if the start sector was found for the file
	 */
	bool GetDVDFileStartSector(String* DVDFilename, unsigned int& SectorHigh, unsigned int& SectorLow);

	/**
	 * Reboots the target console. Must be implemented
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * 
	 * @return true if successful
	 */
	bool Reboot(int TargetIndex);

	/**
	 * Reboots the target console. Must be implemented
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param Configuration Build type to run (Debug, Release, RelaseLTCG, etc)
	 * @param BaseDirectory Location of the build on the console (can be empty for platforms that don't copy to the console)
	 * @param GameName Name of the game to run (Example, UT, etc)
	 * @param URL Optional URL to pass to the executable
	 * 
	 * @return true if successful
	 */
	bool RebootAndRun(int TargetIndex, String* Configuration, String* BaseDirectory, String* GameName, String* URL);

	/**
	 * Retrieve the state of the console (running, not running, crashed, etc)
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 *
	 * @return the current state
	 */
	FConsoleSupport::EConsoleState GetConsoleState(int TargetIndex);

	/**
	 * Get the callstack for a crashed target
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param Callstack Array of output addresses
	 *
	 * @return true if succesful
	 */
	bool GetCallstack(int TargetIndex, IList* Callstack);

	/**
	 * Loads the debug symbols for the an executable
	 *
	 * @param ExecutableName Name of the executable file, or NULL to attempt to use the currently running executable's symbols
	 *
	 * @return true if successful
	 */
	bool LoadSymbols(int TargetIndex, String* ExecutableName);

	/**
	* Unloads any loaded debug symbols
	*/
	void UnloadAllSymbols();

	/**
	 * Turn an address into a symbol for callstack dumping
	 * 
	 * @param Address Code address to be looked up
	 * 
	 * @return Function name for the given address
	 */
	String* ResolveAddressToString(UInt32 Address);


	/**
	 * Send a text command to the target
	 * 
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param Command Command to send to the target
	 */
	void SendConsoleCommand(int TargetIndex, String* Command);


	/**
	 * Receive any new text output by the target
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * 
	 * @return Output text from the target 
	 */
	String* ReceiveOutputText(int TargetIndex);

	/**
	 * Have the console take a screenshot and dump to a file
	 * 
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param Filename Location to place the .bmp file
	 *
	 * @return true if successful
	 */
	bool ScreenshotBMP(int TargetIndex, String* Filename);


private:
	FConsoleSupport* CurrentConsoleSupport;

	/** Array of cached dll specific objects */
	FCachedConsoleData* CachedConsoleDLLData;
};


}
