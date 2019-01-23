/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTOnslaughtNodeTeleporter extends NavigationPoint
	abstract placeable
	hidecategories(Collision,Display,Attachment);

/** base ambient effects */
var ParticleSystem NeutralEffectTemplate;
var array<ParticleSystem> TeamEffectTemplates;

/** component that plays ambient effects for the node teleporter's current state */
var ParticleSystemComponent AmbientEffect;
/** component that plays the render-to-texture portal effect */
var ParticleSystemComponent PortalEffect;

/** teamcolored templates for portal effect */
var array<ParticleSystem> TeamPortalEffectTemplates;

/** current team owner */
var repnotify byte TeamNum;

var StaticMeshComponent FloorMesh;
var MaterialInstanceConstant FloorMaterialInstance;
var LinearColor NeutralFloorColor;
var array<LinearColor> TeamFloorColors;

var soundcue ConstructedSound, ActiveSound;

var AudioComponent AmbientSoundComponent;

/** materials for the portal effect */
var MaterialInterface PortalMaterial;
var MaterialInstanceConstant PortalMaterialInstance;
/** the component that captures the portal scene */
var SceneCapturePortalComponent PortalCaptureComponent;

/** last destination this teleporter sent someone to. The portal is set to view through this actor. */
var repnotify Actor LastDestination;

replication
{
	if (bNetDirty)
		TeamNum, LastDestination;
}

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		UpdateTeamEffects();

		// set up the portal effect
		PortalMaterialInstance = new(self) class'MaterialInstanceConstant';
		PortalMaterialInstance.SetParent(PortalMaterial);
		PortalEffect.SetMaterialParameter('Portal', PortalMaterialInstance);
		if (LastDestination == None)
		{
			LastDestination = self;
		}
		PortalCaptureComponent.SetCaptureParameters(class'TextureRenderTarget2D'.static.Create(256, 256),, LastDestination);
		PortalMaterialInstance.SetTextureParameterValue('RenderToTextureMap', PortalCaptureComponent.TextureTarget);
	}
}

simulated function UpdateTeamEffects()
{
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		if ( FloorMaterialInstance == None )
		{
			FloorMaterialInstance = FloorMesh.CreateAndSetMaterialInstanceConstant(1);
		}
		FloorMaterialInstance.SetVectorParameterValue('Team_Color', (TeamNum < TeamFloorColors.length) ? TeamFloorColors[TeamNum] : NeutralFloorColor);
		AmbientEffect.SetTemplate((TeamNum < TeamEffectTemplates.length) ? TeamEffectTemplates[TeamNum] : NeutralEffectTemplate);
		AmbientEffect.ActivateSystem();

		if (TeamNum < TeamPortalEffectTemplates.length)
		{
			PortalEffect.SetTemplate(TeamPortalEffectTemplates[TeamNum]);
		}

		if (TeamNum != 255)
		{
			PlaySound(ConstructedSound);
			SetAmbientSound(ActiveSound);
			PortalEffect.SetActive(true);
			PortalEffect.SetHidden(false);
			PortalCaptureComponent.SetFrameRate(default.PortalCaptureComponent.FrameRate);
		}
		else
		{
			SetAmbientSound(None);
			PortalCaptureComponent.SetFrameRate(0.001);
			PortalEffect.SetActive(false);
			PortalEffect.SetHidden(true);
		}
	}
}

simulated function SetAmbientSound(SoundCue NewAmbientSound)
{
	// if the component is already playing this sound, don't restart it
	if (NewAmbientSound != AmbientSoundComponent.SoundCue)
	{
		AmbientSoundComponent.Stop();
		AmbientSoundComponent.SoundCue = NewAmbientSound;
		if (NewAmbientSound != None)
		{
			AmbientSoundComponent.Play();
		}
	}
}

simulated function byte GetTeamNum()
{
	return TeamNum;
}

function SetTeamNum(byte NewTeamNum)
{
	local UTOnslaughtGame Game;
	local UTTeamPlayerStart Start;

	TeamNum = NewTeamNum;
	// make sure we're turned on
	SetHidden(false);
	SetCollision(true);
	// update effects
	NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		UpdateTeamEffects();
	}

	// set a default destination when the teleporter changes hands
	Game = UTOnslaughtGame(WorldInfo.Game);
	if (Game != None && TeamNum < ArrayCount(Game.PowerCore) && Game.PowerCore[TeamNum].PlayerStarts.length > 0)
	{
		SetLastDestination(Game.PowerCore[TeamNum].PlayerStarts[0]);
	}
	else
	{
		foreach WorldInfo.AllNavigationPoints(class'UTTeamPlayerStart', Start)
		{
			if (Start.TeamNumber == TeamNum)
			{
				SetLastDestination(Start);
				break;
			}
		}
	}
}

/** called when this node teleporter is associated with a disabled node */
function TurnOff()
{
	SetHidden(true);
	SetCollision(false);
	AmbientEffect.DeactivateSystem();
	NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
}

simulated function SetLastDestination(Actor NewLastDestination)
{
	LastDestination = NewLastDestination;

	if (WorldInfo.NetMode != NM_DedicatedServer && PortalCaptureComponent != None)
	{
		PortalCaptureComponent.SetCaptureParameters(,, LastDestination);
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'TeamNum')
	{
		UpdateTeamEffects();
	}
	else if (VarName == 'LastDestination')
	{
		SetLastDestination(LastDestination);
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

simulated event Attach(Actor Other)
{
	local UTPawn P;

	P = UTPawn(Other);
	if ( (P != None) && P.IsHumanControlled() && P.IsLocallyControlled() )
	{
		SendUseMessage();
	}
}

/** sends 'Press use to node teleport' message to all local players standing on us */
simulated function SendUseMessage()
{
	local PlayerController PC;
	local bool bShouldRetrigger;
	local UTPlayerReplicationInfo PRI;

	foreach LocalPlayerControllers(class'PlayerController', PC)
	{
		if (LocalPlayer(PC.Player) != None && PC.Pawn != None && PC.Pawn.Base == self && WorldInfo.GRI.OnSameTeam(PC, self))
		{
			PRI = UTPlayerReplicationInfo(PC.PlayerReplicationInfo);
			if ( (PRI != None) && (PRI.GetFlag() != None) )
			{
				PC.ReceiveLocalizedMessage(MessageClass, 44);
			}
			else
			{
				PC.ReceiveLocalizedMessage(MessageClass, 22);
			}
			bShouldRetrigger = true;
		}
	}
	if ( bShouldRetrigger )
	{
		SetTimer(2.0, false, 'SendUseMessage');
	}
}

function bool UsedBy(Pawn User)
{
	local UTPlayerController PC;

	PC = UTPlayerController(User.Controller);
	if (PC != None && WorldInfo.GRI.OnSameTeam(PC, self))
	{
		PC.ShowHudMap();
		return true;
	}
	else
	{
		return false;
	}
}

defaultproperties
{

}
