/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_Interp extends SeqAct_Latent
	native(Sequence);

cpptext
{
	// UObject interface.
	/**
	 * Serialize function.
	 *
	 * @param	Ar		The archive to serialize with.
	 */
	virtual void Serialize(FArchive& Ar);

	// USequenceAction interface

	virtual void Activated();
	virtual UBOOL UpdateOp(FLOAT deltaTime);
	virtual void DeActivated();
	virtual void OnCreated();

	virtual void UpdateObject();

	// USeqAct_Interp interface

	/**
	 * Begin playback of this sequence. Only called in game.
	 * Will then advance Position by (PlayRate * Deltatime) each time the SeqAct_Interp is ticked.
	 */
	void Play();

	/** Stop playback at current position. */
	void Stop();

	/** Similar to play, but the playback will go backwards until the beginning of the sequence is reached. */
	void Reverse();

	/** Hold playback at its current position, but leave the sequence initialised. Calling Pause again will continue playback in its current direction. */
	void Pause();

	/** Changes the direction of playback (go in reverse if it was going forward, or vice versa) */
	void ChangeDirection();

	/** Called to notify affected actors when a new impulse changes the interpolation (paused, reversed direction, etc) */
	void NotifyActorsOfChange();

	/** Increment track forwards by given timestep and iterate over each track updating any properties. */
	virtual void StepInterp(FLOAT DeltaTime, UBOOL bPreview=false);

	/** Move interpolation to new position and iterate over each track updating any properties. */
	void UpdateInterp(FLOAT NewPosition, UBOOL bPreview=false, UBOOL bJump=false);

	/** For each InterGroup/Actor combination, create a InterpGroupInst, assign Actor and initialise each track. */
	void InitInterp();

	/** Destroy all InterpGroupInst. */
	void TermInterp();

	/** Reset the 'initial transform' for all movement tracks to be from the current actor positions. */
	void ResetMovementInitialTransforms();

	/** See if there is an instance referring to the supplied Actor. Returns NULL if not. */
	class UInterpGroupInst* FindGroupInst(AActor* Actor);

	/** Find the first group instance based on the given InterpGroup. */
	class UInterpGroupInst* FindFirstGroupInst(class UInterpGroup* InGroup);

	/** Find the first group instance based on the InterpGroup with the given name. */
	class UInterpGroupInst* FindFirstGroupInstByName(FName InGroupName);

	/** Find the InterpData connected to the first Variable connector. Returns NULL if none attached. */
	class UInterpData* FindInterpDataFromVariable();

	/** Synchronise the variable connectors with the currently attached InterpData. */
	void UpdateConnectorsFromData();

	/** Use any existing DirectorGroup to see which Actor we currently want to view through. */
	class AActor* FindViewedActor();

	/**
	 *	Utility for getting all Actors currently being worked on by this Matinee action.
	 *	If bMovementTrackOnly is set, Actors must have a Movement track in their group to be included in the results.
	 */
	void GetAffectedActors(TArray<AActor*>& OutActors, UBOOL bMovementTrackOnly);

	/**
	 * Adds the specified actor and any actors attached to it to the list
	 * of saved actor transforms.  Does nothing if an actor has already
	 * been saved.
	 */
	void SaveActorTransforms(AActor* Actor);

	/**
	 * Applies the saved locations and rotations to all saved actors.
	 */
	void RestoreActorTransforms();

	/**
	 * Stores the current scrub position, restores all saved actor transforms,
	 * then saves off the transforms for actors referenced (directly or indirectly)
	 * by group instances, and finally restores the scrub position.
	 */
	void RecaptureActorTransforms();

	/** called when the level that contains this sequence object is being removed/unloaded */
	virtual void CleanUp();

	/** Sets up the group actor for the specified InterpGroup. */
	void InitGroupActorForGroup(class UInterpGroup* InGroup, class AActor* GroupActor);
}

/**
 * Helper type for storing actors' World-space locations/rotations.
 */
struct native export SavedTransform
{
	var vector	Location;
	var rotator	Rotation;
};

/** A map from actors to their pre-Matinee world-space positions/orientations.  Includes actors attached to Matinee-affected actors. */
var editoronly private const transient noimport native map{AActor*,FSavedTransform} SavedActorTransforms;

/** Time multiplier for playback. */
var()	float					PlayRate;

/** Time position in sequence - starts at 0.0 */
var		float					Position;

/** Time position to always start at if bForceStartPos is set to TRUE. */
var()	float					ForceStartPosition;

/** If sequence is currently playing. */
var		bool					bIsPlaying;

/** Sequence is initialised, but ticking will not increment its current position. */
var		bool					bPaused;

/** Indicates whether this SeqAct_Interp is currently open in the Matinee tool. */
var		transient bool			bIsBeingEdited;

/**
 *	If sequence should pop back to beginning when finished.
 *	Note, if true, will never get Complete/Aborted events - sequence must be explicitly Stopped.
 */
var()	bool					bLooping;

/** If true, sequence will rewind itself back to the start each time the Play input is activated. */
var()	bool					bRewindOnPlay;

/**
 *	If true, when rewinding this interpolation, reset the 'initial positions' of any RelateToInitial movements to the current location.
 *	This allows the next loop of movement to proceed from the current locations.
 */
var()	bool					bNoResetOnRewind;

/**
 *	Only used if bRewindOnPlay if true. Defines what should happen if the Play input is activated while currently playing.
 *	If true, hitting Play while currently playing will pop the position back to the start and begin playback over again.
 *	If false, hitting Play while currently playing will do nothing.
 */
var()	bool					bRewindIfAlreadyPlaying;

/** If sequence playback should be reversed. */
var		bool					bReversePlayback;

/** Whether this action should be initialised and moved to the 'path building time' when building paths. */
var()	bool					bInterpForPathBuilding;

/** Lets you force the sequence to always start at ForceStartPosition */
var()	bool					bForceStartPos;

/** Indicates that this interpolation does not affect gameplay. This means that:
 * -it is not replicated via MatineeActor
 * -it is not ticked if no affected Actors are visible
 * -on dedicated servers, it is completely ignored
 */
var() bool bClientSideOnly;

/** if bClientSideOnly is true, whether this matinee should be completely skipped if none of the affected Actors are visible */
var() bool bSkipUpdateIfNotVisible;

/** Lets you skip the matinee with the CANCELMATINEE exec command. Triggers all events to the end along the way. */
var()	bool					bIsSkippable;

/** Cover linked to this matinee that should be updated once path building time has been played */
var() array<CoverLink>			LinkedCover;

/** Actual track data. Can be shared between SeqAct_Interps. */
var		export InterpData		InterpData;

/** Instance data for interp groups. One for each variable/group combination. */
var		array<InterpGroupInst>	GroupInst;

/** on a net server, actor spawned to handle replicating relevant data to the client */
var const class<MatineeActor> ReplicatedActorClass;
var const MatineeActor ReplicatedActor;

/** sets the position of the interpolation
 * @note if the interpolation is not currently active, this function doesn't send any Kismet or UnrealScript events
 * @param NewPosition the new position to set the interpolation to
 * @param bJump if true, teleport to the new position (don't trigger any events between the old and new positions, etc)
 */
native final function SetPosition(float NewPosition, optional bool bJump = false);

/** adds the passed in PlayerController to all running Director tracks so that its camera is controlled
 * all PCs that are available at playback start time are hooked up automatically, but this needs to be called to hook up
 * any that are created during playback (player joining a network game during a cinematic, for example)
 * @param PC the PlayerController to add
 */
native final function AddPlayerToDirectorTracks(PlayerController PC);

function Reset()
{
	SetPosition(0.0, false);
	// stop if currently playing
	if (bActive)
	{
		InputLinks[2].bHasImpulse = true;
	}
}

defaultproperties
{
	ObjName="Matinee"

	PlayRate=1.0

	InputLinks(0)=(LinkDesc="Play")
	InputLinks(1)=(LinkDesc="Reverse")
	InputLinks(2)=(LinkDesc="Stop")
	InputLinks(3)=(LinkDesc="Pause")
	InputLinks(4)=(LinkDesc="Change Dir")

	OutputLinks(0)=(LinkDesc="Completed")
	OutputLinks(1)=(LinkDesc="Aborted")

	VariableLinks.Empty
	VariableLinks(0)=(ExpectedType=class'InterpData',LinkDesc="Data",MinVars=1,MaxVars=1)

	ReplicatedActorClass=class'MatineeActor'
}
