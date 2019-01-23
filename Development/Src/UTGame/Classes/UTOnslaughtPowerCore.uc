/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtPowerCore extends UTOnslaughtPanelNode
	native(Onslaught)
	nativereplication
	abstract;

var		bool	bWasVulnerable; // for determining transitions
var()	bool	bReverseForThisCore;		// if true, vehicle factories reverse vehicle spawns when controlled by this core's team
var()	bool	bNoCoreSwitch;		// If true, don't switch cores between rounds
/** if set, the team that controls this core is the Necris team (can affect what vehicle factories will activate for this team) */
var()	bool	bNecrisCore;

var int ProcessedTarydium;

var SkeletalMeshComponent BaseMesh;

var MaterialInstanceConstant BaseMaterialInstance;
var LinearColor BaseMaterialColors[2];

/** effect for the inner sphere */
var ParticleSystemComponent InnerCoreEffect;
var ParticleSystem InnerCoreEffectTemplates[2];

/** sparking energy effects visible when the core is vulnerable */
var struct native EnergyEffectInfo
{
	/** reference to the effect component */
	var ParticleSystemComponent Effect;
	/** current bones used for the effect endpoints */
	var name CurrentEndPointBones[2];
} EnergyEffects[6];

/** if the energy effect beam length is greater than this, a new endpoint bone is selected */
var float MaxEnergyEffectDist;

/** parameter names for the endpoints */
var name EnergyEndPointParameterNames[2];

/** energy effect endpoint will only be attached to bones with this prefix */
var string EnergyEndPointBonePrefix;

/** team colored energy effect templates */
var ParticleSystem EnergyEffectTemplates[2];

/** destruction effect */
var ParticleSystem DestructionEffectTemplates[2];

/** physics asset to use for BaseMesh while destroyed */
var PhysicsAsset DestroyedPhysicsAsset;

/** shield effects when not attackable */
var ParticleSystemComponent ShieldEffects[3];
var ParticleSystem ShieldEffectTemplates[2];

/** team colors for light emitted by powercore */
var Color EnergyLightColors[2];

/** dynamic powercore light */
var() PointLightComponent EnergyEffectLight;

/** the sound to play when the core becomes shielded */
var SoundCue ShieldOnSound;
/** the sound to play when the core loses its shield */
var SoundCue ShieldOffSound;
/** ambient sound when shield is up */
var SoundCue ShieldedAmbientSound;
/** ambient sound when shield is down */
var SoundCue UnshieldedAmbientSound;
/** Sound that plays to all players on team as core takes damage */
var SoundCue DamageWarningSound;

/** cached of health for warning effects*/
var float OldHealth;

/** last time warning played*/
var float LastDamageWarningTime;

/** Last time took damage */
var float LastDamagedTime;

var localized String NamePrefix;

var class<UTLocalMessage> ONSAnnouncerMessagesClass;
var class<UTLocalMessage> ONSOrbMessagesClass;

/** SkeletalMeshActor spawned to give Kismet when there are events controlling destruction effects */
var SkeletalMeshActorSpawnable KismetMeshActor;

/** Team specific message classes */
var class<UTLocalMessage> CoreMessageClass, RedMessageClass, BlueMessageClass;

cpptext
{
	virtual INT* GetOptimizedRepList(BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel);
	virtual void TickSpecial(FLOAT DeltaTime);
}

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		ProcessedTarydium;
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'DefenderTeamIndex')
	{
		SetTeamEffects();
	}
	else if (VarName == 'Health')
	{
		CheckForDamageAlarm();
	}
	super.ReplicatedEvent(VarName);
}

simulated function PostBeginPlay()
{
	local int i;

	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		for (i = 0; i < ArrayCount(EnergyEffects); i++)
		{
			EnergyEffects[i].Effect = new(Outer) class'UTParticleSystemComponent';
			EnergyEffects[i].Effect.bAutoActivate = false;
			BaseMesh.AttachComponentToSocket(EnergyEffects[i].Effect, name("EnergyEffect" $ (i + 1)));
		}
		for (i = 0; i < ArrayCount(ShieldEffects); i++)
		{
			ShieldEffects[i] = new(Outer) class'UTParticleSystemComponent';
			ShieldEffects[i].bAutoActivate = false;
			BaseMesh.AttachComponentToSocket(ShieldEffects[i], name("EnergyEffect" $ (i + 1)));
		}
		BaseMaterialInstance = BaseMesh.CreateAndSetMaterialInstanceConstant(0);
	}

	PanelSphereOffset = InnerCoreEffect.GetPosition() - Location;
}

simulated function HighlightOnMinimap(int Switch)
{
	if ( HighlightScale < 1.25 )
	{
		HighlightScale = (Switch == 0) ? 2.0 : MaxHighlightScale;
		LastHighlightUpdate = WorldInfo.TimeSeconds;
	}
}

simulated function string GetHumanReadableName()
{
	return NamePrefix@class'UTTeamInfo'.Default.TeamColorNames[DefenderTeamIndex]@ObjectiveName;
}

simulated function UpdateEffects(bool bPropagate)
{
	local bool bIsVulnerable;
	local PlayerController PC;
	local AnimNodeSequence MainSeq;
	local int i;

	if (WorldInfo.NetMode == NM_DedicatedServer)
		return;

	Super.UpdateEffects(bPropagate);

	bIsVulnerable = PoweredBy(1 - DefenderTeamIndex);

	if ( bIsVulnerable && !bWasVulnerable )
	{
		ForEach LocalPlayerControllers(class'PlayerController', PC)
		{
   			PC.ReceiveLocalizedMessage(CoreMessageClass, 3);
   		}
   		PlaySound(ShieldOffSound, true);
	}
	else if (!bIsVulnerable && bWasVulnerable)
	{
		PlaySound(ShieldOnSound, true);
	}

	bWasVulnerable = bIsVulnerable;

	if (bScriptInitialized)
	{
		SetAmbientSound(bIsVulnerable ? UnshieldedAmbientSound : ShieldedAmbientSound);
		MainSeq = AnimNodeSequence(BaseMesh.FindAnimNode('CorePlayer'));
		if (bIsVulnerable)
		{
			if(MainSeq != none)
			{
				MainSeq.SetAnim('Retract');
				MainSeq.PlayAnim();
			}
			for (i = 0; i < ArrayCount(EnergyEffects); i++)
			{
				EnergyEffects[i].Effect.ActivateSystem();
				EnergyEffects[i].Effect.SetHidden(false);
			}
			for (i = 0; i < ArrayCount(ShieldEffects); i++)
			{
				ShieldEffects[i].DeactivateSystem();
				ShieldEffects[i].SetHidden(true);
			}
		}
		else
		{
			if(MainSeq != none)
			{
				MainSeq.SetAnim('Extend');
				MainSeq.PlayAnim();
			}
			for (i = 0; i < ArrayCount(EnergyEffects); i++)
			{
				EnergyEffects[i].Effect.DeactivateSystem();
				EnergyEffects[i].Effect.SetHidden(true);
			}
			for (i = 0; i < ArrayCount(ShieldEffects); i++)
			{
				ShieldEffects[i].ActivateSystem();
				ShieldEffects[i].SetHidden(false);
			}
		}
	}
}

function InitializeForThisRound(int CoreIndex)
{
	local int Hops;

	// Set the distance in hops from every PowerNode to this powercore
	Hops = 0;
	SetCoreDistance(CoreIndex, Hops);

	// set flag team appropriately
	if (FlagBase != None && FlagBase.myFlag != None)
	{
		FlagBase.myFlag.SetTeam(GetTeamNum());
	}
}

simulated event SetInitialState()
{
	bScriptInitialized = true;

	if ( Role < ROLE_Authority )
		return;

	if (LinkedNodes[0] == None)
	{
		InitialState = 'DisabledNode';
	}
	else
	{
		InitialState = 'ActiveNode';
	}
	GotoState(InitialState,, true);
}

simulated function vector GetHUDOffset(PlayerController PC, Canvas Canvas)
{
	local float Z;

	Z = 420;
	if ( PC.ViewTarget != None )
	{
		Z += 0.02 * VSize(PC.ViewTarget.Location - Location);
	}
	return Z*vect(0,0,1);
}

function bool ValidSpawnPointFor(byte TeamIndex)
{
	return ( TeamIndex == DefenderTeamIndex );
}

function bool KillEnemyFirst(UTBot B)
{
	return false;
}

/** HealDamage()
PowerCores cannot be healed
*/
function bool HealDamage(int Amount, Controller Healer, class<DamageType> DamageType)
{
	if ( PlayerController(Healer) != None )
		PlayerController(Healer).ReceiveLocalizedMessage(MessageClass, 30);
	return false;
}

simulated event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local AnimNodeBlend crossfade;

	if(WorldInfo.NetMode != NM_DedicatedServer)
	{
		CheckForDamageAlarm();
	}
	super.TakeDamage(Damage,InstigatedBy,HitLocation,Momentum,DamageType,HitInfo,DamageCauser);
	if(Health/DamageCapacity < 0.5f )
	{
		crossfade = AnimNodeBlend(BaseMesh.FindAnimNode('DamageCrossfade'));
		if(crossfade != none)
		{
			crossfade.setblendtarget(1-(Health/DamageCapacity*2.0f), 0.001);
		}
	}
}

simulated function CheckForDamageAlarm()
{
	local int ScaledDamage;
	local float PercentageToNextAlarm;
	local PlayerController P;

	LastDamagedTime = WorldInfo.TimeSeconds;
	if (WorldInfo.NetMode != NM_DedicatedServer && LastDamageWarningTime + 5.0 < WorldInfo.TimeSeconds) // since we're doing this just for player information:
	{
		ScaledDamage = OldHealth-Health;
		OldHealth = Health;
		LastDamageWarningTime = WorldInfo.TimeSeconds;
		PercentageToNextAlarm = (Health / DamageCapacity) % 0.07;
		if (PercentageToNextAlarm == 0.0f) // we're right at a 7%, so need to cover a full 7% more to sound off
		{
			PercentageToNextAlarm = 0.07f;
		}
		if (float(ScaledDamage) / DamageCapacity > PercentageToNextAlarm)
		{
			foreach LocalPlayerControllers(class'PlayerController', P)
			{
				if (P.GetTeamNum() == DefenderTeamIndex)
				{
					P.ClientPlaySound(DamageWarningSound);
				}
			}
		}
	}

}
function DisableObjective(Controller InstigatedBy)
{
	local PlayerReplicationInfo PRI;

	if (InstigatedBy != None)
	{
		PRI = InstigatedBy.PlayerReplicationInfo;
	}

	BroadcastLocalizedMessage(CoreMessageClass, 1, PRI,, self);

	ShareScore(Score, DestroyedEvent[IsActive() ? int(DefenderTeamIndex) : (2 + DefenderTeamIndex)]);

	GotoState('ObjectiveDestroyed');
}

/** sets team specific effects depending on DefenderTeamIndex */
simulated function SetTeamEffects()
{
	local int i;

	CoreMessageClass = (DefenderTeamIndex == 0) ? RedMessageClass : BlueMessageClass;
	if (WorldInfo.NetMode != NM_DedicatedServer && DefenderTeamIndex < 2)
	{
		InnerCoreEffect.SetTemplate(InnerCoreEffectTemplates[DefenderTeamIndex]);
		for (i = 0; i < ArrayCount(EnergyEffects); i++)
		{
			EnergyEffects[i].Effect.SetTemplate(EnergyEffectTemplates[DefenderTeamIndex]);
		}
		for (i = 0; i < ArrayCount(ShieldEffects); i++)
		{
			ShieldEffects[i].SetTemplate(ShieldEffectTemplates[DefenderTeamIndex]);
		}
		BaseMaterialInstance.SetVectorParameterValue('PowerCoreColor', BaseMaterialColors[DefenderTeamIndex]);
		EnergyEffectLight.SetLightProperties(20, EnergyLightColors[DefenderTeamIndex]);
	}
}

simulated state ObjectiveDestroyed
{
	event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{}

	function Timer() {}

	function bool LegitimateTargetOf(UTBot B)
	{
		return false;
	}

	function bool TellBotHowToDisable(UTBot B)
	{
		if ( StandGuard(B) )
			return TooClose(B);

		return B.Squad.FindPathToObjective(B, self);
	}

	simulated function UpdateEffects(bool bPropagate)
	{
		local LinearColor MaterialColor;
		local int i;

		Super(UTOnslaughtNodeObjective).UpdateEffects(bPropagate);

		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			for (i = 0; i < ArrayCount(EnergyEffects); i++)
			{
				EnergyEffects[i].Effect.DeactivateSystem();
				EnergyEffects[i].Effect.SetHidden(true);
			}
			for (i = 0; i < ArrayCount(ShieldEffects); i++)
			{
				ShieldEffects[i].DeactivateSystem();
				ShieldEffects[i].SetHidden(true);
			}

			MaterialColor.A = 1.0;
			BaseMaterialInstance.SetVectorParameterValue('PowerCoreColor', MaterialColor);
		}
	}

	/** blows off all panels at once */
	simulated function BlowOffAllPanels()
	{
		local array<name> BoneNames;
		local UTPowerCorePanel Panel;
		local vector BoneLocation;
		local int i;

		if (PanelBoneScaler != None && PanelMesh != None)
		{
			PanelMesh.GetBoneNames(BoneNames);
			for (i = 0; i < BoneNames.length; i++)
			{
				if ( (i >= PanelBoneScaler.BoneScales.length || PanelBoneScaler.BoneScales[i] != DESTROYED_PANEL_BONE_SCALE) &&
					InStr(string(BoneNames[i]), PanelBonePrefix) == 0 )
				{
					BoneLocation = PanelMesh.GetBoneLocation(BoneNames[i]);

					PanelBoneScaler.BoneScales[i] = DESTROYED_PANEL_BONE_SCALE;

					WorldInfo.MyEmitterPool.SpawnEmitter(PanelExplosionTemplates[DefenderTeamIndex], BoneLocation);

					Panel = Spawn(PanelGibClass, self,, BoneLocation, rotator(BoneLocation - PanelMesh.GetPosition()));
					if (Panel != None)
					{
						Panel.Mesh.AddImpulse(Normal(BoneLocation - PanelMesh.GetPosition()) * 500.0);
					}
				}
			}
		}
	}

	simulated function BeginState(Name PreviousStateName)
	{
		local PlayerController PC;
		local array<SequenceEvent> DestructionEvents;
		local int i;

		if (Role == ROLE_Authority)
		{
			UTGame(WorldInfo.Game).ObjectiveDisabled(Self);
			UTGame(WorldInfo.Game).FindNewObjectives(Self);
		}
		SetAmbientSound(None);
		Health = 0;
		NodeState = GetStateName();

		BaseMesh.SetPhysicsAsset(DestroyedPhysicsAsset);
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			if (FindEventsOfClass(class'UTSeqEvent_PowerCoreDestructionEffect', DestructionEvents))
			{
				// play level-specific destruction
				BlowOffAllPanels();
				KismetMeshActor = Spawn(class'SkeletalMeshActorSpawnable');
				KismetMeshActor.DetachComponent(KismetMeshActor.SkeletalMeshComponent);
				KismetMeshActor.SetDrawScale(DrawScale);
				KismetMeshActor.SetDrawScale3D(DrawScale3D);
				KismetMeshActor.SkeletalMeshComponent = new(KismetMeshActor) BaseMesh.Class(BaseMesh);
				KismetMeshActor.AttachComponent(KismetMeshActor.SkeletalMeshComponent);
				SetHidden(true);
				for (i = 0; i < DestructionEvents.length; i++)
				{
					UTSeqEvent_PowerCoreDestructionEffect(DestructionEvents[i]).MeshActor = KismetMeshActor;
					DestructionEvents[i].CheckActivate(self, None);
				}
			}
			else
			{
				foreach LocalPlayerControllers(class'PlayerController', PC)
				{
					PC.ClientPlaySound(DestroyedSound);
				}

				if (DefenderTeamIndex < 2)
				{
					WorldInfo.MyEmitterPool.SpawnEmitter(DestructionEffectTemplates[DefenderTeamIndex], InnerCoreEffect.GetPosition());
				}
				SetTimer(5.0, false, 'BlowOffPanelTimer');
			}
		}

		UpdateLinks();
		UpdateEffects(true);

		if (Role == ROLE_Authority)
		{
			NetUpdateTime = WorldInfo.TimeSeconds - 1;
			Scorers.length = 0;
			UpdateCloseActors();
			UTOnslaughtGame(WorldInfo.Game).MainCoreDestroyed(DefenderTeamIndex);
		}
	}

	simulated event EndState(name NextStateName)
	{
		local AnimNodeBlend crossfade;

		crossfade = AnimNodeBlend(BaseMesh.FindAnimNode('DamageCrossfade'));
		if(crossfade != none)
		{
			crossfade.setblendtarget(0.0f, 0.001);
		}
		Super.EndState(NextStateName);

		SetHidden(false);
		if (KismetMeshActor != None)
		{
			KismetMeshActor.Destroy();
		}
	}
}

simulated state ActiveNode
{
	simulated function string GetNodeString(PlayerController PC)
	{
		if ( DefenderTeamIndex == 0 )
			return "RED Core";
		else
			return "BLUE Core";
	}

	simulated function BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		BaseMesh.SetPhysicsAsset(default.BaseMesh.PhysicsAsset);
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			SetTeamEffects();
			InnerCoreEffect.SetHidden(false);
		}
	}

	simulated function EndState(name NextStateName)
	{
		Super.EndState(NextStateName);

		InnerCoreEffect.SetHidden(true);
	}

	function bool HasActiveDefenseSystem()
	{
		return true;
	}
}

function BroadcastAttackNotification(Controller InstigatedBy)
{
	//attack notification
	NetUpdateTime = WorldInfo.TimeSeconds - 1;
	if (LastAttackMessageTime + 1 < WorldInfo.TimeSeconds)
	{
		if ( Health/DamageCapacity > 0.55 )
		{
			if ( InstigatedBy != None )
				BroadcastLocalizedMessage(CoreMessageClass, 0,,, self);
		}
		else if ( Health/DamageCapacity > 0.45 )
			BroadcastLocalizedMessage(CoreMessageClass, 4,,, self);
		else
			BroadcastLocalizedMessage(CoreMessageClass, 2,,, self);

		if ( InstigatedBy != None )
		{
			UTTeamInfo(WorldInfo.GRI.Teams[DefenderTeamIndex]).AI.CriticalObjectiveWarning(self, instigatedBy.Pawn);
		}
		LastAttackMessageTime = WorldInfo.TimeSeconds;
	}
	LastAttackTime = WorldInfo.TimeSeconds;
}



//		SkeletalMesh=SkeletalMesh'GP_Onslaught.S_Power_Core_Sphere'

DefaultProperties
{
	// all default properties are located in the _Content version for easier modification and single location
}
