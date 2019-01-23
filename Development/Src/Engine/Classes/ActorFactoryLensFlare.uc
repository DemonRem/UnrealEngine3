/**
 * Copyright � 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ActorFactoryLensFlare extends ActorFactory
	config(Editor)
	collapsecategories
	hidecategories(Object)
	native;

cpptext
{
	virtual AActor* CreateActor( const FVector* const Location, const FRotator* const Rotation, const class USeqAct_ActorFactory* const ActorFactoryData );
	virtual UBOOL CanCreateActor(FString& OutErrorMsg);
	virtual void AutoFillFields(class USelection* Selection);
	virtual FString GetMenuName();
}

var()	LensFlare		LensFlareObject;

defaultproperties
{
	MenuName="Add LensFlare"
	NewActorClass=class'Engine.LensFlareSource'
	//GameplayActorClass=class'Engine.EmitterSpawnable'
}
