/**
 * Obsolete/deprecated - will be removed
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class CarriedObject extends Actor
    abstract notplaceable deprecated;

var const NavigationPoint LastAnchor;		// recent nearest path
var		float	LastValidAnchorTime;	// last time a valid anchor was found

event NotReachableBy(Pawn P);

defaultproperties
{
}
