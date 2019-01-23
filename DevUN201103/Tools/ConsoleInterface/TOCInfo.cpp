/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
#include "Stdafx.h"
#include "TOCInfo.h"

namespace ConsoleInterface
{
	TOCInfo::TOCInfo( String ^InFileName, 
					  String ^Hash, 
					  DateTime LastWrite,
					  int SizeInBytes, 
					  int CompressedSizeInBytes, 
					  int StartingSector, 
					  bool ForSync,
					  bool ForTOC)
			: FileName(InFileName), 
			  Size(SizeInBytes), 
			  CompressedSize(CompressedSizeInBytes), 
			  StartSector(StartingSector), 
			  CRC(Hash),
			  LastWriteTime(LastWrite), 
			  bIsForSync(ForSync),
			  bIsForTOC(ForTOC)
	{
	}
}
