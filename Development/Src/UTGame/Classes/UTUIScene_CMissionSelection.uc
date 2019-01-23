/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTUIScene_CMissionSelection extends UTUIScene_Campaign
	native(UI);

var transient UTUIOptionButton NextMission;
var transient UILabel NextMissionCaption;
var transient UIImage PreviousMissionBox;
var transient UILabel PreviousMission;
var transient UILabel PreviousResult;
var transient UTUIDataStore_StringList StringStore;

var transient UILabel MissionTeaserText;
var transient UIImage MissionTeaserPortrait;
var transient UIImage MissionTeaser;

var transient UILabel PlayerLabels[4];

var transient AudioComponent AudioPlayer;

var transient array<UTSeqObj_SPMission> AvailMissionList;

var transient UTSeqObj_SPMission PreviousMissionObj;

var transient bool bIsHost;

var transient int CurrentMissionIndex;
var transient int TeaserIndex;
var transient float TeaserDuration;

var StaticMesh PrevMapPointSFXTemplate, MapPointSFXTemplate, SelectedMapPointSFXTemplate;

cpptext
{
	/**
	 * Update the Teaser
	 *
	 * @param	DeltaTime	the time since the last Tick call
	 */
	virtual void Tick( FLOAT DeltaTime );

}

/**
 * Given a mission index, look up the mission
 *
 * @param	MissionIndex	The index of the mission to get
 */
function UTSeqObj_SPMission GetMission(int MissionIndex)
{
	local int i;
	local UTMissionGRI GRI;

	GRI = UTMissionGRI( GetWorldInfo().GRI );
	if ( GRI != none )
	{
		for (i=0;i<GRI.FullMissionList.Length;i++)
		{
			if (GRI.FullMissionList[i].MissionInfo.MissionIndex == MissionIndex)
			{
				return GRI.FullMissionList[i];
			}
		}
	}

	return None;
}

/**
 * Setup the various UI elements
 */
event PostInitialize()
{
	local UTMissionGRI GRI;
	local int i;
	local string s;

	Super.PostInitialize();

	// Get all the UI controls

	NextMission = UTUIOptionButton(FindChild('NextMission',true));
	NextMissionCaption = UILabel( FindChild('NextMissionCaption',true));
	PreviousMissionBox = UIImage( FindChild('PrevMissionBox',true));
	PreviousMission = UILabel( FindChild('PreviousMission',true));
	PreviousResult  = UILabel( FindChild('PreviousResult',true));

	MissionTeaserText		= UILabel( FindChild('MissionTeaserText',true));
	MissionTeaserPortrait	= UIImage( FindChild('MissionTeaserPortrait',true));
	MissionTeaser 			= UIImage( FindChild('MissionTeaser',true));

	for (i=0;i<4;i++)
	{
	    PlayerLabels[i] = UILabel( FindChild(name("PlayerLabel"$i),true));
	}

	MissionTeaser.OnUIAnimEnd = TeaserAnimEnd;
	NextMission.OnValueChanged = NextMissionValueChanged;

	// Get a pointer to the string list

	StringStore = UTUIDataStore_StringList( ResolveDataStore('UTStringList') );
	if ( StringStore != none )
	{
		StringStore.Empty('MissionList');
	}
	else
	{
		return;
	}

	GRI = UTMissionGRI(GetWorldInfo().GRI);
	if (GRI != none)
	{
		GRI.OnMissionChanged = MissionChanged;
	}

	GRI = UTMissionGRI( GetWorldInfo().GRI );
	if ( GRI != none )
	{
		for (i=0;i<GRI.FullMissionList.Length;i++)
		{
			if (GRI.FullMissionList[i].MissionInfo.MissionRules.MapPoint == '')
			{

				s = GRI.FullMissionList[i].MissionInfo.MissionRules.MissionMap;
				s = Repl(s,"-","");
				s = "B_"$s;

				`log("Fixing up Map Point Bone Name:"@s);

				GRI.FullMissionList[i].MissionInfo.MissionRules.MapPoint = name(s);
			}

			if (GRI.FullMissionList[i].MissionInfo.MissionRules.MapDist <= 0.0 )
			{
				GRI.FullMissionList[i].MissionInfo.MissionRules.MapDist = 384.0;
			}

			if (GRI.FullMissionList[i].MissionInfo.MissionRules.MapGlobeTag == '' )
			{
				GRI.FullMissionList[i].MissionInfo.MissionRules.MapGlobeTag = 'Taryd';
			}


		}
	}

}

/**
 * Cleanup
 */
function NotifyGameSessionEnded()
{
	local int i;
	for (i=0;i<AvailMissionList.Length;i++)
	{
		AvailMissionList[i] = None;
	}

	AvailMissionList.Length = 0;
	PreviousMissionObj=none;
}

/**
 * Find a globel for a given mission
 */

function UTSPGlobe FindGlobe(name GlobeName)
{
	local UTMissionPlayerController MPC;
	local UTSPGlobe Globe;

	MPC = UTMissionPlayerController( GetUTPlayerOwner() );
	if ( MPC != none )
	{
		MPC.FindGlobe(GlobeName, Globe);
	}

	return Globe;
}



/**
 * This is called from the PRI when the results of the last mission are in/replicated.
 * It will generate a list of missions and fill everything out.
 *
 * @param Result	The result of the last mission
 */
function SetResult(ESinglePlayerMissionResult Result, bool bYouAreHost, int LastMissionIndex)
{
	local int i,NoChildren;
	local UTProfileSettings Profile;
	local int CMIdx;
	local UTSeqObj_SPMission Mission;
	local EMissionCondition Condition;
	local UTSPGlobe Globe;

	bIsHost = bYouAreHost;

	if ( ButtonBar != none )
	{
		if (bIsHost)
		{
			ButtonBar.SetButton(1, "<Strings:UTGameUI.ButtonCallouts.AcceptMission>", OnButtonBar_AcceptMission);
		}
		else
		{
			ButtonBar.SetButton(1, "<Strings:UTGameUI.ButtonCallouts.Ready>", OnButtonBar_Ready);
		}
	}

	if ( NextMission != none && !bIsHost )
	{
		NextMission.SetVisibility(false);
		NextMissionCaption.SetVisibility(false);
	}


	// Figure out what the current mission is and get a list of it's completed missions
	Profile = GetPlayerProfile();
	if ( Result == ESPMR_None )
	{
	    Profile = GetPlayerProfile();
	    if ( Profile != none )
	    {
			CMIdx = Profile.GetCurrentMissionIndex();

		    if ( CMIdx > 0 )
		    {
		    	Result = ESPMR_Win;
		    }
		    else if ( CMIdx < 0 )
		    {
		    	Result = ESPMR_Loss;
		    	CMIdx = Abs(CMIdx);
		    }
		}
	}
	else
	{
	    Profile.SetCurrentMissionIndex( Result == ESPMR_Win ? LastMissionIndex : LastMissionIndex * -1);
	    SavePlayerProfile(0);
	    CMIdx = Profile.GetCurrentMissionIndex();
	    CMIdx = Abs(CMIdx); //LastMissionIndex);
	}

	PreviousMissionObj = GetMission(CMIdx);
	if ( PreviousMissionObj != none )
	{
		PreviousMission.SetDatastoreBinding(PreviousMissionObj.MissionInfo.MissionTitle);
		NoChildren = PreviousMissionObj.NumChildren();
		for (i = 0; i < NoChildren; i++)
		{
			Mission = PreviousMissionObj.GetChild(i,Condition);

			if ( CheckMission(Profile, Result, Condition) )
			{

				if ( bIsHost )
				{
					// Check to see if we should auto-forward

					if ( Mission.MissionInfo.MissionRules.bCutSequence )
					{
						CutSequence(Mission);
						CloseScene(Self);
						return;
					}

					if ( Mission.MissionInfo.MissionRules.bAutomaticTransition )
					{
						TravelToMission(Mission);
						return;
					}
				}

				AvailMissionList[AvailMissionList.Length] = Mission;
				StringStore.AddStr('MissionList',Mission.MissionInfo.MissionTitle);

				Globe = FindGlobe( Mission.MissionInfo.MissionRules.MapGlobeTag);
				if ( Globe != none )
				{
					Mission.MissionInfo.MissionRules.MapBeacon = new(self)class'StaticMeshComponent';
					Mission.MissionInfo.MissionRules.MapBeacon.SetStaticMesh(MapPointSFXTemplate);

					Globe.SkeletalMeshComponent.AttachComponent(Mission.MissionInfo.MissionRules.MapBeacon,Mission.MissionInfo.MissionRules.MapPoint);
				}
			}
		}

		Globe = FindGlobe( PreviousMissionObj.MissionInfo.MissionRules.MapGlobeTag);
		if (Globe != none )
		{
			PreviousMissionObj.MissionInfo.MissionRules.MapBeacon = new(self)class'StaticMeshComponent';
			PreviousMissionObj.MissionInfo.MissionRules.MapBeacon.SetStaticMesh(PrevMapPointSFXTemplate);
			Globe.SkeletalMeshComponent.AttachComponent(PreviousMissionObj.MissionInfo.MissionRules.MapBeacon,PreviousMissionObj.MissionInfo.MissionRules.MapPoint);
		}
	}

	if ( Result == ESPMR_None )
	{
		PreviousMissionBox.SetVisibility(false);
	}
	else if ( Result == ESPMR_Win )
	{
		PreviousResult.SetValue("<Strings:UTGameUI.Campaign.MissionWon>");
	}
	else
	{
		PreviousResult.SetValue("<Strings:UTGameUI.Campaign.MissionLost>");
	}

	if ( bIsHost )
	{
		NextMissionValueChanged(NextMission,0);
	}

}

/**
 * Checks to see if a mission can be added to the mission list
 *
 * @param Profile		The profile we are using
 * @param Result		The result of the last match
 * @param Condition 	The conditions to check against
 *
 * @returns true if this mission is good to go
 */
function bool CheckMission(UTProfileSettings Profile, ESinglePlayerMissionResult Result, EMissionCondition Condition)
{
	local int i;
	// Quick Win/Lost Short Circuit
	if ( (Condition.MissionResult == EMResult_Won && Result != ESPMR_Win) ||
			(Condition.MissionResult == EMResult_Lost && Result != ESPMR_Loss) )
	{
		return false;
	}

	// Check for any required persistent keys
	for (i=0;i<Condition.RequiredPersistentKeys.Length;i++)
	{
		if ( !Profile.HasPersistentKey(Condition.RequiredPersistentKeys[i]) )
		{
			return false;
		}
	}

	// Check to see if we are restricted by a persistent key
	for (i=0;i<Condition.RequiredPersistentKeys.Length;i++)
	{
		if ( Profile.HasPersistentKey(Condition.RestrictedPersistentKeys[i]) )
		{
			return false;
		}
	}

	// This mission is ok

	return true;
}

/**
 * Call when it's time to go back to the previous scene
 */
function bool AcceptMission()
{

	local int Idx;
	local UTSeqObj_SPMission Mission;

	Idx = NextMission.GetCurrentIndex();;
	if ( Idx >=0 && Idx < AvailMissionList.Length )
	{
		Mission = AvailMissionList[Idx];
		TravelToMission(Mission);
	}
	return true;
}

/**
 * Create the URL and travel to a given mission.
 */
function TravelToMission(UTSeqObj_SPMission Mission)
{
	local UTPlayerController UTPlayerOwner;
	local UTMissionSelectionPRI PRI;
	local string URL;

	UTPlayerOwner = GetUTPlayerOwner();
	if ( UTPlayerOwner != none )
	{
		PRI = UTMissionSelectionPRI(UTPlayerOwner.PlayerReplicationInfo);
		if ( PRI != none )
		{
			URL = Mission.MissionInfo.MissionRules.MissionMap $ Mission.MissionInfo.MissionRules.MissionURL $ "?SPI="$Mission.MissionInfo.MissionIndex;
			PRI.AcceptMission(Mission.MissionInfo.MissionIndex,URL);
		}
	}
}


/**
 * Travel to a cut sequence
 */

function CutSequence(UTSeqObj_SPMission Mission)
{
	local WorldInfo WI;
	local string URL;

	WI = GetWorldInfo();
	if ( WI != none )
	{
		URL = Mission.MissionInfo.MissionRules.MissionMap $ Mission.MissionInfo.MissionRules.MissionURL $ "?SPI="$Mission.MissionInfo.MissionIndex;
		WI.ServerTravel(URL);
	}
}


/**
 * Button bar callbacks - Accept Mission
 */
function bool OnButtonBar_AcceptMission(UIScreenObject InButton, int InPlayerIndex)
{
	return AcceptMission();
}

/**
 * Button bar callbacks - Ready
 */
function bool OnButtonBar_Ready(UIScreenObject InButton, int InPlayerIndex)
{
}


/**
 * Call when it's time to go back to the previous scene
 */
function bool Scene_Back()
{
	GetUTPlayerOwner().QuitToMainMenu();
	CloseScene(self);
	return true;
}


function NextMissionValueChanged( UIObject Sender, int PlayerIndex )
{
	local int NewMissionIndex;
	local UTMissionSelectionPRI PRI;


	local UTMissionPlayerController MPC;
	MPC = UTMissionPlayerController( GetUTPlayerOwner() );
	if ( MPC != none )
	{
		NewMissionIndex = NextMission.GetCurrentIndex();
	}


	NewMissionIndex = NextMission.GetCurrentIndex();
	if ( NewMissionIndex >= 0 )
	{
		PRI = UTMissionSelectionPRI( GetPRIOwner() );
		if ( PRI != none )
		{
			PRI.ChangeMission(NewMissionIndex);
		}
	}
}

function MissionChanged(int NewMissionIndex, UTMissionGRI GRI)
{
	local UTSeqObj_SPMission Mission;
	local EMissionCondition Condition;
	local UTMissionPlayerController MPC;

	MPC = UTMissionPlayerController( GetUTPlayerOwner() );

	if ( MPC != none )
	{
		if ( NewMissionIndex >= 0 )
		{
			if ( CurrentMissionIndex >= 0 )
			{
				Mission = PreviousMissionObj.GetChild(CurrentMissionIndex, Condition);
				if ( Mission.MissionInfo.MissionRules.MapBeacon != none )
				{
					Mission.MissionInfo.MissionRules.MapBeacon.SetStaticMesh(MapPointSFXTemplate);
				}
			}

			Mission = PreviousMissionObj.GetChild(NewMissionIndex,Condition);
			CurrentMissionIndex=NewMissionIndex;
			TeaserIndex = -1;
			PlayNextTeaser();

            `log("### change to:"@Mission.MissionInfo.MissionRules.MapPoint);
			MPC.SetMissionView(Mission.MissionInfo.MissionRules.MapPoint,
									Mission.MissionInfo.MissionRules.MapGlobeTag,
										Mission.MissionInfo.MissionRules.MapDist);

			if ( Mission.MissionInfo.MissionRules.MapBeacon != none )
			{
				Mission.MissionInfo.MissionRules.MapBeacon.SetStaticMesh(SelectedMapPointSFXTemplate);
			}


		}
	}
}


/**
 * Provides a hook for unrealscript to respond to input using actual input key names (i.e. Left, Tab, etc.)
 *
 * Called when an input key event is received which this widget responds to and is in the correct state to process.  The
 * keys and states widgets receive input for is managed through the UI editor's key binding dialog (F8).
 *
 * This delegate is called BEFORE kismet is given a chance to process the input.
 *
 * @param	EventParms	information about the input event.
 *
 * @return	TRUE to indicate that this input key was processed; no further processing will occur on this input key event.
 */
function bool HandleInputKey( const out InputEventParameters EventParms )
{
	local UTMissionPlayerController MPC;
    local vector v;
    local bool b;

	`log("### Handle Input Key"@EventParms.EventType@EventParms.InputKeyName);

	if(EventParms.EventType==IE_Released)
	{
		if(EventParms.InputKeyName=='XboxTypeS_A')
		{
			AcceptMission();
			return true;
		}

/*
		if ( EventParms.InputKeyName == 'LeftMouseButton' )
		{
			FindMissionNearestToCursor();
		}
*/

	}
	else
	{
		if(EventParms.InputKeyName=='F5' )
		{
			AllMaps();
		}

		b = false;
		if(EventParms.InputKeyName=='W' )
		{
			V.X = 32;
			b=true;
		}

		if(EventParms.InputKeyName=='S' )
		{
			V.X = -32;
			b=true;
		}
		if(EventParms.InputKeyName=='A' )
		{
			V.Y = -32;
			b=true;
		}
		if(EventParms.InputKeyName=='D' )
		{
			V.Y = 32;
			b=true;
		}

		if (b)
		{
			MPC = UTMissionPlayerController( GetUTPlayerOwner() );
			if ( MPC != none )
			{
				MPC.CameraLocation += V;
			}
		}
	}

	return Super.HandleInputKey(EventParms);
}

/**
 * The native code has detected that the currently playing Teaser sound has finished.
 * Play the next one.
 */
event PlayNextTeaser()
{
	local EMissionTeaserInfo Teaser;
	local UTSeqObj_SPMission Mission;
	local EMissionCondition Condition;

	if ( CurrentMissionIndex >= 0 )
	{
		Mission = PreviousMissionObj.GetChild(CurrentMissionIndex,Condition);

		// Check if the Sound cue is still playing

		TeaserIndex++;
		if ( TeaserIndex < Mission.MissionInfo.MissionRules.Teasers.Length )
		{
        	if ( MissionTeaser.IsHidden() )
        	{
				MissionTeaser.SetVisibility(true);
				MissionTeaser.PlayUIAnimation('FadeIn',,2);
			}

			Teaser = Mission.MissionInfo.MissionRules.Teasers[TeaserIndex];
			MissionTeaserText.SetValue(Teaser.Text);

			if ( Teaser.Portrait != none )
			{
				MissionTeaserPortrait.ImageComponent.SetImage(Teaser.Portrait);
				MissionTeaserPortrait.ImageComponent.SetCoordinates(Teaser.PortraitUVs);
			}

			if ( AudioPlayer != none && Teaser.Audio != none )
			{
				AudioPlayer.SoundCue = Teaser.Audio;
				AudioPlayer.Play();
			}
			TeaserDuration = Teaser.MinPlaybackDuration;
		}
		else	// No more teaser
		{
			if ( MissionTeaser.IsVisible() )
			{
			 	MissionTeaser.PlayUIAnimation('FadeOut',,2);
		    }

			TeaserIndex = -1;
		}
	}
}

function AudioTeaserFinished(AudioComponent AC)
{
	if ( TeaserDuration <= 0 )
	{
		PlayNextTeaser();
	}
}

function TeaserAnimEnd(UIObject AnimTarget, int AnimIndex, UIAnimationSeq AnimSeq)
{
	if (AnimSeq.SeqName == 'FadeOut')
	{
		MissionTeaser.SetVisibility(false);
	}
}

function bool AllMaps()
{
	local int i,j;
	local UTMissionGRI GRI;
	local UTSeqObj_SPMission Mission;
	local UTSPGlobe Globe;
	local bool bFound;

	GRI = UTMissionGRI( GetWorldInfo().GRI );
	if ( GRI != none )
	{
		Mission = PreviousMissionObj;
		PreviousMissionObj = Mission;
		for (i=0;i<GRI.FullMissionList.Length;i++)
		{
			bFound = false;
			Mission = GRI.FullMissionList[i];
			for (j=0;j<PreviousMissionObj.MissionInfo.MissionProgression.Length;j++)
			{
				if ( UTSeqObj_SPMission( PreviousMissionObj.OutputLinks[j].Links[0].LinkedOp ) == Mission )
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				for (j=0;j<PreviousMissionObj.MissionInfo.MissionProgression.Length;j++)
				{
					if ( UTSeqObj_SPMission( PreviousMissionObj.OutputLinks[j].Links[0].LinkedOp ).MissionInfo.MissionRules.MapPoint == Mission.MissionInfo.MissionRules.MapPoint )
					{
						bFound = true;
						break;
					}
				}
			}

			if ( !bFound && UTSeqObj_SPRootMission(Mission) == none )
			{
				AvailMissionList[AvailMissionList.Length] = Mission;
				StringStore.AddStr('MissionList',Mission.MissionInfo.MissionTitle);

				Globe = FindGlobe( Mission.MissionInfo.MissionRules.MapGlobeTag);
				if ( Globe != none )
				{
					if ( Mission.MissionInfo.MissionRules.MapBeacon == none )
					{
						Mission.MissionInfo.MissionRules.MapBeacon = new(self)class'StaticMeshComponent';
					}
					Mission.MissionInfo.MissionRules.MapBeacon.SetStaticMesh(MapPointSFXTemplate);
					Globe.SkeletalMeshComponent.AttachComponent(Mission.MissionInfo.MissionRules.MapBeacon,Mission.MissionInfo.MissionRules.MapPoint);
				}

				j = PreviousMissionObj.MissionInfo.MissionProgression.Length;
				PreviousMissionObj.MissionInfo.MissionProgression.Length = j+1;
				PreviousMissionObj.OutputLinks.Length = j+1;
				PreviousMissionObj.OutputLinks[j].Links.Length = 1;
				PreviousMissionObj.OutputLinks[j].Links[0].LinkedOp = Mission;
			}
		}
	}

	return true;

}

defaultproperties
{
	Begin Object class=AudioComponent Name=ACPlayer
		OnAudioFinished=AudioTeaserFinished
	End Object
	AudioPlayer=ACPlayer

	PrevMapPointSFXTemplate=StaticMesh'UI_SinglePlayer_World.Mesh.S_SP_UI_Effect_PointRingPrevious'
	MapPointSFXTemplate=StaticMesh'UI_SinglePlayer_World.Mesh.S_SP_UI_Effect_PointRingEnabled'
	SelectedMapPointSFXTemplate=StaticMesh'UI_SinglePlayer_World.Mesh.S_SP_UI_Effect_PointRing'

}
