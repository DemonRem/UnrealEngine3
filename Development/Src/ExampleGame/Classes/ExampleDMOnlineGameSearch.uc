/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Holds the base configuration settings for TDM match
 *
 * NOTE: This class will normally be code generated, but the tool doesn't exist yet
 */
class ExampleDMOnlineGameSearch extends OnlineGameSearch;

/** Dynamic values that can be exposed to match making */
const PROP_NonStandardOptions = 0x10000006;
const PROP_DedicatedServer = 0x10000009;

/** The online query to execute */
const QUERY_DMSearch = 1;

defaultproperties
{
	GameSettingsClass=class'ExampleGame.ExampleDMOnlineGameSettings'
	// Id of the query to execute
	Query=(ValueIndex=QUERY_DMSearch)
	bIsLanQuery=true
	// Properties and their mappings
	Properties(0)=(PropertyId=PROP_NonStandardOptions,Data=(Type=SDT_Int32,Value1=0))
	Properties(1)=(PropertyId=PROP_DedicatedServer,Data=(Type=SDT_Int32,Value1=0))
	PropertyMappings(0)=(Id=PROP_NonStandardOptions,Name="NonStandardOptions")
	PropertyMappings(1)=(Id=PROP_DedicatedServer,Name="DedicatedServer")
}
