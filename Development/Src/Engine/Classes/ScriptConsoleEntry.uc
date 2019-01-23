/**
 * Example of how to setup a complex widget which contains child widgets in unrealscript.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class ScriptConsoleEntry extends UIPanel;

var	instanced	UIEditBox		InputBox;
var	instanced	UIImage			UpperConsoleBorder;
var	instanced	UIImage			LowerConsoleBorder;

const ConsolePromptText = "(> ";

event Initialized()
{
	Super.Initialized();

	InsertChild(InputBox,INDEX_NONE,false);
	InsertChild(UpperConsoleBorder,INDEX_NONE,false);
	InsertChild(LowerConsoleBorder,INDEX_NONE,false);
}


event PostInitialize()
{
	Super.PostInitialize();

	// setup the lower border image
	LowerConsoleBorder.SetDockTarget(UIFACE_Bottom, Self, UIFACE_Bottom);

	// setup the input label
	InputBox.SetDockTarget(UIFACE_Left,		Self,				UIFACE_Left);
	InputBox.SetDockParameters(UIFACE_Bottom,	LowerConsoleBorder,	UIFACE_Bottom, -3);
	InputBox.SetDockTarget(UIFACE_Right,	Self,				UIFACE_Right);

	// setup the upper border image
	UpperConsoleBorder.SetDockParameters(UIFACE_Bottom, InputBox, UIFACE_Top, -2);

	// setup this widget
	SetDockTarget(UIFACE_Left,		GetParent(),		UIFACE_Left);
	SetDockTarget(UIFACE_Right,		GetParent(),		UIFACE_Right);
	SetDockTarget(UIFACE_Bottom,	GetParent(),		UIFACE_Bottom);
	SetDockTarget(UIFACE_Top,		UpperConsoleBorder,	UIFACE_Top);
}

function SetValue( string NewValue )
{
	InputBox.SetValue(ConsolePromptText $ NewValue);
}

function OnCreateChild( UIObject CreatedWidget, UIScreenObject CreatorContainer )
{
	if ( CreatedWidget == InputBox )
	{
		InputBox.StringRenderComponent.bIgnoreMarkup=true;
		InputBox.StringRenderComponent.EnableAutoSizing(UIORIENT_Vertical);

		InputBox.StringRenderComponent.SetWrapMode(CLIP_Wrap);
		InputBox.StringRenderComponent.StringCaret.bDisplayCaret=true;
	}
}

DefaultProperties
{
	WidgetTag=ConsoleInputBox
	PrimaryStyle=(DefaultStyleTag="ConsoleStyle",RequiredStyleClass=class'Engine.UIStyle_Combo')

	Begin Object Name=PanelBackgroundTemplate
		ImageStyle=(DefaultStyleTag="ConsoleBackgroundStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
	End Object

	// Rendering
	Begin Object Class=UIImage Name=LowerBorderTemplate
		WidgetTag=LowerConsoleBorder
		PrimaryStyle=(DefaultStyleTag="ConsoleImageStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
		Position={(
				Value[EUIWidgetFace.UIFACE_Right]=1.f,ScaleType[EUIWidgetFace.UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[EUIWidgetFace.UIFACE_Bottom]=2,ScaleType[EUIWidgetFace.UIFACE_Bottom]=EVALPOS_PixelOwner
				)}
	End Object
	LowerConsoleBorder=LowerBorderTemplate

	Begin Object Class=UIEditBox Name=ConsoleInputTemplate
		OnCreate=OnCreateChild
		WidgetTag=InputBox
		PrimaryStyle=(DefaultStyleTag="ConsoleStyle",RequiredStyleClass=class'Engine.UIStyle_Combo')
		DataSource=(MarkupString="(> ")
		Position=(Value[EUIWidgetFace.UIFACE_Bottom]=16,ScaleType[EUIWidgetFace.UIFACE_Bottom]=EVALPOS_PixelOwner)
	End Object
	InputBox=ConsoleInputTemplate

	Begin Object Class=UIImage Name=UpperBorderTemplate
		WidgetTag=UpperConsoleBorder
		PrimaryStyle=(DefaultStyleTag="ConsoleImageStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
		Position={(
				Value[EUIWidgetFace.UIFACE_Right]=1.f,ScaleType[EUIWidgetFace.UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[EUIWidgetFace.UIFACE_Bottom]=2,ScaleType[EUIWidgetFace.UIFACE_Bottom]=EVALPOS_PixelOwner
				)}
	End Object
	UpperConsoleBorder=UpperBorderTemplate
}
