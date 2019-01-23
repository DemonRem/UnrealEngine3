//=============================================================================
// Engine: The base class of the global application object classes.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class Engine extends Subsystem
	native(GameEngine)
	abstract
	config(Engine)
	transient;

// Fonts.
var private Font	TinyFont;
var globalconfig string TinyFontName;

var private Font	SmallFont;
var globalconfig string SmallFontName;

var private Font	MediumFont;
var globalconfig string MediumFontName;

var private Font	LargeFont;
var globalconfig string LargeFontName;

/** Any additional fonts that script may use without hard-referencing the font. */
var private array<Font>			AdditionalFonts;
var globalconfig array<string>	AdditionalFontNames;

/** The class to use for the game console. */
var class<Console> ConsoleClass;
var globalconfig string ConsoleClassName;

/** The class to use for the game viewport client. */
var class<GameViewportClient> GameViewportClientClass;
var globalconfig string GameViewportClientClassName;

/** The class to use for managing the global data stores */
var	class<DataStoreClient> DataStoreClientClass;
var	globalconfig string DataStoreClientClassName;

/** The class to use for local players. */
var class<LocalPlayer> LocalPlayerClass;
var config string LocalPlayerClassName;

/** The material used when no material is explicitly applied. */
var Material	DefaultMaterial;
var globalconfig string DefaultMaterialName;

/** The material used to render wireframe meshes. */
var Material	WireframeMaterial;
var globalconfig string WireframeMaterialName;

/** A textured material with an instance parameter for the texture. */
var Material EmissiveTexturedMaterial;
var globalconfig string EmissiveTexturedMaterialName;

/** A translucent material used to render things in geometry mode. */
var Material	GeomMaterial;
var globalconfig string GeomMaterialName;

/** The default fog volume material */
var Material	DefaultFogVolumeMaterial;
var globalconfig string DefaultFogVolumeMaterialName;

/** Material used for drawing a tick mark. */
var Material	TickMaterial;
var globalconfig string TickMaterialName;

/** Material used for drawing a cross mark. */
var Material	CrossMaterial;
var globalconfig string CrossMaterialName;

/** Material used for visualizing level membership in lit viewport modes. */
var Material	LevelColorationLitMaterial;
var globalconfig string LevelColorationLitMaterialName;

/** Material used for visualizing level membership in unlit viewport modes. */
var Material	LevelColorationUnlitMaterial;
var globalconfig string LevelColorationUnlitMaterialName;

/** Material used to indicate that the associated BSP surface should be removed. */
var Material	RemoveSurfaceMaterial;
var globalconfig string RemoveSurfaceMaterialName;

/** The colors used to render light complexity. */
var globalconfig array<color> LightComplexityColors;

/** The colors used to render shader complexity. */
var globalconfig array<color> ShaderComplexityColors;

/** When true, pixel shader complexity is shown, otherwise vertex shader complexity is shown in the shader complexity viewmode. */
var globalconfig bool bUsePixelShaderComplexity;

/** 
* When true, pixel shader complexity is cumulative, otherwise only the last pixel drawn contributes complexity. 
* Has no effect if bUsePixelShaderComplexity is false
*/
var globalconfig bool bUseAdditiveComplexity;

/** 
* Complexity limits for the various complexity viewmode combinations.
* These limits are used to map instruction counts to ShaderComplexityColors.
*/
var globalconfig float MaxPixelShaderAdditiveComplexityCount;
var globalconfig float MaxPixelShaderOpaqueComplexityCount;
var globalconfig float MaxVertexShaderComplexityCount;

/** Range for the texture density viewmode. */
var globalconfig float MinTextureDensity;
var globalconfig float IdealTextureDensity;
var globalconfig float MaxTextureDensity;

/** A material used to render the sides of the builder brush/volumes/etc. */
var Material	EditorBrushMaterial;
var globalconfig string EditorBrushMaterialName;

/** PhysicalMaterial to use if none is defined for a particular object. */
var	PhysicalMaterial	DefaultPhysMaterial;
var globalconfig string DefaultPhysMaterialName;

/** The material used when terrain compilation is too complex. */
var Material	TerrainErrorMaterial;
var globalconfig string TerrainErrorMaterialName;
var globalconfig int TerrainMaterialMaxTextureCount;

/** OnlineSubsystem class to use for netplay */
var	class<OnlineSubsystem> OnlineSubsystemClass;
var globalconfig string DefaultOnlineSubsystemName;

/** Default engine post process chain used for the game and main editor view */
var PostProcessChain DefaultPostProcess;
var config string DefaultPostProcessName;

/** post process chain used for skeletal mesh thumbnails */
var PostProcessChain ThumbnailSkeletalMeshPostProcess;
var config string ThumbnailSkeletalMeshPostProcessName;

/** post process chain used for particle system thumbnails */
var PostProcessChain ThumbnailParticleSystemPostProcess;
var config string ThumbnailParticleSystemPostProcessName;

/** post process chain used for material thumbnails */
var PostProcessChain ThumbnailMaterialPostProcess;
var config string ThumbnailMaterialPostProcessName;

/** post process chain used for rendering the UI */
var PostProcessChain DefaultUIScenePostProcess;
var config string DefaultUIScenePostProcessName;

/** Material used for drawing meshes when their collision is missing. */
var Material	DefaultUICaretMaterial;
var globalconfig string DefaultUICaretMaterialName;

/** Material used for visualizing the reflection scene captures on a surface */
var Material	SceneCaptureReflectActorMaterial;
var globalconfig string SceneCaptureReflectActorMaterialName;

/** Material used for visualizing the cube map scene captures on a mesh */
var Material	SceneCaptureCubeActorMaterial;
var globalconfig string SceneCaptureCubeActorMaterialName;

/** Texture used to get random angles per-pixel by the Branching PCF implementation */
var Texture2D RandomAngleTexture;
var globalconfig string RandomAngleTextureName;

/** Time in seconds (game time) we should wait between purging object references to objects that are pending kill */
var(Settings) config float TimeBetweenPurgingPendingKillObjects;

// Variables.

/** Abstract interface to platform-specific subsystems */
var const client							Client;

/** Viewports for all players in all game instances (all PIE windows, for example) */
var init array<LocalPlayer>					GamePlayers;

/** the viewport representing the current game instance */
var const GameViewportClient				GameViewport;

/** Array of deferred command strings/ execs that get executed at the end of the frame */
var init array<string>	DeferredCommands;

var int TickCycles, GameCycles, ClientCycles;
var(Settings) config bool bUseSound;

/** Whether to use texture streaming. */
var(Settings) config bool bUseTextureStreaming;

/** Whether to allow background level streaming. */ 
var(Settings) config bool bUseBackgroundLevelStreaming;

/** Flag for completely disabling subtitles for localized sounds. */
var(Settings) config bool bSubtitlesEnabled;

/** Flag for forcibly disabling subtitles even if you try to turn them back on they will be off */
var(Settings) config bool bSubtitlesForcedOff;

/** 
 *	Flag for forcing terrain to be 'static' (MinTessellationLevel = MaxTesselationLevel)
 *	Game time only...
 */
var(Settings) config bool bForceStaticTerrain;

/** Global debug manager helper object that stores configuration and state used during development */
var const DebugManager			DebugManager;

/** Entry point for RemoteControl, the in-game UI for the exec system. */
var native pointer				RemoteControlExec{class FRemoteControlExec};

// Color preferences.
var(Colors) color
	C_WorldBox,
	C_BrushWire,
	C_AddWire,
	C_SubtractWire,
	C_SemiSolidWire,
	C_NonSolidWire,
	C_WireBackground,
	C_ScaleBoxHi,
	C_VolumeCollision,
	C_BSPCollision,
	C_OrthoBackground,
	C_Volume;

/** Fudge factor for tweaking the distance based miplevel determination */
var(Settings)	float			StreamingDistanceFactor;

/** Class name of the scout to use for path building */
var const config string ScoutClassName;

/**
 * A transition type.
 */
enum ETransitionType
{
	TT_None,
	TT_Paused,
	TT_Loading,
	TT_Saving,
	TT_Connecting,
	TT_Precaching
};

/** The current transition type. */
var ETransitionType TransitionType;

/** The current transition description text. */
var string TransitionDescription;

/** Level of detail range control for meshes */
var config		float					MeshLODRange;
/** Force to CPU skinning only for skeletal mesh rendering */
var	config		bool					bForceCPUSkinning;
/** Whether to use post processing effects or not */
var	config		bool					bUsePostProcessEffects;
/** whether to send Kismet warning messages to the screen (via PlayerController::ClientMessage()) */
var config bool bOnScreenKismetWarnings;
/** whether kismet logging is enabled. */
var config bool bEnableKismetLogging;
/** whether mature language is allowed **/
var config bool bAllowMatureLanguage;
/** Toggle VSM (Variance Shadow Map) usage for projected shadows */
var config bool bEnableVSMShadows;
/** Toggle Branching PCF implementation for projected shadows */
var config bool bEnableBranchingPCFShadows;
/** Radius, in shadowmap texels, of the filter disk */
var config float ShadowFilterRadius;
/** Depth bias that is applied in the depth pass for all types of projected shadows except VSM */
var config float DepthBias;
/** min dimensions (in texels) allowed for rendering shadow subject depths */
var config int MinShadowResolution;
/** max square dimensions (in texels) allowed for rendering shadow subject depths */
var config int MaxShadowResolution;
/** controls the rate at which mod shadows will fade as they approach their min shadow size */
var config float ModShadowFadeDistanceExponent;

/** Lights with radius below threshold will not cast shadow volumes. */
var config float ShadowVolumeLightRadiusThreshold;
/** Primitives with screen space percantage below threshold will not cast shadow volumes. */
var config float ShadowVolumePrimitiveScreenSpacePercentageThreshold;

/** Terrain collision viewing - If TRUE, overlay collion level else render it and overlay terrain. */
var config bool bRenderTerrainCollisionAsOverlay;

/** Do not use Ageia PhysX hardware */
var config bool bDisablePhysXHardwareSupport;

/** Material used for visualizing terrain collision. */
var Material TerrainCollisionMaterial;
var globalconfig string TerrainCollisionMaterialName;

/** The number of times to attempt the Begin*UP call before assuming the GPU is hosed	*/
var config int BeginUPTryCount;

/** Info about one note dropped in the map during PIE. */
struct native DropNoteInfo
{
	/** Location to create Note actor in edited level. */
	var vector	Location;
	/** Rotation to create Note actor in edited level. */
	var rotator	Rotation;
	/** Text to assign to Note actor in edited level. */
	var string	Comment;
};

/**  */
var transient array<DropNoteInfo>	PendingDroppedNotes;

/** Overridable class for cover mesh rendering in-game, used to get around the editoronly restrictions needed by the base CoverMeshComponent */
var globalconfig string DynamicCoverMeshComponentName;

cpptext
{
	// Constructors.
	UEngine();
	void StaticConstructor();

	// UObject interface.
	virtual void FinishDestroy();

	// UEngine interface.
	virtual void Init();

	/**
	 * Called at shutdown, just before the exit purge.
	 */
	virtual void PreExit() {}

	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Out=*GLog );
	virtual void Tick( FLOAT DeltaSeconds ) PURE_VIRTUAL(UEngine::Tick,);
	virtual void SetClientTravel( const TCHAR* NextURL, ETravelType TravelType ) PURE_VIRTUAL(UEngine::SetClientTravel,);
	virtual INT ChallengeResponse( INT Challenge );
	virtual FLOAT GetMaxTickRate( FLOAT /*DeltaTime*/, UBOOL bAllowFrameRateSmoothing = TRUE ) { return 0.0f; }
	virtual void SetProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds );
	
	/**
	 * Ticks the FPS chart.
	 *
	 * @param DeltaSeconds	Time in seconds passed since last tick.
	 */
	virtual void TickFPSChart( FLOAT DeltaSeconds );

	/**
	* Ticks the Memory chart.
	*
	* @param DeltaSeconds	Time in seconds passed since last tick.
	*/
	virtual void TickMemoryChart( FLOAT DeltaSeconds );

	/**
	 * Resets the FPS chart data.
	 */
	virtual void ResetFPSChart();

	/**
	 * Dumps the FPS chart information to various places.
	 */
	virtual void DumpFPSChart();

	/** Dumps info on DistanceFactor used for rendering SkeletalMeshComponents during the game. */
	virtual void DumpDistanceFactorChart();

	/**
 	 * Resets the Memory chart data.
	 */
	virtual void ResetMemoryChart();

	/**
	 * Dumps the Memory chart information to various places.
	 */
	virtual void DumpMemoryChart();


private:
	/**
	 * Dumps the FPS chart information to HTML.
	 */
	virtual void DumpFPSChartToHTML( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames, UBOOL bOutputToGlobalLog );

	/**
	 * Dumps the FPS chart information to the log.
	 */
	virtual void DumpFPSChartToLog( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames );

	/**
	 * Dumps the FPS chart information to the special stats log file.
	 */
	virtual void DumpFPSChartToStatsLog( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames );

	/**
	 * Dumps the Memory chart information to HTML.
	 */
	virtual void DumpMemoryChartToHTML( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames, UBOOL bOutputToGlobalLog );

	/**
	 * Dumps the Memory chart information to the log.
	 */
	virtual void DumpMemoryChartToLog( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames );

	/**
	 * Dumps the Memory chart information to the special stats log file.
	 */
	virtual void DumpMemoryChartToStatsLog( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames );

public:

	/**
	 * Spawns any registered server actors
	 */
	virtual void SpawnServerActors(void)
	{
	}

	/**
	 * Loads all Engine object references from their corresponding config entries.
	 */
	void InitializeObjectReferences();

	/**
	 * Clean up the GameViewport
	 */
	void CleanupGameViewport();

	/** Get some viewport. Will be GameViewport in game, and one of the editor viewport windows in editor. */
	virtual FViewport* GetAViewport();

	/**
	 * Allows the editor to accept or reject the drawing of wireframe brush shapes based on mode and tool.
	 */
	virtual UBOOL ShouldDrawBrushWireframe( class AActor* InActor ) { return 1; }

	/**
	 * Looks at all currently loaded packages and prompts the user to save them
	 * if their "bDirty" flag is set.
	 * @param bShouldSaveMap	True if the function should save the map first before other packages.
	 * @return True if successful
	 */
	virtual UBOOL SaveDirtyPackages(UBOOL bShouldSaveMap) { return true; }

	/**
	 * Issued by code reuqesting that decals be reattached.
	 */
	virtual void IssueDecalUpdateRequest() {}

	/**
	 * Returns whether or not the map build in progressed was cancelled by the user.
	 */
	virtual UBOOL GetMapBuildCancelled() const
	{
		return FALSE;
	}

	/**
	 * Sets the flag that states whether or not the map build was cancelled.
	 *
	 * @param InCancelled	New state for the cancelled flag.
	 */
	virtual void SetMapBuildCancelled( UBOOL InCancelled )
	{
		// Intentionally empty.
	}

	/**
	 * Computes a color to use for property coloration for the given object.
	 *
	 * @param	Object		The object for which to compute a property color.
	 * @param	OutColor	[out] The returned color.
	 * @return				TRUE if a color was successfully set on OutColor, FALSE otherwise.
	 */
	virtual UBOOL GetPropertyColorationColor(class UObject* Object, FColor& OutColor);

	/**
	 * @return TRUE if we allow shadow volume resources to be loaded/rendered
	 */
	static FORCEINLINE UBOOL ShadowVolumesAllowed()
	{
		return( (GIsEditor && !(GCookingTarget & UE3::PLATFORM_Console)) || (GAllowShadowVolumes && (GIsGame || (GCookingTarget & UE3::PLATFORM_Console))) );
	}
	
protected:
#if !FINAL_RELEASE
	/**
	 * Handles freezing/unfreezing of rendering
	 */
	virtual void ProcessToggleFreezeCommand() 
	{
		// Intentionally empty.
	}
	
	/**
	 * Handles frezing/unfreezing of streaming
	 */
	 virtual void ProcessToggleFreezeStreamingCommand()
	 {
		// Intentionally empty.
	 }
#endif
}

/**
 * Returns a pointer to the current world.
 */
native static final function WorldInfo GetCurrentWorldInfo();

/**
 * Returns the engine's default tiny font
 */
native static final function Font GetTinyFont();

/**
 * Returns the engine's default small font
 */
native static final function Font GetSmallFont();

/**
 * Returns the engine's default medium font
 */
native static final function Font GetMediumFont();

/**
 * Returns the engine's default large font
 */
native static final function Font GetLargeFont();

/**
 * Returns the specified additional font.
 *
 * @param	AdditionalFontIndex		Index into the AddtionalFonts array.
 */
native static final function Font GetAdditionalFont(int AdditionalFontIndex);

defaultproperties
{
	C_WorldBox=(R=0,G=0,B=107,A=255)
	C_BrushWire=(R=192,G=0,B=0,A=255)
	C_AddWire=(R=127,G=127,B=255,A=255)
	C_SubtractWire=(R=255,G=192,B=63,A=255)
	C_SemiSolidWire=(R=127,G=255,B=0,A=255)
	C_NonSolidWire=(R=63,G=192,B=32,A=255)
	C_WireBackground=(R=0,G=0,B=0,A=255)
	C_ScaleBoxHi=(R=223,G=149,B=157,A=255)
	C_VolumeCollision=(R=149,G=223,B=157,A=255)
	C_BSPCollision=(R=149,G=157,B=223,A=255)
	C_OrthoBackground=(R=163,G=163,B=163,A=255)
	C_Volume=(R=255,G=196,B=255,A=255)
}
