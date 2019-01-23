/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * UT3 extended version of the UITabControl
 */
class UTUITabControl extends UITabControl
	native(UI)
	placeable;

function bool ProcessInputKey( const out InputEventParameters EventParms )
{
	//@TODO: This is currently a hack, need to figure out what we want to support in UT and what we dont.
	return false;
}

defaultproperties
{
	TabButtonSize=(Value=0.05,ScaleType=UIEXTENTEVAL_PercentOwner,Orientation=UIORIENT_Vertical)
}
