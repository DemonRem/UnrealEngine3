/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Character Part tab page.
 */

class UTUITabPage_CharacterPart extends UTTabPage
	placeable
	native(UIFrontEnd);

cpptext
{
	virtual void Tick_Widget(FLOAT DeltaTime)
	{
		if(AnimDir != TAD_None)
		{
			eventOnUpdateAnim(DeltaTime);
		}
	}

	/**
	 * Routes rendering calls to children of this screen object.
	 *
	 * @param	Canvas	the canvas to use for rendering
	 */
	virtual void Render_Children( FCanvas* Canvas );
}

var transient vector OffsetVector;
var transient float	AnimTimeElapsed;

enum TabAnimDir
{
	TAD_None,
	TAD_LeftIn,
	TAD_LeftOut,
	TAD_RightOut,
	TAD_RightIn
};

var transient TabAnimDir AnimDir;

/** The type of parts we are enumerating over. */
var() ECharPart CharPartType;

/** List of maps widget. */
var transient UTUIMenuList PartList;

/** Delegate for when the user selects a part on this page. */
delegate transient OnPartSelected(ECharPart PartType, string InPartID);

/** Delegate for when the user changes the selected part on this page. */
delegate transient OnPreviewPartChanged(ECharPart PartType, string InPartID);

/** Post initialization event - Setup widget delegates.*/
event PostInitialize()
{
	Super.PostInitialize();

	// Setup delegates
	PartList = UTUIMenuList(FindChild('lstParts', true));
	if(PartList != none)
	{
		PartList.OnSubmitSelection = OnPartList_SubmitSelection;
		PartList.OnValueChanged = OnPartList_ValueChanged;
	}

	// Set the button tab caption.
	SetDataStoreBinding("<Strings:UTGameUI.FrontEnd.TabCaption_CharPart[" $ int(CharPartType) $ "]>");
}

function bool IsAnimating()
{
	return (AnimDir != TAD_None);
}

function StartAnim(TabAnimDir InAnimDir)
{
	AnimDir = InAnimDir;
	AnimTimeElapsed = 0.0;

	if(AnimDir==TAD_LeftIn || AnimDir==TAD_RightIn)
	{
		Opacity = 0.0;
	}

	SetVisibility(true);
}

event OnUpdateAnim(float DeltaTime)
{
	local float AlphaTime;
	local float TotalAnimTime;

	TotalAnimTime = 0.15;
	AnimTimeElapsed += DeltaTime;
	AlphaTime = fmin(AnimTimeElapsed / TotalAnimTime,1.0);

	switch(AnimDir)
	{
	case TAD_LeftIn:
		Opacity = AlphaTime;
		OffsetVector.X = -512 * (1.0 - AlphaTime); 
		break;
	case TAD_RightIn:
		Opacity = AlphaTime;
		OffsetVector.X = 512 * (1.0 - AlphaTime); 
		break;
	case TAD_LeftOut:
		Opacity = 1.0 - AlphaTime;
		OffsetVector.X = -512 * AlphaTime; 
		break;
	case TAD_RightOut:
		Opacity = 1.0 - AlphaTime;
		OffsetVector.X = 512 * AlphaTime; 
		break;
	}

	if(AnimTimeElapsed > TotalAnimTime)
	{
		OnAnimEnd();
	}
}

function OnAnimEnd()
{
	if(AnimDir==TAD_LeftOut || AnimDir==TAD_RightOut)
	{
		SetVisibility(false);
	}
	else
	{
		Opacity = 1.0;
		PartList.SetFocus(none);
	}

	AnimDir = TAD_None;
}

/**
 * Called when the user changes the current list index.
 *
 * @param	Sender	the list that is submitting the selection
 */
function OnPartList_SubmitSelection( UIList Sender, optional int PlayerIndex=0 )
{
	local int SelectedItem;
	local UTUIMenuList MenuList;
	local string StringValue;

	MenuList = UTUIMenuList(Sender);

	if(MenuList != none)
	{
		SelectedItem = MenuList.GetCurrentItem();

		if(MenuList.GetCellFieldString(MenuList, 'PartID', SelectedItem, StringValue))
		{
			OnPartSelected(CharPartType, StringValue);
		}
	}
}

/**
 * Called when the user presses Enter (or any other action bound to UIKey_SubmitListSelection) while this list has focus.
 *
 * @param	Sender	the list that is submitting the selection
 */
function OnPartList_ValueChanged( UIObject Sender, optional int PlayerIndex=0 )
{
	local int SelectedItem;
	local UTUIMenuList MenuList;
	local string StringValue;

	MenuList = UTUIMenuList(Sender);

	if(MenuList != none)
	{
		SelectedItem = MenuList.GetCurrentItem();

		if(MenuList.GetCellFieldString(MenuList, 'PartID', SelectedItem, StringValue))
		{
			OnPreviewPartChanged(CharPartType, StringValue);
		}
	}
}

defaultproperties
{
	bRequiresTick=true;
}