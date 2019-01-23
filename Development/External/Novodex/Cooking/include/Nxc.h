#ifndef NX_COOKING_NX
#define NX_COOKING_NX
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/
/**
DLL export macros
*/
#ifndef NXC_DLL_EXPORT
	#ifdef NX_COOKING_DLL

		#define NXC_DLL_EXPORT __declspec(dllexport)

	#elif defined NX_COOKING_STATICLIB

		#define NXC_DLL_EXPORT

	#elif defined NX_USE_SDK_DLLS

		#define NXC_DLL_EXPORT __declspec(dllimport)

	#elif defined NX_USE_SDK_STATICLIBS

		#define NXC_DLL_EXPORT

	#else

		#define NXC_DLL_EXPORT __declspec(dllimport)
	
	#endif
#endif

#ifndef NX_C_EXPORT
	#define NX_C_EXPORT extern "C"
#endif

#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
