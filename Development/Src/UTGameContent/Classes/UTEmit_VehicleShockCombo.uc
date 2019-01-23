/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTEmit_VehicleShockCombo extends UTReplicatedEmitter;

var class<UTExplosionLight> ExplosionLightClass;

simulated function PostBeginPlay()
{
	local PlayerController P;
	local float Dist;

	Super.PostBeginPlay();

	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		// decide whether to enable explosion light
		// @todo steve - why doesn't light work as component of Emitter?
		ForEach LocalPlayerControllers(P)
		{
			Dist = VSize(P.ViewTarget.Location - Location);
			if ( (P.Pawn == Instigator) || (Dist < ExplosionLightClass.Default.Radius) || ((Dist < 6000) && ((vector(P.Rotation) dot (Location - P.ViewTarget.Location)) > 0)) )
			{
				AttachComponent(new(Outer) ExplosionLightClass);
				break;
			}
		}
	}
}

defaultproperties
{
	EmitterTemplate=ParticleSystem'WP_ShockRifle.Particles.P_WP_ShockRifle_Explo'
	ExplosionLightClass=class'UTShockComboExplosionLight'
}
