/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtNodeObjective extends UTOnslaughtObjective
	abstract
	native(Onslaught)
	hidecategories(ObjectiveHealth,Collision,Display);

var		float               ConstructionTime;
var		soundcue            DestroyedSound;
var		soundcue			ConstructedSound;
var		soundcue			StartConstructionSound;
var		soundcue            ActiveSound;
var		soundcue            NeutralSound;
var		soundcue            HealingSound;
var		soundcue            HealedSound;
var     repnotify bool      bSevered;			/**	true if active node is severed from its power network */
var		bool				bWasSevered;		/** utility - for determining transitions to being severed */

var InterpCurveFloat AttackEffectCurve;

const MAXNUMLINKS=8;
var() repnotify UTOnslaughtNodeObjective LinkedNodes[MAXNUMLINKS];
var repnotify byte NumLinks;
/** if set, this node can exist and be captured without being linked */
var() bool bStandalone;
/** spawn priority for standalone nodes - lower is better, 255 = autogenerate (FinalCoreDistance is set to this) */
var() byte StandaloneSpawnPriority<tooltip=Lower is better, 255 = autogenerate>;

var array<UTOnslaughtSpecialObjective> ActivatedObjectives;

// Internal
var repnotify name			NodeState;
var float           ConstructionTimeElapsed;
var float           SeveredDamagePerSecond;
var float           HealingTime;

var int ActivationMessageIndex, DestructionMessageIndex; //base switch for UTOnslaughtMessage
var string DestroyedEvent[4];
var string ConstructedEvent[2];
var Controller Constructor;			/** player who touched me to start construction */
var Controller LastHealedBy;

var int NodeNum;

var AudioComponent AmbientSoundComponent;

var LinearColor BeamColor[3];
var StaticMeshComponent NodeBeamEffect;
var protected MaterialInstanceTimeVarying BeamMaterialInstance;

/** material parameter for god beam under attack effect */
var name GodBeamAttackParameterName;

var transient ParticleSystemComponent ShieldedEffect;

var array<UTOnslaughtNodeEnhancement> Enhancements; /** node enhancements (tarydium processors, etc.) */

/** node teleporters controlled by this node */
var array<UTOnslaughtNodeTeleporter> NodeTeleporters;

var UTOnslaughtFlagBase FlagBase;					/** If flagbase is associated with this node, onslaught flag can be returned here */

/** wake up call to fools shooting invalid target :) */
var SoundCue ShieldHitSound;
var int ShieldDamageCounter;

/** emitter spawned when we're being healed */
var UTOnslaughtNodeHealEffectBase HealEffect;
/** the class of that emitter to use */
var class<UTOnslaughtNodeHealEffectBase> HealEffectClasses[2];

/** if specified, the team that owns this PowerCore starts the game with this node in their control */
var() UTOnslaughtPowerCore StartingOwnerCore;

/** localized string parts for creating location descriptions */
var localized string OutsideLocationPrefix, OutsideLocationPostfix, BetweenLocationPrefix, BetweenLocationJoin, BetweenLocationPostFix;


cpptext
{
	virtual UBOOL ProscribePathTo(ANavigationPoint *Nav, AScout *Scout = NULL);
}

replication
{
	//if ( bNetInitial )
	if (bNetDirty) // FIXME: expensive, but need to replicate when !bNetInitial for Kismet changing links. Add a flag?
		LinkedNodes, NumLinks;

	if (bNetDirty && Role == ROLE_Authority)
		NodeState, bSevered;
}

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( NodeBeamEffect != None )
	{
		NodeBeamEffect.SetHidden(true);
	}
}

simulated function string GetLocationStringFor(PlayerReplicationInfo PRI)
{
	if ( (UTPlayerReplicationInfo(PRI) != None) && (UTPlayerReplicationInfo(PRI).SecondaryPlayerLocationHint != None) )
	{
		if ( UTPlayerReplicationInfo(PRI).SecondaryPlayerLocationHint == self )
		{
			return OutsideLocationPrefix$GetHumanReadableName()$OutsideLocationPostfix;
		}
		return BetweenLocationPrefix$GetHumanReadableName()$BetweenLocationJoin$UTPlayerReplicationInfo(PRI).SecondaryPlayerLocationHint.GetHumanReadableName()$BetweenLocationPostFix;
	}
	return LocationPrefix$GetHumanReadableName()$LocationPostfix;
}

function BetweenEndPointFor(Pawn P, out actor MainLocationHint, out actor SecondaryLocationHint)
{
	local int i;
	local UTOnslaughtGame G;
	local vector Dir;
	local float BestDist, NewDot, NewDist;
	local actor Best;

	// find which endpoint pawn is nearest (and in right direction for)
	// use UTOnslaughtgame node list
	G = UTOnslaughtGame(WorldInfo.Game);
	if ( G != None )
	{
		Dir = Normal(Location - P.Location);
		BestDist = 100000.0;
		for ( i=0; i<G.PowerNodes.Length; i++ )
		{
			// check "betweenness"
			if ( G.PowerNodes[i] != self )
			{
				NewDot = Dir dot Normal(P.Location - G.PowerNodes[i].Location);
				if ( NewDot > 0.0 )
				{
					NewDist = VSize(P.Location - G.PowerNodes[i].Location) * (3.0 - NewDot);
					if ( NewDist < BestDist )
					{
						BestDist = NewDist;
						Best = G.PowerNodes[i];
					}
				}
			}

		}
		if ( Best != None )
		{
			SecondaryLocationHint = Best;
			return;
		}
	}

	// return self so location is printed as "Outside Base" instead of "Near Base"
	SecondaryLocationHint = self;
}

function TarydiumBoost(float Quantity)
{
	local float BoostShare;
	local int i;

	if (Enhancements.Length + VehicleFactories.Length > 0)
	{
		BoostShare = Quantity/(Enhancements.Length + VehicleFactories.Length);

		for (i = 0; i < VehicleFactories.Length; i++)
		{
			VehicleFactories[i].TarydiumBoost(BoostShare);
		}

		for (i = 0; i < Enhancements.Length; i++)
		{
			Enhancements[i].TarydiumBoost(BoostShare);
		}
	}
}

function AddActivatedObjective(UTOnslaughtSpecialObjective O)
{
	ActivatedObjectives[ActivatedObjectives.Length] = O;
}

function bool Shootable()
{
	return true;
}

function InitCloseActors()
{
	Super.InitCloseActors();

	NodeTeleporters.length = 0;
}

simulated function bool IsKeyBeaconObjective(bool bFriendlyPowered, UTPlayerController PC)
{
	//`log("###"@self@bFriendlyPowered@PC@PC.LastAutoObjective);
	return (self == PC.LastAutoObjective) && bFriendlyPowered && (UTPlayerReplicationInfo(PC.PlayerReplicationInfo).GetFlag() == None);
}

/**
PostRenderFor()
Hook to allow objectives to render HUD overlays for themselves.
Called only if objective was rendered this tick.
Assumes that appropriate font has already been set
*/
simulated function PostRenderFor(PlayerController PC, Canvas Canvas, vector CameraPosition, vector CameraDir)
{
	local float TextXL, XL, YL, HealthX, HealthMaxX, HealthY, BeaconPulseScale, Dist, IconYL;
	local vector ScreenLoc, IconLoc;
	local LinearColor TeamColor;
	local Color TextColor;
	local bool bFriendlyPowered;
	local UTWeapon Weap;

	// must be in front of player to render HUD overlay
	if ( (CameraDir dot (Location - CameraPosition)) <= 0 )
		return;

	if ( !PoweredBy(PC.GetTeamNum()) )
	{
		if ( IsNeutral() )
			return;
	}
	else
	{
		bFriendlyPowered = true;
	}

	screenLoc = Canvas.Project(Location + GetHUDOffset(PC,Canvas));

	// make sure not clipped out
	if (screenLoc.X < 0 ||
		screenLoc.X >= Canvas.ClipX ||
		screenLoc.Y < 0 ||
		screenLoc.Y >= Canvas.ClipY)
	{
		return;
	}

	if ( !IsKeyBeaconObjective(bFriendlyPowered, UTPlayerController(PC))  )
	{
		// must have been rendered
		if ( !LocalPlayer(PC.Player).GetActorVisibility(self) )
			return;

		// only draw objectives that aren't currently relevant if very close and visible
		Dist = VSize(CameraPosition - Location);

		if ( PC.LODDistanceFactor * Dist > MaxBeaconDistance )
		{
			return;
		}

		// make sure not behind weapon
		if ( UTPawn(PC.Pawn) != None )
		{
			Weap = UTWeapon(UTPawn(PC.Pawn).Weapon);
			if ( (Weap != None) && Weap.CoversScreenSpace(screenLoc, Canvas) )
			{
				return;
			}
		}

		// periodically make sure really visible using traces
		if ( WorldInfo.TimeSeconds - LastPostRenderTraceTime > 0.5 )
		{
			LastPostRenderTraceTime = WorldInfo.TimeSeconds + 0.2*FRand();
			bPostRenderTraceSucceeded = FastTrace(Location, CameraPosition)
										|| FastTrace(Location+CylinderComponent.CollisionHeight*vect(0,0,1), CameraPosition);
		}
		if ( !bPostRenderTraceSucceeded )
		{
			return;
		}
		BeaconPulseScale = 1.0;
	}
	else
	{
		// pulse "key" objective
		BeaconPulseScale = UTPlayerController(PC).BeaconPulseScale;
	}

	class'UTHUD'.Static.GetTeamColor( GetTeamNum(), TeamColor, TextColor);

	TeamColor.A = 1.0;

	// fade if close to crosshair
	if (screenLoc.X > 0.4*Canvas.ClipX &&
		screenLoc.X < 0.6*Canvas.ClipX &&
		screenLoc.Y > 0.4*Canvas.ClipY &&
		screenLoc.Y < 0.6*Canvas.ClipY)
	{
		TeamColor.A = FMax(FMin(1.0, FMax(0.0,Abs(screenLoc.X - 0.5*Canvas.ClipX) - 0.05*Canvas.ClipX)/(0.05*Canvas.ClipX)), FMin(1.0, FMax(0.0, Abs(screenLoc.Y - 0.5*Canvas.ClipY)-0.05*Canvas.ClipX)/(0.05*Canvas.ClipY)));
		if ( TeamColor.A == 0.0 )
		{
			return;
		}
	}

	// fade if far away or not visible
	TeamColor.A = FMin(TeamColor.A, LocalPlayer(PC.Player).GetActorVisibility(self)
									? FClamp(4000/VSize(Location - CameraPosition),0.3, 1.0)
									: 0.2);

	HealthY = 16*Canvas.ClipX*BeaconPulseScale / 1024;
	Canvas.StrLen(ObjectiveName, TextXL, YL);
	XL = FMax(TextXL * BeaconPulseScale, 0.05*Canvas.ClipX);
	YL *= BeaconPulseScale;

	IconYL = XL;
	class'UTHUD'.static.DrawBackground(ScreenLoc.X-0.7*XL,ScreenLoc.Y-0.6*(YL+HealthY)- 0.5*(YL+IconYL),1.4*XL,1.2*(YL+HealthY) + YL + IconYL, TeamColor, Canvas);

	Canvas.DrawColor = TextColor;
	Canvas.DrawColor.A = 255.0 * TeamColor.A;

	// draw node icon
	IconLoc = ScreenLoc;
	IconLoc.Y = IconLoc.Y - 0.25*IconYL;
	DrawIcon(Canvas, IconLoc, IconYL, TeamColor.A);

	// draw node name
	Canvas.SetPos(ScreenLoc.X-0.5*TextXL,ScreenLoc.Y);
	Canvas.DrawTextClipped(ObjectiveName, true);

	// draw health bar
	if ( PostRenderShowHealth() && LocalPlayer(PC.Player).GetActorVisibility(self) )
	{
		HealthMaxX = 0.9 * XL;
		HealthX = HealthMaxX* FMin(1.0, Health/DamageCapacity);
		Class'UTHUD'.static.DrawHealth(ScreenLoc.X-0.45*XL,ScreenLoc.Y + YL,HealthX,HealthMaxX,HealthY, Canvas, Canvas.DrawColor.A);
	}
}

simulated function bool PostRenderShowHealth()
{
	return ( IsActive() || IsConstructing() );
}



/** SetCoreDistance()
determine how many hops each node is from powercore N in hops
*/
function SetCoreDistance(byte TeamNum, int Hops)
{
	local int i;

	if ( Hops < FinalCoreDistance[TeamNum] )
	{
		FinalCoreDistance[TeamNum] = Hops;
		Hops += 1;
		for ( i=0; i<MAXNUMLINKS; i++ )
		{
			if (LinkedNodes[i] == None)
			{
				break;
			}
			else
			{
				LinkedNodes[i].SetCoreDistance(TeamNum, Hops);
			}
		}
		NumLinks = i;
	}
}

/** FindNearestFriendlyNode()
returns nearby node at which team can spawn
*/
function UTGameObjective FindNearestFriendlyNode(int TeamIndex)
{
	local float BestDist, NewDist;
	local UTGameObjective BestNode, NewNode;
	local UTOnslaughtGame Game;
	local int i;

	if (ValidSpawnPointFor(TeamIndex))
	{
		return self;
	}
	else
	{
		Game = UTOnslaughtGame(WorldInfo.Game);
		if (Game != None)
		{
			for (i = 0; i < Game.PowerNodes.length; i++)
			{
				NewNode = Game.PowerNodes[i];
				if (NewNode != None && NewNode.ValidSpawnPointFor(TeamIndex))
				{
					NewDist = VSize(NewNode.Location - Location);
					if (BestNode == None || NewDist < BestDist)
					{
						BestNode = NewNode;
						BestDist = NewDist;
					}
				}
			}
		}

		return BestNode;
	}
}


function InitLinks()
{
	local int i;

	for (i = 0; i < MAXNUMLINKS; i++)
	{
		if (LinkedNodes[i] != None)
		{
			// if linked to a power core, boost defense priority
			if (LinkedNodes[i].IsA('UTOnslaughtPowerCore'))
			{
				DefensePriority = Max(DefensePriority, 5);
			}
			LinkedNodes[i].CheckLink(self);
		}
	}

	if (bStandalone)
	{
		DefensePriority = default.DefensePriority - 1;
	}
}

/** if this node is not already linked to the specified node, add a link to it */
function CheckLink(UTOnslaughtNodeObjective Node)
{
	local int i;

	// if linked to a power core, boost defense priority
	if (Node.IsA('UTOnslaughtPowerCore'))
	{
		DefensePriority = Max(DefensePriority, 5);
	}

	// see if Node is already in list
	for ( i=0; i<MAXNUMLINKS; i++ )
	{
		if ( LinkedNodes[i] == Node )
			return;
	}

	// if not, add it
	for ( i=0; i<MAXNUMLINKS; i++ )
	{
		if (LinkedNodes[i] == None)
		{
			LinkedNodes[i] = Node;
			NumLinks = Max(NumLinks, i + 1);
			return;
		}
	}
}

/** adds a link between two nodes */
function AddLink(UTOnslaughtNodeObjective Node)
{
	CheckLink(Node);
	Node.CheckLink(self);

	UpdateEffects(false);
	Node.UpdateEffects(false);

	UpdateLinks();
}

/** removes a link between two nodes */
function RemoveLink(UTOnslaughtNodeObjective Node)
{
	local int i, j;
	local bool bStillLinkedToCore;

	i = FindNodeLinkIndex(Node);
	if (i != INDEX_NONE)
	{
		NumLinks--;
		for (j = i; j < NumLinks; j++)
		{
			LinkedNodes[j] = LinkedNodes[j + 1];
		}
		LinkedNodes[NumLinks] = None;
		Node.RemoveLink(self);

		UpdateEffects(false);
		UpdateLinks();
		if (Node.IsA('UTOnslaughtPowerCore'))
		{
			// if we were linked to a powercore and we're not anymore, return to default defense priority
			for (j = 0; j < NumLinks; j++)
			{
				if (UTOnslaughtPowerCore(LinkedNodes[j]) != None)
				{
					bStillLinkedToCore = true;
					break;
				}
			}
			if (!bStillLinkedToCore)
			{
				DefensePriority = default.DefensePriority;
			}
		}
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'NodeState' )
	{
		GotoState(NodeState);
	}
	else if ( VarName == 'bUnderAttack' )
	{
		if (bUnderAttack)
		{
			BecameUnderAttack();
		}
	}
	else if (VarName == 'bSevered' || VarName == 'DefenderTeamIndex')
	{
		UpdateEffects(true);
	}
	else if (VarName == 'LinkedNodes' || VarName == 'NumLinks')
	{
		UpdateEffects(true);
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function NotifyLocalPlayerTeamReceived()
{
	UpdateEffects(false);
}

simulated event SetInitialState()
{
	local UTOnslaughtNodeObjective O;
	local bool bDisabled;

	bScriptInitialized = true;

	if (Role == ROLE_Authority)
	{
		if (LinkedNodes[0] == None)
		{
			bDisabled = true;
			foreach WorldInfo.AllNavigationPoints(class'UTOnslaughtNodeObjective', O)
			{
				if (O.FindNodeLinkIndex(self) != -1)
				{
					bDisabled = false;
					break;
				}
			}
		}

		if (bDisabled && !bStandalone)
		{
			GotoState('DisabledNode');
		}
		else if (StartingOwnerCore == None)
		{
			GotoState('NeutralNode',, true);
		}
		else
		{
			DefenderTeamIndex = StartingOwnerCore.GetTeamNum();
			Health = DamageCapacity;
			GotoState('ActiveNode',, true);
		}
	}
}

simulated event bool IsActive()
{
	return false;
}

simulated function bool PoweredBy(byte Team)
{
	local int i;

	if (bStandalone)
	{
		return true;
	}
	else
	{
		for (i = 0; i < NumLinks; i++)
		{
			if (LinkedNodes[i] != None && LinkedNodes[i].IsActive() && !LinkedNodes[i].bSevered && LinkedNodes[i].DefenderTeamIndex == Team)
			{
				return true;
			}
		}
		return false;
	}
}

function UpdateLinks()
{
	UTOnslaughtGame(WorldInfo.Game).UpdateLinks();
}

function Sever()
{
	if ( !WorldInfo.Game.bGameEnded && (UTGame(WorldInfo.Game).ResetCountDown <= 0) )
	{
		SetTimer(1.0, True,'SeveredDamage');
		if (DefenderTeamIndex == 0)
			BroadcastLocalizedMessage(MessageClass, 27,,, self);
		else if (DefenderTeamIndex == 1)
			BroadcastLocalizedMessage(MessageClass, 28,,, self);
	}
}

simulated function bool CreateBeamMaterialInstance()
{
	if ( WorldInfo.NetMode == NM_DedicatedServer )
	{
		return false;
	}
	else
	{
		BeamMaterialInstance = NodeBeamEffect.CreateAndSetMaterialInstanceTimeVarying(0);
		return true;
	}
}

/** called on a timer to update the under attack effect, if necessary */
simulated function UpdateAttackEffect()
{
	if (bUnderAttack)
	{
		BeamMaterialInstance.SetScalarStartTime(GodBeamAttackParameterName, 0.0);
		//@FIXME: MaterialInstanceTimeVarying should be based off game time, not app time
		SetTimer(AttackEffectCurve.Points[AttackEffectCurve.Points.length - 1].InVal * WorldInfo.TimeDilation, false, 'UpdateAttackEffect');
	}
}

/* BecameUnderAttack()
Update Node beam effect to reflect whether node is under attack
*/
simulated function BecameUnderAttack()
{
	if (AttackEffectCurve.Points.length > 0 && (BeamMaterialInstance != None || CreateBeamMaterialInstance()))
	{
		BeamMaterialInstance.SetScalarCurveParameterValue(GodBeamAttackParameterName, AttackEffectCurve);
		BeamMaterialInstance.SetScalarStartTime(GodBeamAttackParameterName, 0.0);
		//@FIXME: MaterialInstanceTimeVarying should be based off game time, not app time
		SetTimer(AttackEffectCurve.Points[AttackEffectCurve.Points.length - 1].InVal * WorldInfo.TimeDilation, false, 'UpdateAttackEffect');
	}
}

/*
NodeBeamEffect parameters:
GodBeamAttack is a scalar parameter that when set to 1 will show the white pulses traveling up the beam.
Team is a vector parameter to set the color inside of the material instance.
*/
/* UpdateEffects()
*/
simulated function UpdateEffects(bool bPropagate)
{
	local int i;
	local bool bPoweredByEnemy;

	if ( WorldInfo.NetMode == NM_DedicatedServer )
		return;

	bPoweredByEnemy = PoweredBy(1-DefenderTeamIndex);

	// update node beam
	if ( bPoweredByEnemy && BeamEnabled() )
	{
		NodeBeamEffect.SetHidden(false);
		if ( (BeamMaterialInstance != None) || CreateBeamMaterialInstance() )
			BeamMaterialInstance.SetVectorParameterValue('Team', BeamColor[DefenderTeamIndex]);
	}
	else
	{
		NodeBeamEffect.SetHidden(true);
	}

	// update shield
	UpdateShield(bPoweredByEnemy);

	UpdateTeamStaticMeshes();

	// propagate to neighbors
	if ( bPropagate )
	{
		for ( i=0; i<NumLinks; i++ )
		{
			LinkedNodes[i].UpdateEffects(false);
		}
	}
}

simulated function UpdateShield(bool bPoweredByEnemy)
{
	if (ShieldedEffect != None)
	{
		ShieldedEffect.SetHidden(bPoweredByEnemy);
		if (bPoweredByEnemy)
		{
			ShieldedEffect.DeactivateSystem();
		}
		else
		{
			ShieldedEffect.ActivateSystem();
		}
	}
}

simulated function bool BeamEnabled()
{
	return false;
}

/** notify any Kismet events connected to this node that our state has changed */
function SendChangedEvent(Controller EventInstigator)
{
	local int i;
	local UTSeqEvent_OnslaughtNodeEvent NodeEvent;

	for (i = 0; i < GeneratedEvents.length; i++)
	{
		NodeEvent = UTSeqEvent_OnslaughtNodeEvent(GeneratedEvents[i]);
		if (NodeEvent != None)
		{
			NodeEvent.NotifyNodeChanged(EventInstigator);
		}
	}
}

simulated function SetAmbientSound(SoundCue NewAmbientSound)
{
	// if the component is already playing this sound, don't restart it
	if (NewAmbientSound != AmbientSoundComponent.SoundCue)
	{
		AmbientSoundComponent.Stop();
		AmbientSoundComponent.SoundCue = NewAmbientSound;
		if (NewAmbientSound != None)
		{
			AmbientSoundComponent.Play();
		}
	}
}

simulated state ActiveNode
{
	function UpdateCloseActors()
	{
	}

	function bool HasActiveDefenseSystem()
	{
		local int i;

		if ( !bHasSensor )
			return false;

		// only if all linked nodes are friendly or neutral
		for ( i=0; i<NumLinks; i++ )
		{
			if ( LinkedNodes[i].DefenderTeamIndex < 2 && LinkedNodes[i].DefenderTeamIndex != DefenderTeamIndex )
				return false;
		}
		return true;
	}

	simulated function bool BeamEnabled()
	{
		return true;
	}

	simulated event bool IsActive()
	{
		return true;
	}

	simulated function bool HasHealthBar()
	{
		return true;
	}

	function bool LegitimateTargetOf(UTBot B)
	{
		return (DefenderTeamIndex != B.Squad.Team.TeamIndex );
	}

	simulated function BeginState(Name PreviousStateName)
	{
		local int i;

		SetAmbientSound(ActiveSound);
		NodeState = GetStateName();

		// Update Visuals
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			PlaySound(ConstructedSound, true);

			if ( BeamMaterialInstance == None )
			{
				CreateBeamMaterialInstance();
			}
		}
		UpdateLinks();
		UpdateEffects(true);

		if (Role == ROLE_Authority)
		{
			Scorers.length = 0;
			// Update any vehicle factories in the power radius to be owned by the controlling team
			for ( i=0; i<VehicleFactories.Length; i++ )
			{
				VehicleFactories[i].Activate(DefenderTeamIndex);
			}

			// Update any deploy lockers int he power radius
			for(i=0;i<DeployableLockers.Length;++i)
			{
				DeployableLockers[i].Activate(DefenderTeamIndex);
			}

			// update node teleporters
			for (i = 0; i < NodeTeleporters.length; i++)
			{
				NodeTeleporters[i].SetTeamNum(DefenderTeamIndex);
			}

			// activate node enhancements
			for (i = 0; i < Enhancements.Length; i++)
			{
				Enhancements[i].Activate();
			}

			// activate special objectives
			for (i = 0; i < ActivatedObjectives.length; i++)
			{
				ActivatedObjectives[i].CheckActivate();
			}
		}

		// check if any players are already touching adjacent neutral nodes
		for ( i=0; i<NumLinks; i++ )
		{
			LinkedNodes[i].CheckTouching();
		}

		SendChangedEvent(Constructor);
	}

	function EndState(name NextStateName)
	{
		local int i;

		// de-activate node enhancements
		for (i = 0; i < Enhancements.Length; i++)
		{
			Enhancements[i].Deactivate();
		}
	}
}

simulated state DisabledNode
{
	simulated event bool IsDisabled()
	{
		return true;
	}

	function Sever() {}

	function SeveredDamage()
	{
		SetTimer(0, false, 'SeveredDamage');
	}

	event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{}

	simulated function RenderLinks( UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, float ColorPercent, bool bFullScreenMap, bool bSelected ) {}

	simulated function bool ShouldRenderLink()
	{
		return false;
	}

	simulated function UpdateShield(bool bPoweredByEnemy)
	{
		if (ShieldedEffect != None)
		{
			ShieldedEffect.DeactivateSystem();
			ShieldedEffect.SetHidden(true);
		}
	}

	simulated event BeginState(Name PreviousStateName)
	{
		local UTOnslaughtNodeObjective Node, Best;
		local float Dist, BestDist;
		local int i;

		NodeState = GetStateName();
		SetHidden(True);
		if ( NodeBeamEffect != None )
		{
			NodeBeamEffect.SetHidden(true);
		}
		NetUpdateTime = WorldInfo.TimeSeconds - 1;
		SetCollision(false, false);
		SetTimer(0, false);
		NetUpdateFrequency = 0.1;

		// try to find a nearby objective to give our close actors to
		// this is so if LDs place multiple different objectives in the same location for different link setups
		// the right one gets all the playerstarts, etc
		BestDist = 500.0;
		foreach WorldInfo.AllNavigationPoints(class'UTOnslaughtNodeObjective', Node)
		{
			Dist = VSize(Node.Location - Location);
			if (Dist < BestDist && !Node.IsDisabled())
			{
				Best = Node;
				BestDist = Dist;
			}
		}
		if (Best != None)
		{
			for (i = 0; i < VehicleFactories.length; i++)
			{
				Best.VehicleFactories[Best.VehicleFactories.length] = VehicleFactories[i];
			}
			VehicleFactories.length = 0;
			for (i = 0; i < PlayerStarts.length; i++)
			{
				Best.PlayerStarts[Best.PlayerStarts.length] = PlayerStarts[i];
			}
			PlayerStarts.length = 0;
			for(i =0; i<DeployableLockers.length;++i)
			{
				Best.DeployableLockers[Best.DeployableLockers.length] = DeployableLockers[i];
			}
			DeployableLockers.length = 0;
			for (i = 0; i < NodeTeleporters.length; i++)
			{
				Best.NodeTeleporters[Best.NodeTeleporters.length] = NodeTeleporters[i];
			}
			NodeTeleporters.length = 0;
		}
		else
		{
			// tell node teleporters they're disabled
			for (i = 0; i < NodeTeleporters.length; i++)
			{
				NodeTeleporters[i].TurnOff();
			}
		}
	}

	event EndState(Name NextStateName)
	{
		SetHidden(default.bHidden);
		NetUpdateTime = WorldInfo.TimeSeconds - 1;
		SetCollision(default.bCollideActors, default.bBlockActors);
		NetUpdateFrequency = default.NetUpdateFrequency;
	}
}

simulated state NeutralNode
{
	function Sever() {}

	function SeveredDamage()
	{
		SetTimer(0, false, 'SeveredDamage');
	}

	function bool Shootable()
	{
		return false;
	}

	function bool TellBotHowToDisable(UTBot B)
	{
		if ( StandGuard(B) )
			return TooClose(B);

		return B.Squad.FindPathToObjective(B, self);
	}


	function bool ValidSpawnPointFor(byte TeamIndex)
	{
		return false;
	}

	function bool LegitimateTargetOf(UTBot B)
	{
		return false;
	}

	event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{}

	function bool HealDamage(int Amount, Controller Healer, class<DamageType> DamageType) { return false; }

	simulated function UpdateShield(bool bPoweredByEnemy)
	{
		if (ShieldedEffect != None)
		{
			ShieldedEffect.DeactivateSystem();
			ShieldedEffect.SetHidden(true);
		}
	}

	simulated event BeginState(Name PreviousStateName)
	{
		NodeState = GetStateName();
		SetAmbientSound(NeutralSound);
		bIsNeutral = true;
		Health = 0;
		DefenderTeamIndex = 2;

		if (Role == ROLE_Authority)
		{
			NetUpdateTime = WorldInfo.TimeSeconds - 1;
		}
		UpdateLinks();
		UpdateEffects(true);
		CheckTouching();
	}

	simulated function EndState(name NextStateName)
	{
		bIsNeutral = false;
	}
}

function BecomeActive()
{
	if (DefenderTeamIndex < 2)
	{
		BroadcastLocalizedMessage(MessageClass, ActivationMessageIndex + DefenderTeamIndex,,, self);
	}
	if (Constructor != None && UTPlayerReplicationInfo(Constructor.PlayerReplicationInfo) != None)
	{
		WorldInfo.Game.ScoreObjective(Constructor.PlayerReplicationInfo, float(Score) / 2.0);
		WorldInfo.Game.ScoreEvent(Constructor.PlayerReplicationInfo, float(Score) / 2.0, ConstructedEvent[DefenderTeamIndex]);
	}
	GotoState('ActiveNode');

	FindNewObjectives();
}

/** calls FindNewObjectives() on the GameInfo; seperated out for subclasses */
function FindNewObjectives()
{
	UTGame(WorldInfo.Game).FindNewObjectives(self);
}

function Reset()
{
	Health = DamageCapacity;
	NetUpdateTime = WorldInfo.TimeSeconds - 1;

	if ( bScriptInitialized )
	{
		SetInitialState();
	}

	UpdateCloseActors();

	SendChangedEvent(None);
}

simulated function bool LinkedTo(UTOnslaughtNodeObjective PC)
{
	return FindNodeLinkIndex(PC) != -1;
}

/** if the given Node is in the LinkedNodes array, returns its index, otherwise INDEX_NONE */
simulated function int FindNodeLinkIndex( UTOnslaughtObjective Node )
{
	local int i;

	if ( Node == None )
	{
		return INDEX_NONE;
	}

	for (i = 0; i < NumLinks; i++)
	{
		if (LinkedNodes[i] == Node)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

/** applies any scaling factors to damage we're about to take */
simulated function ScaleDamage(out int Damage, Controller InstigatedBy, class<DamageType> DamageType)
{
	if (class<UTDamageType>(DamageType) != None)
	{
		Damage *= class<UTDamageType>(DamageType).default.NodeDamageScaling;
	}
	//@note: DamageScaling isn't replicated, so this part doesn't work on clients
	if (Role == ROLE_Authority && InstigatedBy != None && InstigatedBy.Pawn != None)
	{
		Damage *= instigatedBy.Pawn.GetDamageScaling();
	}
}

event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	if (Damage <= 0 || WorldInfo.Game.bGameEnded || UTGame(WorldInfo.Game).ResetCountdown > 0)
		return;

	ScaleDamage(Damage, InstigatedBy, DamageType);

	if (InstigatedBy == None || (InstigatedBy.GetTeamNum() != DefenderTeamIndex && PoweredBy(InstigatedBy.GetTeamNum())))
	{
		SetUnderAttack(true);
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			BecameUnderAttack();
		}
		NetUpdateTime = WorldInfo.TimeSeconds - 1;
		if (InstigatedBy != None)
		{
			LastDamagedBy = UTPlayerReplicationInfo(InstigatedBy.PlayerReplicationInfo);
			LastAttacker = InstigatedBy.Pawn;
			AddScorer(LastDamagedBy, FMin(Health, Damage) / DamageCapacity);
			if (Health >= DamageCapacity)
			{
				UTGame(WorldInfo.Game).BonusEvent('first_strike',InstigatedBy.PlayerReplicationInfo);
			}
		}

		Health -= Damage;
		if (Health <= 0)
		{
    			DisableObjective(InstigatedBy);
		}
		else
		{
			BroadcastAttackNotification(InstigatedBy);
		}
	}
	else if ( (PlayerController(InstigatedBy) != None) && !WorldInfo.GRI.OnSameTeam(InstigatedBy,self) )
	{
		PlayerController(InstigatedBy).ReceiveLocalizedMessage(MessageClass, 5);

		// play 'can't attack' sound if player keeps shooting at us
		ShieldDamageCounter += Damage;
		if (ShieldDamageCounter > 200)
		{
			PlayerController(InstigatedBy).ClientPlaySound(ShieldHitSound);
			ShieldDamageCounter -= 200;
		}
	}
}

function BroadcastAttackNotification(Controller InstigatedBy)
{
	//attack notification
	if (LastAttackMessageTime + 1 < WorldInfo.TimeSeconds)
	{
		BroadcastLocalizedMessage(MessageClass, 9 + DefenderTeamIndex,,, self);
		if ( (InstigatedBy != None) && (InstigatedBy.Pawn != None) )
			UTTeamInfo(WorldInfo.GRI.Teams[DefenderTeamIndex]).AI.CriticalObjectiveWarning(self, InstigatedBy.Pawn);
		LastAttackMessageTime = WorldInfo.TimeSeconds;
	}
	LastAttackTime = WorldInfo.TimeSeconds;
}

function bool HealDamage(int Amount, Controller Healer, class<DamageType> DamageType)
{
	if (Health <= 0 || Amount <= 0 || LinkHealMult <= 0.0 || (Healer != None && !TeamLink(Healer.GetTeamNum())))
	{
		return false;
	}

	if (Health >= DamageCapacity)
	{
		if (WorldInfo.TimeSeconds - HealingTime < 0.5)
		{
			PlaySound(HealedSound);
		}

		return false;
	}

	Amount = Min(Amount * LinkHealMult, DamageCapacity - Health);
	Health += Amount;

	if (Health >= DamageCapacity)
	{
		UTGame(WorldInfo.Game).ObjectiveEvent('node_restored', GetTeamNum(), Healer != None ? Healer.PlayerReplicationInfo : None, nodenum);
	}

	if (Healer != None && UTOnslaughtPRI(Healer.PlayerReplicationInfo) != None)
	{
		UTOnslaughtPRI(Healer.PlayerReplicationInfo).AddHealBonus(float(Amount) / DamageCapacity * Score);
	}

	NetUpdateTime = WorldInfo.TimeSeconds - 1;
	HealingTime = WorldInfo.TimeSeconds;
	LastHealedBy = Healer;

	if (HealEffect == None && DefenderTeamIndex < 2 && HealEffectClasses[DefenderTeamIndex] != None)
	{
		HealEffect = Spawn(HealEffectClasses[DefenderTeamIndex], self);
	}

	SetTimer(0.5, false, 'CheckHealing');
	return true;
}

simulated function CheckHealing()
{
	if (WorldInfo.TimeSeconds - HealingTime >= 0.5)
	{
		if (HealEffect != None)
		{
			HealEffect.ShutDown();
			HealEffect = None;
		}
	}
	else
	{
		SetTimer(0.5 - WorldInfo.TimeSeconds + HealingTime, false, 'CheckHealing');
	}
}

function bool LinkedToCoreConstructingFor(byte Team)
{
	local int i;

	for ( i=0; i<NumLinks; i++ )
		if (LinkedNodes[i].DefenderTeamIndex == Team && LinkedNodes[i].IsConstructing())
			return true;

	return false;
}

/** notify actors associated with this node that it has been destroyed/disabled */
function UpdateCloseActors()
{
	local int i;

	for ( i=0; i<VehicleFactories.Length; i++ )
	{
		VehicleFactories[i].Deactivate();
	}

	for (i = 0; i < NodeTeleporters.length; i++)
	{
		NodeTeleporters[i].SetTeamNum(255);
	}

	// deactivate special objectives
	for (i = 0; i < ActivatedObjectives.length; i++)
	{
		ActivatedObjectives[i].CheckActivate();
	}

	for(i=0; i<DeployableLockers.length;++i)
	{
		DeployableLockers[i].Deactivate();
	}

	FindNewHomeForFlag();
}

/** if a flag's homebase is linked to this node, find a new homebase for it */
function FindNewHomeForFlag()
{
	local UTOnslaughtFlagBase NewFlagBase;

	if (FlagBase != None && FlagBase.myFlag != None)
	{
		NewFlagBase = FlagBase.myFlag.FindNearestFlagBase(self);
		if (NewFlagBase == None)
		{
			NewFlagBase = FlagBase.myFlag.StartingHomeBase;
		}
		if (NewFlagBase != FlagBase.myFlag.HomeBase)
		{
			FlagBase.myFlag.HomeBase = NewFlagBase;
			NewFlagBase.myFlag = FlagBase.myFlag;
			if (FlagBase.myFlag.bHome)
			{
				FlagBase.myFlag.SetLocation(NewFlagBase.Location);
				FlagBase.myFlag.SetRotation(NewFlagBase.Rotation);
				NewFlagBase.ObjectiveChanged();
			}
			FlagBase.myFlag.NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
			FlagBase.myFlag = None;
			FlagBase.HideOrb();
		}
	}
}

// Returns a rating based on nearby vehicles
function float RateCore()
{
	local int i;
	local float Result;

	for ( i=0; i<VehicleFactories.Length; i++ )
	{
		Result += VehicleFactories[i].VehicleClass.Default.MaxDesireability * VehicleFactories[i].VehicleClass.Default.MaxDesireability;
		if ( VehicleFactories[i].VehicleClass.Default.MaxDesireability > 0.6 )
			Result += 0.5;
	}
	return Result;
}

function float TeleportRating(Controller Asker, byte AskerTeam, byte SourceDist)
{
	local int i;
	local UTBot B;
	local float Rating;

	B = UTBot(Asker);
	for ( i=0; i<VehicleFactories.Length; i++ )
	{
		if ( (VehicleFactories[i].ChildVehicle != None) && VehicleFactories[i].ChildVehicle.bTeamLocked && !VehicleFactories[i].ChildVehicle.SpokenFor(Asker) )
		{
			if (B == None)
				Rating = FMax(Rating, VehicleFactories[i].ChildVehicle.MaxDesireability);
			else
				Rating = FMax(Rating, B.Squad.VehicleDesireability(VehicleFactories[i].ChildVehicle, B));
		}
	}
	return (Rating - (FinalCoreDistance[Abs(1 - AskerTeam)] - SourceDist) * 0.1);
}

function bool HasUsefulVehicles(Controller Asker)
{
	local int i;
	local UTBot B;

	B = UTBot(Asker);
	for ( i=0; i<VehicleFactories.Length; i++ )
	{
		if ( (VehicleFactories[i].ChildVehicle != None) && VehicleFactories[i].ChildVehicle.bTeamLocked
			&& (B == None || B.Squad.VehicleDesireability(VehicleFactories[i].ChildVehicle, B) > 0) )
		{
			return true;
		}
	}
	return false;
}

function Actor GetAutoObjectiveActor(UTPlayerController PC)
{
	local int i;
	local UTOnslaughtSpecialObjective Best;

	// redirect to required special objective if necessary
	for (i = 0; i < ActivatedObjectives.Length; i++)
	{
		if ( ActivatedObjectives[i].bMustCompleteToAttackNode && ActivatedObjectives[i].IsActive() &&
			(Best == None || ActivatedObjectives[i].DefensePriority > Best.DefensePriority) )
		{
			Best = ActivatedObjectives[i];
		}
	}

	return (Best != None) ? Best : Super.GetAutoObjectiveActor(PC);
}

function bool TellBotHowToDisable(UTBot B)
{
	if (DefenderTeamIndex == B.Squad.Team.TeamIndex)
		return false;
	if (!PoweredBy(B.Squad.Team.TeamIndex))
	{
		if (B.CanAttack(self))
			return false;
		else
			return B.Squad.FindPathToObjective(B, self);
	}

	//take out defensive turrets first
	if (B.Enemy != None && B.Enemy.bStationary && B.LineOfSightTo(B.Enemy) && UTVehicle(B.Enemy) != None && UTVehicle(B.Enemy).bDefensive)
		return false;

	if ( StandGuard(B) )
		return TooClose(B);

	if ( !B.Pawn.bStationary && B.Pawn.TooCloseToAttack(self) )
	{
		B.GoalString = "Back off from objective";
		B.RouteGoal = B.FindRandomDest();
		B.MoveTarget = B.RouteCache[0];
		B.SetAttractionState();
		return true;
	}
	else if ( B.CanAttack(self) )
	{
		if (KillEnemyFirst(B))
			return false;

		B.GoalString = "Attack Objective";
		B.DoRangedAttackOn(self);
		return true;
	}
	MarkShootSpotsFor(B.Pawn);
	return Super.TellBotHowToDisable(B);
}

function bool KillEnemyFirst(UTBot B)
{
	if ( !bUnderAttack || Health < DamageCapacity * 0.25
	     || (UTVehicle(B.Pawn) != None && UTVehicle(B.Pawn).HasOccupiedTurret()) )
	{
		return false;
	}
	else if (B.Enemy != None && B.Enemy.Controller != None && B.Enemy.CanAttack(B.Pawn))
	{
		return (B.Aggressiveness > 0.4 || B.LastUnderFire > WorldInfo.TimeSeconds - 1.0);
	}
	else if ( WorldInfo.TimeSeconds - HealingTime < 1.0 && LastHealedBy != None && LastHealedBy.Pawn != None &&
		LastHealedBy.Pawn.Health > 0 && B.Squad.SetEnemy(B, LastHealedBy.Pawn) && B.Enemy == LastHealedBy.Pawn )
	{
		//attack enemy healing me
		return true;
	}
	else
	{
		return false;
	}
}

function bool NearObjective(Pawn P)
{
	if (P.CanAttack(self))
		return true;

	return (VSize(Location - P.Location) < BaseRadius && P.LineOfSightTo(self));
}

function CheckTouching()
{
    	local Pawn P;

    	foreach BasedActors(class'Pawn', P)
    	{
    		Bump(P, None, vect(0,0,1));
    		return;
    	}

    	foreach TouchingActors(class'Pawn', P)
    	{
    		Touch(P, None, P.Location, vect(0,0,1));
    		return;
    	}
}

function SeveredDamage()
{
	if ( !bSevered )
	{
		SetTimer(0, false, 'SeveredDamage');
		return;
	}

	Health -= SeveredDamagePerSecond;
	if (Health <= 0)
	{
		DisableObjective(None);
	}
	NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
	SetTimer(1.0, true, 'SeveredDamage');
}

simulated function bool HasHealthBar()
{
	return false;
}

simulated function bool ShouldRenderLink()
{
	return ( !bAlreadyRendered );
}

/** @return the draw color of this objective's map icon */
simulated function color GetIconColor(float ColorPercent)
{
	local color NodeColor;

	if ( IsConstructing() )
	{
		NodeColor = ControlColor[DefenderTeamIndex] * ColorPercent;
		NodeColor.R += ControlColor[2].R * (1.0 - ColorPercent);
		NodeColor.G += ControlColor[2].G * (1.0 - ColorPercent);
		NodeColor.B += ControlColor[2].B * (1.0 - ColorPercent);
	}
	else
	{
		NodeColor = ControlColor[DefenderTeamIndex];
	}

	return NodeColor;
}

simulated function RenderLinks( UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, float ColorPercent, bool bFullScreenMap, bool bSelected )
{
	local color NodeColor, LinkedNodeColor, CurrentNodeColor, DrawColor;
	local int i;
	local vector LinkDir;

	// draw links
	NodeColor = IsConstructing() ? (ControlColor[DefenderTeamIndex] * ColorPercent) : ControlColor[DefenderTeamIndex];
	for ( i=0; i<NumLinks; i++ )
	{
		if (LinkedNodes[i].ShouldRenderLink())
		{
			LinkedNodeColor = ControlColor[LinkedNodes[i].DefenderTeamIndex];
			if (IsNeutral() && LinkedNodes[i].IsActive())
			{
				DrawColor = LinkedNodeColor;
			}
			else if (IsActive() && LinkedNodes[i].IsNeutral())
			{
				DrawColor = NodeColor;
			}
			else
			{
				CurrentNodeColor = NodeColor;
				if (IsConstructing())
				{
					LinkedNodeColor = LinkedNodeColor * (1.0 - ColorPercent);
				}
				else if (LinkedNodes[i].IsConstructing())
				{
					LinkedNodeColor = LinkedNodeColor * ColorPercent;
					CurrentNodeColor = NodeColor * (1.0 - ColorPercent);
				}
				DrawColor.R = CurrentNodeColor.R + LinkedNodeColor.R;
				DrawColor.G = CurrentNodeColor.G + LinkedNodeColor.G;
				DrawColor.B = CurrentNodeColor.B + LinkedNodeColor.B;
			}

			LinkDir = 8.0 * MP.MapScale * Normal(LinkedNodes[i].HUDLocation - HUDLocation);
			Canvas.Draw2DLine(HUDLocation.X + LinkDir.X, HudLocation.Y + LinkDir.Y, LinkedNodes[i].HUDLocation.X - LinkDir.X, LinkedNodes[i].HUDLocation.Y - LinkDir.Y, DrawColor);
		}
	}

	bAlreadyRendered = true;

	if ( IsKeyBeaconObjective(true, PlayerOwner) )
	{
		MP.SetCurrentObjective(HudLocation);
	}

	Super.RenderLinks(MP, Canvas, PlayerOwner, ColorPercent, bFullScreenMap, bSelected);
}

simulated event bool IsLocked(int TeamIndex)
{
	return ( !PoweredBy( TeamIndex ) && !IsA('UTOnslaughtPowerCore') );
}


function DisableObjective(Controller InstigatedBy)
{
	local PlayerReplicationInfo	PRI;
	local int TeamNum;

	if ( InstigatedBy != None )
	{
		Instigator = InstigatedBy.Pawn;
		PRI = InstigatedBy.PlayerReplicationInfo;
		TeamNum = InstigatedBy.GetTeamNum();
	}
	else
	{
		PRI = LastDamagedBy;
		TeamNum = 255;
	}

	BroadcastLocalizedMessage(MessageClass, DestructionMessageIndex + DefenderTeamIndex, PRI,, self);

	if ( DefenderTeamIndex > 1 )
		`log("DisableObjective called with DefenderTeamIndex="$DefenderTeamIndex$" in state "$GetStateName());
	else if ( IsActive() )
		ShareScore(Score, DestroyedEvent[DefenderTeamIndex]);
	else
		ShareScore(Score, DestroyedEvent[2+DefenderTeamIndex]);

	UTGame(WorldInfo.Game).ObjectiveEvent('node_destroyed', TeamNum, PRI, NodeNum);
	GotoState('ObjectiveDestroyed');
}

simulated state ObjectiveDestroyed
{
	event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{}

	simulated event bool IsCurrentlyDestroyed()
	{
		return true;
	}

	function SeveredDamage()
	{
		SetTimer(0, false, 'SeveredDamage');
	}

	function Timer()
	{
		GotoState('NeutralNode');
	}

	simulated function UpdateShield(bool bPoweredByEnemy)
	{
		if (ShieldedEffect != None)
		{
			ShieldedEffect.DeactivateSystem();
			ShieldedEffect.SetHidden(true);
		}
	}

	simulated function BeginState(Name PreviousStateName)
	{
		local PlayerController PC;

		SetAmbientSound(None);
		UpdateLinks();
		UpdateEffects(true);

		if ( Role < ROLE_Authority )
			return;

		Health = 0;
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			ForEach LocalPlayerControllers(class'PlayerController', PC)
				break;
			if (PC != None)
				PC.ClientPlaySound(DestroyedSound);
			else
				PlaySound(DestroyedSound);

			// FIXMESTEVE spawn explosion
		}

		SendChangedEvent(Instigator != None ? Instigator.Controller : None);

		NetUpdateTime = WorldInfo.TimeSeconds - 1;
		Scorers.length = 0;
		UpdateCloseActors();
		DefenderTeamIndex = 2;
		SetTimer(2.0, false);

		UTGame(WorldInfo.Game).ObjectiveDisabled(Self);
		FindNewObjectives();
		NodeState = GetStateName();
	}
}

function bool TeleportTo(UTPawn Traveler)
{
	local NavigationPoint BestStart;
	local float BestRating, NewRating;
	local vector PrevPosition;
	local int i;
	local rotator NewRotation;
	local UTOnslaughtNodeTeleporter Teleporter;

	// Check to see if the teleport is valid
	if ( Traveler.Controller != none && ValidSpawnPointFor( Traveler.GetTeamNum() ) )
	{
		for (i = 0; i < PlayerStarts.length; i++)
		{
			NewRating = WorldInfo.Game.RatePlayerStart(PlayerStarts[i],Traveler.GetTeamNum(),Traveler.Controller);
			if ( NewRating > BestRating )
			{
				BestRating = NewRating;
				BestStart = PlayerStarts[i];
			}
		}

		if (BestStart != None)
		{
			// if a teleporter was used, update its destination
			if (UTOnslaughtGame(WorldInfo.Game) != None && UTOnslaughtGame(WorldInfo.Game).IsTouchingNodeTeleporter(Traveler, Teleporter))
			{
				Teleporter.SetLastDestination(BestStart);
			}
			PrevPosition = Traveler.Location;
			Traveler.SetLocation(BestStart.Location);
			Traveler.DoTranslocate(PrevPosition);
			NewRotation = BestStart.Rotation;
			NewRotation.Roll = 0;
			Traveler.Controller.ClientSetRotation(NewRotation);

			if (UTBot(Traveler.Controller) != None && UTOnslaughtPRI(Traveler.PlayerReplicationInfo) != None)
			{
				UTOnslaughtPRI(Traveler.PlayerReplicationInfo).SetStartObjective(self, false);
			}

			return true;
		}
	}
	return false;
}



defaultproperties
{
	bAlwaysRelevant=true
	RemoteRole=ROLE_SimulatedProxy
	NetUpdateFrequency=1

	FinalCoreDistance[0]=255
	FinalCoreDistance[1]=255
	ConstructionTime=30.0

	DamageCapacity=4500
	Score=10
	DefensePriority=2
	DefenderTeamIndex=2
	DestructionMessageIndex=14
	ActivationMessageIndex=2

	bSevered=False
	SeveredDamagePerSecond=100

	bPathColliding=true

	bStatic=False
	bNoDelete=True
	LinkHealMult=0.0

	bDestinationOnly=true
	bNotBased=True
	bCollideActors=True
	bCollideWorld=True
	bIgnoreEncroachers=True
	bBlockActors=True
	bProjTarget=True
	bCanBeDamaged=true
	bHidden=False
	DestroyedEvent(0)="red_powercore_destroyed"
	DestroyedEvent(1)="blue_powercore_destroyed"
	DestroyedEvent(2)="red_constructing_powercore_destroyed"
	DestroyedEvent(3)="blue_constructing_powercore_destroyed"
	ConstructedEvent(0)="red_powercore_constructed"
	ConstructedEvent(1)="blue_powercore_constructed"
	bBlocksTeleport=true
	bHasSensor=true
	bCanWalkOnToReach=true
	MaxBeaconDistance=4000.0

	BeamColor(0)=(R=1.5,G=0.7,B=0.7)
	BeamColor(1)=(R=0.2,G=0.7,B=4.0)
	BeamColor(2)=(R=1.0,G=1.0,B=1.0)
	GodBeamAttackParameterName=GodBeamAttack
	AttackEffectCurve=(Points=((InVal=0.0,OutVal=1.0),(InVal=0.4,OutVal=5.0),(InVal=1.0,OutVal=1.0)))

	Components.Remove(Sprite)
	Components.Remove(Sprite2)
	GoodSprite=None
	BadSprite=None

	Begin Object Name=CollisionCylinder
		CollisionRadius=+0120.000000
		CollisionHeight=+0150.000000
	End Object

	Begin Object Class=LinkRenderingComponent Name=LinkRenderer
		HiddenGame=True
	End Object
	Components.Add(LinkRenderer)

	Begin Object Class=AudioComponent Name=AmbientComponent
		bShouldRemainActiveIfDropped=true
		bStopWhenOwnerDestroyed=true
	End Object
	AmbientSoundComponent=AmbientComponent
	Components.Add(AmbientComponent)

	IconPosY=0
	IconExtentX=0.25
	IconPosX=0.25
	IconExtentY=0.125

	SupportedEvents.Add(class'UTSeqEvent_OnslaughtNodeEvent')

	StandaloneSpawnPriority=255
}
