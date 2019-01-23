/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTRemoteRedeemer extends Vehicle
	native
	notplaceable
	abstract;

var ParticleSystemComponent Trail;
var float YawAccel, PitchAccel;

/** parameters for the HUD display */
/* FIXME:
var Shader InnerScopeShader, OuterScopeShader, OuterEdgeShader;
var FinalBlend AltitudeFinalBlend;
var float YawToBankingRatio, BankingResetRate, BankingToScopeRotationRatio;
var int Banking, BankingVelocity, MaxBanking, BankingDamping;
var float VelocityToAltitudePanRate, MaxAltitudePanRate;
*/

var repnotify bool bDying;

/** we use this class's defaults for many properties (damage, effects, etc) to reduce code duplication */
var class<UTProj_RedeemerBase> RedeemerProjClass;

/** Controller that should get credit for explosion kills (since Controller variable won't be hooked up during the explosion) */
var Controller InstigatorController;

var AudioComponent PawnAmbientSound;

/** used to avoid colliding with Driver when initially firing */
var bool bCanHitDriver;

replication
{
	if (Role == ROLE_Authority)
		bDying;
}

// ignored functions
function PhysicsVolumeChange(PhysicsVolume Volume);
singular event BaseChange();
function ShouldCrouch(bool Crouch);
event SetWalking(bool bNewIsWalking);
function bool CheatWalk();
function bool CheatGhost();
function bool CheatFly();
function bool DoJump(bool bUpdating);
simulated event PlayDying(class<DamageType> DamageType, vector HitLoc);
simulated function ClientRestart();

simulated event PreBeginPlay()
{
	// skip Vehicle::PreBeginPlay() so we don't get destroyed in gametypes that don't allow vehicles
	Super(Pawn).PreBeginPlay();
}

simulated function PostBeginPlay()
{
	local vector Dir;

	Dir = vector(Rotation);
	Velocity = AirSpeed * Dir;
	Acceleration = Velocity;

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		Trail.SetTemplate(RedeemerProjClass.default.ProjFlightTemplate);
		PawnAmbientSound.SoundCue = RedeemerProjClass.default.AmbientSound;
		PawnAmbientSound.Play();
	}
}

function bool DriverEnter(Pawn P)
{
	if ( !Super.DriverEnter(P) )
	{
		BlowUp();
		return false;
	}

	if (P.IsA('UTPawn'))
	{
		UTPawn(P).SetMeshVisibility(true);
	}
	InstigatorController = Controller;
	SetCollision(true);
	bCanHitDriver = true;
	return true;
}

event bool EncroachingOn(Actor Other)
{
	return Other.bWorldGeometry;
}

event EncroachedBy(Actor Other)
{
	BlowUp();
}

simulated function Destroyed()
{
	if (Driver != None)
	{
		DriverLeave(true);
	}

	Super.Destroyed();
}

simulated function ReplicatedEvent(name VarName)
{
	if (VarName == 'bDying')
	{
		GotoState('Dying');
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated function rotator GetViewRotation()
{
	return Rotation;
}

/**
 *	Calculate camera view point, when viewing this pawn.
 *
 * @param	fDeltaTime	delta time seconds since last update
 * @param	out_CamLoc	Camera Location
 * @param	out_CamRot	Camera Rotation
 * @param	out_FOV		Field of View
 *
 * @return	true if Pawn should provide the camera point of view.
 */
simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	GetActorEyesViewPoint(out_CamLoc, out_CamRot);
	return true;
}

simulated native function bool IsPlayerPawn() const;

event Landed(vector HitNormal, Actor FloorActor)
{
	BlowUp();
}

event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	BlowUp();
}

function UnPossessed()
{
	BlowUp();
}

singular event Touch(Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal)
{
	if (Other.bProjTarget && (bCanHitDriver || Other != Driver))
	{
		BlowUp();
	}
}

singular event Bump(Actor Other, PrimitiveComponent OtherComp, vector HitNormal)
{
	BlowUp();
}

event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	if (Damage > 0 && (InstigatedBy == None || Controller == None || !WorldInfo.GRI.OnSameTeam(InstigatedBy, Controller)))
	{
		if ( InstigatedBy == None || DamageType == class'DmgType_Crushed'
			|| (class<UTDamageType>(DamageType) != None && class<UTDamageType>(DamageType).default.bVehicleHit) )
		{
			BlowUp();
		}
		else
		{
			Spawn(RedeemerProjClass.default.ExplosionClass);
			if ( PlayerController(Controller) != None )
				PlayerController(Controller).ReceiveLocalizedMessage(class'UTLastSecondMessage', 1, Controller.PlayerReplicationInfo, None, None);
			if ( PlayerController(InstigatedBy) != None )
				PlayerController(InstigatedBy).ReceiveLocalizedMessage(class'UTLastSecondMessage', 1, Controller.PlayerReplicationInfo, None, None);
			DriverLeave(true);
			SetCollision(false, false);
			HurtRadius( RedeemerProjClass.default.Damage, RedeemerProjClass.default.DamageRadius * 0.125,
					RedeemerProjClass.default.MyDamageType, RedeemerProjClass.default.MomentumTransfer, Location,, InstigatorController);
			Destroy();
		}
	}
}

/** PoweredUp()
returns true if pawn has game play advantages, as defined by specific game implementation
*/
function bool PoweredUp()
{
	return true;
}

simulated function StartFire(byte FireModeNum)
{
	ServerBlowUp();
}

reliable server function ServerBlowUp()
{
	BlowUp();
}

function BlowUp()
{
	if (Role == ROLE_Authority)
	{
		GotoState('Dying');
	}
}

simulated function DrawHUD(HUD H)
{
/* FIXME:
    local float Offset;
    local Plane SavedCM;

    SavedCM = Canvas.ColorModulate;
    Canvas.ColorModulate.X = 1;
    Canvas.ColorModulate.Y = 1;
    Canvas.ColorModulate.Z = 1;
    Canvas.ColorModulate.W = 1;
    Canvas.Style = 255;
	Canvas.SetPos(0,0);
	Canvas.DrawColor = class'Canvas'.static.MakeColor(255,255,255);
	if ( bDying )
		Canvas.DrawTile( Material'ScreenNoiseFB', Canvas.SizeX, Canvas.SizeY, 0.0, 0.0, 512, 512 );
	else if ( !Level.IsSoftwareRendering() )
	{
	    if (Canvas.ClipX >= Canvas.ClipY)
	    {
	    Offset = Canvas.ClipX / Canvas.ClipY;
	        Canvas.DrawTile( OuterEdgeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512 * (1 - Offset), 0, Offset * 512, 512 );
     	    Canvas.SetPos(0.5*Canvas.SizeX,0);
      	    Canvas.DrawTile( OuterEdgeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512, 0, -512 * Offset, 512 );
	 	Canvas.SetPos(0,0.5* Canvas.SizeY);
	    Canvas.DrawTile( OuterEdgeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512 * (1 - Offset), 512, Offset * 512, -512);
	    Canvas.SetPos(0.5*Canvas.SizeX,0.5* Canvas.SizeY);
	    Canvas.DrawTile( OuterEdgeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512, 512, -512 * Offset, -512 );
	    Canvas.SetPos(0, 0);
	        Canvas.DrawTile( InnerScopeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512 * (1 - Offset), 0, Offset * 512, 512 );
     	    Canvas.SetPos(0.5* Canvas.SizeX,0);
      	    Canvas.DrawTile( InnerScopeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512, 0, -512 * Offset, 512 );
       	    Canvas.SetPos(0,0.5* Canvas.SizeY);
	    Canvas.DrawTile( InnerScopeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512 * (1 - Offset), 512, Offset * 512, -512 );
	    Canvas.SetPos(0.5* Canvas.SizeX,0.5* Canvas.SizeY);
	    Canvas.DrawTile( InnerScopeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512, 512, -512 * Offset, -512 );
	    Canvas.SetPos(0.5 * (Canvas.SizeX - Canvas.SizeY), 0);
	    Canvas.DrawTile( OuterScopeShader, Canvas.SizeX, Canvas.SizeY, 0, 0, 1024 * Offset, 1024 );
	    Canvas.SetPos((512 * (Offset - 1) + 383) * (Canvas.SizeX / (1024 * Offset)), Canvas.SizeY *(451.0/(1024.0)));
	    Canvas.DrawTile( AltitudeFinalBlend, Canvas.SizeX / (8 * Offset), Canvas.SizeY / 8, 0, 0, 128, 128);
	    Canvas.SetPos((512 * (Offset - 1) + 383 + 2*(512-383) - 128) * (Canvas.SizeX / (1024 * Offset)), Canvas.SizeY *(451.0/1024.0));
	    Canvas.DrawTile( AltitudeFinalBlend, Canvas.SizeX / (8 * Offset), Canvas.SizeY / 8, 128, 0, -128, 128);
	}
	else
	{
	    Offset = Canvas.ClipY / Canvas.ClipX;
	        Canvas.DrawTile( OuterEdgeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 0, 512 * (1 - Offset), 512, 512 * Offset);
     	    Canvas.SetPos(0.5*Canvas.SizeX,0);
      	    Canvas.DrawTile( OuterEdgeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512, 512 * (1 - Offset), -512, 512 * Offset);
	 	Canvas.SetPos(0,0.5* Canvas.SizeY);
	    Canvas.DrawTile( OuterEdgeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 0, 512, 512, -512 * Offset);
	    Canvas.SetPos(0.5*Canvas.SizeX,0.5* Canvas.SizeY);
	    Canvas.DrawTile( OuterEdgeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512, 512, -512, -512 * Offset);
	    Canvas.SetPos(0, 0);
	        Canvas.DrawTile( InnerScopeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 0, 512 * (1 - Offset), 512, 512 * Offset);
     	    Canvas.SetPos(0.5*Canvas.SizeX,0);
      	    Canvas.DrawTile( InnerScopeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512, 512 * (1 - Offset), -512, 512 * Offset);
	 	Canvas.SetPos(0,0.5* Canvas.SizeY);
	    Canvas.DrawTile( InnerScopeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 0, 512, 512, -512 * Offset);
	    Canvas.SetPos(0.5*Canvas.SizeX,0.5* Canvas.SizeY);
	    Canvas.DrawTile( InnerScopeShader, 0.5 * Canvas.SizeX, 0.5 * Canvas.SizeY, 512, 512, -512, -512 * Offset);
	    Canvas.SetPos(0, 0.5 * (Canvas.SizeY - Canvas.SizeX));
	    Canvas.DrawTile( OuterScopeShader, Canvas.SizeX, Canvas.SizeY, 0, 0, 1024, 1024 * Offset );
	    Canvas.SetPos(Canvas.SizeX * (383.0/1024.0), (512 * (Offset - 1) + 451) * (Canvas.SizeY / (1024 * Offset)));
	    Canvas.DrawTile( AltitudeFinalBlend, Canvas.SizeX / 8, Canvas.SizeY / (8 * Offset), 0, 0, 128, 128);
	    Canvas.SetPos(Canvas.SizeX * ((383 + 2*(512-383) - 128)) / 1024, (512 * (Offset - 1) + 451) * (Canvas.SizeY / (1024 * Offset)));
	    Canvas.DrawTile( AltitudeFinalBlend, Canvas.SizeX / 8, Canvas.SizeY / (8 * Offset), 128, 0, -128, 128);
	}
   	}
   	Canvas.ColorModulate = SavedCM;
*/
}

function bool Died(Controller Killer, class<DamageType> damageType, vector HitLocation)
{
	BlowUp();
	return true;
}

function Suicide()
{
	BlowUp();
	Super.Suicide();
}

function DriverDied()
{
	local Pawn OldDriver;

	OldDriver = Driver;
	Super.DriverDied();
	// don't consider the pawn as having died while driving a vehicle
	OldDriver.DrivenVehicle = None;

	BlowUp();
}

function bool PlaceExitingDriver(optional Pawn ExitingDriver)
{
	// leave the pawn where it is
	return true;
}

auto state Flying
{
	simulated function FaceRotation(rotator NewRotation, float DeltaTime)
	{
		local vector X,Y,Z;
		local float PitchThreshold;
		local int Pitch;
		/* FIXME:
		local rotator TempRotation;
		local TexRotator ScopeTexRotator;
		local VariableTexPanner AltitudeTexPanner;
		*/
		local PlayerController PC;

		PC = PlayerController(Controller);
		if (PC != None && LocalPlayer(PC.Player) != None)
		{
			// process input and adjust acceleration
			YawAccel = (1 - 2 * DeltaTime) * YawAccel + DeltaTime * PC.PlayerInput.aTurn;
			PitchAccel = (1 - 2 * DeltaTime) * PitchAccel + DeltaTime * PC.PlayerInput.aLookUp;
			SetRotation(rotator(Velocity));

			GetAxes(Rotation,X,Y,Z);
			PitchThreshold = 3000;
			Pitch = Rotation.Pitch & 65535;
			if (Pitch > 16384 - PitchThreshold && Pitch < 49152 + PitchThreshold)
			{
				if (Pitch > 49152 - PitchThreshold)
				{
					PitchAccel = Max(PitchAccel, 0);
				}
				else if (Pitch < 16384 + PitchThreshold)
				{
					PitchAccel = Min(PitchAccel,0);
				}
			}
			Acceleration = Velocity + 5 * (YawAccel * Y + PitchAccel * Z);
			if (Acceleration == vect(0,0,0))
			{
				Acceleration = Velocity;
			}
			Acceleration = Normal(Acceleration) * AccelRate;

			/* FIXME:
			BankingVelocity += DeltaTime * (YawToBankingRatio * PC.PlayerInput.aTurn - BankingResetRate * Banking - BankingDamping * BankingVelocity);
			Banking += DeltaTime * BankingVelocity;
			Banking = Clamp(Banking, -MaxBanking, MaxBanking);
			TempRotation = Rotation;
			TempRotation.Roll = Banking;
			SetRotation(TempRotation);
			ScopeTexRotator = TexRotator(OuterScopeShader.Diffuse);
			if (ScopeTexRotator != None)
			{
				ScopeTexRotator.Rotation.Yaw = Rotation.Roll;
			}
			AltitudeTexPanner = VariableTexPanner(Shader(AltitudeFinalBlend.Material).Diffuse);
			if (AltitudeTexPanner != None)
			{
				AltitudeTexPanner.PanRate = FClamp(Velocity.Z * VelocityToAltitudePanRate, -MaxAltitudePanRate, MaxAltitudePanRate);
			}
			*/
		}
	}

	simulated function Tick(float DeltaTime)
	{
		// if on non-owning client or on the server, just face in the same direction as velocity
		if (!IsLocallyControlled())
		{
			SetRotation(rotator(Velocity));
		}
	}
}

simulated state Dying
{
ignores Trigger, Bump, HitWall, HeadVolumeChange, PhysicsVolumeChange, Falling, BreathTimer, SetInitialState;

	simulated function StartFire(byte FireModeNum);
	function BlowUp();
	reliable server function ServerBlowUp();
	event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser);

	simulated function BeginState(name OldStateName)
	{
		local ParticleSystem Template;

		bDying = true;
		if (Role == ROLE_Authority)
		{
			MakeNoise(1.0);
			SetHidden(True);
			RedeemerProjClass.static.ShakeView(Location, WorldInfo);
		}
		Template = class'UTEmitter'.static.GetTemplateForDistance(RedeemerProjClass.default.DistanceExplosionTemplates, Location, WorldInfo);
		if (Template != None)
		{
			WorldInfo.MyEmitterPool.SpawnEmitter(Template, Location, Rotation);
		}
		SetPhysics(PHYS_None);
		SetCollision(false, false);
	}

Begin:
	Instigator = self;
	PlaySound(RedeemerProjClass.default.ExplosionSound, true);
	RedeemerProjClass.static.RedeemerHurtRadius(0.125, self, InstigatorController);
	Sleep(0.5);
	RedeemerProjClass.static.RedeemerHurtRadius(0.300, self, InstigatorController);
	Sleep(0.2);
	RedeemerProjClass.static.RedeemerHurtRadius(0.475, self, InstigatorController);
	Sleep(0.2);
	if (Role == ROLE_Authority)
	{
		DriverLeave(true);
		RedeemerProjClass.static.DoKnockdown(Location, WorldInfo, InstigatorController);
	}
	RedeemerProjClass.static.RedeemerHurtRadius(0.650, self, InstigatorController);
	Sleep(0.2);
	RedeemerProjClass.static.RedeemerHurtRadius(0.825, self, InstigatorController);
	Sleep(0.2);
	RedeemerProjClass.static.RedeemerHurtRadius(1.0, self, InstigatorController);
	Destroy();
}

defaultproperties
{
	// all default properties are located in the _Content version for easier modification and single location
}
