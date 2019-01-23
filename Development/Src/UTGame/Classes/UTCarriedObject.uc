/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCarriedObject extends Actor
    native abstract notplaceable
    dependson(UTPlayerController);

var const NavigationPoint LastAnchor;		// recent nearest path
var		float	LastValidAnchorTime;	// last time a valid anchor was found

var repnotify bool bHome;
var bool			bLastSecondSave;

var repnotify UTPlayerReplicationInfo HolderPRI;
var Pawn      Holder;

var UTGameObjective   HomeBase;
var float           TakenTime;
var float           MaxDropTime;
var Controller FirstTouch;			// Who touched this objective first
var array<Controller> Assists;		// Who touches it after

// HUD Rendering
var MaterialInstanceConstant HUDMaterialInstance;
var MaterialInstanceConstant EnemyHUDMaterialInstance;
var vector HUDLocation;
var float MapSize;
var float IconXStart;
var float IconYStart;
var float IconXWidth;
var float IconYWidth;

var name 	GameObjBone3P;       /**  Bone to which this carriedobject should be attached in third person */
var vector	GameObjOffset3P;     /**  Offset from attachment bone in third person */
var rotator	GameObjRot3P;        /**  Rotation from attachment bone in third person */
var vector GameObjOffset1P; /** Offset from holder Location in first person */
var rotator GameObjRot1P; /** Offset from holder Rotation in first person */

/** sound to play when we are picked up */
var SoundCue PickupSound;
/** sound to play when we are dropped */
var SoundCue DroppedSound;
/** sound to play when we are sent home */
var SoundCue ReturnedSound;

var Pawn 			OldHolder;
var PointLightComponent FlagLight;
var float			DefaultRadius, DefaultHeight;

var repnotify UTTeamInfo 		Team;

/** announcements used when telling player to pick this object up */
var array<ObjectiveAnnouncementInfo> NeedToPickUpAnnouncements;

// Keep track of our base-most actor.
var Actor	OldBase;
var Actor	OldBaseBase;

/** Used for highlighting on minimap */
var float HighlightScale;

var float MaxHighlightScale;

var float HighlightSpeed;

var float LastHighlightUpdate;

var bool bUseTeamColorForIcon;

cpptext
{
	virtual void PostNetReceiveBase(AActor* NewBase);

	/*
	 * Route finding notifications (sent to target)
	 */
	virtual ANavigationPoint* SpecifyEndAnchor(APawn* RouteFinder);
	virtual void NotifyAnchorFindingResult(ANavigationPoint* EndAnchor, APawn* RouteFinder);
	virtual void TickSpecial(FLOAT DeltaSeconds);
}

replication
{
    if (Role == ROLE_Authority)
		bHome, HolderPRI, Team, HomeBase;
}

// Initialization
function PostBeginPlay()
{
    HomeBase = UTGameObjective(Owner);
    SetOwner(None);

    Super.PostBeginPlay();

	if ( CylinderComponent(CollisionComponent) != None )
	{
		DefaultRadius = CylinderComponent(CollisionComponent).CollisionRadius;
		DefaultHeight = CylinderComponent(CollisionComponent).CollisionHeight;
	}
}

simulated function PrecacheAnnouncer(UTAnnouncer Announcer)
{
	local int i;

	for (i = 0; i < NeedToPickUpAnnouncements.length; i++)
	{
		Announcer.PrecacheSound(NeedToPickUpAnnouncements[i].AnnouncementSound);
	}
}

/** returns true if should be rendered for passed in player */
simulated function bool ShouldMinimapRenderFor(PlayerController PC)
{
	return true;
}

/** function used to update where icon for this actor should be rendered on the HUD
 *  @param NewHUDLocation is a vector whose X and Y components are the X and Y components of this actor's icon's 2D position on the HUD
 */
simulated function SetHUDLocation(vector NewHUDLocation)
{
	HUDLocation = NewHUDLocation;
}

simulated function HighlightOnMinimap(int Switch)
{
	if ( HighlightScale < 1.25 )
	{
		HighlightScale = MaxHighlightScale;
		LastHighlightUpdate = WorldInfo.TimeSeconds;
	}
}

simulated function DrawIcon(Canvas Canvas, vector IconLocation, float IconScale, float IconAlpha)
{
	local float YoverX;
	local LinearColor FlagIconColor;

	if ( bUseTeamColorForIcon )
	{
		FlagIconColor = (Team.TeamIndex == 0) ? MakeLinearColor(1,0,0,IconAlpha) : MakeLinearColor(0,0,1,IconAlpha);
	}
	else
	{
		FlagIconColor = MakeLinearColor(1,1,0,IconAlpha);
	}
	HUDMaterialInstance.SetVectorParameterValue('HUDColor', FlagIconColor);

	YoverX = Canvas.ClipY/Canvas.ClipX;
	Canvas.SetPos(IconLocation.X - 0.5*IconScale, IconLocation.Y - 0.5*IconScale * YoverX);
	Canvas.DrawMaterialTile(HUDMaterialInstance, IconScale, IconScale * YoverX, IconXStart, IconYStart, IconXWidth, IconYWidth);
}

simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner)
{
	local float CurrentScale;
	local LinearColor FlagIconColor;

	if ( HUDMaterialInstance == None )
	{
		HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		HUDMaterialInstance.SetParent(MP.HUDIcons);
		HUDMaterialInstance.SetScalarParameterValue('TexRotation', 0);
		if ( bUseTeamColorForIcon )
		{
			FlagIconColor = (Team.TeamIndex == 0) ? MakeLinearColor(1,0,0,1) : MakeLinearColor(0,0,1,1);
		}
		else
		{
			FlagIconColor = MakeLinearColor(1,1,0,1);
		}
		HUDMaterialInstance.SetVectorParameterValue('HUDColor', FlagIconColor);
	}

	DrawIcon(Canvas, HUDLocation, MapSize * MP.MapScale, 1.0);

	if ( HighlightScale > 1.0 )
	{
		CurrentScale = (WorldInfo.TimeSeconds - LastHighlightUpdate)/HighlightSpeed;
		HighlightScale = FMax(1.0, HighlightScale - CurrentScale * MaxHighlightScale);
		HUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(1,1,0,CurrentScale));
		Canvas.SetPos(HUDLocation.X - 0.5*MapSize * HighlightScale * MP.MapScale, HUDLocation.Y - 0.5*MapSize * HighlightScale * MP.MapScale * Canvas.ClipY / Canvas.ClipX);
		Canvas.DrawMaterialTile(HUDMaterialInstance, MapSize * HighlightScale * MP.MapScale, MapSize * HighlightScale * MP.MapScale * Canvas.ClipY / Canvas.ClipX, IconXStart, IconYStart, IconXWidth, IconYWidth);
	}
}

simulated function RenderEnemyMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, UTGameObjective NearbyObjective)
{
	local float FlashScale, CurrentScale;

	if ( EnemyHUDMaterialInstance == None )
	{
		EnemyHUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		EnemyHUDMaterialInstance.SetParent(MP.HUDIcons);
		EnemyHUDMaterialInstance.SetScalarParameterValue('TexRotation', 0);
	}
	FlashScale = 0.5*abs(cos(2*WorldInfo.TimeSeconds));
	if ( Team.TeamIndex == 0 )
		EnemyHUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(1,FlashScale,0,1));
	else
		EnemyHUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(0,FlashScale,1,1));

	Canvas.SetPos(HUDLocation.X - 0.5*MapSize* MP.MapScale, HUDLocation.Y - 0.5*MapSize* MP.MapScale * Canvas.ClipY / Canvas.ClipX);
	Canvas.DrawMaterialTile(EnemyHUDMaterialInstance, MapSize * MP.MapScale, MapSize * MP.MapScale * Canvas.ClipY / Canvas.ClipX, IconXStart, IconYStart, IconXWidth, IconYWidth);

	if ( HighlightScale > 1.0 )
	{
		CurrentScale = (WorldInfo.TimeSeconds - LastHighlightUpdate)/HighlightSpeed;
		HighlightScale = FMax(1.0, HighlightScale - CurrentScale * MaxHighlightScale);
		if ( Team.TeamIndex == 0 )
			EnemyHUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(1,FlashScale,0,CurrentScale));
		else
			EnemyHUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(0,FlashScale,1,CurrentScale));
		Canvas.SetPos(HUDLocation.X - 0.5*MapSize * HighlightScale * MP.MapScale, HUDLocation.Y - 0.5*MapSize * HighlightScale * MP.MapScale * Canvas.ClipY / Canvas.ClipX);
		Canvas.DrawMaterialTile(HUDMaterialInstance, MapSize * HighlightScale * MP.MapScale, MapSize * HighlightScale * MP.MapScale * Canvas.ClipY / Canvas.ClipX, IconXStart, IconYStart, IconXWidth, IconYWidth);
	}
}

/** GetTeamNum()
* returns teamindex of team with which this UTCarriedObject is associated.
*/
simulated function byte GetTeamNum()
{
	return Team.TeamIndex;
}

// State transitions
function SetHolder(Controller C)
{
	local int i;
	local UTPlayerController PC;

	//`log(self$" setholder c="$c,, 'GameObject');
	LogTaken(c);
	Holder = C.Pawn;
	if ( UTPawn(Holder) != None )
	{
		UTPawn(Holder).DeactivateSpawnProtection();
	}
	HolderPRI = UTPlayerReplicationInfo(Holder.PlayerReplicationInfo);
	HolderPRI.SetFlag(self);
	HolderPRI.NetUpdateTime = WorldInfo.TimeSeconds - 1;

	GotoState('Held');

	// AI Related
	C.MoveTimer = -1;
	Holder.MakeNoise(2.0);

	PC = UTPlayerController(C);
	if (PC != None)
	{
		PC.CheckAutoObjective(true);
	}
	ForEach WorldInfo.AllControllers(class'UTPlayerController', PC)
	{
		if ( PC.LastAutoObjective == self )
		{
			PC.CheckAutoObjective(true);
		}
	}

	// Track First Touch
	if (FirstTouch == None)
		FirstTouch = C;

	// Track Assists
	for (i=0;i<Assists.Length;i++)
		if (Assists[i] == C)
		  return;

	Assists.Length = Assists.Length+1;
  	Assists[Assists.Length-1] = C;

}

function Score()
{
	GetKismetEventObjective().TriggerFlagEvent('Captured', Holder != None ? Holder.Controller : None);
	//`log(self$" score holder="$holder,, 'GameObject');

	// Don't return the flag if the game has ended
	if ( !WorldInfo.Game.bGameEnded )
	{
		Disable('Touch');
		SetLocation(HomeBase.Location);
		SetRotation(HomeBase.Rotation);
		CalcSetHome();
		GotoState('Home');
	}
}

/** called to drop the flag
 */
function Drop()
{
	local UTPlayerController PC;

	OldHolder = Holder;
	HomeBase.ObjectiveChanged();
	RotationRate.Yaw = Rand(200000) - 100000;
	RotationRate.Pitch = Rand(200000 - Abs(RotationRate.Yaw)) - 0.5 * (200000 - Abs(RotationRate.Yaw));

	if ( (OldHolder != None) && (OldHolder.health > 0) )
	{
		Velocity = 0.5 * Holder.Velocity;
		if ( Holder.Health > 0 )
			Velocity += 300*vector(Holder.Rotation) + 100 * (0.5 + FRand()) * VRand();
	}
	Velocity.Z = 250;
	if ( PhysicsVolume.bWaterVolume )
		Velocity *= 0.5;

	//`log(self$" drop holder="$holder,, 'GameObject');
	BaseBoneName = '';
	BaseSkelComponent = None;

	SetLocation(Holder.Location);
	LogDropped(Holder.Controller);
	GotoState('Dropped');

	ForEach WorldInfo.AllControllers(class'UTPlayerController', PC)
	{
		PC.CheckAutoObjective(true);
	}
}

/** called to send the flag to its home base
 * @param Returner the player responsible for returning the flag (may be None)
 */
function SendHome(Controller Returner)
{
	local UTPlayerController PC;

	CalcSetHome();
	LogReturned(Returner);
	PlaySound(ReturnedSound);
	GotoState('Home');

	ForEach WorldInfo.AllControllers(class'UTPlayerController', PC)
	{
		if ( PC.LastAutoObjective == self )
		{
			PC.CheckAutoObjective(true);
		}
	}
}

/** called when a Kismet action returns the flag */
function KismetSendHome()
{
	UTGame(WorldInfo.Game).GameEvent('flag_returned_timeout', GetTeamNum(), None);
	BroadcastReturnedMessage();
	SendHome(None);
}

function BroadcastReturnedMessage()
{
	BroadcastLocalizedMessage(MessageClass, 3 + 7 * GetTeamNum(), None, None, Team);
}

function BroadcastDroppedMessage()
{
	BroadcastLocalizedMessage(MessageClass, 2 + 7 * GetTeamNum(), HolderPRI, None, Team);
}

function BroadcastTakenFromBaseMessage(Controller EventInstigator)
{
	BroadcastLocalizedMessage(MessageClass, 6 + 7 * GetTeamNum(), EventInstigator.PlayerReplicationInfo, None, Team);
}

function BroadcastTakenDroppedMessage(Controller EventInstigator)
{
	BroadcastLocalizedMessage(MessageClass, 4 + 7 * GetTeamNum(), EventInstigator.PlayerReplicationInfo, None, Team);
}

// Helper funcs
protected function CalcSetHome()
{
	local Controller c;

	// AI Related
	foreach WorldInfo.AllControllers(class'Controller', C)
	{
		if (c.MoveTarget == self)
		{
			c.MoveTimer = -1.0;
		}
	}

	OldHolder = None;

	// Reset the assists and First Touch
	FirstTouch = None;
	while (Assists.Length!=0)
	  Assists.Remove(0,1);
}

function ClearHolder()
{
	local int i;
	local UTGameReplicationInfo GRI;
	local UTPlayerReplicationInfo PRI;

	if (Holder == None)
		return;

	if ( Holder.PlayerReplicationInfo == None )
	{
		GRI = UTGameReplicationInfo(WorldInfo.Game.GameReplicationInfo);
		for (i=0; i<GRI.PRIArray.Length; i++)
		{
			PRI = UTPlayerReplicationInfo(GRI.PRIArray[i]);
			if ( PRI.GetFlag() == self )
			{
				PRI.SetFlag(None);
				PRI.NetUpdateTime = WorldInfo.TimeSeconds - 1;
			}
		}
	}
	else
	{
		UTPlayerReplicationInfo(Holder.PlayerReplicationInfo).SetFlag(None);
		Holder.PlayerReplicationInfo.NetUpdateTime = WorldInfo.TimeSeconds - 1;
	}
	Holder = None;
	HolderPRI = None;
}

function Actor Position()
{
	if (Holder != None)
	{
		return Holder;
	}
	else if (bHome)
	{
		return HomeBase;
	}
	else
	{
		return self;
	}
}

function bool ValidHolder(Actor other)
{
	local Pawn p;
	local UTPawn UTP;
	local UTBot B;

	p = Pawn(other);
	if ( p == None || p.Health <= 0 || !p.bCanPickupInventory || !p.IsPlayerPawn() || Vehicle(p.base) != None || p == OldHolder
		|| (UTVehicle(p) != None && !UTVehicle(p).bCanCarryFlag) )
	{
		return false;
	}

	// feigning death pawns can't pick up flags
	UTP = UTPawn(Other);
	if (UTP != None && UTP.IsInState('FeigningDeath'))
	{
		return false;
	}

	B = UTBot(P.Controller);
	if (B != None)
	{
		B.NoVehicleGoal = None;
	}

	return true;
}

// Events
singular event Touch( Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal )
{
    if (!ValidHolder(Other))
	return;

    SetHolder(Pawn(Other).Controller);
}

simulated event FellOutOfWorld(class<DamageType> dmgType)
{
	if ( Role == ROLE_Authority )
	{
		AutoSendHome();
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bHome')
	{
		if (bHome)
		{
			ClientReturnedHome();
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/** called on the client when the flag is returned */
simulated function ClientReturnedHome()
{
	// things can go a little screwy with replication ordering due to all the base/physics/location changes that go on
	// so we use this to make sure the flag's where it should be clientside
	if (HomeBase != None)
	{
		SetLocation(HomeBase.Location);
		SetRotation(HomeBase.Rotation);
		SetPhysics(PHYS_None);
	}
}

event NotReachableBy(Pawn P)
{
	if ( (Physics != PHYS_Falling) && (WorldInfo.Game.NumBots > 0)
		&& (!UTGame(WorldInfo.Game).bAllowTranslocator || (UTBot(P.Controller) == None) || UTBot(P.Controller).CanUseTranslocator()) )
	{
		SendHome(None);
	}
}

event Landed(vector HitNormal, actor FloorActor)
{
	local UTBot B;
	local rotator NewRot;

	NewRot = Rot(16384,0,0);
	NewRot.Yaw = Rotation.Yaw;
	SetRotation(NewRot);
	OldHolder = None;

	//`log(self$" landed",, 'GameObject');

	// tell nearby bots about this
	foreach WorldInfo.AllControllers(class'UTBot', B)
	{
		if ( B.Pawn != None && B.RouteGoal != self && B.MoveTarget != self
			&& VSize(B.Pawn.Location - Location) < 1600.f && B.LineOfSightTo(self) )
		{
			B.Squad.Retask(B);
		}
	}
}

/** returns the game objective we should trigger Kismet flag events on */
function UTGameObjective GetKismetEventObjective()
{
	return HomeBase;
}

simulated event OnBaseChainChanged();

// Logging
function LogTaken(Controller EventInstigator)
{
	GetKismetEventObjective().TriggerFlagEvent('Taken', EventInstigator);
}

function LogReturned(Controller EventInstigator)
{
	GetKismetEventObjective().TriggerFlagEvent('Returned', EventInstigator);
}

// Logging
function LogDropped(Controller EventInstigator)
{
	GetKismetEventObjective().TriggerFlagEvent('Dropped', EventInstigator);
	if (bLastSecondSave)
	{
		BroadcastLocalizedMessage(class'UTLastSecondMessage', 1, HolderPRI, None, Team);
	}
	else
	{
		BroadcastDroppedMessage();
	}
	bLastSecondSave = false;
	UTGame(WorldInfo.Game).GameEvent('flag_dropped',GetTeamNum(), HolderPRI);
}

function CheckTouching()
{
	local int i;

	for ( i=0; i<Touching.Length; i++ )
		if ( ValidHolder(Touching[i]) )
		{
			SetHolder(Pawn(Touching[i]).Controller);
			return;
		}
}

/** send home without player intervention (timed out, fell out of world, etc) */
function AutoSendHome()
{
	BroadcastReturnedMessage();
	UTGame(WorldInfo.Game).GameEvent('flag_returned_timeout', GetTeamNum(), None);
	SendHome(None);
}

// States
auto state Home
{
	ignores SendHome, KismetSendHome, Score, Drop;

	function LogTaken(Controller EventInstigator)
	{
		Global.LogTaken(EventInstigator);
		BroadcastTakenFromBaseMessage(EventInstigator);
		UTGame(WorldInfo.Game).GameEvent('flag_taken', GetTeamNum(), EventInstigator.PlayerReplicationInfo);
	}

	function Timer()
	{
		if (VSize2D(Location - HomeBase.Location) > 10.0 || Abs(Location.Z - HomeBase.Location.Z) > CylinderComponent(CollisionComponent).CollisionHeight)
		{
			UTGame(WorldInfo.Game).GameEvent('flag_returned_timeout',GetTeamNum(),None);
			BroadcastReturnedMessage();
			`log(self$" Home.Timer: had to sendhome",, 'Error');
			BeginState('');
		}
	}

	function BeginState(Name PreviousStateName)
	{
		Disable('Touch');
		bHome = true;
		SetLocation(HomeBase.Location);
		SetRotation(HomeBase.Rotation);
		SetCollision(true, false);
		Enable('Touch');
	}

	function EndState(Name NextStateName)
	{
		bHome = false;
		TakenTime = WorldInfo.TimeSeconds;
	}

Begin:
	// check if an enemy was standing on the base
	Sleep(0.05);
	CheckTouching();
}

state Held
{
	ignores SetHolder;

	function SendHome(Controller Returner)
	{
		// go through most of the drop code before returning home to make sure everything gets reset properly
		OldHolder = Holder;
		HomeBase.ObjectiveChanged();

		BaseBoneName = '';
		BaseSkelComponent = None;
		SetLocation(Holder.Location);

		GotoState('Dropped');

		Global.SendHome(Returner);
	}

	function KismetSendHome()
	{
		UTGame(WorldInfo.Game).GameEvent('flag_returned_timeout', GetTeamNum(), None);
		BroadcastReturnedMessage();
		SendHome(None);
	}

	function Timer()
	{
		if (Holder == None)
		{
			`Log(self$" Held.Timer: had to sendhome",, 'Error');
			UTGame(WorldInfo.Game).GameEvent('flag_returned_timeout',GetTeamNum(),None);
			BroadcastReturnedMessage();
			SendHome(None);
		}
	}

	function BeginState(Name PreviousStateName)
	{
		UTGameReplicationInfo(WorldInfo.GRI).SetFlagHeldEnemy(GetTeamNum());
		WorldInfo.GRI.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		bCollideWorld = false;
		SetCollision(false, false);
		SetLocation(Holder.Location);
		if ( UTPawn(Holder) != None )
		{
			UTPawn(Holder).HoldGameObject(self);
		}
		NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
		SetTimer(10.0, true);
		HomeBase.ObjectiveChanged();
		if (PickupSound != None)
		{
			PlaySound(PickupSound);
		}
	}

	function EndState(Name NextStateName)
	{
		//`log(self$" held.endstate",, 'GameObject');
		ClearHolder();
		SetBase(None);
		NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
	}
}

/** stubs for Dropped functions in case the state gets exited early due to flag being in invalid location */
function CheckFit();
function CheckPain();

state Dropped
{
	ignores Drop;

	function LogTaken(Controller EventInstigator)
	{
		Global.LogTaken(EventInstigator);
		UTGame(WorldInfo.Game).GameEvent('flag_pickup', GetTeamNum(), EventInstigator.PlayerReplicationInfo);
		BroadcastTakenDroppedMessage(EventInstigator);
	}

	function CheckFit()
	{
		local vector X,Y,Z;

		GetAxes(OldHolder.Rotation, X,Y,Z);
		SetRotation(rotator(-1 * X));
		if ( !SetLocation(OldHolder.Location - 2 * OldHolder.GetCollisionRadius() * X + OldHolder.GetCollisionHeight() * vect(0,0,0.5))
		    && !SetLocation(OldHolder.Location) && (OldHolder.GetCollisionRadius() > 0) )
		{
			SetCollisionSize(FMin(DefaultRadius,0.8 * OldHolder.GetCollisionRadius()), FMin(DefaultHeight, 0.8 * OldHolder.GetCollisionHeight()));
			if ( !SetLocation(OldHolder.Location) )
			{
				//`log(self$" Drop sent flag home",,'Error');
				AutoSendHome();
				return;
			}
		}
	}

	function CheckPain()
	{
		if (IsInPain())
		{
			AutoSendHome();
		}
	}

	event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{
		CheckPain();
	}

	singular function PhysicsVolumeChange( PhysicsVolume NewVolume )
	{
		Global.PhysicsVolumeChange(NewVolume);
		CheckPain();
	}

	function Timer()
	{
		AutoSendHome();
	}

	singular event BaseChange()
	{
		if (Pawn(Base) != None)
		{
			// landing on Pawns is not allowed
			Velocity = 100.0 * VRand();
			Velocity.Z += 200.0;
			SetPhysics(PHYS_Falling);
		}
	}

	function BeginState(Name PreviousStateName)
	{
		if (DroppedSound != None)
		{
			PlaySound(DroppedSound);
		}
		UTGameReplicationInfo(WorldInfo.GRI).SetFlagDown(GetTeamNum());
		WorldInfo.GRI.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		SetTimer(MaxDropTime, false);
		SetPhysics(PHYS_Falling);
		bCollideWorld = true;
		SetCollisionSize(0.5 * DefaultRadius, DefaultHeight);
		SetCollision(true, false);
		CheckFit();
		CheckPain();
	}

	function EndState(Name NextStateName)
	{
		//`log(self$" dropped.endstate",, 'GameObject');
		SetPhysics(PHYS_None);
		NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
		bCollideWorld = false;
		SetCollisionSize(DefaultRadius, DefaultHeight);
		ClearTimer();
	}
Begin:
	// check if an enemy was standing on the flag
	Sleep(0.05);
	CheckTouching();
}

defaultproperties
{
	Physics=PHYS_None
	bOrientOnSlope=true
	RemoteRole=ROLE_SimulatedProxy
	bReplicateMovement=true
	bIgnoreRigidBodyPawns=true

	Begin Object Class=CylinderComponent Name=CollisionCylinder
		CollisionRadius=+0048.000000
		CollisionHeight=+0030.000000
		CollideActors=true
	End Object
	CollisionComponent=CollisionCylinder
	Components.Add(CollisionCylinder)

	MaxDropTime=25.f
	bUpdateSimulatedPosition=true
	bAlwaysRelevant=true

	MapSize=32
	IconXStart=0.75
	IconYStart=0.0
	IconXWidth=0.25
	IconYWidth=0.25

	MaxHighlightScale=8.0
	HighlightSpeed=10.0
}
