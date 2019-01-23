/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTStealthVehicle extends UTVehicle_Deployable
	native(Vehicle)
	abstract;

/** name of the Turret control */
var name TurretName;
var name DeployedViewSocket;

var byte TurretFiringMode;

var name ExhaustEffectName;

/** Material to use when the vehicle is visible */
var MaterialInterface VisibleSkin;

/** Material to use when the vehicle is cloaked */
var MaterialInterface CloakedSkin;

var name SkinTranslucencyName;
var name TeamSkinParamName;

var name HitEffectName;
var float HitEffectColor;
var protected MaterialInstanceConstant BodyMaterialInstance;
var float SlowSpeed;
var MeshComponent DeployPreviewMesh;

//============================================================
/** The Link Beam */
var particleSystem BeamTemplate;

/** Holds the Emitter for the Beam */
var ParticleSystemComponent BeamEmitter;

/** Where to attach the Beam */
var name BeamSockets;

/** The name of the EndPoint parameter */
var name EndPointParamName;

var protected AudioComponent BeamAmbientSound;

var soundcue BeamFireSound;
var soundcue BeamStartSound;
var soundcue BeamStopSound;

/** team based colors for beam when targeting a teammate */
var color LinkBeamColors[3];
/** true when want to transition camera back out of deployed camera mode */
var bool bTransitionCamera;
/** time to transition to deployed camera mode */
var(Movement) float FastCamTransitionTime;

/** The current steering offset of the arm when deployed. */
var int AimYawOffset;
/** The Camera Scale before last deploy started, for smoothing*/
var float StoredCameraScale;

var float ArmSpeedTune;

var float CurrentWeaponScale[10];
var int BouncedWeapon;
var int LastSelectedWeapon;

var AudioComponent TurretArmMoveSound;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();
	VerifyBodyMaterialInstance();
	AddBeamEmitter();
}

/**
 * This function will verify that the BodyMaterialInstance variable is setup and ready to go.  This is a key
 * component for the BodyMat overlay system
 */
simulated function bool VerifyBodyMaterialInstance()
{
	local MaterialInstanceConstant BM2C;

	if (WorldInfo.NetMode != NM_DedicatedServer && Mesh != None && (BodyMaterialInstance == None) )
	{
		// set up material instances (for overlay effects)
		if(CloakedSkin == none)
		{
			BodyMaterialInstance = Mesh.CreateAndSetMaterialInstanceConstant(0);
		}
		else
		{
			// Create the material instance.
			BodyMaterialInstance = new(Outer) class'MaterialInstanceConstant';
			BodyMaterialInstance.SetParent(CloakedSkin);


		}

		BM2C = Mesh.CreateAndSetMaterialInstanceConstant(1);
		BM2C.SetScalarParameterValue(HitEffectName, 0.0);
		BM2C.SetScalarParameterValue(SkinTranslucencyName, 0.0);
	}
	return BodyMaterialInstance != None;
}

simulated function TeamChanged()
{
	if(BodyMaterialInstance == none)
	{
		VerifyBodyMaterialInstance();
	}
	if(BodyMaterialInstance != none) // only proceed if the above actually worked.
	{
		BodyMaterialInstance.SetScalarParameterValue(TeamSkinParamName,GetTeamNum()==1?1:0);
	}
}

/**
 * Function cloaks or decloaks the vehicle.
 */
simulated function Cloak(bool bIsEnabled)
{
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		if(bIsEnabled && Mesh.Materials[0] != CloakedSkin)
		{
			Mesh.SetMaterial(0,BodyMaterialInstance);
			Mesh.CastShadow = false;
		}
		else if(!bIsEnabled && Mesh.Materials[1] != VisibleSkin)
		{
			Mesh.SetMaterial(0,VisibleSkin);
			Mesh.CastShadow = true;
		}
	}
}

function PlayHit(float Damage, Controller InstigatedBy, vector HitLocation, class<DamageType> damageType, vector Momentum, TraceHitInfo HitInfo)
{
	HitEffectColor = 1.0;
}

simulated event Destroyed()
{
	super.Destroyed();
	KillBeamEmitter();
}


simulated function int PartialTurn(int original, int desired, float PctTurn)
{
	local float result;

	original = original & 65535;
	desired = desired & 65535;

	if ( abs(original - desired) > 32768 )
	{
		if ( desired > original )
		{
			original += 65536;
		}
		else
		{
			desired += 65536;
		}
	}
	result = original*(1-PctTurn) + desired*PctTurn;
	return (int(result) & 65535);
}

simulated function VehicleCalcCamera(float DeltaTime, int SeatIndex, out vector out_CamLoc, out rotator out_CamRot, out vector CamStart, optional bool bPivotOnly)
{
	local float TimeSinceTransition;
	local float Pct;

	TimeSinceTransition = WorldInfo.TimeSeconds - LastDeployStartTime;
	if ( DeployedState != EDS_deployed && DeployedState != EDS_deploying)
	{
		if(TimeSinceTransition < FastCamTransitionTime)
		{
			Pct = TimeSinceTransition/FastCamTransitionTime;
			SeatCameraScale = (StoredCameraScale - 1)*Pct + 1;
		}
	}
	else
	if ( TimeSinceTransition < FastCamTransitionTime )
	{
		Pct = 1-TimeSinceTransition/FastCamTransitionTime;
		SeatCameraScale = (StoredCameraScale - 1)*Pct + 1;
	}
	super.VehicleCalcCamera(DeltaTime, SeatIndex, out_CamLoc, out_CamRot, CamStart, bPivotOnly);


}
simulated function Rotator GetViewRotation()
{
	local rotator FixedRotation, ControllerRotation;
	local float TimeSinceTransition, PctTurn;
	local int RotationResult;

	// Get baseline rotation and deployed yaw
	FixedRotation = super.GetViewRotation();

	// Start block to find deployed rotation yaw:

	RotationResult = Rotation.Yaw + (AimYawOffset  + 32768)%65536;
	// End block to find rotation yaw

	if ( DeployedState != EDS_deployed && DeployedState != EDS_deploying)
	{
		if ( bTransitionCamera )
		{
			TimeSinceTransition = WorldInfo.TimeSeconds- LastDeployStartTime;
			if ( TimeSinceTransition < FastCamTransitionTime )
			{
				PctTurn = TimeSinceTransition/FastCamTransitionTime;
				FixedRotation.Yaw = PartialTurn(RotationResult, FixedRotation.Yaw, PctTurn);
				FixedRotation.Pitch = PartialTurn(-10000, FixedRotation.Pitch, PctTurn);//-16384
				FixedRotation.Roll = PartialTurn(Rotation.Roll, FixedRotation.Roll, PctTurn);
			}
		}
		Seats[0].CameraTag = default.Seats[0].CameraTag;
		return FixedRotation;
	}

	// swing smoothly around to vehicle rotation
	TimeSinceTransition = WorldInfo.TimeSeconds- LastDeployStartTime;
	if ( TimeSinceTransition < FastCamTransitionTime )
	{
		FixedRotation = super.GetViewRotation();
		PctTurn = TimeSinceTransition/FastCamTransitionTime;
		FixedRotation.Yaw = PartialTurn(FixedRotation.Yaw, RotationResult, PctTurn);
		FixedRotation.Pitch = PartialTurn(FixedRotation.Pitch, -10000, PctTurn);
		FixedRotation.Roll = PartialTurn(FixedRotation.Roll, Rotation.Roll, PctTurn);
		Seats[0].CameraTag = 'DeployableDrop';
		return FixedRotation;
	}
	else
	{
		Seats[0].CameraTag = 'DeployableDrop';
		FixedRotation.Yaw = RotationResult;
		FixedRotation.Pitch = -10000;
		FixedRotation.Roll = rotation.roll;
		if ( Controller != None )
		{
			ControllerRotation = FixedRotation;
			ControllerRotation.Roll = 0;
			Controller.SetRotation(ControllerRotation);
		}
		return FixedRotation;
	}
}

simulated function ProcessViewRotation( float DeltaTime, out rotator out_ViewRotation, out Rotator out_DeltaRot )
{
	if(DeployedState == EDS_Deployed || DeployedState == EDS_Deploying)
	{
		if(out_DeltaRot.Yaw != 0)
		{
			out_DeltaRot.Yaw = 0;
		}
		out_DeltaRot.Pitch = 0;
	}

	super.ProcessViewRotation(deltatime,out_viewrotation,out_deltarot);
}
simulated function SetVehicleDeployed()
{
	Super.SetVehicleDeployed();
	Cloak(false);
}

simulated function SetVehicleUndeployed()
{
	Super.SetVehicleUndeployed();
	Cloak(bDriving);
	TurretArmMoveSound.Stop();
}

simulated function DrivingStatusChanged()
{
	super.DrivingStatusChanged();
	Cloak((bDriving && !IsDeployed()));
}

simulated function bool OverrideBeginFire(byte FireModeNum)
{
	if(FireModeNum == 0 && (DeployedState == EDS_Deploying || DeployedState == EDS_Deployed ))
	{
		CancelDeploy();
		return true;
	}
	return false;
}
simulated function DeployedStateChanged()
{
	super.DeployedStateChanged();
	switch (DeployedState)
	{
		case EDS_Deploying:
			SetDeployVisual();
			if(DeployPreviewMesh != none)
			{
				DeployPreviewMesh.SetHidden(false);
			}
			Seats[0].Gun.EndFire(0); // stop the link beam while deployed
			StoredCameraScale = SeatCameraScale;
			break;
		case EDS_Undeploying:
			AimYawOffset=0.0f;
			break;
		case EDS_Undeployed:
			if(DeployPreviewMesh != none)
			{
				DeployPreviewMesh.SetHidden(true);
			}
			break;
	}
}
simulated native function SetArmLocation(float DeltaSeconds);

simulated function SetDeployVisual()
{
	local UTVWeap_NightshadeGun NSG;
	local SkeletalMeshComponent SkMC;
	local StaticMeshComponent StMC;
	local class<Actor> DepActor;
	local class<UTSlowVolume> SlowActor;
	local float PreviewScale;

	NSG = UTVWeap_NightShadeGun(Seats[0].Gun);
	if(NSG != none && WorldInfo.NetMode != NM_DedicatedServer)
	{
		DepActor = ((NSG.DeployableList[NSG.DeployableIndex].DeployableClass.static.GetTeamDeployable(Instigator.GetTeamNum())));
		if(DepActor != none)
		{
			// HACK: slow volume weird case
			SlowActor = class<UTSlowVolume>(DepActor);
			if(SlowActor != none)
			{
				SkMC = SlowActor.default.GeneratorMesh;
			}
			else
			{
				SkMC = SkeletalMeshComponent(DepActor.default.CollisionComponent);
				if(SkMC == none)
				{
					StMC = StaticMeshComponent(DepActor.default.CollisionComponent);
				}
			}
		}
		// emergency choice is the dropped version:
		if(SkMC == none && StMC == none)
		{
			StMC = StaticMeshComponent((NSG.DeployableList[NSG.DeployableIndex].DeployableClass.default.DroppedPickupMesh));
		}
		// trash the old one so it gets GCed and then we clone out the new one.
		if(DeployPreviewMesh != none)
		{
			Mesh.DetachComponent(DeployPreviewMesh);
		}
		if(SkMC != none || StMC != none)
		{
			PreviewScale = NSG.DeployableList[NSG.DeployableIndex].DeployableClass.default.PreviewScale3p;
			PreviewScale = PreviewScale==0.0f?1.0f:PreviewScale;
			if(SkMC != none)
			{
				DeployPreviewMesh = new(self) SkMC.Class(SkMC);
			}
			else if(StMC != none)
			{
				DeployPreviewMesh = new(self) StMC.Class(StMC);
			}
			if(DeployPReviewMesh != none)
			{
				DeployPreviewMesh.SetShadowParent(Mesh);
				DeployPreviewMesh.SetLightEnvironment(LightEnvironment);
				DeployPreviewMesh.SetTranslation(Vect(0,0,0));
				DeployPreviewMesh.SetScale(PreviewScale);
				DeployPreviewMesh.SetHidden(!IsDeployed());
				Mesh.AttachComponentToSocket(DeployPreviewMesh,'DeployableDrop');
			}
		}
	}
}

/**
request change to adjacent vehicle seat
*/
simulated function AdjacentSeat(int Direction, Controller C)
{
	if ( DeployedState == EDS_deployed || DeployedState == EDS_deploying )
	{
		AdjustCameraScale(Direction < 0);
	}
	else
	{
		super.AdjacentSeat(Direction, C);
	}
}

/** if deployed, changes selected deployable */
simulated function AdjustCameraScale(bool bMoveCameraIn)
{
	local UTVWeap_NightshadeGun NSG;
	local int DesiredIndex;

	NSG = UTVWeap_NightShadeGun(Seats[0].Gun);

	if ( (NSG != None) && (DeployedState == EDS_deployed || DeployedState == EDS_deploying) )
	{
		DesiredIndex = NSG.NextAvailableDeployableIndex(bMoveCameraIn ? -1 : 1);
		if ( DesiredIndex != NSG.DeployableIndex )
		{
			NSG.SelectWeapon(DesiredIndex);
			ServerSwitchWeapon(DesiredIndex+1);
			SetDeployVisual();
		}
	}
	else
	{
		Super.AdjustCameraScale(bMoveCameraIn);
	}
}

simulated function SwitchWeapon(byte NewGroup)
{
	local UTVWeap_NightshadeGun NSG;

	NSG = UTVWeap_NightShadeGun(Seats[0].Gun);
	if( (NSG != none) && (WorldInfo.NetMode != NM_DedicatedServer) && NSG.SelectWeapon(NewGroup-1) )
	{
		ServerSwitchWeapon(NewGroup);
		SetDeployVisual();
	}
}

reliable server function ServerSwitchWeapon(byte NewGroup)
{
	local UTVWeap_NightshadeGun NSG;

	NSG = UTVWeap_NightShadeGun(Seats[0].Gun);
	if(NSG != none)
	{
		NSG.SelectWeapon(NewGroup-1);
	}
}

/**
 * Attach driver to vehicle.
 * Sets up the Pawn to drive the vehicle (rendering, physics, collision..).
 * Called only if bAttachDriver is true.
 * Network : ALL
 */
simulated function AttachDriver( Pawn P )
{
	UTVWeap_NightShadeGun(Seats[0].Gun).bShowDeployableName = false;
	Super.AttachDriver(P);
}

simulated function AddBeamEmitter()
{
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if (BeamEmitter == None)
		{
			if (BeamTemplate != None)
			{
				BeamEmitter = new(Outer) class'UTParticleSystemComponent';
				BeamEmitter.SetTemplate(BeamTemplate);
				BeamEmitter.SetHidden(true);
				Mesh.AttachComponentToSocket( BeamEmitter, BeamSockets );
			}
		}
		else
		{
			BeamEmitter.ActivateSystem();
		}
	}
}

simulated function KillBeamEmitter()
{
	if (BeamEmitter != none)
	{
		BeamEmitter.SetHidden(true);
		BeamEmitter.DeactivateSystem();
	}
}

simulated function SetBeamEmitterHidden(bool bHide)
{
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		if ( BeamEmitter.HiddenGame != bHide )
		{
			if (BeamEmitter != none)
				BeamEmitter.SetHidden(bHide);

			if(bHide)
			{
				PlaySound(BeamStopSound);
			}
			BeamAmbientSound.Stop();
			if (!bHide)
			{
				BeamAmbientSound.SoundCue = BeamFireSound;
				PlaySound(BeamStartSound);
				BeamAmbientSound.Play();
				BeamEmitter.ActivateSystem();
			}
		}
	}
}

static function color GetTeamBeamColor(byte TeamNum)
{
	if (TeamNum >= ArrayCount(default.LinkBeamColors))
	{
		TeamNum = ArrayCount(default.LinkBeamColors) - 1;
	}

	return default.LinkBeamColors[TeamNum];
}

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local color BeamColor;

	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	if (SeatIndex == 0)
	{
		HitEffectColor = 1.0f;
		SetBeamEmitterHidden(false);
		BeamEmitter.SetVectorParameter(EndPointParamName, FlashLocation);

		if (FiringMode == 2 && WorldInfo.GRI.GameClass.Default.bTeamGame)
		{
			BeamColor = GetTeamBeamColor(Instigator.GetTeamNum());
		}
		else
		{
			BeamColor = GetTeamBeamColor(255);
		}
		BeamEmitter.SetColorParameter('Link_Beam_Color', BeamColor);
	}
}

simulated function VehicleWeaponStoppedFiring( bool bViaReplication, int SeatIndex )
{
	SetBeamEmitterHidden(true);
}

event bool DriverLeave(bool bForceLeave)
{
	CancelDeploy();
	return Super.DriverLeave(bForceLeave);
}

function CancelDeploy()
{
	if(DeployedState != EDS_Undeployed)
	{
		if(DeployedState == EDS_Deploying) // we'll have to wait till we're done
		{
			SetTimer(DeployTime+0.0001,false,'ServerToggleDeploy');
		}
		else
		{
			ServerToggleDeploy();
		}
	}
}

/*
*/
function DisplayWeaponBar(Canvas canvas, UTHUD HUD)
{
	local int i, j, SelectedWeapon, PrevWeapIndex, NextWeapIndex;
	local float TotalOffsetX, OffsetX, OffsetY, BoxOffsetSize, OffsetSizeX, OffsetSizeY, DesiredWeaponScale[10];
	local linearcolor AmmoBarColor;
	local float Delta, SelectedAmmoBarX, SelectedAmmoBarY;
	local UTVWeap_NightShadeGun Gun;

	Gun = UTVWeap_NightshadeGun(Seats[0].Gun);
	if ( Gun == None )
	{
		return;
	}

	SelectedWeapon = Gun.DeployableIndex;
	Delta = HUD.WeaponScaleSpeed * (WorldInfo.TimeSeconds - HUD.LastHUDUpdateTime);
	BoxOffsetSize = HUD.HUDScaleX * HUD.WeaponBarScale * HUD.WeaponBoxWidth;


	if ( (SelectedWeapon != LastSelectedWeapon) )
	{
		LastSelectedWeapon = SelectedWeapon;
		HUD.PlayerOwner.ReceiveLocalizedMessage( class'UTWeaponSwitchMessage',,,, Gun );
	}

	// calculate offsets
	for ( i=0; i<5; i++ )
	{
		// optimization if needed - cache desiredweaponscale[] when pending weapon changes
		if ( SelectedWeapon == i )
		{
			if ( BouncedWeapon == i )
			{
				DesiredWeaponScale[i] = HUD.SelectedWeaponScale;
			}
			else
			{
				DesiredWeaponScale[i] = HUD.BounceWeaponScale;
				if ( CurrentWeaponScale[i] >= DesiredWeaponScale[i] )
				{
					BouncedWeapon = i;
				}
			}
		}
		else
		{
			DesiredWeaponScale[i] = 1.0;
		}
		if ( CurrentWeaponScale[i] != DesiredWeaponScale[i] )
		{
			if ( DesiredWeaponScale[i] > CurrentWeaponScale[i] )
			{
				CurrentWeaponScale[i] = FMin(CurrentWeaponScale[i]+Delta,DesiredWeaponScale[i]);
			}
			else
			{
				CurrentWeaponScale[i] = FMax(CurrentWeaponScale[i]-Delta,DesiredWeaponScale[i]);
			}
		}
		TotalOffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
	}
	PrevWeapIndex = SelectedWeapon - 1;
	NextWeapIndex = SelectedWeapon + 1;

	OffsetX = HUD.HUDScaleX * HUD.WeaponBarXOffset + 0.5 * (Canvas.ClipX - TotalOffsetX);
	OffsetY = Canvas.ClipY - HUD.HUDScaleY * HUD.WeaponBarY;

	// @TODO - manually reorganize canvas calls, or can this be automated?
	// draw weapon boxes
	Canvas.SetDrawColor(255,255,255,255);
	OffsetSizeX = HUD.HUDScaleX * HUD.WeaponBarScale * 96 * HUD.SelectedBoxScale;
	OffsetSizeY = HUD.HUDScaleY * HUD.WeaponBarScale * 64 * HUD.SelectedBoxScale;

	for ( i=0; i<5; i++ )
	{
		Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
		if ( SelectedWeapon == i )
		{
			//Current slot overlay
			HUD.TeamHudColor.A = HUD.SelectedWeaponAlpha;
			Canvas.DrawColorizedTile(HUD.AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 530, 248, 69, 49, HUD.TeamHUDColor);

			Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
			Canvas.DrawColorizedTile(HUD.AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 459, 148, 69, 49, HUD.TeamHUDColor);

			Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
			Canvas.DrawColorizedTile(HUD.AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 459, 248, 69, 49, HUD.TeamHUDColor);

			SelectedAmmoBarX = HUD.HUDScaleX * (HUD.SelectedWeaponAmmoOffsetX - HUD.WeaponBarXOffset) + OffsetX;
			SelectedAmmoBarY = Canvas.ClipY - HUD.HUDScaleY * (HUD.WeaponBarY + CurrentWeaponScale[i]*HUD.WeaponAmmoOffsetY);
		}
		else
		{
			HUD.TeamHudColor.A = HUD.OffWeaponAlpha;
			Canvas.DrawColorizedTile(HUD.AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 459, 148, 69, 49, HUD.TeamHUDColor);

			// draw slot overlay?
			if ( i == PrevWeapIndex )
			{
				Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
				Canvas.DrawColorizedTile(HUD.AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 530, 97, 69, 49, HUD.TeamHUDColor);
			}
			else if ( i == NextWeapIndex )
			{
				Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
				Canvas.DrawColorizedTile(HUD.AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 530, 148, 69, 49, HUD.TeamHUDColor);
			}
		}
		OffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
	}

	// draw weapon ammo bars
	// Ammo Bar:  273,494 12,13 (The ammo bar is meant to be stretched)
	Canvas.SetDrawColor(255,255,255,255);
	OffsetX = HUD.HUDScaleX * HUD.WeaponAmmoOffsetX + 0.5 * (Canvas.ClipX - TotalOffsetX);
	OffsetSizeY = HUD.HUDScaleY * HUD.WeaponBarScale * HUD.WeaponAmmoThickness;
	AmmoBarColor = MakeLinearColor(1.0,10.0,1.0,1.0);
	for ( i=0; i<5; i++ )
	{
		if ( SelectedWeapon == i )
		{
			Canvas.SetPos(SelectedAmmoBarX, SelectedAmmoBarY);
		}
		else
		{
			Canvas.SetPos(OffsetX, Canvas.ClipY - HUD.HUDScaleY * (HUD.WeaponBarY + CurrentWeaponScale[i]*HUD.WeaponAmmoOffsetY));
		}
		for ( j=0; j<Gun.Counts[i]; j++ )
		{
			Canvas.DrawColorizedTile(HUD.AltHudTexture, 8 * HUD.HUDScaleY * HUD.WeaponBarScale * CurrentWeaponScale[i], CurrentWeaponScale[i]*OffsetSizeY, 273, 494,12,13,AmmoBarColor);
		}
		OffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
	}

	// draw weapon numbers
	if ( !HUD.bNoWeaponNumbers )
	{
		OffsetX = HUD.HUDScaleX * (HUD.WeaponAmmoOffsetX + HUD.WeaponXOffset) * 0.5 + 0.5 * (Canvas.ClipX - TotalOffsetX);
		OffsetY = Canvas.ClipY - HUD.HUDScaleY * (HUD.WeaponBarY + HUD.WeaponYOffset);
		Canvas.SetDrawColor(255,255,255,255);
		Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(0);
		for ( i=0; i<5; i++ )
		{
			Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
			Canvas.DrawText(int((i+1)%10), false);
			OffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
		}
	}

	// draw weapon icons
	OffsetX = HUD.HUDScaleX * HUD.WeaponXOffset + 0.5 * (Canvas.ClipX - TotalOffsetX);
	OffsetY = Canvas.ClipY - HUD.HUDScaleY * (HUD.WeaponBarY + HUD.WeaponYOffset);
	OffsetSizeX = HUD.HUDScaleX * HUD.WeaponBarScale * 100;
	OffsetSizeY = HUD.HUDScaleY * HUD.WeaponBarScale * HUD.WeaponYScale;
	Canvas.SetDrawColor(255,255,255,255);
	for ( i=0; i<5; i++ )
	{
		Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
		Canvas.DrawTile(HUD.AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, Gun.IconCoords[i].U, Gun.IconCoords[i].V, Gun.IconCoords[i].UL, Gun.IconCoords[i].VL);
		OffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
	}
}

defaultproperties
{
	CameraLag = 0
	FastCamTransitionTime=1.3
	ArmSpeedtune=8192
	bHasWeaponBar=true
}
