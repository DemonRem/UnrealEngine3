/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
 
/**
 * Per-instance data for a decal lifetime policy based on age.
 */
class DecalLifetimeDataAge extends DecalLifetimeData
	native(Decal);

/**
 * The current age of the decal.
 */
var float	Age;

/**
 * The age at which this decal will be killed.
 */
var float	LifeSpan;

defaultproperties
{
	LifetimePolicyName="Age";
	LifeSpan=10.0;
}
