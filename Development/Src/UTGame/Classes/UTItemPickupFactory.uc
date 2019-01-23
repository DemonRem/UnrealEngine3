/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTItemPickupFactory extends UTPickupFactory
	native
	abstract;

var		SoundCue			PickupSound;
var		localized string	PickupMessage;			// Human readable description when picked up.
var		float				RespawnTime;

simulated function InitializePickup()
{
	InitPickupMeshEffects();
}

static function string GetLocalString(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1,
	optional PlayerReplicationInfo RelatedPRI_2
	)
{
	return Default.PickupMessage;
}

/**
 * Give the benefit of this pickup to the recipient
 */
function SpawnCopyFor( Pawn Recipient )
{
	Recipient.PlaySound( PickupSound );
	Recipient.MakeNoise(0.2);

	if ( PlayerController(Recipient.Controller) != None )
	{
		PlayerController(Recipient.Controller).ReceiveLocalizedMessage(MessageClass,,,,class);
	}
}

// Set up respawn waiting if desired.
//
function SetRespawn()
{
	if( WorldInfo.Game.ShouldRespawn(self) )
		StartSleeping();
	else
		GotoState('Disabled');
}

function float GetRespawnTime()
{
	return RespawnTime;
}

function float BotDesireability(Pawn P)
{
	return 0.0;
}

function PickedUpBy(Pawn P)
{
	local UTGame GI;
	Super.PickedUpBy(P);
	GI = UTGame(WorldInfo.Game);

	if (GI != none && GI.GameStats != none && P.PlayerReplicationInfo != none)
	{
		GI.GameStats.PickupInventoryEvent(Class,P.PlayerReplicationInfo,'pickup');
	}
}

defaultproperties
{
     RespawnTime=30.000000
	 MessageClass=class'UTPickupMessage'
	 InventoryType=class'UTGame.UTPickupInventory'
}
