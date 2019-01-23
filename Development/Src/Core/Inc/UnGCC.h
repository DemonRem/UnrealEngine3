/*=============================================================================
	UnGnuG.h: Unreal definitions for Visual C++ SP2 running under Win32.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*----------------------------------------------------------------------------
	Platform compiler definitions.
----------------------------------------------------------------------------*/

#include <string.h>
#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <stdarg.h>

/*----------------------------------------------------------------------------
	Platform specifics types and defines.
----------------------------------------------------------------------------*/

// Generates GCC version like this:  xxyyzz (eg. 030401)
// xx: major version, yy: minor version, zz: patch level
#ifdef __GNUC__
#	define GCC_VERSION	(__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL)
#endif

// Undo any defines.
#undef NULL
#undef BYTE
#undef WORD
#undef DWORD
#undef INT
#undef FLOAT
#undef MAXBYTE
#undef MAXWORD
#undef MAXDWORD
#undef MAXINT
#undef CDECL

// Make sure HANDLE is defined.
#ifndef _WINDOWS_
	#define HANDLE void*
	#define HINSTANCE void*
#endif

// Sizes.
enum {DEFAULT_ALIGNMENT = 8 }; // Default boundary to align memory allocations on.

#define RENDER_DATA_ALIGNMENT DEFAULT_ALIGNMENT // the value to align some renderer bulk data to

// Optimization macros (preceeded by #pragma).
#define DISABLE_OPTIMIZATION optimize("",off)
#ifdef _DEBUG
	#define ENABLE_OPTIMIZATION  optimize("",off)
#else
	#define ENABLE_OPTIMIZATION  optimize("",on)
#endif

// Function type macros.
#define VARARGS     					/* Functions with variable arguments */
#define CDECL	    					/* Standard C function */
#define STDCALL						/* Standard calling convention */
// @todo gcc: is there a gcc forceinline?
#define FORCEINLINE inline			/* Force code to be inline */
// @todo gcc: is there a gcc noinline?
#define FORCENOINLINE
#define ZEROARRAY                           /* Zero-length arrays in structs */

#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE)
#define VSNPRINTF vsnprintf
#define VSNPRINTFA vsnprintf
#else
#define VSNPRINTF wvsnprintf
#define VSNPRINTFA vsnprintf
#endif

// Not supported
#define RESTRICT

#define GET_VARARGS(msg,msgsize,len,lastarg,fmt) {va_list ap; va_start(ap,lastarg);VSNPRINTF(msg,len,fmt,ap);}
#define GET_VARARGS_ANSI(msg,msgsize,len,lastarg,fmt) {va_list ap; va_start(ap,lastarg);vsnprintf(msg,len,fmt,ap);}
#define GET_VARARGS_RESULT(msg,msgsize,len,lastarg,fmt,result) {va_list ap; va_start(ap, lastarg); INT size=VSNPRINTF(msg,len+1,fmt,ap); result=size>len?-1:size; }

// Compiler name.
#ifdef _DEBUG
	#define COMPILER "Compiled with Visual C++ Debug"
#else
	#define COMPILER "Compiled with Visual C++"
#endif

// Unsigned base types.
typedef unsigned char			BYTE;		// 8-bit  unsigned.
typedef unsigned short			WORD;		// 16-bit unsigned.
typedef unsigned int			UINT;		// 32-bit unsigned.
typedef unsigned int			DWORD;		// 32-bit unsigned.
typedef unsigned long long int	QWORD;		// 64-bit unsigned.

// Signed base types.
typedef	signed char				SBYTE;		// 8-bit  signed.
typedef signed short			SWORD;		// 16-bit signed.
typedef signed int  			INT;		// 32-bit signed.
typedef signed int				LONG;		// 32-bit signed.
typedef signed long long int	SQWORD;		// 64-bit signed.

// Character types.
typedef char					ANSICHAR;	// An ANSI character.
typedef unsigned short			UNICHAR;	// A unicode character.
typedef unsigned char			ANSICHARU;	// An ANSI character.
typedef unsigned short			UNICHARU;	// A unicode character.

// Other base types.
typedef signed int				UBOOL;		// Boolean 0 (false) or 1 (true).
typedef float					FLOAT;		// 32-bit IEEE floating point.
typedef double					DOUBLE;		// 64-bit IEEE double.
typedef unsigned long			SIZE_T;     // Corresponds to C SIZE_T.
#ifdef _WIN64
typedef unsigned __int64		PTRINT;		// Integer large enough to hold a pointer.
#elif defined(__LP32__)
typedef unsigned int			PTRINT;		// Integer large enough to hold a pointer.
#else
typedef unsigned long			PTRINT;		// Integer large enough to hold a pointer.
#endif

// Bitfield type.
typedef unsigned int			BITFIELD;	// For bitfields.

#define DECLARE_UINT64(x)	x##ULL

// Make sure characters are unsigned.
#ifdef _CHAR_UNSIGNED
	#error "Bad VC++ option: Characters must be signed"
#endif

// No asm if not compiling for x86.
#if !(defined _M_IX86)
	#undef ASM_X86
	#define ASM_X86 0
#else
	#define ASM_X86 1
#endif

#if defined(__x86_64__) || defined(__i386__)
	#define __INTEL_BYTE_ORDER__ 1
#else
	#define __INTEL_BYTE_ORDER__ 0
#endif

#if !defined(PLATFORM_64BITS) && !defined(PLATFORM_32BITS)
#if defined (__x86_64__)
    #define PLATFORM_64BITS 1
#else
    #define PLATFORM_32BITS 1
#endif
#endif

// DLL file extension.
#define DLLEXT TEXT(".dll")

// Pathnames.
#define PATH(s) s

// NULL.
#define NULL 0

#define FALSE 0
#define TRUE 1

// Platform support options.
#define FORCE_ANSI_LOG           1

// OS unicode function calling.
#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE)
	#define TCHAR ANSICHAR
	#define TCHAR_CALL_OS(funcW,funcA) (funcA)
	#define TCHAR_TO_ANSI(str) str
	#define ANSI_TO_TCHAR(str) str
#elif defined(NO_ANSI_OS_SUPPORT)
	#define TCHAR UNICHAR
	#define TCHAR_CALL_OS(funcW,funcA) (funcW)
	#define TCHAR_TO_ANSI(str) str
	#define ANSI_TO_TCHAR(str) str
#else
	//@todo -- implement Unicode to Ansi conversion routines
	#define TCHAR_CALL_OS(funcW,funcA) (GUnicodeOS ? (funcW) : (funcA))
#endif

// Strings.
#define LINE_TERMINATOR TEXT("\n")
#define PATH_SEPARATOR TEXT("/")
#define appIsPathSeparator( Ch )	((Ch) == TEXT('/') || (Ch) == TEXT('\\'))


// defines for the "standard" unicode-safe string functions
#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE)
#define _tcscpy strcpy
#define _tcslen strlen
#define _tcsstr strstr
#define _tcschr strchr
#define _tcsrchr strrchr
#define _tcscat strcat
#define _tcscmp strcmp
#define _tcsicmp strcasecmp
#define _tcsncmp strncmp
#define _tcsupr gccupr //strupr
#define _tcstoul strtoul
#define _tcsnicmp strncasecmp
#define _tstoi atoi
#define _tstof atof
#define _tcstod strtod
#else
#endif

// Alignment.
#define GCC_BITFIELD_MAGIC __attribute__((packed,aligned(sizeof(BITFIELD))))
#define GCC_PACK(n) __attribute__((packed,aligned(n)))
#define GCC_ALIGN(n) __attribute__((aligned(n)))
#define MS_ALIGN(n)

// GCC doesn't support __noop
#define COMPILER_SUPPORTS_NOOP 0

/*----------------------------------------------------------------------------
	Globals.
----------------------------------------------------------------------------*/

// System identification.
extern "C"
{
	extern HINSTANCE      hInstance;
}

#define MAX_PATH	128 // @todo appCreateBitmap needs this - what is a valid number for all platforms?
/*----------------------------------------------------------------------------
	Math functions.
----------------------------------------------------------------------------*/

const FLOAT	SRandTemp = 1.f;
extern INT GSRandSeed;

inline INT appTrunc( FLOAT F )
{
	return (INT)F;
//	return (INT)truncf(F);
}

inline FLOAT appExp( FLOAT Value ) { return expf(Value); }
inline FLOAT appLoge( FLOAT Value ) {	return logf(Value); }
inline FLOAT appFmod( FLOAT Y, FLOAT X ) { return fmodf(Y,X); }
inline FLOAT appSin( FLOAT Value ) { return sinf(Value); }
inline FLOAT appAsin( FLOAT Value ) { return asinf(Value); }
inline FLOAT appCos( FLOAT Value ) { return cosf(Value); }
inline FLOAT appAcos( FLOAT Value ) { return acosf(Value); }
inline FLOAT appTan( FLOAT Value ) { return tanf(Value); }
inline FLOAT appAtan( FLOAT Value ) { return atanf(Value); }
inline FLOAT appAtan2( FLOAT Y, FLOAT X ) { return atan2f(Y,X); }
inline FLOAT appSqrt( FLOAT Value );
inline FLOAT appPow( FLOAT A, FLOAT B ) { return powf(A,B); }
inline UBOOL appIsNaN( FLOAT A ) { return isnan(A) != 0; }
inline UBOOL appIsFinite( FLOAT A ) { return TRUE; }		//@todo: implement
inline INT appFloor( FLOAT F );
inline INT appCeil( FLOAT Value ) { return appTrunc(ceilf(Value)); }
inline INT appRand() { return rand(); }
inline FLOAT appCopySign( FLOAT A, FLOAT B ) { return copysignf(A,B); }
inline void appRandInit(INT Seed) { srand( Seed ); }
inline void appSRandInit( INT Seed ) { GSRandSeed = Seed; }

inline FLOAT appFractional( FLOAT Value ) { return Value - appTrunc( Value ); }

inline FLOAT appSRand() 
{ 
	GSRandSeed = (GSRandSeed * 196314165) + 907633515; 
	//@todo fix type aliasing
	FLOAT Result;
	*(INT*)&Result = (*(INT*)&SRandTemp & 0xff800000) | (GSRandSeed & 0x007fffff);
	return appFractional(Result); 
} 

//@todo: rand() is currently prohibitively expensive. We need a fast general purpose solution that isn't as bad as appSRand()
inline FLOAT appFrand() { return appSRand(); }

inline INT appRound( FLOAT F )
{
	return appTrunc(roundf(F));
}

inline INT appFloor( FLOAT F )
{
	return appTrunc(floorf(F));
}

inline FLOAT appInvSqrt( FLOAT F )
{
	return 1.0f / sqrtf(F);
}

inline FLOAT appInvSqrtEst( FLOAT F )
{
	return appInvSqrt( F );
}

inline FLOAT appSqrt( FLOAT F )
{
	return sqrtf(F);
}

inline TCHAR* gccupr(TCHAR* str)
{
	for (TCHAR* c = str; *c; c++)
	{
		*c = toupper(*c);
	}
	return str;
}

/**
 * Counts the number of leading zeros in the bit representation of the value
 *
 * @param Value the value to determine the number of leading zeros for
 *
 * @return the number of zeros before the first "on" bit
 */
inline DWORD appCountLeadingZeros(DWORD Value)
{
	if (Value == 0)
	{
		return 32;
	}

	DWORD NumZeros = 0;

	while ((Value & 0x80000000) == 0)
	{
		++NumZeros;
		Value <<= 1;
	}

	return NumZeros;
}

/**
 * Computes the base 2 logarithm for an integer value that is greater than 0.
 * The result is rounded down to the nearest integer.
 *
 * @param Value the value to compute the log of
 */
inline DWORD appFloorLog2(DWORD Value)
{
	return 31 - appCountLeadingZeros(Value);
}

#if defined(__PPC__)
#define DEFINED_appSeconds 1
extern DOUBLE GSecondsPerCycle;
extern QWORD GNumTimingCodeCalls;
inline DOUBLE appSeconds()
{
#if !FINAL_RELEASE
	GNumTimingCodeCalls++;
#endif
	DWORD L,H,T;
    do
    {
		__asm__ __volatile__( "mftbu %0" : "=r" (T) );
		__asm__ __volatile__( "mftb %0" : "=r" (L) );
		__asm__ __volatile__( "mftbu %0" : "=r" (H) );
    } while (T != H);  // in case high register incremented during read.

	//@warning: add big number to make bugs apparent.
	return ((DOUBLE)L +  4294967296.0 * (DOUBLE)H) * GSecondsPerCycle + 16777216.0;
}
inline DWORD appCycles()
{
#if !FINAL_RELEASE
	GNumTimingCodeCalls++;
#endif
	DWORD L;
	__asm__ __volatile__( "mftb %0" : "=r" (L) );
	return L;
}
#endif

/** 
* Returns whether the line can be broken between these two characters
*/
UBOOL appCanBreakLineAt( TCHAR Previous, TCHAR Current );

/*----------------------------------------------------------------------------
	Misc functions.
----------------------------------------------------------------------------*/

/**
 * Handles IO failure by ending gameplay.
 *
 * @param Filename	If not NULL, name of the file the I/O error occured with
 */
FORCEINLINE void appHandleIOFailure( const TCHAR* Filename )
{
	appErrorf(TEXT("appHandleIOFailure not implemented"));
}

/**
 * Push a marker for external profilers.
 *
 * @param MarkerName A descriptive name for the marker to display in the profiler
 */
inline void appPushMarker( const TCHAR* MarkerName )
{
	//@TODO
}

/**
 * Pop the previous marker for external profilers.
 */
inline void appPopMarker( )
{
	//@TODO
}

/*----------------------------------------------------------------------------
	Memory functions.
----------------------------------------------------------------------------*/

#define appAlloca(size) ((size==0) ? 0 : alloca((size+7)&~7))

/**
 * Enforces strict memory load/store ordering across the memory barrier call.
 */
FORCEINLINE void appMemoryBarrier()
{
	//@todo ps3 gcc: do we need to implement this? x86 doesn't.
	__lwsync();
}

// This is a hack, but MUST be defined for 64-bit platforms right now! --ryan.
#if PLATFORM_64BITS

#define SERIAL_POINTER_INDEX 1
#define MAX_SERIALIZED_POINTERS (1024 * 128)
extern void *GSerializedPointers[MAX_SERIALIZED_POINTERS];
extern INT GTotalSerializedPointers;
INT SerialPointerIndex(void *ptr);

#endif

/** 
 * Support functions for overlaying an object/name pointer onto an index (like in script code
 */
inline DWORD appPointerToDWORD(void* Pointer)
{
#if SERIAL_POINTER_INDEX
	return SerialPointerIndex(Pointer);
#else
	return (DWORD)Pointer;
#endif
}

inline void* appDWORDToPointer(DWORD Value)
{
#if SERIAL_POINTER_INDEX
	return GSerializedPointers[Value];
#else
	return (void*)Value;
#endif
}


/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
