/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtPowernode extends UTOnslaughtPanelNode
	native(Onslaught)
	abstract;

//@fixme FIXME: this shouldn't be transient,
//but they used to be StaticMeshComponents and the components got saved into maps due to the lighting channels stuff
//and so if they aren't transient the map copy will get used and break everything
var transient SkeletalMeshComponent EnergySphere;
var StaticMeshComponent NodeBase;
/** spinning piece rotated in C++ at the same rate as the EnergySphere (YawRotationRate) */
var StaticMeshComponent NodeBaseSpinner;
/** these have their 'PowerCoreColor' parameter set to the color of the team that owns this node */
var array<MaterialInstanceConstant> GlowMaterialInstances;
/** the colors to use for the materials */
var LinearColor TeamGlowColors[2];
var LinearColor NeutralGlowColor;

var CylinderComponent EnergySphereCollision;
var transient ParticleSystemComponent AmbientEffect;
var MaterialInstanceConstant NodeMaterialInstance;

var float YawRotationRate;
var float ConstructedCapacity;

/** effect used when the node is neutral */
var ParticleSystem NeutralEffectTemplate;
/** teamcolored effect used when the node is constructing */
var ParticleSystem ConstructingEffectTemplates[2];
/** teamcolored effect used when the node is fully constructed and active */
var ParticleSystem ActiveEffectTemplates[2];

/** teamcolored effect used when the node is fully constructed and shielded */
var ParticleSystem ShieldedActiveEffectTemplates[2];

/** System to play when captured by an orb*/
var ParticleSystemComponent OrbCaptureComponent;
/** templates for above*/
var ParticleSystem OrbCaptureTemplate[2];

/** teamcolored effect used when the node is shielded */
var ParticleSystem ShieldedEffectTemplates[2];
/** teamcolored effect used when the node is vulnerable */
var ParticleSystem VulnerableEffectTemplates[2];

/** teamcolored effect played when local player has the orb and this node cannot be captured */
var ParticleSystemComponent InvulnerableToOrbEffect;
var ParticleSystem InvulnerableToOrbTemplates[2];

/** true when damaged effect should be active */
var repnotify bool bPlayDamagedEffect;
var ParticleSystemComponent DamagedEffect;
var ParticleSystem DamagedEffectTemplates[2];

/** effect used when the node is destroyed */
var ParticleSystem DestroyedEffectTemplate;

/** How long does it take for a panel to reach the top of the node */
var float PanelTravelTime;

/** orb for the team that controls the node */
var repnotify UTOnslaughtFlag ControllingFlag;
/** whether or not the node can be captured or damaged */
var repnotify enum EVulnerabilityStatus
{
	VS_Vulnerable, // node is vulnerable
	VS_InvulnerableByNearbyOrb, // node is invulnerable because of nearby orb
	VS_InvulnerableToOrbCapture, // node is invulnerable (only to orb captures) because it was recently captured by the orb
} Vulnerability;
/** when the friendly orb carrier is this close and visible, this node cannot be captured by the enemy */
var float InvulnerableRadius;
/** emitter that links from this node to the flag when invulnerable */
var UTEmitter FlagLinkEffect;
/** when we are captured, enemy orbs within this radius are returned */
var float CaptureReturnRadius;
/** Sound to play when orb is nearby */
var AudioComponent OrbNearbySound;
/** offset for the FlagLinkEffect */
var float InvEffectZOffset;
/** when the node becomes invulnerable due to a nearby friendly orb, it is also healed this amount per second */
var int OrbHealingPerSecond;

/** special invulnerability to orb capture period granted when the node is first constructed */
var float OrbCaptureInvulnerabilityDuration;

/** last time node was captured by an Orb to keep track of invulnerability */
var float LastOrbCaptureTime;

/** Necris Capture Node Effects **/
var bool bPlayingNecrisEffects;
var StaticMeshComponent NecrisCapturePipesLarge;
var MaterialInstanceTimeVarying MITV_NecrisCapturePipesLarge;
var InterpCurveFloat MITV_NecrisCapturePipes_FadeIn;
var InterpCurveFloat MITV_NecrisCapturePipes_FadeIn2;

var InterpCurveFloat NecrisCapturePipes_FadeOut_Fast;
var InterpCurveFloat NecrisCapturePuddle_FadeIn50;
var InterpCurveFloat NecrisCapturePuddle_FadeIn100;
var InterpCurveFloat NecrisCapturePuddle_FadeOut;

var StaticMeshComponent NecrisCapturePipesSmall;
var MaterialInstanceTimeVarying MITV_NecrisCapturePipesSmall;

var UTParticleSystemComponent PSC_NecrisCapture;
var UTParticleSystemComponent PSC_NecrisGooPuddle;

var MaterialInstanceTimeVarying MITV_NecrisCaptureGoo;

/** Sockets this vehicle can be linked to from */
var array<Name> LinkToSockets;

replication
{
	if (bNetDirty)
		bPlayDamagedEffect, ControllingFlag, Vulnerability, LastOrbCaptureTime;
}


cpptext
{
	virtual void TickSpecial(FLOAT DeltaSeconds);
}


simulated native function vector GetTargetLocation(optional Actor RequestedBy);

simulated event PreBeginPlay()
{
	Super.PreBeginPlay();

	MITV_NecrisCapturePipesLarge = NecrisCapturePipesLarge.CreateAndSetMaterialInstanceTimeVarying(0);
	MITV_NecrisCapturePipesLarge.SetScalarCurveParameterValue( 'Nec_TubeFadeOut', NecrisCapturePipes_FadeOut_Fast );
	MITV_NecrisCapturePipesLarge.SetScalarStartTime( 'Nec_TubeFadeOut', 0.0f );

	MITV_NecrisCapturePipesSmall = NecrisCapturePipesSmall.CreateAndSetMaterialInstanceTimeVarying(0);
	MITV_NecrisCapturePipesSmall.SetScalarCurveParameterValue( 'Nec_TubeFadeOut', NecrisCapturePipes_FadeOut_Fast );
	MITV_NecrisCapturePipesSmall.SetScalarStartTime( 'Nec_TubeFadeOut', 0.0f );

	MITV_NecrisCaptureGoo = new(Outer) class'MaterialInstanceTimeVarying';
	MITV_NecrisCaptureGoo.SetParent( MaterialInstance'GP_Onslaught.Materials.M_GP_Ons_NecrisNode_GooAnimate' );
	PSC_NecrisGooPuddle.SetMaterialParameter( 'Nec_PuddleOpacity', MITV_NecrisCaptureGoo );
}

simulated event PostBeginPlay()
{
	local MaterialInstanceConstant NewMaterial;

	Super.PostBeginPlay();

	NewMaterial = NodeBase.CreateAndSetMaterialInstanceConstant(0);
	GlowMaterialInstances[GlowMaterialInstances.Length] = NewMaterial;

	NewMaterial = NodeBaseSpinner.CreateAndSetMaterialInstanceConstant(0);
	GlowMaterialInstances[GlowMaterialInstances.Length] = NewMaterial;
}

simulated event SetInitialState()
{
	local UTTeamGame Game;
	local UTOnslaughtFlagBase FBase;
	local byte StartingOwnerTeam;

	if (Role == ROLE_Authority && StartingOwnerCore != None)
	{
		StartingOwnerTeam = StartingOwnerCore.GetTeamNum();
		Game = UTTeamGame(WorldInfo.Game);
		if (Game != None && StartingOwnerTeam < ArrayCount(Game.Teams))
		{
			ControllingFlag = UTOnslaughtFlag(Game.Teams[StartingOwnerTeam].TeamFlag);
			if (ControllingFlag == None)
			{
				// the flag might not have been assigned yet if PostBeginPlay() hasn't been called for the flag base, so try to find it
				foreach WorldInfo.AllNavigationPoints(class'UTOnslaughtFlagBase', FBase)
				{
					if (FBase.myFlag != None && FBase.GetTeamNum() == StartingOwnerTeam)
					{
						ControllingFlag = FBase.myFlag;
						break;
					}
				}
				if (ControllingFlag == None)
				{
					`warn("No flag for team" @ StartingOwnerTeam);
					StartingOwnerTeam = 255;
				}
			}
		}
	}

	Super.SetInitialState();
}

simulated event ReplicatedEvent(name VarName)
{
	local rotator NewRot;
	if (VarName == 'Vulnerability' || VarName == 'ControllingFlag')
	{
		if (Vulnerability == VS_InvulnerableByNearbyOrb)
		{
			if (FlagLinkEffect == None || FlagLinkEffect.bDeleteMe)
			{
				FlagLinkEffect = Spawn(class'UTEmitter', self,, ControllingFlag.Location );
				if(ControllingFlag.GetTeamNum()==1)
				{
					FlagLinkEffect.SetTemplate(ParticleSystem'Pickups.PowerCell.Effects.P_LinkOrbAndNode_Blue', false);
				}
				else
				{
					FlagLinkEffect.SetTemplate(ParticleSystem'Pickups.PowerCell.Effects.P_LinkOrbAndNode_Red', false);
				}
				FlagLinkEffect.ParticleSystemComponent.SetAbsolute(false,true,false); // for rotation to be determined by us, not the orb.
				FlagLinkEffect.LifeSpan = 0.0;
				FlagLinkEffect.SetVectorParameter('LinkBeamEnd', Location);
			}
			FlagLinkEffect.SetBase(ControllingFlag);
			NewRot = rotator(Location - ControllingFlag.Location);
			NewRot.Yaw += 8192; // to the right somewhat.
			FlagLinkEffect.SetRotation(NewRot);
			// if we're not already playing, play
			if (OrbNearbySound != None && !OrbNearbySound.bWasPlaying)
			{
				OrbNearbySound.FadeIn(0.3, 1.0);
			}
		}
		else
		{
			if(Vulnerability == VS_InvulnerableToOrbCapture)
			{
				ShowOrbCaptureVisuals();
			}
			if (FlagLinkEffect != None)
			{
				FlagLinkEffect.Destroy();
			}
			// if we're playing; stop.
			if (OrbNearbySound.bWasPlaying)
			{
				OrbNearbySound.FadeOut(0.3, 0.0);
			}
		}

		UpdateShield(PoweredBy(1 - DefenderTeamIndex));
	}
	else if (VarName == 'bPlayDamagedEffect')
	{
		SetDamagedEffect(bPlayDamagedEffect);
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function ShowOrbCaptureVisuals()
{
	OrbCaptureComponent.DeactivateSystem();
	OrbCaptureComponent.SetTemplate(OrbCaptureTemplate[DefenderTeamIndex%2]);
	OrbCaptureComponent.ActivateSystem();
}
/** turns on and off the damaged effect */
simulated function SetDamagedEffect(bool bShowEffect)
{
	bPlayDamagedEffect = bShowEffect;
	if (bShowEffect)
	{
		if (DamagedEffect == None && DefenderTeamIndex < 2)
		{
			DamagedEffect = new(Outer) class'UTParticleSystemComponent';
			DamagedEffect.SetTemplate(DamagedEffectTemplates[DefenderTeamIndex]);
			AttachComponent(DamagedEffect);
		}
	}
	else if (DamagedEffect != None)
	{
		DetachComponent(DamagedEffect);
		DamagedEffect = None;
	}
}

function bool HealDamage(int Amount, Controller Healer, class<DamageType> DamageType)
{
	local bool bResult;

	bResult = Super.HealDamage(Amount, Healer, DamageType);
	if (HealEffect != None)
	{
		HealEffect.SetLocation(EnergySphere.GetPosition());
	}
	if (IsActive())
	{
		SetDamagedEffect(Health < DamageCapacity / 2);
	}
	return bResult;
}

simulated function TakeRadiusDamage( Controller InstigatedBy, float BaseDamage, float DamageRadius, class<DamageType> DamageType,
					float Momentum, vector HurtOrigin, bool bFullDamage, optional Actor DamageCauser )
{
	local vector DamageLocation, HitLocation, HitNormal;

	// the standard code doesn't work very well for HitLocations on our seperated geometry,
	// so for direct hits, attempt to find a HitLocation on the closer of the two
	if (bFullDamage)
	{
		DamageLocation = Location;
		if (VSize(EnergySphereCollision.GetPosition() - HurtOrigin) < VSize(NodeBase.GetPosition() - HurtOrigin))
		{
			if (TraceComponent(HitLocation, HitNormal, EnergySphereCollision, EnergySphereCollision.GetPosition(), HurtOrigin))
			{
				DamageLocation = HitLocation;

			}
		}
		else if (TraceComponent(HitLocation, HitNormal, NodeBase, NodeBase.GetPosition(), HurtOrigin))
		{
			DamageLocation = HitLocation;
		}

		TakeDamage(BaseDamage, InstigatedBy, DamageLocation, Normal(DamageLocation - HurtOrigin) * Momentum, DamageType,, DamageCauser);
	}
	else
	{
		Super.TakeRadiusDamage(InstigatedBy, BaseDamage, DamageRadius, DamageType, Momentum, HurtOrigin, bFullDamage, DamageCauser);
	}
}

simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local PlayerController PC;

	if ( Vulnerability == VS_InvulnerableByNearbyOrb )
	{
		if (Role == ROLE_Authority && !WorldInfo.GRI.OnSameTeam(self, EventInstigator))
		{
			PC = PlayerController(EventInstigator);
			if (PC != None)
			{
				PC.ReceiveLocalizedMessage(MessageClass, (ControllingFlag != None && ControllingFlag.Holder != None) ? 42 : 43);
				// play 'can't attack' sound if player keeps shooting at us
				ShieldDamageCounter += Damage;
				if (ShieldDamageCounter > 200)
				{
					PC.ClientPlaySound(ShieldHitSound);
					ShieldDamageCounter -= 200;
				}
			}
		}
	}
	else
	{
		Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
		if (Role == ROLE_Authority && IsActive())
		{
			SetDamagedEffect(Health < DamageCapacity / 2);
		}
	}
}

/** heals the node while the friendly orb is nearby */
function DoOrbHealing()
{
	HealDamage(OrbHealingPerSecond, (ControllingFlag.Holder != None) ? ControllingFlag.Holder.Controller : None, None);
}

simulated function bool VerifyOrbLock( UTOnslaughtFlag CheckedFlag )
{
	if ( VSize(Location - CheckedFlag.Location) < InvulnerableRadius
			&& FastTrace(Location + (vect(0,0,1) * InvEffectZOffset), CheckedFlag.Position().Location) )
	{
		CheckedFlag.LockedNode = self;
		return true;
	}
	CheckedFlag.LockedNode = None;
	return false;
}

/** checks for the presence of the friendly orb and sets us as invulnerable */
function CheckInvulnerability()
{
	local EVulnerabilityStatus NewVulnerability;

	NewVulnerability = VS_Vulnerable;

	if (ControllingFlag == None)
	{
		// have to find it - this happens e.g. when constructing since no orb was used to capture
		foreach DynamicActors(class'UTOnslaughtFlag', ControllingFlag)
		{
			if (WorldInfo.GRI.OnSameTeam(self, ControllingFlag))
			{
				break;
			}
		}
	}

	if ( ControllingFlag != None )
	{
		if ( ControllingFlag.Holder != None && VerifyOrbLock(ControllingFlag) )
		{
			NewVulnerability = VS_InvulnerableByNearbyOrb;
		}
		else if (WorldInfo.TimeSeconds - LastOrbCaptureTime < OrbCaptureInvulnerabilityDuration)
		{
			NewVulnerability = VS_InvulnerableToOrbCapture;
		}
	}
	if (Vulnerability != NewVulnerability)
	{
		Vulnerability = NewVulnerability;
		NetUpdateTime = WorldInfo.TimeSeconds - 1;

		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			ReplicatedEvent('Vulnerability');
		}

		if (Vulnerability == VS_Vulnerable)
		{
			// check if enemy orb carrier is already touching us
			CheckTouching();
		}
	}
}

simulated function UpdateShield(bool bPoweredByEnemy)
{
	local ParticleSystem NewTemplate;
	local PlayerController PC;
	local bool bPlayInvToOrbEffect;

	if (Vulnerability == VS_InvulnerableToOrbCapture)
	{
		foreach LocalPlayerControllers(class'PlayerController', PC)
		{
			if (PC.PlayerReplicationInfo.bHasFlag && !WorldInfo.GRI.OnSameTeam(PC, self))
			{
				bPlayInvToOrbEffect = true;
				break;
			}
		}
	}
	else if (Vulnerability == VS_InvulnerableByNearbyOrb)
	{
		bPoweredByEnemy = false;
	}

	if (DefenderTeamIndex < 2)
	{
		// update "you can't use the orb here right now" effect
		if (bPlayInvToOrbEffect)
		{
			if (InvulnerableToOrbEffect.Template != InvulnerableToOrbTemplates[DefenderTeamIndex])
			{
				InvulnerableToOrbEffect.SetTemplate(InvulnerableToOrbTemplates[DefenderTeamIndex]);
			}
			InvulnerableToOrbEffect.SetActive(true);
			InvulnerableToOrbEffect.SetHidden(false);
		}
		else
		{
			InvulnerableToOrbEffect.SetActive(false);
			InvulnerableToOrbEffect.SetHidden(true);
		}
		// update shield part
		NewTemplate = bPoweredByEnemy ? VulnerableEffectTemplates[DefenderTeamIndex] : ShieldedEffectTemplates[DefenderTeamIndex];
		ShieldedEffect.SetHidden(false);
		if (ShieldedEffect.Template != NewTemplate)
		{
			ShieldedEffect.SetTemplate(NewTemplate);
		}
		if (!ShieldedEffect.bIsActive)
		{
			ShieldedEffect.ActivateSystem();
		}

		// update the base part
		NewTemplate = bPoweredByEnemy ? ActiveEffectTemplates[DefenderTeamIndex] : ShieldedActiveEffectTemplates[DefenderTeamIndex];
		AmbientEffect.SetHidden(false);
		if (AmbientEffect.Template != NewTemplate)
		{
			AmbientEffect.SetTemplate(NewTemplate);
		}
		if (!AmbientEffect.bIsActive)
		{
			AmbientEffect.ActivateSystem();
		}
	}
}

simulated function bool IsKeyBeaconObjective(bool bFriendlyPowered, UTPlayerController PC)
{
	return  bFriendlyPowered && (self == PC.LastAutoObjective);
}

/** check if the given Pawn has an enemy orb and if so, takes over this node for that player's team */
function bool CheckFlag(Pawn P)
{
	local UTPlayerReplicationInfo UTPRI;

	if (P == None || P.IsA('Vehicle'))
	{
		return false;
	}

	UTPRI = UTPlayerReplicationInfo(P.PlayerReplicationInfo);
	if (UTPRI == None || !PoweredBy(UTPRI.Team.TeamIndex) || UTOnslaughtFlag(UTPRI.GetFlag()) == None)
	{
		return false;
	}

	// friendly team can only plant orb at constructing nodes, enemy team only at vulnerable nodes
	if ( WorldInfo.GRI.OnSameTeam(self, UTPRI) )
	{
		if ( !IsConstructing() )
			return false;
	}
	else
	{
		CheckInvulnerability();
		if (Vulnerability != VS_Vulnerable)
		{
			if ( !PoweredBy(P.GetTeamNum()))
			{
				P.ReceiveLocalizedMessage(MessageClass, 6);
			}
			else
			{
				P.ReceiveLocalizedMessage(MessageClass, (ControllingFlag != None && ControllingFlag.Holder != None) ? 42 : 43);
			}
			return false;
		}
	}

	if (IsActive())
	{
		GotoState('NeutralNode');
	}

	FindNewHomeForFlag(); // NOT UpdateCloseActors(), because we want vehicles to simply swap teams instead of being destroyed and respawned
	ControllingFlag = UTOnslaughtFlag(UTPRI.GetFlag());
	Vulnerability = VS_InvulnerableToOrbCapture;
	SetDamagedEffect(false); // remove lightning if the orb was previously nearly gone.
	NetUpdateTime = WorldInfo.TimeSeconds - 1;
	Constructor = P.Controller;
	DefenderTeamIndex = ControllingFlag.GetTeamNum();
	BecomeActive();
	ControllingFlag.Score();
	Health = DamageCapacity;
	ShowOrbCaptureVisuals();
	LastOrbCaptureTime = WorldInfo.TimeSeconds;
	return true;
}

function BecomeActive()
{
	Super.BecomeActive();

	LastOrbCaptureTime = WorldInfo.TimeSeconds;
}

function FindNewObjectives()
{
	// work off a delay to make sure the flag and all state changes have been completely resolved first
	SetTimer(0.05, false, 'DelayedFindNewObjectives');
}

function DelayedFindNewObjectives()
{
	if (!IsTimerActive('BecomeActive'))
	{
		Super.FindNewObjectives();
	}
}

function OnChangeNodeStatus(UTSeqAct_ChangeNodeStatus Action)
{
	local UTTeamGame Game;

	if (Action.InputLinks[0].bHasImpulse || Action.InputLinks[1].bHasImpulse)
	{
		Game = UTTeamGame(WorldInfo.Game);
		if (Game != None && Action.OwnerTeam < ArrayCount(Game.Teams))
		{
			ControllingFlag = UTOnslaughtFlag(Game.Teams[Action.OwnerTeam].TeamFlag);
			if (ControllingFlag != None)
			{
				Vulnerability = VS_InvulnerableToOrbCapture;
				NetUpdateTime = WorldInfo.TimeSeconds - 1;
				if (IsActive())
				{
					GotoState('ObjectiveDestroyed');
				}
				//@hack FIXME: Hack to work around only being able to activate Kismet events once per frame
				// (0.40 == max tick delta allowed by the engine)
				SetTimer(0.41, false, 'BecomeActive');
			}
		}
	}
}

function Reset()
{
	DefenderTeamIndex = default.DefenderTeamIndex;
	Vulnerability = VS_Vulnerable;
	SetDamagedEffect(false);
	Super.Reset();
}

/** applies any scaling factors to damage we're about to take */
simulated function ScaleDamage(out int Damage, Controller InstigatedBy, class<DamageType> DamageType)
{
	Super.ScaleDamage(Damage, InstigatedBy, DamageType);

	if ( WorldInfo.TimeSeconds - LastOrbCaptureTime < OrbCaptureInvulnerabilityDuration )
	{
		Damage *= 0.5;
	}
}

function Actor GetAutoObjectiveActor(UTPlayerController PC)
{
	local UTTeamGame Game;
	local byte PlayerTeam;
	local UTOnslaughtFlag Orb;
	local UTPlayerController Teammate;
	local int OrbOrderCount;
	local float OrbDist, PawnDist, PawnOrbDist;

	// see if this player needs to get the orb instead
	Game = UTTeamGame(WorldInfo.Game);
	PlayerTeam = PC.GetTeamNum();
	if ( Game != None && PlayerTeam < ArrayCount(Game.Teams) )
	{
		Orb = UTOnslaughtFlag(Game.Teams[PlayerTeam].TeamFlag);
		if ( (Orb != None) && (Orb.Holder == None) )
		{
			// decide whether to switch orders based on player's objective preference
			if ( PC.AutoObjectivePreference == AOP_OrbRunner )
			{
				return Orb;
			}

			// if orb is in dispenser, check if more than one player has already been told to grab it and is still nearby.
			if ( Orb.bHome )
			{
				ForEach WorldInfo.AllControllers(class'UTPlayerController', Teammate)
				{
					if ( (Teammate != PC) && (Teammate.LastAutoObjective == Orb) && (Teammate.Pawn != None)
						&& VSize(Orb.Location - Teammate.Pawn.Location) < Orb.StartingHomeBase.MaxSensorRange )
					{
						OrbOrderCount++;
					}
				}
				if ( OrbOrderCount < 2 )
				{
					// if not orb runner, only tell him to pick up if orb is near me, or he is near orb, or it is on his way.
					OrbDist = VSize(Orb.Location - Location);
					if ( OrbDist < MaxSensorRange )
					{
						return Orb;
					}
					PawnOrbDist = VSize(PC.Pawn.Location - Orb.Location);
					if ( PawnOrbDist < MaxSensorRange )
					{
						return Orb;
					}
					PawnDist = VSize(PC.Pawn.Location - Location);
					if ( PawnOrbDist + OrbDist < 1.5 * PawnDist )
					{
						return Orb;
					}
				}
			}
		}
	}
	return super.GetAutoObjectiveActor(PC);
}

simulated state NeutralNode
{
	ignores DoOrbHealing;

	simulated function UpdateEffects(bool bPropagate)
	{
		local PlayerController PC;
		local int i;

		if ( WorldInfo.NetMode == NM_DedicatedServer )
			return;
		Global.UpdateEffects(bPropagate);

		if (AmbientEffect.Template != NeutralEffectTemplate)
		{
			AmbientEffect.SetTemplate(NeutralEffectTemplate);
		}
		for (i = 0; i < GlowMaterialInstances.length; i++)
		{
			GlowMaterialInstances[i].SetVectorParameterValue('PowerCoreColor', NeutralGlowColor);
		}
		ForEach LocalPlayerControllers(class'PlayerController', PC)
		{
			if ( (PC.PlayerReplicationInfo == None) || (PC.PlayerReplicationInfo.Team == None) )
				return;

			if ( PoweredBy(PC.GetTeamNum()) )
			{
				AmbientEffect.ActivateSystem();
			}
			else
			{
				AmbientEffect.DeactivateSystem();
			}
		}
	}

	simulated function UpdateShield(bool bPoweredByEnemy)
	{
		InvulnerableToOrbEffect.SetActive(false);
		InvulnerableToOrbEffect.SetHidden(true);
		Super.UpdateShield(bPoweredByEnemy);
	}

	event Bump(Actor Other, PrimitiveComponent OtherComp, vector HitNormal)
	{
		local Pawn P;

		P = Pawn(Other);
		if (P != None && P.IsPlayerPawn() && Vehicle(P) == None && !CheckFlag(P))
		{
			if (PoweredBy(P.GetTeamNum()))
			{
					NetUpdateTime = WorldInfo.TimeSeconds - 1;
					DefenderTeamIndex = P.GetTeamNum();
					Constructor = P.Controller;
					GotoState('Constructing');
			}
			else
			{
				P.ReceiveLocalizedMessage(MessageClass, 6);
			}
		}
	}

	event Touch(Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal)
	{
		Bump(Other, OtherComp, HitNormal);
	}

	function CheckInvulnerability()
	{
		if (Vulnerability != VS_Vulnerable)
		{
			Vulnerability = VS_Vulnerable;
			NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
	}

	function OnChangeNodeStatus(UTSeqAct_ChangeNodeStatus Action)
	{
		if (Action.InputLinks[0].bHasImpulse)
		{
			DefenderTeamIndex = Action.OwnerTeam;
			GotoState('Constructing');
		}
		else if (Action.InputLinks[1].bHasImpulse)
		{
			DefenderTeamIndex = Action.OwnerTeam;
			Health = DamageCapacity;
			BecomeActive();
		}
	}

	function bool TellBotHowToDisable(UTBot B)
	{
		return Global.TellBotHowToDisable(B);
	}

	simulated event BeginState(Name PreviousStateName)
	{
		if ( EnergySphere != None )
		{
			EnergySphere.SetHidden(true);
			EnergySphereCollision.SetActorCollision(false, false);
		}
		if (Role == ROLE_Authority)
		{
			ControllingFlag = None;
			Vulnerability = VS_Vulnerable;
			if (WorldInfo.NetMode != NM_DedicatedServer)
			{
				ReplicatedEvent('Vulnerability');
			}
		}
		// note: Super call must be last, so that if in there the state is changed again, we won't clobber changes to the EnergySphere
		Super.BeginState(PreviousStateName);
	}
}

simulated state Constructing
{
	simulated function bool BeamEnabled()
	{
		return true;
	}

	simulated event bool IsConstructing()
	{
		return true;
	}

	function bool LegitimateTargetOf(UTBot B)
	{
		return (DefenderTeamIndex != B.Squad.Team.TeamIndex );
	}

	function bool ValidSpawnPointFor(byte TeamIndex)
	{
		return false;
	}

	simulated function bool HasHealthBar()
	{
		return true;
	}

	event Bump(Actor Other, PrimitiveComponent OtherComp, vector HitNormal)
	{
		CheckFlag(Pawn(Other));
	}

	event Timer()
	{
		local int HealthGain;

		if (bSevered)
		{
			return;
		}
		SetTimer(1.0, True);
	   	if (WorldInfo.TimeSeconds < LastAttackTime + 1.0)
	   	{
			return;
		}

		NetUpdateTime = WorldInfo.TimeSeconds - 1;

		ConstructionTimeElapsed += 1.0;
		HealthGain = (1.0 / ConstructionTime) * DamageCapacity;
		Health += HealthGain;

		// Recalculate taking in

		HealthGain = (1.0 / (ConstructionTime - PanelTravelTime) ) * DamageCapacity;
		HealPanels(HealthGain);

		if ( Health > ConstructedCapacity )
		{
			SetTimer(0.0, False);
			Health = ConstructedCapacity;
			BecomeActive();
		}
	}

	simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{
		local float OldHealth;

		OldHealth = Health;
		Global.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);
		if ( Health < OldHealth )
		{
			ConstructedCapacity = ConstructedCapacity - (OldHealth - Health);
		}
	}

	function OnChangeNodeStatus(UTSeqAct_ChangeNodeStatus Action)
	{
		if (Action.InputLinks[1].bHasImpulse)
		{
			DefenderTeamIndex = Action.OwnerTeam;
			Health = DamageCapacity;
			BecomeActive();
		}
		else if (Action.InputLinks[2].bHasImpulse)
		{
			DisableObjective(None);
		}
	}

	simulated function UpdateEffects(bool bPropagate)
	{
		local int i;

		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			Global.UpdateEffects(bPropagate);

			if (DefenderTeamIndex < 2)
			{
				if (AmbientEffect.Template != ConstructingEffectTemplates[DefenderTeamIndex])
				{
					AmbientEffect.SetRotation(EnergySphere.Rotation);
					AmbientEffect.SetTemplate(ConstructingEffectTemplates[DefenderTeamIndex]);
					if (AmbientEffect.bSuppressSpawning)
					{
						AmbientEffect.ActivateSystem();
					}
				}
				for (i = 0; i < GlowMaterialInstances.length; i++)
				{
					GlowMaterialInstances[i].SetVectorParameterValue('PowerCoreColor', TeamGlowColors[DefenderTeamIndex]);
				}
			}

			if (WorldInfo.GRI != None && UTGameReplicationInfo(WorldInfo.GRI).IsNecrisTeam(DefenderTeamIndex))
			{
				//`log( "Constructing necris" );
				PSC_NecrisGooPuddle.ActivateSystem();
				MITV_NecrisCaptureGoo.SetScalarCurveParameterValue( 'Nec_PuddleOpacity', NecrisCapturePuddle_FadeIn50 );
				MITV_NecrisCaptureGoo.SetScalarStartTime( 'Nec_PuddleOpacity', 0.0f );
				//PSC_NecrisGooPuddle.SetFloatParameter( 'Nec_PuddleOpacity', 0.0f );
				//MITV_NecrisCaptureGoo.SetScalarParameterValue( 'Nec_PuddleOpacity', 0.3f );

				bPlayingNecrisEffects = true;
			}
		}
	}

	simulated event BeginState(Name PreviousStateName)
	{
		local int i;
		local array<name> BoneNames;

		ConstructedCapacity = DamageCapacity;
		NodeState = GetStateName();
		Scorers.length = 0;
		PlaySound(StartConstructionSound, true);
		ConstructionTimeElapsed = 0.0;
		Health = 0.1 * DamageCapacity;

		SetAmbientSound(HealingSound);

		// Update Links
		UpdateLinks();
		UpdateEffects(true);

		SendChangedEvent(Constructor);

		if (Role == ROLE_Authority)
		{
			SetTimer(2.0,false);
			if (DefenderTeamIndex == 0)
			{
				BroadcastLocalizedMessage(MessageClass, 23,,,self);
			}
			else if (DefenderTeamIndex == 1)
			{
				BroadcastLocalizedMessage(MessageClass, 24,,,self);
			}

			SetTimer(0.05, true, 'CheckInvulnerability');
		}

		// remove all panels at the start of construction
		NumPanelsBlownOff = 0;
		PanelMesh.GetBoneNames(BoneNames);
		for (i = 0; i < BoneNames.length; i++)
		{
			if (InStr(string(BoneNames[i]), PanelBonePrefix) == 0)
			{
				if (PanelBoneScaler != None)
				{
					PanelBoneScaler.BoneScales[i] = DESTROYED_PANEL_BONE_SCALE;
				}
				NumPanelsBlownOff++;
			}
		}
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			// for some reason, ForceSkelUpdate() doesn't do the job here
			PanelMesh.LastRenderTime = WorldInfo.TimeSeconds;
			DetachComponent(PanelMesh);
			AttachComponent(PanelMesh);
		}
		ReplicatedNumPanelsBlownOff = NumPanelsBlownOff;

		if ( EnergySphere != None )
		{
			EnergySphere.SetHidden(false);
			EnergySphereCollision.SetActorCollision(true, true);
		}
	}

	simulated event EndState(name NextStateName)
	{
		Super.EndState(NextStateName);

		if (Role == ROLE_Authority)
		{
			ClearTimer();
			ClearTimer('DoOrbHealing');
			ClearTimer('CheckInvulnerability');
			Vulnerability = VS_Vulnerable;
		}
	}
}

simulated state ActiveNode
{
	event Bump(Actor Other, PrimitiveComponent OtherComp, vector HitNormal)
	{
		CheckFlag(Pawn(Other));
	}

	function TarydiumBoost(float Quantity)
	{
		Global.TarydiumBoost(Quantity);

		Health = FMin(Health+Quantity, DamageCapacity);
	}

	function CheckInvulnerability()
	{
		Global.CheckInvulnerability();

		if (Vulnerability == VS_InvulnerableByNearbyOrb && OrbHealingPerSecond > 0)
		{
			if (!IsTimerActive('DoOrbHealing'))
			{
				SetTimer(1.0, true, 'DoOrbHealing');
			}
		}
		else
		{
			ClearTimer('DoOrbHealing');
		}
	}

	function OnChangeNodeStatus(UTSeqAct_ChangeNodeStatus Action)
	{
		if (Action.InputLinks[2].bHasImpulse)
		{
			Health = 0;
			DisableObjective(None);
			NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
		}
		else
		{
			Global.OnChangeNodeStatus(Action);
		}
	}

	simulated function UpdateEffects(bool bPropagate)
	{
		local int i;

		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			Global.UpdateEffects(bPropagate);

			if (DefenderTeamIndex < 2)
			{
				if (AmbientEffect.bSuppressSpawning)
				{
					AmbientEffect.ActivateSystem();
				}
				for (i = 0; i < GlowMaterialInstances.length; i++)
				{
					GlowMaterialInstances[i].SetVectorParameterValue('PowerCoreColor', TeamGlowColors[DefenderTeamIndex]);
				}

				// set the effects for a node that has just been taken over by the necris
				if (WorldInfo.GRI != None && UTGameReplicationInfo(WorldInfo.GRI).IsNecrisTeam(DefenderTeamIndex))
				{
					//`log( "ActiveNode Necris" );

					MITV_NecrisCapturePipesLarge.SetScalarCurveParameterValue( 'Nec_BurnInTime', MITV_NecrisCapturePipes_FadeIn );
					MITV_NecrisCapturePipesLarge.SetScalarCurveParameterValue( 'Nec_TubeFadeOut', MITV_NecrisCapturePipes_FadeIn2 );

					MITV_NecrisCapturePipesLarge.SetScalarStartTime( 'Nec_BurnInTime', 0.0f );
					MITV_NecrisCapturePipesLarge.SetScalarStartTime( 'Nec_TubeFadeOut', 0.0f );

					MITV_NecrisCapturePipesSmall.SetScalarCurveParameterValue( 'Nec_BurnInTime', MITV_NecrisCapturePipes_FadeIn );
					MITV_NecrisCapturePipesSmall.SetScalarCurveParameterValue( 'Nec_TubeFadeOut', MITV_NecrisCapturePipes_FadeIn2 );

					MITV_NecrisCapturePipesSmall.SetScalarStartTime( 'Nec_BurnInTime', 0.0f );
					MITV_NecrisCapturePipesSmall.SetScalarStartTime( 'Nec_TubeFadeOut', 0.0f );

					MITV_NecrisCaptureGoo.SetScalarCurveParameterValue( 'Nec_PuddleOpacity', NecrisCapturePuddle_FadeIn100 );
					MITV_NecrisCaptureGoo.SetScalarStartTime( 'Nec_PuddleOpacity', 0.0f );

					//PSC_NecrisGooPuddle.ActivateSystem();
					//PSC_NecrisGooPuddle.SetScalarCurveParameterValue( 'Nec_PuddleOpacity', NecrisCapturePuddle_FadeIn );
					//PSC_NecrisGooPuddle.SetScalarStartTime( 'Nec_PuddleOpacity', 0.0f );

					//PSC_NecrisGooPuddle.ActivateSystem();
					//PSC_NecrisGooPuddle.SetFloatParameter( 'Nec_PuddleOpacity', 1 );

					PSC_NecrisCapture.ActivateSystem();

					bPlayingNecrisEffects = true;
				}
			}
		}
	}

	simulated function BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);
		if ( EnergySphere != None )
		{
			EnergySphere.SetHidden(false);
			EnergySphereCollision.SetActorCollision(true, true);
		}
		if (Role == ROLE_Authority)
		{
			SetTimer(0.05, true, 'CheckInvulnerability');
		}
	}

	simulated function EndState(Name NextStateName)
	{
		Super.EndState(NextStateName);
		if (Role == ROLE_Authority)
		{
			ClearTimer('DoOrbHealing');
			ClearTimer('CheckInvulnerability');
			Vulnerability = VS_Vulnerable;
		}
	}
}

simulated state ObjectiveDestroyed
{
	ignores DoOrbHealing;

	simulated function UpdateShield(bool bPoweredByEnemy)
	{
		InvulnerableToOrbEffect.SetActive(false);
		InvulnerableToOrbEffect.SetHidden(true);
		Super.UpdateShield(bPoweredByEnemy);
	}

	function CheckInvulnerability()
	{
		if (Vulnerability != VS_Vulnerable)
		{
			Vulnerability = VS_Vulnerable;
			NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
	}

	function OnChangeNodeStatus(UTSeqAct_ChangeNodeStatus Action)
	{
		if (Action.InputLinks[0].bHasImpulse)
		{
			DefenderTeamIndex = Action.OwnerTeam;
			GotoState('Constructing');
		}
		else if (Action.InputLinks[1].bHasImpulse)
		{
			DefenderTeamIndex = Action.OwnerTeam;
			Health = DamageCapacity;
			BecomeActive();
		}
	}

	simulated event BeginState(name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		SetDamagedEffect(false);

		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			WorldInfo.MyEmitterPool.SpawnEmitter(DestroyedEffectTemplate, Location);

			if (bPlayingNecrisEffects)
			{
				if (PreviousStateName == 'ActiveNode')
				{
					MITV_NecrisCapturePipesLarge.SetScalarCurveParameterValue( 'Nec_TubeFadeOut', NecrisCapturePuddle_FadeOut );
					MITV_NecrisCapturePipesLarge.SetScalarStartTime( 'Nec_TubeFadeOut', 0.0f );

					MITV_NecrisCapturePipesSmall.SetScalarCurveParameterValue( 'Nec_TubeFadeOut', NecrisCapturePuddle_FadeOut );
					MITV_NecrisCapturePipesSmall.SetScalarStartTime( 'Nec_TubeFadeOut', 0.0f );
				}

				//PSC_NecrisGooPuddle.DeactivateSystem();
				MITV_NecrisCaptureGoo.SetScalarCurveParameterValue( 'Nec_PuddleOpacity', NecrisCapturePuddle_FadeOut );
				MITV_NecrisCaptureGoo.SetScalarStartTime( 'Nec_PuddleOpacity', 0.0f );

				PSC_NecrisCapture.DeactivateSystem();

				bPlayingNecrisEffects = false;
			}
		}
	}
}

simulated state DisabledNode
{
	simulated event BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);
		if ( EnergySphere != None )
		{
			EnergySphere.SetHidden(true);
			EnergySphereCollision.SetActorCollision(false, false);
		}
		AmbientEffect.DeactivateSystem();
	}
}

simulated function bool TeamLink(int TeamNum)
{
	return ( (LinkHealMult > 0) && (DefenderTeamIndex == TeamNum) );
}

simulated function bool NeedsHealing()
{
	return (Health < DamageCapacity);
}

function bool TellBotHowToDisable(UTBot B)
{
	local UTOnslaughtSquadAI ONSSquadAI;

	if (StandGuard(B))
	{
		return TooClose(B);
	}
	else
	{
		ONSSquadAI = UTOnslaughtSquadAI(B.Squad);
		if (ONSSquadAI != None && ONSSquadAI.ONSTeamAI != None && !WorldInfo.GRI.OnSameTeam(self, B))
		{
			if ( ONSSquadAI.ONSTeamAI.Flag != None && ONSSquadAI.ONSTeamAI.Flag.Holder == None &&
				(B.RouteGoal == ONSSquadAI.ONSTeamAI.Flag.Position() || B.RouteGoal == ONSSquadAI.ONSTeamAI.Flag.LastAnchor || VSize(ONSSquadAI.ONSTeamAI.Flag.Position().Location - B.Pawn.Location) * 2.0 < VSize(Location - B.Pawn.Location)) &&
				(IsNeutral() || !B.CanAttack(self)) )
			{
				// go for orb
				B.GoalString = "Get orb";
				return ONSSquadAI.FindPathToObjective(B, ONSSquadAI.ONSTeamAI.Flag.Position());
			}
			if (Vulnerability != VS_Vulnerable)
			{
				if (ControllingFlag != None && ControllingFlag.Holder != None)
				{
					// if seen enemy orb carrier recently, or close enough to see connecting effect, try to attack that player directly
					if ( (B.Enemy == ControllingFlag.Holder || B.LineOfSightTo(ControllingFlag.Holder) || B.ActorReachable(self)) &&
						ONSSquadAI.TryToIntercept(B, ControllingFlag.Holder, self) )
					{
						B.GoalString = "Attack enemy orb carrier";
						return true;
					}
					// otherwise go towards node until we see the carrier
					return ONSSquadAI.FindPathToObjective(B, self);
				}
				else if (B.Enemy != None)
				{
					// temporary invulnerability, fight nearby enemies until node becomes vulnerable
					return false;
				}
			}

			if (!IsNeutral() && (ONSSquadAI.ONSTeamAI.Flag == None || ONSSquadAI.ONSTeamAI.Flag.HolderPRI != B.PlayerReplicationInfo))
			{
				// no orb or someone else has it, attack the node the old way
				return Super.TellBotHowToDisable(B);
			}
		}
		return B.Squad.FindPathToObjective(B, self);
	}
}

function bool TellBotHowToHeal(UTBot B)
{
	local vector AdjustedLoc;
	local UTVehicle BotVehicle;

	// if bot is in important vehicle, don't get out
	BotVehicle = UTVehicle(B.Pawn);
	if (BotVehicle != None && BotVehicle.ImportantVehicle())
	{
		return TooClose(B);
	}

	AdjustedLoc = EnergySphere.GetPosition();
	AdjustedLoc.Z = B.Pawn.Location.Z;
	if ( VSize(AdjustedLoc - B.Pawn.Location) < 50 )
	{
		// standing right on top of it, move away a little
		B.GoalString = "Move away from "$self;
		B.RouteGoal = B.FindRandomDest();
		B.MoveTarget = B.RouteCache[0];
		B.SetAttractionState();
		return true;
	}
	else if (VSize(EnergySphere.GetPosition() - B.Pawn.Location) > class'UTWeap_LinkGun'.default.WeaponRange)
	{
		// too far to heal
		B.GoalString = "Move closer to "$self;
		if ( B.FindBestPathToward(B.RouteGoal, false, true) )
		{
			B.SetAttractionState();
			return true;
		}
		else
			return false;
	}

	if (!TeamLink(B.GetTeamNum()) || Health >= DamageCapacity)
	{
		return false;
	}

	if (B.Squad.SquadObjective == None)
	{
		if (BotVehicle != None)
		{
			return false;
		}
		// @hack - if bot has no squadobjective, need this for SwitchToBestWeapon() so bot's weapons' GetAIRating()
		// has some way of figuring out bot is trying to heal me
		B.DoRangedAttackOn(self);
	}

	if (BotVehicle != None && !BotVehicle.bKeyVehicle && (B.Enemy == None || (!B.LineOfSightTo(B.Enemy) && WorldInfo.TimeSeconds - B.LastSeenTime > 3)))
	{
		B.LeaveVehicle(true);
		return true;
	}

	if (UTWeapon(B.Pawn.Weapon) != None && UTWeapon(B.Pawn.Weapon).CanHeal(self))
	{
		if (!B.Pawn.CanAttack(self))
		{
			// need to move to somewhere else near objective
			B.GoalString = "Can't shoot"@self@"(obstructed)";
			B.RouteGoal = B.FindRandomDest();
			B.MoveTarget = B.RouteCache[0];
			B.SetAttractionState();
			return true;
		}
		B.GoalString = "Heal "$self;
		B.DoRangedAttackOn(self);
		return true;
	}
	else
	{
		B.SwitchToBestWeapon();
		if (UTWeapon(B.Pawn.InvManager.PendingWeapon) != None && UTWeapon(B.Pawn.InvManager.PendingWeapon).CanHeal(self))
		{
			if (!B.Pawn.CanAttack(self))
			{
				// need to move to somewhere else near objective
				B.GoalString = "Can't shoot"@self@"(obstructed)";
				B.RouteGoal = B.FindRandomDest();
				B.MoveTarget = B.RouteCache[0];
				B.SetAttractionState();
				return true;
			}
			B.GoalString = "Heal "$self;
			B.DoRangedAttackOn(self);
			return true;
		}
		else
		{
			B.StopFiring();
			if (B.FindInventoryGoal(0.0005)) // try to find a weapon to heal the objective
			{
				B.GoalString = "Find weapon or ammo to heal "$self;
				B.SetAttractionState();
				return true;
			}
		}
	}

	return false;
}

simulated function vector GetHUDOffset(PlayerController PC, Canvas Canvas)
{
	local vector Offset;

	// if default offset is too high up, don't use it
	Offset = Super.GetHUDOffset(PC, Canvas);
	return (Canvas.Project(Location + Offset).Y <= 0.f) ? vect(0,0,120) : Offset;
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}

