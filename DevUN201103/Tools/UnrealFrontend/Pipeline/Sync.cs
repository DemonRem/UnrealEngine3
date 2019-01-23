/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Input;

using Color = System.Drawing.Color;

namespace UnrealFrontend.Pipeline
{
	public class RebootAndSync : Pipeline.Sync
	{
		public override bool SupportsReboot { get { return true; } }
	}

	public class Sync : Pipeline.CommandletStep
	{
		#region Pipeline.Step

		public override bool SupportsClean { get { return true; } }

		public override String StepName { get { return "Sync"; } }

		public override String StepNameToolTip { get { return "Sync cooked data. (F6)"; } }

		public override String ExecuteDesc { get { return "Sync Cooked Data"; } }

		public override bool SupportsReboot { get { return false; } }

		public override Key KeyBinding { get { return Key.F6; } }

		public override bool Execute(IProcessManager ProcessManager, Profile InProfile)
		{
			return RunCookerSync(ProcessManager, InProfile, this.SupportsReboot && this.RebootBeforeStep, false);
		}

		public override bool CleanAndExecute(IProcessManager ProcessManager, Profile InProfile)
		{
			return RunCookerSync(ProcessManager, InProfile, this.SupportsReboot && this.RebootBeforeStep, true);
		}

		#endregion

		public static bool RunCookerSync( IProcessManager ProcessManager, Profile InProfile, bool bShouldReboot, bool bForceSync )
		{
			ConsoleInterface.Platform CurPlat = InProfile.TargetPlatform;

			ConsoleInterface.TOCSettings BuildSettings = CreateTOCSettings(InProfile, bForceSync);

			StringBuilder CommandLine = new StringBuilder();

			string TagSet = InProfile.SupportsPDBCopy && InProfile.Sync_CopyDebugInfo ? "ConsoleSync" : "ConsoleSyncProgrammer";

			GenerateCookerSyncCommandLine(InProfile, CommandLine, BuildSettings, TagSet, false, bShouldReboot);

			return ProcessManager.StartProcess(
				"CookerSync.exe",
				CommandLine.ToString(),
				"",
				InProfile.TargetPlatform);
		}


		/// <summary>
		/// Generates the TOC for the current platform.
		/// </summary>
		/// <returns>The TOC for the current platform.</returns>
		internal static ConsoleInterface.TOCSettings CreateTOCSettings(Profile InProfile, bool bForceSync)
		{
			ConsoleInterface.TOCSettings BuildSettings = new ConsoleInterface.TOCSettings(Session.Current.SessionLog.ConsoleOutputHandler);

			foreach (Target CurTarget in InProfile.TargetsList.Targets)
			{
				if (CurTarget.ShouldUseTarget)
				{
					BuildSettings.TargetsToSync.Add(CurTarget.Name);
				}
			}

			BuildSettings.GameName = InProfile.SelectedGameName;
			BuildSettings.TargetBaseDirectory = InProfile.Targets_ConsoleBaseDir;
			BuildSettings.Languages = GetLanguagesToCookAndSync(InProfile);
			BuildSettings.Force = BuildSettings.Force || bForceSync;
			BuildSettings.NoDest = BuildSettings.TargetsToSync.Count == 0;

			return BuildSettings;
		}


		/// <summary>
		/// Writes out the commandline to a text file usable on mobile devices
		/// </summary>
		internal static bool UpdateMobileCommandlineFile(Profile InProfile)
		{
			SessionLog Log = Session.Current.SessionLog;

			// get some options from UFE
			ConsoleInterface.Platform CurPlat = InProfile.TargetPlatform;
			string PlatformName = CurPlat.Type.ToString();
			string GameOptions, EngineOptions;
			GetSPURLOptions(InProfile, out GameOptions, out EngineOptions);

			// compute where to write it
			string DestinationDirectory = "..\\" + InProfile.SelectedGameName + "\\Cooked" + PlatformName + "\\";

			if (!System.IO.Directory.Exists(DestinationDirectory))
			{
				Log.AddLine(Color.Red, "DIRECTORY DOES NOT EXIST: " + DestinationDirectory);
				Log.AddLine(Color.Red, " ... aborting");
				return (false);
			}

			// generate the commandline
			string GameCommandLine = GetFinalURL(InProfile, GameOptions, EngineOptions, false);

			// write it out
			string Filename = DestinationDirectory + "UE3CommandLine.txt";
			System.IO.File.WriteAllText(Filename, GameCommandLine);

			// if we want to use network file loading, then write out this computer's IP address to a special
			// file that is synced (different from the CommandLine.txt so that we can load the commandline
			// over the network)
			if (InProfile.Mobile_UseNetworkFileLoader)
			{
				// get my IP address
				System.Net.IPAddress[] LocalAddresses = System.Net.Dns.GetHostAddresses("");
				if (LocalAddresses == null)
				{
					// make sure we have a local IP address
					System.Windows.MessageBox.Show("Can't use network file hosting, since no local IP address could be determined. Disabling Networked File Loader.");
					InProfile.Mobile_UseNetworkFileLoader = false;
				}
				else
				{
					string HostFilename = DestinationDirectory + "UE3NetworkFileHost.txt";
					// write out our first IPv4 address
					foreach (System.Net.IPAddress Addr in LocalAddresses)
					{
						if (Addr.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
						{
							System.IO.File.WriteAllText(HostFilename, Addr.ToString());
							break;
						}
					}
				}
			}

			Log.WriteCommandletEvent(Color.Blue, string.Format("Writing game commandline [{0}] to '{1}'", GameCommandLine, Filename));
			return (true);
		}



		/// <summary>
		/// Generates the command line used for starting a cooker sync commandlet.
		/// </summary>
		/// <param name="CommandLine">The StringBuilder being used to generate the commandline.</param>
		/// <param name="PlatSettings">The settings being used to sync.</param>
		/// <param name="TagSet">The tag set to sync.</param>
		public static void GenerateCookerSyncCommandLine(Profile InProfile, StringBuilder CommandLine, ConsoleInterface.TOCSettings BuildSettings, string TagSet, bool EnforceFolderName, bool bRebootBeforeSync)
		{
			ConsoleInterface.Platform CurPlat = InProfile.TargetPlatform;
			StringBuilder Languages = new StringBuilder();

			//NOTE: Due to a workaround for an issue with cooking multiple languages
			// it's possible for INT to show up in the languages list twice.
			// Use this dictionary to prevent that.
			Dictionary<string, string> FinalLanguagesToSync = new Dictionary<string, string>();

			foreach (string CurLang in BuildSettings.Languages)
			{
				if (!FinalLanguagesToSync.ContainsKey(CurLang))
				{
					FinalLanguagesToSync[CurLang] = CurLang;

					Languages.Append(" -r ");
					Languages.Append(CurLang);
				}
			}

			if (EnforceFolderName)
			{
				if (!BuildSettings.TargetBaseDirectory.Contains("UnrealEngine3"))
				{
					System.Windows.MessageBox.Show("Console base directory must have 'UnrealEngine3' in it.", "UnrealProp Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
					return;
				}
			}

			CommandLine.AppendFormat("{0} -p {1} -x {2}{3} -b \"{4}\"", BuildSettings.GameName, CurPlat.Type.ToString(), TagSet, Languages.ToString(), BuildSettings.TargetBaseDirectory);

			if (bRebootBeforeSync)
			{
				CommandLine.Append(" -o");
			}			

			if (BuildSettings.Force)
			{
				CommandLine.Append(" -f");
			}

			if (!BuildSettings.GenerateTOC)
			{
				CommandLine.Append(" -notoc");
			}

			if (BuildSettings.VerifyCopy)
			{
				CommandLine.Append(" -v");
			}

			if (BuildSettings.NoSync)
			{
				CommandLine.Append(" -n");
			}

			if (BuildSettings.NoDest)
			{
				CommandLine.Append(" -nd");
			}

			if (BuildSettings.MergeExistingCRC)
			{
				CommandLine.Append(" -m");
			}

			if (BuildSettings.ComputeCRC)
			{
				CommandLine.Append(" -c");
			}

			CommandLine.AppendFormat(" -s {0}", BuildSettings.SleepDelay.ToString());

			foreach (string Target in BuildSettings.TargetsToSync)
			{
				CommandLine.Append(' ');
				CommandLine.Append(Target);
			}

			foreach (string Target in BuildSettings.DestinationPaths)
			{
				CommandLine.Append(' ');
				CommandLine.Append('\"');
				CommandLine.Append(Target);
				CommandLine.Append('\"');
			}
		}

		/// <summary>
		/// Gets the options for a single player game on the current platform.
		/// </summary>
		/// <param name="GameOptions">Receives the game options URL.</param>
		/// <param name="EngineOptions">Receives the engine options URL.</param>
		public static void GetSPURLOptions(Profile InProfile, out string GameOptions, out string EngineOptions)
		{
			if (InProfile.Launch_UseUrl == 1)
			{
				GameOptions = InProfile.Launch_Url.Trim();
			}
			else 
			{
				// put together with URL options
				GameOptions = (InProfile.LaunchDefaultMap ? "" : InProfile.MapToPlay.Name);

			}

			// just use the extra options to start with
			EngineOptions = InProfile.Launch_ExtraOptions;

			EngineOptions = EngineOptions.Trim();

			if( InProfile.TargetPlatformType == ConsoleInterface.PlatformType.PCServer )
			{
				EngineOptions += "-seekfreeloadingserver";
			}
			else if( InProfile.TargetPlatformType == ConsoleInterface.PlatformType.PCConsole )
			{
				EngineOptions += "-seekfreeloadingpcconsole";
			}
			else if( InProfile.TargetPlatformType == ConsoleInterface.PlatformType.PC )
			{
				EngineOptions += "-seekfreeloading";
			}

			//Resolution Res;
			//if (ComboBox_Platform.Text == "PC" && Resolution.TryParse(ComboBox_Game_Resolution.Text, out Res))
			//{
			//    EngineOptions += string.Format(" -resx={0} -resy={1}", Res.Width, Res.Height);
			//}
		}

	}


}
