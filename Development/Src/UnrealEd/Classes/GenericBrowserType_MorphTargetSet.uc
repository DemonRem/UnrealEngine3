/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//-----------------------------------------------------------
// Browser type for morph target sets
//-----------------------------------------------------------
class GenericBrowserType_MorphTargetSet extends GenericBrowserType
	native;

cpptext
{
	virtual void Init();
	virtual UBOOL ShowObjectEditor( UObject* InObject );
}

DefaultProperties
{
	Description="MorphTargetSet"
}
