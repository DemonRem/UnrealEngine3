/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_NightshadeGun extends UTVehicleWeapon
	abstract
	hidedropdown
	native(Vehicle);

struct native DeployableData
{
	/** Class of the actors that can be deployed (team specific)*/
	var class<UTDeployable> DeployableClass;

	/** The Maximum number of deployables available to drop.  Limited to 15 */
	var byte MaxCnt;

	/** How far away from the vehicle should it be dropped */
	var vector DropOffset;

	var array<Actor> Queue;
};

const NUMDEPLOYABLETYPES=5;

var DeployableData DeployableList[NUMDEPLOYABLETYPES];

var bool	bShowDeployableName;

var byte Counts[NUMDEPLOYABLETYPES];

var int DeployableIndex;

/** The sound to play when alt-fire mode is changed */
var SoundCue AltFireModeChangeSound;

/** Holds the Pawn that this weapon is linked to. */
var Actor	LinkedTo;

/** Holds the component we hit on the linked pawn, for determining the linked beam endpoint on multi-component actors (such as Onslaught powernodes) */
var PrimitiveComponent LinkedComponent;

/** Holds the Actor currently being hit by the beam */
var Actor	Victim;

/** Holds the current strength (in #s) of the link */
var int		LinkStrength;

/** Holds the amount of flexibility of the link beam */
var float 	LinkFlexibility;

/** Holds the amount of time to maintain the link before breaking it.  This is important so that you can pass through
    small objects without having to worry about regaining the link */
var float 	LinkBreakDelay;

/** Momentum transfer for link beam (per second) */
var float	MomentumTransfer;

/** This is a time used with LinkBrekaDelay above */
var float	ReaccquireTimer;

/** true if beam currently hitting target */
var bool	bBeamHit;

/** saved partial damage (in case of high frame rate */
var float	SavedDamage;

/** Holds the text UV's for each icon */
var UIRoot.TextureCoordinates IconCoords[NUMDEPLOYABLETYPES];

/** Sound to play when an item is deployed*/
var SoundCue DeployedItemSound;
replication
{
	if (Role == ROLE_Authority)
		LinkedTo,DeployableIndex;
	if ( bNetOwner && (Role == ROLE_Authority) )
		Counts;
}

/**
 * This function looks at who/what the beam is touching and deals with it accordingly.  bInfoOnly
 * is true when this function is called from a Tick.  It causes the link portion to execute, but no
 * damage/health is dealt out.
 */
simulated function UpdateBeam(float DeltaTime)
{
	local Vector		StartTrace, EndTrace;
	local ImpactInfo	TestImpact;

	// define range to use for CalcWeaponFire()
	StartTrace	= InstantFireStartTrace();
	EndTrace	= InstantFireEndTrace(StartTrace);

	// Trace a shot
	TestImpact = CalcWeaponFire( StartTrace, EndTrace );

	// Allow children to process the hit
	ProcessBeamHit(StartTrace, vect(0,0,0), TestImpact, DeltaTime);
}

simulated function PostBeginPlay()
{
	local int i;

	Super.PostBeginPlay();

	for ( i=0; i<NUMDEPLOYABLETYPES; i++ )
	{
		Counts[i] = DeployableList[i].MaxCnt;
	}
}

/**
 * Process the hit info
 */
simulated function ProcessBeamHit(vector StartTrace, vector AimDir, out ImpactInfo TestImpact, float DeltaTime)
{
	local float DamageAmount;
	local vector PushForce, ShotDir, SideDir;

	Victim = TestImpact.HitActor;

	// If we are on the server, attempt to setup the link
	if (Role==ROLE_Authority)
	{
		// Try linking
		AttemptLinkTo(Victim, TestImpact.HitInfo.HitComponent);

		// set the correct firemode on the pawn, since it will change when linked
		SetCurrentFireMode(CurrentFireMode);

		// cause damage or add health/power/etc.
		bBeamHit = false;

		DamageAmount = InstantHitDamage[0] * DeltaTime + SavedDamage;
		SavedDamage = DamageAmount - int(DamageAmount);
		DamageAmount -= SavedDamage;
		if ( DamageAmount >= 1 )
		{
			if ( LinkedTo != None )
			{
				// heal them if linked
				// linked players will use ammo when they fire
				if ( LinkedTo.IsA('UTVehicle') || LinkedTo.IsA('UTGameObjective') )
				{
					// use ammo only if we actually healed some damage
					LinkedTo.HealDamage(DamageAmount * Instigator.GetDamageScaling(), Instigator.Controller, InstantHitDamageTypes[1]);
				}
			}
			else
			{
				if (Victim != None && !WorldInfo.Game.GameReplicationInfo.OnSameTeam(Victim, Instigator))
				{
					bBeamHit = !Victim.bWorldGeometry;
					if ( DamageAmount > 0 )
					{
						ShotDir = Normal(TestImpact.HitLocation - Location);
						SideDir = Normal(ShotDir Cross vect(0,0,1));
						PushForce =  vect(0,0,1) + Normal(SideDir * (SideDir dot (TestImpact.HitLocation - Victim.Location)));
						PushForce *= (Victim.Physics == PHYS_Walking) ? 0.1*MomentumTransfer : DeltaTime*MomentumTransfer;
						Victim.TakeDamage(DamageAmount, Instigator.Controller, TestImpact.HitLocation, PushForce, InstantHitDamageTypes[1], TestImpact.HitInfo, self);
					}
				}
			}
		}
	}
	else
	{
		// set the correct firemode on the pawn, since it will change when linked
		SetCurrentFireMode(CurrentFireMode);
	}

	// if we do not have a link, set the flash location to whatever we hit
	// (if we do have one, AttemptLinkTo() will set the correct flash location for the Actor we're linked to)
	if (LinkedTo == None)
	{
		SetFlashLocation( TestImpact.HitLocation );
	}
}

/**
 * Returns a vector that specifics the point of linking.
 */
simulated function vector GetLinkedToLocation()
{
	if (LinkedTo == None)
	{
		return vect(0,0,0);
	}
	else if (Pawn(LinkedTo) != None)
	{
		return LinkedTo.Location + Pawn(LinkedTo).BaseEyeHeight * vect(0,0,0.5);
	}
	else if (LinkedComponent != None)
	{
		return LinkedComponent.GetPosition();
	}
	else
	{
		return LinkedTo.Location;
	}
}

/**
 * This function looks at how the beam is hitting and determines if this person is linkable
 */
function AttemptLinkTo(Actor Who, PrimitiveComponent HitComponent)
{
	local UTVehicle UTV;
	local UTOnslaughtObjective UTO;
	local Vector 		StartTrace, EndTrace, V, HitLocation, HitNormal;
	local Actor			HitActor;

	// redirect to vehicle if owned by a vehicle and the vehicle allows it
	if( Who != none )
	{
		UTV = UTVehicle(Who.Owner);
		if (UTV != None && UTV.AllowLinkThroughOwnedActor(Who))
		{
			Who = UTV;
		}
	}

	// Check for linking to pawns
	UTV = UTVehicle(Who);
	if ( (UTV != None) && UTV.bValidLinkTarget )
	{
		// Check teams to make sure they are on the same side
		if (WorldInfo.Game.GameReplicationInfo.OnSameTeam(Who,Instigator))
		{
			if (UTV != None)
			{
				LinkedComponent = HitComponent;
				if ( LinkedTo != UTV )
				{
					UnLink();
					LinkedTo = UTV;
					UTV.StartLinkedEffect();
				}
			}
			else
			{
				// not a linkable pawn type, break any links
				UnLink();
			}
		}
		else
		{
			// Enemy got in the way, break any links
			UnLink();
		}
	}
	else
	{
		UTO = UTOnslaughtObjective(Who);
		if ( UTO != none )
		{
			if ( (UTO.LinkHealMult > 0) && WorldInfo.Game.GameReplicationInfo.OnSameTeam(UTO,Instigator) )
			{
				LinkedTo = UTO;
				LinkedComponent = HitComponent;
			}
			else
			{
				UnLink();
			}
		}
	}

	if (LinkedTo != None)
	{
		// Determine if the link has been broken for another reason

		if (LinkedTo.bDeleteMe || (Pawn(LinkedTo) != None && Pawn(LinkedTo).Health <= 0))
		{
			UnLink();
			return;
		}

		// if we were passed in LinkedTo, we know we hit it straight on already, so skip the rest
		if (LinkedTo != Who)
		{
			StartTrace = Instigator.GetWeaponStartTraceLocation();
			EndTrace = GetLinkedtoLocation();

			// First, check to see if we have skewed too much, or if the LinkedTo pawn has died and
			// we didn't get cleaned up.
			V = Normal(EndTrace - StartTrace);
			if ( V dot vector(Instigator.GetViewRotation()) < LinkFlexibility || VSize(EndTrace - StartTrace) > 1.5 * WeaponRange )
			{
				UnLink();
				return;
			}

			//  If something is blocking us and the actor, drop the link
			HitActor = Trace(HitLocation, HitNormal, EndTrace, StartTrace, true);
			if (HitActor != none && HitActor != LinkedTo)
			{
				UnLink(true);		// In this case, use a delayed UnLink
			}
		}
	}

	// if we are linked, make sure the proper flash location is set
	if (LinkedTo != None)
	{
		SetFlashLocation(GetLinkedtoLocation());
	}
}

/**
 * Unlink this weapon from it's parent.  If bDelayed is true, it will give a
 * short delay before unlinking to allow the player to re-establish the link
 */
function UnLink(optional bool bDelayed)
{
	local UTVehicle V;

	if (!bDelayed)
	{
		V = UTVehicle(LinkedTo);
		if(V != none)
		{
			V.StopLinkedEffect();
		}

		LinkedTo = None;
		LinkedComponent = None;
	}
	else if (ReaccquireTimer <= 0)
	{
		// Set the Delay timer
		ReaccquireTimer = LinkBreakDelay;
	}
}
/* ********************************************************** */

simulated function CustomFire()
{
	local vector DepLoc;
	local rotator DepRot;

	if ( Role == ROLE_Authority &&
			MyVehicle.Mesh.GetSocketWorldLocationAndRotation('DeployableDrop',DepLoc,DepRot) )
	{
		if(UTVehicle_Deployable(MyVehicle) == none || UTVehicle_Deployable(MyVehicle).IsDeployed() )
		{
			DeployItem();
		}
		else
		{
			if (Role == ROLE_Authority)
			{
				UTVehicle_Deployable(MyVehicle).ServerToggleDeploy();
			}
			//SetTimer(UTVehicle_Deployable(MyVehicle).DeployTime,false,'DeployItem');
			//MyVehicle.ReceiveLocalizedMessage(class'UTSPMAMessage',2); // works
		}
	}
	//EndFire(0);
}

simulated function bool SelectWeapon(int WeaponNumber)
{
	if(WeaponNumber < NUMDEPLOYABLETYPES)
	{
		if ( Counts[WeaponNumber] <= 0 )
		{
			return false;
		}
		bShowDeployableName = true;
		DeployableIndex = WeaponNumber;
		if ( Instigator.IsLocallyControlled() )
		{
			WeaponPlaySound(AltFireModeChangeSound);
		}
		return true;
	}
	return false;
}

/**
  * Returns index of next available deployable
  */
simulated function int NextAvailableDeployableIndex(int Direction)
{
	local int ResultIndex, TestCount;

	ResultIndex = DeployableIndex + Direction;
	while ( (ResultIndex != DeployableIndex) && (TestCount < NUMDEPLOYABLETYPES) )
	{
		if ( ResultIndex >= NUMDEPLOYABLETYPES )
		{
			ResultIndex = 0;
		}
		else if ( ResultIndex < 0 )
		{
			ResultIndex = NUMDEPLOYABLETYPES - 1;
		}
		if ( Counts[ResultIndex] > 0 )
		{
			return ResultIndex;
		}
		ResultIndex += Direction;
		TestCount++;
	}

	return DeployableIndex;
}

/*********************************************************************************************
 * State WeaponFiring
 * See UTWeapon.WeaponFiring
 *********************************************************************************************/

simulated state WeaponBeamFiring
{
	/**
	 * In this weapon, RefireCheckTimer consumes ammo and deals out health/damage.  It's not
	 * concerned with the effects.  They are handled in the tick()
	 */
	simulated function RefireCheckTimer()
	{
		// If weapon should keep on firing, then do not leave state and fire again.
		if( ShouldRefire() )
		{
			return;
		}

		// Otherwise we're done firing, so go back to active state.
		GotoState('Active');
	}

	/**
	 * Update the beam and handle the effects
	 */
	simulated function Tick(float DeltaTime)
	{
		// If we are in danger of losing the link, check to see if
		// time has run out.
		if ( ReaccquireTimer > 0 )
		{
	    		ReaccquireTimer -= DeltaTime;
	    		if (ReaccquireTimer <= 0)
	    		{
		    		ReaccquireTimer = 0.0;
		    		UnLink();
		    	}
		}

		// Retrace everything and see if there is a new LinkedTo or if something has changed.
		UpdateBeam(DeltaTime);
	}

	simulated function int GetAdjustedFireMode()
	{
		// on the pawn, set a value of 2 if we're linked so the weapon attachment knows the difference
		if (LinkedTo != None)
		{
			return 2;
		}
		else
		{
			return CurrentFireMode;
		}
	}

	simulated function SetCurrentFireMode(byte FiringModeNum)
	{
		CurrentFireMode = FiringModeNum;
		if (Instigator != None)
		{
			Instigator.SetFiringMode(GetAdjustedFireMode());
		}
	}

	function SetFlashLocation(vector HitLocation)
	{
		local byte RealFireMode;

		RealFireMode = CurrentFireMode;
		CurrentFireMode = GetAdjustedFireMode();
		Global.SetFlashLocation(HitLocation);
		CurrentFireMode = RealFireMode;
	}

	simulated function BeginState( Name PreviousStateName )
	{
		// Fire the first shot right away
		RefireCheckTimer();
		TimeWeaponFiring( CurrentFireMode );
	}

	/**
	 * When leaving the state, shut everything down
	 */

	simulated function EndState(Name NextStateName)
	{
		ClearTimer('RefireCheckTimer');
		ClearFlashLocation();

		ReaccquireTimer = 0.0;
		UnLink();
		Victim = None;

		super.EndState(NextStateName);
	}

	simulated function bool IsFiring()
	{
		return true;
	}

	/**
	 * When done firing, we have to make sure we unlink the weapon.
	 */
	simulated function EndFire(byte FireModeNum)
	{
		UnLink();
		Global.EndFire(FireModeNum);
	}


}
simulated function float GetFireInterval( byte FireModeNum )
{
	if (FireModeNum==1)
	{
		return 0.75;
	}
	else
	{
		return Super.GetFireInterval(FireModeNum);
	}
}

function ChangeDeployableCount(int InDeployableIndex, int modification)
{
	Counts[InDeployableIndex] = Min(Counts[InDeployableIndex]+Modification, DeployableList[InDeployableIndex].MaxCnt);
}

simulated function byte GetDeployableCount(int InDeployableIndex)
{
	return Counts[InDeployableIndex];
}

simulated function string GetHumanReadableName()
{
	if ( bShowDeployableName )
		return DeployableList[DeployableIndex].DeployableClass.Default.ItemName;
	else
		return ItemName;
}

simulated function DeployItem()
{
	local vector DepLoc, ActualDropLoc;
	local rotator DepRot;
	local class<Actor> SpawnClass;
	local actor Deployable;

	bShowDeployableName = true;
	MyVehicle.Mesh.GetSocketWorldLocationAndRotation('DeployableDrop',DepLoc,DepRot);
	ActualDropLoc = DepLoc + ( DeployableList[DeployableIndex].DropOffset >> DepRot );
	SpawnClass = DeployableList[DeployableIndex].DeployableClass.static.GetTeamDeployable(MyVehicle.GetTeamNum());
	Deployable = Spawn( SpawnClass, instigator,, ActualDropLoc, DepRot );
	if ( Deployable != none )
	{
		WeaponPlaySound(DeployedItemSound);
		ChangeDeployableCount(DeployableIndex,-1);
		if ( UTSlowVolume(Deployable) != none )
		{
			UTSlowVolume(Deployable).OnDeployableUsedUp = DeployableUsedUp;
		}
		else if ( UTDeployedActor(Deployable) != none )
		{
			UTDeployedActor(Deployable).OnDeployableUsedUp = DeployableUsedUp;
		}

		DeployableList[DeployableIndex].Queue[DeployableList[DeployableIndex].Queue.Length] = Deployable;
	}
	if ( Counts[DeployableIndex] == 0 )
	{
		DeployableIndex = NextAvailableDeployableIndex(1);
	}
	if (UTVehicle_Deployable(MyVehicle) != none)
	{
		if(UTStealthVehicle(MyVehicle) != none)
		{
			UTStealthVehicle(MyVehicle).DeployPreviewMesh.SetHidden(true);
		}
		UTVehicle_Deployable(MyVehicle).ServerToggleDeploy(); // undeploy!
	}
}

function DeployableUsedUp(Actor ChildDeployable)
{
	local int DepIndex,i;

	for ( DepIndex=0; DepIndex<NUMDEPLOYABLETYPES; DepIndex++ )
	{
		for (i=0;i<DeployableList[DepIndex].Queue.Length;i++)
		{
			if ( DeployableList[DepIndex].Queue[i] == ChildDeployable )
			{
				DeployableList[DepIndex].Queue.Remove(i,1);
				ChangeDeployableCount(DepIndex,+1);
				return;
			}
		}
	}
}

simulated function bool ShowCrosshair()
{
	if ( UTVehicle_Deployable(MyVehicle) != none )
	{
		return UTVehicle_Deployable(MyVehicle).DeployedState == EDS_Undeployed;
	}

	return true;
}
simulated function DrawWeaponCrosshair( Hud HUD )
{
	if(ShowCrosshair())
	{
		super.DrawWeaponCrosshair(HUD);
	}
}

simulated event Destroyed()
{
	local int DepIndex, i;
	local float NewLifeSpan;

	if (Role == ROLE_Authority)
	{
		// destroy deployables around when vehicle would respawn so that they don't accumulate
		// if the vehicle doesn't last as long as the deployables it drops
		NewLifeSpan = (MyVehicle != None) ? FMax(1.0, MyVehicle.RespawnTime) : 1.0;
		for (DepIndex = 0; DepIndex < NUMDEPLOYABLETYPES; DepIndex++)
		{
			for (i = 0; i < DeployableList[DepIndex].Queue.Length; i++)
			{
				if (DeployableList[DepIndex].Queue[i] != None)
				{
					DeployableList[DepIndex].Queue[i].LifeSpan = NewLifeSpan;
				}
			}
		}
	}

	Super.Destroyed();
}

defaultproperties
{
}
