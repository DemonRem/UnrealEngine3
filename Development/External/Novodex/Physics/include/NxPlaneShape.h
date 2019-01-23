#ifndef NX_COLLISION_NXPLANESHAPE
#define NX_COLLISION_NXPLANESHAPE
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
#include "NxShape.h"

class NxPlane;
class NxPlaneShapeDesc;

/**
\brief A plane collision detection primitive.

By default it is configured to be the y == 0 plane. You can then set a normal and a d to specify
an arbitrary plane. d is the distance of the plane from the origin along the normal, assuming
the normal is normalized. Thus the plane equation is: 
normal.x * X + normal.y * Y + normal.z * Z  = d

Note: the plane does not represent an infinitely thin object, but rather a completely solid negative
half space (all points p for which normal.dot(p) - d < 0 are inside the solid region.)


Each shape is owned by an actor that it is attached to.


<h3>Creation</h3>

An instance can be created by calling the createShape() method of the NxActor object
that should own it, with a NxPlaneShapeDesc object as the parameter, or by adding the 
shape descriptor into the NxActorDesc class before creating the actor.

The shape is deleted by calling NxActor::releaseShape() on the owning actor.

Example:

\include NxPlaneShape_Create.cpp

<h3>Visualizations</h3>
\li NX_VISUALIZE_COLLISION_AABBS
\li NX_VISUALIZE_COLLISION_SHAPES
\li NX_VISUALIZE_COLLISION_AXES

<b>Platform:</b>
\li PC SW: Yes
\li PPU  : Yes (Hardware rigid body only)
\li PS3  : Yes
\li XB360: Yes


@see NxActor.createShape() NxPlaneShapeDesc NxShape NxPlane
*/

class NxPlaneShape: public NxShape
	{
	public:

	/**
	\brief sets the plane equation.

	<b>Sleeping:</b> Does <b>NOT</b> wake any actors associated with the shape.

	\param[in] normal Normal for the plane, in the global frame. <b>Range:</b> direction vector
	\param[in] d 'd' coefficient of the plane equation. <b>Range:</b> (-inf,inf)

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes (Hardware rigid body only)
	\li PS3  : Yes
	\li XB360: Yes

	@see getPlane() NxPlane
	*/
	virtual void setPlane(const NxVec3 & normal, NxReal d) = 0;

	/*
	\brief Saves the state of the shape object to a descriptor.

	\param[out] desc Descriptor to save this objects state to.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes (Hardware rigid body only)
	\li PS3  : Yes
	\li XB360: Yes

	@see NxPlaneShapeDesc
	*/
	virtual	void	saveToDesc(NxPlaneShapeDesc& desc)	const = 0;

	/**
	\brief retrieves the plane

	\return The description of this plane. See #NxPlane.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes (Hardware rigid body only)
	\li PS3  : Yes
	\li XB360: Yes

	@see setPlane NxPlane
	*/
	virtual NxPlane	getPlane() const = 0;

	};

/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
