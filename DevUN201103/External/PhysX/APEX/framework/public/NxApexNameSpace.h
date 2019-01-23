#include "NxApex.h"
#ifndef __NX_APEX_NAME_SPACE_H__
#define __NX_APEX_NAME_SPACE_H__
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
\brief Defines APEX namespace strings

These are predefined framework namespaces in the named resource provider
*/

/*!
\brief A namespace for names of collision groups (NxCollisionGroup).

Each name in this namespace must map to a 5-bit integer in the range [0..31] (stored in a void*).
The NRP will not automatically generate release requests for names in this namespace.
*/
#define APEX_COLLISION_GROUP_NAME_SPACE				"NSCollisionGroup"

/*!
\brief A namespace for names of NxGroupsMasks

Each name in this namespace must map to a pointer to a persistent 128-bit NxGroupsMask type.
The NRP will not automatically generate release requests for names in this namespace.
*/
#define APEX_COLLISION_GROUP_128_NAME_SPACE			"NSCollisionGroup128"

/*!
\brief A namespace for names of collision group masks

Each name in this namespace must map to a 32-bit integer (stored in a void*), wherein each
bit represents a collision group (NxCollisionGroup). The NRP will not automatically generate
release requests for names in this namespace. 
*/
#define APEX_COLLISION_GROUP_MASK_NAME_SPACE		"NSCollisionGroupMask"

/*!
\brief Internal namespace for authorable asset types

For examples, there are entries in this namespace for "ApexRenderMesh", "NxClothingAsset",
"DestructibleAsset", etc... 
The values stored in this namespace are the namespace IDs of the authorable asset types.
So if your module needs to get a pointer to a FooAsset created by module Foo, you can ask
the ApexSDK for that asset's namespace ID.
*/
#define APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE		"AuthorableAssetTypes"

/*!
\brief Internal namespace for parameterized authorable assets
*/
#define APEX_NX_PARAM_AUTH_ASSETS_TYPES_NAME_SPACE	"NxParamAuthorableAssetTypes"

/*!
\brief A namespace for graphical material names

Each name in this namespace maps to a pointer to a game-engine defined graphical material data
structure. APEX does not interpret or dereference this pointer in any way. APEX provides this
pointer to the NxUserRenderResource::setMaterial(void *material) callback to the rendering engine.
This mapping allows APEX assets to refer to game engine materials (e.g.: texture maps and shader
programs) without imposing any limitations on what a game engine graphical material can contain.
The NRP will not automatically generate release requests for names in this namespace.
*/
#define APEX_MATERIALS_NAME_SPACE					"ApexMaterials"

/*!
\brief A namespace for physical material names

Each name in this namespace maps to NxMaterialID, which is a data type defined
by the PhysX SDK. The NRP will not automatically generate release requests for
names in this namespace. 
*/
#define APEX_PHYSICS_MATERIAL_NAME_SPACE			"NSPhysicalMaterial"

/*!
\brief A namespace for custom vertex buffer semantics names

Each name in this namespace maps to a pointer to a game-engine defined data structure identifying
a custom vertex buffer semantic. APEX does not interpret or dereference this pointer in any way.
APEX provides an array of these pointers in NxUserRenderVertexBufferDesc::customBuffersIdents,
which is passed the rendering engine when requesting allocation of vertex buffers. 
*/
#define APEX_CUSTOM_VB_NAME_SPACE                   "NSCustomVBNames"

#endif
