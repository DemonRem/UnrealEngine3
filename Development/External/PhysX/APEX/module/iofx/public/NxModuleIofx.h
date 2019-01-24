#ifndef __NX_MODULE_IOFX_H__
#define __NX_MODULE_IOFX_H__
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

class NxIofxAsset;
class NxIofxAssetAuthoring;
class NxApexRenderVolume;

/**
IOFX Module. This module is used to convert physical parameters of particles into visual parameters
that can be used to render particles.  NxApexRenderVolume provide the ability to partition world space
into separate renderables.
*/
class NxModuleIofx : public NxModule
{
protected:
	virtual							~NxModuleIofx() {}

public:
	/// Disable use of OGL/D3D Interop.  Must be called before emitters are created to have any effect
	virtual void                    disableCudaInterop() = 0;

	/// Disable use of CUDA IOFX modifiers. Must be called before emitters are created to have any effect
	virtual void                    disableCudaModifiers() = 0;

	/** \brief Disables deferred allocation of IOFX actors
     *
     * By default, IOFX actors and their render resources will not be
     * created until it has objects to render.
     */
	virtual void                    disableDeferredRenderableAllocation() = 0;

	/** \brief Disables triple buffering of interop graphical resources
     *
     * By default, when OGL/D3D interop is in use the IOFX module will triple buffer render resources
	 * to allow the game render thread to be detached from the simulation thread.  If the game can
	 * guarantee that NxApexScene::prepareRenderResourceContexts() will be called by the render thread
	 * between NxApexScene::fetchResults() and NxApexScene::simulate() of the next simulation step, it
	 * may call this method to restrict interop to double buffering.  Must be called before any
	 * emitters are created to have any effect.
     */
	virtual void                    onlyDoubleBufferInteropResources() = 0;

    /** \brief Create a new render volume
     *
     * Render volumes may be created at any time, unlike most other APEX
     * asset and actor types.  Their insertion into the scene is
     * deferred if the simulation is active.
     */
	virtual NxApexRenderVolume *    createRenderVolume( const NxApexScene& apexScene, const PxBounds3& b, PxU32 priority, bool allIofx ) = 0;

    /** \brief Release a render volume
     *
     * Render volumes may be released at any time, unlike most other APEX
     * asset and actor types.  If the simulation is active, their
     * resources are released at the end of the simulation step.
     */
	virtual void                    releaseRenderVolume( NxApexRenderVolume& volume ) = 0;
};

#if !defined(_USRDLL)
#define instantiateModuleIOFX	instantiateModuleIofx
void instantiateModuleIofx();
#endif

PX_POP_PACK

}} // namespace physx::apex

#endif // #ifndef __NX_MODULE_IOFX_H__