/**
* Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
*/
class UTArmorPickup_ShieldBelt extends UTArmorPickupFactory;


/** 
* CanUseShield() returns how many shield units P could use
*
* @Returns returns how many shield units P could use
*/

function int CanUseShield(UTPawn P)
{
	return Max(0,ShieldAmount - P.ShieldBeltArmor);
}

/** 
* AddShieldStrength() add shield to appropriate P armor type.
*
* @Param	P 	The UTPawn to give shields to
*/

function AddShieldStrength(UTPawn P)
{
	local Materialinstance ShieldMat;

	// Get the proper shield material
	ShieldMat = P.GetShieldMaterialInstance(P, WorldInfo.Game.bTeamGame);

	// Assign it
	P.ShieldBeltArmor = Max(ShieldAmount, P.ShieldBeltArmor);
	if (P.GetOverlayMaterial() == None)
	{
		P.SetOverlayMaterial(ShieldMat);
	}
}



defaultproperties
{
	ShieldAmount=100
		bIsSuperItem=true
		RespawnTime=60.000000
		MaxDesireability=1.500000
		PickupSound=SoundCue'A_Pickups.Shieldbelt.Cue.A_Pickups_Shieldbelt_Activate_Cue'

		Begin Object Class=StaticMeshComponent Name=MeshComponentA
		StaticMesh=StaticMesh'Pickups.Armor_ShieldBelt.Mesh.S_Armor_ShieldBelt'
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CollideActors=false
		Scale3D=(X=3.0,Y=3.0,Z=3.0)
		CullDistance=8000
		bUseAsOccluder=FALSE
		End Object
		PickupMesh=MeshComponentA
		Components.Add(MeshComponentA)

}



