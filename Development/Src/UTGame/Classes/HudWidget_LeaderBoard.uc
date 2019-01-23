/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_LeaderBoard extends UTUI_HudWidget;

var instanced UILabel Spread;
var instanced UILabel FirstPlace;
var instanced UILabel SecondPlace;
var instanced UILabel ThirdPlace;
var instanced UILabel InThePack;

var transient string PlaceMarks[4];

event WidgetTick(FLOAT DeltaTime)
{
	local UTPlayerReplicationInfo OwnerPRI;
	local UTHud OwnerHud;
	local int i,MySpread,MyPosition;
	local string MySpreadStr;
	local string Work;
	local GameReplicationInfo GRI;

	Super.WidgetTick(DeltaTime);

	GRI = UTHudSceneOwner.GetWorldInfo().GRI;

	OwnerHud = GetHudOwner();
	if ( UTHudSceneOwner != none && GRI != none )
	{
		OwnerPRI = UTHudSceneOwner.GetPRIOwner();
		if (OwnerHud != none && OwnerPRI != none )
		{
			// Calculate the spread/position and generate the string

			if ( GRI.PRIArray[0] == OwnerPRI )
			{
				MySpread = 0;
				if ( GRI.PRIArray.Length>1 )
				{
					MySpread = OwnerPRI.Score - GRI.PRIArray[1].Score;
				}
				MyPosition = 0;
			}
			else
			{
				MySpread = OwnerPRI.Score - GRI.PRIArray[0].Score;
				MyPosition = GRI.PRIArray.Find(OwnerPRI);
			}

			if (MySpread >0)
			{
				MySpreadStr = "+"$String(MySpread);
			}
			else
			{
				MySpreadStr = string(MySpread);
			}

			Work = string(MyPosition+1) $ PlaceMarks[min(MyPosition,3)] $ " / " $ MySpreadStr;
			Spread.SetValue( Work );

			for (i=0;i<3;i++)
			{
			 	if ( i < GRI.PRIArray.Length && GRI.PRIArray[i] != none )
			 	{
			 		Work = string(i+1) $ PlaceMarks[i] @ GRI.PRIArray[i].PlayerName;
			 	}
			 	else
			 	{
			 		Work = "";
			 	}

		 		switch (i)
		 		{
		 			case 0 : FirstPlace.SetValue(Work); break;
		 			case 1 : SecondPlace.SetValue(Work); break;
		 			case 2 : ThirdPlace.SetValue(Work); break;
		 		}
			}

		}
	}
}


defaultproperties
{
	bVisibleBeforeMatch=false
	bVisibleAfterMatch=false

	WidgetTag=LeaderboardWidget
	bRequiresTick=true

	Begin Object Class=UILabel Name=lblSpread
		DataSource=(MarkupString="2nd / +2",RequiredFieldType=DATATYPE_Property)
		WidgetTag=Spread

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
	Spread=lblSpread

	Begin Object Class=UILabel Name=lblFirstPlace
		DataSource=(MarkupString="1st Place",RequiredFieldType=DATATYPE_Property)
		WidgetTag=FirstPlace

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
	FirstPlace=lblFirstPlace

	Begin Object Class=UILabel Name=lblSecondPlace
		DataSource=(MarkupString="2nd Place",RequiredFieldType=DATATYPE_Property)
		WidgetTag=SecondPlace

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
	SecondPlace=lblSecondPlace

	Begin Object Class=UILabel Name=lblThirdPlace
		DataSource=(MarkupString="3rd Place",RequiredFieldType=DATATYPE_Property)
		WidgetTag=ThirdPlace

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
	ThirdPlace=lblThirdPlace

	Begin Object Class=UILabel Name=lblInThePack
		DataSource=(MarkupString="In The Pack",RequiredFieldType=DATATYPE_Property)
		WidgetTag=InThePack

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
		bHidden=true
	End Object
	InThePack=lblInThePack

	Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageScene,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageScene,
				Value[UIFACE_Right]=0.119140625,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageScene,
				Value[UIFACE_Bottom]=0.069010416,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageScene)}


	PlaceMarks[0]="st: ";
	PlaceMarks[1]="nd: ";
	PlaceMarks[2]="rd: ";
	PlaceMarks[3]="th: ";

}
