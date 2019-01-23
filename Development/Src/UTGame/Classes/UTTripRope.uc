/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTTripRope extends UTTripWire;

/** We use a delegate so that different types of creators can get the OnDestroyed event */
delegate OnDeployableUsedUp(actor ChildDeployable);

function RopeHit(UTPawn Victim, vector HitLocation, vector HitNormal)
{
	Victim.ForceRagdoll();
	Victim.AddVelocity(HitNormal*-1*VSize(Victim.Velocity),HitLocation,none);
	Destroy(); // kill the rope so you can't get stuck in it.
}
defaultproperties
{
	MaxLength=750
	bCanBounce=false

	// content move me
	Begin Object Name=LaserVolume
		StaticMesh=StaticMesh'Pickups.Deployables.Mesh.S_RopeMesh'
		CollideActors=true
		BlockActors=false
		CastShadow=false;
		bOnlyOwnerSee = true
	End Object

	LengthScale=1
	ActivatedSound=SoundCue'A_Pickups.Deployables.Sounds.TripRopeDeployedCue'
	bEMPDisables = false
}

