using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using Microsoft.Win32;

namespace Controller
{
    public partial class Command
	{
		// To do:
		// Retrieve versions from database

		// To verify:
		// SQL server version
		// - MSVC settings
		// - Tegra SDK version
		// - branch names start with UnrealEngine3
		// - check OS version (Vista SP2 required)

		private bool ValidateEnvVarFolder( string EnvVar )
		{
			Builder.Write( "Checking: " + EnvVar );
			string Folder = Environment.GetEnvironmentVariable( EnvVar );
			if( Folder != null )
			{
				DirectoryInfo DirInfo = new DirectoryInfo( Folder );
				if( !DirInfo.Exists )
				{
					Builder.Write( "VALIDATION ERROR: folder does not exist: " + Folder );
					return ( false );
				}

				return ( true );
			}

			Builder.Write( "VALIDATION ERROR: Failed to find environment variable: " + EnvVar );
			return ( false );
		}

		private bool ValidateOSVersion()
		{
			OperatingSystem OSVersion = Environment.OSVersion;

			if( OSVersion.Version.Major < 6 )
			{
				Builder.Write( "VALIDATION ERROR: Operating systems before Vista are not supported" );
				return ( false );
			}
			else if( OSVersion.Version.Minor < 1 )
			{
				// Check for Vista SP2
				if( OSVersion.Version.MajorRevision < 2 )
				{
					// Required for the ISO handling in CookerSync and MakeISO (IMAPI2 update)
					Builder.Write( "VALIDATION ERROR: Vista must be at least Service Pack 2" );
					return ( false );
				}
			}

			return ( true );
		}

		private bool ValidateTimeZone()
		{
			TimeZone LocalZone = TimeZone.CurrentTimeZone;

			// This can be any time zone you like, but must be the same across all build machines
			// Everything is internally UTC, except for label name generation (and hence folder name generation)
			if( LocalZone.StandardName != "Eastern Standard Time" )
			{
				Builder.Write( "VALIDATION ERROR: TimeZone is not EST" );
				return ( false );
			}

			return ( true );
		}

		// Check p4win
		private bool ValidateP4()
		{
			if( !SCC.ValidateP4Settings( Builder ) )
			{
				return ( false );
			}

			Builder.Write( "Perforce settings validated" );
			return ( true );
		}

		// Settings - no DXSDK_DIR
		// Settings - add GFWL
		private bool ValidateMSVC9()
		{
			if( !ValidateEnvVarFolder( "FrameworkDir" ) )
			{
				return ( false );
			}

			if( Environment.GetEnvironmentVariable( "FrameworkVersion" ) != "v2.0.50727" )
			{
				Builder.Write( "VALIDATION ERROR: FrameworkVersion != v2.0.50727" );
				return ( false );
			}

			if( Parent.MSVC9Version != "Microsoft Visual Studio 2008 version 9.0.30729.1" )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find correct version of MSVC 2008." );
				return( false );
			}

			RegistryKey Key = Registry.LocalMachine.OpenSubKey( "SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\" );
			string WindowsSDKDir = ( string )Key.GetValue( "CurrentInstallFolder" );
			if( !WindowsSDKDir.ToLower().EndsWith( "6.0a\\" ) )
			{
				Builder.Write( "VALIDATION ERROR: Windows SDK folder not set to 6.0A." );
				return ( false );
			}

			Key = Registry.LocalMachine.OpenSubKey( "SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v7.0" );
			if( Key == null )
			{
				Builder.Write( "VALIDATION ERROR: Windows SDK 7.0 is not installed; this is required for the GFWL installer." );
				return ( false );
			}

			Builder.Write( "Validated MSVC9 as " + Parent.MSVC9Version );
			return ( true );
		}

		// DX version matches SQL entry
		private bool ValidateDX()
		{
			string DXFolder = "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)";
			DirectoryInfo DirInfo = new DirectoryInfo( DXFolder );
			if( !DirInfo.Exists )
			{
				Builder.Write( "DX not installed: " + DXFolder );
				return ( false );
			}

			Builder.Write( "DirectX version validated as June 2010" );
			return ( true );
		}

		// XDK matches SQL entry
		private bool ValidateXDK()
		{
			if( !ValidateEnvVarFolder( "XEDK" ) )
			{
				return ( false );
			}

			if( Parent.XDKVersion != "11626" && Parent.XDKVersion != "7645" && Parent.XDKVersion != "20209" )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find correct version of the XDK." );
				return ( false );
			}

			Builder.Write( "XDK validated as 11626, 20209 or 7645" );
			return ( true );
		}

		// PS3 SDK matches SQL entry
		private bool ValidatePS3()
		{
			if( !ValidateEnvVarFolder( "SCE_PS3_ROOT" ) )
			{
				return ( false );
			}

			if( !ValidateEnvVarFolder( "SN_PS3_PATH" ) )
			{
				return ( false );
			}

			if( !ValidateEnvVarFolder( "SN_COMMON_PATH" ) )
			{
				return ( false );
			}

			string CELL_SDK = Environment.GetEnvironmentVariable( "CELL_SDK" );
			if( CELL_SDK == null || CELL_SDK != "/c/PS3/cell/current" )
			{
				Builder.Write( "VALIDATION ERROR: CELL_SDK is set incorrectly" );
				return ( false );
			}

			if( Parent.PS3SDKVersion != "330.001" && Parent.PS3SDKVersion != "350.001" )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find correct version of the PS3 SDK." );
				return ( false );
			}

			Builder.Write( "PS3 SDK validated as 330.001 or 350.001" );
			return ( true );
		}

		// Make sure GFWL SDK installed and is the correct version
		private bool ValidateGFWL()
		{
			if( !ValidateEnvVarFolder( "GFWLSDK_DIR" ) )
			{
				return ( false );
			}

			FileInfo Info = new FileInfo( Path.Combine( Environment.GetEnvironmentVariable( "GFWLSDK_DIR" ), "bin\\x86\\security-enabled\\xlive.dll" ) );

			Version ReferenceVersion = new Version( 3, 2, 3, 0 );
			FileVersionInfo VersionInfo = FileVersionInfo.GetVersionInfo( Info.FullName );
			Version GFWLVersion = new Version( VersionInfo.FileMajorPart, VersionInfo.FileMinorPart, VersionInfo.FileBuildPart, VersionInfo.FilePrivatePart );

			if( GFWLVersion != ReferenceVersion )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find correct version of xlive.dll; found version : " + VersionInfo.FileVersion );
				return ( false );
			}

			Builder.Write( "GFWL validated" );
			return ( true );
		}

		// Make sure Perl is installed for the source server
		private bool ValidatePerl()
		{
			DirectoryInfo Dir32Info = new DirectoryInfo( "C:\\Perl\\Bin" );
			DirectoryInfo Dir64Info = new DirectoryInfo( "C:\\Perl64\\Bin" );

			FileInfo PerlBinaryFileInfo = null;

			// Check version
			if( Dir32Info.Exists )
			{
				PerlBinaryFileInfo = new FileInfo( "C:\\Perl\\Bin\\Perl.exe" );
				if( !PerlBinaryFileInfo.Exists )
				{
					Builder.Write( "VALIDATION ERROR: Failed to find Perl at C:\\Perl\\Bin\\Perl.exe" );
					return ( false );
				}
			}
			else if( Dir64Info.Exists )
			{
				PerlBinaryFileInfo = new FileInfo( "C:\\Perl64\\Bin\\Perl.exe" );
				if( !PerlBinaryFileInfo.Exists )
				{
					Builder.Write( "VALIDATION ERROR: Failed to find Perl at C:\\Perl64\\Bin\\Perl.exe" );
					return ( false );
				}
			}
			else
			{
				Builder.Write( "VALIDATION ERROR: Failed to find Perl at either C:\\Perl\\Bin or C:\\Perl64\\Bin" );
				return ( false );
			}

			Version ReferenceVersionA = new Version( 5, 8, 8, 819 );
			Version ReferenceVersionB = new Version( 5, 10, 1, 1006 );
			FileVersionInfo VersionInfo = FileVersionInfo.GetVersionInfo( PerlBinaryFileInfo.FullName );
			Version PerlVersion = new Version( VersionInfo.FileMajorPart, VersionInfo.FileMinorPart, VersionInfo.FileBuildPart, VersionInfo.FilePrivatePart );

			if( PerlVersion != ReferenceVersionA && PerlVersion != ReferenceVersionB )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find correct version of Perl.exe; found version : " + VersionInfo.FileVersion );
				return ( false );
			}

			Builder.Write( "Perl validated" );
			return ( true );
		}

		// Make sure Debugging tools for windows is installed for the source and symbol server
		private bool ValidateDebuggingTools()
		{
			// Check for x86 version
			// TODO: check for x64 also
			FileInfo SourceServerCmdInfo = new FileInfo( Builder.SourceServerCmd );
			if( !SourceServerCmdInfo.Exists )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find source server command: " + Builder.SourceServerCmd );
				return ( false );
			}

			FileInfo SymbolStoreAppInfo = new FileInfo( Builder.SymbolStoreApp );
			if( !SymbolStoreAppInfo.Exists )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find symbol store application: " + Builder.SymbolStoreApp );
				return ( false );
			}

			Builder.Write( "Debugging Tools for Windows validated" );
			return ( true );
		}

		// Make sure we have ILMerge installed for UnSetup
		private bool ValidateILMerge()
		{
			FileInfo ILMerge = new FileInfo( "C:\\Program Files (x86)\\Microsoft\\ILMerge\\Ilmerge.exe" );
			if( !ILMerge.Exists )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find ILMerge: " + ILMerge.FullName );
				return ( false );
			}

			Builder.Write( "ILMerge validated" );
			return ( true );
		}
	
		// Make sure we have the signing tools available and our certificate for signing
		private bool ValidateSigningKey()
		{
			FileInfo SignToolNameInfo = new FileInfo( Builder.SignToolName );
			if( !SignToolNameInfo.Exists )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find signtool: " + Builder.SignToolName );
				return ( false );
			}

			FileInfo CatToolNameInfo = new FileInfo( Builder.CatToolName );
			if( !CatToolNameInfo.Exists )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find cat: " + Builder.CatToolName );
				return ( false );
			}

			// Check to see if we have the pfx is installed
			X509Certificate2Collection CertificateCollection;
			X509Store CertificateStore = new X509Store( StoreLocation.CurrentUser );
			try
			{
				// create and open store for read-only access
				CertificateStore.Open( OpenFlags.ReadOnly );

				// search store for Epic cert
				CertificateCollection = CertificateStore.Certificates.Find( X509FindType.FindBySerialNumber, "1c 16 cd da d8 96 19 07 e5 70 4b 82 d9 a1 58 42", true );
				if( CertificateCollection.Count == 0 )
				{
					Builder.Write( "VALIDATION ERROR: Failed to find certificate" );
					return ( false );
				}
			}
			// always close the store
			finally
			{
				CertificateStore.Close();
			}

			Builder.Write( "Signing tools validated" );
			return ( true );
		}

		// Make sure we have ILMerge installed for UnSetup
		private bool ValidateAndroidSDKInstall()
		{
			DirectoryInfo AndroidSDKDirectory = new DirectoryInfo( "C:\\Android" );
			if( !AndroidSDKDirectory.Exists )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find Android SDK install: " + AndroidSDKDirectory.FullName );
				return ( false );
			}

			Builder.Write( "Android SDK install validated" );
			return ( true );
		}

		private bool ValidateNGPSDKInstall()
		{
			DirectoryInfo SCERootDirectory = new DirectoryInfo( "C:\\Program Files (x86)\\SCE\\" );
			if( !SCERootDirectory.Exists )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find NGP SDK install: " + SCERootDirectory.FullName );
				return ( false );
			}
			DirectoryInfo NGPSDKDirectory = new DirectoryInfo( "C:\\Program Files (x86)\\SCE\\PSP2 SDKs\\0.945" );
			if( !NGPSDKDirectory.Exists )
			{
				Builder.Write( "VALIDATION ERROR: Failed to find NGP SDK install: " + NGPSDKDirectory.FullName );
				return ( false );
			}

			Builder.Write( "NGP SDK install validated" );
			return ( true );
		}

		private void ValidateInstall()
		{
			string LogFileName = Builder.GetLogFileName( COMMANDS.ValidateInstall );
			Builder.OpenLog( LogFileName, false );
			bool Validated = true;

			try
			{
				Validated &= ValidateOSVersion();
				Validated &= ValidateTimeZone();
				Validated &= ValidateP4();
				Validated &= ValidateMSVC9();
				Validated &= ValidateDX();
				Validated &= ValidateXDK();
				Validated &= ValidatePS3();
				Validated &= ValidateGFWL();
				Validated &= ValidatePerl();
				Validated &= ValidateDebuggingTools();
				Validated &= ValidateILMerge();
				Validated &= ValidateSigningKey();
				Validated &= ValidateAndroidSDKInstall();
				Validated &= ValidateNGPSDKInstall();
			}
			catch( Exception Ex )
			{
				Builder.Write( "VALIDATION ERROR: unhandled exception" );
				Builder.Write( "Exception: " + Ex.Message );
				Validated = false;
			}

			if( Validated )
			{
				Builder.Write( "Validation completed successfully!" );
			}
			else
			{
				Builder.Write( "VALIDATION ERROR: Install validation failed!" );
			}

			Builder.CloseLog();

			if( !Validated )
			{
				ErrorLevel = COMMANDS.ValidateInstall;
			}
		}
	}
}
