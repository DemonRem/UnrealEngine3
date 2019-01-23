/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.IO;

namespace UnrealBuildTool
{
	enum PS3GCMType
	{
		Release,
		Debug,
		HUD
	}

	partial class UE3BuildTarget
	{
		bool PS3SupportEnabled()
		{
			return false;
		}

		void SetUpPS3Environment()
		{            
		}

        void SetUpPS3PhysXEnvironment()
        {
        }

		List<FileItem> GetPS3OutputItems()
		{
			return new List<FileItem>();
		}

		public static string GetSCEPS3Root()
		{
			return "";
		}
	}
}
