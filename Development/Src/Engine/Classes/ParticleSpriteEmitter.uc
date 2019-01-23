/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleSpriteEmitter extends ParticleEmitter
	native(Particle)
	collapsecategories		
	hidecategories(Object)
	editinlinenew;

enum EParticleScreenAlignment
{
	PSA_Square,
	PSA_Rectangle,
	PSA_Velocity,
	PSA_TypeSpecific
};

var	deprecated	EParticleScreenAlignment			ScreenAlignment;
var deprecated	MaterialInterface					Material;

cpptext
{
	virtual void PostLoad();
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InComponent);
	virtual void SetToSensibleDefaults();
}
