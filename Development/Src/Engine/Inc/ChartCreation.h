/** 
 * ChartCreation
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#ifndef _CHART_CREATION_INC_
#define _CHART_CREATION_INC_


/*-----------------------------------------------------------------------------
	FPS chart support.
-----------------------------------------------------------------------------*/

/** Helper structure for FPS chart entries. */
struct FFPSChartEntry
{
	/** Number of frames in bucket. */
	INT		Count;
	/** Cumulative time spent in bucket. */
	DOUBLE	CummulativeTime;
};

/** Start time of current FPS chart.										*/
extern DOUBLE			GFPSChartStartTime;
/** FPS chart information. Time spent for each bucket and count.			*/
extern FFPSChartEntry	GFPSChart[STAT_FPSChartLastBucketStat - STAT_FPSChartFirstStat];

#define NUM_DISTANCEFACTOR_BUCKETS	(14)
extern INT GDistanceFactorChart[NUM_DISTANCEFACTOR_BUCKETS];
extern FLOAT GDistanceFactorDivision[NUM_DISTANCEFACTOR_BUCKETS-1];


/** The GPU time taken to render the last frame. Same metric as appCycles(). */
extern DWORD			GGPUFrameTime;
/** The total GPU time taken to render all frames. In seconds. */
extern DOUBLE			GTotalGPUTime;



/*-----------------------------------------------------------------------------
  Memory chart support.
-----------------------------------------------------------------------------*/

/** Helper structure for Memory chart entries. */
struct FMemoryChartEntry
{
	// this will have all of the extensive memory stats and all of the stat memory entries (including the novodex)

	// stored in KB (as otherwise it is too hard to read)
	DOUBLE PhysicalMemUsed;
	DOUBLE VirtualMemUsed;

	DOUBLE AudioMemUsed;
	DOUBLE TextureMemUsed;
	DOUBLE NovodexMemUsed;
	DOUBLE VertexLightingMemUsed;

	DOUBLE StaticMeshVertexMemUsed;
	DOUBLE StaticMeshIndexMemUsed;
	DOUBLE SkeletalMeshVertexMemUsed;
	DOUBLE SkeletalMeshIndexMemUsed;

	DOUBLE VertexShaderMemUsed;
	DOUBLE PixelShaderMemUsed;

	//
	DOUBLE PhysicalTotal;
	DOUBLE GPUTotal;

	DOUBLE GPUMemUsed;

	DOUBLE NumAllocations;
	DOUBLE AllocationOverhead;
	DOUBLE AllignmentWaste;


	// Log:   Total allocated from OS:     244.02 MByte (255876792 Bytes)  
	// Log:   Total allocated to game:     241.18 MByte (252893196 Bytes)  
	// Log:   Average alignment waste:       9.13 Bytes (Over 716617 allocations)  
	// Log:   Expected requested by game:  234.94 MByte (246351856 Bytes)  
	// Log:   Malloc bookkeeping overhead:   2.85 MByte (2983596 Bytes)  
	// Log:   Expected alignment overhead:   6.24 MByte (6541340 Bytes)  
	// Log:   Total expected overhead:       9.08 MByte (9524936 Bytes)  
	// Log: GPU memory info:               231.91 MByte total  
	// Log:   Total allocated:             221.09 MByte (231827000 Bytes)  
	// Log: Misc allocation info:  
	// Log:   GPU mem bookkeeping:         617.68 KByte  

	FMemoryChartEntry():
	PhysicalMemUsed( -1 )
		,VirtualMemUsed( -1 )

		,AudioMemUsed( -1 )
		,TextureMemUsed( -1 )
		,NovodexMemUsed( -1 )
		,VertexLightingMemUsed( -1 )

		,StaticMeshVertexMemUsed( -1 )
		,StaticMeshIndexMemUsed( -1 )
		,SkeletalMeshVertexMemUsed( -1 )
		,SkeletalMeshIndexMemUsed( -1 )

		,VertexShaderMemUsed( -1 )
		,PixelShaderMemUsed( -1 )

		//
		,PhysicalTotal( -1 )
		,GPUTotal( -1 )

		,GPUMemUsed( -1 )

		,NumAllocations( -1 )
		,AllocationOverhead( -1 )
		,AllignmentWaste( -1 )

	{};

	/** This will update the engine level platform agnostic memory stats */
	void UpdateMemoryChartStats();

	FString ToString() const;
	FString ToHTMLString() const;

};

/** How long between memory chart updates **/
extern FLOAT GTimeBetweenMemoryChartUpdates;

/** When the memory chart was last updated **/
extern DOUBLE GLastTimeMemoryChartWasUpdated;


extern TArray<FMemoryChartEntry> GMemoryChart;





#endif // _CHART_CREATION_INC_


