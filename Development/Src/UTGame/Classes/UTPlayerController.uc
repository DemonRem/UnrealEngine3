/**
 * UTPlayerController
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTPlayerController extends GamePlayerController
	dependson(UTPawn)
	dependson(UTCustomChar_Data)
	dependson(UTProfileSettings)
	config(Game)
	native;

var					bool	bLateComer;
var					bool 	bDontUpdate;
var					bool	bIsTyping;
var					bool	bAcuteHearing;			/** makes playercontroller hear much better (used to magnify hit sounds caused by player) */

var globalconfig	bool	bNoVoiceMessages;
var globalconfig	bool	bNoTextToSpeechVoiceMessages;
var globalconfig	bool	bTextToSpeechTeamMessagesOnly;
var globalconfig	bool	bNoVoiceTaunts;
var globalconfig	bool	bNoAutoTaunts;
var globalconfig	bool	bAutoTaunt;
var globalconfig	bool	bNoMatureLanguage;

var globalconfig	bool	bEnableDodging;
var globalconfig    bool    bLookUpStairs;  // look up/down stairs (player)
var globalconfig    bool    bSnapToLevel;   // Snap to level eyeheight when not mouselooking
var globalconfig    bool    bAlwaysMouseLook;
var globalconfig    bool    bKeyboardLook;  // no snapping when true
var					bool    bCenterView;
var globalconfig	bool	bAlwaysLevel;

/** If true, switch to vehicle's rotation when possessing it (except for roll) */
var	globalconfig	bool	bUseVehicleRotationOnPossess;

var bool	bViewingMap;

/** If true, HUD minimap is zoomed and rotates around player */
var bool bRotateMinimap;

/** set when any initial clientside processing required before allowing the player to join the game has completed */
var bool bInitialProcessingComplete;

/** Cache of some profile settings that are later retrieved by the pawn. */
var bool bProfileWeaponBob;
var bool bProfileAutoSwitchWeaponOnPickup;
var EWeaponHand ProfileWeaponHand;

// @todo steve - move pawn shadow detail to worldinfo or other appropriate place
var globalconfig enum EPawnShadowMode
{
	SHADOW_None,
	SHADOW_Self,
	SHADOW_All
} PawnShadowMode;

var		bool	bBehindView;

/** if true, while in the spectating state, behindview will be forced on a player */
var bool bForceBehindView;

/** if true, rotate smoothly to desiredrotation */
var bool bUsePhysicsRotation;

var bool bFreeCamera;

var globalconfig UTPawn.EWeaponHand WeaponHandPreference;
var UTPawn.EWeaponHand				WeaponHand;

var vector		DesiredLocation;

var UTAnnouncer Announcer;
var UTMusicManager MusicManager;

var float LastTauntAnimTime;
var float LastKickWarningTime;

var localized string MsgPlayerNotFound;

/** view shake information */
struct native ViewShakeInfo
{
	/** how far to offset view */
	var() vector OffsetMag;
	/** how fast to offset view */
	var() vector OffsetRate;
	/** total duration of view offset shaking */
	var() float OffsetTime;
	/** @note: the rotation data is in vectors instead of rotators so the same function can be used to manipulate them as the offsets above
		X = Pitch, Y = Yaw, Z = Roll
	*/
	/** how far to rotate view */
	var() vector RotMag;
	/** how fast to rotate view */
	var() vector RotRate;
	/** total duration of view rotation shaking */
	var() float RotTime;
};

/** view shaking that is currently being applied */
var ViewShakeInfo CurrentViewShake;

/** plays camera animations (mostly used for viewshakes) */
var CameraAnimInst CameraAnimPlayer;
/** set when the last camera anim we played was caused by damage - we only let damage shakes override other damage shakes */
var bool bCurrentCamAnimIsDamageShake;
/** stores post processing settings applied by the camera animation
 * applied additively to the default post processing whenever a camera anim is playing
 */
var PostProcessSettings CamOverridePostProcess;

/** camera anim played when hit (blend strength depends on damage taken) */
var CameraAnim DamageCameraAnim;
/** camera anim played when spawned */
var CameraAnim SpawnCameraAnim[3];

/** current offsets applied to the camera due to CurrentViewShake */
var vector ShakeOffset; // current magnitude to offset camera position from shake
var rotator ShakeRot; // current magnitude to offset camera rotation from shake
var globalconfig	bool	bLandingShake;

var float LastCameraTimeStamp; /** Used during matinee sequences */
var class<Camera> MatineeCameraClass;

var config bool bCenteredWeaponFire;

/** This variable stores the objective that the player wishes to spawn closest to */
var UTGameObjective StartObjective;

/** cached result of GetPlayerViewPoint() */
var Actor CalcViewActor;
var vector CalcViewActorLocation;
var rotator CalcViewActorRotation;
var vector CalcViewLocation;
var rotator CalcViewRotation;

var float	LastWarningTime;	/** Last time a warning about a shot being fired at my pawn was accepted. */

var Pawn ShadowPawn;	/** Pawn currently controlled in behindview, for which shadow is turned on.  Turn off shadow if player is no longer controlling this pawn or in behindview */

/** How fast (degrees/sec) should a zoom occur */
var float FOVLinearZoomRate;

/** If TRUE, FOV interpolation for zooming is nonlineear, using FInterpTo.  If FALSE, use linear interp. */
var transient bool bNonlinearZoomInterpolation;

/** Interp speed (as used in FInterpTo) for nonlinear FOV interpolation. */
var transient float FOVNonlinearZoomInterpSpeed;

/** Used to scale changes in rotation when the FOV is zoomed */
var float ZoomRotationModifier;

/** Whether or not we should retrieve settings from the profile on next tick. */
var transient bool bRetrieveSettingsFromProfileOnNextTick;

/** vars for debug freecam, which allows camera to view player from all angles */
var transient bool		bDebugFreeCam;
var transient rotator	DebugFreeCamRot;

/** last time ServerShowPathToBase() was executed, to avoid spamming the world with path effects */
var float LastShowPathTime;

/** enum for the various options for the game telling the player what to do next */
enum EAutoObjectivePreference
{
	AOP_Disabled, // turned off
	AOP_NoPreference,
	AOP_Attack, // tell what to do to attack
	AOP_Defend, // tell what to do to defend
	AOP_OrbRunner,

};
var globalconfig EAutoObjectivePreference AutoObjectivePreference;

struct native ObjectiveAnnouncementInfo
{
	/** the default announcement sound to play (can be None) */
	var() SoundNodeWave AnnouncementSound;
	/** text displayed onscreen for this announcement */
	var() localized string AnnouncementText;
};

/** last objective CheckAutoObjective() sent a notification about */
var Actor LastAutoObjective;

/** Custom scaling for vehicle check radius.  Used by UTConsolePlayerController */
var	float	VehicleCheckRadiusScaling;

/** Holds the template to use for the quick pick scene */
var UTUIScene_WeaponQuickPick QuickPickSceneTemplate;

/** Holds the actual scene.  When the quick pick scene is up, this is valid */
var UTUIScene_WeaponQuickPick QuickPickScene;

/** If true, the quick pick menu will be disable and the key will act like PrevWeapon */
var() bool bDisableQuickPick;

/** Used for pulsing critical objective beacon */
var float BeaconPulseScale;
var float BeaconPulseMax;
var float BeaconPulseRate;
var bool bBeaconPulseDir;

var float PulseTimer;
var bool bPulseTeamColor;

/** Holds the current Map Scene */
var UTUIScene CurrentMapScene;

/** Holds the current Mid Game Menu Scene */
var UTUIScene CurrentMidGameMenu;

/** Holds the current player Loading scene */
//var UTUIScene PawnConstructionScene;

/** Holds a template of the scene to use */
//var UTUIScene PawnConstructionSceneTemplate;

var bool bConstructioningMeshes;

/** Holds the template for the command menu */
var UTUIScene_CommandMenu CommandMenuTemplate;
var UTUIScene_CommandMenu CommandMenu;

var UTUIScene_MapVote TestSceneTemplate;

/** Struct to define values for different post processing presets. */
struct native PostProcessInfo
{
	var float Shadows;
	var float MidTones;
	var float HighLights;
	var float Desaturation;
};

/** Array of possible preset values for post process. */
var transient array<PostProcessInfo>	PostProcessPresets;

/** The effect to play on the camera **/
var UTEmitCameraEffect CameraEffect;

/** This player's Voter Registration Card */
var UTVoteReplicationInfo VoteRI;

cpptext
{
	virtual void HearSound(USoundCue* InSoundCue, AActor* SoundPlayer, const FVector& SoundLocation, UBOOL bStopWhenOwnerDestroyed);
	virtual UBOOL Tick( FLOAT DeltaSeconds, ELevelTick TickType );
	virtual UBOOL MoveWithInterpMoveTrack(UInterpTrackMove* MoveTrack, UInterpTrackInstMove* MoveInst, FLOAT CurTime, FLOAT DeltaTime);
	virtual void ModifyPostProcessSettings(FPostProcessSettings& PPSettings) const;
}

event InitInputSystem()
{
	local UTGameReplicationInfo GRI;

	Super.InitInputSystem();

	AddOnlineDelegates(true);

	// we do this here so that we only bother to create it for local players
	CameraAnimPlayer = new(self) class'CameraAnimInst';

	// see if character processing was already completed
	// this can happen on clients if a PlayerController class switch is performed
	// and the GRI is received before the new PlayerController
	if (WorldInfo.NetMode == NM_Client)
	{
		GRI = UTGameReplicationInfo(WorldInfo.GRI);
		if (GRI != None && GRI.bProcessedInitialCharacters)
		{
			CharacterProcessingComplete();
		}
	}
}

event PlayerTick( float DeltaTime )
{
	Super.PlayerTick(DeltaTime);

	// This needs to be done here because it ensures that all datastores have been registered properly
	// it also ensures we do not update the profile settings more than once.
	if( bRetrieveSettingsFromProfileOnNextTick )
	{
		RetrieveSettingsFromProfile();
		bRetrieveSettingsFromProfileOnNextTick = FALSE;
	}
}

/** Get the preferred custom character setup for this player (from player profile store). */
native function CustomCharData GetPlayerCustomCharData(string CharDataString);

/** tells the server about the character this player is using */
reliable server function ServerSetCharacterData(CustomCharData CharData)
{
	local UTPlayerReplicationInfo PRI;
	local UTTeamInfo Team;
	local int Index;

	PRI = UTPlayerReplicationInfo(PlayerReplicationInfo);
	if (PRI != None)
	{
		PRI.SetCharacterData(CharData);
	}

	// force bots on player's team to be same faction
	if (WorldInfo.NetMode == NM_Standalone)
	{
		Team = UTTeamInfo(PlayerReplicationInfo.Team);
		if (Team != None)
		{
			Index = class'UTCustomChar_Data'.default.Characters.Find('CharID', CharData.BasedOnCharID);
			if (Index != INDEX_NONE)
			{
				Team.Faction = class'UTCustomChar_Data'.default.Characters[Index].Faction;
			}
		}
	}
}

/** Sets online delegates to respond to for this PC. */
function AddOnlineDelegates(bool bRegisterVoice)
{
	// this is done automatically in net games so only need to call it for standalone.
	if (bRegisterVoice && WorldInfo.NetMode == NM_Standalone && VoiceInterface != None)
	{
		VoiceInterface.RegisterLocalTalker(LocalPlayer(Player).ControllerId);
		VoiceInterface.AddRecognitionCompleteDelegate(LocalPlayer(Player).ControllerId, SpeechRecognitionComplete);
	}

	// Register a callback for when the profile finishes reading.
	if (OnlineSub != None)
	{
		if (OnlineSub.PlayerInterface != None)
		{
			OnlineSub.PlayerInterface.AddReadProfileSettingsCompleteDelegate(LocalPlayer(Player).ControllerId, OnReadProfileSettingsComplete);
		}
	}
}

/** Clears previously set online delegates. */
function ClearOnlineDelegates()
{
	local LocalPlayer LP;

	LP = LocalPlayer(Player);
	if (LP != None)
	{
		if (OnlineSub != None)
		{
			if (VoiceInterface != None)
			{
				VoiceInterface.ClearRecognitionCompleteDelegate(LP.ControllerId, SpeechRecognitionComplete);
				VoiceInterface.UnregisterLocalTalker(LP.ControllerId);
			}

			if (OnlineSub.PlayerInterface != None)
			{
				OnlineSub.PlayerInterface.ClearReadProfileSettingsCompleteDelegate(LP.ControllerId, OnReadProfileSettingsComplete);
			}
		}
	}
}

/** Callback for when the profile finishes reading for this PC. */
function OnReadProfileSettingsComplete(bool bWasSuccessful)
{
	`Log("UTPlayerController::OnReadProfileSettingsComplete() - bWasSuccessful: " $ bWasSuccessful $ ", ControllerId: " $ LocalPlayer(Player).ControllerId);

	if (bWasSuccessful)
	{
		bRetrieveSettingsFromProfileOnNextTick=TRUE;
	}
}

/** Cleans up online subsystem game sessions and posts stats if the match is arbitrated. */
function bool CleanupOnlineSubsystemSession(bool bWasFromMenu)
{
	//local int Item;

	if (WorldInfo.NetMode != NM_Standalone &&
		OnlineSub != None &&
		OnlineSub.GameInterface != None &&
		OnlineSub.GameInterface.GetGameSettings() != None)
	{
		/* @todo: JOE GRAF/AMITT - Reenable this to make arbitration work.

		// Write arbitration data, if required
		if (OnlineSub.GameInterface.GetGameSettings().bUsesArbitration)
		{
			if (bWasFromMenu || bHasLostContactWithServer)
			{
				if (WorldInfo.NetMode != NM_Client)
				{
					WorldInfo.Game.bHasEndGameHandshakeBegun = true;
					// Broadcast the dropping to all clients before leaving
//					WorldInfo.Game.NotifyPlayerHasDropped(self);
				}
				else
				{
					// If we lost connection to the host mark them as a dropper
					if (bHasLostContactWithServer)
					{
						ClientWriteStatsForDroppedPlayer(HostId);
						bWasFromMenu = true;
						// Find the host's PRI and mark it as inactive
						for (Item = 0; Item < WorldInfo.GRI.PRIArray.Length; Item++)
						{
							if (WorldInfo.GRI.PRIArray[Item].UniqueId == HostId)
							{
								WorldInfo.GRI.PRIArray[Item].bIsInactive = true;
								break;
							}
						}
					}
					else
					{
						// Otherwise we must be the player dropping
						ClientWriteStatsForDroppedPlayer(PlayerReplicationInfo.UniqueId);
					}
				}
				// Write out the other player stats
				ClientWriteArbitrationEndGameData(WorldInfo.GRI.GameClass.default.OnlineStatsWriteClass);
				// Write true skill for non-dropped players
				ClientWriteOnlinePlayerScores();
			}
			else
			{
				// Write out the all player stats
				ClientWriteArbitrationEndGameData(WorldInfo.GRI.GameClass.default.OnlineStatsWriteClass);
			}
		}*/


		`Log("UTPlayerController::CleanupOnlineSubsystemSession() - Ending Online Game, ControllerId: " $ LocalPlayer(Player).ControllerId);

		// Set the end delegate so we can know when that is complete and call destroy
		OnlineSub.GameInterface.AddEndOnlineGameCompleteDelegate(OnEndOnlineGameComplete);
		OnlineSub.GameInterface.EndOnlineGame();

		return true;
	}
	return false;
}

/** Called when returning to the main menu. */
function QuitToMainMenu()
{
	`Log("UTPlayerController::QuitToMainMenu() - Cleaning Up OnlineSubsystem, ControllerId: " $ LocalPlayer(Player).ControllerId);
	if(CleanupOnlineSubsystemSession(true)==false)
	{
		`Log("UTPlayerController::QuitToMainMenu() - Online cleanup failed, finishing quit.");
		FinishQuitToMainMenu();
	}
}

/** Called after onlinesubsystem game cleanup has completed. */
function FinishQuitToMainMenu()
{
	// Call disconnect to force us back to
	ConsoleCommand("Disconnect");

	`Log("------ QUIT TO MAIN MENU --------");
}

/**
 * Sets a error message in the registry datastore that will display to the user the next time they are in the frontend.
 *
 * @param Title		Title of the messagebox.
 * @param Message	Message of the messagebox.
 */
function SetFrontEndErrorMessage(string Title, string Message)
{
	class'UIRoot'.static.SetDataStoreStringValue("<Registry:FrontEndError_Title>", Title);
	class'UIRoot'.static.SetDataStoreStringValue("<Registry:FrontEndError_Message>", Message);
	class'UIRoot'.static.SetDataStoreStringValue("<Registry:FrontEndError_Display>", "1");
}

/**
 * called when the client loses connection with the server
 *
 * This version cleans up the online game and then quits back to the main menu.
 *
 * @return true if the function will handle the situation, false to automatically be returned to the default map
 */
event bool NotifyConnectionLost()
{
	if (WorldInfo.Game != None)
	{
		// Mark the server as having a problem
		WorldInfo.Game.bHasNetworkError = true;
	}

	// Set some error messaging in the registry to display to the user when they have returned to the main menu.
	SetFrontEndErrorMessage("<Strings:UTGameUI.Errors.ConnectionLost_Title>", "<Strings:UTGameUI.Errors.ConnectionLost_Message>");

	// Start quitting to the main menu
	QuitToMainMenu();

	return true;
}

/**
 * Called when the online game has finished ending.
 */
function OnEndOnlineGameComplete(bool bWasSuccessful)
{
	OnlineSub.GameInterface.ClearEndOnlineGameCompleteDelegate(OnEndOnlineGameComplete);

	// Set the destroy delegate so we can know when that is complete
	OnlineSub.GameInterface.AddDestroyOnlineGameCompleteDelegate(OnDestroyOnlineGameComplete);
	// Now we can destroy the game
	`Log("UTPlayerController::OnEndOnlineGameComplete() - Destroying Online Game, ControllerId: " $ LocalPlayer(Player).ControllerId);
	OnlineSub.GameInterface.DestroyOnlineGame();
}

/**
 * Called when the destroy online game has completed. At this point it is safe
 * to travel back to the menus
 *
 * @param bWasSuccessful whether it worked ok or not
 */
function OnDestroyOnlineGameComplete(bool bWasSuccessful)
{
	`Log("UTPlayerController::OnDestroyOnlineGameComplete() - Finishing Quit to Main Menu, ControllerId: " $ LocalPlayer(Player).ControllerId);
	OnlineSub.GameInterface.ClearDestroyOnlineGameCompleteDelegate(OnDestroyOnlineGameComplete);
	FinishQuitToMainMenu();
}

/** set to VoiceInterface's speech recognition delegate; called when words are recognized */
function SpeechRecognitionComplete()
{
	local array<SpeechRecognizedWord> Words;
	local SpeechRecognizedWord ReplicatedWords[3];
	local int i;

	VoiceInterface.GetRecognitionResults(LocalPlayer(Player).ControllerId, Words);
	if (Words.length > 0)
	{
		for (i = 0; i < 3 && i < Words.length; i++)
		{
			ReplicatedWords[i] = Words[i];
		}
		ServerProcessSpeechRecognition(ReplicatedWords);
	}
}

reliable server function ServerProcessSpeechRecognition(SpeechRecognizedWord ReplicatedWords[3])
{
	local array<SpeechRecognizedWord> Words;
	local int i;
	local UTGame Game;

	Game = UTGame(WorldInfo.Game);
	if (Game != None)
	{
		for (i = 0; i < 3; i++)
		{
			if (ReplicatedWords[i].WordText != "")
			{
				Words[Words.length] = ReplicatedWords[i];
			}
		}

		Game.ProcessSpeechRecognition(self, Words);
	}
}

/** turns on/off voice chat/recognition */
exec function ToggleSpeaking(bool bNowOn)
{
	local LocalPlayer LP;

	if (VoiceInterface != None)
	{
		LP = LocalPlayer(Player);
		if (LP != None)
		{
			if (bNowOn)
			{
				VoiceInterface.StartNetworkedVoice(LP.ControllerId);
				VoiceInterface.StartSpeechRecognition(LP.ControllerId);
			}
			else
			{
				VoiceInterface.StopNetworkedVoice(LP.ControllerId);
				VoiceInterface.StopSpeechRecognition(LP.ControllerId);
			}
		}
	}
}

/* ClientHearSound()
Replicated function from server for replicating audible sounds played on server
UTPlayerController implementation considers sounds from its pawn as local, even if the pawn is not the viewtarget
*/
unreliable client event ClientHearSound(SoundCue ASound, Actor SourceActor, vector SourceLocation, bool bStopWhenOwnerDestroyed, optional bool bIsOccluded )
{
	local AudioComponent AC;

	if ( SourceActor == None )
	{
		AC = GetPooledAudioComponent(ASound, SourceActor, bStopWhenOwnerDestroyed, true, SourceLocation);
		if (AC == None)
		{
			return;
		}
		AC.bUseOwnerLocation = false;
		AC.Location = SourceLocation;
	}
	else if ( (SourceActor == GetViewTarget()) || (SourceActor == self) || (SourceActor == Pawn) )
	{
		AC = GetPooledAudioComponent(ASound, None, bStopWhenOwnerDestroyed);
		if (AC == None)
		{
			return;
		}
		AC.bAllowSpatialization = false;
	}
	else
	{
		AC = GetPooledAudioComponent(ASound, SourceActor, bStopWhenOwnerDestroyed);
		if (AC == None)
		{
			return;
		}
		if (!IsZero(SourceLocation) && SourceLocation != SourceActor.Location)
		{
			AC.bUseOwnerLocation = false;
			AC.Location = SourceLocation;
		}
	}
	if ( bIsOccluded )
	{
		// if occluded reduce volume: @FIXME do something better
		AC.VolumeMultiplier *= 0.5;
		// AC.LowPassFilterApplied = true;
	}
	AC.Play();
}

function bool AimingHelp(bool bInstantHit)
{
	return (WorldInfo.NetMode == NM_Standalone) && bAimingHelp && (WorldInfo.Game.GameDifficulty < 5);
}

/**
* @returns the a scaling factor for the distance from the collision box of the target to accept aiming help (for instant hit shots)
*/
function float AimHelpModifier()
{
	local float AimingHelp;

	AimingHelp = FOVAngle < DefaultFOV - 8 ? 0.5 : 0.75;

	// reduce aiming help at higher difficulty levels
	if ( WorldInfo.Game.GameDifficulty > 2 )
		AimingHelp *= 0.33 * (5 - WorldInfo.Game.GameDifficulty);

	return AimingHelp;
}

/**
 * Adjusts weapon aiming direction.
 * Gives controller a chance to modify the aiming of the pawn. For example aim error, auto aiming, adhesion, AI help...
 * Requested by weapon prior to firing.
 * UTPlayerController implementation doesn't adjust aim, but sets the shottarget (for warning enemies)
 *
 * @param	W, weapon about to fire
 * @param	StartFireLoc, world location of weapon fire start trace, or projectile spawn loc.
 * @param	BaseAimRot, original aiming rotation without any modifications.
 */
function Rotator GetAdjustedAimFor( Weapon W, vector StartFireLoc )
{
	local vector	FireDir, HitLocation, HitNormal;
	local actor		BestTarget, HitActor;
	local float		bestAim, bestDist, MaxRange;
	local rotator	BaseAimRot;

	BaseAimRot = (Pawn != None) ? Pawn.GetBaseAimRotation() : Rotation;
	FireDir	= vector(BaseAimRot);
	MaxRange = W.MaxRange();
	HitActor = Trace(HitLocation, HitNormal, StartFireLoc + MaxRange * FireDir, StartFireLoc, true);

	if ( (HitActor != None) && HitActor.bProjTarget )
	{
		BestTarget = HitActor;
	}
	else if ( ((WorldInfo.Game != None) &&(WorldInfo.Game.Numbots > 0)) || AimingHelp(true) )
	{
		// guess who target is
		// @todo steve
		bestAim = 0.95;
		BestTarget = PickTarget(class'Pawn', bestAim, bestDist, FireDir, StartFireLoc, MaxRange);
		if ( (W != None) && W.bInstantHit )
		{
			InstantWarnTarget(BestTarget,W,vector(BaseAimRot));
		}
	}

	ShotTarget = Pawn(BestTarget);
   	return BaseAimRot;
}


/** Tries to find a vehicle to drive within a limited radius. Returns true if successful */
function bool FindVehicleToDrive()
{
	return ( CheckVehicleToDrive(true) != None );
}

/** returns the Vehicle passed in if it can be driven
  */
function UTVehicle CheckPickedVehicle(UTVehicle V, bool bEnterVehicle)
{
	if ( (V == None) || !bEnterVehicle )
	{
		return V;
	}
	if ( V.TryToDrive(Pawn) )
	{
		return V;
	}
	return None;
}

/** Returns vehicle which can be driven
  * @PARAM bEnterVehicle if true then player enters the found vehicle
  */
function UTVehicle CheckVehicleToDrive(bool bEnterVehicle)
{
	local UTVehicle V, PickedVehicle;
	local vector ViewDir, HitLocation, HitNormal, ViewPoint;
	local rotator ViewRotation;
	local Actor HitActor;
	local float CheckDist;

	// first try to get in vehicle I'm standing on
	PickedVehicle = CheckPickedVehicle(UTVehicle(Pawn.Base), bEnterVehicle);
	if ( PickedVehicle != None )
	{
		return PickedVehicle;
	}

	// see if looking at vehicle
	ViewPoint = Pawn.GetPawnViewLocation();
	ViewRotation = Rotation;
	CheckDist = Pawn.VehicleCheckRadius * VehicleCheckRadiusScaling;
	ViewDir = CheckDist * vector(ViewRotation);
	HitActor = Trace(HitLocation, HitNormal, ViewPoint + ViewDir, ViewPoint, true,,,TRACEFLAG_Blocking);

	PickedVehicle = CheckPickedVehicle(UTVehicle(HitActor), bEnterVehicle);
	if ( PickedVehicle != None )
	{
		return PickedVehicle;
	}

	// make sure not just looking above vehicle
	ViewRotation.Pitch = 0;
	ViewDir = CheckDist * vector(ViewRotation);
	HitActor = Trace(HitLocation, HitNormal, ViewPoint + ViewDir, ViewPoint, true,,,TRACEFLAG_Blocking);

	PickedVehicle = CheckPickedVehicle(UTVehicle(HitActor), bEnterVehicle);
	if ( PickedVehicle != None )
	{
		return PickedVehicle;
	}

	// make sure not just looking above vehicle
	ViewRotation.Pitch = -5000;
	ViewDir = CheckDist * vector(ViewRotation);
	HitActor = Trace(HitLocation, HitNormal, ViewPoint + ViewDir, ViewPoint, true,,,TRACEFLAG_Blocking);

	PickedVehicle = CheckPickedVehicle(UTVehicle(HitActor), bEnterVehicle);
	if ( PickedVehicle != None )
	{
		return PickedVehicle;
	}

	// special case for vehicles like Darkwalker
	if ( UTGame(WorldInfo.Game) != None )
	{
		for ( V=UTGame(WorldInfo.Game).VehicleList; V!=None; V=V.NextVehicle )
		{
			if ( V.bHasCustomEntryRadius && V.InCustomEntryRadius(Pawn) )
			{
				V = CheckPickedVehicle(V, bEnterVehicle);
				if ( V != None )
				{
					return V;
				}
			}
		}
	}

	return None;
}

exec function ToggleMinimap()
{
	bRotateMiniMap = !bRotateMiniMap;
}

exec function DropFlag()
{
	ServerDropFlag();
}

reliable server function ServerDropFlag()
{
	if ( UTPawn(Pawn) != None )
		UTPawn(Pawn).DropFlag();
}

/** LandingShake()
returns true if controller wants landing view shake
*/
simulated function bool LandingShake()
{
	return bLandingShake;
}

simulated function PlayBeepSound()
{
	PlaySound(SoundCue'A_Gameplay.Gameplay.MessageBeepCue', true);
}

/* epic ===============================================
* ::ReceiveWarning
*
* Notification that the pawn is about to be shot by a
* trace hit weapon.
*
* =====================================================
*/
event ReceiveWarning(Pawn shooter, float projSpeed, vector FireDir)
{
	if ( WorldInfo.TimeSeconds - LastWarningTime < 1.0 )
		return;
	LastWarningTime = WorldInfo.TimeSeconds;
	if ( (shooter != None) && !WorldInfo.GRI.OnSameTeam(shooter,self) )
		ClientMusicEvent(0);
}

/* epic ===============================================
* ::ReceiveProjectileWarning
*
* Notification that the pawn is about to be shot by a
* projectile.
*
* =====================================================
*/
function ReceiveProjectileWarning(Projectile proj)
{
	if ( WorldInfo.TimeSeconds - LastWarningTime < 1.0 )
		return;
	LastWarningTime = WorldInfo.TimeSeconds;
	if ( (proj.Instigator != None) && !WorldInfo.GRI.OnSameTeam(proj.Instigator,self) && (proj.Speed > 0) )
	{
		SetTimer(VSize(proj.Location - Pawn.Location)/proj.Speed,false,'ProjectileWarningTimer');
	}
}

function ProjectileWarningTimer()
{
	ClientMusicEvent(0);
}

function PlayWinMessage(bool bWinner)
{
/* //@todo steve
	if ( bWinner )
		bDisplayWinner = true;
	else
		bDisplayLoser = true;
*/
}

simulated function bool TriggerInteracted()
{
	local UTGameObjective O, Best;
	local vector ViewDir, PawnLoc2D, VLoc2D;
	local float NewDot, BestDot;

	// check base UTGameObjective first
	if ((UTGameObjective(Pawn.Base) != None || UTOnslaughtNodeTeleporter(Pawn.Base) != None) && Pawn.Base.UsedBy(Pawn))
	{
		return true;
	}

	// handle touched UTGameObjectives next
	ForEach Pawn.TouchingActors(class'UTGameObjective', O)
	{
		if ( O.UsedBy(Pawn) )
			return true;
	}

	// now handle nearby UTGameObjectives
	ViewDir = vector(Rotation);
	PawnLoc2D = Pawn.Location;
	PawnLoc2D.Z = 0;
	ForEach Pawn.OverlappingActors(class'UTGameObjective', O, Pawn.VehicleCheckRadius)
	{
		if ( O.bAllowRemoteUse )
		{
			// Pawn must be facing objective or overlapping it
			VLoc2D = O.Location;
			Vloc2D.Z = 0;
			NewDot = Normal(VLoc2D-PawnLoc2D) Dot ViewDir;
			if ( NewDot > BestDot )
			{
				// check that objective is visible
				if ( FastTrace(O.Location,Pawn.Location) )
				{
					Best = O;
					BestDot = NewDot;
				}
			}
		}
	}
	if ( Best != None && Best.UsedBy(Pawn) )
		return true;

	// no UT specific triggers used, fall back to general case
	return super.TriggerInteracted();
}

exec function PlayVehicleHorn( int HornIndex )
{
	local UTVehicle V;

	V = UTVehicle(Pawn);
	if ( (V != None) && (V.Health > 0) && (WorldInfo.TimeSeconds - LastTauntAnimTime > 0.3)  )
	{
		ServerPlayVehicleHorn(HornIndex);
		LastTauntAnimTime = WorldInfo.TimeSeconds;
	}
}

unreliable server function ServerPlayVehicleHorn( int HornIndex )
{
	local UTVehicle V;

	V = UTVehicle(Pawn);
	if ( (V != None) && (V.Health > 0)  )
	{
		V.PlayHorn(HornIndex);
	}
}

function Typing( bool bTyping )
{
	bIsTyping = bTyping;
    if ( (Pawn != None) && !Pawn.bTearOff )
	UTPawn(Pawn).bIsTyping = bTyping;
}

simulated event Destroyed()
{
	Super.Destroyed();

	if (Announcer != None)
	{
		Announcer.Destroy();
	}
	if (MusicManager != None)
	{
		MusicManager.Destroy();
	}

	ClearOnlineDelegates();
}


function SoakPause(Pawn P)
{
	`log("Soak pause by "$P);
	SetViewTarget(P);
	SetPause(true);
	bBehindView = true;
	myHud.bShowDebugInfo = true;
}

event KickWarning()
{
	if ( WorldInfo.TimeSeconds - LastKickWarningTime > 0.5 )
	{
		ReceiveLocalizedMessage( class'UTIdleKickWarningMessage', 0, None, None, self );
		LastKickWarningTime = WorldInfo.TimeSeconds;
	}
}

/* CheckJumpOrDuck()
Called by ProcessMove()
handle jump and duck buttons which are pressed
*/
function CheckJumpOrDuck()
{
	if ( bDoubleJump && (bUpdating || UTPawn(Pawn).CanDoubleJump()) )
	{
		UTPawn(Pawn).DoDoubleJump( bUpdating );
	}
    else if ( bPressedJump )
	{
		Pawn.DoJump( bUpdating );
	}
	if ( Pawn.Physics != PHYS_Falling && Pawn.bCanCrouch )
	{
		// crouch if pressing duck
		Pawn.ShouldCrouch(bDuck != 0);
	}
}

function Restart(bool bVehicleTransition)
{
	Super.Restart(bVehicleTransition);

	// re-check auto objective every time the player respawns
	if (!bVehicleTransition)
	{
		CheckAutoObjective(false);
	}
}

reliable client function ClientRestart(Pawn NewPawn)
{
	local UTVehicle V;

	Super.ClientRestart(NewPawn);
	ServerPlayerPreferences(WeaponHandPreference, bAutoTaunt, bCenteredWeaponFire, AutoObjectivePreference);

	if (NewPawn != None)
	{
		// apply vehicle FOV
		V = UTVehicle(NewPawn);
		if (V == None && NewPawn.IsA('UTWeaponPawn'))
		{
			V = UTVehicle(NewPawn.GetVehicleBase());
		}
		if (V != None)
		{
			DefaultFOV = V.DefaultFOV;
			DesiredFOV = DefaultFOV;
			FOVAngle = DesiredFOV;
		}
		else
		{
			FixFOV();
		}
		// if new pawn has empty weapon, autoswitch to new one
		// (happens when switching from Redeemer remote control, for example)
		if (NewPawn.Weapon != None && !NewPawn.Weapon.HasAnyAmmo())
		{
			SwitchToBestWeapon();
		}
	}
	else
	{
		FixFOV();
	}
}

/** attempts to find an objective for the player to complete depending on their settings and tells the player about it
 * @note: needs to be called on the server, not the client (because that's where the AI is)
 * @param bOnlyNotifyDifferent - if true, only send messages to the player if the selected objective is different from the previous one
 */
function CheckAutoObjective(bool bOnlyNotifyDifferent)
{
	local UTGameObjective DesiredObjective;
	local Actor ObjectiveActor;

	if (Pawn != None && PlayerReplicationInfo != None)
	{
		DesiredObjective = UTGame(WorldInfo.Game).GetAutoObjectiveFor(self);

		if (DesiredObjective != None)
		{
			ObjectiveActor = DesiredObjective.GetAutoObjectiveActor(self);
			if (ObjectiveActor != LastAutoObjective || !bOnlyNotifyDifferent)
			{
				LastAutoObjective = ObjectiveActor;
				ClientSetAutoObjective(LastAutoObjective);
				// spawn willow whisp
				if (FindPathToward(LastAutoObjective) != None)
				{
					Spawn(class'UTWillowWhisp', self,, Pawn.Location);
				}

			}
		}
		else
		{
			LastAutoObjective = None;
			ClientSetAutoObjective(LastAutoObjective);
		}
	}
}

/** client-side notification of the current auto objective */
reliable client function ClientSetAutoObjective(Actor NewAutoObjective)
{
	LastAutoObjective = NewAutoObjective;

	ReceiveLocalizedMessage(class'UTObjectiveAnnouncement', AutoObjectivePreference,,, LastAutoObjective);
}

/* epic ===============================================
* ::Possess
*
* Handles attaching this controller to the specified
* pawn.
*
* =====================================================
*/
event Possess(Pawn inPawn, bool bVehicleTransition)
{
	local int SpawnCamIndx;

	Super.Possess(inPawn, bVehicleTransition);

	// force garbage collection when possessing pawn, to avoid GC during gameplay
	if ( bVehicleTransition && ((WorldInfo.NetMode == NM_Client) || (WorldInfo.NetMode == NM_Standalone)) )
	{
		WorldInfo.ForceGarbageCollection();
	}
	if (!bVehicleTransition)
	{
		if(!WorldInfo.Game.bTeamGame)
		{
			SpawnCamIndx = 2;
		}
		else if(PlayerReplicationInfo.Team.TeamIndex == 0) // red
		{
			SpawnCamIndx = 0;
		}
		else // blue
		{
			SpawnCamIndx = 1;
		}
		ClientPlayCameraAnim(SpawnCameraAnim[SpawnCamIndx], 1.0f);
	}

}

function AcknowledgePossession(Pawn P)
{
	local rotator NewViewRotation;

	Super.AcknowledgePossession(P);

	if ( LocalPlayer(Player) != None )
	{
		ClientEndZoom();
		if ( bUseVehicleRotationOnPossess && (Vehicle(P) != None) )
		{
			NewViewRotation = P.Rotation;
			NewViewRotation.Roll = 0;
			SetRotation(NewViewRotation);
		}
		ServerPlayerPreferences(WeaponHandPreference, bAutoTaunt, bCenteredWeaponFire, AutoObjectivePreference);
	}
}

function SetPawnFemale()
{
//@todo steve - only if no valid character	if ( PawnSetupRecord.Species == None )
		PlayerReplicationInfo.bIsFemale = true;
}

function NotifyChangedWeapon( Weapon PreviousWeapon, Weapon NewWeapon )
{
	//@todo steve - should pawn do this directly?
	if ( UTPawn(Pawn) != None )
		UTPawn(Pawn).SetHand(WeaponHand);
}

simulated event ReceivedPlayer()
{
	Super.ReceivedPlayer();

	if (LocalPlayer(Player) != None)
	{
		ServerPlayerPreferences(WeaponHandPreference, bAutoTaunt, bCenteredWeaponFire, AutoObjectivePreference);
	}
	else
	{
		// default auto objective preference to None for non-local players so we don't send objective info
		// until we've received the client's preference
		AutoObjectivePreference = AOP_Disabled;
	}
}

reliable server function ServerPlayerPreferences( UTPawn.EWeaponHand NewWeaponHand, bool bNewAutoTaunt, bool bNewCenteredWeaponFire, EAutoObjectivePreference NewAutoObjectivePreference)
{
	ServerSetHand(NewWeaponHand);
	ServerSetAutoTaunt(bNewAutoTaunt);

	bCenteredWeaponFire = bNewCenteredWeaponFire;

	if (AutoObjectivePreference != NewAutoObjectivePreference)
	{
		AutoObjectivePreference = NewAutoObjectivePreference;
		CheckAutoObjective(false);
	}
}

reliable server function ServerSetHand(UTPawn.EWeaponHand NewWeaponHand)
{
	WeaponHand = NewWeaponHand;
    if ( UTPawn(Pawn) != None )
		UTPawn(Pawn).SetHand(WeaponHand);
}

function SetHand(UTPawn.EWeaponHand NewWeaponHand)
{
	WeaponHandPreference = NewWeaponHand;
    SaveConfig();
    if ( UTPawn(Pawn) != None )
		UTPawn(Pawn).SetHand(WeaponHand);

    ServerSetHand(NewWeaponHand);
}

event ResetCameraMode()
{}

/**
* return whether viewing in first person mode
*/
function bool UsingFirstPersonCamera()
{
	return !bBehindView;
}

// ------------------------------------------------------------------------

reliable server function ServerSetAutoTaunt(bool Value)
{
	bAutoTaunt = Value;
}

exec function SetAutoTaunt(bool Value)
{
	Default.bAutoTaunt = Value;
	StaticSaveConfig();
	bAutoTaunt = Value;

	ServerSetAutoTaunt(Value);
}

exec function ToggleScreenShotMode()
{
	if ( UTHUD(myHUD).bCrosshairShow )
	{
		UTHUD(myHUD).bCrosshairShow = false;
		SetHand(HAND_Hidden);
		myHUD.bShowHUD = false;
		if ( UTPawn(Pawn) != None )
			UTPawn(Pawn).TeamBeaconMaxDist = 0;
	}
	else
	{
		// return to normal
		UTHUD(myHUD).bCrosshairShow = true;
		SetHand(HAND_Right);
		myHUD.bShowHUD = true;
		if ( UTPawn(Pawn) != None )
			UTPawn(Pawn).TeamBeaconMaxDist = UTPawn(Pawn).default.TeamBeaconMaxDist;
	}
}

/** checks if the player has enabled swapped firemodes for this weapon and adjusts FireModeNum accordingly */
function CheckSwappedFire(out byte FireModeNum)
{
	local UTWeapon Weap;

	if (Pawn != None)
	{
		Weap = UTWeapon(Pawn.Weapon);
		if (Weap != None && Weap.bSwapFireModes)
		{
			if (FireModeNum == 0)
			{
				FireModeNum = 1;
			}
			else if (FireModeNum == 1)
			{
				FireModeNum = 0;
			}
		}
	}
}

exec function StartFire(optional byte FireModeNum)
{
	CheckSwappedFire(FireModeNum);
	Super.StartFire(FireModeNum);
}

exec function StopFire(optional byte FireModeNum)
{
	CheckSwappedFire(FireModeNum);
	Super.StopFire(FireModeNum);
}

unreliable client function PlayStartupMessage(byte StartupStage)
{
	ReceiveLocalizedMessage( class'UTStartupMessage', StartupStage, PlayerReplicationInfo );
}

function NotifyTakeHit(Controller InstigatedBy, vector HitLocation, int Damage, class<DamageType> damageType, vector Momentum)
{
	local int iDam;

	Super.NotifyTakeHit(InstigatedBy,HitLocation,Damage,DamageType,Momentum);

	iDam = Clamp(Damage,0,250);
	if (iDam > 0 || bGodMode)
	{
		ClientPlayTakeHit(hitLocation - Pawn.Location, iDam, damageType);
	}
}

unreliable client function ClientPlayTakeHit(vector HitLoc, byte Damage, class<DamageType> DamageType)
{
	DamageShake(Damage, DamageType);
	HitLoc += Pawn.Location;

	if (UTHud(MyHud) != None)
	{
		UTHud(MyHud).DisplayHit(HitLoc, Damage, DamageType);
	}
}

simulated function bool PerformedUseAction()
{
	if (Pawn != None && Pawn.IsInState('FeigningDeath'))
	{
		// can't use things while feigning death
		return true;
	}
	else
	{
		return Super.PerformedUseAction();
	}
}

// Player movement.
// Player Standing, walking, running, falling.
state PlayerWalking
{
	ignores SeePlayer, HearNoise, Bump;

	event bool NotifyLanded(vector HitNormal, Actor FloorActor)
	{
		if (DoubleClickDir == DCLICK_Active)
		{
			DoubleClickDir = DCLICK_Done;
			ClearDoubleClick();
		}
		else
		{
			DoubleClickDir = DCLICK_None;
		}

		if (Global.NotifyLanded(HitNormal, FloorActor))
		{
			return true;
		}

		return false;
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
	{
		if ( !bEnableDodging )
		{
			DoubleClickMove = DCLICK_None;
		}
		if ( (DoubleClickMove == DCLICK_Active) && (Pawn.Physics == PHYS_Falling) )
			DoubleClickDir = DCLICK_Active;
		else if ( (DoubleClickMove != DCLICK_None) && (DoubleClickMove < DCLICK_Active) )
		{
			if ( UTPawn(Pawn).Dodge(DoubleClickMove) )
				DoubleClickDir = DCLICK_Active;
		}

		Super.ProcessMove(DeltaTime,NewAccel,DoubleClickMove,DeltaRot);
	}

    function PlayerMove( float DeltaTime )
    {
//		local rotator ViewRotation;

	GroundPitch = 0;
/* //@todo steve
	ViewRotation = Rotation;
	if ( Pawn.Physics == PHYS_Walking )
	{
	    //if walking, look up/down stairs - unless player is rotating view
	     if ( (bLook == 0)
		&& (((Pawn.Acceleration != Vect(0,0,0)) && bSnapToLevel) || !bKeyboardLook) )
	    {
		if ( bLookUpStairs || bSnapToLevel )
		{
		    GroundPitch = FindStairRotation(deltaTime);
		    ViewRotation.Pitch = GroundPitch;
		}
		else if ( bCenterView )
		{
		    ViewRotation.Pitch = ViewRotation.Pitch & 65535;
		    if (ViewRotation.Pitch > 32768)
			ViewRotation.Pitch -= 65536;
		    ViewRotation.Pitch = ViewRotation.Pitch * (1 - 12 * FMin(0.0833, deltaTime));
		    if ( (Abs(ViewRotation.Pitch) < 250) && (ViewRotation.Pitch < 100) )
			ViewRotation.Pitch = -249;
		}
	    }
	}
	else
	{
	    if ( !bKeyboardLook && (bLook == 0) && bCenterView )
	    {
		ViewRotation.Pitch = ViewRotation.Pitch & 65535;
		if (ViewRotation.Pitch > 32768)
		    ViewRotation.Pitch -= 65536;
		ViewRotation.Pitch = ViewRotation.Pitch * (1 - 12 * FMin(0.0833, deltaTime));
		if ( (Abs(ViewRotation.Pitch) < 250) && (ViewRotation.Pitch < 100) )
		    ViewRotation.Pitch = -249;
	    }
	}
	SetRotation(ViewRotation);
*/
		Super.PlayerMove(DeltaTime);
	}
}

function ServerSpectate()
{
	GotoState('Spectating');
}

state RoundEnded
{
ignores SeePlayer, HearNoise, KilledBy, NotifyBump, HitWall, NotifyHeadVolumeChange, NotifyPhysicsVolumeChange, Falling, TakeDamage, Suicide;

	exec function PrevWeapon() {}
	exec function NextWeapon() {}
	exec function SwitchWeapon(byte T) {}
	exec function ShowQuickPick(){}
	exec function ToggleMelee() {}

	unreliable client function LongClientAdjustPosition
	(
		float TimeStamp,
		name newState,
		EPhysics newPhysics,
		float NewLocX,
		float NewLocY,
		float NewLocZ,
		float NewVelX,
		float NewVelY,
		float NewVelZ,
		Actor NewBase,
		float NewFloorX,
		float NewFloorY,
		float NewFloorZ
	)
	{
		if ( newState == 'PlayerWaiting' )
			GotoState( newState );
	}

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;
		local Rotator DeltaRot, ViewRotation;

		GetAxes(Rotation,X,Y,Z);
		// Update view rotation.
		ViewRotation = Rotation;
		// Calculate Delta to be applied on ViewRotation
		DeltaRot.Yaw	= PlayerInput.aTurn;
		DeltaRot.Pitch	= PlayerInput.aLookUp;
		ProcessViewRotation( DeltaTime, ViewRotation, DeltaRot );
		SetRotation(ViewRotation);

		ViewShake(DeltaTime);

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, vect(0,0,0), DCLICK_None, rot(0,0,0));
		bPressedJump = false;
	}
	function ShowScoreboard()
	{
		myHUD.bShowScores = true;
	}

	function BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);
		if ( myHUD != None )
		{
			myHUD.bShowScores = false;
			SetTimer(13, false, 'ShowScoreboard');
		}
	}

	function EndState(name NextStateName)
	{
		Super.EndState(NextStateName);
		SetBehindView(false);
		StopViewShaking();
	}
}

function ShowScoreboard();

state Dead
{
	ignores SeePlayer, HearNoise, KilledBy, NextWeapon, PrevWeapon;

	exec function SwitchWeapon(byte T){}
	exec function ToggleMelee() {}
	exec function ShowQuickPick(){}
	exec function StartFire( optional byte FireModeNum )
	{
		if ( bFrozen )
		{
			if ( !IsTimerActive() || GetTimerCount() > MinRespawnDelay )
				bFrozen = false;
			return;
		}
		if ( PlayerReplicationInfo.bOutOfLives )
			ServerSpectate();
		else
			super.StartFire( FireModeNum );
	}

	function Timer()
	{
		if (!bFrozen)
			return;

		// force garbage collection while dead, to avoid GC during gameplay
		if ( (WorldInfo.NetMode == NM_Client) || (WorldInfo.NetMode == NM_Standalone) )
		{
			WorldInfo.ForceGarbageCollection();
		}
		bFrozen = false;
		bUsePhysicsRotation = false;
		bPressedJump = false;
	}

	reliable client event ClientSetViewTarget( Actor A, optional float TransitionTime )
	{
		if( A == None )
		{
			ServerVerifyViewTarget();
			return;
		}
		// don't force view to self while dead (since server may be doing it having destroyed the pawn)
		if ( A == self )
			return;
		SetViewTarget( A, TransitionTime );
	}

	function FindGoodView()
	{
		local vector cameraLoc;
		local rotator cameraRot, ViewRotation, RealRotation;
		local int tries, besttry;
		local float bestdist, newdist, RealCameraScale;
		local int startYaw;
		local UTPawn P;

		if ( UTVehicle(ViewTarget) != None )
		{
			DesiredRotation = Rotation;
			bUsePhysicsRotation = true;
			return;
		}

		ViewRotation = Rotation;
		RealRotation = ViewRotation;
		ViewRotation.Pitch = 56000;
		SetRotation(ViewRotation);
		P = UTPawn(ViewTarget);
		if ( P != None )
		{
			RealCameraScale = P.CurrentCameraScale;
			P.CurrentCameraScale = P.CameraScale;
		}

		// use current rotation if possible
		CalcViewActor = None;
		cameraLoc = ViewTarget.Location;
		GetPlayerViewPoint( cameraLoc, cameraRot );
		if ( P != None )
		{
			newdist = VSize(cameraLoc - ViewTarget.Location);
			if (newdist < P.CylinderComponent.CollisionRadius + P.CylinderComponent.CollisionHeight )
			{
				// find alternate camera rotation
				tries = 0;
				besttry = 0;
				bestdist = 0.0;
				startYaw = ViewRotation.Yaw;

				for (tries=1; tries<16; tries++)
				{
					CalcViewActor = None;
					cameraLoc = ViewTarget.Location;
					ViewRotation.Yaw += 4096;
					SetRotation(ViewRotation);
					GetPlayerViewPoint( cameraLoc, cameraRot );
					newdist = VSize(cameraLoc - ViewTarget.Location);
					if (newdist > bestdist)
					{
						bestdist = newdist;
						besttry = tries;
					}
				}
				ViewRotation.Yaw = startYaw + besttry * 4096;
			}
			P.CurrentCameraScale = RealCameraScale;
		}
		SetRotation(RealRotation);
		DesiredRotation = ViewRotation;
		DesiredRotation.Roll = 0;
		bUsePhysicsRotation = true;
	}

	function BeginState(Name PreviousStateName)
	{
		local UTWeaponLocker WL;

		if ( Pawn(Viewtarget) != None )
		{
			SetBehindView(true);
		}
		Super.BeginState(PreviousStateName);

		if ( LocalPlayer(Player) != None )
		{
			ForEach WorldInfo.AllNavigationPoints(class'UTWeaponLocker',WL)
				WL.NotifyLocalPlayerDead(self);
		}

		if ( CurrentMapScene != none )
		{
			CurrentMapScene.SceneClient.CloseScene(CurrentMapScene);
		}

	}

	function EndState(name NextStateName)
	{
		bUsePhysicsRotation = false;
		Super.EndState(NextStateName);
		SetBehindView(false);
		StopViewShaking();
	}

Begin:
    Sleep(5.0);
	if ( (ViewTarget == None) || (ViewTarget == self) || (VSize(ViewTarget.Velocity) < 1.0) )
	{
		Sleep(1.0);
		if ( myHUD != None )
			myHUD.bShowScores = true;
	}
	else
		Goto('Begin');
}

/**
 * list important UTPlayerController variables on canvas. HUD will call DisplayDebug() on the current ViewTarget when
 * the ShowDebug exec is used
 *
 * @param	HUD		- HUD with canvas to draw on
 * @input	out_YL		- Height of the current font
 * @input	out_YPos	- Y position on Canvas. out_YPos += out_YL, gives position to draw text for next debug line.
 */
simulated function DisplayDebug(HUD HUD, out float out_YL, out float out_YPos)
{
	local Canvas Canvas;

	Canvas = HUD.Canvas;
	Canvas.SetDrawColor(255,255,255,255);

	Canvas.DrawText("CONTROLLER "$GetItemName(string(self))$" Physics "$GetPhysicsName()$" Pawn "$GetItemName(string(Pawn))$" Yaw "$Rotation.Yaw);
	out_YPos += out_YL;
	Canvas.SetPos(4, out_YPos);

	if ( Pawn == None )
	{
		if ( PlayerReplicationInfo == None )
			Canvas.DrawText("NO PLAYERREPLICATIONINFO", false);
		else
			PlayerReplicationInfo.DisplayDebug(HUD, out_YL, out_YPos);
		out_YPos += out_YL;
		Canvas.SetPos(4, out_YPos);

		super(Actor).DisplayDebug(HUD, out_YL, out_YPos);
	}
	else if (HUD.ShouldDisplayDebug('AI'))
	{
		if ( Enemy != None )
			Canvas.DrawText(" STATE: "$GetStateName()$" Timer: "$GetTimerCount()$" Enemy "$Enemy.GetHumanReadableName(), false);
		else
			Canvas.DrawText(" STATE: "$GetStateName()$" Timer: "$GetTimerCount()$" NO Enemy ", false);
		out_YPos += out_YL;
		Canvas.SetPos(4, out_YPos);
	}

	if (PlayerCamera != None && HUD.ShouldDisplayDebug('camera'))
	{
		PlayerCamera.DisplayDebug( HUD, out_YL, out_YPos );
	}
}

function Reset()
{
	Super.Reset();
	if ( PlayerCamera != None )
	{
		PlayerCamera.Destroy();
	}
}

reliable client function ClientReset()
{
	local UTGameObjective O;

	Super.ClientReset();
	if ( PlayerCamera != None )
	{
		PlayerCamera.Destroy();
	}

	foreach WorldInfo.AllNavigationPoints(class'UTGameObjective', O)
	{
		O.ClientReset();
	}
}

exec function SetRadius(float newradius)
{
	Pawn.SetCollisionSize(NewRadius,Pawn.GetCollisionHeight());
}

exec function BehindView()
{
	if ( WorldInfo.NetMode == NM_Standalone )
		SetBehindView(!bBehindView);
}

function SetBehindView(bool bNewBehindView)
{
	bBehindView = bNewBehindView;
	if ( !bBehindView )
	{
		bFreeCamera = false;
	}

	if (LocalPlayer(Player) == None)
	{
		ClientSetBehindView(bNewBehindView);
	}
	else if (UTPawn(ViewTarget) != None)
	{
		UTPawn(ViewTarget).SetThirdPersonCamera(bNewBehindView);
	}
	// make sure we recalculate camera position for this frame
	LastCameraTimeStamp = WorldInfo.TimeSeconds - 1.0;
}

reliable client function ClientSetBehindView(bool bNewBehindView)
{
	if (LocalPlayer(Player) != None)
	{
		SetBehindView(bNewBehindView);
	}
	// make sure we recalculate camera position for this frame
	LastCameraTimeStamp = WorldInfo.TimeSeconds - 1.0;
}

/**
 * Set new camera mode
 *
 * @param	NewCamMode, new camera mode.
 */
function SetCameraMode( name NewCamMode )
{
	// will get set back to true below, if necessary
	bDebugFreeCam = FALSE;

	if ( PlayerCamera != None )
	{
		Super.SetCameraMode(NewCamMode);
	}
	else if ( NewCamMode == 'ThirdPerson' )
	{
		if ( !bBehindView )
			SetBehindView(true);
	}
	else if ( NewCamMode == 'FreeCam' )
	{
		if ( !bBehindView )
		{
			SetBehindView(true);
		}
		bDebugFreeCam = TRUE;
		DebugFreeCamRot = Rotation;
	}
	else
	{
		if ( bBehindView )
			SetBehindView(false);
	}
}

/**
 *	Calculate camera view point, when viewing this actor.
 *
 * @param	fDeltaTime	delta time seconds since last update
 * @param	out_CamLoc	Camera Location
 * @param	out_CamRot	Camera Rotation
 * @param	out_FOV		Field of View
 *
 * @return	true if Actor should provide the camera point of view.
 */
simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	out_CamLoc = Location + (CylinderComponent(CollisionComponent).CollisionHeight * Vect(0,0,1));
	out_CamRot = Rotation;
	return false;
}

function SpawnCamera()
{
	local Actor OldViewTarget;

	// Associate Camera with PlayerController
	PlayerCamera = Spawn(MatineeCameraClass, self);
	if (PlayerCamera != None)
	{
		OldViewTarget = ViewTarget;
		PlayerCamera.InitializeFor(self);
		PlayerCamera.SetViewTarget(OldViewTarget);
	}
	else
	{
		`Log("Couldn't Spawn Camera Actor for Player!!");
	}
}

/* GetPlayerViewPoint: Returns Player's Point of View
	For the AI this means the Pawn's Eyes ViewPoint
	For a Human player, this means the Camera's ViewPoint */
simulated event GetPlayerViewPoint( out vector POVLocation, out Rotator POVRotation )
{
	local float DeltaTime;

	if (LastCameraTimeStamp == WorldInfo.TimeSeconds
		&& CalcViewActor == ViewTarget
		&& CalcViewActor.Location == CalcViewActorLocation
		&& CalcViewActor.Rotation == CalcViewActorRotation
		)
	{
		// use cached result
		POVLocation = CalcViewLocation;
		POVRotation = CalcViewRotation;
		return;
	}

	DeltaTime = WorldInfo.TimeSeconds - LastCameraTimeStamp;
	LastCameraTimeStamp = WorldInfo.TimeSeconds;

	// support for using CameraActor views
	if ( CameraActor(ViewTarget) != None )
	{
		if ( PlayerCamera == None )
		{
			super.ResetCameraMode();
			SpawnCamera();
		}
		super.GetPlayerViewPoint( POVLocation, POVRotation );
	}
	else
	{
		if ( PlayerCamera != None )
		{
			PlayerCamera.Destroy();
			PlayerCamera = None;
		}

		if ( ViewTarget != None )
		{
			POVRotation = Rotation;
			ViewTarget.CalcCamera( DeltaTime, POVLocation, POVRotation, FOVAngle );
			if ( bFreeCamera )
			{
				POVRotation = Rotation;
			}
		}
		else
		{
			CalcCamera( DeltaTime, POVLocation, POVRotation, FOVAngle );
		}
	}


	// apply view shake
	POVRotation = Normalize(POVRotation + ShakeRot);
	POVLocation += ShakeOffset >> Rotation;

	if( CameraEffect != none )
	{
		CameraEffect.UpdateLocation(POVLocation, POVRotation, GetFOVAngle());
	}


	// cache result
	CalcViewActor = ViewTarget;
	CalcViewActorLocation = ViewTarget.Location;
	CalcViewActorRotation = ViewTarget.Rotation;
	CalcViewLocation = POVLocation;
	CalcViewRotation = POVRotation;
}


unreliable client function ClientMusicEvent(int EventIndex)
{
	if ( MusicManager != None )
		MusicManager.MusicEvent(EventIndex);
}

/**
  * return true if music manager is already playing action track
  * return true if no music manager (no need to tell non-existent music manager to change tracks
  */
function bool AlreadyInActionMusic()
{
	return (MusicManager != None) ? MusicManager.AlreadyInActionMusic() : true;
}

exec function Music(int EventIndex)
{
	MusicManager.MusicEvent(EventIndex);
}

reliable client function ClientPlayAnnouncement(class<UTLocalMessage> InMessageClass, byte MessageIndex, optional Object OptionalObject)
{
	PlayAnnouncement(InMessageClass, MessageIndex, OptionalObject);
}

function PlayAnnouncement(class<UTLocalMessage> InMessageClass, byte MessageIndex, optional Object OptionalObject)
{
	// Wait for player to be up to date with replication when joining a server, before stacking up messages
	if ( (WorldInfo.GRI == None) || (Announcer == None) )
		return;

	Announcer.PlayAnnouncement(InMessageClass, MessageIndex, OptionalObject);
}

function bool AllowVoiceMessage(name MessageType)
{
	if ( WorldInfo.NetMode == NM_Standalone )
		return true;
	/* //@todo steve
	if ( WorldInfo.TimeSeconds - OldMessageTime < 3 )
	{
		if ( (MessageType == 'TAUNT') || (MessageType == 'AUTOTAUNT') )
			return false;
		if ( WorldInfo.TimeSeconds - OldMessageTime < 1 )
			return false;
	}
	if ( WorldInfo.TimeSeconds - OldMessageTime < 6 )
		OldMessageTime = WorldInfo.TimeSeconds + 3;
	else
		OldMessageTime = WorldInfo.TimeSeconds;
	*/
	return true;
}

/** Causes a view shake based on the amount of damage
	Should only be called on the owning client */
function DamageShake(int Damage, class<DamageType> DamageType)
{
	local float BlendWeight;
	local class<UTDamageType> UTDamage;
	local CameraAnim AnimToPlay;

	UTDamage = class<UTDamageType>(DamageType);
	if (UTDamage != None && UTDamage.default.DamageCameraAnim != None)
	{
		AnimToPlay = UTDamage.default.DamageCameraAnim;
	}
	else
	{
		AnimToPlay = DamageCameraAnim;
	}
	if (AnimToPlay != None)
	{
		// don't override other anims unless it's another, weaker damage anim
		BlendWeight = FClamp(Damage / 200.0, 0.0, 1.0);
		if ( CameraAnimPlayer != None && ( CameraAnimPlayer.bFinished ||
						(bCurrentCamAnimIsDamageShake && CameraAnimPlayer.CurrentBlendWeight < BlendWeight) ) )
		{
			PlayCameraAnim(AnimToPlay, BlendWeight,,,,, true);
		}
	}
}

/** Shakes the player's view with the given parameters
	Should only be called on the owning client */
function ShakeView(ViewShakeInfo NewViewShake)
{
	// apply the new rotation shaking only if it is larger than the current amount
	if (VSize(NewViewShake.RotMag) > VSize(CurrentViewShake.RotMag))
	{
		CurrentViewShake.RotMag = NewViewShake.RotMag;
		CurrentViewShake.RotRate = NewViewShake.RotRate;
		CurrentViewShake.RotTime = NewViewShake.RotTime;
	}
	// apply the new offset shaking only if it is larger than the current amount
	if (VSize(NewViewShake.OffsetMag) > VSize(CurrentViewShake.OffsetMag))
	{
		CurrentViewShake.OffsetMag = NewViewShake.OffsetMag;
		CurrentViewShake.OffsetRate = NewViewShake.OffsetRate;
		CurrentViewShake.OffsetTime = NewViewShake.OffsetTime;
	}
}

function OnCameraShake(UTSeqAct_CameraShake ShakeAction)
{
	ShakeView(ShakeAction.CameraShake);
}

/** Turns off any view shaking */
function StopViewShaking()
{
/** @FIXME: remove if we keep new camera anim system for viewshakes
	local ViewShakeInfo EmptyViewShake;
	CurrentViewShake = EmptyViewShake;
*/
	if (CameraAnimPlayer != None)
	{
		CameraAnimPlayer.Stop();
	}
}

//@FIXME: remove these two if we keep new camera anim system for viewshakes
final native function CheckShake(out float MaxOffset, out float Offset, out float Rate, float Time);

final native function UpdateShakeRotComponent(out float Max, out int Current, out float Rate, float Time, float DeltaTime);

/** plays the specified camera animation with the specified weight (0 to 1)
 * local client only
 */
function PlayCameraAnim( CameraAnim AnimToPlay, optional float Scale=1.f, optional float Rate=1.f,
			optional float BlendInTime, optional float BlendOutTime, optional bool bLoop, optional bool bIsDamageShake )
{
	local AnimatedCamera MatineeAnimatedCam;

	MatineeAnimatedCam = AnimatedCamera(PlayerCamera);

	// if we have a real camera, e.g we're watching through a matinee camera,
	// send the CameraAnim to be played there
	if (MatineeAnimatedCam != None)
	{
		MatineeAnimatedCam.PlayCameraAnim(AnimToPlay, Rate, Scale, BlendInTime, BlendOutTime, bLoop, FALSE);
	}
	else if (CameraAnimPlayer != None)
	{
		// play through normal UT camera
		CamOverridePostProcess = class'CameraActor'.default.CamOverridePostProcess;
		CameraAnimPlayer.Play(AnimToPlay, self, Rate, Scale, BlendInTime, BlendOutTime, bLoop, false);
	}
	bCurrentCamAnimIsDamageShake = bIsDamageShake;
}

/** Stops the currently playing camera animation. */
function StopCameraAnim(optional bool bImmediate)
{
	CameraAnimPlayer.Stop(bImmediate);
}

reliable client function ClientPlayCameraAnim( CameraAnim AnimToPlay, optional float Scale=1.f, optional float Rate=1.f,
						optional float BlendInTime, optional float BlendOutTime, optional bool bLoop)
{
	PlayCameraAnim(AnimToPlay, Scale, Rate, BlendInTime, BlendOutTime, bLoop);
}

reliable client function ClientStopCameraAnim(bool bImmediate)
{
	StopCameraAnim(bImmediate);
}

function OnPlayCameraAnim(UTSeqAct_PlayCameraAnim InAction)
{
	ClientPlayCameraAnim(InAction.AnimToPlay, InAction.IntensityScale, InAction.Rate, InAction.BlendInTime, InAction.BlendOutTime);
}

function OnStopCameraAnim(UTSeqAct_StopCameraAnim InAction)
{
	ClientStopCameraAnim(InAction.bStopImmediately);
}


/** Sets ShakeOffset and ShakeRot to the current view shake that should be applied to the camera */
function ViewShake(float DeltaTime)
{
	if (CameraAnimPlayer != None && !CameraAnimPlayer.bFinished)
	{
		// advance the camera anim - the native code will set ShakeOffset/ShakeRot appropriately
		CamOverridePostProcess = class'CameraActor'.default.CamOverridePostProcess;
		CameraAnimPlayer.AdvanceAnim(DeltaTime, false);
	}
	else
	{
		ShakeOffset = vect(0,0,0);
		ShakeRot = rot(0,0,0);
	}

	/** @FIXME: remove if we keep new camera anim system for viewshakes
	if (!IsZero(CurrentViewShake.OffsetRate))
	{
		// modify shake offset
		ShakeOffset.X += DeltaTime * CurrentViewShake.OffsetRate.X;
		CheckShake(CurrentViewShake.OffsetMag.X, ShakeOffset.X, CurrentViewShake.OffsetRate.X, CurrentViewShake.OffsetTime);

		ShakeOffset.Y += DeltaTime * CurrentViewShake.OffsetRate.Y;
		CheckShake(CurrentViewShake.OffsetMag.Y, ShakeOffset.Y, CurrentViewShake.OffsetRate.Y, CurrentViewShake.OffsetTime);

		ShakeOffset.Z += DeltaTime * CurrentViewShake.OffsetRate.Z;
		CheckShake(CurrentViewShake.OffsetMag.Z, ShakeOffset.Z, CurrentViewShake.OffsetRate.Z, CurrentViewShake.OffsetTime);

		CurrentViewShake.OffsetTime -= DeltaTime;
	}

	if (!IsZero(CurrentViewShake.RotRate))
	{
		UpdateShakeRotComponent(CurrentViewShake.RotMag.X, ShakeRot.Pitch, CurrentViewShake.RotRate.X, CurrentViewShake.RotTime, DeltaTime);
		UpdateShakeRotComponent(CurrentViewShake.RotMag.Y, ShakeRot.Yaw, CurrentViewShake.RotRate.Y, CurrentViewShake.RotTime, DeltaTime);
		UpdateShakeRotComponent(CurrentViewShake.RotMag.Z, ShakeRot.Roll, CurrentViewShake.RotRate.Z, CurrentViewShake.RotTime, DeltaTime);
		CurrentViewShake.RotTime -= DeltaTime;
	}
	*/
}

simulated exec function ToggleMelee()
{
	if ( (Pawn != None) && (Pawn.Weapon != None) && Vehicle(Pawn) == none)
	{
		if ( Pawn.Weapon.bMeleeWeapon )
		{
			UTInventoryManager(Pawn.InvManager).SwitchToPreviousWeapon();
		}
		else
		{
			SwitchWeapon(1);
		}
	}
}

simulated exec function ToggleTranslocator()
{
	if ( Pawn != None )
	{
		if ( UTWeap_Translocator(Pawn.Weapon) != None )
		{
			UTInventoryManager(Pawn.InvManager).SwitchToPreviousWeapon();
		}
		else
		{
			SwitchWeapon(0);
		}
	}
}

//=====================================================================
// UT specific implementation of networked player movement functions
//

function CallServerMove
(
	SavedMove NewMove,
    vector ClientLoc,
    byte ClientRoll,
    int View,
    SavedMove OldMove
)
{
	local vector BuildAccel;
	local byte OldAccelX, OldAccelY, OldAccelZ;

	// compress old move if it exists
	if ( OldMove != None )
	{
		// old move important to replicate redundantly
		BuildAccel = 0.05 * OldMove.Acceleration + vect(0.5, 0.5, 0.5);
		OldAccelX = CompressAccel(BuildAccel.X);
		OldAccelY = CompressAccel(BuildAccel.Y);
		OldAccelZ = CompressAccel(BuildAccel.Z);

		OldServerMove(OldMove.TimeStamp,OldAccelX, OldAccelY, OldAccelZ, OldMove.CompressedFlags());
	}

	if ( PendingMove != None )
	{
		DualServerMove
		(
			PendingMove.TimeStamp,
			PendingMove.Acceleration * 10,
			PendingMove.CompressedFlags(),
			((PendingMove.Rotation.Yaw & 65535) << 16) + (PendingMove.Rotation.Pitch & 65535),
			NewMove.TimeStamp,
			NewMove.Acceleration * 10,
			ClientLoc,
			NewMove.CompressedFlags(),
			ClientRoll,
			View
		);
	}
    else if ( (NewMove.Acceleration * 10 == vect(0,0,0)) && (NewMove.DoubleClickMove == DCLICK_None) && !NewMove.bDoubleJump )
    {
		ShortServerMove
		(
			NewMove.TimeStamp,
			ClientLoc,
			NewMove.CompressedFlags(),
			ClientRoll,
			View
		);
    }
    else
		ServerMove
	(
	    NewMove.TimeStamp,
	    NewMove.Acceleration * 10,
	    ClientLoc,
			NewMove.CompressedFlags(),
	    ClientRoll,
	    View
	);
}

/* ShortServerMove()
compressed version of server move for bandwidth saving
*/
unreliable server function ShortServerMove
(
	float TimeStamp,
	vector ClientLoc,
	byte NewFlags,
	byte ClientRoll,
	int View
)
{
    ServerMove(TimeStamp,vect(0,0,0),ClientLoc,NewFlags,ClientRoll,View);
}

unreliable client function LongClientAdjustPosition( float TimeStamp, name NewState, EPhysics NewPhysics,
					float NewLocX, float NewLocY, float NewLocZ,
					float NewVelX, float NewVelY, float NewVelZ, Actor NewBase,
					float NewFloorX, float NewFloorY, float NewFloorZ )
{
	local UTPawn P;

	Super.LongClientAdjustPosition( TimeStamp, NewState, NewPhysics, NewLocX, NewLocY, NewLocZ,
					NewVelX, NewVelY, NewVelZ, NewBase, NewFloorX, NewFloorY, NewFloorZ );

	// allow changing location of rigid body pawn if feigning death
	P = UTPawn(Pawn);
	if (P != None && P.bFeigningDeath && P.Physics == PHYS_RigidBody)
	{
		// the actor's location (and thus the mesh) were moved in the Super call, so we just need
		// to tell the physics system to do the same
		P.Mesh.SetRBPosition(P.Mesh.GetPosition());
	}
}

auto state PlayerWaiting
{
	exec function StartFire( optional byte FireModeNum )
	{
/* //@todo steve - load skins that haven't been precached (because players joined since game start)
	LoadPlayers();
	if ( !bForcePrecache && (WorldInfo.TimeSeconds > 0.2) )
*/
		ServerReStartPlayer();
	}

    reliable server function ServerRestartPlayer()
	{
		super.ServerRestartPlayer();
		if ( WorldInfo.Game.bWaitingToStartMatch && UTGame(WorldInfo.Game).bWarmupRound && UTGame(WorldInfo.Game).WarmupTime>1.0 )
			WorldInfo.Game.RestartPlayer(self);
	}

	exec function ShowQuickPick(){}


}

function ViewNextBot()
{
	if ( CheatManager != None )
		CheatManager.ViewBot();
}

state BaseSpectating
{
	exec function SwitchWeapon(byte F){}
	exec function ShowQuickPick(){}

	simulated function BeginState(name PrevStateName)
	{
		Super.BeginState(PrevStateName);
		if ( CurrentMapScene != none )
		{
			CurrentMapScene.SceneClient.CloseScene(CurrentMapScene);
		}
	}

	simulated event GetPlayerViewPoint( out vector out_Location, out Rotator out_Rotation )
	{
		if ( PlayerCamera == None )
		{
			out_Location = Location;
			out_Rotation = BlendedTargetViewRotation;
		}
		else
			Global.GetPlayerViewPoint(out_Location, out_Rotation);
	}
}


exec function SwitchWeapon(byte T)
{
	if (UTPawn(Pawn) != None)
		UTPawn(Pawn).SwitchWeapon(t);
	else if (UTVehicleBase(Pawn) != none)
		UTVehicleBase(Pawn).SwitchWeapon(t);
}

unreliable server function ServerViewSelf()
{
	local rotator POVRotation;
	local vector POVLocation;

	GetPlayerViewPoint( POVLocation, POVRotation );
	SetLocation(POVLocation);
	SetRotation(POVRotation);
	SetBehindView(false);
	SetViewTarget( Self );
}

exec function ViewPlayerByName(string PlayerName);

unreliable server function ServerViewPlayerByName(string PlayerName)
{
	local int i;
	for (i=0;i<WorldInfo.GRI.PRIArray.Length;i++)
	{
		if (WorldInfo.GRI.PRIArray[i].PlayerName ~= PlayerName)
		{
			if ( WorldInfo.Game.CanSpectate(self, WorldInfo.GRI.PRIArray[i]) )
			{
				SetViewTarget(WorldInfo.GRI.PRIArray[i]);
			}
			return;
		}
	}

	ClientMessage(MsgPlayerNotFound);
}

exec function ViewObjective()
{
	ServerViewObjective();
}

unreliable server function ServerViewObjective()
{
	if ( UTGame(WorldInfo.Game) != none )
		UTGame(WorldInfo.Game).ViewObjective(self);
}

exec function PrevWeapon()
{
	if (Pawn == None || Vehicle(Pawn) != None)
	{
		AdjustCameraScale(true);
	}
	else if (!Pawn.IsInState('FeigningDeath'))
	{
		Super.PrevWeapon();
	}
}

exec function NextWeapon()
{
	if (Pawn == None || Vehicle(Pawn) != None)
	{
		AdjustCameraScale(false);
	}
	else if (!Pawn.IsInState('FeigningDeath'))
	{
		Super.NextWeapon();
	}
}

/** moves the camera in or out */
exec function AdjustCameraScale(bool bIn)
{
	if (Pawn(ViewTarget) != None)
	{
		Pawn(ViewTarget).AdjustCameraScale(bIn);
	}
}

state Spectating
{
	function BeginState(Name PreviousStateName)
	{
		super.BeginState(PreviousStateName);

		// Ugly hack to change to follow bots around after they are spawned if automated perf testing is enabled.
		// This should be replaced with a more robust solution.
		if( UTGame(WorldInfo.Game)!=None && UTGame(WorldInfo.Game).bAutomatedPerfTesting )
		{
			SetTimer( 3.0f, true, 'ServerViewNextPlayer');
		}
	}

	function EndState(Name NextStateName)
	{
		super.EndState(NextStateName);
		// Reset timer set in BeginState.
		ClearTimer( 'ServerViewNextPlayer' );
	}

	exec function ViewPlayerByName(string PlayerName)
	{
		ServerViewPlayerByName(PlayerName);
	}

	exec function BehindView()
	{
		bForceBehindView = !bForceBehindview;
	}

	/**
	 * The Prev/Next weapon functions are used to move forward and backwards through the player list
	 */

	exec function PrevWeapon()
	{
		ServerViewPrevPlayer();
	}

	exec function NextWeapon()
	{
		ServerViewNextPlayer();
	}

	/**
	 * Fire will select the next/prev objective
	 */
	exec function StartFire( optional byte FireModeNum )
	{
		ServerViewObjective();
	}

	/**
	 * AltFire - Resets to Free Camera Mode
	 */
	exec function StartAltFire( optional byte FireModeNum )
	{
		ServerViewSelf();
	}

	/**
	 * Handle forcing behindview/etc
	 */
	simulated event GetPlayerViewPoint( out vector out_Location, out Rotator out_Rotation )
	{
		// Force first person mode if we're performing automated perf testing.
		if( UTGame(WorldInfo.Game)!=None && UTGame(WorldInfo.Game).bAutomatedPerfTesting )
		{
			SetBehindView(false);
		}
		else if (bBehindview != bForceBehindView && UTPawn(ViewTarget)!=None)
		{
			SetBehindView(bForceBehindView);
		}
		Global.GetPlayerViewPoint(out_Location, out_Rotation);
	}
}

/**
 * This state is used when the player is out of the match waiting to be brought back in
 */
state InQueue extends Spectating
{
	function BeginState(Name PreviousStateName)
	{
		super.BeginState(PreviousStateName);
		PlayerReplicationInfo.bIsSpectator = true;
	}

	function EndState(Name NextStateName)
	{
		super.EndState(NextStateName);
		ServerViewSelf();
	}
}

exec function TestMenu(string NewMenu)
{
	local class<UIScene> SceneClass;
	local UIInteraction UIController;

	UIController = LocalPlayer(Player).ViewportClient.UIController;

	`log("#### TestMenu:"@NewMenu);
	if( LocalPlayer(Player) != None)
	{
		SceneClass = class<UIScene> ( DynamicLoadObject(NewMenu, class'Class'));
		`log("### SceneClass:"@SceneClass);
		UIController.SceneClient.CreateMenu(SceneClass, );
	}
}

exec function Talk()
{
	local Console Console;
	if(LocalPlayer(Player) != None)
	{
		Console = LocalPlayer(Player).ViewportClient.ViewportConsole;
		Console.StartTyping("Say ");
	}
}

exec function TeamTalk()
{
	local Console Console;
	if(LocalPlayer(Player) != None && LocalPlayer(Player).ViewportClient.ViewportConsole != None)
	{
		Console = LocalPlayer(Player).ViewportClient.ViewportConsole;
		Console.StartTyping("TeamSay ");
	}
}

exec function ShowMap()
{
	local UTUIScene_OnsMapMenu MapMenu;
	ShowHudMap();
	if ( CurrentMapScene != none )
	{
		MapMenu = UTUIScene_OnsMapMenu(CurrentMapScene);
		if ( MapMenu != none )
		{
//			MapMenu.DisableTeleport();
		}
	}
}

server reliable function ServerViewingMap(bool bNewValue)
{
	bViewingMap = bNewValue;
}

client reliable function ShowHudMap()
{
	local UTGameReplicationInfo UTGRI;
	local UIScene Scene;

	UTGRI = UTGameReplicationInfo( WorldInfo.GRI );
	if (CurrentMapScene == none && UTGRI != none && UTGRI.MapMenuTemplate != none )
	{
		Scene = OpenUIScene(UTGRI.MapMenuTemplate);
		if ( Scene != none )
		{
			CurrentMapScene = UTUIScene(Scene);
			ServerViewingMap(true);
			CurrentMapScene.OnSceneDeactivated = HudMapClosed;
		}
	}
}

/**
 * Clean up
 */
function HudMapClosed( UIScene DeactivatedScene )
{
	ServerViewingMap(false);
	if ( DeactivatedScene == CurrentMapScene )
	{
		CurrentMapScene = none;
	}
}

exec function ShowMenu()
{
	local UIScene Scene;
	local UTUIScene Template;
	local class<UTGame> UTGameClass;

	UTGameClass = class<UTGame>(WorldInfo.GetGameClass());
	if (UTGameClass == none)
	{
		return;
	}

	Template = UTGameClass.Default.MidGameMenuTemplate;

	if (CurrentMapScene == none && Template != none )
	{
		Scene = OpenUIScene(Template);
		if ( Scene != none )
		{
			CurrentMidGameMenu = UTUIScene(Scene);
			ServerViewingMap(true); // @TODO FIXMESTEVE - only if gametype has map
			CurrentMidGameMenu.OnSceneDeactivated = MidGameMenuClosed;
			MyHud.bShowHud = false;

			if ( WorldInfo != none && WorldInfo.Game != none )
			{
			 	WorldInfo.Game.SetPause(self);
			}

		}
	}
}


/**
 * Clean up
 */
function MidGameMenuClosed( UIScene DeactivatedScene )
{
	ServerViewingMap(false);
	if ( DeactivatedScene == CurrentMidGameMenu )
	{
		CurrentMidGameMenu = none;
	}
	MyHud.bShowHud = true;

	if ( WorldInfo != none && WorldInfo.Game != none )
	{
		WorldInfo.Game.ClearPause();
	}


}

/* epic ===============================================
* ::ClientGameEnded
*
* Replicated equivalent to GameHasEnded().
*
 * @param	EndGameFocus - actor to view with camera
 * @param	bIsWinner - true if this controller is on winning team
* =====================================================
*/
reliable client function ClientGameEnded(optional Actor EndGameFocus, optional bool bIsWinner)
{
	if( EndGameFocus == None )
		ServerVerifyViewTarget();
	else
	{
		SetViewTarget(EndGameFocus);
	}

	SetBehindView(true);

	if ( !PlayerReplicationInfo.bOnlySpectator )
		PlayWinMessage( bIsWinner );

	ClientEndZoom();

	GotoState('RoundEnded');
}

/* epic ===============================================
* ::RoundHasEnded
*
 * @param	EndRoundFocus - actor to view with camera
* =====================================================
*/
function RoundHasEnded(optional Actor EndRoundFocus)
{
	SetViewTarget(EndRoundFocus);
	ClientRoundEnded(EndRoundFocus);
	GotoState('RoundEnded');
}

/* epic ===============================================
* ::ClientRoundEnded
*
 * @param	EndRoundFocus - actor to view with camera
* =====================================================
*/
reliable client function ClientRoundEnded(Actor EndRoundFocus)
{
	if( EndRoundFocus == None )
		ServerVerifyViewTarget();
	else
	{
		SetViewTarget(EndRoundFocus);
	}
	SetBehindView(true);
	ClientEndZoom();

	GotoState('RoundEnded');
}

/* epic ===============================================
* ::CheckBulletWhip
*
 * @param	BulletWhip - whip sound to play
 * @param	FireLocation - where shot was fired
 * @param	FireDir	- direction shot was fired
 * @param	HitLocation - impact location of shot
* =====================================================
*/
function CheckBulletWhip(soundcue BulletWhip, vector FireLocation, vector FireDir, vector HitLocation)
{
	local vector PlayerDir;
	local float Dist, PawnDist;

	if ( ViewTarget != None  )
	{
		// if bullet passed by close enough, play sound
		// first check if bullet passed by at all
		PlayerDir = ViewTarget.Location - FireLocation;
		Dist = PlayerDir Dot FireDir;
		if ( (Dist > 0) && ((FireDir Dot (HitLocation - ViewTarget.Location)) > 0) )
		{
			// check distance from bullet to vector
			PawnDist = VSize(PlayerDir);
			if ( Square(PawnDist) - Square(Dist) < 40000 )
			{
				// check line of sight
				if ( FastTrace(ViewTarget.Location + class'UTPawn'.default.BaseEyeheight*vect(0,0,1), FireLocation + Dist*FireDir) )
				{
					PlaySound(BulletWhip, true,,, HitLocation);
				}
			}
		}
	}
}
/* epic ===============================================
* ::PawnDied - Called when a pawn dies
*
 * @param	P - The pawn that died
* =====================================================
*/

function PawnDied(Pawn P)
{
	Super.PawnDied(P);
	ClientPawnDied();
}

/**
 * Client-side notification that the pawn has died.
 */

reliable client simulated function ClientPawnDied()
{
	// Unduck if ducking

	if (UTPlayerInput(PlayerInput) != none)
	{
		UTPlayerInput(PlayerInput).bDuck = 0;
	}

	// End Zooming

	EndZoom();
}

/** shows the path to the specified team's base or other main objective */
exec function BasePath(byte Num)
{
	if (PlayerReplicationInfo.Team != None)
	{
		ServerShowPathToBase(num);
	}
}

reliable server function ServerShowPathToBase(byte TeamNum)
{
	if (Pawn != None && WorldInfo.TimeSeconds - LastShowPathTime > 0.5)
	{
		LastShowPathTime = WorldInfo.TimeSeconds;

		UTGame(WorldInfo.Game).ShowPathTo(self, TeamNum);
	}
}

//spectating player wants to become active and join the game
reliable server function ServerBecomeActivePlayer()
{
	local UTGame Game;

	Game = UTGame(WorldInfo.Game);
	if (PlayerReplicationInfo.bOnlySpectator && Game != None && Game.AllowBecomeActivePlayer(self))
	{
		SetBehindView(false);
		FixFOV();
		ServerViewSelf();
		PlayerReplicationInfo.bOnlySpectator = false;
		Game.NumSpectators--;
		Game.NumPlayers++;
		PlayerReplicationInfo.Reset();
		BroadcastLocalizedMessage(Game.GameMessageClass, 1, PlayerReplicationInfo);
		if (Game.bTeamGame)
		{
			//@FIXME: get team preference!
			//Game.ChangeTeam(self, Game.PickTeam(int(GetURLOption("Team")), None), false);

			Game.ChangeTeam(self, Game.PickTeam(0, None), false);
		}
		if (!Game.bDelayedStart)
		{
			// start match, or let player enter, immediately
			Game.bRestartLevel = false;  // let player spawn once in levels that must be restarted after every death
			if (Game.bWaitingToStartMatch)
			{
				Game.StartMatch();
			}
			else
			{
				Game.RestartPlayer(self);
			}
			Game.bRestartLevel = Game.Default.bRestartLevel;
		}
		else
		{
			GotoState('PlayerWaiting');
		}

		ClientBecameActivePlayer();
	}
}

exec function Test()
{
	ServerBecomeActivePlayer();
}

reliable client function ClientBecameActivePlayer()
{
	UpdateURL("SpectatorOnly", "", false);
}

exec function WeaponTest()
{
	`log("############ Weapon Debug #################");

	WeaponInfo(Pawn.Weapon);

	`log("############ Weapon Debug #################");

	if (Role < ROLE_Authority)
	{
		ServerWeaponTest();
	}
}

server reliable function ServerWeaponTest()
{
	`log("############ Weapon Debug #################");

    Worldinfo.Game.KillBots();
	WeaponInfo(Pawn.Weapon);

	`log("############ Weapon Debug #################");

}
simulated function WeaponInfo(Weapon Weap)
{
	local Array<String>	DebugInfo;
	local int			i;

	if (Weap != none )
	{

		Weap.GetWeaponDebug( DebugInfo );
		for (i=0;i<DebugInfo.Length;i++)
		{
			`log("####"@Weap@":"@DebugInfo[i]);
		}

		if ( UTWeapon(Weap) != none )
		{
			UTWeapon(Weap).bDebugWeapon = !UTWeapon(Weap).bDebugWeapon;
			`log("####"@Weap@UTWeapon(Weap).bDebugWeapon);
		}
	}
}

exec function FlushDebug()
{
	FlushPersistentDebugLines();
}

exec function VehicleEvent(name EventName)
{
	local utvehicle v;
	local vector hl,hn,s,e;
	local rotator r;
	local actor ha;
	local int i,j;
	local string str;
	local array<string> EventList;

	GetPlayerViewPoint( s,r );
	e = s + ( Vector(r) * 8096);

	ha = Trace(hl,hn,e,s,true,vect(0,0,0));

	if (ha != none && UTVehicle(ha) != none )
	{
		v = UTVehicle(HA);

		if (EventName == '')
		{
			for (i=0;i<V.VehicleEffects.Length;i++)
			{
				str = "Effect Start Trigger:"@V.VehicleEffects[i].EffectStartTag;
				if ( EventList.Find(str) == INDEX_NONE )
				{
					EventList[EventList.Length] = Str;
				}
				str = "Effect end Trigger  :"@V.VehicleEffects[i].EffectEndTag;
				if ( EventList.Find(str) == INDEX_NONE )
				{
					EventList[EventList.Length] = Str;
				}
			}
			for (i=0;i<V.VehicleAnims.Length;i++)
			{
				str = "Anim Start Trigger:  "@V.VehicleAnims[i].AnimTag;
				if ( EventList.Find(str) == INDEX_NONE )
				{
					EventList[EventList.Length] = Str;
				}
			}

			for (i=0;i<V.Seats.Length;i++)
			{
				if ( V.Seats[i].Gun != none )
				{
					for (j=0;j<V.Seats[i].Gun.FireTriggerTags.Length;j++)
					{
						str = "Fire Tag            :"@V.Seats[i].Gun.FireTriggerTags[j];
						if ( EventList.Find(str) == INDEX_NONE )
						{
							EventList[EventList.Length] = Str;
						}
					}
					for (j=0;j<V.Seats[i].Gun.AltFireTriggerTags.Length;j++)
					{
						str = "Alt Fire Tag        :"@V.Seats[i].Gun.AltFireTriggerTags[j];
						if ( EventList.Find(str) == INDEX_NONE )
						{
							EventList[EventList.Length] = Str;
						}
					}
				}
			}

			`Log("### Vehicle "$V$" Event List");
			`log("###");
			for (i=0;i<EventList.Length;i++)
			{
				`log(EventList[i]);
			}
			`log("########################################################");
		}
		else
		{
			V.VehicleEvent(EventName);
		}

	}
}

/*********************************************************************************************
 * Zooming Functions
 *********************************************************************************************/

/**
 * Called each frame from PlayerTick this function is used to transition towards the DesiredFOV
 * if not already at it.
 *
 * @Param	DeltaTime 	-	Time since last update
 */
function AdjustFOV(float DeltaTime)
{
	local float DeltaFOV;

	if ( FOVAngle != DesiredFOV )
	{
		if (bNonlinearZoomInterpolation)
		{
			// do nonlinear interpolation
			FOVAngle = FInterpTo(FOVAngle, DesiredFOV, DeltaTime, FOVNonlinearZoomInterpSpeed);
		}
		else
		{
			// do linear interpolation
			if ( FOVLinearZoomRate > 0.0 )
			{
				DeltaFOV = FOVLinearZoomRate * DeltaTime;

				if (FOVAngle > DesiredFOV)
				{
					FOVAngle = FMax( DesiredFOV, (FOVAngle - DeltaFOV) );
				}
				else
				{
					FOVAngle = FMin( DesiredFOV, (FOVAngle + DeltaFOV) );
				}
			}
			else
			{
				FOVAngle = DesiredFOV;
			}
		}
	}
}

/**
 * This function will cause the PlayerController to begin zooming to a new FOV Level.
 *
 * @Param	NewDesiredFOV		-	The new FOV Value to head towards
 * @Param	NewZoomRate			- 	The rate of transition in degrees per second
 */

simulated function StartZoom(float NewDesiredFOV, float NewZoomRate)
{
	FOVLinearZoomRate = NewZoomRate;
	DesiredFOV = NewDesiredFOV;

	// clear out any nonlinear zoom info
	bNonlinearZoomInterpolation = FALSE;
	FOVNonlinearZoomInterpSpeed = 0.f;
}

/*
 * @Param	bNonlinearInterp	-	TRUE to use FInterpTo, which provides for a nonlinear interpolation with a decelerating arrival characteristic.
 *									FALSE for Linear interpolation.
 */
simulated function StartZoomNonlinear(float NewDesiredFOV, float NewZoomInterpSpeed)
{
	DesiredFOV = NewDesiredFOV;
	FOVNonlinearZoomInterpSpeed = NewZoomInterpSpeed;

	// clear out any linear zoom info
	bNonlinearZoomInterpolation = TRUE;
	FOVLinearZoomRate = 0.f;
}


/**
 * This function will stop the zooming process keeping the current FOV Angle
 */
simulated function StopZoom()
{
	DesiredFOV = FOVAngle;
	FOVLinearZoomRate = 0.0f;
}

/**
 * This function will end a zoom and reset the FOV to the default value
 */

simulated function EndZoom()
{
	DesiredFOV = DefaultFOV;
	FOVAngle = DefaultFOV;
	FOVLinearZoomRate = 0.0f;
	FOVNonlinearZoomInterpSpeed = 0.f;
}

/** Ends a zoom, but interpolates nonlinearly back to the default value. */
simulated function EndZoomNonlinear(float ZoomInterpSpeed)
{
	DesiredFOV = DefaultFOV;
	FOVNonlinearZoomInterpSpeed = ZoomInterpSpeed;

	// clear out any linear zoom info
	bNonlinearZoomInterpolation = TRUE;
	FOVLinearZoomRate = 0.f;
}

/**
 * Allows the server to tell the client to end zoom
 */
reliable simulated client function ClientEndZoom()
{
	EndZoom();
}

/**
 * We override ProcessViewRotation so that we can lower the DeltaRot sensitivity when we are zoomed.
 *
 * @Param	DeltaTime			-	Time since last update
 * @Param	out_ViewRotation	- 	The new ViewRotation passed back out
 * @Param	DeltaRot			- 	The Change in Rotation
 */

 /*
function ProcessViewRotation( float DeltaTime, out Rotator out_ViewRotation, Rotator DeltaRot )
{
	local float ActualZoomModifier;
	local float Perc;

	if ( FOVAngle < DefaultFOV )	// If we are zoomed
	{
		Perc = 1.0 - ( FOVAngle / DefaultFOV );
		ActualZoomModifier = 1.0 - ((1.0 - ZoomRotationModifier) * Perc);

		DeltaRot *= (ZoomRotationModifier * ActualZoomModifier);
	}

	Super.ProcessViewRotation(DeltaTime, out_ViewRotation, DeltaRot);
}
*/
/*
exec function ResetTest()
{
	ServerResetTest();
}

reliable server function ServerResetTest()
{
	ResetKActors();
//	ClientResetTest();
}

reliable client function ClientResetTest()
{
	ResetKActors();
}

simulated function ResetKActors()
{
	local KActor A;
	foreach AllActors(class'KActor',A)
	{
		`log("#### Resetting "@A);
		A.Reset();
	}

}
*/


function UpdateRotation( float DeltaTime )
{
	local rotator DeltaRot;

	if (bDebugFreeCam)
	{
		// Calculate Delta to be applied on ViewRotation
		DeltaRot.Yaw	= PlayerInput.aTurn;
		DeltaRot.Pitch	= PlayerInput.aLookUp;
		ProcessViewRotation( DeltaTime, DebugFreeCamRot, DeltaRot );
	}
	else
	{
		super.UpdateRotation(DeltaTime);
	}
}

/** console command to set things up for easy walker legs debugging */
exec function DebugWalkerLegs(bool bDebugMode)
{
	local UTVehicle_DarkWalker W;

	foreach AllActors(class'UTVehicle_DarkWalker', W)
	{
		W.BaseEyeHeight = bDebugMode ? -500.f : W.default.BaseEyeHeight;
		W.Mesh.SetHidden(bDebugMode);
	}
}

/** DarkWalker debug functionality */
exec function DWStep(int LegIdx, float Mag)
{
	local UTWalkerBody B;
	`log("***"@GetFuncName()@LegIdx@Mag);
	foreach AllActors(class'UTWalkerBody', B)
	{
		B.DoTestStep(LegIdx, Mag);
	}
}

/** DarkWalker debug functionality */
exec function DWReleaseFeet()
{
	local UTWalkerBody B;
	local int Idx;

	`log("***"@GetFuncName());
	foreach AllActors(class'UTWalkerBody', B)
	{
		for (Idx=0; Idx<3; Idx++)
		{
			B.FootConstraints[Idx].ReleaseComponent();
		}
	}
}

/** apply a random whack to all walker vehicles, used for debugging */
exec function WhackWalker(float Mag)
{
	local UTVehicle_Walker W;
	local vector TraceStart, HitLoc, HitNorm;

	foreach AllActors(class'UTVehicle_Walker', W)
	{
		TraceStart = W.Location + VRand() * 1024;

		TraceComponent(HitLoc, HitNorm, W.Mesh, W.Location, TraceStart);

		W.AddVelocity(VRand()*Mag, HitLoc, class'UTDmgType_ShockPrimary');
	}
}

/**
 * Show the Quick Pick Scene
 */
exec function ShowQuickPick()
{
	local UIScene Scene;

	if ( bDisableQuickPick || (Pawn != none && Vehicle(Pawn) != none) )
	{
		NextWeapon();
	}
	else
	{
		if (QuickPickScene == none )
		{
			Scene = OpenUIScene(QuickPickSceneTemplate);
			if ( Scene != none )
			{
				QuickPickScene = UTUIScene_WeaponQuickPick(Scene);
				QuickPickScene.OnSceneDeactivated = QuickPickCanceled;
			}
			else
			{
				`log("Could not open QuickPick Scene!");
				return;
			}
		}
		else
		{
			QuickPickScene.Show();
		}

	}
}

/**
 * Hide the Quick Pick Scene
 */
exec function HideQuickPick()
{
	if ( QuickPickScene != none )
	{
		QuickPickScene.Hide();
	}
}

/**
 * Change the selection in a given QuickPick group
 */
exec function QuickPick(int Quad)
{
	local UTWeapon Weapon;

	if ( QuickPickScene != none && QuickPickScene.IsVisible() )
	{
		Weapon = QuickPickScene.QuickPick(Quad);
		if (Weapon != none )
		{
			if ( Pawn != none && Pawn.InvManager != none )
			{
				Pawn.InvManager.SetCurrentWeapon(Weapon);
			}
		}

	}
}

/**
 * This Delegate is set to the QuickPick scenes' OnSceneDeactivated delegate so that we can
 * clear out our scene when done.
 */
function QuickPickCanceled(UIScene DeactivatedScene )
{
	QuickPickScene = none;
}

/**
 * Turn the QuickPick System off
 */
exec function ToggleQuickPickOff()
{
	if ( bDisableQuickPick )
	{
		bDisableQuickPick = false;
	}
	else
	{
		if ( QuickPickScene != none )
		{
			QuickPickScene.SceneClient.CloseScene(QuickPickScene);
		}

		bDisableQuickPick=true;
	}
}

/**
 * If the quick pick menu is up, then select the best weapon and close it
 */
exec function QuickPickBestWeapon()
{
	if ( QuickPickScene != none && QuickPickScene.IsVisible() )
	{
		HideQuickPick();
		SwitchToBestWeapon(true);
	}
}

/** debug command for bug reports */
exec function GetPlayerLoc()
{
	`Log("Location:" @ Location,, 'PlayerLoc');
	`Log("Rotation:" @ Rotation,, 'PlayerLoc');
}

/** Called from the UTTeamGameMessage, this will cause all TeamColored Images on the hud to pulse. */
function PulseTeamColor()
{
	PulseTimer = default.PulseTimer;
	bPulseTeamColor = true;
}

function SetPawnConstructionScene(bool bShow)
{
	bConstructioningMeshes = bShow;
}

/** called when the GRI finishes processing custom character meshes */
function CharacterProcessingComplete()
{
	SetPawnConstructionScene(false);
	bInitialProcessingComplete = true;
	ServerSetProcessingComplete();
}

/** called after any initial clientside processing is complete to allow the client to spawn in */
reliable server function ServerSetProcessingComplete()
{
	bInitialProcessingComplete = true;
}

event PreClientTravel()
{
	Super.PreClientTravel();

	// set on server in UTGame::ProcessServerTravel()
	bInitialProcessingComplete = false;
}

function bool CanRestartPlayer()
{
	local UTGame Game;

	Game = UTGame(WorldInfo.Game);
	return ((bInitialProcessingComplete || (Game != None && Game.bQuickStart)) && Super.CanRestartPlayer());
}

/**
 * @return Returns the index of this PC in the GamePlayers array.
 */
native function int GetUIPlayerIndex();

/**
 * Sets the current gamma value.
 *
 * @param New Gamma Value, must be between 0.0 and 1.0
 */
native function SetGamma(float GammaValue);

/** Loads the player's custom character from their profile. */
function LoadCharacterFromProfile(UTProfileSettings Profile)
{
	local string OutStringValue;
	local bool bRandomCharacter;

	bRandomCharacter = true;

	// get character info and send to server
	if(Profile.GetProfileSettingValueStringByName('CustomCharData', OutStringValue))
	{
		if(Len(OutStringValue)>0)
		{
			`Log("UTPlayerController::LoadCharacterFromProfile() - Loaded character data from profile.");
			ServerSetCharacterData(GetPlayerCustomCharData(OutStringValue));
			bRandomCharacter = false;
		}
	}

	// Autogenerate character data if they do not have a character set.
	if(bRandomCharacter)
	{
		`Log("UTPlayerController::LoadCharacterFromProfile() - Character data not found, generating a random character.");
		ServerSetCharacterData(class'UTCustomChar_Data'.static.MakeRandomCharData());
	}
}

/** Gathers player settings from the client's profile. */
exec function RetrieveSettingsFromProfile()
{
	local int PlayerIndex;
	local int OutIntValue;
	//local float OutFloatValue;
	local UTProfileSettings Profile;

	Profile = UTProfileSettings(OnlinePlayerData.ProfileProvider.Profile);

	if(Profile != none)
	{
		PlayerIndex = GetUIPlayerIndex();

		`Log("Retrieving Profile Settings for UI PlayerIndex " $ PlayerIndex);

		///////////////////////////////////////////////////////////////////////////
		// Player Custom Character
		///////////////////////////////////////////////////////////////////////////
		LoadCharacterFromProfile(Profile);

		///////////////////////////////////////////////////////////////////////////
		// Shared Options - These options are shared between all players on this machine.
		///////////////////////////////////////////////////////////////////////////

		// Only allow player 0 to set shared options.
		if(PlayerIndex==0)
		{
			// Set volumes
			if(Profile.GetProfileSettingValueIntByName('SFXVolume', OutIntValue))
			{
				SetAudioGroupVolume( 'SFX', (OutIntValue / 10.0f) );
			}

			if(Profile.GetProfileSettingValueIntByName('VoiceVolume', OutIntValue))
			{
				SetAudioGroupVolume( 'VoiceChat', (OutIntValue / 10.0f) );
			}

			if(Profile.GetProfileSettingValueIntByName('AnnouncerVolume', OutIntValue))
			{
				SetAudioGroupVolume( 'Announcer', (OutIntValue / 10.0f) );
			}

			if(Profile.GetProfileSettingValueIntByName('MusicVolume', OutIntValue))
			{
				SetAudioGroupVolume( 'Music', (OutIntValue / 10.0f) );
			}

			// Set Gamma
			if(Profile.GetProfileSettingValueIntByName('Gamma', OutIntValue))
			{
				SetGamma(OutIntValue / 10.0f);
			}
		}

		///////////////////////////////////////////////////////////////////////////
		// Video Options
		///////////////////////////////////////////////////////////////////////////

		// PostProcessPreset
		if(Profile.GetProfileSettingValueIdByName('PostProcessPreset', OutIntValue))
		{
			if(OutIntValue < PostProcessPresets.length)
			{
				Player.PP_DesaturationMultiplier = PostProcessPresets[OutIntValue].Desaturation;
				Player.PP_HighlightsMultiplier = PostProcessPresets[OutIntValue].Highlights;
				Player.PP_MidTonesMultiplier = PostProcessPresets[OutIntValue].MidTones;
				Player.PP_ShadowsMultiplier = PostProcessPresets[OutIntValue].Shadows;

				`Log("PP_MidTonesMultiplier Changed: " $ PostProcessPresets[OutIntValue].MidTones);
				`Log("PP_ShadowsMultiplier Changed: " $ PostProcessPresets[OutIntValue].Shadows);
				`Log("PP_HighlightsMultiplier Changed: " $ PostProcessPresets[OutIntValue].HighLights);
				`Log("PP_DesaturationMultiplier Changed: " $ PostProcessPresets[OutIntValue].Desaturation);
			}
		}

		// DefaultFOV
		if(Profile.GetProfileSettingValueIntByName('DefaultFOV', OutIntValue))
		{
			DefaultFOV = Clamp(OutIntValue, 80, 120);
		}

		///////////////////////////////////////////////////////////////////////////
		// Audio Options
		///////////////////////////////////////////////////////////////////////////

		// AnnounceSetting
		if (Announcer != None && Profile.GetProfileSettingValueIdByName('AnnounceSetting', OutIntValue))
		{
			Announcer.AnnouncerLevel = byte(OutIntValue);
		}

		// AutoTaunt
		if(Profile.GetProfileSettingValueIdByName('AutoTaunt', OutIntValue))
		{
			bAutoTaunt = (OutIntValue==UTPID_VALUE_YES);
		}

		// MessageBeep
		if(Profile.GetProfileSettingValueIdByName('MessageBeep', OutIntValue))
		{
			if (myHUD != None)
			{
				myHUD.bMessageBeep = (OutIntValue==UTPID_VALUE_YES);
			}
			class'HUD'.default.bMessageBeep = (OutIntValue==UTPID_VALUE_YES);
		}

		// TextToSpeechEnable
		if(Profile.GetProfileSettingValueIdByName('TextToSpeechEnable', OutIntValue))
		{
			bNoTextToSpeechVoiceMessages = (OutIntValue==UTPID_VALUE_NO);
		}

		// TextToSpeechTeamMessagesOnly
		if(Profile.GetProfileSettingValueIdByName('TextToSpeechTeamMessagesOnly', OutIntValue))
		{
			// @todo: Hookup
			//TextToSpeechTeamMessagesOnly = (OutIntValue==UTPID_VALUE_YES);
		}

		///////////////////////////////////////////////////////////////////////////
		// Input Options
		///////////////////////////////////////////////////////////////////////////

		// Invert Y
		if(Profile.GetProfileSettingValueIdByName('InvertY', OutIntValue))
		{
			PlayerInput.bInvertMouse = (OutIntValue==PYIO_On);
		}

		// Invert X
		if(Profile.GetProfileSettingValueIdByName('InvertX', OutIntValue))
		{
			PlayerInput.bInvertTurn = (OutIntValue==PXIO_On);
		}

		// Mouse Smoothing
		if(Profile.GetProfileSettingValueIdByName('MouseSmoothing', OutIntValue))
		{
			PlayerInput.bEnableMouseSmoothing = (OutIntValue==UTPID_VALUE_YES);
		}

		// Mouse Sensitivity (Game)
		if(Profile.GetProfileSettingValueIntByName('MouseSensitivityGame', OutIntValue))
		{
			// @todo: Hookup
			//PlayerInput.MouseSensitivity = OutIntValue / 10.0f;
		}

		// Mouse Sensitivity (Menus)
		if(Profile.GetProfileSettingValueIntByName('MouseSensitivityMenus', OutIntValue))
		{
			// @todo: Hookup
			//PlayerInput.MouseSensitivity = OutIntValue / 10.0f;
		}

		// MouseSmoothingStrength
		if(Profile.GetProfileSettingValueIntByName('MouseSmoothingStrength', OutIntValue))
		{
			// @todo: Hookup
			//PlayerInput.MouseSmoothingStrength = OutIntValue / 10.0f;
		}

		// MouseAccelTreshold
		if(Profile.GetProfileSettingValueIntByName('MouseAccelTreshold', OutIntValue))
		{
			// @todo: Hookup
			//PlayerInput.MouseAccelTreshold = OutIntValue / 10.0f;
		}

		// ReduceMouseLag
		if(Profile.GetProfileSettingValueIdByName('ReduceMouseLag', OutIntValue))
		{
			// @todo: Hookup
			//PlayerInput.ReduceMouseLag = (OutIntValue==UTPID_VALUE_YES);
		}

		// Enable Joystick
		if(Profile.GetProfileSettingValueIdByName('EnableJoystick', OutIntValue))
		{
			// @todo: Hookup
			//PlayerInput.bUsingGamepad = (OutIntValue==UTPID_VALUE_YES);
		}


		// DodgeDoubleClickTime
		if(Profile.GetProfileSettingValueIntByName('DodgeDoubleClickTime', OutIntValue))
		{
			PlayerInput.DoubleClickTime = OutIntValue / 100.0f;
		}

		///////////////////////////////////////////////////////////////////////////
		// Game Options
		///////////////////////////////////////////////////////////////////////////

		// Weapon Bob
		if(Profile.GetProfileSettingValueIdByName('ViewBob', OutIntValue))
		{
			bProfileWeaponBob = (OutIntValue==PYIO_On);
		}

		// GoreLevel
		if(Profile.GetProfileSettingValueIdByName('GoreLevel', OutIntValue))
		{
			class'GameInfo'.default.GoreLevel = OutIntValue;
		}

		// EnableDodging
		if(Profile.GetProfileSettingValueIdByName('DodgingEnabled', OutIntValue))
		{
			bEnableDodging = (OutIntValue==UTPID_VALUE_YES);
		}

		// WeaponSwitchOnPickup
		if(Profile.GetProfileSettingValueIdByName('WeaponSwitchOnPickup', OutIntValue))
		{
			bProfileAutoSwitchWeaponOnPickup = (OutIntValue==UTPID_VALUE_YES);
		}

		// Auto Aim
		if(Profile.GetProfileSettingValueIdByName('AutoAim', OutIntValue))
		{
			bAimingHelp = (OutIntValue==PAAO_On);
		}

		// NetworkConnection
		if(Profile.GetProfileSettingValueIdByName('NetworkConnection', OutIntValue))
		{
			// @todo: Find reasonable values for these.
			switch(OutIntValue)
			{
			case NETWORKTYPE_Modem:
				SetNetSpeed(2600);
				break;
			case NETWORKTYPE_ISDN:
				SetNetSpeed(7500);
				break;
			case NETWORKTYPE_Cable:
				SetNetSpeed(10000);
				break;
			case NETWORKTYPE_LAN:
				SetNetSpeed(15000);
				break;
			default:
				SetNetSpeed(10000);
				break;
			}
		}

		// DynamicNetSpeed
		if(Profile.GetProfileSettingValueIdByName('DynamicNetspeed', OutIntValue))
		{
			bDynamicNetSpeed = (OutIntValue==UTPID_VALUE_YES);
		}

		// SpeechRecognition
		/* @FIXME: currently there's no support for (de)activating speech recognition during gameplay
		if(Profile.GetProfileSettingValueIdByName('SpeechRecognition', OutIntValue))
		{
			OnlineSubsystemCommonImpl(OnlineSub).bIsUsingSpeechRecognition = (OutIntValue==UTPID_VALUE_YES);
		}
		*/

		///////////////////////////////////////////////////////////////////////////
		// Weapon Options
		///////////////////////////////////////////////////////////////////////////

		// WeaponHand
		if(Profile.GetProfileSettingValueIdByName('WeaponHand', OutIntValue))
		{
			SetHand(EWeaponHand(OutIntValue));
		}

		// SmallWeapons
		/* @todo: Hookup
		if(Profile.GetProfileSettingValueIdByName('SmallWeapons', OutIntValue))
		{
			bProfileSmallWeapons =(OutIntValue==UTPID_VALUE_YES);
		}
		*/

		// DisplayWeaponBar
		if(Profile.GetProfileSettingValueIdByName('DisplayWeaponBar', OutIntValue))
		{
			if(UTHUD(myHUD) != None)
			{
				UTHUD(myHUD).bShowWeaponbar = (OutIntValue==UTPID_VALUE_YES);
			}
		}

		// ShowOnlyAvailableWeapons
		if(Profile.GetProfileSettingValueIdByName('ShowOnlyAvailableWeapons', OutIntValue))
		{
			if(UTHUD(myHUD) != None)
			{
				UTHUD(myHUD).bShowOnlyAvailableWeapons = (OutIntValue==UTPID_VALUE_YES);
			}
		}


		///////////////////////////////////////////////////////////////////////////
		// Finished Storing Options
		///////////////////////////////////////////////////////////////////////////

		// If we have a UTPawn, tell it to retrieve its settings.
		if(UTPawn(Pawn) != none)
		{
			UTPawn(Pawn).RetrieveSettingsFromPC();
		}

	}
}

simulated exec function TestCameraBlood()
{
	ClientSpawnCameraEffect( UTPawn(Pawn), class'UTEmitCameraEffect_BloodAndChunks' );
}

/** Spawn ClientSide Camera Effects **/
unreliable client function ClientSpawnCameraEffect( UTPawn ThePawn, class<UTEmitCameraEffect> CameraEffectClass )
{
	local vector CamLoc;
	local rotator CamRot;

	local UTPlayerController UTPC;
	UTPC = self; // UTPlayerController(ThePawn.Controller);

	//`log( "attempting to spawn: " $ CameraEffectClass );

	if( (UTPC != None) && (CameraEffectClass != None) && (UTPC.CameraEffect == None) )
	{
		UTPC.CameraEffect = Spawn( CameraEffectClass, self );
		//`log( "    just spawned: " $ CameraEffect );
		if( UTPC.CameraEffect != None )
		{
			GetPlayerViewPoint( CamLoc, CamRot );
			UTPC.CameraEffect.RegisterCamera( UTPC );
			UTPC.CameraEffect.UpdateLocation( CamLoc, CamRot, UTPC.FOVAngle );
		}
	}
}


/**
* @param BloodEmitter If None, remove any blood.  If set, remove only that blood.
*/
function RemoveCameraBlood( UTEmitCameraEffect BloodEmitter )
{
	if( CameraEffect == BloodEmitter )
	{
		CameraEffect = none;
	}
}


function ClearCameraBlood()
{
	if( CameraEffect != None )
	{
		CameraEffect.Destroy();
		CameraEffect = none;
	}
}


function SetViewTarget( Actor NewViewTarget, optional float TransitionTime )
{
	Super.SetViewTarget( NewViewTarget,TransitionTime );

	ClearCameraBlood();
}


/**
 * Attempts to open a UI Scene.
 *
 * @Param	Template		The Template of the scene to open.
 * @Returns the opened scene
 */
simulated function UIScene OpenUIScene(UIScene Template)
{
	local UIInteraction UIController;
	local LocalPlayer LP;
	local UIScene s;

	LP = LocalPlayer(Player);
	if ( LP != none )
	{
		UIController = LocalPlayer(Player).ViewportClient.UIController;
		UIController.OpenScene(Template,,s);
	}

	return S;
}


exec function UIAnimTest(int count)
{
	local UIScene s;
	local UTSimpleList List;
	local int i;


	S = OpenUIScene(TestSceneTemplate);

	List = UTSimpleList(S.FindChild('UTSimpleList_0',true));
	for (i=0;i<Count;i++)
	{
		List.AddItem("Test"@I);
	}
}

exec function ShowCommandMenu()
{
	local UIScene Scene;

	if ( CommandMenu == none )
	{
		Scene = OpenUIScene(CommandMenuTemplate);
		if (Scene != none)
		{
			CommandMenu = UTUIScene_CommandMenu(Scene);
			CommandMenu.OnSceneDeactivated = CommandMenuDeactivated;
		}
	}
}

function CommandMenuDeactivated( UIScene DeactivatedScene )
{
	if (DeactivatedScene == CommandMenu)
	{
		CommandMenu = none;
	}
}


//// START:  Decal testing code
exec function SpawnABloodDecal2()
{
	LeaveADecal2( Pawn.Location, vect(0,0,0) );
}

simulated function LeaveADecal2( vector HitLoc, vector HitNorm )
{
	local UTGib Gib;

	Gib = Spawn( class'UTGib_HumanTorso',,, Pawn.location + vect(0,0,150),,, TRUE );
	Gib.GibMeshComp.WakeRigidBody();
}


exec function SpawnABloodDecal()
{
	LeaveADecal( Pawn.Location, vect(0,0,0) );
}

simulated function LeaveADecal( vector HitLoc, vector HitNorm )
{
	local DecalComponent DC;
	local DecalLifetimeDataAge LifetimePolicy;

	local MaterialInstance MIC_Decal;

	local Actor TraceActor;
	local vector out_HitLocation;
	local vector out_HitNormal;
	local vector TraceDest;
	local vector TraceStart;
	local vector TraceExtent;
	local TraceHitInfo HitInfo;
	local float RandRotation;

	// these should be randomized
	TraceStart = HitLoc + ( Vect(0,0,15));
	TraceDest =  HitLoc - ( Vect(0,0,100));

	RandRotation = FRand() * 360;

	TraceActor = Trace( out_HitLocation, out_HitNormal, TraceDest, TraceStart, false, TraceExtent, HitInfo, TRACEFLAG_PhysicsVolumes );

	if( TraceActor != None )
	{
		LifetimePolicy = new(TraceActor.Outer) class'DecalLifetimeDataAge';
		LifetimePolicy.LifeSpan = 10.0f;

		DC = new(self) class'DecalComponent';
		DC.Width = 200;
		DC.Height = 200;
		DC.Thickness = 10;
		DC.bNoClip = FALSE;


		MIC_Decal = new(outer) class'MaterialInstanceTimeVarying';
		//MIC_Decal = new(outer) class'MaterialInstanceConstant;
		// T_FX.DecalMaterials.M_FX_BloodDecal_Large01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Large02
		// T_FX.DecalMaterials.M_FX_BloodDecal_Medium01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Medium02
		// T_FX.DecalMaterials.M_FX_BloodDecal_Small01
		// T_FX.DecalMaterials.M_FX_BloodDecal_Small02

		MIC_Decal.SetParent( MaterialInstanceTimeVarying'CH_Gibs.Decals.BloodSplatter' );
		//MIC_Decal.SetParent( MaterialInstanceConstant'T_FX.DecalMaterials.M_FX_BloodDecal_Small01' );

		DC.DecalMaterial = MIC_Decal;

		TraceActor.AddDecal( DC.DecalMaterial, out_HitLocation, rotator(-out_HitNormal), RandRotation, DC.Width, DC.Height, DC.Thickness, DC.bNoClip, HitInfo.HitComponent, LifetimePolicy, TRUE, FALSE, HitInfo.BoneName, HitInfo.Item, HitInfo.LevelIndex );

		// here we want to set
		MaterialInstanceTimeVarying(MIC_Decal).SetScalarStartTime( 'DissolveAmount',  3.0f );
		MaterialInstanceTimeVarying(MIC_Decal).SetScalarStartTime( 'BloodAlpha',  3.0f );
		MIC_Decal.SetScalarParameterValue( 'DissolveAmount',  3.0f );
		MIC_Decal.SetScalarParameterValue( 'BloodAlpha',  3.0f );
		//SetTimer( 3.0f, FALSE, 'StartDissolving' );
	}
}

function AdjustPersistentKey(UTSeqAct_AdjustPersistentKey InAction)
{
	local UTProfileSettings Profile;
	Profile = UTProfileSettings(OnlinePlayerData.ProfileProvider.Profile);

	if (InAction.bRemoveKey)
	{
		Profile.RemovePersistentKey(InAction.TargetKey);
	}
	else
	{
		Profile.AddPersistentKey(InAction.TargetKey);
	}
}

function bool HasPersistentKey(ESinglePlayerPersistentKeys SearchKey)
{
	local UTProfileSettings Profile;
	Profile = UTProfileSettings(OnlinePlayerData.ProfileProvider.Profile);

	return Profile.HasPersistentKey(SearchKey);
}


//// END:  Decal testing code

exec function TestSlomo(float T)
{
	WorldInfo.Game.SetGameSpeed(T);
}

/** Used in the Character Editor - outputs current character setup as concise text form. */
native exec function ClipChar();

/** Used in the Character Editor - outputs total poly count for character setup on screen. */
native exec function CharPolyCount();

defaultproperties
{
	TestSceneTemplate=UTUIScene_MapVote'UI_Scenes_HUD.Menus.MapVoteMenu'

	DesiredFOV=90.000000
	DefaultFOV=90.000000
	FOVAngle=90.000
	CameraClass=None
	CheatClass=class'UTCheatManager'
	InputClass=class'UTGame.UTPlayerInput'
	LastKickWarningTime=-1000.0
	bForceBehindview=true
	DamageCameraAnim=CameraAnim'FX_HitEffects.DamageViewShake'
	SpawnCameraAnim[0]=CameraAnim'Envy_Effects.Camera_Shakes.C_Res_IN_Red'
	SpawnCameraAnim[1]=CameraAnim'Envy_Effects.Camera_Shakes.C_Res_IN_Blue'
	SpawnCameraAnim[2]=CameraAnim'Envy_Effects.Camera_Shakes.C_Res_IN'
	MatineeCameraClass=class'Engine.AnimatedCamera'
	bCheckSoundOcclusion=true
	ZoomRotationModifier=0.5
	VehicleCheckRadiusScaling=1.0
	bRotateMiniMap=false

	QuickPickSceneTemplate=UTUIScene_WeaponQuickPick'UI_Scenes_HUD.HUD.QuickPick'

	Pulsetimer = 5.0;

	CommandMenuTemplate=UTUIScene_CommandMenu'UI_Scenes_HUD.HUD.CommandMenuScene'

	PostProcessPresets(PPP_Default)=(Shadows=1.0, Midtones=1.0, Highlights=1.0, Desaturation=1.0)
	PostProcessPresets(PPP_Soft)=(Shadows=0.8, Midtones=0.9, Highlights=1.1, Desaturation=1.1)
	PostProcessPresets(PPP_Lucent)=(Shadows=0.7, Midtones=0.8, Highlights=1.2, Desaturation=1.2)
	PostProcessPresets(PPP_Vibrant)=(Shadows=1.0, Midtones=1.0, Highlights=0.8, Desaturation=0.8)
	MinRespawnDelay=1.5
	BeaconPulseMax=1.1
	BeaconPulseRate=0.5
}

/* //@todo steve
event TeamMessage( PlayerReplicationInfo PRI, coerce string S, name Type  )
{
	local string c;

	// Wait for player to be up to date with replication when joining a server, before stacking up messages
	if ( WorldInfo.NetMode == NM_DedicatedServer || GameReplicationInfo == None )
		return;

	if( AllowTextToSpeech(PRI, Type) )
		TextToSpeech( S, TextToSpeechVoiceVolume );
	if ( Type == 'TeamSayQuiet' )
		Type = 'TeamSay';

	if ( myHUD != None )
		myHUD.Message( PRI, c$S, Type );

	if ( (Player != None) && (Player.Console != None) )
	{
		if ( PRI!=None )
		{
			if ( PRI.Team!=None && GameReplicationInfo.bTeamGame)
			{
    			if (PRI.Team.TeamIndex==0)
					c = chr(27)$chr(200)$chr(1)$chr(1);
    			else if (PRI.Team.TeamIndex==1)
				c = chr(27)$chr(125)$chr(200)$chr(253);
			}
			S = PRI.PlayerName$": "$S;
		}
		Player.Console.Chat( c$s, 6.0, PRI );
	}
}

function ClientVoiceMessage(PlayerReplicationInfo Sender, PlayerReplicationInfo Recipient, name messagetype, byte messageID)
{
    local VoicePack V;

    if ( (Sender == None) || (Sender.voicetype == None) || (Player.Console == None) )
	return;

    V = Spawn(Sender.voicetype, self);
    if ( V != None )
	V.ClientInitialize(Sender, Recipient, messagetype, messageID);
}

function NotifyTakeHit(Controller InstigatedBy, vector HitLocation, int Damage, class<DamageType> damageType, vector Momentum)
{
	if ( (instigatedBy != None) && (instigatedBy != pawn) )
		damageAttitudeTo(instigatedBy, Damage);
}

function damageAttitudeTo(Controller Other, float Damage)
{
    if ( (Other != None) && (Other != self) && (Damage > 0) && (Other.Pawn != None) )
	Enemy = Other.Pawn;
}

function ServerSpeech( name Type, int Index, string Callsign )
{
	// `log("Type:"$Type@"Index:"$Index@"Callsign:"$Callsign);
	if(PlayerReplicationInfo.VoiceType != None)
		PlayerReplicationInfo.VoiceType.static.PlayerSpeech( Type, Index, Callsign, Self );
}

function StopFiring()
{
	if ( (UTPawn(Pawn) != None) && UTPawn(Pawn).StopWeaponFiring() )
		bStoppedFiring = true;

	bCanFire = false;
}

singular event UnPressButtons()
{
	bDuck = 0;
	bRun = 0;
	bVoiceTalk = 0;
	ResetInput();
}
*/

