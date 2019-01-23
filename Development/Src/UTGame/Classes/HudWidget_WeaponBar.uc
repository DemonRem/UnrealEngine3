/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_WeaponBar extends UTUI_HudWidget
	native(UI);

/** This is the scaling modifier that will be applied to a group when
    it's not the active group. */
var() float InactiveScale;

/** How fast do a widget transition from selected to not selected */
var() float TransitionRate;

/** Holds the index in to the GroupWidgets of the currently active group */
var() transient int ActiveGroupIndex;

/** How many groups does this bar hold */
var() float NumberOfGroups;

struct native WeaponGroupData
{
	/** Reference to the actual group */
	var	HudWidget_WeaponGroup Group;

	/** Used to smoothly transition between groups */
	var float Scaler;

	/** Holds the amount of time left in the transition */
	var bool bOversized;

	/** How much to oversize */
	var float OverSizeScaler;

};

/** Holds a sorted list of widgets */
var transient array<WeaponGroupData> GroupWidgets;

/** This is the group template that will be used to create the various group widgets.  They are auto-generated in Initialize */
var() UIPrefab GroupTemplate;

var() float OpacityList[4];		// Holds the opacity value for each group state
var() float ActiveGroupOpacity;	// Holds the override opacity for the active group

cpptext
{
	/**
	 * We manage the weapon groups natively for speed.
	 */

	virtual INT InsertChild(class UUIObject* NewChild,INT InsertIndex=-1,UBOOL bRenameExisting=TRUE);
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );
	virtual void Tick_Widget(FLOAT DeltaTime);
	virtual void Render_Widget(FCanvas* Canvas);

	void InitializeGroup(UHudWidget_WeaponGroup* ChildGrp);
	void SetGroupAsActive(UHudWidget_WeaponGroup* ActiveGroup);

}

defaultproperties
{
	InactiveScale=0.75


	WidgetTag=WeaponBarWidget

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

	TransitionRate=4
	NumberOfGroups=10

	OpacityList(0)=1.0
	OpacityList(1)=0.15
	OpacityList(2)=0.3
	OpacityList(3)=0.5
	ActiveGroupOpacity=1.0


}
