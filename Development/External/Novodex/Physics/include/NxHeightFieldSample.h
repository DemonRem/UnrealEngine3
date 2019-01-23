#ifndef NX_PHYSICS_NXHEIGHTFIELDSAMPLE
#define NX_PHYSICS_NXHEIGHTFIELDSAMPLE
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/
/** \addtogroup physics
  @{
*/

#include "Nxp.h"

/**
\brief Heightfield sample format.

This format corresponds to the #NxHeightFieldFormat member NX_HF_S16_TM.

@see NxHeightfield NxHeightfieldDesc NxHeightfieldDesc.samples
*/
struct NxHeightFieldSample
	{
	NxI16			height			: 16;
	NxU8			materialIndex0	: 7;
	NxU8			tessFlag		: 1;
	NxU8			materialIndex1	: 7;
	NxU8			unused			: 1;
	};

/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
