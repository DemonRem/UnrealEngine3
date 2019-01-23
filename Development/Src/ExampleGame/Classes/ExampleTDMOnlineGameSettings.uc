/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Holds the base configuration settings for DM match
 *
 * NOTE: This class will normally be code generated, but the tool doesn't exist yet
 */
class ExampleTDMOnlineGameSettings extends ExampleDMOnlineGameSettings;

/** Dynamic values that can be exposed to match making */
const PROP_FriendlyFire = 0x10000005;
const PROP_HumanTeamScore = 0x1000000A;
const PROP_OrcTeamScore = 0x1000000B;

defaultproperties
{
	// TDM specific properties
	Properties(2)=(PropertyId=PROP_FriendlyFire,Data=(Type=SDT_Int32,Value1=0))
	Properties(3)=(PropertyId=PROP_HumanTeamScore,Data=(Type=SDT_Int32,Value1=0))
	Properties(4)=(PropertyId=PROP_OrcTeamScore,Data=(Type=SDT_Int32,Value1=0))
	PropertyMappings(2)=(Id=PROP_FriendlyFire,Name="FriendlyFire")
	PropertyMappings(3)=(Id=PROP_HumanTeamScore,Name="HumanTeamScore")
	PropertyMappings(4)=(Id=PROP_OrcTeamScore,Name="OrcTeamScore")
}
