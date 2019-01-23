/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSeqObj_SPMission extends SequenceAction
	dependson(UTProfileSettings)
	native(UI);

enum EMissionResult
{
	EMResult_Any,
	EMResult_Won,
	EMResult_Lost,
};

struct native EMissionCondition
{
	var() editinline EMissionResult MissionResult;
	var() editinline string ConditionDesc;
	var() editinline array<ESinglePlayerPersistentKeys> RequiredPersistentKeys;
	var() editinline array<ESinglePlayerPersistentKeys> RestrictedPersistentKeys;

	structdefaultproperties
	{
		ConditionDesc=""
	}

};

struct native EMissionTeaserInfo
{
	/** The image to display when selecting this map */
	var() Texture2D Portrait;

	/** The UV's */
	var() TextureCoordinates PortraitUVs;

	/** The audio to play */
	var() SoundCue Audio;

	/** The Text to display */
	var() string Text;

	/** This value will guarantee that the sound/teaser is played/displays for a given duration. */
	var() float MinPlaybackDuration;

};

struct native EMissionObjectiveInfo
{
	var() Texture2D BackgroundImg;
	var() string Text;
};

struct native EMissionMapInfo
{
	// The map to use
	var() editinline string	MissionMap;

	// The URL
	var() editinline string MissionURL;

	// The image to use
	var() editinlineuse surface MissionMapImage;

	// Are we using custom coordinates
	var() bool bCustomMapCoords;

	// The Map Coordinates
	var() editinline TextureCoordinates MissionMapCoords;

	// If true, this node references a cut sequence and we should automatically
	// transition.
	var() bool bCutSequence;

	// if true, this node will automatically transition to the next mission skipping
	// the selection menu.
	var() bool bAutomaticTransition;

	// Holds the BoneName of the point on the map to focus on when selecting this mission.
	var() name MapPoint;

	// Which globe does this point reside on
	var() name MapGlobeTag;

	/** How far from the MapPoint should the camera end up */
	var() float MapDist;

	/** This holds information that is say when the user selection this mission in the viewer */
	var() editinline array<EMissionTeaserInfo> Teasers;

	/** This holds information that will be displayed in the Map briefing dialog */
	var() editinline array<EMissionObjectiveInfo> Objectives;

	/** This will be set by the menus */
	var transient StaticMeshComponent MapBeacon;

};



struct native EMissionData
{
	var() int MissionIndex;
	var() string MissionTitle;
	var() string MissionDescription;
	var() editinline EMissionMapInfo MissionRules;
	var() editinline array<EMissionCondition> MissionProgression;


	structdefaultproperties
	{
		MissionTitle="<Edit Me>"
		MissionDescription="<Edit Me>"
	}
};

var() bool bFirstMission;
var() EMissionData MissionInfo;

/** Used to auto-adjust the indices */
var transient int OldIndex;

cpptext
{
	virtual void PreEditChange(UProperty* PropertyThatWillChange);
	virtual void PostEditChange( class FEditPropertyChain& PropertyThatChanged );
	virtual void DrawSeqObj(FCanvas* Canvas, UBOOL bSelected, UBOOL bMouseOver, INT MouseOverConnType, INT MouseOverConnIndex, FLOAT MouseOverTime);
	void WrappedPrint( FCanvas* Canvas, UBOOL Draw, FLOAT CurX, FLOAT CurY, FLOAT &XL, FLOAT& YL, UFont* Font, const TCHAR* Text, FLinearColor DrawColor);
	FString GetDescription();
	virtual void OnCreated();
	void AdjustIndex();
	void SetAsFirstMission();
	virtual void OnPasted();
	void FindInitialIndex();
}


/**
 * Returns the number of Children
 */
function int NumChildren()
{
	return MissionInfo.MissionProgression.Length;
}

/**
 * Returns the Mission object associated with a child
 */

function UTSeqObj_SPMission GetChild(int ChildIndex, out EMissionCondition Condition)
{
	local UTSeqObj_SPMission Mission;

	if ( ChildIndex >= 0 && ChildIndex < MissionInfo.MissionProgression.Length && ChildIndex < OutputLinks.Length )
	{
		Condition = MissionInfo.MissionProgression[ChildIndex];
		Mission = UTSeqObj_SPMission( OutputLinks[ChildIndex].Links[0].LinkedOp );
	}

	return Mission;
}

defaultproperties
{
	ObjColor=(R=255,G=0,B=0,A=255)
	InputLinks.Empty
	OutputLinks.Empty
	VariableLinks.Empty
	ObjName="Single Player Mission"
	InputLinks(0)=(LinkDesc="In")
}

