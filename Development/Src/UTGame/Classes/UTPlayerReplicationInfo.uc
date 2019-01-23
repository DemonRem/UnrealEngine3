/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTPlayerReplicationInfo extends PlayerReplicationInfo
	native
	nativereplication
	dependson(UTCustomChar_Data);

var bool bHolding;
var databinding int spree;
var databinding int		MultiKillLevel;
var float		LastKillTime;

var UTLinkedReplicationInfo CustomReplicationInfo;	// for use by mod authors
var UTSquadAI Squad;
var private UTCarriedObject		HasFlag;

//@todo steve var class<VoicePack>	VoiceType;
var string				VoiceTypeName;

var LinearColor			SkinColor;

// special kill type awards
var databinding int		FlakCount;
var databinding int		ComboCount;
var databinding int		HeadCount;
var databinding int		RanOverCount;

var repnotify UTGameObjective StartObjective;
var UTGameObjective TemporaryStartObjective;

var UTPlayerReplicationInfo LastKillerPRI;	/** Used for revenge reward, and for avoiding respawning near killer */

var color DefaultHudColor;

/** Set when pawn with this PRI has been rendered on HUD map, to avoid drawing it twice */
var bool bHUDRendered;

/** Replicated class of pawn controlled by owner of this PRI.  Replicated when drawing HUD Map */
var class<Pawn> HUDPawnClass;

var vector HUDLocation, HUDPawnLocation;
var MaterialInstanceConstant HUDMaterialInstance;

var Actor SecondaryPlayerLocationHint;

var TeamInfo OldTeam;

/** data for the character this player is using */
var repnotify CustomCharData CharacterData;
/** the mesh constructed from the above data */
var SkeletalMesh CharacterMesh;
/** Texture of render of custom character head. */
var	Texture		CharPortrait;
/** Mesh to use for first person arms. Should only be present for local players! */
var SkeletalMesh FirstPersonArmMesh;
/** Material applied to first person arms. Should only be present for local players! */
var MaterialInterface FirstPersonArmMaterial;

/** counter used in native replication to detect whether the server has sent the latest character data to each client
 * should be incremented whenever CharacterData changes
 */
var byte CharacterDataChangeCount;

/**
 * The clan tag for this player
 */
var databinding string ClanTag;

cpptext
{
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
}

replication
{
	if ( Role==ROLE_Authority )
		CustomReplicationInfo, bHolding, Squad, SecondaryPlayerLocationHint;

	if ( bNetOwner && ROLE==ROLE_Authority )
		StartObjective;

	if ( !bNetOwner ) // && UTPlayerController(ReplicationViewer).bViewingMap (if so, in C++ HUDLocationRep = HUDLocation, HUDPawnClass updated too);
		HUDPawnClass, HUDPawnLocation;

	if (bNetDirty) // && CharacterDataChangeCount != Recent->CharacterDataChangeCount
		CharacterData;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( FRand() < 0.8 )
	{
		if ( FRand() < 0.5 )
			SkinColor.R = 1;
		else
			SkinColor.B = 1;
		if ( FRand() < 0.8 )
			SkinColor.G = FRand();
	}
	else
		SkinColor.G = 1;
}

simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController HUDPlayerOwner, LinearColor FinalColor)
{
	local class<UTVehicle> VClass;
	local class<UTPawn> PClass;
	local float MapSize, IconXStart, IconYStart, IconXWidth, IconYWidth;

	if ( HUDPawnClass == None )
	{
		return;
	}
	if ( HUDMaterialInstance == None )
	{
		HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		HUDMaterialInstance.SetParent(MP.HUDIcons);
		HUDMaterialInstance.SetScalarParameterValue('TexRotation', 0);
	}

	PClass = class<UTPawn>(HUDPawnClass);
	if ( PClass != None )
	{
		MapSize = PClass.Default.MapSize;
		IconXStart = PClass.Default.IconXStart;
		IconYStart = PClass.Default.IconYStart;
		IconXWidth = PClass.Default.IconXWidth;
		IconYWidth = PClass.Default.IconYWidth;
	}
	else
	{
		VClass = class<UTVehicle>(HUDPawnClass);
		if ( VClass != None )
		{
			MapSize = VClass.Default.MapSize;
			IconXStart = VClass.Default.IconXStart;
			IconYStart = VClass.Default.IconYStart;
			IconXWidth = VClass.Default.IconXWidth;
			IconYWidth = VClass.Default.IconYWidth;
		}
		else
		{
			`log(PlayerName$" PRI has bad pawn class "$HUDPawnClass);
			return;
		}
	}
	Canvas.SetPos(HUDLocation.X - 0.5*MapSize* MP.MapScale, HUDLocation.Y - 0.5* MapSize* MP.MapScale * Canvas.ClipY / Canvas.ClipX);
	HUDMaterialInstance.SetVectorParameterValue('HUDColor', FinalColor);
	MP.DrawRotatedMaterialTile(Canvas,HUDMaterialInstance, HUDLocation, Rotation.Yaw, MapSize, MapSize*Canvas.ClipY/Canvas.ClipX,IconXStart, IconYStart, IconXWidth, IconYWidth);
}

/** function used to update where icon for this actor should be rendered on the HUD
 *  @param NewHUDLocation is a vector whose X and Y components are the X and Y components of this actor's icon's 2D position on the HUD
 */
simulated function SetHUDLocation(vector NewHUDLocation)
{
	// Don't set HUDLocationRep here, to avoid bnetdirty unless necessary!!!
	HUDLocation = NewHUDLocation;
}

function UpdatePlayerLocation()
{
    local Volume V, BestV;
	local actor Best;
    local Pawn P;
	local UTGameObjective O;
	local float BestDist, NewDist;

    if( Controller(Owner) != None )
	{
		P = Controller(Owner).Pawn;
	}

    if( P == None )
	{
		PlayerLocationHint = None;
		return;
    }

	// first check for volumes
    foreach P.TouchingActors( class'Volume', V )
    {
		if( V.LocationName == "" )
			continue;

		if( (BestV != None) && (V.LocationPriority <= BestV.LocationPriority) )
			continue;

		if( V.Encompasses(P) )
			BestV = V;
	}
	Best = BestV;

	// try game objectives
	if ( Best == None )
	{
		ForEach WorldInfo.AllNavigationPoints(class'UTGameObjective', O)
		{
			NewDist = VSize(P.Location - O.Location);
			if ( (Best == None) || (NewDist < BestDist) )
			{
				Best = O;
				BestDist = NewDist;
			}
		}
		if ( (Best == None) || (BestDist < UTGameObjective(Best).BaseRadius) )
		{
			SecondaryPlayerLocationHint = None;
		}
		else // try to find "between" endpoint
		{
			UTGameObjective(Best).BetweenEndPointFor(P, Best, SecondaryPlayerLocationHint);
		}
	}
	else
	{
		SecondaryPlayerLocationHint = None;
	}

	// @TODO FIXMESTEVE also try powerup pickups
	PlayerLocationHint = (Best != None) ? Best : P.WorldInfo;
}

reliable server function SetStartObjective(UTGameObjective Objective, bool bTemporary)
{
	if ( Objective == None || Objective.ValidSpawnPointFor(Team.TeamIndex))
	{
		// make sure old StartSpot isn't used by the spawning code since it might not be valid for the new start objective
		Controller(Owner).StartSpot = None;
		if (bTemporary)
		{
			TemporaryStartObjective = Objective;
		}
		else
		{
			StartObjective = Objective;
		}
	}
}

function UTGameObjective GetStartObjective()
{
	local UTBot B;
	local UTGameObjective SelectedPC;

	SelectedPC = TemporaryStartObjective;
	TemporaryStartObjective = None;
	if (SelectedPC != None && SelectedPC.ValidSpawnPointFor(Team.TeamIndex))
	{
		return SelectedPC;
	}
	else
	{
		B = UTBot(Owner);
		if (B != None && B.Squad != None)
		{
			return B.Squad.GetStartObjective(B);
		}
		else
		{
			return (StartObjective != None && StartObjective.ValidSpawnPointFor(Team.TeamIndex)) ? StartObjective : None;
		}
	}
}

function SetFlag(UTCarriedObject NewFlag)
{
	HasFlag = NewFlag;
	bHasFlag = (HasFlag != None);
}

function UTCarriedObject GetFlag()
{
	return HasFlag;
}

function LogMultiKills(bool bEnemyKill)
{
	if ( bEnemyKill && (WorldInfo.TimeSeconds - LastKillTime < 4) )
	{
		MultiKillLevel++;
		UTGame(WorldInfo.Game).BonusEvent(Name("MultiKill_x"$String(MultiKillLevel)),self);
	}
	else
		MultiKillLevel=0;

	if ( bEnemyKill )
		LastKillTime = WorldInfo.TimeSeconds;
}

function IncrementSpree()
{
	spree++;
	if ( spree > 4 )
		UTGame(WorldInfo.Game).NotifySpree(self, spree);
}

/* Reset()
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	Super.Reset();
	SetFlag(None);
	Spree = 0;
}

simulated function string GetCallSign()
{
	if ( TeamID > 14 )
		return "";
	return class'UTGame'.default.CallSigns[TeamID];
}

simulated event string GetNameCallSign()
{
	if ( TeamID > 14 )
		return PlayerName;
	return PlayerName$" ["$class'UTGame'.default.CallSigns[TeamID]$"]";
}

/* //@todo steve

function SetCharacterVoice(string S)
{
	local class<VoicePack> NewVoiceType;

	if ( (WorldInfo.NetMode == NM_DedicatedServer) && (VoiceType != None) )
	{
		VoiceTypeName = S;
		return;
	}
	if ( S == "" )
	{
		VoiceTypeName = "";
		return;
	}

	NewVoiceType = class<VoicePack>(DynamicLoadObject(S,class'Class'));
	if ( NewVoiceType != None )
	{
		VoiceType = NewVoiceType;
		VoiceTypeName = S;
	}
}
*/

unreliable server function SendVoiceMessage(PlayerReplicationInfo Recipient, name messagetype, byte messageID, name broadcasttype)
{
/* //@todo steve
	local Controller P;

	if ( ((Recipient == None) || !bBot)	&& !AllowVoiceMessage(MessageType) )
		return;

	foreach WorldInfo.AllControllers(class'Controller', P)
	{
		if ( PlayerController(P) != None )
		{
			if ((P.PlayerReplicationInfo == self) ||
				(P.PlayerReplicationInfo == Recipient &&
				 (WorldInfo.Game.BroadcastHandler == None ||
				  WorldInfo.Game.BroadcastHandler.AcceptBroadcastSpeech(PlayerController(P), Sender)))
				)
				P.ClientVoiceMessage(self, Recipient, messagetype, messageID);
			else if ( (Recipient == None) || (WorldInfo.NetMode == NM_Standalone) )
			{
				if ( (broadcasttype == 'GLOBAL') || !WorldInfo.Game.bTeamGame || (Team == P.PlayerReplicationInfo.Team) )
					if ( WorldInfo.Game.BroadcastHandler == None || WorldInfo.Game.BroadcastHandler.AcceptBroadcastSpeech(PlayerController(P), Sender) )
						P.ClientVoiceMessage(self, Recipient, messagetype, messageID);
			}
		}
		else if ( (messagetype == 'ORDER') && ((Recipient == None) || (Recipient == P.PlayerReplicationInfo)) )
			P.BotVoiceMessage(messagetype, messageID, owner);
	}
*/
}

/* epic ===============================================
* ::OverrideWith
Get overridden properties from old PRI
*/
function OverrideWith(PlayerReplicationInfo PRI)
{
	local UTPlayerReplicationInfo UTPRI;

	Super.OverrideWith(PRI);

	UTPRI = UTPlayerReplicationInfo(PRI);
	if ( UTPRI == None )
		return;

	VoiceTypeName = UTPRI.VoiceTypeName;
	SkinColor = UTPRI.SkinColor;
}

/* epic ===============================================
* ::CopyProperties
Copy properties which need to be saved in inactive PRI
*/
function CopyProperties(PlayerReplicationInfo PRI)
{
	local UTPlayerReplicationInfo UTPRI;

	Super.CopyProperties(PRI);

	UTPRI = UTPlayerReplicationInfo(PRI);
	if ( UTPRI == None )
		return;

	UTPRI.FlakCount = FlakCount;
	UTPRI.ComboCount = ComboCount;
	UTPRI.HeadCount = HeadCount;
	UTPRI.RanOverCount = RanOverCount;
	UTPRI.CustomReplicationInfo = CustomReplicationInfo;
	UTPRI.CharacterData = CharacterData;
	UTPRI.CharacterDataChangeCount++;
	UTPRI.CharacterMesh = CharacterMesh;
}


/**********************************************************************************************
 Teleporting
 *********************************************************************************************/

/**
 * The function is used to setup the conditions that allow a teleport.  It also defers to the gameinfo
 *
 * @Param		DestinationActor 	The actor to teleport to
 * @param		OwnerPawn			returns the pawn owned by the controlling owner casts to UTPawn
 *
 * @returns		True if the teleport is allowed
 */

function bool AllowClientToTeleport(Actor DestinationActor, out UTPawn OwnerPawn)
{
	local Controller OwnerC;

	OwnerC = Controller(Owner);

//	`log("##"@OwnerC@DestinationActor@UTGame(WorldInfo.Game).AllowClientToTeleport(Self, DestinationActor));


	if ( OwnerC != none && DestinationActor != None &&
			UTGame(WorldInfo.Game) != none && UTGame(WorldInfo.Game).AllowClientToTeleport(Self, DestinationActor) )
	{
		// Cast the Pawn as we know we need it.
		OwnerPawn = UTPawn(OwnerC.Pawn);
		if ( OwnerPawn != none )
		{
			if (bHasFlag)
			{
				GetFlag().Drop();
			}
			return true;
		}
	}

	return false;
}

/**
 * This function is used to teleport directly to actor.  Currently, only 3 types of actors
 * support teleporting.  UTGameObjectives, UTVehicle_Leviathans and UTTeleportBeacons.
 *
 * @param	DestinationActor	This is the Actor the player is trying to teleport to
 */

server reliable function ServerTeleportToActor(Actor DestinationActor)
{
	local UTPawn OwnedPawn;
	local UTGameObjective CurrentObj, Obj;
	local UTVehicle_Leviathan Levi;
	local float BestDist;

	if ( AllowClientToTeleport(DestinationActor, OwnedPawn) )
	{

		// Handle teleporting to Game Objectives

		if ( UTGameObjective(DestinationActor) != none )
		{
			// make sure we're not teleporting to the objective we're already at
			CurrentObj = UTGameObjective(OwnedPawn.Base);
			foreach OwnedPawn.OverlappingActors(class'UTGameObjective',Obj, OwnedPawn.VehicleCheckRadius)
			{
				if (CurrentObj == none || VSize(Obj.Location - OwnedPawn.Location) < BestDist)
				{
					CurrentObj = Obj;
					BestDist = VSize(Obj.Location - OwnedPawn.Location);
				}
			}

			if (CurrentObj == None || (DestinationActor != none && CurrentObj != DestinationActor))
			{
				UTGameObjective(DestinationActor).TeleportTo( OwnedPawn );
			}
		}

		// Handle Leviathans

		else if ( UTVehicle_Leviathan(DestinationActor) != none )
		{
			Levi = 	UTVehicle_Leviathan(DestinationActor);
			if ( Levi.AnySeatAvailable() )
			{
				Levi.TryToDrive(OwnedPawn);
			}
			else
			{
				Levi.PlaceExitingDriver(OwnedPawn);
			}
		}

		// Handle Teleport Beacons

		else if ( UTTeleportBeacon(DestinationActor) != none )
		{
			UTTeleportBeacon(DestinationActor).AttemptTranslocation(OwnedPawn);
		}
	}
}

simulated event byte GetTeamNum()
{
	if (Team != none )
	{
		return Team.TeamIndex;
	}
	else
	{
		return 255;
	}
}

simulated event color GetHudColor()
{
	if ( Team != none )
	{
		return Team.GetHudColor();
	}
	else
	{
		return DefaultHudColor;
	}
}

simulated event ReplicatedEvent(name VarName)
{
	local UTGameReplicationInfo UTGRI;

	if ( VarName == 'Team' )
	{
		// If we have received a team, make the announcement of the team change
		if ( PlayerController(Owner) != none )
		{
			if ( Team != none && Team != OldTeam )
			{
				AnnounceTeamChange();
			}
			OldTeam = Team;
		}
		// try to recreate custom character mesh as they are team specific
		UTGRI = UTGameReplicationInfo(WorldInfo.GRI);
		if (UTGRI != none)
		{
			UTGRI.ProcessCharacterData(self);
		}

		Super.ReplicatedEvent(VarName);
	}
	else if (VarName == 'CharacterData')
	{
		UTGRI = UTGameReplicationInfo(WorldInfo.GRI);
		if (UTGRI != None)
		{
			UTGRI.ProcessCharacterData(self);
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/** Clients only.  This will locally announce a team change */
simulated function AnnounceTeamChange()
{
	PlayerController(Owner).ReceiveLocalizedMessage( class'UTTeamGameMessage', Team.TeamIndex+1, self);
}

/** Called by custom character code when building player mesh to create team-coloured texture.  */
simulated function string GetCustomCharTeamString()
{
	if (Team != None)
	{
		if (Team.TeamIndex == 0)
		{
			return "VRed";
		}
		else if (Team.TeamIndex == 1)
		{
			return "VBlue";
		}
	}

	return "V01";
}

/** sets character data, triggering an update for the local player and any clients */
function SetCharacterData(const out CustomCharData NewData)
{
	local UTGameReplicationInfo GRI;
	local UTGame UTG;

	// If we don't want custom characters (or this is PIE) - skip this part.
	UTG = UTGame(WorldInfo.Game);
	if ((UTG == None || !UTG.bNoCustomCharacters) && !WorldInfo.IsPlayInEditor())
	{
		CharacterData = NewData;
		CharacterDataChangeCount++;
		NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			GRI = UTGameReplicationInfo(WorldInfo.GRI);
			if (GRI != None)
			{
				GRI.ProcessCharacterData(self);
			}
		}
	}
}

/** Accessor that sets the custom character mesh to use for this PRI, and updates instance of player in map if there is one. */
simulated function SetCharacterMesh(SkeletalMesh NewSkelMesh)
{
	local Pawn P;

	if (CharacterMesh != NewSkelMesh)
	{
		CharacterMesh = NewSkelMesh;

		foreach WorldInfo.AllPawns(class'Pawn', P)
		{
			if (P.PlayerReplicationInfo == self)
			{
				P.NotifyTeamChanged();
			}
		}
	}
}

/** Assign a arm mesh and material to this PRI, and updates and instance of player in the map accordingly. */
simulated function SetFirstPersonArmInfo(SkeletalMesh ArmMesh, MaterialInterface ArmMaterial)
{
	local UTPawn UTP;

	FirstPersonArmMesh = ArmMesh;
	FirstPersonArmMaterial = ArmMaterial;

	foreach WorldInfo.AllPawns(class'UTPawn', UTP)
	{
		if (UTP.PlayerReplicationInfo == self)
		{
			UTP.NotifyArmMeshChanged();
		}
	}
}

defaultproperties
{
    LastKillTime=-5.0
    DefaultHudColor=(R=64,G=255,B=255,A=255)
}
