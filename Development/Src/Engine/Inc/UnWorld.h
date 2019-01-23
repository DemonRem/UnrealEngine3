/*=============================================================================
	UnWorld.h: UWorld definition.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnPath.h"

/**
 * UWorld is the global world abstraction containing several levels.
 */
class UWorld : public UObject, public FNetworkNotify
{
	DECLARE_CLASS(UWorld,UObject,CLASS_Intrinsic,Engine)
	NO_DEFAULT_CONSTRUCTOR(UWorld)

	/** The interface to the scene manager for this world. */
	FSceneInterface*							Scene;

	/** Array of levels currently in this world. Not serialized to disk to avoid hard references.								*/
	TArray<ULevel*>								Levels;
	/** Persistent level containing the world info, default brush and actors spawned during gameplay among other things			*/
	ULevel*										PersistentLevel;
	/** Pointer to the current level being edited. Level has to be in the Levels array and == PersistentLevel in the game.		*/
	ULevel*										CurrentLevel;
	/** Pointer to the current level in the queue to be made visible, NULL if none are pending.									*/
	ULevel*										CurrentLevelPendingVisibility;

	/** Saved editor viewport states - one for each view type. Indexed using ELevelViewportType above.							*/
	FLevelViewportInfo							EditorViews[4];

	/** Reference to last save game info used for serialization. The only time this is non NULL is during UEngine::SaveGame(..) */
	class USaveGameSummary*						SaveGameSummary;

	class UNetDriver*							NetDriver;
	FURL										URL;

	/** Fake NetDriver for capturing network traffic to record demos															*/
	class UDemoRecDriver*						DemoRecDriver;

	/** Octree used for collision.																								*/
	FPrimitiveHashBase*							Hash;
	/** Used for backing up regular octree during lighting rebuilds as it is being replaced by subset.							*/
	FPrimitiveHashBase*							BackupHash;

	/** octree for navigation primitives (NavigationPoints, ReachSpecs, etc)													*/
	FNavigationOctree*							NavigationOctree;
	
	/** List of actors that were spawned during tick and need to be ticked														*/
	TArray<AActor*>								NewlySpawned;
	/** Whether we are in the middle of ticking actors/components or not														*/
	UBOOL										InTick;
	/** Toggle used to tell if an actor has been ticked by comparing their Ticked values										*/
	UBOOL										Ticked;
	/** The current ticking group																								*/
	ETickingGroup								TickGroup;

	/** 
	 * Indicates that during world ticking we are doing the final component update of dirty components 
     * (after PostAsyncWork and effect physics scene has run. 
	 */
	UBOOL										bPostTickComponentUpdate;

	INT											NetTag;
	/** Counter for allocating game- unique controller player numbers															*/
	INT											PlayerNum;

	ULineBatchComponent*						LineBatcher;
	ULineBatchComponent*						PersistentLineBatcher;

	//@todo seamless: move debug functionality into UDebugManager
	UBOOL										bShowLineChecks;
	UBOOL										bShowExtentLineChecks;
	UBOOL										bShowPointChecks;

	/** Main physics scene, containing static world geometry. */
	FRBPhysScene*								RBPhysScene;

#if WITH_NOVODEX
	/** Renderer object used by Novodex to draw debug information in the world.													*/
	class FNxDebugRenderer*						DebugRenderer;
#endif // WITH_NOVODEX
	
	/** Time in seconds (game time so we respect time dilation) since the last time we purged references to pending kill objects */
	FLOAT										TimeSinceLastPendingKillPurge;
	
	/** Decal manager. */
	class UDecalManager*						DecalManager;
	
	/** Whether world object has been initialized via Init()																	*/
	UBOOL										bIsWorldInitialized;
	
	/** Stringified dynamic shadow stats data																					*/
	FDynamicShadowStats							DynamicShadowStats;
	/** Whether the code should gather a snapshot of dynamic shadow stats */
	UBOOL										bGatherDynamicShadowStatsSnapshot;

	/** All static light component's attached to the world's scene. */
	TSparseArray<ULightComponent*>				StaticLightList;
	/** All dynamic light component's attached to the world's scene. */
	TSparseArray<ULightComponent*>				DynamicLightList;
	/** All light environment components that need to be notified when a static light changes. */
	TSparseArray<ULightEnvironmentComponent*>	LightEnvironmentList;

	/** Override, forcing level load requests to be allowed. < 0 == not allowed, 0 == have code choose, > 1 == force allow.		 */
	INT											AllowLevelLoadOverride;

	/** If this is TRUE, then AddToWorld will NOT call Sequence->BeginPlay on the level's GameSequences							*/
	UBOOL										bDisallowRoutingSequenceBeginPlay;

	/** Number of frames to delay Streaming Volume updating, useful if you preload a bunch of levels but the camera hasn't caught up yet */
	DWORD										StreamingVolumeUpdateDelay;

	/** Array of any additional objects that need to be referenced by this world, to make sure they aren't GC'd */
	TArray<UObject*>							ExtraReferencedObjects;

#if !FINAL_RELEASE
	/** Is level streaming currently frozen?																					*/
	UBOOL										bIsLevelStreamingFrozen;
#endif

	/**
	 * UWorld constructor called at game startup and when creating a new world in the Editor.
	 * Please note that this constructor does NOT get called when a world is loaded from disk.
	 *
	 * @param	InURL	URL associated with this world.
	 */
	UWorld( const FURL& InURL );
	
	/**
	 * Static constructor, called once during static initialization of global variables for native 
	 * classes. Used to e.g. register object references for native- only classes required for realtime
	 * garbage collection or to associate UProperties.
	 */
	void StaticConstructor();

	// Accessor functions.

	/**
	 * Returns the current netmode
	 *
	 * @return current netmode
	 */
	ENetMode GetNetMode() const;

	/**
	 * Returns the current game info object.
	 *
	 * @return current game info object
	 */
	AGameInfo* GetGameInfo()	const;	

	/**
	 * Returns the first navigation point. Note that ANavigationPoint contains
	 * a pointer to the next navigation point so this basically returns a linked
	 * list of navigation points in the world.
	 *
	 * @return first navigation point
	 */
	ANavigationPoint* GetFirstNavigationPoint() const;

	/**
	 * Returns the first controller. Note that AController contains a pointer to
	 * the next controller so this basically returns a linked list of controllers
	 * associated with the world.
	 *
	 * @return first controller
	 */
	AController* GetFirstController() const;

	/**
	 * Returns the first pawn. Note that APawn contains a pointer to
	 * the next pawn so this basically returns a linked list of pawns
	 * associated with the world.
	 *
	 * @return first pawn
	 */
	APawn* GetFirstPawn() const;

	/**
	 * Returns the default brush.
	 *
	 * @return default brush
	 */
	ABrush* GetBrush() const;

	/**
	 * Returns whether game has already begun play.
	 *
	 * @return TRUE if game has already started, FALSE otherwise
	 */
	UBOOL HasBegunPlay() const;

	/**
	 * Returns whether gameplay has already begun and we are not associating a level
	 * with the world.
	 *
	 * @return TRUE if game has already started and we're not associating a level, FALSE otherwise
	 */
	UBOOL HasBegunPlayAndNotAssociatingLevel() const;

	/**
	 * Returns time in seconds since world was brought up for play, IS stopped when game pauses.
	 *
	 * @return time in seconds since world was brought up for play
	 */
	FLOAT GetTimeSeconds() const;

	/**
	* Returns time in seconds since world was brought up for play, does NOT stop when game pauses.
	*
	* @return time in seconds since world was brought up for play
	*/
	FLOAT GetRealTimeSeconds() const;

	/**
	 * Returns the frame delta time in seconds adjusted by e.g. time dilation.
	 *
	 * @return frame delta time in seconds adjusted by e.g. time dilation
	 */
	FLOAT GetDeltaSeconds() const;

	/**
	 * Returns the default physics volume and creates it if necessary.
	 * 
	 * @return default physics volume
	 */
	APhysicsVolume* GetDefaultPhysicsVolume() const;

	/**
	 * Returns the physics volume a given actor is in.
	 *
	 * @param	Location
	 * @param	Actor
	 * @param	bUseTouch
	 *
	 * @return physics volume given actor is in.
	 */
	APhysicsVolume* GetPhysicsVolume(FVector Location, AActor* Actor, UBOOL bUseTouch) const;

	/**
	 * Returns the current (or specified) level's Kismet sequence
	 *
	 * @param	OwnerLevel		the level to get the sequence from - must correspond to one of the levels in GWorld's Levels array;
	 *							thus, only applicable when editing a multi-level map.  Defaults to the level currently being edited.
	 *
	 * @return	a pointer to the sequence located at the specified element of the specified level's list of sequences, or
	 *			NULL if that level doesn't exist or the index specified is invalid for that level's list of sequences
	 */
	USequence* GetGameSequence( ULevel* OwnerLevel=NULL ) const;

	/**
	 * Sets the current (or specified) level's kismet sequence to the sequence specified.
	 *
	 * @param	GameSequence	the sequence to add to the level
	 * @param	OwnerLevel		the level to add the sequence to - must correspond to one of the levels in GWorld's Levels array;
	 *							thus, only applicable when editing a multi-level map.  Defaults to the level currently being edited.
	 */
	void SetGameSequence( USequence* GameSequence, ULevel* OwnerLevel=NULL );

	/**
	 * Returns the AWorldInfo actor associated with this world.
	 *
	 * @return AWorldInfo actor associated with this world
	 */
	AWorldInfo* GetWorldInfo() const;

	/**
	 * Returns the current levels BSP model.
	 *
	 * @return BSP UModel
	 */
	UModel* GetModel() const;

	/**
	 * Returns the Z component of the current world gravity.
	 *
	 * @return Z component of current world gravity.
	 */
	FLOAT GetGravityZ() const;

	/**
	 * Returns the Z component of the default world gravity.
	 *
	 * @return Z component of the default world gravity.
	 */
	FLOAT GetDefaultGravityZ() const;

	/**
	 * Returns the Z component of the current world gravity scaled for rigid body physics.
	 *
	 * @return Z component of the current world gravity scaled for rigid body physics.
	 */
	FLOAT GetRBGravityZ() const;

	/**
	 * Returns the name of the current map, taking into account using a dummy persistent world
	 * and loading levels into it via PrepareMapChange.
	 *
	 * @return	name of the current map
	 */
	const FString GetMapName() const;

	/**
	 * Adds a level's navigation list to the world's.
	 */
	void AddLevelNavList( class ULevel *Level, UBOOL bDebugNavList = FALSE );

	/**
	 * Removes a level's navigation list from the world's.
	 */
	void RemoveLevelNavList( class ULevel *Level, UBOOL bDebugNavList = FALSE );

	/**
	 * Resets the world navigation point list (by nuking the head).
	 */
	void ResetNavList();

	/**
	 * Searches for a navigation point by guid.
	 */
	ANavigationPoint* FindNavByGuid(FGuid &Guid);

	/**
	 * Inserts the passed in controller at the front of the linked list of controllers.
	 *
	 * @param	Controller	Controller to insert, use NULL to clear list
	 */
	void AddController( AController* Controller );
	
	/**
	 * Removes the passed in controller from the linked list of controllers.
	 *
	 * @param	Controller	Controller to remove
	 */
	void RemoveController( AController* Controller );

	/**
	 * Inserts the passed in pawn at the front of the linked list of pawns.
	 *
	 * @param	Pawn	Pawn to insert, use NULL to clear list
	 */
	void AddPawn( APawn* Pawn );
	
	/**
	 * Removes the passed in pawn from the linked list of pawns.
	 *
	 * @param	Pawn	Pawn to remove
	 */
	void RemovePawn( APawn* Pawn );

	/**
	 * Returns whether the passed in actor is part of any of the loaded levels actors array.
	 *
	 * @param	Actor	Actor to check whether it is contained by any level
	 *	
	 * @return	TRUE if actor is contained by any of the loaded levels, FALSE otherwise
	 */
	UBOOL ContainsActor( AActor* Actor );

	/**
	 * Returns whether audio playback is allowed for this scene.
	 *
	 * @return TRUE if current world is GWorld, FALSE otherwise
	 */
	virtual UBOOL AllowAudioPlayback();

	/**
	 * Serialize function.
	 *
	 * @param Ar	Archive to use for serialization
	 */
	void Serialize( FArchive& Ar );

	/**
	 * Destroy function, cleaning up world components, delete octree, physics scene, ....
	 */
	void FinishDestroy();
	
	/**
	 * Clears all level components and world components like e.g. line batcher.
	 */
	void ClearComponents();

	/**
	 * Updates world components like e.g. line batcher and all level components.
	 *
	 * @param	bCurrentLevelOnly		If TRUE, affect only the current level.
	 */
	void UpdateComponents(UBOOL bCurrentLevelOnly);

	/**
	 * Updates all cull distance volumes.
	 */
	void UpdateCullDistanceVolumes();

	/**
	 * Cleans up components, streaming data and assorted other intermediate data.
	 * @param bSessionEnded whether to notify the viewport that the game session has ended
	 */
	void CleanupWorld(UBOOL bSessionEnded = TRUE);
	
	/**
	 * Invalidates the cached data used to render the levels' UModel.
	 *
	 * @param	bCurrentLevelOnly		If TRUE, affect only the current level.
	 */
	void InvalidateModelGeometry(UBOOL bCurrentLevelOnly);

	/**
	 * Discards the cached data used to render the levels' UModel.  Assumes that the
	 * faces and vertex positions haven't changed, only the applied materials.
	 *
	 * @param	bCurrentLevelOnly		If TRUE, affect only the current level.
	 */
	void InvalidateModelSurface(UBOOL bCurrentLevelOnly);

	/**
	 * Commits changes made to the surfaces of the UModels of all levels.
	 */
	void CommitModelSurfaces();

	/**
	 * Fixes up any cross level paths. Called from e.g. UpdateLevelStreaming when associating a new level with the world.
	 *
	 * @param	bIsRemovingLevel	Whether we are adding or removing a level
	 * @param	Level				Level to add or remove
	 */
	void FixupCrossLevelPaths( UBOOL bIsRemovingLevel, ULevel* Level );

	/**
	 * Associates the passed in level with the world. The work to make the level visible is spread across several frames and this
	 * function has to be called till it returns TRUE for the level to be visible/ associated with the world and no longer be in
	 * a limbo state.
	 *
	 * @param StreamingLevel	Level streaming object whose level we should add
 	 * @param RelativeOffset	Relative offset to move actors
	 */
	void AddToWorld( ULevelStreaming* StreamingLevel, const FVector& RelativeOffset );

	/** 
	 * Dissociates the passed in level from the world. The removal is blocking.
	 *
	 * @param LevelStreaming	Level streaming object whose level we should remove
	 */
	void RemoveFromWorld( ULevelStreaming* StreamingLevel );

	/**
	 * Updates the world based on the current view location of the player and sets level LODs accordingly.	 
	 *
	 * @param ViewFamily	Optional collection of views to take into account
	 */
	void UpdateLevelStreaming( FSceneViewFamily* ViewFamily = NULL );

	/**
	 * Flushs level streaming in blocking fashion and returns when all levels are loaded/ visible/ hidden
	 * so further calls to UpdateLevelStreaming won't do any work unless state changes. Basically blocks
	 * on all async operation like updating components.
	 *
	 * @param ViewFamily				Optional collection of views to take into account
	 * @param bOnlyFlushVisibility		Whether to only flush level visibility operations (optional)
	 */
	void FlushLevelStreaming( FSceneViewFamily* ViewFamily = NULL, UBOOL bOnlyFlushVisibility = FALSE );

	/**
	 * Returns whether the level streaming code is allowed to issue load requests.
	 *
	 * @return TRUE if level load requests are allowed, FALSE otherwise.
	 */
	UBOOL AllowLevelLoadRequests();
	
	/**
	 * Initializes the world, associates the persistent level and sets the proper zones.
	 */
	void Init();

	/**
	 * Static function that creates a new UWorld and replaces GWorld with it.
	 */
	static void CreateNew();

	/**
	 * Called from within SavePackage on the passed in base/ root. The return value of this function will be passed to
	 * PostSaveRoot. This is used to allow objects used as base to perform required actions before saving and cleanup
	 * afterwards.
	 * @param Filename: Name of the file being saved to (includes path)
	 *
	 * @return	Whether PostSaveRoot needs to perform internal cleanup
	 */
	virtual UBOOL PreSaveRoot(const TCHAR* Filename);
	/**
	 * Called from within SavePackage on the passed in base/ root. This function is being called after the package
	 * has been saved and can perform cleanup.
	 *
	 * @param	bCleanupIsRequired	Whether PreSaveRoot dirtied state that needs to be cleaned up
	 */
	virtual void PostSaveRoot( UBOOL bCleanupIsRequired );

	/**
	 * Saves this world.  Safe to call on non-GWorld worlds.
	 *
	 * @param	Filename					The filename to use.
	 * @param	bForceGarbageCollection		Whether to force a garbage collection before saving.
	 * @param	bAutosaving					If TRUE, don't perform optional caching tasks.
	 * @param	bPIESaving					If TRUE, don't perform tasks that will conflict with editor state.
	 */
	UBOOL SaveWorld( const FString& Filename, UBOOL bForceGarbageCollection, UBOOL bAutosaving, UBOOL bPIESaving );

	/**
	 *  Interface to allow WorldInfo to request immediate garbage collection
	 */
	void PerformGarbageCollection();

	/**
	 * Called after the object has been serialized. Currently ensures that CurrentLevel gets initialized as
	 * it is required for saving secondary levels in the multi- level editing case.
	 */
	virtual void PostLoad();

	/**
	 * Update the level after a variable amount of time, DeltaSeconds, has passed.
	 * All child actors are ticked after their owners have been ticked.
	 */
	void Tick( ELevelTick TickType, FLOAT DeltaSeconds );

	void TickNetClient( FLOAT DeltaSeconds );
	void TickNetServer( FLOAT DeltaSeconds );
	INT ServerTickClients( FLOAT DeltaSeconds );

	/** Tick demo recording */
	INT TickDemoRecord( FLOAT DeltaSeconds );

	/** Tick demo playback */
	INT TickDemoPlayback( FLOAT DeltaSeconds );

	/**
	 * Ticks any of our async worker threads (notifies them of their work to do)
	 *
	 * @param DeltaSeconds - The elapsed time between frames
	 */
	void TickAsyncWork(FLOAT DeltaSeconds);

	/**
	 * Waits for any async work that needs to be done before continuing
	 */
	void WaitForAsyncWork(void);

	/**
	 * Issues level streaming load/unload requests based on whether
	 * local players are inside/outside level streaming volumes.
	 *
	 * @param OverrideViewLocation Optional position used to override the location used to calculate current streaming volumes
	 */
	void ProcessLevelStreamingVolumes(FVector* OverrideViewLocation=NULL);

	/**
	 * Stub impl -- prevents UWorld objects from being transacted.  appErrorf's.
	 */
	void Modify( UBOOL bAlwaysMarkDirty=FALSE );

	/**
	 * Transacts the specified level -- the correct way to modify a level
	 * as opposed to calling Level->Modify.
	 */
	void ModifyLevel(ULevel* Level);

	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void ShrinkLevel();
	UBOOL Listen( FURL InURL, FString& Error );
	/**
	 * Builds the master package map
	 */
	void BuildServerMasterMap(void);
	UBOOL IsClient();
	UBOOL IsServer();
	UBOOL IsPaused();
	UBOOL MoveActor( AActor *Actor, const FVector& Delta, const FRotator& NewRotation, DWORD MoveFlags, FCheckResult &Hit );
	UBOOL FarMoveActor( AActor* Actor, const FVector& DestLocation, UBOOL Test=0, UBOOL bNoCheck=0, UBOOL bAttachedMove=0 );

	/**
	 * Wrapper for DestroyActor() that should be called in the editor.
	 *
	 * @param	bShouldModifyLevel		If TRUE, Modify() the level before removing the actor.
	 */
	UBOOL EditorDestroyActor( AActor* Actor, UBOOL bShouldModifyLevel );

	/**
	 * Removes the actor from its level's actor list and generally cleans up the engine's internal state.
	 * What this function does not do, but is handled via garbage collection instead, is remove references
	 * to this actor from all other actors, and kill the actor's resources.  This function is set up so that
	 * no problems occur even if the actor is being destroyed inside its recursion stack.
	 *
	 * @param	ThisActor				Actor to remove.
	 * @param	bNetForce				[opt] Ignored unless called during play.  Default is FALSE.
	 * @param	bShouldModifyLevel		[opt] If TRUE, Modify() the level before removing the actor.  Default is TRUE.
	 * @return							TRUE if destroy, FALSE if actor couldn't be destroyed.
	 */
	UBOOL DestroyActor( AActor* Actor, UBOOL bNetForce=FALSE, UBOOL bShouldModifyLevel=TRUE );

	/**
	 * Removes the passed in actor from the actor lists. Please note that the code actually doesn't physically remove the
	 * index but rather clears it so other indices are still valid and the actors array size doesn't change.
	 *
	 * @param	Actor					Actor to remove.
	 * @param	bShouldModifyLevel		If TRUE, Modify() the level before removing the actor if in the editor.
	 */
	void RemoveActor( AActor* Actor, UBOOL bShouldModifyLevel );

	AActor* SpawnActor( UClass* Class, FName InName=NAME_None, const FVector& Location=FVector(0,0,0), const FRotator& Rotation=FRotator(0,0,0), AActor* Template=NULL, UBOOL bNoCollisionFail=0, UBOOL bRemoteOwned=0, AActor* Owner=NULL, APawn* Instigator=NULL, UBOOL bNoFail=0 );
	ABrush*	SpawnBrush();
	APlayerController* SpawnPlayActor( UPlayer* Player, ENetRole RemoteRole, const FURL& URL, FString& Error );

	/**
	 * Destroys actors marked as bKillDuringLevelTransition. 
	 */
	void CleanUpBeforeLevelTransition();

	UBOOL FindSpot( const FVector& Extent, FVector& Location, UBOOL bUseComplexCollision );
	UBOOL CheckSlice( FVector& Location, const FVector& Extent, INT& bKeepTrying );
	UBOOL CheckEncroachment( AActor* Actor, FVector TestLocation, FRotator TestRotation, UBOOL bTouchNotify );
	UBOOL SinglePointCheck( FCheckResult& Hit, const FVector& Location, const FVector& Extent, DWORD TraceFlags );
	UBOOL EncroachingWorldGeometry( FCheckResult& Hit, const FVector& Location, const FVector& Extent, UBOOL bUseComplexCollision=FALSE );
	UBOOL SingleLineCheck( FCheckResult& Hit, AActor* SourceActor, const FVector& End, const FVector& Start, DWORD TraceFlags, const FVector& Extent=FVector(0,0,0), ULightComponent* SourceLight = NULL );
	FCheckResult* MultiPointCheck( FMemStack& Mem, const FVector& Location, const FVector& Extent, DWORD TraceFlags );
	FCheckResult* MultiLineCheck( FMemStack& Mem, const FVector& End, const FVector& Start, const FVector& Size, DWORD TraceFlags, AActor* SourceActor, ULightComponent* SourceLight = NULL );
	UBOOL BSPLineCheck(	FCheckResult& Hit, AActor* Owner, const FVector& End, const FVector& Start, const FVector& Extent, DWORD TraceFlags );
	UBOOL BSPFastLineCheck( const FVector& End, const FVector& Start );
	UBOOL BSPPointCheck( FCheckResult &Result, AActor *Owner, const FVector& Location, const FVector& Extent );

	void InitWorldRBPhys();
	void TermWorldRBPhys();
	void TickWorldRBPhys(FLOAT DeltaSeconds);
	/**
	 * Waits for the physics scene to be done processing
	 */
	void WaitWorldRBPhys();

	/** Clean up any physics engine resources. Deferred to avoid issues when exiting game. */
	void DeferredRBResourceCleanup();

	// SetGameInfo - Spawns gameinfo for the level.
	void SetGameInfo(const FURL& InURL);

	/** BeginPlay - Begins gameplay in the level.
	 * @param InURL commandline URL
	 * @param bResetTime (optional) whether the WorldInfo's TimeSeconds should be reset to zero
	 */
	void BeginPlay(const FURL& InURL, UBOOL bResetTime = TRUE);

	/** verifies that the client has loaded or can load the package with the specified information
	 * if found, sets the Info's Parent to the package and notifies the server of our generation of the package
	 * if not, handles downloading the package, skipping it, or disconnecting, depending on the requirements of the package
	 * @param Info the info on the package the client should have
	 * @return TRUE if we're done verifying this package, FALSE if we're not done yet (because i.e. async loading is in progress)
	 */
	UBOOL VerifyPackageInfo(FPackageInfo& Info);

	// FNetworkNotify interface.
	EAcceptConnection NotifyAcceptingConnection();
	void NotifyAcceptedConnection( class UNetConnection* Connection );
	UBOOL NotifyAcceptingChannel( class UChannel* Channel );
	UWorld* NotifyGetWorld() {return this;}
	void NotifyReceivedText( UNetConnection* Connection, const TCHAR* Text );
	void NotifyReceivedFile( UNetConnection* Connection, INT PackageIndex, const TCHAR* Error, UBOOL Skipped );
	UBOOL NotifySendingFile( UNetConnection* Connection, FGuid GUID );
	void NotifyProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds );

	void WelcomePlayer( UNetConnection* Connection, const TCHAR* Optional=TEXT("") );

	/** verifies that the navlist pointers have not been corrupted.
	 *	helps track down errors w/ dynamic pathnodes being added/removed from the navigation list
	 */
	static void VerifyNavList( const TCHAR* DebugTxt, ANavigationPoint* IgnoreNav = NULL );

	/**
	 * Sets the number of frames to delay Streaming Volume updating, 
	 * useful if you preload a bunch of levels but the camera hasn't caught up yet 
	 */
	void DelayStreamingVolumeUpdates(INT InFrameDelay)
	{
		StreamingVolumeUpdateDelay = InFrameDelay;
	}
};

/** Global UWorld pointer */
extern UWorld* GWorld;

/** class that encapsulates seamless world traveling */
class FSeamlessTravelHandler
{
private:
	/** set when a transition is in progress */
	UBOOL bTransitionInProgress;
	/** URL we're traveling to */
	FURL PendingTravelURL;
	/** whether or not we've transitioned to the entry level and are now moving on to the specified map */
	UBOOL bSwitchedToDefaultMap;
	/** set to the loaded package once loading is complete. Transition to it is performed in the next tick where it's safe to perform the required operations */
	UObject* LoadedPackage;

	/** callback sent to async loading code to inform us when the level package is complete */
	static void SeamlessTravelLoadCallback(UObject* LevelPackage, void* Handler);

public:
	FSeamlessTravelHandler()
	: bTransitionInProgress(FALSE), bSwitchedToDefaultMap(FALSE), LoadedPackage(NULL)
	{}

	/** starts traveling to the given URL. The required packages will be loaded async and Tick() will perform the transition once we are ready
	 * @param InURL the URL to travel to
	 * @return whether or not we succeeded in starting the travel
	 */
	UBOOL StartTravel(const FURL& InURL);

	/** returns whether a transition is already in progress */
	FORCEINLINE UBOOL IsInTransition()
	{
		return bTransitionInProgress;
	}

	inline FString GetDestinationMapName()
	{
		return (IsInTransition() ? FFilename(PendingTravelURL.Map).GetBaseFilename() : TEXT(""));
	}

	/** cancels transition in progress */
	void CancelTravel();

	/** ticks the transition; handles performing the world switch once the required packages have been loaded */
	void Tick();
};
/** global travel handler */
extern FSeamlessTravelHandler GSeamlessTravelHandler;

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
