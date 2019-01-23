/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
 
/**
 * Decal lifetime policy that kills decal components that are older than a certain age.
 */
class DecalLifetimeAge extends DecalLifetime
	native(Decal);

cpptext
{
public:
	/**
	 * Called by UDecalManager::Tick.
	 */
	virtual void Tick(FLOAT DeltaSeconds);
}

defaultproperties
{
	PolicyName="Age"
}
