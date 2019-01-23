/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDrawMapPanel extends UTDrawPanel
	config(Input);

var transient bool bShowExtents;
var UTUIScene_Hud UTHudSceneOwner;

/**
 * This delegate is called when the selected node is changed.
 *
 * @Param	Selected	The node that is currently selected.
 */
delegate OnCurrentActorChanged(Actor Selected);

/**
 * When the home node is changed, this delgate will be called
 *
 * @Param	NewHomeNode		The node that was selected
 * @Param	SelectedBy		The UTPlayerController that selected the node
 */
delegate OnHomeNodeSelected(UTGameObjective NewHomeNode, UTPlayerController SelectedBy);

/**
 * Called when a node is actually clicked on.
 *
 * @Param	NewHomeNode		The node that was selected
 * @Param	SelectedBy		The UTPlayerController that selected the node
 */
delegate OnActorSelected(Actor Selected, UTPlayerController SelectedBy);


/**
 * Gather Data and setup the input delegates
 */
event PostInitialize()
{
	Super.PostInitialize();
	OnRawInputKey=InputKey;
	UTHudSceneOwner = UTUIScene_Hud( GetScene() );
}

/**
 * Setup Input subscriptions
 */
event GetSupportedUIActionKeyNames(out array<Name> out_KeyNames )
{
	out_KeyNames[out_KeyNames.Length] = 'SelectNode';
	out_KeyNames[out_KeyNames.Length] = 'SetHomeNode';
	out_KeyNames[out_KeyNames.Length] = 'SelectionUp';
	out_KeyNames[out_KeyNames.Length] = 'SelectionDown';
	out_KeyNames[out_KeyNames.Length] = 'SelectionLeft';
	out_KeyNames[out_KeyNames.Length] = 'SelectionRight';
}

function UTMapInfo GetMapInfo(optional out WorldInfo WI)
{
	WI = UTUIScene_Hud( GetScene() ).GetWorldInfo();
	if ( WI != none )
	{

		return UTMapInfo(WI.GetMapInfo());
	}

	return none;
}

/**
 * Called from native code.  This is the only point where Canvas is valid.
 * we use this moment to setup the color fading
 */
event DrawPanel()
{
	local UTMapInfo MI;
	local UTPlayerController PlayerOwner;
	local actor a;

	MI = GetMapInfo();
	PlayerOwner = UTHudSceneOwner.GetUTPlayerOwner(0);

	if ( MI != none && PlayerOwner != none )
	{
		A = GetActorUnderMouse();
		MI.WatchedActor = A;
	    MI.DrawMap(Canvas, PlayerOwner, 0,0, Canvas.ClipX, Canvas.ClipY, true, (Canvas.ClipX / Canvas.ClipY) );
	}
}


/**
 * @Returns the node currently under the mouse cursor.
 */
function Actor GetActorUnderMouse()
{
	local Vector CursorVector;
	local int X,Y;
	local int i;
	local float D;
	local WorldInfo WI;
	local UTMapInfo MI;
	local UTVehicle_Leviathan Levi;
	local UTTeleportBeacon Beacon;

	MI = GetMapInfo(WI);
	if ( WI != none && MI != none )
	{
		class'UIRoot'.static.GetCursorPosition( X, Y );

		// We can't use OrgX/OrgY here because they won't exist outside of the render loop

		CursorVector.X = X - GetPosition(UIFACE_Left, EVALPOS_PixelViewport, true);
		CursorVector.Y = Y - GetPosition(UIFACE_Top, EVALPOS_PixelViewport, true);

		for (i = 0; i < MI.Objectives.Length; i++)
		{
			D = VSize(MI.Objectives[i].HUDLocation - CursorVector);
			if ( !MI.Objectives[i].IsDisabled() && UTOnslaughtNodeObjective(MI.Objectives[i]) != none && D < 20 * MI.MapScale )
			{
				return MI.Objectives[i];
			}
		}

		foreach WI.DynamicActors(class	'UTVehicle_Leviathan',Levi)
		{
			D = VSize(Levi.HUDLocation - CursorVector);
			if ( D < 20 * MI.MapScale )
			{
				return Levi;
			}
		}

		foreach WI.DynamicActors(class	'UTTeleportBeacon',Beacon)
		{
			D = VSize(Beacon.HUDLocation - CursorVector);
			if ( D < 20 * MI.MapScale )
			{
				return Beacon;
			}
		}
	}

	return none;
}

function FindBestActor()
{
	local int i;
	local float Dist,BestDist;
	local UTGameObjective BestObj;
	local UTPlayerController PlayerOwner;
	local UTMapInfo MI;

	MI = GetMapInfo();

	PlayerOwner = UTHudSceneOwner.GetUTPlayerOwner();
	if ( PlayerOwner != none && PlayerOwner.Pawn != none )
	{

		for (i=0;i<MI.Objectives.Length;i++)
		{
			if ( !MI.Objectives[i].IsDisabled() && UTOnslaughtNodeObjective(MI.Objectives[i]) != none )
			{

				Dist = VSize(PlayerOwner.Pawn.Location - MI.Objectives[i].Location);
				if ( BestObj == none || Dist < BestDist )
				{
					BestDist = Dist;
					BestObj = MI.Objectives[i];
				}
			}
		}
	}

	SetCurrentActor(BestObj);
}

function ChangeCurrentActor(Vector V)
{
	local int i;
	local float Dist,BestDist;
	local Actor BestActor;
	local UTVehicle_Leviathan Levi;
	local UTTeleportBeacon Beacon;
	local float VD;
	local WorldInfo WI;
	local UTMapInfo MI;

	MI = GetMapInfo(WI);

	if ( WI != none && MI != none )
	{
		if (MI.CurrentActor == none)
		{
			FindBestActor();
			return;
		}

		for (i=0;i<MI.Objectives.Length;i++)
		{
			if ( !MI.Objectives[i].IsDisabled() && UTOnslaughtNodeObjective(MI.Objectives[i]) != none && MI.Objectives[i] != MI.CurrentActor )
			{
	    		Dist = abs( VSize( MI.GetActorHudLocation(MI.CurrentActor) - MI.Objectives[i].HUDLocation ));
			    if (BestActor == none || BestDist > Dist)
			    {
			    	VD = V dot Normal(MI.Objectives[i].HudLocation - MI.GetActorHudLocation(MI.CurrentActor));

			    	if ( VD > 0.7 )
			    	{
	    				BestDist = Dist;
		            	BestActor = MI.Objectives[i];
		            }
			    }
			}

		}

		foreach WI.DynamicActors(class'UTVehicle_Leviathan', Levi)
		{
			if ( Levi != MI.CurrentActor )
			{
				Dist = abs( VSize( MI.GetActorHudLocation(MI.CurrentActor) - Levi.HUDLocation) );
			    if (BestActor == none || BestDist > Dist)
			    {
			    	VD = V dot Normal(Levi.HudLocation - MI.GetActorHudLocation(MI.CurrentActor));

			    	if ( VD > 0.7 )
			    	{
	    				BestDist = Dist;
		            	BestActor = Levi;
		            }
			    }
			}
		}

		foreach WI.DynamicActors(class'UTTeleportBeacon', Beacon)
		{
			if ( Levi != MI.CurrentActor )
			{
				Dist = abs( VSize( MI.GetActorHudLocation(MI.CurrentActor) - Levi.HUDLocation) );
			    if (BestActor == none || BestDist > Dist)
			    {
			    	VD = V dot Normal(Beacon.HudLocation - MI.GetActorHudLocation(MI.CurrentActor));

			    	if ( VD > 0.7 )
			    	{
	    				BestDist = Dist;
		            	BestActor = Levi;
		            }
			    }
			}
		}

		if ( BestActor != none )
		{
			SetCurrentActor(BestActor);
		}
	}
}

function SetCurrentActor(Actor NewCurrentActor)
{
	local UTMapInfo MI;

	MI = GetMapInfo();
	if ( MI != none )
	{
		MI.CurrentActor = NewCurrentActor;
		OnCurrentActorChanged(NewCurrentActor);
	}
}

function bool InputKey( const out InputEventParameters EventParms )
{
	local UTPlayerController PC;
	local UTUIScene UTS;
	local Actor AUC;

	UTS = UTUIScene(GetScene());
	if ( UTS != none )
	{
		PC = UTS.GetUTPlayerOwner();
		if ( PC != none )
		{
			if ( EventParms.EventType == IE_Released )
			{
				if ( EventParms.InputKeyName == 'LeftMouseButton' )
				{
					AUC = PickActorUnderCursor();
					// Try to select an node under the cursor.  If one is found, select it
					if ( AUC != none )
					{
						SelectActor(PC);	// Select the node
					}
					return true;
				}
			}
			else
			{
				if ( EventParms.InputKeyName == 'XBoxTypes_A' || EventParms.InputKeyName == 'Enter' )
				{
					SelectActor(PC);
				}

				else if ( EventParms.InputKeyName =='XBoxTypes_X' || EventParms.InputKeyName == 'RightMouseButton' )
				{
					HomeNodeSelected(PC);
					return true;
				}
			}
		}

		if ( EventParms.EventType == IE_Pressed || EventParms.EventType == IE_Repeat )
		{
			if ( EventParms.InputKeyName == 'Left' || EventParms.InputKeyName == 'NumPadFour' ||
					EventParms.InputKeyName == 'XBoxTypeS_DPad_Left' || EventParms.InputKeyName == 'GamePad_LeftStick_Left' )
			{
				ChangeCurrentActor( Vect(-1,0,0) );
			}

			else if ( EventParms.InputKeyName == 'Right' || EventParms.InputKeyName == 'NumPadSix' ||
						EventParms.InputKeyName == 'XBoxTypeS_DPad_Right' || EventParms.InputKeyName == 'GamePad_LeftStick_Right' )
			{
				ChangeCurrentActor( Vect(1,0,0) );
			}

			else if ( EventParms.InputKeyName == 'Up' || EventParms.InputKeyName == 'NumPadEight' ||
						EventParms.InputKeyName == 'XBoxTypeS_DPad_Up' || EventParms.InputKeyName == 'GamePad_LeftStick_Up' )
			{
				ChangeCurrentActor( Vect(0,-1,0) );
			}

			else if ( EventParms.InputKeyName == 'Down' || EventParms.InputKeyName == 'NumPadTwo' ||
						EventParms.InputKeyName == 'XBoxTypeS_DPad_Down' || EventParms.InputKeyName == 'GamePad_LeftStick_Down' )
			{
				ChangeCurrentActor( Vect(0,1,0) );
			}
		}

	}

	return true;
}

/**
 * Look under the mouse cursor and pick the node that is there
 */
function Actor PickActorUnderCursor()
{
	local Actor ActorUnderCursor;

	ActorUnderCursor = GetActorUnderMouse();
	if ( ActorUnderCursor != none )
	{
		SetCurrentActor(ActorUnderCursor);
	}

	return ActorUnderCursor;
}

/**
 * The player has attempted to select a node, Look it up and pass it along to the delegate if it exists
 */

function SelectActor(UTPlayerController UTPC)
{
	local UTMapInfo MI;
	MI = GetMapInfo();
	if ( MI != none && MI.CurrentActor != none )
	{
		OnActorSelected( MI.CurrentActor, UTPC );
	}
}

/**
 * The player has selected a home node, make the notifications.
 */
function HomeNodeSelected(UTPlayerController UTPC)
{

	local UTPlayerReplicationInfo UTPRI;
	local UTGameObjective Obj;
	local UTGameObjective CurrentObj;

	local UTMapInfo MI;
	MI = GetMapInfo();
	if ( MI != none && MI.CurrentActor != none )
	{
		CurrentObj = UTGameObjective(MI.CurrentActor);

		if ( CurrentObj != none )
		{
			UTPRI = UTPlayerReplicationInfo(UTPC.PlayerReplicationInfo);
			if ( UTPRI != none )
			{
				Obj = (UTPRI.StartObjective == CurrentObj) ? none : CurrentObj;
				UTPlayerReplicationInfo(UTPC.PlayerReplicationInfo).SetStartObjective( Obj, false);
				OnHomeNodeSelected(Obj,UTPC);
			}
		}
	}
}

defaultproperties
{
	DefaultStates.Add(class'Engine.UIState_Active')
}
