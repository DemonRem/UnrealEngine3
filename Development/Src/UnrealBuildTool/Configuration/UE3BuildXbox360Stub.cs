/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.IO;

namespace UnrealBuildTool
{
	partial class UE3BuildTarget
	{
		bool Xbox360SupportEnabled()
		{
			return false;
		}

		void SetUpXbox360Environment()
		{
		}

		List<FileItem> GetXbox360OutputItems()
		{
			return new List<FileItem>();
		}
	}
}