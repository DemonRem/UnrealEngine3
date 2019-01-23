/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTGameReplicationInfo extends GameReplicationInfo
	config(game)
	native;

var float WeaponBerserk;
var int MinNetPlayers;
var int BotDifficulty;		// for bPlayersVsBots

var bool		bWarmupRound;	// Amount of Warmup Time Remaining
/** whether we have processed all the custom characters for players that were initially in the game (clientside flag) */
var bool bProcessedInitialCharacters;

/** Array of local players that have not been processed yet. */
var array<PlayerController> LocalPCsLeftToProcess;

/** Total number of local players set to be processed so far. */
var int	TotalPlayersSetToProcess;

struct native CreateCharStatus
{
	var		CustomCharMergeState	MergeState;
	var		UTCharFamilyAssetStore	AssetStore;
	var		UTCharFamilyAssetStore	ArmAssetStore;
	var		UTPlayerReplicationInfo	PRI;
	var		float					StartMergeTime;
	var		bool					bNeedsArms;
};

var array<CreateCharStatus>		CharStatus;

enum EFlagState
{
    FLAG_Home,
    FLAG_HeldFriendly,
    FLAG_HeldEnemy,
    FLAG_Down,
};

var EFlagState FlagState[2];

var UTUIScene MapMenuTemplate;

/** If this is set, the game is running in story mode */
var bool bStoryMode;
/** whether the server is a console so we need to make adjustments to sync up */
var bool bConsoleServer;

var repnotify bool bShowMOTD;

/** Holds a list of potential game objectives available in the game.  Objects are responsible for adding or removing themselves */
var array<actor> GameObjectives;

var databinding string MutatorList;

//********** Map Voting **********8/

var int MapVoteTimeRemaining;

/** weapon overlays that are available in this map - figured out in PostBeginPlay() from UTPowerupPickupFactories in the level
 * each entry in the array represents a bit in UTPawn's WeaponOverlayFlags property
 * @see UTWeapon::SetWeaponOverlayFlags() for how this is used
 */
var array<MaterialInterface> WeaponOverlays;

replication
{
	if (bNetInitial)
		WeaponBerserk, MinNetPlayers, BotDifficulty, bStoryMode, bConsoleServer, bShowMOTD, MutatorList;

	if (bNetDirty)
		bWarmupRound, FlagState, MapVoteTimeRemaining;
}

simulated function PostBeginPlay()
{
	local UTGameObjective Node;
	local UTTeleportBeacon Beacon;
	local UTVehicle_Leviathan Levi;
	local UTPowerupPickupFactory Powerup;

	Super.PostBeginPlay();

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		SetNoStreamWorldTextureForFrames(60);
		SetTimer(1.0, false, 'StartProcessingCharacterData');
	}


	ForEach WorldInfo.AllNavigationPoints(class'UTGameObjective', Node)
	{
		AddGameObjective(Node);
	}

	foreach DynamicActors(class'UTTeleportBeacon',Beacon)
	{
		AddGameObjective(Beacon);
	}

	foreach DynamicActors(class'UTVehicle_Leviathan',Levi)
	{
		AddGameObjective(Levi);
	}

	// using DynamicActors here so the overlays don't break if the LD didn't build paths
	foreach DynamicActors(class'UTPowerupPickupFactory', Powerup)
	{
		Powerup.AddWeaponOverlay(self);
	}
}

simulated function ReplicatedEvent(name VarName)
{
	if ( VarName == 'bShowMOTD' )
	{
		DisplayMOTD();
	}
}

simulated function SetFlagHome(int TeamIndex)
{
	FlagState[TeamIndex] = FLAG_Home;
	NetUpdateTime = WorldInfo.TimeSeconds - 1;
}

simulated function bool FlagIsHome(int TeamIndex)
{
	return ( FlagState[TeamIndex] == FLAG_Home );
}

simulated function bool FlagsAreHome()
{
	return ( FlagState[0] == FLAG_Home && FlagState[1] == FLAG_Home );
}

function SetFlagHeldFriendly(int TeamIndex)
{
	FlagState[TeamIndex] = FLAG_HeldFriendly;
}

simulated function bool FlagIsHeldFriendly(int TeamIndex)
{
	return ( FlagState[TeamIndex] == FLAG_HeldFriendly );
}

function SetFlagHeldEnemy(int TeamIndex)
{
	FlagState[TeamIndex] = FLAG_HeldEnemy;
}

simulated function bool FlagIsHeldEnemy(int TeamIndex)
{
	return ( FlagState[TeamIndex] == FLAG_HeldEnemy );
}

function SetFlagDown(int TeamIndex)
{
	FlagState[TeamIndex] = FLAG_Down;
}

simulated function bool FlagIsDown(int TeamIndex)
{
	return ( FlagState[TeamIndex] == FLAG_Down );
}

simulated function Timer()
{
	local byte TimerMessageIndex;
	local PlayerController PC;

	super.Timer();

	if ( WorldInfo.NetMode == NM_Client )
	{
		if ( bWarmupRound && RemainingTime > 0 )
			RemainingTime--;

	}


    if (WorldInfo.NetMode != NM_DedicatedServer && MapVoteTimeRemaining > 0)
    {
    	MapVoteTimeRemaining--;
    	if ( MapVoteTimeRemaining <= 10 && MapVoteTimeRemaining > 0)
    	{
			foreach LocalPlayerControllers(class'PlayerController', PC)
			{
				PC.ReceiveLocalizedMessage(class'UTTimerMessage', MapVoteTimeRemaining);
			}
		}

    }

	// check if we should broadcast a time countdown message
	if (WorldInfo.NetMode != NM_DedicatedServer && (bMatchHasBegun || bWarmupRound) && Winner == None)
	{
		switch (RemainingTime)
		{
			case 300:
				TimerMessageIndex = 16;
				break;
			case 180:
				TimerMessageIndex = 15;
				break;
			case 120:
				TimerMessageIndex = 14;
				break;
			case 60:
				TimerMessageIndex = 13;
				break;
			case 30:
				TimerMessageIndex = 12;
				break;
			case 20:
				TimerMessageIndex = 11;
				break;
			default:
				if (RemainingTime <= 10 && RemainingTime > 0)
				{
					TimerMessageIndex = RemainingTime;
				}
				break;
		}
		if (TimerMessageIndex != 0)
		{
			foreach LocalPlayerControllers(class'PlayerController', PC)
			{
				PC.ReceiveLocalizedMessage(class'UTTimerMessage', TimerMessageIndex);
			}
		}
	}


}

/** @return whether we're still processing character data into meshes */
simulated function bool IsProcessingCharacterData()
{
	// Still characters to process..
	return(CharStatus.length > 0);
}

/** called to notify all local players when character data processing is started/stopped */
simulated function SendCharacterProcessingNotification(bool bNowProcessing)
{
	local PlayerController PC;
	local UTPlayerController UTPC;
	local UTPlayerReplicationInfo PRI;

	if (!bNowProcessing && !bProcessedInitialCharacters)
	{
		// make sure local players got arms - if not, don't allow the processing to end yet
		foreach LocalPlayerControllers(class'PlayerController', PC)
		{
			PRI = UTPlayerReplicationInfo(PC.PlayerReplicationInfo);
			if (PRI != None && PRI.FirstPersonArmMesh == None)
			{
				ProcessCharacterData(PRI);
				bNowProcessing = true;
			}
		}
	}

	foreach LocalPlayerControllers(class'PlayerController', PC)
	{
		UTPC = UTPlayerController(PC);
		if (UTPC != None)
		{
			if (bNowProcessing)
			{
				if (!bProcessedInitialCharacters)
				{
					// open menu so player knows what's going on
					UTPC.SetPawnConstructionScene(true);
				}
			}
			else
			{
				UTPC.CharacterProcessingComplete();
			}
		}
	}

	if (!bNowProcessing)
	{
		bProcessedInitialCharacters = true;
		WorldInfo.bHighPriorityLoadingLocal = false;
	}
}

simulated function StartProcessingCharacterData()
{
	local int i;
	local UTPlayerReplicationInfo PRI;
	local PlayerController PC;

	// Count how many local players there are.
	foreach LocalPlayerControllers(class'PlayerController', PC)
	{
		LocalPCsLeftToProcess.AddItem(PC);
	}

	// process all character data that has already been received
	for (i = 0; i < PRIArray.length; i++)
	{
		PRI = UTPlayerReplicationInfo(PRIArray[i]);
		if (PRI != None && PRI.CharacterData.FamilyID != "" && PRI.CharacterMesh == None)
		{
			ProcessCharacterData(PRI);
		}
	}

	SendCharacterProcessingNotification(IsProcessingCharacterData());

}

/** Reset merging on a character. */
simulated function ResetCharMerge(int StatusIndex)
{
	local CustomCharMergeState TempState;

	// Can't pass member of dynamic arrays by reference in UScript :(
	TempState = CharStatus[StatusIndex].MergeState;
	class'UTCustomChar_Data'.static.ResetCustomCharMerge(TempState);
	CharStatus[StatusIndex].MergeState = TempState;

	CharStatus[StatusIndex].AssetStore = None;
	CharStatus[StatusIndex].ArmAssetStore = None;

	CharStatus[StatusIndex].bNeedsArms = FALSE;
}

/** Attempt to finish merging - returns SkeletalMesh when done. */
simulated function SkeletalMesh FinishCharMerge(int StatusIndex)
{
	local CustomCharMergeState TempState;
	local SkeletalMesh NewMesh;

	// Can't pass member of dynamic arrays by reference in UScript :(
	TempState = CharStatus[StatusIndex].MergeState;
	NewMesh = class'UTCustomChar_Data'.static.FinishCustomCharMerge(TempState);
	CharStatus[StatusIndex].MergeState = TempState;

	return NewMesh;
}

/** determines whether we should process the given player's character data into a mesh
 * and if so, gets that started
 * @note: this can be called multiple times for a given PRI if more info is received (e.g. Team)
 * @param PRI - the PlayerReplicationInfo that holds the data to process
 */
simulated function ProcessCharacterData(UTPlayerReplicationInfo PRI)
{
	local int i;
	local bool bPRIAlreadyPresent;
	local CreateCharStatus NewCharStatus;

	if( WorldInfo.IsPlayInEditor() )
	{
		return;
	}

	// Do nothing if CharData is not filled in
	if((PRI.CharacterData.FamilyID == "" || PRI.CharacterData.FamilyID == "NONE") && !PRI.IsLocalPlayerPRI())
	{
		return;
	}

	// We don't allow non-local characters to be created once gameplay has begun.
	// also skip nonlocal spectators (spectators can join later, so build local mesh anyway so at least player sees his/her own custom mesh)
	if( (bProcessedInitialCharacters || PRI.bOnlySpectator) && !PRI.IsLocalPlayerPRI() )
	{
		return;
	}

	// Decrement count if this is a local player.
	if(PRI.IsLocalPlayerPRI())
	{
		LocalPCsLeftToProcess.RemoveItem(PlayerController(PRI.Owner));
	}
	// If this isn't a local player, don't process it if we haven't got space.
	else if((TotalPlayersSetToProcess + LocalPCsLeftToProcess.length) >= class'UTCustomChar_Data'.default.MaxCustomChars)
	{
		PRI.SetCharacterMesh(None);
		return;
	}

	// clear timer if it was active
	ClearTimer('StartProcessingCharacterData');
	if (!bProcessedInitialCharacters)
	{
		// make sure local players have been informed if we're running the initial processing
		SendCharacterProcessingNotification(true);
	}

	// remove any previous mesh and arms
	PRI.SetCharacterMesh(None);
	PRI.SetFirstPersonArmInfo(None, None);

	// First see if this PRI is already present (eg may have changed team)
	bPRIAlreadyPresent = false;
	for(i=0; i<CharStatus.Length; i++)
	{
		// It is there - reset and abandon any merge work so far.
		if(CharStatus[i].PRI == PRI)
		{
			//`log("PRI in use - resetting:"@PRI);
			ResetCharMerge(i);
			bPRIAlreadyPresent = true;
		}
	}

	// Was not there - add to end of the array
	if(!bPRIAlreadyPresent)
	{
		//`log("Adding PRI:"@PRI);
		NewCharStatus.PRI = PRI;
		CharStatus[CharStatus.length] = NewCharStatus;

		// Increment count of total players set to process
		TotalPlayersSetToProcess++;
	}

	SetTimer(0.05, true, 'TickCharacterMeshCreation');
}

/** Function that disables streaming of world textures for NumFrames. */
native function SetNoStreamWorldTextureForFrames(int NumFrames);

/** called when character meshes are being processed to update it */
simulated function TickCharacterMeshCreation()
{
	local int i, NextCharIndex;
	local UTCharFamilyAssetStore ActiveAssetStore;
	local bool bMergePending;
	local SkeletalMesh NewMesh, ArmMesh;
	local string TeamString, LoadFamily, ArmMeshName, ArmMaterialName;
	local CustomCharTextureRes TexRes;
	local class<UTFamilyInfo> FamilyInfoClass;
	local MaterialInterface ArmMaterial;

	if (WorldInfo.IsInSeamlessTravel())
	{
		// don't create meshes while travelling
		return;
	}

	// To speed up streaming parts - disable level streaming.
	SetNoStreamWorldTextureForFrames(10);

	// First, clear out and reset any entries with a NULL PRI (that is, someone disconnected)
	for(i=CharStatus.Length-1; i>=0; i--)
	{
		if (CharStatus[i].PRI == None || CharStatus[i].PRI.bDeleteMe)
		{
			//`log("PRI NULL - Removing.");
			ResetCharMerge(i);
			CharStatus.Remove(i,1);
		}
	}

	// Invariant: At this point all PRI's are valid

	// Now look to see if one is at phase 2 (that is, streaming textures).
	for(i=CharStatus.Length-1; i>=0; i--)
	{
		if(CharStatus[i].MergeState.bMergeInProgress)
		{
			// Should never be trying to merge without an AssetStore, or with AssetStore not finished loading.
			assert(CharStatus[i].AssetStore != None);
			assert(CharStatus[i].AssetStore.NumPendingPackages == 0);
			// Should only ever have one in this state at a time!
			assert(bMergePending == false);
			// Set flag to indicate there is currently a merge pending
			bMergePending = true;

			//`log("PRI Merge Pending:"@CharStatus[i].PRI);

			// See if we can create skeletal mesh
			NewMesh = FinishCharMerge(i);
			if(!CharStatus[i].MergeState.bMergeInProgress)
			{
				// Merge is done
				`log("CONSTRUCTIONING: Merge Complete:"@CharStatus[i].PRI@"  (Tex stream:"@(WorldInfo.RealTimeSeconds - CharStatus[i].StartMergeTime)@"Secs)");

				if(NewMesh != None)
				{
					// Save newly created mesh into PRI.
					CharStatus[i].PRI.SetCharacterMesh(NewMesh);

					CharStatus[i].PRI.CharPortrait = class'UTCustomChar_Data'.static.MakeCharPortraitTexture(NewMesh, class'UTCustomChar_Data'.default.PortraitSetup);
				}
				else
				{
					CharStatus[i].PRI.SetCharacterMesh(None);
				}

				// If this is a local player, we have one extra step which is to load and store a pointer to the first-person arm mesh.
				if(CharStatus[i].PRI.IsLocalPlayerPRI())
				{
					CharStatus[i].bNeedsArms = TRUE;
					CharStatus[i].AssetStore = None;
				}
				else
				{
					// We are done! Remove this entry from the array. This will let the AssetStore go away next GC.
					CharStatus.Remove(i,1);
				}

				// No merges pending any more
				bMergePending = false;
			}
			// See if we have taken too long waiting for textures to stream in.
			else if((WorldInfo.RealTimeSeconds - CharStatus[i].StartMergeTime) > class'UTCustomChar_Data'.default.CustomCharTextureStreamTimeout)
			{
				`log("TIMEOUT: Streaming Textures for custom char."@CharStatus[i].PRI);
				ResetCharMerge(i);
				CharStatus[i].PRI.SetCharacterMesh(None);

				// Even though we timed out while streaming, we still try to load the FP arm mesh.
				if(CharStatus[i].PRI.IsLocalPlayerPRI())
				{
					CharStatus[i].bNeedsArms = TRUE;
				}
				else
				{
					// We are done! Remove this entry from the array. This will let the AssetStore go away next GC.
					CharStatus.Remove(i,1);
				}

				bMergePending = false;
			}
		}
	}

	// If no merge going, but still PRIs left to process, get the next one going
	if(!bMergePending && CharStatus.length > 0)
	{
		// Look to see if any are at phase 1 (that is, loading assets from disk - has an AssetStore)
		for(i=0; i<CharStatus.Length && !bMergePending; i++)
		{
			// This entry has an asset store
			if(CharStatus[i].AssetStore != None)
			{
				// Make sure it matches the PRI's character setup data
				assert(CharStatus[i].AssetStore.FamilyID == CharStatus[i].PRI.CharacterData.FamilyID);

				// If this is the first entry with a store, remember it
				if(ActiveAssetStore == None)
				{
					ActiveAssetStore = CharStatus[i].AssetStore;
				}
				// If not the first - make sure all AssetStores are the same. Only want one 'in flight' at a time.
				else
				{
					assert(CharStatus[i].AssetStore == ActiveAssetStore);
				}

				// If all assets are loaded, we can start a merge.
				if(CharStatus[i].AssetStore.NumPendingPackages == 0)
				{
					`log("PRI Merge Start:"@CharStatus[i].PRI);

					TeamString = CharStatus[i].PRI.GetCustomCharTeamString();

					// Choose texture res based on whether you are the local player
					if(CharStatus[i].PRI.IsLocalPlayerPRI())
					{
						TexRes = CCTR_Self;
					}
					else
					{
						TexRes = CCTR_Normal;
					}

					CharStatus[i].MergeState = class'UTCustomChar_Data'.static.StartCustomCharMerge(CharStatus[i].PRI.CharacterData, TeamString, "SK1", None, TexRes);
					CharStatus[i].StartMergeTime = WorldInfo.RealTimeSeconds;
					bMergePending = true;
				}
			}
			// If its an arm-loading case
			else if(CharStatus[i].ArmAssetStore != None)
			{
				assert(CharStatus[i].ArmAssetStore.FamilyID == CharStatus[i].PRI.CharacterData.FamilyID);

				ActiveAssetStore = CharStatus[i].ArmAssetStore;

				// If we've finished loading packages containing arms
				if(CharStatus[i].ArmAssetStore.NumPendingPackages == 0)
				{
					LoadFamily = CharStatus[i].PRI.CharacterData.FamilyID;
					// If we just had bogus family, use fallback arms.
					if((LoadFamily == "" || LoadFamily == "NONE"))
					{
						ArmMeshName = class'UTCustomChar_Data'.default.DefaultArmMeshName;
						if (CharStatus[i].PRI.Team != None)
						{
							if (CharStatus[i].PRI.Team.TeamIndex == 0)
							{
								ArmMaterialName = class'UTCustomChar_Data'.default.DefaultRedArmSkinName;
							}
							else if (CharStatus[i].PRI.Team.TeamIndex == 1)
							{
								ArmMaterialName = class'UTCustomChar_Data'.default.DefaultBlueArmSkinName;
							}
						}
					}
					// We have decent family, look in info class
					else
					{
						FamilyInfoClass = class'UTCustomChar_Data'.static.FindFamilyInfo(CharStatus[i].PRI.CharacterData.FamilyID);
						ArmMeshName = FamilyInfoClass.default.ArmMeshName;
						if (CharStatus[i].PRI.Team != None)
						{
							if (CharStatus[i].PRI.Team.TeamIndex == 0)
							{
								ArmMaterialName = FamilyInfoClass.default.RedArmSkinName;
							}
							else if (CharStatus[i].PRI.Team.TeamIndex == 1)
							{
								ArmMaterialName = FamilyInfoClass.default.BlueArmSkinName;
							}
						}
					}

					// Find arm material by name (if we want one)
					if(ArmMaterialName != "")
					{
						ArmMaterial = MaterialInterface(FindObject(ArmMaterialName, class'MaterialInterface'));
						if(ArmMaterial == None)
						{
							`log("WARNING: Could not find ArmMaterial:"@ArmMaterialName);
						}
					}

					// Find arm mesh by name
					ArmMesh = SkeletalMesh(FindObject(ArmMeshName, class'SkeletalMesh'));
					if(ArmMesh == None)
					{
						`log("WARNING: Could not find ArmMesh:"@ArmMeshName);
					}

					// Apply mesh/material to character
					CharStatus[i].PRI.SetFirstPersonArmInfo(ArmMesh, ArmMaterial);

					// Done with this char now! Remove from array.
					CharStatus.Remove(i,1);
					i--;
				}
			}
		}

		// If none have an asset store - create one now and start getting assets loaded
		if(ActiveAssetStore == None)
		{
			// Look for the next character that isn't needing arms.
			// We also set bNeedsArms on any chars that just need arms.
			NextCharIndex = INDEX_NONE;
			for(i=0; i<CharStatus.length; i++)
			{
				if(!CharStatus[i].bNeedsArms)
				{
					// Any characters with bogus FamilyInfo, we just want to load default arms.
					// This should ONLY be for local PRIs (see ).
					LoadFamily = CharStatus[i].PRI.CharacterData.FamilyID;
					if(LoadFamily == "" || LoadFamily == "NONE")
					{
						assert(CharStatus[i].PRI.IsLocalPlayerPRI());
						CharStatus[i].bNeedsArms = TRUE;
					}
					else
					{
						NextCharIndex = i;
						break;
					}
				}
			}

			// We found a non-arms mesh (needs store for parts)
			if(NextCharIndex != INDEX_NONE)
			{
				LoadFamily = CharStatus[NextCharIndex].PRI.CharacterData.FamilyID;
				`log("CustomChar - Load Assets:"@LoadFamily);

				// During initial character creation, block on loading character packages
				if(!bProcessedInitialCharacters)
				{
					WorldInfo.bHighPriorityLoadingLocal = true;
				}

				CharStatus[NextCharIndex].AssetStore = class'UTCustomChar_Data'.static.LoadFamilyAssets(LoadFamily, FALSE, FALSE);

				// Look for others using the same family, and assign same asset
				for(i=0; i<CharStatus.length; i++)
				{
					if((i != NextCharIndex) && CharStatus[i].PRI.CharacterData.FamilyID == LoadFamily)
					{
						CharStatus[i].AssetStore = CharStatus[NextCharIndex].AssetStore;
					}
				}

			}
			// Arms loading case
			else
			{
				// Invariant : The only thing left in CharStatus now is PRIs in need of arms.

				LoadFamily = CharStatus[0].PRI.CharacterData.FamilyID;
				`log("CustomChar - Load Arms:"@LoadFamily);

				if(!bProcessedInitialCharacters)
				{
					WorldInfo.bHighPriorityLoadingLocal = true;
				}

				CharStatus[0].ArmAssetStore = class'UTCustomChar_Data'.static.LoadFamilyAssets(LoadFamily, FALSE, TRUE);

				// Look for others using the same family, and assign same asset
				for(i=1; i<CharStatus.length; i++)
				{
					if(CharStatus[i].PRI.CharacterData.FamilyID == LoadFamily)
					{
						CharStatus[i].ArmAssetStore = CharStatus[NextCharIndex].ArmAssetStore;
					}
				}
			}
		}
	}

	// if we're done, clear the timer and tell local PCs
	if (!IsProcessingCharacterData())
	{
		ClearTimer('TickCharacterMeshCreation');
		SendCharacterProcessingNotification(false);
	}
}

/**
 * Add a game objective to the global list
 *
 * @Param NewObjective		The Objective To Add
 */
function AddGameObjective(actor NewObjective)
{
	if ( GameObjectives.Find(NewObjective) == INDEX_NONE )
	{
		GameObjectives[GameObjectives.Length] = NewObjective;
	}
}

/**
 * Remove a game objective from the global list
 *
 * @Param ObjToRemove	The Objective To Remove
 */
function RemoveGameObjective(actor ObjToRemove)
{
	local int i;
	i = GameObjectives.Find(ObjToRemove);
	if (i>=0)
	{
		GameObjectives.Remove(i,1);
	}
}

/**
 * Displays the message of the day by finding a hud and passing off the call.
 */
simulated function DisplayMOTD()
{
	local PlayerController PC;
	local UTPlayerController UTPC;

	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		UTPC = UTPlayerController(PC);
		if ( UTPC != none )
		{
			UTHud(UTPC.MyHud).DisplayMOTD();
		}

		break;
	}
}

simulated function PopulateMidGameMenu(UTSimpleMenu Menu)
{
	if ( GameClass.Default.bTeamGame )
	{
		Menu.AddItem("<Strings:UTGameUI.MidGameMenu.ChangeTeam>",0);
	}

	Menu.AddItem("<Strings:UTGameUI.MidGameMenu.Settings>",1);

	if ( WorldInfo.NetMode != NM_StandAlone )
	{
		Menu.AddItem("<Strings:UTGameUI.MidGameMenu.Reconnect>",2);
	}

	Menu.AddItem("<Strings:UTGameUI.MidGameMenu.LeaveGame>",3);
}

simulated function bool MidMenuMenu(UTPlayerController UTPC, UTSimpleList List, int Index)
{
	//local UTUIScene S;
	switch ( List.List[Index].Tag)
	{
		case 0:
			UTPC.ChangeTeam();
			return true;
			break;

		case 1:
			/*
			S = UTUIScene( List.GetScene() );
			S.GetSceneClient().CloseScene(S);
			UTPC.OpenUIScene(class'UTUIFrontEnd_MainMenu'.default.SettingsScene);
			*/
			break;

		case 2:
			UTPC.ConsoleCommand("Reconnect");
			break;

		case 3:
			UTPC.QuitToMainMenu();
			return true;
			break;
	}


	return false;
}

/** @return whether the given team is Necris (used by e.g. vehicle factories to control whether they can activate for this team) */
simulated function bool IsNecrisTeam(byte TeamNum);

defaultproperties
{
	WeaponBerserk=+1.0
	BotDifficulty=-1
	FlagState[0]=FLAG_Home
	FlagState[1]=FLAG_Home

	MapMenuTemplate=none
	TickGroup=TG_PreAsyncWork
}
