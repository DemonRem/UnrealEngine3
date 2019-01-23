class UTTeleporterBase extends Teleporter
	abstract;

/** the component that captures the portal scene */
var(SceneCapture) editconst SceneCapturePortalComponent PortalCaptureComponent;
/** resolution parameters */
var(SceneCapture) int TextureResolutionX, TextureResolutionY;
/** materials for the portal effect */
var MaterialInterface PortalMaterial;
var MaterialInstanceConstant PortalMaterialInstance;
/** material parameter that we assign the rendered texture to */
var name PortalTextureParameter;
/** Sound to be played when someone teleports in*/
var SoundCue TeleportingSound;
simulated event PostBeginPlay()
{
	local Teleporter Dest;

	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		// try to find a teleporter to view
		foreach WorldInfo.AllNavigationPoints(class'Teleporter', Dest)
		{
			if (string(Dest.Tag) ~= URL && Dest != Self)
			{
				break;
			}
		}
		InitializePortalEffect(Dest);
	}
}

simulated function InitializePortalEffect(Actor Dest)
{
	if (Dest != None)
	{
		// set up the portal effect
		PortalMaterialInstance = new(self) class'MaterialInstanceConstant';
		PortalMaterialInstance.SetParent(PortalMaterial);
		PortalCaptureComponent.SetCaptureParameters(class'TextureRenderTarget2D'.static.Create(TextureResolutionX, TextureResolutionY),, Dest);
		PortalMaterialInstance.SetTextureParameterValue(PortalTextureParameter, PortalCaptureComponent.TextureTarget);
	}
	else
	{
		DetachComponent(PortalCaptureComponent);
	}
}
simulated event bool Accept( actor Incoming, Actor Source )
{
	if(super.Accept(Incoming,Source))
	{
		PlaySound(TeleportingSound);
		return true;
	}
	return false;
}
defaultproperties
{
	Begin Object Class=SceneCapturePortalComponent Name=SceneCapturePortalComponent0
		FrameRate=15.0
		bSkipUpdateIfOwnerOccluded=true
		MaxUpdateDist=1500.0
		RelativeEntryRotation=(Yaw=32768)
	End Object
	PortalCaptureComponent=SceneCapturePortalComponent0
	Components.Add(SceneCapturePortalComponent0)

	Begin Object Name=CollisionCylinder
		CollisionRadius=50.0
		CollisionHeight=30.0
	End Object

	bStatic=false
	PortalTextureParameter=RenderToTextureMap
	TextureResolutionX=256
	TextureResolutionY=256

	Begin Object Class=AudioComponent Name=AmbientSound
		SoundCue=SoundCue'A_Gameplay.Portal.Portal_Loop01Cue'
		bAutoPlay=true
		bUseOwnerLocation=true
		bShouldRemainActiveIfDropped=true
	End Object
	Components.Add(AmbientSound)

	TeleportingSound=SoundCue'A_Gameplay.Portal.Portal_WalkThrough01Cue'
}
