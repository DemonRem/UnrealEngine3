/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_WeaponGroup extends UTUI_HudWidget
	native(UI);

/** The WeaponGroup this widget is associated with */
var() byte AssociatedWeaponGroup;

/** The WeaponGroup of the pawn's weapon from the last frame.  This allows us to skip
    several parts of the logic if the weapon group hasn't changed */
var private transient byte LastWeaponGroup;

/** The PendingWeaponGroup of the pawn's weapon from the last frame.  This allows us to skip
    several parts of the logic if the weapon group hasn't changed */
var private transient byte LastPendingWeaponGroup;

/** Subwidgets that make up this widget */
var instanced UILabel 		GroupLabel;
var instanced UIProgressBar AmmoBar;
var instanced UIImage 		WeaponIcon;
var instanced UIImage		Background;
var instanced UIImage 		Overlay;
var instanced UIImage 		PendingOverlay;

/** This reference will be filled in when this group is added to a weapon bar. */
var transient HudWidget_WeaponBar WeaponBar;

/** Used for highlighting */
var transient name HighlightStyleNames[3];	// 0 = Right, 1 = Center. 2 = Left

enum EGroupStatus
{
	EGS_Uninitialized,
	EGS_NoWeapons,
	EGS_AmmoOnly,
	EGS_Weapons,

};

var EGroupStatus GroupStatus;

cpptext
{
	/**
	 * We manage the weapon groups natively for speed.
	 */
	virtual void Tick_Widget(FLOAT DeltaTime);

	/** UGLY - Overrride Initialize so that we can set the opacitys on the highlights until Ron get's back */

	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );
	void ChangeState(UBOOL bIsActive);
	void FadeIn(UUIImage* Image, FLOAT FadeRate);
	void FadeOut(UUIImage* Image, FLOAT FadeRate);
}


defaultproperties
{
	// Group Label

	Begin Object Class=UILabel Name=iGroupLabel
		WidgetTag=GroupLabel
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
		DefaultStates.Add(class'UIState_Active')
	End Object
	GroupLabel=iGroupLabel

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
		DefaultStates.Add(class'UIState_Active')
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
		DefaultStates.Add(class'UIState_Active')
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
		DefaultStates.Add(class'UIState_Active')
	End Object
	Overlay=iOverlay

	// Pending Overlay

	Begin Object Class=UIImage Name=iPendingOverlay
		WidgetTag=PendingOverlay
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
		DefaultStates.Add(class'UIState_Active')
	End Object
	PendingOverlay=iPendingOverlay

	bRequiresTick=true

	HighlightStyleNames(0)=WeaponGroupHighlightRight
	HighlightStyleNames(1)=WeaponGroupFullHighlight
	HighlightStyleNames(2)=WeaponGroupHighlightLeft

	// default to out of range to force the first update
	LastWeaponGroup=255

	Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.6,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.2,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

	WidgetTag=WeaponGroupWidget
	GroupStatus=EGS_Uninitialized

}

