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
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace UnSetup
{
	public partial class InstallOptions : Form
	{
		[DllImport( "kernel32.dll", CharSet=CharSet.Auto )]
		[return: MarshalAs( UnmanagedType.Bool )]
		public static extern bool GetDiskFreeSpaceEx(
			string lpDirectoryName,
			out UInt64 lpFreeBytesAvailable,
			out UInt64 lpTotalNumberOfBytes,
			out UInt64 lpTotalNumberOfFreeBytes );
	
		private string StartMenuLocation = "";

		public InstallOptions( bool bIsGame )
		{
			InitializeComponent();

			InstallOptionsFooterLineLabel.Text = Program.Util.UnSetupVersionString;
			InstallLocationGroupBox.Text = Program.Util.GetPhrase( "GBInstallLocation" );
			ChooseInstallLocationButton.Text = Program.Util.GetPhrase( "GBInstallLocationBrowse" );
			InstallButton.Text = Program.Util.GetPhrase( "ButInstall" );
			InstallCancelButton.Text = Program.Util.GetPhrase( "GQCancel" );
			EmailLabel.Text = Program.Util.GetPhrase( "LabEmail" );
			OptionalEmailLabel.Text = Program.Util.GetPhrase( "PrivacyStatement" );
			PrivacyPolicyTextBox.Text = Program.Util.GetPhrase( "PrivacyStatement2" );

			InvalidEmailLabel.Visible = false;
			InvalidEmailLabel.Text = Program.Util.GetPhrase( "IOInvalidEmail" );

			// Show the email subscription options if necessary
			EmailGroupBox.Visible = Program.Util.Manifest.ShowEmailSubscription;

			// Set up the default install location
			if( !bIsGame )
			{
				Text = Program.Util.GetPhrase( "IOInstallOptionsGame" ) + Program.Util.Manifest.RootName;
				InstallOptionsTitleLabel.Text = Text;

				string InstallLocation = Program.Util.Manifest.RootName + "-" + Program.Util.UnSetupTimeStamp;
				StartMenuLocation = Path.Combine( Program.Util.Manifest.FullName + "\\", InstallLocation );

				InstallLocationTextbox.Text = Path.Combine( "C:\\" + Program.Util.Manifest.RootName + "\\", InstallLocation );
			}
			else
			{
				StartMenuLocation = Program.Util.GetGameLongName();
				Text = Program.Util.GetPhrase( Program.Util.GetPhrase( "IOInstallOptionsGame" ) + StartMenuLocation );
				InstallOptionsTitleLabel.Text = Text;

				InstallLocationTextbox.Text = Path.Combine( "C:\\" + Program.Util.Manifest.RootName + "\\", StartMenuLocation );
			}
		}

		private bool ValidateInstallLocation()
		{
			string Location = GetInstallLocation();

			try
			{
				if( !Path.IsPathRooted( Location ) )
				{
					return ( false );
				}

				DirectoryInfo DirInfo = new DirectoryInfo( Location );
				if( !DirInfo.Exists )
				{
					Directory.CreateDirectory( DirInfo.FullName );
				}

				InstallLocationTextbox.Text = DirInfo.FullName;

				UInt64 FreeBytes = 0;
				UInt64 TotalBytes = 0;
				UInt64 TotalFreeBytes = 0;
				if( GetDiskFreeSpaceEx( DirInfo.FullName, out FreeBytes, out TotalBytes, out TotalFreeBytes ) )
				{
					// Need 2GB to install
					UInt64 TwoGig = 2UL * 1024UL * 1024UL * 1024UL;
					if( TotalFreeBytes < TwoGig )
					{
						return ( false );
					}
				}
			}
			catch
			{
				return ( false );
			}

			return ( true );
		}

		public string GetInstallLocation()
		{
			if( InstallLocationTextbox.Text.EndsWith( ":" ) )
			{
				InstallLocationTextbox.Text += "\\";
			}

			return ( InstallLocationTextbox.Text );
		}

		public string GetStartMenuLocation()
		{
			return ( StartMenuLocation );
		}

		public string GetSubscribeAddress()
		{
			string Email = EmailTextBox.Text.Trim();
			return ( Email );
		}

		private void ChooseInstallLocationClick( object sender, MouseEventArgs e )
		{
			ChooseInstallLocationBrowser.SelectedPath = InstallLocationTextbox.Text;
			DialogResult Result = ChooseInstallLocationBrowser.ShowDialog();
			if( Result == DialogResult.OK )
			{
				InstallLocationTextbox.Text = ChooseInstallLocationBrowser.SelectedPath;
			}
		}

		private void InstallButtonClick( object sender, EventArgs e )
		{
			if( !ValidateInstallLocation() )
			{
				GenericQuery Query = new GenericQuery( "GQCaptionInvalidInstall", "GQDescInvalidInstall", false, "GQCancel", true, "GQOK" );
				Query.ShowDialog();
			}
			else
			{
				DialogResult = DialogResult.OK;
				Close();
			}
		}

		private void CancelButtonClick( object sender, EventArgs e )
		{
			DialogResult = DialogResult.Cancel;
			Close();
		}

		private void OnLoad( object sender, EventArgs e )
		{
			Utils.CenterFormToPrimaryMonitor( this );
		}

		private void EmailAddressTextChanged( object sender, EventArgs e )
		{
			bool bWellFormedAddress = Program.Util.ValidateEmailAddress( EmailTextBox.Text.Trim() );
			InvalidEmailLabel.Visible = !bWellFormedAddress;
			InstallButton.Enabled = bWellFormedAddress;
		}

		private void PrivacyLinkClicked( object sender, LinkClickedEventArgs e )
		{
			Process.Start( "rundll32.exe", "url.dll,FileProtocolHandler " + e.LinkText );
		}
	}
}
