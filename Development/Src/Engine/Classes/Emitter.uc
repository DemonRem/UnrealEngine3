//=============================================================================
// Emitter actor class.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class Emitter extends Actor
	native(Particle)
	placeable;

var()	editconst const	ParticleSystemComponent ParticleSystemComponent;
var						bool					bDestroyOnSystemFinish;

/** used to update status of toggleable level placed emitters on clients */
var repnotify bool bCurrentlyActive;

replication
{
	if (bNoDelete)
		bCurrentlyActive;
}

cpptext
{
	void SetTemplate(UParticleSystem* NewTemplate, UBOOL bDestroyOnFinish=false);
	void AutoPopulateInstanceProperties();

	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
}

native noexport event SetTemplate(ParticleSystem NewTemplate, optional bool bDestroyOnFinish);

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	// Let them die quickly on a dedicated server
	if (WorldInfo.NetMode == NM_DedicatedServer)
	{
		LifeSpan = 0.2;
	}

	// Set Notification Delegate
	if (ParticleSystemComponent != None)
	{
		ParticleSystemComponent.OnSystemFinished = OnParticleSystemFinished;
		bCurrentlyActive = ParticleSystemComponent.bAutoActivate;
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'bCurrentlyActive')
	{
		if (bCurrentlyActive)
		{
			ParticleSystemComponent.ActivateSystem();
		}
		else
		{
			ParticleSystemComponent.DeactivateSystem();
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

function OnParticleSystemFinished(ParticleSystemComponent FinishedComponent)
{
	if (bDestroyOnSystemFinish)
	{
		Lifespan = 0.0001f;
	}
	bCurrentlyActive = false;
}

/**
 * Handling Toggle event from Kismet.
 */
simulated function OnToggle(SeqAct_Toggle action)
{
	// Turn ON
	if (action.InputLinks[0].bHasImpulse)
	{
		ParticleSystemComponent.ActivateSystem();
		bCurrentlyActive = TRUE;
	}
	// Turn OFF
	else if (action.InputLinks[1].bHasImpulse)
	{
		ParticleSystemComponent.DeactivateSystem();
		bCurrentlyActive = FALSE;
	}
	// Toggle
	else if (action.InputLinks[2].bHasImpulse)
	{
		// If spawning is suppressed or we aren't turned on at all, activate.
		if ((ParticleSystemComponent.bSuppressSpawning == TRUE) || (bCurrentlyActive == FALSE))
		{
			ParticleSystemComponent.ActivateSystem();
			bCurrentlyActive = TRUE;
		}
		else
		{
			ParticleSystemComponent.DeactivateSystem();
			bCurrentlyActive = FALSE;
		}
	}
	ParticleSystemComponent.LastRenderTime = WorldInfo.TimeSeconds;
	ForceNetRelevant();
}

simulated function SetFloatParameter(name ParameterName, float Param)
{
	if (ParticleSystemComponent != none)
		ParticleSystemComponent.SetFloatParameter(ParameterName, Param);
	else
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
}

simulated function SetVectorParameter(name ParameterName, vector Param)
{
	if (ParticleSystemComponent != none)
		ParticleSystemComponent.SetVectorParameter(ParameterName, Param);
	else
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
}

simulated function SetColorParameter(name ParameterName, color Param)
{
	if (ParticleSystemComponent != none)
		ParticleSystemComponent.SetColorParameter(ParameterName, Param);
	else
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
}

simulated function SetExtColorParameter(name ParameterName, byte Red, byte Green, byte Blue, byte Alpha)
{
	local color c;

	if (ParticleSystemComponent != none)
	{
		c.r = Red;
		c.g = Green;
		c.b = Blue;
		c.a = Alpha;
		ParticleSystemComponent.SetColorParameter(ParameterName, C);
	}
	else
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
}


simulated function SetActorParameter(name ParameterName, actor Param)
{
	if (ParticleSystemComponent != none)
		ParticleSystemComponent.SetActorParameter(ParameterName, Param);
	else
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
}

/**
 * Kismet handler for setting particle instance parameters.
 */
simulated function OnSetParticleSysParam(SeqAct_SetParticleSysParam Action)
{
	local int Idx, ParamIdx;
	if (ParticleSystemComponent != None &&
		Action.InstanceParameters.Length > 0)
	{
		for (Idx = 0; Idx < Action.InstanceParameters.Length; Idx++)
		{
			if (Action.InstanceParameters[Idx].ParamType != PSPT_None)
			{
				// look for an existing entry
				ParamIdx = ParticleSystemComponent.InstanceParameters.Find('Name',Action.InstanceParameters[Idx].Name);
				// create one if necessary
				if (ParamIdx == -1)
				{
					ParamIdx = ParticleSystemComponent.InstanceParameters.Length;
					ParticleSystemComponent.InstanceParameters.Length = ParamIdx + 1;
				}
				// update the instance parm
				ParticleSystemComponent.InstanceParameters[ParamIdx] = Action.InstanceParameters[Idx];
				if (Action.bOverrideScalar)
				{
					ParticleSystemComponent.InstanceParameters[ParamIdx].Scalar = Action.ScalarValue;
				}
			}
		}
	}
}


defaultproperties
{
	// Visual things should be ticked in parallel with physics
	TickGroup=TG_DuringAsyncWork

	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EngineResources.S_Emitter'
		HiddenGame=True
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
		bIsScreenSizeScaled=True
		ScreenSize=0.0025
	End Object
	Components.Add(Sprite)

	Begin Object Class=ParticleSystemComponent Name=ParticleSystemComponent0
		SecondsBeforeInactive=1
	End Object
	ParticleSystemComponent=ParticleSystemComponent0
	Components.Add(ParticleSystemComponent0)

	Begin Object Class=ArrowComponent Name=ArrowComponent0
		ArrowColor=(R=0,G=255,B=128)
		ArrowSize=1.5
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(ArrowComponent0)

	bEdShouldSnap=true
	bHardAttach=true
	bGameRelevant=true
	bNoDelete=true
}
