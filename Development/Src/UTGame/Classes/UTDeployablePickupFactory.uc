/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 * @TODO FIXMESTEVE MOVE TO UTGAMECONTENT
 */
class UTDeployablePickupFactory extends UTPickupFactory;

var() repnotify class<UTDeployable> DeployablePickupClass;

var bool bDelayRespawn;

replication
{
	if (Role == ROLE_Authority && bNetInitial)
		DeployablePickupClass;
}

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();

	PickupClassChanged();
}

simulated function PickupClassChanged()
{
	InventoryType = DeployablePickupClass;
}

simulated function SetPickupMesh()
{
	Super.SetPickupMesh();
	if ( PickupMesh != none )
	{
		DeployablePickupClass.static.InitPickupMesh(PickupMesh);
		PickupMesh.SetLightEnvironment(LightEnvironment);
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'DeployablePickupClass')
	{
		PickupClassChanged();
		SetPickupMesh();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

function SpawnCopyFor( Pawn Recipient )
{
	local Inventory Inv;
	local UTDeployable Deployable;

	Inv = Spawn(InventoryType);
	if ( Inv != None )
	{
		Inv.GiveTo(Recipient);
		Inv.AnnouncePickup(Recipient);
		Deployable = UTDeployable(Inv);
		if ( (Deployable != None) && Deployable.bDelayRespawn )
		{
			bDelayRespawn = true;
			Deployable.Factory = self;
		}
		else
		{
			bDelayRespawn = false;
		}
	}
}

function StartSleeping()
{
	if ( bDelayRespawn )
	{
		GotoState('WaitingForDeployable');
	}
	else
	{
		super.StartSleeping();
	}
}

/** called when the deployable spawned by this factory has been used up */
function DeployableUsed(actor ChildDeployable)
{
	`warn("called when not waiting for deployable!");
}

state WaitingForDeployable
{
	ignores Touch;

	function StartSleeping() {}

	function BeginState(name PrevStateName)
	{
		SetPickupHidden();

		Super.BeginState(PrevStateName);
		bPulseBase=false;
		StartPulse( BaseDimEmissive );
	}

	function DeployableUsed(actor ChildDeployable)
	{
		// now start normal respawn process
		GotoState('Sleeping');
	}

	function bool ReadyToPickup(float MaxWait)
	{
		return false;
	}

Begin:
}

simulated function InitializePickup()
{
	if ( InventoryType == None )
	{
		`warn("No inventory type for "$self);
		return;
	}

	bPredictRespawns = InventoryType.Default.bPredictRespawns;
	MaxDesireability = InventoryType.Default.MaxDesireability;
	SetPickupMesh();
}

defaultproperties
{
	bMovable=FALSE 
	bStatic=FALSE
	bIsSuperItem=true

	bRotatingPickup=true
	bCollideActors=true
	bBlockActors=true

	Begin Object NAME=CollisionCylinder
		BlockZeroExtent=false
	End Object

	// content move me
	Begin Object Name=BaseMeshComp
		StaticMesh=StaticMesh'Pickups.Base_Deployable.Mesh.S_Pickups_Base_Deployable'
		Translation=(X=0.0,Y=0.0,Z=-44.0)
	End Object

	BaseBrightEmissive=(R=1.0,G=25.0,B=1.0)
	BaseDimEmissive=(R=0.25,G=5.0,B=0.25)
	bDoVisibilityFadeIn=false
}
