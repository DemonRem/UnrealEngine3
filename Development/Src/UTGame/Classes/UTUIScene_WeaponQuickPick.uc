/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_WeaponQuickPick extends UTUIScene_Hud
	config(Game)
	native(UI);


/** The QuickPick Cells */
var transient HudWidget_QuickPickCell Cells[8];

/** The Current in use group */
var transient int CurrentGroup;

/** When is the next refresh */
var transient float RefreshTimer;

/** How often do we refresh */
var() float RefreshFrequency;

cpptext
{
	virtual void Tick( FLOAT DeltaTime );
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );
}

native function RefreshCells();

/**
 * Sets the QPBars array and reset the timing */

event PostInitialize()
{
	local int i,cnt;

	FindChildren(self);

	for (i=0;i<8;i++)
	{
		if ( Cells[i] != none )
		{
			cnt++;
		}
	}

	if (Cnt != 8)
	{
		`log("Quick Pick Error - Not enough cells defined in the scene");
	}

	Super.PostInitialize();

}

/** Scan all of the children of this widget and fill out the cell array */
function FindChildren(UIScreenObject Parent)
{
	local int i;
	local HudWidget_QuickPickCell Child;

	for (i=0;i<Parent.Children.Length;i++)
	{
		Child = HudWidget_QuickPickCell( Parent.Children[i] );
		if ( Child != none && Child.CellIndex >=0 && Child.CellIndex < 8 )
		{
			Cells[Child.CellIndex] = Child;
		}
		FindChildren( Parent.Children[i] );
	}
}


/**
 * Make a quick pick in a given group.
 *
 * @Returns the UTWeapon associated with the cell
 */
function UTWeapon QuickPick(int Quad)
{
	local UTWeapon Result;

	Result = none;

    // Unselect the old group

	if (Quad != CurrentGroup)
	{
		if ( CurrentGroup >= 0 && Cells[CurrentGroup] != none )
		{
			Cells[CurrentGroup].UnSelect();
		}

	    if ( Quad >= 0 && Quad < 8 )
	    {
			Cells[Quad].Select();
			Result = Cells[Quad].MyWeapon;
			CurrentGroup = Quad;
		}
	}

	Return Result;
}

/**
 * Shows the scene
 */

function Show()
{
	local int i;

	CurrentGroup = -1;

	for (i=0;i<8;i++)
	{
		if ( Cells[i] != none )
		{
			Cells[i].Unselect();
		}
	}

	RefreshCells();
	SetVisibility(true);

}

/**
 * Hides the scene.
 *
 * @Returns true if we should select the best weapon
 */

function Hide()
{
	SetVisibility(false);
}

defaultproperties
{
	CurrentGroup=-1
	bExemptFromAutoClose=true
	RefreshFrequency=0.25
}
