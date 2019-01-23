
/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class AnimNodeAimOffset extends AnimNodeBlendBase
	native(Anim)
	hidecategories(Object);

/**
 * 9 control points range:
 *
 * Left	Center	Right
 *
 * LU	CU		RU		Up
 * LC	CC		RC		Center
 * LD	CD		RD		Down
 */
struct native AimTransform
{
	var		Rotator	Rotation;
	var()	Quat	Quaternion;
	var()	Vector	Translation;
};


/**
 * definition of an AimComponent.
 */
struct native AimComponent
{
	/** Bone transformed */
	var()	Name			BoneName;

	/** Left column */
	var()	AimTransform	LU;
	var()	AimTransform	LC;
	var()	AimTransform	LD;

	/** Center */
	var()	AimTransform	CU;
	var()	AimTransform	CC;
	var()	AimTransform	CD;

	/** Right */
	var()	AimTransform	RU;
	var()	AimTransform	RC;
	var()	AimTransform	RD;
};



/** Handy enum for working with directions. */
enum EAnimAimDir
{
	ANIMAIM_LEFTUP,
	ANIMAIM_CENTERUP,
	ANIMAIM_RIGHTUP,
	ANIMAIM_LEFTCENTER,
	ANIMAIM_CENTERCENTER,
	ANIMAIM_RIGHTCENTER,
	ANIMAIM_LEFTDOWN,
	ANIMAIM_CENTERDOWN,
	ANIMAIM_RIGHTDOWN
};


/** Angle of aiming, between -1..+1 */
var()	Vector2d		Aim;

/** Angle offset applied to Aim before processing */
var()	Vector2d		AngleOffset;

/** If true, ignore Aim, and use the ForcedAimDir enum instead to determine which aim direction to draw. */
var()	bool			bForceAimDir;

/** If the LOD of this skeletal mesh is at or above this LOD, then this node will do nothing. */
var()	int				PassThroughAtOrAboveLOD;

/** If bForceAimDir is true, this is the direction to render the character aiming in. */
var()	EAnimAimDir		ForcedAimDir;

/** Internal, array of required bones. Selected bones and their parents for local to component space transformation. */
var	transient	Array<byte>	RequiredBones;
/** Bone Index to AimComponent Index look up table. */
var	transient	Array<INT> BoneToAimCpnt;

/** 
 *	Pointer to AimOffset node in package (AnimTreeTemplate), to avoid duplicating profile data. 
 *	Always NULL in AimOffset Editor (in ATE).
 */
var	transient AnimNodeAimOffset	TemplateNode;

/** Bake offsets from animations. */
var()	bool			bBakeFromAnimations;

//// ONLY FOR BACKWARD COMPATIBILITY (before VER_AIMOFFSET_PROFILES)
var		Vector2d			HorizontalRange;
var		Vector2d			VerticalRange;
var		Array<AimComponent>	AimComponents;
var		Name				AnimName_LU;
var		Name				AnimName_LC;
var		Name				AnimName_LD;
var		Name				AnimName_CU;
var		Name				AnimName_CC;
var		Name				AnimName_CD;
var		Name				AnimName_RU;
var		Name				AnimName_RC;
var		Name				AnimName_RD;
////

struct native AimOffsetProfile
{
	/** Name of this aim-offset profile. */
	var()	const editconst name	ProfileName;

	/** Maximum horizontal range (min, max) for horizontal aiming. */
	var()	Vector2d				HorizontalRange;

	/** Maximum horizontal range (min, max) for vertical aiming. */
	var()	Vector2d				VerticalRange;

	/**
	 * Array of AimComponents.
	 * Represents the selected bones and their transformations.
	 */
	var		Array<AimComponent>		AimComponents;

	/**
	 *	Names of animations to use when automatically generating offsets based animations for each direction.
	 *	Animations are not actually used in-game - just for editor.
	 */
	var()	Name	AnimName_LU;
	var()	Name	AnimName_LC;
	var()	Name	AnimName_LD;
	var()	Name	AnimName_CU;
	var()	Name	AnimName_CC;
	var()	Name	AnimName_CD;
	var()	Name	AnimName_RU;
	var()	Name	AnimName_RC;
	var()	Name	AnimName_RD;

	structdefaultproperties
	{
		ProfileName="Default"
		HorizontalRange=(X=-1,Y=+1)
		VerticalRange=(X=-1,Y=+1)
	}
};

/** Array of different aiming 'profiles' */
var()	editconst array<AimOffsetProfile>		Profiles;

/**
 *	Index of currently active Profile.
 *	Use the SetActiveProfileByName or SetActiveProfileByIndex function to change.
*/
var()	const editconst int			CurrentProfileIndex;

cpptext
{
	// UObject interface
	virtual void		PostLoad();
	virtual void		PostEditChange(UProperty* PropertyThatChanged);

	/** Do any initialisation, and then call InitAnim on all children. Should not discard any existing anim state though. */
	virtual void		InitAnim(USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent);

	/** Used to save pointer to AimOffset node in package, to avoid duplicating profile data. */
	virtual void		PostAnimNodeInstance(UAnimNode* SourceNode);

	/** Convert Rotators to Quaternions. Can be removed once VER_AIMOFFSET_ROT2QUAT has been long distributed */
	void				ConvertRotToQuat(INT CompIndex, EAnimAimDir InAimDir);

	/** returns current aim. Override this to pull information from somewhere else, like Pawn actor for example. */
	virtual FVector2D	GetAim() { return Aim; }

	virtual void		GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion);

	/** 
	 * Function called after Aim has been extracted and processed (offsets, range, clamping...).
	 * Gives a chance to PostProcess it before being used by the AimOffset Node.
	 * Note that X,Y range should remain [-1,+1].
	 */
	virtual void		PostAimProcessing(FVector2D &AimOffsetPct) {}

	/** Util for getting the current AimOffsetProfile. */
	FAimOffsetProfile*	GetCurrentProfile();

	/** Update cached list of required bones, use to transform skeleton from parent space to component space. */
	void				UpdateListOfRequiredBones();

	/** Returns TRUE if AimComponents contains specified bone */
	UBOOL				ContainsBone(const FName &BoneName);

	/** Util for grabbing the rotation on a specific bone in a specific direction. */
	FRotator			GetBoneAimRotation(INT CompIndex, EAnimAimDir InAimDir);
	/** Util for grabbing the quaternion on a specific bone in a specific direction. */
	FQuat				GetBoneAimQuaternion(INT CompIndex, EAnimAimDir InAimDir);
	/** Util for grabbing the translation on a specific bone in a specific direction. */
	FVector				GetBoneAimTranslation(INT CompIndex, EAnimAimDir InAimDir);

	/** Util for setting the rotation on a specific bone in a specific direction. */
	void				SetBoneAimRotation(INT CompIndex, EAnimAimDir InAimDir, FRotator InRot);
	/** Util for setting the quaternion on a specific bone in a specific direction. */
	void				SetBoneAimQuaternion(INT CompIndex, EAnimAimDir InAimDir, FQuat InQuat);
	/** Util for setting the translation on a specific bone in a specific direction. */
	void				SetBoneAimTranslation(INT CompIndex, EAnimAimDir InAimDir, FVector InTrans);

	/** Bake in Offsets from supplied Animations. */
	void				BakeOffsetsFromAnimations();
	void				ExtractOffsets(TArray<FBoneAtom>& RefBoneAtoms, TArray<FBoneAtom>& BoneAtoms, EAnimAimDir InAimDir);
	INT					GetComponentIdxFromBoneIdx(const INT BoneIndex, UBOOL bCreateIfNotFound=0);
	/**
	 * Extract Parent Space Bone Atoms from Animation Data specified by Name.
	 * Returns TRUE if successful.
	 */
	UBOOL				ExtractAnimationData(UAnimNodeSequence *SeqNode, FName AnimationName, TArray<FBoneAtom>& BoneAtoms);

	// For slider support
	virtual INT			GetNumSliders() const { return 1; }
	virtual ESliderType GetSliderType(INT InIndex) const { return ST_2D; }
	virtual FLOAT		GetSliderPosition(INT SliderIndex, INT ValueIndex);
	virtual void		HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue);
	virtual FString		GetSliderDrawValue(INT SliderIndex);
}

/**
 *	Change the currently active profile to the one with the supplied name.
 *	If a profile with that name does not exist, this does nothing.
 */
native function SetActiveProfileByName(name ProfileName);

/**
 *	Change the currently active profile to the one with the supplied index.
 *	If ProfileIndex is outside range, this does nothing.
 */
native function SetActiveProfileByIndex(int ProfileIndex);

defaultproperties
{
	Children(0)=(Name="Input",Weight=1.0)
	bFixNumChildren=TRUE

	HorizontalRange=(X=-1,Y=+1)
	VerticalRange=(X=-1,Y=+1)
	ForcedAimDir=ANIMAIM_CENTERCENTER
	PassThroughAtOrAboveLOD=1000
}
