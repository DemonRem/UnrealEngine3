/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Specific derivation of UIDataStore_OnlineStats to expose the TDM leaderboards
 */
class UTDataStore_OnlineStats extends UIDataStore_OnlineStats
	native;

/** The class to load and instance */
var class<Settings> LeaderboardSettingsClass;

/** The set of settings that are to be exposed to the UI for filtering the leaderboards */
var Settings LeaderboardSettings;

/** The data provider that will expose the leaderboard settings to the ui */
var UIDataProvider_Settings SettingsProvider;

cpptext
{
private:
// UIDataStore interface

	/**
	 * Loads and creates an instance of the registered filter object
	 */
	virtual void InitializeDataStore(void);

	/**
	 * Returns the stats read results as a collection and appends the filter provider
	 *
	 * @param OutFields	out value that receives the list of exposed properties
	 */
	virtual void GetSupportedDataFields(TArray<FUIDataProviderField>& OutFields);

	/**
	 * Returns the list element provider for the specified proprety name
	 *
	 * @param PropertyName the name of the property to look up
	 *
	 * @return pointer to the interface or null if the property name is invalid
	 */
	virtual TScriptInterface<IUIListElementProvider> ResolveListElementProvider(const FString& PropertyName)
	{
		// Make a copy because we potentially modify the string
		FString CompareName(PropertyName);
		FString ProviderName;
		// If there is an intervening provider name, snip it off
		if (ParseNextDataTag(CompareName,ProviderName) == FALSE)
		{
			CompareName = ProviderName;
		}
		// Check for the stats results
		if (FName(*CompareName) == StatsReadName)
		{
			return this;
		}
		// See if this is for one of our filters
		return SettingsProvider->ResolveListElementProvider(CompareName);
	}
}

/**
 * Gears specific function that figures out what type of search to do
 */
function SetStatsReadInfo()
{
	local int ObjectIndex;
	local int GameModeId;
	local int PlayerFilterType;
	local int MatchTypeId;

	// Figure out which set of leaderboards to use by gamemode
	LeaderboardSettings.GetStringSettingValue(LF_GameMode,GameModeId);
	LeaderboardSettings.GetStringSettingValue(LF_MatchType,MatchTypeId);
	
`Log("GameModeId is "$GameModeId);
	// @todo: Change this once we have more game mode read objects.
	ObjectIndex = 0;

`Log("MatchTypeId is "$MatchTypeId);
	switch(MatchTypeId)
	{
	case MTS_Ranked:
		ObjectIndex += 0;
		break;
	case MTS_Player:
		ObjectIndex += 1;
		break;
	case MTS_RankedSkill:
		ObjectIndex += 2;
		break;
	}

	// Choose the read object based upon which filter they selected
	StatsRead = StatsReadObjects[ObjectIndex];
`Log("StatsRead is "$StatsRead);
	// Read the set of players they want to view
	LeaderboardSettings.GetStringSettingValue(LF_PlayerFilterType,PlayerFilterType);
	switch (PlayerFilterType)
	{
		case PFS_Player:
			CurrentReadType = SFT_Player;
			break;
		case PFS_CenteredOnPlayer:
			CurrentReadType = SFT_CenteredOnPlayer;
			break;
		case PFS_Friends:
			CurrentReadType = SFT_Friends;
			break;
		case PFS_TopRankings:
			CurrentReadType = SFT_TopRankings;
			break;
	}
}

defaultproperties
{
	LeaderboardSettingsClass=class'UTGame.UTLeaderboardSettings'

// DM stats
	StatsReadClasses(0)=class'UTGame.UTLeaderboardRankedDM'
	StatsReadClasses(1)=class'UTGame.UTLeaderboardPlayerDM'
	StatsReadClasses(2)=class'UTGame.UTLeaderboardRankedSkillDM'

	Tag=UTLeaderboards
}