/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeaponPickupFactory extends UTPickupFactory
	native
	nativereplication;

var() RepNotify	class<UTWeapon>			WeaponPickupClass;
var   			bool					bWeaponStay;
/** The glow that emits from the base while the weapon is available */

var				ParticleSystemComponent	BaseGlow;
/** Used to scale weapon pickup drawscale */
var float WeaponPickupScaling;

replication
{
	if (ROLE==ROLE_Authority && bNetInitial)
		WeaponPickupClass;
}

cpptext
{
	virtual void Serialize(FArchive& Ar);
	virtual void CheckForErrors();
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
}

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();

	PickupClassChanged();
}

simulated function PickupClassChanged()
{
	InventoryType = WeaponPickupClass;

	if ( InventoryType == None )
	{
		GotoState('Disabled');
		return;
	}

	//FIXME: Use a different mesh instead of just scale
	if (WeaponPickupClass.Default.bSuperWeapon)
	{
		if ( BaseMesh != None )
			BaseMesh.SetScale(2.0);
	}

	SetWeaponStay();
}

simulated function SetPickupVisible()
{
	BaseGlow.ActivateSystem();
	super.SetPickupVisible();
}
simulated function SetPickupHidden()
{
	BaseGlow.DeactivateSystem();
	super.SetPickupHidden();
}

simulated function SetPickupMesh()
{
	Super.SetPickupMesh();
	if ( PickupMesh != none )
	{
		PickupMesh.SetScale(PickupMesh.Scale * WeaponPickupScaling);
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'WeaponPickupClass' )
	{
		PickupClassChanged();
		SetPickupMesh();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

function bool CheckForErrors()
{
	if ( Super.CheckForErrors() )
		return true;

	if ( WeaponPickupClass == None )
	{
		`log(self$" no weapon pickup class");
		return true;
	}
	else if (ClassIsChildOf(WeaponPickupClass, class'UTDeployable'))
	{
		`Log(self @ "cannot hold deployables");
		return true;
	}

	return false;
}

/**
 * If our charge is not a super weapon and weaponstay is on, set weapon stay
 */

function SetWeaponStay()
{
	bWeaponStay = ( !WeaponPickupClass.Default.bSuperWeapon && UTGame(WorldInfo.Game).bWeaponStay );
}

function StartSleeping()
{
	if (!bWeaponStay)
	    GotoState('Sleeping');
}

function bool AllowRepeatPickup()
{
    return !bWeaponStay;
}

function SpawnCopyFor( Pawn Recipient )
{
	local Inventory Inv;

	if ( UTInventoryManager(Recipient.InvManager)!=None )
	{
		Inv = UTInventoryManager(Recipient.InvManager).HasInventoryOfClass(WeaponPickupClass);
		if ( UTWeapon(Inv)!=none )
		{
			UTWeapon(Inv).AddAmmo(WeaponPickupClass.Default.AmmoCount);
			UTWeapon(Inv).AnnouncePickup(Recipient);
			return;
		}
	}
	super.SpawnCopyFor(Recipient);

}

defaultproperties
{
	bMovable=FALSE 
	bStatic=FALSE
	
	bWeaponStay=true
	bRotatingPickup=true
	bCollideActors=true
	bBlockActors=true
	WeaponPickupScaling=+1.2
	PivotTranslation=(Y=-25.0)

	RespawnSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Respawn_Cue'

	Begin Object NAME=CollisionCylinder
		BlockZeroExtent=false
	End Object

	Begin Object Name=BaseMeshComp
		StaticMesh=StaticMesh'Pickups.WeaponBase.S_Pickups_WeaponBase'
		Translation=(X=0.0,Y=0.0,Z=-44.0)
		Scale3D=(X=1.0,Y=1.0,Z=1.0)
	End Object

	Begin Object Class=ParticleSystemComponent Name=GlowEffect
		bAutoActivate=TRUE
		Template=ParticleSystem'Pickups.WeaponBase.Effects.P_Pickups_WeaponBase_Glow'
		Translation=(X=0.0,Y=0.0,Z=-44.0)
		SecondsBeforeInactive=1.0f
	End Object
	BaseGlow=GlowEffect;
	Components.Add(GlowEffect);
	bDoVisibilityFadeIn=false
}

