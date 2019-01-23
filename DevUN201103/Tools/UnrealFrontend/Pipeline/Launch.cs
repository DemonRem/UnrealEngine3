/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Windows.Input;

using Color = System.Drawing.Color;

namespace UnrealFrontend.Pipeline
{
	public enum ECurrentConfig
	{
		Global,
		SP,
		MP,
	};

	public class Launch : Pipeline.CommandletStep
	{
		#region Pipeline.Step

		public override bool SupportsClean { get { return false; } }

		public override String StepName { get { return "Launch"; } }

		public override String StepNameToolTip { get { return "Launch Game. (F7)"; } }

		public override bool SupportsReboot { get { return false; } }

		public override String ExecuteDesc { get { return "Launch"; } }

		public override Key KeyBinding { get { return Key.F7; } }

		public override bool Execute(IProcessManager ProcessManager, Profile InProfile)
		{
			string GameOptions;
			string EngineOptions;
			Pipeline.Sync.GetSPURLOptions(InProfile, out GameOptions, out EngineOptions);

			// run on PC or console passing additional engine (-) options
			LaunchApp(InProfile, GameOptions, EngineOptions, Pipeline.ECurrentConfig.SP, false);
			return true;
		}

		public override bool CleanAndExecute(IProcessManager ProcessManager, Profile InProfile)
		{
			Session.Current.SessionLog.AddLine(System.Drawing.Color.DarkMagenta, "----> CANNOT CLEAN A LAUNCH");
			return true;
		}

		#endregion




		/// <summary>
		/// This runs the PC game for non-commandlets (editor, game, etc)
		/// </summary>
		/// <param name="InitialCmdLine"></param>
		/// <param name="PostCmdLine"></param>
		/// <param name="ConfigType"></param>
		public static void LaunchApp(Profile InProfile, string InitialCmdLine, string PostCmdLine, Pipeline.ECurrentConfig ConfigType, bool bForcePC)
		{
			// put together the final commandline (and generate exec file
			string CmdLine = CommandletStep.GetFinalURL(InProfile, InitialCmdLine, PostCmdLine, true);

			if (bForcePC || InProfile.TargetPlatformType == ConsoleInterface.PlatformType.PC || InProfile.TargetPlatformType == ConsoleInterface.PlatformType.PCConsole || InProfile.TargetPlatformType == ConsoleInterface.PlatformType.PCServer )
			{
				System.Diagnostics.Process NewProcess = ExecuteProgram(UnrealFrontend.Pipeline.CommandletStep.GetExecutablePath(InProfile, false, bForcePC), CmdLine, false);
				if (NewProcess != null)
				{
					//Processes.Add(NewProcess);
				}
				else
				{
					System.Windows.MessageBox.Show("Failed to launch game executable (" + UnrealFrontend.Pipeline.CommandletStep.GetExecutablePath(InProfile, false, bForcePC) + ")", "Failed to launch", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
				}
			}
			else
			{
				CheckedReboot(InProfile, true, CmdLine);
			}
		}

		/// <summary>
		/// Reboot and optionally run the game on a console
		/// </summary>
		/// <param name="bShouldRunGame"></param>
		/// <param name="CommandLine"></param>
		public static void CheckedReboot(Profile InProfile, bool bShouldRunGame, string CommandLine)
		{
			SessionLog Log = Session.Current.SessionLog;

			if (bShouldRunGame)
			{
				Log.WriteCommandletEvent(Color.Green, string.Format("Launching {0} with command line \'{1}\'", InProfile.SelectedGameName, CommandLine));
			}
			else
			{
				Log.WriteCommandletEvent(Color.Green, "REBOOTING");
			}

			// Debug/Release etc
			string ConfigStr = GetConsoleConfigurationString( InProfile );

			bool bHadValidTarget = false;
			foreach (Target SomeTarget in InProfile.TargetsList.Targets)
			{
				if (SomeTarget.ShouldUseTarget)
				{
					bHadValidTarget = true;
					string TargetName = SomeTarget.TheTarget.Name;

					if (bShouldRunGame)
					{
						string BaseDir = InProfile.Targets_ConsoleBaseDir;

						if (BaseDir.EndsWith("\\"))
						{
							BaseDir = BaseDir.Substring(0, BaseDir.Length - 1);
						}

						bool bIsConnected = SomeTarget.TheTarget.IsConnected;

						// need to connect to keep track of console state, this is needed for properly launching UC
						if (!bIsConnected)
						{
							if (!SomeTarget.TheTarget.Connect())
							{
								Log.AddLine(Color.Red, "Failed connection attempt with target \'" + TargetName + "\'!");
								if (SomeTarget.TheTarget.ParentPlatform.Type != ConsoleInterface.PlatformType.IPhone)
								{
									continue;
								}
							}
						}

						if (SomeTarget.TheTarget.RebootAndRun(ConfigStr, BaseDir, InProfile.SelectedGameName, CommandLine, false))
						{
							Log.AddLine(null, "Target \'" + TargetName + "\' successfully launched game!");
							Session.Current.LaunchUnrealConsole(SomeTarget.TheTarget, InProfile.Launch_ClearUCWindow);
						}
						else
						{
							Log.AddLine(Color.Red, "Target \'" + TargetName + "\' failed to launch game!");
						}
					}
					else
					{
						if (SomeTarget.TheTarget.Reboot())
						{
							Log.AddLine(null, "Target \'" + TargetName + "\' successfully rebooted!");
						}
						else
						{
							Log.AddLine(Color.Red, "Target \'" + TargetName + "\' failed to reboot!");
						}
					}
				}
			}

			if (!bHadValidTarget)
			{
				Log.AddLine(Color.Red, "No targets specified! Aborting...");
			}
		}

		/// <summary>
		/// Start up the given executable
		/// </summary>
		/// <param name="Executable">The string name of the executable to run.</param>
		/// <param name="CommandLIne">Any command line parameters to pass to the program.</param>
		/// <param name="bOutputToTextBox">Specifying true will output </param>
		/// <returns>The running process if successful, null on failure</returns>
		private static System.Diagnostics.Process ExecuteProgram(string Executable, string CommandLine, bool bOutputToTextBox)
		{
			ProcessStartInfo StartInfo = new ProcessStartInfo();
			// Prepare a ProcessStart structure 
			StartInfo.FileName = Executable;
			StartInfo.WorkingDirectory = System.IO.Directory.GetCurrentDirectory();
			StartInfo.Arguments = CommandLine;

			System.Diagnostics.Process NewProcess = null;
			if (bOutputToTextBox)
			{
				// Redirect the output.
				StartInfo.UseShellExecute = false;
				StartInfo.RedirectStandardOutput = false;
				StartInfo.RedirectStandardError = false;
				StartInfo.CreateNoWindow = true;

				Session.Current.SessionLog.AddLine(Color.OrangeRed, "Running: " + Executable + " " + CommandLine);
			}

			// Spawn the process
			// Try to start the process, handling thrown exceptions as a failure.
			try
			{
				NewProcess = System.Diagnostics.Process.Start(StartInfo);
			}
			catch
			{
				NewProcess = null;
			}

			return NewProcess;
		}

		/// <summary>
		/// Converts a config enumeration into its string representation.
		/// </summary>
		/// <param name="Config">The enumeration to be converted.</param>
		/// <returns>The appropriate string representation of the supplied enumeration.</returns>
		private static string GetConsoleConfigurationString(Profile InProfile)
		{
			string RetVal = string.Empty;

			switch( InProfile.LaunchConfiguration )
			{
				case Profile.Configuration.Debug_32:
				case Profile.Configuration.Debug_64:
					{
						RetVal = "Debug";
						break;
					}
				case Profile.Configuration.Release_32:
				case Profile.Configuration.Release_64:
					{
						RetVal = "Release";
						break;
					}
				case Profile.Configuration.Shipping_32:
				case Profile.Configuration.Shipping_64:
					{
						RetVal = "ReleaseLTCG";
						break;
					}
				case Profile.Configuration.ShippingDebug_32:
				case Profile.Configuration.ShippingDebug_64:
					{
						RetVal = "ReleaseLTCG-DebugConsole";
						break;
					}
			}

			if( InProfile.UseMProfExe )
			{
				RetVal += ".MProf";
			}

			return RetVal;
		}

	}
}
