/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTCTFBase extends UTGameObjective
	hidecategories(UTGameObjective)
	abstract;

/** audio component that should play when the flag has been taken */
var() AudioComponent TakenSound;

var		UTCTFFlag			myFlag;
var		class<UTCTFFlag>	FlagType;
var()	float				BaseExitTime;		// how long it takes to get entirely away from the base

/** Particles to play while the flag is gone */
var		ParticleSystemComponent	FlagEmptyParticles;


/**
 * The MICs for the flag base.  We need to store the MIC we are referencing and then the one we
 * spawn at runtime.
 **/
var MaterialInstanceConstant FlagBaseMaterial;
var MaterialInstanceConstant MIC_FlagBaseColor;

/** Reference to the actual mesh base so we can modify it with a different material **/
var StaticMeshComponent FlagBaseMesh;


var class<UTLocalMessage> CTFAnnouncerMessagesClass;


simulated function PostBeginPlay()
{
//	local UTDefensePoint W;

	Super.PostBeginPlay();

	if ( Role < ROLE_Authority )
		return;

	if ( UTCTFGame(WorldInfo.Game) != None )
	{
		myFlag = Spawn(FlagType, self);

		if (myFlag==None)
		{
			`warn(Self$" could not spawn flag of type '"$FlagType$"' at "$location);
			return;
		}
		else
		{
			UTCTFGame(WorldInfo.Game).RegisterFlag(myFlag, DefenderTeamIndex);
			myFlag.HomeBase = self;
		}
	}

	//calculate distance from this base to all nodes - store in BaseDist[DefenderTeamIndex] for each node
	/* //@todo steve
	SetBaseDistance(DefenderTeamIndex);

	// calculate visibility to base and defensepoints, for all paths
	SetBaseVisibility(DefenderTeamIndex);
	for ( W=DefensePoints; W!=None; W=W.NextDefensePoint )
		if ( W.myMarker != None )
			W.myMarker.SetBaseVisibility(DefenderTeamIndex);
	*/

	MIC_FlagBaseColor = new(Outer) class'MaterialInstanceConstant';
	MIC_FlagBaseColor.SetParent( FlagBaseMaterial );
	FlagBaseMesh.SetMaterial( 0, MIC_FlagBaseColor );
}


simulated function PrecacheAnnouncer(UTAnnouncer Announcer)
{
	Super.PrecacheAnnouncer(Announcer);

	if (myFlag != None)
	{
		myFlag.PrecacheAnnouncer(Announcer);
	}
}

event actor GetBestViewTarget()
{
	if (myFlag.Holder != none)
	{
		return myFlag.Holder;
	}
	else if (!myFlag.bHome)
	{
		return MyFlag;
	}
	else
	{
		return super.GetBestViewTarget();
	}

}

/**
 * Handle flag events.
 */
function ObjectiveChanged()
{
	local PlayerController PC;

	if (myFlag != None)
	{
		// Look to change the spectator
		foreach WorldInfo.AllControllers(class'PlayerController', PC)
		{
			if ( (PC.ViewTarget == self) || (PC.ViewTarget == myFlag) || ((PC.ViewTarget != None) && (PC.ViewTarget == myFlag.Holder)) )
				PC.SetViewTarget(GetBestViewTarget());
		}
	}
}

function UTCarriedObject GetFlag()
{
	return myFlag;
}

simulated function SetAlarm(bool bNowOn)
{
	bUnderAttack = bNowOn;
	if (WorldInfo.NetMode != NM_DedicatedServer && TakenSound != None)
	{
		if (bNowOn)
		{
			TakenSound.Play();
			if(FlagEmptyParticles != none)
			{
				FlagEmptyParticles.ActivateSystem();
			}
		}
		else
		{
			TakenSound.Stop();
			if(FlagEmptyParticles != none)
			{
				FlagEmptyParticles.DeActivateSystem();
			}
		}
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bUnderAttack')
	{
		SetAlarm(bUnderAttack);
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function name GetHudMapStatus()
{
	if ( IsUnderAttack() )
	{
		return 'Critical';
	}
	return Super.GetHudMapStatus();
}

simulated event bool IsActive()
{
	return true;
}


defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}



