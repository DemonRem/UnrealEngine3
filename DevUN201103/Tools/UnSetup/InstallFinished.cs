/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace UnSetup
{
	public partial class InstallFinished : Form
	{
		public InstallFinished( bool IsGame )
		{
			InitializeComponent();

			RadioButton[] RadioButtons = new RadioButton[]
			{
				RadioButton0,
				RadioButton1,
				RadioButton2,
				RadioButton3,
			};

			Text = Program.Util.GetPhrase( "IFInstallFinished" );
			InstallFinishedTitleLabel.Text = Text;
			InstallFinishedOKButton.Text = Program.Util.GetPhrase( "IFFinished" );

			FinishedContentLabel.Text = Program.Util.GetPhrase( "IFInstallContentFinished" );
			if( IsGame )
			{
				FinishedContentLabel.Text += Program.Util.GetGameLongName();
				LaunchUDKCheckBox.Text = Program.Util.GetPhrase( "IFLaunch" ) + Program.Util.GetGameLongName();

				NetCodeRichTextBox.Clear();
				NetCodeRichTextBox.SelectionAlignment = HorizontalAlignment.Center;
				NetCodeRichTextBox.SelectedText = Program.Util.GetPhrase( "NetCodeWarn" );

				// Hide all radio buttons
				foreach( RadioButton Radio in RadioButtons )
				{
					Radio.Visible = false;
				}
			}
			else
			{
				FinishedContentLabel.Text += Program.Util.Manifest.FullName;

				int Index = 0;
				RadioButtons[Index].Checked = true;

				foreach( Utils.GameManifestOptions Game in Program.Util.Manifest.GameInfo )
				{
					RadioButtons[Index].Text = Program.Util.GetPhrase( "IFLaunch" ) + Game.Name;
					RadioButtons[Index].Tag = Game;
					Index++;
				}

				RadioButtons[Index].Text = Program.Util.GetPhrase( "R2D" );
				Index++;

				while( Index < RadioButtons.Length )
				{
					RadioButtons[Index].Visible = false;
					Index++;
				}

				// Hide checkbox
				LaunchUDKCheckBox.Visible = false;
				NetCodeRichTextBox.Enabled = false;
			}

			InstallOptionsFooterLineLabel.Text = Program.Util.UnSetupVersionString;

			if( !Program.Util.bDependenciesSuccessful || Program.Util.bSkipDependencies )
			{
				FinishedContentLabel.Text = Program.Util.GetPhrase( "IFDependsFailed" );
				LaunchUDKCheckBox.Visible = false;
				LaunchUDKCheckBox.Checked = false;
			}
		}

		public bool GetLaunchChecked()
		{
			return ( LaunchUDKCheckBox.Checked );
		}

		public Utils.GameManifestOptions GetLaunchType()
		{
			if( RadioButton0.Checked )
			{
				return ( ( Utils.GameManifestOptions )RadioButton0.Tag );
			}
			else if( RadioButton1.Checked )
			{
				return ( ( Utils.GameManifestOptions )RadioButton1.Tag );
			}
			else if( RadioButton2.Checked )
			{
				return ( ( Utils.GameManifestOptions )RadioButton2.Tag );
			}

			return ( null );
		}

		private void InstallFinishedOKClicked( object sender, EventArgs e )
		{
			DialogResult = DialogResult.OK;
			Close();
		}

		private void InstallFinishedLinkClicked( object sender, LinkClickedEventArgs e )
		{
			Process.Start( "rundll32.exe", "url.dll,FileProtocolHandler " + e.LinkText );
		}

		private void OnLoad( object sender, EventArgs e )
		{
			Utils.CenterFormToPrimaryMonitor( this );
		}
	}
}
