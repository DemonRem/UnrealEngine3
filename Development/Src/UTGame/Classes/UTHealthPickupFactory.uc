/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTHealthPickupFactory extends UTItemPickupFactory
	abstract;

var		int					HealingAmount;
var		bool				bSuperHeal;


simulated static function UpdateHUD(UTHUD H)
{
	Super.UpdateHUD(H);
	H.LastHealthPickupTime = H.LastPickupTime;
}

function SpawnCopyFor( Pawn Recipient )
{
	// Give health to recipient
	Recipient.Health += HealAmount(Recipient);

	super.SpawnCopyFor(Recipient);

}

function int HealAmount(Pawn Recipient)
{
	local UTPawn UTP;

	if (bSuperHeal)
	{
		UTP = UTPawn(Recipient);
		if (UTP != None)
		{
			return FClamp(UTP.SuperHealthMax - Recipient.Health, 0, HealingAmount);
		}
	}
	return FClamp(Recipient.Default.Health - Recipient.Health, 0, HealingAmount);
}

//=============================================================================
// Pickup state: this inventory item is sitting on the ground.

auto state Pickup
{
	/* DetourWeight()
	value of this path to take a quick detour (usually 0, used when on route to distant objective, but want to grab inventory for example)
	*/
	function float DetourWeight(Pawn Other,float PathWeight)
	{
		local int Heal;

		Heal = HealAmount(Other);

		if ( AIController(Other.Controller).PriorityObjective() && (Other.Health > 65) )
			return (0.01 * Heal)/PathWeight;
		return (0.02 * Heal)/PathWeight;
	}

	/* ValidTouch()
	 Validate touch (if valid return true to let other pick me up and trigger event).
	*/
	function bool ValidTouch( Pawn Other )
	{
		if ( !Super.ValidTouch(Other) )
		{
			return false;
		}

		// superhealth can always be picked up in DM (to deny it to other players)
		if ( bSuperHeal && !WorldInfo.Game.bTeamGame )
		{
			return true;
		}

		// does Other need health?
		return ( HealAmount(Other) > 0 );
	}
}

function float BotDesireability(Pawn P)
{
	local float desire;

	desire = HealAmount(P);

	if ( (P.Weapon == None) || (P.Weapon.AIRating > 0.5) )
		desire *= 1.7;
	if ( bSuperHeal || (P.Health < 45) )
		return ( FMin(0.03 * desire, 2.2) );
	else
	{
		if ( desire > 6 )
			desire = FMax(desire,25);
		else if ( (UTBot(P.Controller) != None) && UTBot(P.Controller).bHuntPlayer )
			return 0;
		return ( FMin(0.017 * desire, 2.0) );
	}
}

defaultproperties
{
	bMovable=FALSE
	bStatic=FALSE

	MaxDesireability=0.700000
	HealingAmount=20

	BaseBrightEmissive=(R=2.0,G=50.0,B=10.0,A=1.0)
	BaseDimEmissive=(R=0.20,G=5.0,B=1.0,A=0.0)

	bDoVisibilityFadeIn=FALSE

	RespawnSound=SoundCue'A_Pickups.Health.Cue.A_Pickups_Health_Respawn_Cue'

	Begin Object Class=StaticMeshComponent Name=HealthPickUpMesh
	    CastShadow=FALSE
		bCastDynamicShadow=FALSE
		bAcceptsLights=TRUE
		bForceDirectLightMap=TRUE
		LightEnvironment=PickupLightEnvironment

		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true

		CollideActors=false
		BlockActors=false
		BlockRigidBody=false

		CullDistance=4500
	    bUseAsOccluder=FALSE
	End Object
	PickupMesh=HealthPickUpMesh
	Components.Add(HealthPickUpMesh)

}
