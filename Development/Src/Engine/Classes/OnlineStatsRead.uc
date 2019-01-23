/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Stats class that holds the read request definitions and the resulting data
 */
class OnlineStatsRead extends OnlineStats
	native
	abstract;

/** A single instance of a stat in a row */
struct native OnlineStatsColumn
{
	/** The ordinal value of the column */
	var const int ColumnNo;
	/** The value of the stat for this column */
	var const SettingsData StatValue;
};

/** Holds a single player's set of data for this stats view */
struct native OnlineStatsRow
{
	/** The unique player id of the player these stats are for */
	var const UniqueNetId PlayerId;
	/** The rank of the player in this stats view */
	var const SettingsData Rank;
	/** Player's online nickname */
	var const string NickName;
	/** The set of columns (stat instances) for this row */
	var const array<OnlineStatsColumn> Columns;
};

/** The unique id of the view that these stats are from */
var const int ViewId;

/** The column id to use for sorting rank */
var const int SortColumnId;

/** The columns to read in the view we are interested in */
var const array<int> ColumnIds;

/** The total number of rows in the view */
var const int TotalRowsInView;

/** The rows of data returned by the online service */
var const array<OnlineStatsRow> Rows;

/** Provides human readable values for column ids */
struct native ColumnMetaData
{
	/** Id for the given string */
	var const int Id;
	/** Human readable form of the Id */
	var const name Name;
};

/** Provides metadata for column ids so that we can present their human readable form */
var const array<ColumnMetaData> ColumnMappings;

/** The name of the view in human readable terms */
var const string ViewName;

/**
 * Delegate used to notify the caller when stats read has completed
 */
delegate OnStatsReadComplete();
