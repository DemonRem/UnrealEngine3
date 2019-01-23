/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTScoreboardPanel extends UTDrawPanel
	config(Game);

/** Defines the different font sizes */
enum EFontType
{
	EFT_Tiny,
	EFT_Small,
	EFT_Med,
	EFT_Large,
};

/**
 * Holds the font data.  We cache the max char height for quick lookup
 */
struct SBFontData
{
	var() font Font;
	var transient int CharHeight;

};

/** Font Data 0 = Tiny, 1=Small, 2=Med, 3=Large */
var() SBFontData Fonts[4];

/** If true, this scoreboard will be considered to be interactive */
var() bool bInteractive;

/** Holds a list of PRI's currently being worked on.  Note it cleared every frame */
var transient array<UTPlayerReplicationInfo> PRIList;

/** Cached reference to the HUDSceneOwner */
var UTUIScene_Hud UTHudSceneOwner;

var() transient int EditorTestNoPlayers;

var() float MainPerc;
var() float ClanTagPerc;
var() float MiscPerc;
var() float SpacerPerc;
var() float BarPerc;
var() Texture2D SelBarTex;
var() int AssociatedTeamIndex;
var() bool bDrawPlayerNum;
var() config string LeftMiscStr;
var() config string RightMiscStr;

var transient UTPlayerController PlayerOwner;

var transient int NameCnt;
var transient string FakeNames[32];

var transient color TeamColors[2];

/** The PRI of the currently selected player */
var transient UTPlayerReplicationInfo SelectedPRI;

/** We cache this so we don't have to resize everything for a mouse click */
var transient float LastCellHeight;

/**
 * A delegate that the scene can override to say if this is an important player or not.  If it is,
 * then a second delegate (DrawCriticialPlayer) is made
 *
 * @Returns true if this is a critical player

delegate bool IsCriticalPlayer(UTPlayerReplicationInfo PRI)
{
	return false;
}

delegate PreDrawCriticalPlayer(Canvas C, UTPlayerReplicationInfo PRI, float YPos, float CellHeight, UTScoreboardPanel Panel, int PIndex);
delegate DrawCriticalPlayer(Canvas C, UTPlayerReplicationInfo PRI, float YPos, float CellHeight, UTScoreboardPanel Panel, int PIndex);
 */


event PostInitialize()
{
	Super.PostInitialize();
	SizeFonts();

	UTHudSceneOwner = UTUIScene_Hud( GetScene() );
	NotifyResolutionChanged = OnNotifyResolutionChanged;

	if (bInteractive)
	{
		OnRawInputKey=None;
		OnProcessInputKey=ProcessInputKey;
	}
}

/**
 * Setup Input subscriptions
 */
event GetSupportedUIActionKeyNames(out array<Name> out_KeyNames )
{
	out_KeyNames[out_KeyNames.Length] = 'SelectionUp';
	out_KeyNames[out_KeyNames.Length] = 'SelectionDown';
	out_KeyNames[out_KeyNames.Length] = 'Select';

}


/**
 * Whenever there is a resolution change, make sure we recache the font sizes
 */
function OnNotifyResolutionChanged( const out Vector2D OldViewportsize, const out Vector2D NewViewportSize )
{
	SizeFonts();
}

function NotifyGameSessionEnded()
{
	SelectedPRI = none;
}


/**
 * Precache the sizing of the fonts so we don't have to constant look it up
 */
function SizeFonts()
{
	local int i;
	for (i = 0; i < ARRAYCOUNT(Fonts); i++)
	{
		if ( Fonts[i].Font != none )
		{
			Fonts[i].CharHeight = Fonts[i].Font.GetMaxCharHeight();
		}
	}
}

/**
 * Draw the Scoreboard
 */

event DrawPanel()
{
	local WorldInfo WI;
	local UTGameReplicationInfo GRI;
	local int i;
	local float YPos;
	local float CellHeight;

	/** Which font to use for clan tags */
	local int ClanTagFontIndex;

	/** Which font to use for the Misc line */
	local int MiscFontIndex;

	/** Which font to use for the main text */
	local int FontIndex;

	/** Finally, if we must, scale it */

	local float FontScale;

	WI = UTHudSceneOwner.GetWorldInfo();
	GRI = UTGameReplicationInfo(WI.GRI);

	// Grab the PawnOwner.  We will ditch this at the end of the draw
	// cycle to make sure there are no Object->Actor references laying around

	PlayerOwner = UTHudSceneOwner.GetUTPlayerOwner();

	// Figure out if we can fit everyone at the default font levels

	FontIndex = EFT_Large;

	if ( bInteractive )
	{
		ClanTagFontIndex = -1;
		MiscFontIndex = -1;
	}
	else
	{
		ClanTagFontIndex = EFT_Med;
		MiscFontIndex = EFT_Small;
	}

	FontScale = 1.0f;

	// Attempt to AutoFit the text.

	CellHeight = AutoFit(GRI, FontIndex, ClanTagFontIndex, MiscFontIndex, FontScale, true);
	LastCellHeight = CellHeight;

	// Draw each score.
	NameCnt=0;
	YPos = 0.0;
	for (i=0;i<PRIList.Length;i++)
	{
		// Draw the score

		DrawPRI(i, PRIList[i], CellHeight, FontIndex, ClanTagFontIndex, MiscFontIndex, FontScale, YPos);
		NameCnt++;
	}

	// Clear up Object->Actor references

	PlayerOwner = none;
	PRIList.Length = 0;

}

function CheckSelectedPRI()
{
	local int i;

	for (i=0;i<PRIList.Length;i++)
	{
		if ( PRIList[i] == SelectedPRI )
		{
			return;
		}
	}

	SelectedPRI = none;
}


/**
 * Figure a way to fit the data.  This will probably be specific to each game type
 *
 * @Param	GRI 				The Game ReplicationIfno
 * @Param	FontIndex			The Index to use for the main text
 * @Param 	ClanTagFontIndex	The Index to use for the Clan Tag
 * @Param	MiscFontIndex		The Index to use for the Misc tag
 * @Param	FontSCale			The final font scaling factor to use if all else fails
 * @Param	bPrimeList			Should only be true the first call.  Will build a list of
 *								who needs to be checked.
 */
function float AutoFit(UTGameReplicationInfo GRI, out int FontIndex,out int ClanTagFontIndex,
					out int MiscFontIndex, out float FontScale, bool bPrimeList)
{
	local int i;
	local UTPlayerReplicationInfo PRI;
	local float CellHeight;
	local bool bRecurse;

	// We need to prime our list, so do that first.

	if ( bPrimeList )
	{
		if ( UTHudSceneOwner.IsGame() )
		{
			// Scan the PRIArray and get any valid PRI's for display

			for (i=0; i < GRI.PRIArray.Length; i++)
			{
				PRI = UTPlayerReplicationInfo(GRI.PRIArray[i]);
				if ( PRI != none && IsValidScoreboardPlayer(PRI) )
				{
					PRIList[PRIList.Length] = PRI;
				}
			}

			if (bInteractive)
			{

				if (SelectedPRI != none )
				{
					CheckSelectedPRI();
				}

				if ( SelectedPRI == none )
				{
					SelectedPRI = PRIList[0];
					OnSelectionChange(self,SelectedPRI);
				}
			}
		}
		else
		{
			// Create Fake Entries for the editor

			PRIList.Length = EditorTestNoPlayers;
		}
	}

	// Calculate the Actual Cell Height given all the data

	CellHeight  = Fonts[FontIndex].CharHeight * MainPerc;
	CellHeight += ClanTagFontIndex >= 0 ? (Fonts[ClanTagFontIndex].CharHeight * ClanTagPerc) : 0.0f;
	CellHeight += MiscFontIndex >= 0 ? (Fonts[MiscFontIndex].CharHeight * MiscPerc) : 0.0f;
	CellHeight += CellHeight * SpacerPerc * FontScale;

	// Check to see if we fit
	if ( CellHeight * PRIList.Length > Canvas.ClipY )
	{
		bRecurse = false;		// By default, don't recurse


		// MiscFontIndex is the first to go
		if ( MiscFontIndex > 0 || (ClanTagFontIndex == 0 && MiscFontIndex == 0) )
		{
			MiscFontIndex--;
			bRecurse = true;
		}

		// Then the Clan Tag
		else if ( ClanTagFontIndex >= 0 )
		{
			ClanTagFontIndex--;
			bRecurse = true;
		}
		else
		{
			FontScale = Canvas.ClipY / (CellHeight * PRIList.Length);
		}

		// If we adjusted the ClanTag or Misc sizes, we need to retest the fit.
		if (bRecurse)
		{
			return AutoFit(GRI, FontIndex, ClanTagFontIndex, MiscFontIndex, FontScale, false);
		}

	}

	return CellHeight;

}


/**
 * Tests a PRI to see if we should display it on the scoreboard
 *
 * @Param PRI		The PRI to test
 * @returns TRUE if we should display it, returns FALSE if we shouldn't
 */
function bool IsValidScoreboardPlayer( UTPlayerReplicationInfo PRI)
{
	if ( AssociatedTeamIndex < 0 || PRI.GetTeamNum() == AssociatedTeamIndex )
	{
		return !PRI.bOnlySpectator;
	}

	return false;
}

/**
 * Draw any highlights.  These should render underneath the full width of the cell
 */
function DrawHighlight(UTPlayerReplicationInfo PRI, float YPos, float CellHeight, float FontScale)
{
	local float X;
	local UTPlayerController PC;

	PC = PRI != none ? UTPlayerController(PRI.Owner) : None;

	if ( (!bInteractive && PC != none && PC.Player != none && LocalPlayer(PC.Player) != none ) ||
		 ( bInteractive && PRI == SelectedPRI ) )
	{

		if ( !bInteractive || IsFocused() )
		{
			// Figure out where to draw the bar
			X = (Canvas.ClipX * 0.5) - (Canvas.ClipX * BarPerc * 0.5);
			Canvas.SetPos(X,YPos);
			Canvas.DrawTile(SelBarTex,Canvas.ClipX * BarPerc,CellHeight * FontScale,662,312,274,63);
		}
		Canvas.SetDrawColor(255,255,255,255);
	}
}

/**
 * Draw the player's clan tag.
 */
function DrawClanTag(UTPlayerReplicationInfo PRI, float X, out float YPos, int FontIndex, float FontScale)
{
	if ( FontIndex < 0 )
	{
		return;
	}

	// Draw the clan tag
	DrawString( GetClanTagStr(PRI),X, YPos, FontIndex, FontScale);
	YPos += Fonts[FontIndex].CharHeight * FontScale * ClanTagPerc;
}

/**
 * Draw the Player's Score
 */
function float DrawScore(UTPlayerReplicationInfo PRI, float YPos, int FontIndex, float FontScale)
{
	local string Spot;
	local float x, xl, bxl, yl;

	// Draw the player's Kills

	Spot = GetPlayerDeaths(PRI);
	StrLen("0000",XL,YL, FontIndex, 1.0);
	BXL = XL;
	StrLen(Spot,XL,YL,FontIndex,FontScale);
	X = Canvas.ClipX - BXL - 5 * FontScale;
	DrawString( Spot, X + (BXL *0.5) - XL * 0.5 , YPos,FontIndex,FontScale);

	// Draw the player's Frags

	Spot = GetPlayerScore(PRI);
	StrLen(Spot,XL,YL, FontIndex, FontScale);
	X = X - BXL - 30;
	DrawString( Spot, X + (BXL * 0.5) - XL * 0.5, YPos,FontIndex,FontScale);

	Strlen(" ",XL,YL,FontIndex, FontScale);
	return (X-XL);
}

/**
 * Draw's the player's Number (ie "1.")
 */
simulated function DrawPlayerNum(UTPlayerReplicationInfo PRI,int PIndex, out float YPos, float FontIndex, float FontScale)
{
	local string Spot;

	Spot = ""$PIndex+1$".";
	DrawString( Spot, 0, YPos, FontIndex, FontScale);
}

/**
 * Draw the Player's Name
 */
function DrawPlayerName(UTPlayerReplicationInfo PRI, float NameOfst, float NameClipX, out float YPos, int FontIndex, float FontScale, bool bIncludeClan)
{

	local float XL, YL;
	local string Spot;
	local float NameScale;

	Spot = bIncludeClan ? GetPlayerNameStr(PRI) : GetClanTagStr(PRI)$GetPlayerNameStr(PRI);
	StrLen(Spot, XL, YL, FontIndex, FontScale);

	YL = Fonts[FontIndex].CharHeight * FontScale;

	if ( XL > (NameClipX - NameOfst) )
	{
		NameScale = (NameClipX - NameOfst) / XL;
	}
	else
	{
		NameScale = 1.0;
	}

	DrawString( Spot, NameOfst, YPos + ( (YL - (YL * NameScale)) / 2  ) , FontIndex, FontScale * NameScale);

	YPos += YL * MainPerc; // Cheat because of possible scaling
}

/**
 * Draw any Misc data
 */
function DrawMisc(UTPlayerReplicationInfo PRI, float NameOfst, out float YPos, int FontIndex, float FontScale)
{
	local string Spot;
	local float XL,YL;

	// Draw the Misc Strings

	if ( FontIndex < 0 )
	{
		return;
	}


	DrawString( GetLeftMisc(PRI), NameOfst, YPos, FontIndex, FontScale);
	Spot = GetRightMisc(PRI);
	StrLen(Spot,XL,YL, FontIndex, FontScale);
	DrawString( Spot, Canvas.ClipX-XL-5, YPos, FontIndex,FontScale);
	YPos += Fonts[FontIndex].CharHeight * FontScale * MiscPerc;

}


/**
 * Draw an full cell.. Call the functions above.
 */
function DrawPRI(int PIndex, UTPlayerReplicationInfo PRI, float CellHeight, int FontIndex, int ClanTagFontIndex, int MiscFontIndex, float FontScale, out float YPos)
{
	local float XL,YL;
	local float NameOfst, NameClipX;

	// Set the default Drawing Color

	Canvas.DrawColor = ( PRI != none && AssociatedTeamIndex == 1 ) ? TeamColors[1] : TeamColors[0];
	DrawHighlight(PRI, YPos, CellHeight, FontScale);


	// Figure out if we are drawing the ##. prefix

	if ( bDrawPlayerNum )
	{
		// Figure out how big the #s are

		StrLen("00.",XL,YL, FontIndex, FontScale);
		NameOfst = XL;
	}

	DrawClanTag(PRI, NameOfst, YPos, ClanTagFontIndex, FontScale);

	// Draw the player's Score so we can see how much room we have to draw the name

	NameClipX = DrawScore(PRI, YPos, FontIndex, FontScale);


	// Draw the Player's Name and position on the team - NOTE it doesn't increment YPos

	if (bDrawPlayerNum)
	{
		DrawPlayerNum(PRI, PIndex, YPos, FontIndex, FontScale);
	}

	DrawPlayerName(PRI, NameOfst, NameClipX, YPos, FontIndex, FontScale, (ClanTagFontIndex >= 0));
	DrawMisc(PRI, NameOfst, YPos, MiscFontIndex, FontScale);

	// Draw the space in between

	YPos += CellHeight * SpacerPerc * FontScale;
}

/**
 * Returns the Clan Tag in the PRI
 */
function string GetClanTagStr(UTPlayerReplicationInfo PRI)
{
	if ( PRI != none )
	{
		return PRI.ClanTag != "" ? ("["$PRI.ClanTag$"]") : "";
	}
	else
	{
		return "[Clan]";
	}
}

/**
 * Returns the Player's Name
 */
function string GetPlayerNameStr(UTPlayerReplicationInfo PRI)
{
	if ( PRI != None )
	{
		return PRI.PlayerName;
	}
	else
	{
		return FakeNames[NameCnt];
	}
}

/**
 * Returns the # of deaths as a string
 */
function string GetPlayerDeaths(UTPlayerReplicationInfo PRI)
{
	if ( PRI != None )
	{
		return string(int(PRI.Deaths));
	}
	else
	{
		return "0000";
	}
}

/**
 * Returns the score as a string
 */
function string GetPlayerScore(UTPlayerReplicationInfo PRI)
{
	if ( PRI != None )
	{
		return string(int(PRI.Score));
	}
	else
	{
		return "0000";
	}
}

/**
 * Returns the time online as a string
 */
function string GetTimeOnline(UTPlayerReplicationInfo PRI)
{
	return "Time:"@ class'UTHUD'.static.FormatTime( PRI.WorldInfo.GRI.ElapsedTime );
}

/**
 * Get the Left Misc string
 */
function string GetLeftMisc(UTPlayerReplicationInfo PRI)
{
	if ( PRI != None )
	{
		if (LeftMiscStr != "")
		{
			return UserString(LeftMiscStr, PRI);
		}
		else
		{
			return "";
		}
	}
	return "LMisc";
}
/**
 * Get the Right Misc string
 */

function string GetRightMisc(UTPlayerReplicationInfo PRI)
{
	local float FPH;
	if ( PRI != None )
	{
		if (RightMiscStr != "")
		{
			return UserString(RightMiscStr,PRI);
		}
		else
		{
			FPH = Clamp(3600*PRI.Score/FMax(1,PRI.WorldInfo.GRI.ElapsedTime - PRI.StartTime),-999,9999);
			return "FPH:"@FPH;
		}
	}
	return "RMisc";
}

/**
 * Does string replace on the user string for several values
 */

function string UserString(string Template, UTPlayerReplicationInfo PRI)
{
	// TO-DO - Hook up the various values
	return Template;
}

/**
 * Our own implementation of DrawString that manages font lookup and scaling
 */
function float DrawString(String Text, float XPos, float YPos, int FontIdx, float FontScale)
{
	if (FontIdx >= 0 && Text != "")
	{
		Canvas.Font = Fonts[FontIdx].Font;
		Canvas.SetPos(XPos, YPos);
		Canvas.DrawText(Text,,FontScale,FontScale);
		return Fonts[FontIdx].CharHeight* FontScale;
	}
	return 0;
}

/**
 * Our own version of StrLen that manages font lookup and scaling
 */
function StrLen(String Text, out float XL, out float YL, int FontIdx, float FontScale)
{
	if (FontIdx >= 0 && Text != "")
	{
		Canvas.Font = Fonts[FontIdx].Font;
		Canvas.StrLen(Text, xl,yl);
		xl *= FontScale;
		yl *= FontScale;
	}
	else
	{
		xl = 0;
		yl = 0;
	}
}

/*********************************[ InteractiveMode ]*****************************/

function ClearSelection()
{
	SelectedPRI = none;
}


function ChangeSelection(int Ofst)
{
	local UTGameReplicationInfo GRI;
	local array<UTPlayerReplicationInfo> PRIs;
	local UTPlayerReplicationInfo PRI;
	local int i,idx;

	GRI = UTGameReplicationInfo(UTHudSceneOwner.GetWorldInfo().GRI);

	for (i=0; i < GRI.PRIArray.Length; i++)
	{
		PRI = UTPlayerReplicationInfo(GRI.PRIArray[i]);
		if ( PRI != none && IsValidScoreboardPlayer(PRI) )
		{
			if (PRI == SelectedPRI)
			{
				idx = PRIs.Length;
			}
			PRIs[PRIs.Length] = PRI;
		}
	}

	if ( (ofst>0 && idx < PRIs.Length-1) || (ofst<0 && idx >0) )
	{
		SelectedPRI = PRIs[Idx+Ofst];
		OnSelectionChange(self,SelectedPRI);
	}
}

delegate OnSelectionChange(UTScoreboardPanel TargetScoreboard, UTPlayerReplicationInfo PRI);

function SelectUnderCursor()
{
	local UTGameReplicationInfo GRI;
	local Vector CursorVector;
	local int X,Y, Item, c, i;
	local UTPlayerReplicationInfo PRI;

	class'UIRoot'.static.GetCursorPosition( X, Y );

	// We can't use OrgX/OrgY here because they won't exist outside of the render loop

	CursorVector.X = X - GetPosition(UIFACE_Left, EVALPOS_PixelViewport, true);
	CursorVector.Y = Y - GetPosition(UIFACE_Top, EVALPOS_PixelViewport, true);

	// Attampt to figure out


	Item = int( CursorVector.Y / LastCellHeight);

	GRI = UTGameReplicationInfo(UTHudSceneOwner.GetWorldInfo().GRI);
	for (i=0; i < GRI.PRIArray.Length; i++)
	{
		PRI = UTPlayerReplicationInfo(GRI.PRIArray[i]);
		if ( PRI != none && IsValidScoreboardPlayer(PRI) )
		{
			if (c == Item)
			{
				SelectedPRI = PRI;
				OnSelectionChange(self,SelectedPRI);
				return;
			}
			c++;
		}
	}
}

function bool ProcessInputKey( const out SubscribedInputEventParameters EventParms )
{
	if (EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat)
	{
		if (EventParms.InputAliasName == 'SelectionUp')
		{
		 	ChangeSelection(-1);
		 	return true;
		}
		else if (EventParms.InputAliasName == 'SelectionDown')
		{
			ChangeSelection(1);
			return true;
		}
		else if (EventParms.InputAliasName == 'Select')
		{
			SelectUnderCursor();
			return true;
		}
	}


    return false;
}

defaultproperties
{
	Fonts(0)=(Font=Font'EngineFonts.SmallFont');
		Fonts(1)=(Font=Font'UI_Fonts.Fonts.UI_Fonts_Positec14');
	Fonts(2)=(Font=Font'UI_Fonts.Fonts.UI_Fonts_Positec18');
	Fonts(3)=(Font=Font'UI_Fonts.Fonts.UI_Fonts_Positec36');

	ClanTagPerc=0.7
	MiscPerc=1.1
	MainPerc=0.95
	SelBarTex=Texture2D'UI_HUD.HUD.UI_HUD_BaseC'
	AssociatedTeamIndex=-1
	BarPerc=1.2
	bDrawPlayerNum=true

	FakeNames(0)="WWWWWWWWWWWWWWW"
	FakeNames(1)="DrSiN"
	FakeNames(2)="Mysterial"
	FakeNames(3)="Reaper"
	FakeNames(4)="ThomasDaTank"
	FakeNames(5)="Luke Skywalker"
	FakeNames(6)="Indy"
	FakeNames(7)="UTBabe"
	FakeNames(8)="Mulder"
	FakeNames(9)="Starbuck"
	FakeNames(10)="Scully"
	FakeNames(11)="Starbuck"
	FakeNames(12)="Quiet Riot"
	FakeNames(13)="BonusPoint"
	FakeNames(14)="Gripper"
	FakeNames(15)="Midnight"
	FakeNames(16)="too damn tired"
	FakeNames(17)="Spiff"
	FakeNames(18)="Mr. Sckum"
	FakeNames(19)="SkummyBoy"
	FakeNames(20)="DrSiN"
	FakeNames(21)="Mysterial"
	FakeNames(22)="Reaper"
	FakeNames(23)="Mr.PooPoo"
	FakeNames(24)="ThomasDaTank"
	FakeNames(25)="Luke Skywalker"
	FakeNames(26)="Indy"
	FakeNames(27)="UTBabe"
	FakeNames(28)="Mulder"
	FakeNames(29)="Scully"
	FakeNames(30)="Screwy"
	FakeNames(31)="Starbuck"

    TeamColors(0)=(R=51,G=0,B=0,A=255)
    TeamColors(1)=(R=0,G=0,B=51,A=255)

	DefaultStates.Add(class'Engine.UIState_Active')
	DefaultStates.Add(class'Engine.UIState_Focused')
}

