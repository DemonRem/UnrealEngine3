/**
 * Movable version of SpotLight.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SpotLightMovable extends SpotLight
	native
	placeable;


cpptext
{
public:
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

}


defaultproperties
{
	// Visual things should be ticked in parallel with physics
	TickGroup=TG_DuringAsyncWork

	Begin Object Name=Sprite
		Sprite=Texture2D'EngineResources.LightIcons.Light_Spot_Moveable_DynamicsAndStatics'
	End Object

	// Light component.
	Begin Object Name=SpotLightComponent0
	    LightAffectsClassification=LAC_DYNAMIC_AND_STATIC_AFFECTING

	    CastShadows=TRUE
	    CastStaticShadows=TRUE
	    CastDynamicShadows=TRUE
	    bForceDynamicLight=FALSE
	    UseDirectLightMap=FALSE

	    LightingChannels=(BSP=TRUE,Static=TRUE,Dynamic=TRUE,bInitialized=TRUE)
	End Object


	bMovable=TRUE
	bStatic=FALSE
	bHardAttach=TRUE
}
