/**
 *	LensFlare source actor class.
 *	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class LensFlareSource extends Actor
	native(LensFlare)
	placeable;

var()	editconst const	LensFlareComponent		LensFlareComp;

cpptext
{
	void AutoPopulateInstanceProperties();

	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
}

//native noexport event SetTemplate(LensFlare NewTemplate);
native final function SetTemplate(LensFlare NewTemplate);

/**
 * Handling Toggle event from Kismet.
 */
simulated function OnToggle(SeqAct_Toggle action)
{
/***
	// Turn ON
	if (action.InputLinks[0].bHasImpulse)
	{
		LensFlareComponent.ActivateSystem();
		bCurrentlyActive = TRUE;
	}
	// Turn OFF
	else if (action.InputLinks[1].bHasImpulse)
	{
		LensFlareComponent.DeactivateSystem();
		bCurrentlyActive = FALSE;
	}
	// Toggle
	else if (action.InputLinks[2].bHasImpulse)
	{
		// If spawning is suppressed or we aren't turned on at all, activate.
		if (bCurrentlyActive == FALSE)
		{
			LensFlareComponent.ActivateSystem();
			bCurrentlyActive = TRUE;
		}
		else
		{
			LensFlareComponent.DeactivateSystem();
			bCurrentlyActive = FALSE;
		}
	}
	LensFlareComponent.LastRenderTime = WorldInfo.TimeSeconds;
	ForceNetRelevant();
***/
}

simulated function SetFloatParameter(name ParameterName, float Param)
{
/***
	if (LensFlareComponent != none)
	{
		LensFlareComponent.SetFloatParameter(ParameterName, Param);
	}
	else
	{
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
	}
***/
}

simulated function SetVectorParameter(name ParameterName, vector Param)
{
/***
	if (LensFlareComponent != none)
	{
		LensFlareComponent.SetVectorParameter(ParameterName, Param);
	}
	else
	{
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
	}
***/
}

simulated function SetColorParameter(name ParameterName, linearcolor Param)
{
/***
	if (LensFlareComponent != none)
	{
		LensFlareComponent.SetColorParameter(ParameterName, Param);
	}
	else
	{
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
	}
***/
}

simulated function SetExtColorParameter(name ParameterName, float Red, float Green, float Blue, float Alpha)
{
/***
	local linearcolor c;

	if (LensFlareComponent != none)
	{
		c.r = Red;
		c.g = Green;
		c.b = Blue;
		c.a = Alpha;
		LensFlareComponent.SetColorParameter(ParameterName, C);
	}
	else
	{
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
	}
***/
}


simulated function SetActorParameter(name ParameterName, actor Param)
{
/***
	if (LensFlareComponent != none)
	{
		LensFlareComponent.SetActorParameter(ParameterName, Param);
	}
	else
	{
		`log("Warning: Attempting to set a parameter on "$self$" when the PSC does not exist");
	}
***/
}

/**
 * Kismet handler for setting particle instance parameters.
 */
/*** 
simulated function OnSetLensFlareParam(SeqAct_SetLensFlareParam Action)
{
	local int Idx, ParamIdx;
	if ((LensFlareComponent != None) && (Action.InstanceParameters.Length > 0))
	{
		for (Idx = 0; Idx < Action.InstanceParameters.Length; Idx++)
		{
			if (Action.InstanceParameters[Idx].ParamType != PSPT_None)
			{
				// look for an existing entry
				ParamIdx = LensFlareComponent.InstanceParameters.Find('Name',Action.InstanceParameters[Idx].Name);
				// create one if necessary
				if (ParamIdx == -1)
				{
					ParamIdx = LensFlareComponent.InstanceParameters.Length;
					LensFlareComponent.InstanceParameters.Length = ParamIdx + 1;
				}
				// update the instance parm
				LensFlareComponent.InstanceParameters[ParamIdx] = Action.InstanceParameters[Idx];
				if (Action.bOverrideScalar)
				{
					LensFlareComponent.InstanceParameters[ParamIdx].Scalar = Action.ScalarValue;
				}
			}
		}
	}
}
***/
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

	// Inner cone visualization.
	Begin Object Class=DrawLightConeComponent Name=DrawInnerCone0
		ConeColor=(R=150,G=200,B=255)
	End Object
	Components.Add(DrawInnerCone0)

	// Outer cone visualization.
	Begin Object Class=DrawLightConeComponent Name=DrawOuterCone0
		ConeColor=(R=200,G=255,B=255)
	End Object
	Components.Add(DrawOuterCone0)

	// Light radius visualization.
	Begin Object Class=DrawLightRadiusComponent Name=DrawRadius0
	End Object
	Components.Add(DrawRadius0)

	Begin Object Class=LensFlareComponent Name=LensFlareComponent0
		PreviewInnerCone=DrawInnerCone0
		PreviewOuterCone=DrawOuterCone0
		PreviewRadius=DrawRadius0
	End Object
	LensFlareComp=LensFlareComponent0
	Components.Add(LensFlareComponent0)

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
