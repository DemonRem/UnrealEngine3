//=============================================================================
// ForcedDirVolume
// used to force UTVehicles [of a certain class if wanted] in a certain direction
//
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================

class ForcedDirVolume extends Volume
	placeable native;

var() bool bAllowBackwards; // allows negative forces, thus trapping vehicles on a line instead of in a direction
var() class<UTVehicle> TypeToForce;
var() const ArrowComponent Arrow;
var() bool bDenyExit; // if the vehicle is being effected by a force volume, the player cannot exit the vehicle.
var() bool bBlockPawns;
var vector ArrowDirection;

cpptext
{
	virtual void PostEditChange( UProperty* PropertyThatChanged );
	UBOOL IgnoreBlockingBy( const AActor *Other ) const;
}
simulated event Touch( Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal )
{
	Super.Touch(Other, OtherComp, HitLocation, HitNormal);
	Force(Other);
}

simulated function Force(Actor Other)
{
	local UTVehicle	 OtherV;
	if( ClassIsChildOf(Other.class,TypeToForce))
	{
		OtherV = UTVehicle(Other);
		OtherV.bForceDirectionAllowedNegative = bAllowBackwards;
		OtherV.ForceMovementDirection = ArrowDirection;
		OtherV.bAllowedExit = !bDenyExit;
	}
}

event untouch(Actor Other)
{
	local bool bInAnotherVolume;
	local ForcedDirVolume anotherForce;
	bInAnotherVolume=false;
	if( ClassIsChildOf(Other.class,TypeToForce))
	{
		foreach Other.TouchingActors(class'ForcedDirVolume', anotherForce)
		{
			if(anotherForce != none)
			{
				bInAnotherVolume = true;
				break;
			}
		}
		if(!bInAnotherVolume)
		{
			UTVehicle(Other).ForceMovementDirection=Vect(0,0,0);
		}
		else
		{
			anotherForce.Force(Other);
		}
	}
}

defaultproperties
{
	bAllowBackwards = false;
	bDenyExit = false;
	Begin Object Class=ArrowComponent Name=AC
		ArrowColor=(R=150,G=100,B=150)
		ArrowSize=5.0
		AbsoluteRotation=true
	End Object
	Components.Add(AC)
	Arrow=AC

	TypeToForce=UTVehicle

	Begin Object Name=BrushComponent0
		CollideActors=true
		BlockActors=true
		BlockZeroExtent=false
		BlockNonZeroExtent=true
		BlockRigidBody=false
	End Object

	bWorldGeometry=false
    bCollideActors=True
    bBlockActors=True
}
