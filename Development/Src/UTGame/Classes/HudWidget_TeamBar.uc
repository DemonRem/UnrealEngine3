/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_TeamBar extends UTUI_HudWidget
	native(UI);

var instanced UIImage Separator;

var transient HudWidget_TeamScore TeamWidgets[2];

var transient float TeamScale[2];

/** The scaling modifier will be applied to the widget that coorsponds to the player's team */
var() float TeamScaleModifier;

/** How fast does a given widget transform */
var() float TeamScaleTransitionRate;

/** This is the default width of a single TeamScore widget */
var() float WidgetDefaultWidth;

/** This is the default height of a single TeamScore widget */
var() float WidgetDefaultHeight;

var() transient float TestTeamIndex;

cpptext
{
	virtual INT InsertChild(class UUIObject* NewChild,INT InsertIndex=-1,UBOOL bRenameExisting=TRUE);
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );
	virtual void Tick_Widget(FLOAT DeltaTime);

	void InitializeChild(UHudWidget_TeamScore* Child);
}

enum EStatusIconState
{
	ESIS_Normal,
	ESIS_Elevated,
	ESIS_Critical,
};

/**
 * Used to set the state of the TeamStatusIcon.  See HudWidget_TeamScore for more info
 *
 * @Param	TeamIndex		The team who's status you wish to change
 * @Param	NewState		The new state to switch to
 */

function SetStatusIcon(int TeamIndex, EStatusIconState NewState)
{
	local int i;
	for (i=0;i<2;i++)
	{
		if ( TeamWidgets[i].Teamindex == TeamIndex )
		{
			TeamWidgets[i].SetStatusIcon(NewState);
		}
	}
}

/**
 * Used to set the rotation of the TeamScore's HomeBaseIndicator.  See HudWidget_TeamScore for more info
 *
 * @Param	TeamIndex		The team's who's indicator you wish to change
 * @Param	Angle			The new angle (in Unreal Units)
 */

function SetHomeBaseIndicator(int TeamIndex, int Angle)
{
	local int i;
	for (i=0;i<2;i++)
	{
		if ( TeamWidgets[i].Teamindex == TeamIndex )
		{
			TeamWidgets[i].SetHomeBaseIndicator(Angle);
		}
	}
}

/**
 * Used to set the actual Score for a given team
 *
 * @Param	TeamIndex		The team's who's indicator you wish to change
 * @Param	NewScore		The team's new score
 */
function SetScore(int TeamIndex, int NewScore)
{
	local int i;
	for (i=0;i<2;i++)
	{
		if ( TeamWidgets[i].Teamindex == TeamIndex )
		{
			TeamWidgets[i].SetScore(NewScore);
		}
	}
}

function SetPlayerTeamIndex(int NewTeamIndex)
{
	if ( TeamWidgets[0].TeamIndex != NewTeamIndex )
	{
		TeamWidgets[0].SetTeamIndex(NewTeamIndex);
		TeamWidgets[1].SetTeamIndex(Abs(1-NewTeamIndex));
	}
}

defaultproperties
{
	Begin Object Class=UIImage Name=iSeparator
		WidgetTag=Separator
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.25,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.25,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	Separator=iSeparator


	WidgetTag=TeamBarWidget

	bVisibleBeforeMatch=true
	bVisibleAfterMatch=true
	bRequiresTick=true


	Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=0.6,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.2,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

	TeamScaleModifier=1.5
	TeamScaleTransitionRate=3
}
