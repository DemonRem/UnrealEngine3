/*=============================================================================
	GPUSkinPublicDefs.h: Public definitions for GPU skinning.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __GPUSKINPUBLICDEFS_H__
#define __GPUSKINPUBLICDEFS_H__

/** Max number of bones that can be skinned on the GPU in a single draw call. */
#define MAX_GPUSKIN_BONES 75
/** Max number of bone influences that a single skinned vert can have. */
#define MAX_INFLUENCES 4



namespace SkinningTools
{
	/**
	 * Returns the bone weight index to use for rigid skinning
	 *
	 * @return Bone weight index to use
	 */
	inline INT GetRigidInfluenceIndex()
	{
		// The index of the rigid influence in the BYTE[4] of influences for a skinned vertex, accounting for byte-swapping. 
#if __INTEL_BYTE_ORDER__
		return 0;
#elif WITH_ES2_RHI
		return GUsingES2RHI ? 0 : 3;
#else
		return 3;
#endif
	}
}


#endif // __GPUSKINPUBLICDEFS_H__
