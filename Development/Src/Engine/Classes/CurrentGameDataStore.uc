/**
 * Provides data about the current game.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class CurrentGameDataStore extends UIDataStore_GameState
	native(inherit);

`include(Core/Globals.uci)

cpptext
{
	/**
	 * Gets the list of data fields exposed by this data provider.
	 *
	 * @param	out_Fields	will be filled in with the list of tags which can be used to access data in this data provider.
	 *						Will call GetScriptDataTags to allow script-only child classes to add to this list.
	 */
	virtual void GetSupportedDataFields( TArray<struct FUIDataProviderField>& out_Fields );

	/**
	 * Resolves the value of the data field specified and stores it in the output parameter.
	 *
	 * @param	FieldName		the data field to resolve the value for;  guaranteed to correspond to a property that this provider
	 *							can resolve the value for (i.e. not a tag corresponding to an internal provider, etc.)
	 * @param	out_FieldValue	receives the resolved value for the property specified.
	 *							@see GetDataStoreValue for additional notes
	 * @param	ArrayIndex		optional array index for use with data collections
	 */
	virtual UBOOL GetFieldValue( const FString& FieldName, struct FUIProviderFieldValue& out_FieldValue, INT ArrayIndex=INDEX_NONE );

	/**
	 * Returns a pointer to the data provider which provides the tags for this data provider.  Normally, that will be this data provider,
	 * but for some data stores such as the Scene data store, data is pulled from an internal provider but the data fields are presented as
	 * though they are fields of the data store itself.
	 */
	virtual UUIDataProvider* GetDefaultDataProvider();
}

/**
 * Contains the classes which should be used for instancing data providers.
 */
struct native GameDataProviderTypes
{
	/**
	 * the class to use for the game info data provider
	 */
	var	const	class<GameInfoDataProvider>	GameDataProviderClass;

	/**
	 * the class to use for the player data providers
	 */
	var const	class<PlayerDataProvider>	PlayerDataProviderClass;

	/**
	 * the class to use for the team data provider.
	 */
	var	const	class<TeamDataProvider>		TeamDataProviderClass;
};

var	const	GameDataProviderTypes			ProviderTypes;

/**
 * The GameInfoDataProvider that manages access to the current gameinfo's exposed data.
 */
var	protected	GameInfoDataProvider		GameData;

/**
 * The data providers for all players in the current match
 */
var	protected	array<PlayerDataProvider>	PlayerData;

/**
 * The data providers for all teams in the current match.
 */
var	protected	array<TeamDataProvider>		TeamData;

/**
 * Creates the GameInfoDataProvider that will track all game info state data
 */
final function CreateGameDataProvider( GameReplicationInfo GRI )
{
	if ( GRI != None )
	{
		GameData = new ProviderTypes.GameDataProviderClass;

		if ( !GameData.BindProviderInstance(GRI) )
		{
			`log(`location@"Failed to bind GameReplicationInfo to game data store:"@`showobj(GRI),,'DevDataStore');
		}
	}
	else
	{
		`log(`location@"NULL GRI specified!"@ `showobj(GameData),,'DevDataStore');
	}
}

/**
 * Creates a PlayerDataProvider for the specified PlayerReplicationInfo, and adds it to the PlayerData array.
 *
 * @param	PRI		the PlayerReplicationInfo to create the PlayerDataProvider for.
 */
final function AddPlayerDataProvider( PlayerReplicationInfo PRI )
{
	local int ExistingIndex;
	local PlayerDataProvider DataProvider;

`if(`notdefined(FINAL_RELEASE))
	local string PlayerName;

	PlayerName = PRI != None ? PRI.PlayerName : "None";
`endif

	`log(">> CurrentGameDataStore::AddPlayerDataProvider -" @ PRI @ "(" $ PlayerName $ ")",,'DevDataStore');
	if ( PRI != None )
	{
		if ( GameData != None )
		{
			ExistingIndex = FindPlayerDataProviderIndex(PRI);
			if ( ExistingIndex == INDEX_NONE )
			{
				DataProvider = new ProviderTypes.PlayerDataProviderClass;
				if ( !DataProvider.BindProviderInstance(PRI) )
				{
					`log("Failed to bind PRI to PlayerDataProvider:"@ DataProvider @ "for player" @ PlayerName,,'DevDataStore');
				}
				else
				{
					PlayerData[PlayerData.Length] = DataProvider;
				}
			}
			else
			{
				`log("PlayerDataProvider already registered in 'CurrentGame' data store for" @ PlayerName @ `showobj(PlayerData[ExistingIndex]),,'DevDataStore');
			}
		}
		else
		{
			`log("CurrentGame data provider not yet created!",,'DevDataStore');
		}
	}
	else
	{
		`log("NULL PRI specified - current number of player data provider:"@ PlayerData.Length,,'DevDataStore');
	}
	`log("<< CurrentGameDataStore::AddPlayerDataProvider -" @ PRI @ "(" $ PlayerName $ ")",,'DevDataStore');
}

/**
 * Removes the PlayerDataProvider associated with the specified PlayerReplicationInfo.
 *
 * @param	PRI		the PlayerReplicationInfo to remove the data provider for.
 */
final function RemovePlayerDataProvider( PlayerReplicationInfo PRI )
{
	local int ExistingIndex;

	if ( PRI != None )
	{
		ExistingIndex = FindPlayerDataProviderIndex(PRI);
		if ( ExistingIndex != INDEX_NONE )
		{
			if ( PlayerData[ExistingIndex].CleanupDataProvider() )
			{
				PlayerData.Remove(ExistingIndex,1);
			}
		}
	}
	else
	{
		`log(`location@"NULL PRI specified!"@ `showvar(PlayerData.Length),,'DevDataStore');
	}
}

/**
 * Creates a TeamDataProvider for the specified TeamInfo, and adds it to the TeamData array.
 *
 * @param	TI		the TeamInfo to create the TeamDataProvider for.
 */
final function AddTeamDataProvider( TeamInfo TI )
{
	local int ExistingIndex;
	local TeamDataProvider DataProvider;

`if(`notdefined(FINAL_RELEASE))
	local string TeamName;

	TeamName = TI != None ? TI.TeamName : "None";
`endif

	`log(">> CurrentGameDataStore::AddTeamDataProvider -" @ TI @ "(" $ TeamName $ ")",,'DevDataStore');
	if ( TI != None )
	{
		if ( GameData != None )
		{
			ExistingIndex = FindTeamDataProviderIndex(TI);
			if ( ExistingIndex == INDEX_NONE )
			{
				DataProvider = new ProviderTypes.TeamDataProviderClass;

				if ( !DataProvider.BindProviderInstance(TI) )
				{
					`log("Failed to bind TeamInfo to TeamDataProvider in 'CurrentGame' data store:" @ DataProvider @ "for team" @ TeamName,,'DevDataStore');
				}
				else
				{
					TeamData[TeamData.Length] = DataProvider;
				}
			}
			else
			{
				`log("TeamDataProvider already registered in 'CurrentGame' data store for" @ TeamName @ `showobj(TeamData[ExistingIndex]),,'DevDataStore');
			}
		}
	}
	else
	{
		`log("NULL TeamInfo specified - current number of team data providers:"@ TeamData.Length,,'DevDataStore');
	}
}

/**
 * Removes the TeamDataProvider associated with the specified TeamInfo.
 *
 * @param	TI		the TeamInfo to remove the data provider for.
 */
final function RemoveTeamDataProvider( TeamInfo TI )
{
	local int ExistingIndex;

	if ( TI != None )
	{
		ExistingIndex = FindTeamDataProviderIndex(TI);
		if ( ExistingIndex != INDEX_NONE )
		{
			if ( TeamData[ExistingIndex].CleanupDataProvider() )
			{
				TeamData.Remove(ExistingIndex,1);
			}
		}
	}
	else
	{
		`log(`location@"NULL TeamInfo specified!"@ `showvar(TeamData.Length,TeamCount),,'DevDataStore');
	}
}

/**
 * Returns the index into the PlayerData array for the PlayerDataProvider associated with the specified PlayerReplicationInfo.
 *
 * @param	PRI		the PlayerReplicationInfo to search for
 *
 * @return	an index into the PlayerData array for the PlayerDataProvider associated with the specified PlayerReplicationInfo,
 * or INDEX_NONE if no associated PlayerDataProvider was found
 */
final function int FindPlayerDataProviderIndex( PlayerReplicationInfo PRI )
{
	local int i, Result;

	Result = INDEX_NONE;

	for ( i = 0; i < PlayerData.Length; i++ )
	{
		if ( PlayerData[i].GetDataSource() == PRI )
		{
			Result = i;
			break;
		}
	}

	return Result;
}

/**
 * Returns the index into the TeamData array for the TeamDataProvider associated with the specified TeamInfo.
 *
 * @param	TI		the TeamInfo to search for
 *
 * @return	an index into the TeamData array for the TeamDataProvider associated with the specified TeamInfo, or INDEX_NONE
 *			if no associated TeamDataProvider was found
 */
final function int FindTeamDataProviderIndex( TeamInfo TI )
{
	local int i, Result;

	Result = INDEX_NONE;

	for ( i = 0; i < TeamData.Length; i++ )
	{
		if ( TeamData[i].GetDataSource() == TI )
		{
			Result = i;
			break;
		}
	}

	return Result;
}

/**
 * Returns a reference to the PlayerDataProvider associated with the PRI specified.
 *
 * @param	PRI		the PlayerReplicationInfo to search for
 *
 * @return	the PlayerDataProvider associated with the PRI specified, or None if there was no PlayerDataProvider for the
 *			PRI specified.
 */
final function PlayerDataProvider GetPlayerDataProvider( PlayerReplicationInfo PRI )
{
	local int Index;
	local PlayerDataProvider Provider;

	Index = FindPlayerDataProviderIndex(PRI);
	if ( Index != INDEX_NONE )
	{
		Provider = PlayerData[Index];
	}


	return Provider;
}

/**
 * Returns a reference to the TeamDataProvider associated with the TI specified.
 *
 * @param	TI		the TeamInfo to search for
 *
 * @return	the TeamDataProvider associated with the TeamInfo specified, or None if there was no TeamDataProvider for the
 *			TeamInfo specified.
 */
final function TeamDataProvider GetTeamDataProvider( TeamInfo TI )
{
	local int Index;
	local TeamDataProvider Provider;

	Index = FindTeamDataProviderIndex(TI);
	if ( Index != INDEX_NONE )
	{
		Provider = TeamData[Index];
	}


	return Provider;
}

/**
 * Clears all data provider references.
 */
final function ClearDataProviders()
{
	local int i;

	`log(">>" @ `location,,'DevDataStore');
	if ( GameData != None )
	{
		GameData.CleanupDataProvider();
	}

	for ( i = 0; i < PlayerData.Length; i++ )
	{
		PlayerData[i].CleanupDataProvider();
	}

	for ( i = 0; i < TeamData.Length; i++ )
	{
		TeamData[i].CleanupDataProvider();
	}

	GameData = None;
	PlayerData.Length = 0;
	TeamData.Length = 0;

	`log("<<" @ `location,,'DevDataStore');
}

/**
 * Called when the current map is being unloaded.  Cleans up any references which would prevent garbage collection.
 *
 * @return	TRUE indicates that this data store should be automatically unregistered when this game session ends.
 */
function bool NotifyGameSessionEnded()
{
	ClearDataProviders();

	// this game state data store should not be unregistered when the match is over.
	return false;
}


DefaultProperties
{
	Tag=CurrentGame

	ProviderTypes={(
		GameDataProviderClass=class'Engine.GameInfoDataProvider',
		PlayerDataProviderClass=class'Engine.PlayerDataProvider',
		TeamDataProviderClass=class'Engine.TeamDataProvider'
	)}
}
