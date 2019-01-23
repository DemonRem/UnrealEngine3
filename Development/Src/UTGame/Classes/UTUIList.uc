/**
* Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
*
* Extended version of UIList for UT3.
*/
class UTUIList extends UIList
	native(UI);

DefaultProperties
{
	// No UT lists nav up or down, so disable these events.
	Begin Object Name=WidgetEventComponent
		DisabledEventAliases.Add(NavFocusUp)
		DisabledEventAliases.Add(NavFocusDown)
	End Object
}