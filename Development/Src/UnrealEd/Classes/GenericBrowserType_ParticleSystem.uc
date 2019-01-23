/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//=============================================================================
// GenericBrowserType_ParticleSystem: ParticleSystems
//=============================================================================

class GenericBrowserType_ParticleSystem
	extends GenericBrowserType
	native;

cpptext
{
	virtual void Init();
	virtual UBOOL ShowObjectEditor( UObject* InObject );
}
	
defaultproperties
{
	Description="Particle System"
}
