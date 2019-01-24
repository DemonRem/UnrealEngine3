/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.IO;

namespace UnrealBuildTool
{
    class UE3BuildConfiguration
    {
        /** Whether to compile with GCM HUD support on PS3. */
        public static PS3GCMType PS3GCMType = PS3GCMType.Release;

        /** True if managed code (and all related functionality) will be enabled */
        public static bool bAllowManagedCode = Utils.GetEnvironmentVariable("ue3.bAllowManagedCode", true);

		/** Whether to allow Steam support if requested */
		public static bool bAllowSteamworks = Utils.GetEnvironmentVariable( "ue3.bAllowSteamworks", true );

		/** Whether to force Steam support */
		public static bool bForceSteamworks = Utils.GetEnvironmentVariable( "ue3.bForceSteamworks", false );

		/** Whether to allow GameSpy support if requested */
		public static bool bAllowGameSpy = Utils.GetEnvironmentVariable( "ue3.bAllowGameSpy", true );

		/** Whether to force GameSpy support */
		public static bool bForceGameSpy = Utils.GetEnvironmentVariable( "ue3.bForceGameSpy", false );

		/** Whether to allow Live support if requested */
		public static bool bAllowLive = Utils.GetEnvironmentVariable( "ue3.bAllowLive", true );

		/** Whether to allow GameCenter support if requested */
		public static bool bAllowGameCenter = Utils.GetEnvironmentVariable("ue3.bAllowGameCenter", true);

		/** Whether to force GameCenter support */
		public static bool bForceGameCenter = Utils.GetEnvironmentVariable("ue3.bForceGameCenter", false);

		/** Whether to force Live support */
		public static bool bForceLive = Utils.GetEnvironmentVariable( "ue3.bForceLive", false );

        /** Whether to include FaceFX support */
        public static bool bCompileFaceFX = Utils.GetEnvironmentVariable("ue3.bCompileFaceFX", true);

        /** Whether we should compile FaceFX studio or not. Done in UBT as we need to disregard project. */
        public static bool bCompileFaceFXStudio = Utils.GetEnvironmentVariable("ue3.bCompileFaceFXStudio", true);

		/** Whether we should compile the perforce API or not. Done in UBT as we need to disregard project. */
		public static bool bCompilePerforce = Utils.GetEnvironmentVariable("ue3.bCompilePerforce", true);

        /** Whether to build a stripped down version of the game specifically for dedicated server. */
        public static bool bBuildDedicatedServer = false;

        /** Whether to compile the editor or not. Only Windows will use this, other platforms force this to false */
        public static bool bBuildEditor = true;

		/** Whether we should compile SpeedTree or not. Done in UBT as we need to conditionally link libraries. */
		public static bool bCompileSpeedTree = Utils.GetEnvironmentVariable( "ue3.bCompileSpeedTree", true );

		/** Whether to include Autodesk FBX importer support in editor builds. */
		public static bool bCompileFBX = Utils.GetEnvironmentVariable("ue3.bCompileFBX", true);

        /** Whether to include WxWidgets support */
        public static bool bCompileWxWidgets = Utils.GetEnvironmentVariable("ue3.bCompileWxWidgets", true);

        /** Whether to include Bink support */
        public static bool bCompileBink = Utils.GetEnvironmentVariable("ue3.bCompileBink", true);

		/** Whether to include Scaleform GFx support */
		public static bool bCompileScaleform = Utils.GetEnvironmentVariable("ue3.bCompileScaleform", true);
        /** Forces GFX to link with release libs for faster perf */
        public static bool bForceScaleformRelease = Utils.GetEnvironmentVariable("ue3.bForceScaleformRelease", true);

		/** Whether to compile lean and mean version of UE3. */
        public static bool bCompileLeanAndMeanUE3 = false;

        /** Whether to trim rarely used code like FaceFX studio, TTP integration, etc. */
        public static bool bTrimRarelyUsedCode = Utils.GetEnvironmentVariable("ue3.bTrimRarelyUsedCode", false);

        /** Whether to compile in PS3 Move functionality */
        public static bool bCompilePS3Move = Utils.GetEnvironmentVariable("ue3.bCompilePS3Move", true);

        /** Whether to allow the APEX build settings to be overridden */
        public static bool bOverrideAPEXBuild = Utils.GetEnvironmentVariable("ue3.bOverrideAPEXBuild", false);

        /** Whether to enable APEX, bOverrideAPEXBuild must be set for this to take affect */
        public static bool bUseAPEX = Utils.GetEnvironmentVariable("ue3.bUseAPEX", false);

		/** Whether to compile with PhysX support on mobile platforms */
		public static bool bCompilePhysXWithMobile = Utils.GetEnvironmentVariable("ue3.bCompilePhysXWithMobile", true);

        /**
         * Validates the configuration.
         * 
         * @warning: the order of validation is important
         */
        public static void ValidateConfiguration()
        {
            // Trim rarely used code.
            if (bTrimRarelyUsedCode)
            {
                bCompileFaceFXStudio = false;
                bCompileFBX = false;
            }

            // Lean and mean means no Editor and other frills.
            if (bCompileLeanAndMeanUE3)
            {
                bBuildEditor = false;
				bAllowSteamworks = false;
				bAllowGameSpy = false;
				bAllowLive = false;
				bCompileFBX = false;
                bCompileSpeedTree = false;
                bCompileFaceFXStudio = false;
                bCompilePerforce = false;
                bAllowManagedCode = false;
                bCompileWxWidgets = false;
            }

            // Dedicated server strips out a bunch of stuff
            if (bBuildDedicatedServer)
            {
                bBuildEditor = false;
                bCompileFBX = false;
                bCompileBink = false;
                bCompileSpeedTree = false;
                bCompilePerforce = false;
                bCompileFaceFX = false;
                bCompileFaceFXStudio = false;
                bAllowManagedCode = false;
                bCompileWxWidgets = false;
				bCompileScaleform = false;
            }

            // No FaceFX -> No FaceFXStudio
            if (!bCompileFaceFX)
            {
                bCompileFaceFXStudio = false;
            }
        }
    }
}