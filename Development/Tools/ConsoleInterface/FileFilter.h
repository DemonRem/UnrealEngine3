/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
#pragma once

using namespace System;
using namespace System::Xml::Serialization;

namespace ConsoleInterface
{
	[Serializable]
	public ref class FileFilter
	{
	public:
		[XmlAttribute]
		String ^Name;

		// If this is TRUE (default), the filter matches filenames
		[XmlAttribute]
		bool bIsForFilename;

		// If this is TRUE (not default), the filter matches any single directory name in the path to the file
		[XmlAttribute]
		bool bIsForDirectory;

	public:
		FileFilter();
	};
}
