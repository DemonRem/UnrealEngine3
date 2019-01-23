/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_ScoreboardCell extends UTUI_HudWidget;

var instanced UIImage	StatusIcon;
var instanced UILabel   PlayerName;
var instanced UILabel	PlayerClan;
var instanced UILabel	PlayerScore;
var instanced UILabel	PlayerLocation;
var instanced UILabel	MacroLine;

function SetValues(string PName, int PScore, optional string PClan, optional string PLocation, optional string PMacro, optional bool bHasFlag=false, optional bool bHighlight)
{
	if ( IsHidden() )
	{
		SetVisibility(true);
	}

	StatusIcon.SetVisibility(bHasFlag);
	PlayerLocation.SetVisibility(true);


	if ( bHasFlag )
	{
		if ( StatusIcon.ImageComponent.FadeType == EFT_None )
		{
			StatusIcon.ImageComponent.Pulse(1.0,0.5,3.0);
		}
	}
	else if ( StatusIcon.ImageComponent.FadeType != EFT_None )
	{
		StatusIcon.ImageComponent.ResetFade();
	}

	PlayerName.SetValue(PName);
	PlayerClan.SetValue(PClan);
	PlayerScore.SetValue(string(PScore));
	PlayerLocation.SetValue(PLocation);
	MacroLine.SetValue(PMacro);

	if ( bHighlight )
	{
		PlayerName.StringRenderComponent.SetColor( MakeLinearColor(1.0,1.0,0.2,1.0) );
		PlayerScore.StringRenderComponent.SetColor( MakeLinearColor(1.0,1.0,0.2,1.0) );
		PlayerClan.StringRenderComponent.SetColor( MakeLinearColor(1.0,1.0,0.2,1.0) );
	}
	else
	{
		PlayerName.StringRenderComponent.DisableCustomColor();
		PlayerScore.StringRenderComponent.DisableCustomColor();
		PlayerClan.StringRenderComponent.DisableCustomColor();
	}

}

defaultproperties
{
	Begin Object Class=UIImage Name=iStatusIcon
		WidgetTag=StatusIcon
		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=.25,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=0.25,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	StatusIcon=iStatusIcon

	Begin Object Class=UILabel Name=lblPlayerName
		DataSource=(MarkupString="DrSiN",RequiredFieldType=DATATYPE_Property)
		WidgetTag=PlayerName

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
	PlayerName=lblPlayerName

	Begin Object Class=UILabel Name=lblPlayerClan
		DataSource=(MarkupString="[Epic]",RequiredFieldType=DATATYPE_Property)
		WidgetTag=PlayerClan

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
	PlayerClan=lblPlayerClan

	Begin Object Class=UILabel Name=lblPlayerScore
		DataSource=(MarkupString="234",RequiredFieldType=DATATYPE_Property)
		WidgetTag=PlayerScore

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
	PlayerScore=lblPlayerScore

	Begin Object Class=UILabel Name=lblPlayerLocation
		DataSource=(MarkupString="Near the Red Flag",RequiredFieldType=DATATYPE_Property)
		WidgetTag=PlayerLocation

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
	PlayerLocation=lblPlayerLocation

	Begin Object Class=UILabel Name=lblMacroLine
		DataSource=(MarkupString="Ping: 200ms   P/L: 0%",RequiredFieldType=DATATYPE_Property)
		WidgetTag=MacroLine

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
	MacroLine=lblMacroLine

	Position={( Value[UIFACE_Left]=0,
			ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
			Value[UIFACE_Top]=0,
			ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
			Value[UIFACE_Right]=1.0,
			ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
			Value[UIFACE_Bottom]=1.0,
			ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}


	WidgetTag=ScoreboardCell
	// You don't add this widget in the editor, it's automatically added
//	PrivateFlags=PRIVATE_Protected
}
