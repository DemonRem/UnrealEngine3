/*=============================================================================
	UnConsoleTools.h: Definition of platform agnostic helper functions
					 implemented in a separate DLL.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __UNCONSOLETOOLS_H__
#define __UNCONSOLETOOLS_H__

#ifdef CONSOLETOOLS_EXPORTS
#define CONSOLETOOLS_API __declspec(dllexport)
#else
#define CONSOLETOOLS_API 
#endif

// allow for Managed C++ to include this file and do whatever it needs to use
// the structs
#ifndef MANAGED_CPP_MAGIC 
#define MANAGED_CPP_MAGIC 
#endif

/**
 * Abstract sound cooker interface.
 *
 * Expected usage is:
 *
 * Cook(...)
 * BYTE* CookedData = appMalloc( GetCookedDataSize() );
 * GetCookedData( BYTE* CookedData );
 *
 * and repeat.
 */
class FConsoleSoundCooker
{
public:
	/**
	 * Constructor
	 */
	FConsoleSoundCooker( void ) {}

	/**
	 * Virtual destructor
	 */
	virtual ~FConsoleSoundCooker( void ) {}

	/**
	 * Cooks the source data for the platform and stores the cooked data internally.
	 *
	 * @param	SrcBuffer		Pointer to source buffer
	 * @param	SrcBufferSize	Size in bytes of source buffer
	 * @param	WaveFormat		Pointer to platform specific wave format description
	 * @param	Compression		Quality value ranging from 1 [poor] to 100 [very good]
	 *
	 * @return	TRUE if succeeded, FALSE otherwise
	 */
	virtual bool Cook( short * SrcBuffer, DWORD SrcBufferSize, void * WaveFormat, INT Quality ) = 0;
	
	/**
	 * Cooks upto 8 mono files into a multichannel file (eg. 5.1). The front left channel is required, the rest are optional.
	 *
	 * @param	SrcBuffers		Pointers to source buffers
	 * @param	SrcBufferSize	Size in bytes of source buffer
	 * @param	WaveFormat		Pointer to platform specific wave format description
	 * @param	Compression		Quality value ranging from 1 [poor] to 100 [very good]
	 *
	 * @return	TRUE if succeeded, FALSE otherwise
	 */
	virtual bool CookSurround( short * SrcBuffers[8], DWORD SrcBufferSize, void * WaveFormat, INT Quality ) = 0;

	/**
	 * Returns the size of the cooked data in bytes.
	 *
	 * @return The size in bytes of the cooked data including any potential header information.
	 */
	virtual UINT GetCookedDataSize( void ) = 0;

	/**
	 * Copies the cooked ata into the passed in buffer of at least size GetCookedDataSize()
	 *
	 * @param CookedData	Buffer of at least GetCookedDataSize() bytes to copy data to.
	 */
	virtual void GetCookedData( BYTE * CookedData ) = 0;
	
	/** 
	 * Decompresses the platform dependent format to raw PCM. Used for quality previewing.
	 *
	 * @param	CompressedData		Compressed data
	 * @param	CompressedDataSize	Size of compressed data
	 * @param	AdditionalInfo		Any additonal info required to decompress the sound
	 */
	virtual bool Decompress( const BYTE * CompressedData, DWORD CompressedDataSize, void * AdditionalInfo ) = 0;

	/**
	 * Returns the size of the decompressed data in bytes.
	 *
	 * @return The size in bytes of the raw PCM data
	 */
	virtual DWORD GetRawDataSize( void ) = 0;

	/**
	 * Copies the raw data into the passed in buffer of at least size GetRawDataSize()
	 *
	 * @param DstBuffer	Buffer of at least GetRawDataSize() bytes to copy data to.
	 */
	virtual void GetRawData( BYTE * DstBuffer ) = 0;
};

/**
 * Abstract 2D texture cooker interface.
 * 
 * Expected usage is:
 * Init(...)
 * for( MipLevels )
 * {
 *     Dst = appMalloc( GetMipSize( MipLevel ) );
 *     CookMip( ... )
 * }
 *
 * and repeat.
 */
struct CONSOLETOOLS_API FConsoleTextureCooker
{
	/**
	 * Virtual destructor
	 */
	virtual ~FConsoleTextureCooker(){}

	/**
	 * Associates texture parameters with cooker.
	 *
	 * @param UnrealFormat	Unreal pixel format
	 * @param Width			Width of texture (in pixels)
	 * @param Height		Height of texture (in pixels)
	 * @param NumMips		Number of miplevels
	 * @param CreateFlags	Platform-specific creation flags
	 */
	virtual void Init( DWORD UnrealFormat, UINT Width, UINT Height, UINT NumMips, DWORD CreateFlags ) = 0;

	/**
	 * Returns the platform specific size of a miplevel.
	 *
	 * @param	Level		Miplevel to query size for
	 * @return	Returns	the size in bytes of Miplevel 'Level'
	 */
	virtual UINT GetMipSize( UINT Level ) = 0;

	/**
	 * Cooks the specified miplevel, and puts the result in Dst which is assumed to
	 * be of at least GetMipSize size.
	 *
	 * @param Level			Miplevel to cook
	 * @param Src			Src pointer
	 * @param Dst			Dst pointer, needs to be of at least GetMipSize size
	 * @param SrcRowPitch	Row pitch of source data
	 */
	virtual void CookMip( UINT Level, void* Src, void* Dst, UINT SrcRowPitch ) = 0;

	/**
	* Returns the index of the first mip level that resides in the packed miptail
	* 
	* @return index of first level of miptail 
	*/
	virtual INT GetMipTailBase() = 0;

	/**
	* Cooks all mip levels that reside in the packed miptail. Dst is assumed to 
	* be of size GetMipSize (size of the miptail level).
	*
	* @param Src - ptrs to mip data for each source mip level
	* @param SrcRowPitch - array of row pitch entries for each source mip level
	* @param Dst - ptr to miptail destination
	*/
	virtual void CookMipTail( void** Src, UINT* SrcRowPitch, void* Dst ) = 0;

};

/** The information about an element of a vertex stream that is needed to cook it. */
struct FVertexElementCookInfo
{
	const void* Base;
	UINT Offset;
	UINT Stride;
};

/** The information about a mesh element that is needed to cook it. */
struct FMeshElementCookInfo
{
	const void* PrimaryVertexStreamBase;

	FVertexElementCookInfo PositionVertexElement;
	UINT NumVertices;

	const WORD* Indices;
	UINT NumTriangles;
};

/** The information about a skinned mesh element that is needed to cook it. */
struct FSkinnedMeshElementCookInfo : public FMeshElementCookInfo
{
	FVertexElementCookInfo BoneIndicesVertexElement;
};

/**
 * Abstract skeletal mesh cooker interface.
 * 
 * Expected usage is:
 * Init(...)
 * CookMeshElement( ... )
 *
 * and repeat.
 */
struct CONSOLETOOLS_API FConsoleSkeletalMeshCooker
{
	/**
	 * Virtual destructor
	 */
	virtual ~FConsoleSkeletalMeshCooker(){}

	virtual void Init( void ) = 0;

	/**
	 * Cooks a mesh element.
	 * @param ElementInfo - Information about the element being cooked
	 * @param OutIndices - Upon return, contains the optimized index buffer.
	 * @param OutPartitionSizes - Upon return, points to a list of partition sizes in number of triangles.
	 * @param OutNumPartitions - Upon return, contains the number of partitions.
	 */
	virtual void CookMeshElement(
		const FSkinnedMeshElementCookInfo& ElementInfo,
		WORD* OutIndices,
		UINT*& OutPartitionSizes,
		UINT& OutNumPartitions
		) = 0;
};

/**
 * Abstract static mesh cooker interface.
 * 
 * Expected usage is:
 * Init(...)
 * CookMeshElement( ... )
 *
 * and repeat.
 */
struct CONSOLETOOLS_API FConsoleStaticMeshCooker
{
	/**
	 * Virtual destructor
	 */
	virtual ~FConsoleStaticMeshCooker(){}

	virtual void Init( void ) = 0;

	/**
	 * Cooks a mesh element.
	 * @param ElementInfo - Information about the element being cooked
	 * @param OutIndices - Upon return, contains the optimized index buffer.
	 * @param OutPartitionSizes - Upon return, points to a list of partition sizes in number of triangles.
	 * @param OutNumPartitions - Upon return, contains the number of partitions.
	 */
	virtual void CookMeshElement(
		const FMeshElementCookInfo& ElementInfo,
		WORD* OutIndices,
		UINT*& OutPartitionSizes,
		UINT& OutNumPartitions
		) = 0;
};

/**
 * Base class to handle precompiling shaders offline in a per-platform manner
 */
MANAGED_CPP_MAGIC class FConsoleShaderPrecompiler
{
public:
	/**
	 * Virtual destructor
	 */
	virtual ~FConsoleShaderPrecompiler(){}

	/**
	 * Wrapper function that sets up the strings to call the actual DLL function
	 * DLL will never call this function so it can have undefined types
	 */
	bool CallPrecompiler(const TCHAR* SourceFilename, const TCHAR* FunctionName, const struct FShaderTarget& Target, const struct FShaderCompilerEnvironment& Environment, struct FShaderCompilerOutput& Output);

private:
	/**
	 * Precompile the shader with the given name. Must be implemented
	 *
	 * @param ShaderPath		Pathname to the source shader file ("..\Engine\Shaders\BasePassPixelShader.usf")
	 * @param EntryFunction		Name of the startup function ("pixelShader")
	 * @param bIsVertexShader	True if the vertex shader is being precompiled
	 * @param CompileFlags		Default is 0, otherwise members of the D3DXSHADER enumeration
	 * @param Definitions		Space separated string that contains shader defines ("FRAGMENT_SHADER=1 VERTEX_LIGHTING=0")
	 * @param BytecodeBuffer	Block of memory to fill in with the compiled bytecode
	 * @param BytecodeSize		Size of the returned bytecode
	 * @param ConstantBuffer	String buffer to return the shader definitions with [Name,RegisterIndex,RegisterCount] ("WorldToLocal,100,4 Scale,101,1"), NULL = Error case
	 * @param Errors			String buffer any output/errors
	 * 
	 * @return true if successful
	 */
	virtual bool PrecompileShader(char* ShaderPath, char* EntryFunction, bool bIsVertexShader, DWORD CompileFlags, char* Definitions, unsigned char* BytecodeBufer, int& BytecodeSize, char* ConstantBuffer, char* Errors) = 0;
};

#define MENU_SEPARATOR_LABEL L"--"

/**
 * Base abstract class for a platform-specific support class. Every platform
 * will override the member functions to provide platform-speciific code.
 * This is so that access to platform-specific code is not given to all
 * licensees of the Unreal Engine. This is to protect the NDAs of the
 * console manufacturers from Unreal Licensees who don't have a license with
 * that manufacturer.
 *
 * The ConsoleSupport subclasses will be defined in DLLs, so Epic can control 
 * the code/proprietary information simply by controlling who gets what DLLs.
 */
MANAGED_CPP_MAGIC class FConsoleSupport
{
public:
	enum EConsoleState
	{
		CS_Running,
		CS_NotRunning,
		CS_Crashed,
		CS_Asserted,
		CS_RIP,
	};

	/**
	 * Virtual destructor
	 */
	virtual ~FConsoleSupport(){}

	/** Initialize the DLL with some information about the game/editor
	 * @param	GameName		The name of the current game ("ExampleGame", "UTGame", etc)
	 * @param	Configuration	The name of the configuration to run ("Debug", "Release", etc)
	 */
	virtual void Initialize(const wchar_t* GameName, const wchar_t* Configuration) = 0;

	/**
	 * Return a string name descriptor for this platform (required to implement)
	 *
	 * @return	The name of the platform
	 */
	virtual const wchar_t* GetConsoleName() = 0;

	/**
	 * Return the default IP address to use when sending data into the game for object propagation
	 * Note that this is probably different than the IP address used to debug (reboot, run executable, etc)
	 * the console. (required to implement)
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 *
	 * @return	The in-game IP address of the console, in an Intel byte order 32 bit integer
	 */
	virtual unsigned int GetIPAddress(int TargetIndex) = 0;
	
	/**
	 * Return whether or not this console Intel byte order (required to implement)
	 *
	 * @return	True if the console is Intel byte order
	 */
	virtual bool GetIntelByteOrder() = 0;
	
	/**
	 * @return the number of known xbox targets
	 */
	virtual int GetNumTargets()
	{
		return 0;
	}

	/**
	 * Get the name of the specified target
	 * @param Index Index of the targt to query
	 * @return Name of the target, or NULL if the Index is out of bounds
	 */
	virtual const wchar_t* GetTargetName(int /*Index*/)
	{
		return NULL;
	}

	/**
	 * @return true if this platform needs to have files copied from PC->target (required to implement)
	 */
	virtual bool PlatformNeedsToCopyFiles() = 0;

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
	virtual int ConnectToTarget(const wchar_t* /*TargetName*/)
	{
		// if the platform doesn't need connections, just return a valid connection number
		return 0;
	}

	/**
	 * Called after an atomic operation to close any open connections
	 */
	virtual void DisconnectFromTarget(int /*TargetIndex*/)
	{

	}

	/**
	 * Creates a directory
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param SourceFilename Platform-independent directory name (ie UnrealEngine3\Binaries)
	 *
	 * @return true if successful
	 */
	virtual bool MakeDirectory(int /*TargetIndex*/, const wchar_t* /*DirectoryName*/)
	{
		return false;
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
	virtual bool NeedsToCopyFile(int /*TargetIndex*/, const wchar_t* /*SourceFilename*/, const wchar_t* /*DestFilename*/)
	{
		return false;
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
	virtual bool CopyFile(int /*TargetIndex*/, const wchar_t* /*SourceFilename*/, const wchar_t* /*DestFilename*/)
	{
		return false;
	}

	/**
	 * Sets the name of the layout file for the DVD, so that GetDVDFileStartSector can be used
	 * 
	 * @param DVDLayoutFile Name of the layout file
	 *
	 * @return true if successful
	 */
	virtual bool SetDVDLayoutFile(const wchar_t* /*DVDLayoutFile*/)
	{
		return true;
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
	virtual bool GetDVDFileStartSector(const wchar_t* /*DVDFilename*/, unsigned int& /*SectorHigh*/, unsigned int& /*SectorLow*/)
	{
		return false;
	}

	/**
	 * Reboots the target console. Must be implemented
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * 
	 * @return true if successful
	 */
	virtual bool Reboot(int TargetIndex) = 0;

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
	virtual bool RebootAndRun(int TargetIndex, const wchar_t* Configuration, const wchar_t* BaseDirectory, const wchar_t* GameName, const wchar_t* URL) = 0;

	/**
	 * Run the game on the target console (required to implement)
	 * @param	URL				The map name and options to run the game with
	 * @param	OutputConsoleCommand	A buffer that the menu item can fill out with a console command to be run by the Editor on return from this function
	 * @param	CommandBufferSize		The size of the buffer pointed to by OutputConsoleCommand
	 *
	 * @return	Returns true if the run was successful
	 */
	virtual bool RunGame(const wchar_t* URL, wchar_t* OutputConsoleCommand, int CommandBufferSize) = 0;
	
	/**
	 * Retrieve the state of the console (running, not running, crashed, etc)
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 *
	 * @return the current state
	 */
	virtual EConsoleState GetConsoleState(int TargetIndex) = 0;

	/**
	 * Get the callstack for a crashed target
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param Buffer Memory to hold the callstack addresses
	 * @param BufferLength Number of entries available in Buffer
	 *
	 * @return true if succesful
	 */
	virtual bool GetCallstack(int /*TargetIndex*/, unsigned int* /*Buffer*/, int /*BufferLength*/)
	{
		return false;
	}

	/**
	 * Loads the debug symbols for the an executable
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param ExecutableName Name of the executable file, or NULL to attempt to use the currently running executable's symbols
	 *
	 * @return true if successful
	 */
	virtual bool LoadSymbols(int /*TargetIndex*/, const wchar_t* /*ExecutableName*/)
	{
		return false;
	}

	/**
	 * Unloads any loaded debug symbols
	 */
	virtual void UnloadAllSymbols()
	{

	}

	/**
	 * Turn an address into a symbol for callstack dumping
	 * 
	 * @param Address Code address to be looked up
	 * @param OutString Function name/symbol of the given address
	 * @param OutLength Size of the OutString buffer
	 */
	virtual void ResolveAddressToString(unsigned int /*Address*/, wchar_t* OutString, int /*OutLength*/)
	{
		// by default return empty string
		OutString[0] = 0;
	}

	/**
	 * Resolves passed in address to file/ function and line information
	 *
	 * @param			Address			Address to look up symbol info for
	 * @param			StringLength	Size of the two output buffers
	 * @param	[out]	OutFilename		Filename associated with address
	 * @param	[out]	OutFunction		Function associated with address
	 * @param	[out]	OutLine			Line number associated with address
	 */
	virtual void ResolveAddressToSymboInfo(unsigned int /*Address*/, int /*StringLength*/, wchar_t* OutFilename, wchar_t* OutFunction, int& OutLine )
	{
		OutFilename[0] = 0;
		OutFunction[0] = 0;
		OutLine = -1;
	}

	/**
	 * Send a text command to the target
	 * 
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param Command Command to send to the target
	 */
	virtual void SendConsoleCommand(int TargetIndex, const wchar_t* Command) = 0;

	/**
	 * Receive any new text output by the target
	 *
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param OutputString Text from the target 
	 * @param OutLength Size of the OutputString
	 */
	virtual void ReceiveOutputText(int TargetIndex, wchar_t* OutString, unsigned int OutLength) = 0;

	/**
	 * Have the console take a screenshot and dump to a file
	 * 
	 * @param TargetIndex Index into the Targets array of which target to use
	 * @param Filename Location to place the .bmp file
	 *
	 * @return true if successful
	 */
	virtual bool ScreenshotBMP(int /*TargetIndex*/, const wchar_t* /*Filename*/)
	{
		return false;
	}

	/**
	 * Return the number of console-specific menu items this platform wants to add to the main
	 * menu in UnrealEd.
	 *
	 * @return	The number of menu items for this console
	 */
	virtual int GetNumMenuItems() 
	{ 
		return 0; 
	}
	
	/**
	 * Return the string label for the requested menu item
	 * @param	Index		The requested menu item
	 * @param	bIsChecked	Is this menu item checked (or selected if radio menu item)
	 * @param	bIsRadio	Is this menu item part of a radio group?
	 *
	 * @return	Label for the requested menu item
	 */
	virtual const wchar_t* GetMenuItem(int /*Index*/, bool& /*bIsChecked*/, bool& /*bIsRadio*/) 
	{ 
		return NULL; 
	}
	
	/**
	 * Internally process the given menu item when it is selected in the editor
	 * @param	Index		The selected menu item
	 * @param	OutputConsoleCommand	A buffer that the menu item can fill out with a console command to be run by the Editor on return from this function
	 * @param	CommandBufferSize		The size of the buffer pointed to by OutputConsoleCommand
	 */
	virtual void ProcessMenuItem(int /*Index*/, wchar_t* /*OutputConsoleCommand*/, int /*CommandBufferSize*/) 
	{ 
	}

	/**
	 * Handles receiving a value from the application, when ProcessMenuItem returns that the ConsoleSupport needs to get a value
	 * @param	Value		The actual value received from user
	 */
	virtual void SetValueCallback(const wchar_t* /*Value*/) 
	{ 
	}

	/**
	 * Returns the global sound cooker object.
	 *
	 * @return global sound cooker object, or NULL if none exists
	 */
	virtual FConsoleSoundCooker* GetGlobalSoundCooker( void )
	{
		// Default implementations exist for the sake of e.g. WindowsTools, but should never be called.
		// PURE_VIRTUAL and appErrorf unfortunately cannot be used here because it's not defined for WindowsTools.
		return NULL;
	}

	/**
	 * Returns the global texture cooker object.
	 *
	 * @return global sound cooker object, or NULL if none exists
	 */
	virtual FConsoleTextureCooker* GetGlobalTextureCooker( void )
	{
		// Default implementations exist for the sake of e.g. WindowsTools, but should never be called.
		// PURE_VIRTUAL and appErrorf unfortunately cannot be used here because it's not defined for WindowsTools.
		return NULL;
	}

	/**
	 * Returns the global skeletal mesh cooker object.
	 *
	 * @return global skeletal mesh cooker object, or NULL if none exists
	 */
	virtual FConsoleSkeletalMeshCooker* GetGlobalSkeletalMeshCooker( void )
	{
		// Default implementations exist for the sake of e.g. WindowsTools, but should never be called.
		// PURE_VIRTUAL and appErrorf unfortunately cannot be used here because it's not defined for WindowsTools.
		return NULL;
	}

	/**
	 * Returns the global static mesh cooker object.
	 *
	 * @return global static mesh cooker object, or NULL if none exists
	 */
	virtual FConsoleStaticMeshCooker* GetGlobalStaticMeshCooker( void )
	{
		// Default implementations exist for the sake of e.g. WindowsTools, but should never be called.
		// PURE_VIRTUAL and appErrorf unfortunately cannot be used here because it's not defined for WindowsTools.
		return NULL;
	}

	/**
	 * Returns the global shader precompiler object.
	 * @return global shader precompiler object, or NULL if none exists.
	 */
	virtual FConsoleShaderPrecompiler* GetGlobalShaderPrecompiler()
	{
		// Default implementations exist for the sake of e.g. WindowsTools, but should never be called.
		// PURE_VIRTUAL and appErrorf unfortunately cannot be used here because it's not defined for WindowsTools.
		return NULL;
	}

	/**
	 * Frees memory allocated and returned by the console support module.
	 * @param Pointer - A pointer to the memory to be freed.
	 */
	virtual void FreeReturnedMemory(void* Pointer)
	{
	}
};

// Typedef's for easy DLL binding.
typedef FConsoleSupport* (*FuncGetConsoleSupport) (void);

extern "C"
{
	/** 
	 * Returns a pointer to a subclass of FConsoleSupport.
	 *
	 * @return The pointer to the console specific FConsoleSupport
	 */
	CONSOLETOOLS_API FConsoleSupport* GetConsoleSupport();
}

#endif
