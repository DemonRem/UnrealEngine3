/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTAttachment_LinkGun extends UTBeamWeaponAttachment;

/** team based colors for altfire beam when targeting a teammate */
var color LinkBeamColors[3];
/** team based systems for altfire beam when targetting a teammate*/
var ParticleSystem LinkBeamSystems[3];
/** color for altfire beam when not targeting a teammate and others are linked to us */
var color HighPowerBeamColor;
/** particle system for powered up link beams*/
var ParticleSystem HighPowerSystem;

var MaterialInstanceConstant WeaponMaterialInstance;

var AudioComponent LinkHitSound;

/** templates for beam impact effect */
var ParticleSystem TeamBeamEndpointTemplates[3];
var ParticleSystem HighPowerBeamEndpointTemplate;
/** emitter playing the endpoint effect */
var UTEmitter BeamEndpointEffect;

/** templates for muzzle flash */
var ParticleSystem TeamMuzzleFlashTemplates[3];
var ParticleSystem HighPowerMuzzleFlashTemplate;

/** activated whenever we're linked to other players */
var ParticleSystemComponent PoweredUpEffect;
/** socket to attach PoweredUpEffect to on our mesh */
var name PoweredUpEffectSocket;

static function GetTeamBeamInfo(byte TeamNum, optional out color BeamColor, optional out ParticleSystem BeamSystem, optional out ParticleSystem BeamEndpointTemplate)
{
	BeamColor = default.LinkBeamColors[(TeamNum < ArrayCount(default.LinkBeamColors)) ? int(TeamNum) : ArrayCount(default.LinkBeamColors) - 1];
	BeamSystem = default.LinkBeamSystems[(TeamNum < ArrayCount(default.LinkBeamSystems)) ? int(TeamNum) : ArrayCount(default.LinkBeamSystems) - 1];
	BeamEndpointTemplate = default.TeamBeamEndpointTemplates[(TeamNum < ArrayCount(default.TeamBeamEndpointTemplates)) ? int(TeamNum) : ArrayCount(default.TeamBeamEndpointTemplates) - 1];
}

function ParticleSystem GetTeamMuzzleFlashTemplate(byte TeamNum)
{
	if (TeamNum >= ArrayCount(default.TeamMuzzleFlashTemplates))
	{
		TeamNum = ArrayCount(default.TeamMuzzleFlashTemplates) - 1;
	}
	return default.TeamMuzzleFlashTemplates[TeamNum];
}

simulated function PostBeginPlay()
{
	local color DefaultBeamColor;

	Super.PostBeginPlay();

	if (Mesh != None)
	{
		GetTeamBeamInfo(255, DefaultBeamColor);
		WeaponMaterialInstance = Mesh.CreateAndSetMaterialInstanceConstant(0);
		WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(DefaultBeamColor));
		if (PoweredUpEffect != None)
		{
			Mesh.AttachComponentToSocket(PoweredUpEffect, PoweredUpEffectSocket);
			SetTimer(0.25, true, 'CheckPoweredUp');
		}
	}
}

simulated function SetSkin(Material NewMaterial)
{
	Super.SetSkin(NewMaterial);

	if (NewMaterial == None && Mesh != None)
	{
		Mesh.SetMaterial(0, WeaponMaterialInstance);
	}
}

simulated function SetImpactedActor(Actor HitActor, vector HitLocation, vector HitNormal)
{
	local SoundCue DesiredLinkHitSound;

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if (HitActor != None)
		{
			DesiredLinkHitSound = (UTPawn(HitActor) != None)
							? SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_AltFireImpactFleshCue'
							: SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_AltFireImpactCue';

			if (LinkHitSound == None || LinkHitSound.SoundCue != DesiredLinkHitSound)
			{
				if (LinkHitSound != None)
				{
					LinkHitSound.FadeOut(0.1f, 0.0f);
				}
				if (Instigator != None)
				{
					LinkHitSound = Instigator.CreateAudioComponent(DesiredLinkHitSound, false, true);
				}
				if (LinkHitSound != None)
				{
					LinkHitSound.FadeIn(0.1f, 1.0f);
				}
			}

			if (BeamEndpointEffect != None)
			{
				BeamEndpointEffect.SetRotation(rotator(HitNormal));
			}
		}
		else if (LinkHitSound != None)
		{
			LinkHitSound.FadeOut(0.1f,0.0f);
			LinkHitSound = None;
		}
	}
}

simulated function StopThirdPersonFireEffects()
{
	Super.StopThirdPersonFireEffects();

	if (LinkHitSound != None)
	{
		LinkHitSound.FadeOut(0.1f, 0.0f);
		LinkHitSound = none;
	}
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (LinkHitSound != None)
	{
		LinkHitSound.Stop();
	}

	KillEndpointEffect();
}

simulated function UpdateBeam(byte FireModeNum)
{
	local color BeamColor;
	local byte RealFireModeNum, TeamNum;
	local vector EndPoint, CameraDir, BeamDir, CameraLocation, BeamStart;
	local rotator CameraRotation;
	local PlayerController PC;
	local ParticleSystem BeamSystem, BeamEndpointTemplate, MuzzleFlashTemplate;;

	// the weapon sets a FiringMode of 2 instead of 1 when using altfire on a teammate (so team color it)
	// FiringMode == 3 when using altfire (not on a teammate) while others linked up to this gun
	RealFireModeNum = (FireModeNum >= 2) ? byte(1) : FireModeNum;

	// Make sure the Emitter is visible
	if (BeamEmitter[RealFireModeNum] != None)
	{
		EndPoint = PawnOwner.FlashLocation;
		// if the beam would pass through the camera, adjust the endpoint so it ends in front of the camera (looks better)
		//@todo: this will have issues with splitscreen
		foreach LocalPlayerControllers(class'PlayerController', PC)
		{
			PC.GetPlayerViewPoint(CameraLocation, CameraRotation);

			CameraDir = vector(CameraRotation);
			BeamStart = BeamEmitter[RealFireModeNum].GetPosition();
			BeamDir = Normal(EndPoint - BeamStart);
			if ((CameraDir dot BeamDir) < -0.75f && (Normal(EndPoint - CameraLocation) dot BeamDir) > 0.0f)
			{
				EndPoint = BeamStart + BeamDir * (VSize(CameraLocation - BeamStart) - 20.0f);
				break;
			}
		}

		BeamEmitter[RealFireModeNum].SetVectorParameter(EndPointParamName, EndPoint);

		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			if (BeamEndpointEffect != None && !BeamEndpointEffect.bDeleteMe)
			{
				BeamEndpointEffect.SetLocation(EndPoint);
			}
			else
			{
				BeamEndpointEffect = Spawn(class'UTEmitter', self,, EndPoint);
				BeamEndpointEFfect.LifeSpan = 0.0;
			}
		}
	}

	HideEmitter(RealFireModeNum, false);
	HideEmitter(Abs(RealFireModeNum - 1), true);

	if (FireModeNum == 2 && WorldInfo.GRI.GameClass.Default.bTeamGame)
	{
		TeamNum = Instigator.GetTeamNum();
		GetTeamBeamInfo(TeamNum, BeamColor, BeamSystem, BeamEndpointTemplate);
		MuzzleFlashTemplate = GetTeamMuzzleFlashTemplate(TeamNum);
	}
	else if (FireModeNum == 3 || FireModeNum == 4)
	{
		BeamColor = HighPowerBeamColor;
		BeamSystem = HighPowerSystem;
		BeamEndpointTemplate = HighPowerBeamEndpointTemplate;
		MuzzleFlashTemplate = HighPowerMuzzleFlashTemplate;
		/** FIXME: set particle param if mode == 4 (hit something)
		if (FireModeNum == 4)
		{
		}
		*/
	}
	else
	{
		GetTeamBeamInfo(255, BeamColor, BeamSystem, BeamEndpointTemplate);
		MuzzleFlashTemplate = GetTeamMuzzleFlashTemplate(255);
		/** FIXME: set particle param if mode == 5 (hit something)
		if (FireModeNum == 5)
		{
		}
		*/
	}

	if (FireModeNum > 0)
	{
		if (BeamEmitter[1] != None)
		{
			if (BeamEmitter[1].Template != BeamSystem)
			{
				BeamEmitter[1].SetTemplate(BeamSystem);
			}
			BeamEmitter[1].SetColorParameter('Link_Beam_Color', BeamColor);
		}
		if (BeamEndpointEffect != None && BeamEndpointEffect.ParticleSystemComponent.Template != BeamEndpointTemplate)
		{
			BeamEndpointEffect.SetTemplate(BeamEndpointTemplate, true);
		}
		if (MuzzleFlashTemplate != MuzzleFlashPSC.Template)
		{
			MuzzleFlashPSC.SetTemplate(MuzzleFlashTemplate);
		}
	}

	if (MuzzleFlashPSC != None)
	{
		MuzzleFlashPSC.SetColorParameter('Link_Beam_Color', BeamColor);
	}
	if (UTLinkGunMuzzleFlashLight(MuzzleFlashLight) != None)
	{
		UTLinkGunMuzzleFlashLight(MuzzleFlashLight).SetTeam((FireModeNum == 2 && WorldInfo.GRI.GameClass.Default.bTeamGame) ? Instigator.GetTeamNum() : byte(255));
	}

	if (WeaponMaterialInstance != None)
	{
		WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(BeamColor));
	}
}

/** deactivates the beam endpoint effect, if present */
simulated function KillEndpointEffect()
{
	if (BeamEndpointEffect != None)
	{
		BeamEndpointEffect.ParticleSystemComponent.DeactivateSystem();
		BeamEndpointEffect.LifeSpan = 2.0;
		BeamEndpointEffect = None;
	}
}

simulated function HideEmitter(int Index, bool bHide)
{
	local color EffectColor;

	Super.HideEmitter(Index, bHide);

	if (Index == 1 && bHide)
	{
		// reset material and muzzle flash to default color
		GetTeamBeamInfo(255, EffectColor);
		if (WeaponMaterialInstance != None)
		{
			WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(EffectColor));
		}
		if (MuzzleFlashPSC != None)
		{
			MuzzleFlashPSC.ClearParameter('Link_Beam_Color');
		}

		KillEndpointEffect();
	}
}

simulated function CheckPoweredUp()
{
	local UTPawn UTP;
	local bool bPoweredUp;

	if (Instigator != None && WorldInfo.GRI != None)
	{
		foreach WorldInfo.AllPawns(class'UTPawn', UTP, Instigator.Location, class<UTWeap_LinkGun>(WeaponClass).default.WeaponLinkDistance)
		{
			if ( UTP != Instigator && !UTP.bNoWeaponFiring && UTP.DrivenVehicle == None && UTAttachment_LinkGun(UTP.CurrentWeaponAttachment) != None &&
				WorldInfo.GRI.OnSameTeam(Instigator, UTP) && FastTrace(UTP.Location, Instigator.Location) )
			{
				bPoweredUp = true;
				break;
			}
		}

		if (bPoweredUp)
		{
			if (!PoweredUpEffect.bIsActive)
			{
				PoweredUpEffect.ActivateSystem();
			}
		}
		else if (PoweredUpEffect.bIsActive)
		{
			PoweredUpEffect.DeactivateSystem();
		}
	}
}

simulated function ChangeVisibility(bool bIsVisible)
{
	Super.ChangeVisibility(bIsVisible);

	if (PoweredUpEffect != None)
	{
		PoweredUpEffect.SetHidden(!bIsVisible);
	}
}

defaultproperties
{
	// Weapon SkeletalMesh
	Begin Object Name=SkeletalMeshComponent0
		SkeletalMesh=SkeletalMesh'WP_LinkGun.Mesh.SK_WP_LinkGun_3P'
	End Object

	Begin Object Class=ParticleSystemComponent Name=PoweredUpComponent
		Template=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_PoweredUp'
		bAutoActivate=false
		SecondsBeforeInactive=1.0f
	End Object
	PoweredUpEffect=PoweredUpComponent
	PoweredUpEffectSocket=PowerEffectSocket

	LinkBeamColors(0)=(R=255,G=64,B=64,A=255)
	LinkBeamColors(1)=(R=64,G=64,B=255,A=255)
	LinkBeamColors(2)=(R=128,G=220,B=120,A=255)
	HighPowerBeamColor=(R=192,G=192,B=32,A=255)

	LinkBeamSystems[0]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam_Red'
	LinkBeamSystems[1]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam_Blue'
	LinkBeamSystems[2]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam'

	HighPowerSystem=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam_Gold'

	TeamBeamEndpointTemplates[0]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Beam_Impact_Red'
	TeamBeamEndpointTemplates[1]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Beam_Impact_Blue'
	TeamBeamEndpointTemplates[2]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Beam_Impact'
	HighPowerBeamEndpointTemplate=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Beam_Impact_Gold'

	TeamMuzzleFlashTemplates[0]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_3P_Beam_MF_Red'
	TeamMuzzleFlashTemplates[1]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_3P_Beam_MF_Blue'
	TeamMuzzleFlashTemplates[2]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_3P_Beam_MF'
	HighPowerMuzzleFlashTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_3P_Beam_MF_Gold'

	MuzzleFlashSocket=MussleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_3P_Primary_MF'
	MuzzleFlashAltPSCTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_3P_Beam_MF'
	MuzzleFlashColor=(R=120,G=255,B=120,A=255)
	MuzzleFlashDuration=0.33
	bMuzzleFlashPSCLoops=true
	MuzzleFlashLightClass=class'UTLinkGunMuzzleFlashLight'

	BeamTemplate[1]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam'
	BeamSockets[1]=MussleFlashSocket02

	EndPointParamName=LinkBeamEnd
	WeaponClass=class'UTWeap_LinkGun'
}
