/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace UnrealBuildTool
{
	class UE3BuildUDKGame : UE3BuildGame
	{
        /** Returns the singular name of the game being built ("Example", "UT3", etc) */
		public string GetGameName()
		{
			return "UDKGame";
		}

		/** Returns a subplatform (e.g. dll) to disambiguate object files */
		virtual public string GetSubPlatform()
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
				case UnrealTargetPlatform.Win32:
					if( UE3BuildTarget.SupportsOSSSteamworks() )
					{
						return ( "Steamworks" );
					}
					break;

				case UnrealTargetPlatform.Xbox360:
					if( UE3BuildTarget.SupportsOSSLive() )
					{
						return ( "Live" );
					}
					break;

				case UnrealTargetPlatform.PS3:
					if( CPPEnv.Definitions.Contains( "UDK=1" ) )
					{
						return ( "PC" );
					}
					else if( UE3BuildTarget.SupportsOSSGameSpy() )
					{
						return ( "GameSpy" );
					}
					break;
				}
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
        }

        /** Returns the xex.xml file for the given game */
		public FileItem GetXEXConfigFile()
		{
			return FileItem.GetExistingItemByPath("UTGame/Live/xex.xml");
		}

        /** Allows the game to add any additional environment settings before building */
		public void SetUpGameEnvironment(CPPEnvironment GameCPPEnvironment, LinkEnvironment FinalLinkEnvironment, List<UE3ProjectDesc> GameProjects)
		{
            GameProjects.Add(new UE3ProjectDesc("UDKBase/UDKBase.vcproj"));
            GameCPPEnvironment.IncludePaths.Add("UDKBase/Inc");

            GameProjects.Add(new UE3ProjectDesc("UTGame/UTGame.vcproj"));

			if (UE3BuildConfiguration.bBuildEditor && 
				(GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Win32 || GameCPPEnvironment.TargetPlatform == CPPTargetPlatform.Win64 ))
			{
				GameProjects.Add( new UE3ProjectDesc( "UTEditor/UTEditor.vcproj" ) );
				GameCPPEnvironment.IncludePaths.Add("UTEditor/Inc");
			}

			GameCPPEnvironment.Definitions.Add("GAMENAME=UTGAME");
			GameCPPEnvironment.Definitions.Add("IS_UTGAME=1");
		}
	}

	class UE3BuildUDKGameDLL : UE3BuildUDKGame
	{
		/** Returns a subplatform (e.g. dll) to disambiguate object files */
		override public string GetSubPlatform()
		{
			return ( "DLL" );
		}
	}
}
