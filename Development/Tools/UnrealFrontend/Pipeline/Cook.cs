/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Input;

namespace UnrealFrontend.Pipeline
{
	public class Cook : Pipeline.CommandletStep
	{
		private enum ECookOptions
		{
			None,
			FullRecook,
			INIsOnly,
		}

		#region Pipeline.Step
		public override bool Execute(IProcessManager ProcessManager, Profile InProfile)
		{
			return Execute_Internal(ProcessManager, InProfile, ECookOptions.None);
		}

		public override bool CleanAndExecute(IProcessManager ProcessManager, Profile InProfile)
		{
			return Execute_Internal(ProcessManager, InProfile, ECookOptions.FullRecook);
		}

		public override bool SupportsClean { get { return true; } }

		public override String StepName { get { return "Cook"; } }

		public override String StepNameToolTip { get { return "Cook Packages. (F5)"; } }

		public override bool SupportsReboot { get { return false; } }

		public override String ExecuteDesc { get { return "Cook Packages"; } }

		public override String CleanAndExecuteDesc { get { return "Clean and Full Recook"; } }

		public override Key KeyBinding { get { return Key.F5; } }

		#endregion

		public bool ExecuteINIsOnly( IProcessManager ProcessManager, Profile InProfile )
		{
			return Execute_Internal(ProcessManager, InProfile, ECookOptions.INIsOnly);
		}

		private static bool Execute_Internal(IProcessManager ProcessManager, Profile InProfile, ECookOptions CookingOptions)
		{
			ConsoleInterface.Platform CurPlatform = InProfile.TargetPlatform;

			if (CurPlatform == null)
			{
				return false;
			}

			// handle files (TextureFileCache especially) being open on PC when trying to cook
			if (CurPlatform.Type == ConsoleInterface.PlatformType.PS3 && CookingOptions != ECookOptions.INIsOnly)
			{
				// Disconnect any running PS3s to close all file handles.
				foreach (Target SomeTarget in InProfile.TargetsList.Targets)
				{
					if (SomeTarget.ShouldUseTarget)
					{
						SomeTarget.TheTarget.Disconnect();
					}
				}
			}

			
			// Start the cook
			bool bSuccess = false;
			bSuccess = ProcessManager.StartProcess(
					CommandletStep.GetExecutablePath(InProfile, true, false),
					GetCookingCommandLine(InProfile, CookingOptions),
					"",
					InProfile.TargetPlatform
				);

			if ((InProfile.TargetPlatformType == ConsoleInterface.PlatformType.PS3 ||
					InProfile.TargetPlatformType == ConsoleInterface.PlatformType.NGP)
				 && bSuccess)
			{
				// On PS3 and NGP we want to create the TOC right away because there is no Sync step.
				bSuccess =
					ProcessManager.WaitForActiveProcessToComplete() &&
					Pipeline.Sync.RunCookerSync(ProcessManager, InProfile, false, false);
			}

			return bSuccess;
		}


		private static string GetCookingCommandLine(Profile InProfile, ECookOptions Options)
		{
			// Base command
			string CommandLine = "CookPackages -platform=" + InProfile.TargetPlatform.Name;

			if (Options == ECookOptions.INIsOnly)
			{
				CommandLine += " -inisOnly";
			}
			else
			{
				if (InProfile.Cooking_MapsToCook.Count > 0)
				{
					// Add in map name
					CommandLine += " " + InProfile.Cooking_MapsToCookAsString.Trim();
				}
			}

			if ( InProfile.IsCookDLCProfile )
			{
				ConsoleInterface.Platform CurPlateform = InProfile.TargetPlatform;

				CommandLine += " -user";

				if (CurPlateform.Type == ConsoleInterface.PlatformType.Xbox360 || CurPlateform.Type == ConsoleInterface.PlatformType.PS3)
				{
					CommandLine += " -usermodname=" + InProfile.DLC_Name;
				}
			}

			// Add in the final release option if necessary
			switch (InProfile.ScriptConfiguration)
			{
				case Profile.Configuration.DebugScript:
					CommandLine += " -debug";
					break;
				case Profile.Configuration.FinalReleaseScript:
					CommandLine += " -final_release";
					break;
			}

			// Add in the final release option if necessary
			if (Options == ECookOptions.FullRecook)
			{
				CommandLine += " -full";
			}

			// Get all languages that need to be cooked
			String[] Languages = GetLanguagesToCookAndSync(InProfile);

			// INT is always cooked.
			String LanguageCookString = "INT";

			foreach( String Language in Languages )
			{
				if( Language != "INT")
				{
					// Add the language if its not INT.  INT is already added to the string
					LanguageCookString += "+"+Language ;
				}
			}

			//// Always add in the language we cook for
			CommandLine += " -multilanguagecook=" + LanguageCookString;
			
			{
				String TrimmedAdditionalOptions = InProfile.Cooking_AdditionalOptions.Trim();
				if (TrimmedAdditionalOptions.Length > 0)
				{
					CommandLine += " " + TrimmedAdditionalOptions;
				}
			}

			return CommandLine;
		}
	}
}
