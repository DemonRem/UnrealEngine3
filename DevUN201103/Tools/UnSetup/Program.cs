/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Deployment.Application;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Ionic.Zip;

namespace UnSetup
{
	static class Program
	{
		[DllImport( "kernel32.dll" )]
		static extern bool AttachConsole( UInt32 dwProcessId );

		private const UInt32 ATTACH_PARENT_PROCESS = 0xffffffff;

		static public Utils Util = null;

		static int ProcessArguments( string[] Arguments )
		{
			foreach( string Argument in Arguments )
			{
				if( Argument.ToLower().StartsWith( "-" ) )
				{
					return ( 1 );
				}
				if( Argument.ToLower().StartsWith( "/" ) )
				{
					return ( 2 );
				}
			}

			return ( 2 );
		}

		static string GetWorkFolder()
		{
			string TempFolder = Path.Combine( Path.GetTempPath(), "Epic-" + Util.InstallInfoData.InstallGuidString );
			Directory.CreateDirectory( TempFolder );

			return ( TempFolder );
		}

		static void CopyFile( string WorkFolder, string FileName )
		{
			FileInfo Source = new FileInfo( FileName );
			string DestFileName = Path.Combine( WorkFolder, Source.Name );
			
			// Ensure the destination folder exists
			Directory.CreateDirectory( Path.GetDirectoryName( DestFileName ) );

			// Delete the destination file if it already exists
			FileInfo Dest = new FileInfo( DestFileName );
			if( Dest.Exists )
			{
				Dest.IsReadOnly = false;
				Dest.Delete();
			}

			if( Source.Exists )
			{
				Source.CopyTo( DestFileName );
			}
		}

		static string ExtractWorkFiles()
		{
			// Set the CWD to a work folder
			string WorkFolder = GetWorkFolder();
			Environment.CurrentDirectory = WorkFolder;

			// Extract any files required by the setup
			Util.ExtractSingleFile( "Binaries\\UnSetup.exe" );
			Util.ExtractSingleFile( "Binaries\\UnSetup.exe.config" );
			Util.ExtractSingleFile( "Binaries\\UnSetup.Game.xml" );
			Util.ExtractSingleFile( "Binaries\\UnSetup.Manifests.xml" );
			
			// Extract the EULA for the local language (may not exist)
			string LocEULA = Util.GetLocFileName( "Binaries\\InstallData\\EULA", "rtf" );
			Util.ExtractSingleFile( LocEULA );

			// Always extract the INT EULA (the fallback in case the local language version does not exist)
			Util.ExtractSingleFile( "Binaries\\InstallData\\EULA.INT.rtf" );

			Util.ExtractSingleFile( "Binaries\\InstallData\\Interop.IWshRuntimeLibrary.dll" );

			Util.ExtractSingleFile( "Binaries\\Windows\\UE3Redist.exe" );

			return ( WorkFolder );
		}

		static string ExtractRedistWorkFiles()
		{
			// Set the CWD to a work folder
			string WorkFolder = GetWorkFolder();
			Environment.CurrentDirectory = WorkFolder;

			// Extract any files required by the setup
			Util.ExtractSingleFile( "Binaries/InstallData/EULA.rtf" );

			return ( WorkFolder );
		}

		static void DeleteWorkFile( string WorkFolder, string FileName )
		{
			string PathName = Path.Combine( WorkFolder, FileName );
			FileInfo Info = new FileInfo( PathName );
			if( Info.Exists )
			{
				Info.IsReadOnly = false;
				Info.Delete();
			}
		}

		static string CopyUnSetup()
		{
			string WorkFolder = GetWorkFolder();
#if DEBUG
			CopyFile( Path.Combine( WorkFolder, "Binaries\\" ), Path.Combine( Environment.CurrentDirectory, "UnSetup.exe" ) );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\" ), Path.Combine( Environment.CurrentDirectory, "UnSetup.exe.config" ) );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\" ), Path.Combine( Environment.CurrentDirectory, "UnSetup.Game.xml" ) );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\" ), Path.Combine( Environment.CurrentDirectory, "UnSetup.Manifests.xml" ) );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\InstallData\\" ), Path.Combine( Environment.CurrentDirectory, "InstallData\\InstallInfo.xml" ) );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\InstallData\\" ), Path.Combine( Environment.CurrentDirectory, "InstallData\\Interop.IWshRuntimeLibrary.dll" ) );
#else
			CopyFile( Path.Combine( WorkFolder, "Binaries\\" ), Application.ExecutablePath );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\" ), Application.ExecutablePath + ".config" );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\" ), Path.Combine( Application.StartupPath, "UnSetup.Game.xml" ) );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\" ), Path.Combine( Application.StartupPath, "UnSetup.Manifests.xml" ) );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\InstallData\\" ), Path.Combine( Application.StartupPath, "InstallData\\InstallInfo.xml" ) );
			CopyFile( Path.Combine( WorkFolder, "Binaries\\InstallData\\" ), Path.Combine( Application.StartupPath, "InstallData\\Interop.IWshRuntimeLibrary.dll" ) );
#endif
			return ( WorkFolder );
		}

		static bool Launch( string Executable, string WorkFolder, string CommandLine, bool bWait )
		{
			string FileName = Path.Combine( WorkFolder, "Binaries\\" + Executable );
			FileInfo ExecutableInfo = new FileInfo( FileName );

			if( ExecutableInfo.Exists )
			{
				ProcessStartInfo StartInfo = new ProcessStartInfo();

				StartInfo.FileName = FileName;
				StartInfo.Arguments = CommandLine;
				StartInfo.WorkingDirectory = Path.Combine( WorkFolder, "Binaries\\" );

				Process LaunchedProcess = Process.Start( StartInfo );

				if( bWait == true )
				{
					return Util.WaitForProcess( LaunchedProcess, 12000 );
				}
			}

			return( true );
		}

		static void DisplayErrorMessage( string ErrorMessage )
		{
			string DisplayMessage = Util.GetPhrase( "ErrorMessage" ) + " " + ErrorMessage;
			DisplayMessage += Environment.NewLine + Util.GetPhrase( "UDKTrouble" ) + Environment.NewLine + " http://udk.com/troubleshooting";

			GenericQuery Query = new GenericQuery( "GQCaptionInstallFail", DisplayMessage, false, "GQCancel", true, "GQOK" );
			Query.ShowDialog();
		}

		static void UpdatePath()
		{
			// There has to be a better way of doing this
			IDictionary MachineEnvVars = Environment.GetEnvironmentVariables( EnvironmentVariableTarget.Machine );
			IDictionary UserEnvVars = Environment.GetEnvironmentVariables( EnvironmentVariableTarget.User );

			string SystemPath = "";
			string UserPath = "";

			foreach( DictionaryEntry Entry in MachineEnvVars )
			{
				string Key = ( string )Entry.Key;
				if( Key.ToLower() == "path" )
				{
					SystemPath = ( string )Entry.Value;
					break;
				}
			}

			foreach( DictionaryEntry Entry in UserEnvVars )
			{
				string Key = ( string )Entry.Key;
				if( Key.ToLower() == "path" )
				{
					UserPath = ( string )Entry.Value;
					break;
				}
			}

			Environment.SetEnvironmentVariable( "Path", SystemPath + ";" + UserPath );
		}

		static bool HandleInstallRedist( bool Standalone, bool IsGameInstall )
		{
			// Work out the name of the zip with the redist in it
			string ModuleName = Util.MainModuleName;
			if( Standalone )
			{
				// ModuleName is the UDK install name, so need to remap to the redist package
				ModuleName = "Windows/UE3Redist.exe";
			}

			// Extract all the redist files
			if( Util.OpenPackagedZipFile( ModuleName, Util.UDKStartRedistSignature ) )
			{
				Util.CreateProgressBar( Util.GetPhrase( "PBDecompPrereq" ) );

				// Extract all the redist elements
				string DestFolder = Path.Combine( GetWorkFolder(), "Redist\\" );
				Util.UnzipAllFiles( Util.MainZipFile, DestFolder );

				Util.DestroyProgressBar();

				RedistProgress Progress = new RedistProgress();
				Progress.Show();

				Util.bDependenciesSuccessful = true;
				try
				{
					string Status = "OK";

					// Install the VC redistributables for UE3Redist only (not as part of the UDK install)
					if( !Standalone && !IsGameInstall )
					{
						Status = Util.InstallVCRedist( Progress, DestFolder );
						if( Status != "OK" )
						{
							DisplayErrorMessage( Status );
							Util.bDependenciesSuccessful = false;
						}
					}

					// Install a minimal set up DirectX - this is a different set for UE3Redist vs. UE3RedistGame
					Status = Util.InstallDXCutdown( Progress, DestFolder );
					if( Status != "OK" )
					{
						DisplayErrorMessage( Status );
						Util.bDependenciesSuccessful = false;
					}

					// Install the Microsoft charting tools
					if( !IsGameInstall )
					{
						Status = Util.InstallMSCharting( Progress, DestFolder );
						if( Status != "OK" )
						{
							DisplayErrorMessage( Status );
							Util.bDependenciesSuccessful = false;
						}
					}

					Status = Util.InstallAMDCPUDrivers( Progress, DestFolder );
					if( Status != "OK" )
					{
						DisplayErrorMessage( Status );
						Util.bDependenciesSuccessful = false;
					}
				}
				catch( Exception Ex )
				{
					DisplayErrorMessage( Ex.Message );
					Util.bDependenciesSuccessful = false;
				}

				// Apply any path changes that may have occurred in the redist, and set for the current process
				UpdatePath();

				// Cleanup
				Util.ClosePackagedZipFile();

				// Close the progress bar
				Progress.Close();

				return ( Util.bDependenciesSuccessful );
			}

			return ( false );
		}

		static void InstallRedist()
		{
			DialogResult EULAResult = DialogResult.OK;
			if( !Util.bProgressOnly )
			{
				EULA EULAScreen = new EULA();
				EULAResult = EULAScreen.ShowDialog();
			}
			if( EULAResult == DialogResult.OK )
			{
				bool IsGameInstall = Util.MainModuleName.ToLower().Contains( "game" );
				if( HandleInstallRedist( false, IsGameInstall ) )
				{
					if( !Util.bProgressOnly )
					{
						GenericQuery Query = new GenericQuery( "GQCaptionRedistInstallComplete", "GQDescRedistInstallComplete", false, "GQCancel", true, "GQOK" );
						Query.ShowDialog();
					}
				}
			}
		}

		static void DoUninstall( bool bAll, bool bIsGame )
		{
			UninstallProgress Progress = new UninstallProgress( Util );
			Progress.Show();
			Application.DoEvents();

			Util.DeleteFiles( bAll, bIsGame );
			Progress.SetDeletingShortcuts();
			Util.DeleteShortcuts();

			// Remove from ARP
			Util.RemoveUDKFromARP();
			Progress.Close();

			GenericQuery Query = new GenericQuery( "GQCaptionUninstallComplete", "GQDescUninstallComplete", false, "GQCancel", true, "GQOK" );
			Query.ShowDialog();
		}

		static void HandleUninstall()
		{
			Util.PackageInstallLocation = Util.MainModuleName.Substring( 0, Util.MainModuleName.Length - "Binaries\\UnSetup.exe".Length ).TrimEnd( "\\/".ToCharArray() );
			// Fix path if user installed to drive root
			if( Util.PackageInstallLocation.EndsWith( ":" ) )
			{
				Util.PackageInstallLocation += "\\";
			}

			// Are we a game or the UDK
			FileInfo UDKManifest = new FileInfo( Path.Combine( Util.PackageInstallLocation, Util.ManifestFileName ) );
			bool bIsGameInstall = !UDKManifest.Exists;

			UninstallOptions UninstallOptionsScreen = new UninstallOptions( bIsGameInstall );
			DialogResult UninstallOptionResult = UninstallOptionsScreen.ShowDialog();
			if( UninstallOptionResult == DialogResult.OK )
			{
				if( UninstallOptionsScreen.GetDeleteAll() )
				{
					GenericQuery AreYouSure = new GenericQuery( "GQCaptionUninstallAllSure", "GQDescUninstallAllSure", true, "GQCancel", true, "GQYes" );
					DialogResult AreYouSureResult = AreYouSure.ShowDialog();
					if( AreYouSureResult == DialogResult.OK )
					{
						DoUninstall( true, bIsGameInstall );
					}
				}
				else
				{
					DoUninstall( false, bIsGameInstall );
				}
			}
		}

		static void HandleInstall()
		{
			GenericQuery Query = null;

			// Read in the install specific data
			bool IsGameInstall = Util.IsFileInPackage( "Binaries\\UnSetup.Game.xml" );
			Util.CloseSplash();

			DialogResult EULAResult = DialogResult.OK;
			if( !Util.bProgressOnly )
			{
				EULA EULAScreen = new EULA();
				EULAResult = EULAScreen.ShowDialog();
			}
			if( EULAResult == DialogResult.OK )
			{
				InstallOptions InstallOptionsScreen = new InstallOptions( IsGameInstall );
				DialogResult InstallOptionResult = DialogResult.OK;
				if( !Util.bProgressOnly )
				{
					InstallOptionResult = InstallOptionsScreen.ShowDialog();
				}
				if( InstallOptionResult == DialogResult.OK )
				{
					string Email = InstallOptionsScreen.GetSubscribeAddress();
					if( Email.Length > 0 )
					{
						Query = new GenericQuery( "GQCaptionSubscribe", "GQDescSubscribe", false, "GQCancel", false, "GQOK" );
						Query.Show();
						Application.DoEvents();

						Util.SubscribeToMailingList( Email );

						Query.Close();
					}

					if( !Util.bSkipDependencies )
					{
						HandleInstallRedist( true, IsGameInstall );
					}

					Util.CreateProgressBar( Util.GetPhrase( "PBDecompressing" ) );

					string DestFolder = InstallOptionsScreen.GetInstallLocation();

					Util.OpenPackagedZipFile( Util.MainModuleName, Util.UDKStartZipSignature );
					if( Util.UnzipAllFiles( Util.MainZipFile, DestFolder ) )
					{
						Util.ClosePackagedZipFile();

						// Run each game to generate the proper ini files...
						Util.UpdateProgressBar( Util.GetPhrase( "PBSettingInitial" ) );
						foreach( Utils.GameManifestOptions GameInstallInfo in Util.Manifest.GameInfo )
						{
							Launch( GameInstallInfo.AppToCreateInis, DestFolder, "-firstinstall -LanguageForCooking=" + Util.Phrases.GetUE3Language(), true );
						}

						// Adding shortcuts to the start menu
						Util.UpdateProgressBar( Util.GetPhrase( "GQAddingShortcuts" ) );

						string StartMenu = Util.GetAllUsersStartMenu();

						Application.DoEvents();

						// If they don't exist (are default) then we are installing the UDK
						if( !IsGameInstall )
						{
							StartMenu = Path.Combine( StartMenu, InstallOptionsScreen.GetStartMenuLocation() );
							Directory.CreateDirectory( StartMenu );
                            Directory.CreateDirectory(Path.Combine(StartMenu, "Documentation\\"));
                            Directory.CreateDirectory(Path.Combine(StartMenu, "Tools\\"));

							// Only include SpeedTreeModeler shortcuts if the SpeedTreeModeler folder exists
							DirectoryInfo DirInfo = new DirectoryInfo( Path.Combine( InstallOptionsScreen.GetInstallLocation(), "Binaries\\SpeedTreeModeler\\" ) );

							Util.CreateShortcuts( StartMenu, InstallOptionsScreen.GetInstallLocation(), DirInfo.Exists );

							// Adding to ARP
							Util.UpdateProgressBar( Util.GetPhrase( "RegUDK" ) );
							Util.AddUDKToARP( DestFolder, Util.Manifest.FullName + ": " + Util.UnSetupTimeStamp );
						}
						else
						{
							Util.CreateGameShortcuts( StartMenu, InstallOptionsScreen.GetInstallLocation() );

							// Adding to ARP
							Util.UpdateProgressBar( Util.GetPhrase( "RegGame" ) );
							Util.AddUDKToARP( DestFolder, Util.GetGameLongName() );
						}

						// Delete as many work files as we can
						Util.UpdateProgressBar( Util.GetPhrase( "CleaningUp" ) );
						Util.DeleteTempFiles( GetWorkFolder() );

						// Save install guid
						Util.SaveInstallInfo( Path.Combine( DestFolder, "Binaries" ) );

						// Destroy the progress bar so it does not obscure the finished dialog
						Util.DestroyProgressBar();

						InstallFinished IF = new InstallFinished( IsGameInstall );
						DialogResult InstallFinishedResult = DialogResult.Cancel;
						if( !Util.bProgressOnly )
						{
							InstallFinishedResult = IF.ShowDialog();
						}

						// Launch game or editor if requested
						if( InstallFinishedResult == DialogResult.OK )
						{
							if( IsGameInstall )
							{
								if( IF.GetLaunchChecked() )
								{
									Launch( Util.Manifest.AppToLaunch, DestFolder, "", false );
								}
							}
							else
							{
								Utils.GameManifestOptions AppToLaunch = IF.GetLaunchType();
								if( AppToLaunch != null )
								{
									Launch( AppToLaunch.AppToElevate, DestFolder, AppToLaunch.AppCommandLine, false );
								}
							}
						}
					}
					else
					{
						Util.ClosePackagedZipFile();

						Util.DestroyProgressBar();
					}
				}
			}
		}

		[STAThread]
		static int Main( string[] Arguments )
		{
			AttachConsole( ATTACH_PARENT_PROCESS );

			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault( false );

			string TLLN = System.Threading.Thread.CurrentThread.CurrentUICulture.ThreeLetterWindowsLanguageName;
			string ParentTLLN = System.Threading.Thread.CurrentThread.CurrentUICulture.Parent.ThreeLetterWindowsLanguageName;
			Util = new Utils( TLLN, ParentTLLN );

			Application.CurrentCulture = System.Globalization.CultureInfo.InvariantCulture;
			string WorkFolder = "";
			string CommandLine = "";
			int ErrorCode = 0;

			switch( ProcessArguments( Arguments ) )
			{
			case 0:
				break;

			case 1:
				ErrorCode = Util.ProcessBuildCommandLine( Arguments );

				switch( Util.BuildCommand )
				{
				case Utils.BUILDCOMMANDS.CreateManifest:
					Util.CreateManifest();
					break;

				case Utils.BUILDCOMMANDS.GameCreateManifest:
					ErrorCode = Util.GameCreateManifest();
					break;

				case Utils.BUILDCOMMANDS.BuildInstaller:
					ErrorCode = Util.BuildInstaller( Util.ManifestFileName );
					break;

				case Utils.BUILDCOMMANDS.BuildGameInstaller:
					Util.BuildInstaller( Util.GameManifestFileName );
					break;

				case Utils.BUILDCOMMANDS.Package:
					Util.PackageFiles();
					break;

				case Utils.BUILDCOMMANDS.PackageRedist:
					Util.PackageRedist( false );
					break;
#if DEBUG
				case Utils.BUILDCOMMANDS.UnPackage:
					Util.UnPackageFiles();
					break;
#endif
				}
				break;

			case 2:
				// Process any command line options
				Util.ProcessInstallCommandLine( Arguments );

				switch( Util.Command )
				{
				case Utils.COMMANDS.Help:
					Util.CloseSplash();
					Util.DisplayHelp();
					break;

				case Utils.COMMANDS.EULA:
					Util.CloseSplash();
					ErrorCode = Util.DisplayEULA();
					break;

				case Utils.COMMANDS.Manifest:
					Util.ReadManifestOptions();
					Util.CloseSplash();
					ManifestDialog Manifest = new ManifestDialog( Util.Manifest );
					Manifest.ShowDialog();
					Util.SaveManifestOptions();
					break;

				case Utils.COMMANDS.Game:
					Util.CloseSplash();
					GameDialog Game = new GameDialog();
					DialogResult Result = Game.ShowDialog();
					if( Result == DialogResult.OK )
					{
						Util.SaveGame();
					}
					else
					{
						ErrorCode = 1;
					}
					break;

				case Utils.COMMANDS.SetupInstall:
					if( Util.MainModuleName.EndsWith( "UnSetup.exe" ) )
					{
						Util.CloseSplash();
						Util.DisplayHelp();
					}
					else if( Util.OpenPackagedZipFile( Util.MainModuleName, Util.UDKStartZipSignature ) )
					{
						// Find the offset into the packaged file that represents the zip
						WorkFolder = ExtractWorkFiles();
						CommandLine = Util.GetCommandLine( "/HandleInstall" );
						Launch( "UnSetup.exe", WorkFolder, CommandLine, false );
					}
					break;

				case Utils.COMMANDS.Install:
					// Find the offset into the packaged file that represents the zip
					if( Util.OpenPackagedZipFile( Util.MainModuleName, Util.UDKStartZipSignature ) )
					{
						HandleInstall();
					}
					break;

				case Utils.COMMANDS.SetupUninstall:
					WorkFolder = CopyUnSetup();
					CommandLine = Util.GetCommandLine( "/HandleUninstall" );
					Launch( "UnSetup.exe", WorkFolder, CommandLine, false );
					break;

				case Utils.COMMANDS.Uninstall:
					HandleUninstall();
					break;

				case Utils.COMMANDS.Redist:
					// Find the offset into the packaged file that represents the zip
					if( Util.OpenPackagedZipFile( Util.MainModuleName, Util.UDKStartRedistSignature ) )
					{
						WorkFolder = ExtractRedistWorkFiles();
						CommandLine = Util.GetCommandLine( "/HandleRedist" );
						Launch( "UnSetup.exe", WorkFolder, CommandLine, false );
					}
					break;

				case Utils.COMMANDS.HandleRedist:
					Util.CloseSplash();
					InstallRedist();
					break;
#if DEBUG
				case Utils.COMMANDS.Extract:
					Util.MainZipFile = ZipFile.Read( Util.MainModuleName );
					ExtractWorkFiles();
					break;

				case Utils.COMMANDS.Subscribe:
					Util.SubscribeToMailingList( Util.SubscribeEmail );
					break;

				case Utils.COMMANDS.CheckSignature:
					Util.ValidateCertificate( Util.MainModuleName );
					break;

				case Utils.COMMANDS.Shortcuts:
					string StartMenu = Util.GetAllUsersStartMenu() + "\\UDKTest";
					Directory.CreateDirectory( StartMenu );
                    Directory.CreateDirectory(Path.Combine(StartMenu, "Documentation\\"));
                    Directory.CreateDirectory(Path.Combine(StartMenu, "Tools\\"));
                    Directory.CreateDirectory(Path.Combine(StartMenu, "Uninstall\\"));

					Util.CreateShortcuts( StartMenu, "C:\\UDK\\UDK-2009-09", false );
					break;
#endif
				}

				break;
			}

			Util.Destroy();
			return ( ErrorCode );
		}
	}
}
