#ifndef __CUDACC__
#include "NxApex.h"
#endif

#ifndef NX_USER_RENDER_SPRITEBUFFER_DESC_H
#define NX_USER_RENDER_SPRITEBUFFER_DESC_H

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

/*!
\file
\brief class NxUserRenderSpriteBufferDesc, structs NxRenderDataFormat and NxRenderSpriteSemantic
*/


namespace physx
{
namespace pxtask
{
	class CudaContextManager;
}
};

namespace physx {
namespace apex {

PX_PUSH_PACK_DEFAULT
#pragma warning(push)
#pragma warning(disable:4121)

/**
\brief potential semantics of a sprite buffer
*/
struct NxRenderSpriteSemantic
{
	enum Enum
	{
		POSITION = 0,	//!< Position of sprite
		COLOR,			//!< Color of sprite
		VELOCITY,		//!< Linear velocity of sprite
		SCALE,			//!< Scale of sprite
		LIFE_REMAIN,	//!< 1.0 (new) .. 0.0 (dead)
		DENSITY,		//!< Particle density at sprite location
		SUBTEXTURE,		//!< Sub-texture index of sprite
		ORIENTATION,	//!< 2D sprite orientation (angle in radians, CCW in screen plane)

		NUM_SEMANTICS	//!< Count of semantics, not a valid semantic.
	};
};

struct NxRenderSpriteTextureLayout
{
	enum Enum
	{
		NONE = 0,
		POSITION_FLOAT4, //float4(POSITION.x, POSITION.y, POSITION.z, 1)
		SCALE_ORIENT_SUBTEX_FLOAT4, //float4(SCALE.x, SCALE.y, ORIENTATION, SUBTEXTURE)
		COLOR_BGRA8,
	};
};

#ifndef __CUDACC__
/**
\brief describes the semantics and layout of a sprite buffer
*/
class NxUserRenderSpriteBufferDesc
{
public:
	NxUserRenderSpriteBufferDesc(void)
	{
		maxSprites = 0;
		hint         = NxRenderBufferHint::STATIC;
		registerInCUDA = false;
		interopContext = 0;

		for(physx::PxU32 i=0; i<NxRenderSpriteSemantic::NUM_SEMANTICS; i++)
		{
			semanticFormats[i] = NxRenderDataFormat::UNSPECIFIED;
			semanticOffsets[i] = physx::PxU32(-1);
		}
		stride = 0;
	}
	
	bool isValid(void) const
	{
		physx::PxU32 numFailed = 0;

		numFailed += (semanticFormats[NxRenderSpriteSemantic::POSITION] == NxRenderDataFormat::UNSPECIFIED);
		numFailed += registerInCUDA && (interopContext == 0);
		numFailed += registerInCUDA && (stride == 0);
		numFailed += registerInCUDA && (semanticOffsets[NxRenderSpriteSemantic::POSITION] == physx::PxU32(-1));
		
		return (numFailed == 0);
	}
	
public:
	physx::PxU32					maxSprites;		//!< The maximum number of sprites that APEX will store in this buffer
	NxRenderBufferHint::Enum		hint;			//!< A hint about the update frequency of this buffer


	/**
	\brief Array of the corresponding formats for each semantic

	NxRenderSpriteSemantic::UNSPECIFIED is used for semantics that are disabled
	*/
	NxRenderDataFormat::Enum		semanticFormats[NxRenderSpriteSemantic::NUM_SEMANTICS];

	/**
	\brief Array of the corresponding offsets (in bytes) for each semantic. Required when CUDA interop is used!
	*/
	physx::PxU32					semanticOffsets[NxRenderSpriteSemantic::NUM_SEMANTICS];

	physx::PxU32					stride; //!< The stride between sprites of this buffer. Required when CUDA interop is used!

	bool							registerInCUDA;  //!< Declare if the resource must be registered in CUDA upon creation

	/**
	This context must be used to register and unregister the resource every time the
	device is lost and recreated.
	*/
	physx::pxtask::CudaContextManager *interopContext;  
};
#endif

#pragma warning(pop)
PX_POP_PACK

}} // end namespace physx::apex

#endif
