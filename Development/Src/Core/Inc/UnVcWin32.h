/*=============================================================================
	UnVcWin32.h: Unreal definitions for Visual C++ SP2 running under Win32.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*----------------------------------------------------------------------------
	Platform compiler definitions.
----------------------------------------------------------------------------*/

#define __WIN32__					1
#define __INTEL__					1
#define __INTEL_BYTE_ORDER__		1

#pragma pack(push,8)
#ifndef STRICT
#define STRICT
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#undef PF_MAX
#pragma pack(pop)

/*----------------------------------------------------------------------------
	Platform specifics types and defines.
----------------------------------------------------------------------------*/

// Undo any Windows defines.
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
#undef PlaySound

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
#define VARARGS     __cdecl					/* Functions with variable arguments */
#define CDECL	    __cdecl					/* Standard C function */
#define STDCALL		__stdcall				/* Standard calling convention */
#define FORCEINLINE __forceinline			/* Force code to be inline */
#define FORCENOINLINE __declspec(noinline)	/* Force code to NOT be inline */
#define ZEROARRAY                           /* Zero-length arrays in structs */

// Compiler name.
#ifdef _DEBUG
	#define COMPILER "Compiled with Visual C++ Debug"
#else
	#define COMPILER "Compiled with Visual C++"
#endif

// MS compiler does support __noop
#define COMPILER_SUPPORTS_NOOP 1

#define RESTRICT __restrict


// Unsigned base types.
typedef unsigned char		BYTE;		// 8-bit  unsigned.
typedef unsigned short		WORD;		// 16-bit unsigned.
typedef unsigned int		UINT;		// 32-bit unsigned.
typedef unsigned long		DWORD;		// 32-bit unsigned.
typedef unsigned __int64	QWORD;		// 64-bit unsigned.

// Signed base types.
typedef	signed char			SBYTE;		// 8-bit  signed.
typedef signed short		SWORD;		// 16-bit signed.
typedef signed int  		INT;		// 32-bit signed.
typedef signed __int64		SQWORD;		// 64-bit signed.

// Character types.
typedef char				ANSICHAR;	// An ANSI character.
#ifdef _TCHAR_DEFINED
typedef TCHAR				UNICHAR;
#else
typedef unsigned short      UNICHAR;	// A unicode character.
#endif
typedef unsigned char		ANSICHARU;	// An ANSI character.
typedef unsigned short      UNICHARU;	// A unicode character.

// Other base types.
typedef signed int			UBOOL;		// Boolean 0 (false) or 1 (true).
typedef float				FLOAT;		// 32-bit IEEE floating point.
typedef double				DOUBLE;		// 64-bit IEEE double.
typedef unsigned long       SIZE_T;     // Corresponds to C SIZE_T.
#ifdef _WIN64
typedef unsigned __int64	PTRINT;		// Integer large enough to hold a pointer.
#else
typedef unsigned long		PTRINT;		// Integer large enough to hold a pointer.
#endif

// Bitfield type.
typedef unsigned long       BITFIELD;	// For bitfields.

#define DECLARE_UINT64(x)	x

// Variable arguments.

/**
* Helper function to write formatted output using an argument list
*
* @param Dest - destination string buffer
* @param DestSize - size of destination buffer
* @param Count - number of characters to write (not including null terminating character)
* @param Fmt - string to print
* @param Args - argument list
* @return number of characters written or -1 if truncated
*/
INT appGetVarArgs( TCHAR* Dest, SIZE_T DestSize, INT Count, const TCHAR*& Fmt, va_list ArgPtr );

/**
* Helper function to write formatted output using an argument list
* ASCII version
*
* @param Dest - destination string buffer
* @param DestSize - size of destination buffer
* @param Count - number of characters to write (not including null terminating character)
* @param Fmt - string to print
* @param Args - argument list
* @return number of characters written or -1 if truncated
*/
INT appGetVarArgsAnsi( ANSICHAR* Dest, SIZE_T DestSize, INT Count, const ANSICHAR*& Fmt, va_list ArgPtr );

#define GET_VARARGS(msg,msgsize,len,lastarg,fmt) { va_list ap; va_start(ap,lastarg);appGetVarArgs(msg,msgsize,len,fmt,ap); }
#define GET_VARARGS_ANSI(msg,msgsize,len,lastarg,fmt) { va_list ap; va_start(ap,lastarg);appGetVarArgsAnsi(msg,msgsize,len,fmt,ap); }
#define GET_VARARGS_RESULT(msg,msgsize,len,lastarg,fmt,result) { va_list ap; va_start(ap,lastarg); result = appGetVarArgs(msg,msgsize,len,fmt,ap); }
#define GET_VARARGS_RESULT_ANSI(msg,msgsize,len,lastarg,fmt,result) { va_list ap; va_start(ap,lastarg); result = appGetVarArgsAnsi(msg,msgsize,len,fmt,ap); }

// Unwanted VC++ level 4 warnings to disable.
#pragma warning(disable : 4100) // unreferenced formal parameter										
#pragma warning(disable : 4127) // Conditional expression is constant									
#pragma warning(disable : 4200) // Zero-length array item at end of structure, a VC-specific extension	
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union					
#pragma warning(disable : 4244) // conversion to float, possible loss of data						
#pragma warning(disable : 4245) // conversion from 'enum ' to 'unsigned long', signed/unsigned mismatch 
#pragma warning(disable : 4291) // typedef-name '' used as synonym for class-name ''                    
#pragma warning(disable : 4324) // structure was padded due to __declspec(align())						
#pragma warning(disable : 4355) // this used in base initializer list                                   
#pragma warning(disable : 4389) // signed/unsigned mismatch                                             
#pragma warning(disable : 4511) // copy constructor could not be generated                              
#pragma warning(disable : 4512) // assignment operator could not be generated                           
#pragma warning(disable : 4514) // unreferenced inline function has been removed						
#pragma warning(disable : 4699) // creating precompiled header											
#pragma warning(disable : 4702) // unreachable code in inline expanded function							
#pragma warning(disable : 4710) // inline function not expanded											
#pragma warning(disable : 4711) // function selected for autmatic inlining								
#pragma warning(disable : 4714) // __forceinline function not expanded									
#pragma warning(disable : 4482) // nonstandard extension used: enum 'enum' used in qualified name (having hte enum name helps code readability and should be part of TR1 or TR2)
#pragma warning(disable : 4748)	// /GS can not protect parameters and local variables from local buffer overrun because optimizations are disabled in function

// NOTE: _mm_cvtpu8_ps will generate this falsely if it doesn't get inlined
#pragma warning(disable : 4799)	// Warning: function 'ident' has no EMMS instruction


// all of the /Wall warnings that we are able to enable
// @todo:  once we have 2005 working check:  http://msdn2.microsoft.com/library/23k5d385(en-us,vs.80).aspx
#pragma warning(default : 4191) // 'operator/operation' : unsafe conversion from 'type of expression' to 'type required'
#pragma warning(disable : 4217) // 'operator' : member template functions cannot be used for copy-assignment or copy-construction
#pragma warning(disable : 4242) // 'variable' : conversion from 'type' to 'type', possible loss of data
#pragma warning(default : 4254) // 'operator' : conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default : 4255) // 'function' : no function prototype given: converting '()' to '(void)'
#pragma warning(disable : 4263) // 'function' : member function does not override any base class virtual member function
#pragma warning(default : 4287) // 'operator' : unsigned/negative constant mismatch
#pragma warning(default : 4289) // nonstandard extension used : 'var' : loop control variable declared in the for-loop is used outside the for-loop scope
#pragma warning(default : 4302) // 'conversion' : truncation from 'type 1' to 'type 2'
#pragma warning(default : 4339) // 'type' : use of undefined type detected in CLR meta-data - use of this type may lead to a runtime exception
#pragma warning(disable : 4347) // behavior change: 'function template' is called instead of 'function
#pragma warning(disable : 4514) // unreferenced inline/local function has been removed
#pragma warning(default : 4529) // 'member_name' : forming a pointer-to-member requires explicit use of the address-of operator ('&') and a qualified name
#pragma warning(default : 4536) // 'type name' : type-name exceeds meta-data limit of 'limit' characters
#pragma warning(default : 4545) // expression before comma evaluates to a function which is missing an argument list
#pragma warning(default : 4546) // function call before comma missing argument list
#pragma warning(default : 4547) // 'operator' : operator before comma has no effect; expected operator with side-effect
#pragma warning(default : 4548) // expression before comma has no effect; expected expression with side-effect  (needed as xlocale does not compile cleanly)
#pragma warning(default : 4549) // 'operator' : operator before comma has no effect; did you intend 'operator'?
#pragma warning(disable : 4555) // expression has no effect; expected expression with side-effect
#pragma warning(default : 4557) // '__assume' contains effect 'effect'
#pragma warning(disable : 4623) // 'derived class' : default constructor could not be generated because a base class default constructor is inaccessible
#pragma warning(disable : 4625) // 'derived class' : copy constructor could not be generated because a base class copy constructor is inaccessible
#pragma warning(disable : 4626) // 'derived class' : assignment operator could not be generated because a base class assignment operator is inaccessible
#pragma warning(default : 4628) // digraphs not supported with -Ze. Character sequence 'digraph' not interpreted as alternate token for 'char'
#pragma warning(disable : 4640) // 'instance' : construction of local static object is not thread-safe
#pragma warning(disable : 4668) // 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives'
#pragma warning(default : 4682) // 'parameter' : no directional parameter attribute specified, defaulting to [in]
#pragma warning(default : 4686) // 'user-defined type' : possible change in behavior, change in UDT return calling convention
#pragma warning(disable : 4710) // 'function' : function not inlined / The given function was selected for inline expansion, but the compiler did not perform the inlining.
#pragma warning(default : 4786) // 'identifier' : identifier was truncated to 'number' characters in the debug information
#pragma warning(default : 4793) // native code generated for function 'function': 'reason'

#pragma warning(default : 4905) // wide string literal cast to 'LPSTR'
#pragma warning(default : 4906) // string literal cast to 'LPWSTR'
#pragma warning(disable : 4917) // 'declarator' : a GUID cannot only be associated with a class, interface or namespace ( ocid.h breaks this)
#pragma warning(default : 4931) // we are assuming the type library was built for number-bit pointers
#pragma warning(default : 4946) // reinterpret_cast used between related classes: 'class1' and 'class2'

#pragma warning(default : 4928) // illegal copy-initialization; more than one user-defined conversion has been implicitly applied
#if !USE_SECURE_CRT
#pragma warning(disable : 4996) // 'function' was was declared deprecated  (needed for the secure string functions)
#else
#pragma warning(default : 4996)	// enable deprecation warnings
#endif

// interesting ones to turn on and off at times
#pragma warning(disable : 4264) // 'virtual_function' : no override available for virtual member function from base 'class'; function is hidden
#pragma warning(disable : 4265) // 'class' : class has virtual functions, but destructor is not virtual
#pragma warning(disable : 4266) // '' : no override available for virtual member function from base ''; function is hidden

#pragma warning(disable : 4296) // 'operator' : expression is always true / false

#pragma warning(disable : 4820) // 'bytes' bytes padding added after member 'member'


// It'd be nice to turn these on, but at the moment they can't be used in DEBUG due to the varargs stuff.
#pragma warning(disable : 4189) // local variable is initialized but not referenced 
#pragma warning(disable : 4505) // unreferenced local function has been removed		
					

// If C++ exception handling is disabled, force guarding to be off.
#ifndef _CPPUNWIND
	#error "Bad VCC option: C++ exception handling must be enabled" //lint !e309 suppress as lint doesn't have this defined
#endif

// Make sure characters are unsigned.
#ifdef _CHAR_UNSIGNED
	#error "Bad VC++ option: Characters must be signed" //lint !e309 suppress as lint doesn't have this defined
#endif

// No asm if not compiling for x86 or if we're using VS.NET 2005
#if !(defined _M_IX86) || _MSC_VER >= 1400
	#undef ASM_X86
	#define ASM_X86 0
#else
	#define ASM_X86 1
#endif

// DLL file extension.
#define DLLEXT TEXT(".dll")

// Pathnames.
#define PATH(s) s

// NULL.
#define NULL 0

// Platform support options.
#define FORCE_ANSI_LOG           1

// String conversion classes
#include "UnStringConv.h"

// SIMD intrinsics
#include <intrin.h>

// OS unicode function calling.
#if defined(NO_UNICODE_OS_SUPPORT) || !defined(UNICODE)
	#define TCHAR_CALL_OS(funcW,funcA) (funcA)
	#define TCHAR_TO_ANSI(str) str
	#define ANSI_TO_TCHAR(str) str
#elif defined(NO_ANSI_OS_SUPPORT)
	#define TCHAR_CALL_OS(funcW,funcA) (funcW)
	#define TCHAR_TO_ANSI(str) str
	#define ANSI_TO_TCHAR(str) str
#else
	#define TCHAR_CALL_OS(funcW,funcA) (GUnicodeOS ? (funcW) : (funcA))

/**
 * NOTE: The objects these macros declare have very short lifetimes. They are
 * meant to be used as parameters to functions. You cannot assign a variable
 * to the contents of the converted string as the object will go out of
 * scope and the string released.
 *
 * NOTE: The parameter you pass in MUST be a proper string, as the parameter
 * is typecast to a pointer. If you pass in a char, not char* it will compile
 * and then crash at runtime.
 *
 * Usage:
 *
 *		SomeApi(TCHAR_TO_ANSI(SomeUnicodeString));
 *
 *		const char* SomePointer = TCHAR_TO_ANSI(SomeUnicodeString); <--- Bad!!!
 */
	#define TCHAR_TO_ANSI(str) (ANSICHAR*)FTCHARToANSI((const TCHAR*)str)
	#define TCHAR_TO_OEM(str) (ANSICHAR*)FTCHARToOEM((const TCHAR*)str)
	#define ANSI_TO_TCHAR(str) (TCHAR*)FANSIToTCHAR((const ANSICHAR*)str)
#endif

// Strings.
#define LINE_TERMINATOR TEXT("\r\n")
#define PATH_SEPARATOR TEXT("\\")
#define appIsPathSeparator( Ch )	((Ch) == PATH_SEPARATOR[0])

// Alignment.
#define GCC_PACK(n)
#define GCC_ALIGN(n)
#define GCC_BITFIELD_MAGIC
#define MS_ALIGN(n) __declspec(align(n))

/**
 * Set/restore the Console Interrupt (Control-C, Control-Break, Close) handler
 * This does nothing on if CONSOLE
 */
void appSetConsoleInterruptHandler();

/**
 * Retrieve a environment variable from the system
 *
 * @param VariableName The name of the variable (ie "Path")
 * @param Result The string to copy the value of the variable into
 * @param ResultLength The size of the Result string
 */
void appGetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, INT ResultLength);

/*----------------------------------------------------------------------------
	Globals.
----------------------------------------------------------------------------*/

// System identification.
extern "C"
{
	extern HINSTANCE      hInstance;
}


/*----------------------------------------------------------------------------
	Stack walking.
----------------------------------------------------------------------------*/

/** @name ObjectFlags
* Flags used to control the output from stack tracing
*/
typedef DWORD EVerbosityFlags;

#define VF_DISPLAY_BASIC		0x00000000
#define VF_DISPLAY_FILENAME		0x00000001
#define VF_DISPLAY_MODULE		0x00000002
#define VF_DISPLAY_ALL			0xffffffff

/**
 * Symbol information associated with a program counter.
 */
struct FProgramCounterSymbolInfo
{
	/** Module name.					*/
	ANSICHAR	ModuleName[1024];
	/** Function name.					*/
	ANSICHAR	FunctionName[1024];
	/** Filename.						*/
	ANSICHAR	Filename[1024];
	/** Line number in file.			*/
	INT			LineNumber;
	/** Symbol displacement of address.	*/
	INT			SymbolDisplacement;
};

/**
 * Initializes stack traversal and symbol. Must be called before any other stack/symbol functions. Safe to reenter.
 */
UBOOL appInitStackWalking();

/**
 * Converts the passed in program counter address to a human readable string and appends it to the passed in one.
 * @warning: The code assumes that HumanReadableString is large enough to contain the information.
 *
 * @param	ProgramCounter			Address to look symbol information up for
 * @param	HumanReadableString		String to concatenate information with
 * @param	HumanReadableStringSize size of string in characters
 * @param	VerbosityFlags			Bit field of requested data for output. -1 for all output.
 */ 
void appProgramCounterToHumanReadableString( QWORD ProgramCounter, ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, EVerbosityFlags VerbosityFlags = VF_DISPLAY_ALL );

/**
 * Converts the passed in program counter address to a symbol info struct, filling in module and filename, line number and displacement.
 * @warning: The code assumes that the destination strings are big enough
 *
 * @param	ProgramCounter			Address to look symbol information up for
 * @return	symbol information associated with program counter
 */
FProgramCounterSymbolInfo appProgramCounterToSymbolInfo( QWORD ProgramCounter );

/**
 * Capture a stack backtrace and optionally use the passed in exception pointers.
 *
 * @param	BackTrace			[out] Pointer to array to take backtrace
 * @param	MaxDepth			Entries in BackTrace array
 * @param	Context				Optional thread context information
 */
void appCaptureStackBackTrace( QWORD* BackTrace, DWORD MaxDepth, CONTEXT* Context = NULL );

/**
 * Walks the stack and appends the human readable string to the passed in one.
 * @warning: The code assumes that HumanReadableString is large enough to contain the information.
 *
 * @param	HumanReadableString	String to concatenate information with
 * @param	HumanReadableStringSize size of string in characters
 * @param	IgnoreCount			Number of stack entries to ignore (some are guaranteed to be in the stack walking code)
 * @param	Context				Optional thread context information
 */ 
void appStackWalkAndDump( ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, INT IgnoreCount, CONTEXT* Context = NULL );

/** 
 * Dump current function call stack to log file.
 */
void appDumpCallStackToLog(INT IgnoreCount=2);

/*----------------------------------------------------------------------------
	Math functions.
----------------------------------------------------------------------------*/

const FLOAT	SRandTemp = 1.f;
extern INT GSRandSeed;

//
// MSM: Converts to an integer with truncation towards zero.
//
inline INT appTrunc( FLOAT F )
{
	__asm cvttss2si eax,[F]
	// return value in eax.
}

inline FLOAT appCopySign( FLOAT A, FLOAT B ) { return _copysign(A,B); }
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
inline UBOOL appIsNaN( FLOAT A ) { return _isnan(A) != 0; }
inline UBOOL appIsFinite( FLOAT A ) { return _finite(A); }
inline INT appFloor( FLOAT F );
inline INT appCeil( FLOAT Value ) { return appTrunc(ceilf(Value)); }
inline INT appRand() { return rand(); }
inline void appRandInit(INT Seed) { srand( Seed ); }
inline FLOAT appFrand() { return rand() / (FLOAT)RAND_MAX; }
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

//
//  MSM: Round (to nearest) a floating point number to an integer.
//
inline INT appRound( FLOAT F )
{
	__asm cvtss2si eax,[F]
	// return value in eax.
}

//
// MSM: Converts to integer equal to or less than.
//
inline INT appFloor( FLOAT F )
{
	const DWORD mxcsr_floor = 0x00003f80;
	const DWORD mxcsr_default = 0x00001f80;

	__asm ldmxcsr [mxcsr_floor]		// Round toward -infinity.
	__asm cvtss2si eax,[F]
	__asm ldmxcsr [mxcsr_default]	// Round to nearest
	// return value in eax.
}

//
// MSM: Fast float inverse square root using SSE.
// Accurate to within 1 LSB.
//
FORCEINLINE FLOAT appInvSqrt( FLOAT F )
{
#if ENABLE_VECTORINTRINSICS
	static const __m128 fThree = _mm_set_ss( 3.0f );
	static const __m128 fOneHalf = _mm_set_ss( 0.5f );
	__m128 Y0, X0, Temp;
	FLOAT temp;

	Y0 = _mm_set_ss( F );
	X0 = _mm_rsqrt_ss( Y0 );	// 1/sqrt estimate (12 bits)

	// Newton-Raphson iteration (X1 = 0.5*X0*(3-(Y*X0)*X0))
	Temp = _mm_mul_ss( _mm_mul_ss(Y0, X0), X0 );	// (Y*X0)*X0
	Temp = _mm_sub_ss( fThree, Temp );				// (3-(Y*X0)*X0)
	Temp = _mm_mul_ss( X0, Temp );					// X0*(3-(Y*X0)*X0)
	Temp = _mm_mul_ss( fOneHalf, Temp );			// 0.5*X0*(3-(Y*X0)*X0)
	_mm_store_ss( &temp, Temp );

#else
	const FLOAT fThree = 3.0f;
	const FLOAT fOneHalf = 0.5f;
	FLOAT temp;

	__asm
	{
		movss	xmm1,[F]
		rsqrtss	xmm0,xmm1			// 1/sqrt estimate (12 bits)

		// Newton-Raphson iteration (X1 = 0.5*X0*(3-(Y*X0)*X0))
		movss	xmm3,[fThree]
		movss	xmm2,xmm0
		mulss	xmm0,xmm1			// Y*X0
		mulss	xmm0,xmm2			// Y*X0*X0
		mulss	xmm2,[fOneHalf]		// 0.5*X0
		subss	xmm3,xmm0			// 3-Y*X0*X0
		mulss	xmm3,xmm2			// 0.5*X0*(3-Y*X0*X0)
		movss	[temp],xmm3
	}
#endif
	return temp;
}

inline FLOAT appInvSqrtEst( FLOAT F )
{
	return appInvSqrt( F );
}

//
// MSM: Fast float square root using SSE.
// Accurate to within 1 LSB.
//
inline FLOAT appSqrt( FLOAT F )
{
	// DB: crt's sqrt is ~60% faster than the below code and is defined for near-zero values.
	return sqrt( F );
#if 0
	const FLOAT fZero = 0.0f;
	const FLOAT fThree = 3.0f;
	const FLOAT fOneHalf = 0.5f;
	FLOAT temp;

	__asm
	{
		movss	xmm1,[F]
		rsqrtss xmm0,xmm1			// 1/sqrt estimate (12 bits)
		
		// Newton-Raphson iteration (X1 = 0.5*X0*(3-(Y*X0)*X0))
		movss	xmm3,[fThree]
		movss	xmm2,xmm0
		mulss	xmm0,xmm1			// Y*X0
		mulss	xmm0,xmm2			// Y*X0*X0
		mulss	xmm2,[fOneHalf]		// 0.5*X0
		subss	xmm3,xmm0			// 3-Y*X0*X0
		mulss	xmm3,xmm2			// 0.5*X0*(3-Y*X0*X0)

		movss	xmm4,[fZero]
		cmpss	xmm4,xmm1,4			// not equal

		mulss	xmm3,xmm1			// sqrt(f) = f * 1/sqrt(f)

		andps	xmm3,xmm4			// seet result to zero if input is zero

		movss	[temp],xmm3
	}

	return temp;
#endif
}

/**
 * Computes the base 2 logarithm for an integer value that is greater than 0.
 * The result is rounded down to the nearest integer.
 *
 * @param Value the value to compute the log of
 */
inline DWORD appFloorLog2(DWORD Value) 
{
	DWORD Log2;
	// Use BSR to return the log2 of the integer
	__asm
	{
		bsr eax, Value
		mov Log2, eax
	}
	return Log2;
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
	if (Value == 0) return 32;
	return 31 - appFloorLog2(Value);
}

/*----------------------------------------------------------------------------
	Time functions.
----------------------------------------------------------------------------*/

extern DOUBLE GSecondsPerCycle;
extern QWORD GNumTimingCodeCalls;

#define DEFINED_appSeconds 1
/**
 * Returns time in seconds. Origin is arbitrary.
 *
 * @return time in seconds (arbitrary origin)
 */
inline DOUBLE appSeconds()
{
#if !FINAL_RELEASE
	GNumTimingCodeCalls++;
#endif
	LARGE_INTEGER Cycles;
	QueryPerformanceCounter(&Cycles);
	return Cycles.QuadPart * GSecondsPerCycle;
}

#define DEFINED_appCycles 1
/**
 * Current high resolution cycle counter. Origin is arbitrary.
 *
 * @return current value of high resolution cycle counter - origin is aribtrary
 */
inline DWORD appCycles()
{
#if !FINAL_RELEASE
	GNumTimingCodeCalls++;
#endif
	LARGE_INTEGER Cycles;
	QueryPerformanceCounter(&Cycles);
	return Cycles.QuadPart;
}

/** 
 * Returns whether the line can be broken between these two characters
 */
UBOOL appCanBreakLineAt( TCHAR Previous, TCHAR Current );

/*----------------------------------------------------------------------------
	Memory functions.
----------------------------------------------------------------------------*/

extern "C" void* __cdecl _alloca(size_t);
#define appAlloca(size) ((size==0) ? 0 : _alloca((size+7)&~7))

/**
 * Enforces strict memory load/store ordering across the memory barrier call.
 */
FORCEINLINE void appMemoryBarrier()
{
#if defined _M_IX86
	// Do nothing on x86; the spec requires load/store ordering even in the absence of a memory barrier.
#else
	#error Unknown platform for appMemoryBarrier implementation.
#endif
}

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
	UObject* <-> DWORD functions.
----------------------------------------------------------------------------*/

// This is a hack, but MUST be defined for (nonPS3) 64-bit platforms right now! --ryan.
#ifdef PLATFORM_64BITS

#define SERIAL_POINTER_INDEX 1
#define MAX_SERIALIZED_POINTERS (1024 * 128)
extern void *GSerializedPointers[MAX_SERIALIZED_POINTERS];
extern INT GTotalSerializedPointers;
INT SerialPointerIndex(void *ptr);

#endif


/** 
 * Support functions for overlaying an object/name pointer onto an index (like in script code
 */
FORCEINLINE DWORD appPointerToDWORD(void* Pointer)
{
#if SERIAL_POINTER_INDEX
	return SerialPointerIndex(Pointer);
#else
	return (DWORD)Pointer;
#endif
}

FORCEINLINE void* appDWORDToPointer(DWORD Value)
{
#if SERIAL_POINTER_INDEX
	return GSerializedPointers[Value];
#else
	return (void*)Value;
#endif
}
