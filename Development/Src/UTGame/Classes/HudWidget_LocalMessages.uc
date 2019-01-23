/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_LocalMessages extends UTUI_HudWidget
	native(UI);

/* the message zones */
var Instanced UILabel MessageZones[7];
var Instanced UILabel WeaponName;

var transient string LastWeaponName;

var(Presentation) editinlineuse Font FontPool[4];

var transient bool bHACKInitialize;

/**
 * Sizes the zone to fit the font
 */
native function SizeZone(int ZoneIndex, int FontIndex);

/**
 * Clear any test strings left in the editor
 * FIXME: Widgets are getting their instanced values at some point after PostInitialize
 * is called.  Fix me later.
 */
event PostInitialize()
{
	local int i;

	Super.PostInitialize();

	for (i=0;i<7;i++)
	{
		MessageZones[i].SetValue("");
	}
}

/**
 * Accessors  - This function sets the message text of a given zone.  It won't reset text that already exists
 *
 * @Param	MessageText		The text to set.
 * @Param	Zone			The zone that will contain the text
 *
 * @returns true if the message is new
 */

function bool SetMessageText(int Zone, string MessageText)
{
	if ( MessageZones[Zone].GetValue() != MessageTExt )
	{
		MessageZones[Zone].SetValue(MessageText);
		return true;
	}
	return false;
}

function string GetMessageText(int Zone)
{
	return MessageZones[Zone].GetValue();
}

function SetMessageFont(int Zone, int FontSize)
{

	FontSize = clamp(FontSize, 0 ,3);

	MessageZones[Zone].StringRenderComponent.SetFont( FontPool[FontSize] );
	SizeZone(Zone, FontSize);
}

function SetMessageColor(int Zone, color NewColor)
{
	MessageZones[Zone].StringRenderComponent.SetColor( ColorToLinearColor(NewColor) );
}

function SetMessageOpacity(int Zone, float NewOpacity)
{
	MessageZones[Zone].StringRenderComponent.SetOpacity( NewOpacity );
}

/**
 * Accessors  - This function will cause a zone to fade from Start to Dest over Rate
 *
 * @Param	Zone			The zone that will be affected
 * @Param	StartAlpha		The starting Alpha
 * @Param	DestAlpha		The destination Alpha
 * @Param	Rate			How fast to fade
 */

function FadeMessage(int Zone, optional float StartAlpha=1.0, optional float DestAlpha=0.0, optional float Rate=1.0)
{
	MessageZones[Zone].StringRenderComponent.Fade(StartAlpha, DestAlpha, Rate);
}

/**
 * Accessors  - This function will cause a zone to pulse between fade Alpha and 0.0 in Rate seconds
 *
 * @Param	Zone			The zone that will be affected
 * @Param	MaxAlpha		The Max Alpha
 * @Param	Rate			How fast to fade
 */

function PulseMessage(int Zone, optional float MaxAlpha=1.0, optional float MinAlpha=0.0, optional float Rate=1.0)
{
	MessageZones[Zone].StringRenderComponent.Pulse(MaxAlpha, MinAlpha, Rate);
}


/**
 * Look at the weapon of the current pawn, if it's changed, change the text
 */


event WidgetTick(FLOAT DeltaTime)
{
	local string PendingWeaponName;
	local Pawn PawnOwner;
	local Weapon PendingWeapon;
	local UTWeapon UTW;
	local int i;

    SetVisibility(false);
	return;

	if (!bHACKInitialize)
	{
		bHACKInitialize = true;
		for (i=0;i<7;i++)
		{
			MessageZones[i].SetValue("");
		}
		WeaponName.SetValue("");
	}


	Super.WidgetTick(DeltaTime);

	PawnOwner = UTHudSceneOwner.GetPawnOwner();
	if ( PawnOwner != none && PawnOwner.InvManager != none )
	{
		if ( PawnOwner.InvManager.PendingWeapon != none )
		{
			PendingWeapon = PawnOwner.InvManager.PendingWeapon;
		}
		else
		{
			PendingWeapon = PawnOwner.Weapon;
		}

		if ( PendingWeapon != none )
		{
			PendingWeaponName = PendingWeapon.GetHumanReadableName();
		}

		if ( PendingWeaponName != LastWeaponName )
		{
			WeaponName.SetValue(PendingWeaponName);
			UTW = UTWeapon(PendingWeapon);
			if ( UTW != none )
			{
				WeaponName.StringRenderComponent.SetColor( ColorToLinearColor(UTW.WEaponColor) );
			}
			WeaponName.StringRenderComponent.Fade(2.0,0.0,2.0);
		}



	}
	LastWeaponName = PendingWeaponName;
}


defaultproperties
{
	Begin Object Class=UILabel Name=lMessageZones0
		Position={( Value[UIFACE_Left]=0.2,
				Value[UIFACE_Top]=0.13,
				Value[UIFACE_Right]=0.6,
				Value[UIFACE_Bottom]=0.03125,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		WidgetTag=Zone0
		DataSource=(MarkupString="Zone 0",RequiredFieldType=DATATYPE_Property)
		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	MessageZones(0)=lMessageZones0

	Begin Object Class=UILabel Name=lMessageZones1
		Position={( Value[UIFACE_Left]=0.2,
				Value[UIFACE_Top]=0.242,
				Value[UIFACE_Right]=0.6,
				Value[UIFACE_Bottom]=0.06250,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		WidgetTag=Zone1
		DataSource=(MarkupString="Zone 1",RequiredFieldType=DATATYPE_Property)

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	MessageZones(1)=lMessageZones1

	Begin Object Class=UILabel Name=lMessageZones2
		Position={( Value[UIFACE_Left]=0.2,
				Value[UIFACE_Top]=0.58,
				Value[UIFACE_Right]=0.6,
				Value[UIFACE_Bottom]=0.06250,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		WidgetTag=Zone2
		DataSource=(MarkupString="Zone 2",RequiredFieldType=DATATYPE_Property)

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	MessageZones(2)=lMessageZones2

	Begin Object Class=UILabel Name=lMessageZones3
		Position={( Value[UIFACE_Left]=0.2,
				Value[UIFACE_Top]=0.80,
				Value[UIFACE_Right]=0.6,
				Value[UIFACE_Bottom]=0.06250,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		WidgetTag=Zone3
		DataSource=(MarkupString="Zone 3",RequiredFieldType=DATATYPE_Property)

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	MessageZones(3)=lMessageZones3

	Begin Object Class=UILabel Name=lMessageZones4
		Position={( Value[UIFACE_Left]=0.2,
				Value[UIFACE_Top]=0.86,
				Value[UIFACE_Right]=0.6,
				Value[UIFACE_Bottom]=0.03,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		WidgetTag=Zone4
		DataSource=(MarkupString="Zone 4",RequiredFieldType=DATATYPE_Property)

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	MessageZones(4)=lMessageZones4

	Begin Object Class=UILabel Name=lMessageZones5
		Position={( Value[UIFACE_Left]=0.2,
				Value[UIFACE_Top]=0.16125,
				Value[UIFACE_Right]=0.6,
				Value[UIFACE_Bottom]=0.03125,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		WidgetTag=Zone6
		DataSource=(MarkupString="Zone 5",RequiredFieldType=DATATYPE_Property)

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	MessageZones(5)=lMessageZones5

	Begin Object Class=UILabel Name=lMessageZones6
		Position={( Value[UIFACE_Left]=0.2,
				Value[UIFACE_Top]=0.19250,
				Value[UIFACE_Right]=0.6,
				Value[UIFACE_Bottom]=0.031250,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		WidgetTag=Zone7
		DataSource=(MarkupString="Zone 6",RequiredFieldType=DATATYPE_Property)

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	MessageZones(6)=lMessageZones6

	Begin Object Class=UILabel Name=lWeaponName
		Position={( Value[UIFACE_Left]=0.2,
				Value[UIFACE_Top]=0.19250,
				Value[UIFACE_Right]=0.6,
				Value[UIFACE_Bottom]=0.031250,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}
		WidgetTag=WeaponName
		DataSource=(MarkupString="Weapon Name",RequiredFieldType=DATATYPE_Property)

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	WeaponName=lWeaponName

	Position={( Value[UIFACE_Left]=0.0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageScene,
				Value[UIFACE_Top]=0.0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageScene,
				Value[UIFACE_Right]=1.0,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageScene,
				Value[UIFACE_Bottom]=1.0,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageScene)}

	WidgetTag=LocalMessagesWidget

	FontPool[0]=font'UI_Fonts.MultiFonts.MF_HudSmall'
	FontPool[1]=font'UI_Fonts.MultiFonts.MF_HudMedium'
	FontPool[2]=font'UI_Fonts.MultiFonts.MF_HudLarge'
	FontPool[3]=font'UI_Fonts.MultiFonts.MF_HudHuge'

	bRequiresTick=true
	bHACKInitialize=false
}
