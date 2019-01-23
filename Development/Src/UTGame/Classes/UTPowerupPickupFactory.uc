/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTPowerupPickupFactory extends UTPickupFactory
	abstract
	native;

/** extra spinning component (rotated in C++ when visible) */
var PrimitiveComponent Spinner;

/** extra spinning particles (rotated in C++ when visible) */
var UTParticleSystemComponent ParticleEffects;


cpptext
{
	virtual void TickSpecial(FLOAT DeltaSeconds);
}

/** adds weapon overlay material this powerup uses (if any) to the GRI in the correct spot
 *  @see UTPawn.WeaponOverlayFlags, UTWeapon::SetWeaponOverlayFlags(
 */
simulated function AddWeaponOverlay(UTGameReplicationInfo GRI);

simulated function SetPickupVisible()
{
	if (ParticleEffects != None)
	{
		ParticleEffects.ActivateSystem();
	}

	Super.SetPickupVisible();
}

simulated function SetPickupHidden()
{
	if (ParticleEffects != None)
	{
		ParticleEffects.DeactivateSystem();
	}

	super.SetPickupHidden();
}

defaultproperties
{
	// setting bMovable=FALSE will break pickups and powerups that are on movers.
	// I guess once we get the LightEnvironment brightness issues worked out and if this is needed maybe turn this on?
	// will need to look at all maps and change the defaults for the pickups that move tho.
	bMovable=TRUE
    bStatic=FALSE

	bRotatingPickup=true
	YawRotationRate=32768

	bIsSuperItem=true
	bDoVisibilityFadeIn=FALSE

	// we have lots of powerups so we should be able to keep this content
	Begin Object Name=BaseMeshComp
		StaticMesh=StaticMesh'Pickups.Base_Powerup.Mesh.S_Pickups_Base_Powerup01'
		Translation=(X=0.0,Y=0.0,Z=-44.0)
	End Object


 	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent1
		StaticMesh=StaticMesh'Pickups.Base_Powerup.Mesh.S_Pickups_Base_Powerup01_Disc'
		CastShadow=FALSE
		bCastDynamicShadow=FALSE
		bAcceptsLights=TRUE
		bForceDirectLightMap=TRUE
		LightingChannels=(BSP=TRUE,Dynamic=FALSE,Static=TRUE,CompositeDynamic=FALSE)
		LightEnvironment=PickupLightEnvironment

		Translation=(X=0.0,Y=0.0,Z=-40.0)
		CollideActors=false
		CullDistance=7000
		bUseAsOccluder=FALSE
	End Object
	Spinner=StaticMeshComponent1
 	Components.Add(StaticMeshComponent1)
}
