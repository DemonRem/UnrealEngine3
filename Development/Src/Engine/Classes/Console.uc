//=============================================================================
// Console - A quick little command line console that accepts most commands.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class Console extends Interaction
	within GameViewportClient
	transient
	config(Input)
	native(UserInterface);

// Constants.
const MaxHistory=16;		// # of command history to remember.

/** The player which the next console command should be executed in the context of.  If NULL, execute in the viewport. */
var LocalPlayer ConsoleTargetPlayer;

var	UIScene			LargeConsoleScene, MiniConsoleScene;
var	UILabel			ConsoleBufferText;
var ConsoleEntry	MiniConsoleInput;
var ConsoleEntry	LargeConsoleInput;

var Texture2D		DefaultTexture_Black;
var Texture2D		DefaultTexture_White;

/** The key which opens the console. */
var globalconfig name ConsoleKey;

/** The key which opens the typing bar. */
var globalconfig name TypeKey;

/**  Visible Console stuff */
var globalconfig int 	MaxScrollbackSize;

/** Holds the scrollback buffer */
var array<string> 		Scrollback;

/**  Where in the scrollback buffer are we */
var int 				SBHead, SBPos;

/** index into the History array for the latest command that was entered */
var config int HistoryTop;

/** index into the History array for the earliest command that was entered */
var config int HistoryBot;

/** the index of the current position in the History array */
var config int HistoryCur;

/** tracks previously entered console commands */
var config string History[MaxHistory];

/** The command the user is currently typing. */
var string TypedStr;

var int TypedStrPos;						//Current position in TypedStr
var bool bIgnoreKeys;						// Ignore Key presses until a new KeyDown is received

/** True while a control key is pressed. */
var bool bCtrl;

var config bool bEnableUI;

/**
 * Called when the Console is added to the GameViewportClient's Interactions array.
 */
function Initialized()
{
	Super.Initialized();

	if (bEnableUI)
	{
		LargeConsoleScene = UIScene(DynamicLoadObject("EngineScenes.ConsoleScene", class'Engine.UIScene'));
		MiniConsoleScene = UIScene(DynamicLoadObject("EngineScenes.MiniConsole", class'Engine.UIScene'));

		UIController.SceneClient.InitializeScene(LargeConsoleScene,None,LargeConsoleScene);
		UIController.SceneClient.InitializeScene(MiniConsoleScene,None,MiniConsoleScene);

		LargeConsoleInput = ConsoleEntry(LargeConsoleScene.FindChild('CommandRegion'));
		MiniConsoleInput = ConsoleEntry(MiniConsoleScene.FindChild('CommandRegion'));
		ConsoleBufferText = UILabel(LargeConsoleScene.FindChild('BufferText'));
	}
}

function SetInputText( string Text )
{
	TypedStr = Text;
	if ( bEnableUI )
	{
		MiniConsoleInput.SetValue(Text);
		LargeConsoleInput.SetValue(Text);
	}
}

function SetCursorPos( int Position )
{
	TypedStrPos = Position;
	if ( bEnableUI )
	{
		MiniConsoleInput.CursorPosition = Position;
		LargeConsoleInput.CursorPosition = Position;
	}
}

/**
* Searches console command history and removes any entries matching the specified command.
* @param Command - The command to search for and purge from the history.
*/
function PurgeCommandFromHistory(string Command)
{
	local int HistoryIdx, Idx, NextIdx;

	if ( (HistoryTop >= 0) && (HistoryTop < MaxHistory) )
	{
		for (HistoryIdx=0; HistoryIdx<MaxHistory; ++HistoryIdx)
		{
			if (History[HistoryIdx] ~= Command)
			{
				// from here to the top idx, shift everything back one
				Idx = HistoryIdx;
				NextIdx = (HistoryIdx + 1) % MaxHistory;

				while (Idx != HistoryTop)
				{
					History[Idx] = History[NextIdx];
					Idx = NextIdx;
					NextIdx = (NextIdx + 1) % MaxHistory;
				}

				// top index backs up one as well
				HistoryTop = (HistoryTop == 0) ? (MaxHistory - 1) : (HistoryTop - 1);
			}
		}
	}
}

/**
 * Executes a console command.
 * @param Command - The command to execute.
 */
function ConsoleCommand(string Command)
{
	// Store the command in the console history.
	if ((HistoryTop == 0) ? !(History[MaxHistory - 1] ~= Command) : !(History[HistoryTop - 1] ~= Command))
	{
		// ensure uniqueness
		PurgeCommandFromHistory(Command);

		History[HistoryTop] = Command;
		HistoryTop = (HistoryTop+1) % MaxHistory;

		if ( ( HistoryBot == -1) || ( HistoryBot == HistoryTop ) )
			HistoryBot = (HistoryBot+1) % MaxHistory;
	}
	HistoryCur = HistoryTop;

	// @RON you need to fix this in a better way

	if (Left(Command,4) ~= "Say ")
	{
		Command = "Say \""$Right(Command,Len(Command)-4)$"\"";
	}

	if (Left(Command,8) ~= "TeamSay ")
	{
		Command = "TeamSay \""$Right(Command,Len(Command)-8)$"\"";
	}

	// Save the command history to the INI.
	SaveConfig();

	if ( bEnableUI )
	{
		// must escape these special characters if using the UI
		OutputText("\n\\>\\>\\>" @ Command @ "\\<\\<\\<");
	}
	else
	{
		OutputText("\n>>>" @ Command @ "<<<");
	}

	if(ConsoleTargetPlayer != None)
	{
		// If there is a console target player, execute the command in the player's context.
		ConsoleTargetPlayer.Actor.ConsoleCommand(Command);
	}
	else if(GamePlayers.Length > 0 && GamePlayers[0].Actor != None)
	{
		// If there are any players, execute the command in the first players context.
		GamePlayers[0].Actor.ConsoleCommand(Command);
	}
	else
	{
		// Otherwise, execute the command in the context of the viewport.
		Outer.ConsoleCommand(Command);
	}
}

/**
 * Clears the console output buffer.
 */
function ClearOutput()
{
	SBHead = 0;
	ScrollBack.Remove(0,ScrollBack.Length);

	if ( bEnableUI )
	{
		ConsoleBufferText.SetValue("");
	}
}

/**
 * Prints a single line of text to the console.
 * @param Text - A line of text to display on the console.
 */
function OutputTextLine(coerce string Text)
{
	// If we are full, delete the first line
	if (ScrollBack.Length > MaxScrollbackSize)
	{
		ScrollBack.Remove(0,1);
		SBHead = MaxScrollBackSize-1;
	}
	else
		SBHead++;

	// Add the line
	ScrollBack.Length = ScrollBack.Length+1;
	ScrollBack[SBHead] = Text;

	if ( bEnableUI && ConsoleBufferText != None )
	{
		// add the scrollback to the scene version
		ConsoleBufferText.SetArrayValue(ScrollBack);
	}
}

/**
 * Prints a (potentially multi-line) string of text to the console.
 * The text is split into separate lines and passed to OutputTextLine.
 * @param Text - Text to display on the console.
 */
event OutputText(coerce string Text)
{
	local string RemainingText;
	local int StringLength;
	local int LineLength;

	RemainingText = Text;
	StringLength = Len(Text);
	while(StringLength > 0)
	{
		// Find the number of characters in the next line of text.
		LineLength = InStr(RemainingText,"\n");
		if(LineLength == -1)
		{
			// There aren't any more newlines in the string, assume there's a newline at the end of the string.
			LineLength = StringLength;
		}

		// Output the line to the console.
		OutputTextLine(Left(RemainingText,LineLength));

		// Remove the line from the string.
		RemainingText = Mid(RemainingText,LineLength + 1);
		StringLength -= LineLength + 1;
	};
}

/**
 * Opens the typing bar with text already entered.
 * @param Text - The text to enter in the typing bar.
 */
function StartTyping(coerce string Text)
{
	GotoState('Typing');
	SetInputText(Text);
	SetCursorPos(Len(Text));
}

function PostRender_Console(Canvas Canvas);

/**
 * Process an input key event routed through unrealscript from another object. This method is assigned as the value for the
 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
 *
 * @param	ControllerId	the controller that generated this input key event
 * @param	Key				the name of the key which an event occured for (KEY_Up, KEY_Down, etc.)
 * @param	EventType		the type of event which occured (pressed, released, etc.)
 * @param	AmountDepressed	for analog keys, the depression percent.
 *
 * @return	true to consume the key event, false to pass it on.
 */
function bool InputKey( int ControllerId, name Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = FALSE )
{
	if(Key == ConsoleKey && Event == IE_Pressed)
	{
		GotoState('Open');
		return true;
	}
	else if(Key == TypeKey && Event == IE_Pressed)
	{
		GotoState('Typing');
		return true;
	}

	return false;
}

/**
 * Process a character input event (typing) routed through unrealscript from another object. This method is assigned as the value for the
 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
 *
 * @param	ControllerId	the controller that generated this character input event
 * @param	Unicode			the character that was typed
 *
 * @return	True to consume the character, false to pass it on.
 */
function bool InputChar( int ControllerId, string Unicode )
{
	return false;
}

function bool IsUIConsoleOpen()
{
	return bEnableUI && UIController.SceneClient.ActiveScenes.Find(LargeConsoleScene) != INDEX_NONE;
}

function bool IsUIMiniConsoleOpen()
{
	return bEnableUI && UIController.SceneClient.ActiveScenes.Find(MiniConsoleScene) != INDEX_NONE;
}

/**
 * This state is used when the typing bar is open.
 */
state Typing
{
	/**
	 * Process a character input event (typing) routed through unrealscript from another object. This method is assigned as the value for the
	 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
	 *
	 * @param	ControllerId	the controller that generated this character input event
	 * @param	Unicode			the character that was typed
	 *
	 * @return	True to consume the character, false to pass it on.
	 */
	function bool InputChar( int ControllerId, string Unicode )
	{
		local int	Character;

		if ( IsUIMiniConsoleOpen() )
		{
			return false;
		}

		if ( bIgnoreKeys )
		{
			return true;
		}

		while(Len(Unicode) > 0)
		{
			Character = Asc(Left(Unicode,1));
			Unicode = Mid(Unicode,1);

			if(Character >= 0x20 && Character < 0x100 && Character != Asc("~") && Character != Asc("`"))
			{
				SetInputText(Left(TypedStr, TypedStrPos) $ Chr(Character) $ Right(TypedStr, Len(TypedStr) - TypedStrPos));
				SetCursorPos(TypedStrPos + 1);
			}
		};

		return true;
	}

	/**
	 * Process an input key event routed through unrealscript from another object. This method is assigned as the value for the
	 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
	 *
	 * @param	ControllerId	the controller that generated this input key event
	 * @param	Key				the name of the key which an event occured for (KEY_Up, KEY_Down, etc.)
	 * @param	EventType		the type of event which occured (pressed, released, etc.)
	 * @param	AmountDepressed	for analog keys, the depression percent.
	 *
	 * @return	true to consume the key event, false to pass it on.
	 */
	function bool InputKey( int ControllerId, name Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = FALSE )
	{
		local string Temp;

		if (Event == IE_Pressed)
		{
			bIgnoreKeys=false;
		}

		if( Key == 'Escape' && Event == IE_Released )
		{
			if( TypedStr!="" )
			{
				SetInputText("");
				SetCursorPos(0);
				HistoryCur = HistoryTop;
				return true;
			}
			else
			{
				GotoState( '' );
			}
			return true;
		}
		else if( Key==ConsoleKey && Event == IE_Pressed )
		{
			GotoState('Open');
			return true;
		}
		else if(Key == TypeKey && Event == IE_Pressed)
		{
			GotoState('');
			return true;
		}
		else if( Key=='Enter' && Event == IE_Released )
		{
			if( TypedStr!="" )
			{
				// Make a local copy of the string.
				Temp=TypedStr;

				SetInputText("");
				SetCursorPos(0);

				ConsoleCommand(Temp);

				//OutputText( Localize("Errors","Exec","Core") );

				OutputText( "" );
				GotoState('');
			}
			else
			{
				GotoState('');
			}

			return true;
		}
		else if( Global.InputKey( ControllerId, Key, Event, AmountDepressed, bGamepad ) )
		{
			return true;
		}
		else if( Event != IE_Pressed && Event != IE_Repeat )
		{
			return false;
		}
		else if( Key=='up' )
		{
			if ( HistoryBot >= 0 )
			{
				if (HistoryCur == HistoryBot)
					HistoryCur = HistoryTop;
				else
				{
					HistoryCur--;
					if (HistoryCur<0)
						HistoryCur = MaxHistory-1;
				}

				SetInputText(History[HistoryCur]);
				SetCursorPos(Len(History[HistoryCur]));
			}
			return True;
		}
		else if( Key=='down' )
		{
			if ( HistoryBot >= 0 )
			{
				if (HistoryCur == HistoryTop)
					HistoryCur = HistoryBot;
				else
					HistoryCur = (HistoryCur+1) % MaxHistory;

				SetInputText(History[HistoryCur]);
				SetCursorPos(Len(History[HistoryCur]));
			}

		}
		else if( Key=='backspace' )
		{
			if( TypedStrPos>0 )
			{
				SetInputText(Left(TypedStr,TypedStrPos-1) $ Right(TypedStr, Len(TypedStr) - TypedStrPos));
				SetCursorPos(TypedStrPos-1);
			}

			return true;
		}
		else if ( Key=='delete' )
		{
			if ( TypedStrPos < Len(TypedStr) )
			{
				SetInputText(Left(TypedStr,TypedStrPos) $ Right(TypedStr, Len(TypedStr) - TypedStrPos - 1));
			}
			return true;
		}
		else if ( Key=='left' )
		{
			SetCursorPos(Max(0, TypedStrPos - 1));
			return true;
		}
		else if ( Key=='right' )
		{
			SetCursorPos(Min(Len(TypedStr), TypedStrPos + 1));
			return true;
		}
		else if ( Key=='home' )
		{
			SetCursorPos(0);
			return true;
		}
		else if ( Key=='end' )
		{
			SetCursorPos(Len(TypedStr));
			return true;
		}

		return true;
	}

	event PostRender_Console(Canvas Canvas)
	{
		local float xl,yl;
		local string OutStr;
		local float ClipX;
		local float ClipY;
		local float LeftPos;

		if ( !IsUIMiniConsoleOpen() )
		{
			Global.PostRender_Console(Canvas);

			// Blank out a space

			// use the smallest font
			Canvas.Font	 = class'Engine'.Static.GetSmallFont();
			// determine the position for the cursor
			OutStr = "(>"@TypedStr;
			Canvas.Strlen(OutStr,xl,yl);

			ClipX = Canvas.ClipX;
			ClipY = Canvas.ClipY;
			LeftPos = 0;

			if (Class'WorldInfo'.Static.IsConsoleBuild())
			{
				ClipX	-= 32;
				ClipY	-= 32;
				LeftPos	 = 32;
			}

			// start at the bottom of the screen, then come up 6 pixels more than the height of the font
			Canvas.SetPos(LeftPos,ClipY-6-yl);
			// draw the background texture
			Canvas.DrawTile( DefaultTexture_Black, ClipX, yl+6,0,0,32,32);

			Canvas.SetPos(LeftPos,ClipY-6-yl);

			// change the draw color to green
			Canvas.SetDrawColor(0,255,0);

			// draw the top border of the typing region
			Canvas.DrawTile( DefaultTexture_White, ClipX, 2,0,0,32,32);

			// center the text between the bottom of the screen and the bottom of the border line
			Canvas.SetPos(LeftPos,ClipY-3-yl);
			Canvas.bCenter = False;
			Canvas.DrawText( OutStr, false );

			// determine the cursor position
			OutStr = "(>"@Left(TypedStr,TypedStrPos);
			Canvas.StrLen(OutStr,xl,yl);

			// move the pen to that position
			Canvas.SetPos(LeftPos + xl,ClipY-1-yl);

			// draw the cursor
			Canvas.DrawText("_");
		}
	}

	event BeginState(Name PreviousStateName)
	{
		if ( bEnableUI && MiniConsoleScene != None )
		{
			UIController.OpenScene(MiniConsoleScene);
		}
		bIgnoreKeys = true;
		HistoryCur = HistoryTop;
	}

	event EndState( Name NextStateName )
	{
		if ( MiniConsoleScene != None )
		{
			UIController.CloseScene(MiniConsoleScene);
		}
	}
}

/**
 * This state is used when the console is open.
 */
state Open
{
	function bool InputChar( int ControllerId, string Unicode )
	{
		local int	Character;

		if (bIgnoreKeys)
			return true;

		while(Len(Unicode) > 0)
		{
			Character = Asc(Left(Unicode,1));
			Unicode = Mid(Unicode,1);

			if(Character >= 0x20 && Character < 0x100 && Character != Asc("~") && Character != Asc("`"))
			{
				SetInputText(Left(TypedStr, TypedStrPos) $ Chr(Character) $ Right(TypedStr, Len(TypedStr) - TypedStrPos));
				SetCursorPos(TypedStrPos+1);
			}
		};

		return true;
	}

	/**
	 * Process an input key event routed through unrealscript from another object. This method is assigned as the value for the
	 * OnRecievedNativeInputKey delegate so that native input events are routed to this unrealscript function.
	 *
	 * @param	ControllerId	the controller that generated this input key event
	 * @param	Key				the name of the key which an event occured for (KEY_Up, KEY_Down, etc.)
	 * @param	EventType		the type of event which occured (pressed, released, etc.)
	 * @param	AmountDepressed	for analog keys, the depression percent.
	 *
	 * @return	true to consume the key event, false to pass it on.
	 */
	function bool InputKey( int ControllerId, name Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad = FALSE )
	{
		local string Temp;

		if (Event == IE_Pressed)
		{
			bIgnoreKeys=false;
		}

		if( Key == 'LeftControl' )
		{
			if (Event==IE_Released)
				bCtrl=false;
			else if (Event==IE_Pressed)
				bCtrl=True;

			return true;
		}

		if( Key == 'Escape' && Event == IE_Released )
		{
			if( TypedStr!="" )
			{
				SetInputText("");
				SetCursorPos(0);
				HistoryCur = HistoryTop;
				return true;
			}
			else
			{
				GotoState( '' );
			}
		}
		else if( Key==ConsoleKey && Event == IE_Pressed )
		{
			GotoState('');
			return true;
		}
		else if(Key == TypeKey && Event == IE_Pressed)
		{
			GotoState('Typing');
			return true;
		}
		else if( Key=='Enter' && Event == IE_Released )
		{
			if( TypedStr!="" )
			{
				// Make a local copy of the string.
				Temp=TypedStr;
				SetInputText("");
				SetCursorPos(0);

				if (Temp~="cls")
				{
					ClearOutput();
				}
				else
				{
					ConsoleCommand(Temp);
				}
			}

			return true;
		}
		else if( Global.InputKey( ControllerId, Key, Event, AmountDepressed, bGamepad ) )
		{
			return true;
		}
		else if( Event != IE_Pressed && Event != IE_Repeat )
		{
			return false;
		}
		else if( Key=='up' )
		{

			if (!bCtrl)
			{

				if ( HistoryBot >= 0 )
				{
					if (HistoryCur == HistoryBot)
						HistoryCur = HistoryTop;
					else
					{
						HistoryCur--;
						if (HistoryCur<0)
							HistoryCur = MaxHistory-1;
					}

					SetInputText(History[HistoryCur]);
					SetCursorPos(Len(History[HistoryCur]));
				}
			}
			else
			{
				if (SBPos<ScrollBack.Length-1)
				{
					SBPos++;

					if (SBPos>=ScrollBack.Length)
					  SBPos = ScrollBack.Length-1;
				}
			}
			return True;
		}
		else if( Key=='down' )
		{
			if (!bCtrl)
			{
				if ( HistoryBot >= 0 )
				{
					if (HistoryCur == HistoryTop)
						HistoryCur = HistoryBot;
					else
						HistoryCur = (HistoryCur+1) % MaxHistory;

					SetInputText(History[HistoryCur]);
					SetCursorPos(Len(History[HistoryCur]));
				}
			}
			else
			{
				if (SBPos>0)
				{
					SBPos--;
					if (SBPos<0)
						SBPos = 0;
				}

			}
			return true;
		}
		else if( Key=='backspace' )
		{
			if( TypedStrPos>0 )
			{
				SetInputText(Left(TypedStr,TypedStrPos-1) $ Right(TypedStr, Len(TypedStr) - TypedStrPos));
				SetCursorPos(TypedStrPos-1);
			}

			return true;
		}
		else if ( Key=='delete' )
		{
			if ( TypedStrPos < Len(TypedStr) )
			{
				SetInputText(Left(TypedStr,TypedStrPos) $ Right(TypedStr, Len(TypedStr) - TypedStrPos - 1));
			}
			return true;
		}
		else if ( Key=='left' )
		{
			SetCursorPos(Max(0, TypedStrPos - 1));
			return true;
		}
		else if ( Key=='right' )
		{
			SetCursorPos(Min(Len(TypedStr), TypedStrPos + 1));
			return true;
		}
		else if (bCtrl && Key=='home')
		{
			SBPos=0;
		}
		else if ( Key=='home' )
		{
			SetCursorPos(0);
			return true;
		}
		else if (bCtrl && Key=='end')
		{
			SBPos = ScrollBack.Length-1;
		}
		else if ( Key=='end' )
		{
			SetCursorPos(Len(TypedStr));
			return true;
		}

		else if ( Key=='pageup' || Key=='mousescrollup')
		{
			if (SBPos<ScrollBack.Length-1)
			{
				if (bCtrl)
					SBPos+=5;
				else
					SBPos++;

				if (SBPos>=ScrollBack.Length)
				  SBPos = ScrollBack.Length-1;
			}

			return true;
		}
		else if ( Key=='pagedown' || Key=='mousescrolldown')
		{
			if (SBPos>0)
			{
				if (bCtrl)
					SBPos-=5;
				else
					SBPos--;

				if (SBPos<0)
					SBPos = 0;
			}

			return true;
		}


		return true;
	}

	event PostRender_Console(Canvas Canvas)
	{

		local float Height;
		local float xl,yl,y;
		local string OutStr;
		local int idx;

		// render the buffer

		// Blank out a space
		if ( !IsUIConsoleOpen() )
		{
			Canvas.Font	 = class'Engine'.Static.GetSmallFont();

			// the height of the buffer will be 75% of the height of the screen
			Height = Canvas.ClipY*0.75;

			// change the draw color to white
	        Canvas.SetDrawColor(255,255,255,255);

	        // move the pen to the top-left pixel
			Canvas.SetPos(0,0);

			// draw the black background tile
			Canvas.DrawTile( DefaultTexture_Black, Canvas.ClipX, Height,0,0,32,32);

			// now render the typing region
			OutStr = "(>"@TypedStr;

			// determine the height of the text
			Canvas.Strlen(OutStr,xl,yl);

			// move the pen up + 12 pixels of buffer (for the green borders and some space)
			Canvas.SetPos(0,Height-12-yl);

			// change the draw color to green
			Canvas.SetDrawColor(0,255,0);

			// draw the top typing region border
			Canvas.DrawTile( DefaultTexture_White, Canvas.ClipX, 2,0,0,32,32);

			// move the pen to the bottom of the console buffer area
			Canvas.SetPos(0,Height);

			// draw the bottom typing region border
			Canvas.DrawTile( DefaultTexture_White, Canvas.ClipX, 2,0,0,32,32);

			// center the pen between the two borders
			Canvas.SetPos(0,Height-5-yl);
			Canvas.bCenter = False;

			// render the text that is being typed
			Canvas.DrawText( OutStr, false );

			OutStr = "(>"@Left(TypedStr,TypedStrPos);

			// position the pen at the cursor position
			Canvas.StrLen(OutStr,xl,yl);
			Canvas.SetPos(xl,Height-3-yl);

			// render the cursor
			Canvas.DrawText("_");

			// figure out which element of the scrollback buffer to should appear first (at the top of the screen)
			idx = SBHead - SBPos;
			y = Height-16-(yl*2);

			if (ScrollBack.Length==0)
				return;

			// change the draw color to white
			Canvas.SetDrawColor(255,255,255,255);

			// while we have enough room to draw another line and there are more lines to draw
			while (y>yl && idx>=0)
			{
				// move the pen to the correct position
				Canvas.SetPos(0,y);

				// draw the next line down in the buffer
				Canvas.DrawText(Scrollback[idx],false);
				idx--;
				y-=yl;
			}
		}
	}

	event BeginState(Name PreviousStateName)
	{
		bIgnoreKeys = true;
		HistoryCur = HistoryTop;

		SBPos = 0;
		bCtrl = false;

		if ( bEnableUI && LargeConsoleScene != None )
		{
			UIController.OpenScene(LargeConsoleScene);
		}
	}

	event EndState( Name NextStateName )
	{
		if ( LargeConsoleScene != None )
		{
			UIController.CloseScene(LargeConsoleScene);
		}
	}
}

defaultproperties
{
	OnReceivedNativeInputKey=InputKey
	OnReceivedNativeInputChar=InputChar

	DefaultTexture_Black=Texture2D'EngineResources.Black'
	DefaultTexture_White=Texture2D'EngineResources.White'
}
