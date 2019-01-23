/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModuleTypeDataSubUV extends ParticleModuleTypeDataBase
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object)
	deprecated;

enum ESubUVInterpMethod
{
    PSSUVIM_Linear,
    PSSUVIM_Linear_Blend,
    PSSUVIM_Random,
    PSSUVIM_Random_Blend
};

var(Particle) int					SubImages_Horizontal;
var(Particle) int					SubImages_Vertical;
var(Particle) ESubUVInterpMethod	InterpolationMethod;
var			  bool					DirectUV;

cpptext
{
	virtual FParticleEmitterInstance* CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent);

	virtual void SetToSensibleDefaults();

	virtual void PostEditChange(UProperty* PropertyThatChanged);

}

defaultproperties
{
	InterpolationMethod=PSSUVIM_Linear
	SubImages_Horizontal=1
	SubImages_Vertical=1
	DirectUV=false
}
