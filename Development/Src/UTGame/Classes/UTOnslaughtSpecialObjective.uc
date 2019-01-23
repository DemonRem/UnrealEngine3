/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtSpecialObjective extends UTOnslaughtObjective;

/** node that is associated with this objective */
var() UTOnslaughtNodeObjective ControllingNode;

var() enum EObjectiveType
{
	OBJ_Destroyable,
	OBJ_Touch,
} ObjectiveType;

/** objective can only be disabled once per game (won't reset if ControllingNode changes hands, etc) */
var()	bool	bTriggerOnceOnly;
/** for objectives with no ControllingNode, whether this node starts active (otherwise, ControllingNode handles it) */
var()	bool	bInitiallyActive;
/** disabling changes the objectives team rather than removing it */
var()	bool	bTeamControlled;
/** bot flag; indicates this objective must be completed before it's possible to attack the controlling node */
var() bool bMustCompleteToAttackNode;
/** touch objectives only: if set, becomes un-disabled if everyone stops touching it */
var() bool bReactivateOnUntouch;
/** if false, objective is not shown on the map */
var() bool bShowOnMap;
/** whether this node can be attacked when the controlling node is not powered by the enemy */
var() bool bAttackableWhenNodeShielded;
var		bool	bActive;
var		repnotify bool	bDisabled;			// true when objective has been completed/disabled/reached/destroyed

var		bool	bIsCritical;				// Set when Attackers are located in the 'Critical Volume'
var		bool	bOldCritical;
var		bool	bPlayCriticalAssaultAlarm;

/** announcement played when this objective is attacked */
var(Announcements) ObjectiveAnnouncementInfo UnderAttackAnnouncement;
/** announcement played when this objective is disabled */
var(Announcements) ObjectiveAnnouncementInfo DisabledAnnouncement;
/** if set, announcements only sent to defending team */
var(Announcements) bool bOnlyAnnounceToDefense;

var PlayerReplicationInfo	DisabledBy;

var float		LastWarnTime;

/** class of pawn that must be used to complete this objective */
var() class<Pawn> RequiredPawnClass<AllowAbstract>;

/** for touch objectives, how long it must be touched before it is considered complete */
var(Touch) float HoldTime;
/** pawns currently touching the objective */
var array<Pawn> Touchers;

replication
{
	if ( (Role==ROLE_Authority) && bNetDirty )
		bDisabled, bActive, bIsCritical;
}

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();

	if (Role == ROLE_Authority)
	{
		SetActive(bInitiallyActive);
	}
	if (ControllingNode != None)
	{
		ControllingNode.AddActivatedObjective(self);
	}
}

// Called after PostBeginPlay.
//
simulated event SetInitialState()
{
	bScriptInitialized = true;

	switch ( ObjectiveType )
	{
		case OBJ_Destroyable:
			GotoState('DestroyableObjective');
			break;
		case OBJ_Touch:
			GotoState('TouchObjective');
			break;
		default:
			`Warn("Unknown objective type" @ ObjectiveType);
			break;
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'bDisabled' )
	{
		SetDisabled(bDisabled);
	}
	else
	{
		super.ReplicatedEvent(VarName);
	}
}

function bool ValidSpawnPointFor(byte TeamIndex)
{
	return false;
}

// node that activates this objective will call this function after a state transition.
// this objective decides based on that whether to activate or deactivate
function CheckActivate()
{
	if (ControllingNode.IsActive())
	{
		// activate!
		SetActive(true);
		DefenderTeamIndex = ControllingNode.DefenderTeamIndex;
	}
	else
	{
		// deactivate!
		SetActive(false);
		DefenderTeamIndex = 255;
	}
}

function Reset()
{
	Super.Reset();

	SetInitialState();
	SetActive(bInitiallyActive);
	if (ControllingNode != None)
	{
		CheckActivate();
	}
	DisabledBy = None;
}

function SetActive( bool bActiveStatus )
{
	bActive = bActiveStatus;
	NetUpdateTime = WorldInfo.TimeSeconds - 1;
}

simulated event bool IsActive()
{
	return (bActive && !bDisabled);
}

/* returns objective's progress status 1->0 (=disabled) */
simulated function float GetObjectiveProgress()
{
	return float(!bDisabled);
}

simulated event bool IsCritical()
{
	return IsActive() && bIsCritical;
}

function SetCriticalStatus( bool bNewCriticalStatus )
{
	bIsCritical = bNewCriticalStatus;
	CheckPlayCriticalAlarm();
}

function CheckPlayCriticalAlarm()
{
	local bool bNewCritical;

	if ( !bPlayCriticalAssaultAlarm )
		return;

	bNewCritical = IsCritical();
	if ( bOldCritical != bNewCritical )
	{
		if ( bNewCritical )
		{
			// Only set alarm if objective is currently relevant
			if ( UTGame(WorldInfo.Game).CanDisableObjective( Self ) )
			{
				bOldCritical = bNewCritical;
			}
		}
		else
		{
			bOldCritical = bNewCritical;
		}
	}
}

function DisableObjective(Controller InstigatorController)
{
	local PlayerReplicationInfo	PRI;

	if (!IsActive() || !UTGame(WorldInfo.Game).CanDisableObjective(self))
	{
		return;
	}

	Instigator = InstigatorController.Pawn;
	NetUpdateTime = WorldInfo.TimeSeconds - 1;

	if ( (InstigatorController != None) && (InstigatorController.PlayerReplicationInfo != None) )
	{
		PRI = InstigatorController.PlayerReplicationInfo;
	}

	BroadcastObjectiveMessage(1);

	if ( bTeamControlled )
	{
		if (PRI != None)
			DefenderTeamIndex = PRI.Team.TeamIndex;
	}
	else
	{
		GotoState('DisabledObjective');
	}

	SetCriticalStatus( false );
	DisabledBy = PRI;
	WorldInfo.Game.ScoreObjective( PRI, Score );

	UTGame(WorldInfo.Game).ObjectiveDisabled( Self );

	TriggerEventClass(class'UTSeqEvent_ObjectiveCompleted', InstigatorController);

	UTGame(WorldInfo.Game).FindNewObjectives( Self );
}

/** @return whether this objective can be disabled by the specified controller */
function bool CanBeDisabledBy(Controller Disabler)
{
	return ( UTGame(WorldInfo.Game).CanDisableObjective(Self) && !WorldInfo.GRI.OnSameTeam(self, Disabler) && Disabler.bIsPlayer &&
		(ControllingNode == None || bAttackableWhenNodeShielded || ControllingNode.PoweredBy(Disabler.GetTeamNum())) &&
		(RequiredPawnClass == None || (Disabler.Pawn != None && ClassIsChildOf(Disabler.Pawn.Class, RequiredPawnClass))) );
}

/** checks if the bot has the pawn class required
 * @param bFoundVehicle (out) - set to 1 if we require a certain vehicle and were able to tell the bot how to get one
 * @return whether the bot's pawn is of the required class
 */
function bool CheckBotPawnClass(UTBot B, out byte bFoundVehicle)
{
	local Vehicle VBase;
	local UTVehicle V, BotVehicle;

	bFoundVehicle = 0;
	if (RequiredPawnClass == None)
	{
		return true;
	}
	else
	{
		BotVehicle = UTVehicle(B.Pawn);
		if (ClassIsChildOf(B.Pawn.Class, RequiredPawnClass))
		{
			// mark vehicle as being necessary for an objective
			if (BotVehicle != None)
			{
				BotVehicle.bKeyVehicle = true;
			}
			return true;
		}
		else
		{
			if (ClassIsChildOf(RequiredPawnClass, class'Vehicle'))
			{
				VBase = B.Pawn.GetVehicleBase();
				if (VBase == None)
				{
					// try to find a vehicle of the required class and enter it
					for (V = UTGame(WorldInfo.Game).VehicleList; V != None; V = V.NextVehicle)
					{
						if ( ClassIsChildOf(V.Class, RequiredPawnClass) && B.Squad.VehicleDesireability(V, B) > 0.0 &&
							B.Squad.GotoVehicle(V, B) )
						{
							if (B.MoveTarget == V && Vehicle(B.Pawn) != None)
							{
								B.LeaveVehicle(false);
							}
							bFoundVehicle = 1;
							return false;
						}
					}

					return false;
				}
				else
				{
					return ClassIsChildOf(VBase.Class, RequiredPawnClass);
				}
			}
			else
			{
				return false;
			}
		}
	}
}

/** broadcasts the requested message index */
function BroadcastObjectiveMessage(int Switch)
{
	if (bOnlyAnnounceToDefense)
	{
		WorldInfo.Game.BroadcastLocalizedTeam(DefenderTeamIndex, self, MessageClass, Switch,,, self);
	}
	else
	{
		WorldInfo.Game.BroadcastLocalized(self, MessageClass, Switch,,, self);
	}
}

function SetUnderAttack(bool bNewUnderAttack)
{
	if (!bUnderAttack && bNewUnderAttack)
	{
		BroadcastObjectiveMessage(0);
	}

	Super.SetUnderAttack(bNewUnderAttack);
}

simulated function RenderLinks(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, float ColorPercent, bool bFullScreenMap, bool bSelected)
{
	if (bShowOnMap)
	{
		Super.RenderLinks(MP, Canvas, PlayerOwner, ColorPercent, bFullScreenMap, bSelected);
	}
}

function OnToggle(SeqAct_Toggle InAction)
{
	local bool bNowActive;

	if (InAction.InputLinks[0].bHasImpulse)
	{
		bNowActive = true;
	}
	else if (InAction.InputLinks[1].bHasImpulse)
	{
		bNowActive = false;
	}
	else if (InAction.InputLinks[2].bHasImpulse)
	{
		bNowActive = !bActive;
	}

	if (bNowActive != bActive)
	{
		SetActive(bNowActive);
		if (!bNowActive)
		{
			UTGame(WorldInfo.Game).FindNewObjectives(self);
		}
	}
}

function OnSetBotsMustComplete(UTSeqAct_SetBotsMustComplete Action)
{
	local UTSquadAI S;
	local UTTeamGame Game;
	local int i;

	if (Action.InputLinks[0].bHasImpulse)
	{
		bMustCompleteToAttackNode = true;
		// tell all bots to reevaluate their objective selection
		UTGame(WorldInfo.Game).FindNewObjectives(None);
	}
	else if (Action.InputLinks[1].bHasImpulse)
	{
		bMustCompleteToAttackNode = false;
		// since this objective isn't necessary anymore, make sure bots attacking us consider switching to something else
		Game = UTTeamGame(WorldInfo.Game);
		if (Game != None)
		{
			for (i = 0; i < ArrayCount(Game.Teams); i++)
			{
				for (S = Game.Teams[i].AI.Squads; S != None; S = S.NextSquad)
				{
					if (S.SquadObjective == self)
					{
						S.SquadObjective = None;
						Game.Teams[i].AI.FindNewObjectiveFor(S, true);
					}
				}
			}
		}
	}
}

simulated function bool ValidTargetFor(Controller C)
{
	if ( (C.PlayerReplicationInfo != None) && (C.PlayerReplicationInfo.Team != None) 
		&& (C.PlayerReplicationInfo.Team.TeamIndex == DefenderTeamIndex) )
	{
		return false;
	}
	return ( (DamageCapacity > 0) && bActive && !bDisabled );
}

// used, destroyed, bunker triggered, or keyed

State DestroyableObjective
{
	/* returns objective's progress status 1->0 (=disabled) */
	simulated function float GetObjectiveProgress()
	{
		if ( bDisabled )
			return 0;
		return Health/DamageCapacity;
	}

	/* Reset()
	reset actor to initial state - used when restarting level without reloading.
	*/
	function Reset()
	{
		Health = DamageCapacity;
		global.Reset();
	}

	function bool NearObjective(Pawn P)
	{
		if ( P.CanAttack(self) )
			return true;
		return Global.NearObjective(P);
	}

	function bool Shootable()
	{
		return true;
	}

	simulated function bool TeamLink(int TeamNum)
	{
		return ( LinkHealMult > 0 && (DefenderTeamIndex == TeamNum) );
	}

	simulated function bool NeedsHealing()
	{
		return (Health < DamageCapacity);
	}

	function Actor GetShootTarget()
	{
		return self;
	}

	function bool LegitimateTargetOf(UTBot B)
	{
		return ValidTargetFor(B);
	}

	/* DestroyableObjectives are in danger when CriticalVolume is breached or Objective is damaged
		(In case Objective can be damaged from a great distance */
	simulated event bool IsCritical()
	{
		return (bIsCritical || bUnderAttack);
	}

	/* TellBotHowToDisable()
	tell bot what to do to disable me.
	return true if valid/useable instructions were given
	*/
	function bool TellBotHowToDisable(UTBot B)
	{
		local byte bFoundVehicle;

		if (!CheckBotPawnClass(B, bFoundVehicle))
		{
			return bool(bFoundVehicle);
		}

		if ( !B.Pawn.bStationary && B.Pawn.TooCloseToAttack(GetShootTarget()) )
		{
			B.GoalString = "Back off from objective";
			B.RouteGoal = B.FindRandomDest();
			B.MoveTarget = B.RouteCache[0];
			B.SetAttractionState();
			return true;
		}
		else if ( B.CanAttack(GetShootTarget()) )
		{
			// FIXMESTEVE - unless kill enemy first
			B.GoalString = "Attack Objective";
			B.DoRangedAttackOn(GetShootTarget());
			return true;
		}

		MarkShootSpotsFor(B.Pawn);
		return Global.TellBotHowToDisable(B);
	}

	/* TellBotHowToHeal()
	tell bot what to do to heal me
	return true if valid/useable instructions were given
	*/
	function bool TellBotHowToHeal(UTBot B)
	{
		local UTVehicle OldVehicle;

		if (!TeamLink(B.GetTeamNum()) || Health >= DamageCapacity)
		{
			return false;
		}

		if (B.Squad.SquadObjective == None)
		{
			if (Vehicle(B.Pawn) != None)
			{
				return false;
			}
			// @hack - if bot has no squadobjective, need this for SwitchToBestWeapon() so bot's weapons' GetAIRating()
			// has some way of figuring out bot is trying to heal me
			B.DoRangedAttackOn(self);
		}

		OldVehicle = UTVehicle(B.Pawn);
		if (OldVehicle != None)
		{
			if (!OldVehicle.bKeyVehicle && (B.Enemy == None || (!B.LineOfSightTo(B.Enemy) && WorldInfo.TimeSeconds - B.LastSeenTime > 3)))
			{
				OldVehicle.DriverLeave(false);
			}
			else
			{
				OldVehicle = None;
			}
		}

		if (UTWeapon(B.Pawn.Weapon) != None && UTWeapon(B.Pawn.Weapon).CanHeal(self))
		{
			if (!B.Pawn.CanAttack(GetShootTarget()))
			{
				// need to move to somewhere else near objective
				B.GoalString = "Can't shoot"@self@"(obstructed)";
				B.RouteGoal = B.FindRandomDest();
				B.MoveTarget = B.RouteCache[0];
				B.SetAttractionState();
				return true;
			}
			B.GoalString = "Heal "$self;
			B.DoRangedAttackOn(GetShootTarget());
			return true;
		}
		else
		{
			B.SwitchToBestWeapon();
			if (UTWeapon(B.Pawn.InvManager.PendingWeapon) != None && UTWeapon(B.Pawn.InvManager.PendingWeapon).CanHeal(self))
			{
				if (!B.Pawn.CanAttack(GetShootTarget()))
				{
					// need to move to somewhere else near objective
					B.GoalString = "Can't shoot"@self@"(obstructed)";
					B.RouteGoal = B.FindRandomDest();
					B.MoveTarget = B.RouteCache[0];
					B.SetAttractionState();
					return true;
				}
				B.GoalString = "Heal "$self;
				B.DoRangedAttackOn(GetShootTarget());
				return true;
			}
			if (B.FindInventoryGoal(0.0005)) // try to find a weapon to heal the objective
			{
				B.GoalString = "Find weapon or ammo to heal "$self;
				B.SetAttractionState();
				return true;
			}
		}

		if (OldVehicle != None)
		{
			OldVehicle.UsedBy(B.Pawn);
		}

		return false;
	}

	event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{
		if (Damage <= 0 || !CanBeDisabledBy(InstigatedBy))
			return;

		NetUpdateTime = WorldInfo.TimeSeconds - 1;

		if ( damageType != None)
			Damage *= damageType.default.VehicleDamageScaling;

		if ( (InstigatedBy != None) && (InstigatedBy.Pawn != None) )
		{
			Damage *= InstigatedBy.Pawn.GetDamageScaling();
		}

		Global.TakeDamage(Damage, InstigatedBy, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);

		Health -= Damage;
		if ( InstigatedBy != None )
		{
			// monitor percentage of damage done for score sharing
			if ( UTPlayerReplicationInfo(InstigatedBy.PlayerReplicationInfo) != None )
				AddScorer( UTPlayerReplicationInfo(InstigatedBy.PlayerReplicationInfo), Min(Damage, Health)/DamageCapacity );
		}

		if ( Health < 1 )
			DisableObjective( instigatedBy );
		else
		{
			if ( WorldInfo.TimeSeconds - LastWarnTime > 0.5 )
			{
				LastWarnTime = WorldInfo.TimeSeconds;
				CheckPlayCriticalAlarm();
				if ( (InstigatedBy != None) && (DefenseSquad != None) )
					DefenseSquad.Team.AI.CriticalObjectiveWarning(self, instigatedBy.Pawn);
			}
			SetUnderAttack(true);
		}
	}

	function bool HealDamage(int Amount, Controller Healer, class<DamageType> DamageType)
	{
		if ( !bActive || bDisabled || Health <= 0 || Health >= DamageCapacity || Amount <= 0
			|| Healer == None || !TeamLink(Healer.GetTeamNum()) )
		{
			return false;
		}
		else
		{
			Health = Min(Health + (Amount * LinkHealMult), DamageCapacity);
			NetUpdateTime = WorldInfo.TimeSeconds - 1.f;
			return true;
		}
	}

	function BeginState(Name PreviousStateName)
	{
		bCanBeDamaged = true;
		bProjTarget = true;
		SetCollision(true,bBlockActors);
		Health = DamageCapacity;
	}

	function EndState(Name NextStateName)
	{
		bCanBeDamaged = false;
		bProjTarget = false;
		SetCollision(false,bBlockActors);
	}
}

state TouchObjective
{
	event Touch(Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal)
	{
		local Pawn P;

		P = Pawn(Other);
		if (P != None && P.Controller != None && CanBeDisabledBy(P.Controller) && Touchers.Find(P) == -1)
		{
			Touchers[Touchers.Length] = P;
		}
	}

	event UnTouch(Actor Other)
	{
		local Pawn P;
		local int i;

		P = Pawn(Other);
		if (P != None)
		{
			i = Touchers.Find(P);
			if (i != -1)
			{
				Touchers.Remove(i, 1);
			}
		}

		if (Touchers.length == 0 && bReactivateOnUntouch)
		{
			SetActive(true);
			Health = HoldTime;
			if (bMustCompleteToAttackNode)
			{
				UTGame(WorldInfo.Game).FindNewObjectives(self);
			}
		}
	}

	event Tick(float DeltaTime)
	{
		local int i;
		local UTPlayerReplicationInfo PRI;

		while (i < Touchers.length)
		{
			if (Touchers[i] == None || Touchers[i].Health <= 0)
			{
				Touchers.Remove(i, 1);
			}
			else
			{
				PRI = UTPlayerReplicationInfo(Touchers[i].PlayerReplicationInfo);
				if (PRI != None)
				{
					AddScorer(PRI, DeltaTime / HoldTime);
				}
				i++;
			}
		}

		// decrement timer if someone is still touching
		if (Touchers.length > 0)
		{
			SetUnderAttack(true);
			Health -= DeltaTime;
			if (Health < 0)
			{
				if (bReactivateOnUntouch)
				{
					// this is kinda hacky, but better than duplicating a bunch of code to
					// handle re-activating from a seperate state
					SetActive(false);
					BroadcastObjectiveMessage(1);
					UTGame(WorldInfo.Game).FindNewObjectives(self);
				}
				else
				{
					DisableObjective(Touchers[0].Controller);
				}
			}
		}
	}

	function bool TellBotHowToDisable(UTBot B)
	{
		local UTBot NextBot;
		local byte bFoundVehicle;

		if (!CheckBotPawnClass(B, bFoundVehicle))
		{
			return bool(bFoundVehicle);
		}

		if (VSize(B.Pawn.Location - Location) < 1000.0 && B.LineOfSightTo(self))
		{
			// if teammate already touching, then don't need to
			for (NextBot = B.Squad.SquadMembers; NextBot != None; NextBot = NextBot.NextSquadMember)
			{
				if (NextBot != B && NextBot.Pawn != None && NextBot.Pawn.ReachedDestination(self))
				{
					return false;
				}
			}
		}
		return Global.TellBotHowToDisable(B);
	}

	/* returns objective's progress status 1->0 (=disabled) */
	simulated function float GetObjectiveProgress()
	{
		return (bDisabled) ? 0.0 : (Health / HoldTime);
	}

	/* Reset()
	reset actor to initial state - used when restarting level without reloading.
	*/
	function Reset()
	{
		Health = HoldTime;
		Global.Reset();
	}

	event BeginState(name PreviousStateName)
	{
		SetCollision(true, false);

		Health = HoldTime;
	}

	event EndState(name NextStateName)
	{
		SetCollision(false, false);
	}
}

simulated function SetDisabled(bool bNewValue)
{
	bDisabled = bNewValue;
}

state DisabledObjective
{
	function SetActive( bool bActiveStatus )
	{
		if (bTriggerOnceOnly || !bActiveStatus)
		{
			bActive = false;
		}
		else
		{
			// re-enable
			SetInitialState();
			bActive = true;
			NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
			UTGame(WorldInfo.Game).FindNewObjectives(self);
		}
	}

	function BeginState(Name PreviousStateName)
	{
		bActive = false;
		SetDisabled(true);
	}

	function EndState(Name NextStateName)
	{
		SetDisabled(false);
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollideActors=true
		BlockActors=true
	End Object

	ObjectiveType=OBJ_Destroyable
	bPlayCriticalAssaultAlarm=true
	bShowOnMap=true
	bNotBased=true
	bDestinationOnly=false
	DamageCapacity=1000
	Health=1000
	bAlwaysRelevant=true
	RemoteRole=ROLE_SimulatedProxy

	IconPosX=0.5
	IconPosY=0.25
	IconExtentX=0.125
	IconExtentY=0.25

	MessageClass=class'UTSpecialObjectiveStatusMessage'

	SupportedEvents.Empty
	SupportedEvents(0)=class'UTSeqEvent_ObjectiveCompleted'
	SupportedEvents(1)=class'SeqEvent_TakeDamage'
	SupportedEvents(2)=class'SeqEvent_Touch'
	SupportedEvents(3)=class'SeqEvent_UnTouch'
}
