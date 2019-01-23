/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTGameObjective extends Objective
	abstract
	native
	nativereplication;

var		bool	bIsBeingAttacked;	// temp flag - not always valid
var		bool	bAlreadyRendered;

/** Allow use if player is within Pawn's VehicleCheckRadius */
var		bool	bAllowRemoteUse;

var		repnotify byte	DefenderTeamIndex;	// 0 = defended by team 0
var		byte			StartTeam;

/* Nodes with Higher DefensePriority are defended first */
var()	byte			DefensePriority;	

var UTSquadAI		DefenseSquad;	// squad defending this objective;
var	UTDefensePoint	DefensePoints;

var		localized String	LocationPrefix, LocationPostfix;
var()	localized String	ObjectiveName;

/** bots in vehicles should go to one of these and then proceed on foot */
var() array<NavigationPoint> VehicleParkingSpots;
var() Volume				MyBaseVolume;
var() float							BaseRadius;			

// Score sharing
struct native ScorerRecord
{
	var UTPlayerReplicationInfo PRI;
	var float		Pct;
};
var array<ScorerRecord>	Scorers;

var		int		Score;				// score given to player that completes this objective

var		bool				bFirstObjective;		// First objective in list of objectives defended by same team
var		UTGameObjective		NextObjective;			// list of objectives defended by the same team

var color ControlColor[3];
var vector HUDLocation;
var private transient   MaterialInstanceConstant HUDMaterialInstance, SensorMaterialInstance, AttackMaterialInstance;
var int NodeNumber;									// for HUD map rendering
var float IconPosX, IconPosY;
var float IconExtentX, IconExtentY;
var Material HudMaterial;

var bool bHasSensor;
var float MaxSensorRange, SensorScale;
var float CameraViewDistance;				/** distance away for camera when viewtarget */

var array<UTVehicleFactory> VehicleFactories;

/** list of deployable lockers at this objective */
var array<UTDeployableNodeLocker> DeployableLockers;

var repnotify bool bUnderAttack;

var array<PlayerStart> PlayerStarts;

/** precalculated list of nearby NavigationPoints this objective is shootable from */
var array<NavigationPoint> ShootSpots;
/** if true, allow this objective to be unreachable as long as we could find some ShootSpots for it */
var bool bAllowOnlyShootable;

/** list of teamskinned static meshes that we should notify when our team changes */
var array<UTTeamStaticMesh> TeamStaticMeshes;

/** announcement to use when directing a player to attack this objective */
var(Announcements) ObjectiveAnnouncementInfo AttackAnnouncement;
/** announcement to use when directing a player to defend this objective */
var(Announcements) ObjectiveAnnouncementInfo DefendAnnouncement;

/** Used for highlighting on minimap */
var float HighlightScale;

var float MaxHighlightScale;

var float HighlightSpeed;

var float LastHighlightUpdate;

/** Last time trace test check for drawing postrender beacon was performed */
var float LastPostRenderTraceTime;

/** true is last trace test check for drawing postrender beacon succeeded */
var bool bPostRenderTraceSucceeded;

/** Max distance for drawing beacon for non-critical objective */
var float MaxBeaconDistance;

cpptext
{
	INT* GetOptimizedRepList(BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel);
	virtual void AddForcedSpecs(AScout* Scout);
}

replication
{
	if ( (Role==ROLE_Authority) && bNetDirty )
		DefenderTeamIndex, bUnderAttack;
}

simulated function PostBeginPlay()
{
	local UTGameObjective O, CurrentObjective;
	local PlayerController PC;
	local int i;

	super.PostBeginPlay();
	StartTeam = DefenderTeamIndex;

	// add to objective list
	if ( bFirstObjective )
	{
		CurrentObjective = Self;
		//@note: we have to use AllActors here so if paths haven't been rebuilt we don't get multiple objectives executing this code
		foreach AllActors(class'UTGameObjective', O)
		{
			if (O != self)
			{
				CurrentObjective.NextObjective = O;
				O.bFirstObjective = false;
				CurrentObjective = O;
			}
		}
	}

	if ( Role == Role_Authority )
	{
		StartTeam	= DefenderTeamIndex;

		// find defensepoints
		ForEach WorldInfo.AllNavigationPoints(class'UTDefensePoint', DefensePoints)
			if ( DefensePoints.bFirstScript && (DefensePoints.DefendedObjective == self) )
				break;

		// find AreaVolume
		if ( MyBaseVolume != None )
		{
			MyBaseVolume.AssociatedActor = Self;
		}
	}

	// add to local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
		if ( UTHUD(PC.MyHUD) != None )
			UTHUD(PC.MyHUD).AddPostRenderedActor(self);

	// clear out any empty parking spot entries
	while (i < VehicleParkingSpots.length)
	{
		if (VehicleParkingSpots[i] == None)
		{
			VehicleParkingSpots.Remove(i, 1);
		}
		else
		{
			i++;
		}
	}

	// Add me to the global game objectives list

	if ( UTGameReplicationInfo(WorldInfo.GRI) != none )
	{
		UTGameReplicationInfo(WorldInfo.GRI).AddGameObjective(self);
	}


}

simulated function vector GetHUDOffset(PlayerController PC, Canvas Canvas)
{
	local float Z;

	Z = 550;
	if ( PC.ViewTarget != None )
	{
		Z += 0.02 * VSize(PC.ViewTarget.Location - Location);
	}

	return Z*vect(0,0,1);
}

function BetweenEndPointFor(Pawn P, out actor MainLocationHint, out actor SecondaryLocationHint)
{
	MainLocationHint = None;
	SecondaryLocationHint = None;
}

simulated function string GetLocationStringFor(PlayerReplicationInfo PRI)
{
	return LocationPrefix$GetHumanReadableName()$LocationPostfix;
}

simulated function PrecacheAnnouncer(UTAnnouncer Announcer)
{
	if (AttackAnnouncement.AnnouncementSound != None)
	{
		Announcer.PrecacheSound(AttackAnnouncement.AnnouncementSound);
	}
	if (DefendAnnouncement.AnnouncementSound != None)
	{
		Announcer.PrecacheSound(DefendAnnouncement.AnnouncementSound);
	}
}

/** @return the actor that the given player should use to complete this objective */
function Actor GetAutoObjectiveActor(UTPlayerController PC)
{
	return self;
}

simulated function Destroyed()
{
	local PlayerController PC;

	Super.Destroyed();

	// remove from local HUD's post-rendered list
	ForEach LocalPlayerControllers(class'PlayerController', PC)
		if ( UTHUD(PC.MyHUD) != None )
			UTHUD(PC.MyHUD).RemovePostRenderedActor(self);


	// Remove me from the global game objectives list

	if ( UTGameReplicationInfo(WorldInfo.GRI) != none )
	{
		UTGameReplicationInfo(WorldInfo.GRI).RemoveGameObjective(self);
	}

}

/** adds the given team static mesh to our list and initializes its team */
simulated function AddTeamStaticMesh(UTTeamStaticMesh SMesh)
{
	TeamStaticMeshes[TeamStaticMeshes.length] = SMesh;
	SMesh.SetTeamNum(DefenderTeamIndex);
}

/** updates TeamStaticMeshes array for a change in our team */
simulated function UpdateTeamStaticMeshes()
{
	local int i;

	for (i = 0; i < TeamStaticMeshes.length; i++)
	{
		TeamStaticMeshes[i].SetTeamNum(DefenderTeamIndex);
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'DefenderTeamIndex')
	{
		UpdateTeamStaticMeshes();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/** FindNearestFriendlyNode()
returns nearest node at which team can spawn
*/
function UTGameObjective FindNearestFriendlyNode(int TeamIndex)
{
	return None;
}

/** function used to update where icon for this actor should be rendered on the HUD
 *  @param NewHUDLocation is a vector whose X and Y components are the X and Y components of this actor's icon's 2D position on the HUD
 */
simulated function SetHUDLocation(vector NewHUDLocation)
{
	HUDLocation = NewHUDLocation;
}

function bool UsedBy(Pawn P)
{
	return false;
}

/**
 *	Calculate camera view point, when viewing this actor.
 *
 * @param	fDeltaTime	delta time seconds since last update
 * @param	out_CamLoc	Camera Location
 * @param	out_CamRot	Camera Rotation
 * @param	out_FOV		Field of View
 *
 * @return	true if Actor should provide the camera point of view.
 */
simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	local vector HitNormal, DesiredCamLoc;

	DesiredCamLoc = Location - vector(out_CamRot) * CameraViewDistance;
	if ( Trace(out_CamLoc, HitNormal, DesiredCamLoc, Location, false,vect(12,12,12)) == None )
	{
		out_CamLoc = DesiredCamLoc;
	}

	return false;
}

/** @return the draw color of this objective's map icon */
simulated function color GetIconColor(float ColorPercent)
{
	return (DefenderTeamIndex < ArrayCount(ControlColor)) ? ControlColor[DefenderTeamIndex] : ControlColor[2];
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

	if ( HUDMaterialInstance == None )
	{
		HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		HUDMaterialInstance.SetParent(HUDMaterial);
		HUDMaterialInstance.SetScalarParameterValue('TexRotation', 0);
	}
	YoverX = Canvas.ClipY/Canvas.ClipX;
	HUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(float(Canvas.DrawColor.R)/255,float(Canvas.DrawColor.G)/255,float(Canvas.DrawColor.B)/255,IconAlpha));
	Canvas.SetPos(IconLocation.X - 0.5*IconScale, IconLocation.Y - 0.5*IconScale * YoverX);
	Canvas.DrawMaterialTile(HUDMaterialInstance, IconScale, IconScale * YoverX, IconPosX, IconPosY, IconExtentX, IconExtentY);
}

simulated function DrawAttackIcon(Canvas Canvas, float ColorPercent)
{
	local float AttackScale;

	if ( AttackMaterialInstance == None )
	{
		AttackMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		AttackMaterialInstance.SetParent(HUDMaterial);
		AttackMaterialInstance.SetScalarParameterValue('TexRotation', 0);
	}
	AttackMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(1,1,ColorPercent,1));
	AttackScale = 0.035 * Canvas.ClipX * (1.5 + 0.5*sin(6.0*WorldInfo.TimeSeconds));
	Canvas.SetPos(HUDLocation.X - 0.5*AttackScale, HUDLocation.Y - 0.5*AttackScale);
	Canvas.DrawMaterialTile(AttackMaterialInstance, AttackScale, AttackScale, 0.5, 0.75, 0.25, 0.25);
}

simulated function RenderLinks( UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, float ColorPercent, bool bFullDetail, bool bSelected )
{
	local float CurrentScale;

	// draw self
	if ( HUDMaterial == None )
		return;

	if ( IsUnderAttack() )
	{
		DrawAttackIcon(Canvas, ColorPercent);
	}

	Canvas.DrawColor = GetIconColor(ColorPercent);
	DrawIcon(Canvas, HUDLocation, 16*MP.MapScale, 1.0);

	if ( HighlightScale > 1.0 )
	{
		CurrentScale = (WorldInfo.TimeSeconds - LastHighlightUpdate)/HighlightSpeed;
		HighlightScale = FMax(1.0, HighlightScale - CurrentScale * MaxHighlightScale);
		HUDMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(float(Canvas.DrawColor.R)/255,float(Canvas.DrawColor.G)/255,float(Canvas.DrawColor.B)/255, CurrentScale));
		Canvas.SetPos(HUDLocation.X - 8 * HighlightScale * MP.MapScale, HUDLocation.Y - 8 * HighlightScale * MP.MapScale * Canvas.ClipY / Canvas.ClipX);
		Canvas.DrawMaterialTile(HUDMaterialInstance, 16 * HighlightScale * MP.MapScale, 16 * HighlightScale * MP.MapScale * Canvas.ClipY / Canvas.ClipX, IconPosX, IconPosY, IconExtentX, IconExtentY);
	}

	if ( bFullDetail )
	{
		if (bSelected)
		{
			DrawMapSelection(MP,Canvas,PlayerOwner);
		}

		Canvas.SetDrawColor(255,255,255,255);

		if ( NodeNumber > 0 )
		{
			Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(1);
			Canvas.DrawText( NodeNumber );
		}

/*	-- Remove for Exec review since it's really ugly right now

		if ( bHasSensor && IsActive() )
		{
			// display sensor range
			if ( SensorMaterialInstance == None )
			{
				SensorMaterialInstance = new(Outer) class'MaterialInstanceConstant';
				SensorMaterialInstance.SetParent(HUDMaterial);
				SensorMaterialInstance.SetScalarParameterValue('TexRotation', 0);
			}
			SensorMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(0.05f*float(MP.Canvas.DrawColor.R)/255,0.05f*float(MP.Canvas.DrawColor.G)/255,0.05f*float(MP.Canvas.DrawColor.B)/255,0.01));
			SensorIconSize = SensorScale*MP.MapScale;
			MP.Canvas.SetPos(HUDLocation.X - 0.5*SensorIconSize, HUDLocation.Y - 0.5*SensorIconSize*(MP.Canvas.ClipY/MP.Canvas.ClipX));
			MP.Canvas.DrawMaterialTileClipped(SensorMaterialInstance, SensorIconSize, SensorIconSize*(MP.Canvas.ClipY/MP.Canvas.ClipX), 0.5, 0.125, 0.125, 0.125);
		}
*/
	}
}

simulated function DrawMapSelection( UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner )
{
	Canvas.SetPos(HUDLocation.X - 12 * MP.MapScale, HUDLocation.Y - 12 * MP.MapScale * Canvas.ClipY / Canvas.ClipX);
	Canvas.SetDrawColor(255,255,0,255);
	Canvas.DrawBox(24 * MP.MapScale,24 * MP.MapScale);
}

/** @Returns true if this objective is critical and needs immediate attention */
simulated event bool IsCritical()
{
	return false;
}

simulated function bool IsUnderAttack()
{
	return bUnderAttack;
}

simulated function bool IsNeutral()
{
	return false;
}

simulated event bool IsActive()
{
	return false;
}

simulated event bool IsLocked(int TeamIndex)
{
	return false;
}


function bool Shootable()
{
	return false;
}

function bool TellBotHowToHeal(UTBot B)
{
	return false;
}

simulated function bool TeamLink(int TeamNum)
{
	return false;
}

simulated function bool NeedsHealing()
{
	return false;
}

simulated function byte GetTeamNum()
{
	return DefenderTeamIndex;
}

function bool CanDoubleJump(Pawn Other)
{
	return true;
}

function bool BotNearObjective(UTBot B)
{
	if ( NearObjective(B.Pawn)
		|| ((B.RouteGoal == self) && (B.RouteDist < 2500))
		|| (B.bWasNearObjective && (VSize(Location - B.Pawn.Location) < BaseRadius)) )
	{
		B.bWasNearObjective = true;
		return true;
	}

	B.bWasNearObjective = false;
	return false;
}

function bool NearObjective(Pawn P)
{
	if (MyBaseVolume != None)
	{
		return P.IsInVolume(MyBaseVolume);
	}
	else
	{
		return (VSize(Location - P.Location) < BaseRadius && P.LineOfSightTo(self));
	}
}

simulated function string GetHumanReadableName()
{
	return ObjectiveName;
}

/* TellBotHowToDisable()
tell bot what to do to disable me.
return true if valid/useable instructions were given
*/
function bool TellBotHowToDisable(UTBot B)
{
	return B.Squad.FindPathToObjective(B,self);
}

function int GetNumDefenders()
{
	if ( DefenseSquad == None )
		return 0;
	return DefenseSquad.GetSize();
	// FIXME - max defenders per defensepoint, when all full, report big number
}

function bool BetterObjectiveThan(UTGameObjective Best, byte DesiredTeamNum, byte RequesterTeamNum)
{
	if ( IsDisabled() || (DefenderTeamIndex != DesiredTeamNum) )
		return false;

	if ( (Best == None) || (Best.DefensePriority < DefensePriority) )
		return true;

	return false;
}

/** returns the rating of the highest rated locked vehicle available at this node */
function float GetBestAvailableVehicleRating()
{
	local int i;
	local float BestRating;

	for (i = 0; i < VehicleFactories.length; i++)
	{
		if (VehicleFactories[i].ChildVehicle != None && VehicleFactories[i].ChildVehicle.bTeamLocked)
		{
			BestRating = FMax(BestRating, VehicleFactories[i].ChildVehicle.MaxDesireability);
		}
	}

	return BestRating;
}

/* Reset()
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	super.Reset();

	DefenseSquad		= None;
	DefenderTeamIndex	= StartTeam;
	Scorers.Length		= 0;
	NetUpdateTime		= WorldInfo.TimeSeconds - 1;
}

/** called by UTPlayerController::ClientReset() when restarting level without reloading
 * performs any clientside only actions
 */
simulated function ClientReset();

simulated event bool IsDisabled()
{
	return false;
}

// Score Sharing

/* Keep track of players who contributed in completing the objective to share the score */
function AddScorer( UTPlayerReplicationInfo PRI, float Pct )
{
	local ScorerRecord	S;
	local int			i;

	// Look-up existing entry
	if ( Scorers.Length > 0 )
		for (i=0; i<Scorers.Length; i++)
			if ( Scorers[i].PRI == PRI )
			{
				Scorers[i].Pct += Pct;
				return;
			}

	// Add new entry
	S.PRI		= PRI;
	S.Pct	= Pct;
	Scorers[Scorers.Length] = S;
}

/* Share score between contributors */
function ShareScore( int TotalScore, string EventDesc )
{
	local int	i;
	local float	SharedScore;

	for (i=0; i<Scorers.Length; i++)
	{
		if ( (Scorers[i].PRI == None) || Scorers[i].PRI.bDeleteMe )	// FIXME: obsolete player (left game)
			continue;

		//SharedScore = Round(float(TotalScore) * Scorers[i].Pct);
		SharedScore = float(TotalScore) * Scorers[i].Pct;
		if ( SharedScore > 0 )
		{
			Scorers[i].PRI.Score += SharedScore;
			WorldInfo.Game.ScoreEvent(Scorers[i].PRI, SharedScore, EventDesc);
			WorldInfo.Game.ScoreObjective(Scorers[i].PRI, SharedScore);
		}
	}
}

function SetTeam(byte TeamIndex)
{
	DefenderTeamIndex = TeamIndex;
	UpdateTeamStaticMeshes();
}

/**
 * Returns the actual viewtarget for this actor.  Should be subclassed
 */
event actor GetBestViewTarget()
{
	return self;
}

/** Used by PlayerController.FindGoodView() in RoundEnded State */
simulated function FindGoodEndView(PlayerController PC, out Rotator GoodRotation)
{
	local vector cameraLoc;
	local rotator cameraRot, ViewRotation;
	local int tries;
	local float bestdist, newdist, FOVAngle;

	ViewRotation = GoodRotation;
	ViewRotation.Pitch = 56000;
	tries = 0;
	bestdist = 0.0;
	for (tries=0; tries<16; tries++)
	{
		cameraLoc = Location;
		cameraRot = ViewRotation;
		CalcCamera( 0, cameraLoc, cameraRot, FOVAngle );
		newdist = VSize(cameraLoc - Location);
		if (newdist > bestdist)
		{
			bestdist = newdist;
			GoodRotation = cameraRot;
		}
		ViewRotation.Yaw += 4096;
	}
}

/**
 * Used for a notification chain when an objective changes
 */
function ObjectiveChanged();

/**
 * Will attempt to teleport a pawn to this objective
 */

function bool TeleportTo(UTPawn Traveler)
{
	return false;
}

function bool ValidSpawnPointFor(byte TeamIndex)
{
    return ( (DefenderTeamIndex == TeamIndex) && !bUnderAttack );
}

/** returns the UTCarriedObject (if any) associated with this objective */
function UTCarriedObject GetFlag();

/** turns on or off the alarm sound played when under attack */
function SetAlarm(bool bNowOn);

/** triggers all UTSeqEvent_FlagEvent attached to this objective with the given flag event type */
function TriggerFlagEvent(name EventType, Controller EventInstigator)
{
	local UTSeqEvent_FlagEvent FlagEvent;
	local int i;

	for (i = 0; i < GeneratedEvents.length; i++)
	{
		FlagEvent = UTSeqEvent_FlagEvent(GeneratedEvents[i]);
		if (FlagEvent != None)
		{
			FlagEvent.Trigger(EventType, EventInstigator);
		}
	}
}

/** mark NavigationPoints the given Pawn can shoot this objective from as endpoints for pathfinding
 * this is so that the AI can figure out how to get in range of objectives that are shootable but not reachable
 */
simulated function MarkShootSpotsFor(Pawn P)
{
	local float Range;
	local int i;

	if (P.Weapon != None)
	{
		Range = P.Weapon.MaxRange();
		for (i = 0; i < ShootSpots.length; i++)
		{
			if (ShootSpots[i] != None && VSize(ShootSpots[i].Location - Location) < Range)
			{
				ShootSpots[i].bTransientEndPoint = true;
			}
		}
	}
}

/** returns whether the given Pawn has reached one of our VehicleParkingSpots */
function bool ReachedParkingSpot(Pawn P)
{
	local int i;

	for (i = 0; i < VehicleParkingSpots.length; i++)
	{
		if (P.ReachedDestination(VehicleParkingSpots[i]))
		{
			return true;
		}
	}

	return false;
}

simulated function name GetHudMapStatus()
{
	if ( IsUnderAttack() )
	{
		return 'UnderAttack';
	}
	return '';
}



defaultproperties
{
	bHasSensor=false
	Score=5
	BaseRadius=+2000.0
	bReplicateMovement=false
	bOnlyDirtyReplication=true
	bMustBeReachable=true
	bFirstObjective=true
	NetUpdateFrequency=1
	DefenderTeamIndex=2

	ControlColor(0)=(R=255,G=0,B=0,A=255)
	ControlColor(1)=(R=0,G=0,B=255,A=255)
	ControlColor(2)=(R=255,G=255,B=255,A=255)

	HudMaterial=Material'UI_HUD.Icons.M_UI_HUD_Icons01'
	NodeNumber=-1
	MaxSensorRange=2500.0

	CameraViewDistance=400.0

	SupportedEvents.Add(class'UTSeqEvent_FlagEvent')

	MaxHighlightScale=8.0
	HighlightSpeed=10.0
}
