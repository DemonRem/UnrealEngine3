#ifndef NX_COLLISION_NXUSERCONTACTREPORT
#define NX_COLLISION_NXUSERCONTACTREPORT
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
#include "NxContactStreamIterator.h"

class NxActor;
class NxShape;

#if NX_USE_FLUID_API
class NxFluid;
#endif

/**
\brief Contact pair flags.

@see NxUserContactReport.onContactNotify()
*/
enum NxContactPairFlag
	{
	NX_IGNORE_PAIR					= (1<<0),	//!< Disable contact generation for this pair

	NX_NOTIFY_ON_START_TOUCH		= (1<<1),	//!< Pair callback will be called when the pair starts to be in contact
	NX_NOTIFY_ON_END_TOUCH			= (1<<2),	//!< Pair callback will be called when the pair stops to be in contact
	NX_NOTIFY_ON_TOUCH				= (1<<3),	//!< Pair callback will keep getting called while the pair is in contact
	NX_NOTIFY_ON_IMPACT				= (1<<4),	//!< [Not yet implemented] pair callback will be called when it may be appropriate for the pair to play an impact sound
	NX_NOTIFY_ON_ROLL				= (1<<5),	//!< [Not yet implemented] pair callback will be called when the pair is in contact and rolling.
	NX_NOTIFY_ON_SLIDE				= (1<<6),	//!< [Not yet implemented] pair callback will be called when the pair is in contact and sliding (and not rolling).
	NX_NOTIFY_FORCES				= (1<<7),	//!< The friction force and normal force will be available in the contact report

	NX_NOTIFY_CONTACT_MODIFICATION	= (1<<16),	//!< Generate a callback for all associated contact constraints, making it possible to edit the constraint. This flag is not included in NX_NOTIFY_ALL for performance reasons. \see NxUserContactModify

	NX_NOTIFY_ALL					= (NX_NOTIFY_ON_START_TOUCH|NX_NOTIFY_ON_END_TOUCH|NX_NOTIFY_ON_TOUCH|NX_NOTIFY_ON_IMPACT|NX_NOTIFY_ON_ROLL|NX_NOTIFY_ON_SLIDE|NX_NOTIFY_FORCES)
	};

/**
\brief An instance of this class is passed to NxUserContactReport::onContactNotify().
It contains a contact stream which may be parsed using the class NxContactStreamIterator.

@see NxUserContactReport.onContactNotify()
*/
class NxContactPair
	{
	public:
	NX_INLINE	NxContactPair() : stream(NULL)	{}

	/**
	\brief the two actors that make up the pair.

	@see NxActor
	*/
	NxActor*				actors[2];

	/**
	\brief use this to create stream iter. See #NxContactStreamIterator.

	@see NxConstContactStream
	*/
	NxConstContactStream	stream;

	/**
	\brief the total contact normal force that was applied for this pair, to maintain nonpenetration constraints.
	*/
	NxVec3					sumNormalForce;

	/**
	\brief the total tangential force that was applied for this pair.
	*/
	NxVec3					sumFrictionForce;
	};

/**
\brief The user needs to implement this interface class in order to be notified when
certain contact events occur.

Once you pass an instance of this class to #NxScene::setUserContactReport(), 
its  #onContactNotify() method will be called for each pair of actors which comes into contact, 
for which this behavior was enabled.

You request which events are reported using NxScene::setActorPairFlags(), 
#NxScene::setShapePairFlags(), or #NxScene::getActorGroupPairFlags()

Please note: Kinematic actors will not generate contact reports when in contact with other kinematic actors.

 <b>Threading:</b> It is not necessary to make this class thread safe as it will only be called in the context of the
 user thread.

<h3>Example</h3>

\include NxUserContactReport_Example.cpp

<h3>Visulizations:</h3>
\li #NX_VISUALIZE_CONTACT_POINT
\li #NX_VISUALIZE_CONTACT_NORMAL
\li #NX_VISUALIZE_CONTACT_ERROR
\li #NX_VISUALIZE_CONTACT_FORCE

@see NxScene.setUserContactReport() NxScene.getUserNotify()
*/
class NxUserContactReport
	{
	public:
 	/**
	Called for a pair in contact. The events parameter is a combination of:

	<ul>
	<li>NX_NOTIFY_ON_START_TOUCH,</li>
	<li>NX_NOTIFY_ON_END_TOUCH,</li>
	<li>NX_NOTIFY_ON_TOUCH,</li>
	<li>NX_NOTIFY_ON_IMPACT,	//unimplemented!</li>
	<li>NX_NOTIFY_ON_ROLL,		//unimplemented!</li>
	<li>NX_NOTIFY_ON_SLIDE,		//unimplemented!</li>
	</ul>

	See the documentation of #NxContactPairFlag for an explanation of each. You request which events 
	are reported using #NxScene::setActorPairFlags() or 
	#NxScene::setActorGroupPairFlags(). Do not keep a reference to the passed object, as it will
	be invalid after this function returns.

	\note SDK state should not be modified from within onContactNotify(). In particular objects should not
	be created or destroyed. If state modification is needed then the changes should be stored to a buffer
	and performed after the simulation step.

	\param[in] pair The contact pair we are being notified of. See #NxContactPair.
	\param[in] events Flags raised due to the contact. See #NxContactPairFlag.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Partial (Only NX_NOTIFY_ON_START_TOUCH and NX_NOTIFY_ON_END_TOUCH)
	\li PS3  : Yes
	\li XB360: Yes

	@see NxContactPair NxContactPairFlag
	*/
	virtual void  onContactNotify(NxContactPair& pair, NxU32 events) = 0;

	protected:
	virtual ~NxUserContactReport(){};
	};

/**
\brief The user needs to implement this interface class in order to be notified when trigger events
occur. 

Once you pass an instance of this class to #NxScene::setUserTriggerReport(), shapes
which have been marked as triggers using NxShape::setFlag(NX_TRIGGER_ENABLE,true) will call the
#onTrigger() method when their trigger status changes.

<b>Threading:</b> It is not necessary to make this class thread safe as it will only be called in the context of the
user thread.

Example:

\include NxUserTriggerReport_Usage.cpp

<h3>Visualizations</h3>
\li NX_VISUALIZE_COLLISION_SHAPES

@see NxScene.setUserTriggerReport() NxScene.getUserTriggerReport() NxShapeFlag NxShape.setFlag()
*/
class NxUserTriggerReport
	{
	public:
	/**
	\brief Called when a trigger shape reports a trigger event.

	\note SDK state should not be modified from within onTrigger(). In particular objects should not
	be created or destroyed. If state modification is needed then the changes should be stored to a buffer
	and performed after the simulation step.

	\param[in] triggerShape is the shape that has been marked as a trigger.
	\param[in] otherShape is the shape causing the trigger event.
	\param[in] status is the type of trigger event.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxTriggerFlag
	*/
	virtual void onTrigger(NxShape& triggerShape, NxShape& otherShape, NxTriggerFlag status) = 0;

	protected:
	virtual ~NxUserTriggerReport(){};
	};

/**
\brief An interface class that the user can implement in order to modify contact constraints.

<b>Threading:</b> It is <b>necessary</b> to make this class thread safe as it will be called in the context of the
simulation thread. It might also be necessary to make it reentrant, since some calls can be made by multi-threaded
parts of the physics engine.

You can enable the use of this contact modification callback in two ways:
1. Raise the flag NX_AF_CONTACT_MODIFICATION on a per-actor basis.
or
2. Set the flag NX_NOTIFY_CONTACT_MODIFICATION on a per-actor-pair basis.

Please note: 
+ It is possible to raise the contact modification flags at any time. But the calls will not wake the actors up.
+ It is not possible to turn off the performance degradation by simply removing the callback from the scene, all flags needs to be removed as well.
+ The contacts will only be reported as long as the actors are awake. There will be no callbacks while the actors are sleeping.

@see NxScene.setUserContactModify() NxScene.getUserContactModify()
*/
class NxUserContactModify
{
public:
	//enum to identify what changes have been made
	/**
	\brief This enum is used for marking changes made to contact constraints in the NxUserContactModify callback. OR the values together when making multiple changes on the same contact.
	*/
	enum NxContactConstraintChange {
		NX_CCC_NONE					= 0,		//!< No changes made

		NX_CCC_MINIMPULSE			= (1<<0),	//!< Min impulse value changed
		NX_CCC_MAXIMPULSE			= (1<<1),	//!< Max impulse value changed
		NX_CCC_ERROR				= (1<<2),	//!< Error vector changed
		NX_CCC_TARGET				= (1<<3),	//!< Target vector changed

		NX_CCC_LOCALPOSITION0		= (1<<4),	//!< Local attachment position in shape 0 changed
		NX_CCC_LOCALPOSITION1		= (1<<5),	//!< Local attachment position in shape 1 changed
		NX_CCC_LOCALORIENTATION0	= (1<<6),	//!< Local orientation (normal, friction direction) in shape 0 changed
		NX_CCC_LOCALORIENTATION1	= (1<<7),	//!< Local orientation (normal, friction direction) in shape 1 changed

		NX_CCC_STATICFRICTION0		= (1<<8),	//!< Static friction parameter 0 changed. (Note: 0 does not have anything to do with shape 0/1)
		NX_CCC_STATICFRICTION1		= (1<<9),	//!< Static friction parameter 1 changed. (Note: 1 does not have anything to do with shape 0/1)
		NX_CCC_DYNAMICFRICTION0		= (1<<10),	//!< Dynamic friction parameter 0 changed. (Note: 0 does not have anything to do with shape 0/1)
		NX_CCC_DYNAMICFRICTION1		= (1<<11),	//!< Dynamic friction parameter 1 changed. (Note: 1 does not have anything to do with shape 0/1)
		NX_CCC_RESTITUTION			= (1<<12),	//!< Restitution value changed.

		NX_CCC_FORCE32				= (1<<31)	//!< Not a valid flag value, used by the enum to force the size to 32 bits.
	};

	//The data that can be changed by the callback
	struct NxContactCallbackData {
		NxReal minImpulse;			//!< Minimum impulse value that the solver can apply. Normally this should be 0, negative amount gives sticky contacts.
		NxReal maxImpulse;			//!< Maximum impulse value that the solver can apply. Normally this is FLT_MAX. If you set this to 0 (and the min impulse value is 0) then you will void contact effects of the constraint.
		NxVec3 error;				//!< Error vector. This is the current error that the solver should try to relax.
		NxVec3 target;				//!< Target velocity. This is the relative target velocity of the two bodies.

		NxVec3 localpos0;			//!< Constraint attachment point in shape 0 (shape local space)
		NxVec3 localpos1;			//!< Constraint attachment point in shape 1 (shape local space)
		NxQuat localorientation0;	//!< Constraint orientation quaternion in shape 0 (shape local space). The constraint axis (normal) is along the x-axis of the quaternion.
		NxQuat localorientation1;	//!< Constraint orientation quaternion in shape 1 (shape local space). The constraint axis (normal) is along the x-axis of the quaternion.

		NxReal staticFriction0;		//!< Static friction parameter 0. (Note: 0 does not have anything to do with shape 0/1).
		NxReal staticFriction1;		//!< Static friction parameter 1. (Note: 1 does not have anything to do with shape 0/1).
		NxReal dynamicFriction0;	//!< Dynamic friction parameter 0. (Note: 0 does not have anything to do with shape 0/1).
		NxReal dynamicFriction1;	//!< Dynamic friction parameter 1. (Note: 1 does not have anything to do with shape 0/1).
		NxReal restitution;			//!< Restitution value.
	};

	/**
	\brief This is called when a contact constraint is generated. Modify the parameters in order to affect the generated contact constraint.
	This callback needs to be both thread safe and reentrant.

	\param changeFlags when making changes to the contact point, you must mark in this flag what changes have been made, see NxContactConstraintChange.
	\param shape0 one of the two shapes in contact
	\param shape1 the other shape
	\param featureIndex0 feature on the first shape, which is in contact with the other shape
	\param featureIndex1 feature on the second shape, which is in contact with the other shape
	\param data contact constraint properties, for the user to change. Changes in this also requires changes in the changeFlags parameter.

	\return true if the contact point should be kept, false if it should be discarded.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	virtual bool onContactConstraint(
		NxU32& changeFlags, 
		const NxShape* shape0, 
		const NxShape* shape1, 
		const NxU32 featureIndex0, 
		const NxU32 featureIndex1,
		NxContactCallbackData& data) = 0;

protected:
	virtual ~NxUserContactModify(){};
};


/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
