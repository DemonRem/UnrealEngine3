/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UOnlineStats);
IMPLEMENT_CLASS(UOnlineStatsRead);
IMPLEMENT_CLASS(UOnlineStatsWrite);

/**
 * Searches the view id mappings to find the view id that matches the name
 *
 * @param ViewName the name of the view being searched for
 * @param ViewId the id of the view that matches the name
 *
 * @return true if it was found, false otherwise
 */
UBOOL UOnlineStats::GetViewId(FName ViewName,INT& ViewId)
{
	// Search through the array for the name and set the id that matches
	for (INT Index = 0; Index < ViewIdMappings.Num(); Index++)
	{
		const FStringIdToStringMapping& Mapping = ViewIdMappings(Index);
		if (Mapping.Name == ViewName)
		{
			ViewId = Mapping.Id;
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Finds the human readable name for the view
 *
 * @param ViewId the id to look up in the mappings table
 *
 * @return the name of the view that matches the id or NAME_None if not found
 */
FName UOnlineStats::GetViewName(INT ViewId)
{
	// Search through the array for an id that matches and return the name
	for (INT Index = 0; Index < ViewIdMappings.Num(); Index++)
	{
		const FStringIdToStringMapping& Mapping = ViewIdMappings(Index);
		if (Mapping.Id == ViewId)
		{
			return Mapping.Name;
		}
	}
	return NAME_None;
}

/**
 * Searches the stat mappings to find the stat id that matches the name
 *
 * @param StatName the name of the stat being searched for
 * @param StatId the out value that gets the id
 *
 * @return true if it was found, false otherwise
 */
UBOOL UOnlineStatsWrite::GetStatId(FName StatName,INT& StatId)
{
	// Now search through the potential stats for a matching name
	for (INT Index = 0; Index < StatMappings.Num(); Index++)
	{
		const FStringIdToStringMapping& Mapping = StatMappings(Index);
		if (Mapping.Name == StatName)
		{
			StatId = Mapping.Id;
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Searches the stat mappings to find human readable name for the stat id
 *
 * @param StatId the id of the stats to find the name for
 *
 * @return true if it was found, false otherwise
 */
FName UOnlineStatsWrite::GetStatName(INT StatId)
{
	// Now search through the potential stats for a matching id
	for (INT Index = 0; Index < StatMappings.Num(); Index++)
	{
		const FStringIdToStringMapping& Mapping = StatMappings(Index);
		if (Mapping.Id == StatId)
		{
			return Mapping.Name;
		}
	}
	return NAME_None;
}

/**
 * Sets a stat of type SDT_Float to the value specified. Does nothing
 * if the stat is not of the right type.
 *
 * @param StatId the stat to change the value of
 * @param Value the new value to assign to the stat
 */
void UOnlineStatsWrite::SetFloatStat(INT StatId,FLOAT Value)
{
	FSettingsData* Stat = FindStat(StatId);
	if (Stat != NULL)
	{
		// Set the value
		Stat->SetData(Value);
	}
}

/**
 * Sets a stat of type SDT_Int to the value specified. Does nothing
 * if the stat is not of the right type.
 *
 * @param StatId the stat to change the value of
 * @param Value the new value to assign to the stat
 */
void UOnlineStatsWrite::SetIntStat(INT StatId,INT Value)
{
	FSettingsData* Stat = FindStat(StatId);
	if (Stat != NULL)
	{
		// Set the value
		Stat->SetData(Value);
	}
}

/**
 * Increments a stat of type SDT_Float by the value specified. Does nothing
 * if the stat is not of the right type.
 *
 * @param StatId the stat to increment
 * @param IncBy the value to increment by
 */
void UOnlineStatsWrite::IncrementFloatStat(INT StatId,FLOAT IncBy)
{
	FSettingsData* Stat = FindStat(StatId);
	if (Stat != NULL)
	{
		// Increment the value
		Stat->Increment<DOUBLE,SDT_Double>((DOUBLE)IncBy);
	}
}

/**
 * Increments a stat of type SDT_Int by the value specified. Does nothing
 * if the stat is not of the right type.
 *
 * @param StatId the stat to increment
 * @param IncBy the value to increment by
 */
void UOnlineStatsWrite::IncrementIntStat(INT StatId,INT IncBy)
{
	FSettingsData* Stat = FindStat(StatId);
	if (Stat != NULL)
	{
		// Increment the value
		Stat->Increment<INT,SDT_Int32>(IncBy);
	}
}

/**
 * Decrements a stat of type SDT_Float by the value specified. Does nothing
 * if the stat is not of the right type.
 *
 * @param StatId the stat to decrement
 * @param DecBy the value to decrement by
 */
void UOnlineStatsWrite::DecrementFloatStat(INT StatId,FLOAT DecBy)
{
	FSettingsData* Stat = FindStat(StatId);
	if (Stat != NULL)
	{
		// Decrement the value
		Stat->Decrement<DOUBLE,SDT_Double>((DOUBLE)DecBy);
	}
}

/**
 * Decrements a stat of type SDT_Int by the value specified. Does nothing
 * if the stat is not of the right type.
 *
 * @param StatId the stat to decrement
 * @param DecBy the value to decrement by
 */
void UOnlineStatsWrite::DecrementIntStat(INT StatId,INT DecBy)
{
	FSettingsData* Stat = FindStat(StatId);
	if (Stat != NULL)
	{
		// Decrement the value
		Stat->Decrement<INT,SDT_Int32>(DecBy);
	}
}
