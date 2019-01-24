/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Data;
using System.Data.SqlClient;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace Controller
{
	public partial class Main : Form
	{
		private class CISTask
		{
			private string TestType;
			private bool CompileTaskRequired;
			private bool GameSpecific;
			private bool PlatformSpecific;
			private bool CompileAllowed;
			private bool LockPS3SDK;
			private bool LockXDK;
			private int LastAttempted;
			private List<string> Folders;

			public CISTask( string InTestType, bool InGameSpecific, bool InPlatformSpecific, bool InCompileAllowed, List<string> InFolders )
			{
				TestType = InTestType;
				CompileTaskRequired = false;
				GameSpecific = InGameSpecific;
				PlatformSpecific = InPlatformSpecific;
				CompileAllowed = InCompileAllowed;
				Folders = InFolders;
				LastAttempted = -1;
				LockPS3SDK = true;
				LockXDK = true;

				if( TestType.ToLower() == "xbox360" )
				{
					LockPS3SDK = false;
				}
				if( TestType.ToLower() == "sonyps3" )
				{
					LockXDK = false;
				}			
			}

			public string GetTestType()
			{
				return ( TestType );
			}

			public void CompileRequired()
			{
				CompileTaskRequired = true;
			}

			public bool CheckScript( ScriptParser Builder, string FileName )
			{
				if( GameSpecific )
				{
					if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/" + TestType ) )
					{
						CompileRequired();
						return ( true );
					}
				}

				return ( false );
			}

			public void GetLastAttempted( ScriptParser Builder, BuilderDB DB )
			{
				string Query = "LastAttempted" + TestType;
				LastAttempted = DB.GetIntForBranch( Builder.BranchDef.Branch, Query );

				if( LastAttempted > 0 && Folders != null )
				{
					foreach( string Folder in Folders )
					{
						if( !Directory.Exists( Path.Combine( Environment.CurrentDirectory, Folder ) ) )
						{
							LastAttempted = -1;
						}
					}
				}
			}

			public void SpawnJobs( Main Parent, ScriptParser Builder, BuilderDB DB, string BaseCommand, int Changelist, long JobSpawnTime, bool UpdateLatestProcessedChangelist )
			{
				if( LastAttempted > 0 )
				{
					if( CompileTaskRequired && CompileAllowed )
					{
						string Game = GameSpecific ? TestType : "All";
						string Platform = PlatformSpecific ? TestType : "Win32";
						string LockPS3SDKString = LockPS3SDK ? "1" : "0";
						string LockXDKString = LockXDK ? "1" : "0";

						string CommandString = Parent.CreateJobUpdateMessage( "CIS Code Builder (" + TestType + ")",
																			   "Jobs/CISCodeBuilder" + TestType,
																			   Platform,
																			   Game,
																			   "Release",
																			   "",
																			   "",
																			   "",
																			   "",
																			   Builder.BranchDef.Branch,
																			   LockPS3SDKString,
																			   LockXDKString,
																			   Changelist.ToString(),
																			   "0",
																			   JobSpawnTime );

						DB.Update( BaseCommand + "( " + CommandString + " )" );

						CompileTaskRequired = false;
					}

					if( UpdateLatestProcessedChangelist )
					{
						DB.SetIntForBranch( Builder.BranchDef.Branch, "LastAttempted" + TestType, Changelist );
					}
				}
			}
		}

		enum CISTaskTypes
		{
			Example = 0,
			Mobile,
			Gear,
			Nano,
			NanoMobile,
			Sword,
			UDK,
			SonyPS3,
			Xbox360,
			MobileDevice,
			Tools
		}

		private List<CISTask> CISTasks = new List<CISTask>()
		{
			new CISTask( "Example", true, false, true, new List<string>() { "ExampleGame" } ),
			new CISTask( "Mobile", true, false, true, new List<string>() { "MobileGame" } ),
			new CISTask( "Gear", true, false, true, new List<string>() { "GearGame" } ),
			new CISTask( "Nano", true, false, true, new List<string>() { "NanoGame" } ),
			new CISTask( "NanoMobile", true, false, true, new List<string>() { "NanoMobileGame" } ),
			new CISTask( "Sword", true, false, true, new List<string>() { "SwordGame" } ),
			new CISTask( "UDK", true, false, true, new List<string>() { "UDKGame" } ),
		
			new CISTask( "SonyPS3", false, true, true, new List<string>() { "Development/Src/PS3" } ),
			new CISTask( "Xbox360", false, true, true, new List<string>() { "Development/Src/Xenon" } ),
		
			new CISTask( "MobileDevice", false, false, true, new List<string>() { "Development/Src/IPhone", "Development/Src/Android" } ),
			new CISTask( "Tools", false, false, true, null ),
		};

		private void ProcessP4Changes()
		{
#if DEBUG
			Builder.BranchDef.Branch = "UnrealEngine3";
#endif
			Builder.OpenLog( Builder.GetLogFileName( COMMANDS.CISProcessP4Changes ), false );

			// Get the range of changes and determine if there's any work to do
			int CISLastChangelistProcessed = DB.GetIntForBranch( Builder.BranchDef.Branch, "LastAttemptedOverall" );

			// If there was an error of some kind, just break out of this command
			if( CISLastChangelistProcessed <= 0 )
			{
				return;
			}

			// Define a null range of changes that doesn't update the DB as the default
			int CISFirstChangelistToProcess = CISLastChangelistProcessed + 1;
			int CISLastChangelistToProcess = CISLastChangelistProcessed;
			bool UpdateLatestProcessedChangelist = false;

			// Commonly used value
			int LatestChangelist = SCC.GetLatestChangelist( Builder, "//" + MachineName + "/" + Builder.BranchDef.Branch + "/..." );

			// First, see whether we are building a single, non-DB-updating changelist
			if( Builder.CommandLine.ToLower().StartsWith( "changelist" ) )
			{
				// Determine if the changelist is specified or simply #head
				string ChangelistToBuildFrom = Builder.CommandLine.Substring( "changelist".Length ).Trim();
				if( ChangelistToBuildFrom.ToLower().CompareTo( "head" ) == 0 )
				{
					// Set these values equal to one another to ensure a build
					CISLastChangelistToProcess = LatestChangelist;
					CISFirstChangelistToProcess = CISLastChangelistToProcess;
				}
				else
				{
					// Set these values equal to one another to ensure a build
					CISFirstChangelistToProcess = Builder.SafeStringToInt( ChangelistToBuildFrom );
					CISLastChangelistToProcess = CISFirstChangelistToProcess;
				}

				Builder.Write( "Building only changelist " + CISFirstChangelistToProcess.ToString() + " and not updating CIS Monitor values" );
			}
			else if( Builder.CommandLine.ToLower().CompareTo( "refresh" ) == 0 )
			{
				// Refresh CIS by spawning all the CIS tasks
				SpawnAllCISTasks( LatestChangelist );
				Builder.Write( "Refreshing CIS" );
			}
			else
			{
				// By default, process every single changelist and update the DB accordingly
				CISLastChangelistToProcess = LatestChangelist;
				CISFirstChangelistToProcess = CISLastChangelistProcessed + 1;
				UpdateLatestProcessedChangelist = true;

				Builder.Write( "Building all out-of-date changelists" );
			}

			// Process any changelists as defined by a valid range set above
			if( CISLastChangelistToProcess >= CISFirstChangelistToProcess )
			{
				SpawnCISTasks( CISFirstChangelistToProcess, CISLastChangelistToProcess, UpdateLatestProcessedChangelist );
			}
			else
			{
				Builder.Write( "No new changes have been checked in!" );
			}

			// Done
			Builder.CloseLog();
		}

		private void UpdateMonitorValues()
		{
#if DEBUG
			Builder.BranchDef.Branch = "UnrealEngine3";
#endif
			Builder.OpenLog( Builder.GetLogFileName( COMMANDS.CISUpdateMonitorValues ), false );

			// Compute a minimum of the overall good builds, and a maximum of the overall failed builds
			int LastGood_Overall = Int32.MaxValue;
			int LastFailed_Overall = 0;

			bool BuildIsGood_Overall = true;
			bool SuccessChanged_Overall = false;
			int SuccessChanger_Overall = 0;

			int LastGood = 0;
			int LastFailed = 0;

			bool BuildIsGood = false;
			bool SuccessChanged = false;
			int SuccessChanger = 0;

			int BuildLogID;

			foreach( CISTask Task in CISTasks )
			{
				BuildIsGood = DB.UpdateLastGoodAndFailedValues( Builder.BranchDef.Branch, Task.GetTestType(), out LastGood, out LastFailed, out SuccessChanged, out SuccessChanger, out BuildLogID );

				// LastGood and/or LastFailed are -1 if CIS is disabled
				if( LastGood > 0 && LastFailed > 0 )
				{
					if( BuildIsGood && SuccessChanged )
					{
						SuccessChanged_Overall = true;
						SuccessChanger_Overall = ( SuccessChanger_Overall > SuccessChanger ) ? SuccessChanger_Overall : SuccessChanger;
					}

					BuildIsGood_Overall &= BuildIsGood;
					LastGood_Overall = ( LastGood < LastGood_Overall ) ? LastGood : LastGood_Overall;
					LastFailed_Overall = ( LastFailed > LastFailed_Overall ) ? LastFailed : LastFailed_Overall;
				}
			}

			// Update the summary variables in the DB
			Log( "Overall LastGood   = " + LastGood_Overall, Color.Green );
			Log( "Overall LastFailed = " + LastFailed_Overall, Color.Green );

			if( LastGood_Overall > 0 && LastFailed_Overall > 0 )
			{
				if( BuildIsGood_Overall && SuccessChanged_Overall )
				{
					int OldLastGood_Overall = DB.GetIntForBranch( Builder.BranchDef.Branch, "LastGoodOverall" );
					Mailer.SendCISMail( Builder, "CIS (" + Builder.BranchDef.Branch + ")", OldLastGood_Overall.ToString(), SuccessChanger_Overall.ToString() );
				}

				DB.SetIntForBranch( Builder.BranchDef.Branch, "LastGoodOverall", LastGood_Overall );
				DB.SetIntForBranch( Builder.BranchDef.Branch, "LastFailOverall", LastFailed_Overall );
			}
			else
			{
				Log( "CIS disabled; database not updated", Color.Green );
			}

			// Done
			Builder.CloseLog();
		}

		private void SpawnAllCISTasks( int LatestChangelist )
		{
			Builder.Write( "Refreshing changelist " + LatestChangelist.ToString() );

			// Spawn all supported CIS jobs
			string BaseCommandString = "INSERT INTO Jobs ( Name, Command, Platform, Game, Config, ScriptConfig, Language, Define, Parameter, Branch, LockPDK, LockXDK, Label, Machine, BuildLogID, PrimaryBuild, Active, Complete, Succeeded, Suppressed, Killing, SpawnTime ) VALUES ";
			long JobSpawnTime = DateTime.UtcNow.Ticks;

			foreach( CISTask Task in CISTasks )
			{
				Task.GetLastAttempted( Builder, DB );
				Task.CompileRequired();
				Task.SpawnJobs( this, Builder, DB, BaseCommandString, LatestChangelist, JobSpawnTime, true );
			}
		}

		private void SpawnCISTasks( int CISFirstChangelistToProcess, int CISLastChangelistToProcess, bool UpdateLatestProcessedChangelist )
		{
			Builder.Write( "Processing changelists " + CISFirstChangelistToProcess.ToString() + " through " + CISLastChangelistToProcess.ToString() );

			// For each changelist in the range, get the description and determine what types of builds are needed (this call deposits the changes in
			// the ChangeLists instance variable of the Builder)
			SCC.GetChangesInRange( Builder, "//" + MachineName + "/" + Builder.BranchDef.Branch + "/...", CISFirstChangelistToProcess.ToString(), CISLastChangelistToProcess.ToString() );

			if( Builder.ChangeLists.Count > 0 )
			{
				long JobSpawnTime = DateTime.UtcNow.Ticks;

				foreach( CISTask Task in CISTasks )
				{
					Task.GetLastAttempted( Builder, DB );
				}

				// Then interrogate the (properly ordered) changelists for which builds to spawn
				Builder.ChangeLists.Reverse();
				foreach( ScriptParser.ChangeList CL in Builder.ChangeLists )
				{
					Builder.Write( "    Processing changelist " + CL.Number.ToString() );

					// Evaulate the changelist for jobs, trying to early-out in a few ways
					//
					// The logic for reduced CIS jobs...
					//
					// 1. Filter based on filetype
					//
					// Ends with ".uc", look further
					//      StartsWith "    ... //depot/<branch>/Development/Src/Example", add BuildScript Example
					//      StartsWith "    ... //depot/<branch>/Development/Src/Mobile", add BuildScript Mobile
					//      StartsWith "    ... //depot/<branch>/Development/Src/Gear", add BuildScript Gear
					//      StartsWith "    ... //depot/<branch>/Development/Src/Nano", add BuildScript Nano
					//      StartsWith "    ... //depot/<branch>/Development/Src/Sword", add BuildScript Sword
					//      StartsWith "    ... //depot/<branch>/Development/Src/UDK", add BuildScript UDK
					//      Otherwise, add Code Builder PC which will build everything
					//
					// Ends with ".upk", look further
					//      StartsWith "    ... //depot/<branch>/Development/Src/Example" or
					//                 "    ... //depot/<branch>/Example" or
					//          add LoadPackages Example Filename.upk
					//      StartsWith "    ... //depot/<branch>/Development/Src/Mobile" or
					//                 "    ... //depot/<branch>/Mobile" or
					//          add LoadPackages Mobile Filename.upk
					//      StartsWith "    ... //depot/<branch>/Development/Src/Gear" or
					//                 "    ... //depot/<branch>/Gear" or
					//          add LoadPackages Gear Filename.upk
					//      StartsWith "    ... //depot/<branch>/Development/Src/Nano" or
					//                 "    ... //depot/<branch>/Nano" or
					//          add LoadPackages Nano Filename.upk
					//      StartsWith "    ... //depot/<branch>/Development/Src/Sword" or
					//                 "    ... //depot/<branch>/Sword" or
					//          add LoadPackages Sword Filename.upk
					//      StartsWith "    ... //depot/<branch>/Development/Src/UDK" or
					//                 "    ... //depot/<branch>/UDK" or
					//          add LoadPackages UDK Filename.upk
					//      Otherwise, add LoadPackages Example which will pick up Engine changes
					//
					// Ends with ".umap", add LoadPackages Example Filename.umap
					// Ends with ".mobile", add LoadPackages Mobile Filename.Mobile
					// Ends with ".gear", add LoadPackages Gear Filename.gear
					// Ends with ".nano", add LoadPackages Nano Filename.nano
					// Ends with ".sword", add LoadPackages sword Filename.sword
					// Ends with ".udk", add LoadPackages UDK Filename.udk
					//
					// 2. Filter based on location
					//
					// Starts with "    ... //depot/<branch>/Development/Src/", look further
					//      StartsWith "    ... //depot/<branch>/Development/Src/PS3/", look further
					//          StartsWith "    ... //depot/<branch>/Development/Src/PS3/PS3Tools", add Code Builder Tools
					//          Otherwise, add Code Builder PS3
					//      StartsWith "    ... //depot/<branch>/Development/Src/Windows/", look further
					//          StartsWith "    ... //depot/<branch>/Development/Src/Windows/WindowsTools", add Code Builder Tools
					//          Otherwise, add Code Builder PC
					//      StartsWith "    ... //depot/<branch>/Development/Src/Xenon/", look further
					//          StartsWith "    ... //depot/<branch>/Development/Src/Xenon/XeTools", add Code Builder Tools
					//          Otherwise, add Code Builder Xenon
					//      Otherwise, add Code Builder PC, PS3, Xenon, and Tools
					//
					// Starts with "    ... //depot/<branch>/Development/Tools", look further
					//      Starts with "    ... //depot/<branch>/Development/Tools/UnrealSwarm/Agent", add Code Builder PC
					//      Otherwise, add Code Builder Tools
					//
					// Starts with "    ... //depot/<branch>/Engine/", then add everything
					//
					// Starts with "    ... //depot/<branch>/Development/External/", look further
					//      Has a .lib extension, add Code Builder PC, PS3, Xenon, and Tools
					// 
					// 3. Otherwise, ignore since it's not something CIS can handle today
					//
					// 4. Determine the set of jobs that need spawning
					// 5. Spawn jobs
					// 6. Par-tay
					//

					foreach( string FileName in CL.Files )
					{
						Builder.Write( "        Processing File " + FileName );

						if( FileName.EndsWith( ".uc" ) || FileName.EndsWith( ".uci" ) )
						{
							// In any case, kick off the job that checks for script files in projects
							// TODO: currently this is wedged into the Tools job, because it's fast
							CISTasks[( int )CISTaskTypes.Tools].CompileRequired();

							bool ScriptRequired = false;
							foreach( CISTask Task in CISTasks )
							{
								ScriptRequired |= Task.CheckScript( Builder, FileName );
							}

							if( !ScriptRequired )
							{
								CISTasks[( int )CISTaskTypes.Example].CompileRequired();
								CISTasks[( int )CISTaskTypes.Mobile].CompileRequired();
								CISTasks[( int )CISTaskTypes.Gear].CompileRequired();
								CISTasks[( int )CISTaskTypes.Nano].CompileRequired();
								CISTasks[( int )CISTaskTypes.NanoMobile].CompileRequired();
								CISTasks[( int )CISTaskTypes.Sword].CompileRequired();
								CISTasks[( int )CISTaskTypes.UDK].CompileRequired();
								continue;
							}
						}
						else if( FileName.EndsWith( ".upk" ) )
						{
							// Don't do anything for packages
							continue;
						}
						else if( FileName.EndsWith( ".umap" ) )
						{
							// Don't do anything for maps
							continue;
						}
						else if( FileName.EndsWith( ".mobile" ) )
						{
							// Don't do anything for maps
							continue;
						}
						else if( FileName.EndsWith( ".sword" ) )
						{
							// Don't do anything for maps
							continue;
						}
						else if( FileName.EndsWith( ".gear" ) )
						{
							// Don't do anything for maps
							continue;
						}
						else if( FileName.EndsWith( ".nano" ) )
						{
							// Don't do anything for maps
							continue;
						}
						else if( FileName.EndsWith( ".nanomobile" ) )
						{
							// Don't do anything for maps
							continue;
						}
						else if( FileName.EndsWith( ".ut3" ) ||
								 FileName.EndsWith( ".udk" ) )
						{
							// Don't do anything for maps
							continue;
						}
						else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/" ) && FileName.EndsWith( ".vcproj" ) )
						{
							// The tools CIS task does the vcproj validation
							CISTasks[( int )CISTaskTypes.Tools].CompileRequired();
						}
						else if( FileName.EndsWith( ".ini" ) )
						{
							// Look further
							if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Engine/Config/Base" ) )
							{
								CISTasks[( int )CISTaskTypes.Example].CompileRequired();
								CISTasks[( int )CISTaskTypes.Mobile].CompileRequired();
								CISTasks[( int )CISTaskTypes.Gear].CompileRequired();
								CISTasks[( int )CISTaskTypes.Nano].CompileRequired();
								CISTasks[( int )CISTaskTypes.NanoMobile].CompileRequired();
								CISTasks[( int )CISTaskTypes.Sword].CompileRequired();
								CISTasks[( int )CISTaskTypes.UDK].CompileRequired();
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/ExampleGame/Config/Default" ) )
							{
								CISTasks[( int )CISTaskTypes.Example].CompileRequired();
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/MobileGame/Config/Default" ) )
							{
								CISTasks[( int )CISTaskTypes.Mobile].CompileRequired();
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/GearGame/Config/Default" ) )
							{
								CISTasks[( int )CISTaskTypes.Gear].CompileRequired();
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/NanoGame/Config/Default" ) )
							{
								CISTasks[( int )CISTaskTypes.Nano].CompileRequired();
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/NanoMobileGame/Config/Default" ) )
							{
								CISTasks[( int )CISTaskTypes.NanoMobile].CompileRequired();
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/SwordGame/Config/Default" ) )
							{
								CISTasks[( int )CISTaskTypes.Sword].CompileRequired();
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/UDKGame/Config/Default" ) )
							{
								CISTasks[( int )CISTaskTypes.UDK].CompileRequired();
							}

							// Ignore platform specific inis for now

							continue;
						}
						else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/" ) )
						{
							if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Windows/" ) )
							{
								if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Windows/WindowsTools" ) )
								{
									CISTasks[( int )CISTaskTypes.Tools].CompileRequired();
									continue;
								}
								else
								{
									CISTasks[( int )CISTaskTypes.Example].CompileRequired();
									CISTasks[( int )CISTaskTypes.Mobile].CompileRequired();
									CISTasks[( int )CISTaskTypes.Gear].CompileRequired();
									CISTasks[( int )CISTaskTypes.Nano].CompileRequired();
									CISTasks[( int )CISTaskTypes.NanoMobile].CompileRequired();
									CISTasks[( int )CISTaskTypes.Sword].CompileRequired();
									CISTasks[( int )CISTaskTypes.UDK].CompileRequired();
									continue;
								}
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/PS3/" ) )
							{
								if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/PS3/PS3Tools" ) )
								{
									CISTasks[( int )CISTaskTypes.Tools].CompileRequired();
									continue;
								}
								else
								{
									CISTasks[( int )CISTaskTypes.SonyPS3].CompileRequired();
									continue;
								}
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Xenon/" ) )
							{
								if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Xenon/XeTools" ) )
								{
									CISTasks[( int )CISTaskTypes.Tools].CompileRequired();
									continue;
								}
								else
								{
									CISTasks[( int )CISTaskTypes.Xbox360].CompileRequired();
									continue;
								}
							}
							// All iPhone, Android, and ES2 code paths are specific (currently) to Mobile
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/IPhone/" ) ||
									 FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Android/" ) ||
									 FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/ES2Drv/" ) )
							{
								if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/IPhone/IPhoneTools" ) ||
									FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Android/AndroidTools" ) )
								{
									CISTasks[( int )CISTaskTypes.Tools].CompileRequired();
									continue;
								}
								else
								{
									CISTasks[( int )CISTaskTypes.MobileDevice].CompileRequired();
									continue;
								}
							}
							// See if the file is game-specific
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Example" ) )
							{
								CISTasks[( int )CISTaskTypes.Example].CompileRequired();
								// The console builds build all games
								CISTasks[( int )CISTaskTypes.SonyPS3].CompileRequired();
								CISTasks[( int )CISTaskTypes.Xbox360].CompileRequired();
								continue;
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Mobile" ) )
							{
								CISTasks[( int )CISTaskTypes.Mobile].CompileRequired();
								// Include mobile as well
								CISTasks[( int )CISTaskTypes.MobileDevice].CompileRequired();
								continue;
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Gear" ) )
							{
								CISTasks[( int )CISTaskTypes.Gear].CompileRequired();
								// The console builds build all games
								CISTasks[( int )CISTaskTypes.SonyPS3].CompileRequired();
								CISTasks[( int )CISTaskTypes.Xbox360].CompileRequired();
								continue;
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/NanoMobile" ) )
							{
								CISTasks[( int )CISTaskTypes.NanoMobile].CompileRequired();
								// The console builds build all games
								CISTasks[( int )CISTaskTypes.MobileDevice].CompileRequired();
								continue;
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Nano" ) )
							{
								CISTasks[( int )CISTaskTypes.Nano].CompileRequired();
								// The console builds build all games
								CISTasks[( int )CISTaskTypes.SonyPS3].CompileRequired();
								CISTasks[( int )CISTaskTypes.Xbox360].CompileRequired();
								continue;
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/Sword" ) )
							{
								CISTasks[( int )CISTaskTypes.Sword].CompileRequired();
								// The console builds build all games
								CISTasks[( int )CISTaskTypes.MobileDevice].CompileRequired();
								continue;
							}
							else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Src/UDK" ) )
							{
								CISTasks[( int )CISTaskTypes.UDK].CompileRequired();
								// The console builds build all games
								CISTasks[( int )CISTaskTypes.SonyPS3].CompileRequired();
								CISTasks[( int )CISTaskTypes.Xbox360].CompileRequired();
								continue;
							}
							else
							{
								// Build everything!
								foreach( CISTask Task in CISTasks )
								{
									Task.CompileRequired();
								}
								continue;
							}
						}
						else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Tools/" ) )
						{
							if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/Tools/UnrealSwarm/Agent/" ) )
							{
								// Need to build all PC stuff if the agent changes because of the tight integration
								CISTasks[( int )CISTaskTypes.Example].CompileRequired();
								CISTasks[( int )CISTaskTypes.Mobile].CompileRequired();
								CISTasks[( int )CISTaskTypes.Gear].CompileRequired();
								CISTasks[( int )CISTaskTypes.Nano].CompileRequired();
								CISTasks[( int )CISTaskTypes.NanoMobile].CompileRequired();
								CISTasks[( int )CISTaskTypes.Sword].CompileRequired();
								CISTasks[( int )CISTaskTypes.UDK].CompileRequired();
							}
							else
							{
								CISTasks[( int )CISTaskTypes.Tools].CompileRequired();
							}
							continue;
						}
						else if( FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Engine" ) ||
								 FileName.StartsWith( "    ... //depot/" + Builder.BranchDef.Branch + "/Development/External" ) )
						{
							// Build everything!
							foreach( CISTask Task in CISTasks )
							{
								Task.CompileRequired();
							}
							continue;
						}
						else
						{
							// Ignore for now, CIS can't do anything with it...
						}
					}

					// Spawn needed jobs
					string BaseCommandString = "INSERT INTO Jobs ( Name, Command, Platform, Game, Config, ScriptConfig, Language, Define, Parameter, Branch, LockPDK, LockXDK, Label, Machine, BuildLogID, PrimaryBuild, Active, Complete, Succeeded, Suppressed, Killing, SpawnTime ) VALUES ";
					foreach( CISTask Task in CISTasks )
					{
						Task.SpawnJobs( this, Builder, DB, BaseCommandString, CL.Number, JobSpawnTime, UpdateLatestProcessedChangelist );
					}
				}
			}
			else
			{
				Builder.Write( "None of the new changes require CIS jobs!" );
			}

			if( UpdateLatestProcessedChangelist )
			{
				// Update the variables table with the last processed CL
				DB.SetIntForBranch( Builder.BranchDef.Branch, "LastAttemptedOverall", CISLastChangelistToProcess );
			}
		}
	}

	public partial class BuilderDB
	{
		public bool UpdateLastGoodAndFailedValues( string JobBranchName, string JobNameSuffix, out int LastGoodChangelist, out int LastFailedChangelist, out bool SuccessChanged, out int SuccessChanger, out int BuildLogID )
		{
			// Construct the query we'll use to get the changes of interest
			int OldLastChangelistProcessed = GetIntForBranch( JobBranchName, "LastAttemptedOverall" );

			int OldLastAttemptedChangelist = GetIntForBranch( JobBranchName, "LastAttempted" + JobNameSuffix );

			// CIS not being run for this type
			if( OldLastAttemptedChangelist < 0 )
			{
				LastGoodChangelist = -1;
				LastFailedChangelist = -1;
				SuccessChanged = false;
				SuccessChanger = -1;
				BuildLogID = 0;
				return ( true );
			}

			int OldLastGoodChangelist = GetIntForBranch( JobBranchName, "LastGood" + JobNameSuffix );
			int OldLastFailedChangelist = GetIntForBranch( JobBranchName, "LastFail" + JobNameSuffix );

			bool BuildIsGood = true;
			int FirstChangelist = OldLastGoodChangelist;
			if( OldLastGoodChangelist < OldLastFailedChangelist )
			{
				BuildIsGood = false;
				FirstChangelist = OldLastFailedChangelist;
			}

			// Seed with the old/current/sane values, assuming we'll update them below, if needed
			LastGoodChangelist = OldLastGoodChangelist;
			LastFailedChangelist = OldLastFailedChangelist;
			SuccessChanged = false;
			SuccessChanger = -1;
			BuildLogID = 0;

			// Display the current state of the build before checking the database for updates
			Parent.Log( "Original " + JobNameSuffix + " CIS Monitor values", Color.Green );
			Parent.Log( "    Old LastGood        = " + OldLastGoodChangelist.ToString(), Color.Green );
			Parent.Log( "    Old LastFailed      = " + OldLastFailedChangelist.ToString(), Color.Green );
			Parent.Log( "", Color.Green );

			// Issue the query and collect the results
			long JobExtendedTimeoutTicks = DateTime.UtcNow.AddHours( -4.0 ).Ticks;
			string CISJobsQuery = "SELECT Label, Complete, Succeeded, BuildLogID FROM Jobs " +
									"WHERE ( PrimaryBuild = 0 ) AND ( Killing = 0 ) " +
									"AND ( Label > '" + FirstChangelist.ToString() + "' ) " +
									"AND ( Branch = '" + JobBranchName + "' ) " +
									"AND ( Name = 'CIS Code Builder (" + JobNameSuffix + ")' ) " +
									"AND ( SpawnTime > " + JobExtendedTimeoutTicks.ToString() + " ) ORDER BY Label";
			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.GetLastGoodAndFailedValues", CISJobsQuery, Connection );

			if( DataReader == null )
			{
				// Badness - just say the build is good as we can't access the db
				return ( true );
			}

			// Evaluate the results and update the DB
			bool JobsInFlight = false;
			while( DataReader.Read() )
			{
				int CurrentJobChangelist = -1;
				if( !Int32.TryParse( DataReader.GetString( 0 ), out CurrentJobChangelist ) )
				{
					CurrentJobChangelist = -1;
				}
				bool CurrentJobComplete = DataReader.GetBoolean( 1 );
				bool CurrentJobSuccess = DataReader.GetBoolean( 2 );
				int CurrentBuildLogID = DataReader.GetInt32( 3 );

				// As soon as we hit an incomplete job, stop evaluating more jobs to maintain order
				if( !CurrentJobComplete )
				{
					// Propagate the last known state all the way up to just prior to the next job
					// to complete, which is like implicitly running jobs, with the same results,
					// for all those changes
					if( BuildIsGood )
					{
						LastGoodChangelist = CurrentJobChangelist - 1;
					}
					else
					{
						LastFailedChangelist = CurrentJobChangelist - 1;
					}

					JobsInFlight = true;
					break;
				}

				// Determine if the state of the build has changed, while updating the Last* values
				if( BuildIsGood )
				{
					if( CurrentJobSuccess )
					{
						// No change, just update the last good changelist
						LastGoodChangelist = CurrentJobChangelist;
					}
					else
					{
						// Build has started to fail, update as appropriate
						BuildIsGood = false;
						SuccessChanged = !SuccessChanged;
						SuccessChanger = CurrentJobChangelist;
						BuildLogID = CurrentBuildLogID;
						LastFailedChangelist = CurrentJobChangelist;
					}
				}
				else
				{
					if( CurrentJobSuccess )
					{
						// Build has started succeeding, update as appropriate
						BuildIsGood = true;
						SuccessChanged = !SuccessChanged;
						SuccessChanger = CurrentJobChangelist;
						BuildLogID = CurrentBuildLogID;
						LastGoodChangelist = CurrentJobChangelist;
					}
					else
					{
						// No change, just update the last failed changelist
						LastFailedChangelist = CurrentJobChangelist;
					}
				}
			}

			DataReader.Close();

			// If there are no jobs in flight, promote to the last overall attempted
			if( !JobsInFlight )
			{
				if( BuildIsGood )
				{
					LastGoodChangelist = OldLastChangelistProcessed;
				}
				else
				{
					LastFailedChangelist = OldLastChangelistProcessed;
				}
			}

			// If CIS was disabled, fix up any variables that may have been lost
			if( LastGoodChangelist < 0 )
			{
				LastGoodChangelist = LastFailedChangelist - 1;
			}

			if( LastFailedChangelist < 0 )
			{
				LastFailedChangelist = LastGoodChangelist - 1;
			}

			Parent.Log( "Updated " + JobNameSuffix + " CIS Monitor values", Color.Green );
			Parent.Log( "    New LastGood        = " + LastGoodChangelist.ToString(), Color.Green );
			Parent.Log( "    New LastFailed      = " + LastFailedChangelist.ToString(), Color.Green );
			Parent.Log( "    New Changer         = " + SuccessChanger.ToString(), Color.Green );
			Parent.Log( "    Build state         = " + BuildIsGood.ToString(), Color.Green );
			Parent.Log( "    Build state changed = " + SuccessChanged.ToString(), Color.Green );
			Parent.Log( "", Color.Green );

			// Post the updated results
			SetIntForBranch( JobBranchName, "LastGood" + JobNameSuffix, LastGoodChangelist );
			SetIntForBranch( JobBranchName, "LastFail" + JobNameSuffix, LastFailedChangelist );

			return ( BuildIsGood );
		}
	}
}