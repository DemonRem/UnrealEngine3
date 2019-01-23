/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTOnslaughtFlag extends UTCarriedObject
	native(Onslaught)
	abstract;

var StaticMeshComponent Mesh;
var		Material				FlagMaterials[2];
var		ParticleSystem			FlagEffect[2];
var		ParticleSystemComponent	FlagEffectComp;

var		color					LightColors[2];

/** if a teammate is within this range when the orb would be returned, it halts the timer at zero to give them a final chance to grab it */
var float GraceDist;

var UTOnslaughtGodBeam MyGodBeam;

/** keeps track of starting home base for round resets,
 * as HomeBase changes depending on the closest friendly base when the flag gets returned
 */
var UTOnslaughtFlagBase StartingHomeBase;

var class<UTOnslaughtGodBeam> GodBeamClass;

var float MaxSpringDistance;
var vector OldLocation;

/** teamcolored effect played when we are returned */
var class<UTReplicatedEmitter> ReturnedEffectClasses[2];

/** normal orb scale */
var float NormalOrbScale;

/** scale when viewed by player holding orb and driving hoverboard */
var float HoverboardOrbScale;

/** effect that connects the orb to its holder
 * this is automatically activated in C++ when attached to a Pawn
 */
var ParticleSystemComponent TetherEffect;
/** template for the endpoint of the effect (set to the Pawn's location) */
var name TetherEffectEndPointName;
/** team colored templates for that effect */
var ParticleSystem TetherEffectTemplates[2];

/** How long to wait before building power orb */
var float Prebuildtime, BuildTime;

/** remaining time (in real seconds) orb can be on the ground before it gets returned */
var byte RemainingDropTime;

/** Time when orb building started */
var repnotify float BuildStartTime;

var UTGameObjective LastNearbyObjective;

var float LastIncomingWarning;

var float LastGraceDistance;

/** currently locked node */
var UTOnslaughtPowerNode LockedNode;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

replication
{
	if (bNetDirty)
		RemainingDropTime, LockedNode, BuildStartTime;
}

simulated function PostBeginPlay()
{
	local PlayerController PC;

	Super.PostBeginPlay();

	// add to local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
		if ( UTHUD(PC.MyHUD) != None )
			UTHUD(PC.MyHUD).AddPostRenderedActor(self);
}


/** returns true if should be rendered for passed in player */
simulated function bool ShouldMinimapRenderFor(PlayerController PC)
{
	return (PC.PlayerReplicationInfo.Team == Team);
}

/** If being rebuilt, Draw partial icon to reflect that orb is building */
simulated function DrawIcon(Canvas Canvas, vector IconLocation, float IconScale, float IconAlpha)
{
	local float YoverX, TimerPct;

	if ( (Holder != None) || (RemainingDropTime > 0) )
	{
		BuildStartTime = 0;
	}
	if ( Worldinfo.TimeSeconds - BuildStartTime < PreBuildTime + BuildTime )
	{
		if ( HUDMaterialInstance == None )
		{
			HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
			HUDMaterialInstance.SetParent(class'UTMapInfo'.default.HUDIcons);
			HUDMaterialInstance.SetScalarParameterValue('TexRotation', 0);
		}
		TimerPct = FClamp((Worldinfo.TimeSeconds - BuildStartTime)/(PreBuildTime + BuildTime), 0.0, 1.0);
		YoverX = Canvas.ClipY/Canvas.ClipX;

		// draw low alpha backdrop
		HUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(0.1,0.1,0,0.1*IconAlpha));
		Canvas.SetPos(IconLocation.X - 0.5*IconScale, IconLocation.Y - 0.5*IconScale * YoverX);
		Canvas.DrawMaterialTile(HUDMaterialInstance, IconScale, IconScale * YoverX, IconXStart, IconYStart, IconXWidth, IconYWidth);

		// draw build pct
		HUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(1,1,0,IconAlpha));
		Canvas.SetPos(IconLocation.X - 0.5*IconScale, IconLocation.Y - 0.5*IconScale * YoverX + (1 - TimerPct) * IconScale * YoverX);
		Canvas.DrawMaterialTile(HUDMaterialInstance, IconScale, IconScale * YoverX * TimerPct, IconXStart, IconYStart + (1.0-TimerPct)*IconYWidth, IconXWidth, TimerPct*IconYWidth);
	}
	else
	{
		super.DrawIcon(Canvas, IconLocation, IconScale, IconAlpha);
	}
}

simulated function RenderEnemyMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, UTGameObjective NearbyObjective)
{
	if ( HolderPRI != None
		&& (WorldInfo.TimeSeconds - LastIncomingWarning > 15.0 || LastNearbyObjective == None) )
	{
		LastIncomingWarning = WorldInfo.TimeSeconds;
		PlayerOwner.ReceiveLocalizedMessage(MessageClass, 14, HolderPRI, None, Team);
	}
	LastNearbyObjective = NearbyObjective;

	super.RenderEnemyMapIcon(MP, Canvas, PlayerOwner, NearbyObjective);
}

/**
PostRenderFor()
Hook to allow objectives to render HUD overlays for themselves.
Called only if objective was rendered this tick.
Assumes that appropriate font has already been set
*/
simulated function PostRenderFor(PlayerController PC, Canvas Canvas, vector CameraPosition, vector CameraDir)
{
	local float TextXL, XL, YL, BeaconPulseScale, ScreenOffset, IconYL;
	local vector ScreenLoc, IconLoc;
	local string NodeString;
	local LinearColor TeamColor;
	local Color TextColor;

	// no beacon while held
	if ( HolderPRI != None )
	{
		return;
	}

	// must be in front of player to render HUD overlay
	if ( (CameraDir dot (Location - CameraPosition)) <= 0 )
		return;

	if ( !WorldInfo.GRI.OnSameTeam(self, PC) )
	{
		return;
	}

	ScreenOffset = (RemainingDropTime > 0) ? 75 : 150;
	screenLoc = Canvas.Project(Location + ScreenOffset * vect(0,0,1));
	// make sure not clipped out
	if (screenLoc.X < 0 ||
		screenLoc.X >= Canvas.ClipX ||
		screenLoc.Y < 0 ||
		screenLoc.Y >= Canvas.ClipY)
	{
		return;
	}
	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(2);

	// pulse "key" objective
	BeaconPulseScale = UTPlayerController(PC).BeaconPulseScale;

	class'UTHUD'.Static.GetTeamColor( GetTeamNum(), TeamColor, TextColor);

	NodeString = (RemainingDropTime > 0) ? string(RemainingDropTime) : "ORB";
	Canvas.StrLen(NodeString, TextXL, YL);
	IconYL = BeaconPulseScale * 64.0 *Canvas.ClipX/1024.0;
	XL = FMax(TextXL * BeaconPulseScale, IconYL);
	YL *= BeaconPulseScale;

	// make sure not clipped out
	screenLoc.X = FClamp(screenLoc.X, 0.7*XL, Canvas.ClipX-0.7*XL-1);
	screenLoc.Y = FClamp(screenLoc.Y, +1.8*YL, Canvas.ClipY-1.8*YL-1);

	TeamColor.A = LocalPlayer(PC.Player).GetActorVisibility(bHome ? HomeBase : self)
					? FClamp(4000/VSize(Location - CameraPosition),0.3, 1.0)
					: 0.2;

	// fade if close to crosshair
/*	if (screenLoc.X > 0.45*Canvas.ClipX &&
		screenLoc.X < 0.55*Canvas.ClipX &&
		screenLoc.Y > 0.45*Canvas.ClipY &&
		screenLoc.Y < 0.55*Canvas.ClipY)
	{
		TeamColor.A = TeamColor.A * FMax(FMin(1.0, FMax(0.0,Abs(screenLoc.X - 0.5*Canvas.ClipX) - 0.025*Canvas.ClipX)/(0.025*Canvas.ClipX)), FMin(1.0, FMax(0.0, Abs(screenLoc.Y - 0.5*Canvas.ClipY)-0.025*Canvas.ClipX)/(0.025*Canvas.ClipY)));
		if ( TeamColor.A == 0.0 )
		{
			return;
		}
	}
*/
	class'UTHUD'.static.DrawBackground(ScreenLoc.X-0.7*XL,ScreenLoc.Y- 1.1*YL - 0.5*IconYL,1.4*XL,2.2*YL + IconYL, TeamColor, Canvas);

	Canvas.DrawColor = TextColor;
	Canvas.DrawColor.A = 255.0 * TeamColor.A;
	IconLoc = ScreenLoc;
	IconLoc.Y = IconLoc.Y - 0.25*IconYL;
	DrawIcon(Canvas, IconLoc, IconYL, TeamColor.A);

	Canvas.SetPos(ScreenLoc.X-0.5*TextXL,ScreenLoc.Y);
	Canvas.DrawTextClipped(NodeString, true);
	Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(0);
}

// For onslaught orbs, only send picked up from base messages to same team or if orb visible
function BroadcastTakenFromBaseMessage(Controller EventInstigator)
{
	BroadcastLocalizedTeamMessage(Team.TeamIndex, MessageClass, 6 + 7 * GetTeamNum(), EventInstigator.PlayerReplicationInfo, None, Team);
}

function Reset()
{
	Super.Reset();
	UTOnslaughtFlagBase(HomeBase).myFlag = None;
	HomeBase = StartingHomeBase;
	StartingHomeBase.myFlag = self;
	Global.SendHome(None);
	NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
}

function SetTeam(int TeamIndex)
{
	Team = UTOnslaughtGame(WorldInfo.Game).Teams[TeamIndex];
	Team.TeamFlag = self;
	UpdateTeamEffects();
}

simulated function NotifyLocalPlayerTeamReceived()
{
	UpdateTeamEffects();
}

/** updates shields on nodes when carrier changes since some effects are only for orb carrier */
simulated function UpdateNodeShields()
{
	local UTOnslaughtPowerNode Node;

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		foreach WorldInfo.AllNavigationPoints(class'UTOnslaughtPowerNode', Node)
		{
			if (Node.ControllingFlag != self)
			{
				Node.UpdateShield(Node.PoweredBy(1 - Node.DefenderTeamIndex));
			}
		}
	}
}

/* epic ===============================================
* ::ReplicatedEvent
*
* Called when a variable with the property flag "RepNotify" is replicated
*
* =====================================================
*/
simulated event ReplicatedEvent(name VarName)
{
	local UTOnslaughtFlagBase ONSBase;

	if (VarName == 'Team')
	{
		UpdateTeamEffects();
	}
	else if (VarName == 'BuildStartTime')
	{
		BuildStartTime = WorldInfo.TimeSeconds;
	}
	else
	{
		if (VarName == 'bHome')
		{
			// make sure home base animation is correctly updated
			ONSBase = UTOnslaughtFlagBase(HomeBase);
			if (ONSBase != None)
			{
				ONSBase.OrbHomeStatusChanged();
			}
		}
		else if (VarName == 'HolderPRI')
		{
			UpdateNodeShields();
		}
		Super.ReplicatedEvent(VarName);
	}
}

simulated function ClientReturnedHome()
{
	Super.ClientReturnedHome();

	LastNearbyObjective = None;
}

simulated function UpdateTeamEffects()
{
	local PlayerController PC;

	if ( Team == None )
		return;

	// give flag appropriate color
	if (Team.TeamIndex < 2)
	{
		Mesh.SetMaterial(1,FlagMaterials[Team.TeamIndex]);
		FlagLight.SetLightProperties(, LightColors[Team.TeamIndex]);
		FlagEffectComp.SetTemplate(FlagEffect[Team.TeamIndex]);
		FlagEffectComp.ActivateSystem();

//		TetherEffect.SetTemplate(TetherEffectTemplates[Team.TeamIndex]);
	}

	// god beam only visible to same team
	if ( WorldInfo.GRI != None )
	{
		ForEach LocalPlayerControllers(class'PlayerController', PC)
		{
			if ( WorldInfo.GRI.OnSameTeam(self, PC) )
			{
				if ( MyGodBeam == None )
				{
					// create godbeam;
					MyGodBeam = spawn(GodBeamClass);
					MyGodBeam.SetBase(self);
				}
				return;
			}
		}
	}
	if ( MyGodBeam != None )
		MyGodBeam.Destroy();
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (MyGodBeam != None)
	{
		MyGodBeam.Destroy();
	}
}

simulated function byte GetTeamNum()
{
	if (Team == None && Role == ROLE_Authority)
	{
		Team = UTOnslaughtGame(WorldInfo.Game).Teams[UTOnslaughtFlagBase(HomeBase).GetTeamNum()];
	}
	return ((Team != None) ? Team.TeamIndex : 255);
}

function bool ValidHolder(Actor Other)
{
	local Controller C;

	if ( !Super.ValidHolder(Other) )
	{
		return false;
	}
	C = Pawn(Other).Controller;
	if ( !WorldInfo.GRI.OnSameTeam(self,C) )
	{
		OtherTeamTouch(c);
		return false;
	}

	return true;
}

function SetHolder(Controller C)
{
	Super.SetHolder(C);

	UpdateNodeShields();
}

function ClearHolder()
{
	Super.ClearHolder();

	UpdateNodeShields();
}

function Drop()
{
	Super.Drop();

	// Super clobbers the rotation rate, so put it back
	RotationRate = default.RotationRate;
}

function OtherTeamTouch(Controller C) {}

// States

/** called to send the flag to its home base
 * @param Returner the player responsible for returning the flag (may be None)
 */
function SendHome(Controller Returner)
{
	CalcSetHome();
	LogReturned(Returner);
	PlaySound(ReturnedSound);

	if ( GetTeamNum() < ArrayCount(ReturnedEffectClasses) )
	{
		Spawn(ReturnedEffectClasses[GetTeamNum()]);
	}
	GotoState('Rebuilding');
}

function Score()
{
	GetKismetEventObjective().TriggerFlagEvent('Captured', Holder != None ? Holder.Controller : None);
	//`log(self$" score holder="$holder,, 'GameObject');
	Disable('Touch');
	SetLocation(HomeBase.Location);
	SetRotation(HomeBase.Rotation);
	CalcSetHome();
	GotoState('Rebuilding');
}

function DoBuildOrb()
{
	`log("DoBuildOrb() called in  "$GetStateName());
}

function OrbBuilt()
{
	`log("OrbBuilt() called in  "$GetStateName());
}

auto state Rebuilding
{
	ignores SendHome, KismetSendHome, Score, Drop;

	function Reset()
	{
		Global.Reset();

		// since our home base might have been changed by Reset(), make sure we're in the right location
		SetLocation(HomeBase.Location + vect(0,0,1));
		SetRotation(HomeBase.Rotation);
		SetBase(HomeBase);
	}

	function DoBuildOrb()
	{
		UTOnslaughtFlagBase(HomeBase).BuildOrb();
		SetTimer(BuildTime, false, 'OrbBuilt');
	}

	function OrbBuilt()
	{
		GotoState('Home');
	}

	function BeginState(Name PreviousStateName)
	{
		LastNearbyObjective = None;
		Disable('Touch');
		bHome = true;
		SetCollision(false);
		SetLocation(HomeBase.Location + vect(0,0,1));
		SetRotation(HomeBase.Rotation);
		SetBase(HomeBase);
		SetHidden(true);
		SetTimer(PrebuildTime, false, 'DoBuildOrb');
		BuildStartTime = WorldInfo.TimeSeconds;
		NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
		UTGameReplicationInfo(WorldInfo.GRI).SetFlagHome(GetTeamNum());
	}

	function EndState(Name NextStateName)
	{
		bHome = false;
		SetHidden(false);
	}
}

state Home
{
	ignores SendHome, KismetSendHome, Score, Drop;

	function Reset()
	{
		Global.Reset();

		// since our home base might have been changed by Reset(), make sure we're in the right location
		SetLocation(HomeBase.Location + vect(0,0,1));
		SetRotation(HomeBase.Rotation);
		SetBase(HomeBase);
	}

	function BeginState(Name PreviousStateName)
	{
		//if ( PreviousStateName != 'Rebuilding' )
		//	scripttrace();

		Super.BeginState(PreviousStateName);
		SetTimer(2.0, true);
		SetLocation(HomeBase.Location + vect(0,0,1));
		SetBase(HomeBase);
		SetHidden(true);
		SetCollision(true);
		HomeBase.ObjectiveChanged();
		HomeBase.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		NetUpdateTime = WorldInfo.TimeSeconds - 1;

		UTGameReplicationInfo(WorldInfo.GRI).SetFlagHome(GetTeamNum());
	}

	function EndState(Name NextStateName)
	{
		Super.EndState(NextStateName);
		UTOnslaughtFlagBase(HomeBase).HideOrb();
		SetTimer(0.0, false);
		SetHidden(false);
		UTGameReplicationInfo(WorldInfo.GRI).SetFlagHeldFriendly(GetTeamNum());
	}
}

/** find the nearest flag base to the given objective on the Onslaught node network */
function UTOnslaughtFlagBase FindNearestFlagBase( UTOnslaughtNodeObjective CurrentNode,
							optional out array<UTOnslaughtNodeObjective> CheckedNodes )
{
	local int i;
	local UTOnslaughtFlagBase Result;

	// avoid too much recursion
	if (CheckedNodes.length > 100)
	{
		return None;
	}
	else
	{
		CheckedNodes[CheckedNodes.length] = CurrentNode;
		// check adjacent nodes for a flag base; if we find one, return it
		for (i = 0; i < ArrayCount(CurrentNode.LinkedNodes); i++)
		{
			if ( CurrentNode.LinkedNodes[i] != None && CurrentNode.LinkedNodes[i].FlagBase != None
				&& CurrentNode.LinkedNodes[i].GetTeamNum() == Team.TeamIndex && CurrentNode.LinkedNodes[i].IsActive() )
			{
				return CurrentNode.LinkedNodes[i].FlagBase;
			}
		}
		// didn't find one, so now ask the nodes if any adjacent to them have a flag base
		for (i = 0; i < ArrayCount(CurrentNode.LinkedNodes); i++)
		{
			if ( CurrentNode.LinkedNodes[i] != None && CurrentNode.LinkedNodes[i].GetTeamNum() == Team.TeamIndex
				&& CheckedNodes.Find(CurrentNode.LinkedNodes[i]) == -1 )
			{
				Result = FindNearestFlagBase(CurrentNode.LinkedNodes[i], CheckedNodes);
				if (Result != None)
				{
					return Result;
				}
			}
		}
	}

	return None;
}

/** sets HomeBase to the nearest friendly Onslaught flag base */
function SetHomeBase()
{
	local UTOnslaughtNodeObjective O, Best;
	local float Dist, BestDist;

	// set homebase to closest line dist to current location
  	foreach WorldInfo.AllNavigationPoints(class'UTOnslaughtNodeObjective', O)
  	{
  		if (O.GetTeamNum() == Team.TeamIndex && O.FlagBase != None && O.IsActive())
  		{
  			Dist = VSize(O.Location - Location);
  			if (Best == None || Dist < BestDist)
	  		{
	  			Best = O;
	  			BestDist = Dist;
	  		}
	  	}
  	}

	UTOnslaughtFlagBase(HomeBase).myFlag = None;
  	if (Best != None)
  	{
  		HomeBase = Best.FlagBase;
  	}
  	else
  	{
  		HomeBase = StartingHomeBase;
  	}
  	UTOnslaughtFlagBase(HomeBase).myFlag = self;
}

function LogDropped(Controller EventInstigator)
{
	local UTOnslaughtPowerNode Node;
	local UTOnslaughtGame Game;

	if (bLastSecondSave)
	{
		Game = UTOnslaughtGame(WorldInfo.Game);
		if (Game != None)
		{
			Node = UTOnslaughtPowerNode(Game.ClosestNodeTo(self));
			if (Node != None && Node.GetTeamNum() < ArrayCount(Game.Teams))
			{
				BroadcastLocalizedMessage(class'UTLastSecondMessage', 1, HolderPRI, None, Game.Teams[1 - GetTeamNum()]);
				bLastSecondSave = false;
			}
		}
	}

	Super.LogDropped(EventInstigator);
}

/** @return whether this flag is at its homebase or reasonably close to it */
function bool IsNearlyHome()
{
	return (bHome || (IsInState('Dropped') && VSize(HomeBase.Location - Location) < 1024.0));
}

state Held
{
	ignores SetHolder;

	function Score()
	{
		SetHomeBase();
		Global.Score();
	}
}


state Dropped
{
	ignores Drop;

	function SendHome(Controller Returner)
	{
		SetHomeBase();

	  	Global.SendHome(Returner);
	}

	function Timer()
	{
		local Controller C;
		local float NewGraceDist;

		RemainingDropTime--;
		if ( (RemainingDropTime < 10) && !IsInPain() )
		{
			// slow down tick if friendly players closing in on orb
			// if a team member is very close, give a short grace period
			NewGraceDist = GraceDist + 1;
			ForEach WorldInfo.AllControllers(class'Controller', C)
			{
				if (C.bIsPlayer && C.Pawn != None && WorldInfo.GRI.OnSameTeam(self, C) )
				{
					NewGraceDist = FMin(NewGraceDist, VSize(C.Pawn.Location - Location));
				}
			}
			if ( NewGraceDist < LastGraceDistance )
			{
				RemainingDropTime = Max(RemainingDropTime, 1);
				SetTimer(2*WorldInfo.TimeDilation, true);
			}
			else
			{
				SetTimer(WorldInfo.TimeDilation, true);
			}
			LastGraceDistance = NewGraceDist;
		}

		if ( RemainingDropTime <= 0 )
		{
			super.Timer();
		}
	}

	function OtherTeamTouch(Controller C)
	{
		UTOnslaughtGame(WorldInfo.Game).ScoreFlag(C, self);
		SendHome(C);
		if ( C.Pawn != None )
		{
			C.Pawn.TakeDamage(100, None, C.Pawn.Location, vect(0,0,80000), class'UTDmgType_OrbReturn');
		}
	}

	event BeginState(name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		if (IsInState('Dropped')) // might have left immediately if we were dropped inside a pain volume, etc
		{
			RemainingDropTime = MaxDropTime / WorldInfo.TimeDilation;
			LastGraceDistance = GraceDist;
			SetTimer(WorldInfo.TimeDilation, true);
		}
	}

	event EndState(name NextStateName)
	{
		Super.EndState(NextStateName);

		RemainingDropTime = 0;
	}
}

// highlight friendly flag if not held
simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner)
{
	Super.RenderMapIcon(MP, Canvas, PlayerOwner);

	if ( (HolderPRI == None) && (PlayerOwner != none) && (self == PlayerOwner.LastAutoObjective) )
	{
		MP.SetCurrentObjective(HudLocation);
	}
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}
