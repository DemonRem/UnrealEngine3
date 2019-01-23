/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
 
/**
 * Baseclass for decal lifetime policy objects.
 */
class DecalLifetime extends Object
	native(Decal)
	abstract;

/** Lifetime policy name. */
var const string PolicyName;

/** List of dynamic decals managed by this lifetime policy. */
var const array<DecalComponent> ManagedDecals;

cpptext
{
public:
	/**
	 * Called by UDecalManager::Tick.
	 */
	virtual void Tick(FLOAT DeltaSeconds);

	/**
	 * Instructs this policy to manage the specified decal.  Called by UDecalManager::AddDynamicDecal.
	 */
	void AddDecal(UDecalComponent* InDecalComponent)
	{
		// Invariant: This lifetime policy doesn't already manage this decal.
		ManagedDecals.AddItem( InDecalComponent );
	}

	/**
	 * Instructs this policy to stop managing the specified decal.  Called by UDecalManager::RemoveDynamicDecal.
	 */
	void RemoveDecal(UDecalComponent* InDecalComponent)
	{
		ManagedDecals.RemoveItem( InDecalComponent );
	}

public:
	/**
	 * Helper function for derived classes that enacpsulates decal detachment, owner components array clearing, etc.
	 */
	void EliminateDecal(UDecalComponent* InDecalComponent);
}
