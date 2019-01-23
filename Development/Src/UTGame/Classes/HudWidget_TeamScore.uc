/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_TeamScore extends UTUI_HudWidget
	native(UI);

var instanced UILabel TeamScore;
var instanced UIImage TeamStatusIcon;
var instanced UIImage HomeBaseIndicator;

var() int TeamIndex;

var transient EStatusIconState CurrentState;
var transient int CurrentScore;

/**
 * Used to set the state of the TeamStatusIcon.  There are currently 3 managed states.  ESIS_Normal which means everything is ok so hide the icon,
 * ESIS_Elevated which switches the state to Enabled and ESIS_Critical which switches the state to Active.  The style that
 * gets displayed here should have the proper images for the state.
 *
 * @Param	NewState		The new state to switch to
 */
function SetStatusIcon(EStatusIconState NewState)
{
	if ( NewState != CurrentState )
	{
		switch (NewState)
		{
			case ESIS_Normal:
				TeamStatusIcon.DeactivateStateByClass(class'UIState_UTObjCritical',0);
				TeamStatusIcon.ImageComponent.ResetFade();
				TeamStatusIcon.SetVisibility(false);
				break;
			case ESIS_Elevated:
				TeamStatusIcon.SetVisibility(true);
				TeamStatusIcon.DeactivateStateByClass(class'UIState_UTObjCritical',0);
				TeamStatusIcon.ImageComponent.ResetFade();
				break;
			case ESIS_Critical:
				TeamStatusIcon.SetVisibility(true);
				TeamStatusIcon.ActivateStateByClass(class'UIState_UTObjCritical',0);
				TeamStatusIcon.ImageComponent.Pulse(1.0 ,0.25, 2.0);
				break;
		}
	}
	CurrentState = NewState;
}

/**
 * Used to set the rotation of the HomeBaseIndicator.
 *
 * @Param	Angle			The new angle (in Unreal Units)
 */

function SetHomeBaseIndicator( int Angle)
{
	HomeBaseIndicator.Rotation.Rotation.Yaw = Angle;
	HomeBaseIndicator.UpdateRotationMatrix();
}

/**
 * Used to set the score for this team.
 *
 * @Param	NewScore	The new score
 */

function SetScore( int NewScore)
{
	if (NewScore != CurrentScore )
	{
		TeamScore.SetValue( string(NewScore) );
	}
	CurrentScore = NewScore;
}

function SetTeamIndex(int NewIndex)
{
	local UIImage BkgImage;
	local LinearColor C;
	if ( NewIndex != TeamIndex )
	{
		TeamIndex = NewIndex;

		// Update the Colorization

		NewIndex = Clamp(TeamIndex,0,2);

		C = class'UIComp_DrawTeamColoredImage'.Default.TeamColors[NewIndex];

		HomeBaseIndicator.ImageComponent.SetColor( C );
		BkgImage = UIImage(FindChild('TeamBKG',true));
		if ( BkgImage != none )
		{
			BkgImage.ImageComponent.SetColor( C );
		}

		CurrentState = ESIS_Normal;
	}
}

defaultproperties
{
	Begin Object Class=UILabel Name=lblTeamScore
		DataSource=(MarkupString="000",RequiredFieldType=DATATYPE_Property)
		WidgetTag=TeamScore

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
	TeamScore=lblTeamScore

	Begin Object Class=UIImage Name=iTeamStatusIcon
		WidgetTag=TeamStatusIcon
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
		bHidden=true
		DefaultStates.Add(class'UIState_UTObjCritical')
	End Object
	TeamStatusIcon=iTeamStatusIcon

	Begin Object Class=UIImage Name=iHomeBaseIndicator
		WidgetTag=HomeBaseIndicator
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
	HomeBaseIndicator=iHomeBaseIndicator

	WidgetTag=TeamScoreWidget

	bVisibleBeforeMatch=true
	bVisibleAfterMatch=true
	bRequiresTick=true
}
