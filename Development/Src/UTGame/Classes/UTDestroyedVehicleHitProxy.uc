/**
 * Actor placed on top of vehicles to serve as its collision for purposes of running over and crushing players,
 * since multi-body physics assets do not have accurate encroachment support
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDestroyedVehicleHitProxy extends Actor
	native(Vehicle);

cpptext
{
	virtual UBOOL IgnoreBlockingBy(const AActor* Other) const;
}

function PostBeginPlay()
{
	local UTVehicle OwnerVehicle;

	Super.PostBeginPlay();

	OwnerVehicle = UTVehicle(Owner);
	if (OwnerVehicle == None)
	{
		`warn("Invalid Owner" @ Owner);
		Destroy();
	}
	else
	{
		SetBase(OwnerVehicle);
		// if the vehicle has a special ragdoll cylinder, use it, otherwise make a copy of its normal cylinder to use
		if (OwnerVehicle.RagdollCylinder != None)
		{
			CollisionComponent = OwnerVehicle.RagdollCylinder;
		}
		else
		{
			CollisionComponent = new(Outer) class'CylinderComponent'(OwnerVehicle.CylinderComponent);
		}
		AttachComponent(CollisionComponent);
		// make sure collision is enabled on the component as well
		CollisionComponent.SetActorCollision(true, true);
		CollisionComponent.SetTraceBlocking(true, true);
		SetCollision(true, true);
	}
}

event bool EncroachingOn(Actor Other)
{
	return Owner.EncroachingOn(Other);
}

event EncroachedBy(Actor Other)
{
	Owner.EncroachedBy(Other);
}

event RanInto(Actor Other)
{
	Owner.RanInto(Other);
}

defaultproperties
{
	bGameRelevant=true
	//bHidden=true
	// set physics to PHYS_Interpolating so we are considered an encroacher
	Physics=PHYS_Interpolating
}

