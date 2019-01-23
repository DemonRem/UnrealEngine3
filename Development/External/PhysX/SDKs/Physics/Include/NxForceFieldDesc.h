#ifndef NX_PHYSICS_NXFORCEFIELDDESC
#define NX_PHYSICS_NXFORCEFIELDDESC
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

class NxActor;

/**
\brief Type of force field coordinate space
*/
enum NxForceFieldCoordinates { NX_FFC_CARTESIAN, NX_FFC_SPHERICAL, NX_FFC_CYLINDRICAL, NX_FFC_TOROIDAL };

enum NxForceFieldFlags 
	{ 
	NX_FFF_IGNORE_FLUID_MASS	 		= (1<<0),	//!< apply acceleration instead of force to fluids
	NX_FFF_IGNORE_CLOTH_MASS	 		= (1<<1),	//!< apply acceleration instead of force to cloth
	NX_FFF_IGNORE_SOFTBODY_MASS	 	= (1<<2),	//!< apply acceleration instead of force to soft bodies
	NX_FFF_IGNORE_RIGIDBODY_MASS		= (1<<3),	//!< apply acceleration instead of force to rigid bodies
	};



/**
 \brief Descriptor class for NxForceField class.


<b>Platform:</b>
\li PC SW: Yes
\li PPU  : Yes
\li PS3  : Yes
\li XB360: Yes

 @see NxForceField
*/
class NxForceFieldDesc
	{
	public:

	NxMat34					pose;					//!< Global or (if actor is set) actor relative transformation of the force field.
	NxActor*				actor;					//!< The field is attached to move with an actor. If null, the field is static.
	NxForceFieldCoordinates	coordinates;			//!< Coordinate space of the field.

	// filtering
	NxCollisionGroup		group;					//!< Collision group used for collision filtering.
	NxGroupsMask			groupsMask;				//!< Groups mask used for collision filtering. 

	//function parameters:
	NxVec3					constant;				//!< Constant part of force field function
	NxMat33					positionMultiplier;		//!< Coefficient of force field function position term
	NxVec3					positionTarget;			//!< Force field position target			
	NxMat33					velocityMultiplier;		//!< Coefficient of force field function velocity term
	NxVec3					velocityTarget;			//!< Force field velocity target

	NxVec3					falloffLinear;			//!< Force field velocity target
	NxVec3					falloffQuadratic;		//!< Force field velocity target

	NxReal					torusRadius;			//!< Radius for NX_FFC_TOROIDAL type coordinates.

	NxVec3					noise;					//<! Noise scaling

	NxReal					fluidScale;				//!< Force scale factor for fluids
	NxReal					clothScale;				//!< Force scale factor for cloth
	NxReal					softBodyScale;			//!< Force scale factor for soft bodies
	NxReal					rigidBodyScale;			//!< Force scale factor for rigid bodies

	NxU32					flags;					//!< Force field flags; @see NxForceFieldShapeFlags

	/**
	\brief Possible debug name. The string is not copied by the SDK, only the pointer is stored.

	<b>Default</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	const char* name;

	/**
	\brief Constructor sets to default.
	*/
	NX_INLINE NxForceFieldDesc();

	/**
	\brief (re)sets the structure to the default.	
	*/
	NX_INLINE void setToDefault();

	/**
	\brief Returns true if the descriptor is valid.

	\return true if the current settings are valid
	*/
	NX_INLINE bool isValid() const;
	};

NX_INLINE NxForceFieldDesc::NxForceFieldDesc()
	{
	setToDefault();
	}

NX_INLINE void NxForceFieldDesc::setToDefault()
	{
	pose.id();
	actor = NULL;
	coordinates = NX_FFC_CARTESIAN;

	group				= 0;
	groupsMask.bits0	= 0;
	groupsMask.bits1	= 0;
	groupsMask.bits2	= 0;
	groupsMask.bits3	= 0;

	constant.zero(); 
	positionMultiplier.zero();
	positionTarget.zero();
	velocityMultiplier.zero();
	velocityTarget.zero();
	falloffLinear.zero();
	falloffQuadratic.zero();
	noise.zero();
	torusRadius = 1.0f;

	flags = 0;

	fluidScale		= 1.0f;
	clothScale		= 1.0f;
	softBodyScale	= 1.0f;
	rigidBodyScale	= 1.0f;

	name = NULL;
	}

NX_INLINE bool NxForceFieldDesc::isValid() const
	{
	if(group>=32)
		return false;	// We only support 32 different groups
	if(torusRadius<0.0f)
		return false;
	return true;
	}



/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
