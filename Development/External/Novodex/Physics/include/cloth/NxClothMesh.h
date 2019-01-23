#ifndef NX_PHYSICS_NX_CLOTHMESH
#define NX_PHYSICS_NX_CLOTHMESH
/** \addtogroup cloth
  @{
*/
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/

#include "Nxp.h"
#include "NxClothMeshDesc.h"

class NxStream;

class NxClothMesh
{
protected:
	NX_INLINE NxClothMesh() {}
	virtual ~NxClothMesh() {}

public:
	/**
	\brief Saves the cloth descriptor. 
	A cloth mesh is created via the cooker. The cooker potentially changes the
	order of the arrays references by the pointers points and triangles.
	Since saveToDesc returns the data of the cooked mesh, this data might
	differ from the orignally provided data.
	
	\param desc The descriptor used to retrieve the state of the object.
	\return True on success.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothMeshDesc
	*/
	virtual	bool saveToDesc(NxClothMeshDesc& desc) const = 0;

	/**
	\brief Gets the number of cloth instances referencing this cloth mesh.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxCloth
	*/
	virtual	NxU32 getReferenceCount() const  = 0;
};
/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
