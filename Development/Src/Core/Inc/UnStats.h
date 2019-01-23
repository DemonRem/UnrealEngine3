/*=============================================================================
	UnStats.h: Performance stats framework.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// Change this to one if you want exhaustive stats
#define STATS_SLOW 0

/** Use simple named-section malloc tracking, based on stat section names. */
#define TRACK_MEM_USING_STAT_SECTIONS 0
#if TRACK_MEM_USING_STAT_SECTIONS
#include "TrackAllocSections.h"
#endif

#define STAT_HISTORY_SIZE	20
#define MAX_STAT_LABEL		256

template<class T> inline T InitialStatValue(void) { return 0; }
template <> inline FLOAT InitialStatValue<FLOAT>(void) { return 0.f; }

template<class T> inline const TCHAR* GetStatFormatString(void) { return TEXT(""); }
template<> inline const TCHAR* GetStatFormatString<DWORD>(void) { return TEXT("%d"); }
template<> inline const TCHAR* GetStatFormatString<FLOAT>(void) { return TEXT("%.1f"); }

/**
 * Unique group identifiers. Note these don't have to defined in this header
 * but they do have to be unique. You're better off defining these in your
 * own headers/cpp files
 */
enum EStatGroups
{
	STATGROUP_Default,
	STATGROUP_Object,
	STATGROUP_Engine,
	STATGROUP_Game,
	STATGROUP_Net,
	STATGROUP_Streaming,
	STATGROUP_Physics,
	STATGROUP_Collision,
	STATGROUP_Audio,
	STATGROUP_Memory,
	STATGROUP_Particles,
	STATGROUP_BeamParticles,
	STATGROUP_TrailParticles,
	STATGROUP_RHI,
	STATGROUP_StatSystem,
	STATGROUP_Octree,
	STATGROUP_FaceFX,
	STATGROUP_SceneRendering,
	STATGROUP_Threading,
	STATGROUP_SceneUpdate,
	STATGROUP_Anim,
	STATGROUP_UI,
	STATGROUP_CharacterLighting,
	STATGROUP_AsyncIO,
	STATGROUP_ShaderCompiling,
	STATGROUP_FPSChart,
	STATGROUP_PathFinding,
	STATGROUP_PhysicsFluid,
	STATGROUP_PhysicsCloth,
	STATGROUP_PhysicsFields,
	STATGROUP_Decals,
	// Licensees should create their own enum with the first value being
	// set to the value below
	STATGROUP_LicenseeFirstStatGroup = 10000
};

/**
 * Unique stats identifiers. Note these don't have to defined in this header
 * but they do have to be unique. You're better off defining these in your
 * own headers/cpp files
 */
enum EStats
{
	// Special stats
	STAT_Error,
	STAT_Root,
	STAT_FrameTime,
	// Used by the per group stats to avoid conflicts
	STAT_ObjectFirstStat				= 100,
	STAT_EngineFirstStat				= 200,
	STAT_GameFirstStat					= 300,
	STAT_UnrealScriptTime,// This stat needs to be visible to core
	STAT_NetFirstStat					= 400,
	STAT_StreamingFirstStat				= 500,
	STAT_AsyncLoadingTime,// This stat needs to be here for include order reasons
	STAT_PhysicsFirstStat				= 700,
	STAT_CollisionFirstStat				= 800,
	STAT_AudioFirstStat					= 900,
	STAT_MemoryFirstStat				= 1000,
	STAT_ParticlesFirstStat				= 1200,
	STAT_BeamParticlesFirstStat			= 1300,
	STAT_RHIFirstStat					= 1400,
	STAT_StatSystemFirstStat			= 1500,
	STAT_OctreeFirstStat				= 1600,
	STAT_TrailParticlesFirstStat		= 1700,
	STAT_FaceFXFirstStat				= 1800,
	STAT_SceneRenderingFirstStat		= 1900,
	STAT_ThreadingFirstStat				= 2100,
	STAT_SceneUpdateFirstStat			= 2200,
	STAT_AnimFirstStat					= 2300,
	STAT_UIFirstStat					= 2400,
	STAT_CharacterLightingFirstStat		= 2500,
	STAT_AsyncIOFirstStat				= 2600,
	STAT_FPSChartFirstStat				= 2700,
	STAT_ShaderCompilingFirstStat		= 2800,
	STAT_PathFindingFirstStat			= 2900,
	STAT_PhysicsFluidFirstStat			= 3000,
	STAT_PhysicsClothFirstStat			= 3100,
	STAT_PhysicsFieldsFirstStat			= 3200,
	STAT_DecalFirstStat					= 3300,
	// Licensees should create their own enum with the first value being
	// set to the value below
	STAT_LicenseeFirstStat				= 60000
};


/** Stats for the stat system */
enum EStatSystemStats
{
	STAT_PerFrameCapture = STAT_StatSystemFirstStat,
	STAT_PerFrameCaptureRT,
	STAT_DrawStats,
	STAT_TimingCodeCalls,
};

/**
 * Various memory regions that can be used with memory stats. The exact meaning of
 * the enums are relatively platform-dependent, although the general ones (Physical, GPU)
 * are straightforward. A platform can add more of these, and it won't affect other 
 * platforms, other than a miniscule amount of memory for the StatManager to track the
 * max available memory for each region (uses an array MCR_MAX big)
 */
enum EMemoryCounterRegion
{
	MCR_Physical, // main system memory
	MCR_GPU, // memory directly a GPU (graphics card, etc)
	MCR_GPUSystem, // system memory directly accessible by a GPU
	MCR_RingBuffer1, // general purpose regions for tracking how full a ring buffer gets (NOT the rendering thread command buffer)
	MCR_RingBuffer2, 
	MCR_RingBuffer3, 
	MCR_TexturePool1, // presized texture pools
	MCR_TexturePool2, 
	MCR_TexturePool3, 
	MCR_MAX
};

#if STATS

/**
 * Supported stats rendering modes
 */
enum EStatRenderingMode
{
	SRM_Grouped,
	SRM_Hierarchical,
	SRM_Slow
};


/**
 * Supports history for a given stat including average and maximum
 */
template <class TYPE> class TStatHistory
{
	/**
	 * The list of values to use when calculating an average
	 */
	TYPE History[STAT_HISTORY_SIZE];
	/**
	 * The number of times we've added to the history
	 */
	DWORD HistoryCount;
	/**
	 * Maintains a running total for easy average calculation
	 */
	DOUBLE Total;
	/**
	 * Holds the peak value in the history
	 */
	DOUBLE Max;

public:
	/**
	 * Zeros the history
	 */
	TStatHistory(void) :
		HistoryCount(0),
		Total(0.0),
		Max(0.0)
	{
		// Zero the history array
		for (DWORD Index = 0; Index < STAT_HISTORY_SIZE; Index++)
		{
			History[Index] = InitialStatValue<TYPE>();
		}
	}

	/**
	 * Returns the average
	 */
	inline DOUBLE GetAverage(void) const
	{
		return Total / (DOUBLE)STAT_HISTORY_SIZE;
	}

	/**
	 * Returns the peak value
	 */
	inline DOUBLE GetMax(void) const
	{
		return Max;
	}

	/**
	 * Adds a new value to the history
	 */
	inline void AddToHistory(TYPE Value)
	{
		// Find where we are going to update
		DWORD Index = (HistoryCount++) % STAT_HISTORY_SIZE;
		// Reinit max if we've wrapped around our circular buffer
		if (Index == 0)
		{
			Max = InitialStatValue<TYPE>();
		}
		// Subtract the previous value (note that this will be zero while the
		// count hasn't wrapped)
		Total = Total - (DOUBLE)History[Index] + (DOUBLE)Value;
		// Now that the previous value no longer contributes set the new one
		History[Index] = Value;
		// Update the max
		if ((DOUBLE)Value > Max)
		{
			Max = Value;
		}
	}
};

/**
 * FPS counter that tracks the overall time outside of the stats so that we
 * can sanity check the stats code
 */
class FFPSCounter :
	public TStatHistory<DOUBLE>
{
	/** The last time that was recorded, used for delta calcs */
	DOUBLE LastTime;
	/** The last frame's complete execution time */
	DOUBLE DeltaTime;

public:
	/** Just zeros */
	FFPSCounter(void) :
		LastTime(0.0),
		DeltaTime(0.0)
	{
	}

	/**
	 * Updates the elapsed time and adds that time to the history for averaging
	 *
	 * @param NewTime the latest time value to determine a delta from
	 */
	inline void Update(DOUBLE NewTime)
	{
		DeltaTime = NewTime - LastTime;
		LastTime = NewTime;
		AddToHistory(DeltaTime);
	}

	/** Returns the last calculated frame time */
	FORCEINLINE DOUBLE GetFrameTime(void)
	{
		return DeltaTime;
	}
};

/** Global for easy access */
extern FFPSCounter GFPSCounter;

/**
 * Fulfills the history interface without storing a history
 */
template <class TYPE> class TStatNoHistory
{

public:
	/**
	 * Just here to satisfy the interface
	 */
	inline DOUBLE GetAverage(void)
	{
		return 0.0;
	}

	/**
	 * Just here to satisfy the interface
	 */
	inline DOUBLE GetMax(void)
	{
		return 0.0;
	}

	/**
	 * Just here to satisfy the interface
	 */
	inline void AddToHistory(TYPE)
	{
	}
};

/**
 * Contains the common data shared by all stat types
 */
struct FStatCommonData
{
	/**
	 * The string displayed in the counter view (remote client/pix)
	 */
	const TCHAR* CounterName;
	/**
	 * The unique ID for this stat
	 */
	const DWORD StatId;
	/**
	 * Whether to render this group or not
	 */
	UBOOL bShowStat;

private:
	/**
	 * Hides the default ctor
	 */
	FStatCommonData(void) :
		CounterName(TEXT("")),
		StatId(STAT_Error),
		bShowStat(TRUE)
	{
	}
protected:
	/**
	 * Accessible constructor. Inits data
	 */
	FStatCommonData(const TCHAR* InCounterName,DWORD InStatID) :
		CounterName(InCounterName),
		StatId(InStatID),
		bShowStat(TRUE)
	{
	}
};

/**
 * This class is a counter that doesn't zero between frames and doesn't
 * maintain a history
 */
template <class TYPE> struct TAccumulator :
	public FStatCommonData
{
	/**
	 * The type of stat being counted
	 */
	TYPE Value;

private:
	/**
	 * Hidden ctor
	 */
	TAccumulator(void) :
		FStatCommonData(TEXT(""),"",STAT_Error),
		Value(InitialStatValue<TYPE>()),
		Next(NULL)
	{
	}

	// So that the factory is the only thing that can create stats
	friend struct FStatFactory;

#if !__GNUC__ 
	/**
	 * Zeros the accumulator value and sets the next item in the list
	 */
	TAccumulator(const TCHAR* InCounterName,DWORD InStatId,DWORD InGroupId) :
		FStatCommonData(InCounterName,InStatId),
		Next(NULL),
		Value(InitialStatValue<TYPE>())
	{
		// Get the group for this stat
		FStatGroup* Group = GStatManager.GetGroup(InGroupId);
		check(Group);
		// And add us to it
		Group->AddToGroup(this);
	}
#else // gcc can't properly compile this, so defer until further in the file
	/**
	 * Zeros the accumulator value and sets the next item in the list
	 */
	TAccumulator(const TCHAR* InCounterName,DWORD InStatId,DWORD InGroupId);
#endif


protected:
	/**
	 * Zeros the accumulator value and sets the next item in the list
	 */
	TAccumulator(const TCHAR* InCounterName,DWORD InStatId)  :
		FStatCommonData(InCounterName,InStatId),
		Value(InitialStatValue<TYPE>()),
		Next(NULL)		
	{
		// Don't add to a group as this is done by the child class
	}

public:
	/**
	 * The next accumulator/counter of this type
	 */
	TAccumulator<TYPE>* Next;

	/**
	 * Doesn't zero since accumulators are ever increasing
	 */
	DWORD AdvanceFrame(void)
	{
		return 0;
	}
};

/**
 * This class is an accumulator with special handling for memory information
 */
struct FMemoryCounter :
	public TAccumulator<DWORD>
{

	/**
	 * Memory region that the tracked stat is in 
	 */
	EMemoryCounterRegion Region;

	/**
	 * Should the stat be displayed even if there is no allocated memory?
	 */
	UBOOL bDisplayZeroStats;

private:
	friend struct FMemoryStatFactory;

	/**
	 * Zeros the accumulator value and sets the next item in the list
	 */
	FMemoryCounter(const TCHAR* InCounterName,DWORD InStatId,DWORD InGroupId,
		EMemoryCounterRegion InRegion, UBOOL bInDisplayZeroStats);
};

/**
 * This is a counter that maintains history and zeros its value between frames
 */
template <class TYPE> struct TCounter :
	public TAccumulator<TYPE>
{
	/**
	 * The object that tracks history for this type
	 */
	TStatHistory<TYPE> History;

private:
	// So that the factory is the only thing that can create stats
	friend struct FStatFactory;

#if !__GNUC__ 
	/**
	 * Zeros the counter value and sets the next item in the list
	 */
	TCounter(const TCHAR* InCounterName,DWORD InStatId,DWORD GroupId) :
		TAccumulator<TYPE>(InCounterName,InStatId)
	{
		// Get the group for this stat
		FStatGroup* Group = GStatManager.GetGroup(GroupId);
		check(Group);
		// And add us to it
		Group->AddToGroup(this);
	}
#else // gcc can't properly compile this, so defer until further in the file
	/**
	 * Zeros the value and sets the next item in the list
	 */
	TCounter(const TCHAR* InCounterName,DWORD InStatId,DWORD GroupId);
#endif

public:
	/**
	 * Adds the value to the history and zeros it
	 */
	DWORD AdvanceFrame(void)
	{
		// The atomic copy/zero operation below relies on sizeof(TYPE)==sizeof(INT)
		check(sizeof(TYPE) == sizeof(INT));

		// Make a copy of Value and reset it to zero as an atomic operation.
		static const TYPE InitialTypedValue = InitialStatValue<TYPE>();
		static const INT InitialValue = *(INT*)&InitialTypedValue;
		INT FrameValue;
		do { FrameValue = *(INT*)&this->Value; }
		while(appInterlockedCompareExchange((INT*)&this->Value,InitialValue,FrameValue) != FrameValue);

		// Store the copy of Value in the history.
		History.AddToHistory(*(TYPE*)&FrameValue);

		return 0;
	}
};

// Typdefs for various accumulators/counters
typedef TCounter<DWORD> FStatCounterDWORD;
typedef TCounter<FLOAT> FStatCounterFLOAT;
typedef TAccumulator<DWORD> FStatAccumulatorDWORD;
typedef TAccumulator<FLOAT> FStatAccumulatorFLOAT;

/**
 * Base class for all cycle counting stats
 */
struct FCycleStat : FStatCommonData
{
	/** Keeps track of the number of times called for the stat. */
	TStatHistory<DWORD> NumCallsHistory;
	/**
	 * Keeps track of average and max values for the stat
	 */
	TStatHistory<DWORD> History;
	/**
	 * Keeps track of exclusive average and max values for the stat
	 */
	TStatHistory<DWORD> ExclusiveHistory;
	/**
	 * Tracks the total number of cycles used
	 */
	DWORD Cycles;
	/**
	 * Tracks the number of times called per frame
	 */
	DWORD NumCallsPerFrame;
	/**
	 * The thread this stat was captured on
	 */
	const DWORD ThreadId;
	/**
	 * The parent stat for this stat in the hierarchy
	 */
	const FCycleStat* Parent;
	/**
	 * The next stat in the group this stat belongs to
	 */
	FCycleStat* Next;
	typedef TMap<DWORD,FCycleStat*> FChildStatMap;
	typedef TMap<DWORD,FCycleStat*>::TIterator FChildStatMapIterator;
	/**
	 * Fast look up of a child stat
	 */
	FChildStatMap Children;
	/**
	 * Counts how many times a recursive function has been called
	 */
	DWORD RecursiveCount;
	/**
	 * Global instance counter. Managed solely using interlocked increments
	 */
	static DWORD LastInstanceId;
	/**
	 * The counter used to identify the unique instance
	 */
	DWORD InstanceId;

	/**
	 * Time the "STAT SLOW" code last determined this cycle stat to be above the slow threshold.
	 */
	DOUBLE LastTimeStatWasSlow;

private:
	// So that the factory is the only thing that can create stats
	friend struct FStatFactory;
	/**
	 * Sets the initial state for this stat
	 */
	FCycleStat(DWORD InStatId,const TCHAR* InCounterName,DWORD InThreadId,
		FCycleStat* InParent,DWORD GroupId = STATGROUP_Default);

public:
	/**
	 * Adds the value to the history and zeros it
	 *
	 * @return the inclusive time for this stat
	 */
	DWORD AdvanceFrame(void);

	/**
	 * Renders the stat at the specified location in grouped form
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start drawing at
	 * @param Y the Y location to start drawing at
	 * @param bShowInclusive whether to draw inclusive values
	 * @param bShowExclusive whether to draw exclusive values
	 * @param AfterNameColumnOffset how far over to move to draw the first column after the stat name
	 * @param InterColumnOffset the space between columns to add
	 * @param StatColor the color to draw the stat in
	 */
	INT RenderGrouped(class FCanvas* Canvas,INT X,INT Y,UBOOL bShowInclusive,
		UBOOL bShowExlusive,INT AfterNameColumnOffset,INT InterColumnOffset,
		const FLinearColor& StatColor);

	/**
	 * Renders the stat at the specified location in hierarchical form
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start drawing at
	 * @param Y the Y location to start drawing at
	 * @param StatNum the number to associate with this stat
	 * @param bShowInclusive whether to draw inclusive values
	 * @param bShowExclusive whether to draw exclusive values
	 * @param AfterNameColumnOffset how far over to move to draw the first column after the stat name
	 * @param InterColumnOffset the space between columns to add
	 * @param StatColor the color to draw the stat in
	 */
	INT RenderHierarchical(class FCanvas* Canvas,INT X,INT Y,DWORD StatNum,UBOOL bShowInclusive,
		UBOOL bShowExlusive,INT AfterNameColumnOffset,INT InterColumnOffset,
		const FLinearColor& StatColor);
};

/** Simple stat type enum for factory creation */
enum EStatType
{
	STATTYPE_CycleCounter,
	STATTYPE_AccumulatorFLOAT,
	STATTYPE_AccumulatorDWORD,
	STATTYPE_CounterFLOAT,
	STATTYPE_CounterDWORD,
	STATTYPE_MemoryCounter,
	STATTYPE_Error
};

/**
 * Factory class for creating stats
 */
struct FStatFactory : FStatCommonData
{
	/**
	 * The group the stat being created belongs to
	 */
	const DWORD GroupId;
	/**
	 * The stat type to create
	 */
	const EStatType StatType;
	/**
	 * The next factory in the list
	 */
	const FStatFactory* NextFactory;
	/**
	 * The global list of factories
	 */
	static FStatFactory* FirstFactory;

private:
	/** Hide this ctor */
	FORCENOINLINE FStatFactory(void) :
		FStatCommonData(TEXT(""),STAT_Error),
		GroupId(STATGROUP_Default),
		StatType(STATTYPE_Error),
		NextFactory(NULL)
	{
	}

	/**
	 * Creates a new counter on behalf of the caller
	 */
	inline FStatCounterDWORD* CreateDwordCounter(void) const
	{
		check(StatType == STATTYPE_CounterDWORD);
		return new FStatCounterDWORD(CounterName,StatId,GroupId);
	}

	/**
	 * Creates a new counter on behalf of the caller
	 */
	inline FStatCounterFLOAT* CreateFloatCounter(void) const
	{
		check(StatType == STATTYPE_CounterFLOAT);
		return new FStatCounterFLOAT(CounterName,StatId,GroupId);
	}

	/**
	 * Creates a new Accumulator on behalf of the caller
	 */
	inline FStatAccumulatorDWORD* CreateDwordAccumulator(void) const
	{
		check(StatType == STATTYPE_AccumulatorDWORD);
		return new FStatAccumulatorDWORD(CounterName,StatId,GroupId);
	}

	/**
	 * Creates a new Accumulator on behalf of the caller
	 */
	inline FStatAccumulatorFLOAT* CreateFloatAccumulator(void) const
	{
		check(StatType == STATTYPE_AccumulatorFLOAT);
		return new FStatAccumulatorFLOAT(CounterName,StatId,GroupId);
	}

public:
	/**
	 * Sets the members for this stat and links into the static
	 * list of factories
	 */
	FORCENOINLINE FStatFactory(const TCHAR* InCounterName,DWORD InStatId,
		DWORD InGroupId,const EStatType InStatType) :
		FStatCommonData(InCounterName,InStatId),
		GroupId(InGroupId),
		StatType(InStatType),
		NextFactory(FirstFactory)		
	{
		FirstFactory = this;
	}

	/**
	 * Creates a new cycle counter on behalf of the caller
	 */
	inline FCycleStat* CreateStat(FCycleStat* Parent) const
	{
		check(StatType == STATTYPE_CycleCounter);
		return new FCycleStat(StatId,CounterName,appGetCurrentThreadId(),
			Parent,GroupId);
	}

	/**
	 * Create a stat and return the common interface to it
	 */
	virtual FStatCommonData* CreateStat(void) const;
};

/**
* Factory class for creating memory stats
*/
struct FMemoryStatFactory : FStatFactory
{
	/**
	 * What region is this stat in?
	 */
	EMemoryCounterRegion Region;

	/**
	 * Should the stat be displayed even if there is no allocated memory?
	 */
	UBOOL bDisplayZeroStats;

	/**
	 * Creates a new memory stat
	 */
	inline FMemoryCounter* CreateMemoryStat(void) const
	{
		check(StatType == STATTYPE_MemoryCounter);
		return new FMemoryCounter(CounterName, StatId, GroupId, Region, bDisplayZeroStats);
	}

public:
	/**
	 * Sets the members for this stat and links into the static
	 * list of factories
	 */
	FORCENOINLINE FMemoryStatFactory(const TCHAR* InCounterName,DWORD InStatId,
		DWORD InGroupId,EMemoryCounterRegion InRegion, UBOOL bInDisplayZeroStats) :
	FStatFactory(InCounterName, InStatId, InGroupId, STATTYPE_MemoryCounter),
	Region(InRegion),
	bDisplayZeroStats(bInDisplayZeroStats)
	{
	}


	/**
	* Create a stat and return the common interface to it
	*/
	virtual FStatCommonData* CreateStat(void) const;
};

/** Macros for declaring stat factory instances */
#define DECLARE_CYCLE_STAT(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CycleCounter);
#define DECLARE_FLOAT_COUNTER_STAT(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CounterFLOAT);
#define DECLARE_DWORD_COUNTER_STAT(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CounterDWORD);
#define DECLARE_FLOAT_ACCUMULATOR_STAT(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_AccumulatorFLOAT);
#define DECLARE_DWORD_ACCUMULATOR_STAT(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_AccumulatorDWORD);
#define DECLARE_MEMORY_STAT(CounterName,StatId,GroupId) \
	FMemoryStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,MCR_Physical,TRUE);
#define DECLARE_MEMORY_STAT2(CounterName,StatId,GroupId,Region,bDisplayZeroStats) \
	FMemoryStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,Region,bDisplayZeroStats);

#if STATS_SLOW
#define DECLARE_CYCLE_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CycleCounter);
#define DECLARE_FLOAT_COUNTER_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CounterFLOAT);
#define DECLARE_DWORD_COUNTER_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CounterDWORD);
#define DECLARE_FLOAT_ACCUMULATOR_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_AccumulatorFLOAT);
#define DECLARE_DWORD_ACCUMULATOR_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_AccumulatorDWORD);
#define DECLARE_MEMORY_STAT_SLOW(CounterName,StatId,GroupId) \
	FMemoryStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,MCR_Physical,TRUE);
#define DECLARE_MEMORY_STAT2_SLOW(CounterName,StatId,GroupId,Region,bDisplayZeroStats) \
	FMemoryStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,Region,bDisplayZeroStats);
#endif // STATS_SLOW

/**
 * This class holds the list of stats in a group
 */
struct FStatGroup
{
	/**
	 * The group description. Must be statically created
	 */
	const TCHAR* Desc;
	/** 
	 * The id of this group
	 */
	const DWORD GroupId;
	/**
	 * The set of canonical cycle stats that are members of this group
	 */
	TMap<DWORD,FCycleStat*> CanonicalCycleStats;
	typedef TMap<DWORD,FCycleStat*>::TIterator FCanonicalStatIterator;
	/** Holds the list of cannonical stats */
	FCycleStat* FirstCanonicalStat;
	/**
	 * The first cycle stat that is a member of this group
	 */
	FCycleStat* FirstCycleStat;
	/**
	 * The first float accumulator stat that is a member of this group
	 */
	FStatAccumulatorFLOAT* FirstFloatAccumulator;
	/**
	 * The first float counter stat that is a member of this group
	 */
	FStatCounterFLOAT* FirstFloatCounter;
	/**
	 * The first dword accumulator stat that is a member of this group
	 */
	FStatAccumulatorDWORD* FirstDwordAccumulator;
	/**
	 * The first dword counter stat that is a member of this group
	 */
	FStatCounterDWORD* FirstDwordCounter;
	/**
	 * The first memory stat that is a member of this group
	 */
	FMemoryCounter* FirstMemoryCounter;
	/**
	 * Next group in the list of overall groups
	 */
	FStatGroup* NextGroup;
	/**
	 * Whether to render this group or not
	 */
	UBOOL bShowGroup;
	/**
	 * Whether this group should be processed during notification updates
	 */
	UBOOL bIsNetEnabled;
	/**
	 * Used to indicate whether or not it is safe to contruct these or not
	 */
	static UBOOL bIsPostInit;

	/**
	 * Deletes the list of stats
	 *
	 * @param FirstStat the stat to start deleting with
	 */
	template<class TYPE> void DeleteStatList(TYPE* FirstStat)
	{
		for (TYPE* Stat = FirstStat; Stat != NULL;)
		{
			TYPE* Delete = Stat;
			Stat = (TYPE*)Stat->Next;
			delete Delete;
		}
	}

	// So the factory classes are the creators and nothing else
	friend struct FStatGroupFactory;
private:
	/**
	 * Construct that zeroes the group list
	 */
	FStatGroup(void) :
		Desc(TEXT("")),
		GroupId(STATGROUP_Default),
		FirstCycleStat(NULL),
		FirstFloatAccumulator(NULL),
		FirstFloatCounter(NULL),
		FirstDwordAccumulator(NULL),
		FirstDwordCounter(NULL),
		FirstMemoryCounter(NULL),
		NextGroup(NULL),
		bShowGroup(FALSE),
		bIsNetEnabled(FALSE)
	{
		// These may not be constructed post initialization
		check(bIsPostInit == FALSE);
	}

	/**
	 * Constructor that zeroes the group list and sets the group information
	 *
	 * @param InDesc the description to use for the group
	 * @param InGroupId the group id to use for the group
	 * @param Next the next group in the list
	 */
	FStatGroup(const TCHAR* InDesc,DWORD InGroupId,FStatGroup* Next) :
	    Desc(InDesc),
		GroupId(InGroupId),
		FirstCycleStat(NULL),
		FirstFloatAccumulator(NULL),
		FirstFloatCounter(NULL),
		FirstDwordAccumulator(NULL),
		FirstDwordCounter(NULL),
		FirstMemoryCounter(NULL),
		NextGroup(Next),
		bShowGroup(FALSE),
		bIsNetEnabled(FALSE)
	{
		// These may not be constructed post initialization
		check(bIsPostInit == FALSE);
	}


public:
	/**
	 * Cleans up resources
	 */
	~FStatGroup();

	/**
	 * Thread safe function for inserting at the head of the list
	 */
	void AddToGroup(FCycleStat* Stat);

	/**
	 * Thread safe function for inserting at the head of the list
	 */
	void AddToGroup(FStatCounterFLOAT* Stat);

	/**
	 * Thread safe function for inserting at the head of the list
	 */
	void AddToGroup(FStatAccumulatorFLOAT* Stat);

	/**
	 * Thread safe function for inserting at the head of the list
	 */
	void AddToGroup(FStatCounterDWORD* Stat);

	/**
	 * Thread safe function for inserting at the head of the list
	 */
	void AddToGroup(FStatAccumulatorDWORD* Stat);

	/**
	 * Thread safe function for inserting at the head of the list
	 */
	void AddToGroup(FMemoryCounter* Stat);

	/**
	 * Updates the canonical stats for group views
	 */
	void UpdateCanonicalStats(void);

	/** Class for iterating through all of the stats for a given group */
	class FStatIterator
	{
		/** The group being iterated */
		const FStatGroup* Group;
		/** The first cycle stat that is a member of this group */
		FCycleStat* CycleStat;
		/** The first float accumulator stat that is a member of this group */
		FStatAccumulatorFLOAT* FloatAccumulator;
		/** The first float counter stat that is a member of this group */
		FStatCounterFLOAT* FloatCounter;
		/** The first dword accumulator stat that is a member of this group */
		FStatAccumulatorDWORD* DwordAccumulator;
		/** The first dword counter stat that is a member of this group */
		FStatCounterDWORD* DwordCounter;
		/** The first memory stat that is a member of this group */
		FMemoryCounter* MemoryCounter;

	public:
		/**
		 * Constructs the iterator for the group
		 *
		 * @param InGroup the group to iterate stats for
		 */
		FStatIterator(const FStatGroup* InGroup) :
			Group(InGroup)
		{
			// Copy all of the starting points
			CycleStat = Group->FirstCanonicalStat;
			FloatAccumulator = Group->FirstFloatAccumulator;
			FloatCounter = Group->FirstFloatCounter;
			DwordAccumulator = Group->FirstDwordAccumulator;
			DwordCounter = Group->FirstDwordCounter;
			MemoryCounter = Group->FirstMemoryCounter;
		}

		/** Moves to the next stat in the list */
		void operator++(void)
		{
			if (CycleStat != NULL)
			{
				CycleStat = CycleStat->Next;
			}
			else if (FloatAccumulator != NULL)
			{
				FloatAccumulator = (FStatAccumulatorFLOAT*)FloatAccumulator->Next;
			}
			else if (FloatCounter != NULL)
			{
				FloatCounter = (FStatCounterFLOAT*)FloatCounter->Next;
			}
			else if (DwordAccumulator != NULL)
			{
				DwordAccumulator = (FStatAccumulatorDWORD*)DwordAccumulator->Next;
			}
			else if (DwordCounter != NULL)
			{
				DwordCounter = (FStatCounterDWORD*)DwordCounter->Next;
			}
			else if (MemoryCounter != NULL)
			{
				MemoryCounter = (FMemoryCounter*)MemoryCounter->Next;
			}
		}

		/** Gets the next stat */
		FStatCommonData* operator*(void) const
		{
			FStatCommonData* Stat = NULL;
			if (CycleStat != NULL)
			{
				Stat = CycleStat;
			}
			else if (FloatAccumulator != NULL)
			{
				Stat = FloatAccumulator;
			}
			else if (FloatCounter != NULL)
			{
				Stat = FloatCounter;
			}
			else if (DwordAccumulator != NULL)
			{
				Stat = DwordAccumulator;
			}
			else if (DwordCounter != NULL)
			{
				Stat = DwordCounter;
			}
			else if (MemoryCounter != NULL)
			{
				Stat = MemoryCounter;
			}
			return Stat;
		}

		/** Returns true if there are more items, false otherwise */
		operator UBOOL(void) const
		{
			return CycleStat != NULL ||
				FloatAccumulator != NULL ||
				FloatCounter != NULL ||
				DwordAccumulator != NULL ||
				DwordCounter != NULL ||
				MemoryCounter != NULL;
		}
	};
};

/**
 * Factory class for creating stat groups
 */
struct FStatGroupFactory
{
	/**
	 * The text to show as the name of the group. Expects to be a static string
	 */
	const TCHAR* GroupDesc;
	/**
	 * The id of the group this factory creates
	 */
	const DWORD GroupId;
	/**
	 * The next factory in the list
	 */
	const FStatGroupFactory* NextFactory;
	/**
	 * The global list of factories
	 */
	static FStatGroupFactory* FirstFactory;

private:
	/** Hide this ctor */
	FStatGroupFactory(void) :
		GroupDesc(TEXT("")),
		GroupId(STATGROUP_Default),
		NextFactory(NULL)
	{
	}

public:
	/**
	 * Sets the members for this group and links into the static
	 * list of factories
	 */
	FORCENOINLINE FStatGroupFactory(const TCHAR* InDesc,DWORD InGroupId) :
		GroupDesc(InDesc),
		GroupId(InGroupId),
		NextFactory(FirstFactory)
	{
		FirstFactory = this;
	}

	/**
	 * Creates a new group on behalf of the caller
	 */
	inline FStatGroup* CreateGroup(FStatGroup* Next) const
	{
		return new FStatGroup(GroupDesc,GroupId,Next);
	}
};

/** Macro for declaring group factory instances */
#define DECLARE_STATS_GROUP(GroupDesc,GroupId) \
	FStatGroupFactory GroupFactory_##GroupId(GroupDesc,GroupId);

#include "UnStatsNotifyProvidersBase.h"

/**
 * Manages the creation of groups, stats, and the stats hierarchy
 */
class FStatManager
{
	/**
	 * Used to broadcast stats results to a set of registered notifiers
	 */
	FStatNotifyManager StatNotifyManager;
	/**
	 * Used to synchronize thread access
	 */
	FCriticalSection* SyncObject;
	/**
	 * The first group in the list of available stat groups
	 * NOTE: This assumes that all groups are created during init
	 * and that they are created safely on one thread
	 */
	FStatGroup* FirstGroup;
	/**
	 * For fast finding of group by group enum
	 * NOTE: This assumes that all groups are created during init
	 * and that they are created safely on one thread
	 */
	TMap<DWORD,FStatGroup*> GroupMap;
	/**
	 * Indicates whether this class was properly inited or not
	 */
	UBOOL bIsInitialized;
	typedef TMap<DWORD,FCycleStat*> FThreadStatMap;
	typedef TMap<DWORD,FCycleStat*>::TIterator FThreadStatMapIterator;
	/**
	 * Holds the TLS index for the per thread cycle stat object
	 */
	DWORD PerThreadCycleStats;
	/**
	 * Map for finding stat factories faster
	 */
	TMap<DWORD,const FStatFactory*> StatFactoryMap;
	/**
	 * The current frame having stats tracked for
	 */
	DWORD FrameNumber;
	/**
	 * For fast finding of float accumulators
	 */
	TMap<DWORD,FStatAccumulatorFLOAT*> FloatAccumulatorMap;
	/**
	 * For fast finding of dword accumulators
	 */
	TMap<DWORD,FStatAccumulatorDWORD*> DwordAccumulatorMap;
	/**
	 * Map containing all stats that have been created for a given stat id
	 */
	TMultiMap<DWORD,FStatCommonData*> AllStatsMap;
	/**
	 * The stat we are currently showing the children for. Used with
	 * rendering stats via hierarchy
	 */
	FCycleStat* CurrentRenderedStat;
	/**
	 * How we should render stats
	 */
	EStatRenderingMode StatRenderingMode;
	/**
	 * Whether to show inclusive stats or not
	 */
	UBOOL bShowInclusive;
	/**
	 * Whether to show exclusive stats or not
	 */
	UBOOL bShowExclusive;
	/**
	 * Rendering offset for first column from stat label
	 */
	DWORD AfterNameColumnOffset;
	/**
	 * Rendering offsets for additonal columns
	 */
	DWORD InterColumnOffset;
	/**
	 * Holds the number of groups that are being rendered
	 */
	DWORD NumRenderedGroups;
	/**
	 * Use a flag so that hierarchical rendering can be toggled
	 */
	UBOOL bShowHierarchical;
	/**
	 * Determines whether to show/hide cycle counters during rendering
	 */
	UBOOL bShowCycleCounters;
	/**
	 * Determines whether to show/hide counters/accumulators during rendering
	 */
	UBOOL bShowCounters;
	/**
	 * Color for rendering stats
	 */
	FLinearColor StatColor;
	/**
	 * Color to use when rendering headings
	 */
	FLinearColor HeadingColor;
	/**
	 * Color to use when rendering group names
	 */
	FLinearColor GroupColor;
	/**
	 * Color used as the background for every other stat item to make it easier to read across lines 
	 */
	FLinearColor BackgroundColor;
	/**
	 * Threshold above which a cycle stat is considered slow for "STAT SLOW"
	 */
	FLOAT SlowStatThreshold;
	/**
	 * Minimum duration in seconds a slow cycle stat is displayed for  with "STAT SLOW"
	 */
	FLOAT MinSlowStatDuration;
	/**
	 * The total memory available to the system for % calculations, for each region
	 */
	DWORD TotalMemAvail[MCR_MAX];
	/**
	 * The set of network enabled groups (all if empty)
	 */
	TArray<FString> NetEnabledGroups;

	/**
	 * Factory method for creating a stat and adding it to this object's
	 * list of stats
	 */
	FORCEINLINE FCycleStat* CreateStat(FCycleStat* Parent,DWORD InStatId)
	{
		FScopeLock sl(SyncObject);
		// Look up the factory in the hash
		const FStatFactory* Factory = StatFactoryMap.FindRef(InStatId);
		// Create the new stat since one doesn't exist yet
		FCycleStat* Stat = Factory->CreateStat(Parent);
		// Add to the complete list
		AllStatsMap.Add(Stat->StatId,Stat);
		if (Parent != NULL)
		{
			// Add the child to the map
			Parent->Children.Set(InStatId,Stat);
		}
		return Stat;
	}

	/**
	 * Creates the groups that are registered and places them in a hash
	 */
	void CreateGroups(void);

	/**
	 * Creates the counter/accumulator stats and adds them to there groups and
	 * various hashes
	 */
	void CreateCountersAccumulators(void);

	/**
	 * Builds the mapping of statid to factories for creating it
	 */
	void BuildFactoryMaps(void);

	/**
	 * Create the canonical stats for each group
	 */
	void CreateCanonicalStats(void);

	/**
	 * Adds all of the group and stats descriptions to the stat notifiers
	 */
	void SendNotifiersDescriptions(void);

	/**
	 * Renders stats using groups
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start rendering at
	 * @param Y the Y location to start rendering at
	 */
	void RenderGrouped(class FCanvas* Canvas,INT X,INT Y);

	/**
	 * Renders cycle stats above a certain threshold.
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start rendering at
	 * @param Y the Y location to start rendering at
	 */
	void RenderSlow(class FCanvas* Canvas,INT X,INT Y);

	/**
	 * Renders stats by showing hierarchy
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start rendering at
	 * @param Y the Y location to start rendering at
	 */
	void RenderHierarchical(class FCanvas* Canvas,INT X,INT Y);

	/**
	 * Renders the headings for grouped rendering
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start rendering at
	 * @param Y the Y location to start rendering at
	 *
	 * @return the height of headings rendered
	 */
	INT RenderGroupedHeadings(class FCanvas* Canvas,INT X,INT Y);

	/**
	 * Renders the counter headings for grouped rendering
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start rendering at
	 * @param Y the Y location to start rendering at
	 *
	 * @return the height of headings rendered
	 */
	INT RenderCounterHeadings(class FCanvas* Canvas,INT X,INT Y);

	/**
	 * Renders the memory headings for grouped rendering
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start rendering at
	 * @param Y the Y location to start rendering at
	 *
	 * @return the height of headings rendered
	 */
	INT RenderMemoryHeadings(class FCanvas* Canvas,INT X,INT Y);

	/**
	 * Renders the headings for hierarchical rendering
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start rendering at
	 * @param Y the Y location to start rendering at
	 *
	 * @return the height of headings rendered
	 */
	INT RenderHierarchicalHeadings(class FCanvas* Canvas,INT X,INT Y);

	/**
	 * Changes the currently pointed to stat to one the user chose. Used when
	 * navigating hierarchical stats
	 *
	 * @param ViewChildStat - The child stat number to view
	 */
	void SelectChildStat(DWORD ViewChildStat);

	/**
	 * Sets the currently rendered stat for hierarchical rendering if not set
	 */
	FORCEINLINE void InitCurrentRenderedStat(void)
	{
		if (CurrentRenderedStat == NULL)
		{
			// Search the map for the stat for this thread
			CurrentRenderedStat = GetCurrentStat();
			check(CurrentRenderedStat && "No stats for this thread?");
			// Now search up to the root
			while (CurrentRenderedStat->Parent != NULL)
			{
				CurrentRenderedStat = (FCycleStat*)CurrentRenderedStat->Parent;
			}
			// Move to the FrameTime counter
			CurrentRenderedStat = CurrentRenderedStat->Children.FindRef(STAT_FrameTime);
		}
	}

	/**
	 * Toggles the rendering state for the specified group
	 *
	 * @param GroupName the group to be enabled/disabled
	 */
	void ToggleGroup(const TCHAR* GroupName);

	/**
	 * Disables the rendering state for all groups
	 */
	void DisableAllGroups(void);

	/**
	 * Toggles the rendering state for the specified stat
	 *
	 * @param StatName the stat to be enabled/disabled
	 */
	void ToggleStat(const TCHAR* StatName);

	/**
	 * Iterates through a set of memory stats and renders them.
	 *
	 * @param FirstStat the first stat in the list to render
	 * @param RI the render interface to use
	 * @param X the xoffset to start drawing at
	 * @param Y the y offset to start drawing at
	 * @param AfterNameColumnOffset the space to skip between counter name and first stat
	 * @param InterColumnOffset the space to skip between columns
	 * @param StatColor the color to draw the stat with
	 */
	INT RenderMemoryCounters(FMemoryCounter* FirstStat,class FCanvas* Canvas,
		INT X,INT Y,DWORD AfterNameColumnOffset,DWORD InterColumnOffset,
		const FLinearColor& StatColor);

	/**
	 * Helper function that hides redundant code. Iterates through a set of
	 * stats and has them render.
	 *
	 * @param FirstStat the first stat in the list to render
	 * @param RI the render interface to use
	 * @param X the xoffset to start drawing at
	 * @param Y the y offset to start drawing at
	 * @param AfterNameColumnOffset the space to skip between counter name and first stat
	 * @param InterColumnOffset the space to skip between columns
	 * @param StatColor the color to draw the stat with
	 */
	template<class TYPE,typename DTYPE>
	INT RenderAccumulatorList(TYPE* FirstStat,class FCanvas* Canvas,
		INT X,INT Y,DWORD AfterNameColumnOffset,DWORD InterColumnOffset,
		const FLinearColor& StatColor);

	/**
	 * Helper function that hides redundant code. Iterates through a set of
	 * stats and has them render.
	 *
	 * @param FirstStat the first stat in the list to render
	 * @param RI the render interface to use
	 * @param X the xoffset to start drawing at
	 * @param Y the y offset to start drawing at
	 * @param AfterNameColumnOffset the space to skip between counter name and first stat
	 * @param InterColumnOffset the space to skip between columns
	 * @param StatColor the color to draw the stat with
	 */
	template<class TYPE,typename DTYPE>
	INT RenderCounterList(TYPE* FirstStat,class FCanvas* Canvas,
		INT X,INT Y,DWORD AfterNameColumnOffset,DWORD InterColumnOffset,
		const FLinearColor& StatColor);

	/**
	 * Helper function that hides redundant code. Iterates through a set of
	 * stats and sends them to the stat notify manager for processing
	 *
	 * @param FirstStat the first stat in the list to write out
	 */
	template<class TYPE> void WriteStatList(TYPE* FirstStat)
	{
		// Send each counter/accumulator stat to the notify providers
		for (TYPE* Stat = FirstStat; Stat != NULL; Stat = (TYPE*)Stat->Next)
		{
			StatNotifyManager.WriteStat(Stat->StatId,Stat->Value);
			Stat->AdvanceFrame();
		}
	}

	/**
	 * Helper function that hides redundant code. Iterates through a set of
	 * stats and advances them to the next frame.
	 *
	 * @param FirstStat the first stat in the list to write out
	 */
	template<class TYPE> void AdvanceStatList(TYPE* FirstStat)
	{
		for (TYPE* Stat = FirstStat; Stat != NULL; Stat = (TYPE*)Stat->Next)
		{
			Stat->AdvanceFrame();
		}
	}

	/**
	 * Helper function that iterates through a set of stats and logs their info
	 * to the output device
	 *
	 * @param FirstStat the first stat in the list to write out
	 * @param Ar the device to write to
	 */
	template<class TYPE> void ListStats(TYPE* FirstStat,FOutputDevice& Ar)
	{
		// Send each counter/accumulator stat to the notify providers
		for (TYPE* Stat = FirstStat; Stat != NULL; Stat = (TYPE*)Stat->Next)
		{
			Ar.Logf(TEXT("%s\t\tcounter"),Stat->CounterName);
		}
	}

	/**
	 * Dumps the name of each stats group to the specified output device
	 *
	 * @param Ar the device to write to
	 */
	void ListGroups(FOutputDevice& Ar);

	/**
	 * Dumps the name of each stat in the specified group to the output device
	 *
	 * @param Ar the device to write to
	 */
	void ListStatsForGroup(const TCHAR* Name,FOutputDevice& Ar);

	/**
	 * Returns whether there are any active stats to be rendered in this linked list.
	 *
	 * @param FirstStat the first stat in the list to check
	 * @return TRUE if there are, FALSE otherwise
	 */
	template <class TYPE> UBOOL HasRenderableStats( const TYPE* FirstStat ) const;

public:
	/**
	 * Simple constructor zeros/inits it's values
	 */
	FStatManager(void) :
		SyncObject(NULL),
		FirstGroup(NULL),
		bIsInitialized(FALSE),
		CurrentRenderedStat(NULL),
		StatRenderingMode(SRM_Grouped),
		bShowInclusive(TRUE),
		bShowExclusive(FALSE),
		AfterNameColumnOffset(0),
		InterColumnOffset(0),
		NumRenderedGroups(0),
		bShowHierarchical(FALSE),
		bShowCycleCounters(TRUE),
		bShowCounters(TRUE),
		StatColor(0.f,1.f,0.f),
		HeadingColor(1.f,0.2f,0.f),
		GroupColor(FLinearColor::White),
		BackgroundColor(0.5f, 0.5f, 0.5f, 0.5f),
		SlowStatThreshold(0.01f),
		MinSlowStatDuration(10)
	{
		// Allocate the TLS slot and zero its contents
		PerThreadCycleStats = appAllocTlsSlot();
		appSetTlsValue(PerThreadCycleStats,NULL);

		// clear out the available memory slots
		appMemzero(TotalMemAvail, sizeof(TotalMemAvail));
	}

	/**
	 * Deletes all of the resources associated with this object
	 */
	~FStatManager(void)
	{
		appFreeTlsSlot(PerThreadCycleStats);
		if (SyncObject != NULL)
		{
			GSynchronizeFactory->Destroy(SyncObject);
			SyncObject = NULL;
		}
	}

	/**
	 * Initializes all of the internal state
	 */
	void Init(void);
	/**
	 * Cleans up all internal state
	 */
	void Destroy(void);

	/**
	 * Returns the group object for the specified group id. Note: this
	 * requires that all threads operate on this in a read only fashion
	 */
	inline FStatGroup* GetGroup(DWORD GroupId)
	{
		return GroupMap.FindRef(GroupId);
	}

	/**
	 * Finds the cycle stat for the specified id for the currently running thread
	 *
	 * @param StatId the stat to find the cycle stat for
	 * @param CurrentThreadId the thread id of the stat to match on
	 */
	const FCycleStat* GetCycleStat(DWORD StatId,DWORD CurrentThreadId = appGetCurrentThreadId());

	/**
	 * Finds the current stat for the specified thread
	 */
	inline FCycleStat* GetCurrentStat(void)
	{
		// Read the current stat from TLS
		FCycleStat* Stat = (FCycleStat*)appGetTlsValue(PerThreadCycleStats);
		if (Stat == NULL)
		{
			// Create a new root stat
			Stat = CreateStat(NULL,STAT_Root);
			// Associate this thread with that stat
			appSetTlsValue(PerThreadCycleStats,Stat);
		}
		return Stat;
	}

	/**
	 *	Sets the cycle stat to a specified cycle count and call count
	 *
	 *	@param StatId		Represents the stat to set
	 *	@param Cycles		The cycle count to set
	 *	@param CallCount	The call count to set
	 */
	inline void SetCycleCounter( DWORD StatId, DWORD Cycles, DWORD CallCount );

	/**
	 * Pushes the specified stat on the stack
	 *
	 * @param Stat the stat to make the current one
	 */
	inline void PushStat(FCycleStat* Stat)
	{
		// Use thread local storage to hold the current stat state
		appSetTlsValue(PerThreadCycleStats,Stat);
	}

	/**
	 * Pops the current stat off of the stack for this thread
	 */
	inline void PopStat(void)
	{
		FCycleStat* Stat = GetCurrentStat();
		if (Stat->Parent != NULL)
		{
			// Use thread local storage to hold the current stat state
			appSetTlsValue(PerThreadCycleStats,(void*)Stat->Parent);
		}
	}

	/**
	 * Given a unique stat id, finds the child stat associated with it for
	 * this thread. If one isn't found, it is created and added to our set
	 *
	 * @param Parent the parent stat to search for a child of
	 * @param InStatId the unique stat to find
	 *
	 * @return The child stat for this id
	 */
	inline FCycleStat* GetChildStat(FCycleStat* Parent,DWORD InStatId)
	{
		// Look up the child
		FCycleStat* Child = Parent->Children.FindRef(InStatId);
		if (Child == NULL)
		{
			// Create the child stat
			Child = CreateStat(Parent,InStatId);
		}
		check(Child);
		return Child;
	}

	/**
	 * Increments a float counter/accumulator in a thread safe way
	 *
	 * @param StatId the stat being incremented
	 * @param IncBy the amount to increment by
	 */
	inline void Increment(DWORD StatId,FLOAT IncBy)
	{
		// Heavier weight synching since there isn't an interlocked that
		// supports floats
		FScopeLock sl(SyncObject);
		// Find the stat that we are updating
		FStatAccumulatorFLOAT* Accum = FloatAccumulatorMap.FindRef(StatId);
		check(Accum != NULL);
		// Now change the value
		Accum->Value += IncBy;
	}

	/**
	 * Increments a dword counter/accumulator in a thread safe way
	 *
	 * @param StatId the stat being incremented
	 * @param IncBy the amount to increment by
	 */
	inline void Increment(DWORD StatId,DWORD IncBy)
	{
		// Find the stat that we are updating
		FStatAccumulatorDWORD* Accum = DwordAccumulatorMap.FindRef(StatId);
		check(Accum != NULL);
		// Use the lightweight synch to update
		appInterlockedAdd((INT*)&Accum->Value,IncBy);
	}

	/**
	 * Decrements a float counter/accumulator in a thread safe way
	 *
	 * @param StatId the stat being decremented
	 * @param IncBy the amount to decrement by
	 */
	inline void Decrement(DWORD StatId,FLOAT DecBy)
	{
		// Heavier weight synching since there isn't an interlocked that
		// supports floats
		FScopeLock sl(SyncObject);
		// Find the stat that we are updating
		FStatAccumulatorFLOAT* Accum = FloatAccumulatorMap.FindRef(StatId);
		check(Accum != NULL);
		// Now change the value
		Accum->Value -= DecBy;
	}

	/**
	 * Decrements a dword counter/accumulator in a thread safe way
	 *
	 * @param StatId the stat being decremented
	 * @param IncBy the amount to decrement by
	 */
	inline void Decrement(DWORD StatId,DWORD DecBy)
	{
		// Find the stat that we are updating
		FStatAccumulatorDWORD* Accum = DwordAccumulatorMap.FindRef(StatId);
		check(Accum != NULL);
		// Use the lightweight synch to update
		DWORD StartValue;
		do { StartValue = Accum->Value; }
		while((DWORD)appInterlockedCompareExchange((INT*)&Accum->Value,StartValue - DecBy,StartValue) != StartValue);
	}

	/**
	 * Sets a counter/accumulator to a specific value in a thread safe way
	 *
	 * @param StatId the stat being set
	 * @param IncBy the amount to set it to
	 */
	inline void SetStatValue(DWORD StatId,FLOAT Value)
	{
		// Heavier weight synching since there isn't an interlocked that
		// supports floats
		FScopeLock sl(SyncObject);
		// Find the stat that we are updating
		FStatAccumulatorFLOAT* Accum = FloatAccumulatorMap.FindRef(StatId);
		check(Accum != NULL);
		// Now change the value
		Accum->Value = Value;
	}

	/**
	 * Sets a counter/accumulator to a specific value in a thread safe way
	 *
	 * @param StatId the stat being set
	 * @param IncBy the amount to set it to
	 */
	inline void SetStatValue(DWORD StatId,DWORD Value)
	{
		// Find the stat that we are updating
		FStatAccumulatorDWORD* Accum = DwordAccumulatorMap.FindRef(StatId);
		check(Accum != NULL);
		// Use the lightweight synch to update
		appInterlockedExchange((INT*)&Accum->Value,Value);
	}

	/**
	 * Sends the stats data to the notifiers to send out via the network. Also,
	 * tells each stat to advance itself after notification
	 */
	void AdvanceFrame(void);

	/**
	 * Advances the stats data for a given thread
	 */
	void AdvanceFrameForThread(void);

	/**
	 * Renders the stats that are enabled at the specified location
	 *
	 * @param RI the render interface to draw with
	 * @param X the X location to start rendering at
	 * @param Y the Y location to start rendering at
	 */
	void Render(class FCanvas* Canvas,INT X,INT Y);

	/**
	 * Processes any stat specific exec commands
	 *
	 * @param Cmd the command to parse
	 * @param Ar the output device to write data to
	 *
	 * @return TRUE if processed, FALSE otherwise
	 */
	UBOOL Exec(const TCHAR* Cmd,FOutputDevice& Ar);

	/**
	 * Returns the string name for the specified group id
	 *
	 * @param GroupId the id to find the name of
	 *
	 * @return the string name of the group
	 */
	const TCHAR* GetGroupName(DWORD GroupId);

	/**
	 * Returns the string name for the specified stat id
	 *
	 * @param StatId the id to find the name of
	 *
	 * @return the string name of the stat
	 */
	const TCHAR* GetStatName(DWORD StatId);

	/**
	 * Determines if this stat group is on the network enabled list. The network
	 * inclusion list is an INI setting used to enable entire stat groups for
	 * sending network information. An empty list means that the group is enabled
	 *
	 * @param InDesc the group description to check
	 *
	 * @return TRUE if the group is on the inclusion list, FALSE otherwise
	 */
	UBOOL IsGroupNetEnabled(const TCHAR* InDesc);

	/** Class for iterating through all of the stat groups */
	class FStatGroupIterator
	{
		/** The first group */
		FStatGroup* Group;

	public:
		/**
		 * Constructs the iterator for the group
		 *
		 * @param InGroup the group to iterate stats for
		 */
		FStatGroupIterator(FStatGroup* InGroup) :
			Group(InGroup)
		{
		}

		/** Moves to the next group in the list */
		void operator++(void)
		{
			Group = Group->NextGroup;
		}

		/** Gets the group being pointed to */
		FStatGroup* operator*(void) const
		{
			return Group;
		}

		/** Returns true if there are more items, false otherwise */
		operator UBOOL(void) const
		{
			return Group != NULL;
		}
	};

	/** Returns an iterator for the groups */
	FStatGroupIterator GetGroupIterator(void)
	{
		return FStatGroupIterator(FirstGroup);
	}

	/** Used to tell the stat manager a group has been made visible */
	FORCEINLINE void IncrementNumRendered(void)
	{
		NumRenderedGroups++;
	}

	/** Used to tell the stat manager a group has been hidden */
	FORCEINLINE void DecrementNumRendered(void)
	{
		NumRenderedGroups--;
	}

	/** Returns number of rendered groups */
	FORCEINLINE DWORD GetNumRenderedGroups(void) const
	{
		return NumRenderedGroups;
	}

	FORCEINLINE EStatRenderingMode GetStatRenderingMode(void) const
	{
		return StatRenderingMode;
	}

	/**
	 * Sets the available memory for a particular region (usually called from appPlatformInit)
	 *
	 * @param Region Region this is setting the max avail for
	 * @param AvailableMem Total available memory for this region on this platform
	 */
	void SetAvailableMemory(EMemoryCounterRegion Region, DWORD AvailableMem)
	{
		// set the max available
		TotalMemAvail[Region] = AvailableMem;
	}

	/**
	 * Gets the available memory for a particular region.
	 *
	 * @param Region Region this is getting the value for
	 */
	DWORD GetAvailableMemory(EMemoryCounterRegion Region)
	{
		return TotalMemAvail[Region];
	}
};

extern FStatManager GStatManager;

#if __GNUC__ // Only gcc needs these changes because of it's strange template handling
/**
 * Zeros the accumulator value and sets the next item in the list
 */
template<class TYPE> TAccumulator<TYPE>::TAccumulator(const TCHAR* InCounterName,
	DWORD InStatId,DWORD InGroupId)  :
	FStatCommonData(InCounterName,InStatId),
	Value(InitialStatValue<TYPE>()),
	Next(NULL)
{
	// Get the group for this stat
	FStatGroup* Group = GStatManager.GetGroup(InGroupId);
	check(Group);
	// And add us to it
	Group->AddToGroup(this);
}

/**
 * Zeros the value and sets the next item in the list
 */
template<class TYPE> TCounter<TYPE>::TCounter(const TCHAR* InCounterName,
	DWORD InStatId,DWORD GroupId) :
	TAccumulator<TYPE>(InCounterName,InStatId)
{
	// Get the group for this stat
	FStatGroup* Group = GStatManager.GetGroup(GroupId);
	check(Group);
	// And add us to it
	Group->AddToGroup(this);
}
#endif

/** Macros for updating counters/accumulators */
#define INC_FLOAT_STAT(StatId) \
	GStatManager.Increment(StatId,1.f)
#define INC_DWORD_STAT(StatId) \
	GStatManager.Increment(StatId,(DWORD)1)
#define INC_FLOAT_STAT_BY(StatId,Amount) \
	GStatManager.Increment(StatId,Amount)
#define INC_DWORD_STAT_BY(StatId,Amount) \
	GStatManager.Increment(StatId,(DWORD)Amount)
#define INC_MEMORY_STAT_BY(StatId,Amount) \
	GStatManager.Increment(StatId,(DWORD)Amount)
#define DEC_FLOAT_STAT(StatId) \
	GStatManager.Decrement(StatId,1.f)
#define DEC_DWORD_STAT(StatId) \
	GStatManager.Decrement(StatId,(DWORD)1)
#define DEC_FLOAT_STAT_BY(StatId,Amount) \
	GStatManager.Decrement(StatId,Amount)
#define DEC_DWORD_STAT_BY(StatId,Amount) \
	GStatManager.Decrement(StatId,(DWORD)Amount)
#define DEC_MEMORY_STAT_BY(StatId,Amount) \
	GStatManager.Decrement(StatId,(DWORD)Amount)
#define SET_DWORD_STAT(StatId,Value) \
	GStatManager.SetStatValue(StatId,(DWORD)Value)
#define SET_FLOAT_STAT(StatId,Value) \
	GStatManager.SetStatValue(StatId,Value)

#if STATS_SLOW
#define INC_FLOAT_STAT_SLOW(StatId) \
	GStatManager.Increment(StatId,1.f)
#define INC_DWORD_STAT_SLOW(StatId) \
	GStatManager.Increment(StatId,(DWORD)1)
#define INC_FLOAT_STAT_BY_SLOW(StatId,Amount) \
	GStatManager.Increment(StatId,Amount)
#define INC_DWORD_STAT_BY_SLOW(StatId,Amount) \
	GStatManager.Increment(StatId,(DWORD)Amount)
#define DEC_FLOAT_STAT_SLOW(StatId) \
	GStatManager.Decrement(StatId,1.f)
#define DEC_DWORD_STAT_SLOW(StatId) \
	GStatManager.Decrement(StatId,(DWORD)1)
#define DEC_FLOAT_STAT_BY_SLOW(StatId,Amount) \
	GStatManager.Decrement(StatId,Amount)
#define DEC_DWORD_STAT_BY_SLOW(StatId,Amount) \
	GStatManager.Decrement(StatId,(DWORD)Amount)
#define SET_DWORD_STAT_SLOW(StatId,Value) \
	GStatManager.SetStatValue(StatId,(DWORD)Value)
#define SET_FLOAT_STAT_SLOW(StatId,Value) \
	GStatManager.SetStatValue(StatId,Value)
#endif // STATS_SLOW

/**
 * This is a utility class for counting the number of cycles during the
 * lifetime of the object. It updates the per thread values for this
 * stat.
 */
class FCycleCounter
{
protected:
	/**
	 * The stat that is being updated
	 */
	FCycleStat* Stat;
	/**
	 * Our start time so we can determine how many cycles have elapsed
	 */
	DWORD StartCycles;

public:
	/**
	 * Zeros the values
	 */
	FCycleCounter(void) :
		Stat(NULL),
		StartCycles(0)
	{
	}

	/**
	 * Pushes the specified stat onto the hierarchy for this thread. Starts
	 * the timing of the cycles used
	 */
	FORCEINLINE void Start(DWORD StatId)
	{
		// Get the current stat in the stack
		FCycleStat* CurrentStat = GStatManager.GetCurrentStat();
		// Handle recursive function stats
		if (CurrentStat->StatId != StatId)
		{
			// Now get child stat for the unique id. Use the global object for
			// simplicity of synchronization
			Stat = GStatManager.GetChildStat(CurrentStat,StatId);
			// Push the child onto the stack
			GStatManager.PushStat(Stat);

			// Push a marker for external profilers.
			appPushMarker( Stat->CounterName );
		}
		else
		{
			// We are recusing so reuse the current stat
			Stat = CurrentStat;
		}

		if ( appInterlockedIncrement((INT*)&Stat->RecursiveCount) == 1 )
		{
			// Grab our start time
			StartCycles = appCycles();
		}
	}

	/**
	 * Stops the capturing and stores the result
	 */
	FORCEINLINE void Stop(void)
	{
		// Only do the timing on the final pop of the stat
		if (Stat->RecursiveCount == 0 ||
			appInterlockedDecrement((INT*)&Stat->RecursiveCount) == 0)
		{
			// Update the number of times this has been called this frame
			appInterlockedIncrement((INT*)&Stat->NumCallsPerFrame);
			// Update the stat in a thread safe way since it could be read on one
			// thread while be updated by this thread
			appInterlockedAdd((INT*)&Stat->Cycles,appCycles() - StartCycles);
			// Now pop this stat off the stack
			GStatManager.PopStat();

			// Pop the marker for external profilers.
			appPopMarker();
		}
	}
};

/**
 * This is a utility class for counting the number of cycles during the
 * lifetime of the object. It updates the per thread values for this
 * stat.
 */
class FScopeCycleCounter : public FCycleCounter
{
	/** Hide this ctor */
	FScopeCycleCounter(void);

public:
	/**
	 * Pushes the specified stat onto the hierarchy for this thread. Starts
	 * the timing of the cycles used
	 */
	FScopeCycleCounter(DWORD StatId)
	{
		if( GIsGame 
		&& (	GStatManager.GetNumRenderedGroups() 
			||	GStatManager.GetStatRenderingMode() != SRM_Grouped
			||	GForceEnableScopedCycleStats ) )
		{
			Start(StatId);
		}
	}

	/**
	 * Updates the stat with the time spent
	 */
	~FScopeCycleCounter(void)
	{
		// this will be non-NULL if the constructot started successfully
		if( Stat )
		{
			Stop();
		}
	}
};

/**
 * Utility class to capture time passed in seconds, adding delta time to passed
 * in variable.
 */
class FScopeSecondsCounter
{
public:
	/** Ctor, capturing start time. */
	FScopeSecondsCounter( DOUBLE& InSeconds )
	:	StartTime(appSeconds())
	,	Seconds(InSeconds)
	{}
	/** Dtor, updating seconds with time delta. */
	~FScopeSecondsCounter()
	{
		Seconds += appSeconds() - StartTime;
	}
private:
	/** Start time, captured in ctor. */
	DOUBLE StartTime;
	/** Time variable to update. */
	DOUBLE& Seconds;
};

/** Macro for updating a seconds counter without creating new scope. */
#define SCOPE_SECONDS_COUNTER(Seconds) \
	FScopeSecondsCounter SecondsCount_##Seconds(Seconds);

// Name the allocation section as well as stats section if desired.
#if TRACK_MEM_USING_STAT_SECTIONS
	/** Macro for declaring a scoped cycle counter class without creating new scope */
	#define SCOPE_CYCLE_COUNTER(Stat) \
		FScopeCycleCounter CycleCount_##Stat(Stat);\
		FScopeAllocSection AllocSec_##Stat(TEXT(#Stat));
#else
#if USE_AG_PERFMON
	/** Macro for declaring a scoped cycle counter class without creating new scope */
	#define SCOPE_CYCLE_COUNTER(Stat) \
		FScopeCycleCounter CycleCount_##Stat(Stat); \
		static unsigned __int16 AGPERFMON_ID_##Stat = UnAgPmRegisterEvent(#Stat); \
		AgPerfMonTimer AGPERFMON_##Stat(AGPERFMON_ID_##Stat);
#else
	/** Macro for declaring a scoped cycle counter class without creating new scope */
	#define SCOPE_CYCLE_COUNTER(Stat) \
		FScopeCycleCounter CycleCount_##Stat(Stat);
#endif
#endif

/** Macro for declaring a local counter */
#define DECLARE_CYCLE_COUNTER(Stat) \
	FCycleCounter CycleCounter_##Stat;
/** Macro to start capturing timing data */
#define START_CYCLE_COUNTER(Stat) \
	CycleCounter_##Stat.Start(Stat);
/** Macro to stop capturing timing data */
#define STOP_CYCLE_COUNTER(Stat) \
	CycleCounter_##Stat.Stop();
/** Macro to set timing data directly */
#define SET_CYCLE_COUNTER(StatId,Cycles,CallCount) \
	GStatManager.SetCycleCounter(StatId,Cycles,CallCount)

#if STATS_SLOW
#define SCOPE_CYCLE_COUNTER_SLOW(Stat) \
	FScopeCycleCounter CycleCount_##Stat(Stat);
#define DECLARE_CYCLE_COUNTER_SLOW(Stat) \
	FCycleCounter CycleCounter_##Stat;
#define START_CYCLE_COUNTER_SLOW(Stat) \
	CycleCounter_##Stat.Start(Stat);
#define STOP_CYCLE_COUNTER_SLOW(Stat) \
	CycleCounter_##Stat.Stop();
#endif // STATS_SLOW


/**
 *	Sets the cycle stat to a specified cycle count and call count
 *
 *	@param StatId		Represents the stat to set
 *	@param Cycles		The cycle count to set
 *	@param CallCount	The call count to set
 */
inline void FStatManager::SetCycleCounter( DWORD StatId, DWORD Cycles, DWORD CallCount )
{
	FCycleStat* Stat = (FCycleStat*)GetCycleStat(StatId);
	if (Stat == NULL)
	{
		{
			SCOPE_CYCLE_COUNTER(StatId);
		}
		Stat = (FCycleStat*)GetCycleStat(StatId);
	}
	if( Stat )
	{
		Stat->Cycles = Cycles;
		Stat->NumCallsPerFrame = CallCount;
	}
}


#else // No stats

#if COMPILER_SUPPORTS_NOOP && 0 // __noops are causing a redefinition error in FINAL_RELEASE on Xenon

// Remove all the macros
#define SCOPE_SECONDS_COUNTER(Seconds) __noop
#define SCOPE_CYCLE_COUNTER(Stat) __noop
#define DECLARE_CYCLE_COUNTER(Stat) __noop
#define START_CYCLE_COUNTER(Stat) __noop
#define STOP_CYCLE_COUNTER(Stat) __noop
#define SET_CYCLE_COUNTER(Stat,Cycles,CallCount) __noop
#define DECLARE_CYCLE_STAT(CounterName,StatId,GroupId) __noop
#define DECLARE_FLOAT_COUNTER_STAT(CounterName,StatId,GroupId) __noop
#define DECLARE_DWORD_COUNTER_STAT(CounterName,StatId,GroupId) __noop
#define DECLARE_FLOAT_ACCUMULATOR_STAT(CounterName,StatId,GroupId) __noop
#define DECLARE_DWORD_ACCUMULATOR_STAT(CounterName,StatId,GroupId) __noop
#define DECLARE_MEMORY_STAT(CounterName,StatId,GroupId) __noop
#define DECLARE_MEMORY_STAT2(CounterName,StatId,GroupId,Region,bDisplayZeroStats) __noop
#define DECLARE_STATS_GROUP(GroupDesc,GroupId) __noop
#define INC_FLOAT_STAT(StatId) __noop
#define INC_DWORD_STAT(StatId) __noop
#define INC_FLOAT_STAT_BY(StatId,Amount) __noop
#define INC_DWORD_STAT_BY(StatId,Amount) __noop
#define INC_MEMORY_STAT_BY(StatId,Amount) __noop
#define DEC_FLOAT_STAT(StatId) __noop
#define DEC_DWORD_STAT(StatId) __noop
#define DEC_FLOAT_STAT_BY(StatId,Amount) __noop
#define DEC_DWORD_STAT_BY(StatId,Amount) __noop
#define DEC_MEMORY_STAT_BY(StatId,Amount) __noop
#define SET_DWORD_STAT(StatId,Value) __noop
#define SET_FLOAT_STAT(StatId,Value) __noop
#define DECLARE_STAT_NOTIFY_PROVIDER_FACTORY(FactoryName,ProviderName,InstanceName) __noop


#else // if compiler does not support __noop


// Remove all the macros
#define SCOPE_SECONDS_COUNTER(Seconds)
#define SCOPE_CYCLE_COUNTER(Stat)
#define DECLARE_CYCLE_COUNTER(Stat)
#define START_CYCLE_COUNTER(Stat)
#define STOP_CYCLE_COUNTER(Stat)
#define SET_CYCLE_COUNTER(Stat,Cycles,CallCount)
#define DECLARE_CYCLE_STAT(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_COUNTER_STAT(CounterName,StatId,GroupId)
#define DECLARE_DWORD_COUNTER_STAT(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_ACCUMULATOR_STAT(CounterName,StatId,GroupId)
#define DECLARE_DWORD_ACCUMULATOR_STAT(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT2(CounterName,StatId,GroupId,Region,bDisplayZeroStats)
#define DECLARE_STATS_GROUP(GroupDesc,GroupId)
#define INC_FLOAT_STAT(StatId)
#define INC_DWORD_STAT(StatId)
#define INC_FLOAT_STAT_BY(StatId,Amount)
#define INC_DWORD_STAT_BY(StatId,Amount)
#define INC_MEMORY_STAT_BY(StatId,Amount)
#define DEC_FLOAT_STAT(StatId)
#define DEC_DWORD_STAT(StatId)
#define DEC_FLOAT_STAT_BY(StatId,Amount)
#define DEC_DWORD_STAT_BY(StatId,Amount)
#define DEC_MEMORY_STAT_BY(StatId,Amount)
#define SET_DWORD_STAT(StatId,Value)
#define SET_FLOAT_STAT(StatId,Value)
#define DECLARE_STAT_NOTIFY_PROVIDER_FACTORY(FactoryName,ProviderName,InstanceName)

#endif // COMPILER_SUPPORTS_NOOP

#endif // STATS


// now we check to see if we should enable the SLOW versions of the stats

#if STATS && STATS_SLOW 
#define DECLARE_CYCLE_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CycleCounter);
#define DECLARE_FLOAT_COUNTER_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CounterFLOAT);
#define DECLARE_DWORD_COUNTER_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_CounterDWORD);
#define DECLARE_FLOAT_ACCUMULATOR_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_AccumulatorFLOAT);
#define DECLARE_DWORD_ACCUMULATOR_STAT_SLOW(CounterName,StatId,GroupId) \
	FStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,STATTYPE_AccumulatorDWORD);
#define DECLARE_MEMORY_STAT_SLOW(CounterName,StatId,GroupId) \
	FMemoryStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,MCR_Physical,TRUE);
#define DECLARE_MEMORY_STAT2_SLOW(CounterName,StatId,GroupId,Region,bDisplayZeroStats) \
	FMemoryStatFactory StatFactory_##StatId(CounterName,StatId,GroupId,Region,bDisplayZeroStats);
#else
#define DECLARE_CYCLE_STAT_SLOW(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_COUNTER_STAT_SLOW(CounterName,StatId,GroupId)
#define DECLARE_DWORD_COUNTER_STAT_SLOW(CounterName,StatId,GroupId)
#define DECLARE_FLOAT_ACCUMULATOR_STAT_SLOW(CounterName,StatId,GroupId)
#define DECLARE_DWORD_ACCUMULATOR_STAT_SLOW(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT_SLOW(CounterName,StatId,GroupId)
#define DECLARE_MEMORY_STAT2_SLOW(CounterName,StatId,GroupId,Region,bDisplayZeroStats)
#endif // #if STATS && STATS_SLOW 

#if STATS && STATS_SLOW 
#define SCOPE_CYCLE_COUNTER_SLOW(Stat) \
	FScopeCycleCounter CycleCount_##Stat(Stat);
#define DECLARE_CYCLE_COUNTER_SLOW(Stat) \
	FCycleCounter CycleCounter_##Stat;
#define START_CYCLE_COUNTER_SLOW(Stat) \
	CycleCounter_##Stat.Start(Stat);
#define STOP_CYCLE_COUNTER_SLOW(Stat) \
	CycleCounter_##Stat.Stop();
#else
#define SCOPE_CYCLE_COUNTER_SLOW(Stat)
#define DECLARE_CYCLE_COUNTER_SLOW(Stat)
#define START_CYCLE_COUNTER_SLOW(Stat)
#define STOP_CYCLE_COUNTER_SLOW(Stat)
#endif // #if STATS && STATS_SLOW 

#if STATS && STATS_SLOW 
#define INC_FLOAT_STAT_SLOW(StatId) \
	GStatManager.Increment(StatId,1.f)
#define INC_DWORD_STAT_SLOW(StatId) \
	GStatManager.Increment(StatId,(DWORD)1)
#define INC_FLOAT_STAT_BY_SLOW(StatId,Amount) \
	GStatManager.Increment(StatId,Amount)
#define INC_DWORD_STAT_BY_SLOW(StatId,Amount) \
	GStatManager.Increment(StatId,(DWORD)Amount)
#define DEC_FLOAT_STAT_SLOW(StatId) \
	GStatManager.Decrement(StatId,1.f)
#define DEC_DWORD_STAT_SLOW(StatId) \
	GStatManager.Decrement(StatId,(DWORD)1)
#define DEC_FLOAT_STAT_BY_SLOW(StatId,Amount) \
	GStatManager.Decrement(StatId,Amount)
#define DEC_DWORD_STAT_BY_SLOW(StatId,Amount) \
	GStatManager.Decrement(StatId,(DWORD)Amount)
#define SET_DWORD_STAT_SLOW(StatId,Value) \
	GStatManager.SetStatValue(StatId,(DWORD)Value)
#define SET_FLOAT_STAT_SLOW(StatId,Value) \
	GStatManager.SetStatValue(StatId,Value)
#else
#define INC_FLOAT_STAT_SLOW(StatId)
#define INC_DWORD_STAT_SLOW(StatId)
#define INC_FLOAT_STAT_BY_SLOW(StatId,Amount)
#define INC_DWORD_STAT_BY_SLOW(StatId,Amount)
#define DEC_FLOAT_STAT_SLOW(StatId)
#define DEC_DWORD_STAT_SLOW(StatId)
#define DEC_FLOAT_STAT_BY_SLOW(StatId,Amount)
#define DEC_DWORD_STAT_BY_SLOW(StatId,Amount)
#define SET_DWORD_STAT_SLOW(StatId,Value)
#define SET_FLOAT_STAT_SLOW(StatId,Value)
#endif // #if STATS && STATS_SLOW 




