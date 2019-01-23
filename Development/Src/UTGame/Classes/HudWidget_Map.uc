/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_Map extends UTUI_HudWidget
	native(UI);

/** Will be set to true if this map has been initialized */
var bool bMapInitialized;

/** Widgets */
var instanced UIImage OuterRing;
var instanced UIImage InnerRing;
var instanced UTDrawMapPanel UTMapPanel;

cpptext
{
	virtual void Tick_Widget(FLOAT DeltaTime);
}

defaultproperties
{
	Begin Object Class=UIImage Name=iOuterRing
		WidgetTag=OuterRing
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=1.0,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=1.0,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	OuterRing=iOuterRing

	Begin Object Class=UIImage Name=iInnerRing
		WidgetTag=InnerRing
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=1.0,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=1.0,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protect`ed
		PrivateFlags=PRIVATE_Protected
	End Object
	InnerRing=iInnerRing

	Begin Object Class=UTDrawMapPanel Name=iUTMapPanel
		WidgetTag=DrawMapPanel
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=1.0,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=1.0,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
//		PrivateFlags=PRIVATE_Protected
	End Object
	UTMapPanel=iUTMapPanel

	WidgetTag=MapWidget

	bVisibleBeforeMatch=false
	bVisibleAfterMatch=false
	bRequiresTick=true

	Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.6,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.2,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
}
