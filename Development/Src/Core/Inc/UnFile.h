/*=============================================================================
	UnFile.h: General-purpose file utilities.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __UNFILE_H__
#define __UNFILE_H__

#if USE_SECURE_CRT
#pragma warning(default : 4996)	// enable deprecation warnings
#endif

/*-----------------------------------------------------------------------------
	Defines.
-----------------------------------------------------------------------------*/

// Might be overridde on a per platform basis.
#ifndef DVD_ECC_BLOCK_SIZE
/** Default DVD sector size in bytes.						*/
#define DVD_SECTOR_SIZE		(2048)
/** Default DVD ECC block size for read offset alignment.	*/
#define DVD_ECC_BLOCK_SIZE	(32 * 1024)
/** Default minimum read size for read size alignment.		*/
#define DVD_MIN_READ_SIZE	(128 * 1024)
#endif

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// Global variables.
extern DWORD GCRCTable[];

/*----------------------------------------------------------------------------
	Byte order conversion.
----------------------------------------------------------------------------*/

// These macros are not safe to use unless data is UNSIGNED!
#define BYTESWAP_ORDER16_unsigned(x)   ((((x)>>8)&0xff)+ (((x)<<8)&0xff00))
#define BYTESWAP_ORDER32_unsigned(x)   (((x)>>24) + (((x)>>8)&0xff00) + (((x)<<8)&0xff0000) + ((x)<<24))

static inline WORD BYTESWAP_ORDER16(WORD val)
{
#if PS3_SNC
	return __lhbrx(&val);
#elif __PPC__ && __GNUC__
	register WORD retval;
	__asm__ __volatile__("lhbrx %0,0,%1"
						: "=r" (retval)
						: "r" (&val) );
	return retval;
#else
	return(BYTESWAP_ORDER16_unsigned(val));
#endif
}

static inline SWORD BYTESWAP_ORDER16(SWORD val)
{
	WORD uval = *((WORD *) &val);
	uval = BYTESWAP_ORDER16_unsigned(uval);
	return( *((SWORD *) &uval) );
}

static inline DWORD BYTESWAP_ORDER32(DWORD val)
{
#if PS3_SNC
	return __lwbrx(&val);
#elif __PPC__ && __GNUC__
	register DWORD retval;
	__asm__ __volatile__("lwbrx %0,0,%1"
						: "=r" (retval)
						: "r" (&val) );
	return retval;
#else
	return(BYTESWAP_ORDER32_unsigned(val));
#endif
}

static inline INT BYTESWAP_ORDER32(INT val)
{
	DWORD uval = *((DWORD *) &val);
	uval = BYTESWAP_ORDER32_unsigned(uval);
	return( *((INT *) &uval) );
}

static inline FLOAT BYTESWAP_ORDERF(FLOAT val)
{
	DWORD uval = *((DWORD *) &val);
	uval = BYTESWAP_ORDER32_unsigned(uval);
	return( *((FLOAT *) &uval) );
}

static inline QWORD BYTESWAP_ORDER64(QWORD Value)
{
	QWORD Swapped = 0;
	BYTE* Forward = (BYTE*)&Value;
	BYTE* Reverse = (BYTE*)&Swapped + 7;
	for( INT i=0; i<8; i++ )
	{
		*(Reverse--) = *(Forward++); // copy into Swapped
	}
	return Swapped;
}

#if UNICODE
	static inline void BYTESWAP_ORDER_TCHARARRAY(TCHAR* str)
	{
		for (TCHAR* c = str; *c; ++c)
		{
			*c = BYTESWAP_ORDER16_unsigned(*c);
		}
	}
#else
	#define BYTESWAP_ORDER_TCHARARRAY(x)
#endif

// Bitfields.
#ifndef NEXT_BITFIELD
	#if __INTEL_BYTE_ORDER__
		#define NEXT_BITFIELD(b) ((b)<<1)
		#define FIRST_BITFIELD   (1)
	#else
		#define NEXT_BITFIELD(b) ((b)>>1)
		#define FIRST_BITFIELD   (0x80000000)
	#endif
#endif

// General byte swapping.
#if __INTEL_BYTE_ORDER__
	#define INTEL_ORDER16(x)   (x)
	#define INTEL_ORDER32(x)   (x)
	#define INTEL_ORDERF(x)    (x)
	#define INTEL_ORDER64(x)   (x)
	#define INTEL_ORDER_TCHARARRAY(x)
#else
	#define INTEL_ORDER16(x)			BYTESWAP_ORDER16(x)
	#define INTEL_ORDER32(x)			BYTESWAP_ORDER32(x)
	#define INTEL_ORDERF(x)				BYTESWAP_ORDERF(x)
	#define INTEL_ORDER64(x)			BYTESWAP_ORDER64(x)
	#define INTEL_ORDER_TCHARARRAY(x)	BYTESWAP_ORDER_TCHARARRAY(x)
#endif

/*-----------------------------------------------------------------------------
	Stats.
-----------------------------------------------------------------------------*/

#if STATS
	#define STAT(x) x
#else
	#define STAT(x)
#endif

/*-----------------------------------------------------------------------------
	Global init and exit.
-----------------------------------------------------------------------------*/

/** @name Global init and exit */
//@{
/** General initialization.  Called from within guarded code at the beginning of engine initialization. */
void appInit( const TCHAR* InCmdLine, FOutputDevice* InLog, FOutputDeviceConsole* InLogConsole, FOutputDeviceError* InError, FFeedbackContext* InWarn, FFileManager* InFileManager, FCallbackEventObserver* InCallbackEventDevice, FCallbackQueryDevice* InCallbackQueryDevice, FConfigCache*(*ConfigFactory)() );
/** Pre-shutdown.  Called from within guarded exit code, only during non-error exits.*/
void appPreExit();
/** Shutdown.  Called outside guarded exit code, during all exits (including error exits). */
void appExit();
/** Starts up the socket subsystem */
void appSocketInit(void);
/** Shuts down the socket subsystem */
void appSocketExit(void);
/** Returns the language setting that is configured for the platform */
FString appGetLanguageExt(void);
//@}

/** @name Interface for recording loading errors in the editor */
//@{
void EdClearLoadErrors();
VARARG_DECL( void, static void, VARARG_NONE, EdLoadErrorf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(INT Type), VARARG_EXTRA(Type) );

/** Passed to appMsgf to specify the type of message dialog box. */
enum EAppMsgType
{
	AMT_OK,
	AMT_YesNo,
	AMT_OKCancel,
	AMT_YesNoCancel,
	AMT_CancelRetryContinue,
	AMT_YesNoYesAllNoAll,
	AMT_YesNoYesAllNoAllCancel,
};

/** Returned from appMsgf to specify the type of chosen option. */
enum EAppReturnType
{
	ART_No			= 0,
	ART_Yes,
	ART_YesAll,
	ART_NoAll,
	ART_Cancel,
};

/**
* Pops up a message dialog box containing the input string.  Return value depends on the type of dialog.
*
* @param	Type	Specifies the type of message dialog.
* @return			1 if user selected OK/YES, or 0 if user selected CANCEL/NO.
*/
VARARG_DECL( UBOOL, static UBOOL, return, appMsgf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(EAppMsgType Type), VARARG_EXTRA(Type) );
//@}

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

/** @name DLL access */
//@{
void* appGetDllHandle( const TCHAR* DllName );
/** Frees a DLL. */
void appFreeDllHandle( void* DllHandle );
/** Looks up the address of a DLL function. */
void* appGetDllExport( void* DllHandle, const TCHAR* ExportName );
//@}

/** Does per platform initialization of timing information */
void appInitTiming(void);

/**
 * Launches a uniform resource locator (i.e. http://www.epicgames.com/unreal).
 * This is expected to return immediately as the URL is launched by another task.
 */
void appLaunchURL( const TCHAR* URL, const TCHAR* Parms=NULL, FString* Error=NULL );
/**
 * Creates a new process and its primary thread. The new process runs the
 * specified executable file in the security context of the calling process.
 */
void* appCreateProc( const TCHAR* URL, const TCHAR* Parms );
/** Retrieves the termination status of the specified process. */
UBOOL appGetProcReturnCode( void* ProcHandle, INT* ReturnCode );
/** Creates a new globally unique identifier. */
class FGuid appCreateGuid();

/** 
* Creates a temporary filename. 
*
* @param Path - file pathname
* @param Result1024 - destination buffer to store results of unique path (@warning must be >= MAX_SPRINTF size)
*/
void appCreateTempFilename( const TCHAR* Path, TCHAR* Result, SIZE_T ResultSize );

/**
 * Cleans the package download cache and any other platform specific cleaning 
 */
void appCleanFileCache();

UBOOL appCreateBitmap( const TCHAR* Pattern, INT Width, INT Height, class FColor* Data, FFileManager* FileManager = GFileManager );

/**
 * Finds a usable splash pathname for the given filename
 * 
 * @param SplashFilename Name of the desired splash name ("Splash.bmp")
 * @param OutPath String containing the path to the file, if this function returns TRUE
 *
 * @return TRUE if a splash screen was found
 */
UBOOL appGetSplashPath(const TCHAR* SplashFilename, FString& OutPath);

/** deletes log files older than a number of days specified in the Engine ini file */
void appDeleteOldLogs();

/**
 * This will load up two .ini files and then determine if the Generated one is outdated.
 * Outdatedness is determined by the following mechanic:
 *
 * When a generated .ini is written out it will store the timestamps of the files it was generated
 * from.  This way whenever the Default__.inis are modified the Generated .ini will view itself as
 * outdated and regenerate it self.
 *
 * Outdatedness also can be affected by commandline params which allow one to delete all .ini, have
 * automated build system etc.
 *
 * Additionally, this function will save the previous generation of the .ini to the Config dir with
 * a datestamp.
 *
 * Finally, this function will load the Generated .ini into the global ConfigManager.
 *
 * @param DefaultIniFile		The Default .ini file (e.g. DefaultEngine.ini )
 * @param GeneratedIniFile		The Generated .ini file (e.g. FooGameEngine.ini )
 * @param YesNoToAll			[out] Receives the user's selection if an .ini was out of date.
 */
void appCheckIniForOutdatedness( const TCHAR* DefaultIniFile, const TCHAR* GeneratedIniFile, UINT& YesNoToAll );


/**
 * This will create the .ini filenames for the Default and the Game based off the passed in values.
 * (e.g. DefaultEditor.ini, MyLeetGameEditor.ini  and the respective relative paths to those files )
 *
 * Once the names have been created we will call a function to check to see if the generated .ini
 * is outdated.
 *
 * @param GeneratedIniName				The Global TCHAR[1024] that unreal uses ( e.g. GEngineIni )
 * @param CommandLineDefaultIniToken	The token to look for on the command line to parse the passed in Default<Type>Ini
 * @param CommandLineIniToken			The token to look for on the command line to parse the passed in <Type>Ini
 * @param IniFileName					The IniFile's name  (e.g. Engine.ini Editor.ini )
 * @param DefaultIniPrefix				What the prefix for the Default .inis should be  ( e.g. Default )
 * @param IniPrefix						What the prefix for the Game's .inis should be  ( e.g. <gameName> )
 * @param IniSectionToLookFor			The section to look for in the .ini
 * @param IniKeyToLookFor				The key to look for in the specified section of the .ini
 * @param YesNoToAll					[out] Receives the user's selection if an .ini was out of date.
 */
void appCreateIniNamesAndThenCheckForOutdatedness( TCHAR* GeneratedIniName, const TCHAR* CommandLineDefaultIniName, const TCHAR* CommandLineIniName, const TCHAR* IniFileName, const TCHAR* DefaultIniPrefix, const TCHAR* IniPrefix, UINT& YesNoToAll );

/**
 * Generates a default INI name using the current platform and the IniFileName suffix passed in.
 *
 * @param IniFileName			INI File suffix to append to the end of the generated name (Engine.ini, Editor.ini, etc...)
 * @param OutGeneratedIniName	Pointer to store the generated filename in, should be atleast 1024 bytes.
 */
void appCreateDefaultIniName(const TCHAR* IniFileName, TCHAR* OutGeneratedIniName);

/**
* This will completely load an .ini file into the passed in FConfigFile.  This means that it will 
* recurse up the BasedOn hierarchy loading each of those .ini.  The passed in FConfigFile will then
* have the data after combining all of those .ini 
*
* @param FilenameToLoad - this is the path to the file to 
* @param ConfigFile - This is the FConfigFile which will have the contents of the .ini loaded into and Combined()
* @param bUpdateIniTimeStamps - whether to update the timestamps array.  Only for Default___.ini should this be set to TRUE.  The generated .inis already have the timestamps.
*
**/
void LoadAnIniFile( const TCHAR* FilenameToLoad, FConfigFile& ConfigFile, UBOOL bUpdateIniTimeStamps );

#define XENON_DEFAULT_INI_PREFIX TEXT("Xenon\\Xenon")
#define XENON_INI_PREFIX TEXT("Xe-")
#define XENON_COOKED_CONFIG_DIR TEXT("Xenon\\Cooked\\")

#define UNIX_DEFAULT_INI_PREFIX TEXT("Linux/Linux")
#define UNIX_INI_PREFIX TEXT("Linux-")
#define UNIX_COOKED_CONFIG_DIR This should not be used on Unix

#define PS3_DEFAULT_INI_PREFIX TEXT("PS3\\PS3")
#define PS3_INI_PREFIX TEXT("PS3-")
#define PS3_COOKED_CONFIG_DIR TEXT("PS3\\Cooked\\")

#define PC_DEFAULT_INI_PREFIX TEXT("Default")
#define PC_INI_PREFIX TEXT("")
#define PC_COOKED_CONFIG_DIR This should not be used on PC

// Default ini prefix.
#if XBOX
    #define DEFAULT_INI_PREFIX XENON_DEFAULT_INI_PREFIX
    #define INI_PREFIX XENON_INI_PREFIX
	#define COOKED_CONFIG_DIR XENON_COOKED_CONFIG_DIR
#elif PLATFORM_UNIX
    #define DEFAULT_INI_PREFIX UNIX_DEFAULT_INI_PREFIX
    #define INI_PREFIX UNIX_INI_PREFIX
	#define COOKED_CONFIG_DIR UNIX_COOKED_CONFIG_DIR
#elif PS3
    #define DEFAULT_INI_PREFIX PS3_DEFAULT_INI_PREFIX
    #define INI_PREFIX PS3_INI_PREFIX
	#define COOKED_CONFIG_DIR PS3_COOKED_CONFIG_DIR
#elif defined (WIN32)
    #define DEFAULT_INI_PREFIX PC_DEFAULT_INI_PREFIX
    #define INI_PREFIX PC_INI_PREFIX
	#define COOKED_CONFIG_DIR PC_COOKED_CONFIG_DIR
#else
	#error define your platform here
#endif




/*-----------------------------------------------------------------------------
	Package file cache.
-----------------------------------------------------------------------------*/

/**
 * This will recurse over a directory structure looking for files.
 * 
 * @param Result The output array that is filled out with a file paths
 * @param RootDirectory The root of the directory structure to recurse through
 * @param bFindPackages Should this function add package files to the Resuls
 * @param bFindNonPackages Should this function add non-package files to the Results
 */
void appFindFilesInDirectory(TArray<FString>& Results, const TCHAR* RootDirectory, UBOOL bFindPackages, UBOOL bFindNonPackages);

#define NO_USER_SPECIFIED -1

/** @name Package file cache */
//@{
struct FPackageFileCache
{
	/**
	 * Strips all path and extension information from a relative or fully qualified file name.
	 *
	 * @param	InPathName	a relative or fully qualified file name
	 *
	 * @return	the passed in string, stripped of path and extensions
	 */
	static FString PackageFromPath( const TCHAR* InPathName );

	/**
	 * Parses a fully qualified or relative filename into its components (filename, path, extension).
	 *
	 * @param	InPathName	the filename to parse
	 * @param	Path		[out] receives the value of the path portion of the input string
	 * @param	Filename	[out] receives the value of the filename portion of the input string
	 * @param	Extension	[out] receives the value of the extension portion of the input string
	 */
	static void SplitPath( const TCHAR* InPathName, FString& Path, FString& Filename, FString& Extension );

	/**
	 * Replaces all slashes and backslashes in the string with the correct path separator character for the current platform.
	 *
	 * @param	InFilename	a string representing a filename.
	 */
	static void NormalizePathSeparators( FString& InFilename );

	/** 
	 * Cache all packages found in the engine's configured paths directories, recursively.
	 */
	virtual void CachePaths()=0;

	/**
	 * Adds the package name specified to the runtime lookup map.  The stripped package name (minus extension or path info) will be mapped
	 * to the fully qualified or relative filename specified.
	 *
	 * @param	InPathName		a fully qualified or relative [to Binaries] path name for an Unreal package file.
	 * @param	InOverrideDupe	specify TRUE to replace existing mapping with the new path info
	 * @param	WarnIfExists	specify TRUE to write a warning to the log if there is an existing entry for this package name
	 *
	 * @return	TRUE if the specified path name was successfully added to the lookup table; FALSE if an entry already existed for this package
	 */
	virtual UBOOL CachePackage( const TCHAR* InPathName, UBOOL InOverrideDupe=FALSE, UBOOL WarnIfExists=TRUE)=0;

	/**
	 * Finds the fully qualified or relative pathname for the package specified.
	 *
	 * @param	InName			a string representing the filename of an Unreal package; may include path and/or extension.
	 * @param	Guid			if specified, searches the directory containing files downloaded from other servers (cache directory) for the specified package.
	 * @param	OutFilename		receives the full [or relative] path that was originally registered for the package specified.
	 * @param	Language		Language version to retrieve if overridden for that particular language
	 * @param	bShouldLookForSFPackages If TRUE, first look for Package_SF before looking for Package. Defaults to TRUE.
	 *
	 * @return	TRUE if the package was successfully found.
	 */
	virtual UBOOL FindPackageFile( const TCHAR* InName, const FGuid* Guid, FString& OutFileName, const TCHAR* Language=NULL, UBOOL bShouldLookForSFPackages=TRUE )=0;

	/**
	 * Returns the list of fully qualified or relative pathnames for all registered packages.
	 */
	virtual TArray<FString> GetPackageFileList() = 0;

	/**
	 * Add a downloaded content package to the list of known packages.
	 * Can be assigned to a particular ID for removing with ClearDownloadadPackages.
	 *
	 * @param InPlatformPathName The platform-specific full path name to a package (will be passed directly to CreateFileReader)
	 * @param UserIndex Optional user to associate with the package so that it can be flushed later
	 *
	 * @return TRUE if successful
	 */
	virtual UBOOL CacheDownloadedPackage(const TCHAR* InPlatformPathName, INT UserIndex=NO_USER_SPECIFIED)=0;

	/**
	 * Remove any downloaded packages associated with the given user from the cache.
	 * If the UserIndex is not supplied, only packages cached without a UserIndex
	 * will be removed (it will not remove all packages).
	 *
	 * @param UserIndex Optional UserIndex for which packages to remove
	 */
	virtual void ClearDownloadedPackages(INT UserIndex=NO_USER_SPECIFIED)=0;
};

extern FPackageFileCache* GPackageFileCache;
//@}

/** Converts a relative path name to a fully qualified name */
FString appConvertRelativePathToFull(const FString& InString);

/*-----------------------------------------------------------------------------
	Clipboard.
-----------------------------------------------------------------------------*/

/** @name Clipboard */
//@{
/** Copies text to the operating system clipboard. */
void appClipboardCopy( const TCHAR* Str );
/** Pastes in text from the operating system clipboard. */
FString appClipboardPaste();
//@}

/*-----------------------------------------------------------------------------
	Exception handling.
-----------------------------------------------------------------------------*/

/** @name Exception handling */
//@{
/** For throwing string-exceptions which safely propagate through guard/unguard. */
VARARG_DECL( void VARARGS, static void, VARARG_NONE, appThrowf, VARARG_NONE, const TCHAR*, VARARG_NONE, VARARG_NONE );
//@}

/*-----------------------------------------------------------------------------
	Check macros for assertions.
-----------------------------------------------------------------------------*/

//
// "check" expressions are only evaluated if enabled.
// "verify" expressions are always evaluated, but only cause an error if enabled.
//
/** @name Assertion macros */
//@{

inline void NullAssertFunc(...)		{}

#if DO_CHECK
	#define checkCode( Code )		do { Code } while ( false );
	#define checkMsg(expr,msg)		{ if(!(expr)) {      appFailAssert( #expr " : " #msg , __FILE__, __LINE__ ); }  }
    #define checkFunc(expr,func)	{ if(!(expr)) {func; appFailAssert( #expr, __FILE__, __LINE__ ); }              }
	#define verify(expr)			{ if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ ); }
	#define check(expr)				{ if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ ); }

	/**
	 * Denotes codepaths that should never be reached.
	 */
	#define checkNoEntry()       { appFailAssert( "Enclosing block should never be called", __FILE__, __LINE__ ); }

	/**
	 * Denotes codepaths that should not be executed more than once.
	 */
	#define checkNoReentry()     { static bool s_beenHere##__LINE__ = false;                                         \
	                               checkMsg( !s_beenHere##__LINE__, Enclosing block was called more than once );   \
								   s_beenHere##__LINE__ = true; }

	class FRecursionScopeMarker
	{
	public: 
		FRecursionScopeMarker(WORD &InCounter) : Counter( InCounter ) { ++Counter; }
		~FRecursionScopeMarker() { --Counter; }
	private:
		WORD& Counter;
	};

	/**
	 * Denotes codepaths that should never be called recursively.
	 */
	#define checkNoRecursion()  static WORD RecursionCounter##__LINE__ = 0;                                            \
	                            checkMsg( RecursionCounter##__LINE__ == 0, Enclosing block was entered recursively );  \
	                            const FRecursionScopeMarker ScopeMarker##__LINE__( RecursionCounter##__LINE__ )

	/**
	 * Performs a compile-time assertion.
	 *
	 * @param expr		Must be evaluatable at compile time.
	 */
	#define checkAtCompileTime(expr)  typedef BYTE CompileTimeCheckType##__LINE__[(expr) ? 1 : -1]

#if SUPPORTS_VARIADIC_MACROS
	/**
	* verifyf, checkf: Same as verify, check but with printf style additional parameters
	* Read about __VA_ARGS__ (variadic macros) on http://gcc.gnu.org/onlinedocs/gcc-3.4.4/cpp.pdf.
	*/
	#define verifyf(expr, ...)				{ if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__, ##__VA_ARGS__ ); }
	#define checkf(expr, ...)				{ if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__, ##__VA_ARGS__ ); }
#elif _MSC_VER >= 1300
	#define verifyf(expr,a,b,c,d,e,f,g,h)	{ if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__, VARG(a),VARG(b),VARG(c),VARG(d),VARG(e),VARG(f),VARG(g),VARG(h) ); }
	#define checkf(expr,a,b,c,d,e,f,g,h)	{ if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__, VARG(a),VARG(b),VARG(c),VARG(d),VARG(e),VARG(f),VARG(g),VARG(h) ); }
#endif

#else
#if COMPILER_SUPPORTS_NOOP
	// MS compilers support noop which discards everything inside the parens
	#define checkCode(Code)			{}
	#define check					__noop
	#define checkf					__noop
	#define checkMsg				__noop
	#define checkFunc				__noop
	#define checkNoEntry			__noop
	#define checkNoReentry			__noop
	#define checkNoRecursion		__noop
	#define checkAtCompileTime		__noop
#else
	#define checkCode(Code)				{}
	#define check						{}
	#define checkf						NullAssertFunc
    #define checkMsg(expr,msg)			{}
    #define checkFunc(expr,func)		{}    
	#define checkNoEntry()				{}
	#define checkNoReentry()			{}
	#define checkNoRecursion()			{}
	#define checkAtCompileTime(expr)	{}
#endif
	#define verify(expr)						{ if(!(expr)){} }
	#if SUPPORTS_VARIADIC_MACROS
		#define verifyf(expr, ...)				{ if(!(expr)){} }
	#elif _MSC_VER >= 1300
		#define verifyf(expr,a,b,c,d,e,f,g,h)	{ if(!(expr)){} }
	#endif
#endif

//
// Check for development only.
//
#if DO_GUARD_SLOW
    #if SUPPORTS_VARIADIC_MACROS
        #define checkSlow(expr, ...)   {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
		#define checkfSlow(expr, ...)	{ if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__, ##__VA_ARGS__ ); }
    #elif _MSC_VER >= 1300
        #define checkSlow(expr,a,b,c,d,e,f,g,h)   {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
		#define checkfSlow(expr,a,b,c,d,e,f,g,h)	{ if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__, VARG(a),VARG(b),VARG(c),VARG(d),VARG(e),VARG(f),VARG(g),VARG(h) ); }
    #endif

	#define verifySlow(expr)  {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
#else
#if COMPILER_SUPPORTS_NOOP
	// MS compilers support noop which discards everything inside the parens
	#define checkSlow         __noop
	#define checkfSlow        __noop
#else
	#define checkSlow(expr)   {}
	#define checkfSlow		  NullAssertFunc
#endif
	#define verifySlow(expr)  if(expr){}
#endif
//@}


/*-----------------------------------------------------------------------------
	Localization.
-----------------------------------------------------------------------------*/

// This is an array of all known language extensions. Useful for iterating over
// all possible languages when you can't use just UObject::GetLanguage()
// NOTE: This array is terminated with a NULL TCHAR*, and can be iterated over like so:
//		for (INT LangIndex = 0; GKnownLanguageExtensions[LangIndex]; LangIndex++)

extern const TCHAR* GKnownLanguageExtensions[];


/** @name Localization */
//@{
FString Localize( const TCHAR* Section, const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
FString LocalizeLabel( const TCHAR* Section, const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
FString LocalizeError( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
FString LocalizeProgress( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
FString LocalizeQuery( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
FString LocalizeGeneral( const TCHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
FString LocalizeUnrealEd( const TCHAR* Key, const TCHAR* Package=TEXT("UnrealEd"), const TCHAR* LangExt=NULL );

#if UNICODE
	FString Localize( const ANSICHAR* Section, const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
	FString LocalizeLabel( const ANSICHAR* Section, const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
	FString LocalizeError( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
	FString LocalizeProgress( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
	FString LocalizeQuery( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
	FString LocalizeGeneral( const ANSICHAR* Key, const TCHAR* Package, const TCHAR* LangExt=NULL );
	FString LocalizeUnrealEd( const ANSICHAR* Key, const TCHAR* Package=TEXT("UnrealEd"), const TCHAR* LangExt=NULL );
#endif
//@}

/*-----------------------------------------------------------------------------
	OS functions.
-----------------------------------------------------------------------------*/

/** @name OS functions */
//@{
/** Returns the executable command line. */
const TCHAR* appCmdLine();
/** Returns the startup directory (the directory this executable was launched from).  NOTE: Only one return value is valid at a time! */
const TCHAR* appBaseDir();
/** Returns the computer name.  NOTE: Only one return value is valid at a time! */
const TCHAR* appComputerName();
/** Returns the user name.  NOTE: Only one return value is valid at a time! */
const TCHAR* appUserName();
/** Returns shader dir relative to appBaseDir */
const TCHAR* appShaderDir();
//@}

namespace UE3
{
	/**
	 * The platform that this is running on.  This mask is also used by UFunction::PlatformFlags to determine which platforms
	 * a native function can be bound for.
	 */
	enum EPlatformType
	{
		PLATFORM_Unknown	=	0x00000000,
		PLATFORM_Windows	=	0x00000001,
		PLATFORM_Xenon		=	0x00000004,
		PLATFORM_PS3		=	0x00000008,
		PLATFORM_Linux		=	0x00000010,
		PLATFORM_Mac		=	0x00000020,


		// Combination Masks
		/** PC platform types */
		PLATFORM_PC			=	PLATFORM_Windows|PLATFORM_Linux|PLATFORM_Mac,

		/** Console platform types */
		PLATFORM_Console	=	PLATFORM_Xenon|PLATFORM_PS3,

		/** These flags will be inherited from parent classes */
		PLATFORM_Inherit	=	PLATFORM_Console|PLATFORM_Linux|PLATFORM_Mac,
	};
}

/**
 * Determines which platform we are currently running on
 */
inline UE3::EPlatformType appGetPlatformType(void)
{
#if _MSC_VER && !XBOX
	return UE3::PLATFORM_Windows;
#elif XBOX
	return UE3::PLATFORM_Xenon;
#elif PS3
	return UE3::PLATFORM_PS3;
#elif __GNUG__
	return UE3::PLATFORM_Linux;
#elif MAC
	return UE3::PLATFORM_Mac;
#endif
}

/** Which platform we're cooking for, PLATFORM_Unknown (0) if not cooking. Defined in Core.cpp. */
extern UE3::EPlatformType	GCookingTarget;


/*-----------------------------------------------------------------------------
	Game/ mod specific directories.
-----------------------------------------------------------------------------*/
/** @name Game or mod specific directories */
//@{
/** 
 * Returns the base directory of the "core" engine that can be shared across
 * several games or across games & mods. Shaders and base localization files
 * e.g. reside in the engine directory.
 *
 * @return engine directory
 */
FString appEngineDir();

/**
 * Returns the directory the root configuration files are located.
 *
 * @return root config directory
 */
FString appEngineConfigDir();

/**
 * Returns the base directory of the current game by looking at the global
 * GGameName variable. This is usually a subdirectory of the installation
 * root directory and can be overridden on the command line to allow self
 * contained mod support.
 *
 * @return base directory
 */
FString appGameDir();

/**
 * Returns the directory the engine uses to look for the leaf ini files. This
 * can't be an .ini variable for obvious reasons.
 *
 * @return config directory
 */
FString appGameConfigDir();

/**
 * Returns the directory the engine uses to output logs. This currently can't 
 * be an .ini setting as the game starts logging before it can read from .ini
 * files.
 *
 * @return log directory
 */
FString appGameLogDir();

/**
 * Returns the directory the engine should save compiled script packages to.
 */
FString appScriptOutputDir();

/**
 * Returns the pathnames for the directories which contain script packages.
 *
 * @param	ScriptPackagePaths	receives the list of directory paths to use for loading script packages 
 */
void appGetScriptPackageDirectories( TArray<FString>& ScriptPackagePaths );

/**
 * A single function to get the list of the script packages that are used by 
 * the current game (as specified by the GAMENAME #define)
 * Licensees need to add their own game(s) to the definition of this function!
 *
 * @param PackageNames					The output array that will contain the package names for this game (with no extension)
 * @param bCanIncludeEditorOnlyPackages	If possible, should editor only packages be included in the list?
 */
void appGetGameScriptPackageNames(TArray<FString>& PackageNames, UBOOL bCanIncludeEditorOnlyPackages);

/**
 * A single function to get the list of the script packages containing native
 * classes that are used by the current game (as specified by the GAMENAME #define)
 * Licensees need to add their own game(s) to the definition of this function!
 *
 * @param PackageNames					The output array that will contain the package names for this game (with no extension)
 * @param bCanIncludeEditorOnlyPackages	If possible, should editor only packages be included in the list?
 */
void appGetGameNativeScriptPackageNames(TArray<FString>& PackageNames, UBOOL bCanIncludeEditorOnlyPackages);

/**
 * A single function to get the list of the script packages that are used by the base engine.
 *
 * @param PackageNames					The output array that will contain the package names for this game (with no extension)
 * @param bCanIncludeEditorOnlyPackages	If possible, should editor only packages be included in the list?
 */
void appGetEngineScriptPackageNames(TArray<FString>& PackageNames, UBOOL bCanIncludeEditorOnlyPackages);

/**
 * Get a list of all packages that may be needed at startup, and could be
 * loaded async in the background when doing seek free loading
 *
 * @param PackageNames The output list of package names
 * @param EngineConfigFilename Optional engine config filename to use to lookup the package settings
 */
void appGetAllPotentialStartupPackageNames(TArray<FString>& PackageNames, const TCHAR* EngineConfigFilename=NULL);

/*-----------------------------------------------------------------------------
	Timing functions.
-----------------------------------------------------------------------------*/

/** @name Timing */
//@{


//@see UnPS3.h for this being defined for PS3 land
#if !DEFINED_appCycles && !defined(PS3)
/** Return number of CPU cycles passed. Origin is arbitrary. */
DWORD appCycles();
#endif

#if STATS
#define clock(Timer)   {Timer -= appCycles();}
#define unclock(Timer) {Timer += appCycles();}
#else
#define clock(Timer)
#define unclock(Timer)
#endif

#if !DEFINED_appSeconds
/** Get time in seconds. Origin is arbitrary. */
DOUBLE appSeconds();
#endif

/** Returns the system time. */
void appSystemTime( INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec );
/** Returns a string of system time. */
FString appSystemTimeString( void );
/** Returns string timestamp.  NOTE: Only one return value is valid at a time! */
const TCHAR* appTimestamp();
/** Sleep this thread for Seconds.  0.0 means release the current timeslice to let other threads get some attention. */
void appSleep( FLOAT Seconds );
/** Sleep this thread infinitely. */
void appSleepInfinite();

//@}

/*-----------------------------------------------------------------------------
	Character type functions.
-----------------------------------------------------------------------------*/
#define UPPER_LOWER_DIFF	32

/** @name Character functions */
//@{
#if PS3 && !defined(UNICODE)
inline TCHAR appToUpper( TCHAR sc )
{
	//@fixme
	ANSICHARU c = static_cast<ANSICHARU>(sc);
#else
inline TCHAR appToUpper( TCHAR c )
{
#endif
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	//@hack - ideally, this would be data driven or use some sort of lookup table
	// some special cases
	switch (c)
	{
	// these are not 32 apart
	case /*TEXT('�')*/ 255: return /*TEXT('�')*/ 159;
	case /*TEXT('�')*/ 156: return /*TEXT('�')*/ 140;


	// characters within the 192 - 255 range which have no uppercase/lowercase equivalents
	case /*TEXT('�')*/ 240: return c;
	case /*TEXT('�')*/ 208: return c;
	case /*TEXT('�')*/ 223: return c;
	case /*TEXT('�')*/ 247: return c;
	}

	if ( (c >= TEXT('a') && c <= TEXT('z')) || (c > /*TEXT('�')*/ 223 && c < /*TEXT('�')*/ 255) )
	{
		return c - UPPER_LOWER_DIFF;
	}

	// no uppercase equivalent
	return c;
}
#if PS3 && !defined(UNICODE)
inline TCHAR appToLower( TCHAR sc )
{
	//@fixme
	ANSICHARU c = static_cast<ANSICHARU>(sc);
#else
inline TCHAR appToLower( TCHAR c )
{
#endif
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	// some special cases
	switch (c)
	{
	// these are not 32 apart
	case /*TEXT('�')*/ 159: return /*TEXT('�')*/ 255;
	case /*TEXT('�')*/ 140: return /*TEXT('�')*/ 156;

	// characters within the 192 - 255 range which have no uppercase/lowercase equivalents
	case /*TEXT('�')*/ 240: return c;
	case /*TEXT('�')*/ 208: return c;
	case /*TEXT('�')*/ 223: return c;
	case /*TEXT('�')*/ 247: return c;
	}

	if ( (c >= /*TEXT('�')*/192 && c < /*TEXT('�')*/ 223) || (c >= TEXT('A') && c <= TEXT('Z')) )
	{
		return c + UPPER_LOWER_DIFF;
	}

	// no lowercase equivalent
	return c;
}
#if PS3 && !defined(UNICODE)
inline UBOOL appIsUpper( TCHAR sc )
{
	//@fixme
	ANSICHARU c = static_cast<ANSICHARU>(sc);
#else
inline UBOOL appIsUpper( TCHAR c )
{
#endif
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	return (c==/*TEXT('�')*/ 159) || (c==/*TEXT('�')*/ 140)	// these are outside the standard range
		|| (c==/*TEXT('�')*/ 240) || (c==/*TEXT('�')*/ 247)	// these have no lowercase equivalents
		|| (c>=TEXT('A') && c<=TEXT('Z')) || (c >= /*TEXT('�')*/ 192 && c <= /*TEXT('�')*/ 223);
}
#if PS3 && !defined(UNICODE)
inline UBOOL appIsLower( TCHAR sc )
{
	//@fixme
	ANSICHARU c = static_cast<ANSICHARU>(sc);
#else
inline UBOOL appIsLower( TCHAR c )
{
#endif
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	return (c==/*TEXT('�')*/ 156) 															// outside the standard range
		|| (c==/*TEXT('�')*/ 215) || (c==/*TEXT('�')*/ 208) || (c==/*TEXT('�')*/ 223)	// these have no lower-case equivalents
#if PS3 && !defined(UNICODE)
		|| (c>=TEXT('a') && c<=TEXT('z')) || (c >=/*TEXT('�')*/ 224);
#else
		|| (c>=TEXT('a') && c<=TEXT('z')) || (c >=/*TEXT('�')*/ 224 && c <= /*TEXT('�')*/ 255);
#endif
}
#if PS3 && !defined(UNICODE)
inline UBOOL appIsAlpha( TCHAR sc )
{
	//@fixme
	ANSICHARU c = static_cast<ANSICHARU>(sc);
#else
inline UBOOL appIsAlpha( TCHAR c )
{
#endif
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	return (c>=TEXT('A') && c<=TEXT('Z')) 
#if PS3 && !defined(UNICODE)
		|| (c>=/*TEXT('�')*/ 192)
#else
		|| (c>=/*TEXT('�')*/ 192 && c<=/*TEXT('�')*/ 255)
#endif
		|| (c>=TEXT('a') && c<=TEXT('z')) 
		|| (c==/*TEXT('�')*/ 159) || (c==/*TEXT('�')*/ 140) || (c==/*TEXT('�')*/ 156);	// these are outside the standard range
}
inline UBOOL appIsDigit( TCHAR c )
{
	return c>=TEXT('0') && c<=TEXT('9');
}
inline UBOOL appIsAlnum( TCHAR c )
{
	return appIsAlpha(c) || (c>=TEXT('0') && c<=TEXT('9'));
}
inline UBOOL appIsWhitespace( TCHAR c )
{
	return c == TEXT(' ') || c == TEXT('\t');
}
inline UBOOL appIsLinebreak( TCHAR c )
{
	//@todo - support for language-specific line break characters
	return c == TEXT('\n');
}

/** Returns nonzero if character is a space character. */
inline UBOOL appIsSpace( TCHAR c )
{
#if UNICODE
    return( iswspace(c) != 0 );
#else
    return( isspace(c) != 0 );
#endif
}

inline UBOOL appIsPunct( TCHAR c )
{
#if UNICODE
	return( iswpunct( c ) != 0 );
#else
	return( ispunct( c ) != 0 );
#endif
}
//@}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/
/** @name String functions */
//@{

/** Returns whether the string is pure ANSI. */
UBOOL appIsPureAnsi( const TCHAR* Str );

/**
* strcpy wrapper
*
* @param Dest - destination string to copy to
* @param Destcount - size of Dest in characters
* @param Src - source string
* @return destination string
*/
inline TCHAR* appStrcpy( TCHAR* Dest, SIZE_T DestCount, const TCHAR* Src )
{
#if USE_SECURE_CRT
	_tcscpy_s( Dest, DestCount, Src );
	return Dest;
#else
	return (TCHAR*)_tcscpy( Dest, Src );
#endif
}

/**
* strcpy wrapper
* (templated version to automatically handle static destination array case)
*
* @param Dest - destination string to copy to
* @param Src - source string
* @return destination string
*/
template<SIZE_T DestCount>
inline TCHAR* appStrcpy( TCHAR (&Dest)[DestCount], const TCHAR* Src ) 
{
	return appStrcpy( Dest, DestCount, Src );
}

/**
* strcat wrapper
*
* @param Dest - destination string to copy to
* @param Destcount - size of Dest in characters
* @param Src - source string
* @return destination string
*/
inline TCHAR* appStrcat( TCHAR* Dest, SIZE_T DestCount, const TCHAR* Src ) 
{ 
#if USE_SECURE_CRT
	_tcscat_s( Dest, DestCount, Src );
	return Dest;
#else
	return (TCHAR*)_tcscat( Dest, Src );
#endif
}

/**
* strcat wrapper
* (templated version to automatically handle static destination array case)
*
* @param Dest - destination string to copy to
* @param Src - source string
* @return destination string
*/
template<SIZE_T DestCount>
inline TCHAR* appStrcat( TCHAR (&Dest)[DestCount], const TCHAR* Src ) 
{ 
	return appStrcat( Dest, DestCount, Src );
}

/**
* strupr wrapper
*
* @param Dest - destination string to convert
* @param Destcount - size of Dest in characters
* @return destination string
*/
inline TCHAR* appStrupr( TCHAR* Dest, SIZE_T DestCount ) 
{
#if USE_SECURE_CRT
	_tcsupr_s( Dest, DestCount );
	return Dest;
#else
	return (TCHAR*)_tcsupr( Dest );
#endif
}

/**
* strupr wrapper
* (templated version to automatically handle static destination array case)
*
* @param Dest - destination string to convert
* @return destination string
*/
template<SIZE_T DestCount>
inline TCHAR* appStrupr( TCHAR (&Dest)[DestCount] ) 
{
	return appStrupr( Dest, DestCount );
}

// ANSI character versions of string manipulation functions

/**
* strcpy wrapper (ANSI version)
*
* @param Dest - destination string to copy to
* @param Destcount - size of Dest in characters
* @param Src - source string
* @return destination string
*/
inline ANSICHAR* appStrcpyANSI( ANSICHAR* Dest, SIZE_T DestCount, const ANSICHAR* Src ) 
{ 
#if USE_SECURE_CRT
	strcpy_s( Dest, DestCount, Src );
	return Dest;
#else
	return (ANSICHAR*)strcpy( Dest, Src );
#endif
}

/**
* strcpy wrapper (ANSI version)
* (templated version to automatically handle static destination array case)
*
* @param Dest - destination string to copy to
* @param Src - source string
* @return destination string
*/
template<SIZE_T DestCount>
inline ANSICHAR* appStrcpyANSI( ANSICHAR (&Dest)[DestCount], const ANSICHAR* Src ) 
{ 
	return appStrcpyANSI( Dest, DestCount, Src );
}

/**
* strcat wrapper (ANSI version)
*
* @param Dest - destination string to copy to
* @param Destcount - size of Dest in characters
* @param Src - source string
* @return destination string
*/
inline ANSICHAR* appStrcatANSI( ANSICHAR* Dest, SIZE_T DestCount, const ANSICHAR* Src ) 
{ 
#if USE_SECURE_CRT
	strcat_s( Dest, DestCount, Src );
	return Dest;
#else
	return (ANSICHAR*)strcat( Dest, Src );
#endif
}

/**
* strcat wrapper (ANSI version)
*
* @param Dest - destination string to copy to
* @param Destcount - size of Dest in characters
* @param Src - source string
* @return destination string
*/
template<SIZE_T DestCount>
inline ANSICHAR* appStrcatANSI( ANSICHAR (&Dest)[DestCount], const ANSICHAR* Src ) 
{ 
	return appStrcatANSI( Dest, DestCount, Src );
}

inline INT appStrlen( const TCHAR* String ) { return _tcslen( String ); }
inline TCHAR* appStrstr( const TCHAR* String, const TCHAR* Find ) { return (TCHAR*)_tcsstr( String, Find ); }
inline TCHAR* appStrchr( const TCHAR* String, INT c ) { return (TCHAR*)_tcschr( String, c ); }
inline TCHAR* appStrrchr( const TCHAR* String, INT c ) { return (TCHAR*)_tcsrchr( String, c ); }
inline INT appStrcmp( const TCHAR* String1, const TCHAR* String2 ) { return _tcscmp( String1, String2 ); }
inline INT appStricmp( const TCHAR* String1, const TCHAR* String2 )  { return _tcsicmp( String1, String2 ); }
inline INT appStrncmp( const TCHAR* String1, const TCHAR* String2, INT Count ) { return _tcsncmp( String1, String2, Count ); }
inline INT appAtoi( const TCHAR* String ) { return _tstoi( String ); }
inline FLOAT appAtof( const TCHAR* String ) { return _tstof( String ); }
inline DOUBLE appAtod( const TCHAR* String ) { return _tcstod( String, NULL ); }
inline INT appStrtoi( const TCHAR* Start, TCHAR** End, INT Base ) { return _tcstoul( Start, End, Base ); }
inline INT appStrnicmp( const TCHAR* A, const TCHAR* B, INT Count ) { return _tcsnicmp( A, B, Count ); }

/**
 * Returns a static string that is full of a variable number of space (or other)
 * characters that can be used to space things out, or calculate string widths
 * Since it is static, only one return value from a call is valid at a time.
 *
 * @param NumCharacters Number of characters to but into the string, max of 255
 * @param Char Optional character to put into the string (defaults to space)
 * 
 * @return The string of NumCharacters characters.
 */
const TCHAR* appSpc( INT NumCharacters, BYTE Char=' ' );

/** 
* Copy a string with length checking. Behavior differs from strncpy in that last character is zeroed. 
*
* @param Dest - destination buffer to copy to
* @param Src - source buffer to copy from
* @param MaxLen - max length of the buffer (including null-terminator)
* @return pointer to resulting string buffer
*/
TCHAR* appStrncpy( TCHAR* Dest, const TCHAR* Src, INT MaxLen );

/** 
* Concatenate a string with length checking.
*
* @param Dest - destination buffer to append to
* @param Src - source buffer to copy from
* @param MaxLen - max length of the buffer
* @return pointer to resulting string buffer
*/
TCHAR* appStrncat( TCHAR* Dest, const TCHAR* Src, INT MaxLen );

/** 
* Copy a string with length checking. Behavior differs from strncpy in that last character is zeroed. 
* (ANSICHAR version) 
*
* @param Dest - destination char buffer to copy to
* @param Src - source char buffer to copy from
* @param MaxLen - max length of the buffer (including null-terminator)
* @return pointer to resulting string buffer
*/
ANSICHAR* appStrncpyANSI( ANSICHAR* Dest, const ANSICHAR* Src, INT MaxLen );

/** Finds string in string, case insensitive, requires non-alphanumeric lead-in. */
const TCHAR* appStrfind(const TCHAR* Str, const TCHAR* Find);

/** 
 * Finds string in string, case insensitive 
 * @param Str The string to look through
 * @param Find The string to find inside Str
 * @return Position in Str if Find was found, otherwise, NULL
 */
const TCHAR* appStristr(const TCHAR* Str, const TCHAR* Find);

/** String CRC. */
DWORD appStrCrc( const TCHAR* Data );
/** String CRC, case insensitive. */
DWORD appStrCrcCaps( const TCHAR* Data );

/** Converts an integer to a string. */
FString appItoa( INT Num );


/** 
* Standard string formatted print. 
* @warning: make sure code using appSprintf allocates enough (>= MAX_SPRINTF) memory for the destination buffer
*/
#define MAX_SPRINTF 1024
VARARG_DECL( INT, static INT, return, appSprintf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(TCHAR* Dest), VARARG_EXTRA(Dest) );
/**
* Standard string formatted print (ANSI version).
* @warning: make sure code using appSprintf allocates enough (>= MAX_SPRINTF) memory for the destination buffer
*/
VARARG_DECL( INT, static INT, return, appSprintfANSI, VARARG_NONE, const ANSICHAR*, VARARG_EXTRA(ANSICHAR* Dest), VARARG_EXTRA(Dest) );

/** Trims spaces from an ascii string by zeroing them. */
void appTrimSpaces( ANSICHAR* String);

/** 
* Duplicates a string using appMalloc.  THE RETURNED STRING MUST BE FREED BY CALLER! 
*
* @param String - null terminated array of tchars to copy
* @return newly allocated and copied tchar array 
*/
TCHAR* appStrdup(const TCHAR* String);

#if USE_SECURE_CRT
#define appSSCANF	_stscanf_s
#else
#define appSSCANF	_stscanf
#endif

typedef int QSORT_RETURN;
typedef QSORT_RETURN(CDECL* QSORT_COMPARE)( const void* A, const void* B );
/** Quick sort. */
void appQsort( void* Base, INT Num, INT Width, QSORT_COMPARE Compare );

/** Case insensitive string hash function. */
inline DWORD appStrihash( const TCHAR* Data )
{
	DWORD Hash=0;
	while( *Data )
	{
		TCHAR Ch = appToUpper(*Data++);
		BYTE  B  = Ch;
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
#if UNICODE
		B        = Ch>>8;
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
#endif
	}
	return Hash;
}
//@}

/*-----------------------------------------------------------------------------
	Parsing functions.
-----------------------------------------------------------------------------*/
/** @name Parsing functions */
//@{
/**
 * Sees if Stream starts with the named command.  If it does,
 * skips through the command and blanks past it.  Returns 1 of match,
 * 0 if not.
 */
UBOOL ParseCommand( const TCHAR** Stream, const TCHAR* Match );
/** Parses a name. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FName& Name );
/** Parses a DWORD. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, DWORD& Value );
/** Parses a globally unique identifier. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FGuid& Guid );
/** Parses a string from a text string. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, TCHAR* Value, INT MaxLen );
/** Parses a byte. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, BYTE& Value );
/** Parses a signed byte. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SBYTE& Value );
/** Parses a WORD. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, WORD& Value );
/** Parses a signed word. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SWORD& Value );
/** Parses a floating-point value. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FLOAT& Value );
/** Parses a signed double word. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, INT& Value );
/** Parses a string. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FString& Value );
/** Parses a quadword. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, QWORD& Value );
/** Parses a signed quadword. */
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SQWORD& Value );
/** Parses a boolean value. */
UBOOL ParseUBOOL( const TCHAR* Stream, const TCHAR* Match, UBOOL& OnOff );
/** Parses an object from a text stream, returning 1 on success */
UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, class UClass* Type, class UObject*& DestRes, class UObject* InParent );
/** Get a line of Stream (everything up to, but not including, CR/LF. Returns 0 if ok, nonzero if at end of stream and returned 0-length string. */
UBOOL ParseLine( const TCHAR** Stream, TCHAR* Result, INT MaxLen, UBOOL Exact=0 );
/** Get a line of Stream (everything up to, but not including, CR/LF. Returns 0 if ok, nonzero if at end of stream and returned 0-length string. */
UBOOL ParseLine( const TCHAR** Stream, FString& Resultd, UBOOL Exact=0 );
/** Grabs the next space-delimited string from the input stream. If quoted, gets entire quoted string. */
UBOOL ParseToken( const TCHAR*& Str, TCHAR* Result, INT MaxLen, UBOOL UseEscape );
/** Grabs the next space-delimited string from the input stream. If quoted, gets entire quoted string. */
UBOOL ParseToken( const TCHAR*& Str, FString& Arg, UBOOL UseEscape );
/** Grabs the next space-delimited string from the input stream. If quoted, gets entire quoted string. */
FString ParseToken( const TCHAR*& Str, UBOOL UseEscape );
/** Get next command.  Skips past comments and cr's. */
void ParseNext( const TCHAR** Stream );
/** Checks if a command-line parameter exists in the stream. */
UBOOL ParseParam( const TCHAR* Stream, const TCHAR* Param );
//@}

/*-----------------------------------------------------------------------------
	Array functions.
-----------------------------------------------------------------------------*/

/** @name Array functions
 * Core functions depending on TArray and FString.
 */
//@{
void appBufferToString( FString& Result, const BYTE* Buffer, INT Size );
UBOOL appLoadFileToArray( TArray<BYTE>& Result, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
UBOOL appLoadFileToString( FString& Result, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
UBOOL appSaveArrayToFile( const TArray<BYTE>& Array, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
UBOOL appSaveStringToFile( const FString& String, const TCHAR* Filename, FFileManager* FileManager=GFileManager );
//@}

/*-----------------------------------------------------------------------------
	Memory functions.
-----------------------------------------------------------------------------*/

/** @name Memory functions */
//@{
/** Copies count bytes of characters from Src to Dest. If some regions of the source
 * area and the destination overlap, memmove ensures that the original source bytes
 * in the overlapping region are copied before being overwritten.  NOTE: make sure
 * that the destination buffer is the same size or larger than the source buffer!
 */
void* appMemmove( void* Dest, const void* Src, INT Count );
INT appMemcmp( const void* Buf1, const void* Buf2, INT Count );
UBOOL appMemIsZero( const void* V, int Count );
DWORD appMemCrc( const void* Data, INT Length, DWORD CRC=0 );
void appMemswap( void* Ptr1, void* Ptr2, DWORD Size );

/**
 * Sets the first Count chars of Dest to the character C.
 */
#define appMemset( Dest, C, Count )			memset( Dest, C, Count )

#ifndef DEFINED_appMemcpy
	#define appMemcpy( Dest, Src, Count )	memcpy( Dest, Src, Count )
#endif

#ifndef DEFINED_appMemzero
	#define appMemzero( Dest, Count )		memset( Dest, 0, Count )
#endif

/** Helper function called on first allocation to create and initialize GMalloc */
extern void GCreateMalloc();

//
// C style memory allocation stubs.
//
inline void* appMalloc( DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT ) 
{ 
	if( !GMalloc )
	{
		GCreateMalloc();
	}
	return GMalloc->Malloc( Count, Alignment );
}
inline void* appRealloc( void* Original, DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT ) 
{ 
	if( !GMalloc )
	{
		GCreateMalloc();	
	}
	return GMalloc->Realloc( Original, Count, Alignment );
}	
inline void appFree( void* Original )
{
	if( !GMalloc )
	{
		GCreateMalloc();
	}
	return GMalloc->Free( Original );
}
inline void* appPhysicalAlloc( DWORD Count, ECacheBehaviour CacheBehaviour = CACHE_WriteCombine ) 
{ 
	if( !GMalloc )
	{
		GCreateMalloc();
	}
	return GMalloc->PhysicalAlloc( Count, CacheBehaviour );
}
inline void appPhysicalFree( void* Original )
{
	if( !GMalloc )
	{
		GCreateMalloc();	
	}
	return GMalloc->PhysicalFree( Original );
}

//
// C++ style memory allocation.
//
inline void* operator new( size_t Size )
{
	return appMalloc( Size );
}
inline void operator delete( void* Ptr )
{
	appFree( Ptr );
}

inline void* operator new[]( size_t Size )
{
	return appMalloc( Size );
}
inline void operator delete[]( void* Ptr )
{
	appFree( Ptr );
}
//@}

/** 
 * This will update the passed in FMemoryChartEntry with the platform specific data
 *
 * @param FMemoryChartEntry the struct to fill in
 **/
void appUpdateMemoryChartStats( struct FMemoryChartEntry& MemoryEntry );


/*-----------------------------------------------------------------------------
	Math.
-----------------------------------------------------------------------------*/

#if !XBOX
BYTE appCeilLogTwo( DWORD Arg );
#endif

#ifndef DEFINED_FloatSelect
/**
 * Returns value based on comparand. The main purpose of this function is to avoid
 * branching based on floating point comparison which can be avoided via compiler
 * intrinsics. This is the platform agnostic implementation so we need to branch.
 *
 * Please note that we don't define what happens in the case of NaNs as there might
 * be platform specific differences.
 *
 * @param	Comparand		Comparand the results are based on
 * @param	ValueGEZero		Return value if Comparand >= 0
 * @param	ValueLTZero		Return value if Comparand > 0
 *
 * @return	ValueGEZero if Comparand >= 0, ValueLTZero otherwise
 */
FORCEINLINE FLOAT FloatSelect( FLOAT Comparand, FLOAT ValueGEZero, FLOAT ValueLTZero )
{
	return Comparand >= 0.f ? ValueGEZero : ValueLTZero;
}
/**
 * Returns value based on comparand. The main purpose of this function is to avoid
 * branching based on floating point comparison which can be avoided via compiler
 * intrinsics. This is the platform agnostic implementation so we need to branch.
 *
 * Please note that we don't define what happens in the case of NaNs as there might
 * be platform specific differences.
 *
 * @param	Comparand		Comparand the results are based on
 * @param	ValueGEZero		Return value if Comparand >= 0
 * @param	ValueLTZero		Return value if Comparand > 0
 *
 * @return	ValueGEZero if Comparand >= 0, ValueLTZero otherwise
 */
FORCEINLINE DOUBLE FloatSelect( DOUBLE Comparand, DOUBLE ValueGEZero, DOUBLE ValueLTZero )
{
	return Comparand >= 0.f ? ValueGEZero : ValueLTZero;
}
#endif

/*-----------------------------------------------------------------------------
	MD5 functions.
-----------------------------------------------------------------------------*/

/** @name MD5 functions */
//@{
//
// MD5 Context.
//
struct FMD5Context
{
	DWORD state[4];
	DWORD count[2];
	BYTE buffer[64];
};

//
// MD5 functions.
//!!it would be cool if these were implemented as subclasses of
// FArchive.
//
void appMD5Init( FMD5Context* context );
void appMD5Update( FMD5Context* context, BYTE* input, INT inputLen );
void appMD5Final( BYTE* digest, FMD5Context* context );
void appMD5Transform( DWORD* state, BYTE* block );
void appMD5Encode( BYTE* output, DWORD* input, INT len );
void appMD5Decode( DWORD* output, BYTE* input, INT len );
//@}


/*-----------------------------------------------------------------------------
	SHA-1 functions.
-----------------------------------------------------------------------------*/

/*
 *	NOTE:
 *	100% free public domain implementation of the SHA-1 algorithm
 *	by Dominik Reichl <dominik.reichl@t-online.de>
 *	Web: http://www.dominik-reichl.de/
 */


typedef union
{
	BYTE  c[64];
	DWORD l[16];
} SHA1_WORKSPACE_BLOCK;

class FSHA1
{
public:

	// Constructor and Destructor
	FSHA1();
	~FSHA1();

	DWORD m_state[5];
	DWORD m_count[2];
	DWORD __reserved1[1];
	BYTE  m_buffer[64];
	BYTE  m_digest[20];
	DWORD __reserved2[3];

	void Reset();

	// Update the hash value
	void Update(BYTE *data, DWORD len);

	// Finalize hash and report
	void Final();

	// Report functions: as pre-formatted and raw data
	void GetHash(BYTE *puDest);

	/**
	 * Calculate the hash on a single block and return it
	 *
	 * @param Data Input data to hash
	 * @param DataSize Size of the Data block
	 * @param OutHash Resulting hash value (20 byte buffer)
	 */
	static void HashBuffer(void* Data, DWORD DataSize, BYTE* OutHash);

private:
	// Private SHA-1 transformation
	void Transform(DWORD *state, BYTE *buffer);

	// Member variables
	BYTE m_workspace[64];
	SHA1_WORKSPACE_BLOCK *m_block; // SHA1 pointer to the byte array above
};

/**
 * Gets the stored SHA hash from the platform, if it exists. This function
 * must be able to be called from any thread.
 *
 * @param Pathname Pathname to the file to get the SHA for
 * @param Hash 20 byte array that receives the hash
 *
 * @return TRUE if the hash was found, FALSE otherwise
 */
UBOOL appGetFileSHAHash(const TCHAR* Pathname, BYTE Hash[20]);

/**
 * Callback that is called if the asynchronous SHA verification fails
 * This will be called from a pooled thread.
 *
 * @param FailedPathname Pathname of file that failed to verify
 * @param bFailedDueToMissingHash TRUE if the reason for the failure was that the hash was missing, and that was set as being an error condition
 */
void appOnFailSHAVerification(const TCHAR* FailedPathname, UBOOL bFailedDueToMissingHash);

/*-----------------------------------------------------------------------------
	Compression.
-----------------------------------------------------------------------------*/

/**
 * Flags controlling [de]compression
 */
enum ECompressionFlags
{
	/** No compression																*/
	COMPRESS_None					= 0,
	/** Compress with ZLIB															*/
	COMPRESS_ZLIB 					= 1,
	/** Compress with LZO															*/
	COMPRESS_LZO 					= 2,
	/** Prefer compression that compresses smaller (ONLY VALID FOR COMPRESSION)		*/
	COMPRESS_BiasMemory 			= 4,
	/** Prefer compression that compresses faster (ONLY VALID FOR COMPRESSION)		*/
	COMPRESS_BiasSpeed				= 8,
};

/**
 * Chunk size serialization code splits data into. The loading value CANNOT be changed without resaving all
 * compressed data which is why they are split into two separate defines.
 */
#define LOADING_COMPRESSION_CHUNK_SIZE_PRE_369  32768
#define LOADING_COMPRESSION_CHUNK_SIZE			131072
#define SAVING_COMPRESSION_CHUNK_SIZE			LOADING_COMPRESSION_CHUNK_SIZE

/**
 * Thread-safe abstract compression routine. Compresses memory from uncompressed buffer and writes it to compressed
 * buffer. Updates CompressedSize with size of compressed data. Compression controlled by the passed in flags.
 *
 * @param	Flags						Flags to control what method to use and optionally control memory vs speed
 * @param	CompressedBuffer			Buffer compressed data is going to be written to
 * @param	CompressedSize	[in/out]	Size of CompressedBuffer, at exit will be size of compressed data
 * @param	UncompressedBuffer			Buffer containing uncompressed data
 * @param	UncompressedSize			Size of uncompressed data in bytes
 * @return TRUE if compression succeeds, FALSE if it fails because CompressedBuffer was too small or other reasons
 */
UBOOL appCompressMemory( ECompressionFlags Flags, void* CompressedBuffer, INT& CompressedSize, void* UncompressedBuffer, INT UncompressedSize );

/**
 * Thread-safe abstract decompression routine. Uncompresses memory from compressed buffer and writes it to uncompressed
 * buffer. UncompressedSize is expected to be the exact size of the data after decompression.
 *
 * @param	Flags						Flags to control what method to use to decompress
 * @param	UncompressedBuffer			Buffer containing uncompressed data
 * @param	UncompressedSize			Size of uncompressed data in bytes
 * @param	CompressedBuffer			Buffer compressed data is going to be written to
 * @param	CompressedSize				Size of CompressedBuffer data in bytes
 * @return TRUE if compression succeeds, FALSE if it fails because CompressedBuffer was too small or other reasons
 */
UBOOL appUncompressMemory( ECompressionFlags Flags, void* UncompressedBuffer, INT UncompressedSize, void* CompressedBuffer, INT CompressedSize );


/*-----------------------------------------------------------------------------
	Game Name.
-----------------------------------------------------------------------------*/

const TCHAR* appGetGameName();

#if WITH_PANORAMA || _XBOX
/** Returns the title id of this game */
DWORD appGetTitleId(void);
#endif

#if WITH_GAMESPY
/**
 * Returns the game name to use with GameSpy
 */
const TCHAR* appGetGameSpyGameName(void);

/**
 * Returns the secret key used by this game
 */
const TCHAR* appGetGameSpySecretKey(void);
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

#endif // __UNFILE_H__
