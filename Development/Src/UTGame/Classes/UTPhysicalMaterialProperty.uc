/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTPhysicalMaterialProperty extends PhysicalMaterialPropertyBase
	native;

/** Type of material this is (dirt, gravel, brick, etc) used for looking up material specific effects */
var() name MaterialType;

/** struct for list to map material types supported by an actor to impact sounds and effects */
struct native MaterialImpactEffect
{
	var name MaterialType;
	var SoundCue Sound;
	var MaterialInterface DecalMaterial;
	var float DecalWidth, DecalHeight;
	var ParticleSystem ParticleTemplate;
};

/** Struct for list to map materials to sounds, for sound only applications (e.g. tires) */
struct native MaterialSoundEffect
{
	var name MaterialType;
	var SoundCue Sound;
};

/** Struct for list to map materials to a particle effect */
struct native MaterialParticleEffect
{
	var name MaterialType;
	var ParticleSystem ParticleTemplate;
};
