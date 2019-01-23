#ifndef NX_COLLISION_NXWHEELSHAPEDESC
#define NX_COLLISION_NXWHEELSHAPEDESC
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/


#include "NxShapeDesc.h"
#include "NxSpringDesc.h"


/**
\brief Class used to describe tire properties for a #NxWheelShape

A tire force function takes a measure for tire slip as input, (this is computed differently for lateral and longitudal directions)
and gives a force (or in our implementation, an impulse) as output.

The curve is approximated by a two piece cubic Hermite spline.  The first section goes from (0,0) to (extremumSlip, extremumValue), at which 
point the curve's tangent is zero.

The second section goes from (extremumSlip, extremumValue) to (asymptoteSlip, asymptoteValue), at which point the curve's tangent is again zero.

\image html wheelGraph.png


<b>Platform:</b>
\li PC SW: Yes
\li PPU  : Yes (Software fallback for collision)
\li PS3  : Yes
\li XB360: Yes

See #NxWheelShape.
*/

/*
Force
^		extremum
|    _*_
|   ~   \     asymptote
|  /     \~__*______________
| /
|/
---------------------------> Slip
*/
class NxTireFunctionDesc
	{
	public:

	virtual ~NxTireFunctionDesc(){};
	
	/**
	\brief extremal point of curve.  Both values must be positive.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 1.0
	*/
	NxReal extremumSlip;

	/**
	\brief extremal point of curve.  Both values must be positive.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 0.02
	*/
	NxReal extremumValue;

	/**
	\brief point on curve at which for all x > minumumX, function equals minimumY.  Both values must be positive.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 2.0
	*/
	NxReal asymptoteSlip;
	
	/**
	\brief point on curve at which for all x > minumumX, function equals minimumY.  Both values must be positive.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 0.01
	*/
	NxReal asymptoteValue;


	/**
	\brief Scaling factor for tire force.
	
	This is an additional overall positive scaling that gets applied to the tire forces before passing 
	them to the solver.  Higher values make for better grip.  If you raise the *Values above, you may 
	need to lower this. A setting of zero will disable all friction in this direction.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 1000000.0 (quite stiff by default)
	*/
	NxReal stiffnessFactor;


	/**
	\brief Scaling factor for tire force.
	
	This is an additional overall positive scaling that gets applied to the tire forces before passing 
	them to the solver.  Higher values make for better grip.  If you raise the *Values above, you may 
	need to lower this. A setting of zero will disable all friction in this direction.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 1000000.0 (quite stiff by default)
	*/
	//NxReal stiffnessFactor;


	/**
	constructor sets to default.
	*/
	NX_INLINE					NxTireFunctionDesc();	
	/**
	(re)sets the structure to the default.	
	*/
	NX_INLINE virtual	void	setToDefault();
	/**
	returns true if the current settings are valid
	*/
	NX_INLINE virtual	bool	isValid() const;

	/**
	evaluates the Force(Slip) function
	*/
	NX_INLINE NxReal hermiteEval(NxReal t) const;
	};

enum NxWheelShapeFlags
	{
	/**
	\brief Determines whether the suspension axis or the ground contact normal is used for the suspension constraint.

	*/
	NX_WF_WHEEL_AXIS_CONTACT_NORMAL = 1 << 0,
	
	/**
	\brief If set, the laterial slip velocity is used as the input to the tire function, rather than the slip angle.

	*/
	NX_WF_INPUT_LAT_SLIPVELOCITY = 1 << 1,
	
	/**
	\brief If set, the longutudal slip velocity is used as the input to the tire function, rather than the slip ratio.  
	
	*/
	NX_WF_INPUT_LNG_SLIPVELOCITY = 1 << 2,

	/**
	\brief If set, does not factor out the suspension travel and wheel radius from the spring force computation.  This is the legacy behavior from the raycast capsule approach.
	*/
	NX_WF_UNSCALED_SPRING_BEHAVIOR = 1 << 3, 

	/**
	\brief If set, the axle speed is not computed by the simulation but is rather expected to be provided by the user every simulation step via NxWheelShape::setAxleSpeed().
	*/
	NX_WF_AXLE_SPEED_OVERRIDE = 1 << 4,
	};


/**
 \brief Descriptor for an #NxWheelShape.

<b>Platform:</b>
\li PC SW: Yes
\li PPU  : Yes (Software fallback for collision)
\li PS3  : Yes
\li XB360: Yes

 @see NxWheelShape NxActor.createActor()
*/
class NxWheelShapeDesc : public NxShapeDesc		//TODO: this nor other desc classes can be assigned with = operator, we need to define copy ctors. 
	{
	public:

	virtual ~NxWheelShapeDesc(){};

//geometrical constants:
	
	/**
	\brief distance from wheel axle to a point on the contact surface.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 1.0

	@see NxWheelShape.getRadius() NxWheelShape.setRadius()
	*/
	NxReal radius;
	
	/**
	\brief maximum extension distance of suspension along shape's -Y axis.
	
	The minimum extension is always 0.  

	<b>Range:</b> [0,inf)<br>
	<b>Default:</b> 1.0

	@see NxWheelShape.setSuspensionTravel() NxWheelShape.getSuspensionTravel()
	*/
	NxReal suspensionTravel;

//(In the old model the capsule height was the sum of the two members above.)
//^^^ this may be redundant together with suspension.targetValue, not sure yet.

//simulation constants:
	
	/**
	\brief data intended for car wheel suspension effects.  
	
	targetValue must be in [0, 1], which is the rest length of the suspension along the suspensionTravel.  
	0 is the default, which maps to the tip of the ray cast.

	<b>Range:</b> .spring [0,inf)<br>
	<b>Range:</b> .damper [0,inf)<br>
	<b>Range:</b> .targetValue [0,1]<br>
	<b>Default:</b> See #NxSpringDesc


	@see NxSpringDesc NxWheelShape.setSuspension() NxWheelShape.getSuspension()
	*/
	NxSpringDesc suspension;

	/**
	\brief cubic hermite spline coefficients describing the longitudal tire force curve.

	<b>Range:</b> See #NxTireFunctionDesc<br>
	<b>Default:</b> See #NxTireFunctionDesc

	@see NxWheelShape.getLongitudalTireForceFunction() NxWheelShape.setLongitudalTireForceFunction()
	*/
	NxTireFunctionDesc longitudalTireForceFunction;
	
	/**
	\brief cubic hermite spline coefficients describing the lateral tire force curve.

	<b>Range:</b> See #NxTireFunctionDesc<br>
	<b>Default:</b> See #NxTireFunctionDesc

	@see NxWheelShape.getLateralTireForceFunction() NxWheelShape.setLateralTireForceFunction()
	*/
	NxTireFunctionDesc lateralTireForceFunction;
	
	/**
	\brief inverse mass of the wheel.
	
	Determines the wheel velocity that wheel torques can achieve.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 1.0

	@see NxWheelShape.setInverseWheelMass() NxWheelShape.getInverseWheelMass()
	*/
	NxReal	inverseWheelMass;
	
	/**
	\brief flags from NxWheelShapeFlags

	<b>Default:</b> 0

	@see NxWheelShapeFlags NxWheelShape.getWheelFlags() NxWheelShape.setWheelFlags()
	*/
	NxU32 wheelFlags;

//dynamic inputs:
	
	/**
	\brief Sum engine torque on the wheel axle.
	
	Positive or negative depending on direction.

	<b>Range:</b> (-inf,inf)<br>
	<b>Default:</b> 0.0

	@see NxWheelShape.setMotorTorque() NxWheelShape.getMotorTorque() brakeTorque
	*/
	NxReal motorTorque;
	
	/**
	\brief The amount of torque applied for braking.
	
	Must be nonnegative.  Very large values should lock wheel but should be stable.

	<b>Range:</b> [0,inf)<br>
	<b>Default:</b> 0.0

	@see NxWheelShape.setBrakeTorque() NxWheelShape.getBrakeTorque() motorTorque
	*/
	NxReal brakeTorque;
	
	/**
	\brief steering angle, around shape Y axis.

	<b>Range:</b> (-PI,PI)<br>
	<b>Default:</b> 0.0

	@see NxWheelShape.setSteerAngle() NxWheelShape.getSteerAngle()
	*/
	NxReal steerAngle;




	/**
	constructor sets to default.
	*/
	NX_INLINE					NxWheelShapeDesc();	
	/**
	\brief (re)sets the structure to the default.
	 \param[in] fromCtor Avoid redundant work if called from constructor.
	*/
	NX_INLINE virtual	void	setToDefault(bool fromCtor = false);
	/**
	\brief returns true if the current settings are valid
	*/
	NX_INLINE virtual	bool	isValid() const;
	};


NX_INLINE NxTireFunctionDesc::NxTireFunctionDesc()
	{
	setToDefault();
	}

NX_INLINE void NxTireFunctionDesc::setToDefault()
	{
	extremumSlip = 1.0f;
	extremumValue = 0.02f;
	asymptoteSlip = 2.0f;
	asymptoteValue = 0.01f;	
	stiffnessFactor = 1000000.0f;	//quite stiff by default.
	}

NX_INLINE bool NxTireFunctionDesc::isValid() const
	{
	if(!(0.0f < extremumSlip))			return false;
	if(!(extremumSlip < asymptoteSlip))	return false;
	if(!(0.0f < extremumValue))			return false;
	if(!(0.0f < asymptoteValue))		return false;
	if(!(0.0f <= stiffnessFactor))		return false;

	return true;
	}

NX_INLINE NxReal NxTireFunctionDesc::hermiteEval(NxReal t) const
	{
	NxReal sign;
	if (t < 0.0f)
		{
		t = -t;	//function is mirrored around origin.
		sign = -1.0f;
		}
	else
		sign = 1.0f;

	//NxReal c2 = A3-A2;	//not needed.



	if (t < extremumSlip)	//first curve
		{
		//0 at start, with tangent = line to first control point.
		//(x,y) at end, with tangent = 0;
		sign *= extremumValue;
		t /= extremumSlip;
		NxReal A2 = t * t;
		NxReal A3 = A2 * t;
		NxReal c3 = -2*A3+3*A2;
		NxReal c1 = A3-2*A2+t;
		return (c1 + c3) * sign;	//c1 has coeff = tangent == maximum, c3 has coeff maximum.
		}
	if (t > asymptoteSlip)	//beyond minimum
		{
		return asymptoteValue * sign;
		}
	else	//second curve
		{
		//between two points (extremumSlip,Y), minimum(X,Y), both with tangent 0.

		//remap t so that maximimX --> 0, asymptoteSlip --> 1

		t /= (asymptoteSlip - extremumSlip);
		t -= extremumSlip;

		NxReal A2 = t * t;
		NxReal A3 = A2 * t;
		NxReal c3 = -2*A3+3*A2;
		NxReal c0 = 2*A3-3*A2+1;
		return (c0 * extremumValue + c3 * asymptoteValue) * sign;
		}
	}


NX_INLINE NxWheelShapeDesc::NxWheelShapeDesc() : NxShapeDesc(NX_SHAPE_WHEEL)	//constructor sets to default
	{
	setToDefault(true);
	}

	NX_INLINE void NxWheelShapeDesc::setToDefault(bool fromCtor)
	{
	NxShapeDesc::setToDefault();

	radius = 1.0f;				
	suspensionTravel = 1.0f;	
	//simulation constants:
	inverseWheelMass = 1.0f;
	wheelFlags = 0;
	//dynamic inputs:
	motorTorque = 0.0f;
	brakeTorque = 0.0f;
	steerAngle = 0.0f;
 	if (!fromCtor)
 		{
 		suspension.setToDefault();
 		longitudalTireForceFunction.setToDefault();
 		lateralTireForceFunction.setToDefault();
 		}
	}

NX_INLINE bool NxWheelShapeDesc::isValid() const
	{
	if(!NxMath::isFinite(radius))	return false;
	if(radius<=0.0f)				return false;
	if(!NxMath::isFinite(suspensionTravel))	return false;
	if(suspensionTravel<0.0f)				return false;
	if(!NxMath::isFinite(inverseWheelMass))	return false;
	if(inverseWheelMass<=0.0f)				return false;
	if(!NxMath::isFinite(motorTorque))	return false;
	if(!NxMath::isFinite(brakeTorque))	return false;
	if(brakeTorque<0.0f)				return false;
	if(!NxMath::isFinite(steerAngle))	return false;

	if (!suspension.isValid()) return false;
	if ((suspension.targetValue < 0.0f) || (suspension.targetValue > 1.0f)) return false;
	if (!longitudalTireForceFunction.isValid()) return false;
	if (!lateralTireForceFunction.isValid()) return false;


	return NxShapeDesc::isValid();
	}
#endif

//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
