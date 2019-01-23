#ifndef NX_PHYSICS_NX_COMPARTMENT_DESC
#define NX_PHYSICS_NX_COMPARTMENT_DESC
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

enum NxCompartmentType
	{
	NX_SCT_RIGIDBODY,
	NX_SCT_FLUID,
	NX_SCT_CLOTH
	};

enum NxDeviceCode
	{
	NX_DC_PPU_0	= 0,							//!< Explicit PPU index corresponds to index numeric value (reserved for future use, currently only PPU 0 is valid)
	NX_DC_PPU_1 = 1,
	NX_DC_PPU_2 = 2,
	NX_DC_PPU_3 = 3,
	NX_DC_PPU_4 = 4,
	NX_DC_PPU_5 = 5,
	NX_DC_PPU_6 = 6,
	NX_DC_PPU_7 = 7,
	NX_DC_PPU_8 = 8,
	// ...

	NX_DC_CPU = 0xffff0000,						//!< Compartment is to be simulated on CPU
	NX_DC_PPU_AUTO_ASSIGN = 0xffff0001,			//!< Compartment is to be simulated on a PPU, chosen automatically if there is more than one.
	};


/**
\brief Descriptor class for NxCompartment.  A compartment is a portion of the scene that can
be simulated on a different hardware device than other parts of the scene.
*/
class NxCompartmentDesc
	{
	public:
	NxCompartmentType type;						//!< Compartment meant for this type of simulation.  Will be removed in 3.0, as comparments will become type-neutral.
	NxU32			deviceCode;					//!< A NxDeviceCode, incl. a PPU index from 0 to 31.
	NxReal			gridHashCellSize;			//!< Size in distance units of a single cell in the paging grid.  Should be set to the size of the largest common dynamic object in this compartment.
	NxU32			gridHashTablePower;			//!< 2-power used to determine size of the hash table that geometry gets hashed into.  Hash table size is (1 << gridHashTablePower).

	NX_INLINE void setToDefault()
		{
		type = NX_SCT_RIGIDBODY;
		deviceCode = NX_DC_CPU;
		gridHashCellSize = 100.0f;				//was 2.0f in 2.5, bumped up.
		gridHashTablePower = 8;
		}

	/**
	\brief Returns true if the descriptor is valid.

	\return return true if the current settings are valid
	*/

	NX_INLINE bool isValid() const
		{
		if (deviceCode != NX_DC_PPU_0 && 
			deviceCode != NX_DC_PPU_AUTO_ASSIGN &&
			deviceCode != NX_DC_CPU)
			return false;
		return (type <= NX_SCT_CLOTH) && (gridHashCellSize > 0.0f);
		}

	NxCompartmentDesc()
		{
		setToDefault();
		}
	};

#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
