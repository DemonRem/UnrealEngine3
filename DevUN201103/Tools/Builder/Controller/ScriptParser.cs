/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Threading;
using P4API;
using Ionic.Zip;
using UnrealControls;

namespace Controller
{
    public class ScriptParser
    {
        public enum COLLATION
        {
			Engine,
			Audio,
			Editor,
			GFx,
			Game	= 10,
            Win32	= 100,
			Win64,
			Xbox360,
            PS3,
			Mobile,
			Android,
			iPhone,
            Loc,
			Online,
			GameSpy,
			G4WLive,
			Steam,
			PiB,
			Build,
            UnrealProp,
			Swarm,
			Lightmass,
            Install,
            Tools,
			Unknown,
			Count,
        }

        public class ChangeList
        {
            public int Number;
            public string User;
            public int Time;
            public string Description;
            public COLLATION Collate;
            public List<string> Files = new List<string>();

            public ChangeList()
            {
                Number = 0;
                Time = 0;
                Collate = COLLATION.Count;
            }

            public void CleanDescription()
            {
				string WorkDescription = Description.Replace( "\r", "" ).Trim();
                Description = "";

				// Process lines in their entirety
				string[] Lines = WorkDescription.Split( "\n".ToCharArray() );
				foreach( string Line in Lines )
				{
					// Skip any code review lines
					if( Line.ToLower().StartsWith( "#codereview" ) )
					{
						continue;
					}

					// Parse each word on the line
	                string[] Parms = Line.Trim().Split( " \t".ToCharArray() );
					string NewLine = "";
					for( int i = 0; i < Parms.Length; i++ )
					{
						string Token = Parms[i];

						if( Token.Length < 1 )
						{
							continue;
						}

						// Skip any tags
						if( Token.StartsWith( "#" ) )
						{
							if( Token.Length > 1 )
							{
								if( Token[1] < '0' || Token[1] > '9' )
								{
									continue;
								}
							}
						}

						NewLine += Token + " ";
					}

					if( NewLine.Length > 0 )
					{
						Description += "\t" + NewLine + Environment.NewLine;
					}
                }
            }
        }

        public class CollationType
        {
            public COLLATION Collation;
			public string Title;
            public bool Active;

            public CollationType( COLLATION InCollation, string InTitle )
            {
				Collation = InCollation;
				Title = InTitle;
                Active = false;
            }

            static public COLLATION GetReportType( string CollType, List<string> ValidGames )
            {
				// Is it a member of the predefined enum list?
                if( Enum.IsDefined( typeof( COLLATION ), CollType ) )
                {
                    COLLATION Report = ( COLLATION )Enum.Parse( typeof( COLLATION ), CollType, true );
                    return ( Report );
                }

				// Is it a dynamically generated enum?
				int GameCollation = ( int )COLLATION.Game;
				foreach( string ValidGame in ValidGames )
				{
					string GameName = ValidGame.ToLower();
					string GameNameMaps = GameName + "maps";
					string GameNameContent = GameName + "content";

					if( CollType.ToLower() == GameName )
					{
						return ( ( COLLATION )GameCollation );
					}
					else if( CollType.ToLower() == GameNameMaps )
					{
						return ( ( COLLATION )( GameCollation + 1 ) );
					}
					else if( CollType.ToLower() == GameNameContent )
					{
						return ( ( COLLATION )( GameCollation + 2 ) );
					}

					GameCollation += 3;
				}

                return ( COLLATION.Unknown );
            }
        }

        private string LogFileRootName = "";

        // Location of DevEnv
        public string MSVC8Application { get; set; }

		// Location of DevEnv
		public string MSVC9Application { get; set; }

		// Location of signing tool - C:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin
        public string SignToolName { get; set; }

		// Location of cat tool - C:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin
		public string CatToolName { get; set; }

		// Location of cat tool - GFWLSDK_DIR\tools\ZeroDayPiracyProtection\ZdpSdkTool.exe
		public string ZDPPToolName { get; set; }

        // Location of source server perl script
        public string SourceServerCmd { get; set; }

        // Location of symstore
        public string SymbolStoreApp { get; set; }

        // Location of XDK tools
        public string BlastTool { get; set; }

        // The location of the symbol store database
        public string SymbolStoreLocation { get; set; }

		// The location of the pfx file used to sign installers
		public string KeyLocation { get; set; }

		// The password for the above pfx
		public string KeyPassword { get; set; }

        // Location to save error string (if any)
        public string ErrorSaveVariable { get; set; }

        // Location of make for PS3
        public string MakeApplication { get; set; }

		// Location of the Steam Content Tool
		public string SteamContentToolLocation { get; set; }

		// Location of the Steam Content Server
		public string SteamContentServerLocation { get; set; }

        // The master file that contains the version to use
        public string EngineVersionFile { get; set; }

        // The misc version files that the engine version is propagated to
        public string MiscVersionFiles { get; set; }

		// The console specific version files that the engine version is propagated to
		public string ConsoleVersionFiles { get; set; }

		// The mobile specific version files that the engine version is propagated to
		public string MobileVersionFiles { get; set; }

		// The current job to copy down from server
        public string UBTEnvVar { get; set; }

		public enum ErrorModeType
		{
			CheckErrors,
			IgnoreErrors,
			SuppressErrors,
		}

		// Whether to check for errors in the build log
		public ErrorModeType ErrorMode { get; set; }

		// Whether a suppressed error has been seen in this task
		public bool HasSuppressedErrors { get; set; }

		// Whether to allow warnings when compiling script code
		public bool AllowSloppyScript { get; set; }

		// Whether to cc EngineQA
		public bool OfficialBuild { get; set; }

		// Whether to include the disambiguating timestamp folder when acquiring an installable build
		public bool IncludeTimestampFolder { get; set; }

		// Do we allow XGE for non primary builds
		public bool AllowXGE { get; set; }

		// Do we allow PCH for compilation?
		public bool AllowPCH { get; set; }

		// Do we want to specify the output on the commandline?
		public bool SpecifyOutput { get; set; }

		// Is this build a primary build or a verification build?
		public bool IsPrimaryBuild { get; set; }

        // The most recent changelist that was attempted for this build
		public int LastAttemptedBuild { get; set; }

        // The most recent changelist that was successful for this build
        public int LastGoodBuild { get; set; }

		// The most recent changelist that was a failure for this build
		public int LastFailedBuild { get; set; }

		// The branch that we are compiling from e.g. UnrealEngine3-Delta
        public Main.BranchDefinition BranchDef { get; set; }

        // Cached string containing the build description
        public string BuildDescription { get; set; }

        // Cached string containing the name of the user who triggered the build
        public string Operator { get; set; }

        // Cached string containing the name of the user who killed the build
        public string Killer { get; set; }

        // The most recent label that was synced to
        public string SyncedLabel { get; set; }

		// The head changelist when syncing starts
		public int HeadChangelist { get; set; }

        // Time the local build started
        public DateTime BuildStartedAt { get; set; }

        // Time the local build started waiting for network bandwidth
        public DateTime StartWaitForConch { get; set; }

        // The last time the local build sent a status update
        public DateTime LastWaitForConchUpdate { get; set; }

        // Whether a label has already been created to host the files
        public bool NewLabelCreated { get; set; }

		// Use this name instead of the game name in the publish folder
		public string GameNameOverride { get; set; }

		// The sub branch to use in addition to the branch the game runs out of
		public string SubBranchName { get; set; }

		// Whether to include the platform in the publish folder
		public bool IncludePlatform { get; set; }

        // Whether to block other publish operations to throttle the network
        public bool BlockOnPublish { get; set; }

        // Whether to ignore timestamps when publishing
        public bool ForceCopy { get; set; }

		// Whether to use the 64 bit or 32 bit binaries
		public bool Use64BitBinaries { get; set; }

		// Whether to strip source content when wrangling
		public bool StripSourceContent { get; set; }

		public enum PublishModeType
		{
			Files,
			Zip,
			Iso,
			Xsf
		}

		// Whether to publish raw files, add them to a zip, or add to an iso
		public PublishModeType PublishMode { get; set; }

		public enum PublishVerificationType
		{
			None,
			MD5
		}

		// The type of verification used when publishing builds
		public PublishVerificationType PublishVerification { get; set; }

		// Which audio assets to keep when wrangling
		public string KeepAssets { get; set; }

		// Whether to allow engine packages to be submitted after a several fixup redirects
		public bool bCommandletsSuccessful { get; set; }

		// The type of installer UnSetup will build
		public string UnSetupType { get; set; }

		// The database connection info
		public string DataBaseName { get; set; }
		public string DataBaseCatalog { get; set; }

        // Whether to enable unity stress testing
        public bool UnityStressTest { get; set; }

		// Whether to enable unity source file concatenation
		public bool UnityDisable { get; set; }

		// The name of the sku (currently used for disambiguating layout filenames)
		public string SkuName { get; set; }

		// The root location where the cooked files are copied to
        public string PublishFolder { get; set; }

        // The email addresses to send to when a build is triggered
        public string TriggerAddress { get; set; }

        // The email addresses to send to when a build fails
        public string FailAddress { get; set; }

        // The email addresses to send to when a build is successful
        public string SuccessAddress { get; set; }

		// A mailing list to cc all the emails to
		public string CarbonCopyAddress { get; set; }

        // The label or build that this build syncs to
        public string Dependency { get; set; }

        // The other build needed from cross platform conforming
		public string SourceBuild { get; set; }
        
        // A C++ define to be passed to the compiler
        public string BuildDefine { get; set; }

        // Destination folder of simple copy operations
        public string CopyDestination { get; set; }

		// The mode for saving a DVD image (e.g. ISO or XSF)
		public string ImageMode { get; set; }

        // The current command line for the builder command
        public string CommandLine { get; set; }

        // The current mod name we are cooking for
        public string ModName { get; set; }

        // The current command line for the builder command
        public string LogFileName { get; set; }

        // If there is a 'submit' at some point in the build script
        public bool IsBuilding { get; set; }

		// If we want to tag files to the label when submitting
		public bool IsTagging { get; set; }

        // If there is a 'tag' at some point in the build script
        public bool IsPromoting { get; set; }

        // If there is a 'publish' at some point in the build script
        public bool IsPublishing { get; set; }

        // If there is a 'UserChanges' at some point in the build script
        public bool IsSendingQAChanges { get; set; }

		// FTP settings
		public string FTPServer { get; set; }
		public string FTPUserName { get; set; }
		public string FTPPassword { get; set; }

		// The thread running the FTP upload
		public Thread ManageFTPThread = null;

		// Current zip class we are adding files to
		public ZipFile CurrentZip = null;

        // Creates a unique log file name for language independent operations
        public string GetLogFileName( COMMANDS Command )
        {
            LogFileName = LogFileRootName + "_" + LineCount.ToString() + "_" + Command.ToString() + ".txt";
            return ( LogFileName );
        }

        // Creates a unique log file name for language dependent operations
        public string GetLogFileName( COMMANDS Command, string Lang )
        {
            LogFileName = LogFileRootName + "_" + LineCount.ToString() + "_" + Command.ToString() + "_" + Lang + ".txt";
            return ( LogFileName );
        }

        public List<ChangeList> ChangeLists = new List<ChangeList>();
		private CollationType[] CollationTypes = new CollationType[( int )COLLATION.Count];
		private List<COLLATION> AllReports = new List<COLLATION>();
		private Queue<COLLATION> Reports = new Queue<COLLATION>();
		private List<string> ValidGames = new List<string>();

        private Main Parent = null;
        private StreamReader Script = null;
        public int LineCount = 0;
        public LabelInfo LabelInfo;

        // Symbol store
        private Queue<string> SymStoreCommands = new Queue<string>();

        // Normally set once at the beginning of the file

        // List of language variants that have full audio loc
        private Queue<string> Languages = new Queue<string>();
        // Subset of languages that have a valid loc'd variant
        private Queue<string> ValidLanguages;
		// List of language variants that have text only loc
		private Queue<string> TextLanguages = new Queue<string>();
		private TimeSpan OperationTimeout = new TimeSpan( 0, 10, 0 );
        private TimeSpan RespondingTimeout = new TimeSpan( 0, 10, 0 );

        // Changed multiple times during script execution
        private List<string> PublishDestinations = new List<string>();

		// Additional messages to report with the final success email
		private List<string> StatusReport = new List<string>();

        public string BuildConfiguration = "Release";
		public string ScriptConfiguration = "";
		public string ScriptConfigurationDLC = "";
		private string CookConfiguration = "";
		private string CommandletConfiguration = "";
		private string AnalyzeReferencedContentConfiguration = "";
		private string InstallConfiguration = "";
        private string ContentPath = "";
        private string CompressionConfiguration = "size";

		// The current Perforce server
		public Main.PerforceServerInfo ServerInfo = new Main.PerforceServerInfo();

        // Working variables
        private COMMANDS CommandID = COMMANDS.Config;
		private COMMANDS ErrorLevel = COMMANDS.None;
		public bool ChangelistsCollated = false;

		// Compile-specific values
		public bool bXGEHasFailed = false;

        // The log file that gets referenced
        private TextWriter Log = null;

		public ScriptParser( Main InParent, string ScriptFileName, string Description, Main.PerforceServerInfo InServerInfo, Main.BranchDefinition InBranchDef, bool PrimaryBuild, int LastAttemptedChangeList, int LastGoodChangeList, int LastFailedChangeList, DateTime BuildStarted, string InOperator )
        {
            Parent = InParent;
            try
            {
				// Static initialisers
				MSVC8Application = Environment.GetEnvironmentVariable( "VS80COMNTOOLS" ) + "../IDE/Devenv.com";
				MSVC9Application = Environment.GetEnvironmentVariable( "VS90COMNTOOLS" ) + "../IDE/Devenv.com";
				SignToolName = "C:/Program Files/Microsoft SDKs/Windows/v6.0A/Bin/SignTool.exe";
				CatToolName = "C:/Program Files/Microsoft SDKs/Windows/v6.0A/Bin/MakeCat.exe";
				ZDPPToolName = Environment.GetEnvironmentVariable( "GFWLSDK_DIR" ) + "tools/ZeroDayPiracyProtection/ZdpSdkTool.exe";
				SourceServerCmd = "C:/Program Files (x86)/Debugging Tools for Windows (x86)/srcsrv/p4index.cmd";
				SymbolStoreApp = "C:/Program Files (x86)/Debugging Tools for Windows (x86)/symstore.exe";
				BlastTool = Environment.GetEnvironmentVariable( "XEDK" ) + "\\bin\\win32\\blast.exe";
				SymbolStoreLocation = Properties.Settings.Default.SymbolStoreLocation;
				KeyLocation = "";
				KeyPassword = "";
		        ErrorSaveVariable = "";
				MakeApplication = "IBMake.exe";
				SteamContentToolLocation = "Development/External/Steamworks/sdk/tools/ContentTool.com";
				SteamContentServerLocation = "Development/External/Steamworks/sdk/tools/contentserver/contentserver.exe";
				EngineVersionFile = "Development/Src/Core/Src/UnObjVer.cpp";
				MiscVersionFiles = "Development/Src/Launch/Resources/Version.h;" +
								   "Development/Src/Launch/Resources/PCLaunch.rc;" +
								   "Binaries/PIB/NP/XPIFiles/install.rdf;" +
								   "Binaries/PIB/AX/ATLUE3.inf;" +
								   "Binaries/PIB/GC/manifest.json;" +
								   "Binaries/build.properties";
				ConsoleVersionFiles = "Development/Src/ExampleGame/Live/xex.xml;" +
									  "Development/Src/GearGame/Live/xex.xml;" +
									  "Development/Src/NanoGame/Live/xex.xml;" +
									  "Development/Src/UTGame/Live/xex.xml;";
				MobileVersionFiles = "Development/Src/IPhone/Resources/IPhone-Info.plist;" +
									 "MobileGame/Build/IPhone/IPhone-Info.plist";
				UBTEnvVar = "";

				CommandLine = "";

				ErrorMode = ErrorModeType.CheckErrors;
				HasSuppressedErrors = false;
				AllowSloppyScript = false;
				OfficialBuild = true;
				IncludeTimestampFolder = true;
				AllowXGE = true;
				AllowPCH = true;
				SpecifyOutput = true;
				LastAttemptedBuild = 0;
				LastGoodBuild = 0;
				LastFailedBuild = 0;

				Killer = "";
				SyncedLabel = "";
				HeadChangelist = -1;

				LastWaitForConchUpdate = DateTime.UtcNow;
				NewLabelCreated = false;
				GameNameOverride = "";
				SubBranchName = "";
				IncludePlatform = true;
				BlockOnPublish = false;
				ForceCopy = false;
				Use64BitBinaries = true;
				StripSourceContent = false;
				PublishMode = PublishModeType.Files;
				PublishVerification = PublishVerificationType.MD5;
				
				KeepAssets = "";
				bCommandletsSuccessful = true;
				UnSetupType = "";
				
				UnSetupType = "UDK";
				UnityStressTest = false;
				UnityDisable = false;

				SkuName = "";
				PublishFolder = "";
				TriggerAddress = "";
				SuccessAddress = "";
				FailAddress = "";
				CarbonCopyAddress = "";
				Dependency = "";
				SourceBuild = "";
				BuildDefine = "";
				CopyDestination = "";
				ImageMode = "ISO";
				CommandLine = "";
				ModName = "";
				LogFileName = "";
				
				IsBuilding = false;
				IsTagging = false;
				IsPromoting = false;
				IsPublishing = false;
				IsSendingQAChanges = false;

				FTPServer = "";
				FTPUserName = "";
				FTPPassword = "";

				// Dynamic initialisation
				ServerInfo = InServerInfo;
				BranchDef = InBranchDef;
				LastAttemptedBuild = LastAttemptedChangeList;
				LastGoodBuild = LastGoodChangeList;
				LastFailedBuild = LastFailedChangeList;
				BuildStartedAt = BuildStarted;
                BuildDescription = Description;
                Operator = InOperator;
                StartWaitForConch = BuildStarted;
				IsPrimaryBuild = PrimaryBuild;

                // Always need some label
                LabelInfo = new LabelInfo( Parent, this );
                LabelInfo.BuildType = Description;

                string ScriptFile = "Development/Builder/Scripts/" + ScriptFileName + ".build";
                string NewScriptFile = "Development/Builder/Scripts/Current.build";

                FileInfo ScriptInfo = new FileInfo( NewScriptFile );
                if( ScriptInfo.Exists )
                {
                    ScriptInfo.IsReadOnly = false;
                    ScriptInfo.Delete();
                }

                FileInfo Info = new FileInfo( ScriptFile );
                Info.CopyTo( NewScriptFile, true );

                Script = new StreamReader( NewScriptFile );
                Parent.Log( "Using build script '" + BranchDef.Branch + "/" + ScriptFile + "'", Color.Magenta );

                LogFileRootName = "Development/Builder/Logs/Builder_[" + GetTimeStamp() + "]";

                // Make sure there are no defines hanging around
                Environment.SetEnvironmentVariable( "CL", "" );

				InitCollationTypes();
            }
            catch
            {
                Parent.Log( "Error, problem loading build script", Color.Red );
            }
        }

        public void Destroy()
        {
            Parent.Log( LineCount.ToString() + " lines of build script parsed", Color.Magenta );
            if( Script != null )
            {
                Script.Close();
            }
        }

		public void OpenLog( string LogFileName, bool Append )
        {
			Log = TextWriter.Synchronized( new StreamWriter( LogFileName, Append, Encoding.Unicode ) );
		}

        public void CloseLog()
        {
            if( Log != null )
            {
                Log.Close();
				Log = null;
            }
        }

		public TextWriter OpenSystemLog()
		{
			return TextWriter.Synchronized( new StreamWriter( LogFileRootName + "_SystemLog.txt", false, Encoding.Unicode ) );
		}
		
		public void Write( string Output )
        {
            if( Log != null && Output != null )
            {
                try
                {
					Log.Write( DateTime.Now.ToString( "HH:mm:ss" ) + ": " + Output + Environment.NewLine );
                }
                catch( Exception Ex )
                {
                    Parent.Log( "Log write exception: exception '" + Ex.Message + "'", Color.Red );
                    Parent.Log( "Log write exception: intended to write '" + Output + "'", Color.Red );
                }
            }
        }

		public string[] SafeSplit( string Line )
		{
			string[] Parms = Line.Split( " \t".ToCharArray() );
			List<string> SafeParms = new List<string>();

			foreach( string Parm in Parms )
			{
				if( Parm.Length > 0 )
				{
					SafeParms.Add( Parm );
				}
			}

			return ( SafeParms.ToArray() );
		}

		public string[] SplitCommandline()
		{
			return ( SafeSplit( CommandLine ) );
		}

        public Queue<string> GetLanguages()
        {
            return ( Languages );
        }

		public Queue<string> GetTextLanguages()
		{
			return ( TextLanguages );
		}

        public void SetValidLanguages( Queue<string> ValidLangs )
        {
            ValidLanguages = ValidLangs;
        }

        public Queue<string> GetValidLanguages()
        {
            return ( ValidLanguages );
        }

		public string GetFolderName()
		{
			return ( LabelInfo.GetFolderName( GameNameOverride, IncludePlatform ) );
		}

		public string GetSubBranch()
		{
			string SubBranch = "";
			if( SubBranchName.Length > 0 )
			{
				SubBranch = "\\" + SubBranchName;
			}

			return ( SubBranch );
		}

		public string GetImageName()
		{
			string FolderName = LabelInfo.GetFolderName( GameNameOverride, IncludePlatform );
			if( SkuName.Length > 0 )
			{
				FolderName += "_" + SkuName;
			}
			return ( FolderName );
		}

        public string GetScriptConfiguration()
        {
			string ScriptConfig = "";
			if( ScriptConfiguration.Length > 0 )
            {
                string[] Configs = SafeSplit( ScriptConfiguration );
                foreach( string Config in Configs )
                {
                    if( Config.ToLower() == "fr" )
                    {
                        ScriptConfig += " -final_release";
                    }
                    else
                    {
                        ScriptConfig += " -" + Config;
                    }
                }
            }
			if( ScriptConfigurationDLC.Length > 0 )
			{
				ScriptConfig += " -dlc " + ScriptConfigurationDLC;
			}
			return ScriptConfig;
        }

        public string GetCookConfiguration()
        {
            if( CookConfiguration.Length > 0 )
            {
                return ( " -" + CookConfiguration );
            }
            return ( "" );
        }

		public string GetOptionalCommandletConfig()
		{
			if( bCommandletsSuccessful && CommandletConfiguration.Length > 0 )
			{
				return ( " -" + CommandletConfiguration );
			}

			return ( "" );
		}

		public string GetDataBaseConnectionInfo()
		{
			string ConnectionInfo = "";
			if( DataBaseName.Length > 0 && DataBaseCatalog.Length > 0 )
			{
				ConnectionInfo = " -DATABASE=" + DataBaseName + " -CATALOG=" + DataBaseCatalog;
			}

			return ( ConnectionInfo );
		}

		public string GetAnalyzeReferencedContentConfiguration()
		{
			if( AnalyzeReferencedContentConfiguration.Length > 0 )
			{
				return ( " " + AnalyzeReferencedContentConfiguration );
			}
			return ( "" );
		}

        public string GetInstallConfiguration()
        {
            if( InstallConfiguration.ToLower() == "target" )
            {
                return ( " -t" );
            }
            if( InstallConfiguration.ToLower() == "demo" )
            {
                return ( " -d" );
            } 
            return ( "" );
        }

        public string GetContentPath()
        {
            if( ContentPath.Length > 0 )
            {
                return ( " -paths=" + ContentPath );
            }
            return ( "" );
        }

        public string GetModName()
        {
            if( ModName.Length > 0 )
            {
                return ( " -usermodname=" + ModName + " -user" );
            }
            return ( "" );
        }

        public string GetCompressionConfiguration()
        {
            if( CompressionConfiguration.ToLower() == "speed" )
            {
                return ( " -NOPACKAGECOMPRESSION" );
            }
            return ( "" );
        }

        public void ClearPublishDestinations()
        {
            PublishDestinations.Clear();
        }

        public void AddPublishDestination( string Dest )
        {
            PublishDestinations.Add( Dest );
        }

        public List<string> GetPublishDestinations()
        {
            return ( PublishDestinations );
        }

		public void AddToSuccessStatus( string AdditionalStatus )
		{
			StatusReport.Add( AdditionalStatus );
		}

		public List<string> GetStatusReport()
		{
			return( StatusReport );
		}

        public void AddUpdateSymStore( string Command )
        {
            SymStoreCommands.Enqueue( Command );
        }

        public string PopUpdateSymStore()
        {
            if( !UpdateSymStoreEmpty() )
            {
                return ( SymStoreCommands.Dequeue() );
            }

            return ( "" );
        }

        public bool UpdateSymStoreEmpty()
        {
            return ( SymStoreCommands.Count == 0 );
        }

		public void SetErrorLevel( COMMANDS Error )
        {
            ErrorLevel = Error;
        }

		public COMMANDS GetErrorLevel()
        {
            return ( ErrorLevel );
        }

        public string GetTimeStamp()
        {
			DateTime LocalTime = BuildStartedAt.ToLocalTime();

			string TimeStamp = LocalTime.Year + "-"
						+ LocalTime.Month.ToString( "00" ) + "-"
						+ LocalTime.Day.ToString( "00" ) + "_"
						+ LocalTime.Hour.ToString( "00" ) + "."
						+ LocalTime.Minute.ToString( "00" ) + "."
						+ LocalTime.Second.ToString( "00" );
			return ( TimeStamp );
        }

        public void SetCommandID( COMMANDS ID )
        {
            CommandID = ID;
        }

        public COMMANDS GetCommandID()
        {
            return ( CommandID );
        }

        public int SafeStringToInt( string Number )
        {
            int Result = 0;

            try
            {
                Number = Number.Trim();
                Result = Int32.Parse( Number );
            }
            catch
            {
            }

            return ( Result );
        }

		public long SafeStringToLong( string Number )
		{
			long Result = 0;

			try
			{
				Number = Number.Trim();
				Result = long.Parse( Number );
			}
			catch
			{
			}

			return ( Result );
		}

        public GameConfig AddCheckedOutGame()
        {
			GameConfig Config = new GameConfig( CommandLine, LabelInfo.Platform, BuildConfiguration, LabelInfo.Defines, true, Use64BitBinaries );
            LabelInfo.Games.Add( Config );

            return ( Config );
        }

        public GameConfig CreateGameConfig( string Game, string InPlatform )
        {
			GameConfig Config = new GameConfig( Game, InPlatform, BuildConfiguration, LabelInfo.Defines, true, Use64BitBinaries );

            return ( Config );
        }

        public GameConfig CreateGameConfig( string Game )
        {
			GameConfig Config = new GameConfig( Game, LabelInfo.Platform, BuildConfiguration, LabelInfo.Defines, true, Use64BitBinaries );

            return ( Config );
        }

        public GameConfig CreateGameConfig()
        {
			GameConfig Config = new GameConfig( LabelInfo.Game, LabelInfo.Platform, BuildConfiguration, LabelInfo.Defines, true, Use64BitBinaries );

            return ( Config );
        }

        public GameConfig CreateGameConfig( bool Local )
        {
			GameConfig Config = new GameConfig( LabelInfo.Game, LabelInfo.Platform, BuildConfiguration, LabelInfo.Defines, Local, Use64BitBinaries );

            return ( Config );
        }

        public TimeSpan GetTimeout()
        {
            return ( OperationTimeout );
        }

        public TimeSpan GetRespondingTimeout()
        {
            return ( RespondingTimeout );
        }

        // Set the CL env var based on required defines
        public void HandleMSVCDefines()
        {
			// Clear out env var
            Environment.SetEnvironmentVariable( "CL", "" );

			// Set up new one if necessary
			string EnvVar = "";
			if( LabelInfo.Defines.Count > 0 )
			{
				foreach( string Define in LabelInfo.Defines )
				{
					EnvVar += "/D " + Define + " ";
				}

				Environment.SetEnvironmentVariable( "CL", EnvVar.Trim() );
			}

            Write( "Setting CL=" + EnvVar );
        }

        // Passed in on the command line, nothing special to do here
        public string HandleGCCDefines()
        {
			// Clear out env var
			Environment.SetEnvironmentVariable( "CL", "" );

			// Set up new parameter
			string CommandLine = "";
			if( LabelInfo.Defines.Count > 0 )
			{
				CommandLine = "ADD_DEFINES=\"";
				foreach( string Define in LabelInfo.Defines )
				{
					CommandLine += Define + " ";
				}
				CommandLine = CommandLine.Trim() + "\"";
			}

            return ( CommandLine );
        }

        // Passed in on the command line, nothing special to do here
        public string HandleUBTDefines()
        {
			// Clear out env var
			Environment.SetEnvironmentVariable( "CL", "" );

			// Set up new parameter
			string CommandLine = "";
			if( LabelInfo.Defines.Count > 0 )
			{
				foreach( string Define in LabelInfo.Defines )
				{
					CommandLine += "-define " + Define + " ";
				}
				CommandLine = " " + CommandLine.Trim();
			}

            return ( CommandLine );
        }

        // Passed in on the command line, nothing special to do here
        public string HandleUBTEnvVar()
        {
            string CommandLine = "";
            if( UBTEnvVar.Length > 0 )
            {
                CommandLine += " -EnvVar " + UBTEnvVar;
            }

            return ( CommandLine );
        }

        public COMMANDS ParseNextLine()
        {
            string Line;

            if( Script == null )
            {
				ErrorLevel = COMMANDS.NoScript;
                CommandID = COMMANDS.Error;
                return( CommandID );
            }

            Line = Script.ReadLine();
            if( Line == null )
            {
                Parent.Log( "[STATUS] Script parsing completed", Color.Magenta );
                CommandID = COMMANDS.Finished;
                return( CommandID );
            }

            LineCount++;
            CommandID = COMMANDS.Config;
            CommandLine = "";
            string[] Parms = SafeSplit( Line );
            if( Parms.Length > 0 )
            {
                string Command = Parms[0].ToLower();
                CommandLine = Line.Substring( Parms[0].Length ).Trim();
                CommandLine = CommandLine.Replace( '\\', '/' );
                // Need to keep the double backslashes for network locations
                CommandLine = CommandLine.Replace( "//", "\\\\" );

                if( Command.Length == 0 || Command == "//" )
                {
                    // Comment - ignore
                    CommandID = COMMANDS.Config;
                }
                else if( Command == "status" )
                {
                    // Status string - just echo
                    Parent.Log( "[STATUS] " + Parent.ExpandString( CommandLine, BranchDef.Branch ), Color.Magenta );
                    CommandID = COMMANDS.Config;
                }
                else if( Command == "watchstart" )
                {
                    // Start a timer
					string PerfKeyName = Parent.ExpandString( CommandLine, BranchDef.Branch );
                    Parent.Log( "[WATCHSTART " + PerfKeyName + "]", Color.Magenta );
                    CommandID = COMMANDS.Config;
                }
                else if( Command == "watchstop" )
                {
                    // Stop a timer
                    Parent.Log( "[WATCHSTOP]", Color.Magenta );
                    CommandID = COMMANDS.Config;
                }
				else if( Command == "errormode" )
				{
					string ErrorModeString = Parent.ExpandString( CommandLine, BranchDef.Branch );
					try
					{
						ErrorMode = ( ErrorModeType )Enum.Parse( typeof( ErrorModeType ), ErrorModeString, true );
					}
					catch
					{
						ErrorMode = ErrorModeType.CheckErrors;
					}

					Parent.Log( "ErrorMode set to '" + ErrorMode.ToString() + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "allowsloppyscript" )
				{
					AllowSloppyScript = true;
					Parent.Log( "Script warnings will not fail the build", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "unofficial" )
				{
					OfficialBuild = false;
					Parent.Log( "EngineQA will not be cc'd on build failure", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ambiguousfoldername" )
				{
					IncludeTimestampFolder = false;
					Parent.Log( "The timestamped folder will not be used when getting an installable build", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ubtenablexge" )
				{
					AllowXGE = true;
					Parent.Log( "XGE is now ENABLED in UBT", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ubtdisablexge" )
				{
					AllowXGE = false;
					Parent.Log( "XGE is now DISABLED in UBT", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ubtenablepch" )
				{
					AllowPCH = true;
					Parent.Log( "PCHs are now ENABLED in UBT", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ubtdisablepch" )
				{
					AllowPCH = false;
					Parent.Log( "PCHs are now DISABLED in UBT", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ubtspecifyoutput" )
				{
					SpecifyOutput = true;
					Parent.Log( "The output is now SPECIFIED in UBT", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ubtnospecifyoutput" )
				{
					SpecifyOutput = false;
					Parent.Log( "The output is now NOT SPECIFIED in UBT", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "platformspecific" )
				{
					IncludePlatform = true;
					Parent.Log( "Publish folder includes the platform", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "platformagnostic" )
				{
					IncludePlatform = false;
					Parent.Log( "Publish folder does NOT include the platform", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "gamenameoverride" )
				{
					GameNameOverride = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Publish folder uses '" + GameNameOverride + "' rather than the game name", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "subbranch" )
				{
					SubBranchName = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Publish folder adds in '" + SubBranchName + "' to the branch name", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "languagespecific" )
                {
					Parent.Log( "[DEPRECATED] languagespecific", Color.Magenta );
                    CommandID = COMMANDS.Config;
                }
                else if( Command == "languageagnostic" )
                {
					Parent.Log( "[DEPRECATED] languageagnostic", Color.Magenta );
                    CommandID = COMMANDS.Config;
                }
                else if( Command == "touchlabel" )
                {
					// This is used to update the timestamp of the folder to publish to
                    LabelInfo.Touch();
                    Parent.Log( "The current label has been touched", Color.Magenta );
                    CommandID = COMMANDS.Config;
                }
                else if( Command == "updatelabel" )
                {
                    CommandID = COMMANDS.UpdateLabel;
                }
                else if( Command == "updatefolder" )
                {
                    CommandID = COMMANDS.UpdateFolder;
                }
				else if( Command == "clearpublishdestinations" )
				{
					Parent.Log( "Clearing publish destinations", Color.Magenta );
					ClearPublishDestinations();
					CommandID = COMMANDS.Config;
				}
				else if( Command == "saveerror" )
				{
					ErrorSaveVariable = Parent.ExpandString( CommandLine, BranchDef.Branch );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "keylocation" )
				{
					KeyLocation = Parent.ExpandString( CommandLine, BranchDef.Branch );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "keypassword" )
				{
					KeyPassword = Parent.ExpandString( CommandLine, BranchDef.Branch );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "tag" )
				{
					Parent.Log( "The latest build label will be copied to '" + CommandLine + "'", Color.Magenta );
					IsPromoting = true;
					CommandID = COMMANDS.SCC_Tag;
				}
				else if( Command == "report" )
				{
					Reports.Clear();

					string ExpandedCommandLine = Parent.ExpandString( CommandLine, BranchDef.Branch );
					string[] NewParms = SafeSplit( ExpandedCommandLine );

					for( int i = 0; i < NewParms.Length; i++ )
					{
						if( NewParms[i].Length > 0 )
						{
							COLLATION Collation = CollationType.GetReportType( NewParms[i], ValidGames );
							if( Collation != COLLATION.Unknown )
							{
								Reports.Enqueue( Collation );
							}
							else
							{
								Parent.Log( "Missing COLLATION type: " + NewParms[i], Color.Red );
							}
						}
					}
				}
				else if( Command == "triggeraddress" )
				{
					// Email addresses to use when the build is triggered
					TriggerAddress = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "TriggerAddress set to '" + TriggerAddress + "'", Color.Magenta );
					CommandID = COMMANDS.TriggerMail;
				}
				else if( Command == "failaddress" )
				{
					// Email addresses to use if the build fails
					FailAddress = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "FailAddress set to '" + FailAddress + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "successaddress" )
				{
					// Email addresses to use if the build succeeds
					SuccessAddress = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "SuccessAddress set to '" + SuccessAddress + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "carboncopyaddress" )
				{
					// Email addresses to use when the build is triggered
					CarbonCopyAddress = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "CarbonCopyAddress set to '" + CarbonCopyAddress + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "sendemail" )
				{
					// Send an email based on the current state of the build
					Parent.Log( "Sending update email", Color.Magenta );
					CommandID = COMMANDS.SendEmail;
				}
				else if( Command == "cisprocessp4changes" )
				{
					// Instruct the controller to process all unprocessed p4 changes for CIS tasks
					Parent.Log( "Processing P4 changes...", Color.Magenta );
					CommandID = COMMANDS.CISProcessP4Changes;
				}
				else if( Command == "cisupdatemonitorvalues" )
				{
					// Instruct the controller to evaluate all completed CIS jobs and post results
					// the CIS Monitor can use
					Parent.Log( "Processing completed CIS jobs...", Color.Magenta );
					CommandID = COMMANDS.CISUpdateMonitorValues;
				}
				else if( Command == "sku" )
				{
					SkuName = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Sku set to: " + SkuName, Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ftpserver" )
				{
					FTPServer = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "FTP server set to: " + FTPServer, Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ftpusername" )
				{
					FTPUserName = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "FTP username set to: " + FTPUserName, Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ftppassword" )
				{
					FTPPassword = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "FTP password set to: <NOT TELLING!>", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ftpsendfile" )
				{
					CommandID = COMMANDS.FTPSendFile;
				}
				else if( Command == "ftpsendimage" )
				{
					CommandID = COMMANDS.FTPSendImage;
				}
				else if( Command == "zipaddimage" )
				{
					CommandID = COMMANDS.ZipAddImage;
				}
				else if( Command == "zipaddfile" )
				{
					CommandID = COMMANDS.ZipAddFile;
				}
				else if( Command == "zipsave" )
				{
					CommandID = COMMANDS.ZipSave;
				}
				else if( Command == "raraddimage" )
				{
					CommandID = COMMANDS.RarAddImage;
				}
				else if( Command == "raraddfile" )
				{
					CommandID = COMMANDS.RarAddFile;
				}
				else if( Command == "cleanupsymbolserver" )
				{
					CommandID = COMMANDS.CleanupSymbolServer;
				}
				else if( Command == "define" )
				{
					// Define for the compiler to use
					LabelInfo.ClearDefines();

					BuildDefine = Parent.ExpandString( CommandLine, BranchDef.Branch );
					if( BuildDefine.Length > 0 )
					{
						string[] DefineParms = SafeSplit( BuildDefine );
						foreach( string Define in DefineParms )
						{
							LabelInfo.AddDefine( Define );
						}
					}
					Parent.Log( "Define set to '" + BuildDefine + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "ubtsetenv" )
				{
					UBTEnvVar = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "[DEPRECATED] Setting UBT environment variable '" + UBTEnvVar + "'", Color.Magenta );
				}
				else if( Command == "updatewbts" )
				{
					CommandID = COMMANDS.UpdateWBTS;
				}
				else if( Command == "language" )
				{
					// Language for the cooker to use
					LabelInfo.Language = Parent.ExpandString( CommandLine, BranchDef.Branch ).ToUpper();
					Parent.Log( "Language set to '" + LabelInfo.Language + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "languages" )
				{
					// List of languages used to conform or ML cook
					Languages.Clear();

					string LangLine = Parent.ExpandString( CommandLine, BranchDef.Branch ).ToUpper();
					string[] LangParms = SafeSplit( LangLine );
					foreach( string Lang in LangParms )
					{
						if( Lang.Length > 0 )
						{
							Languages.Enqueue( Lang );
						}
					}

					Parent.Log( "Number of languages to process: " + Languages.Count, Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "textlanguages" )
				{
					// List of languages that only have text loc
					TextLanguages.Clear();

					string LangLine = Parent.ExpandString( CommandLine, BranchDef.Branch ).ToUpper();
					string[] LangParms = SafeSplit( LangLine );
					foreach( string Lang in LangParms )
					{
						if( Lang.Length > 0 )
						{
							TextLanguages.Enqueue( Lang );
						}
					}

					Parent.Log( "Number of text languages to process: " + TextLanguages.Count, Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "clientspec" )
				{
					Parent.SendErrorMail( "Deprecated command 'ClientSpec'", "" );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "timeout" )
				{
					// Number of minutes to failure
					int OpTimeout = SafeStringToInt( Parent.ExpandString( CommandLine, BranchDef.Branch ) );
					OperationTimeout = new TimeSpan( 0, OpTimeout, 0 );
					Parent.Log( "Timeout set to " + OpTimeout.ToString() + " minutes", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "msvc8application" )
				{
					// MSVC command line app
					MSVC8Application = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "MSVC8 executable set to '" + MSVC8Application + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "msvc9application" )
				{
					// MSVC command line app
					MSVC9Application = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "MSVC9 executable set to '" + MSVC9Application + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "makeapplication" )
				{
					// make app
					MakeApplication = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Make executable set to '" + MakeApplication + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "symbolstoreapplication" )
				{
					// Sign tool location
					SymbolStoreApp = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Symbol store executable set to '" + SymbolStoreApp + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "signtoolapplication" )
				{
					// Sign tool location
					SignToolName = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Sign tool executable set to '" + SignToolName + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "sourceservercommand" )
				{
					// bat/perl script to index pdbs
					SourceServerCmd = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Source server script set to '" + SourceServerCmd + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "symbolstorelocation" )
				{
					// bat/perl script to index pdbs
					SymbolStoreLocation = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Symbol store location set to '" + SymbolStoreLocation + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "versionfile" )
				{
					// Main version file
					EngineVersionFile = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Engine version file set to '" + EngineVersionFile + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "miscversionfiles" )
				{
					// Version files that have a version propagated to
					MiscVersionFiles = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Misc version files set to '" + MiscVersionFiles + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "consoleversionfiles" )
				{
					// Version files that have a version propagated to
					ConsoleVersionFiles = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Console version files set to '" + ConsoleVersionFiles + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "mobileversionfiles" )
				{
					// Version files that have a version propagated to
					MobileVersionFiles = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Mobile version files set to '" + MobileVersionFiles + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "game" )
				{
					// Game we are interested in
					LabelInfo.Game = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Game set to '" + LabelInfo.Game + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "platform" )
				{
					// Platform we are interested in
					LabelInfo.Platform = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Platform set to '" + LabelInfo.Platform + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "buildconfig" )
				{
					// Build we are interested in
					BuildConfiguration = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Build configuration set to '" + BuildConfiguration + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "scriptconfig" )
				{
					// Script release or final_release we are interested in
					ScriptConfiguration = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Script configuration set to '" + ScriptConfiguration + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "scriptconfigdlc" )
				{
					// Script release or final_release we are interested in
					ScriptConfigurationDLC = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "DLC script configuration set to '" + ScriptConfigurationDLC + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "cookconfig" )
				{
					// Cook config we are interested in (eg. CookForServer - dedicated server trimming)
					CookConfiguration = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Cook configuration set to '" + CookConfiguration + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "modname" )
				{
					// The mod name we are cooking for
					ModName = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "ModName set to '" + ModName + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "installconfig" )
				{
					// Install we are interested in
					InstallConfiguration = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Install configuration set to '" + InstallConfiguration + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "contentpath" )
				{
					// Used after a wrangle
					ContentPath = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "ContentPath set to '" + ContentPath + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "compressionconfig" )
				{
					// Set to size or speed
					CompressionConfiguration = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Compression biased for '" + CompressionConfiguration + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "commandletconfig" )
				{
					// Set to size or speed
					CommandletConfiguration = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "CommandletConfiguration set to '" + CommandletConfiguration + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "database" )
				{
					string[] DataBaseConnection = SplitCommandline();
					if( DataBaseConnection.Length == 2 )
					{
						DataBaseName = DataBaseConnection[0];
						DataBaseCatalog = DataBaseConnection[1];
						Parent.Log( "Database connection set to '" + DataBaseName + "' and '" + DataBaseCatalog + "'", Color.Magenta );
					}
					else
					{
						Parent.Log( "Error, need both database and catalog in database definition", Color.Magenta );

						ErrorLevel = COMMANDS.IllegalCommand;
						CommandID = COMMANDS.Error;
					}
					CommandID = COMMANDS.Config;
				}
				else if( Command == "dependency" )
				{
					Dependency = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "Dependency set to '" + Dependency + "'", Color.DarkGreen );
					CommandID = COMMANDS.SetDependency;
				}
				else if( Command == "sourcebuild" )
				{
					SourceBuild = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "SourceBuild set to '" + SourceBuild + "'", Color.DarkGreen );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "copydest" )
				{
					CopyDestination = Parent.ExpandString( CommandLine, BranchDef.Branch );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "blockonpublish" )
				{
					BlockOnPublish = true;
					CommandID = COMMANDS.Config;
				}
				else if( Command == "forcecopy" )
				{
					ForceCopy = true;
					CommandID = COMMANDS.Config;
				}
				else if( Command == "use64bit" )
				{
					Use64BitBinaries = true;
					CommandID = COMMANDS.Config;
				}
				else if( Command == "use32bit" )
				{
					Use64BitBinaries = false;
					CommandID = COMMANDS.Config;
				}
				else if( Command == "stripsourcecontent" )
				{
					StripSourceContent = true;
					CommandID = COMMANDS.Config;
				}
				else if( Command == "publishmode" )
				{
					string PublishModeString = Parent.ExpandString( CommandLine, BranchDef.Branch );
					try
					{
						PublishMode = ( PublishModeType )Enum.Parse( typeof( PublishModeType ), PublishModeString, true );
					}
					catch
					{
						PublishMode = PublishModeType.Files;
					}

					Parent.Log( "Publishing mode set to '" + PublishMode.ToString() + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "publishverification" )
				{
					string PublishVerificationString = Parent.ExpandString( CommandLine, BranchDef.Branch );
					try
					{
						PublishVerification = ( PublishVerificationType )Enum.Parse( typeof( PublishVerificationType ), PublishVerificationString, true );
					}
					catch
					{
						PublishVerification = PublishVerificationType.MD5;
					}

					Parent.Log( "Publishing verification set to '" + PublishVerification.ToString() + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "keepassets" )
				{
					KeepAssets = Parent.ExpandString( CommandLine, BranchDef.Branch );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "imagemode" )
				{
					ImageMode = Parent.ExpandString( CommandLine, BranchDef.Branch );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "unity" )
				{
					string UnityParameter = Parent.ExpandString( CommandLine, BranchDef.Branch );
					if( UnityParameter == "disable" )
					{
						UnityDisable = true;
					}
					else if( UnityParameter == "stresstest" )
					{
						UnityStressTest = true;
					}
					Parent.Log( "Unity set to '" + UnityParameter + "'", Color.Magenta );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "cleanup" )
				{
					CommandID = COMMANDS.PreHeatMapOven;
				}
				else if( Command == "copy" )
				{
					CommandID = COMMANDS.SimpleCopy;
				}
				else if( Command == "sourcebuildcopy" )
				{
					CommandID = COMMANDS.SourceBuildCopy;
				}
				else if( Command == "delete" )
				{
					CommandID = COMMANDS.SimpleDelete;
				}
				else if( Command == "rename" )
				{
					CommandID = COMMANDS.SimpleRename;
				}
				else if( Command == "renamecopy" )
				{
					CommandID = COMMANDS.RenamedCopy;
				}
				else if( Command == "checkconsistency" )
				{
					CommandID = COMMANDS.SCC_CheckConsistency;
				}
				else if( Command == "getpublisheddata" )
				{
					CommandID = COMMANDS.GetPublishedData;
				}
				else if( Command == "getcookedbuild" )
				{
					CommandID = COMMANDS.GetCookedBuild;
				}
				else if( Command == "gettagset" )
				{
					CommandID = COMMANDS.GetTagset;
				}
				else if( Command == "getcookedlanguage" )
				{
					CommandID = COMMANDS.GetCookedLanguage;
				}
				else if( Command == "steammakeversion" )
				{
					CommandID = COMMANDS.SteamMakeVersion;
				}
				else if( Command == "updatesteamserver" )
				{
					CommandID = COMMANDS.UpdateSteamServer;
				}
				else if( Command == "restartsteamserver" )
				{
					CommandID = COMMANDS.RestartSteamServer;
				}
				else if( Command == "unsetuptype" )
				{
					UnSetupType = Parent.ExpandString( CommandLine, BranchDef.Branch );
					CommandID = COMMANDS.Config;
				}
				else if( Command == "unsetup" )
				{
					CommandID = COMMANDS.UnSetup;
				}
				else if( Command == "createdvdlayout" )
				{
					CommandID = COMMANDS.CreateDVDLayout;
				}
				else if( Command == "sync" )
				{
					CommandID = COMMANDS.SCC_Sync;
				}
				else if( Command == "synctohead" )
				{
					CommandID = COMMANDS.SCC_SyncToHead;
				}
				else if( Command == "outofbranchsync_temp" )
				{
					CommandID = COMMANDS.SCC_OutOfBranchSync;
				}
				else if( Command == "artistsync" )
				{
					CommandID = COMMANDS.SCC_ArtistSync;
				}
				else if( Command == "getchanges" )
				{
					CommandID = COMMANDS.SCC_GetChanges;
				}
				else if( Command == "sendqachanges" )
				{
					CommandID = COMMANDS.Config;
					IsSendingQAChanges = true;
				}
				else if( Command == "syncsinglechangelist" )
				{
					CommandID = COMMANDS.SCC_SyncSingleChangeList;
				}
				else if( Command == "checkout" )
				{
					CommandID = COMMANDS.SCC_Checkout;
				}
				else if( Command == "openfordelete" )
				{
					CommandID = COMMANDS.SCC_OpenForDelete;
				}
				else if( Command == "checkoutgame" )
				{
					CommandID = COMMANDS.SCC_CheckoutGame;
				}
				else if( Command == "checkoutgfwlgame" )
				{
					CommandID = COMMANDS.SCC_CheckoutGFWLGame;
				}
				else if( Command == "checkoutgadcheckpoint" )
				{
					CommandID = COMMANDS.SCC_CheckoutGADCheckpoint;
				}
				else if( Command == "checkoutshader" )
				{
					CommandID = COMMANDS.SCC_CheckoutShader;
				}
				else if( Command == "checkoutdialog" )
				{
					CommandID = COMMANDS.SCC_CheckoutDialog;
				}
				else if( Command == "checkoutfonts" )
				{
					CommandID = COMMANDS.SCC_CheckoutFonts;
				}
				else if( Command == "checkoutlocpackage" )
				{
					CommandID = COMMANDS.SCC_CheckoutLocPackage;
				}
				else if( Command == "checkoutgdf" )
				{
					CommandID = COMMANDS.SCC_CheckoutGDF;
				}
				else if( Command == "checkoutcat" )
				{
					CommandID = COMMANDS.SCC_CheckoutCat;
				}
				else if( Command == "checkoutlayout" )
				{
					CommandID = COMMANDS.SCC_CheckoutLayout;
				}
				else if( Command == "checkouthashes" )
				{
					CommandID = COMMANDS.SCC_CheckoutHashes;
				}
				else if( Command == "checkoutenginecontent" )
				{
					CommandID = COMMANDS.SCC_CheckoutEngineContent;
				}
				else if( Command == "checkoutgamecontent" )
				{
					CommandID = COMMANDS.SCC_CheckoutGameContent;
				}
				else if( Command == "submit" )
				{
					IsBuilding = true;
					IsTagging = false;
					CommandID = COMMANDS.SCC_Submit;
				}
				else if( Command == "submitandtag" )
				{
					IsBuilding = true;
					IsTagging = true;
					CommandID = COMMANDS.SCC_Submit;
				}
				else if( Command == "createnewlabel" || Command == "createlabel" )
				{
					CommandID = COMMANDS.SCC_CreateNewLabel;
				}
				else if( Command == "updatelabeldescription" )
				{
					CommandID = COMMANDS.SCC_LabelUpdateDescription;
				}
				else if( Command == "revert" )
				{
					CommandID = COMMANDS.SCC_Revert;
				}
				else if( Command == "revertfilespec" || Command == "revertfile" )
				{
					CommandID = COMMANDS.SCC_RevertFileSpec;
				}
				else if( Command == "waitforactive" )
				{
					CommandID = COMMANDS.WaitForActive;
				}
				else if( Command == "waitforjobs" )
				{
					CommandID = COMMANDS.WaitForJobs;
				}
				else if( Command == "addunrealgamejob" )
				{
					CommandID = COMMANDS.AddUnrealGameJob;
				}
				else if( Command == "addunrealfullgamejob" )
				{
					CommandID = COMMANDS.AddUnrealFullGameJob;
				}
				else if( Command == "addunrealgfwlgamejob" )
				{
					CommandID = COMMANDS.AddUnrealGFWLGameJob;
				}
				else if( Command == "addunrealgfwlfullgamejob" )
				{
					CommandID = COMMANDS.AddUnrealGFWLFullGameJob;
				}
				else if( Command == "clean" )
				{
					CommandID = COMMANDS.Clean;
				}
				else if( Command == "buildubt" )
				{
					CommandID = COMMANDS.BuildUBT;
				}
				else if( Command == "msbuild" )
				{
					CommandID = COMMANDS.MS8Build;
				}
				else if( Command == "ms9build" )
				{
					CommandID = COMMANDS.MS9Build;
				}
				else if( Command == "unrealbuild" )
				{
					CommandID = COMMANDS.UnrealBuild;
				}
				else if( Command == "msvcclean" )
				{
					CommandID = COMMANDS.MSVC8Clean;
				}
				else if( Command == "msvcbuild" )
				{
					CommandID = COMMANDS.MSVC8Build;
				}
				else if( Command == "msvcfull" )
				{
					CommandID = COMMANDS.MSVC8Full;
				}
				else if( Command == "msvc9clean" )
				{
					CommandID = COMMANDS.MSVC9Clean;
				}
				else if( Command == "msvc9build" )
				{
					CommandID = COMMANDS.MSVC9Build;
				}
				else if( Command == "msvc9deploy" )
				{
					CommandID = COMMANDS.MSVC9Deploy;
				}
				else if( Command == "msvc9full" )
				{
					CommandID = COMMANDS.MSVC9Full;
				}
				else if( Command == "gccclean" )
				{
					CommandID = COMMANDS.GCCClean;
				}
				else if( Command == "gccbuild" )
				{
					CommandID = COMMANDS.GCCBuild;
				}
				else if( Command == "gccfull" )
				{
					CommandID = COMMANDS.GCCFull;
				}
				else if( Command == "shaderclean" )
				{
					CommandID = COMMANDS.ShaderClean;
				}
				else if( Command == "shaderbuild" )
				{
					CommandID = COMMANDS.ShaderBuild;
				}
				else if( Command == "localshaderbuild" )
				{
					CommandID = COMMANDS.LocalShaderBuild;
				}
				else if( Command == "cookshaderbuild" )
				{
					CommandID = COMMANDS.CookShaderBuild;
				}
				else if( Command == "shaderfull" )
				{
					CommandID = COMMANDS.ShaderFull;
				}
				else if( Command == "ps3makepatchbinary" )
				{
					CommandID = COMMANDS.PS3MakePatchBinary;
				}
				else if( Command == "ps3makepatch" )
				{
					CommandID = COMMANDS.PS3MakePatch;
				}
				else if( Command == "ps3makedlc" )
				{
					CommandID = COMMANDS.PS3MakeDLC;
				}
				else if( Command == "ps3maketu" )
				{
					CommandID = COMMANDS.PS3MakeTU;
				}
				else if( Command == "pcmaketu" )
				{
					CommandID = COMMANDS.PCMakeTU;
				}
				else if( Command == "pcpackagetu" )
				{
					CommandID = COMMANDS.PCPackageTU;
				}
				else if( Command == "buildscript" )
				{
					CommandID = COMMANDS.BuildScript;
				}
				else if( Command == "buildscriptnoclean" )
				{
					CommandID = COMMANDS.BuildScriptNoClean;
				}
				else if( Command == "iphonepackage" )
				{
					CommandID = COMMANDS.iPhonePackage;
				}
				else if( Command == "preheatmapoven" )
				{
					CommandID = COMMANDS.PreHeatMapOven;
				}
				else if( Command == "preheatdlc" )
				{
					CommandID = COMMANDS.PreHeatDLC;
				}
				else if( Command == "cookmaps" )
				{
					CommandID = COMMANDS.CookMaps;
				}
				else if( Command == "cookinimaps" )
				{
					CommandID = COMMANDS.CookIniMaps;
				}
				else if( Command == "cooksounds" )
				{
					CommandID = COMMANDS.CookSounds;
				}
				else if( Command == "createhashes" )
				{
					CommandID = COMMANDS.CreateHashes;
				}
				else if( Command == "wrangle" )
				{
					CommandID = COMMANDS.Wrangle;
				}
				else if( Command == "publish" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.Publish;
				}
				else if( Command == "publishtagset" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.PublishTagset;
				}
				else if( Command == "publishlanguage" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.PublishLanguage;
				}
				else if( Command == "publishlayout" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.PublishLayout;
				}
				else if( Command == "publishlayoutlanguage" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.PublishLayoutLanguage;
				}
				else if( Command == "publishdlc" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.PublishDLC;
				}
				else if( Command == "publishtu" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.PublishTU;
				}
				else if( Command == "publishfiles" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.PublishFiles;
				}
				else if( Command == "publishrawfiles" )
				{
					IsPublishing = true;
					CommandID = COMMANDS.PublishRawFiles;
				}
				else if( Command == "makeiso" )
				{
					CommandID = COMMANDS.MakeISO;
				}
				else if( Command == "makemd5" )
				{
					CommandID = COMMANDS.MakeMD5;
				}
				else if( Command == "copyfolder" )
				{
					CommandID = COMMANDS.CopyFolder;
				}
				else if( Command == "movefolder" )
				{
					CommandID = COMMANDS.MoveFolder;
				}
				else if( Command == "cleanfolder" )
				{
					CommandID = COMMANDS.CleanFolder;
				}
				else if( Command == "conform" )
				{
					CommandID = COMMANDS.Conform;
				}
				else if( Command == "patchscript" )
				{
					CommandID = COMMANDS.PatchScript;
				}
				else if( Command == "checkpointgameassetdatabase" )
				{
					CommandID = COMMANDS.CheckpointGameAssetDatabase;
				}
				else if( Command == "updategameassetdatabase" )
				{
					CommandID = COMMANDS.UpdateGameAssetDatabase;
				}
				else if( Command == "tagreferencedassets" )
				{
					CommandID = COMMANDS.TagReferencedAssets;
				}
				else if( Command == "tagdvdassets" )
				{
					CommandID = COMMANDS.TagDVDAssets;
				}
				else if( Command == "auditcontent" )
				{
					CommandID = COMMANDS.AuditContent;
				}
				else if( Command == "findstaticmeshcanbecomedynamic" )
				{
					CommandID = COMMANDS.FindStaticMeshCanBecomeDynamic;
				}
				else if( Command == "fixupredirects" )
				{
					CommandID = COMMANDS.FixupRedirects;
				}
				else if( Command == "resavedeprecatedpackages" )
				{
					CommandID = COMMANDS.ResaveDeprecatedPackages;
				}
				else if( Command == "analyzereferencedcontent" )
				{
					AnalyzeReferencedContentConfiguration = Parent.ExpandString( CommandLine, BranchDef.Branch );
					Parent.Log( "AnalyzeReferencedContentConfiguration set to '" + AnalyzeReferencedContentConfiguration + "'", Color.Magenta );
					CommandID = COMMANDS.AnalyzeReferencedContent;
				}
				else if( Command == "minecookedpackages" )
				{
					CommandID = COMMANDS.MineCookedPackages;
				}
				else if( Command == "contentcomparison" )
				{
					CommandID = COMMANDS.ContentComparison;
				}
				else if( Command == "dumpmapsummary" )
				{
					CommandID = COMMANDS.DumpMapSummary;
				}
				else if( Command == "crossbuildconform" )
				{
					CommandID = COMMANDS.CrossBuildConform;
				}
				else if( Command == "trigger" )
				{
					CommandID = COMMANDS.Trigger;
				}
				else if( Command == "sqlexecint" )
				{
					CommandID = COMMANDS.SQLExecInt;
				}
				else if( Command == "sqlexecdouble" )
				{
					CommandID = COMMANDS.SQLExecDouble;
				}
				else if( Command == "validateinstall" )
				{
					CommandID = COMMANDS.ValidateInstall;
				}
				else if( Command == "bumpagentversion" )
				{
					CommandID = COMMANDS.BumpAgentVersion;
				}
				else if( Command == "bumpengineversion" )
				{
					CommandID = COMMANDS.BumpEngineVersion;
				}
				else if( Command == "getengineversion" )
				{
					CommandID = COMMANDS.GetEngineVersion;
				}
				else if( Command == "updategdfversion" )
				{
					CommandID = COMMANDS.UpdateGDFVersion;
				}
				else if( Command == "makegfwlcat" )
				{
					CommandID = COMMANDS.MakeGFWLCat;
				}
				else if( Command == "zdpp" )
				{
					CommandID = COMMANDS.ZDPP;
				}
				else if( Command == "steamdrm" )
				{
					CommandID = COMMANDS.SteamDRM;
				}
				else if( Command == "savedefines" )
				{
					CommandID = COMMANDS.SaveDefines;
				}
				else if( Command == "updatesymbolserver" )
				{
					CommandID = COMMANDS.UpdateSourceServer;
				}
				else if( Command == "addexetosymbolserver" )
				{
					CommandID = COMMANDS.AddExeToSymbolServer;
				}
				else if( Command == "adddlltosymbolserver" )
				{
					CommandID = COMMANDS.AddDllToSymbolServer;
				}
				else if( Command == "blast" )
				{
					CommandID = COMMANDS.Blast;
				}
				else if( Command == "blastla" )
				{
					CommandID = COMMANDS.BlastLA;
				}
				else if( Command == "blastdlc" )
				{
					CommandID = COMMANDS.BlastDLC;
				}
				else if( Command == "blasttu" )
				{
					CommandID = COMMANDS.BlastTU;
				}
				else if( Command == "xbla" )
				{
					CommandID = COMMANDS.XBLA;
				}
				else if( Command == "xlastgentu" )
				{
					CommandID = COMMANDS.XLastGenTU;
				}
				else if( Command == "sign" )
				{
					CommandID = COMMANDS.CheckSigned;
				}
				else if( Command == "signcat" )
				{
					CommandID = COMMANDS.SignCat;
				}
				else if( Command == "signbinary" )
				{
					CommandID = COMMANDS.SignBinary;
				}
				else if( Command == "signfile" )
				{
					CommandID = COMMANDS.SignFile;
				}
				else if( Command == "trackfilesize" )
				{
					CommandID = COMMANDS.TrackFileSize;
				}
				else if( Command == "trackfoldersize" )
				{
					CommandID = COMMANDS.TrackFolderSize;
				}
				else if( Command == "wait" )
				{
					CommandID = COMMANDS.Wait;
				}
				else if( Command == "checkforucinvcprojfiles" )
				{
					CommandID = COMMANDS.CheckForUCInVCProjFiles;
				}
				else if( Command == "smoketest" )
				{
					CommandID = COMMANDS.SmokeTest;
				}
				else if( Command == "loadpackages" )
				{
					CommandID = COMMANDS.LoadPackages;
				}
				else if( Command == "cookpackages" )
				{
					CommandID = COMMANDS.CookPackages;
				}
				else
				{
					CommandLine = Line;
					ErrorLevel = COMMANDS.IllegalCommand;
					CommandID = COMMANDS.Error;
				}
            }
            return ( CommandID );
        }

		/**
		 * Init the collation types for processing changelists
		 */
		public void InitCollationTypes()
		{
			ValidGames = UnrealControls.GameLocator.LocateGames( Environment.CurrentDirectory );

			CollationTypes[( int )COLLATION.Engine]			= new CollationType( COLLATION.Engine, "Engine Code Changes" );
			CollationTypes[( int )COLLATION.Audio]			= new CollationType( COLLATION.Audio, "Audio Code Changes" );
			CollationTypes[( int )COLLATION.Editor]			= new CollationType( COLLATION.Editor, "Editor Code Changes" );
			CollationTypes[( int )COLLATION.GFx]			= new CollationType( COLLATION.GFx, "GFx Code Changes" );

			int GameCollation = ( int )COLLATION.Game;
			foreach( string ValidGame in ValidGames )
			{
				CollationTypes[GameCollation] = new CollationType( ( COLLATION )GameCollation, ValidGame + " Code Changes" );
				GameCollation++;
				CollationTypes[GameCollation] = new CollationType( ( COLLATION )GameCollation, ValidGame + " Map Changes" );
				GameCollation++;
				CollationTypes[GameCollation] = new CollationType( ( COLLATION )GameCollation, ValidGame + " Content Changes" );
				GameCollation++;
			}

			CollationTypes[( int )COLLATION.Win32]			= new CollationType( COLLATION.Win32, "Win32 Specific Changes" );
			CollationTypes[( int )COLLATION.Win64]			= new CollationType( COLLATION.Win64, "Win64 Specific Changes" );
			CollationTypes[( int )COLLATION.Xbox360]		= new CollationType( COLLATION.Xbox360, "Xbox360 Changes" );
			CollationTypes[( int )COLLATION.PS3]			= new CollationType( COLLATION.PS3, "PS3 Changes" );
			CollationTypes[( int )COLLATION.Mobile]			= new CollationType( COLLATION.Mobile, "Mobile Changes" );
			CollationTypes[( int )COLLATION.Android]		= new CollationType( COLLATION.Android, "Android Changes" );
			CollationTypes[( int )COLLATION.iPhone]			= new CollationType( COLLATION.iPhone, "iPhone Changes" );
			CollationTypes[( int )COLLATION.Loc]			= new CollationType( COLLATION.Loc, "Localisation Changes" );
			CollationTypes[( int )COLLATION.Online]			= new CollationType( COLLATION.Online, "Online Changes" );
			CollationTypes[( int )COLLATION.GameSpy]		= new CollationType( COLLATION.GameSpy, "GameSpy Changes" );
			CollationTypes[( int )COLLATION.G4WLive]		= new CollationType( COLLATION.G4WLive, "G4W Live Changes" );
			CollationTypes[( int )COLLATION.Steam]			= new CollationType( COLLATION.Steam, "Steam Changes" );
			CollationTypes[( int )COLLATION.PiB]			= new CollationType( COLLATION.PiB, "PiB Changes" );
			CollationTypes[( int )COLLATION.Build]			= new CollationType( COLLATION.Build, "Build System Changes" );
			CollationTypes[( int )COLLATION.UnrealProp]		= new CollationType( COLLATION.UnrealProp, "UnrealProp Changes" );
			CollationTypes[( int )COLLATION.Swarm]			= new CollationType( COLLATION.Swarm, "Swarm Changes" );
			CollationTypes[( int )COLLATION.Lightmass]		= new CollationType( COLLATION.Lightmass, "Lightmass Changes" );
			CollationTypes[( int )COLLATION.Install]		= new CollationType( COLLATION.Install, "Installer Changes" );
			CollationTypes[( int )COLLATION.Tools]			= new CollationType( COLLATION.Tools, "Tools Changes" );

			// Set the order of the reports 
			Reports.Enqueue( COLLATION.Engine );
			Reports.Enqueue( COLLATION.Audio );
			Reports.Enqueue( COLLATION.Editor );
			Reports.Enqueue( COLLATION.GFx );

			int GameStep = ValidGames.Count;

			// Add in source changes per game
			GameCollation = ( int )COLLATION.Game;
			foreach( string ValidGame in ValidGames )
			{
				Reports.Enqueue( ( COLLATION )GameCollation );
				GameCollation += 3;
			}

			// Add in map changes per game
			GameCollation = ( ( int )COLLATION.Game ) + 1;
			foreach( string ValidGame in ValidGames )
			{
				Reports.Enqueue( ( COLLATION )GameCollation );
				GameCollation += 3;
			}

			// Add in content changes per game
			GameCollation = ( ( int )COLLATION.Game ) + 2;
			foreach( string ValidGame in ValidGames )
			{
				Reports.Enqueue( ( COLLATION )GameCollation );
				GameCollation += 3;
			}

			Reports.Enqueue( COLLATION.Win32 );
			Reports.Enqueue( COLLATION.Win64 );
			Reports.Enqueue( COLLATION.Xbox360 );
			Reports.Enqueue( COLLATION.PS3 );
			Reports.Enqueue( COLLATION.Mobile );
			Reports.Enqueue( COLLATION.Android );
			Reports.Enqueue( COLLATION.iPhone );

			Reports.Enqueue( COLLATION.Loc );
			Reports.Enqueue( COLLATION.Online );
			Reports.Enqueue( COLLATION.GameSpy );
			Reports.Enqueue( COLLATION.G4WLive );
			Reports.Enqueue( COLLATION.Steam );

			Reports.Enqueue( COLLATION.Swarm );
			Reports.Enqueue( COLLATION.Lightmass );

			Reports.Enqueue( COLLATION.Tools );
			Reports.Enqueue( COLLATION.PiB );
			Reports.Enqueue( COLLATION.Install );
			Reports.Enqueue( COLLATION.Build );
			Reports.Enqueue( COLLATION.UnrealProp );

			// Remember the prioritised order of the reporting tags
			AllReports = new List<COLLATION>( Reports );
			AllReports.Add( COLLATION.Count );
		}

		// Iterate over all submitted changelists, and extract a list of users
		public string[] GetPerforceUsers()
		{
			List<string> Users = new List<string>();

			// Extract unique P4 users
			foreach( ChangeList CL in ChangeLists )
			{
				if( !Users.Contains( CL.User.ToLower() ) )
				{
					Users.Add( CL.User.ToLower() );
				}
			}

			Users.Sort();
			return ( Users.ToArray() );
		}

        // Goes over a changelist description and extracts the relevant information
        public void ProcessChangeList( P4Record Record )
        {
            ChangeList CL = new ChangeList();

			CL.Description = Record.Fields["desc"];

			// Get the latest changelist number in this build - even if it is from the builder
			CL.Number = SafeStringToInt( Record.Fields["change"] );
			if( CL.Number > LabelInfo.Changelist )
			{
				LabelInfo.Changelist = CL.Number;
			}

			// Add any changelist that isn't from builder to a list for later processing
			if( !CL.Description.StartsWith( "[BUILDER] " ) )
			{
				CL.User = Record.Fields["user"];
				CL.Time = SafeStringToInt( Record.Fields["time"] );

				foreach( string DepotFile in Record.ArrayFields["depotFile"] )
				{
					CL.Files.Add( "    ... " + DepotFile );
				}

				ChangeLists.Add( CL );
			}
        }

        private COLLATION CollateGameChanges( COLLATION NewCollation, ChangeList CL, string CleanFile, string Game, int GameCollation )
        {
            if( CleanFile.IndexOf( "." + Game ) >= 0 )
            {
				return ( ( COLLATION )( GameCollation + 1 ) );
            }
            else if( CleanFile.IndexOf( "development/src/" + Game ) >= 0 )
            {
				return ( ( COLLATION )GameCollation );
            }
            else if( CleanFile.IndexOf( Game + "game/" ) >= 0 )
            {
				return ( ( COLLATION )( GameCollation + 2 ) );
            }

			return ( NewCollation );
        }

        private void CollateChanges()
        {
			if( ChangelistsCollated == true )
			{
				return;
			}

            foreach( ChangeList CL in ChangeLists )
            {
				COLLATION NewCollation = COLLATION.Count;

				// Check files that were checked in
				foreach( string File in CL.Files )
				{
					string CleanFile = File.ToLower().Replace( '\\', '/' );

					if( CleanFile.IndexOf( "development/src/core" ) >= 0 
						|| CleanFile.IndexOf( "development/src/engine" ) >= 0
						|| CleanFile.IndexOf( "development/src/gameframework" ) >= 0
						|| CleanFile.IndexOf( "development/src/launch" ) >= 0
						|| CleanFile.IndexOf( "engine/config" ) >= 0 
						|| CleanFile.IndexOf( "engine/content" ) >= 0 
						|| CleanFile.IndexOf( "engine/shaders" ) >= 0 )
					{
						NewCollation = COLLATION.Engine;
					}
					else if( CleanFile.IndexOf( "development/builder" ) >= 0
							 || CleanFile.IndexOf( "development/tools/builder" ) >= 0
							 || CleanFile.IndexOf( "development/src/targets" ) >= 0
							 || CleanFile.IndexOf( "development/src/unrealbuildtool" ) >= 0
							 || CleanFile.IndexOf( "development/src/xenon/unrealbuildtool" ) >= 0
							 || CleanFile.IndexOf( "development/src/ps3/unrealbuildtool" ) >= 0
							 || CleanFile.IndexOf( "binaries/cookersync.xml" ) >= 0 )
					{
						NewCollation = COLLATION.Build;
					}
					else if( CleanFile.IndexOf( "development/src/alaudio" ) >= 0
							 || CleanFile.IndexOf( "development/src/xaudio2" ) >= 0 )
					{
						NewCollation = COLLATION.Audio;
					}
					else if( CleanFile.IndexOf( "development/src/gfxui" ) >= 0
							 || CleanFile.IndexOf( "development/src/gfxuieditor" ) >= 0 )
					{
						NewCollation = COLLATION.GFx;
					}
					else if( CleanFile.IndexOf( "development/src/unrealed" ) >= 0
							 || CleanFile.IndexOf( "engine/editorresources/facefx" ) >= 0
							 || CleanFile.IndexOf( "engine/editorresources/wpf/controls" ) >= 0
							 || CleanFile.IndexOf( "engine/editorresources/wx" ) >= 0 )
					{
						NewCollation = COLLATION.Editor;
					}
					else if( CleanFile.IndexOf( "development/src/unrealswarm" ) >= 0
							 || CleanFile.IndexOf( "development/tools/unrealswarm" ) >= 0 )
					{
						NewCollation = COLLATION.Swarm;
					}
					else if( CleanFile.IndexOf( "development/src/ps3" ) >= 0 )
					{
						NewCollation = COLLATION.PS3;
					}
					else if( CleanFile.IndexOf( "development/src/xenon" ) >= 0 )
					{
						NewCollation = COLLATION.Xbox360;
					}
					else if( CleanFile.IndexOf( "development/src/d3d9drv" ) >= 0
							 || CleanFile.IndexOf( "development/src/d3d10drv" ) >= 0
							 || CleanFile.IndexOf( "development/src/windrv" ) >= 0
							 || CleanFile.IndexOf( "development/src/windows" ) >= 0 )
					{
						NewCollation = COLLATION.Win32;
					}
					else if( CleanFile.IndexOf( "development/src/android" ) >= 0 )
					{
						NewCollation = COLLATION.Android;
					}
					else if( CleanFile.IndexOf( "development/src/es2drv" ) >= 0
							 || CleanFile.IndexOf( "development/tools/mobile" ) >= 0
							 || CleanFile.IndexOf( "development/src/iphone" ) >= 0 )
					{
						NewCollation = COLLATION.iPhone;
					}
					else if( CleanFile.IndexOf( "development/src/onlinesubsystemgamespy" ) >= 0 )
					{
						NewCollation = COLLATION.GameSpy;
					}
					else if( CleanFile.IndexOf( "development/src/onlinesubsystemlive" ) >= 0 )
					{
						NewCollation = COLLATION.G4WLive;
					}
					else if( CleanFile.IndexOf( "development/src/onlinesubsystemPC" ) >= 0
							 || CleanFile.IndexOf( "development/src/ipdrv" ) >= 0 )
					{
						NewCollation = COLLATION.Online;
					}
					else if( CleanFile.IndexOf( "development/src/onlinesubsystemsteamworks" ) >= 0 )
					{
						NewCollation = COLLATION.Steam;
					}
					else if( CleanFile.IndexOf( "development/tools/pib" ) >= 0 )
					{
						NewCollation = COLLATION.PiB;
					}
					else if( CleanFile.IndexOf( "development/tools/unrealprop" ) >= 0 )
					{
						NewCollation = COLLATION.UnrealProp;
					}
					else if( CleanFile.IndexOf( "development/tools/unreallightmass" ) >= 0 )
					{
						NewCollation = COLLATION.Lightmass;
					}
					else if( CleanFile.IndexOf( "development/tools/unsetup" ) >= 0 )
					{
						NewCollation = COLLATION.Install;
					}
					else if( CleanFile.IndexOf( "development/tools" ) >= 0 )
					{
						NewCollation = COLLATION.Tools;
					}
					else if( CleanFile.IndexOf( "engine/localization" ) >= 0
							 || CleanFile.IndexOf( "engine/editorresources/wpf/localized/" ) >= 0 )
					{
						NewCollation = COLLATION.Loc;
					}
					else
					{
						int GameCollation = ( int )COLLATION.Game;
						foreach( string ValidGame in ValidGames )
						{
							NewCollation = CollateGameChanges( NewCollation, CL, CleanFile, ValidGame.ToLower(), GameCollation );
							GameCollation += 3;
						}
					}

					if( AllReports.IndexOf( NewCollation ) < AllReports.IndexOf( CL.Collate ) )
					{
						CL.Collate = NewCollation;
					}
				}

				if( CL.Collate < COLLATION.Count )
				{
					CollationTypes[( int )CL.Collate].Active = true;
				}

				Parent.Log( " ... changelist " + CL.Number.ToString() + " placed in collation " + CL.Collate.ToString(), Color.DarkGreen );

                // Remove identifiers and blank lines
                CL.CleanDescription();
            }

			ChangelistsCollated = true;
        }

        private bool GetCollatedChanges( StringBuilder Changes, COLLATION Collate, string User )
        {
            bool CollTypeHasEntries = false;
            CollationType CollType = CollationTypes[( int )Collate];
            if( CollType.Active )
            {
                foreach( ChangeList CL in ChangeLists )
                {
                    if( CL.Collate == Collate && ( User.Length == 0 || User.ToLower() == CL.User.ToLower() ) )
                    {
                        CollTypeHasEntries = true;
                        break;
                    }
                }

                if( CollTypeHasEntries )
                {
                    Changes.Append( "------------------------------------------------------------" + Environment.NewLine );
                    Changes.Append( "\t" + CollType.Title + Environment.NewLine );
                    Changes.Append( "------------------------------------------------------------" + Environment.NewLine + Environment.NewLine );

                    foreach( ChangeList CL in ChangeLists )
                    {
                        if( CL.Collate == Collate && ( User.Length == 0 || User.ToLower() == CL.User.ToLower() ) )
                        {
                            DateTime Time = new DateTime( 1970, 1, 1 );
                            Time += new TimeSpan( ( long )CL.Time * 10000 * 1000 );
                            Changes.Append( "Change: " + CL.Number.ToString() + " by " + CL.User + " on " + Time.ToLocalTime().ToString() + Environment.NewLine );
                            Changes.Append( CL.Description + Environment.NewLine );
                        }
                    }
                }
            }

            return ( CollTypeHasEntries );
        }

        public string GetChanges( string User )
        {
            bool HasChanges = false;
            StringBuilder Changes = new StringBuilder();

            Changes.Append( LabelInfo.Description );

            Changes.Append( Environment.NewLine );

            CollateChanges();

            foreach( COLLATION Collation in Reports )
            {
                HasChanges |= GetCollatedChanges( Changes, Collation, User );
            }

            if( HasChanges )
            {
                return ( Changes.ToString() );
            }

            return ( "" );
        }
	}
}
