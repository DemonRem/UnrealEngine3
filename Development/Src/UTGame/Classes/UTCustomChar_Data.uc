/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 * This object is used as a store for all custom character part and profile information.
 */
class UTCustomChar_Data extends Object
	native
	config(CustomChar);

/** Enum defining different 'slots' for a custom character. */
enum ECharPart
{
	PART_Head,
	PART_Helmet,
	PART_Facemask,
	PART_Goggles,
	PART_Torso,
	PART_ShoPad,
	PART_Arms,
	PART_Thighs,
	PART_Boots
};

/** Structure defining a complete custom character, along with the faction and its name. */
struct native CustomCharData
{
	/** Used to find voice-pack etc, and as fallback character setup. */
	var string BasedOnCharID;

	/** This defines which 'set' of parts we are drawing from. */
	var string FamilyID;

	var string HeadID;
	var string HelmetID;
	var string FacemaskID;
	var string GogglesID;
	var string TorsoID;
	var string ShoPadID;
	var bool bHasLeftShoPad;
	var bool bHasRightShoPad;
	var string ArmsID;
	var string ThighsID;
	var string BootsID;
};

/** information about AI abilities/personality (generally map directly to UTBot properties) */
struct native CustomAIData
{
	var float Tactics, StrafingAbility, Accuracy, Aggressiveness, CombatStyle, Jumpiness, ReactionTime;
	/** full path to class of bot's favorite weapon */
	var string FavoriteWeapon;

	structdefaultproperties
	{
		Aggressiveness=0.4
		CombatStyle=0.2
	}
};

/** Structure defining a pre-made character in the game. */
struct native CharacterInfo
{
	/** Short unique string . */
	var string CharID;

	/** Friendly name for character. */
	var localized string CharName;

	/** Localized description of the character. */
	var localized string Description;

	/** Preview image markup for the character. */
	var string PreviewImageMarkup;

	/** Faction to which this character belongs (e.g. IronGuard). */
	var string Faction;
	/** What this character looks like. */
	var CustomCharData CharData;
	/** AI personality */
	var CustomAIData AIData;
	/** any extra properties of this character (for mod use) */
	var string ExtraInfo;
	/** whether this character shows up in menus, or is only usable by bots */
	var bool bExcludeFromMenus;

	// @TODO: VOICE PACK
};

/** Structure defining information about a particular faction (eg. Ironguard) */
struct native FactionInfo
{
	var string Faction;

	/** Preview image markup for the faction. */
	var string PreviewImageMarkup;

	/** Localized version of the faction name to display in the UI. */
	var localized string FriendlyName;

	/** Description of the faction. */
	var localized string Description;
};

/** Structure defining one part of a custom character. */
struct native CustomCharPart
{
	/** Which 'slot' this part is for. */
	var ECharPart	Part;

	/** Name of actual SkeletalMesh object to find for this part. */
	var string		ObjectName;

	/** Short ID used within the CustomCharData. */
	var string		PartID;

	/** 'Set' to which this part belongs. All parts of a CustomCharData belong to the same family. */
	var string		FamilyID;

	/** If true, do not show goggles when this part is equipped. */
	var bool		bNoGoggles;

	/** If true, do not show facemask when this part is equipped. */
	var bool		bNoFacemask;
};

struct native CustomCharMergeState
{
	var bool				bMergeInProgress;
	var bool				bUseKrallRules;

	var CustomCharData		CharData;
	var string				TeamString;
	var string				SkinString;

	/** If mesh is created and passed in - this is it - the one to place resulting merge mesh into. */
	var SkeletalMesh		UseMesh;

	// Diffuse, Normal, Specular, SpecPower, EmMask
	var Texture2DComposite	HeadTextures[5];
	var Texture2DComposite	BodyTextures[5];

	// Output MICs applied to head/body - Parent is FamilyInfo.BaseMICParent
	var MaterialInstanceConstant	DefaultHeadMIC;
	var MaterialInstanceConstant	DefaultBodyMIC;
};

/** Array of all parts, defined in UTCustomChar.ini file. */
var() config array<CustomCharPart>		Parts;

/** Aray of all complete character profiles, defined in UTCustomChar.ini file. */
var() config array<CharacterInfo>		Characters;

/** Array of top-level factions (eg Iron Guard). */
var() config array<FactionInfo>			Factions;

/** Array of info for each family (eg IRNM) */
var() array< class<UTFamilyInfo> >		Families;

var() config array<SourceTexture2DRegion>	HeadRegions; // head, eyes, teeth, stump, eyewear, facemask, helmet/hair
var() config array<SourceTexture2DRegion>	BodyRegions; // chest, thighs, shoulder pads, arms, boots

// Diffuse, Normal, Specular, SpecPower, EmMask
var() config int HeadMaxTexSize[5];
var() config int BodyMaxTexSize[5];

var() config int SelfHeadMaxTexSize[5];
var() config int SelfBodyMaxTexSize[5];

var() config float LOD1DisplayFactor;
var() config float LOD2DisplayFactor;

var() config float CustomCharTextureStreamTimeout;

var() config int MaxCustomChars;

/** Structure defining setup for capturing character portrait bitmap. */
struct native CharPortraitSetup
{
	/** Name of bone to center view on. */
	var name	CenterOnBone;

	/** Translation of mesh (applied on top of CenterOnBone alignment. */
	var vector	MeshOffset;

	/** Rotation of mesh. */
	var rotator	MeshRot;

	/** FOV of camera. */
	var	float	CamFOV;

	/** Directional light rotation. */
	var rotator	DirLightRot;

	/** Directional light brightness. */
	var	float	DirLightBrightness;

	/** Directional light color. */
	var color	DirLightColor;

	/** Skylight brightness. */
	var float	SkyBrightness;

	/** Sky light color */
	var color	SkyColor;

	/** Sky lower brightness */
	var float	SkyLowerBrightness;

	/** Sky lower colour */
	var color	SkyLowerColor;

	/** Background material. */
	var MaterialInterface BackgroundMaterial; 
};

/** Enum for specifying resolution for texture created for custom chars. */
enum CustomCharTextureRes
{
	CCTR_Normal,
	CCTR_Full,
	CCTR_Self
};

// Default (fallback) first-person arm mesh - for when character does not have a custom char. */
var string	DefaultArmMeshPackageName;
var string	DefaultArmMeshName;
var	string	DefaultArmSkinPackageName;
var	string	DefaultRedArmSkinName;
var	string	DefaultBlueArmSkinName;

// Default portrait config to use
var() config CharPortraitSetup	PortraitSetup;

/** Given a family, part and ID string, give the SkeletalMesh object name to use. */
static native final function string FindPartObjName(string InFamilyID, ECharPart InPart, string InPartID);

/** Given a family, part and ID string, find the SkeletalMesh object itself. */
static native final function SkeletalMesh FindPartSkelMesh(string InFamilyID, ECharPart InPart, string InPartID, bool bLeftShoPad, string InBasedOnCharID);

/** Given a faction and character ID, find the character that defines all its parts. */
static native final function CharacterInfo FindCharacter(string InFaction, string InCharID);

/** Find the info class for a particular family */
static native final function class<UTFamilyInfo> FindFamilyInfo(string InFamilyID);

/**
 *	This loads all assets associated with a custom character family (based on ini file) and create a
 *	UTCharFamilyAssetStore which is used to keep refs to all the required assets.
 *	@param bBlocking	If true, game will block until all assets are loaded.
 *	@param bArms		Load package containing arm mesh for this family
 */
static native final function UTCharFamilyAssetStore LoadFamilyAssets(string InFamilyID, bool bBlocking, bool bArms);

/**
 *	Given a complete character profile, combine all the individual parts into a new SkeletalMesh.
 *	New SkeletalMesh, Texture2Ds and MaterialInterface are created in the transient package.
 *	TeamString and SkinString are strings used to replace V01 and SK1 in the diffuse and spec texture names. This allows
 *	for team- and skin-specific versions of the material to be created.
 *	This function starts all the relevant textures streaming in.
 *	@param	UseMesh	If supplied, this mesh will be used for the result. If not, a new mesh in the transient package will be created.
 */
static native final function CustomCharMergeState StartCustomCharMerge(CustomCharData InCharData, string TeamString, string SkinString, SkeletalMesh UseMesh, CustomCharTextureRes TextureRes);

/**
 *	Must call StartCustomCharMerge before this function.
 *	If all necessary textures are streamed in, parts will be combined and a new SkeletalMesh will be returned, and the part textures allowed to stream out again.
 *	If textures are not streamed in yet, NULL will be returned.
 */
static native final function SkeletalMesh FinishCustomCharMerge(out CustomCharMergeState MergeState);

/** Call to abandon merging of parts - textures are set to unstream and MergeState is reset to defaults. */
static native final function ResetCustomCharMerge(out CustomCharMergeState MergeState);

/** 
 *	Util for creating a portrait texture for the supplied skeletal mesh. 
 */
static native final function texture MakeCharPortraitTexture(SkeletalMesh CharMesh, CharPortraitSetup Setup);

/** Utility for creating a random player character. */
static native final function CustomCharData MakeRandomCharData();

/** Utility for converting custom char data into one string */
static native final function string CharDataToString(CustomCharData InCharData);

/** Utility for converting a string into a custom char data. */
static native final function CustomCharData CharDataFromString(string InString);

/** Saves the character data to an INI file. */
//static native function TempSaveDataToINI(string Data);

/** Loads the character data from an INI file. */
//static native function bool TempLoadDataFromINI(out string OutData);

defaultproperties
{
	Families.Add(class'UTFamilyInfo_Ironguard_Female')
	Families.Add(class'UTFamilyInfo_Ironguard_Male')
	Families.Add(class'UTFamilyInfo_Krall_Male')
	Families.Add(class'UTFamilyInfo_Liandri_Male')
	Families.Add(class'UTFamilyInfo_Necris_Female')
	Families.Add(class'UTFamilyInfo_Necris_Male')
	Families.Add(class'UTFamilyInfo_TwinSouls_Female')
	Families.Add(class'UTFamilyInfo_TwinSouls_Male')

	DefaultArmMeshPackageName="WP_Arms"
	DefaultArmMeshName="WP_Arms.Mesh.SK_WP_IronGuard_MaleB_1P_Arms"
	DefaultArmSkinPackageName="CH_IronGuard_Arms"
	DefaultRedArmSkinName="CH_IronGuard_Arms.Materials.M_CH_IronG_Arms_FirstPersonArm_VRed"
	DefaultBlueArmSkinName="CH_IronGuard_Arms.Materials.M_CH_IronG_Arms_FirstPersonArm_VBlue"
}
