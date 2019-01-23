/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// UTPickupInventory:
// This abstract class is used to redirect
// queries normally made to an inventory class to the associated UTItemPickupFactory
//=============================================================================
class UTPickupInventory extends UTInventory
	abstract;


/* Inventory has an AI interface to allow AIControllers, such as bots, to assess the
 desireability of acquiring that pickup.  The BotDesireability() method returns a
 float typically between 0 and 1 describing how valuable the pickup is to the
 AIController.  This method is called when an AIController uses the
 FindPathToBestInventory() navigation intrinsic.
*/
static function float BotDesireability( Actor PickupHolder, Pawn P )
{
	local UTItemPickupFactory F;

	F = UTItemPickupFactory(PickupHolder);

	if ( F == None )
		return 0;

	return F.BotDesireability(P);
}

defaultproperties
{
}
