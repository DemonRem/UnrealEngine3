#ifndef NX_FOUNDATION_NX
#define NX_FOUNDATION_NX
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/

/** \addtogroup foundation
  @{
*/

/**
DLL export macros
*/
#ifndef NX_C_EXPORT
	#define NX_C_EXPORT extern "C"
#endif

#ifndef NX_CALL_CONV
	#if defined WIN32
		#define NX_CALL_CONV __cdecl
	#elif defined LINUX
		#define NX_CALL_CONV
    #elif defined __APPLE__
        #define NX_CALL_CONV
	#elif defined __CELLOS_LV2__
		#define NX_CALL_CONV
	#elif defined _XBOX
        #define NX_CALL_CONV
	#else
		#error custom definition of NX_CALL_CONV for your OS needed!
	#endif
#endif

#if	  defined NX32
#elif defined NX64
#elif defined WIN32
	#ifdef NX64
		#error PhysX SDK: Platforms pointer size ambiguous!  The defines WIN32 and NX64 are in conflict.  
	#endif
	#define NX32
#elif defined WIN64
	#ifdef NX32
		#error PhysX SDK: Platforms pointer size ambiguous!  The defines WIN64 and NX32 are in conflict.  
	#endif
	#define NX64
#elif defined __CELLOS_LV2__
	#ifdef __LP32__
            #define NX32
    #else
            #define NX64
    #endif
#elif defined _XBOX
	#define NX32
#else
	#error PhysX SDK: Platforms pointer size ambiguous.  Please define NX32 or Nx64 in the compiler settings!
#endif


#if !defined __CELLOS_LV2__
	#define NX_COMPILE_TIME_ASSERT(exp)	extern char NX_CompileTimeAssert[ size_t((exp) ? 1 : -1) ]
#else
    // GCC4 don't like the line above
	#define _CELL_NX_COMPILE_TIME_NAME_(x) NX_CompileTimeAssert ## x
	#define _CELL_NX_COMPILE_TIME_NAME(x) _CELL_NX_COMPILE_TIME_NAME_(x)
	#define NX_COMPILE_TIME_ASSERT(exp) extern char _CELL_NX_COMPILE_TIME_NAME(__LINE__)[ (exp) ? 1 : -1]
#endif


#if _MSC_VER
	#define NX_MSVC		// Compiling with VC++
	#if _MSC_VER >= 1300
		#define NX_VC7		// Compiling with VC7
	#else
		#define NX_VC6		// Compiling with VC6
		#define __FUNCTION__	"Undefined"
	#endif
#endif

/**
 Nx SDK misc defines.
*/

//NX_INLINE
#if (_MSC_VER>=1000)
	#define NX_INLINE __forceinline	//alternative is simple inline
	#pragma inline_depth( 255 )

	#include <string.h>
	#include <stdlib.h>
	#pragma intrinsic(memcmp)
	#pragma intrinsic(memcpy)
	#pragma intrinsic(memset)
	#pragma intrinsic(strcat)
	#pragma intrinsic(strcmp)
	#pragma intrinsic(strcpy)
	#pragma intrinsic(strlen)
	#pragma intrinsic(abs)
	#pragma intrinsic(labs)
#elif defined(__MWERKS__) 
	//optional: #pragma always_inline on
	#define NX_INLINE inline
#else
	#define NX_INLINE inline
#endif

	#define NX_DELETE(x)	delete x
	#define NX_DELETE_SINGLE(x)	if (x) { delete x;		x = NULL; }
	#define NX_DELETE_ARRAY(x)	if (x) { delete []x;	x = NULL; }

	template<class Type> NX_INLINE void NX_Swap(Type& a, Type& b)
		{
		const Type c = a; a = b; b = c;
		}

/**
\brief Error codes

These error codes are passed to #NxUserOutputStream

@see NxUserOutputStream
*/

enum NxErrorCode
	{
	/**
	\brief no error
	*/
	NXE_NO_ERROR			= 0,
	/**
	\brief method called with invalid parameter(s)
	*/
	NXE_INVALID_PARAMETER	= 1,
	/**
	\brief method was called at a time when an operation is not possible
	*/
	NXE_INVALID_OPERATION	= 2,
	/**
	\brief method failed to allocate some memory
	*/
	NXE_OUT_OF_MEMORY		= 3,
	/**
	\brief The library failed for some reason.
	
	Usually because you have passed invalid values like NaNs into the system, which are not checked for.
	*/
	NXE_INTERNAL_ERROR		= 4,

	/**
	\brief an assertion failed.
	*/
	NXE_ASSERTION			= 107,

	/**
	\brief An informational message.

	Only emitted when NX_USER_DEBUG_MODE is defined.
	*/
	NXE_DB_INFO				= 205,
	/**
	\brief a warning message for the user to help with debugging

	Only emitted when NX_USER_DEBUG_MODE is defined.
	*/
	NXE_DB_WARNING			= 206,
	/**
	\brief the message should simply be printed without any additional infos (line number, etc).

	Only emitted when NX_USER_DEBUG_MODE is defined.
	*/
	NXE_DB_PRINT			= 208

	};


#if _MSC_VER
//get rid of browser info warnings 
#pragma warning( disable : 4786 )  //identifier was truncated to '255' characters in the debug information
#pragma warning( disable : 4251 )  //class needs to have dll-interface to be used by clients of class
#endif

//files to always include:
#include "NxSimpleTypes.h"
#include "NxAssert.h"
#ifdef LINUX
#include <time.h>
#elif __APPLE__
#include <time.h>
#elif __CELLOS_LV2__
	#include <string.h>
#endif

#define NX_DEBUG_MALLOC 0

// Don't use inline for alloca !!!
#ifdef WIN32
	#include <malloc.h>
	#define NxAlloca(x)	_alloca(x)
#elif LINUX
	#include <malloc.h>
	#define NxAlloca(x)	alloca(x)
#elif __APPLE__
	#include <alloca.h>
	#include <stdlib.h>
	#define NxAlloca(x)	alloca(x)
#elif __CELLOS_LV2__
	#include <alloca.h>
	#include <stdlib.h>
	#define NxAlloca(x)	alloca(x)
#elif _XBOX
	#include <malloc.h>
	#define NxAlloca(x)	_alloca(x)
#endif

/**
\brief Used to specify a thread priority.
*/
enum NxThreadPriority 
{
	/**
	\brief High priority
	*/
	NX_TP_HIGH		=0,

	/**
	\brief Normal/default priority
	*/
	NX_TP_NORMAL	=1,

	/**
	\brief Low priority.
	*/
	NX_TP_LOW		=2,

	NX_TP_FORCE_DWORD = 0xffFFffFF


};

 /** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
