/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Deployable extends UTVehicle
	abstract
	native(Vehicle);

enum EDeployState
{
	EDS_Undeployed,
	EDS_Deploying,
	EDS_Deployed,
	EDS_UnDeploying,
};

/** If true, this SPMA is deployed */
var	repnotify 	EDeployState	DeployedState;


/** How long does it take to deploy */
var float	DeployTime;

/** How long does it take to undeploy */
var float	UnDeployTime;

var float	LastDeployStartTime;

/** Helper to allow quick access to playing deploy animations */
var AnimNodeSequence	AnimPlay;

/** Animations */

var name GetInAnim[2];
var name GetOutAnim[2];
var name IdleAnim[2];
var name DeployAnim[2];

/** Sounds */

var SoundCue DeploySound;
var SoundCue UndeploySound;

var SoundCue DeployedEnterSound;
var SoundCue DeployedExitSound;

var(Deploy) float MaxDeploySpeed;
var(Deploy) bool bRequireAllWheelsOnGround;

replication
{
	if( bNetDirty && (Role==ROLE_Authority) )
		DeployedState;
}

/**
 * Play an animation
 */
simulated function PlayAnim(name AnimName)
{
	if (AnimPlay != none && AnimName != '')
	{
		AnimPlay.SetAnim(AnimName);
		AnimPlay.PlayAnim();
	}
}

simulated function StopAnim()
{
	if (AnimPlay != none)
	{
		AnimPlay.StopAnim();
	}
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	AnimPlay = AnimNodeSequence( Mesh.Animations.FindAnimNode('AnimPlayer') );
	PlayAnim( IdleAnim[0] );
}

/**
 * Play the ambients when an action anim finishes
 */
simulated function OnAnimEnd(AnimNodeSequence SeqNode, float PlayedTime, float ExcessTime)
{
	if ( DeployedState == EDS_Undeployed )
	{
		if (Driver != none)
		{
			PlayAnim(IdleAnim[1]);
		}
		else
		{
			PlayAnim(IdleAnim[0]);
		}

	}
}

/**
 * Play Enter Animations
 */
simulated function AttachDriver( Pawn P )
{
	Super.AttachDriver(P);

	if ( DeployedState == EDS_Undeployed )
	{
		PlayAnim( GetInAnim[0] );
	}
	else
	{
		PlayAnim( GetInAnim[1] );
	}
}

/**
 * Play Exit Animation
 */
simulated function DetachDriver( Pawn P )
{
	Super.DetachDriver(P);

	if ( DeployedState == EDS_Undeployed )
	{
		PlayAnim( GetOutAnim[0] );
	}
	else
	{
		PlayAnim( GetOutAnim[1] );
	}
}

// Accessor to change the deployed state.  It unsure all the needed function calls are made

function ChangeDeployState(EDeployState NewState)
{
	DeployedState = NewState;
	DeployedStateChanged();
}


simulated function DeployedStateChanged()
{
	local int i;

	switch (DeployedState)
	{
		case EDS_Deploying:
			LastDeployStartTime = WorldInfo.TimeSeconds;
			VehicleEvent('StartDeploy');
			VehiclePlayExitSound();
			SetVehicleDeployed();
			PlayAnim(DeployAnim[0]);
			if ( DeploySound != none )
			{
				PlaySound(DeploySound);
			}

			break;

		case EDS_Deployed:
			VehicleEvent('Deployed');
			AnimPlay.SetAnim(DeployAnim[0]);
			AnimPlay.SetPosition(AnimPlay.AnimSeq.NumFrames * AnimPlay.Rate, false);
			for (i = 0; i < Seats.length; i++)
			{
				if (Seats[i].Gun != None)
				{
					Seats[i].Gun.NotifyVehicleDeployed();
				}
			}
			break;

		case EDS_UnDeploying:
			LastDeployStartTime = WorldInfo.TimeSeconds;
			VehicleEvent('StartUnDeploy');
			PlayAnim(DeployAnim[1]);
			if ( UndeploySound != none )
			{
				PlaySound(UndeploySound);
			}
			break;

		case EDS_Undeployed:
			VehicleEvent('UnDeployed');
			AnimPlay.SetAnim(DeployAnim[1]);
			SetVehicleUnDeployed();
			VehiclePlayEnterSound();
			AnimPlay.SetPosition(AnimPlay.AnimSeq.NumFrames * AnimPlay.Rate, false);
			for (i = 0; i < Seats.length; i++)
			{
				if (Seats[i].Gun != None)
				{
					Seats[i].Gun.NotifyVehicleUndeployed();
				}
			}
			break;

	}
}
simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'DeployedState' )
	{
		DeployedStatechanged();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}


simulated function bool IsDeployed()
{
	return (DeployedState == EDS_Deployed);
}

simulated function bool ShouldClamp()
{
	return !IsDeployed();
}

/**
 * Jump Deploys / Undeploys
 */
function bool DoJump(bool bUpdating)
{
	if (Role == ROLE_Authority)
	{
		ServerToggleDeploy();
	}
	return true;
}

/**
 * @Returns true if this vehicle can deploy
 */
function bool CanDeploy()
{
	local int i;

	// Check current speed

	if (VSize(Velocity) > MaxDeploySpeed)
	{
		ReceiveLocalizedMessage(class'UTSPMAMessage',0);
		return false;
	}
	else if (IsFiring())
	{
		return false;
	}
	else
	{
		// Make sure all 4 wheels are on the ground if required
		if (bRequireAllWheelsOnGround)
		{
			for (i=0;i<Wheels.Length;i++)
			{
				if ( !Wheels[i].bWheelOnGround )
				{
					ReceiveLocalizedMessage(class'UTSPMAMessage',3);
					return false;
				}
			}
		}
		return true;
	}
}

reliable server function ServerToggleDeploy()
{
	if ( CanDeploy() )	// Are we stopped (or close enough)
	{
		GotoState('Deploying');
	}
}

simulated function SetVehicleDeployed()
{
	VehicleEvent('EngineStop');

	SetPhysics(PHYS_None);
	bStationary = true;
	bBlocksNavigation = true;
}

simulated function SetVehicleUndeployed()
{
	VehicleEvent('EngineStart');

	SetPhysics(PHYS_RigidBody);
	bStationary = false;
	bBlocksNavigation = !bDriving;
}


state Deploying
{
	reliable server function ServerToggleDeploy();

	simulated function BeginState(name PreviousStateName)
	{
		SetTimer(DeployTime,False,'VehicleIsDeployed');

		if (Role == ROLE_Authority)
		{
			SetVehicleDeployed();
		}

		ChangeDeployState(EDS_Deploying);
	}

	simulated function VehicleIsDeployed()
	{
		GotoState('Deployed');
	}
}

state Deployed
{
	reliable server function ServerToggleDeploy()
	{
		if (!IsFiring())
		{
			GotoState('UnDeploying');
		}
	}

	simulated function DrivingStatusChanged()
	{
		Global.DrivingStatusChanged();

		// force bBlocksNavigation when deployed, even if being driven
		bBlocksNavigation = true;
	}

	/** traces down to the ground from the wheels and undeploys the vehicle if wheels are too far off the ground */
	function CheckStability()
	{
		local int i;
		local vector WheelLoc, MeshDir;

		for (i = 0; i < Wheels.Length; i++)
		{
			MeshDir.X = Mesh.LocalToWorld.XPlane.X;
			MeshDir.Y = Mesh.LocalToWorld.XPlane.Y;
			MeshDir.Z = Mesh.LocalToWorld.XPlane.Z;
			WheelLoc = Mesh.GetPosition() + (Wheels[i].WheelPosition >> rotator(MeshDir));
			if (FastTrace(WheelLoc - vect(0,0,1) * (Wheels[i].WheelRadius + Wheels[i].SuspensionTravel), WheelLoc))
			{
				SetPhysics(PHYS_RigidBody);
				GotoState('UnDeploying');
				return;
			}
		}
	}

	function BeginState(name PreviousStateName)
	{
		ChangeDeployState(EDS_Deployed);
		if (bRequireAllWheelsOnGround)
		{
			SetTimer(1.0, true, 'CheckStability');
		}
	}

	function EndState(name NextStateName)
	{
		ClearTimer('CheckStability');
	}
}

simulated state UnDeploying
{
	reliable server function ServerToggleDeploy();

	simulated function BeginState(name PreviousStateName)
	{
		SetTimer(UnDeployTime,False,'VehicleUnDeployIsFinshed');
		ChangeDeployState(EDS_UnDeploying);
	}

	simulated function VehicleUnDeployIsFinshed()
	{
		if (ROLE==ROLE_Authority && WorldInfo.NetMode != NM_ListenServer)
		{
			SetVehicleUndeployed();
		}

		ChangeDeployState(EDS_UnDeployed);
		GotoState('');
	}
}

simulated function VehiclePlayEnterSound()
{
	local SoundCue EnterCue;

	if (DeployedState == EDS_Deployed || DeployedState == EDS_Undeployed)
	{
		/*
		if (ExitVehicleSound != none)
		{
			ExitVehicleSound.Stop();
		}

		if (DeployedExitSound != none)
		{
			DeployedExitSound.Stop();
		}*/

		EnterCue = (DeployedState == EDS_Deployed)? DeployedEnterSound : EnterVehicleSound;

		if (EnterCue != None)
		{
			PlaySound(EnterCue);
		}
		StartEngineSoundTimed();
	}
}

simulated function VehiclePlayExitSound()
{
	local SoundCue ExitCue;

	StopEngineSound();

	if (DeployedState == EDS_Deploying || DeployedState == EDS_Undeployed)
	{
		/*
		// These have been replaced with ref sound cues, so we can't stop.
		if (EnterVehicleSound != none)
		{
			EnterVehicleSound.Stop();
		}


		if (DeployedEnterSound != none)
		{
			DeployedEnterSound.Stop();
		}
		*/

		ExitCue = (DeployedState == EDS_Deployed)? DeployedExitSound : ExitVehicleSound;

		if (ExitCue != None)
		{
			PlaySound(ExitCue);
		}

		StopEngineSoundTimed();
	}
}

simulated function StartEngineSound()
{
	if (EngineSound != None && DeployedState == EDS_Undeployed)
	{
		EngineSound.Play();
	}
	ClearTimer('StartEngineSound');
	ClearTimer('StopEngineSound');
}


defaultproperties
{
	DeployTime = 2.1;
	UnDeployTime = 2.0;

	MaxDeploySpeed=100
	bRequireAllWheelsOnGround=true


}
