/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPickupFactory_HealthVial extends UTHealthPickupFactory;

var() float SoundPitch;

simulated function ModifyHearSoundComponent(AudioComponent AC)
{
	if (AC.SoundCue == PickupSound)
	{
		AC.bUseOwnerLocation = true;
		AC.Location = Location;
		AC.SetFloatParameter('SoundPitch', SoundPitch);
	}
}

/**
 * Give the benefit of this pickup to the recipient
 */
function SpawnCopyFor( Pawn Recipient )
{
	// Give health to recipient
	Recipient.Health += HealAmount(Recipient);

	PlaySound( PickupSound );

	if ( PlayerController(Recipient.Controller) != None )
	{
		PlayerController(Recipient.Controller).ReceiveLocalizedMessage(MessageClass,,,,class);
	}
}

auto state Pickup
{
	/* DetourWeight()
	value of this path to take a quick detour (usually 0, used when on route to distant objective, but want to grab inventory for example)
	*/
	function float DetourWeight(Pawn Other,float PathWeight)
	{
		if ( PathWeight > 500 )
			return 0;

		return Super.DetourWeight(Other, PathWeight);
	}
}

defaultproperties
{
	SoundPitch=1.0

	bSuperHeal=true
	bIsSuperItem=false
	RespawnTime=30.000000
	MaxDesireability=0.3
	HealingAmount=5
	PickupSound=SoundCue'A_Pickups.Health.Cue.A_Pickups_Health_Small_Cue_Modulated'

	bRotatingPickup=true
	YawRotationRate=32000

	bFloatingPickup=true
	bRandomStart=true
	BobSpeed=4.0
	BobOffset=5.0

	Begin Object Name=HealthPickUpMesh
		StaticMesh=StaticMesh'Pickups.Health_Small.Mesh.S_Pickups_Health_Small'
		Scale3D=(X=1.0,Y=1.0,Z=1.0)
	End Object


	Begin Object NAME=CollisionCylinder
		CollisionRadius=+00030.000000
		CollisionHeight=+00020.000000
		CollideActors=true
	End Object
}
