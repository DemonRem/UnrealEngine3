/**
 * Example of how to setup a scene in unrealscript.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UIScriptConsoleScene extends UIScene
	transient
	notplaceable;

/** the console's buffer text */
var	instanced UILabel				BufferText;

/** the background for the console */
var	instanced UIImage				BufferBackground;

/** where the text that is currently being typed appears */
var	instanced ScriptConsoleEntry	CommandRegion;

event Initialized()
{
	Super.Initialized();

	InsertChild(BufferBackground);
	InsertChild(BufferText);
	InsertChild(CommandRegion);
}

event PostInitialize()
{
	Super.PostInitialize();

	BufferBackground.SetDockTarget(UIFACE_Left,		Self,			UIFACE_Left);
	BufferBackground.SetDockTarget(UIFACE_Top,		Self,			UIFACE_Top);
	BufferBackground.SetDockTarget(UIFACE_Right,	Self,			UIFACE_Right);
	BufferBackground.SetDockTarget(UIFACE_Bottom,	CommandRegion,	UIFACE_Top);

	BufferText.SetDockTarget(UIFACE_Bottom, CommandRegion, UIFACE_Top);
}

function OnCreateChild( UIObject CreatedWidget, UIScreenObject CreatorContainer )
{
	if ( CreatedWidget == BufferText )
	{
		BufferText.StringRenderComponent.EnableAutoSizing(UIORIENT_Vertical);
		BufferText.StringRenderComponent.SetWrapMode(CLIP_Wrap);
	}
}

DefaultProperties
{
	SceneTag=ConsoleScene

	Position={(
		Value[EUIWidgetFace.UIFACE_Right]=1.f,ScaleType[EUIWidgetFace.UIFACE_Right]=EVALPOS_PercentageViewport,
		Value[EUIWidgetFace.UIFACE_Bottom]=0.75,ScaleType[EUIWidgetFace.UIFACE_Bottom]=EVALPOS_PercentageViewport
			)}

	Begin Object Class=UIImage Name=BufferBackgroundTemplate
		WidgetTag=BufferBackground
		PrimaryStyle=(DefaultStyleTag="ConsoleBufferImageStyle")
	End Object
	BufferBackground=BufferBackgroundTemplate

	Begin Object Class=UILabel Name=BufferTextTemplate
		OnCreate=OnCreateChild
		WidgetTag=BufferText
		PrimaryStyle=(DefaultStyleTag="ConsoleBufferStyle")
		Position=(Value[EUIWidgetFace.UIFACE_Right]=1.f,ScaleType[EUIWidgetFace.UIFACE_Right]=EVALPOS_PercentageOwner)
	End Object
	BufferText=BufferTextTemplate

	Begin Object Class=ScriptConsoleEntry Name=CommandRegionTemplate
	End Object
	CommandRegion=CommandRegionTemplate
}

