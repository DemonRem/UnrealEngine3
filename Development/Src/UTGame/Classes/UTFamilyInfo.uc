/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 * Structure defining information about a particular 'family' (eg. Ironguard Male)
 */

class UTFamilyInfo extends Object
	native
	abstract;

/** Matches the FamilyID in the CustomCharData */
var string FamilyID;

/** Faction that this family belongs to. */
var string Faction;

/** Mesh to use for first person weapon */
//var SkeletalMesh ArmMesh;

/** Package to load to find the arm mesh for this char. */
var string	ArmMeshPackageName;
/** Name of mesh within ArmMeshPackageName to use for arms. */
var string	ArmMeshName;
/** Package that contains team-skin materials for first-person arms. */
var		string				ArmSkinPackageName;
/** Name of red team material for first-person arms. */
var		string				RedArmSkinName;
/** Name of blue team material for first-person arms. */
var		string				BlueArmSkinName;

/** Physics Asset to use  */
var PhysicsAsset		PhysAsset;

/** Animation sets to use for a character in this 'family' */
var	array<AnimSet>		AnimSets;

/** Names for specific bones in the skeleton */
var name LeftFootBone;
var name RightFootBone;
var array<name> TakeHitPhysicsFixedBones;

var class<UTPawnSoundGroup> SoundGroupClass;

var MaterialInstanceConstant	BaseMICParent;
var MaterialInstanceConstant	BioDeathMICParent;

/** Contains all bones used by this family - used for animating character on creation screen. */
var	SkeletalMesh				MasterSkeleton;

/** When not in a team game, this is the color to use for glowy bits. */
var	LinearColor					NonTeamEmissiveColor;

/** When not in a team game, this is the color to tint character at a distance. */
var LinearColor					NonTeamTintColor;

/** Structure containing information about a specific emote */
struct native EmoteInfo
{
	/** Category to which this emote belongs. */
	var name		CategoryName;
	/** This is a unquie tag used to look up this emote */
	var name		EmoteTag;
	/** Friendly name of this emote (eg for menu) */
	var string		EmoteName;
	/** Name of animation to play. Should be in AnimSets above. */
	var name		EmoteAnim;
	/** SoundCue to play for this emote. */
	var SoundCue	EmoteSound;
	/** Indicates that this is a whole body 'victory' emote which should only be offered at the end of the game. */
	var bool		bVictoryEmote;
	/** Emote should only be played on top half of body. */
	var bool		bTopHalfEmote;
	/** The command that goes with this emote */
	var name  		Command;
	/** if true, the command requires a PRI */
	var bool		bRequiresPlayer;
};

/** Set of all emotes for this family. */
var array<EmoteInfo>	FamilyEmotes;

//// Gibs

/** information on what gibs to spawn and where */
struct native GibInfo
{
	/** the bone to spawn the gib at */
	var name BoneName;
	/** the gib class to spawn */
	var class<UTGib> GibClass;
	var bool bHighDetailOnly;
};
var array<GibInfo> Gibs;

/** Head gib */
var GibInfo HeadGib;


// NOTE:  this can probably be moved to the DamageType.  As the damage type is probably not going to have different types of mesh per race (???)
/** This is the skeleton skel mesh that will replace the character's mesh for various death effects **/
var SkeletalMesh DeathMeshSkelMesh;
var PhysicsAsset DeathMeshPhysAsset;

/** This is the number of materials on the DeathSkeleton **/
var int DeathMeshNumMaterialsToSetResident;

/** Which joints we can break when applying damage **/
var Array<Name> DeathMeshBreakableJoints;



/**
 * Returns the # of emotes in a given group
 */
function static int GetEmoteGroupCnt(name Category)
{
	local int i,cnt;
	for (i=0;i<default.FamilyEmotes.length;i++)
	{
		if (default.FamilyEmotes[i].CategoryName == Category )
		{
			cnt++;
		}
	}

	return cnt;
}

/**
 * returns all the Emotes in a group
 */
function static GetEmotes(name Category, out array<string> Captions, out array<name> EmoteTags)
{
	local int i;
	local int cnt;
	for (i=0;i<default.FamilyEmotes.length;i++)
	{
		if (default.FamilyEmotes[i].CategoryName == Category )
		{
			Captions[cnt] = default.FamilyEmotes[i].EmoteName;
			EmoteTags[cnt] = default.FamilyEmotes[i].EmoteTag;
			cnt++;
		}
	}
}

/**
 * Finds the index of the emote given a tag
 */

function static int GetEmoteIndex(name EmoteTag)
{
	local int i;
	for (i=0;i<default.FamilyEmotes.length;i++)
	{
		if ( default.FamilyEmotes[i].EmoteTag == EmoteTag )
		{
			return i;
		}
	}
	return -1;
}

defaultproperties
{
	LeftFootBone=b_LeftAnkle
	RightFootBone=b_RightAnkle
	TakeHitPhysicsFixedBones[0]=b_LeftAnkle
	TakeHitPhysicsFixedBones[1]=b_RightAnkle
	SoundGroupClass=class'UTPawnSoundGroup'

	FamilyEmotes.Add((CategoryName="Taunt",EmoteTag="TauntA",EmoteName="Mess You Up",EmoteAnim="Taunt_UB_Slit_Throat",EmoteSound=SoundCue'A_Taunts_Malcolm.A_Taunts_Malcolm_ImAMessYouUpCue',bTopHalfEmote=true))
	FamilyEmotes.Add((CategoryName="Taunt",EmoteTag="TauntB",EmoteName="I'm On Fire",EmoteAnim="Stumble_Bwd",EmoteSound=SoundCue'A_Taunts_Malcolm.A_Taunts_Malcolm_ImOnFireCue'))
	FamilyEmotes.Add((CategoryName="Warning",EmoteTag="WarningA",EmoteName="Here They Come",EmoteSound=SoundCue'A_Taunts_Malcolm.A_Taunts_Malcolm_HereTheyComeCue'))
	FamilyEmotes.Add((CategoryName="Victory",EmoteTag="VictoryA",EmoteName="Cheer",EmoteAnim="Taunt_FB_Victory"))
	FamilyEmotes.Add((CategoryName="Action",EmoteTag="ActionA",EmoteName="Take Flag",EmoteAnim="Taunt_UB_Flag_Pickup",bTopHalfEmote=true,Command="Attack",bRequiresPlayer=true))

	// default death mesh here
	DeathMeshSkelMesh=SkeletalMesh'CH_Skeletons.Mesh.SK_CH_Skeleton_Human_Male'
	DeathMeshPhysAsset=PhysicsAsset'CH_Skeletons.Mesh.SK_CH_Skeleton_Human_Male_Physics'

	DeathMeshNumMaterialsToSetResident=1

	DeathMeshBreakableJoints=("b_LeftArm","b_RightArm","b_LeftLegUpper","b_RightLegUpper")

	NonTeamEmissiveColor=(R=10.0,G=0.2,B=0.2)
	NonTeamTintColor=(R=4.0,G=2.0,B=0.5)
}
