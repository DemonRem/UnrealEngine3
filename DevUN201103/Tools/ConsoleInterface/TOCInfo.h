/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
#pragma once

using namespace System;

namespace ConsoleInterface
{
	public ref class TOCInfo
	{
	public:
		String ^FileName;
		String ^CRC;
		DateTime LastWriteTime;
		int Size;
		int CompressedSize;
		int StartSector;
		bool bIsForSync;
		bool bIsForTOC;

	public:
		TOCInfo( String ^InFileName, 
				 String ^Hash, 
				 DateTime LastWrite, 
				 int SizeInBytes, 
				 int CompressedSizeInBytes, 
				 int StartingSector, 
				 bool bIsForSync,
				 bool ForTOC );
	};
}
