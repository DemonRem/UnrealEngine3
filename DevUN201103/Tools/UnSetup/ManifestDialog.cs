/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace UnSetup
{
	public partial class ManifestDialog : Form
	{
		public ManifestDialog( Utils.ManifestOptions Options )
		{
			InitializeComponent();

			Text = Program.Util.GetPhrase( "UpdateFileManifests" );

			OptionsPropertyGrid.SelectedObject = Options;
		}

		private void OnLoad( object sender, EventArgs e )
		{
			Utils.CenterFormToPrimaryMonitor( this );
		}
	}
}
