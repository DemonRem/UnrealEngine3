/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace UnrealFrontend.Pipeline
{
	public class SyncAndroid : Pipeline.CommandletStep
	{
		#region Pipeline.Step
		
		public override bool SupportsClean { get { return false; } }

		public override string StepName { get { return "Sync Android"; } }

		public override String StepNameToolTip { get { return "Sync to Android Device"; } }

		public override bool SupportsReboot { get { return false; } }

		public override String ExecuteDesc { get { return "Sync to Android Device"; } }

		public override bool Execute(IProcessManager ProcessManager, Profile InProfile)
		{
			ConsoleInterface.TOCSettings BuildSettings = Pipeline.Sync.CreateTOCSettings(InProfile, false);

			// Android doesn't support syncing or launching (yet). But it does need the UE3CommandLine.txt file to be generated.
			if (!Pipeline.Sync.UpdateMobileCommandlineFile(InProfile))
			{
				return false;
			}
			return true;
		}

		public override bool CleanAndExecute(IProcessManager ProcessManager, Profile InProfile)
		{
			throw new NotImplementedException();
		}

		#endregion
	}
}
