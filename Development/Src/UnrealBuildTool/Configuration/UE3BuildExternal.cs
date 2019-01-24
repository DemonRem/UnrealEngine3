/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	partial class UE3BuildTarget
	{
		public static string ForceOnlineSubsystem( UnrealTargetPlatform Platform )
		{
			switch( Platform )
			{
			case UnrealTargetPlatform.Win32:
				if( UE3BuildConfiguration.bForceSteamworks )
				{
					return ( "Steamworks" );
				}
				else if( UE3BuildConfiguration.bForceGameSpy )
				{
					return ( "GameSpy" );
				}
				else if( UE3BuildConfiguration.bForceLive )
				{
					return ( "Live" );
				}
				break;

			case UnrealTargetPlatform.Win64:
				// no onlinesubsystems support Win64
				break;

			case UnrealTargetPlatform.Xbox360:
				// Xbox360 only supports Live
				break;

			case UnrealTargetPlatform.PS3:
				// Neither Steamworks nor Live are supported on the PS3
				if( UE3BuildConfiguration.bForceGameSpy )
				{
					return ( "GameSpy" );
				}
				break;

			case UnrealTargetPlatform.IPhoneDevice:
			case UnrealTargetPlatform.IPhoneSimulator:
				// Only GameCenter (and PC) are supported on the IPhone
				if (UE3BuildConfiguration.bForceGameCenter)
				{
					return ("GameCenter");
				}
				break;
			}

			return ( null );
		}

		public static bool SupportsOSSPC()
		{
			// No external dependencies required for this
			return ( true );
		}

		public static bool SupportsOSSSteamworks()
		{
			// Check for command line/env var suppression
			if( !UE3BuildConfiguration.bAllowSteamworks )
			{
				return ( false );
			}

			if( !Directory.Exists( "../External/Steamworks" ) )
			{
				return ( false );
			}

			if( !File.Exists( "OnlineSubsystemSteamworks/OnlineSubsystemSteamworks.vcproj" ) )
			{
				return ( false );
			}

			return ( true );
		}

		public static bool SupportsOSSGameSpy()
		{
			// Check for command line/env var suppression
			if( !UE3BuildConfiguration.bAllowGameSpy )
			{
				return ( false );
			}

			if( !Directory.Exists( "../External/GameSpy" ) )
			{
				return ( false );
			}

			if( !File.Exists( "OnlineSubsystemGamespy/OnlineSubsystemGamespy.vcproj" ) )
			{
				return ( false );
			}

			return ( true );
		}

		public static bool SupportsOSSGameCenter()
		{
			// Check for command line/env var suppression
			if (!UE3BuildConfiguration.bAllowGameCenter)
			{
				return false;
			}

			if (!File.Exists("OnlineSubsystemGamecenter/OnlineSubsystemGamecenter.vcproj"))
			{
				return false;
			}

			return true;
		}

		public static bool SupportsOSSLive()
		{
			// Check for command line/env var suppression
			if( !UE3BuildConfiguration.bAllowLive )
			{
				return ( false );
			}

			if( !File.Exists( "OnlineSubsystemLive/OnlineSubsystemLive.vcproj" ) )
			{
				return ( false );
			}

			string GFWLDir = Environment.GetEnvironmentVariable( "GFWLSDK_DIR" );
			if( ( GFWLDir == null ) || !Directory.Exists( GFWLDir.TrimEnd( "\\".ToCharArray() ) ) )
			{
				return ( false );
			}

			// Do the special env var checking for GFWL
			string WithPanoramaEnvVar = Environment.GetEnvironmentVariable( "WITH_PANORAMA" );
			if( WithPanoramaEnvVar != null )
			{
				bool Setting = false;
				try
				{
					Setting = Convert.ToBoolean( WithPanoramaEnvVar );
				}
				catch
				{
				}

				if( !Setting )
				{
					return ( false );
				}
			}

			return ( true );
		}

		/// <summary>
		/// Adds the paths and projects needed for Windows Live
		/// </summary>
		void SetUpPCEnvironment()
		{
			GlobalCPPEnvironment.IncludePaths.Add( "OnlineSubsystemPC/Inc" );
			NonGameProjects.Add( new UE3ProjectDesc( "OnlineSubsystemPC/OnlineSubsystemPC.vcproj" ) );
		}

		/// <summary>
		/// Adds the paths and projects needed for Steamworks
		/// </summary>
        void SetUpSteamworksEnvironment()
        {
			// Compile and link with Steamworks.
            GlobalCPPEnvironment.IncludePaths.Add("OnlineSubsystemSteamworks/Inc");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/Steamworks/sdk/public");
            FinalLinkEnvironment.LibraryPaths.Add("../External/Steamworks/sdk/redistributable_bin");
            FinalLinkEnvironment.AdditionalLibraries.Add("steam_api.lib");
            NonGameProjects.Add(new UE3ProjectDesc("OnlineSubsystemSteamworks/OnlineSubsystemSteamworks.vcproj"));

			GlobalCPPEnvironment.Definitions.Add( "WITH_STEAMWORKS=1" );
		}

		/// <summary>
		/// Adds the paths and projects needed for GameSpy
		/// </summary>
        void SetUpGameSpyEnvironment()
        {
            GlobalCPPEnvironment.IncludePaths.Add("OnlineSubsystemGameSpy/Inc");

            // Compile and link with GameSpy.
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/GameSpy");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/GameSpy/voice2/speex-1.0.5/include");
			GlobalCPPEnvironment.Definitions.Add( "GSI_UNICODE=1" );
            GlobalCPPEnvironment.Definitions.Add("SB_ICMP_SUPPORT=1");
            GlobalCPPEnvironment.Definitions.Add("_CRT_SECURE_NO_WARNINGS=1");
            GlobalCPPEnvironment.Definitions.Add("UNIQUEID=1");
            GlobalCPPEnvironment.Definitions.Add("DIRECTSOUND_VERSION=0x0900");
            NonGameProjects.Add( new UE3ProjectDesc( "OnlineSubsystemGamespy/OnlineSubsystemGamespy.vcproj" ) );

			GlobalCPPEnvironment.Definitions.Add( "WITH_GAMESPY=1" );
			if (Platform == UnrealTargetPlatform.PS3)
			{
				GlobalCPPEnvironment.Definitions.Add("GP_NP_BUDDY_SYNC=1");
			}
        }

		/// <summary>
		/// Adds the paths and projects needed for Windows Live
		/// </summary>
		void SetUpWindowsLiveEnvironment()
		{
			GlobalCPPEnvironment.IncludePaths.Add( "OnlineSubsystemLive/Inc" );

			GlobalCPPEnvironment.IncludePaths.Add( "$(GFWLSDK_DIR)include" );
			FinalLinkEnvironment.LibraryPaths.Add( "$(GFWLSDK_DIR)lib/x86" );
			NonGameProjects.Add( new UE3ProjectDesc( "OnlineSubsystemLive/OnlineSubsystemLive.vcproj" ) );

			GlobalCPPEnvironment.Definitions.Add( "WITH_PANORAMA=1" );
		}

		/// <summary>
		/// Adds the paths and projects needed for Game Center
		/// </summary>
		void SetUpGameCenterEnvironment()
		{
			GlobalCPPEnvironment.Definitions.Add("WITH_GAMECENTER=1");
			GlobalCPPEnvironment.IncludePaths.Add("OnlineSubsystemGameCenter/Inc");
			NonGameProjects.Add(new UE3ProjectDesc("OnlineSubsystemGameCenter/OnlineSubsystemGameCenter.vcproj"));
		}

        /// <summary>
        /// Adds the libraries needed for SpeedTree 5.0
        /// </summary>
        void SetUpSpeedTreeEnvironment()
        {
            if (UE3BuildConfiguration.bCompileSpeedTree && Directory.Exists("../External/SpeedTree/Lib"))
            {
                GlobalCPPEnvironment.Definitions.Add("WITH_SPEEDTREE=1");
                GlobalCPPEnvironment.IncludePaths.Add("../External/SpeedTree/Include");

                string SpeedTreeLibrary = "SpeedTreeCore_";

                if (File.Exists("../External/SpeedTree/Lib/Windows/VC9/SpeedTreeCore_v5.2_VC90MTDLL_Static_d.lib"))
                {
                    SpeedTreeLibrary += "v5.2_";
                } 
                else if (File.Exists("../External/SpeedTree/Lib/Windows/VC9/SpeedTreeCore_v5.1_VC90MTDLL_Static_d.lib"))
                {
                    SpeedTreeLibrary += "v5.1_";
                }
                else
                {
                    SpeedTreeLibrary += "v5.0_";
                }

                if (Platform == UnrealTargetPlatform.Win64)
                {
                    FinalLinkEnvironment.LibraryPaths.Add("../External/SpeedTree/Lib/Windows/VC9.x64/");
                    SpeedTreeLibrary += "VC90MTDLL64_Static";
                }
                else if (Platform == UnrealTargetPlatform.Win32)
                {
                    FinalLinkEnvironment.LibraryPaths.Add("../External/SpeedTree/Lib/Windows/VC9/");
                    SpeedTreeLibrary += "VC90MTDLL_Static";
                }
                else if (Platform == UnrealTargetPlatform.Xbox360)
                {
                    FinalLinkEnvironment.LibraryPaths.Add("../External/SpeedTree/Lib/360/VC9/");
                    SpeedTreeLibrary += "VC90MT_Static";
                }
                else if (Platform == UnrealTargetPlatform.PS3)
                {
                    FinalLinkEnvironment.LibraryPaths.Add("../External/SpeedTree/Lib/PS3/");
                    SpeedTreeLibrary += "Static";
                }

                if (Configuration == UnrealTargetConfiguration.Debug)
                {
                    SpeedTreeLibrary += "_d";
                }

                if (Platform != UnrealTargetPlatform.PS3)
                {
                    SpeedTreeLibrary += ".lib";
                }

                FinalLinkEnvironment.AdditionalLibraries.Add(SpeedTreeLibrary);
            }
            else
            {
                GlobalCPPEnvironment.Definitions.Add("WITH_SPEEDTREE=0");
            }
        }

        void SetUpPhysXEnvironment()
		{
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/Foundation/include");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/Physics/include");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/Physics/include/fluids");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/Physics/include/softbody");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/Physics/include/cloth");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/Cooking/include");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/PhysXLoader/include");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/PhysXExtensions/include");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/SDKs/TetraMaker/NxTetra");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/Nxd/include");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/PxFoundation");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/PxFoundation/legacy");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/PxRenderDebug");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/PxTask");
			if (Platform == UnrealTargetPlatform.Win32 || Platform == UnrealTargetPlatform.Win64)
			{
				GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/PxFoundation/windows");
			}
			else if (Platform == UnrealTargetPlatform.PS3)
			{
				GlobalCPPEnvironment.SystemIncludePaths.Add("PS3/External/PhysX/PxFoundation/ps3");
			}
			else if (Platform == UnrealTargetPlatform.Xbox360)
			{
				GlobalCPPEnvironment.SystemIncludePaths.Add("Xenon/External/PhysX/SDKs/Foundation/include");
				GlobalCPPEnvironment.SystemIncludePaths.Add("Xenon/External/PhysX/PxFoundation/xbox360");
			}

            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/framework/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/NxParameterized/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/EditorWidgets");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/module/clothing/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/module/destructible/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/module/emitter/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/module/explosion/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/module/forcefield/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/module/iofx/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/module/basicios/public");
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/PhysX/APEX/module/particles/public");

            // If USE_DEBUG_NOVODEX is defined in UnNovodexSupport.h, then the PhysXLoaderDEBUG.dll should be added to the delay load list!
            bool UseDebugNovodex = false;

            if (Platform == UnrealTargetPlatform.Win64)
            {
                if (UseDebugNovodex && (Configuration == UnrealTargetConfiguration.Debug))
                {
                    FinalLinkEnvironment.DelayLoadDLLs.Add("PhysXLoader64DEBUG.dll");
                }
                else
                {
                    FinalLinkEnvironment.DelayLoadDLLs.Add("PhysXLoader64.dll");
                }
            }
 			else
 			{
                if (UseDebugNovodex && (Configuration == UnrealTargetConfiguration.Debug))
                {
                    FinalLinkEnvironment.DelayLoadDLLs.Add("PhysXLoaderDEBUG.dll");
                }
                else
                {
                    FinalLinkEnvironment.DelayLoadDLLs.Add("PhysXLoader.dll");
                }
			}
		}

		void SetUpFaceFXEnvironment()
		{
			// If FaceFX isn't available locally, disable it.
            if ( !UE3BuildConfiguration.bCompileFaceFX || !Directory.Exists( "../External/FaceFX" ) )
			{
				GlobalCPPEnvironment.Definitions.Add("WITH_FACEFX=0");
			}
			else
			{
				// Compile and link with the FaceFX SDK on all platforms.
				GlobalCPPEnvironment.SystemIncludePaths.Add("../External/FaceFX/FxSDK/Inc");
				GlobalCPPEnvironment.SystemIncludePaths.Add("../External/FaceFX/FxCG/Inc");
				GlobalCPPEnvironment.SystemIncludePaths.Add("../External/FaceFX/FxAnalysis/Inc");

				// Only compile FaceFX studio on Windows and only if we want it to be compiled.
				if( UE3BuildConfiguration.bCompileFaceFXStudio 
					&& (Platform == UnrealTargetPlatform.Win32 || Platform == UnrealTargetPlatform.Win64 )
					&& UE3BuildConfiguration.bBuildEditor)
				{
					// Compile and link with FaceFX studio on Win32.
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Main/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Widgets/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Framework/Audio/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Framework/Commands/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Framework/Console/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Framework/GUI/Inc");
                    GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Framework/Misc/Extensions/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Framework/Misc/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Framework/Proxies/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/Framework/Gestures/Inc");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/External/OpenAL/include");
					GlobalCPPEnvironment.IncludePaths.Add("../External/FaceFX/Studio/External/libresample-0.1.3/include");
                    GlobalCPPEnvironment.IncludePaths.Add("../External/tinyXML");
					NonGameProjects.Add( new UE3ProjectDesc( "../External/FaceFX/Studio/Studio_vs9.vcproj" ) );
				}
				else
				{
					GlobalCPPEnvironment.Definitions.Add("WITH_FACEFX_STUDIO=0");
				}
			}
		}

		void SetUpBinkEnvironment()
		{
			bool bPlatformAllowed = 
					Platform != UnrealTargetPlatform.IPhoneDevice
				&&	Platform != UnrealTargetPlatform.IPhoneSimulator
				&&	Platform != UnrealTargetPlatform.Android
				&&	Platform != UnrealTargetPlatform.NGP;

			if( UE3BuildConfiguration.bCompileBink && Directory.Exists( "../External/Bink" ) && bPlatformAllowed )
			{
				// Bink is allowed, add the proper include paths, but let UnBuild.h to specify USE_BINK_CODEC
				GlobalCPPEnvironment.SystemIncludePaths.Add( "../External/Bink" );
			}
			else
			{
				// Not using Bink
				GlobalCPPEnvironment.Definitions.Add( "USE_BINK_CODEC=0" );
			}
		}

		void SetUpSourceControlEnvironment()
		{
			// Perforce can only be used under managed code
			if (UE3BuildConfiguration.bCompilePerforce && UE3BuildConfiguration.bBuildEditor && UE3BuildConfiguration.bAllowManagedCode)
			{
				GlobalCPPEnvironment.Definitions.Add("WITH_PERFORCE=1");
				if (Platform == UnrealTargetPlatform.Win32)
				{
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add("../../Binaries/win32/p4api.dll");
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add("../../Binaries/win32/p4dn.dll");
				}
				else if (Platform == UnrealTargetPlatform.Win64)
				{
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add("../../Binaries/win64/p4api.dll");
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add("../../Binaries/win64/p4dn.dll");
				}
			}
		}

        string GFxDir;
        void SetUpGFxEnvironment()
        {
			bool bPlatformAllowed = 
					Platform != UnrealTargetPlatform.IPhoneDevice
				&&	Platform != UnrealTargetPlatform.IPhoneSimulator
				&&	Platform != UnrealTargetPlatform.Android;

			if ( !bPlatformAllowed || !UE3BuildConfiguration.bCompileScaleform )
			{
				GlobalCPPEnvironment.Definitions.Add("WITH_GFx=0");
			}
            if (UE3BuildConfiguration.bForceScaleformRelease && Configuration == UnrealTargetConfiguration.Debug)
            {
				if (Platform != UnrealTargetPlatform.PS3)
				{
					// This override does not work on the PS3; we get linker errors.
					GlobalCPPEnvironment.Definitions.Add("GFC_BUILD_RELEASE");
				}
            }

			// We always need to include Scaleform GFx headers!
			// When needed, GFx will be compiled out in the code via WITH_GFx=0.
			{
				GFxDir = "../External/GFx";

				// Scaleform headers
				GlobalCPPEnvironment.SystemIncludePaths.Add(GFxDir + "/Include");
				GlobalCPPEnvironment.SystemIncludePaths.Add(GFxDir + "/Src/GRenderer");
				GlobalCPPEnvironment.SystemIncludePaths.Add(GFxDir + "/Src/GFxIME");
			}

            if (Configuration == UnrealTargetConfiguration.Shipping)
            {
                GlobalCPPEnvironment.Definitions.Add("GFC_BUILD_SHIPPING");
            }
        }
	}
}
