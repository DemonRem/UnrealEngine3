/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Management;
using System.ServiceProcess;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;

namespace Controller
{
	public enum MODES
	{
		Init,
		Monitor,
		Wait,
		WaitForFTP,
		WaitForActive,
		WaitForJobs,
		Finalise,
		Exit,
	}

	public enum COMMANDS
	{
		None,
		Error,
		SilentlyFail,
		NoScript,
		IllegalCommand,
		TimedOut,
		WaitTimedOut,
		WaitJobsTimedOut,
		FailedJobs,
		Crashed,
		CriticalError,
		Process,
		Config,
		TriggerMail,
		SendEmail,
		CookingSuccess,
		CookerSyncSuccess,
		SCC_FileLocked,
		SCC_CheckConsistency,
		SCC_GetLatestChangelist,
		SCC_Sync,
		SCC_OutOfBranchSync,
		SCC_SyncToHead,
		SCC_Resolve,
		SCC_ArtistSync,
		SCC_GetChanges,
		SCC_SyncSingleChangeList,
		SCC_Checkout,
		SCC_OpenForDelete,
		SCC_GetClientRoot,
		SCC_CheckoutGame,
		SCC_CheckoutGFWLGame,
		SCC_CheckoutShader,
		SCC_CheckoutDialog,
		SCC_CheckoutFonts,
		SCC_CheckoutLocPackage,
		SCC_CheckoutGDF,
		SCC_CheckoutCat,
		SCC_CheckoutGADCheckpoint,
		SCC_CheckoutLayout,
		SCC_CheckoutHashes,
		SCC_CheckoutEngineContent,
		SCC_CheckoutGameContent,
		SCC_Submit,
		SCC_CreateNewLabel,
		SCC_DeleteLabel,
		SCC_LabelCreateNew,
		SCC_LabelUpdateDescription,
		SCC_GetLabelInfo,
		SCC_LabelDelete,
		SCC_OpenedFiles,
		SCC_Revert,
		SCC_RevertFileSpec,
		SCC_Tag,
		SCC_TagMessage,
		SCC_CreatePendingChangelist,
		SCC_GetDepotFileSpec,
		SCC_GetClientBranches,
		SCC_GetUserInfo,
		SCC_GetChangelists,
		SCC_GetHaveRevision,
		SCC_GetIncorrectCheckedOutFiles,
		WaitForActive,
		WaitForJobs,
		AddUnrealGameJob,
		AddUnrealFullGameJob,
		AddUnrealGFWLGameJob,
		AddUnrealGFWLFullGameJob,
		SetDependency,
		Clean,
		BuildUBT,
		MSBuild,
		MS8Build,
		MS9Build,
		MSVCClean,
		MSVCBuild,
		MSVCDeploy,
		MSVC8Clean,
		MSVC8Build,
		MSVC9Clean,
		MSVC9Build,
		MSVC9Deploy,
		UnrealBuild,
		GCCClean,
		GCCBuild,
		ShaderClean,
		ShaderBuild,
		LocalShaderBuild,
		CookShaderBuild,
		BuildScript,
		BuildScriptNoClean,
		iPhonePackage,
		PreHeatMapOven,
		PreHeatDLC,
		CookMaps,
		CookIniMaps,
		CookSounds,
		CreateHashes,
		Wrangle,
		Publish,
		PublishTagset,
		PublishLanguage,
		PublishLayout,
		PublishLayoutLanguage,
		PublishDLC,
		PublishTU,
		PublishFiles,
		PublishRawFiles,
		MakeISO,
		MakeMD5,
		CopyFolder,
		MoveFolder,
		CleanFolder,
		GetPublishedData,
		GetCookedBuild,					// deprecated for GetPublishedData
		GetTagset,						// deprecated for GetPublishedData
		GetCookedLanguage,				// deprecated for GetPublishedData
		SteamMakeVersion,
		UpdateSteamServer,
		StartSteamServer,
		StopSteamServer,
		RestartSteamServer,
		UnSetup,
		CreateDVDLayout,
		Conform,
		PatchScript,
		CheckpointGameAssetDatabase,
		UpdateGameAssetDatabase,
		TagReferencedAssets,
		TagDVDAssets,
		AuditContent,
		FindStaticMeshCanBecomeDynamic,
		FixupRedirects,
		ResaveDeprecatedPackages,
		AnalyzeReferencedContent,
		MineCookedPackages,
		ContentComparison,
		DumpMapSummary,
		CrossBuildConform,
		UpdateWBTS,
		Finished,
		Trigger,
		SQLExecInt,
		SQLExecDouble,
		ValidateInstall,
		BumpAgentVersion,
		BumpEngineVersion,
		GetEngineVersion,
		UpdateGDFVersion,
		MakeGFWLCat,
		ZDPP,
		SteamDRM,
		FixupSteamDRM,
		SaveDefines,
		UpdateSourceServer,
		UpdateSymbolServer,
		AddExeToSymbolServer,
		AddDllToSymbolServer,
		UpdateSymbolServerTick,
		BlastLA,
		Blast,
		BlastDLC,
		BlastTU,
		XBLA,
		XLastGenTU,
		PS3MakePatchBinary,
		PS3MakePatch,
		PS3MakeDLC,
		PS3MakeTU,
		PCMakeTU,
		PCPackageTU,
		CheckSigned,
		Sign,
		SignCat,
		SignBinary,
		SignFile,
		TrackFileSize,
		TrackFolderSize,
		SimpleCopy,
		SourceBuildCopy,
		SimpleDelete,
		SimpleRename,
		RenamedCopy,
		Wait,
		UpdateLabel,
		UpdateFolder,
		CISProcessP4Changes,
		CISUpdateMonitorValues,
		CheckForUCInVCProjFiles,
		SmokeTest,
		LoadPackages,
		CookPackages,
		FTPSendFile,
		FTPSendImage,
		ZipAddImage,
		ZipAddFile,
		ZipSave,
		RarAddImage,
		RarAddFile,
		CleanupSymbolServer,

		VCFull,         // Composite command - clean then build
		MSVC8Full,      // Composite command - clean then build
		MSVC9Full,      // Composite command - clean then build
		GCCFull,        // Composite command - clean then build
		ShaderFull,     // Composite command - clean then build
	}

	public enum RevisionType
	{
		Invalid,
		ChangeList,
		Label
	}

	public partial class Main : Form
	{
		// For consistent formatting to the US standard (05/29/2008 11:33:52)
		public const string US_TIME_DATE_STRING_FORMAT = "MM/dd/yyyy HH:mm:ss";

		private UnrealControls.OutputWindowDocument MainLogDoc = new UnrealControls.OutputWindowDocument();

		public string LabelRoot = "UnrealEngine3";

		public BuilderDB DB = null;
		public Emailer Mailer = null;
		public P4 SCC = null;
		public Watcher Watcher = null;

		public string iPhone_CompileServerName = GetStringEnvironmentVariable( "ue3.iPhone_CompileServerName", "best_available" );
		public string iPhone_SigningServerName = GetStringEnvironmentVariable( "ue3.iPhone_SigningServerName", "a1488" );
		public string[] iPhone_PotentialCompileServerNames = {
			"a1488",
			"a1487"
		};
		public string iPhone_DeveloperSigningIdentity = GetStringEnvironmentVariable( "ue3.iPhone_DeveloperSigningIdentity", "iPhone Developer: Mike Capps (E6TD75QUE5)" );
		public string iPhone_DistributionSigningIdentity = GetStringEnvironmentVariable( "ue3.iPhone_DistributionSigningIdentity", "iPhone Distribution: Epic Games" );

		private DateTime LastPingTime = DateTime.UtcNow;
		private DateTime LastTempTime = DateTime.UtcNow;
		private DateTime LastPrimaryBuildPollTime = DateTime.UtcNow;
		private DateTime LastNonPrimaryBuildPollTime = DateTime.UtcNow;
		private DateTime LastPrimaryJobPollTime = DateTime.UtcNow;
		private DateTime LastNonPrimaryJobPollTime = DateTime.UtcNow;
		private DateTime LastSystemDownTime = DateTime.UtcNow;
		private DateTime LastBuildKillTime = DateTime.UtcNow;
		private DateTime LastJobKillTime = DateTime.UtcNow;
		private DateTime LastCheckForJobTime = DateTime.UtcNow;

		// The next scheduled time we'll update the database
		DateTime NextMaintenanceTime = DateTime.UtcNow;
		DateTime NextStatsUpdateTime = DateTime.UtcNow;

		// Time periods, in seconds, between various checks
		uint MaintenancePeriod = 30;
		uint StatsUpdatePeriod = 600;

		/// 
		/// A collection of simple performance counters used to get basic monitoring stats
		/// 
		private class PerformanceStats
		{
			// CPU stats
			public PerformanceCounter CPUBusy = null;

			// Disk stats
			public PerformanceCounter LogicalDiskReadLatency = null;
			public PerformanceCounter LogicalDiskWriteLatency = null;
			public PerformanceCounter LogicalDiskTransferLatency = null;
			public PerformanceCounter LogicalDiskQueueLength = null;
			public PerformanceCounter LogicalDiskReadQueueLength = null;

			// Memory stats
			public PerformanceCounter SystemMemoryAvailable = null;
		};
		PerformanceStats LocalPerfStats = null;

		public class BranchDefinition
		{
			public string Branch;
			public string Server;
			public string DirectXVersion;
			public string XDKVersion;
			public string PS3SDKVersion;
			public string PFXSubjectName;

			public BranchDefinition()
			{
			}
		}

		public List<BranchDefinition> AvailableBranches = new List<BranchDefinition>();

		public class PerforceServerInfo
		{
			public string Server;
			public string User;
			public string Password;

			public PerforceServerInfo()
			{
				Server = "p4-server:1666";
				User = "build_machine";
				Password = "NotAChance";
			}

			public PerforceServerInfo( string InServer, string InUser, string InPassword )
			{
				Server = InServer;
				User = InUser;
				Password = InPassword;
			}

			public PerforceServerInfo( BuilderDB DB, string SourceControlBranch )
			{
				Server = DB.GetStringForBranch( SourceControlBranch, "Server" );
				User = DB.GetStringForBranch( SourceControlBranch, "P4User" );
				Password = DB.GetStringForBranch( SourceControlBranch, "P4Password" );
			}
		}

		// The root folder of the clientspec - where all the branches reside
		public string RootFolder { get; set; }

		// A timestamp of when this builder was compiled
		public DateTime CompileDateTime { get; set; }

		// The installed version of MSVC8
		public string MSVC8Version { get; set; }

		// The installed version of MSVC9
		public string MSVC9Version { get; set; }

		// The used version of DirectX
		public string DXVersion { get; set; }

		// The local current version of the XDK
		public string XDKVersion { get; set; }

		// The local current version of the PS3 SDK
		public string PS3SDKVersion { get; set; }

		// Number of processors in this machine - used to define number of threads to use when choice is available
		public int NumProcessors { get; set; }

		// Number of GB RAM in this machine
		public int PhysicalMemory { get; set; }

		// Number of parallel jobs to use (based on the number of logical processors)
		public int NumJobs { get; set; }

		// Whether this machine is locked to a specific build
		public int MachineLock { get; set; }

		// A unique id for each job set
		public long JobSpawnTime { get; set; }

		// The number of jobs placed into the job table
		public int NumJobsToWaitFor { get; set; }

		// The number of jobs completed so far
		public int NumCompletedJobs { get; set; }

		// A log of everything the system 
		public TextWriter SystemLog { get; set; }

		public bool Ticking = true;
		public bool Restart = false;
		public string MachineName = "";
		private string LoggedOnUser = "";
		private string SourceBranch = "";

		public int CommandID = 0;
		public int JobID = 0;
		private int BuilderID = 0;
		private int BuildLogID = 0;
		private int Promotable = 0;
		private Command CurrentCommand = null;
		private Queue<COMMANDS> PendingCommands = new Queue<COMMANDS>();
		private int BlockingBuildID = 0;
		private int BlockingBuildLogID = 0;
		private DateTime BlockingBuildStartTime = DateTime.UtcNow;
		private ScriptParser Builder = null;
		private MODES Mode = 0;
		private int CompileRetryCount = 0;
		private int LinkRetryCount = 0;
		private string FinalStatus = "";

		delegate void DelegateAddLine( string Line, Color TextColor );
		delegate void DelegateSetStatus( string Line );
		delegate void DelegateMailGlitch();

		public static string GetStringEnvironmentVariable( string VarName, string Default )
		{
			string Value = Environment.GetEnvironmentVariable( VarName );
			if( Value != null )
			{
				return Value;
			}
			return Default;
		}

		public void SendWarningMail( string Title, string Warnings, bool CCIT )
		{
			Mailer.SendWarningMail( Title, Warnings, CCIT );
		}

		public void SendErrorMail( string Title, string Warnings )
		{
			Mailer.SendErrorMail( Title, Warnings, false );
		}

		public void SendAlreadyInProgressMail( string Operator, int CommandID, string BuildDescription )
		{
			Mailer.SendAlreadyInProgressMail( Operator, CommandID, BuildDescription );
		}

		public void DeleteFiles( string Path, int DaysOld )
		{
			// Delete all the files in the directory tree
			DirectoryInfo DirInfo = new DirectoryInfo( Path );
			if( DirInfo.Exists )
			{
				DirectoryInfo[] Directories = DirInfo.GetDirectories();
				foreach( DirectoryInfo Directory in Directories )
				{
					DeleteFiles( Directory.FullName, DaysOld );
				}

				FileInfo[] Files = DirInfo.GetFiles();
				foreach( FileInfo File in Files )
				{
					bool bShouldDelete = true;
					if( DaysOld > 0 )
					{
						TimeSpan Age = DateTime.UtcNow - File.CreationTimeUtc;
						TimeSpan MaxAge = new TimeSpan( DaysOld, 0, 0, 0 );
						if( Age <= MaxAge )
						{
							bShouldDelete = false;
						}
					}
					if( bShouldDelete )
					{
						try
						{
							File.IsReadOnly = false;
							File.Delete();
						}
						catch
						{
						}
					}
				}
			}
		}

		public void DeleteEmptyDirectories( string Path )
		{
			// Delete all the files in the directory tree
			DirectoryInfo DirInfo = new DirectoryInfo( Path );
			if( DirInfo.Exists )
			{
				DirectoryInfo[] Directories = DirInfo.GetDirectories();
				foreach( DirectoryInfo Directory in Directories )
				{
					DeleteEmptyDirectories( Directory.FullName );

					// Delete this folder if it has no subfolders or files
					FileInfo[] SubFiles = Directory.GetFiles();
					DirectoryInfo[] SubDirectories = Directory.GetDirectories();

					if( SubFiles.Length == 0 && SubDirectories.Length == 0 )
					{
						try
						{
							Directory.Delete( true );
						}
						catch
						{
						}
					}
				}
			}
		}

		// Recursively delete an entire directory tree
		public bool DeleteDirectory( string Path, int DaysOld )
		{
			try
			{
				// Delete all the files in the directory tree
				DeleteFiles( Path, DaysOld );

				// Delete all the empty folders
				DeleteEmptyDirectories( Path );
			}
			catch( Exception Ex )
			{
				Log( "Failed to delete directory: '" + Path + "' with Exception: " + Ex.Message, Color.Red );
				return ( false );
			}

			return ( true );
		}

		// Ensure the base build folder exists to copy builds to
		public void EnsureDirectoryExists( string Path )
		{
			DirectoryInfo Dir = new DirectoryInfo( Path );
			if( !Dir.Exists )
			{
				Dir.Create();
			}
		}

		// Extract the time and date of compilation from the version number
		private void CalculateCompileDateTime()
		{
			System.Version Version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
			CompileDateTime = DateTime.Parse( "01/01/2000" ).AddDays( Version.Build ).AddSeconds( Version.Revision * 2 );

			Log( "Controller compiled on " + CompileDateTime.ToString(), Color.Blue );
		}

		private void GetInfo()
		{
			MachineName = Environment.MachineName;
			LoggedOnUser = Environment.UserName;

#if DEBUG
			MachineName = "JSCOTT-D1078";
#endif

			Log( "Welcome \"" + LoggedOnUser + "\" running on \"" + MachineName + "\"", Color.Blue );
		}

		private void GetMSVCVersion()
		{
			try
			{
				string MSVC8Exe = Environment.GetEnvironmentVariable( "VS80COMNTOOLS" );
				if( MSVC8Exe != null && MSVC8Exe.Length > 0 )
				{
					FileVersionInfo VI = FileVersionInfo.GetVersionInfo( MSVC8Exe + "/../IDE/Devenv.exe" );
					MSVC8Version = VI.FileDescription + " version " + VI.ProductVersion;
					Log( "Compiling with: " + MSVC8Version, Color.Blue );
				}
				else
				{
					Log( "Could not find VS80COMNTOOLS environment variable", Color.Blue );
				}

				string MSVC9Exe = Environment.GetEnvironmentVariable( "VS90COMNTOOLS" );
				if( MSVC9Exe != null && MSVC9Exe.Length > 0 )
				{
					FileVersionInfo VI = FileVersionInfo.GetVersionInfo( MSVC9Exe + "/../IDE/Devenv.exe" );
					MSVC9Version = VI.FileDescription + " version " + VI.ProductVersion;
					Log( "Compiling with: " + MSVC9Version, Color.Blue );
				}
				else
				{
					Log( "Could not find VS90COMNTOOLS environment variable", Color.Blue );
				}
			}
			catch
			{
				Ticking = false;
			}
		}

		private void ValidatePS3EnvVars()
		{
			try
			{
				string[] PS3EnvVars = new string[] { "SCE_PS3_ROOT" };

				foreach( string EnvVar in PS3EnvVars )
				{
					string EnvVarValue = Environment.GetEnvironmentVariable( EnvVar );
					if( EnvVarValue != null )
					{
						Log( " ...... EnvVar '" + EnvVar + "' = '" + EnvVarValue + "'", Color.Blue );
					}
				}
			}
			catch
			{
			}
		}

		private string GetPS3SDKVersion()
		{
			string Line = "";
			try
			{
				string Dir = Environment.GetEnvironmentVariable( "SCE_PS3_ROOT" );
				string PS3SDK = Path.Combine( Dir, "version-SDK" );
				StreamReader Reader = new StreamReader( PS3SDK );
				if( Reader != null )
				{
					Line = Reader.ReadToEnd();
					Reader.Close();
				}
			}
			catch
			{
			}

			return ( Line.Trim() );
		}

		private void GetCurrentPS3SDKVersion()
		{
			PS3SDKVersion = GetPS3SDKVersion();

			if( PS3SDKVersion.Length > 0 )
			{
				Log( " ... using PS3 SDK: " + PS3SDKVersion, Color.Blue );

				ValidatePS3EnvVars();
			}
		}

		private void GetXDKVersion()
		{
			try
			{
				string Line;

				string XEDK = Environment.GetEnvironmentVariable( "XEDK" );
				if( XEDK != null )
				{
					string XDK = XEDK + "/include/win32/vs2005/xdk.h";
					FileInfo Info = new FileInfo( XDK );
					if( !Info.Exists )
					{
						XDK = XEDK + "/include/win32/xdk.h";
						Info = new FileInfo( XDK );
					}

					if( Info.Exists )
					{
						StreamReader Reader = new StreamReader( XDK );
						if( Reader != null )
						{
							Line = Reader.ReadLine();
							while( Line != null )
							{
								if( Line.StartsWith( "#define" ) )
								{
									int Offset = Line.IndexOf( "_XDK_VER" );
									if( Offset >= 0 )
									{
										XDKVersion = Line.Substring( Offset + "_XDK_VER".Length ).Trim();
										break;
									}
								}

								Line = Reader.ReadLine();
							}
							Reader.Close();
							Log( " ... using XDK: " + XDKVersion, Color.Blue );
						}
					}
				}
				else
				{
					Log( "Could not find XDK environment variable", Color.Blue );
				}
			}
			catch
			{
			}
		}

		private void GetAvailableBranches( int ID )
		{
			// Get all the Perforce servers listed in the BranchConfig table
			List<string> Servers = DB.GetPerforceServers();

			foreach( string Server in Servers )
			{
				// Gets a list of client branches based on the clientspec for the given server and user
				string User = DB.GetStringForServer( Server, "P4User" );
				string Password = DB.GetStringForServer( Server, "P4Password" );
				PerforceServerInfo ServerInfo = new PerforceServerInfo( Server, User, Password );
				List<string> ClientBranches = SCC.GetClientBranches( ServerInfo );
#if DEBUG
				// Do not pull builds from the main branch when developing
				ClientBranches.Remove( "unrealengine3" );
				ClientBranches.Remove( "unrealengine3-chair" );
#endif
				foreach( string Branch in ClientBranches )
				{
					BranchDefinition BranchDef = DB.GetBranchDefinition( Branch );
					if( BranchDef != null )
					{
						string Command = "INSERT INTO Branches ( BuilderID, Branch ) VALUES ( " + ID.ToString() + ", '" + BranchDef.Branch + "' )";
						DB.Update( Command );

						AvailableBranches.Add( BranchDef );
						Log( " ... added branch: " + BranchDef.Branch + " for server '" + Server + "'", Color.DarkGreen );
					}
				}
			}
		}

		private BranchDefinition FindBranchDefinition( string BranchName )
		{
			BranchDefinition FoundBranchDef = null;

			foreach( BranchDefinition BranchDef in AvailableBranches )
			{
				if( BranchDef.Branch.ToLower() == BranchName.ToLower() )
				{
					FoundBranchDef = BranchDef;
				}
			}

			return ( FoundBranchDef );
		}

		private void BroadcastMachine()
		{
			string Query, Command;
			int ID;

			// Clearing out zombie connections if the process stopped unexpectedly
			Query = "SELECT ID FROM Builders WHERE ( Machine ='" + MachineName + "' AND State != 'Dead' AND State != 'Zombied' )";
			ID = DB.ReadInt( Query );
			while( ID != 0 )
			{
				Command = "UPDATE Builders SET State = 'Zombied' WHERE ( ID = " + ID.ToString() + " )";
				DB.Update( Command );

				Log( "Cleared zombie connection " + ID.ToString() + " from database", Color.Orange );
				ID = DB.ReadInt( Query );
			}

			// Clear out zombie commands
			Query = "SELECT ID FROM Commands WHERE ( Machine ='" + MachineName + "' )";
			ID = DB.ReadInt( Query );
			if( ID != 0 )
			{
				DB.SetString( "Commands", ID, "Machine", "" );
				DB.Delete( "Commands", ID, "BuildLogID" );
			}

			// Clear out any orphaned jobs
			Query = "SELECT ID FROM Jobs WHERE ( Machine ='" + MachineName + "' AND Complete = 0 )";
			ID = DB.ReadInt( Query );
			if( ID != 0 )
			{
				DB.SetBool( "Jobs", ID, "Complete", true );
				Log( "Cleared orphaned job " + ID.ToString() + " from database", Color.Orange );
			}

			Log( "Registering '" + MachineName + "' with database", Color.Blue );

			// Insert machine as new row
			Command = "INSERT INTO Builders ( Machine, State, StartTime, PS3_SDKVersion, X360_SDKVersion ) VALUES ( '"
						+ MachineName + "', 'Connected', '" + DateTime.UtcNow.ToString( US_TIME_DATE_STRING_FORMAT ) + "', "
						+ "'" + PS3SDKVersion + "', '" + XDKVersion + "' )";
			DB.Update( Command );

			Query = "SELECT @@IDENTITY";
			ID = DB.ReadDecimal( Query );

			if( ID != 0 )
			{
				GetAvailableBranches( ID );
			}

			// If this machine locked to a build then don't allow it to grab normal ones
			Query = "SELECT [ID] FROM Commands WHERE ( '" + MachineName + "' LIKE MachineLock )";
			MachineLock = DB.ReadInt( Query );
			if( MachineLock == 0 )
			{
				Log( "Starting up with machine not locked to any specific command", Color.Blue );
			}
			else
			{
				Log( "Starting up with machine locked to command ID " + MachineLock.ToString(), Color.Blue );
			}
		}

		private void MaintainMachine()
		{
			// Check to see if it's time for maintenance
			if( DateTime.UtcNow >= NextMaintenanceTime )
			{
				// Schedule the next update
				NextMaintenanceTime = DateTime.UtcNow.AddSeconds( MaintenancePeriod );

				string Command = "SELECT ID FROM [Builders] WHERE ( Machine = '" + MachineName + "' AND State != 'Dead' AND State != 'Zombied' )";
				int ID = DB.ReadInt( Command );
				if( ID != 0 )
				{
					// We'll always update the time
					Command = "UPDATE [Builders] SET CurrentTime = '" + DateTime.UtcNow.ToString( US_TIME_DATE_STRING_FORMAT ) + "'";

					// Check to see if it's time for the big stats update as well
					if( DateTime.UtcNow >= NextStatsUpdateTime )
					{
						// Schedule the next update
						NextStatsUpdateTime = DateTime.UtcNow.AddSeconds( StatsUpdatePeriod );

						// Combine the stats with the time
						Command += ( ", CPUBusy = " + LocalPerfStats.CPUBusy.NextValue().ToString()
								  + ", DiskReadLatency = " + LocalPerfStats.LogicalDiskReadLatency.NextValue().ToString()
								  + ", DiskWriteLatency = " + LocalPerfStats.LogicalDiskWriteLatency.NextValue().ToString()
								  + ", DiskTransferLatency = " + LocalPerfStats.LogicalDiskTransferLatency.NextValue().ToString()
								  + ", DiskQueueLength = " + LocalPerfStats.LogicalDiskQueueLength.NextValue().ToString()
								  + ", DiskReadQueueLength = " + LocalPerfStats.LogicalDiskReadQueueLength.NextValue().ToString()
								  + ", SystemMemoryAvailable = " + LocalPerfStats.SystemMemoryAvailable.NextValue().ToString() );
					}
					Command += " WHERE ( ID = " + ID.ToString() + " )";

					DB.Update( Command );
				}

				LastPingTime = DateTime.UtcNow;
			}

			// If this machine locked to a build then don't allow it to grab normal ones
			string Query = "SELECT [ID] FROM Commands WHERE ( '" + MachineName + "' LIKE MachineLock )";
			int MachineLockOld = MachineLock;
			MachineLock = DB.ReadInt( Query );
			if( MachineLock != MachineLockOld )
			{
				Log( "Detected a change in MachineLock state for '" + MachineName + "'", Color.Blue );
				if( MachineLock == 0 )
				{
					Log( "    Machine is unlocked!", Color.Blue );
				}
				else
				{
					Log( "    Machine is locked to Command ID " + MachineLock.ToString(), Color.Blue );
				}
			}
		}

		private int PollForBuild( bool PrimaryBuildsOnly )
		{
			int ID = 0;

			// Check every 5 seconds
			TimeSpan PingTime = new TimeSpan( 0, 0, 5 );

			// Be sure to check against the correct timer
			if( ( PrimaryBuildsOnly && ( DateTime.UtcNow - LastPrimaryBuildPollTime > PingTime ) ) ||
									  ( DateTime.UtcNow - LastNonPrimaryBuildPollTime > PingTime ) )
			{
				ID = DB.PollForBuild( MachineName, PrimaryBuildsOnly );
				if( PrimaryBuildsOnly )
				{
					LastPrimaryBuildPollTime = DateTime.UtcNow;
				}
				else
				{
					LastNonPrimaryBuildPollTime = DateTime.UtcNow;
				}
				if( ID >= 0 )
				{
					// Check for build already running
					string Query = "SELECT Machine FROM [Commands] WHERE ( ID = " + ID.ToString() + " )";
					string Machine = DB.ReadString( Query );
					if( Machine.Length > 0 )
					{
						string BuildType = DB.GetString( "Commands", ID, "Description" );
						string Operator = DB.GetString( "Commands", ID, "Operator" );
						Log( "[STATUS] Suppressing retrigger of '" + BuildType + "'", Color.Magenta );
						Mailer.SendAlreadyInProgressMail( Operator, ID, BuildType );

						ID = 0;
					}
				}
				else
				{
					Mailer.SendWarningMail( "PollForBuild", "DB.PollForBuild returned an error! Check logs for more details.", false );
					ID = 0;
				}
			}
			return ( ID );
		}

		private int PollForJob( bool PrimaryBuildsOnly )
		{
			int ID = 0;

			if( MachineLock == 0 )
			{
				// Check every 5 seconds
				TimeSpan PingTime = new TimeSpan( 0, 0, 5 );

				if( ( PrimaryBuildsOnly && ( DateTime.UtcNow - LastPrimaryJobPollTime > PingTime ) ) ||
										  ( DateTime.UtcNow - LastNonPrimaryJobPollTime > PingTime ) )
				{
					ID = DB.PollForJob( MachineName, PrimaryBuildsOnly );
					if( PrimaryBuildsOnly )
					{
						LastPrimaryJobPollTime = DateTime.UtcNow;
					}
					else
					{
						LastNonPrimaryJobPollTime = DateTime.UtcNow;
					}

					if( ID < 0 )
					{
						ID = 0;
					}

				}
			}

			return ( ID );
		}

		private int PollForKillBuild()
		{
			int ID = 0;

			// Check every 2 seconds
			TimeSpan PingTime = new TimeSpan( 0, 0, 2 );
			if( DateTime.UtcNow - LastBuildKillTime > PingTime )
			{
				ID = DB.PollForKillBuild();
				LastBuildKillTime = DateTime.UtcNow;
			}

			return ( ID );
		}

		private int PollForKillJob()
		{
			int ID = 0;

			// Check every 2 seconds
			TimeSpan PingTime = new TimeSpan( 0, 0, 2 );
			if( DateTime.UtcNow - LastJobKillTime > PingTime )
			{
				ID = DB.PollForKillJob();
				LastJobKillTime = DateTime.UtcNow;
			}

			return ( ID );
		}

		private void UnbroadcastMachine()
		{
			Log( "Unregistering '" + MachineName + "' from database", Color.Blue );

			string Command = "SELECT [ID] FROM [Builders] WHERE ( Machine = '" + MachineName + "' AND State != 'Dead' AND State != 'Zombied' )";
			int ID = DB.ReadInt( Command );
			if( ID != 0 )
			{
				Command = "UPDATE Builders SET EndTime = '" + DateTime.UtcNow.ToString( US_TIME_DATE_STRING_FORMAT ) + "' WHERE ( ID = " + ID.ToString() + " )";
				DB.Update( Command );

				Command = "UPDATE Builders SET State = 'Dead' WHERE ( ID = " + ID.ToString() + " )";
				DB.Update( Command );
			}
		}

		protected void CheckSystemDown()
		{
			// Check every 5 minutes
			TimeSpan PingTime = new TimeSpan( 0, 5, 0 );
			if( DateTime.UtcNow - LastSystemDownTime > PingTime )
			{
				string CommandString = "SELECT Value FROM [Variables] WHERE ( Variable = 'StatusMessage' )";
				string Message = DB.ReadString( CommandString );

				if( Message.Length > 0 )
				{
					// Just exit - don't restart
					Ticking = false;
				}

				LastSystemDownTime = DateTime.UtcNow;
			}
		}

		private void GetSourceBranch()
		{
			string CWD = Environment.CurrentDirectory;
			int Index = CWD.LastIndexOf( '\\' );
			if( Index >= 0 )
			{
				RootFolder = CWD.Substring( 0, Index );
				SourceBranch = CWD.Substring( Index + 1 );
			}
		}

		private void AppExceptionHandler( object sender, UnhandledExceptionEventArgs args )
		{
			Exception E = ( Exception )args.ExceptionObject;
			Log( "Application exception: " + E.ToString(), Color.Red );

			// Send a warning email with this error, if possible
			if( Mailer != null )
			{
				Mailer.SendErrorMail( "Application Exception in Controller!", E.ToString(), false );
			}
		}

		public void Init()
		{
			// Register application exception handler
			AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler( AppExceptionHandler );

			// Show log window
			Show();

			DB = new BuilderDB( this );
			SCC = new P4( this );
			Mailer = new Emailer( this, SCC );
			Watcher = new Watcher( this );

			// Create performance monitoring objects
			LocalPerfStats = new PerformanceStats();
			LocalPerfStats.CPUBusy = new PerformanceCounter( "Processor", "% Processor Time", "_Total" );
			LocalPerfStats.LogicalDiskReadLatency = new PerformanceCounter( "LogicalDisk", "Avg. Disk sec/Read", "_Total" );
			LocalPerfStats.LogicalDiskWriteLatency = new PerformanceCounter( "LogicalDisk", "Avg. Disk sec/Write", "_Total" );
			LocalPerfStats.LogicalDiskTransferLatency = new PerformanceCounter( "LogicalDisk", "Avg. Disk sec/Transfer", "_Total" );
			LocalPerfStats.LogicalDiskQueueLength = new PerformanceCounter( "LogicalDisk", "Avg. Disk Queue Length", "_Total" );
			LocalPerfStats.LogicalDiskReadQueueLength = new PerformanceCounter( "LogicalDisk", "Avg. Disk Read Queue Length", "_Total" );
			LocalPerfStats.SystemMemoryAvailable = new PerformanceCounter( "Memory", "Available Bytes" );

			Application.DoEvents();

			GetInfo();
			CalculateCompileDateTime();
			GetMSVCVersion();
			GetCurrentPS3SDKVersion();
			GetXDKVersion();
			GetSourceBranch();

			Application.DoEvents();

			// Something went wrong during setup - sleep and retry
			if( !Ticking )
			{
				System.Threading.Thread.Sleep( 30000 );
				Restart = true;
				return;
			}

			// Register with DB
			BroadcastMachine();
			// Inform user of restart
			Mailer.SendRestartedMail();

			PerforceServerInfo ServerInfo = new PerforceServerInfo( "p4-server:1666", "build_machine", DB.GetStringForBranch( "UnrealEngine3", "P4Password" ) );
			SCC.GetClientRoot( ServerInfo );
			Log( "Running from root folder '" + RootFolder + "'", Color.DarkGreen );
		}

		public void Destroy()
		{
			UnbroadcastMachine();

			if( DB != null )
			{
				DB.CloseConnection();
			}
		}

		public Main()
		{
			InitializeComponent();
			MainLogWindow.Document = MainLogDoc;

			RootFolder = "";
			CompileDateTime = DateTime.UtcNow;
			MSVC8Version = "";
			MSVC9Version = "";
			DXVersion = "March (2009)";
			XDKVersion = "11164";
			PS3SDKVersion = "300.001";
			NumProcessors = 2;
			NumJobs = 4;
			MachineLock = Int32.MaxValue;
			JobSpawnTime = 0;
			NumJobsToWaitFor = 0;
			NumCompletedJobs = 0;
			SystemLog = null;
		}

		public void Log( string Line, Color TextColour )
		{
			if( Line == null || !Ticking )
			{
				return;
			}

			// if we need to, invoke the delegate
			if( InvokeRequired )
			{
				Invoke( new DelegateAddLine( Log ), new object[] { Line, TextColour } );
				return;
			}

			// Not UTC as this is visible to users
			string FullLine = DateTime.Now.ToString( "HH:mm:ss" ) + ": " + Line;

			MainLogDoc.AppendText( TextColour, FullLine + Environment.NewLine );

			// Log it out to the system log, if there is one
			if( SystemLog != null )
			{
				SystemLog.WriteLine( FullLine );
			}

			CheckStatusUpdate( Line );
		}

		private void HandleWatchStatus( string Line )
		{
			string KeyName = "";
			long Value = -1;

			if( Line.StartsWith( "[WATCHSTART " ) )
			{
				KeyName = Line.Substring( "[WATCHSTART ".Length ).TrimEnd( "]".ToCharArray() );
				Watcher.WatchStart( KeyName );
				Value = -1;
			}
			else if( Line.StartsWith( "[WATCHSTOP]" ) )
			{
				Value = Watcher.WatchStop( ref KeyName );
			}
			else if( Line.StartsWith( "[WATCHTIME " ) )
			{
				KeyName = "";
				string KeyValue = Line.Substring( "[WATCHTIME ".Length ).TrimEnd( "]".ToCharArray() );

				string[] Split = Builder.SafeSplit( KeyValue );
				if( Split.Length == 2 )
				{
					KeyName = Split[0];
					Value = Builder.SafeStringToInt( Split[1] );
				}
			}

			// Write the perf data to the database
			DB.WritePerformanceData( MachineName, KeyName, Value );
		}

		public void AddPerfData( string Line )
		{
			string Key = "";
			string Value = "";

			string[] KeyValue = Builder.SafeSplit( Line );

			foreach( string Name in KeyValue )
			{
				if( Name.Length > 0 )
				{
					if( Key.Length == 0 )
					{
						Key = Name;
					}
					else if( Value.Length == 0 )
					{
						Value = Name;
					}
					else
					{
						break;
					}
				}
			}

			// Write the perf data to the database
			long LongValue = Builder.SafeStringToLong( Value );
			DB.WritePerformanceData( MachineName, Key, LongValue );
		}

		public void CheckStatusUpdate( string Line )
		{
			if( Line == null || !Ticking || BuildLogID == 0 )
			{
				return;
			}

			// Handle any special controls
			if( Line.StartsWith( "[STATUS] " ) )
			{
				Line = Line.Substring( "[STATUS] ".Length );
				SetStatus( Line.Trim() );
			}
			else if( Line.IndexOf( "------ " ) >= 0 )
			{
				Line = Line.Substring( Line.IndexOf( "------ " ) + "------ ".Length );
				Line = Line.Substring( 0, Line.IndexOf( " ------" ) );
				SetStatus( Line.Trim() );
			}
			else if( Line.StartsWith( "[WATCH" ) )
			{
				HandleWatchStatus( Line );
			}

			if( Line.IndexOf( "=> NETWORK " ) >= 0 )
			{
				MailGlitch();
			}
		}

		private long GetDirectorySize( string Directory )
		{
			long CurrentSize = 0;

			DirectoryInfo DirInfo = new DirectoryInfo( Directory );
			if( DirInfo.Exists )
			{
				FileInfo[] Infos = DirInfo.GetFiles();
				foreach( FileInfo Info in Infos )
				{
					if( Info.Exists )
					{
						CurrentSize += Info.Length;
					}
				}

				DirectoryInfo[] SubDirInfo = DirInfo.GetDirectories();
				foreach( DirectoryInfo Info in SubDirInfo )
				{
					if( Info.Exists )
					{
						CurrentSize += GetDirectorySize( Info.FullName );
					}
				}
			}

			return ( CurrentSize );
		}

		private string GetDirectoryStatus( ScriptParser Builder )
		{
			StringBuilder Status = new StringBuilder();

			try
			{
				string[] Parms = Builder.SplitCommandline();
				long Total = 0;

				foreach( string Directory in Parms )
				{
					long BytesUsed = GetDirectorySize( Directory ) / ( 1024 * 1024 * 1024 );
					Total += BytesUsed;

					Status.Append( "'" + Directory + "' consumes " + BytesUsed.ToString() + " GB" + Environment.NewLine );
				}

				float TotalTB = ( float )( Total / 1024.0 );
				Status.Append( Environment.NewLine + "All builds consume " + TotalTB.ToString( "#.00" ) + " TB" + Environment.NewLine );
				Status.Append( "Build drive is 3.63TB in total" + Environment.NewLine );
			}
			catch( Exception Ex )
			{
				Status = new StringBuilder();
				Status.Append( "ERROR while interrogating directories" + Environment.NewLine + Ex.ToString() );
			}
			return ( Status.ToString() );
		}

		public void MailGlitch()
		{
			if( InvokeRequired )
			{
				Invoke( new DelegateMailGlitch( MailGlitch ), new object[] { } );
				return;
			}

			Mailer.SendGlitchMail();
		}

		public void KillProcess( string ProcessName )
		{
			try
			{
				Process[] ChildProcesses = Process.GetProcessesByName( ProcessName );
				foreach( Process ChildProcess in ChildProcesses )
				{
					Log( " ... killing: '" + ChildProcess.ProcessName + "'", Color.Red );
					ChildProcess.Kill();
				}
			}
			catch
			{
				Log( " ... killing failed", Color.Red );
			}
		}

		public void SetStatus( string Line )
		{
			// if we need to, invoke the delegate
			if( InvokeRequired )
			{
				Invoke( new DelegateSetStatus( SetStatus ), new object[] { Line } );
				return;
			}

			if( Line.Length > 127 )
			{
				Line = Line.Substring( 0, 127 );
			}

			DB.SetString( "BuildLog", BuildLogID, "CurrentStatus", Line );
		}

		public void Log( Array Lines, Color TextColour )
		{
			foreach( string Line in Lines )
			{
				Log( Line, TextColour );
			}
		}

		public string ExpandCommandParameter( string Input, string Parameter, string TableEntry )
		{
			while( Input.Contains( Parameter ) )
			{
				Input = Input.Replace( Parameter, DB.GetString( "Commands", CommandID, TableEntry ) );
			}

			return ( Input );
		}

		public string ExpandJobParameter( string Input, string Parameter, string TableEntry )
		{
			while( Input.Contains( Parameter ) )
			{
				Input = Input.Replace( Parameter, DB.GetString( "Jobs", JobID, TableEntry ) );
			}

			return ( Input );
		}

		public string ExpandString( string Input, string Branch )
		{
			// Expand predefined constants
			Input = ExpandCommandParameter( Input, "%Game%", "Game" );
			Input = ExpandCommandParameter( Input, "%Platform%", "Platform" );
			Input = ExpandCommandParameter( Input, "%Config%", "Config" );
			Input = ExpandCommandParameter( Input, "%Language%", "Language" );
			Input = ExpandCommandParameter( Input, "%DatabaseParameter%", "Parameter" );
			Input = Input.Replace( "%Branch%", Branch );

			Input = ExpandJobParameter( Input, "%JobPlatform%", "Platform" );
			Input = ExpandJobParameter( Input, "%JobGame%", "Game" );
			Input = ExpandJobParameter( Input, "%JobConfig%", "Config" );
			Input = ExpandJobParameter( Input, "%JobScriptConfig%", "ScriptConfig" );
			Input = ExpandJobParameter( Input, "%JobLanguage%", "Language" );
			Input = ExpandJobParameter( Input, "%JobDefine%", "Define" );
			Input = ExpandJobParameter( Input, "%JobParameter%", "Parameter" );
			Input = ExpandJobParameter( Input, "%JobLabel%", "Label" );
			Input = Input.Replace( "%JobBranch%", Branch );

			// Expand variables from variables table
			string[] Parms = Builder.SafeSplit( Input );
			string Operator = DB.GetString( "Commands", CommandID, "Operator" );
			for( int i = 0; i < Parms.Length; i++ )
			{
				if( Parms[i].StartsWith( "#" ) && Parms[i] != "#head" )
				{
					// Special handling for the verification label
					if( Parms[i].ToLower() == "#verificationlabel" && Operator != "AutoTimer" )
					{
						Parms[i] = "#head";
					}
					else
					{
						// .. otherwise just look up the variable in the database
						Parms[i] = DB.GetVariable( Branch, Parms[i].Substring( 1 ) );
					}
				}
			}

			// Reconstruct Command line
			string Line = "";
			foreach( string Parm in Parms )
			{
				Line += Parm + " ";
			}

			return ( Line.Trim() );
		}

		private string CommonPath
		{
			get
			{
				string ProgramFilesFolder = "C:\\Program Files (x86)";
				string PS3Root = Environment.GetEnvironmentVariable( "SCE_PS3_ROOT" );
				string SNCommonPath = Environment.GetEnvironmentVariable( "SN_COMMON_PATH" );
				string SNPS3Path = Environment.GetEnvironmentVariable( "SN_PS3_PATH" );
				string XEDKPath = Environment.GetEnvironmentVariable("XEDK");
				string SCERootPath = Environment.GetEnvironmentVariable("SCE_ROOT_DIR");

				string SearchPath = "";
				if( SNCommonPath != null )
				{
					SearchPath += Path.Combine( SNCommonPath, "VSI\\bin" ) + ";";
					SearchPath += Path.Combine( SNCommonPath, "bin" ) + ";";
				}

				if( SNPS3Path != null )
				{
					SearchPath += Path.Combine( SNPS3Path, "bin" ) + ";";
				}

				if( SCERootPath != null )
				{
					SearchPath += Path.Combine( SCERootPath, "PSP2\\Tools\\Target Manager Server\\bin" ) + ";";
				}

				if( XEDKPath != null )
				{
					SearchPath += Path.Combine( XEDKPath, "bin\\win32" ) + ";";
				}

				if( PS3Root != null )
				{
					SearchPath += Path.Combine( PS3Root, "\\host-win32\\bin" ) + ";";
					SearchPath += Path.Combine( PS3Root, "\\host-win32\\ppu\\bin" ) + ";";
					SearchPath += Path.Combine( PS3Root, "\\host-win32\\spu\\bin" ) + ";";
					SearchPath += Path.Combine( PS3Root, "\\host-win32\\Cg\\bin" ) + ";";
				}

				SearchPath += ProgramFilesFolder + "\\NVIDIA Corporation\\PhysX\\Common;";
				SearchPath += ProgramFilesFolder + "\\NVIDIA Corporation\\PhysX\\Common64;";
				SearchPath += ProgramFilesFolder + "\\Microsoft DirectX SDK (" + DXVersion + ")\\Utilities\\Bin\\x86;";
				SearchPath += ProgramFilesFolder + "\\Microsoft SQL Server\\90\\Tools\\binn\\;";
				SearchPath += ProgramFilesFolder + "\\Perforce;";
				SearchPath += "C:\\Perl\\bin\\;";
				SearchPath += "C:\\Perl64\\bin\\;";
				SearchPath += ProgramFilesFolder + "\\Windows Imaging\\;";
				SearchPath += ProgramFilesFolder + "\\Xoreax\\IncrediBuild;";
				SearchPath += "C:\\Program Files\\System Center Operations Manager 2007\\;";
				SearchPath += "C:\\Windows\\system32;";
				SearchPath += "C:\\Windows;";
				SearchPath += "C:\\Windows\\System32\\Wbem;";

				return ( SearchPath );
			}
		}

		private string VC8Path
		{
			get
			{
				string ProgramFilesFolder = "C:\\Program Files (x86)";

				string SearchPath = ProgramFilesFolder + "\\Microsoft Visual Studio 8\\Common7\\IDE;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\BIN;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 8\\Common7\\Tools;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 8\\Common7\\Tools\\bin;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\PlatformSDK\\bin;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 8\\SDK\\v2.0\\bin;";
				SearchPath += "C:\\Windows\\Microsoft.NET\\Framework\\v2.0.50727;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\VCPackages;";

				return ( SearchPath );
			}
		}

		private string VC9Path
		{
			get
			{
				string ProgramFilesFolder = "C:\\Program Files (x86)";

				string SearchPath = ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\Common7\\IDE;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\BIN;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\Common7\\Tools;";
				SearchPath += "C:\\Program Files\\Microsoft SDKs\\Windows\\v6.0A\\bin;";
				SearchPath += "C:\\Windows\\Microsoft.NET\\Framework\\v3.5;";
				SearchPath += "C:\\Windows\\Microsoft.NET\\Framework\\v2.0.50727;";
				SearchPath += ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\VCPackages;";

				return ( SearchPath );
			}
		}

		public void SetMSVC8EnvVars()
		{
			string ProgramFilesFolder = "C:\\Program Files (x86)";

			Environment.SetEnvironmentVariable( "VSINSTALLDIR", ProgramFilesFolder + "\\Microsoft Visual Studio 8" );
			Environment.SetEnvironmentVariable( "VCINSTALLDIR", ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC" );

			Environment.SetEnvironmentVariable( "INCLUDE", ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\ATLMFC\\INCLUDE;" + ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\INCLUDE;" + ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\PlatformSDK\\include;" + ProgramFilesFolder + "\\Microsoft Visual Studio 8\\SDK\\v2.0\\include;" );
			Environment.SetEnvironmentVariable( "LIB", ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\ATLMFC\\LIB;" + ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\LIB;" + ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\PlatformSDK\\lib;" + ProgramFilesFolder + "\\Microsoft Visual Studio 8\\SDK\\v2.0\\lib;" );
			Environment.SetEnvironmentVariable( "LIBPATH", "C:\\Windows\\Microsoft.NET\\Framework\\v2.0.50727;" + ProgramFilesFolder + "\\Microsoft Visual Studio 8\\VC\\ATLMFC\\LIB;" );
			Environment.SetEnvironmentVariable( "Path", VC8Path + CommonPath );
		}

		public void SetMSVC9EnvVars()
		{
			string ProgramFilesFolder = "C:\\Program Files (x86)";

			Environment.SetEnvironmentVariable( "VSINSTALLDIR", ProgramFilesFolder + "\\Microsoft Visual Studio 9.0" );
			Environment.SetEnvironmentVariable( "VCINSTALLDIR", ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC" );

			Environment.SetEnvironmentVariable( "INCLUDE", ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\ATLMFC\\INCLUDE;" + ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\INCLUDE;C:\\Program Files\\Microsoft SDKs\\Windows\\v6.0A\\include;" );
			Environment.SetEnvironmentVariable( "LIB", ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\ATLMFC\\LIB;" + ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\LIB;C:\\Program Files\\Microsoft SDKs\\Windows\\v6.0A\\lib;" );
			Environment.SetEnvironmentVariable( "LIBPATH", "C:\\Windows\\Microsoft.NET\\Framework\\v3.5;C:\\Windows\\Microsoft.NET\\Framework\\v2.0.50727;" + ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\ATLMFC\\LIB;" + ProgramFilesFolder + "\\Microsoft Visual Studio 9.0\\VC\\LIB;" );
			Environment.SetEnvironmentVariable( "Path", VC9Path + CommonPath );
		}

		private void SetDXSDKEnvVar()
		{
			string DXSDKLocation = "C:\\Program Files (x86)\\Microsoft DirectX SDK (" + DXVersion + ")\\";

			Environment.SetEnvironmentVariable( "DXSDK_DIR", DXSDKLocation );
			// Log( "Set DXSDK_DIR='" + DXSDKLocation + "'", Color.DarkGreen );
		}

		private void SetIPhoneEnvVar()
		{
			// All of the special incantations necessary for iPhone building and signing remotely on a Mac
			Environment.SetEnvironmentVariable( "ue3.iPhone_CompileServerName", iPhone_CompileServerName );
			Environment.SetEnvironmentVariable( "ue3.iPhone_SigningServerName", iPhone_SigningServerName );

			// Special cases for individual games
			if( Builder.BranchDef.Branch == "UnrealEngine3-ProjectSword" )
			{
				Environment.SetEnvironmentVariable( "ue3.iPhone_DeveloperSigningIdentity", "iPhone Developer: Mike Capps (DZ9JEF33YR)" );
				Environment.SetEnvironmentVariable( "ue3.iPhone_DistributionSigningIdentity", "iPhone Distribution: Chair" );
			}
			else
			{
				Environment.SetEnvironmentVariable( "ue3.iPhone_DeveloperSigningIdentity", "iPhone Developer: Mike Capps (E6TD75QUE5)" );
				Environment.SetEnvironmentVariable( "ue3.iPhone_DistributionSigningIdentity", "iPhone Distribution: Epic Games" );
			}
		}

		private void SetNGPSDKEnvVar()
		{
			string SCERootLocation = "C:\\Program Files (x86)\\SCE\\";

			// TODO: replace the hard-coded SDK version with a per-branch value
			string NGPSDKLocation = "C:\\Program Files (x86)\\SCE\\PSP2 SDKs\\0.945";

			Environment.SetEnvironmentVariable( "SCE_ROOT_DIR", SCERootLocation );
			Environment.SetEnvironmentVariable( "SCE_PSP2_SDK_DIR", NGPSDKLocation );
		}

		private void TrackFileSize()
		{
			try
			{
				FileInfo Info = new FileInfo( Builder.CommandLine );
				if( Info.Exists )
				{
					string KeyName = Builder.CommandLine.Replace( '/', '_' );
					if( KeyName.Length > 0 )
					{
						DB.WritePerformanceData( MachineName, KeyName, Info.Length );
					}
				}
			}
			catch
			{
			}
		}

		private void RecursiveGetFolderSize( DirectoryInfo Path, ref long TotalBuildSize )
		{
			foreach( FileInfo File in Path.GetFiles() )
			{
				TotalBuildSize += File.Length;
			}

			foreach( DirectoryInfo Dir in Path.GetDirectories() )
			{
				RecursiveGetFolderSize( Dir, ref TotalBuildSize );
			}
		}

		private void TrackFolderSize()
		{
			try
			{
				DirectoryInfo Info = new DirectoryInfo( Builder.CommandLine );
				if( Info.Exists )
				{
					string KeyName = Builder.CommandLine.Replace( '/', '_' );
					if( KeyName.Length > 0 )
					{
						long TotalBuildSize = 0;
						RecursiveGetFolderSize( Info, ref TotalBuildSize );
						DB.WritePerformanceData( MachineName, KeyName, TotalBuildSize );
					}
				}
			}
			catch
			{
			}
		}

		// Any cleanup that needs to happen when the build fails
		private void FailCleanup()
		{
			if( Builder.NewLabelCreated )
			{
				string LabelName = Builder.LabelInfo.GetLabelName();
				// Kill any jobs that are dependent on this label
				DB.KillAssociatedJobs( LabelName );

				// Delete the partial label so that no one can get a bad sync
				SCC.DeleteLabel( Builder, LabelName );
				Builder.NewLabelCreated = false;
			}
		}

		// Cleanup that needs to happen on success or failure
		private void Cleanup()
		{
			// Revert all open files
			CurrentCommand = new Command( this, SCC, Builder );
			CurrentCommand.Execute( COMMANDS.SCC_Revert );

			bool LoopingCommand = DB.GetBool( "Commands", CommandID, "Looping" );
			int LoopingCommandID = CommandID;

			DB.SetString( "Builders", BuilderID, "State", "Connected" );

			if( CommandID != 0 )
			{
				DB.SetString( "Commands", CommandID, "Machine", "" );
				DB.Delete( "Commands", CommandID, "Killing" );
				DB.Delete( "Commands", CommandID, "BuildLogID" );
				DB.Delete( "Commands", CommandID, "ConchHolder" );
			}
			else if( JobID != 0 )
			{
				DB.SetBool( "Jobs", JobID, "Complete", true );
			}

			// Set the DB up with the result of the build
			DB.SetString( "BuildLog", BuildLogID, "CurrentStatus", FinalStatus );

			// Remove any active watchers
			Watcher = new Watcher( this );

			FinalStatus = "";
			CurrentCommand = null;
			Builder.Destroy();
			Builder = null;
			BlockingBuildID = 0;
			BlockingBuildLogID = 0;
			Promotable = 0;
			JobID = 0;
			NumJobsToWaitFor = 0;
			CommandID = 0;
			BuilderID = 0;
			BuildLogID = 0;

			if( SystemLog != null )
			{
				SystemLog.Close();
				SystemLog = null;
			}

			if( LoopingCommand )
			{
				DB.Trigger( LoopingCommandID );
			}

			TimeSpan OneHour = new TimeSpan( 1, 0, 0 );
			LastSystemDownTime = DateTime.UtcNow - OneHour;
		}

		/** 
		 * Send any emails based on the current state of the build
		 */
		private void HandleEmails()
		{
			if( Builder.IsPromoting )
			{
				// There was a 'tag' command in the script
				Mailer.SendPromotedMail( Builder, CommandID );
			}
			else if( Builder.IsPublishing )
			{
				// There was a 'publish' command in the script
				Mailer.SendPublishedMail( Builder, CommandID, BuildLogID );
			}
			else if( Builder.IsBuilding )
			{
				// There was a 'submit' command in the script
				Mailer.SendSucceededMail( Builder, CommandID, BuildLogID, Builder.GetChanges( "" ) );
			}
			else if( Builder.IsSendingQAChanges )
			{
				string[] PerforceUsers = Builder.GetPerforceUsers();
				foreach( string User in PerforceUsers )
				{
					Mailer.SendUserChanges( Builder, User );
				}
			}
			else if( Builder.IsPrimaryBuild )
			{
				if( Builder.SuccessAddress.Length > 0 )
				{
					Mailer.SendSucceededMail( Builder, CommandID, BuildLogID, Builder.GetChanges( "" ) );
				}
			}
			else if( !Builder.IsPrimaryBuild && CommandID != 0 )
			{
				// For verification builds only, if the last time we attempted this build it did not succeed but it has succeeded this time,
				// send an email notifying the appropriate parties that the build now passes
				if( Builder.LastAttemptedBuild != Builder.LastGoodBuild )
				{
					Mailer.SendNewSuccessMail( Builder, CommandID, BuildLogID, Builder.GetChanges( "" ) );
				}
			}
		}

		private MODES HandleComplete()
		{
			DB.SetDateTime( "BuildLog", BuildLogID, "BuildEnded", DateTime.UtcNow );

			if( CommandID != 0 )
			{
				HandleEmails();

				if( Builder.LabelInfo.Changelist > Builder.LastGoodBuild )
				{
					DB.SetLastGoodBuild( CommandID, Builder.LabelInfo.Changelist, DateTime.UtcNow );
					DB.SetLastAttemptedBuild( CommandID, Builder.LabelInfo.Changelist, DateTime.UtcNow );

					string Label = Builder.LabelInfo.GetLabelName();
					if( Builder.NewLabelCreated )
					{
						DB.SetString( "Commands", CommandID, "LastGoodLabel", Label );
						DB.SetString( "BuildLog", BuildLogID, "BuildLabel", Label );
					}

					if( Builder.IsPromoting )
					{
						DB.SetString( "Commands", CommandID, "LastGoodLabel", Label );
					}
				}
			}
			else if( JobID != 0 )
			{
				DB.SetBool( "Jobs", JobID, "Succeeded", true );
				DB.SetBool( "Jobs", JobID, "Suppressed", Builder.HasSuppressedErrors );
			}

			FinalStatus = "Succeeded";
			return ( MODES.Exit );
		}

		public string GetLabelToSync()
		{
			// If we have a valid label - use that
			if( Builder.LabelInfo.RevisionType == RevisionType.Label )
			{
				string Label = Builder.LabelInfo.GetLabelName();
				Log( "Label revision: @" + Label, Color.DarkGreen );
				return ( "@" + Label );
			}
			else
			{
				// ... or there may just be a changelist
				int Changelist = Builder.LabelInfo.Changelist;
				if( Changelist != 0 )
				{
					Log( "Changelist revision: @" + Changelist.ToString(), Color.DarkGreen );
					return ( "@" + Changelist.ToString() );
				}
			}

			// No label to sync - sync to head
			Log( "Invalid or nonexistent label, default: #head", Color.DarkGreen );
			return ( "#head" );
		}

		public string GetChangeListToSync()
		{
			if( Builder.LabelInfo.RevisionType == RevisionType.Label )
			{
				string Changelist = Builder.LabelInfo.Changelist.ToString();
				Log( "Changelist revision: @" + Changelist, Color.DarkGreen );
				return ( "@" + Changelist );
			}

			// No label to sync - grab the changelist when we synced to #head
			if( Builder.HeadChangelist < 0 )
			{
				Builder.HeadChangelist = SCC.GetLatestChangelist( Builder, "//" + MachineName + "/" + Builder.BranchDef.Branch + "/..." );
				Builder.LabelInfo.Changelist = Builder.HeadChangelist;
			}

			Log( "Using changelist at time of sync #head command", Color.DarkGreen );
			return ( "@" + Builder.HeadChangelist.ToString() );
		}

		private string GetTaggedMessage()
		{
			// Refresh the label info
			SCC.GetLabelInfo( Builder, Builder.LabelInfo.GetLabelName(), Builder.LabelInfo );

			string FailureMessage = Builder.LabelInfo.Description;
			int Index = Builder.LabelInfo.Description.IndexOf( "Job failed" );
			if( Index > 0 )
			{
				FailureMessage = FailureMessage.Substring( Index );
			}
			return ( FailureMessage );
		}

		private string CreateJobUpdateMessage( string Name, string Command, string Platform, string Game, string BuildConfiguration, string ScriptConfiguration, string Language, string Define, string Parameter,
												string Branch, string LockPDK, string LockXDK, string Dependency, string PrimaryBuild, long JobSpawnTime )
		{
			string CommandString = " '" + Name + "',";
			CommandString += " '" + Command + "',";
			CommandString += " '" + Platform + "',";
			CommandString += " '" + Game + "',";
			CommandString += " '" + BuildConfiguration + "',";
			CommandString += " '" + ScriptConfiguration + "',";
			CommandString += " '" + Language + "',";
			CommandString += " '" + Define + "',";
			CommandString += " '" + Parameter + "',";
			CommandString += " '" + Branch + "',";
			CommandString += " " + LockPDK + ",";
			CommandString += " " + LockXDK + ",";
			CommandString += " '" + Dependency + "',";
			CommandString += " '',";
			CommandString += " 0, " + PrimaryBuild + ", 0, 0, 0, 0, 0,";
			CommandString += " " + JobSpawnTime.ToString();

			return ( CommandString );
		}

		public void AddJob( JobInfo Job )
		{
			string LockXDK = "1";
			string LockPDK = "1";

			switch( Builder.LabelInfo.Platform.ToLower() )
			{
			case "xenon":
			case "xbox360":
				LockPDK = "0";
				break;

			case "ps3":
			case "sonyps3":
				LockXDK = "0";
				break;

			case "pc":
			case "win32":
			case "win64":
				// As the Windows binaries link in the tools from various SDKs, they are dependent on them
				break;
			}

			string CommandString = "INSERT INTO Jobs ( Name, Command, Platform, Game, Config, " +
								   "ScriptConfig, Language, Define, Parameter, " +
								   "Branch, LockPDK, LockXDK, Label, Machine, BuildLogID, " +
								   "PrimaryBuild, Active, Complete, Succeeded, Suppressed, Killing, SpawnTime ) VALUES ( ";

			string Define = Builder.LabelInfo.GetDefinesAsString();

			CommandString += CreateJobUpdateMessage( Job.Name, Job.Command, Builder.LabelInfo.Platform, Builder.LabelInfo.Game, Builder.BuildConfiguration,
													 Builder.ScriptConfiguration, Builder.LabelInfo.Language, Define, Job.Parameter,
													 Builder.BranchDef.Branch, LockPDK, LockXDK, Builder.Dependency, "1", JobSpawnTime );
			CommandString += " )";

			DB.Update( CommandString );

			NumJobsToWaitFor++;

			Log( "Added job: " + Job.Name, Color.DarkGreen );
		}

		// Kill an in progress build
		private void KillBuild( int ID )
		{
			if( CommandID > 0 && CommandID == ID )
			{
				Log( "[STATUS] Killing build ...", Color.Red );

				// Kill the active command
				if( CurrentCommand != null )
				{
					CurrentCommand.Kill();
				}

				// Kill all associated jobs
				string CommandString = "UPDATE [Jobs] SET Killing = 1 WHERE ( SpawnTime = " + JobSpawnTime.ToString() + " )";
				DB.Update( CommandString );

				// Clean up
				FailCleanup();

				Mode = MODES.Exit;
				Log( "Process killed", Color.Red );

				string Killer = DB.GetString( "Commands", CommandID, "Killer" );
				Mailer.SendKilledMail( Builder, CommandID, BuildLogID, Killer );

				Cleanup();
			}
		}

		private void KillJob( int ID )
		{
			if( JobID > 0 && JobID == ID )
			{
				Log( "[STATUS] Killing job ...", Color.Red );

				if( CurrentCommand != null )
				{
					CurrentCommand.Kill();
				}

				Mode = MODES.Exit;
				Log( "Process killed", Color.Red );

				Cleanup();
			}
		}

		private void CleanupRogueProcesses()
		{
			// Kill the crash dialog
			KillProcess( "WerFault" );
			// Kill the msdev crash dialog if it exists
			KillProcess( "DW20" );
			// Kill the ProDG compiler if necessary
			KillProcess( "vsimake" );
			// Kill the command prompt
			KillProcess( "cmd" );
			// Kill the autoreporter which mysteriously stays open now
			KillProcess( "Autoreporter" );
			// Kill the pdb handler
			KillProcess( "mspdbsrv" );
			// Kill the linker
			KillProcess( "link" );
			// Kill the RPCUtility
			KillProcess( "rpcutility" );
			// Kill the XGE BuildSystem (should auto shutdown after the build is done)
			KillProcess( "BuildSystem" );
			// Kill the iPhonePackager used during iPhone builds
			KillProcess( "iPhonePackager" );
		}

		private void SpawnBuild( int ID )
		{
			string CommandString;

			// Before we spawn any work, make sure all rogue processes are killed off
			CleanupRogueProcesses();

			SetMSVC9EnvVars();

#if !DEBUG
			DeleteDirectory( "C:\\Builds", 0 );
#endif
			EnsureDirectoryExists( "C:\\Builds" );

#if !DEBUG
			DeleteDirectory( "C:\\Install", 0 );
#endif
			EnsureDirectoryExists( "C:\\Install" );

			string TempFolder = Environment.GetEnvironmentVariable( "TEMP" );
			DeleteDirectory( TempFolder, 0 );

			CommandID = ID;

			DateTime BuildStarted = DateTime.UtcNow;
			CompileRetryCount = 0;
			string Script = DB.GetString( "Commands", CommandID, "Command" );
			string Description = DB.GetString( "Commands", CommandID, "Description" );
			string BranchName = DB.GetString( "Commands", CommandID, "Branch" );
			BranchDefinition BranchDef = FindBranchDefinition( BranchName );

			if( BranchDef == null )
			{
				Log( "ERRROR: Could not find branch in available branches", Color.Red );
				Mode = MODES.Exit;
				return;
			}

			PerforceServerInfo ServerInfo = new PerforceServerInfo( DB, BranchDef.Branch );
			DXVersion = BranchDef.DirectXVersion;
			int LastAttemptedBuild = DB.GetInt( "Commands", CommandID, "LastAttemptedChangeList" );
			int LastGoodBuild = DB.GetInt( "Commands", CommandID, "LastGoodChangeList" );
			int LastFailedBuild = DB.GetInt( "Commands", CommandID, "LastFailedChangeList" );
			bool IsPrimaryBuild = DB.GetBool( "Commands", CommandID, "PrimaryBuild" );

			// Make sure we have the latest build scripts
			if( SCC.SyncBuildFiles( ServerInfo, BranchDef.Branch, "/Development/Builder/Scripts/..." ) )
			{
				Environment.CurrentDirectory = RootFolder + "\\" + BranchDef.Branch;

				EnsureDirectoryExists( "Development\\Builder\\Logs" );
				DeleteDirectory( "Development\\Builder\\Logs", 5 );

				// Make sure there are no pending kill commands
				DB.Delete( "Commands", CommandID, "Killing" );
				// Clear out killer (if any)
				DB.SetString( "Commands", CommandID, "Killer", "" );

				string Operator = DB.GetString( "Commands", CommandID, "Operator" );

				Builder = new ScriptParser( this, Script, Description, ServerInfo, BranchDef, IsPrimaryBuild, LastAttemptedBuild, LastGoodBuild, LastFailedBuild, BuildStarted, Operator );
				SystemLog = Builder.OpenSystemLog();

				CommandString = "SELECT [ID] FROM [Builders] WHERE ( Machine = '" + MachineName + "' AND State != 'Dead' AND State != 'Zombied' )";
				BuilderID = DB.ReadInt( CommandString );

				// Set builder to building
				DB.SetString( "Builders", BuilderID, "State", "Building" );

				// Add a new entry with the command
				CommandString = "INSERT INTO [BuildLog] ( BuildStarted ) VALUES ( '" + BuildStarted.ToString( US_TIME_DATE_STRING_FORMAT ) + "' )";
				DB.Update( CommandString );

				ProcedureParameter ParmTable = new ProcedureParameter( "TableName", "BuildLog", SqlDbType.Text );
				BuildLogID = DB.ReadInt( "GetNewID", ParmTable, null, null, null );

				if( BuildLogID != 0 )
				{
					CommandString = "UPDATE [BuildLog] SET CurrentStatus = 'Spawning', Machine = '" + MachineName + "', Command = '" + Script + "' WHERE ( ID = " + BuildLogID.ToString() + " )";
					DB.Update( CommandString );

					CommandString = "UPDATE [Commands] SET BuildLogID = " + BuildLogID.ToString() + ", Machine = '" + MachineName + "' WHERE ( ID = " + CommandID.ToString() + " )";
					DB.Update( CommandString );

					CommandString = "SELECT Promotable FROM [Commands] WHERE ( ID = " + CommandID.ToString() + " )";
					Promotable = DB.ReadInt( CommandString );
					DB.SetInt( "BuildLog", BuildLogID, "Promotable", Promotable );
				}

				JobSpawnTime = DateTime.UtcNow.Ticks;

				// Once we initialize everything, make sure we're starting with a clean slate by reverting everything that may be open for edit or delete.
				// Do this is easy way by simply enqueueing a revert as the first command.
				PendingCommands.Clear();
				PendingCommands.Enqueue( COMMANDS.SCC_Revert );

				Mode = MODES.Init;
			}
			else
			{
				Log( "ERROR: Invalid clientspec", Color.Red );
				Mailer.SendErrorMail( "Invalid Clientspec", "Description: " + Description + Environment.NewLine +
															  "Server: " + ServerInfo.Server + Environment.NewLine +
															  "SourceControlBranch: " + BranchDef.Branch + Environment.NewLine, false );
				Mode = MODES.Exit;
			}
		}

		private void SpawnJob( int ID )
		{
			string CommandString;

			// Before we spawn any work, make sure all rogue processes are killed off
			CleanupRogueProcesses();

			SetMSVC9EnvVars();

			DeleteDirectory( "C:\\Builds", 0 );
			EnsureDirectoryExists( "C:\\Builds" );

			DeleteDirectory( "C:\\Install", 0 );
			EnsureDirectoryExists( "C:\\Install" );

			JobID = ID;

			DateTime BuildStarted = DateTime.UtcNow;
			CompileRetryCount = 0;
			string JobName = DB.GetString( "Jobs", JobID, "Name" );
			string Script = DB.GetString( "Jobs", JobID, "Command" );
			string BranchName = DB.GetString( "Jobs", JobID, "Branch" );
			BranchDefinition BranchDef = FindBranchDefinition( BranchName );

			if( BranchDef == null )
			{
				Log( "ERROR: Could not find branch in available branches", Color.Red );
				Mode = MODES.Exit;
				return;
			}

			PerforceServerInfo ServerInfo = new PerforceServerInfo( DB, BranchDef.Branch );
			DXVersion = BranchDef.DirectXVersion;
			bool IsPrimaryBuild = DB.GetBool( "Jobs", JobID, "PrimaryBuild" );

			// Make sure we have the latest build scripts
			if( SCC.SyncBuildFiles( ServerInfo, BranchDef.Branch, "/Development/Builder/Scripts/..." ) )
			{
				Environment.CurrentDirectory = RootFolder + "\\" + BranchDef.Branch;

				EnsureDirectoryExists( "Development\\Builder\\Logs" );
				DeleteDirectory( "Development\\Builder\\Logs", 5 );

				string Operator = DB.GetString( "Commands", CommandID, "Operator" );

				Builder = new ScriptParser( this, Script, JobName, ServerInfo, BranchDef, IsPrimaryBuild, 0, 0, 0, BuildStarted, Operator );

				CommandString = "SELECT [ID] FROM [Builders] WHERE ( Machine = '" + MachineName + "' AND State != 'Dead' AND State != 'Zombied' )";
				BuilderID = DB.ReadInt( CommandString );

				// Set builder to building
				DB.SetString( "Builders", BuilderID, "State", "Building" );

				// Add a new entry with the command
				CommandString = "INSERT INTO [BuildLog] ( BuildStarted ) VALUES ( '" + BuildStarted.ToString( US_TIME_DATE_STRING_FORMAT ) + "' )";
				DB.Update( CommandString );

				ProcedureParameter ParmTable = new ProcedureParameter( "TableName", "BuildLog", SqlDbType.Text );
				BuildLogID = DB.ReadInt( "GetNewID", ParmTable, null, null, null );

				if( BuildLogID != 0 )
				{
					CommandString = "UPDATE [BuildLog] SET CurrentStatus = 'Spawning', Machine = '" + MachineName + "', Command = '" + Script + "' WHERE ( ID = " + BuildLogID.ToString() + " )";
					DB.Update( CommandString );

					CommandString = "UPDATE [Jobs] SET BuildLogID = " + BuildLogID.ToString() + ", Machine = '" + MachineName + "' WHERE ( ID = " + JobID.ToString() + " )";
					DB.Update( CommandString );
				}

				// Grab game and platform
				CommandString = "SELECT [Game] FROM [Jobs] WHERE ( ID = " + JobID.ToString() + " )";
				Builder.LabelInfo.Game = DB.ReadString( CommandString );

				CommandString = "SELECT [Platform] FROM [Jobs] WHERE ( ID = " + JobID.ToString() + " )";
				Builder.LabelInfo.Platform = DB.ReadString( CommandString );

				Builder.Dependency = DB.GetString( "Jobs", JobID, "Label" );

				// Handle the special case of CIS tasks passing in a numerical changelist
				if( IsPrimaryBuild )
				{
					// Standard, non-CIS case using the dependency as a standard label or folder
					Builder.LabelInfo.Init( SCC, Builder );
				}
				else
				{
					// Convert the dependency label to an integer changelist
					Builder.LabelInfo.Init( Builder, Builder.SafeStringToInt( Builder.Dependency ) );
				}

				// Once we initialize everything, make sure we're starting with a clean slate by reverting everything that may be open for edit or delete.
				// Do this is easy way by simply enqueueing a revert as the first command.
				PendingCommands.Clear();
				PendingCommands.Enqueue( COMMANDS.SCC_Revert );

				Mode = MODES.Init;
			}
			else
			{
				Log( "ERRROR: Invalid clientspec", Color.Red );
				Mailer.SendErrorMail( "Invalid Clientspec", "Script: " + Script + Environment.NewLine +
															  "Server: " + ServerInfo.Server + Environment.NewLine +
															  "SourceControlBranch: " + BranchDef.Branch + Environment.NewLine, false );
				Mode = MODES.Exit;
			}
		}

		private MODES HandleError()
		{
			int TimeOutMinutes;
			bool CheckCookingSuccess = ( Builder.GetCommandID() == COMMANDS.CookMaps );
			bool CheckCookerSyncSuccess = ( Builder.GetCommandID() == COMMANDS.Publish );
			string GeneralStatus = "";
			string Status = "Succeeded";

			// Always free up the conch whenever a command finishes to let other builds finish
			DB.Delete( "Commands", CommandID, "ConchHolder" );
			Builder.StartWaitForConch = Builder.BuildStartedAt;

			// Internal error?
			COMMANDS ErrorLevel = Builder.GetErrorLevel();

			if( CurrentCommand != null && ErrorLevel == COMMANDS.None )
			{
				// ...or error that requires parsing the log
				ErrorLevel = CurrentCommand.GetErrorLevel();

				LogParser Parser = new LogParser( this, Builder );
				bool ReportEverything = ( ErrorLevel >= COMMANDS.SCC_ArtistSync && ErrorLevel <= COMMANDS.SCC_TagMessage );
				Status = Parser.Parse( ReportEverything, CheckCookingSuccess, CheckCookerSyncSuccess, ref ErrorLevel );
			}

#if !DEBUG
			// If we were cooking, and didn't find the cooking success message, set to fail
			if( CheckCookingSuccess )
			{
				if( ErrorLevel == COMMANDS.CookingSuccess )
				{
					ErrorLevel = COMMANDS.None;
				}
				else if( ErrorLevel == COMMANDS.None && Status == "Succeeded" )
				{
					ErrorLevel = COMMANDS.CookMaps;
					Status = "Could not find cooking successful message";
				}
			}
#endif

			// If we were publishing, and didn't find the publish success message, set to fail
			if( CheckCookerSyncSuccess )
			{
				if( ErrorLevel == COMMANDS.CookerSyncSuccess )
				{
					ErrorLevel = COMMANDS.None;
				}
				else if( ErrorLevel == COMMANDS.None && Builder.PublishMode == ScriptParser.PublishModeType.Files && Status == "Succeeded" )
				{
					ErrorLevel = COMMANDS.Publish;
					Status = "Could not find publish completion message";
				}
			}

			// Check for total success
			if( ErrorLevel == COMMANDS.None && Status == "Succeeded" )
			{
				LinkRetryCount = 0;
				return ( MODES.Init );
			}

			// If we were checking to see if a file was signed, conditionally add another command
			if( ErrorLevel == COMMANDS.CheckSigned )
			{
				// Error checking to see if file was signed - so sign it
				if( CurrentCommand.GetCurrentBuild().GetExitCode() != 0 )
				{
					PendingCommands.Enqueue( COMMANDS.Sign );
				}
				return ( MODES.Init );
			}

			// Auto retry the connection to the Verisign timestamp server
			if( Status.Contains( "The specified timestamp server could not be reached" ) )
			{
				Mailer.SendWarningMail( "Verisign connection failure", "The build machine failed to contact the Verisign timestamp server; retrying." + Environment.NewLine + Environment.NewLine + Status, false );
				Builder.LineCount++;
				PendingCommands.Enqueue( CurrentCommand.LastExecutedCommand );
				return ( MODES.Init );
			}

			// Deal with an XGE issue that causes some builds to fail immediately, but succeed on retry
			if( Status.Contains( "Fatal Error: Failed to initiate build" )
				|| Status.Contains( ": fatal error LNK1103:" )
				|| Status.Contains( ": fatal error LNK1106:" )
				|| Status.Contains( ": fatal error LNK1136:" )
				|| Status.Contains( ": fatal error LNK1143:" )
				|| Status.Contains( ": fatal error LNK1179:" )
				|| Status.Contains( ": fatal error LNK1183:" )
				|| Status.Contains( ": fatal error LNK1190:" )
				|| Status.Contains( ": fatal error LNK1215:" )
				|| Status.Contains( ": fatal error LNK1235:" )
				|| Status.Contains( ": fatal error LNK1248:" )
				|| Status.Contains( ": fatal error LNK1257:" )
				|| Status.Contains( ": fatal error LNK2022:" )
				|| Status.Contains( ": error: L0015:" )
				|| Status.Contains( ": error: L0072:" )
				|| Status.Contains( "corrupted section headers in file" )
				|| Status.Contains( ": warning LNK4019: corrupt string table" )
				|| Status.Contains( "Sorry but the link was not completed because memory was exhausted." )
				|| Status.Contains( "Another build is already started on this computer." )
				|| Status.Contains( "Failed to initialize Build System:" )
				|| Status.Contains( "simply rerunning the compiler might fix this problem" )
				|| Status.Contains( "Internal Linker Exception:" )
				// This message keeps appearing in windows ce builds - a rebuild seems to clean it up
				|| Status.Contains( ": unexpected error with pch, try rebuilding the pch" ) )
			{
				if( CompileRetryCount < 2 )
				{
					// Send a warning email
					if( CompileRetryCount == 0 )
					{
						// Restart the service and try again
						Mailer.SendWarningMail( "Compile (XGE) Failure #" + CompileRetryCount.ToString(), "XGE appears to have failed to start up; restarting the XGE service and trying again." + Environment.NewLine + Environment.NewLine + Status, false );

						Process ServiceStop = Process.Start( "net", "stop \"Incredibuild Agent\"" );
						ServiceStop.WaitForExit();

						Process ServiceStart = Process.Start( "net", "start \"Incredibuild Agent\"" );
						ServiceStart.WaitForExit();

						DB.WritePerformanceData( MachineName, "XGEServiceRestarted", 1 );
					}
					else
					{
						// Queue up a retry without XGE
						Mailer.SendWarningMail( "Compile (XGE) Failure #" + CompileRetryCount.ToString(), "XGE appears to have failed to start up; retrying without XGE entirely." + Environment.NewLine + Environment.NewLine + Status, false );
						Builder.bXGEHasFailed = true;

						DB.WritePerformanceData( MachineName, "XGEServiceFailed", 1 );
					}

					Builder.LineCount++;
					PendingCommands.Enqueue( COMMANDS.Clean );
					PendingCommands.Enqueue( COMMANDS.BuildUBT );
					PendingCommands.Enqueue( COMMANDS.UnrealBuild );

					CompileRetryCount++;
					return ( MODES.Init );
				}

				Status = Status + Environment.NewLine + Environment.NewLine + " ... after " + CompileRetryCount.ToString() + " retries with and without XGE";
			}

			// If we had a failure while linking due to being unable to access the file,
			// retry the build, which should end up being only a relink
			if( Status.Contains( "fatal error LNK1201: error writing to program database" ) ||
				Status.Contains( "fatal error LNK1104: cannot open file" ) ||
				Status.Contains( "error: L0055: could not open output file" ) )
			{
				LinkRetryCount++;
				if( LinkRetryCount < 2 )
				{
					// Send a warning email
					Mailer.SendWarningMail( "Link Failure #" + LinkRetryCount.ToString(), "Couldn't write to the output file, sleeping then retrying." + Environment.NewLine + Environment.NewLine + Status, false );
					// Sleep for 10 seconds
					System.Threading.Thread.Sleep( 10 * 1000 );

					// Queue up a retry
					Builder.LineCount++;
					PendingCommands.Enqueue( COMMANDS.UnrealBuild );

					return ( MODES.Init );
				}

				Status = Status + Environment.NewLine + Environment.NewLine + " ... after " + LinkRetryCount.ToString() + " link retries";
			}

			// Error found, but no description located. Explain this.
			if( Status == "Succeeded" )
			{
				Status = "Could not find error";
			}

			// Handle suppression and ignoring of errors
			switch( Builder.ErrorMode )
			{
			// Completely ignore the error and continue
			case ScriptParser.ErrorModeType.IgnoreErrors:
				return ( MODES.Init );

			// Ignore the error, but send an informative email
			case ScriptParser.ErrorModeType.SuppressErrors:
				Mailer.SendSuppressedMail( Builder, CommandID, BuildLogID, Status );
				Builder.HasSuppressedErrors = true;
				return ( MODES.Init );

			// Continue to handle the failure
			case ScriptParser.ErrorModeType.CheckErrors:
				break;
			}

			// Copy the failure log to a common network location
			FileInfo LogFile = new FileInfo( "None" );
			if( Builder.LogFileName.Length > 0 )
			{
				LogFile = new FileInfo( Builder.LogFileName );
				try
				{
					if( LogFile.Exists )
					{
						LogFile.CopyTo( Properties.Settings.Default.FailedLogLocation + "/" + LogFile.Name );
					}
				}
				catch
				{
				}
			}

			// If we wish to save the error string in the database, do so now
			if( Builder.ErrorSaveVariable.Length > 0 )
			{
				string ErrorMessage = Status.Replace( '\'', '\"' );
				ErrorMessage += Environment.NewLine + "<a href=\"file:///" + Properties.Settings.Default.FailedLogLocation.Replace( '\\', '/' ) + "/" + LogFile.Name + "\"> Click here for the detailed log </a>";

				string UpdateFolder = "UPDATE BranchConfig SET " + Builder.ErrorSaveVariable + " = '" + ErrorMessage + "' WHERE ( Branch = '" + Builder.BranchDef.Branch + "' )";
				DB.Update( UpdateFolder );
			}

			// Set the end time and detailed log path as we've now finished
			DB.SetDateTime( "BuildLog", BuildLogID, "BuildEnded", DateTime.UtcNow );
			DB.SetString( "BuildLog", BuildLogID, "DetailedLogPath", LogFile.Name );

			// Special work for non-CIS (Jobs) verification builds
			if( !Builder.IsPrimaryBuild && CommandID != 0 )
			{
				// Report only if we actually attempted something new, or if
				// this is a CIS verification job 
				if( Builder.LastAttemptedBuild == Builder.LabelInfo.Changelist )
				{
					ErrorLevel = COMMANDS.SilentlyFail;
				}
				else
				{
					// This is a new failure, so update the DB with the last CL
					// in this build to attempt to bookend the failure CL range
					// which is the CL this build was built from.
					DB.SetLastFailedBuild( CommandID, Builder.LabelInfo.Changelist, DateTime.UtcNow );
				}
			}

			// Try to explain the error
			string Explanation = LogParser.ExplainError( Status );
			if( Explanation.Length > 0 )
			{
				Status += Environment.NewLine;
				Status += Explanation;
			}

			// Be sure to update the DB with this attempt 
			DB.SetLastAttemptedBuild( CommandID, Builder.LabelInfo.Changelist, DateTime.UtcNow );

			// Handle specific errors
			switch( ErrorLevel )
			{
			case COMMANDS.None:
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );

				Log( "LOG ERROR: " + Builder.GetCommandID() + " " + Builder.CommandLine + " failed", Color.Red );
				Log( Status, Color.Red );
				break;

			case COMMANDS.SilentlyFail:
				Log( "LOG ERROR: " + Builder.GetCommandID() + " " + Builder.CommandLine + " failed", Color.Red );
				Log( "LOG ERROR: failing silently (without email) because this is a verification build that doesn't require direct reporting", Color.Red );
				Log( Status, Color.Red );
				break;

			case COMMANDS.NoScript:
				Status = "No build script";
				Log( "ERROR: " + Status, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );
				break;

			case COMMANDS.IllegalCommand:
				Status = "Illegal command: '" + Builder.CommandLine + "'";
				Log( "ERROR: " + Status, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );
				break;

			case COMMANDS.SCC_Submit:
			case COMMANDS.SCC_FileLocked:
			case COMMANDS.SCC_Sync:
			case COMMANDS.SCC_Checkout:
			case COMMANDS.SCC_Revert:
			case COMMANDS.SCC_Tag:
			case COMMANDS.SCC_GetClientRoot:
			case COMMANDS.UpdateSymbolServer:
			case COMMANDS.AddExeToSymbolServer:
			case COMMANDS.AddDllToSymbolServer:
			case COMMANDS.CriticalError:
			case COMMANDS.ValidateInstall:
				GeneralStatus = Builder.GetCommandID() + " " + Builder.CommandLine + " failed with error '" + ErrorLevel.ToString() + "'";
				GeneralStatus += Environment.NewLine + Environment.NewLine + Status;
				Log( "ERROR: " + GeneralStatus, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, GeneralStatus, LogFile.Name, "" );
				break;

			case COMMANDS.Process:
			case COMMANDS.MSVC8Clean:
			case COMMANDS.MSVC8Build:
			case COMMANDS.MSVC9Clean:
			case COMMANDS.MSVC9Build:
			case COMMANDS.MSVC9Deploy:
			case COMMANDS.GCCBuild:
			case COMMANDS.GCCClean:
			case COMMANDS.ShaderBuild:
			case COMMANDS.LocalShaderBuild:
			case COMMANDS.CookShaderBuild:
			case COMMANDS.ShaderClean:
			case COMMANDS.BuildScript:
			case COMMANDS.BuildScriptNoClean:
			case COMMANDS.CookMaps:
			case COMMANDS.CheckForUCInVCProjFiles:
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );
				break;

			case COMMANDS.Publish:
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "IT@epicgames.com" );
				break;

			case COMMANDS.CheckSigned:
				return ( MODES.Init );

			case COMMANDS.TimedOut:
				TimeOutMinutes = ( int )Builder.GetTimeout().TotalMinutes;
				GeneralStatus = "'" + Builder.GetCommandID() + " " + Builder.CommandLine + "' TIMED OUT after " + TimeOutMinutes.ToString() + " minutes";
				GeneralStatus += Environment.NewLine + Environment.NewLine + "(This normally means a child process crashed)";
				GeneralStatus += Environment.NewLine + Environment.NewLine + Status;
				Log( "ERROR: " + GeneralStatus, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, GeneralStatus, LogFile.Name, "" );
				break;

			case COMMANDS.WaitTimedOut:
				TimeOutMinutes = ( int )Builder.GetTimeout().TotalMinutes;
				Status = "Waiting for '" + Builder.CommandLine + "' TIMED OUT after " + TimeOutMinutes.ToString() + " minutes";
				Log( "ERROR: " + Status, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );
				break;

			case COMMANDS.WaitJobsTimedOut:
				TimeOutMinutes = ( int )Builder.GetTimeout().TotalMinutes;
				string WaitQuery = "SELECT COUNT( ID ) FROM Jobs WHERE ( SpawnTime = " + JobSpawnTime.ToString() + " AND Complete = 1 )";
				int JobCount = DB.ReadInt( WaitQuery );
				Status = "Waiting for " + ( NumJobsToWaitFor - JobCount ).ToString() + " job(s) out of " + NumJobsToWaitFor.ToString() + " TIMED OUT after " + TimeOutMinutes.ToString() + " minutes";
				Log( "ERROR: " + Status, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );
				break;

			case COMMANDS.FailedJobs:
				Status = "All jobs completed, but one or more failed." + Environment.NewLine + GetTaggedMessage();
				Log( "ERROR: " + Status, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );
				break;

			case COMMANDS.Crashed:
				int NotRespondingMinutes = ( int )Builder.GetRespondingTimeout().TotalMinutes;
				Status = "'" + Builder.GetCommandID() + " " + Builder.CommandLine + "' was not responding for " + NotRespondingMinutes.ToString() + " minutes; presumed crashed.";
				Log( "ERROR: " + Status, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );
				break;

			default:
				Status = "'" + Builder.GetCommandID() + " " + Builder.CommandLine + "' unhandled error '" + ErrorLevel.ToString() + "'";
				Log( "ERROR: " + Status, Color.Red );
				Mailer.SendFailedMail( Builder, CommandID, BuildLogID, Status, LogFile.Name, "" );
				break;
			}

			// Any cleanup that needs to happen only in a failure case
			FailCleanup();

			FinalStatus = "Failed";
			return ( MODES.Exit );
		}

		private bool RunBuild()
		{
			// Start off optimistic 
			bool StillRunning = true;
			string Query;
			switch( Mode )
			{
			case MODES.Init:
				COMMANDS NextCommand;

				CurrentCommand = null;

				// Get a new command ...
				if( PendingCommands.Count > 0 )
				{
					// ... either pending 
					NextCommand = PendingCommands.Dequeue();
					Builder.SetCommandID( NextCommand );
				}
				else
				{
					// ... or from the script
					NextCommand = Builder.ParseNextLine();
				}

				// Expand out any variables
				Builder.CommandLine = ExpandString( Builder.CommandLine, Builder.BranchDef.Branch );

				// Set up the required env
				if( NextCommand == COMMANDS.MSVC8Build || NextCommand == COMMANDS.MSVC8Clean || NextCommand == COMMANDS.MS8Build )
				{
					SetMSVC8EnvVars();
				}
				else if( NextCommand == COMMANDS.MSVC9Build || NextCommand == COMMANDS.MSVC9Clean || NextCommand == COMMANDS.MSVC9Deploy || NextCommand == COMMANDS.MS9Build )
				{
					SetMSVC9EnvVars();
				}

				SetDXSDKEnvVar();

				SetIPhoneEnvVar();

				SetNGPSDKEnvVar();

				switch( NextCommand )
				{
				case COMMANDS.Error:
					Mode = MODES.Finalise;
					break;

				case COMMANDS.Finished:
					Mode = HandleComplete();
					break;

				case COMMANDS.Config:
					break;

				case COMMANDS.TriggerMail:
					Mailer.SendTriggeredMail( Builder, CommandID );
					Mode = MODES.Finalise;
					break;

				case COMMANDS.SendEmail:
					HandleEmails();
					Mode = MODES.Finalise;
					break;

				case COMMANDS.Wait:
					Query = "SELECT ID FROM Commands WHERE ( Description = '" + Builder.CommandLine + "' )";
					BlockingBuildID = DB.ReadInt( Query );
					BlockingBuildStartTime = DateTime.UtcNow;
					Mode = MODES.Wait;
					break;

				case COMMANDS.WaitForActive:
					Query = "SELECT ID FROM Commands WHERE ( Description = '" + Builder.CommandLine + "' )";
					BlockingBuildID = DB.ReadInt( Query );
					BlockingBuildStartTime = DateTime.UtcNow;
					Log( "[STATUS] Waiting for '" + Builder.CommandLine + "' to start up (ID = " + BlockingBuildID.ToString() + ")", Color.Magenta );
					Mode = MODES.WaitForActive;
					break;

				case COMMANDS.WaitForJobs:
					Log( "[STATUS] Waiting for " + ( NumJobsToWaitFor - NumCompletedJobs ).ToString() + " jobs.", Color.Magenta );
					BlockingBuildStartTime = DateTime.UtcNow;
					Mode = MODES.WaitForJobs;
					break;

				case COMMANDS.SetDependency:
					string CommandQuery = "SELECT ID FROM Commands WHERE ( Description = '" + Builder.Dependency + "' )";
					int DependentCommandID = DB.ReadInt( CommandQuery );
					if( DependentCommandID == 0 )
					{
						Builder.LabelInfo.Init( SCC, Builder );
					}
					else
					{
						Builder.LabelInfo.Init( Builder, DB.GetInt( "Commands", DependentCommandID, "LastGoodChangeList" ) );
					}
					Mode = MODES.Finalise;
					break;

				case COMMANDS.RestartSteamServer:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.StopSteamServer );

					PendingCommands.Enqueue( COMMANDS.StartSteamServer );
					break;

				case COMMANDS.SteamDRM:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.SteamDRM );

					PendingCommands.Enqueue( COMMANDS.FixupSteamDRM );
					break;

				case COMMANDS.MSVC8Full:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.MSVC8Clean );

					PendingCommands.Enqueue( COMMANDS.MSVC8Build );
					break;

				case COMMANDS.MSVC9Full:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.MSVC9Clean );

					PendingCommands.Enqueue( COMMANDS.MSVC9Build );
					break;

				case COMMANDS.GCCFull:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.GCCClean );

					PendingCommands.Enqueue( COMMANDS.GCCBuild );
					break;

				case COMMANDS.ShaderFull:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.ShaderClean );

					PendingCommands.Enqueue( COMMANDS.ShaderBuild );
					break;

				case COMMANDS.SignBinary:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.SignBinary );
					break;

				case COMMANDS.SignCat:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.SignCat );
					break;

				case COMMANDS.UpdateSourceServer:
					if( Builder.LabelInfo.Platform.ToLower() != "ps3" && Builder.LabelInfo.Platform.ToLower() != "sonyps3" )
					{
						CurrentCommand = new Command( this, SCC, Builder );
						Mode = CurrentCommand.Execute( COMMANDS.UpdateSourceServer );

						PendingCommands.Enqueue( COMMANDS.UpdateSymbolServer );
					}
					else
					{
						Log( "Suppressing UpdateSourceServer for PS3 ", Color.DarkGreen );
						Mode = MODES.Finalise;
					}
					break;

				case COMMANDS.UpdateSymbolServer:
				case COMMANDS.AddExeToSymbolServer:
				case COMMANDS.AddDllToSymbolServer:
					if( Builder.LabelInfo.Platform.ToLower() != "ps3" && Builder.LabelInfo.Platform.ToLower() != "sonyps3" )
					{
						CurrentCommand = new Command( this, SCC, Builder );
						Mode = CurrentCommand.Execute( NextCommand );

						PendingCommands.Enqueue( COMMANDS.UpdateSymbolServerTick );
					}
					else
					{
						Log( "Suppressing UpdateSymbolServer for PS3 ", Color.DarkGreen );
						Mode = MODES.Finalise;
					}
					break;

				case COMMANDS.UpdateSymbolServerTick:
					if( Builder.LabelInfo.Platform.ToLower() != "ps3" && Builder.LabelInfo.Platform.ToLower() != "sonyps3" )
					{
						CurrentCommand = new Command( this, SCC, Builder );
						Mode = CurrentCommand.Execute( COMMANDS.UpdateSymbolServerTick );

						if( CurrentCommand.GetCurrentBuild() != null )
						{
							PendingCommands.Enqueue( COMMANDS.UpdateSymbolServerTick );
						}
					}
					else
					{
						Log( "Suppressing UpdateSymbolServerTick for PS3 ", Color.DarkGreen );
						Mode = MODES.Finalise;
					}
					break;

				case COMMANDS.Trigger:
					string BuildType = ExpandString( Builder.CommandLine, Builder.BranchDef.Branch );
					Log( "[STATUS] Triggering build '" + BuildType + "'", Color.Magenta );
					DB.Trigger( CommandID, BuildType );
					break;

				case COMMANDS.SQLExecInt:
					string SQLExecIntFunction = ExpandString( Builder.CommandLine, Builder.BranchDef.Branch );
					int SQLExecIntResult = DB.ReadInt( SQLExecIntFunction );
					Builder.AddToSuccessStatus( SQLExecIntFunction + " : " + SQLExecIntResult.ToString() );
					break;

				case COMMANDS.SQLExecDouble:
					string SQLExecDoubleFunction = ExpandString( Builder.CommandLine, Builder.BranchDef.Branch );
					double SQLExecDoubleResult = DB.ReadDouble( SQLExecDoubleFunction );
					Builder.AddToSuccessStatus( SQLExecDoubleFunction + " : " + SQLExecDoubleResult.ToString( "F2" ) );
					break;

				case COMMANDS.Conform:
					if( Builder.GetValidLanguages() == null || Builder.GetValidLanguages().Count < 2 )
					{
						// used for on-the-fly conforming
						// we require that the files exist or the commandlet will fail
						Queue<string> ValidLanguages = new Queue<string>( Builder.GetLanguages() );
						Builder.SetValidLanguages( ValidLanguages );
					}

					if( Builder.GetValidLanguages().Count > 2 )
					{
						PendingCommands.Enqueue( COMMANDS.Conform );
					}

					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( COMMANDS.Conform );
					break;

				case COMMANDS.SCC_Submit:
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( NextCommand );
					break;

				case COMMANDS.UpdateLabel:
					string LabelCmdLine = ExpandString( Builder.CommandLine, Builder.BranchDef.Branch );
					string UpdateLabel = "UPDATE Variables SET Value = '" + Builder.LabelInfo.GetLabelName() + "' WHERE ( Variable = '" + LabelCmdLine + "' AND Branch = '" + Builder.BranchDef.Branch + "' )";
					DB.Update( UpdateLabel );
					break;

				case COMMANDS.UpdateFolder:
					string FolderCmdLine = ExpandString( Builder.CommandLine, Builder.BranchDef.Branch );
					string UpdateFolder = "UPDATE Variables SET Value = '" + Builder.GetFolderName() + "' WHERE ( Variable = '" + FolderCmdLine + "' AND Branch = '" + Builder.BranchDef.Branch + "' )";
					DB.Update( UpdateFolder );
					break;

				case COMMANDS.Publish:
				case COMMANDS.FTPSendImage:
					if( Builder.BlockOnPublish )
					{
						if( !DB.AvailableBandwidth( CommandID ) )
						{
							PendingCommands.Enqueue( NextCommand );

							if( Builder.StartWaitForConch == Builder.BuildStartedAt )
							{
								Builder.StartWaitForConch = DateTime.UtcNow;
								Builder.LastWaitForConchUpdate = Builder.StartWaitForConch;

								Log( "[STATUS] Waiting for bandwidth ( 00:00:00 )", Color.Yellow );
							}
							else if( DateTime.UtcNow - Builder.LastWaitForConchUpdate > new TimeSpan( 0, 0, 5 ) )
							{
								Builder.LastWaitForConchUpdate = DateTime.UtcNow;
								TimeSpan Taken = DateTime.UtcNow - Builder.StartWaitForConch;
								Log( "[STATUS] Waiting for bandwidth ( " + Taken.Hours.ToString( "00" ) + ":" + Taken.Minutes.ToString( "00" ) + ":" + Taken.Seconds.ToString( "00" ) + " )", Color.Yellow );
							}
						}
						else
						{
							Log( "[STATUS] Bandwidth acquired - publishing/ftping!", Color.Magenta );

							CurrentCommand = new Command( this, SCC, Builder );
							Mode = CurrentCommand.Execute( NextCommand );
						}
					}
					else
					{
						CurrentCommand = new Command( this, SCC, Builder );
						Mode = CurrentCommand.Execute( NextCommand );
					}
					break;

				case COMMANDS.CISProcessP4Changes:
					ProcessP4Changes();
					Mode = MODES.Finalise;
					break;

				case COMMANDS.CISUpdateMonitorValues:
					UpdateMonitorValues();
					Mode = MODES.Finalise;
					break;

				case COMMANDS.TrackFileSize:
					TrackFileSize();
					break;

				case COMMANDS.TrackFolderSize:
					TrackFolderSize();
					break;

				default:
					// Fire off a new process safely
					CurrentCommand = new Command( this, SCC, Builder );
					Mode = CurrentCommand.Execute( NextCommand );

					// Handle the returned info (special case)
					if( NextCommand == COMMANDS.SCC_Sync )
					{
						DB.SetInt( "BuildLog", BuildLogID, "ChangeList", Builder.LabelInfo.Changelist );
					}
					break;
				}
				break;

			case MODES.Monitor:
				// Check for completion
				Mode = CurrentCommand.IsFinished();
				break;

			case MODES.Wait:
				if( BlockingBuildID != 0 )
				{
					// Has the child build been updated to the same build?
					int LastGoodChangeList = DB.GetInt( "Commands", BlockingBuildID, "LastGoodChangeList" );
					if( LastGoodChangeList >= Builder.LabelInfo.Changelist )
					{
						Mode = MODES.Finalise;
					}
					else
					{
						// Try to get the build log
						if( BlockingBuildLogID == 0 )
						{
							BlockingBuildLogID = DB.GetInt( "Commands", BlockingBuildID, "BuildLogID" );
						}

						// Try and get when the build started
						if( BlockingBuildLogID != 0 )
						{
							BlockingBuildStartTime = DB.GetDateTime( "BuildLog", BlockingBuildLogID, "BuildStarted" );
						}

						// Check to see if the build timed out (default time is when wait was started)
						if( DateTime.UtcNow - BlockingBuildStartTime > Builder.GetTimeout() )
						{
							Builder.SetErrorLevel( COMMANDS.WaitTimedOut );
							Mode = MODES.Finalise;
						}
					}
				}
				else
				{
					Mode = MODES.Finalise;
				}
				break;

			case MODES.WaitForFTP:
				// Check every 5 seconds
				TimeSpan WaitForFTPTime = new TimeSpan( 0, 0, 5 );
				if( DateTime.UtcNow - LastCheckForJobTime > WaitForFTPTime )
				{
					if( Builder.ManageFTPThread == null )
					{
						Builder.CloseLog();
						Mode = MODES.Finalise;
					}
					else if( !Builder.ManageFTPThread.IsAlive )
					{
						Builder.ManageFTPThread = null;
						Builder.CloseLog();
						Mode = MODES.Finalise;
					}
					// Check to see if waiting for the build timed out
					else if( CurrentCommand.IsTimedOut() )
					{
						Builder.SetErrorLevel( COMMANDS.WaitTimedOut );
						Builder.ManageFTPThread.Abort();
						Builder.ManageFTPThread = null;
						Builder.CommandLine = "FTP send from " + Builder.CommandLine;
						Builder.CloseLog();
						Mode = MODES.Finalise;
					}

					LastCheckForJobTime = DateTime.UtcNow;
				}
				break;

			case MODES.WaitForActive:
				if( BlockingBuildID != 0 )
				{
					// Check every 5 seconds
					TimeSpan WaitForActiveTime = new TimeSpan( 0, 0, 5 );
					if( DateTime.UtcNow - LastCheckForJobTime > WaitForActiveTime )
					{
						bool bDoneWaiting = false;

						// If there is a build log ID, then the build is running, so we're done.
						// Otherwise, see if the build has already started and finished instead.
						if( ( DB.GetInt( "Commands", BlockingBuildID, "BuildLogID" ) != 0 ) ||
							( DB.GetDateTime( "Commands", BlockingBuildID, "LastAttemptedDateTime" ) >= Builder.BuildStartedAt ) )
						{
							bDoneWaiting = true;
						}
						// Check to see if waiting for the build timed out (default time is when wait was started)
						else if( DateTime.UtcNow - BlockingBuildStartTime > Builder.GetTimeout() )
						{
							Builder.SetErrorLevel( COMMANDS.WaitTimedOut );
							bDoneWaiting = true;
						}

						if( bDoneWaiting )
						{
							// Reset the build log ID and start time
							BlockingBuildStartTime = DateTime.MinValue;
							Mode = MODES.Finalise;
						}

						LastCheckForJobTime = DateTime.UtcNow;
					}
				}
				else
				{
					Mode = MODES.Finalise;
				}
				break;

			case MODES.WaitForJobs:
				// Check every 5 seconds
				TimeSpan WaitForJobsTime = new TimeSpan( 0, 0, 5 );
				if( DateTime.UtcNow - LastCheckForJobTime > WaitForJobsTime )
				{
					string WaitQuery = "SELECT COUNT( ID ) FROM Jobs WHERE ( SpawnTime = " + JobSpawnTime.ToString() + " AND Complete = 1 )";
					int Count = DB.ReadInt( WaitQuery );

					if( NumCompletedJobs != Count )
					{
						int RemainingJobs = NumJobsToWaitFor - Count;
						Log( "[STATUS] Waiting for " + RemainingJobs.ToString() + " package jobs.", Color.Magenta );
						NumCompletedJobs = Count;
					}

					// See if we're done waiting yet
					if( Count == NumJobsToWaitFor )
					{
						// Check the status of jobs that have completed
						WaitQuery = "SELECT COUNT( ID ) FROM Jobs WHERE ( SpawnTime = " + JobSpawnTime.ToString() + " AND Succeeded = 1 )";
						Count = DB.ReadInt( WaitQuery );
						if( Count != NumJobsToWaitFor )
						{
							Builder.SetErrorLevel( COMMANDS.FailedJobs );
						}

						// Find any jobs that had suppressed errors
						string SuppressedQuery = "SELECT COUNT( ID ) FROM Jobs WHERE ( SpawnTime = " + JobSpawnTime.ToString() + " AND Suppressed = 1 )";
						int Suppressed = DB.ReadInt( SuppressedQuery );

						if( Suppressed > 0 )
						{
							// Find suppressed error jobs and add to Builder.LabelInfo.FailedGames
							DB.FindSuppressedJobs( Builder, JobSpawnTime );
						}

						NumJobsToWaitFor = 0;
						JobSpawnTime = DateTime.UtcNow.Ticks;
						Mode = MODES.Finalise;
					}
					else if( DateTime.UtcNow - BlockingBuildStartTime > Builder.GetTimeout() )
					{
						// Check to see if the build timed out
						Builder.SetErrorLevel( COMMANDS.WaitJobsTimedOut );
						Mode = MODES.Finalise;
					}

					LastCheckForJobTime = DateTime.UtcNow;
				}
				break;

			case MODES.Finalise:
				// Analyze logs and restart or exit
				Mode = HandleError();
				break;

			case MODES.Exit:
				Cleanup();
				StillRunning = false;
				break;
			}
			return StillRunning;
		}

		/** 
		 * Main loop tick - returns true if a build is in progress
		 */
		public bool Run()
		{
			if( DB == null )
			{
				return ( false );
			}

			// Ping the server to say we're still alive every 30 seconds
			MaintainMachine();

			if( BuildLogID != 0 || JobID != 0 )
			{
				bool StillRunning = RunBuild();
				if( StillRunning )
				{
					int ID = PollForKillBuild();
					if( ID != 0 )
					{
						KillBuild( ID );
					}

					ID = PollForKillJob();
					if( ID != 0 )
					{
						KillJob( ID );
					}
				}
				return ( StillRunning );
			}
			else
			{
				// Check for restarts
				CheckSystemDown();

				if( Ticking )
				{
					// Poll the DB for commands

					// Poll for Primary Builds
					int ID = PollForBuild( true );
					if( ID > 0 )
					{
						SpawnBuild( ID );
						return ( true );
					}

					// Pool for Primary Jobs
					ID = PollForJob( true );
					if( ID > 0 )
					{
						SpawnJob( ID );
						return ( true );
					}

					// Poll for Non-Primary Builds
					ID = PollForBuild( false );
					if( ID > 0 )
					{
						SpawnBuild( ID );
						return ( true );
					}

					// Pool for Non-Primary Jobs
					ID = PollForJob( false );
					if( ID > 0 )
					{
						SpawnJob( ID );
						return ( true );
					}
				}
			}

			return ( false );
		}

		private void Main_FormClosed( object sender, FormClosedEventArgs e )
		{
			DB.SetString( "Commands", CommandID, "Killer", "LocalUser" );

			KillBuild( CommandID );
			Ticking = false;
		}
	}
}