/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class DecalActor extends Actor
	native(Decal)
	placeable;

var() editconst const DecalComponent Decal;

cpptext
{
	// UObject interface.
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	// AActor interface.
	virtual void PostEditImport();
	virtual void PostEditMove(UBOOL bFinished);
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);

	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
}

defaultproperties
{
	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EngineResources.S_Actor'
		HiddenGame=True
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)

	Begin Object Class=DecalComponent Name=NewDecalComponent
		bStaticDecal=true
	End Object
	Decal=NewDecalComponent
	Components.Add(NewDecalComponent)

	Begin Object Class=ArrowComponent Name=ArrowComponent0
		HiddenGame=true
	End Object
	Components.Add(ArrowComponent0)

	bStatic=true
	bMovable=false
}
