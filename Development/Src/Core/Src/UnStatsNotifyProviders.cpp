/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This file contains the implementations of the various remote stats facilities.
 */

#include "CorePrivate.h"

#include "UnStatsNotifyProviders.h"

#include <stdio.h>

#if STATS

/**
 * Tells the parent class to open the file and then writes the opening
 * XML data to it
 */
UBOOL FStatNotifyProvider_File::Init(void)
{
	if( GIsSilent == TRUE )
	{
		return FALSE;
	}

	// Builds the file name based upon the log directory and game name
	// and the string the derived class asked to have appended
	FString FileName(appBaseDir());
	FileName += appGameLogDir();
	FileName += appGetGameName();
#if !CONSOLE
	FString PassedInName;
	// Check for the name passed in on the command line
	if (Parse(appCmdLine(),TEXT("STATFILE="),PassedInName))
	{
		FileName = appBaseDir();
		FileName += appGameLogDir();
		FileName += PassedInName;
	}
	// Use different log files for editor versus in game
	if (GIsEditor == TRUE)
	{
		FileName += TEXT("_ed");
	}
#elif XBOX
	FileName += TEXT("_Xe");
#elif PS3
	FileName += TEXT("_PS3");
#endif
	FileName += AppendedName;
	// Ask for a file writer
	//@todo -- joeg Create an optimized async file writer ala UC2
	File = GFileManager->CreateFileWriter(*FileName,
		FILEWRITE_AllowRead | FILEWRITE_Unbuffered);
	return File != NULL;
}

/**
 * Closes the file
 */
void FStatNotifyProvider_File::Destroy(void)
{
	delete File;
	File = NULL;
}

/**
 * Builds the output string from varargs and writes it out
 *
 * @param Format the format string to use with the varargs
 */
void FStatNotifyProvider_File::WriteString(const ANSICHAR* Format,...)
{
	check(File);
	ANSICHAR Array[1024];
	va_list ArgPtr;
	va_start(ArgPtr,Format);
	// Build the string
	INT Result = appGetVarArgsAnsi(Array,ARRAY_COUNT(Array),ARRAY_COUNT(Array)-1,Format,ArgPtr);
	// Now write that to the file
	File->Serialize((void*)Array,Result);
}

/**
 * Tells the parent class to open the file and then writes the opening
 * XML data to it
 */
UBOOL FStatNotifyProvider_XML::Init(void)
{
	UBOOL bOk = FStatNotifyProvider_File::Init();
	if (bOk)
	{
		// Create the opening element
		WriteString("<StatFile SecondsPerCycle=\"%e\">\r\n",GSecondsPerCycle);
	}
	return bOk;
}

/**
 * Makes the XML data well formed and closes the file
 */
void FStatNotifyProvider_XML::Destroy(void)
{
	// Close the previous frame element if needed
	if (CurrentFrame != (DWORD)-1)
	{
		WriteString("\t\t</Stats>\r\n");
		WriteString("\t</Frame>\r\n");
	}
	WriteString("\t</Frames>\r\n");
	// Close the opening element
	WriteString("</StatFile>\r\n");
	// Now close the file
	FStatNotifyProvider_File::Destroy();
}

/**
 * Writes the opening XML tag out
 */
void FStatNotifyProvider_XML::StartDescriptions(void)
{
	WriteString("\t<Descriptions>\r\n");
}

/**
 * Writes the closing XML tag out
 */
void FStatNotifyProvider_XML::EndDescriptions(void)
{
	WriteString("\t</Descriptions>\r\n");
	WriteString("\t<Frames>\r\n");
}

/**
 * Writes the opening XML tag out
 */
void FStatNotifyProvider_XML::StartGroupDescriptions(void)
{
	WriteString("\t\t<Groups>\r\n");
}

/**
 * Writes the closing XML tag out
 */
void FStatNotifyProvider_XML::EndGroupDescriptions(void)
{
	WriteString("\t\t</Groups>\r\n");
}

/**
 * Writes the opening XML tag out
 */
void FStatNotifyProvider_XML::StartStatDescriptions(void)
{
	WriteString("\t\t<Stats>\r\n");
}

/**
 * Writes the closing XML tag out
 */
void FStatNotifyProvider_XML::EndStatDescriptions(void)
{
	WriteString("\t\t</Stats>\r\n");
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
void FStatNotifyProvider_XML::AddStatDescription(DWORD StatId,const TCHAR* StatName,
	DWORD StatType,DWORD GroupId)
{
	// Write out the stat description element
	WriteString("\t\t\t<Stat ID=\"%d\" N=\"%S\" ST=\"%d\" GID=\"%d\"/>\r\n",
		StatId,StatName,StatType,GroupId);
}

/**
 * Adds a group to the list of descriptions
 *
 * @param GroupId the id of the group being added
 * @param GroupName the name of the group
 */
void FStatNotifyProvider_XML::AddGroupDescription(DWORD GroupId,const TCHAR* GroupName)
{
	// Write out the group description element
	WriteString("\t\t\t<Group ID=\"%d\" N=\"%S\"/>\r\n",GroupId,GroupName);
}

/**
 * Function to write the stat out to the provider's data store
 *
 * @param StatId the id of the stat that is being written out
 * @param ParentId the id of the parent stat
 * @param InstanceId the instance id of the stat being written
 * @param ParentInstanceId the instance id of parent stat
 * @param ThreadId the thread this stat is for
 * @param Value the value of the stat to write out
 * @param CallsPerFrame the number of calls for this frame
 */
void FStatNotifyProvider_XML::WriteStat(DWORD StatId,DWORD ParentId,
	DWORD InstanceId,DWORD ParentInstanceId,DWORD ThreadId,DWORD Value,
	DWORD CallsPerFrame)
{
	// Write out the stat element
	WriteString("\t\t\t\t<Stat ID=\"%d\" IID=\"%d\" PID=\"%d\" TID=\"%d\" V=\"%d\" PF=\"%d\"/>\r\n",
		StatId,InstanceId,ParentInstanceId,ThreadId,Value,CallsPerFrame);
}

/**
 * Function to write the stat out to the provider's data store
 *
 * @param StatId the id of the stat that is being written out
 * @param Value the value of the stat to write out
 */
void FStatNotifyProvider_XML::WriteStat(DWORD StatId,FLOAT Value)
{
	// Write out the stat element
	WriteString("\t\t\t\t<Stat ID=\"%d\" V=\"%f\"/>\r\n",
		StatId,Value);
}

/**
 * Function to write the stat out to the provider's data store
 *
 * @param StatId the id of the stat that is being written out
 * @param Value the value of the stat to write out
 */
void FStatNotifyProvider_XML::WriteStat(DWORD StatId,DWORD Value)
{
	// Write out the stat element
	WriteString("\t\t\t\t<Stat ID=\"%d\" V=\"%d\"/>\r\n",
		StatId,Value);
}

/**
 * Creates a new frame element
 *
 * @param FrameNumber the new frame number being processed
 */
void FStatNotifyProvider_XML::SetFrameNumber(DWORD FrameNumber)
{
	// Close the previous frame element if needed
	if (FrameNumber != CurrentFrame && CurrentFrame != (DWORD)-1)
	{
		WriteString("\t\t\t</Stats>\r\n");
		WriteString("\t\t</Frame>\r\n");
	}
	// Call base class version
	FStatNotifyProvider::SetFrameNumber(FrameNumber);
	// And create the new frame element
	WriteString("\t\t<Frame N=\"%d\">\r\n",FrameNumber);
	WriteString("\t\t\t<Stats>\r\n");
}

/**
 * Writes out the SecondsPerCycle info and closes the file
 */
void FStatNotifyProvider_CSV::Destroy(void)
{
	WriteString("\r\nSecondsPerCycle,%e\r\n",GSecondsPerCycle);
	// Now close the file
	FStatNotifyProvider_File::Destroy();
}

/**
 * Writes the CSV headers for the column names
 */
void FStatNotifyProvider_CSV::StartDescriptions(void)
{
	// Write out the headers
	WriteString("Frame,InstanceID,ParentInstanceID,StatID,ThreadID,Name,Value,CallsPerFrame\r\n");
}

/**
 * Function to write the stat out to the provider's data store
 *
 * @param StatId the id of the stat that is being written out
 * @param ParentId the parent id of the stat
 * @param InstanceId the instance id of the stat being written
 * @param ParentInstanceId the instance id of parent stat
 * @param ThreadId the thread this stat is for
 * @param Value the value of the stat to write out
 * @param CallsPerFrame the number of calls for this frame
 */
void FStatNotifyProvider_CSV::WriteStat(DWORD StatId,DWORD ParentId,
	DWORD InstanceId,DWORD ParentInstanceId,DWORD ThreadId,DWORD Value,
	DWORD CallsPerFrame)
{
	WriteString(WRITE_STAT_1,CurrentFrame,InstanceId,ParentInstanceId,StatId,
		ThreadId,GStatManager.GetStatName(StatId),Value,CallsPerFrame);
}

/**
 * Function to write the stat out to the provider's data store
 *
 * @param StatId the id of the stat that is being written out
 * @param Value the value of the stat to write out
 */
void FStatNotifyProvider_CSV::WriteStat(DWORD StatId,FLOAT Value)
{
	WriteString(WRITE_STAT_2,CurrentFrame,StatId,
		GStatManager.GetStatName(StatId),Value);
}

/**
 * Function to write the stat out to the provider's data store
 *
 * @param StatId the id of the stat that is being written out
 * @param Value the value of the stat to write out
 */
void FStatNotifyProvider_CSV::WriteStat(DWORD StatId,DWORD Value)
{
	WriteString(WRITE_STAT_3,CurrentFrame,StatId,
		GStatManager.GetStatName(StatId),Value);
}

#endif
