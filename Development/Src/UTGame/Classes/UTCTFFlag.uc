/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCTFFlag extends UTCarriedObject
	native
	abstract;

/** particle system to play when the flag respawns at home base */
var ParticleSystem RespawnEffect;
/** if true the flag is just starting to res out and needs its bright param ramped up*/
var bool bBringUpBright;
/** if true the bright param has peaked and now we need to phase the flag out */
var bool bBringDownFromBright;
/** The Skeletal Mesh of the flag */
var SkeletalMeshComponent SkelMesh;

var repnotify bool  bFadingOut;
var repnotify bool  bRespawning;
var array<MaterialInstanceConstant> MICArray;
var bool bWasClothEnabled;

var	vector	RunningClothVelClamp;
var	vector	HoverboardingClothVelClamp;

var ParticleSystemComponent SuccessfulCaptureSystem;


/** The Flags's light environment */
var DynamicLightEnvironmentComponent LightEnvironment;

replication
{
	if(Role == ROLE_AUTHORITY)
		bFadingOut, bRespawning;
}
cpptext
{
	virtual void PostNetReceiveLocation();
}


simulated event ReplicatedEvent(name VarName)
{
	if(VarName=='bFadingOut')
	{
		if(bFadingOut)
		{
			CustomFadeOutEffects();
		}
	}
	else if(VarName=='bRespawning')
	{
		if(bRespawning)
		{
			CustomRespawnEffects();
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}


simulated function PostBeginPlay()
{
	local int i;
	super.PostBeginPlay();
	for(i=0;i<SkelMesh.Materials.Length;++i)
	{
		MICArray.Insert(i,1);
		MICArray[i] = SkelMesh.CreateAndSetMaterialInstanceConstant(i);
	}
}


// @TODO: Make this native
simulated function Tick(float DeltaTime)
{
	local int i;
	local name TimerInUse;
	if(WorldInfo.NetMode != NM_DEDICATEDSERVER)
	{
		if(bFadingOut)
		{
			TimerInUse='CustomFadeOutEffects';
		}
		else if(bRespawning)
		{
			TimerInUse='bringUpBrightOff';
		}
		if(bBringUpBright)
		{
			for(i=0;i<MICArray.Length;++i)
			{
				MICArray[i].setScalarParameterValue('FlagBrightness',1 - (GetTimerRate(TimerInUse)-GetTimerCount(TimerInUse))/0.5);
			}
		}
		else if(bBringDownFromBright)
		{
			for(i=0;i<MICArray.Length;++i)
			{
				MICArray[i].setScalarParameterValue('FlagOpacity',(GetTimerRate(TimerInUse)-GetTimerCount(TimerInUse))/0.5);
			}
		}
	}
}


/** Update damping on the cloth sim based on what we are doing. */
simulated event OnBaseChainChanged()
{
	if (UTPawn(Base) != None)
	{
		if(UTVehicle_Hoverboard(Base.Base) != None)
		{
			SkelMesh.bClothBaseVelClamp = TRUE;
			SkelMesh.ClothBaseVelClampRange = HoverboardingClothVelClamp;
			//`log("HOVERBOARD---------");
		}
		else
		{
			SkelMesh.bClothBaseVelClamp = TRUE;
			SkelMesh.ClothBaseVelClampRange = RunningClothVelClamp;
			SkelMesh.SetAttachClothVertsToBaseBody(FALSE);
			//`log("RUNNING------");
		}
	}
	else
	{
		SkelMesh.bClothBaseVelClamp = FALSE;
		SkelMesh.SetAttachClothVertsToBaseBody(FALSE);
		//`log("DROPPED------");
	}
}


/** returns true if should be rendered for passed in player */
simulated function bool ShouldMinimapRenderFor(PlayerController PC)
{
	return bHome || (PC.PlayerReplicationInfo.Team != Team);
}


simulated function ClientReturnedHome()
{
	Super.ClientReturnedHome();

	if (HomeBase != None)
	{
		SetBase(HomeBase);
	}
}


// State transitions
function SetHolder(Controller C)
{
	local UTCTFSquadAI S;
	local UTPawn UTP;
	// when the flag is picked up we need to set the flag translation so it doesn't stick in the ground
	SkelMesh.SetTranslation( vect(0.0,0.0,0.0) );
	UTP = UTPawn(C.Pawn);
	SkelMesh.SetLightEnvironment( UTP.LightEnvironment );
	SkelMesh.SetShadowParent( UTP.Mesh );

	ClearTimer( 'SetFlagDynamicLightToNotBeDynamic' );

	// AI Related
	if ( UTBot(C) != None )
	{
		S = UTCTFSquadAI(UTBot(C).Squad);
	}
	else if ( PlayerController(C) != None )
	{
		S = UTCTFSquadAI(UTTeamInfo(C.PlayerReplicationInfo.Team).AI.FindHumanSquad());
	}

	if ( S != None )
	{
		S.EnemyFlagTakenBy(C);
	}

	Super.SetHolder(C);

    // FIXMESTEVE C.SendMessage(None, 'OTHER', C.GetMessageIndex('GOTENEMYFLAG'), 10, 'TEAM');
}

function bool ValidHolder(Actor Other)
{
    local Controller C;

    if ( !Super.ValidHolder(Other) )
	{
		return false;
	}

    C = Pawn(Other).Controller;

	if ( WorldInfo.GRI.OnSameTeam(self,C) )
	{
		SameTeamTouch(c);
		return false;
	}

    return true;
}

function SameTeamTouch(Controller C)
{
}

/** Intended to have different flags override if they need custom effects such as material params set on a respawn */
simulated function CustomRespawnEffects()
{
	local int i;

	bRespawning = true;
	bFadingOut = false;
	ClearTimer('CustomFadeOutEffects');
	setTimer(1.0,false,'BringUpBrightOff');

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if (RespawnEffect != None)
		{
			WorldInfo.MyEmitterPool.SpawnEmitter(RespawnEffect, Location, Rotation, self);
		}

		bBringUpBright = true;
		bBringDownFromBright = false;
		for(i = 0;i < MICArray.Length; i++)
		{
			MICArray[i].setScalarParameterValue('FlagOpacity', 1.0f);
		}

		bWasClothEnabled = SkelMesh.bEnableClothSimulation;
		SkelMesh.SetEnableClothSimulation(false); // to prevent cloth flag from going nutty due to teleport
	}
}
simulated function bringUpBrightOff()
{
	local int i;

	if(WorldInfo.NetMode != NM_DEDICATEDSERVER)
	{
		bBringUpBright = false;

		for(i=0;i<MICArray.Length;++i)
		{
			MICArray[i].setScalarParameterValue('FlagBrightness',0.0f);
		}

		SkelMesh.SetEnableClothSimulation(bWasClothEnabled); // re-enable from above.
	}

	bRespawning = false;
}

/** intended to have different flags override if they need custom effects just before a flag is returned to it's base */
simulated function CustomFadeOutEffects()
{
	local int i;

	if(WorldInfo.NetMode != NM_DEDICATEDSERVER)
	{
		if(bBringUpBright) // we've been brought up to bright, so now fade down
		{
			bBringUpBright=false;
			bBringDownFromBright=true;
			for(i=0;i<MICArray.Length;++i)
			{
				MICArray[i].setScalarParameterValue('FlagBrightness',0.0f);
			}
		}
		else if(bBringDownFromBright) // we've done the whole sequence, so call super and don't do this any more
		{
			for(i=0;i<MICArray.Length;++i)
			{
				MICArray[i].setScalarParameterValue('FlagOpacity',1.0f);
			}
			bBringDownFromBright = false;
			return;
		}
		else // we haven't done anything, so bring up brights.
		{
			bBringUpBright=true;
		}

		setTimer(0.5,false,'CustomFadeOutEffects');
	}
}

// States
auto state Home
{
	ignores SendHome, Score, Drop;

	function BeginState(Name PreviousStateName)
	{
		// need to reset the flag skel mesh translation
		SetFlagPropertiesToStationaryFlagState();

		CustomRespawnEffects();
		Super.BeginState(PreviousStateName);

		if ( Team != None )
		{
			// note team is none at start of match, but flagstate is already correctly set
			UTGameReplicationInfo(WorldInfo.GRI).SetFlagHome(Team.TeamIndex);
		}

		HomeBase.SetAlarm(false);
		HomeBase.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		NetUpdateTime = WorldInfo.TimeSeconds - 1;
		SetTimer(1.0, true);
		SetLocation(Location + Vect(0,0,1));
		SetBase(HomeBase);
		HomeBase.ObjectiveChanged();

	}

	function EndState(Name NextStateName)
	{
		Super.EndState(NextStateName);
		HomeBase.SetAlarm(true);
		HomeBase.NetUpdateTime = WorldInfo.TimeSeconds - 1;
		SetTimer(0.0, false);
	}

	function SameTeamTouch(Controller C)
	{
		local UTCTFFlag flag;

		if (C.PlayerReplicationInfo.bHasFlag)
		{
			// Score!
			flag = UTCTFFlag(UTPlayerReplicationInfo(C.PlayerReplicationInfo).GetFlag());
			UTCTFGame(WorldInfo.Game).ScoreFlag(C, flag);
			SuccessfulCaptureSystem.ActivateSystem();
			flag.Score();

			if (C.Pawn != None && UTBot(C) != None && UTBot(C).Squad.GetOrders() == 'Attack')
			{
				UTBot(C).Pawn.SetAnchor(HomeBase);
				UTBot(C).Squad.SetAlternatePathTo(UTCTFSquadAI(UTBot(C).Squad).EnemyFlag.HomeBase, UTBot(C));
			}
		}
	}
}

state Dropped
{
	ignores Drop;

	function SameTeamTouch(Controller c)
	{
		// returned flag
		UTCTFGame(WorldInfo.Game).ScoreFlag(C, self);
		SendHome(C);
	}

	function Timer() // TODO: Look into resetting scalars on endstate too, just in case picked up mid-fade
	{
		if(bFadingOut)
		{
			super.Timer();
			bFadingOut=false;
		}
		else
		{
			bFadingOut = true;
			CustomFadeOutEffects();
			SetTimer(1.0f);
		}

	}
}
// FIXMESTEVE - always collide, flagbase never does


/** need to reset the flag skel mesh translation **/
function Drop()
{
	SetFlagPropertiesToStationaryFlagState();

	Super.Drop();
}


/**
 * This function will set the flag properties back to what they should be when the flag is stationary.  (i.e. dropped or at a flag base
 **/
function SetFlagPropertiesToStationaryFlagState()
{
	SkelMesh.SetTranslation( vect(0.0,0.0,-40.0) );
	LightEnvironment.bDynamic = TRUE;
	SkelMesh.SetLightEnvironment( LightEnvironment );
	SkelMesh.SetShadowParent( None );
	SetTimer( 0.5f, FALSE, 'SetFlagDynamicLightToNotBeDynamic' );
}

/**
 * This is used to set the LightEnvironment to not be dynamic.
 * Basically when a flag is dropped we need to update the LightEnvironment and then set it to not update anymore.
 **/
function SetFlagDynamicLightToNotBeDynamic()
{
	LightEnvironment.bDynamic = FALSE;
}


defaultproperties
{
	bHome=True
	bStatic=False
	NetPriority=+00003.000000
	bCollideWorld=True
	bCollideActors=true
	bUseTeamColorForIcon=true

	Begin Object Name=CollisionCylinder
		CollisionRadius=+0048.000000
		CollisionHeight=+0085.000000
		CollideActors=true
	End Object

	Begin Object class=PointLightComponent name=FlagLightComponent
		Brightness=5.0
		LightColor=(R=255,G=255,B=255)
		Radius=250.0
		CastShadows=false
		bEnabled=true
		LightingChannels=(Dynamic=FALSE,CompositeDynamic=FALSE)
	End Object
	FlagLight=FlagLightComponent
	Components.Add(FlagLightComponent)

	Begin Object Class=DynamicLightEnvironmentComponent Name=FlagLightEnvironment
	    bDynamic=FALSE
	End Object
	LightEnvironment=FlagLightEnvironment
	Components.Add(FlagLightEnvironment)

	Begin Object Class=SkeletalMeshComponent Name=TheFlagSkelMesh
		CollideActors=true
		BlockActors=false
		BlockZeroExtent=true
		BlockNonZeroExtent=true
		PhysicsWeight=0
		bHasPhysicsAssetInstance=true
		BlockRigidBody=true
		RBChannel=RBCC_Cloth
		RBCollideWithChannels=(Default=FALSE,GameplayPhysics=FALSE,EffectPhysics=TRUE,Cloth=true)
		ClothRBCollideWithChannels=(Default=false,GameplayPhysics=false,EffectPhysics=true,Cloth=true)
		LightEnvironment=FlagLightEnvironment
		bUseAsOccluder=FALSE
		bEnableClothSimulation=true
		bAutoFreezeClothWhenNotRendered=true
		bUpdateSkelWhenNotRendered=false
		ClothWind=(X=20.0,Y=10.0)
		bAcceptsDecals=false
		Translation=(X=0.0,Y=0.0,Z=-40.0)  // this is needed to make the flag line up with the flag base
	End Object
	SkelMesh=TheFlagSkelMesh;
	Components.Add(TheFlagSkelMesh)

 	bHardAttach=true

	GameObjBone3P=b_spine2
	GameObjOffset3P=(X=0,Y=16,Z=0)
	GameObjRot3P=(Roll=-16384,Yaw=16384)
	GameObjRot1P=(Yaw=16384,Roll=-3640)
	GameObjOffset1P=(X=-45.0,Y=-8.0,Z=30.0)

	RunningClothVelClamp=(X=500,Y=500,Z=200)
	HoverboardingClothVelClamp=(X=300,Y=300,Z=200)
}
