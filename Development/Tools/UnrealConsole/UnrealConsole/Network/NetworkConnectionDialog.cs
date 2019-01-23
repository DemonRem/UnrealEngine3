
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Text;


namespace UnrealNetwork
{
	/// <summary>
	/// The platform the game is running on
	/// </summary>
	public enum PlatformType
	{
		Unknown = 0x00000000,
		//		Windows	= 0x00000001,
		Xenon = 0x00000004,
		PS3 = 0x00000008,
		//		Linux	= 0x00000010,
		//		Mac		= 0x00000020
	};

	
	/// <summary>
	/// This dialog shows the user a list of available game instances that
	/// can be connected to
	/// </summary>
	public class NetworkConnectionDialog : System.Windows.Forms.Form
	{
		/// <summary>
		/// Holds the address and server information for a potential server
		/// </summary>
		public class Target
		{
			/// <summary>
			/// The name of the target
			/// </summary>
			public string Name;
			/// <summary>
			/// The platform it is running on
			/// </summary>
			public string Platform;
		}

		/// <summary>
		/// The list of targets
		/// </summary>
		private ArrayList Targets = new ArrayList();

		/// <summary>
		/// Holds the selected server item
		/// </summary>
		public Target SelectedTarget;

		#region Windows Form Designer generated code
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.ListView ConnectionList;
		private System.Windows.Forms.ColumnHeader TargetNameHeader;
		private System.Windows.Forms.ColumnHeader PlatformTypeHeader;
		private System.Windows.Forms.Button ConnectButton;
		private System.Windows.Forms.Button CancelBtn;
		private System.Windows.Forms.Button RefreshButton;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		#endregion

		/// <summary>
		/// Default constructor
		/// </summary>
		public NetworkConnectionDialog()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			// Init connection list view
			ConnectionList.View = View.Details;
			ConnectionList.LabelEdit = false;
			ConnectionList.AllowColumnReorder = true;
			ConnectionList.CheckBoxes = false;
			ConnectionList.GridLines = false;
			ConnectionList.ListViewItemSorter = new ListViewItemComparer(1);

			GetPossibleTargets();
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.ConnectionList = new System.Windows.Forms.ListView();
			this.TargetNameHeader = new System.Windows.Forms.ColumnHeader();
			this.PlatformTypeHeader = new System.Windows.Forms.ColumnHeader();
			this.label1 = new System.Windows.Forms.Label();
			this.ConnectButton = new System.Windows.Forms.Button();
			this.CancelBtn = new System.Windows.Forms.Button();
			this.RefreshButton = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// ConnectionList
			// 
			this.ConnectionList.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.ConnectionList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.TargetNameHeader,
            this.PlatformTypeHeader});
			this.ConnectionList.FullRowSelect = true;
			this.ConnectionList.HideSelection = false;
			this.ConnectionList.Location = new System.Drawing.Point(16, 32);
			this.ConnectionList.MultiSelect = false;
			this.ConnectionList.Name = "ConnectionList";
			this.ConnectionList.Size = new System.Drawing.Size(568, 386);
			this.ConnectionList.Sorting = System.Windows.Forms.SortOrder.Ascending;
			this.ConnectionList.TabIndex = 0;
			this.ConnectionList.UseCompatibleStateImageBehavior = false;
			this.ConnectionList.ItemActivate += new System.EventHandler(this.ConnectionList_ItemActivate);
			this.ConnectionList.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.ConnectionList_ColumnClick);
			// 
			// TargetNameHeader
			// 
			this.TargetNameHeader.Text = "Target";
			this.TargetNameHeader.Width = 150;
			// 
			// PlatformTypeHeader
			// 
			this.PlatformTypeHeader.Text = "Platform";
			this.PlatformTypeHeader.Width = 100;
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(13, 9);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(100, 20);
			this.label1.TabIndex = 1;
			this.label1.Text = "Available Targets:";
			// 
			// ConnectButton
			// 
			this.ConnectButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.ConnectButton.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.ConnectButton.Location = new System.Drawing.Point(432, 424);
			this.ConnectButton.Name = "ConnectButton";
			this.ConnectButton.Size = new System.Drawing.Size(75, 23);
			this.ConnectButton.TabIndex = 4;
			this.ConnectButton.Text = "&Connect";
			// 
			// CancelBtn
			// 
			this.CancelBtn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.CancelBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.CancelBtn.Location = new System.Drawing.Point(512, 424);
			this.CancelBtn.Name = "CancelBtn";
			this.CancelBtn.Size = new System.Drawing.Size(75, 23);
			this.CancelBtn.TabIndex = 5;
			this.CancelBtn.Text = "C&ancel";
			// 
			// RefreshButton
			// 
			this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.RefreshButton.Location = new System.Drawing.Point(325, 424);
			this.RefreshButton.Name = "RefreshButton";
			this.RefreshButton.Size = new System.Drawing.Size(75, 23);
			this.RefreshButton.TabIndex = 6;
			this.RefreshButton.Text = "&Refresh";
			this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
			// 
			// NetworkConnectionDialog
			// 
			this.AcceptButton = this.ConnectButton;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.CancelBtn;
			this.ClientSize = new System.Drawing.Size(600, 454);
			this.ControlBox = false;
			this.Controls.Add(this.RefreshButton);
			this.Controls.Add(this.CancelBtn);
			this.Controls.Add(this.ConnectButton);
			this.Controls.Add(this.ConnectionList);
			this.Controls.Add(this.label1);
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "NetworkConnectionDialog";
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Connect To";
			this.Closing += new System.ComponentModel.CancelEventHandler(this.NetworkConnectionDialog_Closing);
			this.ResumeLayout(false);

		}
		#endregion

		private void GetPossibleTargets()
		{
			// go over all the platforms
			ConsoleInterface.DLLInterface DLLInterface = UnrealConsole.UnrealConsole.DLLInterface;
			for (int PlatformIndex = 0; PlatformIndex < DLLInterface.GetNumPlatforms(); PlatformIndex++)
			{
				// activate the platform interface
				string PlatformName = DLLInterface.GetPlatformName(PlatformIndex);
				if (DLLInterface.ActivatePlatform(PlatformName))
				{
					// go over all the targets for this platform
					for (int TargetIndex = 0; TargetIndex < DLLInterface.GetNumTargets(); TargetIndex++)
					{
						Target Target = new Target();
						Target.Name = DLLInterface.GetTargetName(TargetIndex);
						Target.Platform = PlatformName;

						// Add the server info with IP addr
						ListViewItem lvi = new ListViewItem(Target.Name);
						lvi.SubItems.Add(Target.Platform);
						lvi.Tag = Target;
						// Add the data to the ui
						ConnectionList.Items.Add(lvi);
					}
				}
			}
		}

		/// <summary>
		/// Rebuilds the connection and sends the server announce request
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void RefreshButton_Click(object sender, System.EventArgs e)
		{
			// Clear out our cached lists of servers
			Targets.Clear();
			ConnectionList.Items.Clear();

			GetPossibleTargets();
		}

		/// <summary>
		/// Shuts down any sockets and sets our out variables
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void NetworkConnectionDialog_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			// Store the selected item as the server to connect to
			if (ConnectionList.SelectedItems.Count > 0)
			{
				SelectedTarget = (Target)ConnectionList.SelectedItems[0].Tag;
			}
		}

		/// <summary>
		/// Handles double clicking on a specific server. Same as clicking once
		/// and closing the dialog via Connect
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ConnectionList_ItemActivate(object sender, System.EventArgs e)
		{
			DialogResult = DialogResult.OK;
			Close();
		}

		private void ConnectionList_ColumnClick(object sender, System.Windows.Forms.ColumnClickEventArgs e)
		{
			ConnectionList.ListViewItemSorter = new ListViewItemComparer(e.Column);
		}
	}


	// Implements the manual sorting of items by columns.
	class ListViewItemComparer : IComparer
	{
		private int ColumnIndex;
		public ListViewItemComparer()
		{
			ColumnIndex = 0;
		}
		public ListViewItemComparer(int Column)
		{
			ColumnIndex = Column;
		}
		public int Compare(object x, object y)
		{
			return String.Compare(((ListViewItem)x).SubItems[ColumnIndex].Text, ((ListViewItem)y).SubItems[ColumnIndex].Text);
		}
	}
}
