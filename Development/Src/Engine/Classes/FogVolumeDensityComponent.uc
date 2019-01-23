/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class FogVolumeDensityComponent extends ActorComponent
	native(FogVolume)
	collapsecategories
	hidecategories(Object)
	abstract
	editinlinenew;

/** True if the fog is enabled. */
var()	const	bool			bEnabled;

/** Color used to approximate fog material color on transparency. */
var()	const	interp	LinearColor	ApproxFogLightColor;

/** Array of actors that will define the shape of the fog volume. */
var()	const	array<Actor>	FogVolumeActors;

cpptext
{
private:
	/** Adds the fog volume components to the scene */
	void AddFogVolumeComponents();

	/** Removes the fog volume components from the scene */
	void RemoveFogVolumeComponents();

	/** 
	 * Sets up FogVolumeActors's mesh components to defaults that are common usage with fog volumes.  
	 * Collision is disabled for the actor, each component gets assigned the default fog volume material,
	 * lighting, shadowing, decal accepting, and occluding are disabled.
	 */
	void SetFogActorDefaults();

protected:
	// ActorComponent interface.
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach();

	// UObject interface
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );

public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const PURE_VIRTUAL(UFogVolumeDensityComponent::CreateFogVolumeDensityInfo,return NULL;);

	/** 
	 * Checks for partial fog volume setup that will not render anything.
	 */
	virtual void CheckForErrors();
}

/**
 * Changes the enabled state of the height fog component.
 * @param bSetEnabled - The new value for bEnabled.
 */
final native function SetEnabled(bool bSetEnabled);

defaultproperties
{
	bEnabled=TRUE
	ApproxFogLightColor=(R=0.5,G=0.5,B=0.7,A=1.0)
}
