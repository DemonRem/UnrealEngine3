/**
 * Activated when a new UITabPage becomes the ActivePage of a tab control.
 *
 * Copyright 2007 Epic Games, Inc. All Rights Reserved
 */
class UIEvent_TabPageActivated extends UIEvent_TabControl;

DefaultProperties
{
	ObjName="Page Activated"
	bAutoActivateOutputLinks=false

	OutputLinks.Empty
	OutputLinks(0)=(LinkDesc="Activated")
	OutputLinks(1)=(LinkDesc="Deactivated")
}
