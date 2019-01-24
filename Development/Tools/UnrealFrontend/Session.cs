/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using ConsoleInterface;
using Assembly = System.Reflection.Assembly;
using ChannelServices = System.Runtime.Remoting.Channels.ChannelServices;
using Color = System.Drawing.Color;
using Directory = System.IO.Directory;
using File = System.IO.File;
using IpcChannel = System.Runtime.Remoting.Channels.Ipc.IpcChannel;
using Path = System.IO.Path;
using ProcessStartInfo = System.Diagnostics.ProcessStartInfo;
using RemoteUCObject = UnrealConsoleRemoting.RemoteUCObject;



namespace UnrealFrontend
{
	public enum EUDKMode
	{
		None,
		UDK,
	}

	public interface IProcessManager
	{
		bool StartProcess(String ExecutablePath, String ExeArgs, String CWD, Platform InPlatform);
		void StopProcess();
		bool WaitForActiveProcessToComplete();
	}

	public class Session : INotifyPropertyChanged, IProcessManager
	{
		private Session()
		{
		}

		#region Singleton

		public static Session Current { get; private set; }
		public static void Init()
		{
			Current = new Session();
			Current.InitInternal();
		}

		#endregion

		/// Thread where all the actual work is done.
		private WorkerThread Worker {get; set;}

		/// Are we a UDK product?
		public EUDKMode UDKMode { get; private set; }

		/// Info string: changelist, engine version, etc.
		public String VersionString { get { return (UDKMode != EUDKMode.None) ? "" : String.Format("cl {0} v{1}", Versioning.Changelist, Versioning.EngineVersion); } }

		private UnrealControls.EngineVersioning Versioning = new UnrealControls.EngineVersioning();

		/// List of known platforms based on available DLLs.
		public List<ConsoleInterface.PlatformType> KnownPlatformTypes { get; private set; }

		/// Games that were discovered during startup.
		public List<String> KnownGames { get; private set; }

		/// Is RPCUtil.exe is available to us
		public bool IsRPCUtilityFound { get; private set; }

		/// Document for the log window.
		public UnrealControls.OutputWindowDocument OutpuDocument { get { return SessionLog.OutputDocument; } }

		/// The profiles available to the user.
		public System.Collections.ObjectModel.ObservableCollection<Profile> Profiles { get; private set; }

		/// Default profiles serve as templates when the users need guidance.
		private List<Profile> DefaultProfiles { get; set; }

		/// Abstracts queueing up of messages into the UI thread.
		public SessionLog SessionLog { get; private set; }

		/// UFE settings; e.g. last selected profile.
		public Settings Settings { get; set; }
		
		/// Studio-wide UFE settings
		public StudioSettings StudioSettings { get; set; }

		private void InitInternal()
		{
			// Start up the worker thread.
			System.Windows.Threading.Dispatcher UIDispatcher = System.Windows.Application.Current.Dispatcher;
			Worker = new WorkerThread("UFE2MainWorker");
			Worker.PickedUpTask = (TaskType) =>
			{
				DispatchWorkStarted(UIDispatcher, TaskType);
			};
			Worker.WorkQueueEmpty = () =>
			{
				DispatchWorkCompleted(UIDispatcher);
			};
			

			// Bring up the ConsoleInterface; use the worker thread to avoid COM issues.
			Worker.QueueWork( new WorkerThread.VoidTask(delegate(){
				InitConsoleInterface();
			}));
			Worker.Flush();

			String RPCUtilFullpath = Path.GetFullPath(Path.Combine("IPhone", "RPCUtility.exe"));
			this.IsRPCUtilityFound = File.Exists(RPCUtilFullpath);
			

			// Check for UDK usage
			UDKMode = EUDKMode.None;
			if (Versioning.SpecialDefines.Contains("UDK=1"))
			{
				UDKMode = EUDKMode.UDK;
			}

			// Cache a list of all known platforms; this value does not change throughout the session.
			KnownPlatformTypes = new List<ConsoleInterface.PlatformType>();
			foreach (ConsoleInterface.Platform SomePlatform in ConsoleInterface.DLLInterface.Platforms)
			{
				KnownPlatformTypes.Add(SomePlatform.Type);
			}

			// Restrict the available platforms for UDK, UDKM
			if (UDKMode == EUDKMode.UDK)
			{
				KnownPlatformTypes.RemoveAll(SomePlatformType => SomePlatformType != PlatformType.PC && SomePlatformType != PlatformType.IPhone);
			}

			// Add in the known games that have a content folder
			KnownGames = UnrealControls.GameLocator.LocateGames().ConvertAll<String>(SomeGame=>SomeGame+"Game");


			this.SessionLog = new SessionLog();
			this.Profiles = new System.Collections.ObjectModel.ObservableCollection<Profile>();

			this.Settings = new Settings();
			this.StudioSettings = new StudioSettings();
		}

		void InitConsoleInterface()
		{
			// Load in the CookerSync xml file (required only for PS3)
			ConsoleInterface.DLLInterface.LoadCookerSyncManifest();

			// force loading of platform dll's here
			ConsoleInterface.DLLInterface.LoadPlatforms(ConsoleInterface.PlatformType.All);
		}

		void ShutdownConsoleInterface()
		{
			foreach (ConsoleInterface.Platform CurPlatform in ConsoleInterface.DLLInterface.Platforms)
			{
				CurPlatform.Dispose();
			}
		}

		/// The session of UFE is starting; load all the settins, profiles, etc.
		public void OnSessionStarting()
		{
			// Load Studio settings (e.g. per-game Cooker help URLs)
			if ( File.Exists( Settings.StudioSettingsFilename ) )
			{
				this.StudioSettings = UnrealControls.XmlHandler.ReadXml<UnrealFrontend.StudioSettings>(Settings.StudioSettingsFilename);
			}
			this.StudioSettings.AddGamesIfMissing(KnownGames);

			// Load UFE2 settings
			if (File.Exists( Settings.SettingsFilename ))
			{
				this.Settings = UnrealControls.XmlHandler.ReadXml<UnrealFrontend.Settings>(Settings.SettingsFilename);
			}

			PopulateProfiles();

			// Print something so the user is not confused about an empty window.
			SessionLog.AddLine( Color.Purple, String.Format("Unreal Frontend 2 started {0}...", DateTime.Now) );
		}

		/// This session of UFE is shutting down; save out all settings/profiles.
		public void OnSessionClosing()
		{
			// Cancel all work. Shut down the worker thread.
			StopProcess();
			Worker.QueueWork(() =>
			{
				ShutdownConsoleInterface();
			});
			Worker.Flush();
			Worker.BeginShutdown();

			SaveProfiles();

			// Save UFE2 settings
			{
				UnrealControls.XmlHandler.WriteXml(this.Settings, Settings.SettingsFilename, "");
			}

			// Save the studio settings
			{
				UnrealControls.XmlHandler.WriteXml(this.StudioSettings, Settings.StudioSettingsFilename, "");
			}

		}

		private void PopulateProfiles()
		{
			// Load default profiles for each known game
			DefaultProfiles = new List<Profile>();
			foreach (String Game in Session.Current.KnownGames)
			{
				// Find the default profile for game and try to load it
				String DefaultProfilePath = Path.GetFullPath(PathUtils.Combine("..", Game, "Config", "UnrealFrontend.DefaultProfiles"));
				if ( Directory.Exists(DefaultProfilePath) )
				{
					XmlUtils LoadProfileHelper = new XmlUtils();
					DefaultProfiles.AddRange(LoadProfileHelper.ReadProfiles(DefaultProfilePath));
				}
			}

			
			// Load user profiles
			XmlUtils XmlHelper = new XmlUtils();
			List<Profile> LoadedProfiles = XmlHelper.ReadProfiles(Settings.ProfileDirLocation);
			foreach (Profile SomeProfile in LoadedProfiles)
			{
				Session.Current.Profiles.Add(SomeProfile);
			}


			// There are no user profiles, so populate list with defaults.
			if (Session.Current.Profiles.Count == 0)
			{				
				foreach (Profile SomeProfile in DefaultProfiles)
				{
					Session.Current.Profiles.Add( SomeProfile.Clone() );
				}

				if (Session.Current.Profiles.Count == 0)
				{
					Session.Current.Profiles.Add(new Profile("Default Profile", new ProfileData()));
				}
				
			}
		}

		private void SaveProfiles()
		{
			String ProfileLocation = Settings.ProfileDirLocation;

			// Save profiles
			foreach (Profile SomeProfile in Profiles)
			{
				SomeProfile.OnAboutToSave();
				XmlUtils XmlHelper = new XmlUtils();
				XmlHelper.WriteProfile(SomeProfile, SomeProfile.Filename);
			}

			// We never delete profiles until the session is closing.
			// Now is the time to clean up any profiles that are not in our
			// active profile list.
			List<String> FilesInProfileDir = FileUtils.GetFilesInProfilesDirectory(Settings.ProfileDirLocation);
			List<String> FilenamesOfProfilesInMemory = Session.Current.Profiles.ToList().ConvertAll<String>(SomeProfile => Path.Combine(ProfileLocation, SomeProfile.Filename));

			foreach (String SomeProfileOnDisk in FilesInProfileDir)
			{
				// If we have a profile on disk that is not in memory, let's delete it.
				if (null == FilenamesOfProfilesInMemory.Find(SomeProfileInMemory => SomeProfileInMemory.Equals(SomeProfileOnDisk, StringComparison.InvariantCultureIgnoreCase)))
				{
					try
					{
						File.Delete(SomeProfileOnDisk);
					}
					catch (Exception)
					{
					}

				}
			}
		}


		#region IProcessManager

		/// The current commandlet process.
		CommandletProcess ActiveProcess{get; set;}

		/// <summary>
		/// Async start the new commandlet
		/// </summary>
		/// <param name="ExecutablePath">The executable to run</param>
		/// <param name="ExeArgs">Argument to pass to the executable</param>
		/// <param name="CWD">The directory in which to start the exe</param>
		/// <param name="InPlatform">Platform we are targeting</param>
		/// <returns>true if the commandlet started; false if starting the commandlet failed</returns>
		public bool StartProcess( String ExecutablePath, String ExeArgs, String CWD, Platform InPlatform )
		{
			ActiveProcess = new CommandletProcess(ExecutablePath, InPlatform);
			ActiveProcess.Exited += OnProcessExited;
			ActiveProcess.Output += SessionLog.HandleProcessOutput;

			try
			{
				ActiveProcess.Start(ExeArgs.ToString(), CWD);

				SessionLog.WriteCommandletEvent(Color.Green, string.Format("COMMANDLET \'{0} {1}\' STARTED IN '{2}'", Path.GetFileName(ActiveProcess.ExecutablePath), ActiveProcess.CommandLine, CWD));

				return true;
			}
			catch (Exception ex)
			{
				SessionLog.WriteCommandletEvent(Color.Red, string.Format("COMMANDLET \'{0} {1}\' FAILED", Path.GetFileName(ActiveProcess.ExecutablePath), ActiveProcess.CommandLine));
				SessionLog.AddLine(Color.Red, ex.ToString());

				ActiveProcess.Dispose();
				ActiveProcess = null;

				return false;
			}
		}

		/// Terminate the currently active process. Cancel any queued work in the worker thread.
		public void StopProcess()
		{
			Worker.CancelQueuedWork();

			if (ActiveProcess != null)
			{
				if (!ActiveProcess.HasExited)
				{
					SessionLog.WriteCommandletEvent(Color.Maroon, string.Format("STOPPING COMMANDLET '{0} {1}'...", Path.GetFileName(ActiveProcess.ExecutablePath), ActiveProcess.CommandLine));

					ActiveProcess.Kill();
				}
			}

			Worker.Flush();

		}

		/// <summary>
		/// Wait for the currently active commandlet to exit
		/// </summary>
		/// <returns>true if the process terminated correctly; false if exited with errors.</returns>
		public bool WaitForActiveProcessToComplete()
		{
			while (ActiveProcess != null && !ActiveProcess.HasExited)
			{
				System.Threading.Thread.Sleep(100);
			}

			if (ActiveProcess != null && ActiveProcess.ExitCode != 0)
			{
				// The step failed; should probably cancel all remaining queued work
				return false;
			}
			return true;
		}

		#endregion

		/// Called when the active process exits.
		public void OnProcessExited(object sender, EventArgs e)
		{
			SessionLog.CommandletProcess_Exited(sender, e);
		}
		
		private DateTime LastTaskStartTime;
		/// How long the queued work took to complete.
		public double LastTaskElapsedSeconds { get; private set; }

		private bool mIsRefreshingTargets = false;
		/// True when a target refresh is ongoing
		public bool IsRefreshingTargets
		{
			get { return mIsRefreshingTargets; }
			set
			{
				if (mIsRefreshingTargets != value)
				{
					mIsRefreshingTargets = value;
					NotifyPropertyChanged("IsRefreshingTargets");
				}
			}
		}

		public bool IsUDK
		{
			get { return UDKMode == EUDKMode.UDK; }
		}

		private bool mIsWorking = false;
		/// When true, there is work in the work queue; we should be spinning a throbber and showing a stop.
		public bool IsWorking
		{
			get { return mIsWorking; }
			set
			{
				if (mIsWorking != value)
				{
					if (value == true)
					{
						LastTaskStartTime = DateTime.Now;
					}
					else
					{
						LastTaskElapsedSeconds = (DateTime.Now - LastTaskStartTime).TotalSeconds;
					}					
					mIsWorking = value;
					NotifyPropertyChanged("IsWorking");
				}
			}
		}

		/// Given a Profile, queue up execution of the Nth step in the pipeline, where
		/// N is specified by the Index. If bClean is true, CleanAndExecute the step.
		public void QueueExecuteStep( Profile InProfile, int Index, bool bClean )
		{
			// Clone data so the WorkerThread can safely do work on it.
			Profile ClonedProfile = InProfile.Clone();
			ClonedProfile.Validate();
			QueueWork( ()=>
			{
				if ( bClean )
				{
					ClonedProfile.Pipeline.Steps[Index].CleanAndExecute(this, ClonedProfile);
				}
				else
				{
					ClonedProfile.Pipeline.Steps[Index].Execute(this, ClonedProfile);	
				}
				
			});
		}

		
		/// Run the entire pipeline for this profile. Certain frequent usage scenarios are encoded
		/// by RunOptions; e.g. Full recook or Cook INIs only.
		public void RunProfile(Profile ProfileToExecute, Pipeline.ERunOptions RunOptions)
		{
			// Fix any incompatible/invalid settings the user may have set
			ProfileToExecute.Validate();

			// Make a copy because user changes to the profile should not affect the work already queued.
			ProfileToExecute = ProfileToExecute.Clone();

			QueueWork(() =>
			{
				bool bNoErrors = true;
				foreach (Pipeline.Step SomeStep in ProfileToExecute.Pipeline.Steps)
				{
					if (!SomeStep.ShouldSkipThisStep && bNoErrors)
					{
						Pipeline.Step StepToExecute = SomeStep;
						if (StepToExecute is Pipeline.Cook)
						{
							Pipeline.Cook CookStep = StepToExecute as Pipeline.Cook;
							if ( (RunOptions & Pipeline.ERunOptions.FullReCook) != 0 )
							{
								bNoErrors = CookStep.CleanAndExecute(this, ProfileToExecute);
							}
							else if ( (RunOptions & Pipeline.ERunOptions.CookINIsOnly) != 0 )
							{
								bNoErrors = CookStep.ExecuteINIsOnly(this, ProfileToExecute);
							}
							else
							{
								bNoErrors = CookStep.Execute(this, ProfileToExecute);
							}
						}
						else if ( (RunOptions & Pipeline.ERunOptions.RebuildScript) != 0 && SomeStep is Pipeline.MakeScript)
						{
							bNoErrors = StepToExecute.CleanAndExecute(this, ProfileToExecute);
						}
						else
						{
							bNoErrors = StepToExecute.Execute(this, ProfileToExecute);
						}

						if (bNoErrors)
						{
							bNoErrors = WaitForActiveProcessToComplete();
						}
						
					}
					else
					{
						SessionLog.AddLine(bNoErrors ? Color.DarkMagenta : Color.Red, String.Format("\n[Skipping {0}]", SomeStep.StepName));
					}
				
				}
				if( bNoErrors )
				{
					SessionLog.AddLine(Color.Green, String.Format("\n[{0}] ALL PIPELINE STEPS COMPLETED SUCCESSFULLY.", DateTime.Now.ToString("MMM d, h:mm tt")));
				}
				else
				{
					SessionLog.AddLine(Color.Red, String.Format("\n[{0}] PIPELINE FAILED TO COMPLETE.", DateTime.Now.ToString("MMM d, h:mm tt")));
				}
			});
		}

		/// Queue up a reboot of all active tasks for the fiven profile.
		public void QueueReboot( Profile ProfileToReboot )
		{
			this.QueueWork( () => Pipeline.Launch.CheckedReboot(ProfileToReboot, false, null) );
		}

		public delegate void QueueWorkDelegate();

		private void QueueWork(QueueWorkDelegate InWork)
		{
			Worker.QueueWork(() =>
			{
				InWork();
				WaitForActiveProcessToComplete();
			});
		}

		/// Queue up a refresh task; refresh tasks are executed which higher priority than any other task in the worker queue.
		public void QueueRefreshTask(QueueWorkDelegate InWork)
		{
			Worker.QueuePlatformRefresh(() =>
			{
				InWork();
				WaitForActiveProcessToComplete();
			});
		}

		/// The work thread is empty; queue up an update on the UI thread.
		private void DispatchWorkCompleted(System.Windows.Threading.Dispatcher UIDispatcher)
		{
			UIDispatcher.BeginInvoke(new WorkerThread.VoidTask(delegate()
			{
				IsWorking = false;
				IsRefreshingTargets = false;
			}));
		}

		/// The worker thread has new tasks; queue u pan update in the UI thread.
		private void DispatchWorkStarted(System.Windows.Threading.Dispatcher UIDispatcher, WorkerThread.ETaskType TaskType)
		{
			UIDispatcher.BeginInvoke(new WorkerThread.VoidTask(delegate()
			{
				if (TaskType == WorkerThread.ETaskType.Refresh)
				{
					IsWorking = false;
					IsRefreshingTargets = true;
				}
				else
				{
					IsWorking = true;
					IsRefreshingTargets = false;
				}
			}));
		}

		/// Launch unreal console for each active target; do it on the worker thread.
		public void QueueLaunchUnrealConsole( Profile InProfile )
		{
			Worker.QueueWork(() =>
			{
				bool bLaunchedUnrealConsole = false;
				foreach (Target SomeTarget in InProfile.TargetsList.Targets)
				{
					if (SomeTarget.ShouldUseTarget)
					{
						if (!SomeTarget.TheTarget.IsConnected)
						{
							if (!SomeTarget.TheTarget.Connect())
							{
								SessionLog.AddLine(Color.Red, "Failed connection attempt with target \'" + SomeTarget.Name + "\'!");
							}
						}
						bLaunchedUnrealConsole |= this.LaunchUnrealConsole(SomeTarget.TheTarget, InProfile.Launch_ClearUCWindow);
					}
				}

				if (!bLaunchedUnrealConsole)
				{
					this.LaunchUnrealConsole(null, false);
				}
			});
		}

		/// <summary>
		/// @todo : This needs some serious cleanup. Ported over from the old UFE.
		/// </summary>
		#region Commandlet Stuff (needs reworking)

		// used to talk to unreal console
		private IpcChannel UCIpcChannel;

		/// <summary>
		/// Launches a new instance of UnrealConsole for the specified target.
		/// </summary>
		/// <param name="Target">The target for unreal console to connect to.</param>
		public bool LaunchUnrealConsole(ConsoleInterface.PlatformTarget Target, bool bClearUnrealWindow)
		{
			bool UCLaunched = false;

			while (!UCLaunched)
			{
				try
				{
					if (UCIpcChannel == null)
					{
						UCIpcChannel = new IpcChannel();
						ChannelServices.RegisterChannel(UCIpcChannel, true);
					}

					string Dir = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
					Dir = Path.Combine(Dir, "UnrealConsole.exe");
					string TargetName = string.Empty;

					if (Target != null)
					{
						// everything except the PS3 has an accessible debug channel IP but the PS3 has an always accessible name
						if (Target.ParentPlatform.Type == ConsoleInterface.PlatformType.PS3 ||
							Target.ParentPlatform.Type == ConsoleInterface.PlatformType.NGP)
						{
							TargetName = Target.Name;
						}
						else
						{
							TargetName = Target.DebugIPAddress.ToString();
						}

						if (Target.IsConnected && Target.ParentPlatform.Type == ConsoleInterface.PlatformType.Xbox360)
						{
							while (Target.State == ConsoleInterface.TargetState.Rebooting)
							{
								System.Threading.Thread.Sleep(50);
							}
						}
					}

					// See if there is a UC window already open...
					System.Diagnostics.Process[] LocalProcesses = System.Diagnostics.Process.GetProcessesByName("UnrealConsole");

					if (LocalProcesses.Length == 0)
					{
						// Spawn a new instance of UC if none are already running
						ProcessStartInfo Info = (Target!=null)
							? new ProcessStartInfo(Dir, string.Format("-platform={0} -target={1}", Target.ParentPlatform.Name, TargetName))
							: new ProcessStartInfo(Dir, "");
						Info.CreateNoWindow = false;
						Info.UseShellExecute = true;
						Info.Verb = "open";

						SessionLog.AddLine(Color.Green, "Spawning: " + Dir + " " + Info.Arguments);
						System.Diagnostics.Process.Start(Info).Dispose();
						UCLaunched = true;
					}
					else if (Target != null)
					{
						// Create a new tab in and existing instance
						foreach (System.Diagnostics.Process CurProc in LocalProcesses)
						{
							try
							{
								// Get a remote object in UnrealConsole
								RemoteUCObject RemoteObj = (RemoteUCObject)Activator.GetObject(typeof(RemoteUCObject), string.Format("ipc://{0}/{0}", CurProc.Id.ToString()));

								// This always returns true
								bool bFoundTarget = RemoteObj.HasTarget(Target.ParentPlatform.Name, TargetName);
								if (bFoundTarget)
								{
									try
									{
										SessionLog.AddLine(Color.Green, " ... using existing UnrealConsole: -platform=" + Target.ParentPlatform.Name + " -target=" + TargetName);
										RemoteObj.OpenTarget(Target.ParentPlatform.Name, TargetName, bClearUnrealWindow );

										UCLaunched = true;
									}
									catch (Exception ex)
									{
										string ErrStr = ex.ToString();
										System.Diagnostics.Debug.WriteLine(ErrStr);

										SessionLog.AddLine(Color.Orange, "Warning: Could not open target in UnrealConsole instance \'" + CurProc.Id.ToString() + "\'");
									}

									break;
								}
							}
							catch (Exception)
							{
								SessionLog.AddLine( Color.Orange, "Failed to connect to existing instance of UnrealConsole. Please select it manually." );
								UCLaunched = true;
							}
						}
					}

					foreach (System.Diagnostics.Process CurProc in LocalProcesses)
					{
						CurProc.Dispose();
					}
				}
				catch (Exception ex)
				{
					string ErrStr = ex.ToString();
					System.Diagnostics.Debug.WriteLine(ErrStr);

					SessionLog.AddLine(Color.Red, ErrStr);

					// Fatal error - break out
					UCLaunched = true;
				}
			}

			return (UCLaunched);
		}

		#endregion


		#region INotifyPropertyChanged

		public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;
		private void NotifyPropertyChanged(String PropertyName)
		{
			if (PropertyChanged != null)
			{
				PropertyChanged(this, new System.ComponentModel.PropertyChangedEventArgs(PropertyName));
			}
		}

		private void AssignBool(String InPropName, ref bool InOutProp, bool InNewValue)
		{
			if (InOutProp != InNewValue)
			{
				InOutProp = InNewValue;
				NotifyPropertyChanged(InPropName);
			}
		}

		#endregion
	}

}
