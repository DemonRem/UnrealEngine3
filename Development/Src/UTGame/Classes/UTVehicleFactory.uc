/**
 * Vehicle spawner.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicleFactory extends NavigationPoint
	abstract
	native(Vehicle)
	nativereplication
	placeable;

var		class<UTVehicle>	VehicleClass;		// FIXMESTEVE - this should get changed depending on vehicle set of controlling team
var		UTVehicle			ChildVehicle;

var		float			RespawnProgress;		/** Timer for determining when to spawn vehicles */
var		float			RespawnRateModifier;
var()   bool            bMayReverseSpawnDirection;
var()	bool			bStartNeutral;			/** Not applicable to Onslaught */
var		bool			bHasLockedVehicle;		/** Whether vehicles spawned at this factory are initially team locked */
/** vehicle factory can't be activated while this is set */
var() bool bDisabled;
/** if set, replicate ChildVehicle reference */
var bool bReplicateChildVehicle;

var		UTGameObjective	ReverseObjective;		/** Reverse spawn dir if controlled by same team controlling this objective */
var     int             TeamNum;

var		vector			HUDLocation;
var private transient   MaterialInstanceConstant HUDMaterialInstance;

/** This array holds the initial gun rotations for a spawned vehicle. */
var() array<Rotator>	InitialGunRotations;

var() UTOnslaughtObjective ONSObjectiveOverride;

/** allows setting this vehicle factory to only spawn when one team controls this factory */
var() enum ETeamSpawning
{
	TS_All,
	TS_AxonOnly,
	TS_NecrisOnly
} TeamSpawningControl;

cpptext
{
	virtual void CheckForErrors();
	virtual void TickSpecial( FLOAT DeltaSeconds );
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
}

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		bHasLockedVehicle;
	if (bNetDirty && Role == ROLE_Authority && bReplicateChildVehicle)
		ChildVehicle;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( Role == ROLE_Authority )
	{
		if ( UTGame(WorldInfo.Game) != None )
		{
			UTGame(WorldInfo.Game).ActivateVehicleFactory(self);
		}
		else
		{
			bStartNeutral = true;
			Activate(255);
		}
	}
	else
	{
		AddToClosestObjective();
	}
}

simulated function AddToClosestObjective()
{
	local UTGameObjective O, Best;
	local float Distance, BestDistance;

	if (ONSObjectiveOverride != None)
	{
		Best = ONSObjectiveOverride;
	}
	else if ( UTOnslaughtGame(WorldInfo.Game) != None )
	{
		Best = UTOnslaughtGame(WorldInfo.Game).ClosestObjectiveTo(self);
	}
	else
	{
		foreach WorldInfo.AllNavigationPoints(class'UTGameObjective', O)
		{
			Distance = VSize(Location - O.Location);
			if ( (Best == None) || (Distance < BestDistance) )
			{
				BestDistance = Distance;
				Best = O;
			}
		}
	}
	if ( Best != None )
	{
		Best.VehicleFactories[Best.VehicleFactories.Length] = self;
	}
}

// Called after PostBeginPlay.
//
simulated event SetInitialState()
{
	bScriptInitialized = true;
}

/** function used to update where icon for this actor should be rendered on the HUD
 *  @param NewHUDLocation is a vector whose X and Y components are the X and Y components of this actor's icon's 2D position on the HUD
 */
simulated function SetHUDLocation(vector NewHUDLocation)
{
	HUDLocation = NewHUDLocation;
}

simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, LinearColor FinalColor)
{
	if ( !bHasLockedVehicle )
		return;

	if ( HUDMaterialInstance == None )
	{
		HUDMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		HUDMaterialInstance.SetParent(MP.HUDIcons);
	}
	HUDMaterialInstance.SetVectorParameterValue('HUDColor', FinalColor);
	MP.DrawRotatedMaterialTile(Canvas,HUDMaterialInstance, HUDLocation, Rotation.Yaw + 16384, VehicleClass.Default.MapSize, VehicleClass.Default.MapSize*Canvas.ClipY/Canvas.ClipX,VehicleClass.Default.IconXStart, VehicleClass.Default.IconYStart, VehicleClass.Default.IconXWidth, VehicleClass.Default.IconYWidth);
}

function bool CanActivateForTeam(byte T)
{
	local UTGameReplicationInfo GRI;

	GRI = UTGameReplicationInfo(WorldInfo.GRI);
	return ( TeamSpawningControl == TS_All || GRI == None ||
		(TeamSpawningControl == TS_AxonOnly ? !GRI.IsNecrisTeam(T) : GRI.IsNecrisTeam(T)) );
}

function Activate(byte T)
{
	if (!bDisabled && CanActivateForTeam(T))
	{
		TeamNum = T;
		GotoState('Active');
	}
}

function Deactivate()
{
	local vector HitLocation, HitNormal;

	GotoState('');
	TeamNum = 255;
	if (ChildVehicle != None && !ChildVehicle.bDeleteMe && ChildVehicle.bTeamLocked)
	{
		if (UTGame(WorldInfo.Game).MatchIsInProgress())
		{
			HitLocation = Location;
			ChildVehicle.Health = -2 * ChildVehicle.Default.Health;
			ChildVehicle.TearOffMomentum = vect(0,0,1);
			TraceComponent(HitLocation, HitNormal, ChildVehicle.Mesh, ChildVehicle.Location, Location);
			ChildVehicle.Died(None, class'UTDmgType_NodeDestruction', HitLocation);
		}
		else
		{
			ChildVehicle.Destroy();
		}
	}
}

function TarydiumBoost(float Quantity);

/** called when someone starts driving our child vehicle */
function VehicleTaken()
{
	TriggerEventClass(class'UTSeqEvent_VehicleFactory', None, 1);
	bHasLockedVehicle = false;
	// it's possible that someone could enter and immediately exit the vehicle, but if that happens we mark the
	// vehicle as a navigation obstruction and the AI will use that codepath to avoid it, so this extra cost isn't necessary
	ExtraCost = 0;
}

function VehicleDestroyed( UTVehicle V )
{
	TriggerEventClass(class'UTSeqEvent_VehicleFactory', None, 2);
	ChildVehicle = None;
	bHasLockedVehicle = false;
	ExtraCost = 0;
}

simulated function byte GetTeamNum()
{
	return TeamNum;
}

event SpawnVehicle();

function TriggerSpawnedEvent()
{
	TriggerEventClass(class'UTSeqEvent_VehicleFactory', None, 0);
}

function OnToggle(SeqAct_Toggle Action)
{
	local UTGameObjective Objective;

	if (Action.InputLinks[0].bHasImpulse)
	{
		bDisabled = false;
	}
	else if (Action.InputLinks[1].bHasImpulse)
	{
		bDisabled = true;
	}
	else
	{
		bDisabled = !bDisabled;
	}

	if (bDisabled)
	{
		Deactivate();
	}
	else
	{
		// find the objective that owns us and use it to activate us
		foreach WorldInfo.AllNavigationPoints(class'UTGameObjective', Objective)
		{
			if (Objective.VehicleFactories.Find(self) != INDEX_NONE)
			{
				Activate(Objective.GetTeamNum());
				break;
			}
		}
	}
}

state Active
{
	function TarydiumBoost(float Quantity)
	{
		RespawnProgress -= Quantity;
	}

	function VehicleDestroyed( UTVehicle V )
	{
		Global.VehicleDestroyed(V);
		RespawnProgress = VehicleClass.Default.RespawnTime - VehicleClass.Default.SpawnInTime;
	}

	event SpawnVehicle()
	{
		local Pawn P;
		local bool bIsBlocked;
		local Rotator SpawnRot, TurretRot;
		local int i;

		if ( (ChildVehicle != None) && !ChildVehicle.bDeleteMe )
		{
			return;
		}

		// tell AI to avoid navigating through factories with a vehicle on top of them
		ExtraCost = FMax(ExtraCost,5000);

		foreach CollidingActors(class'Pawn', P, VehicleClass.default.SpawnRadius)
		{
			bIsBlocked = true;
			if (PlayerController(P.Controller) != None)
				PlayerController(P.Controller).ReceiveLocalizedMessage(class'UTVehicleMessage', 2);
		}

		if (bIsBlocked)
		{
			SetTimer(1.0, false, 'SpawnVehicle'); //try again later
		}
		else
		{
			SpawnRot = Rotation;
			if ( bMayReverseSpawnDirection && (ReverseObjective != None) && (ReverseObjective.DefenderTeamIndex == TeamNum) )
			{
				SpawnRot.Yaw += 32768;
			}
			ChildVehicle = spawn(VehicleClass,,,, SpawnRot);
			if (ChildVehicle != None )
			{
				ChildVehicle.SetTeamNum(TeamNum);
				ChildVehicle.ParentFactory = Self;
				if ( bStartNeutral )
					ChildVehicle.bTeamLocked = false;
				else if ( ChildVehicle.bTeamLocked )
					bHasLockedVehicle = true;
				ChildVehicle.Mesh.WakeRigidBody();

				for (i=0; i<ChildVehicle.Seats.Length;i++)
				{
					if (i < InitialGunRotations.Length)
					{
						TurretRot = InitialGunRotations[i];
						if ( bMayReverseSpawnDirection && (ReverseObjective != None) && (ReverseObjective.DefenderTeamIndex == TeamNum) )
						{
							TurretRot.Yaw += 32768;
						}
					}
					else
					{
						TurretRot = SpawnRot;
					}

					ChildVehicle.ForceWeaponRotation(i,TurretRot);
				}
				if (UTGame(WorldInfo.Game).MatchIsInProgress())
				{
					ChildVehicle.PlaySpawnEffect();
				}
				// if gameplay hasn't started yet, we need to wait a bit for everything to be initialized
				if (WorldInfo.bStartup)
				{
					SetTimer(0.1, false, 'TriggerSpawnedEvent');
				}
				else
				{
					TriggerSpawnedEvent();
				}
			}
		}
	}

	function Activate(byte T)
	{
		if (!CanActivateForTeam(T))
		{
			Deactivate();
		}
		else
		{
			TeamNum = T;
			if (ChildVehicle != None)
			{
				// if we have an unused vehicle available, just change its team
				if (ChildVehicle.bTeamLocked)
				{
					ChildVehicle.SetTeamNum(T);
				}
			}
			else
			{
				// force a new vehicle to be spawned
				RespawnProgress = 0.0;
				ClearTimer('SpawnVehicle');
				SpawnVehicle();
			}
		}
	}

	function BeginState(name PreviousStateName)
	{
		if ( UTGame(WorldInfo.Game).MatchIsInProgress() )
		{
			RespawnProgress = VehicleClass.Default.InitialSpawnDelay - VehicleClass.Default.SpawnInTime;
			if (RespawnProgress <= 0.0)
			{
				SpawnVehicle();
			}
		}
		else
		{
			RespawnProgress = 0.0;
			SpawnVehicle();
		}
	}

	function EndState(name NextStateName)
	{
		RespawnProgress = 0.0;
		ClearTimer('SpawnVehicle');
	}
}

defaultproperties
{
	Begin Object Class=SkeletalMeshComponent Name=SVehicleMesh
		CollideActors=false
		HiddenGame=true
		AlwaysLoadOnClient=false
		AlwaysLoadOnServer=false
		bUpdateSkelWhenNotRendered=false
	End Object
	Components.Add(SVehicleMesh)

	Components.Remove(Sprite2)
	GoodSprite=None
	BadSprite=None

	bHidden=true
	bBlockable=true
	bAlwaysRelevant=true
	bSkipActorPropertyReplication=true
	RemoteRole=ROLE_SimulatedProxy
	bStatic=False
	bNoDelete=True
	TeamNum=255
	RespawnRateModifier=1.0
	NetUpdateFrequency=1.0

	SupportedEvents.Add(class'UTSeqEvent_VehicleFactory')
}

