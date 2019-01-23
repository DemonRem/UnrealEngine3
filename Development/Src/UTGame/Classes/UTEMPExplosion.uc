/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTEMPExplosion extends actor;

static function Explode(actor ExplodingActor, float Radius, optional float EMPDamage)
{	
	local UTPawn hitPawn;
	local UTDeployedActor hitDeploy;
	local UTVehicle hitVehicle;
	
	Foreach ExplodingActor.OverlappingActors(class 'UTPawn', hitPawn, radius)
	{
		if(hitPawn.ShieldBeltArmor > 0) // destroy all UTPawn's shieldbelts.
		{
			hitPawn.ShieldBeltArmor = 0;
			hitPawn.ShieldAbsorb(0);
		}
	}

	Foreach ExplodingActor.OverlappingActors(class'UTDeployedActor',hitDeploy,radius)
	{
			hitDeploy.HitByEMP();
	}

	Foreach ExplodingActor.OverlappingActors(class'UTVehicle',hitVehicle,radius)
	{
		hitVehicle.DisableVehicle();
		hitVehicle.TakeDamage(EMPDamage, Pawn(ExplodingActor).Controller, hitVehicle.location, -(hitVehicle.velocity * 0.75), class'UTDmgType_EMP',, ExplodingActor);
	}
}

