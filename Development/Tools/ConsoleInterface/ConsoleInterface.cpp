// This is the main DLL file.

#include "stdafx.h"
#include <windows.h>
#include <assert.h>
#include "ConsoleInterface.h"

using namespace ConsoleInterface;
using namespace System::Runtime::InteropServices;

#define BYTE_SWAP_32(x) \
	(((x) & 0xFF000000) >> 24) | \
	(((x) & 0x00FF0000) >> 8) | \
	(((x) & 0x0000FF00) << 8) | \
	(((x) & 0x000000FF) << 24)


struct FPlatformInfo
{
	/** Name of the platform */
	char* Name;

	/** Alternate name of the platform */
	char* AltName;

	/** Path to the DLL */
	char* DLL;
};

FPlatformInfo KnownPlatforms[] = 
{ 
	{ "PS3", "PlayStation3", "PS3\\PS3Tools.dll" } ,
	{ "Xbox360", "Xenon", "Xenon\\XeTools.dll" } ,
};

DLLInterface::DLLInterface()
{
	CurrentConsoleSupport = NULL;

	// allocate space to cache data
	CachedConsoleDLLData = new FCachedConsoleData[GetNumPlatforms()];
}

DLLInterface::~DLLInterface()
{
	// shutdown any loaded libraries
	for (int PlatformIndex = 0; PlatformIndex < GetNumPlatforms(); PlatformIndex++)
	{
		if (CachedConsoleDLLData[PlatformIndex].DLLHandle)
		{
			FreeLibrary(CachedConsoleDLLData[PlatformIndex].DLLHandle);
		}
	}
}

/**
 * @return the number of known platforms supported by this DLL
 */
int DLLInterface::GetNumPlatforms()
{
	return sizeof(KnownPlatforms) / sizeof(KnownPlatforms[0]);
}

/**
 * @ return The string name of the requested platform
 */
String* DLLInterface::GetPlatformName(int Index)
{
	assert(Index < GetNumPlatforms());
	return KnownPlatforms[Index].Name;
}

/**
 * @return the index that corresponds to the given platform name
 */
int DLLInterface::GetPlatformIndex(String* PlatformName)
{
	int RetVal = -1;
	// go through each platform
	for (int PlatformIndex = 0; PlatformIndex < GetNumPlatforms(); PlatformIndex++)
	{
		// try to match names
		if (String::Compare(PlatformName, KnownPlatforms[PlatformIndex].Name, true) == 0 ||
			String::Compare(PlatformName, KnownPlatforms[PlatformIndex].AltName, true) == 0)
		{
			// remember index if we matched
			RetVal = PlatformIndex;
			break;
		}
	}

	return RetVal;
}

/**
 * Convert a platform name into one of the known platform names, or returns what
 * was specified if unknown (also handles prettifying PC)
 * @param PlatformName Input platform name
 * @return Known standard platform name
 */
String* DLLInterface::GetFriendlyPlatformName(String* PlatformName)
{
	int PlatformIndex = GetPlatformIndex(PlatformName);
	// handle unknown platform string
	if (PlatformIndex == -1)
	{
		// handle special PC case
		if (String::Compare(PlatformName, "PC", true) == 0 ||
			String::Compare(PlatformName, "Windows", true) == 0)
		{
			return S"PC";
		}

		// if not PC, then return what was passed in
		return PlatformName;
	}

	// otherwise, return the name from the KnownPlatforms array
	return GetPlatformName(PlatformIndex);
}

/**
 * Enables a platform. Previously activated platform will be disabled.
 *
 * @param PlatformName Hardcoded platform name (PS3, Xenon, etc)
 *
 * @return true if successful
 */
bool DLLInterface::ActivatePlatform(String* PlatformName)
{
	// get the index for the platform name
	int PlatformIndex = GetPlatformIndex(PlatformName);

	// if its not a valid platform, error out
	if (PlatformIndex == -1)
	{
		return false;
	}

	// get the cached data object
	FCachedConsoleData& CachedData = CachedConsoleDLLData[PlatformIndex];

	// have we already created and cached the console support object?
	if (CachedData.ConsoleSupport)
	{
		CurrentConsoleSupport = CachedData.ConsoleSupport;
	}
	else
	{
		// have we already loaded the DLL?
		if (CachedData.DLLHandle == NULL)
		{
			// if not, load it
			CachedData.DLLHandle = LoadLibraryA(KnownPlatforms[PlatformIndex].DLL);
		}

		// make sure the DLL was loaded
		if (CachedData.DLLHandle == NULL)
		{
			return false;
		}

		// get the function pointer to the accessor
		FuncGetConsoleSupport GetSupportFunc = (FuncGetConsoleSupport)GetProcAddress(CachedData.DLLHandle, "GetConsoleSupport");

		// get console support, and set it to the current one
		CurrentConsoleSupport = CachedData.ConsoleSupport = GetSupportFunc();
	}

	// final validation
	return CurrentConsoleSupport != NULL;
}


/**
 * @return the number of known xbox targets
 */
int DLLInterface::GetNumTargets()
{
	return CurrentConsoleSupport->GetNumTargets();
}

/**
 * Get the name of the specified target
 * @param Index Index of the targt to query
 * @return Name of the target, or NULL if the Index is out of bounds
 */
String* DLLInterface::GetTargetName(int Index)
{
	return CurrentConsoleSupport->GetTargetName(Index);
}

/**
 * Gets the Game IP address (not debug IP) for the given target
 *
 * @param TargetIndex Index into the Targets array of which target to use 
 *
 * @return IP address as an unsigned int
 */
unsigned int DLLInterface::GetGameIPAddress(int TargetIndex)
{
	unsigned int IPAddr = CurrentConsoleSupport->GetIPAddress(TargetIndex);
	// byte swap if needed
	if (!CurrentConsoleSupport->GetIntelByteOrder())
	{
		IPAddr = BYTE_SWAP_32(IPAddr);
	}

	return IPAddr;
}

/**
 * @return true if this platform needs to have files copied from PC->target (required to implement)
 */
bool DLLInterface::PlatformNeedsToCopyFiles()
{
	return CurrentConsoleSupport->PlatformNeedsToCopyFiles();
}

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
int DLLInterface::ConnectToTarget(String* TargetName)
{
	// copy over the string to non-gc memory
	IntPtr BStr = Marshal::StringToBSTR(TargetName);

	int Ret = CurrentConsoleSupport->ConnectToTarget((wchar_t*)BStr.ToPointer());

	// free temp memory
	Marshal::FreeBSTR(BStr);

	return Ret;
}

/**
 * Called after an atomic operation to close any open connections
 */
void DLLInterface::DisconnectFromTarget(int TargetIndex)
{
	if (CurrentConsoleSupport)
	{
		CurrentConsoleSupport->DisconnectFromTarget(TargetIndex);
	}
}

/**
 * Creates a directory
 *
 * @param TargetIndex Index into the Targets array of which target to use
 * @param SourceFilename Platform-independent directory name (ie UnrealEngine3\Binaries)
 *
 * @return true if successful
 */
bool DLLInterface::MakeDirectory(int TargetIndex, String* DirectoryName)
{
	// copy over the string to non-gc memory
	IntPtr BStr = Marshal::StringToBSTR(DirectoryName);

	// do the command
	bool Ret = CurrentConsoleSupport->MakeDirectory(TargetIndex, (wchar_t*)BStr.ToPointer());

	// free temp memory
	Marshal::FreeBSTR(BStr);

	// return the result
	return Ret;
}

/**
 * Determines if the given file needs to be copied
 *
 * @param TargetIndex Index into the Targets array of which target to use
 * @param SourceFilename Path of the source file on PC
 * @param DestFilename Platform-independent destination filename (ie, no xe:\\ for Xbox, etc)
 *
 * @return true if successful
 */
bool DLLInterface::NeedsToSendFile(int TargetIndex, String* SourceFilename, String* DestFilename)
{
	// copy over the strings to non-gc memory
	IntPtr BStr1 = Marshal::StringToBSTR(SourceFilename);
	IntPtr BStr2 = Marshal::StringToBSTR(DestFilename);

	bool Ret = CurrentConsoleSupport->NeedsToCopyFile(TargetIndex, (wchar_t*)BStr1.ToPointer(), (wchar_t*)BStr2.ToPointer());
	
	// free temp memory
	Marshal::FreeBSTR(BStr2);
	Marshal::FreeBSTR(BStr1);

	// return the result
	return Ret;
}

/**
 * Copies a single file from PC to target
 *
 * @param TargetIndex Index into the Targets array of which target to use
 * @param SourceFilename Path of the source file on PC
 * @param DestFilename Platform-independent destination filename (ie, no xe:\\ for Xbox, etc)
 *
 * @return true if successful
 */
bool DLLInterface::SendFile(int TargetIndex, String* SourceFilename, String* DestFilename)
{
	// copy over the strings to non-gc memory
	IntPtr BStr1 = Marshal::StringToBSTR(SourceFilename);
	IntPtr BStr2 = Marshal::StringToBSTR(DestFilename);

	bool Ret = CurrentConsoleSupport->CopyFile(TargetIndex, (wchar_t*)BStr1.ToPointer(), (wchar_t*)BStr2.ToPointer());
	
	// free temp memory
	Marshal::FreeBSTR(BStr2);
	Marshal::FreeBSTR(BStr1);

	// return the result
	return Ret;
}

/**
 * Sets the name of the layout file for the DVD, so that GetDVDFileStartSector can be used
 * 
 * @param DVDLayoutFile Name of the layout file
 *
 * @return true if successful
 */
bool DLLInterface::SetDVDLayoutFile(String* DVDLayoutFile)
{
	// copy over the strings to non-gc memory
	IntPtr BStr1 = Marshal::StringToBSTR(DVDLayoutFile);

	bool Ret = CurrentConsoleSupport->SetDVDLayoutFile((wchar_t*)BStr1.ToPointer());

	// free temp memory
	Marshal::FreeBSTR(BStr1);

	// return the result
	return Ret;
}

/**
 * Gets the starting sector of a file on the DVD (or whatever optical medium)
 *
 * @param DVDFilename Path to the file on the DVD
 * @param SectorHigh High 32 bits of the sector location
 * @param SectorLow Low 32 bits of the sector location
 * 
 * @return true if the start sector was found for the file
 */
bool DLLInterface::GetDVDFileStartSector(String* DVDFilename, unsigned int& SectorHigh, unsigned int& SectorLow)
{
	// copy over the strings to non-gc memory
	IntPtr BStr1 = Marshal::StringToBSTR(DVDFilename);

	bool Ret = CurrentConsoleSupport->GetDVDFileStartSector((wchar_t*)BStr1.ToPointer(), SectorHigh, SectorLow);

	// free temp memory
	Marshal::FreeBSTR(BStr1);

	// return the result
	return Ret;
}

/**
 * Reboots the target console. Must be implemented
 *
 * @param TargetIndex Index into the Targets array of which target to use
 * 
 * @return true if successful
 */
bool DLLInterface::Reboot(int TargetIndex)
{
	// reboot the console
	return CurrentConsoleSupport->Reboot(TargetIndex);
}

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
bool DLLInterface::RebootAndRun(int TargetIndex, String* Configuration, String* BaseDirectory, String* GameName, String* URL)
{
	// copy over the strings to non-gc memory
	IntPtr BStr1 = Marshal::StringToBSTR(Configuration);
	IntPtr BStr2 = Marshal::StringToBSTR(BaseDirectory);
	IntPtr BStr3 = Marshal::StringToBSTR(GameName);
	IntPtr BStr4 = Marshal::StringToBSTR(URL);

	// run the game
	bool Ret = CurrentConsoleSupport->RebootAndRun(TargetIndex, (wchar_t*)BStr1.ToPointer(), (wchar_t*)BStr2.ToPointer(), (wchar_t*)BStr3.ToPointer(), (wchar_t*)BStr4.ToPointer());
	
	// free temp memory
	Marshal::FreeBSTR(BStr4);
	Marshal::FreeBSTR(BStr3);
	Marshal::FreeBSTR(BStr2);
	Marshal::FreeBSTR(BStr1);

	// return the result
	return Ret;
}

/**
  * Retrieve the state of the console (running, not running, crashed, etc)
  *
  * @param TargetIndex Index into the Targets array of which target to use
  *
  * @return the current state
  */
FConsoleSupport::EConsoleState DLLInterface::GetConsoleState(int TargetIndex)
{
	return (FConsoleSupport::EConsoleState)CurrentConsoleSupport->GetConsoleState(TargetIndex);
}

/**
 * Get the callstack for a crashed target
 *
 * @param TargetIndex Index into the Targets array of which target to use
 * @param Callstack Array of output addresses
 *
 * @return true if succesful
 */
bool DLLInterface::GetCallstack(int TargetIndex, IList* Callstack)
{
#define CALLSTACK_SIZE 20
	unsigned int Buffer[CALLSTACK_SIZE];
	
	if (!CurrentConsoleSupport->GetCallstack(TargetIndex, Buffer, CALLSTACK_SIZE))
	{
		return false;
	}

	// add the ints to the C# collection
	for (int Depth = 0; Depth < CALLSTACK_SIZE && Buffer[Depth]; Depth++)
	{
		System::Object* Obj = __box(Buffer[Depth]);
		Callstack->Add(Obj);
	}

	return true;
}


/**
 * Loads the debug symbols for the an executable
 *
 * @param ExecutableName Name of the executable file, or NULL to attempt to use the currently running executable's symbols
 *
 * @return true if successful
 */
bool DLLInterface::LoadSymbols(int TargetIndex, String* ExecutableName)
{
	IntPtr BStr1 = Marshal::StringToBSTR(ExecutableName);

	// load symbols
	bool Ret = CurrentConsoleSupport->LoadSymbols(TargetIndex, (wchar_t*)BStr1.ToPointer());

	Marshal::FreeBSTR(BStr1);

	return Ret;
}

/**
 * Unloads any loaded debug symbols
 */
void DLLInterface::UnloadAllSymbols()
{
	CurrentConsoleSupport->UnloadAllSymbols();
}

/**
 * Turn an address into a symbol for callstack dumping
 * 
 * @param Address Code address to be looked up
 * 
 * @return Function name for the given address
 */
String* DLLInterface::ResolveAddressToString(UInt32 Address)
{
#define BUFFER_SIZE 2048
	// make space for output string
	wchar_t OutBuffer[BUFFER_SIZE + 1];
	// null terminate it
	OutBuffer[BUFFER_SIZE] = 0;

	// ask target for any text
	CurrentConsoleSupport->ResolveAddressToString(Address, OutBuffer, BUFFER_SIZE);

	// return it
	if (OutBuffer[0] == 0)
	{
		return S"<Unknown address>";
	}

	return OutBuffer;
}


/**
 * Send a text command to the target
 *  
 * @param TargetIndex Index into the Targets array of which target to use
 * @param Command Command to send to the target
 */
void DLLInterface::SendConsoleCommand(int TargetIndex, String* Command)
{
	// copy over the strings to non-gc memory
	IntPtr BStr1 = Marshal::StringToBSTR(Command);

	// send the console command
	CurrentConsoleSupport->SendConsoleCommand(TargetIndex, (wchar_t*)BStr1.ToPointer());

	// free temp memory
	Marshal::FreeBSTR(BStr1);
}


/**
 * Receive any new text output by the target
 *
 * @param TargetIndex Index into the Targets array of which target to use
 * 
 * @return Output text from the target 
 */
String* DLLInterface::ReceiveOutputText(int TargetIndex)
{
#define BUFFER_SIZE 2048
	// make space for output string
	wchar_t OutBuffer[BUFFER_SIZE + 1];
	// null terminate it
	OutBuffer[BUFFER_SIZE] = 0;

	// ask target for any text
	CurrentConsoleSupport->ReceiveOutputText(TargetIndex, OutBuffer, BUFFER_SIZE);

	// return it
	if (OutBuffer[0] == 0)
	{
		return NULL;
	}

	return OutBuffer;
}

/**
 * Have the console take a screenshot and dump to a file
 * 
 * @param Filename Location to place the .bmp file
 *
 * @return true if successful
 */
bool DLLInterface::ScreenshotBMP(int TargetIndex, String* Filename)
{
	// copy over the strings to non-gc memory
	IntPtr BStr1 = Marshal::StringToBSTR(Filename);

	// send the console command
	bool Ret = CurrentConsoleSupport->ScreenshotBMP(TargetIndex, (wchar_t*)BStr1.ToPointer());

	// free temp memory
	Marshal::FreeBSTR(BStr1);

	return Ret;
}
