/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_SingleValue extends UTUI_HudWidget;

var instanced UILabel ValueText;

var transient float FlashRate;
var transient LinearColor FlashColor;

/**
 * Call each frame to find out if the value of this widget has changed.
 * !! This function should be subclassed !!
 *
 * @params out ValueStr	- The value of the text will be returned here
 * @returns true if the value has changed
 */

function bool GetText(out string ValueStr, float DeltaTime)
{
	return false;
}


/**
 * Accessor for setting the value of this widget outright.  Children should override this
 * function and perform any fixup.
 *
 * @params NewText	- Text to change to
 */
function SetText(string NewValue)
{
	ValueText.SetValue(NewValue);
}

/**
 * Call natively each tick.  Look to see if the value has changed and if so, update the label
 *
 * @params DeltaTime	- Amount of time since last call
 */

event WidgetTick(float DeltaTime)
{
	local string NewValue;

	if (FlashRate > 0.0f)
	{
		FlashColor.R += (1.0f - FlashColor.R) * (DeltaTime / FlashRate);
		FlashColor.G += (1.0f - FlashColor.G) * (DeltaTime / FlashRate);
		FlashColor.B += (1.0f - FlashColor.B) * (DeltaTime / FlashRate);
		FlashRate -= DeltaTime;

		if ( FlashRate < 0.0f )
		{
			FlashRate = 0.0f;
		}

		ValueText.StringRenderComponent.SetColor(FlashColor);
	}
	else
	{
		ValueText.StringRenderComponent.DisableCustomColor();
	}

	if ( GetText( NewValue, DeltaTime ) )
	{
		ValueText.SetValue( NewValue );
	}
}

event Flash(float NewRate, LinearColor NewColor)
{
	FlashRate = NewRate;
	FlashColor = NewColor;
}

defaultproperties
{
	Begin Object Class=UILabel Name=lblValueText
		DataSource=(MarkupString="000",RequiredFieldType=DATATYPE_Property)
		WidgetTag=ValueText

		Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageOwner,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageOwner,
				Value[UIFACE_Right]=1.0,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageOwner,
				Value[UIFACE_Bottom]=1.0,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageOwner)}

		// PRIVATE_Protected
		PrivateFlags=PRIVATE_Protected
	End Object
	ValueText=lblValueText

	bRequiresTick=true

	Position={( Value[UIFACE_Left]=0,
				ScaleType[UIFACE_Left]=EVALPOS_PercentageScene,
				Value[UIFACE_Top]=0,
				ScaleType[UIFACE_Top]=EVALPOS_PercentageScene,
				Value[UIFACE_Right]=0.119140625,
				ScaleType[UIFACE_Right]=EVALPOS_PercentageScene,
				Value[UIFACE_Bottom]=0.069010416,
				ScaleType[UIFACE_Bottom]=EVALPOS_PercentageScene)}

	FlashRate=4
}

