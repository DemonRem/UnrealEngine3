/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace UnrealFrontend.Pipeline
{
	public abstract class CommandletStep : Pipeline.Step
	{

		/// <summary>
		/// Calculate the name of the PC executable for commandlets and PC game
		/// </summary>
		/// <param name="bCommandlet">True if this is a path for a commandlet.</param>
		/// <returns></returns>
		public static string GetExecutablePath(Profile InProfile, bool bCommandlet, bool bPromoteTox64)
		{
			// Figure out executable path.
			Profile.Configuration SelectedConfig = bCommandlet ? InProfile.CommandletConfiguration : InProfile.LaunchConfiguration;

			string Executable = InProfile.SelectedGameName;

			if( InProfile.TargetPlatformType == ConsoleInterface.PlatformType.PCServer && !bCommandlet)
			{
				Executable += "Server";
			}

			if( InProfile.UseMProfExe && !bCommandlet )
			{
				Executable += ".MProf";
			}

			switch (SelectedConfig)
			{
				case Profile.Configuration.Debug_32:
					Executable = "Win32\\DEBUG-" + Executable + ".exe";
					break;

				case Profile.Configuration.Release_32:
					Executable = "Win32\\" + Executable + ".exe";
					break;

				case Profile.Configuration.ShippingDebug_32:
				case Profile.Configuration.Shipping_32:
					Executable = "Win32\\ShippingPC-" + Executable + ".exe";
					break;

				case Profile.Configuration.Debug_64:
					Executable = "Win64\\DEBUG-" + Executable + ".exe";
					break;

				case Profile.Configuration.Release_64:
					Executable = "Win64\\" + Executable + ".exe";
					break;

				case Profile.Configuration.ShippingDebug_64:
				case Profile.Configuration.Shipping_64:
					Executable = "Win64\\ShippingPC-" + Executable + ".exe";
					break;
			}

			if (Session.Current.UDKMode != EUDKMode.None)
			{
				if (Executable.EndsWith("UDKGame.exe"))
				{
					Executable = "Win32\\UDK.exe";
					if (bPromoteTox64 && Win32Helper_UFE.Is64Bit())
					{
						Executable = "Win64\\UDK.exe";
					}
				}
				else if (Executable.EndsWith("MobileGame.exe"))
				{
					Executable = "Win32\\UDKMobile.exe";
					if (bPromoteTox64 && Win32Helper_UFE.Is64Bit())
					{
						Executable = "Win64\\UDKMobile.exe";
					}
				}
			}

			return (Executable);
		}




		/// <summary>
		/// Retrieves the list of languages to use for cooking and syncing.
		/// </summary>
		/// <returns>An array of languages to use to for cooking and syncing.</returns>
		public static string[] GetLanguagesToCookAndSync(Profile InProfile)
		{
			List<string> Languages = new List<string>();

			foreach (LangOption LanguageOption in InProfile.Cooking_LanguagesToCookAndSync.Content)
			{
				if (LanguageOption.IsEnabled)
				{
					Languages.Add(LanguageOption.Name);
				}
			}

			// Always cook INT if nothing was checked
			if (Languages.Count == 0)
			{
				Languages.Add("INT");
			}

			return Languages.ToArray();
		}


		/// <summary>
		/// Creates the exec temp file.
		/// </summary>
		public static void CreateTempExec(Profile InProfile)
		{
			string TmpExecLocation;

			TmpExecLocation = "UnrealFrontend_TmpExec.txt";

			if (InProfile.UseExecCommands)
			{
				System.IO.File.WriteAllLines(TmpExecLocation, InProfile.Launch_ExecCommands.Split(Environment.NewLine[0]));
			}
			
		}

		/// <summary>
		/// Generate a final URL to pass to the game
		/// </summary>
		/// <param name="GameOptions">Game type options (? options)</param>
		/// <param name="PostCmdLine">Engine type options (- options)</param>
		/// <param name="bCreateExecFile"></param>
		/// <returns></returns>
		protected static string GetFinalURL(Profile InProfile, string GameOptions, string EngineOptions, bool bCreateExecFile)
		{
			// build the commandline
			StringBuilder CmdLine = new StringBuilder();

			if (GameOptions != null && GameOptions.Length > 0)
			{
				CmdLine.Append(GameOptions);
			}

			GameOptions = BuildGameCommandLine(InProfile);

			if (GameOptions != null && GameOptions.Length > 0)
			{
				CmdLine.Append(' ');
				CmdLine.Append(GameOptions);
			}

			if (EngineOptions != null && EngineOptions.Length > 0)
			{
				CmdLine.Append(' ');
				CmdLine.Append(EngineOptions);
			}

			CmdLine.Append(" -Exec=UnrealFrontend_TmpExec.txt");

			// final pass for execs
			if (bCreateExecFile)
			{
				CreateTempExec(InProfile);
			}

			// if the DefaultMapString is in the URL, replace it with nothing (to make the game use the default map)
			CmdLine = CmdLine.Replace(DefaultMapString, "");

			return CmdLine.ToString();
		}


		/// <summary>
		/// Builds the game execution command line.
		/// </summary>
		/// <returns>The command line that will be used to execute the game with the current options and configuration.</returns>
		public static string BuildGameCommandLine(Profile InProfile)
		{
			StringBuilder Bldr = new StringBuilder();

			if (InProfile.Launch_NoVSync)
			{
				Bldr.Append("-novsync ");
			}

			Bldr.Append("-norc ");

			if (InProfile.Launch_CaptureFPSChartInfo)
			{
				Bldr.Append("-gCFPSCI ");
			}

			return Bldr.ToString().Trim();
		}

		// the string to show in the Map to Play box when no map is manually entered - this implies using the default map in the game's .ini file
		private static readonly string DefaultMapString = "<Default>";



	}
}
