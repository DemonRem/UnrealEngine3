/**
 *
 * Copyright 1998-2010 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.IO;

namespace UnrealBuildTool
{
	partial class UE3BuildTarget
	{
		bool NGPSupportEnabled()
		{
			return false;
		}

		void SetUpNGPEnvironment()
		{            
		}

        void SetUpNGPPhysXEnvironment()
        {
        }

		List<FileItem> GetNGPOutputItems()
		{
			return new List<FileItem>();
		}

		public static string GetSCENGPRoot()
		{
			return "";
		}
	}
}
