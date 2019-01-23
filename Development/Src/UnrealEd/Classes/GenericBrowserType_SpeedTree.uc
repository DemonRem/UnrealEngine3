/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
 
class GenericBrowserType_SpeedTree extends GenericBrowserType
	native;

cpptext
{
	virtual void Init();
	virtual UBOOL ShowObjectEditor( UObject* InObject );
}
	
defaultproperties
{
	Description="SpeedTree"
}
