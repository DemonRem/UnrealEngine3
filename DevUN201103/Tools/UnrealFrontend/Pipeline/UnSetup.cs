using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace UnrealFrontend.Pipeline
{
	public class UnSetup : Pipeline.Step
	{
		public UnSetup()
		{
			this.ShouldSkipThisStep = true;
		}

		public override bool SupportsClean { get { return false; } }

		public override string StepName { get { return "Package Game"; } }

		public override String StepNameToolTip { get { return "Package Game"; } }

		public override bool SupportsReboot { get { return false; } }

		public override String ExecuteDesc { get { return "Package Game"; } }

		public override bool Execute(IProcessManager ProcessManager, Profile InProfile)
		{
			String CWD = Environment.CurrentDirectory.Substring(0, Environment.CurrentDirectory.Length - "\\Binaries".Length);
			if (CWD.EndsWith(":"))
			{
				CWD += "\\";
			}

			StringBuilder CommandLine = new StringBuilder();

			// Step 1: Configure mod
			CommandLine.Append("/GameSetup");
			bool bSuccess = ProcessManager.StartProcess("UnSetup.exe", CommandLine.ToString(), CWD, InProfile.TargetPlatform);
			if(bSuccess)
			{
				bSuccess = ProcessManager.WaitForActiveProcessToComplete();
			}

			if(bSuccess)
			{
				// Step 2: Create mod manifest
				CommandLine = new StringBuilder();
				CommandLine.Append("-GameCreateManifest");
				bSuccess = ProcessManager.StartProcess("UnSetup.exe", CommandLine.ToString(), CWD, InProfile.TargetPlatform);
				if (bSuccess)
				{
					bSuccess = ProcessManager.WaitForActiveProcessToComplete();
				}
			}

			if(bSuccess)
			{
				// Step 3: Build mod installer 
				CommandLine = new StringBuilder();
				CommandLine.Append("-BuildGameInstaller");
				bSuccess = ProcessManager.StartProcess("UnSetup.exe", CommandLine.ToString(), CWD, InProfile.TargetPlatform);
				if (bSuccess)
				{
					bSuccess = ProcessManager.WaitForActiveProcessToComplete();
				}
			}

			if(bSuccess)
			{
				// Step 4: Package game 
				CommandLine = new StringBuilder();
				CommandLine.Append("-Package");
				bSuccess = ProcessManager.StartProcess("UnSetup.exe", CommandLine.ToString(), CWD, InProfile.TargetPlatform);
				if (bSuccess)
				{
					bSuccess = ProcessManager.WaitForActiveProcessToComplete();
				}
			}

			return bSuccess;
		}

		public override bool CleanAndExecute(IProcessManager ProcessManager, Profile InProfile)
		{
			throw new NotImplementedException();
		}


	}
}
