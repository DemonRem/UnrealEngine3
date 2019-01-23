#ifndef NX_PHYSICS_NX_COMPARTMENT
#define NX_PHYSICS_NX_COMPARTMENT
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

#include "NxCompartmentDesc.h"

/**
\brief A scene compartment is a portion of the scene that can
be simulated on a different hardware device than other parts of the scene.

Note: You cannot release scene compartments explicitly, they are automatically released when the scene is released.
To create a compartment, call NxScene::createCompartment()
*/
class NxCompartment
	{
	public:
	/**
	\return the compartment simulation type.  Will be removed in 3.0, as compartments will become type-neutral.
	*/
	virtual NxCompartmentType getType() const = 0;

	/**
	\brief The device code that is specified when creating a compartment or the automatically
	assigned device code, if NX_DC_PPU_AUTO_ASSIGN was used.

	\return the ::NxDeviceCode of the compartment.
	*/
	virtual NxU32 getDeviceCode() const = 0;

	/**
	\return the paging grid cell size.
	*/
	virtual NxReal getGridHashCellSize() const = 0;

	/**
	\return the paging grid power.
	*/
	virtual NxU32 gridHashTablePower() const = 0;

	/**
	\brief Saves the compartment to a descriptor. Please note that the device code will be the same as the 
	value returned by getDeviceCode(), i.e. not necessarily the same as the value you assigned when first
	creating the compartment (when using auto-assign).

	\return true
	*/
	virtual bool saveToDesc(NxCompartmentDesc& desc) const = 0;
	};

#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
