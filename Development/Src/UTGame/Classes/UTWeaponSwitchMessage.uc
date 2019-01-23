/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
//
// OptionalObject is an Pickup class
//
class UTWeaponSwitchMessage extends UTLocalMessage;

static function string GetString(
	optional int Switch,
	optional bool bPRI1HUD,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if ( Actor(OptionalObject) != None )
	{
		return Actor(OptionalObject).GetHumanReadableName();
	}
	return "";
}

static function color GetColor(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	if ( UTWeapon(OptionalObject) != None )
	{
		return UTWeapon(OptionalObject).default.WeaponColor;
	}
	return Default.DrawColor;
}

defaultproperties
{
	bIsUnique=true
	bCountInstances=true
	DrawColor=(R=255,G=255,B=0,A=255)
	FontSize=2

	MessageArea=4
}
