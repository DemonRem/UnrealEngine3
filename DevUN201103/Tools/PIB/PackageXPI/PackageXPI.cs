/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Serialization;
using System.Windows.Forms;
using Ionic.Zip;

/**
C:\MozillaSigning>..\MozillaBins\nss-3.9\bin\certutil.exe -N -d c:\Builds\CertificateDB
Enter a password which will be used to encrypt your keys.
The password should be at least 8 characters long,
and should contain at least one non-alphabetic character.

Enter new password:
Re-enter password:

C:\MozillaSigning>pvkimprt -import -pfx \\epicgames.net\Root\UE3\SigningKeys\epic-2009-nopass.spc \\epicgames.net\Root\UE3\SigningKeys\epic-2009-nopass.pvk

C:\MozillaSigning>..\MozillaBins\nss-3.9\bin\pk12util -i c:\Builds\CertificateDB\epic-2009-nopass.pkcs12.pfx -d c:\Builds\CertificateDB
Enter Password or Pin for "NSS Certificate DB":
Enter password for PKCS12 file:
pk12util: PKCS12 IMPORT SUCCESSFUL

C:\MozillaSigning>..\MozillaBins\nss-3.9\bin\certutil.exe -L -d .
9438a20f-46ce-40c5-9a45-2cf3a85547dd                         u,u,u
VeriSign Class 3 Code Signing 2004 CA - VeriSign, Inc.       c,,c

C:\MozillaSigning>..\MozillaBins\nss-3.9\bin\signtool.exe -d . -k c2306349-0dc0-45fe-a7a3-cb6407ef7398 -p "nochance" signed/
using certificate directory: .
Generating signed//META-INF/manifest.mf file..
--> install.rdf
--> plugins/npue3.dll
Generating zigbert.sf file..
tree "signed/" signed successfully

*/

namespace PackageXPI
{
	public partial class PackageXPI : Form
	{
		public class PackageXPIOptions
		{
			[CategoryAttribute( "ToolSettings" )]
			[DescriptionAttribute( "Location of NSS tools" )]
			[XmlElement]
			public string NSSToolPath { get; set; }

			[CategoryAttribute( "ToolSettings" )]
			[DescriptionAttribute( "Location of certificate database" )]
			[XmlElement]
			public string CertificateDBPath { get; set; }

			[CategoryAttribute( "ToolSettings" )]
			[DescriptionAttribute( "Location of XPI files" )]
			[XmlElement]
			public string XPIFilePath { get; set; }

			[CategoryAttribute( "KeyLocation" )]
			[DescriptionAttribute( "Location of private and public keys" )]
			[XmlElement]
			public string PrivateKeyPath { get; set; }

			public PackageXPIOptions()
			{
				NSSToolPath = "C:\\MozillaBins\\nss-3.9";
				CertificateDBPath = ".\\NP\\CertDB";
				XPIFilePath = ".\\NP\\XPIFiles";
				PrivateKeyPath = "Secret!";
			}
		}

		delegate void DelegateAddLine( string Line, Color TextColor );
		delegate void LogCapture( string Line );

		public PackageXPIOptions Options = null;
		public bool Ticking = false;

		private Guid KeyGUID = Guid.Empty;
		private LogCapture LogCaptureFunc = null;
		private Process RunningProcess = null;

		private List<string> SignatureFiles = new List<string>()
		{
			// !!!!!!!!!!!!!!!! MUST BE IN THIS ORDER !!!!!!!!!!!!!!
			"META-INF/zigbert.rsa",
			"META-INF/manifest.mf",
			"META-INF/zigbert.sf"
		};

		public PackageXPI()
		{
			InitializeComponent();
		}

		public void Log( string Line, Color TextColour )
		{
			if( Line == null || !Ticking )
			{
				return;
			}

			// If we need to, invoke the delegate
			if( InvokeRequired )
			{
				Invoke( new DelegateAddLine( Log ), new object[] { Line, TextColour } );
				return;
			}

			DateTime Now = DateTime.Now;
			string FullLine = Now.ToLongTimeString() + ": " + Line;

			MainTextBox.Focus();
			MainTextBox.SelectionLength = 0;

			// Only set the color if it is different than the foreground colour
			if( MainTextBox.SelectionColor != TextColour )
			{
				MainTextBox.SelectionColor = TextColour;
			}

			MainTextBox.AppendText( FullLine + Environment.NewLine );

			if( LogCaptureFunc != null )
			{
				LogCaptureFunc( Line );
			}
		}

		private bool CheckKey( string KeyPath )
		{
			FileInfo PVKInfo = new FileInfo( KeyPath );
			if( !PVKInfo.Exists )
			{
				Log( "ERROR: Missing key file: " + KeyPath, Color.Red );
			}

			return ( PVKInfo.Exists );
		}

		private bool CheckKeys( string KeyRootFolder, string KeyName )
		{
			if( !CheckKey( KeyRootFolder + "\\" + KeyName + ".pvk" ) )
			{
				return ( false );
			}

			if( !CheckKey( KeyRootFolder + "\\" + KeyName + ".spc" ) )
			{
				return ( false );
			}

			return ( true );
		}

		private void CheckKeyPath()
		{
			// If it's never been setup, do so now
			while( Options.PrivateKeyPath == "Secret!" )
			{
				if( FileBrowserDialog.ShowDialog() == DialogResult.Cancel )
				{
					Log( "ERROR: Skipping key location!", Color.Red );
					break;
				}

				string KeyFolder = Path.GetDirectoryName( FileBrowserDialog.FileName );
				string KeyName = Path.GetFileNameWithoutExtension( FileBrowserDialog.FileName );
				if( !CheckKeys( KeyFolder, KeyName ) )
				{
					continue;
				}

				Options.PrivateKeyPath = Path.Combine( KeyFolder, KeyName );
			}

			// Make sure both keys exist
			if( Options.PrivateKeyPath != "Secret!" )
			{
				string KeyFolder = Path.GetDirectoryName( Options.PrivateKeyPath );
				string KeyName = Path.GetFileNameWithoutExtension( Options.PrivateKeyPath );

				if( CheckKeys( KeyFolder, KeyName ) )
				{
					Log( " ... found private and public keys.", Color.Black );
				}
			}
		}

		private bool CheckCertDB()
		{
			CreateDBButton.Enabled = true;

			// Check to see if the folder exists
			Log( " ... checking certificate database: " + Options.CertificateDBPath, Color.Black );
			DirectoryInfo DirInfo = new DirectoryInfo( Options.CertificateDBPath );
			if( !DirInfo.Exists )
			{
				Log( " ...... does not exist, creating.", Color.Red );
				DirInfo.Create();
				Log( " ...... created.", Color.Black );
				return( false );
			}

			// Check to see if the db files exist
			List<string> DBFiles = new List<string>()
			{
				"cert8.db",
				"key3.db",
				"secmod.db"
			};

			Log( " ... checking certificate database files.", Color.Black );
			bool DBExists = true;
			foreach( string DBFile in DBFiles )
			{
				string FileToCheck = Path.Combine( DirInfo.FullName, DBFile );
				FileInfo Info = new FileInfo( FileToCheck );
				if( !Info.Exists )
				{
					DBExists = false;
					Log( " ....... file does not exist: " + Info.Name, Color.Red );
					break;
				}
			}

			if( DBExists )
			{
				Log( " ... all certificate files found.", Color.Black );
			}
			else
			{
				Log( " ... some certificate database files missing; the database needs to be created.", Color.Red );
			}

			CreateDBButton.Enabled = !DBExists;
			return ( DBExists );
		}

		private bool CheckPFX()
		{
			CreatePFXButton.Enabled = true;

			DirectoryInfo DirInfo = new DirectoryInfo( Options.CertificateDBPath );
			FileInfo[] FileInfos = DirInfo.GetFiles( "*.pfx" );
			if( FileInfos.Length > 0 )
			{
				CreatePFXButton.Enabled = false;
				Log( " ... found pfx file in certificate database location.", Color.Black );
				return ( true );
			}

			return ( false );
		}

		private void CheckGUID()
		{
			ImportPFXButton.Enabled = true;
			GetCertificateButton.Enabled = true;

			if( KeyGUID != Guid.Empty )
			{
				ImportPFXButton.Enabled = false;
				GetCertificateButton.Enabled = false;

				Log( " ... found valid GUID.", Color.Black );
			}
		}

		public void Init()
		{
			Environment.CurrentDirectory = Application.StartupPath;

			string SettingsName = Path.Combine( Application.StartupPath, "PackageXPI.Settings.xml" );
			Options = ReadXml<PackageXPIOptions>( SettingsName );

			Show();
			Ticking = true;

			CheckKeyPath();
			if( CheckCertDB() )
			{
				if( CheckPFX() )
				{
					GetCertificate();
					CheckGUID();
				}
			}

			Environment.CurrentDirectory = Options.CertificateDBPath;

			Log( "Instructions:", Color.Blue );
			Log( "Click on the buttons in order; if a button is disabled, it has already been done.", Color.Blue );
		}

		public void Destroy()
		{
			if( RunningProcess != null )
			{
				RunningProcess.Kill();
			}

			string SettingsName = Path.Combine( Application.StartupPath, "PackageXPI.Settings.xml" );
			WriteXml<PackageXPIOptions>( SettingsName, Options );
			Hide();
		}

		public void Run()
		{
		}

		private void PackageXPIFormClosed( object sender, FormClosedEventArgs e )
		{
			Ticking = false;
		}

		private void PackageXPIFile( string XPIName )
		{
			Environment.CurrentDirectory = Options.XPIFilePath;

			using( ZipFile PackagedXPI = new ZipFile() )
			{
				foreach( string SignatureFile in SignatureFiles )
				{
					PackagedXPI.AddFile( SignatureFile );
				}

				PackagedXPI.AddFile( "plugins/npue3.dll" );
				PackagedXPI.AddFile( "install.rdf" );

				PackagedXPI.Save( XPIName );
			}

			Environment.CurrentDirectory = Application.StartupPath;
		}

		private int SpawnCertUtil( string CommandLine, bool ShowWindow )
		{
			string CertUtil = Path.Combine( Options.NSSToolPath, "bin\\certutil.exe" );
			return ( SpawnProcess( CertUtil, CommandLine, ShowWindow ) );
		}

		private int SpawnPK12Util( string CommandLine, bool ShowWindow )
		{
			string PK12Util = Path.Combine( Options.NSSToolPath, "bin\\pk12util.exe" );
			return ( SpawnProcess( PK12Util, CommandLine, ShowWindow ) );
		}

		private int SpawnSignTool( string CommandLine )
		{
			string SignTool = Path.Combine( Options.NSSToolPath, "bin\\signtool.exe" );
			return( SpawnProcess( SignTool, CommandLine, false ) );
		}

		private int SpawnPVKImport( string CommandLine )
		{
			string PVKImportTool = Path.Combine( Application.StartupPath, "..\\NoRedist\\PIB\\pvkimprt.exe" );
			return( SpawnProcess( PVKImportTool, CommandLine, false ) );
		}

		private void PackageXPIClick( object sender, EventArgs e )
		{
			Log( "Packaging XPI file", Color.Black );
			string XPIName = Path.Combine( Application.StartupPath, "npue3.xpi" );
			PackageXPIFile( XPIName );
			Log( " ... completed", Color.Black );
		}

		private void SignPackageClick( object sender, EventArgs e )
		{
			// Clean old signature files
			string XPIFiles = Path.Combine( Application.StartupPath, Options.XPIFilePath );
			foreach( string SignatureFile in SignatureFiles )
			{
				string FileName = Path.Combine( XPIFiles, SignatureFile );
				File.Delete( FileName );
			}

			// \nss-3.9\bin\signtool.exe -d <cert db path> -k 01234567-0123-0123-0123-0123456789ab -p "nochance" <files to package>

			Log( "Signing the files in the XPI", Color.Black );
			Log( " ... XPI file path: " + Options.XPIFilePath, Color.Black );

			SpawnSignTool( "-d . -k " + KeyGUID.ToString() + " -p \"\" " + Options.XPIFilePath );
		}

		private void CheckCertCapture( string Line )
		{
			// 9438a20f-46ce-40c5-9a45-2cf3a85547dd                         u,u,u

			Regex R = new Regex( "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12} +u,u,u", RegexOptions.None );
			if( R.IsMatch( Line ) )
			{
				KeyGUID = new Guid( Line.Substring( 0, "01234567-0123-0123-0123-0123456789ab".Length ) );
				GetCertificateButton.Enabled = false;
				LogCaptureFunc = null;
			}
		}

		private void GetCertificate()
		{
			LogCaptureFunc = CheckCertCapture;
			SpawnCertUtil( "-L -d .", false );
		}

		private void GetCertClick( object sender, EventArgs e )
		{
			// \nss-3.9\bin\certutil.exe -L -d <cert db path>

			Log( "Listing certificates in the certificate database", Color.Black );

			GetCertificate();
		}

		private void ImportPFXClick( object sender, EventArgs e )
		{
			// \nss-3.9\bin\pk12util.exe -i <file>.pfx -d <cert db path>

			Log( "Importing pfx into certificate database.", Color.Black );
			Log( "Instructions:", Color.Blue );
			Log( " ... type in the pfx password, and press enter.", Color.Blue );

			string KeyToImport = Path.GetFileNameWithoutExtension( Options.PrivateKeyPath );
			if( SpawnPK12Util( "-i " + KeyToImport + ".pfx -d .", true ) == 0 )
			{
				ImportPFXButton.Enabled = false;
			}
		}

		private void CreatePFXClick( object sender, EventArgs e )
		{
			// pvkimprt -import -pfx <file>.spc <file>.pvk
			// Investigate removing pvkimprt and using code mentioned here http://msdn.microsoft.com/en-us/library/ms867088.aspx

			Log( "Creating a pfx file to be imported into the certificate database", Color.Black );
			Log( "Instructions:", Color.Blue );
			Log( " ... click the 'Next' button", Color.Blue );
			Log( " ... select the 'Yes, export the private key' option (not the default)", Color.Blue );
			Log( " ... check the 'Include all certificates' and 'Export all extended properties' boxes, and click 'Next'", Color.Blue );
			Log( " ... type and confirm the password, then click 'Next'", Color.Blue );
			Log( " ... browse to the certificate database location: " + Options.CertificateDBPath + " add a filename, and click 'Next'", Color.Blue );
			Log( " ... click the 'Finish' button", Color.Blue );
			Log( " ... click the 'OK' button", Color.Blue );

			if( SpawnPVKImport( "-import -pfx " + Options.PrivateKeyPath + ".spc " + Options.PrivateKeyPath + ".pvk" ) == 0 )
			{
				CreatePFXButton.Enabled = false;
			}
		}

		private void CreateCertDBClick( object sender, EventArgs e )
		{
			// \nss-3.9\bin\certutil.exe -N -d <cert db path>

			Log( "Creating the certificate database", Color.Black );
			Log( " ... creating certificate DB in: " + Options.CertificateDBPath, Color.Black );
			Log( " ... press enter twice.", Color.Blue );

			if( SpawnCertUtil( "-N -d .", true ) == 0 )
			{
				CreateDBButton.Enabled = false;
			}
		}

		private void SettingsButtonClick( object sender, EventArgs e )
		{
			SettingsDialog Settings = new SettingsDialog( Options );
			if( Settings.ShowDialog() == DialogResult.OK )
			{
				Options = ( PackageXPIOptions )Settings.SettingsPropertyGrid.SelectedObject;
			}
		}

		public void PrintLog( object Sender, DataReceivedEventArgs e )
		{
			string Line = e.Data;
			Log( Line, Color.Magenta );
		}

		public void ProcessExit( object Sender, EventArgs e )
		{
			RunningProcess.EnableRaisingEvents = false;
			Log( "Process exited", Color.Magenta );
		}

		private int SpawnProcess( string Command, string CommandLine, bool ShowWindow )
		{
			RunningProcess = new Process();
			int ExitCode = -1;
			string Executable = Command;

			// Prepare a ProcessStart structure 
			RunningProcess.StartInfo.FileName = Command;
			RunningProcess.StartInfo.Arguments = CommandLine;

			// Redirect the output.
			RunningProcess.StartInfo.UseShellExecute = false;
			RunningProcess.StartInfo.CreateNoWindow = !ShowWindow;
			RunningProcess.StartInfo.RedirectStandardError = true;
			RunningProcess.StartInfo.RedirectStandardOutput = true;

			Log( "Spawning: " + Executable + " " + CommandLine, Color.DarkGreen );

			// Spawn the process - try to start the process, handling thrown exceptions as a failure.
			try
			{
				RunningProcess.OutputDataReceived += new DataReceivedEventHandler( PrintLog );
				RunningProcess.ErrorDataReceived += new DataReceivedEventHandler( PrintLog );
				RunningProcess.Exited += new EventHandler( ProcessExit );

				RunningProcess.Start();

				RunningProcess.BeginOutputReadLine();
				RunningProcess.BeginErrorReadLine();
				RunningProcess.EnableRaisingEvents = true;
			}
			catch( Exception Ex )
			{
				Log( "Exception while spawning process: " + Ex.ToString(), Color.Red );
				RunningProcess = null;
			}

			// Wait for the process to finish
			if( RunningProcess != null )
			{
				while( !RunningProcess.HasExited || RunningProcess.EnableRaisingEvents )
				{
					Application.DoEvents();
					System.Threading.Thread.Sleep( 50 );
				}

				ExitCode = RunningProcess.ExitCode;
				RunningProcess = null;
				Log( " ...  exited with exit code: " + ExitCode.ToString(), Color.DarkGreen );
			}

			return ( ExitCode );
		}

		protected void XmlSerializer_UnknownAttribute( object sender, XmlAttributeEventArgs e )
		{
		}

		protected void XmlSerializer_UnknownNode( object sender, XmlNodeEventArgs e )
		{
		}

		private T ReadXml<T>( string FileName ) where T : new()
		{
			T Instance = new T();
			try
			{
				using( Stream XmlStream = new FileStream( FileName, FileMode.Open, FileAccess.Read, FileShare.None ) )
				{
					// Creates an instance of the XmlSerializer class so we can read the settings object
					XmlSerializer ObjSer = new XmlSerializer( typeof( T ) );
					// Add our callbacks for a busted XML file
					ObjSer.UnknownNode += new XmlNodeEventHandler( XmlSerializer_UnknownNode );
					ObjSer.UnknownAttribute += new XmlAttributeEventHandler( XmlSerializer_UnknownAttribute );

					// Create an object graph from the XML data
					Instance = ( T )ObjSer.Deserialize( XmlStream );
				}
			}
			catch( Exception E )
			{
				Console.WriteLine( E.Message );
			}

			return ( Instance );
		}

		private bool WriteXml<T>( string FileName, T Instance )
		{
			// Make sure the file we're writing is actually writable
			FileInfo Info = new FileInfo( FileName );
			if( Info.Exists )
			{
				Info.IsReadOnly = false;
			}

			// Write out the xml stream
			Stream XmlStream = null;
			try
			{
				using( XmlStream = new FileStream( FileName, FileMode.Create, FileAccess.Write, FileShare.None ) )
				{
					XmlSerializer ObjSer = new XmlSerializer( typeof( T ) );

					// Add our callbacks for a busted XML file
					ObjSer.UnknownNode += new XmlNodeEventHandler( XmlSerializer_UnknownNode );
					ObjSer.UnknownAttribute += new XmlAttributeEventHandler( XmlSerializer_UnknownAttribute );

					ObjSer.Serialize( XmlStream, Instance );
				}
			}
			catch( Exception E )
			{
				Console.WriteLine( E.ToString() );
				return ( false );
			}

			return ( true );
		}
	}
}
