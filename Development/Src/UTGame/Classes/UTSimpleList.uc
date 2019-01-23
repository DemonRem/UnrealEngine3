/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSimpleList extends UTDrawPanel
	native(UI);

/************ These values can be set in the editor ***********/

/** The font to use for rendering */
var() font TextFont;

var() color NormalColor;
var() color AboveBelowColor;
var() color SelectedColor;
var() color SelectionBarColor;

var() texture2D SelectionImage;

/** This is how big the normal cell of text is at 1024x768. */
var() float DefaultCellHeight;

/** How mucbh to multiply the above and below cells by */
var() float AboveBelowCellHeightMultiplier;

/** How much to multiply the selected cell by */
var() float SelectionCellHeightMultiplier;

/** How fast should we transition between objects */
var() float TransitionTime;

/** This is the ratio of Height to widget for the selection widget's box.  It will be used to size the list*/
var() float ScrollWidthRatio;

struct native SimpleListData
{
	// Text for this entry.  Can be markup as it will be resolved when set
	var()	string	Text;

	// A general int TAG for indexing extra data.
	var() int		Tag;

	// HeightMultipliers - Used to interpolate between the various heights of a widget
	var float HeightMultipliers[2];

	// The current Height Multiplier
	var float CurHeightMultiplier;

	// How much time left before the cell has completed it's transition
	var float CellTransitionTime;

	// Where was this item last rendered.  We will use this to
	// figure out where to show the scrollmarks.  NOTE: We only store the Y since X is implicit
	var float YBounds[2];

	// Is set to true if this cell was actually rendered this frame
	var bool bWasRendered;

	structdefaultproperties
	{
		CurHeightMultiplier=1.0
		HeightMultipliers(0)=1.0
		HeightMultipliers(1)=1.0
		CellTransitionTime=0.0
	}
};

/** The list itself. */
var() editinline array<SimpleListData> List;

/** Index of the current top widget */
var transient int Top;

/** Index of the currently selected widget */
var() transient int Selection;

/** If true, the positions have been invalidated so recalculate the sizes/etc. */
var transient bool bInvalidated;

/** This is calculated once each frame and cached.  It holds the resolution scaling factor
    that is forced to a 4:3 aspect ratio.  */
var transient vector2D ResScaling;

/** Last time this scene was rendered */
var transient float LastRenderTime;

/** Defines the top of the window in to the overall list to begin rendering */
var transient float WindowTop;

/** How big is the rendering window */
var transient float WindowHeight;

/** How big is the list in pixels */
var transient float ListHeightInPixel;

/** Used to animate the scroll.  These hold the target top position and the current transition timer */
var transient float TargetWindowTop;
var transient float WindowTopTransitionTime;

/** We cache the rendering bounds of the Up/Down arrows for quick mouse look up. */
var transient float UpArrowBounds[4];
var transient float DownArrowBounds[4];


var transient bool bDragging;
var transient float DragAdjustment;

var float LastMouseUpdate;

var float SelectionBarTransitionTime;

/*	======================================================================
		Natives
	====================================================================== */

cpptext
{
	virtual void PostEditChange(UProperty* PropertyThatChanged);
}


/**
 * @Returns the index of the first entry matching SearchText
 */
native function int Find(string SearchText);

/**
 * @Returns the index of the first entry with a tag that matches SearchTag
 */
native function int FindTag(int SearchTag);


/**
 * Sorts the list
 */

native function SortList();

native function UpdateAnimation(FLOAT DeltaTime);


/**
 * Setup the input system
 */
event PostInitialize()
{
	local int i;
	Super.PostInitialize();
	OnProcessInputKey=ProcessInputKey;
	OnProcessInputAxis=ProcessInputAxis;

	// Force resolve of everything

	for (i=0;i<List.Length;i++)
	{
		List[i].Text = ResolveText(List[i].Text);
	}

}

/*	======================================================================
		Input
	====================================================================== */


event GetSupportedUIActionKeyNames(out array<Name> out_KeyNames )
{
	out_KeyNames[out_KeyNames.Length] = 'SelectionUp';
	out_KeyNames[out_KeyNames.Length] = 'SelectionDown';
	out_KeyNames[out_KeyNames.Length] = 'SelectionHome';
	out_KeyNames[out_KeyNames.Length] = 'SelectionEnd';
	out_KeyNames[out_KeyNames.Length] = 'SelectionPgUp';
	out_KeyNames[out_KeyNames.Length] = 'SelectionPgDn';
	out_KeyNames[out_KeyNames.Length] = 'Select';
	out_keyNames[out_KeyNames.Length] = 'Click';
	out_keyNames[out_KeyNames.Length] = 'MouseMoveX';
	out_keyNames[out_KeyNames.Length] = 'MouseMoveY';
}

/**
 * @Returns the mouse position in widget space
 */
function Vector GetMousePosition()
{
	local int x,y;
	local vector2D MousePos;
	local vector AdjustedMousePos;
	class'UIRoot'.static.GetCursorPosition( X, Y );
	MousePos.X = X;
	MousePos.Y = Y;
	AdjustedMousePos = PixelToCanvas(MousePos);
	AdjustedMousePos.X -= GetPosition(UIFACE_Left,EVALPOS_PixelViewport);
	AdjustedMousePos.Y -= GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
	return AdjustedMousePos;
}

function bool ProcessInputKey( const out SubscribedInputEventParameters EventParms )
{
	if (EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat || EventParms.EventType == IE_DoubleClick)
	{
	 	if ( EventParms.InputAliasName == 'SelectionUp' )
	 	{
		 	SelectItem(Selection - 1);
		 	PlayUISound('ListUp');
		}
		else if ( EventParms.InputAliasName == 'SelectionDown' )
		{
			SelectItem(Selection + 1);
			PlayUISound('ListDown');
		}
		else if ( EventParms.InputAliasName == 'SelectionHome' )
		{
			SelectItem(0);
			PlayUISound('ListSubmit');
		}
		else if ( EventParms.InputAliasName == 'SelectionEnd' )
		{
			SelectItem(List.Length-1);
			PlayUISound('ListSubmit');
		}
		else if ( EventParms.InputAliasName == 'SelectionPgUp' )
		{
			PgUp();
			PlayUISound('ListSubmit');
		}
		else if ( EventParms.InputAliasName == 'SelectionPgDn' )
		{
			PgDn();
			PlayUISound('ListSubmit');
		}
		else if ( EventParms.InputAliasName == 'Select' )
		{
			ItemChosen(EventParms.PlayerIndex);
		}
		else if ( EventParms.InputAliasName == 'Click' )
		{
			PlayUISound('ListSubmit');
			SelectUnderCursor();

			if ( EventParms.EventType == IE_DoubleClick )
			{
				ItemChosen(EventParms.PlayerIndex);
			}
		}

	}
	else if ( EventParms.EventType == IE_Released && EventParms.InputAliasName == 'Click' )
	{
		bDragging = false;
	}

	return true;
}

function ItemChosen(int PlayerIndex)
{
	OnItemChosen(self, Selection,PlayerIndex);
}

function bool MouseInBounds()
{
	local Vector MousePos;
	local float w,h;

	MousePos = GetMousePosition();

    w = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelViewport);
    h = GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport);

	return ( MousePos.X >=0 && MousePos.X < w && MousePos.Y >=0 && MousePos.Y < h );
}

/**
 * Enable hottracking if we are dragging
 */
function bool ProcessInputAxis( const out SubscribedInputEventParameters EventParms )
{
	if ( bDragging && EventParms.InputKeyName=='MouseY' )
	{
		if ( MouseInBounds() )
		{
			SelectUnderCursor();
		}

	    return true;
	}

	return false;
}

/**
 * @Returns true if the mouse is within the bounds given
 * @Param X1		Left
 * @Param Y1		Top
 * @Param X2		Right
 * @Param Y2		Bottom
 *
 * All are in pixels
 */
function bool CursorCheck(float X1, float Y1, float X2, float Y2)
{
	local vector MousePos;;

	MousePos = GetMousePosition();

	return ( (MousePos.X >= X1 && MousePos.X <= X2) && (MousePos.Y >= Y1 && MousePos.Y <= Y2) );
}


/**
 * Select whichever widget happens to be under the cursor
 */
function SelectUnderCursor()
{
	local int w;
	local vector AdjustedMousePos;
	local int i;

	AdjustedMousePos = GetMousePosition();

	w = GetBounds(UIORIENT_Horizontal, EVALPOS_PixelViewport);

	if ( (CursorCheck(UpArrowBounds[0],UpArrowBounds[1],UpArrowBounds[2],UpArrowBounds[3])) ||
		 (CursorCheck(DownArrowBounds[0],DownArrowBounds[1],DownArrowBounds[2],DownArrowBounds[3])) )
	{
		bDragging = true;
	 	return;
	}

	if ( AdjustedMousePos.X >= 0 && AdjustedMousePos.X <= w)
	{
		for (i=0;i<List.Length;i++)
		{
			if ( List[i].bWasRendered && (AdjustedMousePos.Y >= List[i].YBounds[0] && AdjustedMousePos.Y <= List[i].YBounds[1]) )
			{
				SelectItem(i);
				break;
			}
		}
	}
}

/**
 * Move up 10 items
 */
function PgUp()
{
	local int NewSelection;
	NewSelection = Clamp(Selection - 10, 0, List.Length-1);
	SelectItem(NewSelection);
}

/**
 * Move down 10 items
 */
function PgDn()
{
	local int NewSelection;
	NewSelection = Clamp(Selection + 10, 0, List.Length-1);
	SelectItem(NewSelection);
}


/*	======================================================================
		List Management
	====================================================================== */


/**
 * Imports a string list and fills the array
 *
 * @Param StringList 	The Stringlist to import
 */
 event ImportStringList(array<string> StringList)
{
	local int i;
	for (i=0;i<StringList.Length;i++)
	{
		AddItem(StringList[i]);
	}
}


/**
 * Attempt to resolve markup to it's string value
 *
 * @Returns the resolve text or the original text if unresolable
 */
function string ResolveText(string Markup)
{
	local string s;
	if ( class'UIRoot'.static.GetDataStoreStringValue(Markup, S) )
	{
		return s;
	}
	return Markup;
}

/**
 * Adds an item to the list
 *
 * @Param Text		String caption of what to add
 * @Param Tag		A generic tag that can be assoicated with this list
 *
 * @ToDo: Add support for resolving markup on assignment.
 */
event AddItem(string Text, optional int Tag=-1)
{
	local int Index;
	Index = List.Length;
	List.Length = Index+1;
	List[Index].Text = ResolveText(Text);
	List[Index].Tag = Tag;
	List[Index].HeightMultipliers[0] = 1.0;
	List[Index].HeightMultipliers[1] = 1.0;
	List[Index].CurHeightMultiplier = 1.0;

	bInvalidated = true;

	// Select the first item

	if (List.Length == 1)
	{
		SelectItem(Index);
	}
	else
	{
		SelectItem(Selection);
	}

}

/**
 * Inserts a string somewhere in the stack
 *
 * @Param Text		String caption of what to add
 * @Param Tag		A generic tag that can be assoicated with this list
 *
 * @ToDo: Add support for resolving markup on assignment.
 */
event InsertItem(int Index, string Text, optional int Tag=-1)
{
	List.Insert(Index,1);
	List.Length = Index+1;
	List[Index].Text = ResolveText(Text);
	List[Index].Tag = Tag;
	List[Index].HeightMultipliers[0] = 1.0;
	List[Index].HeightMultipliers[1] = 1.0;
	List[Index].CurHeightMultiplier = 1.0;

	bInvalidated = true;

	// Select the first item

	if (List.Length == 1)
	{
		SelectItem(Index);
	}
	else
	{
		SelectItem(Selection);
	}


}

/**
 * Removes an item from the list
 */
event RemoveItem(int IndexToRemove)
{
	List.Remove(IndexToRemove,1);
	bInvalidated = true;

}


/**
 * Attempts to find a string then remove it from the list
 *
 * @Param TextToRemove		The String to remove
 */
 event RemoveString(string TextToRemove)
 {
 	local int i;
 	i = Find(TextToRemove);
 	if (i > INDEX_None)
 	{
 		RemoveItem(i);
 	}

	bInvalidated = true;

}

/**
 * Empties the list
 */
event Empty()
{
	Selection = -1;
	Top = -1;
	List.Remove(0,List.Length);
	bInvalidated = true;

}

/**
 * @Returns the list as an array of strings
 */
function ToStrings(out array<string> StringList, optional bool bAppend)
{
	local int i;

	if (!bAppend)
	{
		StringList.Remove(0,StringList.Length);
	}

	for (i=0;i<List.Length;i++)
	{
		StringList[i] = List[i].Text;
	}
}

/*	======================================================================
		Rendering / Sizing - All of these function assume Canvas is valid.
	====================================================================== */

/**
 * SizeList is called directly before rendering anything in the list.
 */

event SizeList()
{
	local int i;
	local Vector2D ViewportSize;
	local float SelectedPosition;
	local float SelectedSize;
	local float Size;

	/** Calculate the resolution scaling factor */

	GetViewportSize(ViewportSize);

	ResScaling.Y = ViewportSize.Y / 768;
	ResScaling.X = ResScaling.Y * 1.33333333;	// 4:3

	ListHeightInPixel = 0;
	for (i=0;i<List.Length;i++)
	{

		Size = DefaultCellHeight * List[i].CurHeightMultiplier * ResScaling.Y;

		if (i == Selection )
		{
			SelectedPosition = ListHeightInPixel;
			SelectedSize = Size;
		}

		ListHeightInPixel += Size;
	}

	WindowHeight = GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport);

	WindowHeight = FMin(WindowHeight, ListHeightInPixel);

	// Try and position the selected item in the center of the list.

	if ( bDragging )
	{
		if (SelectedPosition < WindowTop)
		{
			TargetWindowTop = SelectedPosition;
		}
		else if (SelectedPosition > WindowTop + WindowHeight - (DefaultCellHeight * SelectionCellHeightMultiplier) )
		{
			TargetWindowTop = SelectedPosition - WindowHeight + (DefaultCellHeight * SelectionCellHeightMultiplier);
		}
	}
	else
	{
		TargetWindowTop = SelectedPosition + (SelectedSize * 0.5) - (WindowHeight * 0.5);
	}

	TargetWindowTop = FClamp(TargetWindowTop, 0, ListHeightInPixel - WindowHeight);
	if (TargetWindowTop != WindowTop && WindowTopTransitionTime <= 0.0)
	{
		WindowTopTransitionTime = 0.15;
	}
}

/**
 * Render the list.  At this point each cell should be sized, etc.
 */
event DrawPanel()
{
	local int DrawIndex;
	local float XPos, YPos, CellHeight;
	local float TimeSeconds,DeltaTime;
	local vector MousePos;

	// If the list is empty, exit right away.

	if ( List.Length == 0 )
	{
		return;
	}


	TimeSeconds = UTUIScene( GetScene() ).GetWorldInfo().TimeSeconds;
	DeltaTime = TimeSeconds - LastRenderTime;
	LastRenderTime = TimeSeconds;

	if (bDragging )
	{
		if (!MouseInBounds() && LastMouseUpdate <= 0.0)
		{
			MousePos = GetMousePosition();
			if (MousePos.Y < 0)
			{
				SelectItem(Selection-1);
			}
			else
			{
				SelectItem(Selection+1);
			}
			LastMouseUpdate = 0.15;
		}
		else
		{
			LastMouseUpdate -= Deltatime;
		}
	}

	if (TargetWindowTop != WindowTop)
	{
		if ( WindowTopTransitionTime > 0.0 )
		{
			WindowTop += (TargetWindowTop - WindowTop) * (DeltaTime / WindowTopTransitionTime);
			WindowTopTransitionTime -= DeltaTime;
		}

		if ( WindowTopTransitionTime <= 0.0 )
		{
			WindowTop = TargetWindowTop;
			WindowTopTransitionTime = 0;
		}
	}
	else
	{
		WindowTopTransitionTime = 0.0f;
	}

	UpdateAnimation(DeltaTime * UTUIScene( GetScene() ).GetWorldInfo().TimeDilation);


	// FIXME: Big optimization if we don't have to recalc the
	// list size each frame.  We should only have to do this the resoltuion changes,
	// if we have added items to the list, or if the list is moving.  But for now this is
	// fine.

	bInvalidated = true;

	Canvas.Font = TextFont;

	SizeList();

	XPos = DefaultCellHeight * SelectionCellHeightMultiplier * ScrollWidthRatio;
	YPos = 0 - WindowTop;	// Figure out where to start rendering

	DrawIndex = 0;
	for (DrawIndex = 0; DrawIndex < List.Length; DrawIndex++)
	{
		// Determine if we are past the end of the visible portion of the list..

		CellHeight = (DefaultCellHeight * List[DrawIndex].CurHeightMultiplier * ResScaling.Y);

		// Calculate the Bounds

    	List[DrawIndex].YBounds[0] = YPos;
    	List[DrawIndex].YBounds[1] = YPos + CellHeight;

		// Clear the rendered flag

    	List[DrawIndex].bWasRendered = false;

		// Render if we should.

		if ( YPos < WindowHeight )	// Check for the bottom edge clip
		{
			// Ok, we haven't gone past the window, so see if we are before it

			if (YPos + CellHeight > 0)
			{
				// Allow a delegate first crack at rendering, otherwise use the default
				// string rendered.

				if ( !OnDrawItem(self, DrawIndex, XPos, YPos) )
				{
					DrawItem(DrawIndex, XPos, YPos);
			    	List[DrawIndex].bWasRendered = true;
				}
			}
		}
		YPos += CellHeight;
	}

}

/**
 * Starts an item down the road to transition
 */
function SetTransition(int ItemIndex, float NewDestinationTransition)
{
	if (NewDestinationTransition != List[ItemIndex].CurHeightMultiplier )
	{
		List[ItemIndex].HeightMultipliers[0] = List[ItemIndex].CurHeightMultiplier;
		List[ItemIndex].HeightMultipliers[1] = NewDestinationTransition;
		List[ItemIndex].CellTransitiontime = TransitionTime;
	}
}


/**
 * Selects an item
 */

event SelectItem(int NewSelection)
{
	local int i;
	local int OldSelection;


	NewSelection = Clamp(NewSelection,0,List.Length-1);


	OldSelection = Selection;
	Selection = NewSelection;


	// Set the transition on the Selected Item
	SetTransition(Selection,SelectionCellHeightMultiplier);

	if ( Selection > 0 )
	{
		SetTransition(Selection-1, AboveBelowCellHeightMultiplier);
	}

	if ( Selection < List.Length - 1)
	{
		SetTransition(Selection+1, AboveBelowCellHeightMultiplier);
	}

	// Set all others to 1.0
	for (i=0;i <List.Length;i++)
	{
		if ( Abs(i-Selection) > 1 )
		{
			SetTransition(i,1.0);
		}
	}

	WindowTopTransitionTime = FClamp(WindowTopTransitionTime+0.15, 0.0, TransitionTime);

	if (Selection != OldSelection)
	{
		OnSelectionChange(self, Selection);
	}
}

/**
 * This delegate allows anyone to alter the drawing code of the list.
 *
 * @Returns true to skip the default drawing
 */
delegate bool OnDrawItem(UTSimpleList SimpleList, int ItemIndex, float XPos, out float YPos)
{
	return false;
}

/**
 * This delegate is called when an item in the list is chosen.
 */
delegate OnItemChosen(UTSimpleList SourceList, int SelectedIndex, int PlayerIndex);

/**
 * This delegate is called when the selection index changes
 */
delegate OnSelectionChange(UTSimpleList SourceList, int NewSelectedIndex);


/**
 * Draws an item to the screen.  NOTE this function can assume that the item
 * being drawn is not the selected item
 */

function DrawItem(int ItemIndex, float XPos, out float YPos)
{
	local float H;
	local vector2d DrawScale;

	DrawScale = ResScaling;
	DrawScale.X *= List[ItemIndex].CurHeightMultiplier;
	DrawScale.Y *= List[ItemIndex].CurHeightMultiplier;

	// Figure out the total height of this cell
	H = DefaultCellHeight * DrawScale.Y;

	// If we are the selected widget, draw the bar

	if ( ItemIndex == Selection )
	{
		DrawSelectionBar(YPos, H);
	}


	if ( List[ItemIndex].Text != "" )
	{

		if ( ItemIndex == Selection )
		{
			Canvas.DrawColor = SelectedColor;
		}
		else
		{
			Canvas.DrawColor = ( Abs(ItemIndex-Selection)>1 ? NormalColor : AboveBelowColor );
		}

		DrawStringToFit(List[ItemIndex].Text,XPos, YPos, YPos+H);
	}
}

/**
 * Draw the selection Bar
 */
function DrawSelectionBar(float YPos, float CellHeight)
{
	local float Width,Height;
	local float AWidth, AHeight, AYPos;
	local bool b;



	Height = DefaultCellHeight * SelectionCellHeightMultiplier * ResScaling.Y;
	Width = Height * ScrollWidthRatio;

	YPos = YPos + (CellHeight * 0.5) - (Height * 0.5);


	// Draw the Bar

	Canvas.SetPos(0,YPos);
	Canvas.DrawColor=SelectionBarColor;
	Canvas.DrawTile(Selectionimage, Width,Height, 149,363,106,86);
	Canvas.SetPos(Width,YPos);
	Canvas.DrawTile(SelectionImage, Canvas.ClipX-Width,Height, 652,375,372,49);

	// ------------ Draw the up/Down Arrows


	// Calculate the sizes
	AYPos = YPos - Height;
	AHeight = Height * 0.9;
	AWidth = AHeight * 0.5;

		// Draw The up button

	// Cache the bounds for mouse lookup later

	UpArrowBounds[0] = (Width * 0.5) - (AWidth * 0.5);
	UpArrowBounds[2] = UpArrowBounds[0] + AWidth;
	UpArrowBounds[1] = AYPos;
	UpArrowBounds[3] = AYPos + AHeight;

	b = CursorCheck(UpArrowBounds[0],UpArrowBounds[1],UpArrowBounds[2],UpArrowBounds[3]);
	DrawSpecial(UpArrowBounds[0],UpArrowBounds[1], AWidth, AHeight,77,198,63,126,SelectionBarColor,b);

		// Draw The down button

	// Cache the bounds for mouse lookup later

	AYPos = YPos + Height + (Height * 0.1);

	DownArrowBounds[0] = (Width * 0.5) - (AWidth * 0.5);
	DownArrowBounds[2] = DownArrowBounds[0] + AWidth;
	DownArrowBounds[1] = AYPos;
	DownArrowBounds[3] = AYPos + AHeight;

	b = CursorCheck(DownArrowBounds[0],DownArrowBounds[1],DownArrowBounds[2],DownArrowBounds[3]);
	DrawSpecial(DownArrowBounds[0],DownArrowBounds[1], AWidth, AHeight,77,358,63,126,SelectionBarColor,b);

}

function DrawSpecial(float x, float y, float w, float h, float u, float v, float ul, float vl, color DrawColor, bool bOver)
{
	if (bDragging || bOver)
	{
		Canvas.SetDrawColor(255,255,128,255);
		Canvas.SetPos(x-2,y-2);
		Canvas.DrawTileClipped(SelectionImage, w+4, h+4, u,v,ul,vl);
	}

	if ( !bDragging )
	{
		Canvas.SetPos(x,y);
		Canvas.DrawColor = DrawColor;
		Canvas.DrawTileClipped(SelectionImage, w, h, u, v, ul, vl);
	}
}


function DrawStringToFit(string StringToDraw, float XPos, float y1, float y2)
{
	local float xl,yl;
	local float H;
	local float TextScale;

	Canvas.StrLen(StringToDraw,XL,YL);

	H = Y2-Y1;

	TextScale = H / YL;
	Canvas.SetPos(XPos, y1 + (H*0.5) - (YL * TextScale * 0.5));
	Canvas.DrawTextClipped(StringToDraw,,TextScale,TextScale);
}

defaultproperties
{
	TextFont=MultiFont'UI_Fonts.MultiFonts.MF_HudLarge'
	DefaultCellHeight=23
	AboveBelowCellHeightMultiplier=1.5
	SelectionCellHeightMultiplier=2.0
    TransitionTime=0.25

//	NormalColor=(R=51,G=0,B=1,A=180)
//	AboveBelowColor=(R=51,G=0,B=1,A=255)
	NormalColor=(R=255,G=255,B=255,A=255)
	AboveBelowColor=(R=255,G=255,B=255,A=255)
	SelectedColor=(R=255,G=255,B=0,A=255)
	SelectionBarColor=(R=51,G=0,B=1,A=255)
	Selection=-1
	Top=0
	ScrollWidthRatio=1.29347;
	SelectionImage=Texture2D'UI_HUD.HUD.UI_HUD_BaseC'

}
