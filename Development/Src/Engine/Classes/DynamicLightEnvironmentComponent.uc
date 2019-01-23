/**
 * This is used to light components / actors during the game.  Doing something like:
 * LightEnvironment=FooLightEnvironment
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class DynamicLightEnvironmentComponent extends LightEnvironmentComponent
	native;

/** The current state of the light environment. */
var private native transient const pointer State{class FDynamicLightEnvironmentState};

/** The number of seconds between light environment updates for actors which aren't visible. */
var() float InvisibleUpdateTime;

/** Minimum amount of time that needs to pass between full environment updates. */
var() float MinTimeBetweenFullUpdates;

/** The number of visibility samples to use within the primitive's bounding volume. */
var() int NumVolumeVisibilitySamples;

/** The color of the ambient shadow. */
var() LinearColor AmbientShadowColor;

/** The direction of the ambient shadow source. */
var() vector AmbientShadowSourceDirection;

/** Ambient color added in addition to the level's lighting. */
var() LinearColor AmbientGlow;

/** Desaturation percentage of level lighting, which can be used to help team colored characters stand out better under colored lighting. */
var() float LightDesaturation;

/** The distance to create the light from the owner's origin, in radius units. */
var() float LightDistance;

/** The distance for the shadow to project beyond the owner's origin, in radius units. */
var() float ShadowDistance;

/** Whether the light environment should cast shadows */
var() bool bCastShadows;

/** Time since the caster was last visible at which the mod shadow will fade out completely.  */
var() float ModShadowFadeoutTime;

/** Exponent that controls mod shadow fadeout curve. */
var() float ModShadowFadeoutExponent;

/** Quality of shadow buffer filtering to use on the light environment */
var() EShadowFilterQuality ShadowFilterQuality;

/** Whether the light environment should be dynamically updated. */
var() bool bDynamic;

cpptext
{
	// UObject interface.
	virtual void FinishDestroy();
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );
	virtual void Serialize(FArchive& Ar);

	// UActorComponent interface.
	virtual void Tick(FLOAT DeltaTime);
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach();
}

defaultproperties
{
	InvisibleUpdateTime=4.0
	MinTimeBetweenFullUpdates=0.3
	NumVolumeVisibilitySamples=1
	AmbientShadowColor=(R=0.15,G=0.15,B=0.15)
	AmbientShadowSourceDirection=(X=0,Y=0,Z=1)
	LightDistance=1.5
	ShadowDistance=1.0
	TickGroup=TG_PostAsyncWork
	bCastShadows=TRUE
	ModShadowFadeoutExponent=3.0
	ShadowFilterQuality=SFQ_Low
	bDynamic=TRUE
}
