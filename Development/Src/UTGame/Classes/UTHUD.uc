/**
 * UTHUD
 * UT Heads Up Display
 *
 *
* Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTHUD extends HUD
	dependson(UTWeapon)
	native(UI)
	config(Game);

/** Holds a list of Actors that need PostRender calls */
var array<Actor> PostRenderedActors;

/** Cached reference to the hud texture */
var const Texture2D HudTexture;

/** Cached reference to the another hud texture */
var const Texture2D AltHudTexture;

var const LinearColor LC_White;

/** used to pulse the scaled of several hud elements */
var float LastPickupTime, LastAmmoPickupTime, LastWeaponPickupTime, LastHealthPickupTime, LastArmorPickupTime;

/** The Pawn that is currently owning this hud */
var Pawn PawnOwner;

/** Points to the UT Pawn.  Will be resolved if in a vehicle */
var UTPawn UTPawnOwner;

/** Cached a typed Player contoller.  Unlike PawnOwner we only set this once in PostBeginPlay */
var UTPlayerController UTPlayerOwner;

/** Cached typed reference to the PRI */
var UTPlayerReplicationInfo UTOwnerPRI;

/** If true, we will allow Weapons to show their crosshairs */
var() globalconfig bool bCrosshairShow;

/** Debug flag to show AI information */
var bool bShowAllAI;

/** Cached reference to the GRI */
var UTGameReplicationInfo UTGRI;

/** Holds the scaling factor given the current resolution.  This is calculated in PostRender() */
var float ResolutionScale;

var bool bHudMessageRendered;

/******************************************************************************************
  UI/SCENE data for the hud
 ******************************************************************************************/

/** The Scoreboard. */
var UTUIScene_Scoreboard ScoreboardSceneTemplate;
var UTUIScene_Scoreboard ScoreboardScene;

/** class of dynamic music manager used with this hud/gametype */
var class<UTMusicManager> MusicManagerClass;

/** A collection of fonts used in the hud */
var array<font> HudFonts;

/** If true, we will alter the crosshair when it's over a friendly */
var bool bCrosshairOnFriendly;

/******************************************************************************************
 Character Portraits
 ******************************************************************************************/

/** The material used to display the portrait */
var material CharPortraitMaterial;

/** The MI that we will set */
var MaterialInstanceConstant CharPortraitMI;

/** How far down the screen will it be rendered */
var float CharPortraitYPerc;

/** When sliding in, where should this image stop */
var float CharPortraitXPerc;

/** How long until we are done */
var float CharPortraitTime;

/** Total Amount of time to display a portrait for */
var float CharPortraitSlideTime;

/** % of Total time to slide In/Out.  It will be used on both sides.  Ex.  If set to 0.25 then
    the slide in will be 25% of the total time as will the slide out leaving 50% of the time settled
    on screen. **/
var float CharPortraitSlideTransitionTime;

/** How big at 1024x768 should this be */
var vector2D CharPortraitSize;

/** Holds the PRI of the person speak */
var UTPlayerReplicationInfo CharPRI;

/** Holds the PRI of who we want to switch to */
var UTPlayerReplicationInfo CharPendingPRI;


/******************************************************************************************
 WEAPONBAR
 ******************************************************************************************/

/** If true, weapon bar is never displayed */
var bool bShowWeaponbar;

/** If true, only show available weapons on weapon bar */
var bool bShowOnlyAvailableWeapons;

/** If true, only weapon bar if have pendingweapon */
var bool bOnlyShowWeaponBarIfChanging;

/** Scaling to apply to entire weapon bar */
var float WeaponBarScale;

var float WeaponBoxWidth, WeaponBoxHeight;

/** Resolution dependent HUD scaling factor */
var float HUDScaleX, HUDScaleY;
var linearcolor TeamHUDColor;
var color TeamColor;
var color TeamTextColor;

/** Weapon bar top left corner at 1024x768, normal scale */
var float WeaponBarY;

/** List of weapons to display in weapon bar */
var UTWeapon WeaponList[10];
var float CurrentWeaponScale[10];

var float SelectedWeaponScale;
var float BounceWeaponScale;
var float SelectedWeaponAlpha;
var float OffWeaponAlpha;
var float EmptyWeaponAlpha;
var float LastHUDUpdateTime;
var int BouncedWeapon;
var float WeaponScaleSpeed;
var float WeaponBarXOffset;
var float WeaponXOffset;
var float SelectedBoxScale;
var float WeaponYScale;
var float WeaponYOffset;
var float WeaponAmmoLength;
var float WeaponAmmoThickness;
var float WeaponAmmoOffsetX;
var float WeaponAmmoOffsetY;
var float SelectedWeaponAmmoOffsetX;
var bool bNoWeaponNumbers;
var float LastWeaponBarDrawnTime;

/******************************************************************************************
 MOTD
 ******************************************************************************************/

var UTUIScene_MOTD MOTDSceneTemplate;

/******************************************************************************************
 Messaging
 ******************************************************************************************/

/** Y offsets for local message areas - value above 1 = special position in right top corner of HUD */
var float MessageOffset[7];

/** Various colors */
var const color BlackColor, GoldColor;

/******************************************************************************************
 Map / Radar
 ******************************************************************************************/

/** The background texture for the map */
var Texture2D MapBackground;

/** Holds the default size in pixels at 1024x768 of the map */
var config float MapDefaultSize;

/** The orders to display when rendering the map */
var string DisplayedOrders;


var Weapon LastSelectedWeapon;


/******************************************************************************************
 Glowing Fonts
 ******************************************************************************************/

var font GlowFonts[2];	// 0 = the Glow, 1 = Text

/******************************************************************************************
 Safe Regions
 ******************************************************************************************/

// TODO - MAKE ME CONFIG

/** The percentage of the view that should be considered safe */
var config float SafeRegionPct;

/** Holds the full width and height of the viewport */
var float FullWidth, FullHeight;

/******************************************************************************************
 The damage direction indicators
 ******************************************************************************************/
/**
 * Holds the various data for each Damage Type
 */
struct native DamageInfo
{
	var	float	FadeTime;
	var float	FadeValue;
	var MaterialInstanceConstant MatConstant;
};

/** Holds the Max. # of indicators to be shown */
var int MaxNoOfIndicators;

/** List of DamageInfos. */
var array<DamageInfo> DamageData;

/** This holds the base material that will be displayed */
var Material BaseMaterial;

/** How fast should it fade out */
var float FadeTime;

/** Name of the material parameter that controls the position */
var name PositionalParamName;

/** Name of the material parameter that controls the fade */
var name FadeParamName;

/******************************************************************************************
 The Distortion Effect (Full Screen)
 ******************************************************************************************/

/** maximum hit effect intensity */
var float MaxHitEffectIntensity;

/** maximum hit effect color */
var LinearColor MaxHitEffectColor;

/** whether we're currently fading out the hit effect */
var bool bFadeOutHitEffect;

/** the amount the time it takes to fade the hit effect from the maximum values */
var float HitEffectFadeTime;

/** reference to the hit effect */
var MaterialEffect HitEffect;

/** material instance for the hit effect */
var transient MaterialInstanceConstant HitEffectMaterialInstance;


/******************************************************************************************
 Widget Locations / Visibility flags
 ******************************************************************************************/

var config bool bShowClock;
var config vector2d ClockPosition;

var config bool bShowDoll;
var config vector2d DollPosition;

	var int LastHealth;
	var float HealthPulseTime;
	var int LastArmorAmount;
	var float ArmorPulseTime;

var config bool bShowAmmo;
var config vector2d AmmoPosition;

	var UTWeapon LastWeapon;
	var int LastAmmoCount;
	var float AmmoPulseTime;

var config bool bShowMap;
var config vector2d MapPosition;

var config bool bShowPowerups;
var config vector2d PowerupDims;

	struct native PowerupSlotData
	{
		// Powerup this slot is for
		var UTTimedPowerup Powerup;
		// Are we transitioning in or out
		var bool bTransitionIn;
		// How much time left in the transition
		var float TransitionTime;
	};

	/** How long to fade */
	var float PowerupTransitionTime;

	/** Holds the 4 powerup slots */
	var PowerupSlotData PowerupSlots[4];

var config bool bShowScoring;
var config vector2d ScoringPosition;
var config bool bShowFragCount;
var config bool bShowLeaderboard;

	var float FragPulseTime;
	var int LastFragCount;


var config bool bShowVehicle;
var config vector2d VehiclePosition;

var config bool bShowDamage;
var config float DamageIndicatorSize;

/******************************************************************************************
 Pulses
 ******************************************************************************************/

/** How long should the pulse take total */
var float PulseDuration;
/** When should the pulse switch from Out to in */
var float PulseSplit;
/** How much should the text pulse - NOTE this will be added to 1.0 (so PulseMultipler 0.5 = 1.5) */
var float PulseMultiplier;


/******************************************************************************************
 Localize Strings -- TODO - Go through and make sure these are all localized
 ******************************************************************************************/

var localized string WarmupString;				// displayed when playing warmup round
var localized string WaitingForMatch;			// Waiting for the match to begin
var localized string SpectatorMessage;			// When you are a spectator
var localized string DeadMessage;				// When you are dead
var localized string FireToRespawnMessage;  	// Press [Fire] to Respawn
var localized string YouHaveWon;				// When you win the match
var localized string YouHaveLost;				// You have lost the match
var localized string ConstructioningMessage;	// Constructioning Meshes

var localized string BoostMessage;
var localized string EjectMessage;

var localized string PlaceMarks[4];

/******************************************************************************************
 Misc vars used for laying out the hud
 ******************************************************************************************/

var float THeight;
var float TX;
var float TY;

/**
 * Draw a glowing string
 */
native function DrawGlowText(string Text, float X, float Y, optional float MaxHeightInPixels=0.0, optional float PulseScale=1.0, optional bool bRightJustified);

/**
 * Draws a textured centered around the current position
 */
function DrawTileCentered(texture2D Tex, float xl, float yl, float u, float v, float ul, float vl, LinearColor C)
{
	local float x,y;

	x = Canvas.CurX - (xl * 0.5);
	y = Canvas.CurY - (yl * 0.5);

	Canvas.SetPos(x,y);
	Canvas.DrawColorizedTile(Tex, xl,yl,u,v,ul,vl,C);
}

function SetDisplayedOrders(string OrderText)
{
	DisplayedOrders = OrderText;
}

/**
 * This function will attempt to auto-link up HudWidgets to their associated transient
 * property here in the hud.
 */
native function LinkToHudScene();

/**
 * Create a list of actors needing post renders for.  Also Create the Hud Scene
 */
simulated function PostBeginPlay()
{
	local Pawn P;
	local UTGameObjective O;
	local UTDeployableNodeLocker DNL;
	local UTOnslaughtFlag F;
	local int i;

	super.PostBeginPlay();

	SetTimer(1.0, true);

	UTPlayerOwner = UTPlayerController(PlayerOwner);

	// add actors to the PostRenderedActors array
	ForEach DynamicActors(class'Pawn', P)
	{
		if ( (UTPawn(P) != None) || (UTVehicle(P) != None) )
			AddPostRenderedActor(P);
	}

	ForEach WorldInfo.AllNavigationPoints(class'UTGameObjective',O)
	{
		AddPostRenderedActor(O);
	}

	ForEach AllActors(class'UTDeployableNodeLocker',DNL)
	{
		AddPostRenderedActor(DNL);
	}

	ForEach AllActors(class'UTOnslaughtFlag',F)
	{
		AddPostRenderedActor(F);
	}

	if ( UTConsolePlayerController(PlayerOwner) != None )
	{
		bShowOnlyAvailableWeapons = true;
		bNoWeaponNumbers = true;
	}

	// Cache data that will be used a lot

	UTPlayerOwner = UTPlayerController(Owner);
	UTOwnerPRI = UTPlayerReplicationInfo(UTPlayerOwner.PlayerReplicationInfo);

	ScoreboardScene = UTUIScene_Scoreboard( OpenScene( ScoreboardSceneTemplate ));
	if (ScoreboardScene != none )
	{
		ScoreboardScene.SetVisibility(false);
	}
	else
	{
		`log("Could not instance ScoreboardScene:"@ScoreboardSceneTemplate);
		return;
	}

	// Setup Damage indicators,etc.

	// Create the 3 Damage Constants

	DamageData.Length = MaxNoOfIndicators;

	for (i = 0; i < MaxNoOfIndicators; i++)
	{
		DamageData[i].FadeTime = 0.0f;
		DamageData[i].FadeValue = 0.0f;
		DamageData[i].MatConstant = new(self) class'MaterialInstanceConstant';
		if (DamageData[i].MatConstant != none && BaseMaterial != none)
		{
			DamageData[i].MatConstant.SetParent(BaseMaterial);
		}
	}

	// create hit effect material instance

	HitEffect = MaterialEffect(LocalPlayer(UTPlayerOwner.Player).PlayerPostProcess.FindPostProcessEffect('HitEffect'));
	if (HitEffect != None)
	{
		if (MaterialInstanceConstant(HitEffect.Material) != None && HitEffect.Material.GetPackageName() == 'Transient')
		{
			// the runtime material already exists; grab it
			HitEffectMaterialInstance = MaterialInstanceConstant(HitEffect.Material);
		}
		else
		{
			HitEffectMaterialInstance = new(HitEffect) class'MaterialInstanceConstant';
			HitEffectMaterialInstance.SetParent(HitEffect.Material);
			HitEffect.Material = HitEffectMaterialInstance;
		}
		HitEffect.bShowInGame = false;
	}
}

/**
 * Given a default screen position (at 1024x768) this will return the hud position at the current resolution.
 * NOTE: If the default position value is < 0.0f then it will attempt to place the right/bottom face of
 * the "widget" at that offset from the ClipX/Y.
 *
 * @Param Position		The default position (in 1024x768 space)
 * @Param Width			How wide is this "widget" at 1024x768
 * @Param Height		How tall is this "widget" at 1024x768
 *
 * @returns the hud position
 */
function Vector2D ResolveHUDPosition(vector2D Position, float Width, float Height)
{
	local vector2D FinalPos;
	FinalPos.X = (Position.X < 0) ? Canvas.ClipX - (Position.X * ResolutionScale) - (Width * ResolutionScale)  : Position.X * ResolutionScale;
	FinalPos.Y = (Position.Y < 0) ? Canvas.ClipY - (Position.Y * ResolutionScale) - (Height * ResolutionScale) : Position.Y * ResolutionScale;

	return FinalPos;
}


/* toggles displaying scoreboard (used by console controller)
*/
exec function ReleaseShowScores()
{
	// make sure that release will turn off scores by setting true before toggling
	bShowScores = true;
	ShowScores();
}

function GetScreenCoords(float PosY, out float ScreenX, out float ScreenY, out HudLocalizedMessage InMessage )
{
	local float Offset;

	if ( PosY > 1.0 )
	{
		// position under minimap
		Offset = PosY - int(PosY);
		if ( Offset < 0 )
		{
			Offset = Offset + 1.0;
		}
		ScreenY = (0.38 + Offset) * Canvas.ClipY;
		ScreenX = 0.98 * Canvas.ClipX - InMessage.DX;
		return;
	}

    ScreenX = 0.5 * Canvas.ClipX;
    ScreenY = (PosY * HudCanvasScale * Canvas.ClipY) + (((1.0f - HudCanvasScale) * 0.5f) * Canvas.ClipY);

    ScreenX -= InMessage.DX * 0.5;
    ScreenY -= InMessage.DY * 0.5;
}


function DrawMessageText(HudLocalizedMessage LocalMessage, float ScreenX, float ScreenY)
{
	local color CanvasColor;
	local string StringMessage;
	local class<UTDamageType> KillDamageType;

	StringMessage = LocalMessage.StringMessage;
	if ( LocalMessage.Count > 0 )
	{
		if ( Right(StringMessage, 1) ~= "." )
		{
			StringMessage = Left(StringMessage, Len(StringMessage) -1);
		}
		StringMessage = StringMessage$" X "$LocalMessage.Count;
	}

	CanvasColor = Canvas.DrawColor;

	// first draw drop shadow string
	Canvas.DrawColor = BlackColor;
	Canvas.DrawColor.A = CanvasColor.A;
	Canvas.SetPos( ScreenX+2, ScreenY+2 );
	Canvas.DrawTextClipped( StringMessage, false );

	// now draw string with normal color
	Canvas.DrawColor = CanvasColor;
	Canvas.SetPos( ScreenX, ScreenY );

	// Draw kill weapon
	if ( class<UTVictimMessage>(LocalMessage.Message) != None )
	{
		KillDamageType = class<UTDamageType>(LocalMessage.OptionalObject);
		if ( KillDamageType!= None )
		{
			Canvas.DrawTextClipped( StringMessage$"  ", false );
			KillDamageType.static.DrawKillIcon(Canvas, Canvas.ClipX-ScreenX, ScreenY, HUDScaleX, HUDScaleY);
		}
	}
	else
	{
		Canvas.DrawTextClipped( StringMessage, false );
	}
}

/**
 * Perform any value precaching, and set up various safe regions
 *
 * NOTE: NO DRAWING should ever occur in PostRender.  Put all drawing code in DrawHud().
 */
event PostRender()
{
	local int TeamIndex;
	local float x,y,w,h;

    // Clear the flag
    bHudMessageRendered = false;

    PawnOwner = Pawn(PlayerOwner.ViewTarget);

	UTPawnOwner = UTPawn(PawnOwner);
	if ( UTPawnOwner == none )
	{
		if ( UTVehicleBase(PawnOwner) != none )
		{
			UTPawnOwner = UTPawn( UTVehicleBase(PawnOwner).Driver);
		}
	}


	if (Vehicle(PawnOwner) != none )

	// Cache the current Team Index of this hud and the GRI

	TeamIndex = 2;
	if ( PawnOwner != None )
	{
		if ( (PawnOwner.PlayerReplicationInfo != None) && (PawnOwner.PlayerReplicationInfo.Team != None) )
			TeamIndex = PawnOwner.PlayerReplicationInfo.Team.TeamIndex;
	}
	else if ( (PlayerOwner.PlayerReplicationInfo != None) && (PlayerOwner.PlayerReplicationInfo.team != None) )
	{
		TeamIndex = PlayerOwner.PlayerReplicationInfo.Team.TeamIndex;
	}

	UTGRI = UTGameReplicationInfo(WorldInfo.GRI);

	HUDScaleX = Canvas.ClipX/1280;
	HUDScaleY = Canvas.ClipX/1280;

	ResolutionScale = Canvas.ClipY / 768;

	GetTeamColor(TeamIndex, TeamHUDColor, TeamTextColor);

	TeamColor.R = TeamHUDColor.R * 256;
	TeamColor.G = TeamHUDColor.G * 256;
	TeamColor.B = TeamHUDColor.B * 256;
	TeamColor.A = TeamHUDColor.A * 256;

	FullWidth = Canvas.ClipX;
	FullHeight = Canvas.ClipY;

	// Create the safe region

	w = FullWidth * SafeRegionPct;
	h = FullHeight * SafeRegionPct;

	X = Canvas.OrgX + (Canvas.ClipX - w) * 0.5;
	Y = Canvas.OrgY + (Canvas.ClipY - h) * 0.5;

	Canvas.OrgX = X;
	Canvas.OrgY = Y;
	Canvas.ClipX = w;
	Canvas.ClipY = h;

	Canvas.Reset(true);

	// Handle displaying the scoreboard.  Allow the Mid Game Menu to override displaying
	// it.

	if ( bShowScores && UTPlayerOwner.CurrentMidGameMenu == none )
	{
		if ( ScoreboardScene != none )
		{
			if ( ScoreboardScene.IsHidden() )
			{
				ScoreboardScene.SetVisibility(true);
				ScoreboardScene.TickScene(0);
			}
		}
		else
		{
			if ( ScoreBoard != None )
			{
				ScoreBoard.Canvas = Canvas;
				ScoreBoard.DrawHud();
				if ( Scoreboard.bDisplayMessages )
				{
					DisplayConsoleMessages();
				}
			}
		}

		return;
	}
	else
	{
		if ( ScoreboardScene != none )
		{
			if ( ScoreboardScene.IsVisible() )
			{
				ScoreboardScene.SetVisibility(false);
			}
		}
	}

	DrawHud();
}

/**
 * This is the main drawing pump.  It will determine which hud we need to draw (Game or PostGame).  Any drawing that should occur
 * regardless of the game state should go here.
 */
function DrawHUD()
{
	// Set up delta time
	RenderDelta = WorldInfo.TimeSeconds - LastHUDRenderTime;
	LastHUDRenderTime = WorldInfo.TimeSeconds;

	// Handle constructioning meshes

	if ( UTPlayerOwner.bConstructioningMeshes )
	{
		DisplayHUDMessage(ConstructioningMessage);
	}

	// If we are not over, draw the hud

	if ( !UTGRI.bMatchIsOver )
	{
		DrawGameHud();
	}
	else	// Match is over
	{
		DrawPostGameHud();
	}

	LastHUDUpdateTime = WorldInfo.TimeSeconds; // @TODO FIXMESTEVE - pass deltatime to PostRender()!!!
}

/**
 * This function is called to draw the hud while the game is still in progress.  You should only draw items here
 * that are always displayed.  If you want to draw something that is displayed only when the player is alive
 * use DrawLivingHud().
 */
function DrawGameHud()
{
	local float xl, yl, ypos;

	// Draw any spectator information

	if ( UTOwnerPRI.bOnlySpectator )
	{
		DisplayHUDMessage(SpectatorMessage);
	}
	else if ( UTOwnerPRI.bIsSpectator )
	{
		DisplayHUDMessage(WaitingForMatch);
	}
	else if ( UTPlayerOwner.IsDead() )
	{
	 	DisplayHUDMessage(DeadMessage @ (UTPlayerOwner.bFrozen) ? "" : FireToRespawnMessage);
	}

	// Draw the Warmup if needed

	if (UTGRI.bWarmupRound)
	{
		Canvas.Font = GetFontSizeIndex(2);
		Canvas.DrawColor = WhiteColor;
		Canvas.StrLen(WarmupString, XL, YL);
		Canvas.SetPos((Canvas.ClipX - XL) * 0.5, Canvas.ClipY * 0.175);
		Canvas.DrawText(WarmupString);
	}

	// Draw actors in the world

	DrawActorOverlays();

	// Allow the PC to draw a hud

	if ( PlayerOwner != none )
	{
	    PlayerOwner.DrawHud( Self );
	}

	if ( bCrosshairOnFriendly )
	{
		// verify that crosshair trace might hit friendly
		CheckCrosshairOnFriendly();
		bCrosshairOnFriendly = false;
	}

	if ( bShowDebugInfo )
	{
		Canvas.Font = GetFontSizeIndex(0);
		Canvas.DrawColor = ConsoleColor;
		Canvas.StrLen("X", XL, YL);
		YPos = 0;
		PlayerOwner.ViewTarget.DisplayDebug(self, YL, YPos);

		if (ShouldDisplayDebug('AI') && (Pawn(PlayerOwner.ViewTarget) != None))
		{
			DrawRoute(Pawn(PlayerOwner.ViewTarget));
		}
		return;
	}

	if (bShowAllAI)
	{
		DrawAIOverlays();
	}

	DisplayLocalMessages();
	DisplayConsoleMessages();

	if ( bShowWeaponBar )
	{
		DisplayWeaponBar();
	}

	// The weaponbar potentially tweaks TeamHUDColor's Alpha.  Reset it here
	TeamHudColor.A = 1.0;

	// Draw the character portrait
	if ( CharPRI != none  )
	{
		DisplayPortrait(RenderDelta);
	}

	if ( bShowClock )
	{
   		DisplayClock();
   	}

	if ( bShowMap )
	{
		DisplayMap();
	}

	if ( bShowScoring )
	{
		DisplayScoring();
	}

	// If the player isn't dead, draw the living hud

	if ( !UTPlayerOwner.IsDead() )
	{
		DrawLivingHud();
	}

	if ( bShowDamage )
	{
		DisplayDamage();
	}

}

/**
 * Anything drawn in this function will be displayed ONLY when the player is living.
 */
function DrawLivingHud()
{
    local UTWeapon Weapon;

	// Pawn Doll

	if ( bShowDoll && UTPawnOwner != none )
	{
		DisplayPawnDoll();
	}

	// If we are driving a vehicle, give it hud time
	if ( bShowVehicle && UTVehicleBase(PawnOwner) != none )
	{
		UTVehicleBase(PawnOwner).DisplayHud(self, Canvas, VehiclePosition);
	}

	// Powerups

	if ( bShowPowerups && UTPawnOwner != none && UTPawnOwner.InvManager != none )
	{
		DisplayPowerups();
	}

	// Manage the weapon.  NOTE: Vehicle weapons are managed by the vehicle
	// since they are integrated in to the vehicle health bar

	if( PawnOwner != none )
	{
		Weapon = UTWeapon(PawnOwner.Weapon);
		if ( Weapon != none && UTVehicleWeapon(Weapon) == none )
		{
			if ( bShowAmmo )
			{
				DisplayAmmo(Weapon);
			}
		}
	}
}


/**
 * This function is called when we are drawing the hud but the match is over.
 */

function DrawPostGameHud()
{
	local bool bWinner;
	if ( UTPlayerReplicationInfo(WorldInfo.GRI.Winner) != none )
	{
		bWinner = UTPlayerReplicationInfo(WorldInfo.GRI.Winner) == UTOwnerPRI;
	}
	else
	{
		bWinner = WorldInfo.GRI.Winner.GetTeamNum() == UTPlayerOwner.GetTeamNum();
	}

	DisplayHUDMessage((bWinner ? YouHaveWon : YouHaveLost));
}

/**
 * This function is used to determine a scaling factor that is applied to some of the widgets
 * when an event happens (such as gaining health).
 */
function CalculatePulse(out float PulseScale, out float PulseTime)
{
	local float PulsePct;
	local float PulseChangeAt;

	PulsePct = PulseTime / PulseDuration;
	PulseChangeAt = (1.0 - PulseSplit);

	if (PulsePct >= PulseChangeAt) // Growing
	{
		PulseScale = 1.0 + (PulseMultiplier * (1.0 - ( (PulsePct - PulseChangeAt) / PulseSplit)));
	}
	else
	{
		PulseScale = 1.0 + (PulseMultiplier * (PulsePct / PulseChangeAt));
	}
	PulseTime -= RenderDelta;
}



function CheckCrosshairOnFriendly()
{
	local float Size;
	local vector HitLocation, HitNormal, StartTrace, EndTrace;
	local actor HitActor;
	local UTVehicle V;
	local UTWeapon W;
	local int SeatIndex;

	if ( PawnOwner == None )
	{
		return;
	}

	V = UTVehicle(PawnOwner);
	if ( V != None )
	{
		for ( SeatIndex=0; SeatIndex<V.Seats.Length; SeatIndex++ )
		{
			if ( V.Seats[SeatIndex].SeatPawn == PawnOwner )
			{
				HitActor = V.Seats[SeatIndex].AimTarget;
				break;
			}
		}
	}
	else
	{
		W = UTWeapon(PawnOwner.Weapon);
		if ( W != None )
		{
			StartTrace = W.InstantFireStartTrace();
			EndTrace = W.InstantFireEndTrace(StartTrace);
			HitActor = PawnOwner.Trace(HitLocation, HitNormal, EndTrace, StartTrace, true, vect(0,0,0),, TRACEFLAG_Bullet);

			if ( Pawn(HitActor) == None )
			{
				if ( UTWalkerBody(HitActor) != None )
				{
					HitActor = UTWalkerBody(HitActor).WalkerVehicle;
				}
				else
				{
					HitActor = (HitActor == None) ? None : Pawn(HitActor.Base);
				}
			}
		}
	}

	if ( (Pawn(HitActor) == None) || !Worldinfo.GRI.OnSameTeam(HitActor, PawnOwner) )
	{
		return;
	}

	// if trace hits friendly, draw "no shoot" symbol
	Size = 28 * (Canvas.ClipY / 768);
	Canvas.SetPos( (Canvas.ClipX * 0.5) - (Size *0.5), (Canvas.ClipY * 0.5) - (Size * 0.5) );
	Canvas.SetDrawColor(255,255,255,255);
	Canvas.DrawTile(texture2d'T_UTHudGraphics.Textures.UTOldHUD', Size, Size, 235,323,56,56);
}

/*
*/
function DisplayWeaponBar()
{
	local int i, SelectedWeapon, LastWeapIndex, PrevWeapIndex, NextWeapIndex;
	local float TotalOffsetX, OffsetX, OffsetY, BoxOffsetSize, OffsetSizeX, OffsetSizeY, DesiredWeaponScale[10];
	local UTWeapon W;
	local UTVehicle V;
	local float Delta;
	local linearcolor AmmoBarColor;
	local float SelectedAmmoBarX, SelectedAmmoBarY;

	if ( (PawnOwner == None) || (bOnlyShowWeaponBarIfChanging && (WorldInfo.TimeSeconds - LastWeaponBarDrawnTime > 0.5)) )
	{
		return;
	}
	if ( (PawnOwner.Weapon == None) || (PawnOwner.InvManager == None) || (UTVehicle(PawnOwner) != None) )
	{
		V = UTVehicle(PawnOwner);
		if ( (V != None) && V.bHasWeaponBar )
		{
			V.DisplayWeaponBar(Canvas, self);
		}
		return;
	}

	// FIXME manage weapon list based on weapon add/discard from inventorymanager
	for ( i=0; i<10; i++ )
	{
		WeaponList[i] = None;
	}
	ForEach PawnOwner.InvManager.InventoryActors(class'UTWeapon', W)
	{
		if ( (W.InventoryGroup < 11) && (W.InventoryGroup > 0) )
			WeaponList[W.InventoryGroup-1] = W;
	}

	if ( PawnOwner.InvManager.PendingWeapon != None )
	{
		LastWeaponBarDrawnTime = WorldInfo.TimeSeconds;
	}

	SelectedWeapon = (PawnOwner.InvManager.PendingWeapon != None) ? UTWeapon(PawnOwner.InvManager.PendingWeapon).InventoryGroup-1 : UTWeapon(PawnOwner.Weapon).InventoryGroup-1;
	Delta = WeaponScaleSpeed * (WorldInfo.TimeSeconds - LastHUDUpdateTime);
	BoxOffsetSize = HUDScaleX * WeaponBarScale * WeaponBoxWidth;
	PrevWeapIndex = -1;
	NextWeapIndex = -1;
	LastWeapIndex = -1;

	if ( (PawnOwner.InvManager.PendingWeapon != None) && (PawnOwner.InvManager.PendingWeapon != LastSelectedWeapon) )
	{
		LastSelectedWeapon = PawnOwner.InvManager.PendingWeapon;

		// clear any pickup messages for this weapon
		for ( i=0; i<8; i++ )
		{
			if( LocalMessages[i].Message == None )
			{
				break;
			}
			if( LocalMessages[i].OptionalObject == LastSelectedWeapon.default.Class )
			{
				LocalMessages[i].EndOfLife = WorldInfo.TimeSeconds - 1.0;
				break;
			}
		}

		PlayerOwner.ReceiveLocalizedMessage( class'UTWeaponSwitchMessage',,,, LastSelectedWeapon );
	}

	// calculate offsets
	for ( i=0; i<10; i++ )
	{
		if ( WeaponList[i] != None )
		{
			// optimization if needed - cache desiredweaponscale[] when pending weapon changes
			if ( SelectedWeapon == i )
			{
				PrevWeapIndex = LastWeapIndex;
				if ( BouncedWeapon == i )
				{
					DesiredWeaponScale[i] = SelectedWeaponScale;
				}
				else
				{
					DesiredWeaponScale[i] = BounceWeaponScale;
					if ( CurrentWeaponScale[i] >= DesiredWeaponScale[i] )
					{
						BouncedWeapon = i;
					}
				}
			}
			else
			{
				if ( LastWeapIndex == SelectedWeapon )
				{
					NextWeapIndex = i;
				}
				DesiredWeaponScale[i] = 1.0;
			}
			if ( CurrentWeaponScale[i] != DesiredWeaponScale[i] )
			{
				if ( DesiredWeaponScale[i] > CurrentWeaponScale[i] )
				{
					CurrentWeaponScale[i] = FMin(CurrentWeaponScale[i]+Delta,DesiredWeaponScale[i]);
				}
				else
				{
					CurrentWeaponScale[i] = FMax(CurrentWeaponScale[i]-Delta,DesiredWeaponScale[i]);
				}
			}
			TotalOffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
			LastWeapIndex = i;
		}
		else if ( bShowOnlyAvailableWeapons )
		{
			CurrentWeaponScale[i] = 0;
		}
		else
		{
			// draw empty background
			CurrentWeaponScale[i] = 1.0;
			TotalOffsetX += BoxOffsetSize;
		}
	}
	if ( !bShowOnlyAvailableWeapons )
	{
		PrevWeapIndex = SelectedWeapon - 1;
		NextWeapIndex = SelectedWeapon + 1;
	}

	OffsetX = HUDScaleX * WeaponBarXOffset + 0.5 * (Canvas.ClipX - TotalOffsetX);
	OffsetY = Canvas.ClipY - HUDScaleY * WeaponBarY;

	// @TODO - manually reorganize canvas calls, or can this be automated?
	// draw weapon boxes
	Canvas.SetDrawColor(255,255,255,255);
	OffsetSizeX = HUDScaleX * WeaponBarScale * 96 * SelectedBoxScale;
	OffsetSizeY = HUDScaleY * WeaponBarScale * 64 * SelectedBoxScale;
	AmmoBarColor = MakeLinearColor(10.0,10.0,10.0,1.0);

	for ( i=0; i<10; i++ )
	{
		if ( WeaponList[i] != None )
		{
			Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
			if ( SelectedWeapon == i )
			{
				//Current slot overlay
				TeamHudColor.A = SelectedWeaponAlpha;
				Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 530, 248, 69, 49, TeamHUDColor);

				Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
				Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 459, 148, 69, 49, TeamHUDColor);

				Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
				Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 459, 248, 69, 49, TeamHUDColor);

				// draw ammo bar ticks for selected weapon
				SelectedAmmoBarX = HUDScaleX * (SelectedWeaponAmmoOffsetX - WeaponBarXOffset) + OffsetX;
				SelectedAmmoBarY = Canvas.ClipY - HUDScaleY * (WeaponBarY + CurrentWeaponScale[i]*WeaponAmmoOffsetY);
				Canvas.SetPos(SelectedAmmoBarX, SelectedAmmoBarY);
				Canvas.DrawTileStretched(AltHudTexture, CurrentWeaponScale[i]*HUDScaleY * WeaponBarScale * WeaponAmmoLength, CurrentWeaponScale[i]*HUDScaleY*WeaponBarScale* WeaponAmmoThickness, 407, 479, 118, 16, AmmoBarColor);
			}
			else
			{
				TeamHudColor.A = OffWeaponAlpha;
				Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 459, 148, 69, 49, TeamHUDColor);

				// draw slot overlay?
				if ( i == PrevWeapIndex )
				{
					Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
					Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 530, 97, 69, 49, TeamHUDColor);
				}
				else if ( i == NextWeapIndex )
				{
					Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
					Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 530, 148, 69, 49, TeamHUDColor);
				}
			}
			OffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
		}
		else if ( !bShowOnlyAvailableWeapons )
		{
			TeamHudColor.A = EmptyWeaponAlpha;
			Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
			Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 459, 97, 69, 49, TeamHUDColor);

			// draw slot overlay?
			if ( i == PrevWeapIndex )
			{
				Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
				Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 530, 198, 69, 49, TeamHUDColor);
			}
			else if ( i == NextWeapIndex )
			{
				Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
				Canvas.DrawColorizedTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, 459, 198, 69, 49, TeamHUDColor);
			}
			OffsetX += BoxOffsetSize;
		}
	}

	// draw weapon ammo bars
	// Ammo Bar:  273,494 12,13 (The ammo bar is meant to be stretched)
	Canvas.SetDrawColor(255,255,255,255);
	OffsetX = HUDScaleX * WeaponAmmoOffsetX + 0.5 * (Canvas.ClipX - TotalOffsetX);
	OffsetSizeY = HUDScaleY * WeaponBarScale * WeaponAmmoThickness;
	AmmoBarColor = MakeLinearColor(1.0,10.0,1.0,1.0);
	for ( i=0; i<10; i++ )
	{
		if ( WeaponList[i] != None )
		{
			if ( SelectedWeapon == i )
			{
				Canvas.SetPos(SelectedAmmoBarX, SelectedAmmoBarY);
			}
			else
			{
				Canvas.SetPos(OffsetX, Canvas.ClipY - HUDScaleY * (WeaponBarY + CurrentWeaponScale[i]*WeaponAmmoOffsetY));
			}
			Canvas.DrawTileStretched(AltHudTexture, HUDScaleY * WeaponBarScale * WeaponAmmoLength*CurrentWeaponScale[i]*FMin(1.0,float(WeaponList[i].AmmoCount)/float(WeaponList[i].MaxAmmoCount)), CurrentWeaponScale[i]*OffsetSizeY, 273, 494,12,13,AmmoBarColor);
		}
		OffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
	}

	// draw weapon numbers
	if ( !bNoWeaponNumbers )
	{
		OffsetX = HUDScaleX * (WeaponAmmoOffsetX + WeaponXOffset) * 0.5 + 0.5 * (Canvas.ClipX - TotalOffsetX);
		OffsetY = Canvas.ClipY - HUDScaleY * (WeaponBarY + WeaponYOffset);
		Canvas.SetDrawColor(255,255,255,255);
		Canvas.Font = GetFontSizeIndex(0);
		for ( i=0; i<10; i++ )
		{
			if ( WeaponList[i] != None )
			{
				Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
				Canvas.DrawText(int((i+1)%10), false);
				OffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
			}
			else if ( !bShowOnlyAvailableWeapons )
			{
				OffsetX += BoxOffsetSize;
			}
		}
	}

	// draw weapon icons
	OffsetX = HUDScaleX * WeaponXOffset + 0.5 * (Canvas.ClipX - TotalOffsetX);
	OffsetY = Canvas.ClipY - HUDScaleY * (WeaponBarY + WeaponYOffset);
	OffsetSizeX = HUDScaleX * WeaponBarScale * 100;
	OffsetSizeY = HUDScaleY * WeaponBarScale * WeaponYScale;
	Canvas.SetDrawColor(255,255,255,255);
	for ( i=0; i<10; i++ )
	{
		if ( WeaponList[i] != None )
		{
			Canvas.SetPos(OffsetX, OffsetY - OffsetSizeY*CurrentWeaponScale[i]);
			Canvas.DrawTile(AltHudTexture, CurrentWeaponScale[i]*OffsetSizeX, CurrentWeaponScale[i]*OffsetSizeY, WeaponList[i].IconCoordinates.U, WeaponList[i].IconCoordinates.V, WeaponList[i].IconCoordinates.UL, WeaponList[i].IconCoordinates.VL);
			OffsetX += CurrentWeaponScale[i] * BoxOffsetSize;
		}
		else if ( !bShowOnlyAvailableWeapons )
		{
			OffsetX += BoxOffsetSize;
		}
	}
}

/**
 * Draw the Map
 */
function DisplayMap()
{
	local UTMapInfo MI;
	local float ScaleY, W,H,X,Y, ScreenX, ScreenY, XL, YL;
	local color CanvasColor;

	// draw orders
	Canvas.Font = GetFontSizeIndex(2);
	Canvas.StrLen(DisplayedOrders, XL, YL);
	ScreenY = 0.02 * Canvas.ClipY;
	ScreenX = 0.98 * Canvas.ClipX - XL;

	CanvasColor = Canvas.DrawColor;

	// first draw drop shadow string
	Canvas.DrawColor = BlackColor;
	Canvas.SetPos( ScreenX+2, ScreenY+2 );
	Canvas.DrawTextClipped( DisplayedOrders, false );

	// now draw string with normal color
	Canvas.DrawColor = GoldColor;
	Canvas.SetPos( ScreenX, ScreenY );
	Canvas.DrawTextClipped( DisplayedOrders, false );
	Canvas.DrawColor = CanvasColor;

	// draw map
	MI = UTMapInfo( WorldInfo.GetMapInfo() );
	if ( MI != none )
	{
		ScaleY = Canvas.ClipY / 768;
		H = MapDefaultSize * ScaleY;
		W = MapDefaultSize * ScaleY;

		X = (MapPosition.X < 0.f) ? Canvas.ClipX - (Canvas.ClipX * Abs(MapPosition.X)) - W : (Canvas.ClipX * MapPosition.X);
		Y = (MapPosition.Y < 0.f) ? Canvas.ClipY - (Canvas.ClipY * Abs(MapPosition.Y)) - H : (Canvas.ClipY * MapPosition.Y);

		if ( MapBackground != none )
		{
			Canvas.SetPos(X,Y);
			Canvas.DrawColorizedTile(MapBackground, W, H, 0 ,0, 1024, 1024, TeamHudColor);
		}

		MI.DrawMap(Canvas, UTPlayerController(PlayerOwner), X, Y, W ,H, false, (Canvas.ClipX / Canvas.ClipY) );
	}
}

/** draws AI goal overlays over each AI pawn */
function DrawAIOverlays()
{
	local UTBot B;
	local vector Pos;
	local float XL, YL;
	local string Text;

	Canvas.Font = GetFontSizeIndex(0);

	foreach WorldInfo.AllControllers(class'UTBot', B)
	{
		if (B.Pawn != None)
		{
			// draw route
			DrawRoute(B.Pawn);
			// draw goal string
			if ((vector(PlayerOwner.Rotation) dot (B.Pawn.Location - PlayerOwner.ViewTarget.Location)) > 0.f)
			{
				Pos = Canvas.Project(B.Pawn.Location + B.Pawn.GetCollisionHeight() * vect(0,0,1.1));
				Text = B.GetHumanReadableName() $ ":" @ B.GoalString;
				Canvas.StrLen(Text, XL, YL);
				Pos.X = FClamp(Pos.X, 0.f, Canvas.ClipX - XL);
				Pos.Y = FClamp(Pos.Y, 0.f, Canvas.ClipY - YL);
				Canvas.SetPos(Pos.X, Pos.Y);
				if (B.PlayerReplicationInfo != None && B.PlayerReplicationInfo.Team != None)
				{
					Canvas.DrawColor = B.PlayerReplicationInfo.Team.GetHUDColor();
					// brighten the color a bit
					Canvas.DrawColor.R = Min(Canvas.DrawColor.R + 64, 255);
					Canvas.DrawColor.G = Min(Canvas.DrawColor.G + 64, 255);
					Canvas.DrawColor.B = Min(Canvas.DrawColor.B + 64, 255);
				}
				else
				{
					Canvas.DrawColor = ConsoleColor;
				}
				Canvas.DrawColor.A = LocalPlayer(PlayerOwner.Player).GetActorVisibility(B.Pawn) ? 255 : 128;
				Canvas.DrawText(Text);
			}
		}
	}
}

/* DrawActorOverlays()
draw overlays for actors that were rendered this tick
*/
function DrawActorOverlays()
{
	local int i;
	local Actor A;
	local vector ViewPoint, ViewDir;
	local rotator ViewRotation;

	if ( !bShowHud )
		return;
	Canvas.Font = GetFontSizeIndex(0);

	// determine rendered camera position
	PlayerOwner.GetPlayerViewPoint(ViewPoint, ViewRotation);
	ViewDir = vector(ViewRotation);

	for ( i=0; i<PostRenderedActors.Length; i++ )
	{
		A = PostRenderedActors[i];
		if ( A != None )
		{
			A.PostRenderFor(PlayerOwner,Canvas,ViewPoint,ViewDir);
		}
	}
}


/************************************************************************************************************
 * Accessors for the UI system for opening scenes (scoreboard/menus/etc)
 ***********************************************************************************************************/

function UIInteraction GetUIController(optional out LocalPlayer LP)
{
	LP = LocalPlayer(PlayerOwner.Player);
	if ( LP != none )
	{
		return LP.ViewportClient.UIController;
	}

	return none;
}

/**
 * OpenScene - Opens a UIScene
 *
 * @Param Template	The scene template to open
 */
function UTUIScene OpenScene(UTUIScene Template)
{
	return UTUIScene(UTPlayerOwner.OpenUIScene(Template));
}


/************************************************************************************************************
 Misc / Utility functions
************************************************************************************************************/

exec function ToggleHUD()
{
	bShowHUD = !bShowHUD;
}


function SpawnScoreBoard(class<Scoreboard> ScoringType)
{

	Super.SpawnScoreBoard(ScoringType);
	if (UTPlayerOwner.Announcer == None)
	{
		UTPlayerOwner.Announcer = Spawn(class'UTAnnouncer', UTPlayerOwner);
	}
	UTPlayerOwner.Announcer.PrecacheAnnouncements();

	if (UTPlayerOwner.MusicManager == None)
	{
		UTPlayerOwner.MusicManager = Spawn(MusicManagerClass, UTPlayerOwner);
	}
}

exec function StartMusic()
{
	if (UTPlayerOwner.MusicManager == None)
	{
		UTPlayerOwner.MusicManager = Spawn(MusicManagerClass, UTPlayerOwner);
	}
}

static simulated function GetTeamColor(int TeamIndex, optional out LinearColor ImageColor, optional out Color TextColor)
{
	switch ( TeamIndex )
	{
		case 0 :
			ImageColor = MakeLinearColor(3.0, 0.0, 0.05, 0.8);
			TextColor = Makecolor(255,255,0,255);
			break;
		case 1 :
			ImageColor = MakeLinearColor(0.5, 0.8, 10.0, 0.8);
			TextColor = Makecolor(255,255,0,255);
			break;
		default:
			ImageColor = MakeLinearColor(4.0, 2.0, 0.5, 0.5);
			TextColor = Makecolor(0,0,0,255);
			break;
	}
}


/************************************************************************************************************
 Damage Indicator
************************************************************************************************************/

exec function TestDamage()
{
	DisplayHit(Vector(PawnOwner.Rotation), 80, class'UTDmgType_Enforcer');
}

/**
 * Called from various functions.  It allows the hud to track when a hit is scored
 * and display any indicators.
 *
 * @Param	HitDir		- The vector to which the hit came at
 * @Param	Damage		- How much damage was done
 * @Param	DamageType  - Type of damage
 */
function DisplayHit(vector HitDir, int Damage, class<DamageType> damageType)
{
	local Vector Loc;
	local Rotator Rot;
	local float DirOfHit_L;
	local vector AxisX, AxisY, AxisZ;
	local vector ShotDirection;
	local bool bIsInFront;
	local vector2D	AngularDist;
	local float PositionInQuadrant;
	local float QuadrantStart;
	local float Multiplier;
	local float DamageIntensity;


	if ( !bShowDamage )
	{
		return;
	}

	if ( (PawnOwner != None) && (PawnOwner.Health > 0) )
	{
		DamageIntensity = PawnOwner.InGodMode() ? 0.5 : (float(Damage)/100.0 + float(Damage)/float(PawnOwner.Health));
	}
	else
	{
		DamageIntensity = 0.02*float(Damage);
	}

	// Figure out the directional based on the victims current view

	PlayerOwner.GetPlayerViewPoint(Loc, Rot);
	GetAxes(PawnOwner.Rotation, AxisX, AxisY, AxisZ);

	ShotDirection = Normal(HitDir - PawnOwner.Location);
	bIsInFront = GetAngularDistance( AngularDist, ShotDirection, AxisX, AxisY, AxisZ);
	GetAngularDegreesFromRadians(AngularDist);

	Multiplier = 0.26f / 90.f;
	PositionInQuadrant = Abs(AngularDist.X) * Multiplier;

	// 0 - .25  UpperRight
	// .25 - .50 LowerRight
	// .50 - .75 LowerLeft
	// .75 - 1 UpperLeft

	if( bIsInFront == TRUE )
	{
	   if( AngularDist.X > 0 )
	   {
		   QuadrantStart = 0;
		   DirOfHit_L = QuadrantStart + PositionInQuadrant;
	   }
	   else
	   {
		   QuadrantStart = 0;
		   DirOfHit_L = QuadrantStart - PositionInQuadrant;
	   }
	}
	else
	{
		if( AngularDist.X > 0 )
		{
		   QuadrantStart = 0.52;
		   DirOfHit_L = QuadrantStart - PositionInQuadrant;
		}
	   else
	   {
		   QuadrantStart = 0.52;
		   DirOfHit_L = QuadrantStart + PositionInQuadrant;
	   }
	}

	// Cause a damage indicator to appear

	DirOfHit_L = -1 * DirOfHit_L;
	FlashDamage(DirOfHit_L);
	//@todo: should we play this for same team hits when team damage is disabled?
	if (DamageIntensity > 0 && HitEffect != None)
	{
		DamageIntensity = FClamp(DamageIntensity, 0.2, 1.0);
		if (PawnOwner.Health <= 0)
		{
			// long effect duration if killed by this damage
			HitEffectFadeTime = PlayerOwner.MinRespawnDelay;
		}
		else
		{
			HitEffectFadeTime = default.HitEffectFadeTime * DamageIntensity;
		}
		HitEffectMaterialInstance.SetScalarParameterValue('HitAmount', MaxHitEffectIntensity * DamageIntensity);
		HitEffectMaterialInstance.SetVectorParameterValue('HitColor', MaxHitEffectColor);
		HitEffect.bShowInGame = true;
		bFadeOutHitEffect = true;
	}
}

/**
 * Configures a damage directional indicator and makes it appear
 *
 * @param	FlashPosition		Where it should appear
 */


function FlashDamage(float FlashPosition)
{
	local int i,MinIndex;
	local float Min;

	Min = 1.0;

	// Find an available slot

	for (i = 0; i < MaxNoOfIndicators; i++)
	{
		if (DamageData[i].FadeValue <= 0.0)
		{
			DamageData[i].FadeValue = 1.0;
			DamageData[i].FadeTime = FadeTime;
			DamageData[i].MatConstant.SetScalarParameterValue(PositionalParamName,FlashPosition);
			DamageData[i].MatConstant.SetScalarParameterValue(FadeParamName,1.0);
			return;
		}
		else if (DamageData[i].FadeValue < Min)
		{
			MinIndex = i;
			Min = DamageData[i].FadeValue;
		}
	}

	// Set the data

	DamageData[MinIndex].FadeValue = 1.0;
	DamageData[MinIndex].FadeTime = FadeTime;
	DamageData[MinIndex].MatConstant.SetScalarParameterValue(PositionalParamName,FlashPosition);
	DamageData[MinIndex].MatConstant.SetScalarParameterValue(FadeParamName,1.0);
}


function DisplayDamage()
{
	local int i;
	local float HitAmount;
	local LinearColor HitColor;

	// Update the fading on the directional indicators.

	for (i=0; i<MaxNoOfIndicators; i++)
	{
		if (DamageData[i].FadeTime > 0)
		{
			DamageData[i].FadeValue += ( 0 - DamageData[i].FadeValue) * (RenderDelta / DamageData[i].FadeTime);
			DamageData[i].FadeTime -= RenderDelta;
			DamageData[i].MatConstant.SetScalarParameterValue(FadeParamName,DamageData[i].FadeValue);

			Canvas.SetPos( ((Canvas.ClipX * 0.5) - (DamageIndicatorSize * 0.5 * ResolutionScale)),
						 	((Canvas.ClipY * 0.5) - (DamageIndicatorSize * 0.5 * ResolutionScale)));

			Canvas.DrawMaterialTile( DamageData[i].MatConstant, DamageIndicatorSize * ResolutionScale, DamageIndicatorSize * ResolutionScale, 0.0, 0.0, 1.0, 1.0);
		}
	}

	// Update the color/fading on the full screen distortion
	if (bFadeOutHitEffect)
	{
		HitEffectMaterialInstance.GetScalarParameterValue('HitAmount', HitAmount);
		HitAmount -= MaxHitEffectIntensity * RenderDelta / HitEffectFadeTime;
		if (HitAmount <= 0.0)
		{
			HitEffect.bShowInGame = false;
			bFadeOutHitEffect = false;
		}
		else
		{
			HitEffectMaterialInstance.SetScalarParameterValue('HitAmount', HitAmount);
			// now scale the color
			HitEffectMaterialInstance.GetVectorParameterValue('HitColor', HitColor);
			HitEffectMaterialInstance.SetVectorParameterValue('HitColor', HitColor - MaxHitEffectColor * (RenderDelta / HitEffectFadeTime));
		}
	}


}

/************************************************************************************************************
 Actor Render - These functions allow for actors in the world to gain access to the hud and render
 information on it.
************************************************************************************************************/


/** RemovePostRenderedActor()
remove an actor from the PostRenderedActors array
*/
function RemovePostRenderedActor(Actor A)
{
	local int i;

	for ( i=0; i<PostRenderedActors.Length; i++ )
	{
		if ( PostRenderedActors[i] == A )
		{
			PostRenderedActors[i] = None;
			return;
		}
	}
}

/** AddPostRenderedActor()
add an actor to the PostRenderedActors array
*/
function AddPostRenderedActor(Actor A)
{
	local int i;

	// make sure that A is not already in list
	for ( i=0; i<PostRenderedActors.Length; i++ )
	{
		if ( PostRenderedActors[i] == A )
		{
			return;
		}
	}

	// add A at first empty slot
	for ( i=0; i<PostRenderedActors.Length; i++ )
	{
		if ( PostRenderedActors[i] == None )
		{
			PostRenderedActors[i] = A;
			return;
		}
	}

	// no empty slot found, so grow array
	PostRenderedActors[PostRenderedActors.Length] = A;
}

/************************************************************************************************************
************************************************************************************************************/


static simulated function DrawBackground(float X, float Y, float Width, float Height, LinearColor DrawColor, Canvas DrawCanvas)
{
	DrawCanvas.SetPos(X,Y);
	DrawCanvas.DrawColorizedTile(Default.AltHudTexture, Width, Height, 631,202,98,48, DrawColor);
}

/**
  * Draw a beacon healthbar
  * @PARAM Width is the actual health width
  * @PARAM MaxWidth corresponds to the max health
  */
static simulated function DrawHealth(float X, float Y, float Width, float MaxWidth, float Height, Canvas DrawCanvas, optional byte Alpha=255)
{
	local float HealthX;
	local color DrawColor, BackColor;

	// Bar color depends on health
	DrawColor = Default.WhiteColor;
	HealthX = Width/MaxWidth;

	DrawColor.B = 0;
	if ( HealthX > 0.8 )
	{
		DrawColor.R = 0;
	}
	else if (HealthX < 0.4 )
	{
		DrawColor.G = 128;
	}

	DrawColor.A = Alpha;
	BackColor = default.WhiteCOlor;
	BackColor.A = Alpha;
	DrawBarGraph(X,Y,Width,MaxWidth,Height,DrawCanvas,DrawColor,BackColor);

}
static simulated function DrawBarGraph(float X, float Y, float Width, float MaxWidth, float Height, Canvas DrawCanvas, Color BarColor, Color BackColor)
{
	// Draw health bar backdrop ticks
	if ( MaxWidth > 24.0 )
	{
		// determine size of health bar caps
		DrawCanvas.DrawColor = BackColor;
		DrawCanvas.SetPos(X,Y);
		DrawCanvas.DrawTile(default.AltHudTexture,MaxWidth,Height,407,479,FMin(MaxWidth,118),16);
	}

	DrawCanvas.DrawColor = BarColor;
	DrawCanvas.SetPos(X, Y);
	DrawCanvas.DrawTile(default.AltHudTexture,Width,Height,277,494,4,13);
}

simulated event Timer()
{
	Super.Timer();
	if ( WorldInfo.GRI != None )
	{
		WorldInfo.GRI.SortPRIArray();
	}
}

/**
 * Creates a string from the time
 */
static function string FormatTime(int Seconds)
{
	local int Hours, Mins;
	local string NewTimeString;

	Hours = Seconds / 3600;
	Seconds -= Hours * 3600;
	Mins = Seconds / 60;
	Seconds -= Mins * 60;
	NewTimeString = "" $ ( Hours > 9 ? String(Hours) : "0"$String(Hours)) $ ":";
	NewTimeString = NewTimeString $ ( Mins > 9 ? String(Mins) : "0"$String(Mins)) $ ":";
	NewTimeString = NewTimeString $ ( Seconds > 9 ? String(Seconds) : "0"$String(Seconds));

	return NewTimeString;
}

static function Font GetFontSizeIndex(int FontSize)
{
	return default.HudFonts[Clamp(FontSize,0,3)];
}

/**
 * Given a PRI, show the Character portrait on the screen.
 *
 * @Param ShowPRI					The PRI to show
 * @Param bOverrideCurrentSpeaker	If true, we will quickly slide off the current speaker and then bring on this guy
 */
simulated function ShowPortrait(UTPlayerReplicationInfo ShowPRI, optional bool bOverrideCurrentSpeaker)
{
	if ( ShowPRI != none && ShowPRI.CharPortrait != none )
	{
		// See if there is a current speaker

		if ( CharPRI != none )  // See if we should override this speaker
		{
			if ( bOverrideCurrentSpeaker )
			{
				CharPendingPRI = ShowPRI;
				HidePortrait();
    		}
			return;
		}


		// Noone is sliding in, set us up.

		// Make sure we have the Instance

		if ( CharPortraitMI == none )
		{
			CharPortraitMI = new(Outer) class'MaterialInstanceConstant';
			CharPortraitMI.SetParent(CharPortraitMaterial);
		}

		// Set the image

		CharPortraitMI.SetTextureParameterValue('PortraitTexture',ShowPRI.CharPortrait);
		CharPRI = ShowPRI;
		CharPortraitTime = 0.0;
	}
}

/** If the portrait is visible, this will immediately try and hide it */
simulated function HidePortrait()
{
	local float CurrentPos;

	// Figure out the slide.

	CurrentPos = CharPortraitTime / CharPortraitSlideTime;

	// Slide it back out the equal percentage

	if (CurrentPos < CharPortraitSlideTransitionTime)
	{
		CharPortraitTime = CharPortraitSlideTime * (1.0 - CurrentPos);
	}

	// If we aren't sliding out, do it now

	else if ( CurrentPos < (1.0 - CharPortraitSlideTransitionTime ) )
	{
		CharPortraitTime = CharPortraitSlideTime * (1.0 - CharPortraitSlideTransitionTime);
	}
}

/**
 * Render the character portrait on the screen.
 *
 * @Param	RenderDelta		How long since the last render
 */
simulated function DisplayPortrait(float DeltaTime)
{
	local float CurrentPos, LocalPos, XPos, Ypos;
	local float w,h;

	H = CharPortraitSize.Y * (Canvas.ClipY/768.0);
	W = CharPortraitSize.Y * (CharPortraitSize.X / CharPortraitSize.Y);

	CharPortraitTime += DeltaTime * (CharPendingPRI != none ? 1.5 : 1.0);

	CurrentPos = CharPortraitTime / CharPortraitSlideTime;
	// Figure out what we are doing

	if (CurrentPos < CharPortraitSlideTransitionTime)	// Siding In
	{
		LocalPos = CurrentPos / CharPortraitSlideTransitionTime;
		XPos = FCubicInterp((W * -1), 0.0, (Canvas.ClipX * CharPortraitXPerc), 0.0, LocalPos);
	}
	else if ( CurrentPos < 1.0 - CharPortraitSlideTransitionTime )	// Sitting there
	{
		XPos = Canvas.ClipX * CharPortraitXPerc;
	}
	else if ( CurrentPos < 1.0 )	// Siding out
	{
		LocalPos = (CurrentPos - (1.0 - CharPortraitSlideTransitionTime)) / CharPortraitSlideTransitionTime;
		XPos = FCubicInterp((W * -1), 0.0, (Canvas.ClipX * CharPortraitXPerc), 0.0, 1.0-LocalPos);
	}
	else	// Done, reset everything
	{
		CharPRI = none;
		if ( CharPendingPRI != none )	// If we have a pending PRI, then display it
		{
			ShowPortrait(CharPendingPRI);
			CharPendingPRI = none;
		}
		return;
	}

	// Draw the portrait
	YPos = Canvas.ClipY * CharPortraitYPerc;
	Canvas.SetPos(XPos, YPos);
	Canvas.DrawColor = Whitecolor;
	/* FIXME: Currently Drawing the UTexture on the hud is broken.  Epic Please Fix :) */
	Canvas.DrawTile(HudTexture,W,H,0,0,255,255);

	Canvas.SetPos(XPos,YPos + H + 5);
	Canvas.Font = HudFonts[0];
	Canvas.DrawText(CharPRI.PlayerName);
}

exec function TestPRI(bool b)
{
	local int i;

	i = Rand(Worldinfo.GRI.PRIArray.Length);
	ShowPortrait(UTPlayerReplicationInfo(Worldinfo.GRI.PRIArray[i]),b);
}

/**
 * Displays the MOTD Scene
 */
function DisplayMOTD()
{
	OpenScene(MOTDSceneTemplate);
}

/**
 * Displays a HUD message
 */
function DisplayHUDMessage(string Message)
{
	local float XL,YL;
	local float BarHeight, Height, YBuffer, XBuffer, YCenter;

	if (!bHudMessageRendered)
	{

		// Preset the Canvas

		Canvas.SetDrawColor(255,255,255,255);
		Canvas.Font = GetFontSizeIndex(2);
		Canvas.StrLen(Message,XL,YL);

		// Figure out sizes/positions

		BarHeight = YL * 1.1;
		YBuffer = Canvas.ClipY * 0.05;
		XBuffer = Canvas.ClipX * 0.05;
		Height = YL * 2.0;

		YCenter = Canvas.ClipY - YBuffer - (Height * 0.5);

		// Draw the Bar

		Canvas.SetPos(0,YCenter - (BarHeight * 0.5) );
		Canvas.DrawTile(AltHudTexture, Canvas.ClipX, BarHeight, 382, 441, 127, 16);

		// Draw the Symbol

		Canvas.SetPos(XBuffer, YCenter - (Height * 0.5));
		Canvas.DrawTile(AltHudTexture, Height * 1.33333, Height, 734,190, 82, 70);

		// Draw the Text

		Canvas.SetPos(XBuffer + Height * 1.5, YCenter - (YL * 0.5));
		Canvas.DrawText(Message);

		bHudMessageRendered = true;
	}
}

function DisplayClock()
{
	local string Time;
	local vector2D POS;

	POS = ResolveHudPosition(ClockPosition,183,44);
	Time = FormatTime(UTGRI.TimeLimit != 0 ? UTGRI.RemainingTime : UTGRI.ElapsedTime);

	Canvas.SetPos(POS.X, POS.Y);
	Canvas.DrawColorizedTile(AltHudTexture, 183 * ResolutionScale,44 * ResolutionScale,489,395,183,44,TeamHudColor);

	Canvas.DrawColor = WhiteColor;
	DrawGlowText(Time, POS.X + (28 * ResolutionScale), POS.Y, 39 * ResolutionScale);
}

function DisplayPawnDoll()
{
	local vector2d POS;
	local string Amount;
	local int Health;
	local float xl,yl,PulseScale;
	local float ArmorAmount;

	POS = ResolveHudPosition(DollPosition,216, 115);

	Canvas.DrawColor = WhiteColor;

	// First, handle the Pawn Doll

		// The Background

		Canvas.SetPos(POS.X,POS.Y);
		Canvas.DrawColorizedTile(AltHudTexture,73 * ResolutionScale, 115 * ResolutionScale, 0, 54, 73, 115, TeamHudColor);

		// The ShieldBelt/Default Doll

		Canvas.SetPos(POS.X + (47 * ResolutionScale), POS.Y + (58 * ResolutionScale));
		if ( UTPawnOwner.ShieldBeltArmor > 0.0f )
		{
			ArmorAmount += UTPawnOwner.ShieldBeltArmor;
			DrawTileCentered(AltHudTexture, 60 * ResolutionScale, 93 * ResolutionScale, 71, 224, 56, 109,LC_White);
		}
		else
		{
			DrawTileCentered(AltHudTexture, 60 * ResolutionScale, 93 * ResolutionScale, 4, 224, 56, 109,TeamHudColor);
		}

		if ( UTPawnOwner.VestArmor > 0.0f )
		{
			Canvas.SetPos(POS.X + (47 * ResolutionScale), POS.Y+(35 * ResolutionScale));
			DrawTileCentered(AltHudTexture, 43 * ResolutionScale, 20 * ResolutionScale, 132, 223, 45, 24,LC_White);
			ArmorAmount += UTPawnOwner.VestArmor;
		}

		if (UTPawnOwner.ThighpadArmor > 0.0f )
		{
			Canvas.SetPos(POS.X + (47 * ResolutionScale), POS.Y+(60 * ResolutionScale));
			DrawTileCentered(AltHudTexture, 34 * ResolutionScale, 30 * ResolutionScale, 132, 247, 46, 50,LC_White);
			ArmorAmount += UTPawnOwner.ThighpadArmor;
		}

		if (UTPawnOwner.HelmetArmor > 0.0f )
		{
			Canvas.SetPos(POS.X + (47 * ResolutionScale), POS.Y+(20 * ResolutionScale));
			DrawTileCentered(AltHudTexture, 23 * ResolutionScale, 23 * ResolutionScale, 202, 261, 21, 21,LC_White);
			ArmorAmount += UTPawnOwner.HelmetArmor;
		}

		if (UTPawnOwner.JumpBootCharge > 0 )
		{
			Canvas.SetPos(POS.X + (49 * ResolutionScale), POS.Y+(88 * ResolutionScale));
			DrawTileCentered(AltHudTexture, 59 * ResolutionScale, 31 * ResolutionScale, 223, 261, 41, 22,LC_White);

			Amount = ""$UTPawnOwner.JumpBootCharge;
			Canvas.Font = GlowFonts[0];
			Canvas.Strlen(Amount,XL,YL);
			DrawGlowText(Amount,POS.X + (60 * ResolutionScale) - (XL * 0.5), POS.Y+(80 * ResolutionScale) ,18 * ResolutionScale);
		}

	// Next, the health and Armor widgets

    	// Draw the Health Background

		Canvas.SetPos(POS.X + (73 *  ResolutionScale),POS.Y + ((115 - 65) * ResolutionScale));
		Canvas.DrawColorizedTile(AltHudTexture, 143 * ResolutionScale, 53 * ResolutionScale, 74, 100, 142, 54, TeamHudColor);
		Canvas.DrawColor = WhiteColor;

		// Draw the Health Text

			// Figure out if we should be pulsing

			Health = UTPawnOwner.Health;

			if ( Health > LastHealth )
			{
				HealthPulseTime = PulseDuration;
			}

			LastHealth = Health;

			if (HealthPulsetime > 0)
			{
				CalculatePulse(PulseScale, HealthPulseTime);
			}
			else
			{
				PulseScale = 1.0;
			}


		Amount = (Health > 0) ? ""$Health : "0";
		DrawGlowText(Amount, POS.X + (190 * ResolutionScale), POS.Y + (40 * ResolutionScale), 60 * ResolutionScale, PulseScale,true);

		// Draw the Health Icon

		Canvas.SetPos(POS.X + (90 * ResolutionScale), POS.Y + (70 * ResolutionScale));
		DrawTileCentered(AltHudTexture, (42 * ResolutionScale * PulseScale) , (30 * ResolutionScale * PulseScale), 216, 102, 56, 40, LC_White);

		// Only Draw the Arrmor if there is any
		// TODO - Add fading
		if ( ArmorAmount > 0 )
		{

		if (ArmorAmount > LastArmorAmount)
			{
				ArmorPulseTime = PulseDuration;
			}

			LastArmorAmount = ArmorAmount;

			if (ArmorPulsetime > 0)
			{
				CalculatePulse(PulseScale, ArmorPulseTime);
			}
			else
			{
				PulseScale = 1.0;
			}

	    	// Draw the Armor Background

			Canvas.SetPos(POS.X + (73 *  ResolutionScale),POS.Y + ((115 - 118) * ResolutionScale));
			Canvas.DrawColorizedTile(AltHudTexture, 143 * ResolutionScale, 53 * ResolutionScale, 74, 54, 142, 46, TeamHudColor);
			Canvas.DrawColor = WhiteColor;


			// Draw the Armor Text

			DrawGlowText(""$INT(ArmorAmount), POS.X + (173 * ResolutionScale), POS.Y + (7 * ResolutionScale), 45 * ResolutionScale,PulseScale,true);

			// Draw the Armor Icon

			Canvas.SetPos(POS.X + (95 * ResolutionScale), POS.Y + (30 * ResolutionScale));
			DrawTileCentered(AltHudTexture, (33 * ResolutionScale) , (24 * ResolutionScale), 225, 68, 42, 32, LC_White);

		}
}

function DisplayAmmo(UTWeapon Weapon)
{
	local vector2d POS;
	local string Amount;
	local AmmoWidgetDisplayStyle DisplayStyle;
	local float BarWidth;
	local float PercValue;
	local int AmmoCount;
	local float PulseScale;

	// Figure out if we should be pulsing

	AmmoCount = Weapon.GetAmmoCount();


	if ( LastWeapon == Weapon && AmmoCount > LastAmmoCount )
	{
		AmmoPulseTime = PulseDuration;
	}

	LastWeapon = Weapon;
	LastAmmoCount = AmmoCount;

	if (AmmoPulsetime > 0)
	{
		CalculatePulse(PulseScale, AmmoPulseTime);
	}
	else
	{
		PulseScale = 1.0;
	}

	PercValue = Weapon.GetPowerPerc();
	DisplayStyle = Weapon.AmmoDisplayType;

	// Resolve the position

	POS = ResolveHudPosition(AmmoPosition,128,71);

	if ( DisplayStyle != EAWDS_BarGraph )
	{

		// Draw the background

		Canvas.SetPos(POS.X,POS.Y);
		Canvas.DrawColorizedTile(AltHudTexture, 128 * ResolutionScale, 71 * ResolutionScale, 135, 297, 128, 71, TeamHudColor);

		// Draw the amount
		Amount = ""$Weapon.GetAmmoCount();
		Canvas.DrawColor = WhiteColor;

		DrawGlowText(Amount, POS.X + (113 * ResolutionScale), POS.Y + (4 * ResolutionScale), 58 * ResolutionScale, PulseScale,true);
	}


	// If we have a bar graph display, do it here

	if ( DisplayStyle != EAWDS_Numeric )
	{
		Canvas.SetPos(POS.X + (47 * ResolutionScale), POS.Y + (-8 * ResolutionScale));
		Canvas.DrawColorizedTile(AltHudTexture, 76 * ResolutionScale, 18 * ResolutionScale, 376,458, 88, 14, LC_White);

		BarWidth = 70 * ResolutionScale;
		DrawHealth(POS.X + (50 * ResolutionScale), POS.Y + (-4 * ResolutionScale), BarWidth * PercValue,  BarWidth, 16, Canvas);
	}
}

function DisplayPowerups()
{
	local vector2d POS;
	local int i,slotindex;
	local UTTimedPowerup TP;
	local float w,h,a;
	local linearColor LC;
	local float PulseScale;
	local byte SlotsFound[4];

	POS.X = Canvas.ClipX - (PowerupDims.X * ResolutionScale) * 0.5;
	POS.Y = (Canvas.ClipY * 0.85);

	foreach UTPawnOwner.InvManager.InventoryActors(class'UTTimedPowerup', TP)
	{
		if ( TP.TimeRemaining > 0.0 )
		{
			SlotIndex = -1;
			for (i=0;i<4;i++)
			{
				if ( PowerupSlots[i].Powerup != none )
				{

					// We are the current powerup
					if ( PowerupSlots[i].Powerup == TP )
					{
						SlotIndex = i;
					}

					// Check to see if we are a new copy of the powerup
					else if ( PowerupSlots[i].Powerup.Class == TP.Class )
					{
						// If this one is in transitioning out, reset it
						if (PowerupSlots[i].TransitionTime > 0 && !PowerupSlots[i].bTransitionIn)
						{
							PowerupSlots[i].bTransitionIn = true;
							PowerupSlots[i].TransitionTime = PowerupTransitionTime - PowerupSlots[i].TransitionTime;
						}
						SlotIndex = i;
						break;
					}
				}
			}

			// Add new powerups to a slot
			if (SlotIndex < 0)
			{
				for (i=0;i<4;i++)
				{
					if (PowerupSlots[i].PowerUp == none)
					{
						PowerupSlots[i].bTransitionIn = true;
						PowerupSlots[i].TransitionTime = PowerupTransitionTime;
						SlotIndex = i;
						break;
					}
				}
			}

			PowerupSlots[SlotIndex].Powerup = TP;
			SlotsFound[SlotIndex] = 1;
		}
	}

	// set any powerups that are no longer owned by this pawn to fade out immediately
	for (SlotIndex = 0; SlotIndex < ArrayCount(SlotsFound); SlotIndex++)
	{
		if (PowerupSlots[SlotIndex].Powerup != None && SlotsFound[SlotIndex] == 0 && PowerupSlots[SlotIndex].TransitionTime == 0.0)
		{
			PowerupSlots[SlotIndex].TransitionTime = PowerupTransitionTime;
			PowerupSlots[SlotIndex].bTransitionIn = false;
		}
	}

	// Draw the powerups
	for (SlotIndex = 0; SlotIndex < 4; SlotIndex++)
	{
		if ( PowerupSlots[SlotIndex].Powerup != None )
		{
			if ( PowerupSlots[SlotIndex].Powerup.TimeRemaining <= 0 && PowerupSlots[SlotIndex].TransitionTime == 0 )
			{
				PowerupSlots[SlotIndex].TransitionTime = PowerupTransitionTime;
				PowerupSlots[SlotIndex].bTransitionIn = false;
			}

			w = PowerupDims.X * ResolutionScale;
			h = PowerupDims.Y * ResolutionScale;
			a = 1.0;

			// If we are transitioning, adjust the scales

			if (PowerupSlots[SlotIndex].Powerup.TimeRemaining <= 5.0)
			{
				PulseScale = 0.5 + (0.5 * abs(cos(5-(PowerupSlots[SlotIndex].Powerup.TimeRemaining)*3)));
				W *= PulseScale;
				H *= PulseScale;
			}
			else
			{
				PulseScale = 1.0;
			}

			if ( PowerupSlots[SlotIndex].TransitionTime > 0.0f )
			{
				if ( PowerupSlots[SlotIndex].bTransitionIn )
				{
					w *= 1 + (PowerupSlots[SlotIndex].TransitionTime / PowerupTransitionTime * 16);
					h *= 1 + (PowerupSlots[SlotIndex].TransitionTime / PowerupTransitionTime * 16);
					a = 1.0 - (PowerupSlots[SlotIndex].TransitionTime / PowerupTransitionTime);
				}
				else
				{
					w *= (PowerupSlots[SlotIndex].TransitionTime / PowerupTransitionTime);
					h *= (PowerupSlots[SlotIndex].TransitionTime / PowerupTransitionTime);
					a = (PowerupSlots[SlotIndex].TransitionTime / PowerupTransitionTime);
				}

				PowerupSlots[SlotIndex].TransitionTime -= RenderDelta;
				if ( PowerupSlots[SlotIndex].TransitionTime <= 0.0f )
				{
					if (!PowerupSlots[SlotIndex].bTransitionIn)
					{
						PowerupSlots[SlotIndex].Powerup = none;
						break;
					}
					PowerupSlots[SlotIndex].TransitionTime = 0.0;
				}
			}

			// We have our scale/pos/etc.. let's play

			// Draw the Symbol first

			Canvas.SetPos(POS.X,POS.Y);
			LC = LC_White;
			LC.A = A;
			DrawTileCentered(HudTexture,W,H,3,168,67,75,LC);

			// Draw the Text
			Canvas.SetDrawColor(255,255,255,255*a);
			DrawGlowText(""$INT(PowerupSlots[SlotIndex].Powerup.TimeRemaining), POS.X - (W * 0.60) , (POS.Y - (16 * ResolutionScale)), (32 * ResolutionScale),,true);
			Pos.Y -= PowerupDims.Y * 1.1;

		}
	}
}

function DisplayScoring()
{
	local vector2d POS;

	POS = ResolveHudPosition(ScoringPosition, 115,44);

	if ( bShowFragCount )
	{
		DisplayFragCount(POS);
	}

	if ( bShowLeaderboard )
	{
		DisplayLeaderBoard(POS);
	}
}


function DisplayFragCount(vector2d POS)
{
	local int FragCount;
	local float PulseScale;
	Canvas.SetPos(POS.X, POS.Y);
	Canvas.DrawColorizedTile(AltHudTexture, 115 * ResolutionScale, 44 * ResolutionScale, 374, 395, 115, 44, TeamHudColor);
	Canvas.DrawColor = WhiteColor;

	// Draw the Health Text


		// Figure out if we should be pulsing

		FragCount = UTOwnerPRI.Score;

		if ( FragCount > LastFragCount )
		{
			FragPulseTime = PulseDuration;
		}

		LastFragCOunt = FragCount;

		if (FragPulsetime > 0)
		{
			CalculatePulse(PulseScale, FragPulseTime);
		}
		else
		{
			PulseScale = 1.0;
		}


	DrawGlowText(""$FragCount, POS.X + (87 * ResolutionScale), POS.Y + (-2 * ResolutionScale), 42 * ResolutionScale, PulseScale,true);
}

function DisplayLeaderBoard(vector2d POS)
{
	local string Work,MySpreadStr;
	local int i, MySpread, MyPosition;
	local float XL,YL;

	POS.X = Canvas.ClipX;
	POS.Y += 50 * ResolutionScale;

	// Figure out your Spread

	if ( UTGRI.PRIArray[0] == UTOwnerPRI )
	{
		MySpread = 0;
		if ( UTGRI.PRIArray.Length>1 )
		{
			MySpread = UTOwnerPRI.Score - UTGRI.PRIArray[1].Score;
		}
		MyPosition = 0;
	}
	else
	{
		MySpread = UTOwnerPRI.Score - UTGRI.PRIArray[0].Score;
		MyPosition = UTGRI.PRIArray.Find(UTOwnerPRI);
	}

	if (MySpread >0)
	{
		MySpreadStr = "+"$String(MySpread);
	}
	else
	{
		MySpreadStr = string(MySpread);
	}

	// Draw the Spread

	Work = string(MyPosition+1) $ PlaceMarks[min(MyPosition,3)] $ " / " $ MySpreadStr;

	Canvas.Font = GetFontSizeIndex(2);
	Canvas.SetDrawColor(255,255,255,255);

	Canvas.Strlen(Work,XL,YL);
	Canvas.SetPos(POS.X - XL, POS.Y);
	Canvas.DrawText(Work);

	POS.Y += YL * 1.2;

	// Draw the leaderboard

	Canvas.Font = GetFontSizeIndex(1);
	Canvas.SetDrawColor(200,200,200,255);
	for (i=0;i<3 && i < UTGRI.PRIArray.Length;i++)
	{
		if ( UTGRI.PRIArray[i] != none )
		{
			Work = string(i+1) $ PlaceMarks[i] $ ":" @ UTGRI.PRIArray[i].PlayerName;
			Canvas.StrLen(Work,XL,YL);
			Canvas.SetPos(POS.X-XL,POS.Y);
			Canvas.DrawText(Work);
			POS.Y += YL;
		}
	}

}

defaultproperties
{
	WeaponBarScale=0.75
	WeaponBarY=16
	SelectedWeaponScale=1.5
	BounceWeaponScale=2.25
	SelectedWeaponAlpha=1.0
	OffWeaponAlpha=0.5
	EmptyWeaponAlpha=0.25
	WeaponBoxWidth=100.0
	WeaponBoxHeight=64.0
	WeaponScaleSpeed=10.0
	WeaponBarXOffset=70
	WeaponXOffset=60
	SelectedBoxScale=1.0
	WeaponYScale=64
	WeaponYOffset=8

	WeaponAmmoLength=48
	WeaponAmmoThickness=16
	SelectedWeaponAmmoOffsetX=110
	WeaponAmmoOffsetX=100
	WeaponAmmoOffsetY=16

	HudTexture=Texture2D'T_UTHudGraphics.Textures.UTOldHUD'
	AltHudTexture=Texture2D'UI_HUD.HUD.UI_HUD_BaseA'

	ScoreboardSceneTemplate=Scoreboard_DM'UI_Scenes_Scoreboards.sbDM'
   	MusicManagerClass=class'UTGame.UTMusicManager'

	HudFonts(0)=MultiFont'UI_Fonts.MultiFonts.MF_HudSmall'
	HudFonts(1)=MultiFont'UI_Fonts.MultiFonts.MF_HudMedium'
	HudFonts(2)=MultiFont'UI_Fonts.MultiFonts.MF_HudLarge'
	HudFonts(3)=MultiFont'UI_Fonts.MultiFonts.MF_HudHuge'

	CharPortraitMaterial=Material'UI_HUD.Materials.CharPortrait'
	CharPortraitYPerc=0.2
	CharPortraitXPerc=0.05
	CharPortraitSlideTime=2.0
	CharPortraitSlideTransitionTime=0.175
	CharPortraitSize=(X=128,Y=160)

	CurrentWeaponScale(0)=1.0
	CurrentWeaponScale(1)=1.0
	CurrentWeaponScale(2)=1.0
	CurrentWeaponScale(3)=1.0
	CurrentWeaponScale(4)=1.0
	CurrentWeaponScale(5)=1.0
	CurrentWeaponScale(6)=1.0
	CurrentWeaponScale(7)=1.0
	CurrentWeaponScale(8)=1.0
	CurrentWeaponScale(9)=1.0

    MOTDSceneTemplate=UTUIScene_MOTD'UI_Scenes_HUD.Menus.MOTDMenu'

	MessageOffset(0)=0.15
	MessageOffset(1)=0.242
	MessageOffset(2)=0.36
	MessageOffset(3)=0.58
	MessageOffset(4)=0.83
	MessageOffset(5)=0.9
	MessageOffset(6)=2.0

	BlackColor=(R=0,G=0,B=0,A=255)
	GoldColor=(R=255,G=255,B=0,A=255)

	MapBackground=Texture2D'UI_HUD.HUD.UI_HUD_MinimapB'

	GlowFonts(0)=font'UI_Fonts.Fonts.UI_Fonts_Camo42Glow'
	GlowFonts(1)=font'UI_Fonts.Fonts.UI_Fonts_Camo42'

  	LC_White=(R=1.0,G=1.0,B=1.0,A=1.0)

	PulseDuration=0.33
	PulseSplit=0.25
	PulseMultiplier=0.5

	PowerupTransitionTime=0.5

	MaxNoOfIndicators=3
	BaseMaterial=Material'UI_HUD.HUD.M_UI_HUD_DamageDir'
	FadeTime=0.5
	PositionalParamName=DamageDirectionRotation
	FadeParamName=DamageDirectionAlpha

	HitEffectFadeTime=0.50
	MaxHitEffectIntensity=0.25
	MaxHitEffectColor=(R=2.0,G=-1.0,B=-1.0)
}
