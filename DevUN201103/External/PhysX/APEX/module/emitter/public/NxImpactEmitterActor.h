#ifndef __NX_IMPACT_EMITTER_ACTOR_H__
#define __NX_IMPACT_EMITTER_ACTOR_H__
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

namespace physx {
namespace apex {


PX_PUSH_PACK_DEFAULT

class NxImpactEmitterAsset;
class NxApexRenderVolume;

///Impact emitter actor.  Emits particles at impact places
class NxImpactEmitterActor : public NxApexActor
{
protected:
	virtual ~NxImpactEmitterActor() {}

public:
	///Gets the pointer to the underlying asset
	virtual NxImpactEmitterAsset *	getEmitterAsset() const = 0;
	
	/**
	\brief Registers an impact in the queue
	
	\param hitPos impact position
	\param hitDir impact direction
	\param surfNorm normal of the surface that is hit by the impact
	\param setID - id for the event set which should be spawned. Specifies the behavior. \sa NxImpactEmitterAsset::querySetID 

	*/
	virtual void registerImpact( const physx::PxVec3 & hitPos, const physx::PxVec3 & hitDir, const physx::PxVec3 & surfNorm, physx::PxU32 setID ) = 0;

	///Emitted particles are injected to specified render volume on initial frame.
	///Set to NULL to clear the preferred volume.
	virtual void setPreferredRenderVolume( NxApexRenderVolume* volume ) = 0;
};


PX_POP_PACK

}} // end namespace physx::apex

#endif
