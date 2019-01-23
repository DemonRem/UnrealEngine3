/*=============================================================================
	ResourceArray.h: Resource array definitions and platform includes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * An element type independent interface to the resource array.
 */
class FResourceArrayInterface
{
public:

	/**
	* @return A pointer to the resource data.
	*/
	virtual const void* GetResourceData() const = 0;

	/**
	* @return size of resource data allocation
	*/
	virtual DWORD GetResourceDataSize() const = 0;

	/**
	* Called on non-UMA systems after the RHI has copied the resource data, and no longer needs the CPU's copy.
	*/
	virtual void Discard() = 0;

	/**
	* @return TRUE if the resource array is static and shouldn't be modified
	*/
	virtual UBOOL IsStatic() const = 0;

#if PS3
	/**
	 * PS3-only
	 * @return TRUE if the array is allocated in system, not GPU, memory
	 */
	virtual UBOOL UsesSystemMemory() const
	{
		return FALSE;
	}
#endif
};

#if _WINDOWS
// PC D3D resource array 
#include "..\..\D3DDrv\Inc\D3DResourceArray.h"
#elif XBOX
// Xenon D3D resource array
#include "XeD3DResourceArray.h"
#elif PS3
// PS3 resource array
#include "PS3ResourceArray.h"
#else
// version not implemented
#error "missing platform ResourceArray implementation"
#endif

