/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
/** jump boots drastically increase a player's double jump velocity */
class UTJumpBoots extends UTInventory;

/** the Z velocity boost to give the owner's double jumps */
var float MultiJumpBoost;
/** the number of jumps that the owner can do before the boots run out */
var repnotify databinding byte Charges;
/** sound to play when the boots are used */
var SoundCue ActivateSound;
/** message to send to the owner when the boots run out */
var databinding	localized string RanOutText;

replication
{
	if (bNetOwner && bNetDirty && Role==ROLE_Authority)
		Charges;
}

function GivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	Super.GivenTo(NewOwner, bDoNotActivate);
	AdjustPawn(UTPawn(NewOwner), false);
}

function ItemRemovedFromInvManager()
{
	AdjustPawn(UTPawn(Owner), true);
}

reliable client function ClientGivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	Super.ClientGivenTo(NewOwner, bDoNotActivate);
	if (Role < ROLE_Authority)
	{
		AdjustPawn(UTPawn(NewOwner), false);
	}
}

simulated event Destroyed()
{
	if (Role < ROLE_Authority)
	{
		AdjustPawn(UTPawn(Owner), true);
	}

	Super.Destroyed();
}

simulated function ReplicatedEvent(name VarName)
{
	if (VarName == 'Charges')
	{
		AdjustPlayerDoll(UTPawn(Owner), true, Charges);
	}

	Super.ReplicatedEvent(VarName);
}

simulated function AdjustPlayerDoll(UTPawn P, bool bPawnHasBoots, int NoCharges)
{
	local UTPlayerController PC;
	local UTHud H;

	if (P != none )
	{
		PC = UTPlayerController(P.Owner);
		if ( PC!= none )
		{
			H = UTHud( PC.myHUD );
			if ( H != none )
			{
				H.ChangeBootStatus(bPawnHasBoots,NoCharges);
			}
		}
	}
}
/** adds or removes our bonus from the given pawn */
simulated function AdjustPawn(UTPawn P, bool bRemoveBonus)
{
	if (P != None)
	{
		if (bRemoveBonus)
		{
			AdjustPlayerDoll(P, false, 0);
			P.MultiJumpBoost -= MultiJumpBoost;
			P.MaxFallSpeed -= MultiJumpBoost;
		}
		else
		{
			P.MultiJumpBoost += MultiJumpBoost;
			P.MaxFallSpeed += MultiJumpBoost;
			AdjustPlayerDoll(P, true, Charges);
		}
	}
}

simulated function OwnerEvent(name EventName)
{
	if (Role == ROLE_Authority)
	{
		if (EventName == 'MultiJump')
		{
			Charges--;
			Owner.PlaySound(ActivateSound, false, true, false);
			AdjustPlayerDoll( UTPawn(Owner), true, Charges);
		}
		else if (EventName == 'Landed' && Charges <= 0)
		{
			ClientExpired();
			Destroy();
		}
	}
	else if (EventName == 'MultiJump')
	{
		Owner.PlaySound(ActivateSound, false, true, false);
	}
}

/** called by the server on the client when Charges reaches 0 */
reliable client function ClientExpired()
{
	if (Pawn(Owner) != None)
	{
		Pawn(Owner).ClientMessage(RanOutText);
	}
}

function bool DenyPickupQuery(class<Inventory> ItemClass, Actor Pickup)
{
	if (ItemClass == Class)
	{
		Charges = default.Charges;
		AdjustPlayerDoll( UTPawn(Owner), true, Charges);
		Pickup.PickedUpBy(Instigator);
		AnnouncePickup(Instigator);
		return true;
	}

	return false;
}

function DropFrom(vector StartLocation, vector StartVelocity)
{
	if (Charges <= 0)
	{
		Destroy();
	}
	else
	{
		Super.DropFrom(StartLocation, StartVelocity);
	}
}

static function float BotDesireability(Actor PickupHolder, Pawn P)
{
	local UTJumpBoots AlreadyHas;

	AlreadyHas = UTJumpBoots(P.FindInventoryType(default.Class));
	if (AlreadyHas != None)
	{
		return (default.MaxDesireability / (1 + AlreadyHas.Charges));
	}

	return default.MaxDesireability;
}

static function float DetourWeight(Pawn Other, float PathWeight)
{
	return (default.MaxDesireability / PathWeight);
}

simulated function RenderOverlays( HUD H )
{

	//// ---------- FIXME

/*
	local UTHud UTH;
	local float x,y;

	UTH = UTHud(H);
	if (UTH!=none)
	{
		X = H.Canvas.ClipX-5;
		Y = 74 * (H.Canvas.CLipX / 1024);
		UTH.DrawNumericWidget(X, Y, 72, 225, 43, 39, Charges, 3, true);
	}
*/
}


defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=StaticMeshComponent1
		StaticMesh=StaticMesh'Pickups.JumpBoots.Mesh.S_JumpBoots'
		bOnlyOwnerSee=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CollideActors=false
		Translation=(X=0.0,Y=0.0,Z=-8.0)
		Scale=1.4
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=StaticMeshComponent1
	PickupFactoryMesh=StaticMeshComponent1

	MaxDesireability=0.50
	RespawnTime=30.0
	bReceiveOwnerEvents=true
	bDropOnDeath=true
	PickupSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_JumpBoots_PickupCue'

	Charges=3
	MultiJumpBoost=750.0
	ActivateSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_JumpBoots_JumpCue'

	bRenderOverlays=true
}
