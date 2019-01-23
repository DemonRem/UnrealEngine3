/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class GenericBrowserType_DecalMaterial
	extends GenericBrowserType
	native;

cpptext
{
	virtual void Init();
	virtual UBOOL ShowObjectEditor( UObject* InObject );
	virtual void InvokeCustomCommand( INT InCommand, UObject* InObject );
}
	
defaultproperties
{
	Description="Decal Material"
}
