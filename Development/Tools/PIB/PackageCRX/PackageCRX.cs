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
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Serialization;

namespace PackageCRX
{
	public partial class PackageCRX : Form
	{
		public class PackageCRXOptions
		{
			[CategoryAttribute( "Chrome" )]
			[DescriptionAttribute( "Location of Google Chrome executable" )]
			[XmlElement]
			public string ChromePath { get; set; }

			[CategoryAttribute( "Chrome" )]
			[DescriptionAttribute( "Location of the Chrome plugin files" )]
			[XmlElement]
			public string ChromePluginFilesPath { get; set; }

			public PackageCRXOptions()
			{
				ChromePath = "%LOCALAPPDATA%\\Google\\Chrome\\Application\\chrome.exe";
				ChromePluginFilesPath = ".\\GC";
			}
		}

		delegate void DelegateAddLine( string Line, Color TextColor );
		delegate void LogCapture( string Line );

		public PackageCRXOptions Options = null;
		public bool Ticking = false;

		private Process RunningProcess = null;

		public PackageCRX()
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
		}

		public void Init()
		{
			Environment.CurrentDirectory = Application.StartupPath;

			string SettingsName = Path.Combine( Application.StartupPath, "PackageCRX.Settings.xml" );
			Options = ReadXml<PackageCRXOptions>( SettingsName );

			Show();
			Ticking = true;

			Log( "Instructions:", Color.Blue );
			Log( "Click on the buttons in order; if a button is disabled, it has already been done.", Color.Blue );
		}

		public void Destroy()
		{
			if( RunningProcess != null )
			{
				RunningProcess.Kill();
			}

			string SettingsName = Path.Combine( Application.StartupPath, "PackageCRX.Settings.xml" );
			WriteXml<PackageCRXOptions>( SettingsName, Options );
			Hide();
		}

		public void Run()
		{
		}

		private void PackageCRXButtonClick( object sender, EventArgs e )
		{
			string GCFolderPath = Path.Combine( Environment.CurrentDirectory, Options.ChromePluginFilesPath );
			GCFolderPath = Path.GetFullPath( GCFolderPath );
			string CommandLine = "--pack-extension=" + GCFolderPath + " --no-message-box";

			string ExePath = Environment.ExpandEnvironmentVariables( Options.ChromePath );

			SpawnProcess( ExePath, CommandLine, false );
		}

		private void SettingsButtonClick( object sender, EventArgs e )
		{
			SettingsDialog Settings = new SettingsDialog( Options );
			if( Settings.ShowDialog() == DialogResult.OK )
			{
				Options = ( PackageCRXOptions )Settings.SettingsPropertyGrid.SelectedObject;
			}
		}

		private void PackageCRXFormClosed( object sender, FormClosedEventArgs e )
		{
			Ticking = false;
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
