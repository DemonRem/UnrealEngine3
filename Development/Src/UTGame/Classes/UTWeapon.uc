/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeapon extends GameWeapon
	native
	nativereplication
	dependson(UTPlayerController)
	config(Weapon)
	abstract;

cpptext
{
	INT* GetOptimizedRepList( BYTE* InDefault, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel );
}

/** if set, when this class is compiled, a menu entry for it will be automatically added/updated in its package.ini file
 * (abstract classes are skipped even if this flag is set)
 */
var bool bExportMenuData;

/*********************************************************************************************
 Ammo / Pickups / Inventory
********************************************************************************************* */

var class<UTAmmoPickupFactory> AmmoPickupClass;

/** Current ammo count */
var databinding	repnotify int AmmoCount;

/** Initial ammo count if in weapon locker */
var databinding	int LockerAmmoCount;

/** Max ammo count */
var databinding	int MaxAmmoCount;

/** Holds the amount of ammo used for a given shot */
var databinding	array<int> ShotCost;

/** Holds the min. amount of reload time that has to pass before you can switch */
var array<float> MinReloadPct;

/** View shake applied when a shot is fired
 * @FIXME: remove if keep camera anim system for firing viewshake
 */
var array<UTPlayerController.ViewShakeInfo> FireShake;

/** camera anim to play when firing (for camera shakes) */
var array<CameraAnim> FireCameraAnim;

var array<name> EffectSockets;

var int IconX, IconY, IconWidth, IconHeight;

/** used when aborting a weapon switch (WeaponAbortEquip and WeaponAbortPutDown) */
var float SwitchAbortTime;

var UIRoot.TextureCoordinates IconCoordinates;
var UIRoot.TextureCoordinates CrossHairCoordinates;

/** Holds the image to use for the crosshair. */
var Surface CrosshairImage;

var MaterialInstanceConstant CrosshairMat;

/** color to use when drawing the crosshair */
var config color CrosshairColor;

/** If true, this crosshair will be scaled against 1024x768 */
var bool bScaleCrosshair;

/** If true, use smaller 1st person weapons */
var globalconfig databinding bool bSmallWeapons;

/*********************************************************************************************
 Zooming
********************************************************************************************* */

/** If set to non-zero, this fire mode will zoom the weapon. */
var array<byte>	bZoomedFireMode;

/** Are we zoomed */
enum EZoomState
{
	ZST_NotZoomed,
	ZST_ZoomingOut,
	ZST_ZoomingIn,
	ZST_Zoomed,
};

/** Holds the fire mode num of the zoom */
var byte 	ZoomedFireModeNum;

var float	ZoomedTargetFOV;

var float	ZoomedRate;

var float 	ZoomFadeTime;
var float	FadeTime;

/** Sounds to play when zooming begins/ends/etc. */
var SoundCue ZoomInSound, ZoomOutSound;


/*********************************************************************************************
 Attachments
********************************************************************************************* */

/** The class of the attachment to spawn */
var class<UTWeaponAttachment> 	AttachmentClass;

/** If true, this weapon is a super weapon.  Super Weapons have longer respawn times a different
    pickup base and never respect weaponstay */
var bool bSuperWeapon;

/*********************************************************************************************
 Overlays
********************************************************************************************* */

/** mesh for overlay - Each weapon will need to add its own overlay mesh in its default props */
var protected MeshComponent OverlayMesh;
/*********************************************************************************************
 Inventory Grouping/etc.
********************************************************************************************* */

/** The weapon/inventory set, 0-9. */
var databinding	byte InventoryGroup;

/** position within inventory group. (used by prevweapon and nextweapon) */
var float GroupWeight;

/** The final inventory weight.  It's calculated in PostBeginPlay() */
var float InventoryWeight;

/** If true, this will will never accept a forwarded pending fire */
var bool bNeverForwardPendingFire;


/** The index in to the Quick Pick radial menu to use */
var int QuickPickGroup;

/** Used to sort it within a group */
var float QuickPickWeight;

/*********************************************************************************************
 Animations and Sounds
********************************************************************************************* */

var bool bSuppressSounds;

/** Animation to play when the weapon is fired */
var(Animations)	array<name>	WeaponFireAnim;
var(Animations) array<name> ArmFireAnim;
var(Animations) AnimSet ArmsAnimSet;
/** Animation to play when the weapon is Put Down */
var(Animations) name	WeaponPutDownAnim;
var(Animations) name	ArmsPutDownAnim;
/** Animation to play when the weapon is Equipped */
var(Animations) name	WeaponEquipAnim;
var(Animations) name	ArmsEquipAnim;

var(Animations) array<name> WeaponIdleAnims;
var(Animations) array<name> ArmIdleAnims;

var(Animations) bool bUsesOffhand;
/** Sound to play when the weapon is fired */
var(Sounds)	array<SoundCue>	WeaponFireSnd;

/** Sound to play when the weapon is Put Down */
var(Sounds) SoundCue 	WeaponPutDownSnd;

/** Sound to play when the weapon is Equipped */
var(Sounds) SoundCue 	WeaponEquipSnd;

/*********************************************************************************************
 First person weapon view positioning and rendering
********************************************************************************************* */

/** How much to damp view bob */
var() float	BobDamping;

/** How much to damp jump and land bob */
var() float	JumpDamping;

/** Limit for pitch lead */
var		float MaxPitchLag;

/** Limit for yaw lead */
var		float MaxYawLag;

/** Last Rotation update time for this weapon */
var		float LastRotUpdate;

/** Last Rotation update for this weapon */
var		Rotator LastRotation;

/** How far weapon was leading last tick */
var float OldLeadMag[2];

/** rotation magnitude last tick */
var int OldRotDiff[2];

/** max lead amount last tick */
var float OldMaxDiff[2];

/** Scaling faster for leading speed */
var float RotChgSpeed;

/** Scaling faster for returning speed */
var float ReturnChgSpeed;


/*********************************************************************************************
 Misc
********************************************************************************************* */

/** The Color used when drawing the Weapon's Name on the Hud */
var databinding	color WeaponColor;

/** Percent (from right edge) of screen space taken by weapon on x axis. */
var float WeaponCanvasXPct;

/** Percent (from bottom edge) of screen space taken by weapon on y axis. */
var float WeaponCanvasYPct;

/*********************************************************************************************
 Muzzle Flash
********************************************************************************************* */

/** Holds the name of the socket to attach a muzzle flash too */
var name					MuzzleFlashSocket;

/** Muzzle flash PSC and Templates*/
var ParticleSystemComponent	MuzzleFlashPSC;

/** Normal Fire and Alt Fire Templates */
var ParticleSystem			MuzzleFlashPSCTemplate, MuzzleFlashAltPSCTemplate;

/** UTWeapon looks to set the color via a color parameter in the emitter */
var color					MuzzleFlashColor;

/** Set this to true if you want the flash to loop (for a rapid fire weapon like a minigun) */
var bool					bMuzzleFlashPSCLoops;

/** dynamic light */
var	UTExplosionLight		MuzzleFlashLight;

/** dynamic light class */
var class<UTExplosionLight> MuzzleFlashLightClass;

/** How long the Muzzle Flash should be there */
var() float					MuzzleFlashDuration;


/** Offset from view center */
var(FirstPerson) vector	PlayerViewOffset;

/*********************************************************************************************
 Weapon locker
********************************************************************************************* */
var rotator LockerRotation;
var vector LockerOffset;

/*********************************************************************************************
 * AI Hints
 ********************************************************************************************* */

var		bool	bSplashJump;
var		bool	bSplashDamage;
var		bool	bRecommendSplashDamage;
var 	bool	bSniping;
var		bool	bCanDestroyBarricades;
var		float	CurrentRating;		// most recently calculated rating

/** How much error to add to each shot */
var float AimError;

/*********************************************************************************************
 * WeaponStatsIndex
 ********************************************************************************************* */

var		int		OwnerStatsID;
var 	int		WeaponStatsID;

/*********************************************************************************************
 * Weapon Lock On
 ********************************************************************************************* */

enum	EAutoLock
{
	WEAPLOCK_None,
	WEAPLOCK_Default,
	WEAPLOCK_Constant,
};

/*********************************************************************************************
 * Weapon lock on support
 ********************************************************************************************* */

/** When true, thos weapon supports Locking on to a target */
var(Locking) EAutoLock	WeaponLockType;

/** What kinds of pawns this weapon can lock onto */
var	class<Pawn> LockablePawnClass;

/** The frequency with which we will check for a lock */
var(Locking) float		LockCheckTime;

/** How far out should we be considering actors for a lock */
var(Locking) int		LockRange;

/** How long does the player need to target an actor to lock on to it*/
var(Locking) float		LockAcquireTime;

/** Once locked, how long can the player go without painting the object before they lose the lock */
var(Locking) float		LockTolerance;

/** When true, this weapon is locked on target */
var bool 				bLockedOnTarget;

/** What "target" is this weapon locked on to */
var Actor 				LockedTarget;

/** What "target" is current pending to be locked on to */
var Actor				PendingLockedTarget;

/** How long since the Lock Target has been valid */
var float  				LastLockedOnTime;

/** When did the pending Target become valid */
var float				PendingLockedTargetTime;

/** When was the last time we had a valid target */
var float				LastValidTargetTime;

/** angle for locking for lock targets */
var float 				LockAim;

/** angle for locking for lock targets when on Console */
var float 				ConsoleLockAim;

/** Sound Effects to play when Locking */
var SoundCue 			LockAcquiredSound;
var SoundCue			LockLostSound;

/** if true, pressing fire activates firemode 1 and altfire activates firemode 0 (only affects human players) */
var config bool bSwapFireModes;

var bool bDebugWeapon;

/** Distance from target collision box to accept near miss when aiming help is enabled */
var float AimingHelpRadius[2];

/** Set for ProcessInstantHit based on whether aiminghelp was used for this shot */
var bool bUsingAimingHelp;

/** whether to allow this weapon to fire by uncontrolled pawns */
var bool bAllowFiringWithoutController;

enum AmmoWidgetDisplayStyle
{
	EAWDS_Numeric,
	EAWDS_BarGraph,
	EAWDS_Both
};

var AmmoWidgetDisplayStyle AmmoDisplayType;

/** TEMP - Allow static mesh weapons and weapons without bringup/putdown anims to pitch in */

var float LastSPTime;
var float PitchOffset;

/** @FIXME: TEMP HACKFIX FOR OVERLAY FRAME-BEHIND BUG */
var bool bUseOverlayHack;


//// start of adhesion friction vars

// all this needs to move to the GameWeapon

/** When the weapon is zoomed in then the turn speed is reduced by this much **/
var() config float ZoomedTurnSpeedScalePct;

/** Target friction enabled? */
var() config bool bTargetFrictionEnabled;

/** Min distance for friction */
var() config float TargetFrictionDistanceMin;

/** Peak distance for friction */
var() config float TargetFrictionDistancePeak;

/** Max distance allow for friction */
var() config float TargetFrictionDistanceMax;

/** Interp curve that allows for piece wise functions for the TargetFrictionDistance amount at different ranges **/
var() config InterpCurveFloat TargetFrictionDistanceCurve;

/** Min/Max friction multiplier applied when target acquired */
var() config Vector2d TargetFrictionMultiplierRange;

/** Amount of additional radius/height given to target cylinder when at peak distance */
var() config float TargetFrictionPeakRadiusScale;
var() config float TargetFrictionPeakHeightScale;

/** Offset to apply to friction target location (aim for the chest, etc) */
var() config vector TargetFrictionOffset;

/** Boost the Target Friction by this much when zoomed in **/
var() config float TargetFrictionZoomedBoostValue;

var() config bool bTargetAdhesionEnabled;

/** Max time to attempt adhesion to the friction target */
var() config float TargetAdhesionTimeMax;

/** Max distance to allow adhesion to still kick in */
var() config float TargetAdhesionDistanceMax;

/** Max distance from edge of cylinder for adhesion to be valid */
var() config float TargetAdhesionAimDistY;
var() config float TargetAdhesionAimDistZ;

/** Min/Max amount to scale for adhesive purposes */
var() config Vector2d TargetAdhesionScaleRange;

/** Min amount to scale for adhesive purposes */
var() config float TargetAdhesionScaleAmountMin;

/** Require the target to be moving for adhesion to kick in? */
var() config float TargetAdhesionTargetVelocityMin;

/** Require the player to be moving for adhesion to kick in? */
var() config float TargetAdhesionPlayerVelocityMin;

/** Boost the Target Adhesion by this much when zoomed in **/
var() config float TargetAdhesionZoomedBoostValue;

//// end of adhesion friction vars

replication
{
	// Server->Client properties
	if (Role == ROLE_Authority)
		AmmoCount, bLockedOnTarget, LockedTarget;
}

simulated function StartFire(byte FireModeNum)
{
	if (bDebugWeapon)
	{
		`log("---"@self$"."$GetStateName()$".StartFire("$FireModeNum$")");
		ScriptTrace();
	}
	Super.StartFire(FireModeNum);
}

reliable server function ServerStartFire(byte FireModeNum)
{
	if (bDebugWeapon)
	{
		`log("---"@self$"."$GetStateName()$".ServerStartFire("$FireModeNum$")");
		ScriptTrace();
	}
	// don't allow firing if no controller (generally, because player entered/exited a vehicle while simultaneously pressing fire)
	if (Instigator == None || Instigator.Controller != None || bAllowFiringWithoutController)
	{
		Super.ServerStartFire(FireModeNum);
	}
}

/*********************************************************************************************
 * Initialization / System Messages / Utility
 *********************************************************************************************/

/**
 * Initialize the weapon
 */
simulated function PostBeginPlay()
{
	local UTGameReplicationInfo GRI;

	Super.PostBeginPlay();

	CalcInventoryWeight();

	// tweak firing/reload/putdown/bringup rate if on console
	GRI = UTGameReplicationInfo(WorldInfo.GRI);
	if (GRI != None && GRI.bConsoleServer)
	{
		AdjustWeaponTimingForConsole();
	}
}

/**
  * Adjust weapon equip and fire timings so they match between PC and console
  * This is important so the sounds match up.
  */
simulated function AdjustWeaponTimingForConsole()
{
	local int i;

	For ( i=0; i<FireInterval.Length; i++ )
	{
		FireInterval[i] = FireInterval[i]/1.1;
	}
	EquipTime = EquipTime/1.1;
	PutDownTime = PutDownTime/1.1;
}

simulated function CreateOverlayMesh()
{
	local SkeletalMeshComponent SKM_Source, SKM_Target;
	local StaticMeshComponent STM;

	if (WorldInfo.NetMode != NM_DedicatedServer && Mesh != None)
	{
		OverlayMesh = new(outer) Mesh.Class;
		if (OverlayMesh != None)
		{
			OverlayMesh.SetScale(1.00);
			OverlayMesh.SetOwnerNoSee(Mesh.bOwnerNoSee);
			OverlayMesh.SetDepthPriorityGroup(SDPG_Foreground);
			OverlayMesh.CastShadow = false;

			if ( SkeletalMeshComponent(OverlayMesh) != none )
			{
				SKM_Target = SkeletalMeshComponent(OverlayMesh);
				SKM_Source = SkeletalMeshComponent(Mesh);

				SKM_Target.SetSkeletalMesh(SKM_Source.SkeletalMesh);
				SKM_Target.AnimSets = SKM_Source.AnimSets;
				SKM_Target.SetAnimTreeTemplate(SKM_Source.AnimTreeTemplate);
				SKM_Target.SetParentAnimComponent(SKM_Source);
				SKM_Target.bUpdateSkelWhenNotRendered = false;
				SKM_Target.bIgnoreControllersWhenNotRendered = true;

				if (UTSkeletalMeshComponent(SKM_Target) != none)
				{
					UTSkeletalMeshComponent(SKM_Target).SetFOV(UTSkeletalMeshComponent(SKM_Source).FOV);
				}
			}
			else if ( StaticMeshComponent(OverlayMesh) != none )
			{
				STM = StaticMeshComponent(OverlayMesh);
				STM.SetStaticMesh(StaticMeshComponent(Mesh).StaticMesh);
				STM.SetScale3D(Mesh.Scale3D);
				STM.SetTranslation(Mesh.Translation);
				STM.SetRotation(Mesh.Rotation);
			}
			OverlayMesh.SetHidden(Mesh.HiddenGame);
		}
		else
		{
			`Warn("Could not create Weapon Overlay mesh for" @ self @ Mesh);
		}
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if ( VarName == 'AmmoCount' && !HasAnyAmmo() )
	{
		WeaponEmpty();
	}

	Super.ReplicatedEvent(VarName);

}


/**
 * Each Weapon needs to have a unique InventoryWeight in order for weapon switching to
 * work correctly.  This function calculates that weight using the various inventory values
 * and the item name.
 */
simulated function CalcInventoryWeight()
{
	local int i;

	// Figure out the weight

	InventoryWeight = ((InventoryGroup+1) * 1000) + (GroupWeight * 100);
	for (i=0;i<Len(ItemName);i++)
	{
		InventoryWeight += float(asc(mid(ItemName,i,1))) / 1000;
	}

	if ( Priority < 0 )
		Priority = InventoryWeight;
}

/**
  *  returns true if this weapon is currently lower priority than InWeapon
  *  used to determine whether to switch to InWeapon
  */
simulated function bool ShouldSwitchTo(UTWeapon InWeapon)
{
	return !IsFiring() && !DenyClientWeaponSet() && (Priority < InWeapon.Priority);
}

/**
 * Material control
 *
 * @Param 	NewMaterial		The new material to apply or none to clear it
 */
simulated function SetSkin(Material NewMaterial)
{
	local int i,Cnt;

	if ( NewMaterial == None )
	{
		// Clear the materials
		if ( default.Mesh.Materials.Length > 0 )
		{
			Cnt = Default.Mesh.Materials.Length;
			for (i=0;i<Cnt;i++)
			{
				Mesh.SetMaterial( i, Default.Mesh.GetMaterial(i) );
			}
		}
		else if (Mesh.Materials.Length > 0)
		{
			Cnt = Mesh.Materials.Length;
			for ( i=0; i < Cnt; i++ )
			{
				Mesh.SetMaterial(i, none);
			}
		}
	}
	else
	{
		// Set new material
		if ( default.Mesh.Materials.Length > 0 || Mesh.GetNumElements() > 0 )
		{
			Cnt = default.Mesh.Materials.Length > 0 ? default.Mesh.Materials.Length : Mesh.GetNumElements();
			for ( i=0; i < Cnt; i++ )
			{
				Mesh.SetMaterial(i, NewMaterial);
			}
		}
	}
}

exec function hiddenTest()
{
	`log("### Here");
	Instigator.Mesh.SetHidden(false);
}

/*********************************************************************************************
 * Weapon Adjustment Functions
 *********************************************************************************************/

exec function AdjustMesh(string cmd)
{
	local string c,v;
	local vector t,s,o,k;
	local float f;
	local rotator r;
	local float sc;

	c = left(Cmd,InStr(Cmd,"="));
	v = mid(Cmd,InStr(Cmd,"=")+1);

	t  = PlayerViewOffset;//Mesh.Translation;
	r  = Mesh.Rotation;
	s  = Mesh.Scale3D;
	o  = FireOffset;
	sc = Mesh.Scale;
	k  = Mesh.Translation;


	if ( UTSkeletalMeshComponent(Mesh) != none)
		f = UTSkeletalMeshComponent(Mesh).FOV;
	else
		f = -1.0;

	if (c~="x")  t.X += float(v);
	if (c~="ax") t.X =  float(v);
	if (c~="y")  t.Y += float(v);
	if (c~="ay") t.Y =  float(v);
	if (c~="z")  t.Z += float(v);
	if (c~="az") t.Z =  float(v);

	if (c~="kx")  k.X += float(v);
	if (c~="kax") k.X =  float(v);
	if (c~="ky")  k.Y += float(v);
	if (c~="kay") k.Y =  float(v);
	if (c~="kz")  k.Z += float(v);
	if (c~="kaz") k.Z =  float(v);


	if (c~="ox")  o.X += float(v);
	if (c~="oax") o.X =  float(v);
	if (c~="oy")  o.Y += float(v);
	if (c~="oay") o.Y =  float(v);
	if (c~="oz")  o.Z += float(v);
	if (c~="oaz") o.Z =  float(v);


	if (c~="r")   R.Roll  += int(v);
	if (c~="ar")  R.Roll  =  int(v);
	if (c~="p")   R.Pitch += int(v);
	if (c~="ap")  R.Pitch =  int(v);
	if (c~="w")   R.Yaw   += int(v);
	if (c~="aw")  R.Yaw   =  int(v);

	if (c~="scalex") s.X = float(v);
	if (c~="scaley") s.Y = float(v);
	if (c~="scalez") s.Z = float(v);

	if (c~="scale") sc = float(v);

	if (c~="resetscale")
	{
		sc = 1.0;
		s.X = 1.0;
		s.Y = 1.0;
		s.Z = 1.0;
	}

	if ( c~="fov" && UTSkeletalMeshComponent(Mesh) != none )
		f = float(v);


	//Mesh.SetTranslation(t);
	PlayerViewOffset = t;
	FireOffset = o;
	Mesh.SetTranslation(k);
	Mesh.SetRotation(r);
	Mesh.SetScale(sc);
	Mesh.SetScale3D(s);

	if ( UTSkeletalMeshComponent(Mesh) != none)
	{
		UTSkeletalMeshComponent(Mesh).SetFOV(f);
	}


	`log("#### AdjustMesh ####");
	`log("####    View Offset :"@PlayerViewOffset);
	`log("####    Translation :"@Mesh.Translation);
	`log("####    Effects     :"@FireOffset);
	`log("####    Rotation    :"@Mesh.Rotation);
	`log("####    Scale3D     :"@Mesh.Scale3D);
	`log("####    scale       :"@Mesh.Scale);
	if ( UTSkeletalMeshComponent(Mesh) != none)
	 	`log("####    FOV         :"@UTSkeletalMeshComponent(Mesh).FOV);
	else
		`log("####    FOV         : None");
}

exec function AdjustFire(string cmd)
{
	local string c,v;
	local vector t;

	c = left(Cmd,InStr(Cmd,"="));
	v = mid(Cmd,InStr(Cmd,"=")+1);

	t  = FireOffset;

	if (c~="x")  t.X += float(v);
	if (c~="ax") t.X =  float(v);
	if (c~="y")  t.Y += float(v);
	if (c~="ay") t.Y =  float(v);
	if (c~="z")  t.Z += float(v);
	if (c~="az") t.Z =  float(v);

	FireOffset = t;
	`log("#### FireOffset ####");
	`log("####    Vector :"@FireOffset);

}


/*********************************************************************************************
 * Hud/Crosshairs
 *********************************************************************************************/

/**
 * Access to HUD and Canvas.
 * Event always called when the InventoryManager considers this Inventory Item currently "Active"
 * (for example active weapon)
 *
 * @param	HUD			- HUD with canvas to draw on
 */
simulated function ActiveRenderOverlays( HUD H )
{
	DrawZoom( H );

	DrawWeaponCrosshair( H );
	if (bLockedOnTarget)
	{
		DrawLockedOn( H );
	}
}

/**
 * Draw the locked on symbol
 */
simulated function DrawLockedOn( HUD H )
{
	local string s;
	local float xl,yl;

	S = "[Locked]";

	H.Canvas.DrawColor = H.WhiteColor;
	H.Canvas.Font = class'UTHUD'.static.GetFontSizeIndex(1);
	H.Canvas.Strlen(S, xl, yl);

	H.Canvas.SetPos(H.Canvas.ClipX/2 - xl/2, H.Canvas.ClipY/2+5+YL);
	H.Canvas.DrawText(s);
}

/**
 * Draw the Crosshairs
 */
simulated function DrawWeaponCrosshair( Hud HUD )
{
	local vector2d CrosshairSize;
	local float x,y,PickupScale,mU,mV,mUL,mVL;
	local UTHUD	H;


	H = UTHUD(HUD);
	if ( H == None )
		return;

	// Apply pickup scaling

	if ( H.LastPickupTime > WorldInfo.TimeSeconds - 0.3 )
	{
		if ( H.LastPickupTime > WorldInfo.TimeSeconds - 0.15 )
		{
			PickupScale = (1 + 5 * (WorldInfo.TimeSeconds - H.LastPickupTime));
		}
		else
		{
			PickupScale = (1 + 5 * (H.LastPickupTime + 0.3 - WorldInfo.TimeSeconds));
		}
	}
	else
	{
		PickupScale = 1.0;
	}


 	CrosshairSize.Y = CrossHairCoordinates.VL * ( bScaleCrosshair ? (H.Canvas.ClipY / 768) : 1.0 ) * PickupScale;
  	CrosshairSize.X = CrosshairSize.Y * ( CrossHairCoordinates.UL / CrossHairCoordinates.VL );

	X = H.Canvas.ClipX * 0.5;
	Y = H.Canvas.ClipY * 0.5;

	H.Canvas.DrawColor = CrosshairColor;
	H.Canvas.SetPos(X - (CrosshairSize.X * 0.5), Y - (CrosshairSize.Y * 0.5));

	if ( Texture2D(CrosshairImage) != none )
	{
		H.Canvas.DrawTile(Texture2D(CrosshairImage),CrosshairSize.X, CrosshairSize.Y, CrossHairCoordinates.U, CrossHairCoordinates.V, CrossHairCoordinates.UL,CrossHairCoordinates.VL);
	}
	else if ( Material(CrosshairImage) != none )
	{

    	// Create a MI so we can adjust colors/etc.

		if ( CrosshairMat == none )
		{
			CrosshairMat = new(Outer) class'MaterialInstanceConstant';
			CrosshairMat.SetParent(Material(CrosshairImage));
		}

	CrosshairMat.SetVectorParameterValue('CrosshairColor',MakeLinearColor(WeaponColor.R / 255,WeaponColor.G / 255,WeaponColor.B / 255, 1.0) );

    	mU = CrosshairCoordinates.U / 512;
    	mV = CrosshairCoordinates.V / 512;
    	mUL = CrosshairCoordinates.UL / 512;
    	mVL = CrosshairCoordinates.VL / 512;

		H.Canvas.DrawMaterialTile(CrosshairMat,CrosshairSize.X, CrosshairSize.Y, mU, mV, mUL,mVL);
	}
}

exec function SetSTP(float NewSTP)
{
	if ( CrosshairMat != none )
	{
		CrosshairMat.SetScalarParameterValue('SceneTexturePower',NewSTP);
	}
}

exec function SetSTM(float NewSTM)
{
	if ( CrosshairMat != none )
	{
		CrosshairMat.SetScalarParameterValue('SceneTextureMultiply',NewSTM);
	}
}

simulated function int GetAmmoCount()
{
	return AmmoCount;
}

/**
 * DrawZoom - Draws the overlays and fades when zooming
 */
simulated function DrawZoom( HUD H )
{
	local float FadeValue;
	local EZoomState ZoomState;

	ZoomState = GetZoomedState();

	if ( ZoomState > ZST_ZoomingOut )
	{
		DrawZoomedOverlay(H);
	}

	super.ActiveRenderOverlays(H);

	if ( ZoomState == ZST_ZoomingOut && FadeTime > 0.0 )
	{

		FadeValue = 255 * ( 1.0 - (WorldInfo.TimeSeconds - ZoomFadeTime)/FadeTime);
		H.Canvas.DrawColor.A = FadeValue;
		H.Canvas.SetPos(0,0);
		H.Canvas.DrawTile( Texture2D 'EngineResources.Black', H.Canvas.SizeX, H.Canvas.SizeY, 0.0, 0.0, 16, 16);
	}
}

/**
 * This function show draw any overlays needed when in Zoomed mode
 */
simulated function DrawZoomedOverlay( HUD H )
{
}


/**
 * list important Weapon variables on canvas.  HUD will call DisplayDebug() on the current ViewTarget when
 * the ShowDebug exec is used
 *
 * @param	HUD			- HUD with canvas to draw on
 * @input	out_YL		- Height of the current font
 * @input	out_YPos	- Y position on Canvas. out_YPos += out_YL, gives position to draw text for next debug line.
 */
simulated function DisplayDebug(HUD HUD, out float out_YL, out float out_YPos)
{
	Super.DisplayDebug(HUD, out_YL, out_YPos);

	if (UTPawn(Instigator) != None)
	{
		HUD.Canvas.DrawText("Eyeheight "$Instigator.EyeHeight$" base "$Instigator.BaseEyeheight$" landbob "$UTPawn(Instigator).Landbob$" just landed "$UTPawn(Instigator).bJustLanded$" land recover "$UTPawn(Instigator).bLandRecovery, false);
		out_YPos += out_YL;
	}

	HUD.Canvas.SetPos(4,out_YPos);
	HUD.Canvas.DrawText("Zoom State:"@GetZoomedState()@"ZoomedRate:"@ZoomedRate@"Target FOV:"@ZoomedTargetFOV);
	out_YPos+= out_YL;
}

simulated function GetWeaponDebug( out Array<String> DebugInfo )
{
	Super.GetWeaponDebug(DebugInfo);

	if ( WeaponLockType != WEAPLOCK_None )
	{
		DebugInfo[DebugInfo.Length] = "Locked: "@bLockedOnTarget@LockedTarget@LastLockedontime@(WorldInfo.TimeSeconds-LastLockedOnTime);
		DebugInfo[DebugInfo.Length] = "Pending:"@PendingLockedTarget@PendingLockedTargetTime@WorldInfo.TimeSeconds;
	}
}


/*********************************************************************************************
 * Attachments / Effects / etc
 *********************************************************************************************/
/**
 * Returns interval in seconds between each shot, for the firing state of FireModeNum firing mode.
 *
 * @param	FireModeNum	fire mode
 * @return	Period in seconds of firing mode
 */
simulated function float GetFireInterval( byte FireModeNum )
{
	return FireInterval[FireModeNum] * ((UTPawn(Owner)!= None) ? UTPawn(Owner).FireRateMultiplier : 1.0);
}

simulated function PlayArmAnimation( Name Sequence, float fDesiredDuration, optional bool OffHand, optional bool bLoop, optional SkeletalMeshComponent SkelMesh)
{
	local UTPawn UTP;
	// do not play on a dedicated server or if they aren't being seen
	if( WorldInfo.NetMode == NM_DedicatedServer || !Instigator.IsFirstPerson())
	{
		return;
	}
	UTP = UTPawn(Instigator);
	if(UTP == none)
	{
		return;
	}
	if(UTP.bArmsAttached)
	{
		// Check we have access to mesh and animations
		if(!OffHand)
		{
			if( UTP.ArmsMesh[0] == None || ArmsAnimSet==none || GetArmAnimNodeSeq() == None )
			{
				return;
			}

			// @todo - this should call GetWeaponAnimNodeSeq, move 'duration' code into AnimNodeSequence and use that.
			UTP.ArmsMesh[0].PlayAnim(Sequence, fDesiredDuration, bLoop);
		}
		else
		{
			if(UTP.ArmsMesh[1] == None || GetArmAnimNodeSeq() == None )
			{
				return;
			}

			// @todo - this should call GetWeaponAnimNodeSeq, move 'duration' code into AnimNodeSequence and use that.
			UTP.ArmsMesh[1].PlayAnim(Sequence, fDesiredDuration, bLoop);
		}
	}
}


/**
 * PlayFireEffects Is the root function that handles all of the effects associated with
 * a weapon.  This function creates the 1st person effects.  It should only be called
 * on a locally controlled player.
 */
simulated function PlayFireEffects( byte FireModeNum, optional vector HitLocation )
{
	// Play Weapon fire animation

	if ( FireModeNum < WeaponFireAnim.Length && WeaponFireAnim[FireModeNum] != '' )
		PlayWeaponAnimation( WeaponFireAnim[FireModeNum], GetFireInterval(FireModeNum) );
	if ( FireModeNum < ArmFireAnim.Length && ArmFireAnim[FireModeNum] != '' && ArmsAnimSet != none)
		PlayArmAnimation( ArmFireAnim[FireModeNum], GetFireInterval(FireModeNum) );

	// Start muzzle flash effect
	CauseMuzzleFlash();

	ShakeView();
}

simulated function StopFireEffects(byte FireModeNum)
{
	StopMuzzleFlash();
}

/** plays view shake on the owning client only */
simulated function ShakeView()
{
	local UTPlayerController PC;
	//local ViewShakeInfo AdjustedShake;
	//local float ZoomRatio;

	PC = UTPlayerController(Instigator.Controller);
	if (PC != None && LocalPlayer(PC.Player) != None && CurrentFireMode < FireCameraAnim.length && FireCameraAnim[CurrentFireMode] != None)
	{
		PC.PlayCameraAnim(FireCameraAnim[CurrentFireMode], (GetZoomedState() > ZST_ZoomingOut) ? PC.FOVAngle / PC.DefaultFOV : 1.0);
	}
	/** @FIXME: remove if we keep new camera anim system for viewshakes
	if (PC != None && LocalPlayer(PC.Player) != None && CurrentFireMode < FireShake.length)
	{
		// reduce the viewshake if the player is zoomed in
		if ( GetZoomedState() > ZST_ZoomingOut )
		{
			ZoomRatio =  PC.FOVAngle / PC.DefaultFOV;
			AdjustedShake = FireShake[CurrentFireMode];
			AdjustedShake.RotMag *= ZoomRatio;
			AdjustedShake.OffsetMag *= ZoomRatio;
			PC.ShakeView(AdjustedShake);
		}
		else
		{
			PC.ShakeView(FireShake[CurrentFireMode]);
		}
	}
	*/
}

simulated function WeaponPlaySound(SoundCue Sound, optional float NoiseLoudness)
{
	if (!bSuppressSounds)
	{
		Super.WeaponPlaySound(Sound, NoiseLoudness);
	}
}

/**
 * Tells the weapon to play a firing sound (uses CurrentFireMode)
 */
simulated function PlayFiringSound()
{
	if (CurrentFireMode<WeaponFireSnd.Length)
	{
		// play weapon fire sound
		if ( WeaponFireSnd[CurrentFireMode] != None )
		{
			MakeNoise(1.0);
			WeaponPlaySound( WeaponFireSnd[CurrentFireMode] );
		}
	}
}

/**
 * Turns the MuzzleFlashlight off
 */
simulated event MuzzleFlashTimer()
{
	if (MuzzleFlashPSC != none && (!bMuzzleFlashPSCLoops) )
	{
		MuzzleFlashPSC.DeactivateSystem();
	}
}

/**
 * Causes the muzzle flashlight to turn on
 */
simulated event CauseMuzzleFlashLight()
{
	if ( WorldInfo.bDropDetail )
		return;

	if ( MuzzleFlashLight != None )
	{
		MuzzleFlashLight.ResetLight();
	}
	else if ( MuzzleFlashLightClass != None )
	{
		MuzzleFlashLight = new(Outer) MuzzleFlashLightClass;
		SkeletalMeshComponent(Mesh).AttachComponentToSocket(MuzzleFlashLight,MuzzleFlashSocket);
	}
}

/**
 * Causes the muzzle flash to turn on and setup a time to
 * turn it back off again.
 */
simulated event CauseMuzzleFlash()
{
	CauseMuzzleFlashLight();

	if (MuzzleFlashPSC != none)
	{
		if (!bMuzzleFlashPSCLoops || (!MuzzleFlashPSC.bIsActive || MuzzleFlashPSC.bWasDeactivated))
		{
			if (Instigator != None && Instigator.FiringMode == 1 && MuzzleFlashAltPSCTemplate != None)
			{
				MuzzleFlashPSC.SetTemplate(MuzzleFlashAltPSCTemplate);
			}
			else if ( MuzzleFlashPSCTemplate != None )
			{
				MuzzleFlashPSC.SetTemplate(MuzzleFlashPSCTemplate);
			}
			SetMuzzleFlashParams(MuzzleFlashPSC);
			MuzzleFlashPSC.ActivateSystem();
		}
	}

	// Set when to turn it off.
	SetTimer(MuzzleFlashDuration,false,'MuzzleFlashTimer');
}

simulated event StopMuzzleFlash()
{
	ClearTimer('MuzzleFlashTimer');
	MuzzleFlashTimer();

	if ( MuzzleFlashPSC != none )
	{
		MuzzleFlashPSC.DeactivateSystem();
	}
}


/**
 * Sets the timing for putting a weapon down.  The WeaponIsDown event is trigged when expired
*/
simulated function TimeWeaponPutDown()
{
	if( Instigator.IsFirstPerson() )
	{
		PlayWeaponPutDown();
	}

	super.TimeWeaponPutDown();
}

/**
 * Show the weapon being put away
 */
simulated function PlayWeaponPutDown()
{
	// Play the animation for the weapon being put down

	if ( WeaponPutDownAnim != '' )
		PlayWeaponAnimation( WeaponPutDownAnim, PutDownTime );
	if ( ArmsPutDownAnim != '' && ArmsAnimSet != none)
	{
		PlayArmAnimation( ArmsPutDownAnim, PutDownTime );
	}

	// play any associated sound
	if ( WeaponPutDownSnd != None )
		WeaponPlaySound( WeaponPutDownSnd );
}

/**
 * Sets the timing for equipping a weapon.
 * The WeaponEquipped event is trigged when expired
 */
simulated function TimeWeaponEquipping()
{
	// The weapon is equipped, attach it to the mesh.
	AttachWeaponTo( Instigator.Mesh );

	// Play the animation
	if( Instigator.IsFirstPerson() )
	{
		PlayWeaponEquip();
	}

	SetTimer( GetEquipTime() , false, 'WeaponEquipped');
}

simulated function float GetEquipTime()
{
	local float ETime;

	ETime = EquipTime>0 ? EquipTime : 0.01;
	if ( PendingFire(0) || PendingFire(1) )
	{
		ETime += 0.25;
	}
	return ETime;
}

/**
 * Show the weapon begin equipped
 */
simulated function PlayWeaponEquip()
{
	// Play the animation for the weapon being put down

	if ( WeaponEquipAnim != '' )
		PlayWeaponAnimation( WeaponEquipAnim, EquipTime );
	if ( ArmsEquipAnim != '' && ArmsAnimSet != none)
	{
		PlayArmAnimation(ArmsEquipAnim, EquipTime);
	}

	// play any assoicated sound
	if ( WeaponEquipSnd != None )
		WEaponPlaySound( WeaponEquipSnd );
}

 /**
 * Attach Weapon Mesh, Weapon MuzzleFlash and Muzzle Flash Dynamic Light to a SkeletalMesh
 *
 * @param	who is the pawn to attach to
 */
simulated function AttachWeaponTo( SkeletalMeshComponent MeshCpnt, optional Name SocketName )
{
	local UTPawn UTP;

	AttachComponent( Mesh );

	if ( bDebugWeapon && Instigator != none && Instigator.Controller != none && PlayerController(Instigator.Controller) != none )
	{
		`log("###"@self$"."$GetOwnerName()@".AttachWeaponTo"@MeshCpnt@Instigator.IsFirstPerson());
	}

	UTP = UTPawn(Instigator);
	// Attach 1st Person Muzzle Flashes, etc,
	if ( Instigator.IsFirstPerson() )
	{
		SetHidden(False);
		AttachMuzzleFlash();
		Mesh.SetLightEnvironment(UTP.LightEnvironment);
	}
	else
	{
		SetHidden(True);
		if (UTP != None)
		{
			UTP.ArmsMesh[0].SetHidden(true);
			UTP.ArmsMesh[1].SetHidden(true);
			if (UTP.ArmsOverlay[0] != None)
			{
				UTP.ArmsOverlay[0].SetHidden(true);
				UTP.ArmsOverlay[1].SetHidden(true);
			}
		}
	}

	/** @FIXME: TEMP HACKFIX FOR OVERLAY FRAME-BEHIND BUG */
	if (bUseOverlayHack)
	{
		SetTimer(0.1, false, 'ApplyOverlay');
	}
	else
	{
		SetWeaponOverlayFlags( UTPawn(Instigator) );
	}

	// Spawn the 3rd Person Attachment
	if (Role == ROLE_Authority && UTP != None)
	{
		UTP.CurrentWeaponAttachmentClass = AttachmentClass;
		if (Instigator.IsLocallyControlled() || WorldInfo.NetMode == NM_ListenServer || WorldInfo.NetMode == NM_Standalone)
		{
			UTP.WeaponAttachmentChanged();
		}
	}

	SetSkin(UTPawn(Instigator).ReplicatedBodyMaterial);

}

simulated function ApplyOverlay()
{
	SetWeaponOverlayFlags(UTPawn(Instigator));
}

/**
 * Allows a child to setup custom parameters on the muzzle flash
 */

simulated function SetMuzzleFlashParams(ParticleSystemComponent PSC)
{
	PSC.SetColorParameter('MuzzleFlashColor', MuzzleFlashColor);
	PSC.SetVectorParameter('MFlashScale',Vect(0.5,0.5,0.5));
}

/**
 * Called on a client, this function Attaches the WeaponAttachment
 * to the Mesh.
 */


simulated function AttachMuzzleFlash()
{
	local SkeletalMeshComponent SKMesh;

	// Attach the Muzzle Flash
	SKMesh = SkeletalMeshComponent(Mesh);
	if (  SKMesh != none )
	{
		if ( (MuzzleFlashPSCTemplate != none) || (MuzzleFlashAltPSCTemplate != none) )
		{
			MuzzleFlashPSC = new(Outer) class'UTParticleSystemComponent';
			MuzzleFlashPSC.bAutoActivate = false;
			MuzzleFlashPSC.SetDepthPriorityGroup(SDPG_Foreground);
			SKMesh.AttachComponentToSocket(MuzzleFlashPSC, MuzzleFlashSocket);
		}
	}

}


/**
 * Detach weapon from skeletal mesh
 *
 * @param	SkeletalMeshComponent weapon is attached to.
 */
simulated function DetachWeapon()
{
	local UTPawn P;

	DetachComponent( Mesh );
	if (OverlayMesh != None)
	{
		DetachComponent(OverlayMesh);
	}

	SetSkin(None);

	P = UTPawn(Instigator);
	if (P != None)
	{
		if (Role == ROLE_Authority && P.CurrentWeaponAttachmentClass == AttachmentClass)
		{
			P.CurrentWeaponAttachmentClass = None;
			if (Instigator.IsLocallyControlled())
			{
				P.WeaponAttachmentChanged();
			}
		}
		P.bDualWielding = false;
	}

	SetBase(None);
	SetHidden(True);
	DetachMuzzleFlash();
	Mesh.SetLightEnvironment(None);
}

/**
 * Remove/Detach the muzzle flash components
 */
simulated function DetachMuzzleFlash()
{
	local SkeletalMeshComponent SKMesh;

	SKMesh = SkeletalMeshComponent(Mesh);
	if (  SKMesh != none )
	{
		if (MuzzleFlashPSC != none)
			SKMesh.DetachComponent( MuzzleFlashPSC );
	}
}
/**
 * This function is called from the pawn when the visibility of the weapon changes
 */

simulated function ChangeVisibility(bool bIsVisible)
{
	local UTPawn UTP;
	local SkeletalMeshComponent SkelMesh;
	local PrimitiveComponent Primitive;

	if (Mesh != None)
	{
		Mesh.SetHidden(!bIsVisible);
		SkelMesh = SkeletalMeshComponent(Mesh);
		if (SkelMesh != None)
		{
			foreach SkelMesh.AttachedComponents(class'PrimitiveComponent', Primitive)
			{
				Primitive.SetHidden(!bIsVisible);
			}
		}
	}
	if (ArmsAnimSet != None)
	{
		UTP = UTPawn(Instigator);
		if (UTP != None && UTP.ArmsMesh[0] != None)
		{
			UTP.ArmsMesh[0].SetHidden(!bIsVisible);
			if (UTP.ArmsOverlay[0] != None)
			{
				UTP.ArmsOverlay[0].SetHidden(!bIsVisible);
			}
		}
	}

	if ( OverlayMesh != none )
	{
		OverlayMesh.SetHidden(!bIsVisible);
	}
}

/*********************************************************************************************
 * Pawn/Controller/View functions
 *********************************************************************************************/

simulated function GetViewAxes( out vector xaxis, out vector yaxis, out vector zaxis )
{
	if ( UTVehicle(Owner) != none )
	{
		UTVehicle(Owner).GetWeaponViewAxes(self, xaxis, yaxis, zaxis);
	}
	else if ( Instigator.Controller == None )
	{
	GetAxes( Instigator.Rotation, xaxis, yaxis, zaxis );
    }
    else
    {
	GetAxes( Instigator.Controller.Rotation, xaxis, yaxis, zaxis );
    }
}

/**
 * This function is called whenever you attempt to reselect the same weapon
 */
reliable server function ServerReselectWeapon();


/**
 * Returns true if this item can be thrown out.
 */
simulated function bool CanThrow()
{
	return bCanThrow && HasAnyAmmo();
}


/**
 * Called from pawn to tell the weapon it's being held in hand NewWeaponHand
 */
simulated function SetHand(UTPawn.EWeaponHand NewWeaponHand)
{
	// FIXME:: Invert the mesh
}

/**
 * Returns the current Weapon Hand
 */
simulated function UTPawn.EWeaponHand GetHand()
{
	// Get the Weapon Hand from the pawn or default to HAND_Right
	if ( UTPawn(Instigator)!=none )
	{
		if ( UTBot(Instigator.Controller) != None )
		{
			return HAND_Centered;
		}
		return UTPawn(Instigator).WeaponHand;
	}
	else
		return HAND_Right;
}
/**
 * This function aligns the gun model in the world
 */
simulated event SetPosition(UTPawn Holder)
{
	local vector DrawOffset, ViewOffset, FinalLocation;
	local UTPawn.EWeaponHand CurrentHand;
	local rotator NewRotation, FinalRotation;

	if ( !Instigator.IsFirstPerson() )
		return;

	// Hide the weapon if hidden
	CurrentHand = GetHand();
	if ( CurrentHand == HAND_Hidden)
	{
		SetHidden(True);
		return;
	}
	SetHidden(False);

	// Adjust for the current hand
	ViewOffset = PlayerViewOffset;
	switch ( CurrentHand )
	{
		case HAND_Left:
			ViewOffset.Y *= -1;
			break;

		case HAND_Centered:
			ViewOffset.Y = 0;
			break;
	}

	// Calculate the draw offset
	if ( Holder.Controller == None )
	{
		DrawOffset = (ViewOffset >> Rotation) + Holder.GetEyeHeight() * vect(0,0,1);
	}
	else
	{
		DrawOffset.Z = Holder.GetEyeHeight();
		if ( Holder.bWeaponBob )
		{
			DrawOffset += Holder.WeaponBob(BobDamping, JumpDamping);
		}

		if ( UTPlayerController(Holder.Controller) != None )
		{
			DrawOffset += UTPlayerController(Holder.Controller).ShakeOffset >> Holder.Controller.Rotation;
		}

		DrawOffset = DrawOffset + ( ViewOffset >> Holder.Controller.Rotation );
	}

	// Adjust it in the world
	FinalLocation = Holder.Location + DrawOffset;
	SetLocation(FinalLocation);
	SetBase(Holder);

	// HACKCODE
	//DrawOffset = UTPlayerController(Holder.Controller).ShakeOffset >> Holder.Controller.Rotation;
	//DrawOffset = Holder.WeaponBob(BobDamping, JumpDamping);
	//DrawOffset.Z += Holder.GetEyeHeight();
	// ENDHACK
	if (ArmsAnimSet != None)
	{
		Holder.ArmsMesh[0].SetTranslation(DrawOffset);
		Holder.ArmsMesh[1].SetTranslation(DrawOffset);
		if (Holder.ArmsOverlay[0] != None)
		{
			Holder.ArmsOverlay[0].SetTranslation(DrawOffset);
			Holder.ArmsOverlay[1].SetTranslation(DrawOffset);
		}
	}

	NewRotation = (Holder.Controller == None ) ? Holder.Rotation : Holder.Controller.Rotation;
	if ( Holder.bWeaponBob )
	{
		// if bWeaponBob, then add some rotation leading
		FinalRotation.Yaw = LagRot(NewRotation.Yaw & 65535, LastRotation.Yaw & 65535, MaxYawLag, 0);
		FinalRotation.Pitch = LagRot(NewRotation.Pitch & 65535, LastRotation.Pitch & 65535, MaxPitchLag, 1);
		LastRotUpdate = WorldInfo.TimeSeconds;
		LastRotation = NewRotation;
	}
	else
	{
		FinalRotation = NewRotation;
	}
	SetRotation(FinalRotation);
	if (ArmsAnimSet != None)
	{
		Holder.ArmsMesh[0].SetRotation(FinalRotation);
		Holder.ArmsMesh[1].SetRotation(FinalRotation);
		if (Holder.ArmsOverlay[0] != None)
		{
			Holder.ArmsOverlay[0].SetRotation(FinalRotation);
			Holder.ArmsOverlay[1].SetRotation(FinalRotation);
		}
	}
}

/** @return whether the weapon's rotation is allowed to lag behind the holder's rotation */
simulated function bool ShouldLagRot()
{
	return false;
}

simulated function int LagRot(int NewValue, int LastValue, float MaxDiff, int Index)
{
	local int RotDiff;
	local float LeadMag, DeltaTime;

	if ( NewValue ClockWiseFrom LastValue )
	{
		if ( LastValue > NewValue )
		{
			LastValue -= 65536;
		}
	}
	else
	{
		if ( NewValue > LastValue )
		{
			NewValue -= 65536;
		}
	}

	DeltaTime = WorldInfo.TimeSeconds - LastRotUpdate;
	RotDiff = NewValue - LastValue;
	if ( (RotDiff == 0) || (OldRotDiff[Index] == 0) )
	{
		LeadMag = ShouldLagRot() ? OldLeadMag[Index] : 0.0;
		if ( (RotDiff == 0) && (OldRotDiff[Index] == 0) )
		{
			OldMaxDiff[Index] = 0;
		}
	}
	else if ( (RotDiff > 0) == (OldRotDiff[Index] > 0) )
	{
		if (ShouldLagRot())
		{
			MaxDiff = FMin(1, Abs(RotDiff)/(12000*DeltaTime)) * MaxDiff;
			if ( OldMaxDiff[Index] != 0 )
				MaxDiff = FMax(OldMaxDiff[Index], MaxDiff);

			OldMaxDiff[Index] = MaxDiff;
			LeadMag = (NewValue > LastValue) ? -1* MaxDiff : MaxDiff;
		}
		else
		{
			LeadMag = 0;
		}
		if ( DeltaTime < 1/RotChgSpeed )
		{
			LeadMag = (1.0 - RotChgSpeed*DeltaTime)*OldLeadMag[Index] + RotChgSpeed*DeltaTime*LeadMag;
		}
		else
		{
			LeadMag = 0;
		}
	}
	else
	{
		LeadMag = 0;
		OldMaxDiff[Index] = 0;
		if ( DeltaTime < 1/ReturnChgSpeed )
		{
			LeadMag = (1 - ReturnChgSpeed*DeltaTime)*OldLeadMag[Index] + ReturnChgSpeed*DeltaTime*LeadMag;
		}
	}
	OldLeadMag[Index] = LeadMag;
	OldRotDiff[Index] = RotDiff;

	return NewValue + LeadMag;
}

/**
 * called every time owner takes damage while holding this weapon - used by shield gun
 */
function AdjustPlayerDamage( out int Damage, Controller InstigatedBy, Vector HitLocation,
			     out Vector Momentum, class<DamageType> DamageType)
{
}

/*********************************************************************************************
 * AI interface
 *********************************************************************************************/

/**
 * Returns a weight reflecting the desire to use the
 * given weapon, used for AI and player best weapon
 * selection.
 *
 * @param	Weapon W
 * @return	Weapon rating (range -1.f to 1.f)
 */
simulated function float GetWeaponRating()
{
	if ( (Instigator == None) || (UTBot(Instigator.Controller) == None) || !HasAnyAmmo() )
		CurrentRating = Priority;
	else
		CurrentRating = UTBot(Instigator.Controller).RateWeapon(self);

	return CurrentRating;
}

/**
 * return false if out of range, can't see target, etc.
 */
function bool CanAttack(Actor Other)
{
	local float Dist, CheckDist, OtherRadius, OtherHeight;
	local vector HitLocation, HitNormal, projStart;
	local Actor HitActor;
	local class<Projectile> ProjClass;
	local int i;

	if (Instigator == None || Instigator.Controller == None)
	{
		return false;
	}

	// check that target is within range
	Dist = VSize(Instigator.Location - Other.Location);
	if (Dist > MaxRange())
	{
		return false;
	}

	// check that can see target
	if (!Instigator.Controller.LineOfSightTo(Other))
	{
		return false;
	}

	if ( !bInstantHit )
	{
		ProjClass = GetProjectileClass();
		if ( ProjClass == None )
		{
			for (i = 0; i < WeaponProjectiles.length; i++)
			{
				ProjClass = WeaponProjectiles[i];
				if (ProjClass != None)
				{
					break;
				}
			}
		}
		if (ProjClass == None)
		{
			`warn("No projectile class for "$self);
			CheckDist = 300;
		}
		else
		{
			CheckDist = FMax(CheckDist, 0.5 * ProjClass.default.Speed);
			CheckDist = FMax(CheckDist, 300);
			CheckDist = FMin(CheckDist, VSize(Other.Location - Location));
		}
	}

	// check that would hit target, and not a friendly
	projStart = Instigator.GetWeaponStartTraceLocation(self);
	Other.GetBoundingCylinder(OtherRadius,OtherHeight);
	// perform the trace
	if ( bInstantHit )
	{
		HitActor = GetTraceOwner().Trace(HitLocation, HitNormal, Other.Location + OtherHeight * vect(0,0,0.8), projStart, true);
	}
	else
	{
		// for non-instant hit, only check partial path (since others may move out of the way)
		HitActor = GetTraceOwner().Trace( HitLocation, HitNormal,
						projStart + CheckDist * Normal(Other.Location + OtherHeight * vect(0,0,0.8) - Location),
						projStart, true );
	}

	if (HitActor == None || HitActor == Other || Pawn(HitActor) == None
		|| Pawn(HitActor).Controller == None || !WorldInfo.GRI.OnSameTeam(Instigator, HitActor) )
	{
		return true;
	}

	return false;
}

/**
 * tell the bot how much it wants this weapon pickup
 * called when the bot is trying to decide which inventory pickup to go after next
 */
static function float BotDesireability(Actor PickupHolder, Pawn P)
{
	local UTWeapon AlreadyHas;
	local float desire;
	local UTBot Bot;

	Bot = UTBot(P.Controller);
	if ( Bot == None )
		return 0;

	if ( UTWeaponLocker(PickupHolder) != None )
		return UTWeaponLocker(PickupHolder).BotDesireability(P);

	// bots adjust their desire for their favorite weapons
	desire = Default.MaxDesireability;
	if (ClassIsChildOf(default.Class, Bot.FavoriteWeapon))
	{
		desire *= 1.5;
	}

	// see if bot already has a weapon of this type
	AlreadyHas = UTWeapon(P.FindInventoryType(default.class));
	if ( AlreadyHas != None )
	{
		if ( Bot.bHuntPlayer )
			return 0;

		// can't pick it up if weapon stay is on
		if ( (UTWeaponPickupFactory(PickupHolder) != None) && !UTWeaponPickupFactory(PickupHolder).AllowRepeatPickup() )
			return 0;

		if ( AlreadyHas.AmmoMaxed(0) )
			return 0.25 * desire;

		// bot wants this weapon for the ammo it holds
		if( AlreadyHas.AmmoCount > 0 )
		{
			if ( Default.AmmoPickupClass == None )
				return 0.05;
			else
				return FMax( 0.25 * desire,
						Default.AmmoPickupClass.Default.MaxDesireability
						* FMin(1, 0.15 * AlreadyHas.MaxAmmoCount/AlreadyHas.AmmoCount) );
		}
		else
			return 0.05;
	}
	if ( Bot.bHuntPlayer && (desire * 0.833 < P.Weapon.AIRating - 0.1) )
		return 0;

	// incentivize bot to get this weapon if it doesn't have a good weapon already
	if ( (P.Weapon == None) || (P.Weapon.AIRating < 0.5) )
		return 2*desire;

	return desire;
}

/**
 * CanHeal()
 * used by bot AI should return true if this weapon is able to heal Other
 */
function bool CanHeal(Actor Other)
{
	return false;
}

/** used by bot AI to get the optimal range for shooting Target
 * can be called on friendly Targets if trying to heal it
 */
function float GetOptimalRangeFor(Actor Target)
{
	return MaxRange();
}

function FireHack();

/**
 * tells AI that it needs to release the fire button for this weapon to do anything
 */
function bool FireOnRelease()
{
	return ( ShouldFireOnRelease[CurrentFireMode] != 0 );
}

/**
 * return true if recommend jumping while firing to improve splash damage (by shooting at feet)
 * true for R.L., for example
 */
function bool SplashJump()
{
    return bSplashJump;
}

/**
 * called by AI when camping/defending
 * return true if it is useful to fire this weapon even though bot doesn't have a target
 * for example, a weapon that launches turrets or mines
 */
function bool ShouldFireWithoutTarget()
{
	return false;
}

/**
 * BestMode()
 * choose between regular or alt-fire
 */
function byte BestMode()
{
	local byte Best;
	if ( IsFiring() )
		return CurrentFireMode;

	if ( FRand() < 0.5 )
		Best = 1;

	if ( Best < bZoomedFireMode.Length && bZoomedFireMode[Best] != 0 )
		return 0;
	else
		return Best;
}

/**
 * ReadyToFire()
 * called by NPC firing weapon.
 * bFinished should only be true if called from the Finished() function
 */
function bool ReadyToFire(bool bFinished)
{
	return false;
}

simulated function bool StillFiring(byte FireMode)
{
	if ( UTBot(Instigator.Controller) != None )
	{
		ClearPendingFire(0);
		ClearPendingFire(1);
		UTBot(Instigator.Controller).WeaponFireAgain(RefireRate(), true);
	}

	return super.StillFiring(FireMode);
}

/*********************************************************************************************
 * Ammunition / Inventory
 *********************************************************************************************/

/**
 * Consumes some of the ammo
 */
function ConsumeAmmo( byte FireModeNum )
{
	// Subtract the Ammo
	AddAmmo(-ShotCost[FireModeNum]);
}

/**
 * This function is used to add ammo back to a weapon.  It's called from the Inventory Manager
 */
function int AddAmmo( int Amount )
{
	AmmoCount = Clamp(AmmoCount + Amount,0,MaxAmmoCount);
	// check for infinite ammo
	if (AmmoCount <= 0 && (UTInventoryManager(InvManager) == None || UTInventoryManager(InvManager).bInfiniteAmmo))
	{
		AmmoCount = MaxAmmoCount;
	}

	return AmmoCount;
}

/**
 * This function will fill the weapon up to it's maximum amount of ammo
 */
simulated function FillToInitialAmmo()
{
	AmmoCount = Max(AmmoCount,Default.AmmoCount);
}

/**
 * Retrusn true if the ammo is maxed out
 */
simulated function bool AmmoMaxed(int mode)
{
	return (AmmoCount >= MaxAmmoCount);
}

/**
 * This function checks to see if the weapon has any ammo availabel for a given fire mode.
 *
 * @param	FireModeNum		- The Fire Mode to Test For
 * @param	Amount			- [Optional] Check to see if this amount is available.  If 0 it will default to checking
 *							  for the ShotCost
 */
simulated function bool HasAmmo( byte FireModeNum, optional int Amount )
{
	if (Amount==0)
		return (AmmoCount >= ShotCost[FireModeNum]);
	else
		return ( AmmoCount >= Amount );
}

/**
 * returns true if this weapon has any ammo
 */
simulated function bool HasAnyAmmo()
{
	return ( ( AmmoCount > 0 ) || (ShotCost[0]==0 && ShotCost[1]==0) );
}
/**
 * This function retuns how much of the clip is empty.
 */
simulated function float DesireAmmo(bool bDetour)
{
	return (1.f - float(AmmoCount)/MaxAmmoCount);
}

/**
 * Returns true if the current ammo count is less than the default ammo count
 */
simulated function bool NeedAmmo()
{
	return ( AmmoCount < Default.AmmoCount );
}

/**
 * Cheat Help function the loads out the weapon
 *
 * @param 	bUseWeaponMax 	- [Optional] If true, this function will load out the weapon
 *							  with the actual maximum, not 999
 */
simulated function Loaded(optional bool bUseWeaponMax)
{
	if (bUseWeaponMax)
		AmmoCount = MaxAmmoCount;
	else
		AmmoCount = 999;
}

function bool DenyPickupQuery(class<Inventory> ItemClass, Actor Pickup)
{
	local DroppedPickup DP;

	// By default, you can only carry a single item of a given class.
	if ( ItemClass == class )
	{
		DP = DroppedPickup(Pickup);
		if (DP != None)
		{
			if ( DP.Instigator == Instigator )
			{
				// weapon was dropped by this player - disallow pickup
				return true;
			}
			// take the ammo that the dropped weapon has
			AddAmmo(UTWeapon(DP.Inventory).AmmoCount);
			DP.PickedUpBy(Instigator);
			AnnouncePickup(Instigator);
		}
		else if (bSuperWeapon || !UTGame(WorldInfo.Game).bWeaponStay)
		{
			// add the ammo that the pickup should give us, then tell it to respawn
			AddAmmo(default.AmmoCount);
			Pickup.PickedUpBy(Instigator);
			AnnouncePickup(Instigator);
		}
		return true;
	}

	return false;
}


/**
 * Called when the weapon runs out of ammo during firing
 */
simulated function WeaponEmpty()
{

	if (bDebugWeapon)
	{
		`log("---"@self$"."$GetStateName()$".WeaponEmpty()"@IsFiring()@Instigator@Instigator.IsLocallyControlled());
		ScriptTrace();
	}


	// If we were firing, stop
	if ( IsFiring() )
	{
		GotoState('Active');
	}

	if ( Instigator != none && Instigator.IsLocallyControlled() )
	{
		Instigator.InvManager.SwitchToBestWeapon( true );
	}
}

/**
 * @returns false if the weapon isn't ready to be fired.  For example, if it's in the Inactive/WeaponPuttingDown states.
 */
simulated function bool bReadyToFire()
{
	return true;
}

/*********************************************************************************************
 * Firing
 *********************************************************************************************/

/**
* @returns position of trace start for instantfire()
*/
simulated function vector InstantFireStartTrace()
{
	return Instigator.GetWeaponStartTraceLocation();
}

/**
* @returns end trace position for instantfire()
*/
simulated function vector InstantFireEndTrace(vector StartTrace)
{
	return StartTrace + vector(GetAdjustedAim(StartTrace)) * GetTraceRange();
}

/**
 * Performs an 'Instant Hit' shot.
 * Also, sets up replication for remote clients,
 * and processes all the impacts to deal proper damage and play effects.
 *
 * Network: Local Player and Server
 */
simulated function InstantFire()
{
	local vector StartTrace, EndTrace;
	local Array<ImpactInfo>	ImpactList;
	local ImpactInfo RealImpact, NearImpact;
	local int i, FinalImpactIndex;

	UpdateFiredStats(1);

	// define range to use for CalcWeaponFire()
	StartTrace = InstantFireStartTrace();
	EndTrace = InstantFireEndTrace(StartTrace);
	bUsingAimingHelp = false;
	// Perform shot
	RealImpact = CalcWeaponFire(StartTrace, EndTrace, ImpactList);
	FinalImpactIndex = ImpactList.length - 1;

	if (FinalImpactIndex >= 0 && (ImpactList[FinalImpactIndex].HitActor == None || !ImpactList[FinalImpactIndex].HitActor.bProjTarget))
	{
		// console aiming help
		NearImpact = InstantAimHelp(StartTrace, EndTrace, RealImpact);
		if ( NearImpact.HitActor != None )
		{
			bUsingAimingHelp = true;
			ImpactList[FinalImpactIndex] = NearImpact;
		}
	}

	for (i = 0; i < ImpactList.length; i++)
	{
		ProcessInstantHit(CurrentFireMode, ImpactList[i]);
	}

	if (Role == ROLE_Authority)
	{
		// Set flash location to trigger client side effects.
		// if HitActor == None, then HitLocation represents the end of the trace (maxrange)
		// Remote clients perform another trace to retrieve the remaining Hit Information (HitActor, HitNormal, HitInfo...)
		// Here, The final impact is replicated. More complex bullet physics (bounce, penetration...)
		// would probably have to run a full simulation on remote clients.
		if ( NearImpact.HitActor != None )
		{
			SetFlashLocation(NearImpact.HitLocation);
		}
		else
		{
			SetFlashLocation(RealImpact.HitLocation);
		}
	}
}

/**
  * Look for "near miss" of target within UTPC.AimHelpModifier() * AimingHelpRadius
  * Return that target as a hit if it was a near miss
  */
simulated function ImpactInfo InstantAimHelp(vector StartTrace, vector EndTrace, ImpactInfo RealImpact)
{
	local ImpactInfo NearImpact;
	local Pawn ShotTarget;
	local UTPlayerController UTPC;
	local float AimHelpDist;
	local vector ClosestPoint;

	NearImpact.HitActor = None;
	UTPC = (Instigator != None) ? UTPlayerController(Instigator.Controller) : None;
	if ( (UTPC != None) && (UTPC.ShotTarget != None) && UTPC.AimingHelp(true) && (AimingHelpRadius[Min(CurrentFireMode,1)] > 0.0) )
	{
		ShotTarget = UTPC.ShotTarget;
		if ( RealImpact.HitActor != None )
		{
			EndTrace = RealImpact.HitLocation;
		}
		if ( ((EndTrace - ShotTarget.Location) Dot (ShotTarget.Location - StartTrace)) > 0 )
		{
			PointDistToLine(ShotTarget.Location, EndTrace - StartTrace, StartTrace, ClosestPoint);
			AimHelpDist = UTPC.AimHelpModifier() * AimingHelpRadius[Min(CurrentFireMode,1)];

			// reduce help if target isn't moving
			if ( ShotTarget.Velocity == vect(0,0,0) )
				AimHelpDist *= 0.5;

			// accept near miss if within AimHelpDist
			if ( (abs(ClosestPoint.Z - ShotTarget.Location.Z) < ShotTarget.CylinderComponent.CollisionHeight + AimHelpDist)
				&& (VSize2D(ClosestPoint - ShotTarget.Location) < ShotTarget.CylinderComponent.CollisionRadius + AimHelpDist) )
			{
				NearImpact.HitActor = ShotTarget;
				NearImpact.HitLocation = ClosestPoint;
				NearImpact.HitNormal = Normal(EndTrace - StartTrace);
			}
		}
	}
	return NearImpact;
}

/**
 * Fires a projectile.
 * Spawns the projectile, but also increment the flash count for remote client effects.
 * Network: Local Player and Server
 */
simulated function Projectile ProjectileFire()
{
	local vector		RealStartLoc;
	local Projectile	SpawnedProjectile;

	// tell remote clients that we fired, to trigger effects
	IncrementFlashCount();

	if( Role == ROLE_Authority )
	{
		// this is the location where the projectile is spawned.
		RealStartLoc = GetPhysicalFireStartLoc();

		// Spawn projectile
		SpawnedProjectile = Spawn(GetProjectileClass(),,, RealStartLoc);
		if( SpawnedProjectile != None && !SpawnedProjectile.bDeleteMe )
		{
			SpawnedProjectile.Init( Vector(GetAdjustedAim( RealStartLoc )) );
			if ( UTProjectile(SpawnedProjectile) != none )
			{
				UTProjectile(SpawnedProjectile).InitStats(self);
			}
		}
		UpdateFiredStats(1);

		// Return it up the line
		return SpawnedProjectile;
	}

	return None;
}

simulated function CustomFire()
{
	UpdateFiredStats(1);
	Super.CustomFire();
}

simulated function ProcessInstantHit( byte FiringMode, ImpactInfo Impact )
{
	local bool bFixMomentum;

	if (Impact.HitActor != None && !Impact.HitActor.bStatic)
	{
		if ( (UTPawn(Impact.HitActor) == None) && (InstantHitMomentum[FiringMode] == 0) )
		{
			InstantHitMomentum[FiringMode] = 1;
			bFixMomentum = true;
		}
		if (Role == ROLE_Authority && (Pawn(Impact.HitActor) != None || Objective(Impact.HitActor) != None))
		{
			UpdateHitStats(true);
		}
		Super.ProcessInstantHit(FiringMode, Impact);
		if (bFixMomentum)
		{
			InstantHitMomentum[FiringMode] = 0;
		}
	}
}

/**
 * Update the stats for this weapon
 */

function UpdateFiredStats(int NoShots)
{
	local int i;
	local UTGame GI;

	GI = UTGame(WorldInfo.Game);
	if (GI != none && GI.GameStats != none && WeaponStatsID >=0)
	{
		for (i=0;i<NoShots;i++)
		{
			GI.GameStats.WeaponEvent(OwnerStatsID, WeaponStatsID, CurrentFireMode, Instigator.PlayerReplicationInfo,'fired');
		}
	}
}

function UpdateHitStats(bool bDirectHit)
{
	local UTGame GI;

	GI = UTGame(WorldInfo.Game);
	if (GI != none && GI.GameStats != none && WeaponStatsID >=0)
	{
		if (bDirectHit)
		{
			GI.GameStats.WeaponEvent(OwnerStatsID, WeaponStatsID, CurrentFireMode, Instigator.PlayerReplicationInfo,'directhit');
		}
		else
		{
			GI.GameStats.WeaponEvent(OwnerStatsID, WeaponStatsID, CurrentFireMode, Instigator.PlayerReplicationInfo,'hit');
		}
	}
}

/*********************************************************************************************
 * Zooming Functions
 *********************************************************************************************/

/**
 * Returns true if we are currently zoomed
 */

simulated function EZoomState GetZoomedState()
{
	local PlayerController PC;
	PC = PlayerController(Instigator.Controller);
	if ( PC != none && PC.FOVAngle != PC.DefaultFOV )
	{
		if ( PC.FOVAngle == PC.DesiredFOV )
		{
			return ZST_Zoomed;
		}

		return ( PC.FOVAngle < PC.DesiredFOV ) ? ZST_ZoomingOut : ZST_ZoomingIn;
	}
	return ZST_NotZoomed;
}

/**
 * We Override beginfire to add support for zooming.  Should only be called from BeginFire()
 *
 * @param	FireModeNum 	The current Firing Mode
 *
 * @returns true we should abort the BeginFire call
 */
simulated function bool CheckZoom(byte FireModeNum)
{
	local UTPlayerController PC;
	PC = UTPlayerController(Instigator.Controller);
	if (PC != None && LocalPlayer(PC.Player) != none && FireModeNum < bZoomedFireMode.Length && bZoomedFireMode[FireModeNum] != 0)
	{
		if (GetZoomedState() == ZST_Zoomed)
		{
			EndZoom(PC);
			EndFire(FireModeNum);		// Kill this fire command
			return true;
		}
		else if ( GetZoomedState() == ZST_NotZoomed )
		{
			StartZoom(PC);
			ZoomedFireModeNum = FireModeNum;
		}
	}

	return false;
}

/** Called when zooming starts
 * @param PC - cast of Instigator.Controller for convenience
 */
simulated function StartZoom(UTPlayerController PC)
{
	PC.StartZoom(ZoomedTargetFOV, ZoomedRate);
	PlaySound(ZoomInSound, true);
}

/** Called when zooming ends
 * @param PC - cast of Instigator.Controller for convenience
 */
simulated function EndZoom(UTPlayerController PC)
{
	PC.EndZoom();
	PlaySound(ZoomOutSound, true);
}


client reliable simulated function ClientEndFire(byte FireModeNum)
{
	if (Role != ROLE_Authority)
	{
		ClearPendingFire(FireModeNum);
		EndFire(FireModeNum);
	}
}

/**
 * We Override endfire to add support for zooming
 */

simulated function EndFire(Byte FireModeNum)
{
	local UTPlayerController PC;

	// Don't bother performing if this is a dedicated server

	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		PC = UTPlayerController(Instigator.Controller);
		if (PC != None && LocalPlayer(PC.Player) != none && FireModeNum < bZoomedFireMode.Length && bZoomedFireMode[FireModeNum] != 0 )
		{
			PC.StopZoom();
		}
	}
	super.EndFire(FireModeNum);
}

/**
 * Don't send a zoomed fire mode in to a firing state
 */
simulated function SendToFiringState( byte FireModeNum )
{

	if (bDebugWeapon)
	{
		`log("---"@self$"."$GetStateName()$".SendToFiringState()");
		ScriptTrace();
	}

	// Don't send if it's a zoomed firemode

	if (FireModeNum < bZoomedFireMode.Length && bZoomedFireMode[FireModeNum] != 0 )
	{
		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".SendToFiringState() Quick Exit");
		}
		return;
	}

	super.SendToFiringState(FireModeNum);
}

reliable client function ClientWeaponSet( bool bOptionalSet )
{
	local PlayerController PC;

	if (bDebugWeapon)
	{
		`log("---"@self$"."$GetStateName()$".ClientWeaponSet()");
	}

	if (Instigator != None)
	{
		PC = PlayerController(Instigator.Controller);
		if ( PC != None && LocalPlayer(PC.Player) != none )
		{
			PC.FOVAngle = PC.DefaultFOV;
		}
	}
	Super.ClientWeaponSet(bOptionalSet);
}

/**
 * Deactiveate Spawn Protection and cancel any lock (if applicable)
 */

simulated function FireAmmunition()
{
	if (CurrentFireMode >= bZoomedFireMode.Length || bZoomedFireMode[CurrentFireMode] == 0)
	{
		Super.FireAmmunition();

		if (UTPawn(Instigator) != None)
		{
			UTPawn(Instigator).DeactivateSpawnProtection();
		}

		if (ROLE==ROLE_Authority && WeaponLockType == WEAPLOCK_Default)
		{
			AdjustLockTarget( none );
		}

		InvManager.OwnerEvent('FiredWeapon');
	}
}


/*********************************************************************************************
 * state Inactive
 * This state is the default state.  It needs to make sure Zooming is reset when entering/leaving
 *********************************************************************************************/

auto simulated state Inactive
{
	simulated function BeginState(name PreviousStateName)
	{
		local PlayerController PC;

		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".BeginState("$PreviousStateName$")");
		}


		if ( Instigator != None )
		{
		  PC = PlayerController(Instigator.Controller);
		  if ( PC != None && LocalPlayer(PC.Player)!= none )
		  {
			  PC.FOVAngle = PC.DefaultFOV;
		  }
		}

		Super.BeginState(PreviousStateName);

		if (Role == ROLE_Authority)
		{
			ClearTimer('CheckTargetLock');
			AdjustLockTarget(None);
		}
	}

	simulated function EndState(Name NextStateName)
	{
		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".EndState("$NextStateName$")");
		}

		Super.EndState(NextStateName);

		if (Role == ROLE_Authority && WeaponLockType != WEAPLOCK_None)
		{
			CheckTargetLock();
			SetTimer( LockCheckTime, true, 'CheckTargetLock' );
		}
	}

	/**
	 * @returns false if the weapon isn't ready to be fired.  For example, if it's in the Inactive/WeaponPuttingDown states.
	 */
	simulated function bool bReadyToFire()
	{
		return false;
	}

}


/*********************************************************************************************
 * State WeaponFiring
 * This is the default Firing State.  It's performed on both the client and the server.
 *********************************************************************************************/
simulated state WeaponFiring
{
	simulated function EndState(Name NextStateName)
	{
		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".EndState("$NextStateName$")");
		}
		super.EndState(NextStateName);
	}

	simulated function BeginState(Name PrevStateName)
	{
		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".EndState("$PrevStateName$")");
		}
		super.BeginState(PrevStateName);
	}

	simulated event ReplicatedEvent(name VarName)
	{
		if ( VarName == 'AmmoCount' && !HasAnyAmmo() )
		{
			return;
		}

		Global.ReplicatedEvent(VarName);

	}

	/**
	 * We override BeginFire() so that we can check for zooming and/or empty weapons
	 */

	simulated function BeginFire( Byte FireModeNum )
	{
		if ( CheckZoom(FireModeNum) )
		{
			return;
		}

		Global.BeginFire(FireModeNum);

		// No Ammo, then do a quick exit.
		if( !HasAmmo(FireModeNum) )
		{
			WeaponEmpty();
			return;
		}
	}

	/**
	 * When we are in the firing state, don't allow for a pickup to switch the weapon
	 */

	simulated function bool DenyClientWeaponSet()
	{
		return true;
	}


}

/*********************************************************************************************
 * state WeaponEquipping
 * This state is entered when a weapon is becomeing active (ie: Being brought up)
 *********************************************************************************************/

simulated state WeaponEquipping
{
	/**
	 * We want to being this state by setting up the timing and then notifying the pawn
	 * that the weapon has changed.
	 */

	simulated function BeginState(Name PreviousStateName)
	{
		local rotator r;

		super.BeginState(PreviousStateName);

		// Notify the pawn that it's weapon has changed.
		//SetupArmsAnim();

		if (Instigator.IsLocallyControlled() && UTPawn(Instigator)!=None)
		{
			UTPawn(Instigator).WeaponChanged(self);
		}

		/** TEMP - Remove me when all weapons are skeletal and have anims */
		if (SkeletalMeshComponent(Mesh) == none || WeaponEquipAnim == '')
		{
			PitchOffset = -32767;
			LastSPTime = WorldInfo.TimeSeconds;
			R = Mesh.Rotation;
			R.Pitch += PitchOffset;
			Mesh.SetRotation(R);
		}
	}

	simulated function bool TryPutDown()
	{
		// We want the abort to be the same amount of time as
		// we have already spent equipping

		SwitchAbortTime = PutDownTime * GetTimerCount('WeaponEquipped') / GetTimerRate('WeaponEquipped');
		GotoState('WeaponAbortEquip');

		return true;
	}

	simulated function EndState(Name NextStateName)
	{
		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".EndState("$NextStateName$")");
		}


		if (SkeletalMeshComponent(Mesh) == none || WeaponEquipAnim == '')
		{
			Mesh.SetRotation(Default.Mesh.Rotation);
		}
		ClearTimer('WeaponEquipped');
	}

	/**
	 * TEMP - Remove me when all weapons are skeletal and have anims
	 */
	simulated event SetPosition(UTPawn Holder)
	{
		local float DeltaTime;
		local rotator r;

		Global.SetPosition(Holder);

		if (SkeletalMeshComponent(Mesh) == none || WeaponEquipAnim == '')
		{
			R = Default.Mesh.Rotation;
			if ( PitchOffset<0 )
			{
				DeltaTime = WorldInfo.TimeSeconds - LastSPTime;
				PitchOffset = PitchOffset + ( (32767.0 * DeltaTime) / EquipTime);
				R.Pitch += INT(PitchOffset);
				LastSPTime = WorldInfo.TimeSeconds;
			}
			Mesh.SetRotation(R);
		}

	}

}

simulated state WeaponAbortEquip
{
	simulated function BeginState(name PrevStateName)
	{
		local AnimNodeSequence AnimNode;
		local float Rate;

		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".EndState("$PrevStateName$")");
		}


		// Time the abort
		SetTimer(FMax(SwitchAbortTime, 0.01),, 'WeaponEquipAborted');

		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			// play anim
			if (WeaponEquipAnim != '')
			{
				AnimNode = GetWeaponAnimNodeSeq();
				if (AnimNode != None && AnimNode.AnimSeq != None)
				{
					AnimNode.SetAnim(WeaponPutDownAnim);
					Rate = AnimNode.AnimSeq.SequenceLength / PutDownTime;
					AnimNode.PlayAnim(false, Rate, AnimNode.AnimSeq.SequenceLength - SwitchAbortTime * Rate);
				}

			}
			if(ArmsEquipAnim != '' && ArmsAnimSet != none && Instigator != none /* && Instigator.IsLocallyControlled() && Instigator.IsHumanControlled() */)
			{
				AnimNode = GetArmAnimNodeSeq();
				if (AnimNode != None && AnimNode.AnimSeq != None)
				{
					AnimNode.SetAnim(ArmsPutDownAnim);
					Rate = AnimNode.AnimSeq.SequenceLength/PutDownTime;
					AnimNode.PlayAnim(false, Rate, AnimNode.AnimSeq.SequenceLength - SwitchAbortTime * Rate);
				}
			}
		}
	}

	simulated function WeaponEquipAborted()
	{
		// This weapon is down, remove it from the mesh
		DetachWeapon();

		// Put weapon to sleep
		//@warning: must be before ChangedWeapon() because that can reactivate this weapon in some cases
		GotoState('Inactive');

		// switch to pending weapon
		InvManager.ChangedWeapon();
	}

	simulated function EndState(Name NextStateName)
	{
		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".EndState("$NextStateName$")");
		}

		ClearTimer('WeaponEquipAborted');
		Super.EndState(NextStateName);
	}
}

/**
  * Force streamed textures to be loaded.  Used to get MIPS streamed in before weapon comes up
  */
simulated function PreloadTextures(bool bForcePreload)
{
	if ( UTSkeletalMeshComponent(Mesh) != None )
	{
		UTSkeletalMeshComponent(Mesh).PreloadTextures(bForcePreload, WorldInfo.TimeSeconds + 2);
	}
}

/** called on both Instigator's current weapon and its pending weapon (if they exist)
 * @return whether Instigator is allowed to switch to NewWeapon
 */
simulated function bool AllowSwitchTo(Weapon NewWeapon)
{
	return true;
}

/**
 * When attempting to put the weapon down, look to see if our MinReloadPct has been met.  If so just put it down
 */
simulated function bool TryPutDown()
{
	local float MinTimerTarget;
	local float TimerCount;

	bWeaponPutDown = true;

	MinTimerTarget = GetTimerRate('RefireCheckTimer') * MinReloadPct[CurrentFireMode];
	TimerCount = GetTimerCount('RefireCheckTimer');

	if (bDebugWeapon)
	{
		`log("---"@self$"."$GetStateName()$".TryPutDown"@TimerCount@MinTimerTarget);
		ScriptTrace();
	}


	if (TimerCount > MinTimerTarget)
	{
		PutDownWeapon();
	}
	else
	{
		// Shorten the wait time
		SetTimer(TimerCount + (MinTimerTarget - TimerCount), false, 'RefireCheckTimer');
	}

	return true;
}

simulated function Vector GetPhysicalFireStartLoc(optional vector AimDir)
{
	local UTPlayerController PC;

	if( Instigator != none )
	{
		PC = UTPlayerController(Instigator.Controller);

		if ( PC!=none && PC.bCenteredWeaponFire )
		{
			return Instigator.GetPawnViewLocation() + (vector(Instigator.GetViewRotation()) * FireOffset.X);
		}
		else
		{
			return Instigator.GetPawnViewLocation() + (FireOffset >> Instigator.GetViewRotation());
		}
	}

	return Location;
}

/**
 * Returns the location + offset from which to spawn projectiles/effects/etc.
 */
simulated function vector GetEffectLocation()
{
	local vector SocketLocation;
	local Rotator SocketRotation;

	if (SkeletalMeshComponent(Mesh)!=none && EffectSockets[CurrentFireMode]!='')
	{
		if (!SkeletalMeshComponent(Mesh).GetSocketWorldLocationAndrotation(EffectSockets[CurrentFireMode], SocketLocation, SocketRotation))
			SocketLocation = Location;
	}
	else if (Mesh!=none)
		SocketLocation = Mesh.Bounds.Origin + (vect(45,0,0) >> Rotation);
	else
		SocketLocation = Location;

 	return SocketLocation;
}


simulated function RefireCheckTimer();

simulated function SetupArmsAnim()
{
	local UTPawn UTP;
	UTP = UTPawn(Instigator);
	if (UTP != None)
	{
		UTP.ArmsMesh[0].StopAnim(); // let's stop anything already going.
		if (ArmsAnimSet != None)
		{
			UTP.ArmsMesh[0].AnimSets[1] = ArmsAnimSet;
			UTP.ArmsMesh[0].SetHidden(false);
			UTP.ArmsMesh[0].SetLightEnvironment(UTP.LightEnvironment);
			if (UTP.ArmsOverlay[0] != None)
			{
				UTP.ArmsOverlay[0].SetHidden(false);
			}
		}
		else
		{
			UTP.ArmsMesh[0].SetHidden(true);
			UTP.ArmsMesh[1].SetHidden(true);
			if (UTP.ArmsOverlay[0] != None)
			{
				UTP.ArmsOverlay[0].SetHidden(true);
				UTP.ArmsOverlay[1].SetHidden(true);
			}
		}
	}
}
simulated state WeaponPuttingDown
{
	simulated function BeginState( Name PreviousStateName )
	{
		local UTPlayerController PC;

		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".BeginState("$PreviousStateName$")");
		}


		PC = UTPlayerController(Instigator.Controller);
		if (PC != None && LocalPlayer(PC.Player) != none )
		{
			PC.EndZoom();
		}

		TimeWeaponPutDown();
		bWeaponPutDown = false;

		/** TEMP - Remove me when all weapons are skeletal and have anims */
		if (SkeletalMeshComponent(Mesh) == none || WeaponEquipAnim == '')
		{
			PitchOffset = 0;
			LastSPTime = WorldInfo.TimeSeconds;
		}


	}

	simulated function EndState(Name NextStateName)
	{
		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".BeginState("$NextStateName$")");
		}


		Super.EndState(NextStateName);
		if (SkeletalMeshComponent(Mesh) == none || WeaponEquipAnim == '')
		{
			Mesh.SetRotation(Default.Mesh.Rotation);
		}
	}

	simulated function Activate()
	{
		// We want the abort to be the same amount of time as
		// we have already spent putting down
		SwitchAbortTime = EquipTime * GetTimerCount('WeaponIsDown') / GetTimerRate('WeaponIsDown');
		SetupArmsAnim();
		GotoState('WeaponAbortPutDown');
	}

	/**
	 * @returns false if the weapon isn't ready to be fired.  For example, if it's in the Inactive/WeaponPuttingDown states.
	 */
	simulated function bool bReadyToFire()
	{
		return false;
	}

	/**
	 * TEMP - Remove me when all weapons are skeletal and have anims
	 */
	simulated event SetPosition(UTPawn Holder)
	{
		local float DeltaTime;
		local rotator r;

		Global.SetPosition(Holder);

		if (SkeletalMeshComponent(Mesh) == none || WeaponEquipAnim == '')
		{
			R = Default.Mesh.Rotation;
			DeltaTime = WorldInfo.TimeSeconds - LastSPTime;
			PitchOffset = PitchOffset + ( (32767.0 * DeltaTime) / EquipTime);
			R.Pitch -= INT(PitchOffset);
			LastSPTime = WorldInfo.TimeSeconds;
			Mesh.SetRotation(R);
		}

	}


}

simulated function AnimNodeSequence GetArmAnimNodeSeq()
{
	local UTPawn P;

	P = UTPawn(Instigator);
	if (P != None && P.ArmsMesh[0] != None)
	{
		return AnimNodeSequence(P.ArmsMesh[0].Animations);
	}

	return None;
}

simulated state WeaponAbortPutDown
{
	simulated function BeginState(name PrevStateName)
	{
		local AnimNodeSequence AnimNode;
		local float Rate;

		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".BeginState("$PrevStateName$")");
		}


		// Time the abort
		SetTimer(FMax(SwitchAbortTime, 0.01),, 'WeaponPutDownAborted');
		// play anim
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			if (WeaponEquipAnim != '')
			{
				AnimNode = GetWeaponAnimNodeSeq();
				if (AnimNode != None && AnimNode.AnimSeq != None)
				{
					AnimNode.SetAnim(WeaponEquipAnim);
					Rate = AnimNode.AnimSeq.SequenceLength / EquipTime;
					AnimNode.PlayAnim(false, Rate, AnimNode.AnimSeq.SequenceLength - SwitchAbortTime * Rate);
				}
			}
			if(ArmsEquipAnim != '' && ArmsAnimSet != none && Instigator != none /* && Instigator.IsLocallyControlled() && Instigator.IsHumanControlled() */)
			{
				AnimNode = GetArmAnimNodeSeq();
				if (AnimNode != None && AnimNode.AnimSeq != None)
				{
					AnimNode.SetAnim(ArmsEquipAnim);
					Rate = AnimNode.AnimSeq.SequenceLength/EquipTime;
					AnimNode.PlayAnim(false, Rate, AnimNode.AnimSeq.SequenceLength - SwitchAbortTime * Rate);
				}
			}
		}
	}

	simulated function WeaponPutDownAborted()
	{
		GotoState('Active');
	}

	simulated function Activate()
	{
		SetupArmsAnim();
	}

	simulated function bool bReadyToFire()
	{
		return false;
	}

	simulated function EndState(Name NextStateName)
	{
		if (bDebugWeapon)
		{
			`log("---"@self$"."$GetStateName()$".BeginState("$NextStateName$")");
		}
		ClearTimer('WeaponPutDownAborted');

		Super.EndState(NextStateName);
	}
}

function GivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	local UTGame GI;

	super(Inventory).GivenTo(NewOwner, bDoNotActivate);

	if (bDebugWeapon)
	{
		`log("---"@self$"."$GetStateName()$".GivenTo("$NewOwner@bDoNotActivate$")");
	}

	GI = UTGame(WorldInfo.Game);
	if (GI != none && GI.GameStats != none)
	{
		GI.GameStats.PickupWeaponEvent(Class,NewOwner.PlayerReplicationInfo, OwnerStatsID, WeaponStatsID);
	}
}

/*********************************************************************************************
 * Target Locking
 *********************************************************************************************/

/**
 * The function checks to see if we are locked on a target
 */
function CheckTargetLock()
{
	local Actor 		TA, BestTarget;
	local float 		BestDist, BestAim;
	local vector		StartTrace, EndTrace, Aim, TargetDir;
	local ImpactInfo	Impact;
	local UTVehicle V;
	local UTPlayerController UTPC;

	if (Instigator == None || Instigator.Controller == None)
	{
		return;
	}

	if (Instigator.bNoWeaponFiring)
	{
		AdjustLockTarget(None);
		PendingLockedTarget = None;
		return;
	}

	// Begin by tracing the shot to see if it hits anyone
	StartTrace = Instigator.GetWeaponStartTraceLocation();
	Aim = vector(GetAdjustedAim(StartTrace));
	EndTrace = StartTrace + Aim * LockRange;
	Impact = CalcWeaponFire(StartTrace, EndTrace);

	// Check for a hit
	if ( Impact.HitActor == none || !CanLockOnTo(Impact.HitActor)  )
	{
		// We didn't hit a valid target, have the controller attempt to pick a good target
		UTPC = UTPlayerController(Instigator.Controller);
		BestAim	= ((UTPC != None) && UTPC.AimingHelp(true)) ? ConsoleLockAim : LockAim;
		TA = Instigator.Controller.PickTarget(LockablePawnClass, BestAim, BestDist, Aim, StartTrace, LockRange);
		if (TA != None && CanLockOnTo(TA))
		{
			BestTarget = TA;
		}
		else if (UTGame(WorldInfo.Game) != None)
		{
			// ask vehicles if they have any other targets we should consider
			for (V = UTGame(WorldInfo.Game).VehicleList; V != None; V = V.NextVehicle)
			{
				TA = V.GetAlternateLockTarget();
				if (TA != None)
				{
					TargetDir = TA.Location - StartTrace;
					if (VSize(TargetDir) < LockRange && (Normal(TargetDir) dot Aim) > LockAim && CanLockOnTo(V))
					{
						BestTarget = TA;
					}
				}
			}
		}
	}
	else	// We hit a valid target
	{
		BestTarget = Impact.HitActor;
	}

	// If we have a "possible" target, note its time mark
	if (BestTarget != none)
	{
		LastValidTargetTime = WorldInfo.TimeSeconds;
	}

	// Attempt to invalidate the current locked Target
	if ( LockedTarget != none &&
 	    ( WorldInfo.TimeSeconds - LastLockedOnTime > LockTolerance  ||
	  LockedTarget.bDeleteMe ||
 	   	  (Pawn(LockedTarget) != none && Pawn(LockedTarget).Health <=0)) )
	{
		AdjustLockTarget(none);
	}

	// Next attempt to invalidate the Pending Target
	if ( PendingLockedTarget != none && ( WorldInfo.TimeSeconds - LastValidTargetTime > LockTolerance || PendingLockedTarget.bDeleteMe ) )
	{
		PendingLockedTarget = none;
	}

	// We have our best target, see if they should become our current target.
	if (BestTarget == LockedTarget)	// We are already locked on to the right guy
	{
		LastLockedOnTime = WorldInfo.TimeSeconds;
	}
	else if (BestTarget != none)	// We have a possible target, track it
	{
		// Check for a new Pending Lock
		if (PendingLockedTarget != BestTarget)
		{
			PendingLockedTarget = BestTarget;
			PendingLockedTargetTime = WorldInfo.TimeSeconds + LockAcquireTime;
		}

		// Otherwise check to see if we have been tracking the pending lock long enough
		else if (PendingLockedTarget == BestTarget && WorldInfo.TimeSeconds >= PendingLockedTargetTime )
		{
			AdjustLockTarget(PendingLockedTarget);
			LastLockedOnTime 	= WorldInfo.TimeSeconds;
			PendingLockedTarget = None;
			PendingLockedTargetTime = 0;
		}
	}
}


/**
 *  This function is used to adjust the LockTarget.
 */
function AdjustLockTarget(actor NewLockTarget)
{
	if (NewLockTarget == None)
	{
		// Clear the lock
		if (bLockedOnTarget)
		{
			LockedTarget = None;

			bLockedOnTarget = false;

			if (LockLostSound != None && Instigator != None && Instigator.IsHumanControlled() )
			{
				PlayerController(Instigator.Controller).ClientPlaySound(LockLostSound);
			}
		}
	}
	else
	{
		// Set the lcok
		bLockedOnTarget = true;
		LockedTarget = NewLockTarget;
		if ( LockAcquiredSound != None && Instigator != None  && Instigator.IsHumanControlled() )
		{
			PlayerController(Instigator.Controller).ClientPlaySound(LockAcquiredSound);
		}
	}
}

/**
 * Given an actor (TA) determine if we can lock on to it.  By default only allow locking on
 * to pawns.  Some weapons may want to be able to lock on to other actors.
 */
function bool CanLockOnTo(Actor TA)
{
	if ( !TA.bProjTarget || TA.bDeleteMe || Pawn(TA) == none || Pawn(TA) == Instigator || Pawn(TA).Health <= 0 || !ClassIsChildOf(TA.Class,LockablePawnClass) )
	{
		return false;
	}

 	if (!WorldInfo.Game.bTeamGame)
 		return true;

 	return  (!WorldInfo.GRI.OnSameTeam(Instigator,TA) );

}

simulated event Destroyed()
{
	if (Instigator != None && Instigator.IsHumanControlled() && Instigator.IsLocallyControlled())
	{
		PreloadTextures(false);
	}
	AdjustLockTarget(none);
	super.Destroyed();
}

simulated state Active
{
	/**
	 * We override BeginFire() so that we can check for zooming
	 */
	simulated function BeginFire( Byte FireModeNum )
	{
		if (WorldInfo.NetMode != NM_DedicatedServer)
		{
			if ( CheckZoom(FireModeNum) )
			{
				return;
			}
		}
		Super.BeginFire(FireModeNum);
	}

	simulated event OnAnimEnd(optional AnimNodeSequence SeqNode, optional float PlayedTime, optional float ExcessTime)
	{
		local int IdleIndex;
		local AnimSet ASet;
		local SkeletalMeshComponent SkelMesh;
		local AnimSequence AS;
		local float length;
		local int i, j;
		if ( WorldInfo.NetMode != nm_DedicatedServer && WeaponIdleAnims.Length > 0 )
		{
			length = 0.0f;
			SkelMesh = SkeletalMeshComponent(Mesh);
			IdleIndex = Rand(WeaponIdleAnims.Length); //WeaponIdleAnims.Length * frand();
			if(SkelMesh != none)
			{
				for(i=0;i<SkelMesh.AnimSets.Length;++i)
				{
					ASet = SkelMesh.AnimSets[i];
					if(ASet != none)
					{
						for(j=0;j<ASet.Sequences.Length;++j)
						{
							AS = ASet.Sequences[j];
							if(AS != none && AS.SequenceName == WeaponIdleAnims[IdleIndex])
							{
								length = AS.SequenceLength*AS.RateScale;
								break;
							}
						}
					}
					else
					{
						length = 1.0f;
						break;
					}
					if(length  != 0.0f)
					{
						break;
					}
				}

			}
			else
			{
				length = 1.0f;
			}

			PlayWeaponAnimation(WeaponIdleAnims[IdleIndex], length);
			if(ArmIdleAnims.Length > IdleIndex && ArmsAnimSet != none)
			{
				PlayArmAnimation(ArmIdleAnims[IdleIndex], length);
			}
		}
	}
	simulated function PlayWeaponAnimation( Name Sequence, float fDesiredDuration, optional bool bLoop, optional SkeletalMeshComponent SkelMesh)
	{
		Global.PlayWeaponAnimation(Sequence,fDesiredDuration,bLoop,SkelMesh);
		ClearTimer('OnAnimEnd');
		if (!bLoop)
		{
			SetTimer(fDesiredDuration,false,'OnAnimEnd');
		}
	}

	simulated function bool ShouldLagRot()
	{
		return true;
	}

	reliable server function ServerStartFire( byte FireModeNum )
	{
		// Check to see if the weapon is active, but not the current weapon.  If it is, force the
		// client to reset
		if (Instigator != none && Instigator.Weapon != self)
		{
			`Log("########## WARNING: Server Received ServerStartFire on "$self$" while in the active state but not current weapon.  Attempting Realignment");
			`log("##########        : "$Instigator.PlayerReplicationInfo.PlayerName);
			`log("##########        : "$Instigator.Weapon@Instigator.Weapon.GetStateName());
			InvManager.ClientSyncWeapon(Instigator.Weapon);
			Global.ServerStartFire(FireModeNum);
		}
		else
		{
			Global.ServerStartFire(FireModeNum);
		}
	}

	simulated function EndState(Name NextStateName)
	{
		Super.EndState(NextStateName);
		if (bDebugWeapon)
		{
			`log("---"@self@"has left the Active State"@NextStateName);
		}
	}

	/** Initialize the weapon as being active and ready to go. */
	simulated function BeginState( Name PreviousStateName )
	{
		OnAnimEnd(none, 0.f, 0.f);

		if (bDebugWeapon)
		{
			`log("---"@self@"has entered the Active State"@PreviousStateName);
		}

		Super.BeginState(PreviousStateName);

		if (InvManager != none && InvManager.LastAttemptedSwitchToWeapon != none)
		{
			InvManager.LastAttemptedSwitchToWeapon.ClientWeaponSet( true );
			InvManager.LastAttemptedSwitchToWeapon = none;
		}
	}
}

simulated function SetWeaponOverlayFlags(UTPawn OwnerPawn)
{
	local MaterialInterface InstanceToUse;
	local byte Flags;
	local int i;
	local UTGameReplicationInfo GRI;

	if(OwnerPawn != none)
	{
		GRI = UTGameReplicationInfo(WorldInfo.GRI);
		if (GRI != None)
		{
			Flags = OwnerPawn.WeaponOverlayFlags;
			for (i = 0; i < GRI.WeaponOverlays.length; i++)
			{
				if (GRI.WeaponOverlays[i] != None && bool(Flags & (1 << i)))
				{
					InstanceToUse = GRI.WeaponOverlays[i];
					break;
				}
			}
		}
		if ( OverlayMesh == None )
			CreateOverlayMesh();
		if ( OverlayMesh != None )
		{
			if (InstanceToUse != none)
			{
				for (i=0;i<OverlayMesh.GetNumElements(); i++)
				{
					OverlayMesh.SetMaterial(i, InstanceToUse);
				}

				if (!OverlayMesh.bAttached)
				{
					OverlayMesh.SetHidden(Mesh.HiddenGame);
					AttachComponent(OverlayMesh);
				}
			}
			else if ( OverlayMesh.bAttached )
			{
				DetachComponent(OverlayMesh);
				OverlayMesh.SetHidden(true);
			}
		}
	}
}

static function float DetourWeight(Pawn Other, float PathWeight)
{
	local UTBot B;

	B = UTBot(Other.Controller);
	if (B != None && B.NeedWeapon() && Other.FindInventoryType(default.Class) == None)
	{
		return (default.MaxDesireability / PathWeight);
	}
	else
	{
		return 0.0;
	}
}

/**
 * Allow the weapon to adjust the turning speed of the pawn
 * @FIXME: Add support for validation on a server
 *
 * @param	aTurn		The aTurn value from PlayerInput to throttle
 * @param	aLookup		The aLookup value from playerInput to throttle
 */
simulated function ThrottleLook(out float aTurn, out float aLookup);

simulated function Activate()
{
//	`log("###"@self$"."$GetOwnerName()@".Activate() I="$Instigator@"M="$Mesh, bDebugWeapon);
	SetupArmsAnim();
	super.Activate();
}

simulated function string GetOwnerName()
{
	return (Instigator==none) ? "None" : Instigator.PlayerReplicationInfo.PlayerName;
}

/**
 * This function is meant to be overridden in children.  It turns the current power percentage for
 * a weapon.  It's called mostly from the hud
 *
 * @returns	the percentage of power ( 1.0 - 0.0 )
 */
simulated event float GetPowerPerc()
{
	return 0.0;
}

function DropFrom(vector StartLocation, vector StartVelocity)
{
//	local name StartState;

	if (Instigator != None && Instigator.IsHumanControlled() && Instigator.IsLocallyControlled())
	{
		PreloadTextures(false);
	}
//	StartState = GetStateName();

	Super.DropFrom(StartLocation, StartVelocity);

	//`log("---"@self$".DropFrom() SS:"@StartState@" ES:"@GetStateName());
}

reliable client function ClientWeaponThrown()
{
//	local name StartState;

//	StartState = GetStateName();

	if ( Instigator != none && UTPlayerController(Instigator.Controller) != none )
	{
		UTPlayerController(Instigator.Controller).EndZoom();
	}

	Super.ClientWeaponThrown();

	//`log("---"@self$".ClientWeaponThrown() SS:"@StartState@" ES:"@GetStateName());
}

/** called when Instigator enters a vehicle while we are its Weapon */
simulated function HolderEnteredVehicle();

simulated function bool CoversScreenSpace(vector ScreenLoc, Canvas Canvas)
{
	return ( (ScreenLoc.X > (1-WeaponCanvasXPct)*Canvas.ClipX)
		&& (ScreenLoc.Y > (1-WeaponCanvasYPct)*Canvas.ClipY) );
}

simulated static function DrawKillIcon(Canvas Canvas, float ScreenX, float ScreenY, float HUDScaleX, float HUDScaleY)
{
	local color CanvasColor;

	// save current canvas color
	CanvasColor = Canvas.DrawColor;

	// draw weapon shadow
	Canvas.DrawColor = class'UTHUD'.default.BlackColor;
	Canvas.DrawColor.A = CanvasColor.A;
	Canvas.SetPos( ScreenX - 2, ScreenY - 2 );
	Canvas.DrawTile(class'UTHUD'.default.AltHudTexture, 4 + HUDScaleX * 96, 4 + HUDScaleY * 64, default.IconCoordinates.U, default.IconCoordinates.V, default.IconCoordinates.UL, default.IconCoordinates.VL);

	// draw the weapon icon
	Canvas.DrawColor =  class'UTHUD'.default.WhiteColor;
	Canvas.DrawColor.A = CanvasColor.A;
	Canvas.SetPos( ScreenX, ScreenY );
	Canvas.DrawTile(class'UTHUD'.default.AltHudTexture, HUDScaleX * 96, HUDScaleY * 64, default.IconCoordinates.U, default.IconCoordinates.V, default.IconCoordinates.UL, default.IconCoordinates.VL);
	Canvas.DrawColor = CanvasColor;
}

defaultproperties
{
	Begin Object Class=UTSkeletalMeshComponent Name=FirstPersonMesh
		DepthPriorityGroup=SDPG_Foreground
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bOnlyOwnerSee=true
		bOverrideAttachmentOwnerVisibility=true
		CastShadow=false
	End Object
	Mesh=FirstPersonMesh
	//Components.Add(FirstPersonMesh)

	Begin Object Class=SkeletalMeshComponent Name=PickupMesh
		bOnlyOwnerSee=false
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		CollideActors=false
		BlockRigidBody=false
		bUseAsOccluder=false
		CullDistance=6000
		bForceRefPose=true
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bAcceptsDecals=false
	End Object
	DroppedPickupMesh=PickupMesh
	PickupFactoryMesh=PickupMesh

	MessageClass=class'UTPickupMessage'
	DroppedPickupClass=class'UTDroppedPickup'

	ShotCost(0)=1
	ShotCost(1)=1

	MaxAmmoCount=1

	FiringStatesArray(0)=WeaponFiring
	FiringStatesArray(1)=WeaponFiring

	WeaponFireTypes(0)=EWFT_InstantHit
	WeaponFireTypes(1)=EWFT_InstantHit

	WeaponProjectiles(0)=none
	WeaponProjectiles(1)=none

	FireInterval(0)=+1.0
	FireInterval(1)=+1.0

	Spread(0)=0.0
	Spread(1)=0.0

	InstantHitDamage(0)=0.0
	InstantHitDamage(1)=0.0
	InstantHitMomentum(0)=0.0
	InstantHitMomentum(1)=0.0
	InstantHitDamageTypes(0)=class'DamageType'
	InstantHitDamageTypes(1)=class'DamageType'

	EffectSockets(0)=MuzzleFlashSocket
	EffectSockets(1)=MuzzleFlashSocket
	MuzzleFlashDuration=0.33

	WeaponFireSnd(0)=none
	WeaponFireSnd(1)=none

	MinReloadPct(0)=0.6
	MinReloadPct(1)=0.6

	MuzzleFlashSocket=MuzzleFlashSocket

	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0

	LockerRotation=(Pitch=16384)

	WeaponColor=(R=255,G=255,B=255,A=255)
	BobDamping=0.85000
	JumpDamping=1.0
	AimError=600
	CurrentRating=+0.5
	MaxDesireability=0.5

	WeaponFireAnim(0)=WeaponFire
	WeaponFireAnim(1)=WeaponFire
	ArmFireAnim(0)=WeaponFire
	ArmFireAnim(1)=WeaponFire

	WeaponPutDownAnim=WeaponPutDown
	ArmsPutDownAnim=WeaponPutDown
	WeaponEquipAnim=WeaponEquip
	ArmsEquipAnim=WeaponEquip
	WeaponStatsID=-1

	WeaponLockType=WEAPLOCK_None
	LockablePawnClass=class'UTGame.UTVehicle'

	IconX=458
	IconY=83
	IconWidth=31
	IconHeight=45

	EquipTime=+0.45
	PutDownTime=+0.33

	MaxPitchLag=600
	MaxYawLag=800
	RotChgSpeed=3.0
	ReturnChgSpeed=3.0
	AimingHelpRadius[0]=20.0
	AimingHelpRadius[1]=20.0

	bUseOverlayHack=true

//    CrosshairImage=Material'UI_HUD.CrossHair.M_UI_HUD_InvertingCrossHair_01'
	CrosshairImage=Texture2D'UI_HUD.HUD.UTCrossHairs'
	CrossHairCoordinates=(U=192,V=64,UL=64,VL=64)
	bScaleCrosshair=true
	IconCoordinates=(U=273,V=107,UL=84,VL=50)

 	ZoomInSound=SoundCue'A_Weapon.SniperRifle.Cue.A_Weapon_SN_ZoomIn01_Cue'
	ZoomOutSound=SoundCue'A_Weapon.SniperRifle.Cue.A_Weapon_SN_ZoomOut_Cue'

	WeaponCanvasXPct=0.35
	WeaponCanvasYPct=0.35

	QuickPickGroup=-1
	QuickPickWeight=0.0

	LockAim=0.996
	ConsoleLockAim=0.989
	LockRange=8000

	bExportMenuData=true
	LockerOffset=(X=0.0,Z=-15.0)

	bUsesOffhand=false;
}
