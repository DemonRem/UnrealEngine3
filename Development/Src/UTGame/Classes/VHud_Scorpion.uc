/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class VHud_Scorpion extends UTUIScene_VehicleHud
	native(UI);

/** Cached reference to the ProgressBar for displaying the boost. */
var transient UIProgressBar BoostBar;
var transient UILabel BoostMsg;
var() name BoostBarName;
var() name BoostMsgName;

var transient string LastBoostMsg;
var transient string BoostMarkup;
var transient string EjectMarkup;

cpptext
{
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );
	virtual void Tick( FLOAT DeltaTime );
}

defaultproperties
{
	BoostBarName=BoostBar
	BoostMsgName=BoostMsg

	BoostMarkup="<Strings:UTGameUI.VHud_Scorpion.BoostMsg>"
	EjectMarkup="<Strings:UTGameUI.VHud_Scorpion.EjectMsg>"

}
