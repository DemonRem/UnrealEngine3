/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This file contains the definitions of the various remote stats facilities.
 */


#ifndef _INC_STATSNOTIFYPROVIDER
#define _INC_STATSNOTIFYPROVIDER

#if STATS

/**
 * This provider adds common members/functions for file based stats writing
 */
class FStatNotifyProvider_File : public FStatNotifyProvider
{
	/**
	 * The file we are writing to
	 */
	FArchive* File;
	/**
	 * The string to append to the game name when creating a stats file name
	 */
	const TCHAR* AppendedName;

protected:
	/**
	 * Writes out the string specified
	 *
	 * @param Format the format string to use with the varargs
	 */
	void CDECL WriteString(const ANSICHAR* Format,...);

public:
	/**
	 * Default constructor. Zeros pointer and passes the provider name
	 */
	FStatNotifyProvider_File(const TCHAR* ProviderName,const TCHAR* InAppendedName) :
		FStatNotifyProvider(ProviderName),
		File(NULL),
		AppendedName(InAppendedName)
	{
	}

	/**
	 * Creates the file that is going to be written to
	 */
	virtual UBOOL Init(void);
	/**
	 * Closes the file
	 */
	virtual void Destroy(void);
};

/**
 * This provider writes the stats data to an XML file
 */
class FStatNotifyProvider_XML : public FStatNotifyProvider_File
{
public:
	/**
	 * Default constructor. Just passes info up the chain
	 */
	FStatNotifyProvider_XML(void) :
		FStatNotifyProvider_File(TEXT("XmlStatNotifyProvider"),TEXT("_Stats.xml"))
	{
	}

	/**
	 * Creates the XML file that is going to be written to
	 */
	virtual UBOOL Init(void);

	/**
	 * Makes the XML data well formed and closes the file
	 */
	virtual void Destroy(void);

	/**
	 * Tells the provider that we are starting to supply it with descriptions
	 * for all of the stats/groups.
	 */
	virtual void StartDescriptions(void);

	/**
	 * Tells the provider that we are finished sending descriptions for all of
	 * the stats/groups.
	 */
	virtual void EndDescriptions(void);

	/**
	 * Tells the provider that we are starting to supply it with group descriptions
	 */
	virtual void StartGroupDescriptions(void);

	/**
	 * Tells the provider that we are finished sending stat descriptions
	 */
	virtual void EndGroupDescriptions(void);

	/**
	 * Tells the provider that we are starting to supply it with stat descriptions
	 */
	virtual void StartStatDescriptions(void);

	/**
	 * Tells the provider that we are finished sending group descriptions
	 */
	virtual void EndStatDescriptions(void);

	/**
	 * Adds a stat to the list of descriptions. Used to allow custom stats to
	 * report who they are, parentage, etc. Prevents applications that consume
	 * the stats data from having to change when stats information changes
	 *
	 * @param StatId the id of the stat
	 * @param StatName the name of the stat
	 * @param StatType the type of stat this is
	 * @param GroupId the id of the group this stat belongs to
	 */
	virtual void AddStatDescription(DWORD StatId,const TCHAR* StatName,DWORD StatType,DWORD GroupId);

	/**
	 * Adds a group to the list of descriptions
	 *
	 * @param GroupId the id of the group being added
	 * @param GroupName the name of the group
	 */
	virtual void AddGroupDescription(DWORD GroupId,const TCHAR* GroupName);

	/**
	 * Function to write the stat out to the provider's data store
	 *
	 * @param StatId the id of the stat that is being written out
	 * @param ParentId the id of parent stat
	 * @param InstanceId the instance id of the stat being written
	 * @param ParentInstanceId the instance id of parent stat
	 * @param ThreadId the thread this stat is for
	 * @param Value the value of the stat to write out
	 * @param CallsPerFrame the number of calls for this frame
	 */
	virtual void WriteStat(DWORD StatId,DWORD ParentId,DWORD InstanceId,
		DWORD ParentInstanceId,DWORD ThreadId,DWORD Value,
		DWORD CallsPerFrame);

	/**
	 * Function to write the stat out to the provider's data store
	 *
	 * @param StatId the id of the stat that is being written out
	 * @param Value the value of the stat to write out
	 */
	virtual void WriteStat(DWORD StatId,FLOAT Value);

	/**
	 * Function to write the stat out to the provider's data store
	 *
	 * @param StatId the id of the stat that is being written out
	 * @param Value the value of the stat to write out
	 */
	virtual void WriteStat(DWORD StatId,DWORD Value);

	/**
	 * Creates a new frame element
	 *
	 * @param FrameNumber the new frame number being processed
	 */
	virtual void SetFrameNumber(DWORD FrameNumber);
};

#if !PS3 || !UNICODE
// Defines for cross platform settings
#define WRITE_STAT_1 "%d,%d,%d,%d,%d,%S,%d,%d\r\n"
#define WRITE_STAT_2 "%d, ,%d, ,%S,%f, \r\n"
#define WRITE_STAT_3 "%d, ,%d, ,%S,%d, \r\n"
#else
#define WRITE_STAT_1 "%d,%d,%d,%d,%s,%d,%d\r\n"
#define WRITE_STAT_2 "%d, ,%d, ,%s,%f, \r\n"
#define WRITE_STAT_3 "%d, ,%d, ,%s,%d, \r\n"
#endif

/**
 * This provider writes the stats data to an XML file
 */
class FStatNotifyProvider_CSV : public FStatNotifyProvider_File
{
public:
	/**
	 * Default constructor. Just passes info up the chain
	 */
	FStatNotifyProvider_CSV(void) :
		FStatNotifyProvider_File(TEXT("CsvStatNotifyProvider"),TEXT("_Stats.csv"))
	{
	}

	/**
	 * Writes out the SecondsPerCycle info and closes the file
	 */
	virtual void Destroy(void);

	/**
	 * Tells the provider that we are starting to supply it with descriptions
	 * for all of the stats/groups.
	 */
	virtual void StartDescriptions(void);

	/**
	 * Tells the provider that we are finished sending descriptions for all of
	 * the stats/groups.
	 */
	virtual void EndDescriptions(void)
	{
		// Ignored
	}

	/**
	 * Tells the provider that we are starting to supply it with group descriptions
	 */
	virtual void StartGroupDescriptions(void)
	{
		// Ignored
	}

	/**
	 * Tells the provider that we are finished sending stat descriptions
	 */
	virtual void EndGroupDescriptions(void)
	{
		// Ignored
	}

	/**
	 * Tells the provider that we are starting to supply it with stat descriptions
	 */
	virtual void StartStatDescriptions(void)
	{
		// Ignored
	}

	/**
	 * Tells the provider that we are finished sending group descriptions
	 */
	virtual void EndStatDescriptions(void)
	{
		// Ignored
	}

	/**
	 * Adds a stat to the list of descriptions. Used to allow custom stats to
	 * report who they are, parentage, etc. Prevents applications that consume
	 * the stats data from having to change when stats information changes
	 *
	 * @param StatId the id of the stat
	 * @param StatName the name of the stat
	 * @param StatType the type of stat this is
	 * @param GroupId the id of the group this stat belongs to
	 */
	virtual void AddStatDescription(DWORD StatId,const TCHAR* StatName,DWORD StatType,DWORD GroupId)
	{
		// Ignored
	}

	/**
	 * Adds a group to the list of descriptions
	 *
	 * @param GroupId the id of the group being added
	 * @param GroupName the name of the group
	 */
	virtual void AddGroupDescription(DWORD GroupId,const TCHAR* GroupName)
	{
		// Ignored
	}

	/**
	 * Function to write the stat out to the provider's data store
	 *
	 * @param StatId the id of the stat that is being written out
	 * @param ParentId the id of parent stat
	 * @param InstanceId the instance id of the stat being written
	 * @param ParentInstanceId the instance id of parent stat
	 * @param ThreadId the thread this stat is for
	 * @param Value the value of the stat to write out
	 * @param CallsPerFrame the number of calls for this frame
	 */
	virtual void WriteStat(DWORD StatId,DWORD ParentId,DWORD InstanceId,
		DWORD ParentInstanceId,DWORD ThreadId,DWORD Value,
		DWORD CallsPerFrame);

	/**
	 * Function to write the stat out to the provider's data store
	 *
	 * @param StatId the id of the stat that is being written out
	 * @param Value the value of the stat to write out
	 */
	virtual void WriteStat(DWORD StatId,FLOAT Value);

	/**
	 * Function to write the stat out to the provider's data store
	 *
	 * @param StatId the id of the stat that is being written out
	 * @param Value the value of the stat to write out
	 */
	virtual void WriteStat(DWORD StatId,DWORD Value);
};

#endif

#endif
