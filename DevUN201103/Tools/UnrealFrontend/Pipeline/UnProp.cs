using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace UnrealFrontend.Pipeline
{
	public class UnProp : Pipeline.Step
	{
		public override bool SupportsClean { get { return false; } }

		public override string StepName { get { return "Prop"; } }

		public override String StepNameToolTip { get { return "Distribute a build to UnrealProp."; } }

		public override bool SupportsReboot { get { return false; } }

		public override String ExecuteDesc { get { return "Send to UnProp"; } }

		public UnProp()
		{
			this.ShouldSkipThisStep = true;
		}

		public override bool Execute(IProcessManager ProcessManager, Profile InProfile)
		{
			string PlatformName = InProfile.TargetPlatformType.ToString();

			if (InProfile.TargetPlatformType == ConsoleInterface.PlatformType.Xbox360)
			{
				PlatformName = "Xenon";
			}

			string TimeStampString = DateTime.Now.ToString("yyyy-MM-dd_HH.mm");

			DirectoryInfo Branch = Directory.GetParent(Directory.GetCurrentDirectory());
			string DestPath = string.Format("\\\\prop-06\\Builds\\{0}User\\{1}\\{0}_{1}_[{2}]_[{3}]", InProfile.SelectedGameName.Replace("Game", ""), PlatformName, TimeStampString, Environment.UserName.ToUpper());

			List<string> DestPathList = new List<string>();
			DestPathList.Add(DestPath);

			ConsoleInterface.TOCSettings BuildSettings = Pipeline.Sync.CreateTOCSettings(InProfile, false);

			// we don't want to sync to a console so clear targets
			BuildSettings.TargetsToSync.Clear();
			BuildSettings.SleepDelay = 25;
			BuildSettings.TargetBaseDirectory = BuildSettings.TargetBaseDirectory.Replace(" ", "_");
			BuildSettings.DestinationPaths = DestPathList;

			StringBuilder CommandLine = new StringBuilder();

			string TagSet = "CompleteBuild";

			Pipeline.Sync.GenerateCookerSyncCommandLine(InProfile, CommandLine, BuildSettings, TagSet, true, this.RebootBeforeStep);

			return ProcessManager.StartProcess(
				"CookerSync.exe",
				CommandLine.ToString(),
				"",
				InProfile.TargetPlatform);

		}

		public override bool CleanAndExecute(IProcessManager ProcessManager, Profile InProfile)
		{
			return true;
		}
	}
}
