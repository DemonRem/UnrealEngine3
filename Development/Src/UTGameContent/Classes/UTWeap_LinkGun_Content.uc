/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_LinkGun_Content extends UTWeap_LinkGun;

/** cached cast of attachment class for calling coloring functions */
var class<UTAttachment_LinkGun> LinkAttachmentClass;

var ParticleSystem TeamMuzzleFlashTemplates[3];
var ParticleSystem HighPowerMuzzleFlashTemplate;

simulated function PostBeginPlay()
{
	local color DefaultBeamColor;

	LinkAttachmentClass = class<UTAttachment_LinkGun>(AttachmentClass);

	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer && Mesh != None)
	{
		LinkAttachmentClass.static.GetTeamBeamInfo(255, DefaultBeamColor);
		WeaponMaterialInstance = Mesh.CreateMaterialInstance(0);
		WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(DefaultBeamColor));
	}
}

simulated function ParticleSystem GetTeamMuzzleFlashTemplate(byte TeamNum)
{
	if (TeamNum >= ArrayCount(default.TeamMuzzleFlashTemplates))
	{
		TeamNum = ArrayCount(default.TeamMuzzleFlashTemplates) - 1;
	}
	return default.TeamMuzzleFlashTemplates[TeamNum];
}

simulated function UpdateBeamEmitter(vector FlashLocation, vector HitNormal)
{
	local color BeamColor;
	local ParticleSystem BeamSystem, BeamEndpointTemplate, MuzzleFlashTemplate;
	local byte TeamNum;

	if (LinkedTo != None)
	{
		FlashLocation = GetLinkedToLocation();
	}

	Super.UpdateBeamEmitter(FlashLocation, HitNormal);

	if (LinkedTo != None && WorldInfo.GRI.GameClass.Default.bTeamGame)
	{
		TeamNum = Instigator.GetTeamNum();
		LinkAttachmentClass.static.GetTeamBeamInfo(TeamNum, BeamColor, BeamSystem, BeamEndpointTemplate);
		MuzzleFlashTemplate = GetTeamMuzzleFlashTemplate(TeamNum);
	}
	else if ( (LinkStrength > 1) || (Instigator.DamageScaling >= 2.0) )
	{
		if ( bBeamHit )
			BeamColor = LinkAttachmentClass.default.HighHitBeamColor;
		else
			BeamColor = LinkAttachmentClass.default.HighPowerBeamColor;
		BeamSystem = LinkAttachmentClass.default.HighPowerSystem;
		BeamEndpointTemplate = LinkAttachmentClass.default.HighPowerBeamEndpointTemplate;
		MuzzleFlashTemplate = HighPowerMuzzleFlashTemplate;
	}
	else
	{
		LinkAttachmentClass.static.GetTeamBeamInfo(255, BeamColor, BeamSystem, BeamEndpointTemplate);
		if ( bBeamHit )
		{
			BeamColor = LinkAttachmentClass.default.HitBeamColor;
		}

		MuzzleFlashTemplate = GetTeamMuzzleFlashTemplate(255);
	}

	if ( BeamLight != None )
	{
		if ( HitNormal == vect(0,0,0) )
		{
			BeamLight.Beamlight.Radius = 48;
			if ( FastTrace(FlashLocation, FlashLocation-vect(0,0,32)) )
				BeamLight.SetLocation(FlashLocation - vect(0,0,32));
			else
				BeamLight.SetLocation(FlashLocation);
		}
		else
		{
			BeamLight.Beamlight.Radius = 32;
			BeamLight.SetLocation(FlashLocation + 16*HitNormal);
		}
		BeamLight.BeamLight.SetLightProperties(, BeamColor);
	}

	if (BeamEmitter[CurrentFireMode] != None)
	{
		BeamEmitter[CurrentFireMode].SetColorParameter('Link_Beam_Color', BeamColor);
		if (BeamEmitter[CurrentFireMode].Template != BeamSystem)
		{
			BeamEmitter[CurrentFireMode].SetTemplate(BeamSystem);
		}
	}

	if (MuzzleFlashPSC != None)
	{
		MuzzleFlashPSC.SetColorParameter('Link_Beam_Color', BeamColor);
		if (MuzzleFlashTemplate != MuzzleFlashPSC.Template)
		{
			MuzzleFlashPSC.SetTemplate(MuzzleFlashTemplate);
		}
	}
	if (UTLinkGunMuzzleFlashLight(MuzzleFlashLight) != None)
	{
		UTLinkGunMuzzleFlashLight(MuzzleFlashLight).SetTeam((LinkedTo != None && WorldInfo.GRI.GameClass.Default.bTeamGame) ? Instigator.GetTeamNum() : byte(255));
	}

	if (WeaponMaterialInstance != None)
	{
		WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(BeamColor));
	}

	if (WorldInfo.NetMode != NM_DedicatedServer && Instigator != None && Instigator.IsFirstPerson())
	{
		if (BeamEndpointEffect != None && !BeamEndpointEffect.bDeleteMe)
		{
			BeamEndpointEffect.SetLocation(FlashLocation);
			BeamEndpointEffect.SetRotation(rotator(HitNormal));
			if (BeamEndpointEffect.ParticleSystemComponent.Template != BeamEndpointTemplate)
			{
				BeamEndpointEffect.SetTemplate(BeamEndpointTemplate, true);
			}
		}
		else
		{
			BeamEndpointEffect = Spawn(class'UTEmitter', self,, FlashLocation, rotator(HitNormal));
			BeamEndpointEffect.SetTemplate(BeamEndpointTemplate, true);
			BeamEndpointEFfect.LifeSpan = 0.0;
		}
	}
}

simulated state WeaponBeamFiring
{
	simulated function EndState(Name NextStateName)
	{
		local color EffectColor;

		WeaponPlaySound(EndAltFireSound);

		Super.EndState(NextStateName);

		if ( BeamLight != None )
			BeamLight.Destroy();

		ReaccquireTimer = 0.0;
		UnLink();
		Victim = None;

		// reset material and muzzle flash to default color
		LinkAttachmentClass.static.GetTeamBeamInfo(255, EffectColor);
		if (WeaponMaterialInstance != None)
		{
			WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(EffectColor));
		}
		if (MuzzleFlashPSC != None)
		{
			MuzzleFlashPSC.ClearParameter('Link_Beam_Color');
		}
	}
}

simulated function Projectile ProjectileFire()
{
	local UTProj_LinkPlasma Proj;

	IncrementFlashCount();
	if (Role == ROLE_Authority)
	{
		CalcLinkStrength();
		Proj = UTProj_LinkPlasma(super.ProjectileFire());
		if (LinkStrength > 1) // powered shot
		{
			if (UTProj_LinkPowerPlasma(Proj) != None)
			{
				UTProj_LinkPowerPlasma(Proj).SetStrength(LinkStrength);
			}
		}


	}
	return none;
}

function class<Projectile> GetProjectileClass()
{
	return (CurrentFireMode == 0 && LinkStrength > 1) ? class'UTProj_LinkPowerPlasma' : Super.GetProjectileClass();
}

defaultproperties
{
	WeaponColor=(R=255,G=255,B=0,A=255)
	FireInterval(0)=+0.16
	FireInterval(1)=+0.35
	PlayerViewOffset=(X=16.0,Y=-18,Z=-18.0)

	FiringStatesArray(1)=WeaponBeamFiring

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
		bCauseActorAnimEnd=true
	End Object

	// Weapon SkeletalMesh
	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_LinkGun.Mesh.SK_WP_Linkgun_1P'
		AnimSets(0)=AnimSet'WP_LinkGun.Anims.K_WP_LinkGun_1P_Base'
		Animations=MeshSequenceA
		Scale=0.9
		FOV=60.0
	End Object

	AttachmentClass=class'UTGameContent.UTAttachment_Linkgun'

	// Pickup staticmesh
	Begin Object Class=StaticMeshComponent Name=MeshComponent1
		StaticMesh=StaticMesh'WP_LinkGun.Mesh.S_WP_LinkGun_3P'
		bOnlyOwnerSee=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CollideActors=false
		Translation=(X=0.0,Y=0.0,Z=-10.0)
		Rotation=(Yaw=32768)
		Scale=0.75
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=MeshComponent1
	PickupFactoryMesh=MeshComponent1

	Begin Object Class=ParticleSystemComponent Name=PoweredUpComponent
		Template=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_PoweredUp'
		bAutoActivate=false
		DepthPriorityGroup=SDPG_Foreground
	End Object
	PoweredUpEffect=PoweredUpComponent
	PoweredUpEffectSocket=PowerEffectSocket

	FireOffset=(X=12,Y=10,Z=-10)

	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_LinkPlasma' // UTProj_LinkPowerPlasma if linked (see GetProjectileClass() )

	WeaponEquipSnd=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_RaiseCue'
	WeaponPutDownSnd=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_LowerCue'
	WeaponFireSnd(0)=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_FireCue'
	WeaponFireSnd(1)=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_AltFireCue'

	MaxDesireability=0.7
	AIRating=+0.71
	CurrentRating=+0.71
	BotRefireRate=0.99
	bInstantHit=false
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0
	InventoryGroup=5
	GroupWeight=0.5

	WeaponRange=900
	LinkStrength=1
	LinkFlexibility=0.64	// determines how easy it is to maintain a link.
							// 1=must aim directly at linkee, 0=linkee can be 90 degrees to either side of you

	LinkBreakDelay=0.5		// link will stay established for this long extra when blocked (so you don't have to worry about every last tree getting in the way)
	WeaponLinkDistance=350.0

	InstantHitDamage(1)=100
	InstantHitDamageTypes(1)=class'UTDmgType_LinkBeam'

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Link_Cue'

	AmmoCount=50
	LockerAmmoCount=70
	MaxAmmoCount=220
	MomentumTransfer=50000.0
	BeamAmmoUsePerSecond=8.5

	FireShake(0)=(OffsetMag=(X=0.0,Y=1.0,Z=0.0),OffsetRate=(X=0.0,Y=-2000.0,Z=0.0),OffsetTime=4,RotMag=(X=40.0,Y=0.0,Z=0.0),RotRate=(X=2000.0,Y=0.0,Z=0.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=0.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=3,RotMag=(X=0.0,Y=0.0,Z=60.0),RotRate=(X=0.0,Y=0.0,Z=4000.0),RotTime=6)

	EffectSockets(0)=MuzzleFlashSocket
	EffectSockets(1)=MuzzleFlashSocket

	BeamPreFireAnim(1)=WeaponAltFireStart
	BeamFireAnim(1)=WeaponAltFire
	BeamPostFireAnim(1)=WeaponAltFireEnd

	MaxLinkStrength=3

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Primary'
	MuzzleFlashAltPSCTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam'
	bMuzzleFlashPSCLoops=true
	MuzzleFlashLightClass=class'UTGame.UTLinkGunMuzzleFlashLight'

	TeamMuzzleFlashTemplates[0]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam_Red'
	TeamMuzzleFlashTemplates[1]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam_Blue'
	TeamMuzzleFlashTemplates[2]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam'
	HighPowerMuzzleFlashTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam_Gold'

	MuzzleFlashColor=(R=120,G=255,B=120,A=255)
	MuzzleFlashDuration=0.33;

	BeamTemplate[1]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam'
	BeamSockets[1]=MuzzleFlashSocket02
	EndPointParamName=LinkBeamEnd

	IconX=412
	IconY=82
	IconWidth=40
	IconHeight=36

	StartAltFireSound=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_AltFireStartCue'
	EndAltFireSound=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_AltFireStopCue'
	CrossHairCoordinates=(U=384,V=0,UL=64,VL=64)

	LockerOffset=(Z=15.0)
	IconCoordinates=(U=273,V=159,UL=84,VL=50)

	QuickPickGroup=3
	QuickPickWeight=0.8

}
