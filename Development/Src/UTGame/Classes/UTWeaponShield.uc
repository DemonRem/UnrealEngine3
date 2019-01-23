/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** special actor that only blocks weapons fire */
class UTWeaponShield extends Actor
	native
	abstract;

cpptext
{
	virtual UBOOL IgnoreBlockingBy(const AActor* Other) const;
	virtual UBOOL ShouldTrace(UPrimitiveComponent* Primitive, AActor* SourceActor, DWORD TraceFlags);
}

defaultproperties
{
	bProjTarget=true
	bCollideActors=true
}
