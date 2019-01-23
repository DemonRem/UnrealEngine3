//=============================================================================
// UT specific modifications to Pickup items.
//
// PickupFactory should be used to place items in the level.  This class is for dropped inventory, which should attach
// itself to this pickup, and set the appropriate mesh
//
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class UTDroppedPickup extends DroppedPickup
	notplaceable
	native;

var() float YawRotationRate;
var PrimitiveComponent PickupMesh;
var float StartScale;
var bool bRotatingPickup;
var bool bPickupable; // EMP forces a pickup to be unusable until it lands
var LightEnvironmentComponent MyLightEnvironment;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

event PreBeginPlay()
{
	Super.PreBeginPlay();

	// if player who dropped me is still alive, prevent picking up until landing
	// to prevent that player from immediately picking us up
	bPickupable = (Instigator == None || Instigator.Health <= 0);
}

simulated event SetPickupMesh(PrimitiveComponent NewPickupMesh)
{
	if (NewPickupMesh != None && WorldInfo.NetMode != NM_DedicatedServer )
	{
		PickupMesh = new(self) NewPickupMesh.Class(NewPickupMesh);
		if ( class<UTWeapon>(InventoryClass) != None )
		{
			PickupMesh.SetScale(PickupMesh.Scale * 1.2);
		}
		PickupMesh.SetLightEnvironment(MyLightEnvironment);
		AttachComponent(PickupMesh);
	}
}

simulated event Landed(vector HitNormal, Actor FloorActor)
{
	local float DotP;

	Super.Landed(HitNormal, FloorActor);

	if (PickupMesh != None)
	{
		DotP = HitNormal dot vect(0,0,1);
		if (DotP != 0.0)
		{
			PickupMesh.SetTranslation(vect(0,0,-1) * sqrt(1.0 - square(DotP)) * CylinderComponent(CollisionComponent).CollisionRadius/DotP);
		}
	}
}

auto state Pickup
{
	/*
	 Validate touch (if valid return true to let other pick me up and trigger event).
	*/
	function bool ValidTouch(Pawn Other)
	{
		return (bPickupable) ? Super.ValidTouch(Other) : false;
	}

	simulated event Landed(vector HitNormal, Actor FloorActor)
	{
		Global.Landed(HitNormal, FloorActor);
		if (Role == ROLE_Authority && !bPickupable)
		{
			bPickupable = true;
			CheckTouching();
		}
	}
}

State FadeOut
{
	simulated function BeginState(Name PreviousStateName)
	{
		bFadeOut = true;
		if ( PickupMesh != None )
		{
			StartScale = PickupMesh.Scale;
		}
		LifeSpan = 1.0;
		YawRotationRate = 60000;
	}
}

defaultproperties
{
	Begin Object Class=DynamicLightEnvironmentComponent Name=MyLightEnvironment0
	End Object
	Components.Add(MyLightEnvironment0)
	MyLightEnvironment=MyLightEnvironment0

	bRotatingPickup=false
	YawRotationRate=32768
	bPickupable=true
}
