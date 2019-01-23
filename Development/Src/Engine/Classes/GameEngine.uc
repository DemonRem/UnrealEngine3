//=============================================================================
// GameEngine: The game subsystem.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class GameEngine extends Engine
	native(GameEngine)
	config(Engine)
	transient;

// URL structure.
struct transient native URL
{
	var		string			Protocol;	// Protocol, i.e. "unreal" or "http".
	var		string			Host;		// Optional hostname, i.e. "204.157.115.40" or "unreal.epicgames.com", blank if local.
	var		int				Port;		// Optional host port.
	var		string			Map;		// Map name, i.e. "SkyCity", default is "Index".
	var		array<string>	Op;			// Options.
	var		string			Portal;		// Portal to enter through, default is "".
	var		int 			Valid;
structcpptext
{

	// Statics.
	static FString DefaultProtocol;
	static FString DefaultName;
	static FString DefaultMap;
	static FString DefaultLocalMap;
	static FString DefaultTransitionMap; // map used as in-between for seamless travel
	static FString DefaultHost;
	static FString DefaultPortal;
	static FString DefaultMapExt;
	static FString DefaultSaveExt;
	static INT DefaultPort;
	static UBOOL bDefaultsInitialized;

	// Constructors.
	FURL( const TCHAR* Filename=NULL );
	FURL( FURL* Base, const TCHAR* TextURL, ETravelType Type );
	static void StaticInit();
	static void StaticExit();

	// Functions.
	UBOOL IsInternal() const;
	UBOOL IsLocalInternal() const;
	UBOOL HasOption( const TCHAR* Test ) const;
	const TCHAR* GetOption( const TCHAR* Match, const TCHAR* Default ) const;
	void LoadURLConfig( const TCHAR* Section, const TCHAR* Filename=NULL );
	void SaveURLConfig( const TCHAR* Section, const TCHAR* Item, const TCHAR* Filename=NULL ) const;
	void AddOption( const TCHAR* Str );
	void RemoveOption( const TCHAR* Key, const TCHAR* Section = NULL, const TCHAR* Filename = NULL);
	FString String( UBOOL FullyQualified=0 ) const;
	friend FArchive& operator<<( FArchive& Ar, FURL& U );

	// Operators.
	UBOOL operator==( const FURL& Other ) const;
}
};

var			PendingLevel	GPendingLevel;
/** URL the last time we travelled */
var			URL				LastURL;
/** last server we connected to (for "reconnect" command) */
var URL LastRemoteURL;
var config	array<string>	ServerActors;

var			string			TravelURL;
var			byte			TravelType;

/** The singleton online interface for all game code to use */
var OnlineSubsystem OnlineSubsystem;

/**
 * Array of package/ level names that need to be loaded for the pending map change. First level in that array is
 * going to be made a fake persistent one by using ULevelStreamingPersistent.
 */
var const	array<name>		LevelsToLoadForPendingMapChange;
/** Array of already loaded levels. The ordering is arbitrary and depends on what is already loaded and such.	*/
var	const	array<level>	LoadedLevelsForPendingMapChange;
/** Human readable error string for any failure during a map change request. Empty if there were no failures.	*/
var const	string			PendingMapChangeFailureDescription;
/** If TRUE, commit map change the next frame.																	*/
var const	bool			bShouldCommitPendingMapChange;
/** Whether to skip triggering the level startup event on the next map commit.									*/
var const	bool			bShouldSkipLevelStartupEventOnMapCommit;
/** Whether to skip triggering the level begin event on the next map commit.									*/
var const	bool			bShouldSkipLevelBeginningEventOnMapCommit;
/** Whether to enable framerate smoothing.																		*/
var config	bool			bSmoothFrameRate;
/** Maximum framerate to smooth. Code will try to not go over via waiting.										*/
var config	float			MaxSmoothedFrameRate;
/** Minimum framerate smoothing will kick in.																	*/
var config	float			MinSmoothedFrameRate;

/** level streaming updates that should be applied immediately after committing the map change */
struct native LevelStreamingStatus
{
	var name PackageName;
	var bool bShouldBeLoaded, bShouldBeVisible;

	structcpptext
	{
		/** Constructors */
		FLevelStreamingStatus(FName InPackageName, UBOOL bInShouldBeLoaded, UBOOL bInShouldBeVisible)
		: PackageName(InPackageName), bShouldBeLoaded(bInShouldBeLoaded), bShouldBeVisible(bInShouldBeVisible)
		{}
		FLevelStreamingStatus()
		{}
    		FLevelStreamingStatus(EEventParm)
		{
			appMemzero(this, sizeof(FLevelStreamingStatus));
		}
	}
};
var const array<LevelStreamingStatus> PendingLevelStreamingStatusUpdates;

/** Handles to object references; used by the engine to e.g. the prevent objects from being garbage collected.	*/
var const	array<ObjectReferencer>	ObjectReferencers;

enum EFullyLoadPackageType
{
	/** Load the packages when the map in Tag is loaded */
	FULLYLOAD_Map,
	/** Load the packages before the game class in Tag is loaded. The Game name MUST be specified in the URL (game=Package.GameName). Useful for loading packages needed to load the game type (a DLC game type, for instance) */
	FULLYLOAD_Game_PreLoadClass,
	/** Load the packages after the game class in Tag is loaded. Will work no matter how game is specified in UWorld::SetGameInfo. Useful for modifying shipping gametypes by loading more packages (mutators, for instance) */
	FULLYLOAD_Game_PostLoadClass,
};

/** Struct to help hold information about packages needing to be fully-loaded for DLC, etc */
struct native FullyLoadedPackagesInfo
{
	/** When to load these packages */
	var EFullyLoadPackageType FullyLoadType;
	
	/** When this map or gametype is loaded, the packages in the following array will be loaded and added to root, then removed from root when map is unloaded */
	var string Tag;

	/** The list of packages that will be fully loaded when the above Map is loaded */
	var array<name> PackagesToLoad;

	/** List of objects that were loaded, for faster cleanup */
	var array<object> LoadedObjects;
};

/** A list of tag/array pairs that is used at LoadMap time to fully load packages that may be needed for the map/game with DLC, but we can't use DynamicLoadObject to load from the packages */
var array<FullyLoadedPackagesInfo> PackagesToFullyLoad;


/** Returns the global online subsytem pointer. This will be null for PIE */
native static final noexport function OnlineSubsystem GetOnlineSubsystem();

cpptext
{

	// Constructors.
	UGameEngine();

	/**
	 * Redraws all viewports.
	 */
	void RedrawViewports();

	// UObject interface.
	void FinishDestroy();

	// UEngine interface.
	void Init();

	/**
	 * Called at shutdown, just before the exit purge.
	 */
	virtual void PreExit();

	void Tick( FLOAT DeltaSeconds );
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void SetClientTravel( const TCHAR* NextURL, ETravelType TravelType );
	virtual FLOAT GetMaxTickRate( FLOAT DeltaTime, UBOOL bAllowFrameRateSmoothing = TRUE );
	INT ChallengeResponse( INT Challenge );
	void SetProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds );

#if !FINAL_RELEASE
	/**
	 * Handles freezing/unfreezing of rendering
	 */
	virtual void ProcessToggleFreezeCommand();

	/**
	 * Handles frezing/unfreezing of streaming
	 */
	 virtual void ProcessToggleFreezeStreamingCommand();
#endif

	// UGameEngine interface.
	virtual UBOOL Browse( FURL URL, FString& Error );
	virtual UBOOL LoadMap( const FURL& URL, class UPendingLevel* Pending, FString& Error );
	virtual void CancelPending();
	virtual void UpdateConnectingMessage();

	/**
	 * Spawns all of the registered server actors
	 */
	virtual void SpawnServerActors(void);

	/**
	 * Returns the online subsystem object. Returns null if GEngine isn't a
	 * game engine
	 */
	static UOnlineSubsystem* GetOnlineSubsystem(void);

	/**
	 * Creates the online subsystem that was specified in UEngine's
	 * OnlineSubsystemClass. This function is virtual so that licensees
	 * can provide their own version without modifying Epic code.
	 */
	virtual void InitOnlineSubsystem(void);


	// Async map change/ persistent level transition code.

	/**
	 * Prepares the engine for a map change by pre-loading level packages in the background.
	 *
	 * @param	LevelNames	Array of levels to load in the background; the first level in this
	 *						list is assumed to be the new "persistent" one.
	 *
	 * @return	TRUE if all packages were in the package file cache and the operation succeeded,
	 *			FALSE otherwise. FALSE as a return value also indicates that the code has given
	 *			up.
	 */
	UBOOL PrepareMapChange(const TArray<FName>& LevelNames);

	/**
	 * Returns the failure description in case of a failed map change request.
	 *
	 * @return	Human readable failure description in case of failure, empty string otherwise
	 */
	FString GetMapChangeFailureDescription();

	/**
	 * Returns whether we are currently preparing for a map change or not.
	 *
	 * @return TRUE if we are preparing for a map change, FALSE otherwise
	 */
	UBOOL IsPreparingMapChange();

	/**
	 * Returns whether the prepared map change is ready for commit having called.
	 *
	 * @return TRUE if we're ready to commit the map change, FALSE otherwise
	 */
	UBOOL IsReadyForMapChange();

	/**
	 * Finalizes the pending map change that was being kicked off by PrepareMapChange.
	 *
	 * @param bShouldSkipLevelStartupEvent TRUE if this function NOT fire the SeqEvent_LevelBStartup event.
	 * @param bShouldSkipLevelBeginningEvent TRUE if this function NOT fire the SeqEvent_LevelBeginning event. Useful for when skipping to the middle of a map by savegame or something
	 *
	 * @return	TRUE if successful, FALSE if there were errors (use GetMapChangeFailureDescription
	 *			for error description)
	 */
	UBOOL CommitMapChange(UBOOL bShouldSkipLevelStartupEvent=FALSE, UBOOL bShouldSkipLevelBeginningEvent=FALSE);

	/**
	 * Commit map change if requested and map change is pending. Called every frame.
	 */
	void ConditionalCommitMapChange();

	/**
	 * Cancels pending map change.
	 */
	void CancelPendingMapChange();

	/**
	 * Adds a map/package array pair for pacakges to load at LoadMap
	 *
	 * @param FullyLoadType When to load the packages (based on map, gametype, etc)
	 * @param Tag Map/game for which the packages need to be loaded
	 * @param Packages List of package names to fully load when the map is loaded
	 * @param bLoadPackagesForCurrentMap If TRUE, the packages for the currently loaded map will be loaded now
	 */
	void AddPackagesToFullyLoad(EFullyLoadPackageType FullyLoadType, const FString& Tag, const TArray<FName>& Packages, UBOOL bLoadPackagesForCurrentMap);

	/**
	 * Empties the PerMapPackages array, and removes any currently loaded packages from the Root
	 */
	void CleanupAllPackagesToFullyLoad();

	/**
	 * Loads the PerMapPackages for the given map, and adds them to the RootSet
	 *
	 * @param FullyLoadType When to load the packages (based on map, gametype, etc)
	 * @param Tag Name of the map/game to load packages for
	 */
	void LoadPackagesFully(EFullyLoadPackageType FullyLoadType, const FString& Tag);

	/**
	 * Removes the PerMapPackages from the RootSet
	 *
	 * @param FullyLoadType When to load the packages (based on map, gametype, etc)
	 * @param Tag Name of the map/game to cleanup packages for
	 */
	void CleanupPackagesToFullyLoad(EFullyLoadPackageType FullyLoadType, const FString& Tag);
}
