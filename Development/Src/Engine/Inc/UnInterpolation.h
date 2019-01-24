/*=============================================================================
	UnInterpolation.h: Matinee related C++ declarations
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/** Struct of data that is passed to the input interface. */
struct FInterpEdInputData
{
	FInterpEdInputData()
		: InputType( 0 ),
		  InputData( 0 ),
		  TempData( NULL ),
		  bCtrlDown( FALSE ),
		  bAltDown( FALSE ),
		  bShiftDown( FALSE ),
		  MouseStart( 0, 0 ),
		  MouseCurrent( 0, 0 ),
		  PixelsPerSec( 0.0f )
	{
	}

	FInterpEdInputData( INT InType, INT InData )
		: InputType( InType ),
		  InputData( InData ),
		  TempData( NULL ),
		  bCtrlDown( FALSE ),
		  bAltDown( FALSE ),
		  bShiftDown( FALSE ),
		  MouseStart( 0, 0 ),
		  MouseCurrent( 0, 0 ),
		  PixelsPerSec( 0.0f )
	{
	}

	INT InputType;
	INT InputData;
	void* TempData;	 // Should only be initialized in StartDrag and should only be deleted in EndDrag!

	// Mouse data - Should be filled in automatically.
	UBOOL bCtrlDown;
	UBOOL bAltDown;
	UBOOL bShiftDown;
	FIntPoint MouseStart;
	FIntPoint MouseCurrent;
	FLOAT PixelsPerSec;
};

/** Defines a set of functions that provide drag drop functionality for the interp editor classes. */
class FInterpEdInputInterface
{
public:
	/**
	 * @return Returns the mouse cursor to display when this input interface is moused over.
	 */
	virtual EMouseCursor GetMouseCursor(FInterpEdInputData &InputData) {return MC_NoChange;}

	/**
	 * Lets the interface object know that we are beginning a drag operation.
	 */
	virtual void BeginDrag(FInterpEdInputData &InputData) {}

	/**
	 * Lets the interface object know that we are ending a drag operation.
	 */
	virtual void EndDrag(FInterpEdInputData &InputData) {}

	/** @return Whether or not this object can be dropped on. */
	virtual UBOOL AcceptsDropping(FInterpEdInputData &InputData, FInterpEdInputInterface* DragObject) {return FALSE;}

	/**
	 * Called when an object is dragged.
	 */
	virtual void ObjectDragged(FInterpEdInputData& InputData) {};

	/**
	 * Allows the object being dragged to be draw on the canvas.
	 */
	virtual void DrawDragObject(FInterpEdInputData &InputData, FViewport* Viewport, FCanvas* Canvas) {}

	/**
	 * Allows the object being dropped on to draw on the canvas.
	 */
	virtual void DrawDropObject(FInterpEdInputData &InputData, FViewport* Viewport, FCanvas* Canvas) {}

	/** @return Whether or not the object being dragged can be dropped. */
	virtual UBOOL ShouldDropObject(FInterpEdInputData &InputData) {return FALSE;}

	/** @return Returns a UObject pointer of this instance if it also inherits from UObject. */
	virtual UObject* GetUObject() {return NULL;}
};


class FInterpEdSelKey
{
public:
	FInterpEdSelKey()
	{
		Group = NULL;
		Track = NULL;
		KeyIndex = INDEX_NONE;
		UnsnappedPosition = 0.f;
	}

	FInterpEdSelKey(class UInterpGroup* InGroup, class UInterpTrack* InTrack, INT InKey)
	{
		Group = InGroup;
		Track = InTrack;
		KeyIndex = InKey;
	}

	UBOOL operator==(const FInterpEdSelKey& Other) const
	{
		if(	Group == Other.Group &&
			Track == Other.Track &&
			KeyIndex == Other.KeyIndex )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	class UInterpGroup* Group;
	class UInterpTrack* Track;
	INT KeyIndex;
	FLOAT UnsnappedPosition;
};



/** Parameters for drawing interp tracks, used by Matinee */
class FInterpTrackDrawParams
{

public:

	/** This track's index */
	INT TrackIndex;
	
	/** Track display width */
	INT TrackWidth;
	
	/** Track display height */
	INT TrackHeight;
	
	/** The view range start time (within the sequence) */
	FLOAT StartTime;
	
	/** Scale of the track window in pixels per second */
	FLOAT PixelsPerSec;

	/** Current position of the Matinee time cursor along the timeline */
	FLOAT TimeCursorPosition;

	/** Current snap interval (1.0 / frames per second) */
	FLOAT SnapAmount;

	/** True if we want frame numbers to be rendered instead of time values where appropriate */
	UBOOL bPreferFrameNumbers;

	/** True if we want time cursor positions drawn for all anim tracks */
	UBOOL bShowTimeCursorPosForAllKeys;

	/** True if the user should be allowed to select using "keyframe bars," such as those for audio tracks */
	UBOOL bAllowKeyframeBarSelection;

	/** True if the user should be allowed to select using keyframe text */
	UBOOL bAllowKeyframeTextSelection;

	/** List of keys that are currently selected */
	TArray< FInterpEdSelKey > SelectedKeys;
};

/** Helper struct for creating sub tracks supported by this track */
struct FSupportedSubTrackInfo
{
	/** The sub track class which is supported by this track */
	UClass* SupportedClass;
	/** The name of the subtrack */
	FString SubTrackName;
	/** Index into the any subtrack group this subtrack belongs to (can be -1 for no group) */
	INT GroupIndex;
};

/** A small structure holding data for grouping subtracks. (For UI drawing purposes) */
struct FSubTrackGroup
{
	/** Name of the subtrack  group */
	FString GroupName;
	/** Indices to tracks in the parent track subtrack array. */
	TArray<INT> TrackIndices;
	/** If this group is collapsed */
	BITFIELD bIsCollapsed:1;
	/** If this group is selected */ 
	BITFIELD bIsSelected:1;
};

/** Required condition for this track to be enabled */
enum ETrackActiveCondition
{
	/** Track is always active */
	ETAC_Always,

	/** Track is active when extreme content (gore) is enabled */
	ETAC_GoreEnabled,

	/** Track is active when extreme content (gore) is disabled */
	ETAC_GoreDisabled
};


class UInterpTrack : public UObject, public FCurveEdInterface, public FInterpEdInputInterface
{
public:
	/** A list of subtracks that belong to this track */
	TArrayNoInit<UInterpTrack*> SubTracks;
	/** A list of subtrack groups (for editor UI organization only) */
	TArrayNoInit<FSubTrackGroup> SubTrackGroups;
	/** A list of supported tracks that can be a subtrack of this track. */
	TArrayNoInit<FSupportedSubTrackInfo> SupportedSubTracks;
    class UClass* TrackInstClass;
	BYTE ActiveCondition;
    FStringNoInit TrackTitle;
    BITFIELD bOnePerGroup:1;
    BITFIELD bDirGroupOnly:1;
private:
	BITFIELD bDisableTrack:1;
public:
	BITFIELD bIsAnimControlTrack:1;
	BITFIELD bSubTrackOnly:1;
	BITFIELD bVisible:1;
	BITFIELD bIsSelected:1;
	BITFIELD bIsRecording:1;
	BITFIELD bIsCollapsed:1;
	SCRIPT_ALIGN;
    DECLARE_CLASS_NOEXPORT(UInterpTrack,UObject,0,Engine)

	// InterpTrack interface

	/** Total number of keyframes in this track. */
	virtual INT GetNumKeyframes() const { return 0; }

	/** Get first and last time of keyframes in this track. */
	virtual void GetTimeRange(FLOAT& StartTime, FLOAT& EndTime) const { StartTime = 0.f; EndTime = 0.f; }

	/** Get the time of the keyframe with the given index. */
	virtual FLOAT GetKeyframeTime(INT KeyIndex) const {return 0.f; }

	/** Get the index of the keyframe with the given time. */
	virtual INT GetKeyframeIndex( FLOAT KeyTime ) const { return -1; }

	/** Add a new keyframe at the speicifed time. Returns index of new keyframe. */
	virtual INT AddKeyframe( FLOAT Time, class UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode ) { return INDEX_NONE; }

	/**
     * Adds a keyframe to a child track 
	 *
	 * @param ChildTrack		The child track where the keyframe should be added
	 * @param Time				What time the keyframe is located at
	 * @param TrackInst			The track instance of the parent track(this track)
	 * @param InitInterpMode	The initial interp mode for the keyframe?	 
	 */
	virtual INT AddChildKeyframe( class UInterpTrack* ChildTrack, FLOAT Time, UInterpTrackInst* TrackInst, EInterpCurveMode InitInterpMode ) { return INDEX_NONE; }

	/** Change the value of an existing keyframe. */
	virtual void UpdateKeyframe(INT KeyIndex, class UInterpTrackInst* TrInst) {}

	/**
     * Updates a child track keyframe
	 *
	 * @param ChildTrack		The child track with keyframe to update
	 * @param KeyIndex			The index of the key to be updated
	 * @param TrackInst			The track instance of the parent track(this track)
	 */
	virtual void UpdateChildKeyframe( class UInterpTrack* ChildTrack, INT KeyIndex, UInterpTrackInst* TrackInst ) {};

	/** Change the time position of an existing keyframe. This can change the index of the keyframe - the new index is returned. */
	virtual INT SetKeyframeTime(INT KeyIndex, FLOAT NewKeyTime, UBOOL bUpdateOrder=true) { return INDEX_NONE; }

	/** Remove the keyframe with the given index. */
	virtual void RemoveKeyframe(INT KeyIndex) {}

	/** 
	 *	Duplicate the keyframe with the given index to the specified time. 
	 *	Returns the index of the newly created key.
	 */
	virtual INT DuplicateKeyframe(INT KeyIndex, FLOAT NewKeyTime) { return INDEX_NONE; }

	/** Return the closest time to the time passed in that we might want to snap to. */
	virtual UBOOL GetClosestSnapPosition(FLOAT InPosition, TArray<INT> &IgnoreKeys, FLOAT& OutPosition) { return false; }

	/** 
	 *	Conditionally calls PreviewUpdateTrack depending on whether or not the track is enabled.
	 */
	virtual void ConditionalPreviewUpdateTrack(FLOAT NewPosition, class UInterpTrackInst* TrInst);

	/** 
	 *	Conditionally calls UpdateTrack depending on whether or not the track is enabled.
	 */
	virtual void ConditionalUpdateTrack(FLOAT NewPosition, class UInterpTrackInst* TrInst, UBOOL bJump);


	/** 
	 *	Function which actually updates things based on the new position in the track. 
	 *  This is called in the editor, when scrubbing/previewing etc.
	 */
	virtual void PreviewUpdateTrack(FLOAT NewPosition, class UInterpTrackInst* TrInst) {} // This is called in the editor, when scrubbing/previewing etc

	/** 
	 *	Function which actually updates things based on the new position in the track. 
	 *  This is called in the game, when USeqAct_Interp is ticked
	 *  @param bJump	Indicates if this is a sudden jump instead of a smooth move to the new position.
	 */
	virtual void UpdateTrack(FLOAT NewPosition, class UInterpTrackInst* TrInst, UBOOL bJump) {} // This is called in the game, when USeqAct_Interp is ticked

	/** Called when playback is stopped in Matinee. Useful for stopping sounds etc. */
	virtual void PreviewStopPlayback(class UInterpTrackInst* TrInst) {}

	/** Get the name of the class used to help out when adding tracks, keys, etc. in UnrealEd.
	* @return	String name of the helper class.*/
	virtual const FString	GetEdHelperClassName() const;

	/** Get the icon to draw for this track in Matinee. */
	virtual class UMaterial* GetTrackIcon() const;

	/** Whether or not this track is allowed to be used on static actors. */
	virtual UBOOL AllowStaticActors() { return FALSE; }

	/** Draw this track with the specified parameters */
	virtual void DrawTrack( FCanvas* Canvas, UInterpGroup* Group, const FInterpTrackDrawParams& Params );

	/** Return color to draw each keyframe in Matinee. */
	virtual FColor GetKeyframeColor(INT KeyIndex) const;

	/**
	 * @return	The ending time of the track. 
	 */
	virtual FLOAT GetTrackEndTime() const { return 0.0f; }

	/**
	 *	For drawing track information into the 3D scene.
	 *	TimeRes is how often to draw an event (eg. resoltion of spline path) in seconds.
	 */
	virtual void Render3DTrack(class UInterpTrackInst* TrInst, 
		const FSceneView* View, 
		FPrimitiveDrawInterface* PDI, 
		INT TrackIndex, 
		const FColor& TrackColor, 
		TArray<class FInterpEdSelKey>& SelectedKeys) {}

	/** Set this track to sensible default values. Called when track is first created. */
	virtual void SetTrackToSensibleDefault() {}

	// FInterpEdInputInterface Interface

	/** @return Returns a UObject pointer of this instance if it also inherits from UObject. */
	virtual UObject* GetUObject() {return this;}

	/** 
	 * Returns the outer group of this track.  If this track is a subtrack, the group of its parent track is returned
	 */
	UInterpGroup* GetOwningGroup();

	/** 
	 * Enables this track and optionally, all subtracks.
	 * 
	 * @param bInEnable				True if the track should be enabled, false to disable
	 * @param bPropagateToSubTracks	True to propagate the state to all subtracks
	 */
	void EnableTrack( UBOOL bInEnable, UBOOL bPropagateToSubTracks = TRUE );

	/** Returns true if this track has been disabled.  */
	UBOOL IsDisabled() const { return bDisableTrack; }

	/**
     * Creates and adds subtracks to this track
	 *
	 * @param bCopy	If subtracks are being added as a result of a copy
	 */
	virtual void CreateSubTracks( UBOOL bCopy ) {};
};
