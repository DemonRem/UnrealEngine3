#ifndef NX_PHYSICS_NXFORCEFIELD
#define NX_PHYSICS_NXFORCEFIELD
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
#include "NxForceFieldDesc.h"
#include "NxEffector.h"

class NxEffector;
class NxForceFieldShape;
class NxForceFieldShapeDesc;
/**
 \brief A force field effector.

 Instances of this object automate the application of forces onto rigid bodies, fluid, soft bodies and cloth.

<b>Platform:</b>
\li PC SW: Yes
\li PPU  : Yes
\li PS3  : Yes
\li XB360: Yes

 @see NxForceFieldDesc, NxScene::createForceField()
*/
class NxForceField
	{
	public:
	/**
	\brief Writes all of the effector's attributes to the description, as well
	as setting the actor connection point.

	\param[out] desc The descriptor used to retrieve the state of the effector.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	virtual void saveToDesc(NxForceFieldDesc &desc) = 0;

	/**
	\brief Retrieves the force field's transform.  
	
	This transform is either from world space or from actor space, depending on whether the actor pointer is set.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see setPose() getActor() NxForceFieldDesc::pose
	*/
	virtual NxMat34  getPose() const = 0;

	/**
	\brief Sets the force field's transform.  
	
	This transform is either from world space or from actor space, depending on whether the actor pointer is set.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see getPose() getActor() NxForceFieldDesc::pose
	*/
	virtual void setPose(const NxMat34  & pose) = 0;

	/**
	\brief Retrieves the actor pointer that this force field is attached to.  
	
	Unattached force fields return NULL.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see setActor() NxForceFieldDesc::actor
	*/
	virtual NxActor * getActor() const = 0; 

	/**
	\brief Sets the actor pointer that this force field is attached to.  
	
	Pass NULL for unattached force fields.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see getActor() NxForceFieldDesc::actor
	*/
	virtual void setActor(NxActor * actor) = 0;

	/**
	\brief Retrieves the value set with #setGroup().

	NxCollisionGroup is an integer between 0 and 31.

	\return The collision group this shape belongs to.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see setGroup() NxCollisionGroup
	*/
	virtual NxCollisionGroup  getGroup() const = 0; 

	/**
	\brief Sets which collision group this shape is part of.
	
	Default group is 0. Maximum possible group is 31.
	Collision groups are sets of shapes which may or may not be set
	to collision detect with each other; this can be set using NxScene::setGroupCollisionFlag()

	\param[in] collisionGroup The collision group for this shape.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see getGroup() NxCollisionGroup
	*/
	virtual void setGroup(NxCollisionGroup collisionGroup) = 0; 

	/**
	\brief Gets 128-bit mask used for collision filtering. See comments for ::NxGroupsMask

	\return The group mask for the shape.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see setGroupsMask()
	*/
	virtual NxGroupsMask  getGroupsMask() const = 0; 

	/**
	\brief Sets 128-bit mask used for collision filtering. See comments for ::NxGroupsMask

	\param[in] mask The group mask to set for the shape.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see getGroupsMask()
	*/
	virtual void setGroupsMask(NxGroupsMask  mask) = 0; 

	/**
	\brief Sets a name string for the object that can be retrieved with getName().
	
	This is for debugging and is not used by the SDK. The string is not copied by the SDK, only the pointer is stored.
	
	\param[in] name String to set the objects name to.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see getName()
	*/
	virtual void  setName (const char* name)= 0;

	/**
	\brief Retrieves the name string set with setName().

	\return The name string for this object.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see setName()
	*/
	virtual const char*  getName () const = 0;

	virtual NxForceFieldCoordinates getCoordinates() const = 0;			//!< Gets the Coordinate space of the field.
	virtual void setCoordinates(NxForceFieldCoordinates coordinates) = 0;//!< Sets the Coordinate space of the field.
	virtual NxVec3 getConstant () const= 0;								//!< Gets the constant part of force field function
	virtual void setConstant (const NxVec3 &) = 0;						//!< Sets the constant part of force field function
	virtual NxMat33 getPositionMultiplier () const = 0;					//!< Gets the coefficient of force field function position term
	virtual void setPositionMultiplier (const NxMat33 & ) = 0;			//!< Sets the coefficient of force field function position term
	virtual NxMat33 getVelocityMultiplier () const = 0;					//!< Gets the coefficient of force field function velocity term
	virtual void setVelocityMultiplier (const NxMat33 & ) = 0;			//!< Sets the coefficient of force field function velocity term
	virtual NxVec3 getPositionTarget () const = 0;						//!< Gets the force field position target
	virtual void setPositionTarget (const NxVec3 & ) = 0;				//!< Sets the force field position target
	virtual NxVec3 getVelocityTarget () const = 0;						//!< Gets the force field velocity target
	virtual void setVelocityTarget (const NxVec3 & ) = 0;				//!< Sets the force field velocity target
	virtual NxVec3 getFalloffLinear() const = 0;						//!< Sets the linear falloff term
	virtual void setFalloffLinear(const NxVec3 &) = 0;					//!< Sets the linear falloff term
	virtual NxVec3 getFalloffQuadratic() const = 0;						//!< Sets the quadratic falloff term
	virtual void setFalloffQuadratic(const NxVec3 &) = 0;				//!< Sets the quadratic falloff term
	virtual NxVec3 getNoise () const = 0;								//!< Gets the force field noise
	virtual void setNoise (const NxVec3 & ) = 0;						//!< Sets the force field noise
	virtual NxReal getTorusRadius () const = 0;							//!< Gets the toroidal radius
	virtual void setTorusRadius(NxReal) = 0;							//!< Sets the toroidal radius

	virtual	NxReal	getFluidScale()		const	= 0;					//!< Gets the fluid scale factor
	virtual	void	setFluidScale(NxReal)		= 0;					//!< Sets the fluid scale factor
	virtual	NxReal	getClothScale()		const	= 0;					//!< Gets the cloth scale factor
	virtual	void	setClothScale(NxReal)		= 0;					//!< Sets the cloth scale factor
	virtual	NxReal	getSoftBodyScale()	const	= 0;					//!< Gets the soft body scale factor
	virtual	void	setSoftBodyScale(NxReal)	= 0;					//!< Sets the soft body scale factor
	virtual	NxReal	getRigidBodyScale()	const	= 0;					//!< Gets the rigid body scale factor
	virtual	void	setRigidBodyScale(NxReal)	= 0;					//!< Sets the rigid body scale factor

	virtual NxU32	getFlags () const			= 0;					//!< Gets the flags @see NxForceFieldFlags
	virtual void	setFlags(NxU32)				= 0;					//!< Sets the flags @see NxForceFieldFlags
	/**
	\brief Creates a NxForceFieldShape. 
	
	The volume of activity of the force field is defined by the union of all of the force field's shapes' volumes created
	here without the NX_FFS_EXCLUDE flag set, minus the union of all of the force field's shapes with the NX_FFS_EXCLUDE flag set.

	The shapes are owned by the force field and released if the force field is released.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see releaseShape() NxForceFieldShapeDesc
	*/
	virtual NxForceFieldShape * createShape(const NxForceFieldShapeDesc &) = 0;
	
	/**
	\brief Releases the passed force field shape. 

	The passed force field shape must previously have been created with this force field's createShape() call.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see createShape() NxBoxForceFieldShapeDesc
	*/	
	virtual void releaseShape(const NxForceFieldShape &) = 0;

	//! Returns the number of shapes in the force field. 
	virtual NxU32  getNbShapes () const = 0; 

	//! Restarts the shape iterator so that the next call to getNextShape() returns the first shape in the force field.  
	virtual void  resetShapesIterator ()= 0; 

	//! Retrieves the next shape when iterating.  
	virtual NxForceFieldShape *  getNextShape ()= 0; 

	/**
	\brief Samples the force field. Incoming points & velocities must be in world space. The velocities pointer is optional and can be null. 

	\param[in] numPoints Size of the buffers
	\param[in] points Buffer of sample points
	\param[in] velocities Buffer of velocities at the sample points
	\param[out] outForces Buffer for the returned forces

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	virtual	void				samplePoints(NxU32 numPoints, const NxVec3* points, const NxVec3* velocities, NxVec3* outForces)	const = 0;
	};


/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
