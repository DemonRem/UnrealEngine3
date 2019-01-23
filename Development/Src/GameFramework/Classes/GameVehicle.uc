/**
 * GameVehicle
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class GameVehicle extends SVehicle
	config(Game)
	native
	abstract
	notplaceable;



defaultproperties
{
	bCanBeAdheredTo=TRUE
	bCanBeFrictionedTo=TRUE
}
