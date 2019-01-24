/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;

namespace UnrealBuildTool
{
	class UE3BuildNanoMobileGame : UE3BuildGame
	{
        /** Returns the singular name of the game being built ("Castle", "UT3", etc) */
		public string GetGameName()
		{
			return "NanoMobileGame";
		}

		/** Returns a subplatform (e.g. dll) to disambiguate object files */
		public string GetSubPlatform()
		{
			return ( "" );
		}

		/** Get the desired OnlineSubsystem. */
		public string GetDesiredOnlineSubsystem( CPPEnvironment CPPEnv, UnrealTargetPlatform Platform )
		{
			string ForcedOSS = UE3BuildTarget.ForceOnlineSubsystem( Platform );
			if( ForcedOSS != null )
			{
				return ( ForcedOSS );
			}
			else
			{
				switch( Platform )
				{
					case UnrealTargetPlatform.IPhoneDevice:
					case UnrealTargetPlatform.IPhoneSimulator:
						if (UE3BuildTarget.SupportsOSSGameCenter())
						{
							return ("GameCenter");
						}
						break;

					default:
						return "PC";
				}
			}

			return "PC";
		}

		/** Returns true if the game wants to have PC ES2 simulator (ie ES2 Dynamic RHI) enabled */
		public bool ShouldCompileES2()
		{
			return true;
		}
		
		/** Allows the game add any global environment settings before building */
        public void GetGameSpecificGlobalEnvironment(CPPEnvironment GlobalEnvironment)
        {
        }

        /** Returns the xex.xml file for the given game */
		public FileItem GetXEXConfigFile()
		{
			return FileItem.GetExistingItemByPath("NanoMobileGame/Live/xex.xml");
		}

        /** Allows the game to add any additional environment settings before building */
		public void SetUpGameEnvironment(CPPEnvironment GameCPPEnvironment, LinkEnvironment FinalLinkEnvironment, List<UE3ProjectDesc> GameProjects)
		{
            GameCPPEnvironment.IncludePaths.Add("NanoGameShared/Inc");
            GameProjects.Add(new UE3ProjectDesc("NanoGameShared/NanoGameShared.vcproj"));

            GameProjects.Add(new UE3ProjectDesc("NanoMobileGame/NanoMobileGame.vcproj"));
            GameCPPEnvironment.IncludePaths.Add("NanoMobileGame/Inc");

// 			if (UE3BuildConfiguration.bBuildEditor &&
// 				(GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Win64))
// 			{
            // 				GameProjects.Add( new UE3ProjectDesc( "NanoMobileGameEditor/NanoMobileGameEditor.vcproj") );
            // 				GameCPPEnvironment.IncludePaths.Add("NanoMobileGameEditor/Inc");
// 			}

			GameCPPEnvironment.Definitions.Add("GAMENAME=NANOMOBILEGAME");
			GameCPPEnvironment.Definitions.Add("IS_NANOMOBILEGAME=1");
			GameCPPEnvironment.Definitions.Add("SUPPORTS_TILT_PUSHER=1");

		}
	}
}
