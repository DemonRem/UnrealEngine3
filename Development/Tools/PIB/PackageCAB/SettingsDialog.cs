using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace PackageCAB
{
	public partial class SettingsDialog : Form
	{
		public SettingsDialog( PackageCAB.PackageCABOptions Options )
		{
			InitializeComponent();

			SettingsPropertyGrid.SelectedObject = Options;
		}

		private void UIOKButtonClick( object sender, EventArgs e )
		{
			Close();
		}
	}
}
