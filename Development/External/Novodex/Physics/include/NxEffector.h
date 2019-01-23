#ifndef NX_PHYSICS_NXEFFECTOR
#define NX_PHYSICS_NXEFFECTOR
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

class NxSpringAndDamperEffector;
class NxScene;

/**
 \brief An effector is a class that gets called before each tick of the
 scene.
 
 At this point it may apply any permissible effect
 to the objects. For example: #NxSpringAndDamperEffector

<b>Platform:</b>
\li PC SW: Yes
\li PPU  : Yes
\li PS3  : Yes
\li XB360: Yes

 @see NxSpringAndDamperEffector NxScene.createSpringAndDamperEffector
*/
class NxEffector
	{
	public:
	/**
	\brief Attempts to perform a downcast to the type returned.
	
	Returns 0 if this object is not of the appropriate type.

	\return If this is a spring and damper constraint a pointer otherwise NULL.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxSpringAndDamperEffector()
	*/
	virtual		NxSpringAndDamperEffector*	isSpringAndDamperEffector() = 0;

	/**
	\brief Retrieves the owner scene

	\return The scene which this effector belongs to.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxScene
	*/
	virtual		NxScene&					getScene() = 0;

	void * userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.

	protected:
	virtual ~NxEffector(){};
	};

/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
