/**
 * DemoPlayerController
 * Demo specific playercontroller
 * implements physics spectating state for demoing purpose
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class DemoPlayerController extends GamePlayerController
	dependson(DemoCamMod_ScreenShake)
	config(Game);

var()				float			WeaponImpulse;
var()				float			HoldDistanceMin;
var()				float			HoldDistanceMax;
var()				float			ThrowImpulse;
var()				float			ChangeHoldDistanceIncrement;

var()				RB_Handle		PhysicsGrabber;
var					float			HoldDistance;
var					Quat			HoldOrientation;

/** Impulse component to trigger explosions */
var() const editconst	RB_RadialImpulseComponent	ImpulseComponent;

var() DemoCamMod_ScreenShake.ScreenShakeStruct		ExplosionShake;

/** Ref to the last CameraAnim that was played via Kismet. */
var transient protected CameraAnim					LastPlayedCameraAnim;

event InitInputSystem()
{
	local array<SequenceObject> AllSeqEvents;
	local Sequence GameSeq;
	local DemoKismetInputInteraction InputInteraction;
	local int i;

	Super.InitInputSystem();

	// Get the gameplay sequence.
	GameSeq = WorldInfo.GetGameSequence();
	if (GameSeq != None)
	{
		// find any input events that exist
		GameSeq.FindSeqObjectsByClass(class'SeqEvent_DemoInput', true, AllSeqEvents);

		// if there were any, add an interaction to query them
		if (AllSeqEvents.length > 0)
		{
			InputInteraction = new(self) class'DemoKismetInputInteraction';
			Interactions.Insert(0, 1);
			Interactions[0] = InputInteraction;
			for (i = 0; i < AllSeqEvents.length; i++)
			{
				InputInteraction.InputEvents[InputInteraction.InputEvents.length] = SeqEvent_DemoInput(AllSeqEvents[i]);
			}
		}
	}
}

/** Switch between walking physics to flying */
exec function TogglePhysicsMode()
{
	local Rotator	CurrentRot;
	local Pawn		OldPawn;

	CurrentRot	= Rotation;
	PlayerReplicationInfo.bOnlySpectator = true;
	if( Pawn != None )
	{
		OldPawn = Pawn;
		UnPossess();
		OldPawn.Destroy();
	}
	SetRotation( CurrentRot );
	GotoState('PhysicsSpectator');
}

/** Start as PhysicsSpectator by default */
auto state PlayerWaiting
{
Begin:
	PlayerReplicationInfo.bOnlySpectator = false;
	WorldInfo.Game.bRestartLevel = false;
	WorldInfo.Game.RestartPlayer( Self );
	WorldInfo.Game.bRestartLevel = true;
}


exec function PrevWeapon()
{
	HoldDistance += ChangeHoldDistanceIncrement;
	HoldDistance = FMin(HoldDistance, HoldDistanceMax);
}

exec function NextWeapon()
{
	HoldDistance -= ChangeHoldDistanceIncrement;
	HoldDistance = FMax(HoldDistance, HoldDistanceMin);
}

exec function StartFire( optional byte FireModeNum )
{
	local vector			CamLoc, StartShot, EndShot, X, Y, Z;
	local vector			HitLocation, HitNormal, ZeroVec;
	local actor				HitActor;
	local TraceHitInfo		HitInfo;
	local rotator			CamRot;

	GetPlayerViewPoint(CamLoc, CamRot);

	GetAxes( CamRot, X, Y, Z );
	ZeroVec = vect(0,0,0);

	if ( PhysicsGrabber.GrabbedComponent == None )
	{
		// Do simple line check then apply impulse
		StartShot	= CamLoc;
		EndShot		= StartShot + (10000.0 * X);
		HitActor	= Trace(HitLocation, HitNormal, EndShot, StartShot, True, ZeroVec, HitInfo);
		// `log("HitActor:"@HitActor@"Hit Bone:"@HitInfo.BoneName);
		if ( HitActor != None && HitInfo.HitComponent != None )
		{
			HitInfo.HitComponent.AddImpulse(X * WeaponImpulse, HitLocation, HitInfo.BoneName);
		}
	}
	else
	{
		PhysicsGrabber.GrabbedComponent.AddImpulse(X * ThrowImpulse, , PhysicsGrabber.GrabbedBoneName);
		PhysicsGrabber.ReleaseComponent();
	}
}

exec function StartAltFire( optional byte FireModeNum )
{
	local vector					CamLoc, StartShot, EndShot, X, Y, Z;
	local vector					HitLocation, HitNormal, Extent;
	local actor						HitActor;
	local float						HitDistance;
	local Quat						PawnQuat, InvPawnQuat, ActorQuat;
	local TraceHitInfo				HitInfo;
	local SkeletalMeshComponent		SkelComp;
	local rotator					CamRot;

	GetPlayerViewPoint(CamLoc, CamRot);

	// Do ray check and grab actor
	GetAxes( CamRot, X, Y, Z );
	StartShot	= CamLoc;
	EndShot		= StartShot + (10000.0 * X);
	Extent		= vect(0,0,0);
	HitActor	= Trace(HitLocation, HitNormal, EndShot, StartShot, True, Extent, HitInfo);
	HitDistance = VSize(HitLocation - StartShot);

	if( HitActor != None &&
		HitActor != WorldInfo &&
		HitDistance > HoldDistanceMin &&
		HitDistance < HoldDistanceMax )
	{
		// If grabbing a bone of a skeletal mesh, dont constrain orientation.
		PhysicsGrabber.GrabComponent(HitInfo.HitComponent, HitInfo.BoneName, HitLocation, bRun==0);

		// If we succesfully grabbed something, store some details.
		if( PhysicsGrabber.GrabbedComponent != None )
		{
			HoldDistance	= HitDistance;
			PawnQuat		= QuatFromRotator( CamRot );
			InvPawnQuat		= QuatInvert( PawnQuat );

			if ( HitInfo.BoneName != '' )
			{
				SkelComp = SkeletalMeshComponent(HitInfo.HitComponent);
				ActorQuat = SkelComp.GetBoneQuaternion(HitInfo.BoneName);
			}
			else
			{
				ActorQuat = QuatFromRotator( PhysicsGrabber.GrabbedComponent.Owner.Rotation );
			}

			HoldOrientation = QuatProduct(InvPawnQuat, ActorQuat);
		}
	}
}

exec function StopAltFire( optional byte FireModeNum )
{
	if ( PhysicsGrabber.GrabbedComponent != None )
	{
		PhysicsGrabber.ReleaseComponent();
	}
}

function PlayerTick(float DeltaTime)
{
	local vector	CamLoc, NewHandlePos, X, Y, Z;
	local Quat		PawnQuat, NewHandleOrientation;
	local rotator	CamRot;

	super.PlayerTick(DeltaTime);

	if ( PhysicsGrabber.GrabbedComponent == None )
	{
		return;
	}

	PhysicsGrabber.GrabbedComponent.WakeRigidBody( PhysicsGrabber.GrabbedBoneName );

	// Update handle position on grabbed actor.
	GetPlayerViewPoint(CamLoc, CamRot);
	GetAxes( CamRot, X, Y, Z );
	NewHandlePos = CamLoc + (HoldDistance * X);
	PhysicsGrabber.SetLocation( NewHandlePos );

	// Update handle orientation on grabbed actor.
	PawnQuat = QuatFromRotator( CamRot );
	NewHandleOrientation = QuatProduct(PawnQuat, HoldOrientation);
	PhysicsGrabber.SetOrientation( NewHandleOrientation );
}

// default state, physics spectator!
state PhysicsSpectator extends BaseSpectating
{
ignores NotifyBump, PhysicsVolumeChange, SwitchToBestWeapon;

	exec function TogglePhysicsMode()
	{
		local Vector	CurrentLoc;
		local Rotator	CurrentRot;

		PlayerReplicationInfo.bOnlySpectator = false;
		CurrentLoc = Location;
		CurrentRot = Rotation;
		//@fixme - temp workaround for players not being able to spawn, figure out why this is now necessary
		WorldInfo.Game.bRestartLevel = false;
		WorldInfo.Game.RestartPlayer( Self );
		WorldInfo.Game.bRestartLevel = true;
		if (Pawn == None)
		{
			`warn("Failed to create new pawn?");
		}
		else
		{
			Pawn.SetLocation( CurrentLoc );
			Pawn.SetRotation( CurrentRot );
		}
		SetRotation( CurrentRot );
	}

	function bool CanRestartPlayer()
	{
		//return false;
		return !PlayerReplicationInfo.bOnlySpectator;
	}

	exec function Jump() {}

	reliable server function ServerSuicide() {}

	reliable server function ServerChangeTeam( int N )
	{
	WorldInfo.Game.ChangeTeam(self, 0, true);
	}

    reliable server function ServerRestartPlayer()
	{
		if ( WorldInfo.TimeSeconds < WaitDelay )
			return;
		if ( WorldInfo.NetMode == NM_Client )
			return;
		if ( WorldInfo.Game.bWaitingToStartMatch )
			PlayerReplicationInfo.bReadyToPlay = true;
		else
			WorldInfo.Game.RestartPlayer(self);
	}



	function EndState(Name NextStateName)
	{
		bCollideWorld = false;
	}

	function BeginState(Name PreviousStateName)
	{
		bCollideWorld = true;
	}
}

/**
 * Scripting hook for camera shakes.
 */
simulated function OnDemoCameraShake( SeqAct_DemoCameraShake inAction )
{
	local DemoCamMod_ScreenShake.ScreenShakeStruct	DemoCameraShake;
	local float										AmplitudeScale, FrequencyScale;

	AmplitudeScale = inAction.GlobalScale * inAction.GlobalAmplitudeScale;
	FrequencyScale = inAction.GlobalScale * inAction.GlobalFrequencyScale;

	// Copy shake, so we don't change its properties directly.
	DemoCameraShake = inAction.CameraShake;

	DemoCameraShake.TimeDuration *= inAction.GlobalScale;
	DemoCameraShake.RotAmplitude *= AmplitudeScale;
	DemoCameraShake.RotFrequency *= FrequencyScale;
	DemoCameraShake.LocAmplitude *= AmplitudeScale;
	DemoCameraShake.LocFrequency *= FrequencyScale;
	DemoCameraShake.FOVAmplitude *= AmplitudeScale;
	DemoCameraShake.FOVFrequency *= FrequencyScale;

	if( inAction.InputLinks[0].bHasImpulse )
	{
		PlayCameraShake( DemoCameraShake );
	}
	else
	if( inAction.InputLinks[1].bHasImpulse )
	{

		DemoCameraShake.TimeDuration = 9999999.f;
		PlayCameraShake( DemoCameraShake );
	}
	else
	if( inAction.InputLinks[2].bHasImpulse )
	{
		DemoCameraShake.TimeDuration = -1.f;
		PlayCameraShake( DemoCameraShake );
	}
}

function OnPlayCameraAnim(UTSeqAct_PlayCameraAnim InAction)
{
	local AnimatedCamera Cam;

	// just pass the call onto the animated camera
	Cam = AnimatedCamera(PlayerCamera);
	if (Cam != None)
	{
		Cam.PlayCameraAnim(InAction.AnimToPlay, InAction.Rate, InAction.IntensityScale, InAction.BlendInTime, InAction.BlendOutTime);
		LastPlayedCameraAnim = InAction.AnimToPlay;
	}
}

function OnStopCameraAnim(UTSeqAct_StopCameraAnim InAction)
{
	local AnimatedCamera Cam;

	// just pass the call onto the animated camera
	Cam = AnimatedCamera(PlayerCamera);
	if (Cam != None)
	{
		Cam.StopCameraAnim(LastPlayedCameraAnim, InAction.bStopImmediately);
	}
}

/** Play Camera Shake */
function PlayCameraShake( DemoCamMod_ScreenShake.ScreenShakeStruct ScreenShake )
{
	if( DemoPlayerCamera(PlayerCamera) != None )
	{
		DemoPlayerCamera(PlayerCamera).DemoCamMod_ScreenShake.AddScreenShake( ScreenShake );
	}
}

/**
 * FIXMELAURENT: backward compatibilty with old camera shakes. (Used by AnimNotifies).
 */
function CameraShake
(
	float	Duration,
	vector	newRotAmplitude,
	vector	newRotFrequency,
	vector	newLocAmplitude,
	vector	newLocFrequency,
	float	newFOVAmplitude,
	float	newFOVFrequency
)
{
	if( DemoPlayerCamera(PlayerCamera) != None )
	{
		DemoPlayerCamera(PlayerCamera).DemoCamMod_ScreenShake.StartNewShake( Duration, newRotAmplitude, newRotFrequency, newLocAmplitude, newLocFrequency, newFOVAmplitude, newFOVFrequency );
	}
}


function PlayLocalizedShake( DemoCamMod_ScreenShake.ScreenShakeStruct ScreenShake, vector ShakeLocation, float ShakeRadius )
{
	local Rotator		CamRot;
	local Vector		CamLoc;
	local float			DistToOrigin, Pct;

	if( ShakeRadius <= 0 )
	{
		return;
	}

	// Get camera position
	GetPlayerViewPoint(CamLoc, CamRot);

	DistToOrigin = VSize(ShakeLocation - CamLoc);
	if( DistToOrigin < ShakeRadius )
	{
		Pct = 1.f - (DistToOrigin / ShakeRadius);
		ScreenShake.TimeDuration	*= Pct;
		ScreenShake.RotAmplitude	*= Pct;
		ScreenShake.RotFrequency	*= Pct;
		ScreenShake.LocAmplitude	*= Pct;
		ScreenShake.LocFrequency	*= Pct;
		ScreenShake.FOVAmplitude	*= Pct;
		ScreenShake.FOVFrequency	*= Pct;

		if( DemoPlayerCamera(PlayerCamera) != None )
		{
			DemoPlayerCamera(PlayerCamera).DemoCamMod_ScreenShake.AddScreenShake( ScreenShake );
		}
	}
}

exec function TriggerExplosion()
{
	local Rotator		CamRot;
	local Vector		CamLoc, StartShot, EndShot, HitLocation, HitNormal;
	local Actor			HitActor;
	local float			MaxHitDistance;
	local Emitter		PrtclEmit;

	MaxHitDistance = 10000.f;

	// Get camera position
	GetPlayerViewPoint(CamLoc, CamRot);

	// Do ray check and grab actor
	StartShot	= CamLoc;
	EndShot		= StartShot + (MaxHitDistance * Vector(CamRot));
	HitActor	= Trace(HitLocation, HitNormal, EndShot, StartShot, true, vect(0,0,0));

	if( HitActor != None )
	{
		PlaySound(SoundCue'A_Weapon.RocketLauncher.Cue.A_Weapon_RL_Impact_Cue');
		PlayLocalizedShake( ExplosionShake, HitLocation, 1024.f );
		ImpulseComponent.FireImpulse( HitLocation );

		PrtclEmit = Spawn(class'UTEmitter',,,HitLocation+HitNormal*2,Rotator(HitNormal));
		if( PrtclEmit != None )
		{
			PrtclEmit.SetTemplate( ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketExplosion', true );
			PrtclEmit.ParticleSystemComponent.ActivateSystem();
		}
	}
}

defaultproperties
{
	CameraClass=class'DemoPlayerCamera'

	HoldDistanceMin=50.0
	HoldDistanceMax=750.0
	WeaponImpulse=600.0
	ThrowImpulse=800.0
	ChangeHoldDistanceIncrement=50.0

	Begin Object Class=RB_Handle Name=RB_Handle0
	End Object
	PhysicsGrabber=RB_Handle0
	Components.Add(RB_Handle0)

	Begin Object Class=RB_RadialImpulseComponent Name=ImpulseComponent0
		ImpulseFalloff=RIF_Linear
		ImpulseStrength=2000
		ImpulseRadius=250
	End Object
	ImpulseComponent=ImpulseComponent0
	Components.Add(ImpulseComponent0)
}
