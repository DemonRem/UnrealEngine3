/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_SniperRifle extends UTWeapon;

var class<UTDamageType> HeadShotDamageType;
var float HeadShotDamageMult;

var Texture2D HudMaterial;

var array<MaterialInterface> TeamSkins;

/** headshot scale factor when moving slowly or stopped */
var float SlowHeadshotScale;
/** headshot scale factor when running or falling */
var float RunningHeadshotScale;
/** Zoom minimum time*/
var bool bAbortZoom;
/** sound while the zoom is in progress */
var audiocomponent ZoomLoop;
var soundcue ZoomLoopCue;
/** Whether the standard crosshair should be displayed */
var bool bDisplayCrosshair;
/** tracks number of zoom started calls before zoom is ended */
var int ZoomCount;
//-----------------------------------------------------------------
// AI Interface

function float SuggestAttackStyle()
{
    return -0.4;
}

function float SuggestDefenseStyle()
{
    return 0.2;
}

function float GetAIRating()
{
	local UTBot B;
	local float ZDiff, dist, Result;

	B = UTBot(Instigator.Controller);
	if ( B == None )
		return AIRating;
	if ( B.IsShootingObjective() )
		return AIRating - 0.15;
	if ( B.Enemy == None )
		return AIRating;

	if ( B.Stopped() )
		result = AIRating + 0.1;
	else
		result = AIRating - 0.1;
	if ( Vehicle(B.Enemy) != None )
		result -= 0.2;
	ZDiff = Instigator.Location.Z - B.Enemy.Location.Z;
	if ( ZDiff < -200 )
		result += 0.1;
	dist = VSize(B.Enemy.Location - Instigator.Location);
	if ( dist > 2000 )
	{
		if ( !B.LineOfSightTo(B.Enemy) )
			result = result - 0.15;
		return ( FMin(2.0,result + (dist - 2000) * 0.0002) );
	}
	if ( !B.LineOfSightTo(B.Enemy) )
		return AIRating - 0.1;

	return result;
}

function bool RecommendRangedAttack()
{
	local UTBot B;

	B = UTBot(Instigator.Controller);
	if ( (B == None) || (B.Enemy == None) )
		return true;

	return ( VSize(B.Enemy.Location - Instigator.Location) > 2000 * (1 + FRand()) );
}

simulated function ProcessInstantHit( byte FiringMode, ImpactInfo Impact )
{
	local float Scaling;
	local int HeadDamage;

	if( (Role == Role_Authority) && !bUsingAimingHelp )
	{
		if (Instigator == None || VSize(Instigator.Velocity) < Instigator.GroundSpeed * Instigator.CrouchedPct)
		{
			Scaling = SlowHeadshotScale;
		}
		else
		{
			Scaling = RunningHeadshotScale;
		}

		HeadDamage = InstantHitDamage[FiringMode]* HeadShotDamageMult;
		if ( (UTPawn(Impact.HitActor) != None && UTPawn(Impact.HitActor).TakeHeadShot(Impact, HeadShotDamageType, HeadDamage, Scaling, Instigator.Controller)) ||
			(UTVehicleBase(Impact.HitActor) != None && UTVehicleBase(Impact.HitActor).TakeHeadShot(Impact, HeadShotDamageType, HeadDamage, Scaling, Instigator.Controller)) )
		{
			SetFlashLocation(Impact.HitLocation);
			return;
		}
	}

	super.ProcessInstantHit( FiringMode, Impact );
}


simulated function DrawZoomedOverlay( HUD H )
{
	local float CX,CY,Sc;

	bDisplayCrosshair = false;

	CX = H.Canvas.ClipX/2;
	CY = H.Canvas.ClipY/2;
	SC = H.Canvas.ClipX/1024;

	H.Canvas.SetDrawColor(0,0,0);

	// Draw the crosshair
	H.Canvas.SetPos(CX-169*SC,CY-155*SC);
	H.Canvas.DrawTile(HudMaterial,169*SC,310*SC, 164,35, 169,310);
	H.Canvas.SetPos(CX,CY-155*SC);
	H.Canvas.DrawTile(HudMaterial,169*SC,310*SC, 332,345, -169,-310);

	// Draw Cornerbars
	H.Canvas.SetPos(160*SC,160*SC);
	H.Canvas.DrawTile(HudMaterial, 111*SC, 111*SC , 0 , 0, 111, 111);

	H.Canvas.SetPos(H.Canvas.ClipX-271*SC,160*SC);
	H.Canvas.DrawTile(HudMaterial, 111*SC, 111*SC , 111 , 0, -111, 111);

	H.Canvas.SetPos(160*SC,H.Canvas.ClipY-271*SC);
	H.Canvas.DrawTile(HudMaterial, 111*SC, 111*SC, 0 , 111, 111, -111);

	H.Canvas.SetPos(H.Canvas.ClipX-271*SC,H.Canvas.ClipY-271*SC);
	H.Canvas.DrawTile(HudMaterial, 111*SC, 111*SC , 111 , 111, -111, -111);

	// Draw the 4 corners
	H.Canvas.SetPos(0,0);
	H.Canvas.DrawTile(HudMaterial,160*SC,160*SC, 0, 274, 159, -158);

	H.Canvas.SetPos(H.Canvas.ClipX-160*SC,0);
	H.Canvas.DrawTile(HudMaterial,160*SC,160*SC, 159,274, -159, -158);

	H.Canvas.SetPos(0,H.Canvas.ClipY-160*SC);
	H.Canvas.DrawTile(HudMaterial,160*SC,160*SC, 0,116, 159, 158);

	H.Canvas.SetPos(H.Canvas.ClipX-160*SC,H.Canvas.ClipY-160*SC);
	H.Canvas.DrawTile(HudMaterial,160*SC,160*SC, 159, 116, -159, 158);

	// Draw the Horz Borders
	H.Canvas.SetPos(160*SC,0);
	H.Canvas.DrawTile(HudMaterial, H.Canvas.ClipX-320*SC, 160*SC, 284, 512, 32, -160);

	H.Canvas.SetPos(160*SC,H.Canvas.ClipY-160*SC);
	H.Canvas.DrawTile(HudMaterial, H.Canvas.ClipX-320*SC, 160*SC, 284, 352, 32, 160);

	// Draw the Vert Borders
	H.Canvas.SetPos(0,160*SC);
	H.Canvas.DrawTile(HudMaterial, 160*SC, H.Canvas.ClipY-320*SC, 0,308, 160,32);

	H.Canvas.SetPos(H.Canvas.ClipX-160*SC,160*SC);
	H.Canvas.DrawTile(HudMaterial, 160*SC, H.Canvas.ClipY-320*SC, 160,308, -160,32);
}

simulated function DrawWeaponCrosshair( Hud HUD )
{
	if( bDisplayCrosshair )
		super.DrawWeaponCrosshair(HUD);
}
/** Called when zooming starts
 * @param PC - cast of Instigator.Controller for convenience
 */
simulated function StartZoom(UTPlayerController PC)
{
	ZoomCount++;
	if (ZoomCount == 1 && !IsTimerActive('Gotozoom') && IsActiveWeapon() && HasAmmo(0) && Instigator.IsFirstPerson())
	{
		bDisplayCrosshair = false;
		PlayWeaponAnimation('WeaponZoomIn',0.2);
		PlayArmAnimation('WeaponZoomIn',0.2);
		bAbortZoom = false;
		SetTimer(0.2, false, 'Gotozoom');
	}
}

simulated function EndFire(Byte FireModeNum)
{
	local UTPlayerController PC;

	// Don't bother performing if this is a dedicated server

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		PC = UTPlayerController(Instigator.Controller);
		if (PC != None && LocalPlayer(PC.Player) != none && FireModeNum < bZoomedFireMode.Length && bZoomedFireMode[FireModeNum] != 0 )
		{
			if(GetZoomedState() == ZST_NotZoomed && ZoomCount != 0)
			{
				if(bAbortZoom)
				{
					ClearTimer('Gotozoom');
					LeaveZoom();
				}
				else
				{
					bAbortZoom=true;
				}
			}
			if(ZoomLoop != none)
			{
				ZoomLoop.Stop();
				ZoomLoop = none;
			}
		}
	}
	super.EndFire(FireModeNum);
}

/** Called when zooming ends
 * @param PC - cast of Instigator.Controller for convenience
 */
simulated function EndZoom(UTPlayerController PC)
{
	PlaySound(ZoomOutSound, true);
	bAbortZoom = false;
	if (IsTimerActive('Gotozoom'))
	{
		ClearTimer('Gotozoom');
	}
	SetTimer(0.001,false,'LeaveZoom');

}
simulated function LeaveZoom()
{
	local UTPlayerController PC;

	bAbortZoom = false;
	PC = UTPlayerController(Instigator.Controller);
	if (PC != none)
	{
		PC.EndZoom();
	}
	ZoomCount = 0;
	if(Instigator.IsFirstPerson())
	{
		ChangeVisibility(true);
	}

	PlayWeaponAnimation('WeaponZoomOut',0.3);
	PlayArmAnimation('WeaponZoomOut',0.3);
	SetTimer(0.3,false,'RestartCrosshair');

}
simulated function ChangeVisibility(bool bIsVisible)
{
	super.Changevisibility(bIsvisible);
	if(bIsVisible)
	{
		PlayArmAnimation('WeaponZoomOut',0.00001); // to cover zooms ended while in 3p
	}
	if(!Instigator.IsFirstPerson()) // to be consistent with not allowing zoom from 3p
	{
		LeaveZoom();
	}

}
simulated function RestartCrosshair()
{
	bDisplayCrosshair = true;
}

simulated function PutDownWeapon()
{
	ClearTimer('GotoZoom');
	ClearTimer('StopZoom');
	LeaveZoom();
	super.PutDownWeapon();
}

simulated function HolderEnteredVehicle()
{
	local UTPawn UTP;

	// clear timers and reset anims
	ClearTimer('GotoZoom');
	ClearTimer('StopZoom');
	PlayWeaponAnimation('WeaponZoomOut', 0.3);

	// partially copied from PlayArmAnimation() - we don't want to abort if third person here because they definitely will
	// since they just got in a vehicle
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		UTP = UTPawn(Instigator);
		if (UTP != None)
		{
			// Check we have access to mesh and animations
			if (UTP.ArmsMesh[0] != None && ArmsAnimSet != None && GetArmAnimNodeSeq() != None)
			{
				UTP.ArmsMesh[0].PlayAnim('WeaponZoomOut', 0.3, false);
			}
		}
	}
}

simulated function Gotozoom()
{
	local UTPlayerController PC;

	PC = UTPlayerController(Instigator.Controller);
	if (GetZoomedState() == ZST_NotZoomed)
	{
		PC.FOVAngle = 40;
		Super.StartZoom(PC);
		ChangeVisibility(false);
		if (bAbortZoom) // stop the zoom after 1 tick
		{
			SetTimer(0.0001, false, 'StopZoom');
		}
		else 
		{
			if(ZoomLoop == none)
			{
				ZoomLoop = CreateAudioComponent(ZoomLoopCue, false, true);
			}
			if(ZoomLoop != none)
			{
				ZoomLoop.Play();
			}
		}
	}

}
simulated function PlayWeaponPutDown()
{
	ClearTimer('GotoZoom');
	ClearTimer('StopZoom');
	if(UTPlayerController(Instigator.Controller) != none)
	{
		UTPlayerController(Instigator.Controller).EndZoom();
	}
	super.PlayWeaponPutDown();
}

simulated function StopZoom()
{
	local UTPlayerController PC;

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		PC = UTPlayerController(Instigator.Controller);
		if (PC != None && LocalPlayer(PC.Player) != none)
		{
			PC.StopZoom();
		}
	}
}

simulated event CauseMuzzleFlash()
{
	if(GetZoomedState() == ZST_NotZoomed)
	{
		super.CauseMuzzleFlash();
	}
}
function byte BestMode()
{
	return 0;
}

simulated function SetSkin(Material NewMaterial)
{
	local int TeamIndex;

	if ( NewMaterial == None ) 	// Clear the materials
	{
		TeamIndex = Instigator.GetTeamNum();
		if (TeamIndex > TeamSkins.length)
		{
			TeamIndex = 0;
		}
		Mesh.SetMaterial(0,TeamSkins[TeamIndex]);
	}
	else
	{
		Super.SetSkin(NewMaterial);
	}
}


defaultproperties
{
	WeaponColor=(R=255,G=0,B=64,A=255)
	PlayerViewOffset=(X=0,Y=0,Z=0)

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	// Weapon SkeletalMesh
	Begin Object Name=FirstPersonMesh
	    SkeletalMesh=SkeletalMesh'WP_SniperRifle.Mesh.SK_WP_SniperRifle_1P'
		AnimSets(0)=AnimSet'WP_SniperRifle.Anims.K_WP_SniperRifle_1P_Base'
		Animations=MeshSequenceA
		Scale=1.5
		FOV=65
	End Object
	AttachmentClass=class'UTAttachment_SniperRifle'

	ArmsAnimSet=AnimSet'WP_SniperRifle.Anims.K_WP_SniperRifle_1P_Arms'

	// Pickup staticmesh
	Components.Remove(PickupMesh)
	Begin Object Name=PickupMesh
		SkeletalMesh=SkeletalMesh'WP_SniperRifle.Mesh.SK_WP_SniperRifle_3P'
	End Object
		
	ZoomLoopCue=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_ZoomLoop_Cue'
	
	InstantHitDamage(0)=70
	InstantHitDamage(1)=0
	InstantHitMomentum(0)=10000.0

	FireInterval(0)=+1.33
	FireInterval(1)=+1.33

	FiringStatesArray(1)=Active

	WeaponRange=17000

	InstantHitDamageTypes(0)=class'UTDmgType_SniperPrimary'
	InstantHitDamageTypes(1)=None

	WeaponFireSnd[0]=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_Fire_Cue'
	//WeaponFireSnd[1]=SoundCue'A_Weapon.SniperRifle.Cue.A_Weapon_SN_Fire01_Cue'
	WeaponEquipSnd=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_Raise_Cue'
	WeaponPutDownSnd=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_Lower_Cue'
	ZoomOutSound=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_ZoomOut_Cue'
	ZoomInSound=SoundCue'A_Weapon_Sniper.Sniper.A_Weapon_Sniper_ZoomIn_Cue'

	WeaponFireTypes(0)=EWFT_InstantHit

	LockerRotation=(pitch=0,yaw=0,roll=-16384)

	MaxDesireability=0.63
	AIRating=+0.7
	CurrentRating=+0.7
	BotRefireRate=0.7
	bInstantHit=true
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=true
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0
	InventoryGroup=9
	GroupWeight=0.5
	AimError=850

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Sniper_Cue'

	AmmoCount=10
	LockerAmmoCount=10
	MaxAmmoCount=40

	HeadShotDamageType=class'UTDmgType_SniperHeadShot'
	HeadShotDamageMult=2.0
	SlowHeadshotScale=1.75
	RunningHeadshotScale=0.8

	// Configure the zoom

	bZoomedFireMode(0)=0
	bZoomedFireMode(1)=1

	ZoomedTargetFOV=12.0
	ZoomedRate=30.0

	FadeTime=0.3

	FireShake(0)=(OffsetMag=(X=-15.0,Y=0.0,Z=10.0),OffsetRate=(X=-4000.0,Y=0.0,Z=4000.0),OffsetTime=1.6,RotMag=(X=0.0,Y=0.0,Z=0.0),RotRate=(X=0.0,Y=0.0,Z=0.0),RotTime=2)
	HudMaterial=Texture2D'WP_SniperRifle.Textures.T_SniperCrosshair'

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_SniperRifle.Effects.P_WP_SniperRifle_MuzzleFlash'
	MuzzleFlashDuration=0.33
	MuzzleFlashLightClass=class'UTGame.UTRocketMuzzleFlashLight'

	IconX=451
	IconY=448
	IconWidth=54
	IconHeight=51

	EquipTime=+0.6
	PutDownTime=+0.45
	CrossHairCoordinates=(U=256,V=64,UL=64,VL=64)
	IconCoordinates=(U=357,V=107,UL=84,VL=50)
	QuickPickGroup=5
	QuickPickWeight=0.8

	TeamSkins[0]=MaterialInterface'WP_SniperRifle.Materials.M_WP_SniperRifle'
	TeamSkins[1]=MaterialInterface'WP_SniperRifle.Materials.M_WP_SniperRifle_Blue'

	bDisplaycrosshair = true;
}
