/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
/*=============================================================================
	Core.h: Unreal core public header file.
=============================================================================*/

#ifndef _INC_CORE
#define _INC_CORE

// phantom definitions to help VAX parse our VARARG_* macros (VAX build 1440)
#ifdef VISUAL_ASSIST_HACK
	#define VARARG_DECL( FuncRet, StaticFuncRet, Return, FuncName, Pure, FmtType, ExtraDecl, ExtraCall ) FuncRet FuncName( ExtraDecl FmtType Fmt, ... )
	#define VARARG_BODY( FuncRet, FuncName, FmtType, ExtraDecl ) FuncRet FuncName( ExtraDecl FmtType Fmt, ... )
#endif // VISUAL_ASSIST_HACK

/*----------------------------------------------------------------------------
	Low level includes.
----------------------------------------------------------------------------*/

// Unwanted Intel C++ level 4 warnings/ remarks.
#if __ICL
#pragma warning(disable : 873)
#pragma warning(disable : 981)
#pragma warning(disable : 522)
#pragma warning(disable : 271)
#pragma warning(disable : 424)
#pragma warning(disable : 193)
#pragma warning(disable : 444)
#pragma warning(disable : 440)
#pragma warning(disable : 171)
#pragma warning(disable : 1125)
#pragma warning(disable : 488)
#pragma warning(disable : 858)
#pragma warning(disable : 82)
#pragma warning(disable : 1)
#pragma warning(disable : 177)
#pragma warning(disable : 279)
#endif

// Build options.
#include "UnBuild.h"

#if USE_AG_PERFMON
	unsigned __int16 UnAgPmRegisterEvent(const char *name);
	class AgPerfMonTimer
	{
			unsigned __int16 PerfEventID;
		public:
			AgPerfMonTimer(unsigned __int16 id);
			
			~AgPerfMonTimer(void);
	};
#endif

// note: If you add more platforms here, check out appNetworkNeedsByteSwapping().
// it will probably need to be fixed as well
#if XBOX || PS3
#define CONSOLE					1
#define EXCEPTIONS_DISABLED		1
#define NO_BYTE_ORDER_SERIALIZE 1
#else
#define CONSOLE					0
#endif

// secure crt only for use on win32 and VC 8. disable otherwise
#if CONSOLE || _MSC_VER < 1400
#undef USE_SECURE_CRT
#define USE_SECURE_CRT 0
#endif

#if _MSC_VER || __ICC || __LINUX__
	#define SUPPORTS_PRAGMA_PACK 1
#else
	#define SUPPORTS_PRAGMA_PACK 0
#endif

// Compiler specific include.
#if XBOX
	#include "UnXenon.h"
#elif _MSC_VER
	#include "UnVcWin32.h"
#elif PS3
	#include "UnPS3.h"
#elif __GNUG__
	#include "UnGCC.h"
#elif __MWERKS__
	#error No Metrowerks support
#elif __ICC
	#error No Intel compiler support
#else
	#error Unknown Compiler
#endif

// OS specific include.
#if __UNIX__
	#include "UnUnix.h"
	#include <signal.h>
#endif

// Check support for variadic macros:
#if (GCC_VERSION > 030401) || (_MSC_VER >= 1400)
	#define SUPPORTS_VARIADIC_MACROS	1
#elif _MSC_VER >= 1300
	//Visual Studio C++ 2003 doesn't support it, but there is a workaround:
	#pragma warning(disable: 4002)		// Warning: too many actual parameters for macro 'ident'
	#pragma warning(disable: 4003)		// Warning: not enough actual parameters for macro 'ident'
	#define SUPPORTS_VARIADIC_MACROS	0
	template <typename T>
	inline const T &		VARG( const T &t )				{ return t; }
	inline const TCHAR *	VARG( )							{ return TEXT(""); }
#else
	#define SUPPORTS_VARIADIC_MACROS	0
#endif
#if !(SUPPORTS_VARIADIC_MACROS || (_MSC_VER >= 1300))
	#error Compiler does not support variable number arguments to #define macros.
#endif

// CPU specific includes.
#if ((__INTEL__) && (!defined __GNUC__))
#define __HAS_SSE__ 1
#pragma warning(disable : 4799)
#include <xmmintrin.h>
#include <fvec.h>
#define PREFETCH(x) _mm_prefetch( (char*)(x), _MM_HINT_T0 );
#endif
#if XBOX
#define __HAS_ALTIVEC__ 1
#include "ppcintrinsics.h"
struct __vector4_c : public __vector4
{
	__vector4_c( FLOAT InX, FLOAT InY, FLOAT InZ, FLOAT InW )
	{
		v[0] = InX; v[1] = InY; v[2] = InZ; v[3] = InW;
	}
	__vector4_c( DWORD InX, DWORD InY, DWORD InZ, DWORD InW )
	{
		u[0] = InX; u[1] = InY; u[2] = InZ; u[3] = InW;
	}

};
#define PREFETCH(x) __dcbt( 0, (char*)(x) );
#endif
#if PS3
#define __HAS_ALTIVEC__ 1
#define PREFETCH(x) __dcbt( (char*)(x) );
#endif

#ifndef PREFETCH
#define PREFETCH(x)
#endif

// Global constants.
enum {MAXBYTE		= 0xff       };
enum {MAXWORD		= 0xffffU    };
enum {MAXDWORD		= 0xffffffffU};
enum {MAXSBYTE		= 0x7f       };
enum {MAXSWORD		= 0x7fff     };
enum {MAXINT		= 0x7fffffff };
enum {INDEX_NONE	= -1         };
enum {UNICODE_BOM   = 0xfeff     };
enum ENoInit {E_NoInit = 0};

// Unicode or single byte character set mappings.
#ifdef UNICODE
	#ifndef _TCHAR_DEFINED
		typedef UNICHAR  TCHAR;
	#endif
	typedef UNICHARU TCHARU;

    #ifndef _TEXT_DEFINED
	#undef TEXT
	#define TEXT(s) L##s
    #endif

    #ifndef _US_DEFINED
	#undef US
	#define US FString(L"")
    #endif

	inline TCHAR    FromAnsi   ( ANSICHAR In ) { return (BYTE)In;                        }
	inline TCHAR    FromUnicode( UNICHAR In  ) { return In;                              }
	inline ANSICHAR ToAnsi     ( TCHAR In    ) { return (WORD)In<0x100 ? In : MAXSBYTE;  }
	inline UNICHAR  ToUnicode  ( TCHAR In    ) { return In;                              }
#else
	#ifndef _TCHAR_DEFINED
		typedef ANSICHAR  TCHAR;
	#endif
	typedef ANSICHARU TCHARU;

	#undef TEXT
	#define TEXT(s) s
	#undef US
	#define US FString("")
	inline TCHAR    FromAnsi   ( ANSICHAR In ) { return In;                              }
	inline TCHAR    FromUnicode( UNICHAR In  ) { return (WORD)In<0x100 ? In : MAXSBYTE;  }
	inline ANSICHAR ToAnsi     ( TCHAR In    ) { return In; }
	inline UNICHAR  ToUnicode  ( TCHAR In    ) { return (BYTE)In;                        }
#endif

/*----------------------------------------------------------------------------
	Forward declarations.
----------------------------------------------------------------------------*/

// Objects.
class	UObject;
class		UComponent;
class		UExporter;
class		UFactory;
class		UField;
class			UConst;
class			UEnum;
class			UProperty;
class				UByteProperty;
class				UIntProperty;
class				UBoolProperty;
class				UFloatProperty;
class				UObjectProperty;
class					UComponentProperty;
class					UClassProperty;
class				UNameProperty;
class				UStructProperty;
class               UStrProperty;
class               UArrayProperty;
class				UMapProperty;
class				UDelegateProperty;
class			UStruct;
class				UFunction;
class				UState;
class					UClass;
class				UScriptStruct;
class		ULinker;
class			ULinkerLoad;
class			ULinkerSave;
class		UPackage;
class		USubsystem;
class			USystem;
class		UTextBuffer;
class		UPackageMap;
class		UDebugger; //DEBUGGER
class		UObjectRedirector;

// Structs.
class FName;
class FArchive;
class FCompactIndex;
class FExec;
class FGuid;
class FMemStack;
class FPackageInfo;
class FTransactionBase;
class FUnknown;
class FRepLink;
class FArray;
class FString;
class FMalloc;
class FFilename;

// Templates.
template<class T, DWORD Alignment=DEFAULT_ALIGNMENT> class TArray;
template<class T> class TTransArray;
template<class TK, class TI> class TMap;
template<class TK, class TI> class TMultiMap;
template<class T,UBOOL DisableByteSwap=FALSE> class TArrayForceByteSwap;

// Globals.
extern class FOutputDevice*					GNull;
extern class FOutputDeviceRedirectorBase*	GLog;
extern class FOutputDevice*					GThrow;
extern class FOutputDeviceError*			GError;
extern class FFeedbackContext*				GWarn;

// EName definition.
#include "UnNames.h"


/** Breaks into the debugger.  Forces a GPF in non-debug builds. */
#ifdef _DEBUG
	#if defined(XBOX)
		#define appIsDebuggerPresent()	TRUE
		#define appDebugBreak()			DebugBreak()
	#elif PS3
		extern void PS3Callstack();
		#define appIsDebuggerPresent()	TRUE
		#define appDebugBreak()			( *((INT*)3) = 13 )
	#else // PC
		#define appIsDebuggerPresent	IsDebuggerPresent
		#define appDebugBreak()			( IsDebuggerPresent() ? (DebugBreak(),1) : 1 )
	#endif
#else
	#if defined(XBOX)
        #define appIsDebuggerPresent()	TRUE
		#define appDebugBreak()			( *((INT*)3) = 13 )
	#elif PS3
		extern void PS3Callstack();
        #define appIsDebuggerPresent()	TRUE
		#define appDebugBreak()			( *((INT*)3) = 13 )
	#else // PC
		#define appIsDebuggerPresent	IsDebuggerPresent
		#define appDebugBreak()			( IsDebuggerPresent() ? *((INT*)3)=13 : 1 )
	#endif
#endif

// use a light-weight version of the PURE_VIRTUAL macro for FOutputDevice, necessary
// because of dependency issues
#if CHECK_PUREVIRTUALS
#define _PURE_VIRTUAL =0;
#else
#define _PURE_VIRTUAL { appDebugBreak(); }
#endif




#define TMAPS_IMPLEMENTED 0

/*-----------------------------------------------------------------------------
	Ugly VarArgs type checking (debug builds only).
-----------------------------------------------------------------------------*/

#define VARARG_EXTRA(A) A,
#define VARARG_NONE
#define VARARG_PURE =0

#if _MSC_VER

static inline DWORD			CheckVA(DWORD dw)		{ return dw; }
static inline BYTE			CheckVA(BYTE b)			{ return b; }
static inline UINT			CheckVA(UINT ui)		{ return ui; }
static inline INT			CheckVA(INT i)			{ return i; }
static inline QWORD			CheckVA(QWORD qw)		{ return qw; }
static inline SQWORD		CheckVA(SQWORD sqw)		{ return sqw; }
static inline DOUBLE		CheckVA(DOUBLE d)		{ return d; }
static inline TCHAR			CheckVA(TCHAR c)		{ return c; }
static inline void*			CheckVA(ANSICHAR* s)	{ return (void*)s; }
template<class T> T*		CheckVA(T* p)			{ return p; }
template<class T> const T*	CheckVA(const T* p)		{ return p; }

#define VARARG_DECL( FuncRet, StaticFuncRet, Return, FuncName, Pure, FmtType, ExtraDecl, ExtraCall )	\
	FuncRet FuncName##__VA( ExtraDecl FmtType Fmt, ... ) Pure;  \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt) {Return FuncName##__VA(ExtraCall (Fmt));} \
	template<class T1> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1) {T1 v1=CheckVA(V1);Return FuncName##__VA(ExtraCall (Fmt),(v1));} \
	template<class T1,class T2> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2));} \
	template<class T1,class T2,class T3> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3));} \
	template<class T1,class T2,class T3,class T4> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4));} \
	template<class T1,class T2,class T3,class T4,class T5> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10,T11 V11) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);T11 v11=CheckVA(V11);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10),(v11));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10,T11 V11,T12 V12) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);T11 v11=CheckVA(V11);T12 v12=CheckVA(V12);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10),(v11),(v12));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10,T11 V11,T12 V12,T13 V13) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);T11 v11=CheckVA(V11);T12 v12=CheckVA(V12);T13 v13=CheckVA(V13);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10),(v11),(v12),(v13));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10,T11 V11,T12 V12,T13 V13,T14 V14) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);T11 v11=CheckVA(V11);T12 v12=CheckVA(V12);T13 v13=CheckVA(V13);T14 v14=CheckVA(V14);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10),(v11),(v12),(v13),(v14));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10,T11 V11,T12 V12,T13 V13,T14 V14,T15 V15) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);T11 v11=CheckVA(V11);T12 v12=CheckVA(V12);T13 v13=CheckVA(V13);T14 v14=CheckVA(V14);T15 v15=CheckVA(V15);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10),(v11),(v12),(v13),(v14),(v15));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10,T11 V11,T12 V12,T13 V13,T14 V14,T15 V15,T16 V16) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);T11 v11=CheckVA(V11);T12 v12=CheckVA(V12);T13 v13=CheckVA(V13);T14 v14=CheckVA(V14);T15 v15=CheckVA(V15);T16 v16=CheckVA(V16);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10),(v11),(v12),(v13),(v14),(v15),(v16));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10,T11 V11,T12 V12,T13 V13,T14 V14,T15 V15,T16 V16,T17 V17) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);T11 v11=CheckVA(V11);T12 v12=CheckVA(V12);T13 v13=CheckVA(V13);T14 v14=CheckVA(V14);T15 v15=CheckVA(V15);T16 v16=CheckVA(V16);T17 v17=CheckVA(V17);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10),(v11),(v12),(v13),(v14),(v15),(v16),(v17));} \
	template<class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18> \
	StaticFuncRet FuncName(ExtraDecl FmtType Fmt,T1 V1,T2 V2,T3 V3,T4 V4,T5 V5,T6 V6,T7 V7,T8 V8,T9 V9,T10 V10,T11 V11,T12 V12,T13 V13,T14 V14,T15 V15,T16 V16,T17 V17,T18 V18) {T1 v1=CheckVA(V1);T2 v2=CheckVA(V2);T3 v3=CheckVA(V3);T4 v4=CheckVA(V4);T5 v5=CheckVA(V5);T6 v6=CheckVA(V6);T7 v7=CheckVA(V7);T8 v8=CheckVA(V8);T9 v9=CheckVA(V9);T10 v10=CheckVA(V10);T11 v11=CheckVA(V11);T12 v12=CheckVA(V12);T13 v13=CheckVA(V13);T14 v14=CheckVA(V14);T15 v15=CheckVA(V15);T16 v16=CheckVA(V16);T17 v17=CheckVA(V17);T18 v18=CheckVA(V18);Return FuncName##__VA(ExtraCall (Fmt),(v1),(v2),(v3),(v4),(v5),(v6),(v7),(v8),(v9),(v10),(v11),(v12),(v13),(v14),(v15),(v16),(v17),(v18));}

#define VARARG_BODY( FuncRet, FuncName, FmtType, ExtraDecl )		\
	FuncRet FuncName##__VA( ExtraDecl  FmtType Fmt, ... )

#else  // !_MSC_VER

#define VARARG_DECL( FuncRet, StaticFuncRet, Return, FuncName, Pure, FmtType, ExtraDecl, ExtraCall )	\
	FuncRet FuncName( ExtraDecl FmtType Fmt, ... ) Pure
#define VARARG_BODY( FuncRet, FuncName, FmtType, ExtraDecl )		\
	FuncRet FuncName( ExtraDecl FmtType Fmt, ... )

#endif // _MSC_VER

/*-----------------------------------------------------------------------------
	Abstract interfaces.
-----------------------------------------------------------------------------*/


// An output device.
class FOutputDevice
{
public:
	virtual ~FOutputDevice(){}

	// FOutputDevice interface.
	virtual void Serialize( const TCHAR* V, EName Event ) _PURE_VIRTUAL;
	virtual void Flush(){};

	/**
	 * Closes output device and cleans up. This can't happen in the destructor
	 * as we might have to call "delete" which cannot be done for static/ global
	 * objects.
	 */
	virtual void TearDown(){};

	// Simple text printing.
	void Log( const TCHAR* S );
	void Log( enum EName Type, const TCHAR* S );
	void Log( const FString& S );
	void Log( enum EName Type, const FString& S );
	VARARG_DECL( void, void, {}, Logf, VARARG_NONE, const TCHAR*, VARARG_NONE, VARARG_NONE );
	VARARG_DECL( void, void, {}, Logf, VARARG_NONE, const TCHAR*, VARARG_EXTRA(enum EName E), VARARG_EXTRA(E) );
};

/**
 * Abstract base version of FOutputDeviceRedirector, needed due to order of dependencies.
 */
class FOutputDeviceRedirectorBase : public FOutputDevice
{
public:
	/**
	 * Adds an output device to the chain of redirections.	
	 *
	 * @param OutputDevice	output device to add
	 */
	virtual void AddOutputDevice( FOutputDevice* OutputDevice ) = 0;
	/**
	 * Removes an output device from the chain of redirections.	
	 *
	 * @param OutputDevice	output device to remove
	 */
	virtual void RemoveOutputDevice( FOutputDevice* OutputDevice ) = 0;
	/**
	 * Returns whether an output device is currently in the list of redirectors.
	 *
	 * @param	OutputDevice	output device to check the list against
	 * @return	TRUE if messages are currently redirected to the the passed in output device, FALSE otherwise
	 */
	virtual UBOOL IsRedirectingTo( FOutputDevice* OutputDevice ) = 0;

	/** Flushes lines buffered by secondary threads. */
	virtual void FlushThreadedLogs() = 0;

	/**
	 * Sets the current thread to be the master thread that prints directly
	 * (isn't queued up)
	 */
	virtual void SetCurrentThreadAsMasterThread() = 0;
};

// Error device.
class FOutputDeviceError : public FOutputDevice
{
public:
	virtual void HandleError()=0;
};

/**
 * This class servers as the base class for console window output.
 */
class FOutputDeviceConsole : public FOutputDevice
{
public:
	/**
	 * Shows or hides the console window. 
	 *
	 * @param ShowWindow	Whether to show (TRUE) or hide (FALSE) the console window.
	 */
	virtual void Show( UBOOL ShowWindow )=0;

	/** 
	 * Returns whether console is currently shown or not
	 *
	 * @return TRUE if console is shown, FALSE otherwise
	 */
	virtual UBOOL IsShown()=0;

	/**
	 * Returns whether the console has been inherited (TRUE) or created (FALSE)
	 *
	 * @return TRUE if console is inherited, FALSE if it was created
     */
	virtual UBOOL IsInherited() const { return FALSE; }
};

/*-----------------------------------------------------------------------------
	Logging and critical errors.
-----------------------------------------------------------------------------*/

/** @name Logging and critical errors */
//@{
/**
 * Requests application exit.
 *
 * @param	Force	If true, perform immediate exit (dangerous because config code isn't flushed, etc).
 *                  If false, request clean main-loop exit from the platform specific code.
 */
void appRequestExit( UBOOL Force );

/** Sends a message to the debugging output. */
void appOutputDebugString( const TCHAR *Message );

/** Sends a formatted message to the debugging output. */
void VARARGS appOutputDebugStringf( const TCHAR *Format, ... );

/** Sends a message to a remote tool. */
void appSendNotificationString( const ANSICHAR *Message );

/** Sends a formatted message to a remote tool. */
void VARARGS appSendNotificationStringf( const ANSICHAR *Format, ... );

/** Failed assertion handler.  Warning: May be called at library startup time. */
void VARARGS appFailAssertFunc( const ANSICHAR* Expr, const ANSICHAR* File, INT Line, const TCHAR* Format=TEXT(""), ... );

/** Failed assertion handler.  This version only calls appOutputDebugString. */
void VARARGS appFailAssertFuncDebug( const ANSICHAR* Expr, const ANSICHAR* File, INT Line, const TCHAR* Format=TEXT(""), ... );
void VARARGS appFailAssertFuncDebug( const ANSICHAR* Expr, const ANSICHAR* File, INT Line, enum EName Type, const TCHAR* Format=TEXT(""), ... );

#if SUPPORTS_VARIADIC_MACROS
	#define appFailAssert(expr,file,line,...)				{ if (appIsDebuggerPresent()) appFailAssertFuncDebug(expr, file, line, ##__VA_ARGS__); appDebugBreak(); appFailAssertFunc(expr, file, line, ##__VA_ARGS__); }
#elif _MSC_VER >= 1300
	#define appFailAssert(expr,file,line,a,b,c,d,e,f,g,h)	{ if (appIsDebuggerPresent()) appFailAssertFuncDebug(expr,file,line,VARG(a),VARG(b),VARG(c),VARG(d),VARG(e),VARG(f),VARG(g),VARG(h)); appDebugBreak(); appFailAssertFunc(expr,file,line,VARG(a),VARG(b),VARG(c),VARG(d),VARG(e),VARG(f),VARG(g),VARG(h)); }
#endif

/** Returns the last system error code in string form.  NOTE: Only one return value is valid at a time! */
const TCHAR* appGetSystemErrorMessage( INT Error=0 );
/** Pops up a message dialog box containing the input string. */
void appDebugMessagef( const TCHAR* Fmt, ... );
/** Pops up a message dialog box containing the last system error code in string form. */
void appGetLastError( void );

/** Wrapper for appDebugBreak() - used while debugging for calls that should be removed prior to checking code in */
inline void DebugRemoveMe() { appDebugBreak(); }

// Define NO_LOGGING to strip out all writing to log files, OutputDebugString(), etc.
// This is needed for consoles that require no logging (Xbox, Xenon)
#if NO_LOGGING || FINAL_RELEASE
	#if COMPILER_SUPPORTS_NOOP
		// MS compilers support noop which discards everything inside the parens
		#define debugf			__noop
		#define appErrorf		__noop
		#define warnf			__noop
		#define KISMET_LOG		__noop
		#define KISMET_WARN		__noop
		#define KISMET_LOG_REF(obj)		__noop
		#define KISMET_WARN_REF(obj)	__noop
	#else
		#pragma message("Logging can only be disabled on MS compilers")
		#define debugf(...)
		#define appErrorf(...)
		#define warnf(...)
		#define KISMET_LOG(...)
		#define KISMET_WARN(...)
		#define KISMET_LOG_REF(...)
		#define KISMET_WARN_REF(...)
	#endif
#else
	#if SUPPORTS_VARIADIC_MACROS
		#define appErrorf(...)				( (appIsDebuggerPresent() ? appFailAssertFuncDebug("appErrorf", __FILE__, __LINE__, ##__VA_ARGS__),1 : 1), appDebugBreak(), GError->Logf(__VA_ARGS__), 1 )
	#elif _MSC_VER >= 1300
		#define appErrorf(a,b,c,d,e,f,g,h)	( (appIsDebuggerPresent() ? appFailAssertFuncDebug("appErrorf", __FILE__, __LINE__, VARG(a),VARG(b),VARG(c),VARG(d),VARG(e),VARG(f),VARG(g),VARG(h)),1 : 1), appDebugBreak(), GError->Logf(VARG(a),VARG(b),VARG(c),VARG(d),VARG(e),VARG(f),VARG(g),VARG(h)), 1 )
	#endif

	#define debugf				GLog->Logf
	#define warnf				GWarn->Logf

	#if CONSOLE
		#if COMPILER_SUPPORTS_NOOP
			#define KISMET_LOG	__noop
			#define KISMET_WARN	__noop
			#define KISMET_LOG_REF(obj)		__noop
			#define KISMET_WARN_REF(obj)	__noop
		#else
			#define KISMET_LOG(...)
			#define KISMET_WARN(...)
			#define KISMET_LOG_REF(...)
			#define KISMET_WARN_REF(...)
		#endif
	#else
		#define KISMET_LOG		GetRootSequence()->ScriptLogf
		#define KISMET_WARN		GetRootSequence()->ScriptWarnf
		#define KISMET_LOG_REF(obj) obj->GetRootSequence()->ScriptLogf
		#define KISMET_WARN_REF(obj) obj->GetRootSequence()->ScriptWarnf
	#endif
#endif

#if DO_LOG_SLOW
	#define debugfSlow			GLog->Logf
#else
	#if COMPILER_SUPPORTS_NOOP
		// MS compilers support noop which discards everything inside the parens
		#define debugfSlow		__noop
	#else
		#define debugfSlow(...)
	#endif
#endif

#if DO_GUARD_SLOW
	#define appErrorfSlow		GError->Logf
#else
	#if COMPILER_SUPPORTS_NOOP
		// MS compilers support noop which discards everything inside the parens
		#define appErrorfSlow	__noop
	#else
		#define appErrorfSlow(...)
	#endif
#endif

#if SUPPORT_SUPPRESSED_LOGGING
	#define debugfSuppressed		GLog->Logf
	#define appErrorfSuppressed		GError->Logf
#else
	#if COMPILER_SUPPORTS_NOOP
		// MS compilers support noop which discards everything inside the parens
		#define debugfSuppressed	__noop
		#define appErrorfSuppressed	__noop
	#else
		#define debugfSuppressed(...)
		#define appErrorfSuppressed(...)
	#endif
#endif
//@}

#if NO_LOGGING || FINAL_RELEASE
#define ENABLE_SCRIPT_TRACING 0
#else
#define ENABLE_SCRIPT_TRACING 1
#endif


// This define must come after the class declaration for FOutputDevice, since the macro uses GError->Logf
#if CHECK_PUREVIRTUALS
	#define PURE_VIRTUAL(func,extra) =0;
#else
	#define PURE_VIRTUAL(func,extra) { appErrorf(TEXT("Pure virtual not implemented (%s)"), TEXT(#func)); extra }
#endif

// undefine the simpler version of the PURE_VIRTUAL macro so that
// no one accidentally uses the wrong version
#ifdef _PURE_VIRTUAL
	#undef _PURE_VIRTUAL
#endif

// Memory allocator.

enum ECacheBehaviour
{
	CACHE_Normal		= 0,
	CACHE_WriteCombine	= 1,
	CACHE_None			= 2,
	CACHE_Virtual		= 3,
	CACHE_MAX			// needs to be last entry
};

//
// C style memory allocation stubs that fall back to C runtime
//
#define appSystemMalloc		malloc
#define appSystemFree		free
#define appSystemRealloc	realloc

/**
 * Inherit from FUseSystemMallocForNew if you want your objects to be placed in memory
 * alloced by the system malloc routines, bypassing GMalloc. This is e.g. used by FMalloc
 * itself.
 */
class FUseSystemMallocForNew
{
public:
	/**
	 * Overloaded new operator using the system allocator.
	 *
	 * @param	Size	Amount of memory to allocate (in bytes)
	 * @return			A pointer to a block of memory with size Size or NULL
	 */
	void* operator new( size_t Size )
	{
		return appSystemMalloc( Size );
	}

	/**
	 * Overloaded delete operator using the system allocator
	 *
	 * @param	Ptr		Pointer to delete
	 */
	void operator delete( void* Ptr )
	{
		appSystemFree( Ptr );
	}

	/**
	 * Overloaded array new operator using the system allocator.
	 *
	 * @param	Size	Amount of memory to allocate (in bytes)
	 * @return			A pointer to a block of memory with size Size or NULL
	 */
	void* operator new[]( size_t Size )
	{
		return appSystemMalloc( Size );
	}

	/**
	 * Overloaded array delete operator using the system allocator
	 *
	 * @param	Ptr		Pointer to delete
	 */
	void operator delete[]( void* Ptr )
	{
		appSystemFree( Ptr );
	}
};


// Any object that is capable of taking commands.
class FExec
{
public:
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar ) PURE_VIRTUAL(FExec::Exec,return FALSE;)
};

class FMalloc  : public FUseSystemMallocForNew, public FExec
{
public:
	virtual void* Malloc( DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT ) = 0;
	virtual void* Realloc( void* Original, DWORD Count, DWORD Alignment=DEFAULT_ALIGNMENT ) = 0;
	virtual void Free( void* Original ) = 0;
	virtual void* PhysicalAlloc( DWORD Count, ECacheBehaviour CacheBehaviour = CACHE_WriteCombine ) { return NULL; }
	virtual void PhysicalFree( void* Original ) {}
		
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar ) { return 0; }
	
	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 */
	virtual UBOOL IsInternallyThreadSafe() { return FALSE; }

	/**
	 * Gathers memory allocations for both virtual and physical allocations.
	 *
	 * @param Virtual	[out] size of virtual allocations
	 * @param Physical	[out] size of physical allocations	
	 */
	virtual void GetAllocationInfo( SIZE_T& Virtual, SIZE_T& Physical ) { Virtual = Physical = 0; }
};

#define CONFIG_NO_USER -1

// Configuration database cache.
class FConfigCache
{
public:
	virtual INT GetArray( const TCHAR* Section, const TCHAR* Key, TArray<FString>& Arr, const TCHAR* Filename=NULL )=0;
	virtual UBOOL GetBool( const TCHAR* Section, const TCHAR* Key, UBOOL& Value, const TCHAR* Filename )=0;
	virtual UBOOL GetDouble( const TCHAR* Section, const TCHAR* Key, DOUBLE& Value, const TCHAR* Filename )=0;
	virtual UBOOL GetFloat( const TCHAR* Section, const TCHAR* Key, FLOAT& Value, const TCHAR* Filename )=0;
	virtual UBOOL GetInt( const TCHAR* Section, const TCHAR* Key, INT& Value, const TCHAR* Filename )=0;
	virtual UBOOL GetSection( const TCHAR* Section, TArray<FString>& Value, const TCHAR* Filename )=0;
	virtual UBOOL GetString( const TCHAR* Section, const TCHAR* Key, class FString& Str, const TCHAR* Filename )=0;
	virtual FString GetStr( const TCHAR* Section, const TCHAR* Key, const TCHAR* Filename )=0;
	virtual TMultiMap<FString,FString>* GetSectionPrivate( const TCHAR* Section, UBOOL Force, UBOOL Const, const TCHAR* Filename )=0;

	/**
	 * Retrieve a list of all of the config files stored in the cache
	 *
	 * @param ConfigFilenames Out array to receive the list of filenames
	 */
	virtual void GetConfigFilenames(TArray<FFilename>& ConfigFilenames)=0;

	/**
	 * Retrieve the names for all sections contained in the file specified by Filename
	 *
	 * @param	Filename			the file to retrieve section names from
	 * @param	out_SectionNames	will receive the list of section names
	 *
	 * @return	TRUE if the file specified was successfully found;
	 */
	virtual UBOOL GetSectionNames( const TCHAR* Filename, TArray<FString>& out_SectionNames )=0;

	/**
	 * Retrieve the names of sections which contain data for the specified PerObjectConfig class.
	 *
	 * @param	Filename			the file to retrieve section names from
	 * @param	SearchClass			the name of the PerObjectConfig class to retrieve sections for.
	 * @param	out_SectionNames	will receive the list of section names that correspond to PerObjectConfig sections of the specified class
	 * @param	MaxResults			the maximum number of section names to retrieve
	 *
	 * @return	TRUE if the file specified was found and it contained at least 1 section for the specified class
	 */
	virtual UBOOL GetPerObjectConfigSections( const TCHAR* Filename, const FString& SearchClass, TArray<FString>& out_SectionNames, INT MaxResults=1024 )=0;

	virtual void SetArray( const TCHAR* Section, const TCHAR* Key, const TArray<FString>& Value, const TCHAR* Filename = NULL )=0;
	virtual void SetBool( const TCHAR* Section, const TCHAR* Key, UBOOL Value, const TCHAR* Filename )=0;
	virtual void SetDouble( const TCHAR* Section, const TCHAR* Key, DOUBLE Value, const TCHAR* Filename )=0;
	virtual void SetFloat( const TCHAR* Section, const TCHAR* Key, FLOAT Value, const TCHAR* Filename )=0;
	virtual void SetInt( const TCHAR* Section, const TCHAR* Key, INT Value, const TCHAR* Filename )=0;
	virtual void SetString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value, const TCHAR* Filename )=0;

	virtual void EmptySection( const TCHAR* Section, const TCHAR* Filename )=0;
	virtual void Flush( UBOOL Read, const TCHAR* Filename=NULL )=0;

	virtual class FConfigFile* FindConfigFile( const TCHAR* Filename )=0;
	virtual void LoadFile( const TCHAR* InFilename, const FConfigFile* Fallback = NULL )=0;
	virtual void SetFile( const TCHAR* InFilename, const FConfigFile* NewConfigFile )=0;
	virtual void UnloadFile( const TCHAR* Filename )=0;
	virtual void Detach( const TCHAR* Filename )=0;

	/**
	 * Calling this function will mark any new sections imported as downloaded,
	 * which has special properties (won't be saved to disk, can be removed, etc)
	 * 
	 * @param UserIndex	Optional user index to associate with new config sections
	 */
	virtual void StartUsingDownloadedCache(INT UserIndex=CONFIG_NO_USER)=0;

	/**
	 * Return behavior to normal (new sections will no longer be marked as downloaded)
	 */
	virtual void StopUsingDownloadedCache()=0;

	/**
	 * Flush all downloaded sections from all files with the given user index
	 * NOTE: Passing CONFIG_NO_USER will NOT flush all downloaded sections, just
	 * those sections that were marked with no user
	 * 
	 * @param UserIndex Optional user index for which associated sections to flush
	 */
	virtual void RemoveDownloadedSections(INT UserIndex=CONFIG_NO_USER)=0;

	/**
	* Disables any file IO by the config cache system
	*/
	virtual void DisableFileOperations()=0;

	/**
	* Re-enables file IO by the config cache system
	*/
	virtual void EnableFileOperations()=0;

	/**
	 * Returns whether or not file operations are disabled
	 */
	virtual UBOOL AreFileOperationsDisabled()=0;

	/**
	 * Coalesces .ini and localization files into single files.
	 * DOES NOT use the config cache in memory, rather it reads all files from disk,
	 * so it would be a static function if it wasn't virtual
	 *
	 * @param ConfigDir The base directory to search for .ini files
	 * @param bNeedsByteSwapping TRUE if the output file is destined for a platform that expects byte swapped data
	 * @param IniFileWithFilters Name of ini file to look in for the list of files to filter out
	 */
	virtual void CoalesceFilesFromDisk(const TCHAR* ConfigDir, UBOOL bNeedsByteSwapping, const TCHAR* IniFileWithFilters)=0;

	/**
	 * Reads a coalesced file, breaks it up, and adds the contents to the config cache
	 *
	 * @param CoalescedFilename Path of the file to read in
	 */
	virtual void LoadCoalescedFile(const TCHAR* CoalescedFilename)=0;

	/**
	 * Prases apart an ini section that contains a list of 1-to-N mappings of names in the following format
	 *	 [PerMapPackages]
	 *	 MapName=Map1
	 *	 Package=PackageA
	 *	 Package=PackageB
	 *	 MapName=Map2
	 *	 Package=PackageC
	 *	 Package=PackageD
	 * 
	 * @param Section Name of section to look in
	 * @param KeyOne Key to use for the 1 in the 1-to-N (MapName in the above example)
	 * @param KeyN Key to use for the N in the 1-to-N (Package in the above example)
	 * @param OutMap Map containing parsed results
	 * @param Filename Filename to use to find the section
	 *
	 * NOTE: The function naming is weird because you can't apparently have an overridden function differnt only by template type params
	 */
	virtual void Parse1ToNSectionOfStrings(const TCHAR* Section, const TCHAR* KeyOne, const TCHAR* KeyN, TMap<FString, TArray<FString> >& OutMap, const TCHAR* Filename)=0;

	/**
	 * Prases apart an ini section that contains a list of 1-to-N mappings of strings in the following format
	 *	 [PerMapPackages]
	 *	 MapName=Map1
	 *	 Package=PackageA
	 *	 Package=PackageB
	 *	 MapName=Map2
	 *	 Package=PackageC
	 *	 Package=PackageD
	 * 
	 * @param Section Name of section to look in
	 * @param KeyOne Key to use for the 1 in the 1-to-N (MapName in the above example)
	 * @param KeyN Key to use for the N in the 1-to-N (Package in the above example)
	 * @param OutMap Map containing parsed results
	 * @param Filename Filename to use to find the section
	 *
	 * NOTE: The function naming is weird because you can't apparently have an overridden function differnt only by template type params
	 */
	virtual void Parse1ToNSectionOfNames(const TCHAR* Section, const TCHAR* KeyOne, const TCHAR* KeyN, TMap<FName, TArray<FName> >& OutMap, const TCHAR* Filename)=0;

	virtual void Exit()=0;
	virtual void Dump( FOutputDevice& Ar )=0;

	/**
	 * Dumps memory stats for each file in the config cache to the specified archive.
	 *
	 * @param	Ar	the output device to dump the results to
	 */
	virtual void ShowMemoryUsage( FOutputDevice& Ar )=0;

	virtual ~FConfigCache() {};
};

// Notification hook.
class FNotifyHook
{
public:
	virtual void NotifyDestroy( void* Src ) {}
	virtual void NotifyPreChange( void* Src, UProperty* PropertyAboutToChange ) {}
	virtual void NotifyPostChange( void* Src, UProperty* PropertyThatChanged ) {}
	virtual void NotifyPreChange( class FEditPropertyChain* PropertyAboutToChange );
	virtual void NotifyPostChange( class FEditPropertyChain* PropertyThatChanged );
	virtual void NotifyExec( void* Src, const TCHAR* Cmd ) {}
};

// Interface for returning a context string.
class FContextSupplier
{
public:
	virtual FString GetContext()=0;
};

// Class for handling undo/redo transactions among objects.
typedef void( *STRUCT_AR )( FArchive& Ar, void* TPtr );
typedef void( *STRUCT_DTOR )( void* TPtr );

/**
 * Baseclass for transactions.
 */
class FTransactionBase
{
public:
	virtual void SaveObject( UObject* Object )=0;
	virtual void SaveArray( UObject* Object, FArray* Array, INT Index, INT Count, INT Oper, INT ElementSize, STRUCT_AR Serializer, STRUCT_DTOR Destructor )=0;

	/**
	 * Enacts the transaction.
	 *
	 */
	virtual void Apply()=0;
};

// File manager.
enum EFileTimes
{
	FILETIME_Create				= 0,
	FILETIME_LastAccess			= 1,
	FILETIME_LastWrite			= 2,
};
enum EFileWrite
{
	FILEWRITE_NoFail            = 0x01,
	FILEWRITE_NoReplaceExisting = 0x02,
	FILEWRITE_EvenIfReadOnly    = 0x04,
	FILEWRITE_Unbuffered        = 0x08,
	FILEWRITE_Append			= 0x10,
	FILEWRITE_AllowRead         = 0x20,
};
enum EFileRead
{
	FILEREAD_NoFail             = 0x01,
	FILEREAD_Uncached			= 0x02,
};
enum ECopyResult
{
	COPY_OK						= 0x00,
	COPY_MiscFail				= 0x01,
	COPY_ReadFail				= 0x02,
	COPY_WriteFail				= 0x03,
	COPY_Canceled				= 0x06,
};
#define COMPRESSED_EXTENSION	TEXT(".uz2")

struct FCopyProgress
{
	virtual UBOOL Poll( FLOAT Fraction )=0;
};


class FFileManager
{
public:

	/** Timestamp structure */
	struct timestamp
	{
		// Time is in UTC
		INT     Year;           /* year                             */
		INT     Month;          /* months since January - [0,11]    */
		INT     Day;            /* day of the month - [1,31]        */
		INT     Hour;           /* hours since midnight - [0,23]    */
		INT     Minute;         /* minutes after the hour - [0,59]  */
		INT     Second;         /* seconds after the minute - [0,59]*/
		INT     DayOfWeek;      /* days since Sunday - [0,6]        */
		INT     DayOfYear;      /* days since January 1 - [0,365]   */

		INT     GetJulian     ( void )         const;
		INT     GetSecondOfDay( void )         const;
		UBOOL   operator< ( timestamp& Other ) const;
		UBOOL   operator> ( timestamp& Other ) const;        
		UBOOL   operator==( timestamp& Other ) const;        
		UBOOL   operator>=( timestamp& Other ) const;        
		UBOOL   operator<=( timestamp& Other ) const;        
		UBOOL   operator!=( timestamp& Other ) const;        
	};

	virtual void Init(UBOOL Startup) {}
	virtual FArchive* CreateFileReader( const TCHAR* Filename, DWORD ReadFlags=0, FOutputDevice* Error=GNull )=0;
	virtual FArchive* CreateFileWriter( const TCHAR* Filename, DWORD WriteFlags=0, FOutputDevice* Error=GNull )=0;
	virtual INT FileSize( const TCHAR* Filename )=0;
	/**
	 * If the given file is compressed, this will return the size of the uncompressed file,
	 * if the platform supports it.
	 * @param Filename Name of the file to get information about
	 * @return Uncompressed size if known, otherwise -1
	 */
	virtual INT UncompressedFileSize( const TCHAR* Filename )=0;
	/**
	 * Returns the starting "sector" of a file on its storage device (ie DVD)
	 * @param Filename Name of the file to get information about
	 * @return Starting sector if known, otherwise -1
	 */
	virtual INT GetFileStartSector( const TCHAR* Filename )=0;
	virtual UBOOL IsReadOnly( const TCHAR* Filename )=0;
	virtual UBOOL Delete( const TCHAR* Filename, UBOOL RequireExists=0, UBOOL EvenReadOnly=0 )=0;
	virtual DWORD Copy( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0, FCopyProgress* Progress=NULL )=0;
	virtual UBOOL Move( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0 )=0;

	/**
	 * Updates the modification time of the file on disk to right now, just like the unix touch command
	 * @param Filename Path to the file to touch
	 * @return TRUE if successful
	 */
	virtual UBOOL TouchFile(const TCHAR* Filename)=0;

	/**
	 * Creates filenames belonging to the set  "Base####.Extension" where #### is a 4-digit number in [0000-9999] and
	 * no file of that name currently exists.  The return value is the index of first empty filename, or -1 if none
	 * could be found.  Clients that call FindAvailableFilename repeatedly will want to cache the result and pass it
	 * in to the next call thorugh the StartVal argument.  Base and Extension must valid pointers.
	 * Example usage:
	 * \verbatim
	   // Get a free filename of form <appGameDir>/SomeFolder/SomeFilename####.txt
	   FString Output;
	   FindAvailableFilename( *(appGameDir() * TEXT("SomeFolder") * TEXT("SomeFilename")), TEXT("txt"), Output );
	   \enverbatim
	 *
	 * @param	Base			Filename base, optionally including a path.
	 * @param	Extension		File extension.
	 * @param	OutFilename		[out] A free filename (untouched on fail).
	 * @param	StartVal		[opt] Can be used to hint beginning of index search.
	 * @return					The index of the created file, or -1 if no free file with index (StartVal, 9999] was found.
	 */
	virtual INT FindAvailableFilename( const TCHAR* Base, const TCHAR* Extension, FString& OutFilename, INT StartVal=-1 )=0;

	virtual UBOOL MakeDirectory( const TCHAR* Path, UBOOL Tree=0 )=0;
	virtual UBOOL DeleteDirectory( const TCHAR* Path, UBOOL RequireExists=0, UBOOL Tree=0 )=0;
	virtual void FindFiles( TArray<FString>& FileNames, const TCHAR* Filename, UBOOL Files, UBOOL Directories )=0;
	virtual DOUBLE GetFileAgeSeconds( const TCHAR* Filename )=0;
	virtual DOUBLE GetFileTimestamp( const TCHAR* Filename )=0;
	virtual UBOOL SetDefaultDirectory()=0;
	virtual FString GetCurrentDirectory()=0;
	/** 
	 * Get the timestamp for a file
	 * @param Path Path for file
	 * @Timestamp Output timestamp
	 * @return success code
	 */
	virtual UBOOL GetTimestamp( const TCHAR* Path, timestamp& Timestamp )=0;
};

class FEdLoadError;

/**
 * Base abstract class for an object propagator. Specific subclasses will be written
 * for propagating object property changes from the editor into either the 
 * Play From Here window or a console connected across the network.
 */
class FObjectPropagator : public FExec
{
public:
	virtual UBOOL Connect() { return true; }
	virtual void Disconnect() {}
	virtual void OnPropertyChange(UObject* Object, UProperty* Property, INT PropertyOffset) {};
	virtual void OnObjectRename(UObject* Object, const TCHAR* NewName) {}
	virtual void OnActorMove(class AActor* Actor) {}
	virtual void OnActorCreate(class AActor* Actor) {}
	virtual void OnActorDelete(class AActor* Actor) {}

	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar ) { return 0; }

	virtual void Tick(FLOAT DeltaTime) {}
	virtual void SetIPAddress(DWORD RemoteIPAddress, UBOOL bIsIntelByteOrder) {}

	static void SetPropagator(FObjectPropagator* InPropagator);
	static void ClearPropagator();

	static void Pause();
	static void Unpause();

	/**
	 * Send an entire actor over the propagator, including any special properties
	 * @param Actor The actor to propagate
	 */
	virtual void PropagateActor(class AActor* Actor) {}

protected:
	static DWORD Paused;
};

/*----------------------------------------------------------------------------
	Global variables.
----------------------------------------------------------------------------*/

class FOutputDeviceRedirector;

// Core globals.
extern FMemStack				GMem;
extern FConfigCache*			GConfig;
extern FTransactionBase*		GUndo;
extern FOutputDeviceConsole*	GLogConsole;
extern FMalloc*					GMalloc;
extern FFileManager*			GFileManager;
extern USystem*					GSys;
extern UProperty*				GProperty;
/** Points to the UProperty currently being serialized */
extern UProperty*				GSerializedProperty;
extern BYTE*					GPropAddr;
extern UObject*					GPropObject;
extern DWORD					GRuntimeUCFlags;
extern class UPropertyWindowManager*	GPropertyWindowManager;
//extern USubsystem*				GPropertyWindowManager;
extern TCHAR				    GErrorHist[16384];
extern TCHAR					GTrue[64], GFalse[64], GYes[64], GNo[64], GNone[64];
extern DOUBLE					GSecondsPerCycle;
extern DWORD					GUglyHackFlags;

/** Helper function to flush resource streaming. */
extern void						(*GFlushStreamingFunc)(void);

enum EUglyHackFlags
{
	HACK_DisableSubobjectInstancing		=	0x01,
	HACK_ClassLoadingDisabled			=	0x02,	// indicates that classes aren't being loaded from disk, thus all classes should be treated as intrinsic classes.
	HACK_UpdateArchetypeFromInstance	=	0x04,
	HACK_IsReloadObjArc					=	0x10,

	/**
	 * prevents component instancing code from instancing new components from templates;
	 * useful in certain commandlets [like diffpackage] that want to look at data from disk only.
	 * does not prevent existing instanced components from being loaded from disk.
	 */
	HACK_DisableComponentCreation		=	0x40,

	/** Disable loading of objects not contained within script files; used during script compilation */
	HACK_VerifyObjectReferencesOnly		=	0x80,
};


extern class FCallbackEventObserver*	GCallbackEvent;
extern class FCallbackQueryDevice*	GCallbackQuery;

/** 
 *	True if we are in the editor. 
 *	Note that this is still true when using Play In Editor. You may want to use GWorld->HasBegunPlay in that case.
 */
#if !CONSOLE
extern UBOOL					GIsEditor;
extern UBOOL					GIsUCC;
extern UBOOL					GIsUCCMake;
#else
#define GIsEditor				0
#define GIsUCC					0
#define GIsUCCMake				0
#endif // CONSOLE

extern UBOOL					GEdSelectionLock;
extern UBOOL					GIsClient;
extern UBOOL					GIsServer;
extern UBOOL					GIsCriticalError;
extern UBOOL					GIsStarted;
extern UBOOL					GIsRunning;
extern UBOOL					GIsGarbageCollecting;
extern UBOOL					GIsReplacingObject;

/**
 * These are set when the engine first starts up.
 **/

/**
 * This determines if we should pop up any dialogs.  If Yes then no popping up dialogs.
 **/
extern UBOOL					GIsUnattended;

/**
 * This determines if we should output any log text.  If Yes then no log text should be emitted.
 **/
extern UBOOL					GIsSilent;

/**
 * Used by non-UObject constructors of UObjects with multiple inheritance to
 * determine whether we're constructing the class default object
 */
extern UBOOL					GIsAffectingClassDefaultObject;
extern UBOOL					GIsSlowTask;
extern UBOOL					GIsGuarded;
extern UBOOL					GIsRequestingExit;
extern UBOOL					GUnicode;
extern UBOOL					GUnicodeOS;
extern class FGlobalMath		GMath;
extern class FArchive*			GDummySave;
/** Archive for serializing arbitrary data to and from memory						*/
extern class FReloadObjectArc*	GMemoryArchive;
extern TArray<FEdLoadError>		GEdLoadErrors;
extern UDebugger*				GDebugger; //DEBUGGER
extern UBOOL					GIsBenchmarking;
/* Whether we are dumping screenshots */
extern UBOOL					GIsDumpingMovie;
extern QWORD					GMakeCacheIDIndex;
extern TCHAR					GEngineIni[1024];
extern TCHAR					GEditorIni[1024];
extern TCHAR					GEditorUserSettingsIni[1024];
extern TCHAR					GInputIni[1024];
extern TCHAR					GGameIni[1024];
extern TCHAR					GUIIni[1024];
extern FLOAT					NEAR_CLIPPING_PLANE;
/** Current delta time in seconds.													*/
extern DOUBLE					GDeltaTime;
extern DOUBLE					GCurrentTime;
extern INT						GSRandSeed;
extern UBOOL					GExitPurge;
extern TCHAR					GGameName[64];
/** Exec handler for game debugging tool, allowing commands like "editactor", ...	*/
extern FExec*					GDebugToolExec;
/** Whether we are currently cooking.												*/
extern UBOOL					GIsCooking;
/** Whether we're currently in the async loading codepath or not					*/
extern UBOOL					GIsAsyncLoading;
/** The global object property propagator											*/
extern FObjectPropagator*		GObjectPropagator;
/** Whether to allow execution of Epic internal code like e.g. TTP integration, ... */
extern UBOOL					GIsEpicInternal;
/** Whether GWorld points to the play in editor world								*/
extern UBOOL					GIsPlayInEditorWorld;
/** TRUE if a normal or PIE game is active (basically !GIsEditor || GIsPlayInEditorWorld) */
extern UBOOL					GIsGame;
/** TRUE if the runtime needs textures to be powers of two							*/
extern UBOOL					GPlatformNeedsPowerOfTwoTextures;
/** TRUE if we're associating a level with the world, FALSE otherwise				*/
extern UBOOL					GIsAssociatingLevel;
/** Global IO manager																*/
extern struct FIOManager*		GIOManager;
/** Time at which appSeconds() was first initialized (very early on)				*/
extern DOUBLE					GStartTime;
/** Whether to use the seekfree package map over the regular linker based one.		*/
extern UBOOL					GUseSeekFreePackageMap;
/** Whether we are currently precaching or not.										*/
extern UBOOL					GIsPrecaching;
/** Whether we are still in the initial loading proces.								*/
extern UBOOL					GIsInitialLoad;
/** Whether we are currently purging an object in the GC purge pass.				*/
extern UBOOL					GIsPurgingObject;
/** TRUE when we are routing ConditionalPostLoad/PostLoad to objects				*/
extern UBOOL					GIsRoutingPostLoad;
/** Steadily increasing frame counter.												*/
extern QWORD					GFrameCounter;

#if !SHIPPING_PC_GAME
// We cannot count on this variable to be accurate in a shipped game, so make sure no code tries to use it
/** Whether we are the first instance of the game running.							*/
extern UBOOL					GIsFirstInstance;
#endif

/** Whether to always use the compression resulting in the smallest size.			*/
extern UBOOL					GAlwaysBiasCompressionForSize;
/** The number of full speed hardware threads the system has.						*/
extern UINT						GNumHardwareThreads;
/** This flag signals that the rendering code should throw up an appropriate error.	*/
extern UBOOL					GHandleDirtyDiscError;
/** Whether to forcefully enable capturing of scoped script stats (if > 0).			*/
extern INT						GForceEnableScopedCycleStats;
/** Size to break up data into when saving compressed data							*/
extern INT						GSavingCompressionChunkSize;
/** Total amount of calls to appSeconds and appCycles.								*/
extern QWORD					GNumTimingCodeCalls;
/** Whether we are using the seekfree/ cooked loading codepath.						*/
#if CONSOLE
#define							GUseSeekFreeLoading 1
#else
extern UBOOL					GUseSeekFreeLoading;
#endif

#if ENABLE_SCRIPT_TRACING
/** Whether we are tracing script to a log file */
extern UBOOL					GIsUTracing;
#endif

/** Whether to emit begin/ end draw events.											*/
extern UBOOL					GEmitDrawEvents;

// Per module globals.
extern "C" TCHAR GPackage[];

// Normal includes.
#include "UnFile.h"						// Low level utility code.
#include "UnObjVer.h"					// Object version info.
#include "UnArc.h"						// Archive class.
#include "UnTemplate.h"					// Dynamic arrays.
#include "ResourceArray.h"				// Resource array definitions.
#include "RefCounting.h"				// Reference counting definitions.
#include "RingBuffer.h"					// Ring buffer definitions.
#include "BitArray.h"					// Bit array definition.
#include "SparseArray.h"				// Sparse array definitions.
#include "ChunkedArray.h"				// Chunked array definitions.
#include "UnName.h"						// Global name subsystem.
#include "UnStack.h"					// Script stack definition.
#include "UnScriptMacros.h"				// Script macro definitions
#include "UnObjBas.h"					// Object base class.
#include "HashSet.h"					// Hash set definitions.
#include "UnMath.h"						// Vector math functions.
#include "SHMath.h"						// SH math functions.
#include "Random.h"						// Random math functions.
#include "CoreClasses.h"				// Low level utility code.
#include "UnCoreNet.h"					// Core networking.
#include "UnCorObj.h"					// Core object class definitions.
#include "UnObjGC.h"					// Realtime garbage collection helpers
#include "UnClass.h"					// Class definition.
#include "Casts.h"                      // Templated casts
#include "UnType.h"						// Base property type.
#include "UnScript.h"					// Script class.
#include "UFactory.h"					// Factory definition.
#include "UExporter.h"					// Exporter definition.
#include "UnMem.h"						// Stack based memory management.
#include "UnCId.h"						// Cache ID's.
#include "UnBits.h"						// Bitstream archiver.
#include "FCallbackDevice.h"			// Base class for callback devices.
#include "UnThreadingBase.h"			// Non-platform specific multi-threaded support.
#include "UnAsyncWork.h"				
#include "UnOutputDevices.h"			// Output devices
#include "UnObjectRedirector.h"			// Cross-package object redirector
#include "UnArchive.h"					// Utility archive classes
#include "UnBulkData.h"					// Bulk data classes
#include "PerfCounter.h"				// Serialization performance tracking classes
#include "UnLinker.h"					// Linker.
#include "FSerializableObject.h"		// non- UObject object serializer
#include "UnIOBase.h"					// base IO declarations, FIOManager, FIOSystem
#include "UnAsyncLoading.h"				// FAsyncPackage definition

#if TRACK_SERIALIZATION_PERFORMANCE || LOOKING_FOR_PERF_ISSUES
/** Tracks time spent serializing UObject data, per object type */
extern FStructEventMap* GObjectSerializationPerfTracker;

/** Tracks the amount of time spent in UClass serialization, per class */
extern FStructEventMap* GClassSerializationPerfTracker;

#endif

/** Identifier for specifying the type of map check message. */
enum MapCheckType
{
	MCTYPE_ERROR	= 0,
	MCTYPE_WARNING	= 1,
	MCTYPE_NOTE		= 2,
	MCTYPE_KISMET   = 3,
	MCTYPE_INFO     = 4,
	MCTYPE_NUM
};

/** Identifier for specifying the recommended action to take when dealing with a check message. */
enum MapCheckAction
{
	MCACTION_NONE	= 0,
	MCACTION_DELETE	= 1,
	MCACTION_NUM
};

/** A context for displaying modal warning messages. */
class FFeedbackContext : public FOutputDevice
{
public:
	VARARG_DECL( virtual UBOOL, UBOOL, return, YesNof, VARARG_PURE, const TCHAR*, VARARG_NONE, VARARG_NONE );
	virtual void BeginSlowTask( const TCHAR* Task, UBOOL StatusWindow )=0;
	virtual void EndSlowTask()=0;
	VARARG_DECL( virtual UBOOL, UBOOL VARARGS, return, StatusUpdatef, VARARG_PURE, const TCHAR*, VARARG_EXTRA(INT Numerator) VARARG_EXTRA(INT Denominator), VARARG_EXTRA(Numerator) VARARG_EXTRA(Denominator) );

	virtual FContextSupplier* GetContext() const { return NULL; }
	virtual void SetContext( FContextSupplier* InSupplier ) {}


	/** Map checking interface */

	/**
	 * @return	TRUE if a map check is currently active.
	 */
	virtual UBOOL MapCheck_IsActive() const { return FALSE; }

	/**
	 * Shows the map check dialog.
	 */
	virtual void MapCheck_Show() {};

	/**
	 * Same as MapCheck_Show, except it won't display the map check dialog if there are no errors in it.
	 */
	virtual void MapCheck_ShowConditionally() {};

	/**
	 * Hides the map check dialog.
	 */
	virtual void MapCheck_Hide() {};

	/**
	 * Clears out all errors/warnings.
	 */
	virtual void MapCheck_Clear() {};

	/** Called around bulk MapCheck_Add calls. */
	virtual void MapCheck_BeginBulkAdd() {};

	/**
	 * Adds a message to the map check dialog, to be displayed when the dialog is shown.
	 *
	 * @param	InType					The	type of message.
	 * @param	InActor					Actor associated with the message; can be NULL.
	 * @param	InMessage				The message to display.
	 * @param	InRecommendedAction		[opt] The recommended course of action to take; default is MCACTION_NONE.
	 * @param	InUDNPage				UDN Page to visit if the user needs more info on the warning.  This will send the user to https://udn.epicgames.com/Three/MapErrors#InUDNPage. 
	 */
	virtual void MapCheck_Add(MapCheckType InType, UObject* InActor, const TCHAR* InMessage, MapCheckAction bRecommendDeletion=MCACTION_NONE, const TCHAR* InUDNPage=TEXT("")) {};

	/** Called around bulk MapCheck_Add calls. */
	virtual void MapCheck_EndBulkAdd() {};

	TArray<FString> Warnings;
	TArray<FString> Errors;

	UBOOL	TreatWarningsAsErrors;

	/**
	* A pointer to the editors frame window.  This gives you the ability to parent windows
	* correctly in projects that are at a lower level than UnrealEd.
	*/
	DWORD	winEditorFrame;				
	DWORD	hWndEditorFrame;				

	FFeedbackContext() :
		 TreatWarningsAsErrors( 0 )
		, winEditorFrame( 0 )
		, hWndEditorFrame( 0 )
	{}
};

// Worker class for tracking loading errors in the editor
class FEdLoadError
{
public:
	FEdLoadError()
	{}
	FEdLoadError( INT InType, TCHAR* InDesc )
	{
		Type = InType;
		Desc = InDesc;
	}
	~FEdLoadError()
	{}

	// The types of things that could be missing.
	enum
	{
		TYPE_FILE		= 0,	// A totally missing file
		TYPE_RESOURCE	= 1,	// Texture/Sound/StaticMesh/etc
	};

	INT Type;		// TYPE_
	FString Desc;	// Description of the error

	UBOOL operator==( const FEdLoadError& LE ) const
	{
		return Type==LE.Type && Desc==LE.Desc;
	}
	FEdLoadError& operator=( const FEdLoadError Other )
	{
		Type = Other.Type;
		Desc = Other.Desc;
		return *this;
	}
};

/** Coordinate system identifiers. */
enum ECoordSystem
{
	COORD_None	= -1,
	COORD_World,
	COORD_Local,
	COORD_Max,
};

/** Very basic abstract debugger class. */
class UDebugger
{
public:
	virtual ~UDebugger() {};

	virtual void  DebugInfo( const UObject* Debugee, const FFrame* Stack, BYTE OpCode, INT LineNumber, INT InputPos )=0;

	virtual void  NotifyBeginTick()=0;
	virtual void  NotifyGC()=0;
	virtual void  NotifyAccessedNone()=0;
	virtual UBOOL NotifyAssertionFailed( const INT LineNumber )=0;
	virtual UBOOL NotifyInfiniteLoop()=0;
};

#include "UnCoreNative.h"

#include "UnThreadingBase.h"

#include "UnStats.h"

/** Memory stats */
enum EMemoryStats
{
	STAT_AudioMemory = STAT_MemoryFirstStat,
	STAT_PhysicalAllocSize,
#if PS3
	STAT_GPUAllocSize,
	STAT_HostAllocSize,
#else
	STAT_VirtualAllocSize,
#endif
	STAT_MemoryNovodexTotalAllocationSize,
	STAT_VertexLightingMemory,
	STAT_StaticMeshVertexMemory,
	STAT_StaticMeshIndexMemory,
	STAT_SkeletalMeshVertexMemory,
	STAT_SkeletalMeshIndexMemory,
	STAT_VertexShaderMemory,
	STAT_PixelShaderMemory,
	STAT_FaceFXPeakAllocSize,
	STAT_FaceFXCurrentAllocSize,
	STAT_TextureMemory,

	STAT_GameToRendererMallocPSSP,
	STAT_GameToRendererMallocSkMSP,
	STAT_GameToRendererMallocStMSP,
	STAT_GameToRendererMallocLFSP,
	STAT_GameToRendererMallocOther,
	STAT_GameToRendererMallocTotal,
	STAT_GameToRendererMalloc,
	STAT_GameToRendererFree,
	STAT_GameToRendererNet,

#if PS3
	// NOTE: These enums must match EAllocationType (PSGcmMalloc.h)!
	STAT_FirstGPUStat,
	
	STAT_LocalCommandBufferSize,
	STAT_LocalFrameBufferSize,
	STAT_LocalZBufferSize,
	STAT_LocalRenderTargetSize,
	STAT_LocalTextureSize,
	STAT_LocalVertexShaderSize,
	STAT_LocalPixelShaderSize,
	STAT_LocalVertexBufferSize,
	STAT_LocalIndexBufferSize,
	STAT_LocalRingBufferSize,
	STAT_LocalCompressionTagSize,
	STAT_LocalResourceArraySize,
	STAT_HostCommandBufferSize,
	STAT_HostFrameBufferSize,
	STAT_HostZBufferSize,
	STAT_HostRenderTargetSize,
	STAT_HostTextureSize,
	STAT_HostVertexShaderSize,
	STAT_HostPixelShaderSize,
	STAT_HostVertexBufferSize,
	STAT_HostIndexBufferSize,
	STAT_HostRingBufferSize,
	STAT_HostCompressionTagSize,
	STAT_HostResourceArraySize,

	STAT_LastGPUStat,
#endif
};

/**
 * Holds the list of stat ids for object stat gathering
 */
enum EObjectStats
{
	STAT_ConstructObject = STAT_ObjectFirstStat,
	STAT_LoadConfig,
	STAT_LoadLocalized,
	STAT_LoadObject,
	STAT_FindObject,
	STAT_FindObjectFast,
	STAT_InitProperties,
	STAT_NameTableEntries,
	STAT_NameTableMemorySize,
};

/** Threading stats */
enum EThreadingStats
{
	STAT_RenderingIdleTime = STAT_ThreadingFirstStat,
	STAT_RenderingBusyTime,
	STAT_GameIdleTime,
	STAT_GameTickWaitTime,
	STAT_GameTickWantedWaitTime,
	STAT_GameTickAdditionalWaitTime,
};

/**
 * Async IO stats - STATGROUP_AsyncIO
 */
enum EAsyncIOStats
{
	STAT_AsyncIO_FulfilledReadCount	= STAT_AsyncIOFirstStat,
	STAT_AsyncIO_FulfilledReadSize,
	STAT_AsyncIO_CanceledReadCount,
	STAT_AsyncIO_CanceledReadSize,
	STAT_AsyncIO_OutstandingReadCount,
	STAT_AsyncIO_OutstandingReadSize,
	STAT_AsyncIO_PlatformReadTime,
	STAT_AsyncIO_UncompressorWaitTime,
	STAT_AsyncIO_MainThreadBlockTime,
};

/*-----------------------------------------------------------------------------
	Seekfree defines.
-----------------------------------------------------------------------------*/

#define STANDALONE_SEEKFREE_SUFFIX	TEXT("_SF")

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif

