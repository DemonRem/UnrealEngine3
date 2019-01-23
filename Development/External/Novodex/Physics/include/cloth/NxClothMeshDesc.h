#ifndef NX_PHYSICS_NX_CLOTHMESHDESC
#define NX_PHYSICS_NX_CLOTHMESHDESC
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

#include "NxSimpleTriangleMesh.h"

/**
\brief Cloth mesh targets for cooking.

@see NxClothMeshDesc.target
*/

enum NxClothMeshTarget
{
	/**
	\brief cook the cloth mesh for software mode
	*/
	NX_CLOTH_MESH_SOFTWARE		= 0,

	/**
	\brief cook the cloth mesh for hardware (PPU Athena) mode
	*/
	NX_CLOTH_MESH_PPU_ATHENA	= 1,
};

/**
\brief Cloth mesh flags.

@see NxClothMeshDesc.flags
*/

enum NxClothMeshFlags
	{
	/**
	\brief Specifies whether extra space is allocated for tearing on the PPU.
	If this flag is not set, less memory is needed on the PPU but tearing is not possible.
	These flags for clothMeshes extend the set of flags defined in NxMeshFlags.
	*/
	NX_CLOTH_MESH_TEARABLE	=	(1<<8),
	};

/**
\brief Cloth vertex flags.

@see NxClothMeshDesc.vertexFlags
*/

enum NxClothVertexFlags
	{
	/**
	\brief Specifies whether a cloth vertex can be torn.
	*/
	NX_CLOTH_VERTEX_TEARABLE	=	(1<<7),
	};

/*----------------------------------------------------------------------------*/

/**
\brief Descriptor class for #NxClothMesh.

Note that this class is derived from NxSimpleTriangleMesh which contains the
members that describe the basic mesh. The mesh data is *copied* when an
NxClothMesh object is created from this descriptor. After the creation the
user may discard the basic mesh data.

@see NxClothMesh NxSimpleTriangleMesh
*/

class NxClothMeshDesc : public NxSimpleTriangleMesh
{
public:
	NxU32 vertexMassStrideBytes;		//!< Offset between vertex masses in bytes.
	NxU32 vertexFlagStrideBytes;		//!< Offset between vertex flags in bytes.

	/**
	\brief Pointer to first vertex mass.
	
	Caller may add vertexMassStrideBytes bytes to the pointer to access the next vertex mass.
	*/
	const void* vertexMasses;

	/**
	\brief Pointer to first vertex flag. Flags are of type #NxClothVertexFlags.
	
	Caller may add vertexFlagStrideBytes bytes to the pointer to access the next vertex flag.
	*/
	const void* vertexFlags;

	/**
	\brief Defines how the cloth mesh is cooked.
	
	<b>Default:</b> NX_CLOTH_MESH_SOFTWARE

	@see NxClothMeshTarget
	*/
	NxClothMeshTarget target;

	/**
	\brief Constructor sets to default.
	*/
	NX_INLINE NxClothMeshDesc();

	/**
	\brief (Re)sets the structure to the default.	
	*/
	NX_INLINE void setToDefault();

	/**
	\brief Returns true if the current settings are valid
	*/
	NX_INLINE bool isValid() const;
};

/*----------------------------------------------------------------------------*/

NX_INLINE NxClothMeshDesc::NxClothMeshDesc()
{
	setToDefault();
}

/*----------------------------------------------------------------------------*/

NX_INLINE void NxClothMeshDesc::setToDefault()
{
	NxSimpleTriangleMesh::setToDefault();
	vertexMassStrideBytes	= sizeof(NxReal);
	vertexFlagStrideBytes	= sizeof(NxU32);
	vertexMasses			= NULL;
	vertexFlags				= NULL;
	target = NX_CLOTH_MESH_SOFTWARE;
}

/*----------------------------------------------------------------------------*/

NX_INLINE bool NxClothMeshDesc::isValid() const
{
	if (
		target != NX_CLOTH_MESH_SOFTWARE && 
		target != NX_CLOTH_MESH_PPU_ATHENA)
		return false;
	if(vertexMasses && (vertexMassStrideBytes < sizeof(NxReal)))
		return false;
	if(vertexFlags && (vertexFlagStrideBytes < sizeof(NxU32)))
		return false;

	return NxSimpleTriangleMesh::isValid();
}

/*----------------------------------------------------------------------------*/
/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
