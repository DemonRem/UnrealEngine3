/**
 * This is used by the scene management to isolate lights and primitives.  For lighting and actor or component
 * use a DynamicLightEnvironmentComponent.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class LightEnvironmentComponent extends ActorComponent
	native;

var() const bool bEnabled;

/** The set of lights which affect the component. */
var transient noimport const array<LightComponent> Lights;

/** The scene's info about the light environment. */
var native private transient noimport const pointer	SceneInfo{class FLightEnvironmentSceneInfo};

/** The time when a primitive in the light environment was last rendered. */
var transient const float LastRenderTime;

cpptext
{
	/** Resets the light environment's light interaction. */
	void ResetLightInteractions();

	/**
	 * Sets whether a light affects the light environment or not.
	 * @param Light - The light in question.
	 * @param bAffectThisLightEnvironment - TRUE if the light affects the light environment.
	 */
	void SetLightInteraction(ULightComponent* Light,UBOOL bAffectThisLightEnvironment);

	/** Default constructor. */
	ULightEnvironmentComponent();

	// UActorComponent interface.
	virtual void Attach();
	virtual void Detach();

	// UObject interface.
	virtual void FinishDestroy();
}

/**
 * Changes the value of bEnabled.
 * @param bNewEnabled - The value to assign to bEnabled.
 */
native final function SetEnabled(bool bNewEnabled);

defaultproperties
{
	bEnabled=True
}
