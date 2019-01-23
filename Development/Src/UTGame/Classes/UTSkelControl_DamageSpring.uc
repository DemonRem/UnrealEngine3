/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_DamageSpring extends UTSkelControl_Damage
	native(Animation);

/** The Maximum size of the angle this spring can open to in Degrees */
var(Spring) rotator MaxAngle;

/** The Minmum size of the angle this spring can open to in Degrees */
var(Spring) rotator MinAngle;

/** How fast does it return to normal */
var(Spring) float Falloff;

/** How stiff is the spring */
var(Spring) float SpringStiffness;

var(Spring) float AVModifier;

/** The current angle of the hinge */
var transient rotator CurrentAngle;

/** % of movement decided randomly */
var float RandomPortion;

/** force that pushes the part up when the part is broken off to clear the vehicle. */
var() float BreakSpeed;

// to add momentum from breaking due to a damage hit
var vector LastHitMomentum;
var float LastHitTime;
var float MomentumPortion;
cpptext
{
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
	virtual int CalcAxis(INT &InAngle, FLOAT CurVelocity, FLOAT MinAngle, FLOAT MaxAngle);
	virtual UBOOL InitializeControl(USkeletalMeshComponent* SkelComp);
}


simulated event BreakAPart(vector PartLocation)
{
	local UTBrokenCarPart Part;
	local vector CompositeVector;
	local float PortionHold; // holds portion if no momentum
	Part = OwnerVehicle.Spawn(class'UTBrokenCarPart',,,PartLocation, OwnerVehicle.Rotation);

	if (Part != none && OwnerVehicle != none)
	{
		Part.Mesh.SetStaticMesh(BreakMesh);
		if(OwnerVehicle.WorldInfo.TimeSeconds - LastHitTime < 2.0)	
		{
			CompositeVector = normal(LastHitMomentum)*MomentumPortion;
			PortionHold = 0.0f;
		}
		else
		{
			PortionHold = MomentumPortion;
			MomentumPortion = 0.0;
		}
	
		if ( VSize(OwnerVehicle.Velocity) > 100 )
		{
			CompositeVector += (normal(Vector(CurrentAngle))*(1-RandomPortion) + VRand()*RandomPortion)*(1-MomentumPortion);
		}
		else
		{
			Part.TossZ *= 0.5;
			Part.Speed *= 0.25;
			CompositeVector += ( normal( DefaultBreakDir >> OwnerVehicle.Rotation )*(1-RandomPortion) + VRand()*RandomPortion )*(1-MomentumPortion);
		}
		Part.Init(CompositeVector);
		if(PortionHold != 0.0f)
		{
			MomentumPortion = PortionHold;
		}
		Part.SetDrawScale(OwnerVehicle.DrawScale);
		Part.SetDrawScale3D(OwnerVehicle.DrawScale3d);
		Part.Mesh.AddImpulse(Vect(0,0,1)*BreakSpeed);
	}		
	bIsBroken = true;
	bIsBreaking=false;
}	


defaultproperties
{
	SpringStiffness=-0.035
	bApplyRotation=true
	BoneRotationSpace=BCS_ActorSpace
	Falloff=0.975
	AVModifier=1.0
	bControlStrFollowsHealth=true
	ActivationThreshold=1.0
	RandomPortion=0.2f
	MomentumPortion=0.75f
	BreakSpeed = 50.0f
}
