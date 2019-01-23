/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTMissionGRI extends UTGameReplicationInfo
	native;

/** The full mission list */
var array<UTSeqObj_SPMission> FullMissionList;

/** Holds the current Mission Index */
var repnotify int CurrentMissionIndex;

/** The index of the last mission */
var int LastMissionIndex;

/** Holds the result of the previous mission */
var ESinglePlayerMissionResult LastMissionResult;

var repnotify name RequestedMenuName;

/** Scene information */

var string SelectionSceneTemplate;
var transient UTUIScene_CMissionSelection SelectionScene;

var string BriefingSceneTemplate;
var transient UTUIScene_CMissionBriefing BriefingScene;

replication
{
	if (Role == ROLE_Authority)
		CurrentMissionIndex, LastMissionIndex, LastMissionResult, RequestedMenuName;
}

/**
 * Find all of the UTSeqObj_SPMission objects and fill out the list
 */
native function FillMissionList();

/**
 * Prime the Mission List
 */
simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	FillMissionlist();
}

/** Allow anyone to hook in and know that the mission has changed */
delegate OnMissionChanged(int NewMissionIndex, UTMissionGRI GRI);


simulated function ReplicatedEvent(name VarName)
{
	if ( VarName == 'CurrentMissionIndex' )
	{
		ChangeMission(CurrentMissionIndex);
	}

	else if ( VarName == 'RequestedMenuName' )
	{
		GotoState('HandleMenuRequest');
	}
	Super.ReplicatedEvent(VarName);
}

/**
 * Note, until the menu scene is activated, this will be unused
 */
simulated function ChangeMission(int NewMissionIndex)
{
	if ( SelectionScene != none )
	{
		CurrentMissionIndex = NewMissionIndex;
		OnMissionChanged(NewMissionIndex,self);
	}
}


function RequestMenu(Name MenuName)
{
	RequestedMenuName = MenuName;
	NetUpdateTime = WorldInfo.TimeSeconds - 1.0f;
	GotoState('HandleMenuRequest');
}

state HandleMenuRequest
{
	// Set the timer to check
	simulated function BeginState(name PrevStateName)
	{

		Super.BeginState(PrevStateName);
		if ( !AttemptMenu() )
		{
			SetTimer(0.1,true,'TryMenuAgain');
		}
		else
		{
			GotoState('');
		}
	}

	// Check replication again
	simulated function TryMenuAgain()
	{
		if ( AttemptMenu() )
		{
			ClearTimer('TryMenuAgain');
			GotoState('');
		}
	}

	// Attempt to open a given menu on the client.
	simulated function bool AttemptMenu()
	{
		local PlayerController PC;
		local LocalPlayer LP;
		local UTMissionSelectionPRI PRI;
		local UIInteraction UIController;
		local UIScene OutScene;
		local UIScene ResolvedSceneTemplate;

		if (WorldInfo.GRI == self && !WorldInfo.IsInSeamlessTravel())
		{
			ForEach LocalPlayerControllers(class'PlayerController', PC)
			{
				// Check all replication conditions
				LP = LocalPlayer(PC.Player);

				PRI = UTMissionSelectionPRI(PC.PlayerReplicationInfo);
				if (PRI != none && PRI.Owner == PC && LP != none)
				{
                	PRI.bIsHost = WorldInfo.NetMode == NM_ListenServer;
					UIController = LP.ViewportClient.UIController;
					if ( UIController != none )
					{
						if ( RequestedMenuName == 'SelectionMenu' )
						{
							ResolvedSceneTemplate = UIScene(DynamicLoadObject(SelectionSceneTemplate, class'UIScene'));
							if(ResolvedSceneTemplate != None)
							{
								UIController.OpenScene(ResolvedSceneTemplate,LP,OutScene);
								SelectionScene = UTUIScene_CMissionSelection(OutScene);
								SelectionScene.SetResult(LastMissionResult, PRI.bIsHost , LastMissionIndex);
								ChangeMission(CurrentMissionIndex);
							}
						}
						else if (RequestedMenuName == 'BriefingMenu' )
						{
							if ( SelectionScene != none )
							{

								SelectionScene.CloseScene(SelectionScene);
								SelectionScene = none;
							}

							if ( BriefingScene == none )
							{
								ResolvedSceneTemplate = UIScene(DynamicLoadObject(BriefingSceneTemplate, class'UIScene'));
								if(ResolvedSceneTemplate != None)
								{
									UIController.OpenScene(ResolvedSceneTemplate,LP,OutScene);
									BriefingScene = UTUIScene_CMissionBriefing(OutScene);
									BriefingScene.SetMission(CurrentMissionIndex);
								}
							}

							PRI.ServerReadyToPlay();
						}
					}
					return true;
				}
			}
		}
		return false;
	}
}


defaultproperties
{
	CurrentMissionIndex=-1

	SelectionSceneTemplate="UI_Scenes_Campaign.Scenes.CampMissionSelection"
	BriefingSceneTemplate="UI_Scenes_Campaign.Scenes.CampMissionBriefing"
}
