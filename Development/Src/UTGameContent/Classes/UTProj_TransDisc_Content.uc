/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_TransDisc_Content extends UTProj_TransDisc;

defaultproperties
{
	ProjFlightTemplate=ParticleSystem'WP_Translocator.Particles.P_WP_Translocator_Trail'

	TLEffect(0)=ParticleSystem'WP_Translocator.Particles.P_WP_Translocator_Trail_Red'
	TLEffect(1)=ParticleSystem'WP_Translocator.Particles.P_WP_Translocator_Trail'

	BounceColors[0]=(R=255,G=0,B=0,A=255)
	BounceColors[1]=(R=0,G=0,B=255,A=255)

	BounceTemplate=ParticleSystem'WP_Translocator.Particles.P_WP_Translocator_BounceEffect'

	// Add the mesh
	Begin Object Name=ProjectileMesh
		StaticMesh=StaticMesh'Pickups.Translocator.Mesh.S_Translocator'
		scale=2
		CastShadow=true
		bAcceptsLights=true
		Translation=(Z=2)
	End Object

	Begin Object Class=UTParticleSystemComponent Name=BrokenPCS
		Template=ParticleSystem'WP_Translocator.Particles.P_WP_Translocator_Beacon'
		HiddenGame=true
	End Object
	DisruptedEffect=BrokenPCS
	Components.Add(BrokenPCS)
}