/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_CMissionBriefing extends UTUIScene_Campaign;

var transient UILabel MissionTitle;
var transient UILabel MissionDescription;
var transient UIImage MapImage;
var transient UIImage ObjBackgroundImage;

var transient UILabelButton ObjButtons[4];

var EMissionData Mission;
var bool bHasValidMission;

event PostInitialize()
{
	local int i;

	Super.PostInitialize();

	MissionTitle = UILabel(FindChild('MissionTitle',true));
	MissionDescription = UILabel(FindChild('MissionDescription',true));
	MapImage = UIImage(FindChild('MiniMap',true));
	ObjBackgroundImage = UIImage(FindChild('MissionBackground',true));
	ObjBackgroundImage.SetVisibility(false);

	for (i=0;i<4;i++)
	{
		ObjButtons[i] = UILabelButton(FindChild(name("MissionObj"$i),true));
		ObjButtons[i].SetVisibility(false);
	}
	bCloseOnLevelChange=true;

}

/**
 * Given a mission index, look up the mission
 *
 * @param	MissionIndex	The index of the mission to get
 */
function UTSeqObj_SPMission GetMission(int MissionIndex)
{
	local int i;
	local UTMissionGRI GRI;

	GRI = UTMissionGRI( GetWorldInfo().GRI );
	if ( GRI != none )
	{
		for (i=0;i<GRI.FullMissionList.Length;i++)
		{
			if (GRI.FullMissionList[i].MissionInfo.MissionIndex == MissionIndex)
			{
				return GRI.FullMissionList[i];
			}
		}
	}

	return None;
}


function SetMission(int MissionIndex)
{
	local UTSeqObj_SPMission MissionObj;
	local int i;

	MissionObj = GetMission(MissionIndex);

	if ( MissionObj != none )
	{
		Mission = MissionObj.MissionInfo;
		MissionTitle.SetDatastoreBinding(Mission.MissionTitle);
		MissionDescription.SetDatastoreBinding(Mission.MissionDescription);

		if ( Mission.MissionRules.MissionMapImage != none )
		{
			MapImage.ImageComponent.SetImage(Mission.MissionRules.MissionMapImage);
			if ( Mission.MissionRules.bCustomMapCoords )
			{
				MapImage.ImageComponent.SetCoordinates(Mission.MissionRules.MissionMapCoords);
			}
		}


		for (i=0;i<4;i++)
		{
			if ( Mission.MissionRules.Objectives[i].Text != "" )
			{
				ObjButtons[i].SetDataStoreBinding( Mission.MissionRules.Objectives[i].Text );
				ObjButtons[i].SetVisibility(true);
				ObjButtons[i].OnClicked = OnObjClicked;
			}
		}

	}

	ShowObjective(0);
}

function bool OnObjClicked(UIScreenObject EventObject, int PlayerIndex)
{
	local int i;
	if ( UILabelButton(EventObject) != none )
	{
		for (i=0;i<4;i++)
		{
			if (ObjButtons[i] == UILabelButton(EventObject) )
			{
				ShowObjective(i);
			}
		}
	}
	return true;
}

function ShowObjective(int ObjIndex)
{
	if ( ObjButtons[ObjIndex].IsVisible()  )
	{
		if ( Mission.MissionRules.Objectives[ObjIndex].BackgroundImg != none )
		{
			ObjBackgroundImage.ImageComponent.SetImage(Mission.MissionRules.Objectives[ObjIndex].BackgroundImg);
			ObjBackgroundImage.SetVisibility(true);
			return;
		}
	}

	ObjBackgroundImage.SetVisibility(false);

}


defaultproperties
{
}
