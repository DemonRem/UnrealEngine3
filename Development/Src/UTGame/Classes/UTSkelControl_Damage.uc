/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_Damage extends SkelControlSingleBone
	native(Animation);

/** Is this control initialized */
var bool bInitialized;

/** Quick link to the owning UTVehicle */
var UTVehicle OwnerVehicle;

/** Which morph Target to use for health.  If none, use the main vehicle health */
var float HealthPerc;

/** How much damage the control can take */
var(Damage) int DamageMax;
/** If the health target is above this threshold, this control will be inactive */
var(Damage) float ActivationThreshold;

/** Once activated, does we generate the control strength as a product of the health remaining, or is it always full */
var(Damage) bool bControlStrFollowsHealth;

/** The Static Mesh component to display when it breaks off */
var(Damage) StaticMesh	BreakMesh;

/** The threshold at which the spring will begin looking to break */
var(Damage) float BreakThreshold;

/** This is the amount of time to go from breaking to broken */
var(Damage) float BreakTime;

/** When breaking off, use this to build the vector */
var(Damage) vector DefaultBreakDir;

/** Is this control broken */
var transient bool bIsBroken;

/** This is set to true when Break() is called.  It signals the control is breaking but not yet broken */
var transient bool bIsBreaking;

/** This holds the name of the bone that was broken */
var transient name BrokenBone;

/** This holds the real-time at which this should break */
var transient float BreakTimer;

/** Precahced MaxDamage for a vehicle */
var transient float OwnerVehicleMaxHealth;


cpptext
{
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);
	virtual FLOAT GetBoneScale(INT BoneIndex, USkeletalMeshComponent* SkelComp);
	virtual UBOOL InitializeControl(USkeletalMeshComponent* SkelComp);
}

/**
 * This event is triggered when the spring has decided to break.
 *
 * Network - Called everywhere except on a dedicated server.
 */

simulated event BreakAPart(vector PartLocation)
{
	local UTBrokenCarPart Part;

	Part = OwnerVehicle.Spawn(class'UTBrokenCarPart',,,PartLocation, OwnerVehicle.Rotation);

	if (Part != none && OwnerVehicle != none)
	{
		Part.Mesh.SetStaticMesh(BreakMesh);

		if ( VSize(OwnerVehicle.Velocity) > 100 )
		{
			Part.Init( normal(OwnerVehicle.Velocity * -1) );
		}
		else
		{
			Part.TossZ *= 0.5;
			Part.Speed *= 0.25;
			Part.Init( normal( DefaultBreakDir >> OwnerVehicle.Rotation ) );
		}
		Part.SetDrawScale(OwnerVehicle.DrawScale);
		Part.SetDrawScale3D(OwnerVehicle.DrawScale3d);
	}

	bIsBreaking = false;
	bIsBroken = true;
}


defaultproperties
{
	bControlStrFollowsHealth=false
	BreakThreshold=1.1
	BreakTime=1.0
	DefaultBreakDir=(X=1)
	HealthPerc = 1.f
	DamageMax = 25
	bIgnoreWhenNotRendered=true
}
