/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_QuickPickCell extends UTUI_HudWidget
	native(UI);

/** Subwidgets that make up this widget */
var instanced UIProgressBar AmmoBar;
var instanced UIImage 		WeaponIcon;
var instanced UIImage		Background;
var instanced UIImage 		Overlay;

var() int CellIndex;

var transient UTWeapon MyWeapon;

cpptext
{
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );
	void AssociatedWithWeapon(AUTWeapon* Weapon);
}

/**
 * Clean up
 */
function NotifyGameSessionEnded()
{
	MyWeapon = none;
}

/**
 * Enable the selection widget
 */
function Select()
{
	Overlay.ImageComponent.Fade(Overlay.ImageComponent.FadeAlpha, 1.0 , 0.05);
}

/**
 * Disable the selection Widget
 */
function UnSelect()
{
	Overlay.ImageComponent.Fade(Overlay.ImageComponent.FadeAlpha, 0.0 , 0.05);
}

event WidgetTick(FLOAT DeltaTime)
{
	local float Perc;
	Super.WidgetTick(DeltaTime);

	if ( MyWeapon != none && AmmoBar != none )
	{
		Perc = float(MyWeapon.AmmoCount) / float(MyWeapon.MaxAmmoCount);
		AmmoBar.SetValue( Perc, true);
	}
}



defaultproperties
{
	// Ammo Bar

	Begin Object Class=UIProgressBar Name=pAmmoBar
		WidgetTag=AmmoBar
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
	AmmoBar=pAmmoBar

	// Weapon Icon

	Begin Object Class=UIImage Name=iWeaponIcon
		WidgetTag=WeaponIcon
		Position={( Value[UIFACE_Left]=0.1,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0.8,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=1.8,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.15,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	WeaponIcon=iWeaponIcon

	// Background

	Begin Object Class=UIImage Name=iBackground
		WidgetTag=Background
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
	Background=iBackground

	// Overlay

	Begin Object Class=UIImage Name=iOverlay
		WidgetTag=Overlay
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
	Overlay=iOverlay


	bRequiresTick=true

	Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.6,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.2,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

	WidgetTag=QuickPickCell
	bManualVisibility=true
}
