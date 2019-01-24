/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;

namespace UnrealBuildTool
{
	class UE3BuildNanoGame : UE3BuildGame
	{
        /** Returns the singular name of the game being built ("Example", "UT3", etc) */
		public string GetGameName()
		{
			return "NanoGame";
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

			return ( "PC" );
		}

		/** Returns true if the game wants to have PC ES2 simulator (ie ES2 Dynamic RHI) enabled */
		public bool ShouldCompileES2()
		{
			return false;
		}

		/** Allows the game add any global environment settings before building */
        public void GetGameSpecificGlobalEnvironment( CPPEnvironment GlobalEnvironment )
        {
            // Disable ActorX in Nano
            GlobalEnvironment.Definitions.Add("WITH_ACTORX=0");
        }

        /** Returns the xex.xml file for the given game */
		public FileItem GetXEXConfigFile()
		{
			return FileItem.GetExistingItemByPath("NanoGame/Live/xex.xml");
		}

        /** Allows the game to add any additional environment settings before building */
		public void SetUpGameEnvironment(CPPEnvironment GameCPPEnvironment, LinkEnvironment FinalLinkEnvironment, List<UE3ProjectDesc> GameProjects)
		{
			GameCPPEnvironment.IncludePaths.Add( "NanoGameShared/Inc" );
			GameProjects.Add( new UE3ProjectDesc( "NanoGameShared/NanoGameShared.vcproj" ) );

			GameProjects.Add( new UE3ProjectDesc( "NanoGame/NanoGame.vcproj" ) );
			GameCPPEnvironment.IncludePaths.Add( "NanoGame/Inc" );

			if (UE3BuildConfiguration.bBuildEditor &&
				(GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Win64))
			{
				GameProjects.Add( new UE3ProjectDesc( "NanoEditor/NanoEditor.vcproj" ) );
				GameCPPEnvironment.IncludePaths.Add("NanoEditor/Inc");
			}

			GameCPPEnvironment.Definitions.Add("GAMENAME=NANOGAME");
			GameCPPEnvironment.Definitions.Add("IS_NANOGAME=1");

			if (GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Xbox360)
			{
				// Compile and link with Vince on Xbox360.
				GameCPPEnvironment.IncludePaths.Add("../External/Vince/include");

				// Compile and link with the XNA content server on Xbox 360.
				GameCPPEnvironment.IncludePaths.Add("../External/XNAContentServer/include");
				FinalLinkEnvironment.LibraryPaths.Add("../External/XNAContentServer/lib/xbox");
			}
		}
	}
}
