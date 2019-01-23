#ifndef NX_CLOTHING_MATERIAL_LIBRARY_AUTHORING_H
#define NX_CLOTHING_MATERIAL_LIBRARY_AUTHORING_H
/*
 * Copyright 2009-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and
 * international Copyright laws.  Users and possessors of this source code
 * are hereby granted a nonexclusive, royalty-free license to use this code
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOURCE CODE.
 *
 * U.S. Government End Users.   This source code is a "commercial item" as
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of
 * "commercial computer  software"  and "commercial computer software
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995)
 * and is provided to the U.S. Government only as a commercial end item.
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
 * source code with only those rights set forth herein.
 *
 * Any use of this source code in individual and commercial software must
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */
#include "NxApex.h"
#include "NxApexAsset.h"
#include "NxApexInterface.h"

namespace physx {
namespace apex {

PX_PUSH_PACK_DEFAULT

class NxClothingMaterial;

/**
\brief Container for clothing materials. Allows adding and removing materials as well.

\deprecated This class is going to be removed from 1.1
*/
class NxClothingMaterialLibraryAuthoring : public NxApexAssetAuthoring
{
public:
	/**
	\brief Releases the material library

	\note, this is only successful if all references from NxClothingActors to any of the materials are removed
	\see NxClothingMaterialLibraryAuthoring::getReferenceCount()
	*/
	virtual void release() = 0;

	/**
	\brief The user data pointer
	*/
	void* userData;

	/**
	\brief Adds a new material to this library.

	\note The name must be unique, but not persistent in memory, a copy of the name will be made inside.
	*/
	virtual NxClothingMaterial* createMaterial(const char* name) = 0;

	/**
	\brief Returns the number of Materials inside this material library.
	*/
	virtual physx::PxU32 getNumMaterials() const = 0;

	/**
	\brief Returns the material at index.

	\note index must be smaller than NxClothingMaterialLibraryAuthoring::getNumMaterials()
	*/
	virtual NxClothingMaterial* getMaterial(physx::PxU32 index) = 0;

	/**
	\brief Returns the sum of all materials' reference counts in this library
	*/
	virtual physx::PxU32 getReferenceCount() const = 0;

	/**
	\brief Queries whether any of the authoring materials have been changed since their creation.

	\note This can be used to query whether the material library needs to be saved to disk
	*/
	virtual bool hasChanged() const = 0;

	/**
	\brief Returns the name of the clothing material library that is being authored.
	*/
	virtual const char* getName() const = 0;

		/**
	 * \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	 */
	virtual NxParameterized::Interface * releaseAndReturnNxParameterizedInterface(void) 
	{
		return NULL;
	}
};


PX_POP_PACK

}} // namespace physx::apex

#endif
