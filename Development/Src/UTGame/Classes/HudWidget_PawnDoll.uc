/**
 * This is the base class for all of the pawn doll widgets.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_PawnDoll extends UTUI_HudWidget
	abstract
	native(UI);

var instanced UIImage BaseImg;

defaultproperties
{
	Begin Object Class=UIImage Name=iBaseImg
		WidgetTag=BaseImg
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
	BaseImg=iBaseImg
	bVisibleBeforeMatch=false
	bVisibleAfterMatch=false
	WidgetTag=PawnDoll

}
