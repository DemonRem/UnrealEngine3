/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/** superclass of nodes that have a sphere of panels that get blown off as they take damage */
class UTOnslaughtPanelNode extends UTOnslaughtNodeObjective
	native(Onslaught)
	nativereplication
	abstract;

/** bone scale destroyed panels are set to
 * we use a small non-zero value so that the transforms are still valid (for GetBoneLocation() and such)
 */
const DESTROYED_PANEL_BONE_SCALE = 0.01;

/** scaling control for the sphere panels */
var UTSkelControl_MassBoneScaling PanelBoneScaler;
/** explosion effect spawned when a panel gets blown off */
var ParticleSystem PanelExplosionTemplates[2];
/** a panel may be blown off every time this much health is lost */
var int PanelHealthMax;
/** how much health remaining before a panel is blown off (note: calculated clientside) */
var int PanelHealthRemaining;
/** number of panels blown off so far - used to update clients */
var int NumPanelsBlownOff;
var repnotify int ReplicatedNumPanelsBlownOff;
/** name prefix for panel bones so we know when we hit one */
var string PanelBonePrefix;
/** offset from Location for the center of the sphere of panels */
var vector PanelSphereOffset;
/** Component that has the panels on it */
var SkeletalMeshComponent PanelMesh;
/** teamcolored effect attached to the panel bone when a panel is restored */
var ParticleSystem PanelHealEffectTemplates[2];
/** bone names of panels currently playing heal effect */
var array<name> PanelsBeingHealed;
/** actor spawned at panel location when it is blown off */
var class<UTPowerCorePanel> PanelGibClass;

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		ReplicatedNumPanelsBlownOff;
}

cpptext
{
	virtual INT* GetOptimizedRepList(BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel);
}

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		PanelBoneScaler = UTSkelControl_MassBoneScaling(PanelMesh.FindSkelControl('BoneScaler'));
		if (PanelBoneScaler == None)
		{
			`Warn("Failed to find UTSkelControl_MassBoneScaling named BoneScaler in AnimTree" @ PanelMesh.Animations);
		}
	}

	PanelHealthRemaining = PanelHealthMax;
}

/** returns a random vector near the panels */
simulated function vector GetRandomPanelLocation()
{
	return Location + PanelSphereOffset + (VRand() * 100.0);
}

simulated event ReplicatedEvent(name VarName)
{
	local int i;
	local bool bSpawnExplosion;

	if (VarName == 'ReplicatedNumPanelsBlownOff')
	{
		if (ReplicatedNumPanelsBlownOff > NumPanelsBlownOff)
		{
			bSpawnExplosion = (LastRenderTime > WorldInfo.TimeSeconds - 3.0 && ReplicatedNumPanelsBlownOff - NumPanelsBlownOff <= 3);
			for (i = NumPanelsBlownOff; i < ReplicatedNumPanelsBlownOff; i++)
			{
				BlowOffPanel(GetRandomPanelLocation(), bSpawnExplosion);
			}
			NumPanelsBlownOff = ReplicatedNumPanelsBlownOff;
		}
		else
		{
			while (ReplicatedNumPanelsBlownOff < NumPanelsBlownOff)
			{
				RestoreRandomPanel();
				NumPanelsBlownOff--;
			}
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/** blows off the closest panel to the passed in HitLocation
 * @return whether or not a panel was blown off
 */
simulated function bool BlowOffPanel(vector HitLocation, bool bSpawnExplosion)
{
	local name BoneName;
	local vector BoneLocation;
	local UTPowerCorePanel Panel;

	// make sure mesh is up to date so FindClosestBone() returns accurate results
	PanelMesh.ForceSkelUpdate();

	BoneName = PanelMesh.FindClosestBone(HitLocation, BoneLocation, DESTROYED_PANEL_BONE_SCALE);
	if (BoneName != 'None' && InStr(string(BoneName), PanelBonePrefix) == 0 && PanelBoneScaler.GetBoneScale(BoneName) > DESTROYED_PANEL_BONE_SCALE)
	{
		// we set the scale to a very small non-zero value so that the transforms are still valid (for GetBoneLocation() and such)
		PanelBoneScaler.SetBoneScale(BoneName, DESTROYED_PANEL_BONE_SCALE);
		if (bSpawnExplosion && DefenderTeamIndex < 2 && EffectIsRelevant(Location, false))
		{
			WorldInfo.MyEmitterPool.SpawnEmitter(PanelExplosionTemplates[DefenderTeamIndex], BoneLocation);

			Panel = Spawn(PanelGibClass, self,, BoneLocation, rotator(BoneLocation - PanelMesh.GetPosition()));
			// panels can fail due to collision issues
			if( Panel != None )
			{
				Panel.Mesh.AddImpulse(Normal(HitLocation - BoneLocation) * 500.0, HitLocation);
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}

/** called when a panel heal effect finishes playing */
simulated function PanelHealEffectFinished(ParticleSystemComponent PSystem)
{
	PanelBoneScaler.SetBoneScale(PSystem.Owner.BaseBoneName, 1.0);
	PSystem.Owner.LifeSpan = 0.0001f;
	PanelsBeingHealed.Remove(PanelsBeingHealed.Find(PSystem.Owner.BaseBoneName), 1);
}

/** restores a random panel to its default scale */
simulated function RestoreRandomPanel()
{
	local int StartIndex, i;
	local UTEmitter Effect;
	local array<name> BoneNames;

	if (PanelBoneScaler != None)
	{
		PanelMesh.GetBoneNames(BoneNames);
		StartIndex = Rand(PanelBoneScaler.BoneScales.length);
		i = StartIndex;
		do
		{
			i++;
			if (i >= PanelBoneScaler.BoneScales.length)
			{
				i = 0;
			}
		} until (i == StartIndex || (PanelBoneScaler.BoneScales[i] == DESTROYED_PANEL_BONE_SCALE && PanelsBeingHealed.Find(BoneNames[i]) == INDEX_NONE));
		if (DefenderTeamIndex < 2 && PanelHealEffectTemplates[DefenderTeamIndex] != None && EffectIsRelevant(Location, false))
		{
			Effect = Spawn(class'UTEmitter', self,, PanelMesh.GetBoneLocation(BoneNames[i]), rotator(-PanelMesh.GetBoneAxis(BoneNames[i], AXIS_X)));
			Effect.SetTemplate(PanelHealEffectTemplates[DefenderTeamIndex]);
			Effect.SetBase(self,, PanelMesh, BoneNames[i]);
			Effect.ParticleSystemComponent.OnSystemFinished = PanelHealEffectFinished;
			PanelsBeingHealed[PanelsBeingHealed.length] = BoneNames[i];
		}
		else
		{
			PanelBoneScaler.BoneScales[i] = 1.0;
		}
	}
}

/** restores all of the panels to their default scale */
simulated function RestoreAllPanels()
{
	local int i;

	NumPanelsBlownOff = 0;
	if (WorldInfo.NetMode != NM_Client)
	{
		ReplicatedNumPanelsBlownOff = 0;
	}
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		// clear bone scales
		PanelBoneScaler.BoneScales.Length = 0;
		// re-scale to zero bones in the process of being healed
		for (i = 0; i < PanelsBeingHealed.length; i++)
		{
			PanelBoneScaler.SetBoneScale(PanelsBeingHealed[i], DESTROYED_PANEL_BONE_SCALE);
		}
	}
	PanelHealthRemaining = PanelHealthMax;
}

simulated function DamagePanels(int Damage, vector HitLocation)
{
	PanelHealthRemaining -= Damage;
	while (PanelHealthRemaining <= 0)
	{
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			if (!BlowOffPanel(HitLocation, true))
			{
				// if the HitLocation isn't near any panels, just choose one at random
				BlowOffPanel(GetRandomPanelLocation(), true);
			}
		}
		NumPanelsBlownOff++;
		if (WorldInfo.NetMode != NM_Client)
		{
			ReplicatedNumPanelsBlownOff++;
		}
		PanelHealthRemaining += PanelHealthMax;
	}
}

simulated event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local byte Team;

	Super.TakeDamage(Damage, InstigatedBy, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);

	if (InstigatedBy != None)
	{
		Team = InstigatedBy.GetTeamNum();
	}
	else if (Role == ROLE_Authority)
	{
		// force allow for no InstigatedBy, as Super.TakeDamage() does
		Team = 255;
	}
	else if (DamageCauser != None && DamageCauser.Instigator != None)
	{
		Team = DamageCauser.Instigator.GetTeamNum();
	}
	else
	{
		// assume can't attack
		return;
	}

	if (Damage > 0 && DefenderTeamIndex != Team && (Team == 255 ||PoweredBy(Team)))
	{
		ScaleDamage(Damage, InstigatedBy, DamageType);
		DamagePanels(Damage, HitLocation);
	}
}

function HealPanels(int Amount)
{
	// check if enough damage was healed to restore a panel
	PanelHealthRemaining += Amount;
	while (PanelHealthRemaining >= PanelHealthMax && NumPanelsBlownOff > 0)
	{
		NumPanelsBlownOff--;
		if (NumPanelsBlownOff == 0)
		{
			RestoreAllPanels();
		}
		else
		{
			if (WorldInfo.NetMode != NM_DedicatedServer)
			{
				RestoreRandomPanel();
			}
			if (WorldInfo.NetMode != NM_Client)
			{
				ReplicatedNumPanelsBlownOff--;
			}
			PanelHealthRemaining -= PanelHealthMax;
		}
	}
}

function bool HealDamage(int Amount, Controller Healer, class<DamageType> DamageType)
{
	local int OldHealth;

	OldHealth = Health;
	if (Super.HealDamage(Amount, Healer, DamageType))
	{
		HealPanels(Health - OldHealth);
		return true;
	}
	else
	{
		return false;
	}
}

simulated state ObjectiveDestroyed
{
	simulated function BlowOffPanelTimer()
	{
		local int i;

		for (i = 0; i < 3; i++)
		{
			BlowOffPanel(GetRandomPanelLocation(), true);
		}
		SetTimer(0.10, false, 'BlowOffPanelTimer');
	}

	simulated event BeginState(name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			BlowOffPanelTimer();
		}
	}

	simulated event EndState(name NextStateName)
	{
		Super.EndState(NextStateName);

		ClearTimer('BlowOffPanelTimer');
	}
}

simulated state ActiveNode
{
	simulated function BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		// don't reset the panels if we're entering this state on spawn, as doing so does nothing on the server
		// and on the client can screw up any previously replicated panel info
		if (PreviousStateName != '')
		{
			RestoreAllPanels();
		}
	}
}


defaultproperties
{
	PanelHealthMax=70
	PanelBonePrefix="Column"
}
