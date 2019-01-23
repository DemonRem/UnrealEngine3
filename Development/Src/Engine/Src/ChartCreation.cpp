/** 
 * ChartCreation
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"
#include "ChartCreation.h"



/** Start time of current FPS chart.										*/
DOUBLE			GFPSChartStartTime;
/** FPS chart information. Time spent for each bucket and count.			*/
FFPSChartEntry	GFPSChart[STAT_FPSChartLastBucketStat - STAT_FPSChartFirstStat];
/** Chart of DistanceFactors that skeletal meshes are rendered at. */
INT				GDistanceFactorChart[NUM_DISTANCEFACTOR_BUCKETS];
/** Range for each DistanceFactor bucket. */
FLOAT			GDistanceFactorDivision[NUM_DISTANCEFACTOR_BUCKETS-1]={0.01f,0.02f,0.03f,0.05f,0.075f,0.1f,0.15f,0.25f,0.5f,0.75f,1.f,1.5f,2.f};

/** The GPU time taken to render the last frame. Same metric as appCycles(). */
DWORD			GGPUFrameTime;
/** The total GPU time taken to render all frames. In seconds. */
DOUBLE			GTotalGPUTime;


// Memory 
/** Contains all of the memory info */
TArray<FMemoryChartEntry> GMemoryChart;

/** How long between memory chart updates **/
FLOAT GTimeBetweenMemoryChartUpdates = 0;

/** When the memory chart was last updated **/
DOUBLE GLastTimeMemoryChartWasUpdated = 0;




/** 
 * This will look at where the map is doing a bot test or a flythrough test.
 *
 * @ return the name of the chart to use
 **/
static FString GetFPSChartType()
{
	FString Retval;

	// causeevent=automatedperftesting  is what the FlyThrough event is
	// look for the string automatedperftesting and don't care about case
	if( FString(appCmdLine()).InStr( TEXT( "causeevent=automatedperftesting" ), FALSE, TRUE ) != -1 )
	{
		Retval = TEXT ( "FlyThrough" );
	}
	else
	{
		Retval = TEXT ( "FPS" );
	}


	return Retval;
}


/**
 * This will get the changelist that should be used with the Automated Performance Testing
 * If one is passed in we use that otherwise we use the GBuiltFromChangeList.  This allows
 * us to have build machine built .exe and test them. 
 *
 * NOTE: had to use AutomatedBenchmarking as the parsing code is flawed and doesn't match
 *       on whole words.  so automatedperftestingChangelist was failing :-( 
 **/
static INT GetChangeListNumberForPerfTesting()
{
	INT Retval = GBuiltFromChangeList;

	// if we have passed in the changelist to use then use it
	INT FromCommandLine = 0;
	Parse( appCmdLine(), TEXT("-AutomatedBenchmarkingChangelist="), FromCommandLine );

	// we check for 0 here as the CIS always appends -AutomatedPerfChangelist but if it
	// is from the "built" builds when it will be a 0
	if( FromCommandLine != 0 )
	{
		Retval = FromCommandLine;
	}

	return Retval;
}


/**
 * This will create the file name for the file we are saving out.
 *
 * @param the type of chart we are making
 **/
FString CreateFileNameForChart( const FString& ChartType, const FString& FileExtension, UBOOL bOutputToGlobalLog )
{
	FString Retval;

	// Map name we're dumping.
	FString MapName;
	if( bOutputToGlobalLog == TRUE )
	{
		MapName = TEXT( "Global" );
	}
	else
	{
		MapName = GWorld ? GWorld->GetMapName() : TEXT("None");
	}


	// Create FPS chart filename.
	FString Platform;
	// determine which platform we are
#if XBOX
	Platform = TEXT( "Xbox360" );
#elif PS3
	Platform = TEXT( "PS3" );
#else
	Platform = TEXT( "PC" );
#endif

	Retval = MapName + TEXT("-") + ChartType + TEXT("-") + Platform + FileExtension;

	return Retval;
}




/**
* Ticks the FPS chart.
*
* @param DeltaSeconds	Time in seconds passed since last tick.
*/
void UEngine::TickFPSChart( FLOAT DeltaSeconds )
{
	// Use our own delta time if we're benchmarking.
	if( GIsBenchmarking )
	{
		static DOUBLE LastTime = 0;
		const DOUBLE CurrentTime = appSeconds();
		if( LastTime > 0 )
		{
			DeltaSeconds = CurrentTime - LastTime;
		}
		LastTime = CurrentTime;
	}

	// Disregard frames that took longer than one second when accumulating data.
	if( DeltaSeconds < 1.f )
	{
		FLOAT CurrentFPS = 1 / DeltaSeconds;

		if( CurrentFPS < 5 )
		{
			GFPSChart[ STAT_FPSChart_0_5 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_0_5 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 10 )
		{
			GFPSChart[ STAT_FPSChart_5_10 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_5_10 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 15 )
		{
			GFPSChart[ STAT_FPSChart_10_15 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_10_15 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 20 )
		{
			GFPSChart[ STAT_FPSChart_15_20 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_15_20 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 25 )
		{
			GFPSChart[ STAT_FPSChart_20_25 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_20_25 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 30 )
		{
			GFPSChart[ STAT_FPSChart_25_30 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_25_30 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 35 )
		{
			GFPSChart[ STAT_FPSChart_30_35 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_30_35 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 40 )
		{
			GFPSChart[ STAT_FPSChart_35_40 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_35_40 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 45 )
		{
			GFPSChart[ STAT_FPSChart_40_45 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_40_45 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 50 )
		{
			GFPSChart[ STAT_FPSChart_45_50 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_45_50 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 55 )
		{
			GFPSChart[ STAT_FPSChart_50_55 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_50_55 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else if( CurrentFPS < 60 )
		{
			GFPSChart[ STAT_FPSChart_55_60 - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_55_60 - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
		else
		{
			GFPSChart[ STAT_FPSChart_60_INF - STAT_FPSChartFirstStat ].Count++;
			GFPSChart[ STAT_FPSChart_60_INF - STAT_FPSChartFirstStat ].CummulativeTime += DeltaSeconds;
		}
	}

	GTotalGPUTime += GGPUFrameTime * GSecondsPerCycle;

#if STATS
	// Propagate gathered data to stats system.

	// Iterate over all buckets, gathering total frame count and cumulative time.
	FLOAT TotalTime = 0;
	INT	  NumFrames = 0;
	for( INT BucketIndex=0; BucketIndex<ARRAY_COUNT(GFPSChart); BucketIndex++ )
	{
		NumFrames += GFPSChart[BucketIndex].Count;
		TotalTime += GFPSChart[BucketIndex].CummulativeTime;
	}

	// Set percentage stats.
	FLOAT ThirtyPlusPercentage = 0;
	for( INT BucketIndex=0; BucketIndex<ARRAY_COUNT(GFPSChart); BucketIndex++ )
	{
		FLOAT BucketPercentage = 0.f;
		if( TotalTime > 0.f )
		{
			BucketPercentage = (GFPSChart[BucketIndex].CummulativeTime / TotalTime) * 100;
		}
		if( (BucketIndex + STAT_FPSChartFirstStat) >= STAT_FPSChart_30_35 )
		{
			ThirtyPlusPercentage += BucketPercentage;
		}
		SET_FLOAT_STAT( BucketIndex + STAT_FPSChartFirstStat, BucketPercentage );
	}

	// Update unaccounted time and frame count stats.
	const FLOAT UnaccountedTime = appSeconds() - GFPSChartStartTime - TotalTime;
	SET_FLOAT_STAT( STAT_FPSChart_30Plus, ThirtyPlusPercentage );
	SET_FLOAT_STAT( STAT_FPSChart_UnaccountedTime, UnaccountedTime );
	SET_DWORD_STAT( STAT_FPSChart_FrameCount, NumFrames );
#endif
}

/**
* Resets the FPS chart data.
*/
void UEngine::ResetFPSChart()
{
	for( INT BucketIndex=0; BucketIndex<ARRAY_COUNT(GFPSChart); BucketIndex++ )
	{
		GFPSChart[BucketIndex].Count = 0;
		GFPSChart[BucketIndex].CummulativeTime = 0;
	}
	GFPSChartStartTime = appSeconds();

	GTotalGPUTime = 0;
}


/**
* Dumps the FPS chart information to the log.
*/
void UEngine::DumpFPSChartToLog( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames )
{
	// Map name we're dumping.
	FString MapName	= GWorld ? GWorld->GetMapName() : TEXT("None");

	debugf(TEXT("--- Begin : FPS chart dump for level '%s'"), *MapName);

	// Iterate over all buckets, dumping percentages.
	for( INT BucketIndex=0; BucketIndex<ARRAY_COUNT(GFPSChart); BucketIndex++ )
	{
		// Figure out bucket time and frame percentage.
		const FLOAT BucketTimePercentage  = 100.f * GFPSChart[BucketIndex].CummulativeTime / TotalTime;
		const FLOAT BucketFramePercentage = 100.f * GFPSChart[BucketIndex].Count / NumFrames;

		// Figure out bucket range.
		const INT StartFPS	= BucketIndex * 5;
		INT EndFPS		= StartFPS + 5;
		if( BucketIndex + STAT_FPSChartFirstStat == STAT_FPSChart_60_INF )
		{
			EndFPS = 99;
		}

		// Log bucket index, time and frame Percentage.
		debugf(TEXT("Bucket: %2i - %2i  Time: %5.2f  Frame: %5.2f"), StartFPS, EndFPS, BucketTimePercentage, BucketFramePercentage);
	}

	debugf(TEXT("%i frames collected over %4.2f seconds, disregarding %4.2f seconds for a %4.2f FPS average"), 
		NumFrames, 
		DeltaTime, 
		Max<FLOAT>( 0, DeltaTime - TotalTime ), 
		NumFrames / TotalTime );
	debugf(TEXT("Average GPU frametime: %4.2f ms"), FLOAT((GTotalGPUTime / NumFrames)*1000.0));
	debugf(TEXT("--- End"));
}

void UEngine::DumpDistanceFactorChart()
{
	INT TotalStats = 0;
	for(INT i=0; i<NUM_DISTANCEFACTOR_BUCKETS; i++)
	{
		TotalStats += GDistanceFactorChart[i];
	}
	const FLOAT TotalStatsF = (FLOAT)TotalStats;

	if(TotalStats > 0)
	{
		debugf(TEXT("--- DISTANCEFACTOR CHART ---"));
		debugf(TEXT("<%2.3f\t%3.2f"), GDistanceFactorDivision[0], 100.f*GDistanceFactorChart[0]/TotalStatsF);
		for(INT i=1; i<NUM_DISTANCEFACTOR_BUCKETS-1; i++)
		{
			const FLOAT BucketStart = GDistanceFactorDivision[i-1];
			const FLOAT BucketEnd = GDistanceFactorDivision[i];
			debugf(TEXT("%2.3f-%2.3f\t%3.2f"), BucketStart, BucketEnd, 100.f*GDistanceFactorChart[i]/TotalStatsF);
		}
		debugf(TEXT(">%2.3f\t%3.2f"), GDistanceFactorDivision[NUM_DISTANCEFACTOR_BUCKETS-2], 100.f*GDistanceFactorChart[NUM_DISTANCEFACTOR_BUCKETS-1]/TotalStatsF);
	}
}


/**
* Dumps the FPS chart information to the special stats log file.
*/
void UEngine::DumpFPSChartToStatsLog( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames )
{
	// Map name we're dumping.
	FString MapName	= GWorld ? GWorld->GetMapName() : TEXT("None");

	// Create folder for FPS chart data.
	FString OutputDir = appGameDir() + TEXT("Stats") + PATH_SEPARATOR;
	GFileManager->MakeDirectory( *OutputDir );
	// Create archive for log data.
	const FString ChartType = GetFPSChartType();

	FArchive* OutputFile = GFileManager->CreateFileWriter( *(OutputDir + CreateFileNameForChart( ChartType, TEXT( ".log" ), FALSE ) ), FILEWRITE_Append );

	if( OutputFile )
	{
		OutputFile->Logf(TEXT("Dumping FPS chart at %s using build %i built from changelist %i"), *appSystemTimeString(), GEngineVersion, GetChangeListNumberForPerfTesting() );

		// Iterate over all buckets, dumping percentages.
		for( INT BucketIndex=0; BucketIndex<ARRAY_COUNT(GFPSChart); BucketIndex++ )
		{
			// Figure out bucket time and frame percentage.
			const FLOAT BucketTimePercentage  = 100.f * GFPSChart[BucketIndex].CummulativeTime / TotalTime;
			const FLOAT BucketFramePercentage = 100.f * GFPSChart[BucketIndex].Count / NumFrames;

			// Figure out bucket range.
			const INT StartFPS	= BucketIndex * 5;
			INT EndFPS		= StartFPS + 5;
			if( BucketIndex + STAT_FPSChartFirstStat == STAT_FPSChart_60_INF )
			{
				EndFPS = 99;
			}

			// Log bucket index, time and frame Percentage.
			OutputFile->Logf(TEXT("Bucket: %2i - %2i  Time: %5.2f  Frame: %5.2f"), StartFPS, EndFPS, BucketTimePercentage, BucketFramePercentage);
		}

		OutputFile->Logf(TEXT("%i frames collected over %4.2f seconds, disregarding %4.2f seconds for a %4.2f FPS average"), 
			NumFrames, 
			DeltaTime, 
			Max<FLOAT>( 0, DeltaTime - TotalTime ), 
			NumFrames / TotalTime );
		OutputFile->Logf(TEXT("Average GPU frame time: %4.2f ms"), FLOAT((GTotalGPUTime / NumFrames)*1000.0));
		OutputFile->Logf( LINE_TERMINATOR LINE_TERMINATOR LINE_TERMINATOR );

		// Flush, close and delete.
		delete OutputFile;
	}
}

/**
* Dumps the FPS chart information to HTML.
*/
void UEngine::DumpFPSChartToHTML( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames, UBOOL bOutputToGlobalLog )
{
	// Load the HTML building blocks from the Engine\Stats folder.
	FString FPSChartPreamble;
	FString FPSChartPostamble;
	FString FPSChartRow;
	UBOOL	bAreAllHTMLPartsLoaded = TRUE;
	bAreAllHTMLPartsLoaded = bAreAllHTMLPartsLoaded && appLoadFileToString( FPSChartPreamble,	*(appEngineDir() + TEXT("Stats\\FPSChart_Preamble.html")	) );
	bAreAllHTMLPartsLoaded = bAreAllHTMLPartsLoaded && appLoadFileToString( FPSChartPostamble,	*(appEngineDir() + TEXT("Stats\\FPSChart_Postamble.html")	) );
	bAreAllHTMLPartsLoaded = bAreAllHTMLPartsLoaded && appLoadFileToString( FPSChartRow,		*(appEngineDir() + TEXT("Stats\\FPSChart_Row.html")			) );

	// Successfully loaded all HTML templates.
	if( bAreAllHTMLPartsLoaded )
	{
		// Keep track of percentage of time at 30+ FPS.
		FLOAT PctTimeAbove30 = 0;

		// Iterate over all buckets, updating row 
		for( INT BucketIndex=0; BucketIndex<ARRAY_COUNT(GFPSChart); BucketIndex++ )
		{
			// Figure out bucket time and frame percentage.
			const FLOAT BucketTimePercentage  = 100.f * GFPSChart[BucketIndex].CummulativeTime / TotalTime;
			const FLOAT BucketFramePercentage = 100.f * GFPSChart[BucketIndex].Count / NumFrames;

			// Figure out bucket range.
			const INT StartFPS	= BucketIndex * 5;
			INT EndFPS		= StartFPS + 5;
			if( BucketIndex + STAT_FPSChartFirstStat == STAT_FPSChart_60_INF )
			{
				EndFPS = 99;
			}

			// Keep track of time spent at 30+ FPS.
			if( StartFPS >= 30 )
			{
				PctTimeAbove30 += BucketTimePercentage;
			}

			const FString SrcToken = FString::Printf( TEXT("TOKEN_%i_%i"), StartFPS, EndFPS );
			const FString DstToken = FString::Printf( TEXT("%5.2f"), BucketTimePercentage );

			// Replace token with actual values.
			FPSChartRow	= FPSChartRow.Replace( *SrcToken, *DstToken );
		}


		// Update non- bucket stats.
		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_MAPNAME"),		    *FString::Printf(TEXT("%s"), GWorld ? *GWorld->GetMapName() : TEXT("None") ) );
		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_CHANGELIST"),		*FString::Printf(TEXT("%i"), GetChangeListNumberForPerfTesting() ) );
		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_DATESTAMP"),         *FString::Printf(TEXT("%s"), *appSystemTimeString() ) );

		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_AVG_FPS"),			*FString::Printf(TEXT("%4.2f"), NumFrames / TotalTime) );
		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_PCT_ABOVE_30"),		*FString::Printf(TEXT("%4.2f"), PctTimeAbove30) );
		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_TIME_DISREGARDED"),	*FString::Printf(TEXT("%4.2f"), Max<FLOAT>( 0, DeltaTime - TotalTime ) ) );
		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_TIME"),				*FString::Printf(TEXT("%4.2f"), DeltaTime) );
		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_FRAMECOUNT"),		*FString::Printf(TEXT("%i"), NumFrames) );
		FPSChartRow = FPSChartRow.Replace( TEXT("TOKEN_AVG_GPUTIME"),		*FString::Printf(TEXT("%4.2f ms"), FLOAT((GTotalGPUTime / NumFrames)*1000.0) ) );

		// Create folder for FPS chart data.
		const FString OutputDir = appGameDir() + TEXT("Stats") + PATH_SEPARATOR;
		GFileManager->MakeDirectory( *OutputDir );

		// Map name we're dumping.
		const FString MapName = GWorld ? GWorld->GetMapName() : TEXT("None");

		// Create FPS chart filename.
		const FString ChartType = GetFPSChartType();

		const FString& FPSChartFilename = OutputDir + CreateFileNameForChart( ChartType, TEXT( ".html" ), bOutputToGlobalLog );
		FString FPSChart;

		// See whether file already exists and load it into string if it does.
		if( appLoadFileToString( FPSChart, *FPSChartFilename ) )
		{
			// Split string where we want to insert current row.
			const FString HeaderSeparator = TEXT("<UE3></UE3>");
			FString FPSChartBeforeCurrentRow, FPSChartAfterCurrentRow;
			FPSChart.Split( *HeaderSeparator, &FPSChartBeforeCurrentRow, &FPSChartAfterCurrentRow );

			// Assemble FPS chart by inserting current row at the top.
			FPSChart = FPSChartPreamble + FPSChartRow + FPSChartAfterCurrentRow;
		}
		// Assemble from scratch.
		else
		{
			FPSChart = FPSChartPreamble + FPSChartRow + FPSChartPostamble;
		}

		// Save the resulting file back to disk.
		appSaveStringToFile( FPSChart, *FPSChartFilename );
	}
	else
	{
		debugf(TEXT("Missing FPS chart template files."));
	}
}

/**
* Dumps the FPS chart information to the passed in archive.
*/
void UEngine::DumpFPSChart()
{
#if FINAL_RELEASE_DEBUGCONSOLE || !FINAL_RELEASE || !CONSOLE
	// Iterate over all buckets, gathering total frame count and cumulative time.
	FLOAT TotalTime = 0;
	const FLOAT DeltaTime = appSeconds() - GFPSChartStartTime;
	INT	  NumFrames = 0;
	for( INT BucketIndex=0; BucketIndex<ARRAY_COUNT(GFPSChart); BucketIndex++ )
	{
		NumFrames += GFPSChart[BucketIndex].Count;
		TotalTime += GFPSChart[BucketIndex].CummulativeTime;
	}

	UBOOL bMemoryChartIsActive = FALSE;
	ParseUBOOL( appCmdLine(), TEXT("-CaptureMemoryChartInfo="), bMemoryChartIsActive );

	if( ( TotalTime > 0.f ) 
		&& ( NumFrames > 0 )
		&& ( bMemoryChartIsActive == FALSE ) // we do not want to dump out FPS stats if we have been gathering mem stats as that will probably throw off the stats
		)
	{
		// Log chart info to the log.
		DumpFPSChartToLog( TotalTime, DeltaTime, NumFrames );

		// Only log FPS chart data to file in the game and not PIE.
		if( GIsGame && !GIsEditor )
		{
			DumpFPSChartToStatsLog( TotalTime, DeltaTime, NumFrames );
			DumpFPSChartToHTML( TotalTime, DeltaTime, NumFrames, FALSE );
			DumpFPSChartToHTML( TotalTime, DeltaTime, NumFrames, TRUE );
		}
	}
#endif
}

/**
* Ticks the Memory chart.
*
* @param DeltaSeconds	Time in seconds passed since last tick.
*/
void UEngine::TickMemoryChart( FLOAT DeltaSeconds )
{
#if !FINAL_RELEASE

	static UBOOL bHasCheckedForCommandLineOption = FALSE;
	static UBOOL bMemoryChartIsActive = FALSE;

	if( bHasCheckedForCommandLineOption == FALSE )
	{
		Parse( appCmdLine(), TEXT("-TimeBetweenMemoryChartUpdates="), GTimeBetweenMemoryChartUpdates );

		// check to see if we have a value  else default this to 30.0 seconds
		if( GTimeBetweenMemoryChartUpdates == 0.0f )
		{
			GTimeBetweenMemoryChartUpdates = 30.0f;  
		}

		ParseUBOOL( appCmdLine(),TEXT("-CaptureMemoryChartInfo="), bMemoryChartIsActive );

		bHasCheckedForCommandLineOption = TRUE;
	}


	if( bMemoryChartIsActive == TRUE )
	{
		const DOUBLE TimeSinceLastUpdate = appSeconds() - GLastTimeMemoryChartWasUpdated;

		// test to see if we should update the memory chart this tick
		if( TimeSinceLastUpdate > GTimeBetweenMemoryChartUpdates )
		{
			FMemoryChartEntry NewMemoryEntry = FMemoryChartEntry();
			NewMemoryEntry.UpdateMemoryChartStats();

			GMemoryChart.AddItem( NewMemoryEntry );

			GLastTimeMemoryChartWasUpdated = appSeconds();
		}
	}

#endif //!FINAL_RELEASE
}



void FMemoryChartEntry::UpdateMemoryChartStats()
{
#if !FINAL_RELEASE

	const FStatGroup* MemGroup = GStatManager.GetGroup( STATGROUP_Memory );

	const FMemoryCounter* Curr = MemGroup->FirstMemoryCounter;

	while( Curr != NULL )
	{
		const EMemoryStats CurrStat = static_cast<EMemoryStats>(Curr->StatId);

		switch( CurrStat )
		{
			// stored in KB (as otherwise it is too hard to read)
#if !PS3
		case STAT_VirtualAllocSize: PhysicalMemUsed = Curr->Value/1024.0f; break;
#endif // !PS3

		case STAT_PhysicalAllocSize: VirtualMemUsed = Curr->Value/1024.0f; break;

		case STAT_AudioMemory: AudioMemUsed = Curr->Value/1024.0f; break;
		case STAT_TextureMemory: TextureMemUsed = Curr->Value/1024.0f; break;
		case STAT_MemoryNovodexTotalAllocationSize: NovodexMemUsed = Curr->Value/1024.0f; break;
		case STAT_VertexLightingMemory: VertexLightingMemUsed = Curr->Value/1024.0f; break;

		case STAT_StaticMeshVertexMemory: StaticMeshVertexMemUsed = Curr->Value/1024.0f; break;
		case STAT_StaticMeshIndexMemory: StaticMeshIndexMemUsed = Curr->Value/1024.0f; break;
		case STAT_SkeletalMeshVertexMemory: SkeletalMeshVertexMemUsed = Curr->Value/1024.0f; break;
		case STAT_SkeletalMeshIndexMemory: SkeletalMeshIndexMemUsed = Curr->Value/1024.0f; break;

		case STAT_VertexShaderMemory: VertexShaderMemUsed = Curr->Value/1024.0f; break;
		case STAT_PixelShaderMemory: PixelShaderMemUsed = Curr->Value/1024.0f; break;

		default: break;
		}

		Curr = (const FMemoryCounter*)Curr->Next;
	}


	PhysicalTotal = GStatManager.GetAvailableMemory( MCR_Physical )/1024.0f;
	GPUTotal = GStatManager.GetAvailableMemory( MCR_GPU )/1024.0f;


	UBOOL bDoDetailedMemStatGathering = FALSE;
	ParseUBOOL( appCmdLine(),TEXT("-DoDetailedMemStatGathering="), bDoDetailedMemStatGathering );
	if( bDoDetailedMemStatGathering == TRUE )
	{
		// call the platform specific version to fill in the goodies
		appUpdateMemoryChartStats( *this );
	}

#endif //!FINAL_RELEASE
}


/**
* Resets the Memory chart data.
*/
void UEngine::ResetMemoryChart()
{
	GMemoryChart.Empty();
}

/**
* Dumps the Memory chart information to various places.
*/
void UEngine::DumpMemoryChart()
{
	if( GMemoryChart.Num() > 0 )
	{
		// Only log FPS chart data to file in the game and not PIE.
		if( GIsGame && !GIsEditor )
		{
			DumpMemoryChartToStatsLog( 0, 0, 0 );
			DumpMemoryChartToHTML( 0, 0, 0, FALSE );
			DumpMemoryChartToHTML( 0, 0, 0, TRUE );
		}
	}
}

/**
* Dumps the Memory chart information to HTML.
*/
void UEngine::DumpMemoryChartToHTML( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames, UBOOL bOutputToGlobalLog )
{
	// Load the HTML building blocks from the Engine\Stats folder.
	FString MemoryChartPreamble;
	FString MemoryChartPostamble;
	FString MemoryChartRowTemplate;
	UBOOL	bAreAllHTMLPartsLoaded = TRUE;
	bAreAllHTMLPartsLoaded = bAreAllHTMLPartsLoaded && appLoadFileToString( MemoryChartPreamble,	*(appEngineDir() + TEXT("Stats\\MemoryChart_Preamble.html")	) );
	bAreAllHTMLPartsLoaded = bAreAllHTMLPartsLoaded && appLoadFileToString( MemoryChartPostamble,	*(appEngineDir() + TEXT("Stats\\MemoryChart_Postamble.html")	) );
	bAreAllHTMLPartsLoaded = bAreAllHTMLPartsLoaded && appLoadFileToString( MemoryChartRowTemplate,		*(appEngineDir() + TEXT("Stats\\MemoryChart_Row.html")			) );

	FString MemoryChartRows;

	// Successfully loaded all HTML templates.
	if( bAreAllHTMLPartsLoaded )
	{
		// Iterate over all data
		for( INT MemoryIndex=0; MemoryIndex < GMemoryChart.Num(); ++MemoryIndex )
		{
			// so for the outputting to the global file we only output the first and last entry in the saved data
			if( ( bOutputToGlobalLog == TRUE ) && 
				( ( MemoryIndex != 0 ) && ( MemoryIndex != GMemoryChart.Num() - 1 ) )
				)
			{
				continue;
			}

			// add a new row to the table
			MemoryChartRows += MemoryChartRowTemplate + GMemoryChart(MemoryIndex).ToHTMLString();
			MemoryChartRows = MemoryChartRows.Replace( TEXT("TOKEN_DURATION"), *FString::Printf(TEXT("%5.2f"), (GTimeBetweenMemoryChartUpdates * MemoryIndex)  ) );
			MemoryChartRows += TEXT( "</TR>" );
		}

		// Update non- bucket stats.
		MemoryChartRows = MemoryChartRows.Replace( TEXT("TOKEN_MAPNAME"),		*FString::Printf(TEXT("%s"), GWorld ? *GWorld->GetMapName() : TEXT("None") ) );
		MemoryChartRows = MemoryChartRows.Replace( TEXT("TOKEN_CHANGELIST"), *FString::Printf(TEXT("%i"), GetChangeListNumberForPerfTesting() ) );
		MemoryChartRows = MemoryChartRows.Replace( TEXT("TOKEN_DATESTAMP"), *FString::Printf(TEXT("%s"), *appSystemTimeString() ) );

		// Create folder for Memory chart data.
		const FString OutputDir = appGameDir() + TEXT("Stats") + PATH_SEPARATOR;
		GFileManager->MakeDirectory( *OutputDir );

		const FString& MemoryChartFilename = OutputDir + CreateFileNameForChart( TEXT( "Mem" ), TEXT( ".html" ), bOutputToGlobalLog );

		FString MemoryChart;
		// See whether file already exists and load it into string if it does.
		if( appLoadFileToString( MemoryChart, *MemoryChartFilename ) )
		{
			// Split string where we want to insert current row.
			const FString HeaderSeparator = TEXT("<UE3></UE3>");
			FString MemoryChartBeforeCurrentRow, MemoryChartAfterCurrentRow;
			MemoryChart.Split( *HeaderSeparator, &MemoryChartBeforeCurrentRow, &MemoryChartAfterCurrentRow );

			// Assemble Memory chart by inserting current row at the top.
			MemoryChart = MemoryChartPreamble + MemoryChartRows + MemoryChartAfterCurrentRow;
		}
		// Assemble from scratch.
		else
		{
			MemoryChart = MemoryChartPreamble + MemoryChartRows + MemoryChartPostamble;
		}

		// Save the resulting file back to disk.
		appSaveStringToFile( MemoryChart, *MemoryChartFilename );
	}
	else
	{
		debugf(TEXT("Missing Memory chart template files."));
	}
}

/**
* Dumps the Memory chart information to the log.
*/
void UEngine::DumpMemoryChartToStatsLog( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames )
{
	// Map name we're dumping.
	const FString MapName = GWorld ? GWorld->GetMapName() : TEXT("None");

	// Create folder for Memory chart data.
	const FString OutputDir = appGameDir() + TEXT("Stats") + PATH_SEPARATOR;
	GFileManager->MakeDirectory( *OutputDir );
	// Create archive for log data.
	FArchive* OutputFile = GFileManager->CreateFileWriter( *(OutputDir + CreateFileNameForChart( TEXT( "Mem" ), TEXT( ".log" ), FALSE ) ), FILEWRITE_Append );

	if( OutputFile )
	{
		OutputFile->Logf(TEXT("Dumping Memory chart at %s using build %i built from changelist %i   %i"), *appSystemTimeString(), GEngineVersion, GetChangeListNumberForPerfTesting(), GMemoryChart.Num() );

		OutputFile->Logf( TEXT( "MemoryUsed: ,PhysicalMemUsed, VirtualMemUsed, AudioMemUsed, TextureMemUsed, NovodexMemUsed, VertexLightingMemUsed, StaticMeshVertexMemUsed, StaticMeshIndexMemUsed, SkeletalMeshVertexMemUsed, SkeletalMeshIndexMemUsed, VertexShaderMemUsed, PixelShaderMemUsed, PhysicalTotal, GPUTotal, GPUMemUsed, NumAllocations, AllocationOverhead, AllignmentWaste" ) ); 

		// Iterate over all data
		for( INT MemoryIndex=0; MemoryIndex < GMemoryChart.Num(); ++MemoryIndex )
		{
			const FMemoryChartEntry& MemEntry = GMemoryChart(MemoryIndex);

			// Log bucket index, time and frame Percentage.
			OutputFile->Logf( *MemEntry.ToString() );
		}

		OutputFile->Logf( LINE_TERMINATOR LINE_TERMINATOR LINE_TERMINATOR );

		// Flush, close and delete.
		delete OutputFile;
	}
}


/**
* Dumps the Memory chart information to the special stats log file.
*/
void UEngine::DumpMemoryChartToLog( FLOAT TotalTime, FLOAT DeltaTime, INT NumFrames )
{
	// don't dump out memory data to log during normal running or ever!
}

FString FMemoryChartEntry::ToString() const
{
	return FString::Printf( TEXT( "MemoryUsed: ,%5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f, %5.0f" ) 
		,PhysicalTotal 
		,PhysicalMemUsed 
		,VirtualMemUsed

		,GPUTotal
		,GPUMemUsed 

		,AudioMemUsed 
		,TextureMemUsed
		,NovodexMemUsed 
		,VertexLightingMemUsed

		,StaticMeshVertexMemUsed 
		,StaticMeshIndexMemUsed
		,SkeletalMeshVertexMemUsed 
		,SkeletalMeshIndexMemUsed

		,VertexShaderMemUsed 
		,PixelShaderMemUsed

		,NumAllocations
		,AllocationOverhead 
		,AllignmentWaste
		);
}

FString FMemoryChartEntry::ToHTMLString() const
{
	FString Retval;

	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), PhysicalTotal );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), PhysicalMemUsed );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), VirtualMemUsed );

	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), GPUTotal );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), GPUMemUsed );

	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), AudioMemUsed );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), TextureMemUsed );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), NovodexMemUsed );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), VertexLightingMemUsed );

	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), StaticMeshVertexMemUsed );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), StaticMeshIndexMemUsed );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), SkeletalMeshVertexMemUsed );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), SkeletalMeshIndexMemUsed );

	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), VertexShaderMemUsed );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), PixelShaderMemUsed );


	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), NumAllocations );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), AllocationOverhead );
	Retval += FString::Printf( TEXT( "<TD><DIV CLASS=\"value\">%5.0f</DIV></TD> \r\n" ), AllignmentWaste );


	return Retval;
}




