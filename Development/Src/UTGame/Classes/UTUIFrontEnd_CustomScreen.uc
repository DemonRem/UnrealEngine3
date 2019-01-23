/**
* Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
*
* Custom Scene for UT3, contains a description label and an automated way of providing descriptions for widgets.
*/
class UTUIFrontEnd_CustomScreen extends UTUIFrontEnd;

var transient UILabel	DescriptionLabel;

/** Mapping of widget tag to field description. */
struct DescriptionMapping
{
	var name WidgetTag;
	var string DataStoreMarkup;
};

var transient array<DescriptionMapping> DescriptionMap;

/** PostInitialize event - Sets delegates for the scene. */
event PostInitialize( )
{
	Super.PostInitialize();

	DescriptionLabel = UILabel(FindChild('lblDescription', true));
}


/** Callback for when the object's active state changes. */
function OnNotifyActiveStateChanged( UIScreenObject Sender, int PlayerIndex, UIState NewlyActiveState, optional UIState PreviouslyActiveState )
{
	local int MappingIdx;

	if(NewlyActiveState.Class == class'UIState_Focused'.default.Class)
	{

		// Loop through all description mappings and try to set a description based on the currently focused widget.
		for(MappingIdx=0; MappingIdx<DescriptionMap.length; MappingIdx++)
		{
			if(DescriptionMap[MappingIdx].WidgetTag==Sender.Name)
			{
				DescriptionLabel.SetDataStoreBinding(DescriptionMap[MappingIdx].DataStoreMarkup);
				break;
			}
		}
	}
}

