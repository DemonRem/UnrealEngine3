/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;

namespace UnrealBuildTool
{
	class UE3BuildGearGame : UE3BuildGame
	{
        /** Returns the singular name of the game being built ("Example", "UT3", etc) */
		public string GetGameName()
		{
			return "GearGame";
		}
		
		/** Returns a subplatform (e.g. dll) to disambiguate object files */
		virtual public string GetSubPlatform()
		{
			return ( "" );
		}

		/** Get the desired OnlineSubsystem. */
		public string GetDesiredOnlineSubsystem( CPPEnvironment CPPEnv, UnrealTargetPlatform Platform )
		{
			// Note: no overriding of the Online Subsystem allowed
			switch( Platform )
			{
			case UnrealTargetPlatform.Win32:
			case UnrealTargetPlatform.Xbox360:
				if( UE3BuildTarget.SupportsOSSLive() )
				{
					return ( "Live" );
				}
				break;
			}

			return ( "PC" );
		}

		/** Returns true if the game wants to have PC ES2 simulator (ie ES2 Dynamic RHI) enabled */
		public bool ShouldCompileES2()
		{
			return false;
		}
		
		/** Allows the game add any global environment settings before building */
        public void GetGameSpecificGlobalEnvironment(CPPEnvironment GlobalEnvironment)
        {
			// Disable TTS and speech recognition for GearGame only
			GlobalEnvironment.Definitions.Add( "WITH_TTS=0" );
			GlobalEnvironment.Definitions.Add( "WITH_SPEECH_RECOGNITION=0" );
		}

        /** Returns the xex.xml file for the given game */
		public FileItem GetXEXConfigFile()
		{
			return FileItem.GetExistingItemByPath("GearGame/Live/xex.xml");
		}

        /** Allows the game to add any additional environment settings before building */
		virtual public void SetUpGameEnvironment(CPPEnvironment GameCPPEnvironment, LinkEnvironment FinalLinkEnvironment, List<UE3ProjectDesc> GameProjects)
		{
			GameProjects.Add( new UE3ProjectDesc( "GearGame/GearGame.vcproj" ) );
			GameCPPEnvironment.IncludePaths.Add("GearGame/Inc");

			if (UE3BuildConfiguration.bBuildEditor &&
				(GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Win64))
			{
				GameProjects.Add( new UE3ProjectDesc( "GearEditor/GearEditor.vcproj" ) );
				GameCPPEnvironment.IncludePaths.Add("GearEditor/Inc");
			}

			GameCPPEnvironment.Definitions.Add("GAMENAME=GEARGAME");
			GameCPPEnvironment.Definitions.Add("IS_GEARGAME=1");

			if (GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Xbox360)
			{
				// Compile and link with the XNA content server on Xbox 360.
				GameCPPEnvironment.IncludePaths.Add("../External/XNAContentServer/include");
				FinalLinkEnvironment.LibraryPaths.Add("../External/XNAContentServer/lib/xbox");
			}
		}
	}

    class UE3BuildGearGameServer : UE3BuildGearGame
    {
		/** Returns a subplatform (e.g. dll) to disambiguate object files */
		override public string GetSubPlatform()
		{
			return ( "Server" );
		}
    }
}
