using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Xml;
using System.Xml.Serialization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using Stats;

namespace StatsViewer
{
	/// <summary>
	/// The game types for browsing
	/// </summary>
	public enum GameType
	{
		Unknown,
		Editor,
		Server,
		ListenServer,
		Client
	};

	/// <summary>
	/// The platform the game is running on
	/// </summary>
	public enum PlatformType
	{
        Unknown = 0x00,
		Windows = 0x01,
		Xenon = 0x04,
		PS3 = 0x08,
		Linux = 0x10,
		Mac = 0x20
	};

	/// <summary>
	/// Indicates the type of per frame search we are trying to do for a stat
	/// </summary>
	public enum SearchByType
	{
		GreaterThan,
		LessThan,
		EqualTo
	};

	/// <summary>
	/// Stats viewer application, aka UnrealPerfMon. Displays stats from the
	/// engine in a UI. Stats are either from a file or from the network when
	/// connected to a remote PC/Xe/PS3
	/// </summary>
	public class StatsViewerFrame : System.Windows.Forms.Form
	{
		/// <summary>
		/// This is the currently opened stats file
		/// </summary>
		private StatFile CurrentFile;
		/// <summary>
		/// Whether the last load attempt worked or not
		/// </summary>
		private bool bLoadedOk;
		/// <summary>
		/// Holds the string that is initially assigned to the main frame
		/// </summary>
		private string InitialWindowName;
		/// <summary>
		/// The number of frames to have in the viewable window 1 per 2 pixels
		/// </summary>
		private int NumViewableFrames = 0;
		/// <summary>
		/// The starting frame to draw at
		/// </summary>
		private int DrawRangeStart = 0;
		/// <summary>
		/// The ending frame to draw to
		/// </summary>
		private int DrawRangeEnd = 0;
		/// <summary>
		/// Scale to apply to the number of frames to have in a given
		/// viewable window
		/// </summary>
		private float ScaleFactor = 1.0F;
		/// <summary>
		/// Last scale set
		/// </summary>
		private float LastScaleFactor = 1.0F;
		/// <summary>
		/// The value used to figure out where to plot points. Represents
		/// the max value that can be displayed. Defaults to 100 ms.
		/// </summary>
		private double DrawMaxValue = 100.0;
		/// <summary>
		/// Holds the list of stats being tracked
		/// </summary>
		private ArrayList TrackedStats = new ArrayList();
		/// <summary>
		/// Holds our set of colors to use when drawing the graph data
		/// </summary>
		private GraphColors Colors = new GraphColors();
		/// <summary>
		/// Holds the last used color id for a newly added stat
		/// </summary>
		private int LastColorId = 0;
		/// <summary>
		/// Enum of text modes for drawing on the graph area
		/// </summary>
		public enum GraphTextOptions { GRAPH_ShowFrameNums, GRAPH_ShowTimes };
		/// <summary>
		/// Determines which text to place on the graph
		/// </summary>
		private GraphTextOptions ShowTextOptions = GraphTextOptions.GRAPH_ShowFrameNums;
		/// <summary>
		/// This is the font used in drawing text on the graph display
		/// </summary>
		private Font GraphFont = new Font(FontFamily.GenericSansSerif,8);
		/// <summary>
		/// This is a fast look up for the ranged aggregate data
		/// </summary>
		private SortedList StatIdToRangeAggData = new SortedList();
		/// <summary>
		/// Enum of modes for aggregate data
		/// </summary>
		public enum AggregateDataOptions { AGGDATA_Ranged, AGGDATA_Overall };
		/// <summary>
		/// The setting for which type of aggregate data to show
		/// </summary>
		private AggregateDataOptions AggDataOptions = AggregateDataOptions.AGGDATA_Overall;
		/// <summary>
		/// Holds the async stats UDP connection for receiving stats data and
		/// sending commands to the server
		/// </summary>
		private ServerConnection Connection;
		/// <summary>
		/// Periodic callback to process any queued packets
		/// </summary>
		private System.Windows.Forms.Timer ProcessPacketsTimer = new System.Windows.Forms.Timer();
		/// <summary>
		/// Whether to scroll the display as stats arrive via network
		/// </summary>
		private bool bShouldAutoScroll = true;
		/// <summary>
		/// Tracks the location of the mouse within the stats area
		/// </summary>
		private Point StatsAreaMouseLocation = new Point();
		/// <summary>
		/// Holds the list of currently open frame views
		/// </summary>
		private ArrayList PerFrameViews = new ArrayList();
		/// <summary>
		/// Holds the settings for the last use of the tool
		/// </summary>
		private SavedSettings LastUsedSettings;

		#region Windows Form Designer generated code
		private System.Windows.Forms.MenuItem menuItem1;
		private System.Windows.Forms.MenuItem ZoomIn;
		private System.Windows.Forms.MenuItem ZoomOut;
		private System.Windows.Forms.MenuItem ShowTimes;
		private System.Windows.Forms.MenuItem menuItem3;
		private System.Windows.Forms.MenuItem ShowFrameNums;
		private System.Windows.Forms.MainMenu mainMenu1;
		private System.Windows.Forms.MenuItem menuItem4;
		private System.Windows.Forms.Splitter splitter1;
		private System.Windows.Forms.Splitter splitter2;
		private System.Windows.Forms.TreeView StatGroupsTree;
		private System.Windows.Forms.Panel StatsArea;
		private System.Windows.Forms.ListView StatDataList;
		private System.Windows.Forms.MenuItem OpenStatFile;
		private System.Windows.Forms.MenuItem SaveStatFile;
		private System.Windows.Forms.MenuItem Exit;
		private System.Windows.Forms.MenuItem FileMenu;
		private System.Windows.Forms.MenuItem ViewMenu;
		private System.Windows.Forms.HScrollBar StatsAreaScrollbar;
		private System.Windows.Forms.MenuItem menuItem2;
		private System.Windows.Forms.MenuItem ViewRangedAggData;
		private System.Windows.Forms.MenuItem ViewOverallAggData;
		private System.Windows.Forms.MenuItem menuItem5;
		private System.Windows.Forms.MenuItem ConnectTo;
		private System.Windows.Forms.MenuItem CloseConnection;
		private System.Windows.Forms.MenuItem AutoScrollDisplay;
		private System.Windows.Forms.ContextMenu ViewFrameMenu;
		private System.Windows.Forms.MenuItem ViewFrameNumMenu;
		private System.Windows.Forms.ContextMenu StatsTreeMenu;
		private System.Windows.Forms.MenuItem ViewFramesByCriteria;
		private System.Windows.Forms.MenuItem ConnectToIP;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		#endregion
	

		/// <summary>
		/// Sets up all of the base settings (selection, columns, etc.)
		/// </summary>
		private void InitListView()
		{
			// Set up the list view prefs
			StatDataList.View = View.Details;
			StatDataList.LabelEdit = false;
			StatDataList.AllowColumnReorder = true;
			StatDataList.CheckBoxes = false;
			StatDataList.FullRowSelect = true;
			StatDataList.GridLines = true;
			StatDataList.Sorting = SortOrder.Ascending;
			// Create columns for the items and subitems.
			StatDataList.Columns.Add("Color",-2,HorizontalAlignment.Left);
			StatDataList.Columns.Add("Stat",-2,HorizontalAlignment.Left);
			StatDataList.Columns.Add("Average",-2,HorizontalAlignment.Left);
			StatDataList.Columns.Add("Average Per Call",-2,HorizontalAlignment.Left);
			StatDataList.Columns.Add("Min     ",-2,HorizontalAlignment.Left);
			StatDataList.Columns.Add("Max     ",-2,HorizontalAlignment.Left);
			// Init the menu check state
			switch (ShowTextOptions)
			{
				case GraphTextOptions.GRAPH_ShowFrameNums:
				{
					ShowTimes.Checked = false;
					ShowFrameNums.Checked = true;
					break;
				}
				case GraphTextOptions.GRAPH_ShowTimes:
				{
					ShowTimes.Checked = true;
					ShowFrameNums.Checked = false;
					break;
				}
			}
			// Init aggregate data menus check states
			switch (AggDataOptions)
			{
				case AggregateDataOptions.AGGDATA_Ranged:
				{
					ViewRangedAggData.Checked = true;
					ViewOverallAggData.Checked = false;
					break;
				}
				case AggregateDataOptions.AGGDATA_Overall:
				{
					ViewRangedAggData.Checked = false;
					ViewOverallAggData.Checked = true;
					break;
				}
			}
		}

		/// <summary>
		/// Frame window intialization code
		/// </summary>
		public StatsViewerFrame()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();
			// Save off the intial window name so we can append the file name later
			InitialWindowName = Text + " - ";
			// Set our list view defaults
			InitListView();
			// Add the callback for our timer
			ProcessPacketsTimer.Tick += new EventHandler(OnTimer);
			// And now kick it off
			ProcessPacketsTimer.Interval = 500;
			ProcessPacketsTimer.Start();
			// Load the last used settings
			LoadUserSettings();
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
				// Save the last used settings
				SaveUserSettings();
				// Discard any connection
				if (IsConnected())
				{
					PerformDisconnect();
					PerformSave(true);
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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(StatsViewerFrame));
			this.mainMenu1 = new System.Windows.Forms.MainMenu();
			this.FileMenu = new System.Windows.Forms.MenuItem();
			this.OpenStatFile = new System.Windows.Forms.MenuItem();
			this.SaveStatFile = new System.Windows.Forms.MenuItem();
			this.menuItem4 = new System.Windows.Forms.MenuItem();
			this.Exit = new System.Windows.Forms.MenuItem();
			this.ViewMenu = new System.Windows.Forms.MenuItem();
			this.ViewOverallAggData = new System.Windows.Forms.MenuItem();
			this.ViewRangedAggData = new System.Windows.Forms.MenuItem();
			this.menuItem2 = new System.Windows.Forms.MenuItem();
			this.ShowTimes = new System.Windows.Forms.MenuItem();
			this.ShowFrameNums = new System.Windows.Forms.MenuItem();
			this.menuItem3 = new System.Windows.Forms.MenuItem();
			this.ZoomIn = new System.Windows.Forms.MenuItem();
			this.ZoomOut = new System.Windows.Forms.MenuItem();
			this.menuItem1 = new System.Windows.Forms.MenuItem();
			this.AutoScrollDisplay = new System.Windows.Forms.MenuItem();
			this.menuItem5 = new System.Windows.Forms.MenuItem();
			this.ConnectTo = new System.Windows.Forms.MenuItem();
			this.CloseConnection = new System.Windows.Forms.MenuItem();
			this.StatGroupsTree = new System.Windows.Forms.TreeView();
			this.StatsTreeMenu = new System.Windows.Forms.ContextMenu();
			this.ViewFramesByCriteria = new System.Windows.Forms.MenuItem();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.StatsArea = new System.Windows.Forms.Panel();
			this.ViewFrameMenu = new System.Windows.Forms.ContextMenu();
			this.ViewFrameNumMenu = new System.Windows.Forms.MenuItem();
			this.StatsAreaScrollbar = new System.Windows.Forms.HScrollBar();
			this.splitter2 = new System.Windows.Forms.Splitter();
			this.StatDataList = new System.Windows.Forms.ListView();
			this.ConnectToIP = new System.Windows.Forms.MenuItem();
			this.StatsArea.SuspendLayout();
			this.SuspendLayout();
			// 
			// mainMenu1
			// 
			this.mainMenu1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.FileMenu,
																					  this.ViewMenu,
																					  this.menuItem5});
			// 
			// FileMenu
			// 
			this.FileMenu.Index = 0;
			this.FileMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					 this.OpenStatFile,
																					 this.SaveStatFile,
																					 this.menuItem4,
																					 this.Exit});
			this.FileMenu.Text = "&File";
			// 
			// OpenStatFile
			// 
			this.OpenStatFile.Index = 0;
			this.OpenStatFile.Text = "&Open";
			this.OpenStatFile.Click += new System.EventHandler(this.OpenStatFile_Click);
			// 
			// SaveStatFile
			// 
			this.SaveStatFile.Index = 1;
			this.SaveStatFile.Text = "&Save";
			this.SaveStatFile.Click += new System.EventHandler(this.SaveStatFile_Click);
			// 
			// menuItem4
			// 
			this.menuItem4.Index = 2;
			this.menuItem4.Text = "-";
			// 
			// Exit
			// 
			this.Exit.Index = 3;
			this.Exit.Text = "E&xit";
			this.Exit.Click += new System.EventHandler(this.Exit_Click);
			// 
			// ViewMenu
			// 
			this.ViewMenu.Index = 1;
			this.ViewMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					 this.ViewOverallAggData,
																					 this.ViewRangedAggData,
																					 this.menuItem2,
																					 this.ShowTimes,
																					 this.ShowFrameNums,
																					 this.menuItem3,
																					 this.ZoomIn,
																					 this.ZoomOut,
																					 this.menuItem1,
																					 this.AutoScrollDisplay});
			this.ViewMenu.Text = "&View";
			// 
			// ViewOverallAggData
			// 
			this.ViewOverallAggData.Checked = true;
			this.ViewOverallAggData.Index = 0;
			this.ViewOverallAggData.RadioCheck = true;
			this.ViewOverallAggData.Text = "O&verall Aggregate Data";
			this.ViewOverallAggData.Click += new System.EventHandler(this.ViewOverallAggData_Click);
			// 
			// ViewRangedAggData
			// 
			this.ViewRangedAggData.Index = 1;
			this.ViewRangedAggData.RadioCheck = true;
			this.ViewRangedAggData.Text = "&Ranged Aggregate Data";
			this.ViewRangedAggData.Click += new System.EventHandler(this.ViewRangedAggData_Click);
			// 
			// menuItem2
			// 
			this.menuItem2.Index = 2;
			this.menuItem2.Text = "-";
			// 
			// ShowTimes
			// 
			this.ShowTimes.Index = 3;
			this.ShowTimes.RadioCheck = true;
			this.ShowTimes.Text = "Show &Times";
			this.ShowTimes.Click += new System.EventHandler(this.ShowTimes_Click);
			// 
			// ShowFrameNums
			// 
			this.ShowFrameNums.Index = 4;
			this.ShowFrameNums.RadioCheck = true;
			this.ShowFrameNums.Text = "Show &Frame #";
			this.ShowFrameNums.Click += new System.EventHandler(this.ShowFrameNums_Click);
			// 
			// menuItem3
			// 
			this.menuItem3.Index = 5;
			this.menuItem3.Text = "-";
			// 
			// ZoomIn
			// 
			this.ZoomIn.Index = 6;
			this.ZoomIn.Shortcut = System.Windows.Forms.Shortcut.F1;
			this.ZoomIn.Text = "Zoom &In";
			this.ZoomIn.Click += new System.EventHandler(this.ZoomIn_Click);
			// 
			// ZoomOut
			// 
			this.ZoomOut.Index = 7;
			this.ZoomOut.Shortcut = System.Windows.Forms.Shortcut.F2;
			this.ZoomOut.Text = "Zoom O&ut";
			this.ZoomOut.Click += new System.EventHandler(this.ZoomOut_Click);
			// 
			// menuItem1
			// 
			this.menuItem1.Index = 8;
			this.menuItem1.Text = "-";
			// 
			// AutoScrollDisplay
			// 
			this.AutoScrollDisplay.Checked = true;
			this.AutoScrollDisplay.Index = 9;
			this.AutoScrollDisplay.Shortcut = System.Windows.Forms.Shortcut.F3;
			this.AutoScrollDisplay.Text = "&Auto-scroll Display";
			this.AutoScrollDisplay.Click += new System.EventHandler(this.AutoScrollDisplay_Click);
			// 
			// menuItem5
			// 
			this.menuItem5.Index = 2;
			this.menuItem5.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																					  this.ConnectTo,
																					  this.ConnectToIP,
																					  this.CloseConnection});
			this.menuItem5.Text = "&Connection";
			// 
			// ConnectTo
			// 
			this.ConnectTo.Index = 0;
			this.ConnectTo.Text = "&Connect To...";
			this.ConnectTo.Click += new System.EventHandler(this.ConnectTo_Click);
			// 
			// CloseConnection
			// 
			this.CloseConnection.Index = 2;
			this.CloseConnection.Text = "C&lose Connection";
			this.CloseConnection.Click += new System.EventHandler(this.CloseConnection_Click);
			// 
			// StatGroupsTree
			// 
			this.StatGroupsTree.ContextMenu = this.StatsTreeMenu;
			this.StatGroupsTree.Dock = System.Windows.Forms.DockStyle.Left;
			this.StatGroupsTree.ImageIndex = -1;
			this.StatGroupsTree.Location = new System.Drawing.Point(0, 0);
			this.StatGroupsTree.Name = "StatGroupsTree";
			this.StatGroupsTree.SelectedImageIndex = -1;
			this.StatGroupsTree.Size = new System.Drawing.Size(200, 529);
			this.StatGroupsTree.TabIndex = 0;
			this.StatGroupsTree.ItemDrag += new System.Windows.Forms.ItemDragEventHandler(this.StatsGroupTree_ItemDrag);
			// 
			// StatsTreeMenu
			// 
			this.StatsTreeMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																						  this.ViewFramesByCriteria});
			this.StatsTreeMenu.Popup += new System.EventHandler(this.StatsTreeMenu_Popup);
			// 
			// ViewFramesByCriteria
			// 
			this.ViewFramesByCriteria.Index = 0;
			this.ViewFramesByCriteria.Text = "&View Frames By Criteria...";
			this.ViewFramesByCriteria.Click += new System.EventHandler(this.ViewFramesByCriteria_Click);
			// 
			// splitter1
			// 
			this.splitter1.Location = new System.Drawing.Point(200, 0);
			this.splitter1.Name = "splitter1";
			this.splitter1.Size = new System.Drawing.Size(3, 529);
			this.splitter1.TabIndex = 1;
			this.splitter1.TabStop = false;
			// 
			// StatsArea
			// 
			this.StatsArea.AllowDrop = true;
			this.StatsArea.BackColor = System.Drawing.SystemColors.Control;
			this.StatsArea.ContextMenu = this.ViewFrameMenu;
			this.StatsArea.Controls.Add(this.StatsAreaScrollbar);
			this.StatsArea.Controls.Add(this.splitter2);
			this.StatsArea.Controls.Add(this.StatDataList);
			this.StatsArea.Dock = System.Windows.Forms.DockStyle.Fill;
			this.StatsArea.Location = new System.Drawing.Point(203, 0);
			this.StatsArea.Name = "StatsArea";
			this.StatsArea.Size = new System.Drawing.Size(597, 529);
			this.StatsArea.TabIndex = 2;
			this.StatsArea.Resize += new System.EventHandler(this.StatsArea_Resize);
			this.StatsArea.DragEnter += new System.Windows.Forms.DragEventHandler(this.StatOrGroup_DragEnter);
			this.StatsArea.Paint += new System.Windows.Forms.PaintEventHandler(this.StatsArea_Paint);
			this.StatsArea.DragDrop += new System.Windows.Forms.DragEventHandler(this.StatOrGroup_DragDrop);
			this.StatsArea.DoubleClick += new System.EventHandler(this.StatsArea_DoubleClick);
			this.StatsArea.MouseMove += new System.Windows.Forms.MouseEventHandler(this.StatsArea_MouseMove);
			// 
			// ViewFrameMenu
			// 
			this.ViewFrameMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																						  this.ViewFrameNumMenu});
			this.ViewFrameMenu.Popup += new System.EventHandler(this.ViewFrameMenu_Popup);
			// 
			// ViewFrameNumMenu
			// 
			this.ViewFrameNumMenu.Index = 0;
			this.ViewFrameNumMenu.Text = "View Frame #";
			this.ViewFrameNumMenu.Click += new System.EventHandler(this.ViewFrameNumMenu_Click);
			// 
			// StatsAreaScrollbar
			// 
			this.StatsAreaScrollbar.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.StatsAreaScrollbar.Location = new System.Drawing.Point(0, 365);
			this.StatsAreaScrollbar.Name = "StatsAreaScrollbar";
			this.StatsAreaScrollbar.Size = new System.Drawing.Size(597, 17);
			this.StatsAreaScrollbar.TabIndex = 2;
			this.StatsAreaScrollbar.Scroll += new System.Windows.Forms.ScrollEventHandler(this.StatsAreaScrollbar_Scroll);
			// 
			// splitter2
			// 
			this.splitter2.BackColor = System.Drawing.SystemColors.Control;
			this.splitter2.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.splitter2.Location = new System.Drawing.Point(0, 382);
			this.splitter2.Name = "splitter2";
			this.splitter2.Size = new System.Drawing.Size(597, 3);
			this.splitter2.TabIndex = 1;
			this.splitter2.TabStop = false;
			// 
			// StatDataList
			// 
			this.StatDataList.AllowDrop = true;
			this.StatDataList.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.StatDataList.Location = new System.Drawing.Point(0, 385);
			this.StatDataList.Name = "StatDataList";
			this.StatDataList.Size = new System.Drawing.Size(597, 144);
			this.StatDataList.TabIndex = 0;
			this.StatDataList.ItemActivate += new System.EventHandler(this.StatDataList_ItemActivate);
			this.StatDataList.KeyUp += new System.Windows.Forms.KeyEventHandler(this.StatDataList_KeyUp);
			this.StatDataList.DragDrop += new System.Windows.Forms.DragEventHandler(this.StatOrGroup_DragDrop);
			this.StatDataList.DragEnter += new System.Windows.Forms.DragEventHandler(this.StatOrGroup_DragEnter);
			this.StatDataList.SelectedIndexChanged += new System.EventHandler(this.StatDataList_SelectedIndexChanged);
			// 
			// ConnectToIP
			// 
			this.ConnectToIP.Index = 1;
			this.ConnectToIP.Text = "Connect To &IP...";
			this.ConnectToIP.Click += new System.EventHandler(this.ConnectToIP_Click);
			// 
			// StatsViewerFrame
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(800, 529);
			this.Controls.Add(this.StatsArea);
			this.Controls.Add(this.splitter1);
			this.Controls.Add(this.StatGroupsTree);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Menu = this.mainMenu1;
			this.Name = "StatsViewerFrame";
			this.Text = "Unreal Engine 3 Stats Viewer";
			this.StatsArea.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] Args)
		{
			try
			{
				// Create the frame window
				StatsViewerFrame Frame = new StatsViewerFrame();
				// If the user passed the filename open it
				if (Args.Length > 0)
				{
					Frame.OpenFile(Args[0]);
				}
				Application.Run(Frame);
			}
			catch(Exception E)
			{
				MessageBox.Show("Exception starting app:\r\n" + E.ToString());
			}
		}

		/// <summary>
		/// Close the app
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void Exit_Click(object sender, System.EventArgs e)
		{
			Application.Exit();
		}

		/// <summary>
		/// Empties the contents of the tree & list views
		/// </summary>
		private void EmptyUI()
		{
			// Close the dependent views first
			ClosePerFrameViews();
			// Clear the stats form
			StatDataList.Items.Clear();
			StatGroupsTree.Nodes.Clear();
			// Empty the scroll bar
			StatsAreaScrollbar.Minimum = 0;
			StatsAreaScrollbar.Maximum = 0;
			LastColorId = 0;
		}

		/// <summary>
		/// Initializes the UI from the newly opened file data
		/// </summary>
		private void BuildUI()
		{
			BuildTreeView();
			// Set up scroll bars based on the number of frames
			if (CurrentFile != null)
			{
				StatsAreaScrollbar.Minimum = 0;
				StatsAreaScrollbar.Maximum = CurrentFile.Frames.Length - 1;
			}
			DrawRangeStart = 0;
			ScaleFactor = 1.0F;
			NumViewableFrames = StatsArea.Width / 2;
			// Set our ending range to the number of viewable frames or the
			// length of the array
			DrawRangeEnd = (int)Math.Min(NumViewableFrames * ScaleFactor,CurrentFile.Frames.Length - 1);
			// Force a redraw
			StatsArea.Invalidate();
		}

		/// <summary>
		/// Common method for opening a file and populating the view
		/// </summary>
		/// <param name="FileName">The file to open and process</param>
		public void OpenFile(string FileName)
		{
			// Show that we are going to be busy
			Cursor = Cursors.WaitCursor;
			bLoadedOk = true;
			// Get the XML data stream to read from
			Stream XmlStream = new FileStream(FileName,FileMode.Open,FileAccess.Read,
				FileShare.None,256 * 1024,false);
			// Creates an instance of the XmlSerializer class so we can
			// read the object graph
			XmlSerializer ObjSer = new XmlSerializer(typeof(StatFile));
			// Add our callbacks for a busted XML file
			ObjSer.UnknownNode += new XmlNodeEventHandler(XmlSerializer_UnknownNode);
			ObjSer.UnknownAttribute += new XmlAttributeEventHandler(XmlSerializer_UnknownAttribute);
			// Create an object graph from the XML data
			CurrentFile = (StatFile)ObjSer.Deserialize(XmlStream);
			// Done with the file so close it
			XmlStream.Close();
			// Update the title bar if we loaded ok
			if (bLoadedOk)
			{
				// Clear out any windows
				EmptyUI();
				Text = InitialWindowName + FileName;
				// Fix up all the object cross references and calculate
				// the aggregate adata
				FixupData();
				// Update the UI views
				BuildUI();
			}
			else
			{
				MessageBox.Show(this,"Unable to read/parse file","File Error");
			}
			// Restore the cursor when done
			Cursor = Cursors.Default;
		}

		/// <summary>
		/// Opens an XML file containing all of the stats data
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void OpenStatFile_Click(object sender, System.EventArgs e)
		{
			try
			{
				OpenFileDialog OpenFileDlg = new OpenFileDialog();
				// Filter on our XML file extension
				OpenFileDlg.Filter = "XML Files (*.xml)|*.xml|All Files (*.*)|*.*";
				OpenFileDlg.FilterIndex = 1;
				OpenFileDlg.RestoreDirectory = true;
				OpenFileDlg.ValidateNames = true;
				// Show the dialog and only load the file if the user said ok
				if (OpenFileDlg.ShowDialog() == DialogResult.OK)
				{
					// Use the common file opening routine
					OpenFile(OpenFileDlg.FileName);
				}
			}
			catch(Exception E)
			{
				bLoadedOk = false;
				Console.WriteLine("Exception parsing XML: " + E.ToString());
			}
		}

		/// <summary>
		/// Logs the node information for debugging purposes
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e">The info about the bad node type</param>
		protected void XmlSerializer_UnknownNode(object sender, XmlNodeEventArgs e)
		{
			bLoadedOk = false;
			Console.WriteLine("Unknown Node:" + e.Name + "\t" + e.Text);
		}
		
		/// <summary>
		/// Logs the bad attribute information for debugging purposes
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e">The attribute info</param>
		protected void XmlSerializer_UnknownAttribute(object sender, XmlAttributeEventArgs e)
		{
			bLoadedOk = false;
			System.Xml.XmlAttribute attr = e.Attr;
			Console.WriteLine("Unknown attribute " + attr.Name + "='" + attr.Value + "'");
		}

		/// <summary>
		/// Writes the current stats data out to an XML file
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void SaveStatFile_Click(object sender, System.EventArgs e)
		{
			try
			{
				// Use the common save routine and don't prompt
				PerformSave(false);
			}
			catch(Exception E)
			{
				Console.WriteLine("Exception saving XML: " + E.ToString());
				MessageBox.Show(this,"Unable to save the file data","File Error");
			}
		}

		/// <summary>
		/// Fixes up the data in the CurrentFile graph that isn't serialized as part
		/// of the XML data.
		/// </summary>
		private void FixupData()
		{
			// Tell the descriptions to update themselves
			CurrentFile.FixupData();
			// Zero out our per file instance data
			TrackedStats.Clear();
			// Empty any range aggregate data
			StatIdToRangeAggData.Clear();
		}

		/// <summary>
		/// Adds all of the known groups to the tree
		/// </summary>
		/// <param name="Parent">The parent node to attach the groups to</param>
		private void AddGroupsToTree(TreeNode Parent)
		{
			// Add each group to the tree view
			foreach (Group group in CurrentFile.Descriptions.Groups)
			{
				// Create the new tree node for this group
				TreeNode GroupNode = new TreeNode(group.Name);
				// Set the object that it is representing
				GroupNode.Tag = group;
				// Add the group to the tree
				Parent.Nodes.Add(GroupNode);
				// Now add all the stats in this group
				foreach (Stat stat in group.OwnedStats)
				{
					// Create the new tree node for this group
					TreeNode NewNode = new TreeNode(stat.Name);
					// Set the object that it is representing
					NewNode.Tag = stat;
					// Add the stat to the group's node
					GroupNode.Nodes.Add(NewNode);
				}
			}
		}

		/// <summary>
		/// Adds the data in the CurrentFile to the treeview portion of the UI
		/// </summary>
		private void BuildTreeView()
		{
			// Avoid multiple repaint flashes
			StatGroupsTree.BeginUpdate();
			if (StatGroupsTree.Nodes.Count > 0)
			{
				StatGroupsTree.Nodes.Clear();
			}
			// Add the grouped node to the tree
			TreeNode GroupsNode = new TreeNode("Groups");
			StatGroupsTree.Nodes.Add(GroupsNode);
			// Add all of the groups to the tree
			AddGroupsToTree(GroupsNode);
			GroupsNode.Expand();
			// Repaint the control now
			StatGroupsTree.EndUpdate();
		}

		/// <summary>
		/// Handles drag and drop being initiated from the stats tree view
		/// </summary>
		/// <param name="sender">The tree view</param>
		/// <param name="e">The node being dragged</param>
		private void StatsGroupTree_ItemDrag(object sender, System.Windows.Forms.ItemDragEventArgs e)
		{
			int DragDropValue = 0;
			// Get the node so we can get the stat/group data out of it
			TreeNode Item = (TreeNode)e.Item;
			// Root node doesn't have an object associated with it
			if (Item.Tag != null)
			{
				UIBaseElement Base = (UIBaseElement)Item.Tag;
				// Handle building the value based upon the type
				if (Base.UIType == UIBaseElement.ElementType.StatsObject)
				{
					Stat stat = (Stat)Base;
					DragDropValue = (((byte)UIBaseElement.ElementType.StatsObject) << 24) | stat.StatId;
				}
				else if (Base.UIType == UIBaseElement.ElementType.GroupObject)
				{
					Group group = (Group)Base;
					DragDropValue = (((byte)UIBaseElement.ElementType.GroupObject) << 24) | group.GroupId;
				}
				// Kick off drag and drop
				StatGroupsTree.DoDragDrop(DragDropValue,DragDropEffects.Link);
			}
		}

		/// <summary>
		/// Called when a drag and drop operation first enters the window. The
		/// data is checked to see if we can parse it. If so, we accept the
		/// drag operation
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatOrGroup_DragEnter(object sender, System.Windows.Forms.DragEventArgs e)
		{
			// If this is one of our types, allow it to be dropped
			if (e.Data.GetDataPresent(typeof(int)))
			{
				e.Effect = DragDropEffects.Link;
			}
			else
			{
				// Unknown type so ignore
				e.Effect = DragDropEffects.None;
			}
		}

		/// <summary>
		/// Adds the stat to the list view
		/// </summary>
		/// <param name="StatToAdd">The stat to add the data for</param>
		private void AddStatToList(Stat StatToAdd)
		{
			AggregateStatData AggData = null;
			// Figure out which aggregate data object to add to the display
			switch (AggDataOptions)
			{
				case AggregateDataOptions.AGGDATA_Ranged:
				{
					AggData = (AggregateStatData)StatIdToRangeAggData[StatToAdd.StatId];
					break;
				}
				case AggregateDataOptions.AGGDATA_Overall:
				{
					AggData = CurrentFile.GetAggregateData(StatToAdd.StatId);
					break;
				}
			}
			if (AggData != null)
			{
				// Create the list view item to add. Note the name is added as an
				// string of spaces so that the background color is the color of
				// the stat
				ListViewItem lvi = new ListViewItem("       ");
				lvi.Tag = StatToAdd;
				lvi.BackColor = Colors.GetColor(StatToAdd.ColorId);
				lvi.UseItemStyleForSubItems = false;
				// Add the name of the stat
				lvi.SubItems.Add(StatToAdd.Name);
				// Now add the aggregate data
				lvi.SubItems.Add(AggData.Average.ToString("F2"));
				lvi.SubItems.Add(AggData.AveragePerCall.ToString("F2"));
				// If the data hasn't been initialized show zero rather than Min/Max double
				if (AggData.NumStats > 0)
				{
					lvi.SubItems.Add(AggData.Min.ToString("F2"));
					lvi.SubItems.Add(AggData.Max.ToString("F2"));
				}
				else
				{
					lvi.SubItems.Add("0");
					lvi.SubItems.Add("0");
				}
				// Add to the list view
				StatDataList.Items.Add(lvi);
			}
		}

		/// <summary>
		/// Adds the stat to our tracked list. Also, chooses a new color for the stat
		/// </summary>
		/// <param name="StatToTrack">The stat to add to our tracked list</param>
		private void TrackStat(Stat StatToTrack)
		{
			// Don't add twice
			if (TrackedStats.Contains(StatToTrack) == false)
			{
				// Add this to our list that we iterate during rendering
				TrackedStats.Add(StatToTrack);
				StatToTrack.ColorId = Colors.GetNextColorId(LastColorId);
				LastColorId = StatToTrack.ColorId;
			}
		}

		/// <summary>
		/// Iterates through the group's stats adding them to the list
		/// </summary>
		/// <param name="GroupToAdd">The group to add all the stats for</param>
		private void AddGroupToList(Group GroupToAdd)
		{
			// Add each stat in this group
			foreach (Stat stat in GroupToAdd.OwnedStats)
			{
				TrackStat(stat);
				AddStatToList(stat);
			}
		}

		/// <summary>
		/// Handles the drop event. If the item dropped is a stat, that stat is
		/// added. If the item is a group, all of it's items are added.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatOrGroup_DragDrop(object sender, System.Windows.Forms.DragEventArgs e)
		{
			// Don't flicker while inserting multiple items
			StatDataList.BeginUpdate();
			if (e.Data.GetDataPresent(typeof(int)))
			{
				// Decode the int into two fields
				int WholeValue = (int)e.Data.GetData(typeof(int));
				int Type = (WholeValue >> 24) & 0xFF;
				int ID = WholeValue & ~(0xFF << 24);
				// Figure out which type it is
				switch ((UIBaseElement.ElementType)Type)
				{
						// Figure out if we are adding a stat
					case UIBaseElement.ElementType.StatsObject:
					{
						// Get the stat that was referenced. Handle null
						// in case the format is busted
						Stat StatToAdd = CurrentFile.GetStat(ID);
						if (StatToAdd != null)
						{
							TrackStat(StatToAdd);
							AddStatToList(StatToAdd);
						}
						break;
					}
						// Or an entire group of stats
					case UIBaseElement.ElementType.GroupObject:
					{
						// Get the group that was referenced. Handle null
						// in case the format is busted
						Group GroupToAdd = CurrentFile.GetGroup(ID);
						if (GroupToAdd != null)
						{
							AddGroupToList(GroupToAdd);
						}
						break;
					}
				}
			}
			// Resize the stat name column
			StatDataList.Columns[1].Width = -1;
			// Force an update of the draw range
			UpdateDrawRange();
			StatsArea.Invalidate();
			// Repaint the list view
			StatDataList.EndUpdate();
		}

		/// <summary>
		/// Determines if the specified stat is selected in the list view
		/// </summary>
		/// <param name="stat">The stat to check</param>
		/// <returns>True if the stat is selected, false otherwise</returns>
		private bool IsStatSelected(Stat stat)
		{
			// Iterate through the list of items and remove them
			foreach (ListViewItem item in StatDataList.SelectedItems)
			{
				if (stat == item.Tag)
				{
					return true;
				}
			}
			return false;
		}

		/// <summary>
		/// Draws the bottom lines, frame #s, and times
		/// </summary>
		/// <param name="Gfx">The graphics object to draw with</param>
		private void DrawBottomMarkers(Graphics Gfx)
		{
			// Draw the bottom bar and tick marks
			int Y = StatsAreaScrollbar.Top - 4;
			Gfx.DrawLine(Pens.Black,0,Y,StatsArea.Width,Y);
			// Left tick
			Gfx.DrawLine(Pens.Black,0,Y,0,Y - 6);
			// Center tick
			int Center = StatsArea.Width / 2;
			Gfx.DrawLine(Pens.Black,Center,Y,Center,Y - 6);
			// Right tick
			int Right = StatsArea.Width - 2;
			Gfx.DrawLine(Pens.Black,Right,Y,Right,Y - 6);
			// Calculate where to draw the text
			int TextY = Y - 6 - GraphFont.Height;
			// Determine the type of text to show
			switch (ShowTextOptions)
			{
				case GraphTextOptions.GRAPH_ShowFrameNums:
				{
					string Text = DrawRangeStart.ToString();
					// Draw the starting frame #
					Gfx.DrawString(Text,GraphFont,Brushes.Black,0,TextY);
					// Draw the middle frame #
					Text = (DrawRangeStart + ((DrawRangeEnd - DrawRangeStart) / 2)).ToString();
					SizeF StrSize = Gfx.MeasureString(Text,GraphFont);
					Gfx.DrawString(Text,GraphFont,Brushes.Black,
						Center - (StrSize.Width / 2),TextY);
					// Draw the ending frame #
					Text = DrawRangeEnd.ToString();
					StrSize = Gfx.MeasureString(Text,GraphFont);
					Gfx.DrawString(Text,GraphFont,Brushes.Black,
						Right - StrSize.Width,TextY);
					break;
				}
				case GraphTextOptions.GRAPH_ShowTimes:
				{
					// Draw the starting frame time
					double ElapsedTime = CurrentFile.Frames[DrawRangeStart].ElapsedTime / 1000.0;
					string Text = ElapsedTime.ToString("F2");
					Gfx.DrawString(Text,GraphFont,Brushes.Black,0,TextY);
					// Draw the middle frame time
					ElapsedTime = CurrentFile.Frames[(DrawRangeStart + ((DrawRangeEnd - DrawRangeStart) / 2))].ElapsedTime / 1000.0;
					Text = ElapsedTime.ToString("F2");
					SizeF StrSize = Gfx.MeasureString(Text,GraphFont);
					Gfx.DrawString(Text,GraphFont,Brushes.Black,Center - (StrSize.Width / 2),TextY);
					// Draw the ending frame time
					ElapsedTime = CurrentFile.Frames[DrawRangeEnd].ElapsedTime / 1000.0;
					Text = ElapsedTime.ToString("F2");
					StrSize = Gfx.MeasureString(Text,GraphFont);
					Gfx.DrawString(Text,GraphFont,Brushes.Black,Right - StrSize.Width,TextY);
					break;
				}
			}
		}

		/// <summary>
		///  Draw the scale info on the side
		/// </summary>
		/// <param name="Gfx">The graphics object to draw with</param>
		/// <param name="ViewportHeight">The height of the stats area</param>
		void DrawLeftMarkers(Graphics Gfx,int ViewportHeight)
		{
			// Draw the lines
			Gfx.DrawLine(Pens.Black,0,0,0,ViewportHeight);
			Gfx.DrawLine(Pens.Black,0,0,2,0);
			Gfx.DrawLine(Pens.Black,0,ViewportHeight / 2,6,ViewportHeight / 2);
			Gfx.DrawLine(Pens.Black,0,ViewportHeight,2,ViewportHeight);
			int MaxScaledValue = (int)(DrawMaxValue * ScaleFactor);
			int HalfMaxScaledValue = MaxScaledValue / 2;
			// Draw the scale values
			Gfx.DrawString("0",GraphFont,Brushes.Black,2,ViewportHeight - 12);
			Gfx.DrawString(HalfMaxScaledValue.ToString(),GraphFont,Brushes.Black,2,ViewportHeight / 2 + 2);
			Gfx.DrawString(MaxScaledValue.ToString(),GraphFont,Brushes.Black,2,2);
		}

		/// <summary>
		/// Draws the graph of the current set of stats
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatsArea_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
		{
			try
			{
				// Don't try to draw if we don't have any data
				if (CurrentFile != null && CurrentFile.Frames.Length > 0)
				{
					// Draw a stat every 2 (or more) pixels
					float XStepSize = Math.Max(2.0F,(float)StatsArea.Width /
						Math.Min((float)CurrentFile.Frames.Length,(float)NumViewableFrames));
					int CurrentX = 0;
					float FloatCurrentX = 0.0F;
					int ViewportHeight = StatsAreaScrollbar.Top - 12 - GraphFont.Height;
					// Figure out how many pixels per stat value to use
					float YScale = (float)((double)ViewportHeight / (DrawMaxValue * ScaleFactor));
					// For each frame in the range
					for (int FrameNo = DrawRangeStart; FrameNo < DrawRangeEnd; FrameNo++)
					{
						// Get the frame that is being processing
						Frame CurrentFrame = CurrentFile.Frames[FrameNo];
						Frame NextFrame = CurrentFile.Frames[FrameNo + 1];
						if (CurrentFrame != null && NextFrame != null)
						{
							// Loop through the tracked stats and get their values
							foreach (Stat stat in TrackedStats)
							{
								int StartY, EndY;
								// Set our X positions
								int StartX = (int)CurrentX;
								int EndX = (int)(CurrentX + XStepSize);
								// Get the first & next stat's instance
								PerFrameStatData FirstStat = CurrentFrame.GetPerFrameStat(stat.StatId);
								PerFrameStatData NextStat = NextFrame.GetPerFrameStat(stat.StatId);
								// It's possible that we dropped a stat or it hasn't
								// arrived yet
								if (FirstStat != null && NextStat != null)
								{
									// Use the premultiplied value
									if (stat.Type == Stat.StatType.STATTYPE_CycleCounter)
									{
										// Determine the Y values for the points
										StartY = ViewportHeight - (int)Math.Min(ViewportHeight,
											Math.Max(0,YScale * FirstStat.TotalTime));
										EndY = ViewportHeight - (int)Math.Min(ViewportHeight,
											Math.Max(0,YScale * NextStat.TotalTime));
									}
									else
									{
										// Determine the Y values for the points
										StartY = ViewportHeight - (int)Math.Min(ViewportHeight,
											Math.Max(0,YScale * FirstStat.Total));
										EndY = ViewportHeight - (int)Math.Min(ViewportHeight,
											Math.Max(0,YScale * NextStat.Total));
									}
									// Get the pen to draw with for this stat. Selected stats
									// use the bold pen
									Pen pen = Colors.GetPen(stat.ColorId,IsStatSelected(stat));
									// Now draw it
									e.Graphics.DrawLine(pen,StartX,StartY,EndX,EndY);
								}
							}
						}
						// Move to the next X location to draw from
						FloatCurrentX += XStepSize;
						CurrentX = (int)FloatCurrentX;
					}
					// Draw the frame # / time markers
					DrawBottomMarkers(e.Graphics);
					// Draw the scale info on the side
					DrawLeftMarkers(e.Graphics,ViewportHeight);
				}
			}
			catch (Exception E)
			{
				Console.WriteLine("Exception in StatsArea_Paint:\r\n" + E.ToString());
			}
		}

		/// <summary>
		/// Handles keypresses in the list view. Used to delete stats from the list
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatDataList_KeyUp(object sender, System.Windows.Forms.KeyEventArgs e)
		{
			// If the user is trying to remove the selected stat from the list
			if (e.KeyCode == Keys.Delete || e.KeyCode == Keys.Back)
			{
				StatDataList.BeginUpdate();
				// Iterate through the list of items and remove them
				foreach (ListViewItem item in StatDataList.SelectedItems)
				{
					// Get the stat that this item corresponds to
					Stat stat = (Stat)item.Tag;
					// Remove the item from the list view
					StatDataList.Items.Remove(item);
					// Remove the stat from our tracked stats
					TrackedStats.Remove(stat);
				}
				StatDataList.EndUpdate();
				// Redraw the view
				StatsArea.Invalidate();
			}
		}

		/// <summary>
		/// Recalculates the draw range (which frames) for the graph display
		/// </summary>
		/// <param name="Delta">The change to apply to the range</param>
		private void UpdateDrawRange(int Delta)
		{
			if (Delta != 0)
			{
				// Change the range of what we are drawing based upon the
				// change in scroll location. Make sure the window stays within
				// array boundaries
				int NewStart = Math.Max(DrawRangeStart + Delta,0);
				// Don't adjust the start until we have more data than the
				// number of frames we intend to show
				if (NewStart + NumViewableFrames < CurrentFile.Frames.Length)
				{
					DrawRangeStart = NewStart;
				}
				// Don't draw beyond the end
				DrawRangeEnd = Math.Min(DrawRangeStart + NumViewableFrames - 1,
					CurrentFile.Frames.Length - 1);
				// Update the aggregate information for the range
				RebuildListView(AggDataOptions == AggregateDataOptions.AGGDATA_Ranged);
			}
		}

		/// <summary>
		/// Recalculates the draw range (which frames) for the graph display
		/// </summary>
		private void UpdateDrawRange()
		{
			if (CurrentFile != null && CurrentFile.Frames.Length > 0)
			{
				// Don't draw beyond the end
				DrawRangeEnd = Math.Min(DrawRangeStart + NumViewableFrames - 1,
					CurrentFile.Frames.Length - 1);
				// Update the aggregate information for the range
				RebuildListView(AggDataOptions == AggregateDataOptions.AGGDATA_Ranged);
			}
			else
			{
				DrawRangeStart = DrawRangeEnd = 0;
			}
		}

		/// <summary>
		/// Handles scrolling through the frame data
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatsAreaScrollbar_Scroll(object sender, System.Windows.Forms.ScrollEventArgs e)
		{
			// Determine the amount of change that happend
			int Delta = e.NewValue - StatsAreaScrollbar.Value;
			if (Delta != 0)
			{
				UpdateDrawRange(Delta);
				// Force an update
				StatsArea.Invalidate();
			}
		}

		/// <summary>
		/// Refreshes the display area when the user selects different stats
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatDataList_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			StatsArea.Invalidate();
		}

		/// <summary>
		/// Handles the zoom in request
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ZoomIn_Click(object sender, System.EventArgs e)
		{
			// Don't scale too small
			if (ScaleFactor > 0.125)
			{
				LastScaleFactor = ScaleFactor;
				ScaleFactor /= 2;
				StatsArea.Invalidate();
			}
		}

		/// <summary>
		/// Handles the zoom out request
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ZoomOut_Click(object sender, System.EventArgs e)
		{
			if (ScaleFactor < 16)
			{
				LastScaleFactor = ScaleFactor;
				ScaleFactor *= 2;
				StatsArea.Invalidate();
			}
		}

		/// <summary>
		/// Brings up the color chooser so that the user can change the
		/// color of the stat that was selected
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatDataList_ItemActivate(object sender, System.EventArgs e)
		{
			ListViewItem Item = StatDataList.SelectedItems[0];
			// Get the stat from the list view
			Stat stat = (Stat)Item.Tag;
			// Get the color and set it as the color in the color dialog
			Color color = Colors.GetColor(stat.ColorId);
			ColorDialog ColorDlg = new ColorDialog();
			ColorDlg.Color = color;
			// Show the color chooser box
			if (ColorDlg.ShowDialog(this) == DialogResult.OK)
			{
				// Replace the color with the one the user chose
				Colors.SetColorForId(stat.ColorId,ColorDlg.Color);
				Item.BackColor = ColorDlg.Color;
				// Force redraws
				StatDataList.Invalidate();
				StatsArea.Invalidate();
			}
			ColorDlg.Dispose();
		}

		/// <summary>
		/// Toggles the show times flag
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ShowTimes_Click(object sender, System.EventArgs e)
		{
			ShowTextOptions = GraphTextOptions.GRAPH_ShowTimes;
			ShowTimes.Checked = true;
			ShowFrameNums.Checked = false;
			StatsArea.Invalidate();
		}

		/// <summary>
		/// Toggles the show frame numbers flag
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ShowFrameNums_Click(object sender, System.EventArgs e)
		{
			ShowTextOptions = GraphTextOptions.GRAPH_ShowFrameNums;
			ShowTimes.Checked = false;
			ShowFrameNums.Checked = true;
			StatsArea.Invalidate();
		}

		/// <summary>
		/// Allows the user to change the background color of the stats area
		/// by double clicking on it
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatsArea_DoubleClick(object sender, System.EventArgs e)
		{
			ColorDialog ColorDlg = new ColorDialog();
			// Get the background color so we can change it
			ColorDlg.Color = StatsArea.BackColor;
			// Show the color chooser box
			if (ColorDlg.ShowDialog(this) == DialogResult.OK)
			{
				// Replace the color with the one the user chose
				StatsArea.BackColor = ColorDlg.Color;
				StatsArea.Invalidate();
			}
			ColorDlg.Dispose();
		}

		/// <summary>
		/// Repopulates the list view when data and/or view preference changes
		/// </summary>
		/// <param name="bRefreshList">Whether to empty and re-add stats or not</param>
		private void RebuildListView(bool bRefreshList)
		{
			// Update the ranged data
			RebuildRangedAggregateData();
			// Does this require empting w/re-adding
			if (bRefreshList)
			{
				// Stop redraws until we are done adding
				StatDataList.BeginUpdate();
				// Empty the list view
				StatDataList.Items.Clear();
				// Add each tracked stat to the view again
				foreach (Stat stat in TrackedStats)
				{
					AddStatToList(stat);
				}
				StatDataList.Invalidate();
				// Allow redraws
				StatDataList.EndUpdate();
			}
		}

		/// <summary>
		/// Changes the view mode for the aggregate data
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ViewOverallAggData_Click(object sender, System.EventArgs e)
		{
			AggregateDataOptions LastOpts = AggDataOptions;
			// Change to viewing all aggregate data
			ViewRangedAggData.Checked = false;
			ViewOverallAggData.Checked = true;
			AggDataOptions = AggregateDataOptions.AGGDATA_Overall;
			// Don't cause an update if it's the same
			if (LastOpts != AggDataOptions)
			{
				// Repopulate the list with the new data
				RebuildListView(true);
			}
		}

		/// <summary>
		/// Changes the view mode for the aggregate data
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ViewRangedAggData_Click(object sender, System.EventArgs e)
		{
			AggregateDataOptions LastOpts = AggDataOptions;
			// Change to viewing ranged aggregate data
			ViewRangedAggData.Checked = true;
			ViewOverallAggData.Checked = false;
			AggDataOptions = AggregateDataOptions.AGGDATA_Ranged;
			// Don't cause an update if it's the same
			if (LastOpts != AggDataOptions)
			{
				// Repopulate the list with the new data
				RebuildListView(true);
			}
		}

		/// <summary>
		/// Iterates through the frames within the range, updating the aggregate
		/// data for each stat that we are tracking
		/// </summary>
		private void RebuildRangedAggregateData()
		{
			// Get rid of any previous data
			StatIdToRangeAggData.Clear();
			// Don't calculate if we aren't updating ranged data
			if (AggDataOptions == AggregateDataOptions.AGGDATA_Ranged)
			{
				// For each tracked stat, go through and build a ranged set of data
				foreach (Stat stat in TrackedStats)
				{
					// Create a new aggregate object to hold the aggregate data
					AggregateStatData AggData = new AggregateStatData();
					// Insert it into our hash
					StatIdToRangeAggData.Add(stat.StatId,AggData);
					// Go through each frame in this range updating the aggregate data
					for (int Index = DrawRangeStart; Index < DrawRangeEnd; Index++)
					{
						// Get the next frame
						Frame CurrentFrame = CurrentFile.Frames[Index];
						// Find the frame's set of data for this stat
						PerFrameStatData PerFrameData = CurrentFrame.GetPerFrameStat(stat.StatId);
						if (PerFrameData != null)
						{
							// Add each occurance to the aggregate
							foreach (Stat StatInstance in PerFrameData.Stats)
							{
								// Add it to our aggregate data
								AggData += StatInstance;
							}
						}
					}
				}
			}
		}

		/// <summary>
		/// Determines if the viewer is currently connected
		/// </summary>
		/// <returns>True if connected, false otherwise</returns>
		private bool IsConnected()
		{
			return Connection != null;
		}

		/// <summary>
		/// Handles disconnecting from a remote server. Prompts the user
		/// </summary>
		/// <returns>True if the user chose to disconnect, false otherwise</returns>
		private bool PerformDisconnect()
		{
			bool bDisconnected = false;
			// Ask if they really want to disconnect
			if (MessageBox.Show(this,"Are you sure you want to disconnect from your current session?",
				"Disconnect from server?",MessageBoxButtons.YesNo,MessageBoxIcon.Question) ==
				DialogResult.Yes)
			{
				// Update the title bar
				Text = InitialWindowName + "Disconnected from: " +
					Connection.Address.ToString();
				// Tells the server we are no longer interested in receiving packets
				Connection.SendDisconnectRequest();
				bDisconnected = true;
				// Disconnect from the server
				Connection.Disconnect();
				// Process any outstanding packets so that all the data is accounted for
				ProcessPackets();
				// Mark the connection as empty
				Connection = null;
			}
			return bDisconnected;
		}

		/// <summary>
		/// Saves the data to the specified file. Prompts the user about saving
		/// the bShouldPrompt flag is true. 
		/// </summary>
		/// <param name="bShouldPrompt"></param>
		/// <returns></returns>
		private bool PerformSave(bool bShouldPrompt)
		{
			bool bDidSave = false;
			// Default to true if we aren't prompting, otherwise default to false
			bool bShouldSave = bShouldPrompt == false;
			// Ask the user if they want to save
			if (bShouldPrompt)
			{
				bShouldSave = MessageBox.Show(this,
					"You have unsaved data. Do you want to save before continuing?",
					"Save current data?",MessageBoxButtons.YesNo,
					MessageBoxIcon.Question) == DialogResult.Yes;
			}
			// If we should save, bring up the dialog
			if (bShouldSave)
			{
				SaveFileDialog SaveFileDlg = new SaveFileDialog();
				// Filter on our XML file extension
				SaveFileDlg.Filter = "XML Files (*.xml)|*.xml|All Files (*.*)|*.*";
				SaveFileDlg.FilterIndex = 1;
				SaveFileDlg.RestoreDirectory = true;
				// Show the dialog and only load the file if the user said ok
				if (SaveFileDlg.ShowDialog() == DialogResult.OK)
				{
					// Get the XML data stream to read from
					Stream XmlStream = SaveFileDlg.OpenFile();
					// Creates an instance of the XmlSerializer class so we can
					// write the object graph
					XmlSerializer ObjSer = new XmlSerializer(typeof(StatFile));
					// Create an object graph from the XML data
					ObjSer.Serialize(XmlStream,CurrentFile);
					// Don't keep the document locked
					XmlStream.Close();
					// Update the title bar
					Text = InitialWindowName + SaveFileDlg.FileName;
					SaveFileDlg.Dispose();
					bDidSave = true;
				}
			}
			return bDidSave;
		}

		/// <summary>
		/// Creates the connection dialog showing the list of available games
		/// that can be connected to. If the user chooses a connection, it
		/// closes the previous one.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ConnectTo_Click(object sender, System.EventArgs e)
		{
			// If we are currently connected, prompt to close/save the data
			if (IsConnected())
			{
				if (PerformDisconnect() == false)
				{
					// Now prompt to save the data
					PerformSave(true);
				}
				else
				{
					// The user didn't want to disconnect
					return;
				}
			}
			// Now show the connection dialog
			NetworkConnectionDialog NetDlg = new NetworkConnectionDialog();
			if (NetDlg.ShowDialog(this) == DialogResult.OK)
			{
				// Get the connection info from the dialog
				Connection = NetDlg.GetServerConnection();
				// Connect to the specified server
				ConnectToServer();
				// Update the title bar
				Text = InitialWindowName + "Connected to: " +
					Connection.Address.ToString();
			}
			NetDlg.Dispose();
		}

		/// <summary>
		/// Closes the remote connection by telling the client to remove it
		/// from the list of interested clients
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void CloseConnection_Click(object sender, System.EventArgs e)
		{
			// If we are currently connected, prompt to close/save the data
			if (IsConnected())
			{
				// Make sure they really meant this
				if (PerformDisconnect() == true)
				{
					// Now prompt to save the data
					PerformSave(true);
				}
			}
		}

		/// <summary>
		/// Creates our connection to the remote server. Creates a new file object
		/// to receive the remote data
		/// </summary>
		private void ConnectToServer()
		{
			try
			{
				// Cache the settings
				LastUsedSettings.LastIpAddress = Connection.Address.ToString();
				LastUsedSettings.LastPort = Connection.Port;
				// Close the dependent views first
				ClosePerFrameViews();
				// Create the new place we'll store data
				CurrentFile = new StatFile();
				// Set up our in/out connections
				Connection.Connect();
				// Tell the server we're ready to start receiving data
				Connection.SendConnectRequest();
			}
			catch (Exception e)
			{
				MessageBox.Show("Failed to connect to server " +
					Connection.Address.ToString() + "\r\nDue to: " +
					e.ToString());
			}
		}
		
		/// <summary>
		/// Processes any pending packets by converting them into object form
		/// </summary>
		private void ProcessPackets()
		{
			if (IsConnected())
			{
				// Vars used for figuring out the type of updates needed
				bool bRequiresTreeRebuild = false;
				int NumPacketsProcessed = 0;
				int NumFramesAdded = 0;
				// While there are packets to be processed
				for (Packet packet = Connection.GetNextPacket();
					packet != null;
					packet = Connection.GetNextPacket())
				{
					NumPacketsProcessed++;
					// Get the 2 character packet code
					string PacketType = Encoding.ASCII.GetString(packet.Data,0,2);
					// Route to the correct class/handler based upon packet type
					switch (PacketType)
					{
						// SD == Stat description
						case "SD":
						{
							bRequiresTreeRebuild = true;
							// Add the new stat description
							CurrentFile.AppendStatDescription(new Stat(PacketType,packet.Data));
							break;
						}
						// Ux == Update an existing stat
						case "UC":
						case "UD":
						case "UF":
						{
							// Create a new stat object from the update data
							CurrentFile.AppendStat(new Stat(PacketType,packet.Data));
							break;
						}
						// GD == Group description
						case "GD":
						{
							bRequiresTreeRebuild = true;
							// Create a new group description
							CurrentFile.AppendGroupDescription(new Group(packet.Data));
							break;
						}
						// PC == Sends the value of Seconds Per Cycle so that we
						// can properly convert from cycles to ms
						case "PC":
						{
							CurrentFile.UpdateConversionFactor(packet.Data);
							break;
						}
						// NF == New frame
						case "NF":
						{
							NumFramesAdded++;
							// Create a new frame object that we'll add updates to
							CurrentFile.AppendFrame(new Frame(packet.Data));
							break;
						}
						default:
						{
							Console.WriteLine("Unknown packet type " + PacketType);
							break;
						}
					}
				}
				// Don't do the updating of data unless needed
				if (NumPacketsProcessed > 0)
				{
					// Fix up an items that were added
					CurrentFile.FixupRecentItems();
					// Rebuild the tree data if new stats/groups were added
					if (bRequiresTreeRebuild)
					{
						BuildTreeView();
					}
					// Update the scrollbars and stats area if needed
					if (NumFramesAdded > 0)
					{
						ScrollStats();
					}
				}
			}
		}

		/// <summary>
		/// Scrolls the scrollbar and updates the view
		/// </summary>
		private void ScrollStats()
		{
			// Set the new max
			StatsAreaScrollbar.Maximum = CurrentFile.Frames.Length - 1;
			if (bShouldAutoScroll)
			{
				int Delta = StatsAreaScrollbar.Maximum - StatsAreaScrollbar.Value - 1;
				// It was all the way to the right so auto scroll
				StatsAreaScrollbar.Value = StatsAreaScrollbar.Maximum;
				UpdateDrawRange(Delta);
				// Force an update
				StatsArea.Invalidate();
			}
		}

		/// <summary>
		/// Processes any packets that have been queued up since the last timer
		/// event
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void OnTimer(Object sender,EventArgs e)
		{
			// Stop it in case it takes a long time to process
			ProcessPacketsTimer.Stop();
			// Process any pending packets
			ProcessPackets();
			// Start the timer again
			ProcessPacketsTimer.Start();
		}

		/// <summary>
		/// Toggles the auto scrolling state
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void AutoScrollDisplay_Click(object sender, System.EventArgs e)
		{
			bShouldAutoScroll ^= true;
			AutoScrollDisplay.Checked = bShouldAutoScroll;
		}

		/// <summary>
		/// Updates the stats area when resized
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatsArea_Resize(object sender, System.EventArgs e)
		{
			// Change the viewable range
			NumViewableFrames = StatsArea.Width / 2;
			// Force update that
			UpdateDrawRange();
			// Force a redraw
			StatsArea.Invalidate();
		}

		/// <summary>
		/// Updates the view frame popup menu based upon the selected frame
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ViewFrameMenu_Popup(object sender, System.EventArgs e)
		{
			// Get the menu item that needs adjusting
			MenuItem Item = ViewFrameMenu.MenuItems[0];
			// Make sure this was done from the stats area
			if (ViewFrameMenu.SourceControl == StatsArea && IsInStatsArea())
			{
				// Get the frame number so it can be appended
				int FrameNum = GetFrameNumUnderMouse();
				// Update the item depending upon whether we have valid data
				if (CurrentFile != null && CurrentFile.Frames.Length > 0 &&
					FrameNum >= 0)
				{
					Item.Enabled = true;
					Item.Text = "View Frame " + FrameNum.ToString();
				}
				else
				{
					// Disable it
					Item.Enabled = false;
					Item.Text = "View Frame";
				}
			}
			else
			{
				// Disable it
				Item.Enabled = false;
				Item.Text = "View Frame";
			}
		}

		/// <summary>
		/// Determines if the mouse is currently within the stats graph area
		/// </summary>
		/// <returns>true if it is, false otherwise</returns>
		private bool IsInStatsArea()
		{
			// Transform the mouse position relative to the stats area
			Point LocalPos = StatsArea.PointToClient(Cursor.Position);
			int ViewportHeight = StatsAreaScrollbar.Top;
			int ViewportWidth = StatsArea.Width;
			// If it's within the width & height, then it's ok
			return LocalPos.X < ViewportWidth && LocalPos.Y < ViewportHeight;
		}

		/// <summary>
		/// Tracks the mouse position for determining which frame # the mouse
		/// is currently positioned at
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatsArea_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			// Track the location of the mouse so we can show frame data
			StatsAreaMouseLocation.X = e.X;
			StatsAreaMouseLocation.Y = e.Y;
		}

		/// <summary>
		/// Determines the frame object that is beneath the mouse cursor and
		/// returns that object to the caller
		/// </summary>
		/// <returns>The frame beneath the current mouse location</returns>
		private int GetFrameNumUnderMouse()
		{
			int FrameOffset = -1;
			if (CurrentFile != null && CurrentFile.Frames.Length > 0)
			{
				// Determine the number of pixels per frame
				float XStepSize = Math.Max(2.0F,(float)StatsArea.Width /
					Math.Min((float)CurrentFile.Frames.Length,(float)NumViewableFrames));
				// Divide the X location by the step size to figure out the
				// frame number from the start
				FrameOffset = (int)((float)StatsAreaMouseLocation.X / XStepSize);
				// Clamp that frame offset
				FrameOffset = Math.Min(DrawRangeStart + FrameOffset,CurrentFile.Frames.Length);
			}
			return FrameOffset;
		}

		/// <summary>
		/// Opens a call graph view for the frames near the mouse
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ViewFrameNumMenu_Click(object sender, System.EventArgs e)
		{
			// Figure out which frame we are over
			int FrameNum = GetFrameNumUnderMouse();
			ArrayList Frames = new ArrayList();
			if (FrameNum > 0)
			{
				// Add the previous frame first
				Frames.Add(CurrentFile.Frames[FrameNum - 1]);
			}
			// Add the currently specified frame
			Frames.Add(CurrentFile.Frames[FrameNum]);
			// Add the following frame too
			if (FrameNum < CurrentFile.Frames.Length - 1)
			{
				Frames.Add(CurrentFile.Frames[FrameNum + 1]);
			}
			// Crete the view with the set of frames
			CreateFramesView(Frames);
		}

		/// <summary>
		/// Common function for creating a frames view from an array of frames
		/// </summary>
		/// <param name="Frames">The data to populate the view with</param>
		private void CreateFramesView(ArrayList Frames)
		{
			// Create the new view with the set of frames
			PerFrameView NewView = new PerFrameView(this,Frames);
			NewView.Show();
			// Add to our internal list so it can be closed when the file/connection is closed
			PerFrameViews.Add(NewView);
		}

		/// <summary>
		/// Removes the specified view from the child view list
		/// </summary>
		/// <param name="View">the view to remove</param>
		public void RemovePerFrameView(PerFrameView View)
		{
			PerFrameViews.Remove(View);
		}

		/// <summary>
		/// Closes all of the per frame views and empties the list
		/// </summary>
		public void ClosePerFrameViews()
		{
			// Make a new per frame list
			ArrayList LocalList = PerFrameViews;
			PerFrameViews = new ArrayList();
			// Tell each to close down
			foreach (PerFrameView View in LocalList)
			{
				View.Close();
			}
		}

		/// <summary>
		/// Processes the context menu popup event. Enables/disables the menu as
		/// based upon whether the user selected a stat
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void StatsTreeMenu_Popup(object sender, System.EventArgs e)
		{
			bool bEnabled = false;
			// Transform the mouse position relative to the tree control
			Point LocalPos = StatGroupsTree.PointToClient(Cursor.Position);
			// Find the item that was selected
			TreeNode Node = StatGroupsTree.GetNodeAt(LocalPos);
			if (Node != null)
			{
				UIBaseElement Base = (UIBaseElement)Node.Tag;
				// Skip if not a stats object
				if (Base.UIType == UIBaseElement.ElementType.StatsObject)
				{
					bEnabled = true;
					StatGroupsTree.SelectedNode = Node;
				}
			}
			// Enable or disable the menu accordingly
			StatsTreeMenu.MenuItems[0].Enabled = bEnabled;
		}

		/// <summary>
		/// Displays the search dialog when chosen. Kicks off the search with
		/// the user specifications
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ViewFramesByCriteria_Click(object sender, System.EventArgs e)
		{
			// Find the item that was selected
			TreeNode Node = StatGroupsTree.SelectedNode;
			if (Node != null)
			{
				UIBaseElement Base = (UIBaseElement)Node.Tag;
				// Skip if not a stats object
				if (Base.UIType == UIBaseElement.ElementType.StatsObject)
				{
					Stat stat = (Stat)Base;
					// Create the dialog and tell it this stat
					ViewFramesByCriteriaDialog Dlg = new ViewFramesByCriteriaDialog(stat.Name);
					if (Dlg.ShowDialog(this) == DialogResult.OK)
					{
						// Open a set of frames that match the search criteria
						DisplaySearchResults(stat,Dlg.GetSearchType(),
							Dlg.GetSearchValue());
					}
				}
			}
		}

		/// <summary>
		/// Opens a tree view with a set of frames from a search
		/// </summary>
		/// <param name="stat">The stat being searched for</param>
		/// <param name="SearchType">The criteria to search with</param>
		/// <param name="Value">The value to compare against</param>
		private void DisplaySearchResults(Stat stat,SearchByType SearchType,double Value)
		{
			ArrayList Frames = new ArrayList();
			// Iterate over the set of frames capturing ones that match
			foreach (Frame frame in CurrentFile.Frames)
			{
				if (DoesFrameMatchSearchCriteria(frame,stat,SearchType,Value))
				{
					Frames.Add(frame);
				}

			}
			// Show the built data
			CreateFramesView(Frames);
		}

		/// <summary>
		/// Builds an aggregate of the frame's data for a specific stat and then
		/// compares the results against the search criteria
		/// </summary>
		/// <param name="frame">The frame to process</param>
		/// <param name="stat">The stat to search for</param>
		/// <param name="SearchType">The search matching option</param>
		/// <param name="Value">The value to match</param>
		/// <returns>true if this frame is a match, false otherwise</returns>
		bool DoesFrameMatchSearchCriteria(Frame frame,Stat stat,SearchByType SearchType,double Value)
		{
			bool bMatches = false;
			// Check the per frame data for the stat
			PerFrameStatData PerFrameData = frame.GetPerFrameStat(stat.StatId);
			if (PerFrameData != null)
			{
				double StatValue = 0.0;
				// Figure out if it is a counter or not
				if (stat.Type == Stat.StatType.STATTYPE_CycleCounter)
				{
					StatValue = PerFrameData.TotalTime;
				}
				else
				{
					StatValue = PerFrameData.Total;
				}
				// Now do the compare based upon the search type
				if (SearchType == SearchByType.GreaterThan)
				{
					bMatches = (StatValue > Value);
				}
				else if (SearchType == SearchByType.GreaterThan)
				{
					bMatches = (StatValue < Value);
				}
				else
				{
					bMatches = (StatValue == Value);
				}
			}
			return bMatches;
		}

		/// <summary>
		/// Displays a dialog for entering an IP address and connects to it
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ConnectToIP_Click(object sender, System.EventArgs e)
		{
			// If we are currently connected, prompt to close/save the data
			if (IsConnected())
			{
				if (PerformDisconnect() == false)
				{
					// Now prompt to save the data
					PerformSave(true);
				}
				else
				{
					// The user didn't want to disconnect
					return;
				}
			}
			// Now show the connection dialog
			ConnectToIPDialog ConnectToDlg = new ConnectToIPDialog(LastUsedSettings.LastIpAddress,
				LastUsedSettings.LastPort.ToString());
			if (ConnectToDlg.ShowDialog(this) == DialogResult.OK)
			{
				// Get the connection info from the dialog
				Connection = ConnectToDlg.GetServerConnection();
				// Connect to the specified server
				ConnectToServer();
				// Update the title bar
				Text = InitialWindowName + "Connected to: " +
					Connection.Address.ToString();
			}
			ConnectToDlg.Dispose();
		}

		/// <summary>
		/// Reads the last used application settings from the XML file
		/// </summary>
		/// <returns>true if there were valid settings, false otherwise</returns>
		bool LoadUserSettings()
		{
			Stream XmlStream = null;
			try
			{
				string Name = Application.ExecutablePath.Substring(0,Application.ExecutablePath.Length - 4);
				Name += "_user.xml";
				// Get the XML data stream to read from
				XmlStream = new FileStream(Name,FileMode.Open,
					FileAccess.Read,FileShare.None,256 * 1024,false);
				// Creates an instance of the XmlSerializer class so we can
				// read the settings object
				XmlSerializer ObjSer = new XmlSerializer(typeof(SavedSettings));
				// Add our callbacks for a busted XML file
				ObjSer.UnknownNode += new XmlNodeEventHandler(XmlSerializer_UnknownNode);
				ObjSer.UnknownAttribute += new XmlAttributeEventHandler(XmlSerializer_UnknownAttribute);
				// Create an object graph from the XML data
				LastUsedSettings = (SavedSettings)ObjSer.Deserialize(XmlStream);
				if (LastUsedSettings == null)
				{
					LastUsedSettings = new SavedSettings();
				}
			}
			catch (Exception e)
			{
				Console.WriteLine(e.ToString());
				return false;
			}
			finally
			{
				if (XmlStream != null)
				{
					// Done with the file so close it
					XmlStream.Close();
				}
			}
			return true;
		}

		/// <summary>
		/// Saves the current application settings to the XML file
		/// </summary>
		private void SaveUserSettings()
		{
			Stream XmlStream = null;
			try
			{
				string Name = Application.ExecutablePath.Substring(0,Application.ExecutablePath.Length - 4);
				Name += "_user.xml";
				// Get the XML data stream to read from
				XmlStream = new FileStream(Name,FileMode.Create,
					FileAccess.Write,FileShare.None,256 * 1024,false);
				// Creates an instance of the XmlSerializer class so we can
				// save the settings object
				XmlSerializer ObjSer = new XmlSerializer(typeof(SavedSettings));
				// Write the object graph as XML data
				ObjSer.Serialize(XmlStream,LastUsedSettings);
			}
			catch (Exception e)
			{
				Console.WriteLine(e.ToString());
			}
			finally
			{
				if (XmlStream != null)
				{
					// Done with the file so close it
					XmlStream.Close();
				}
			}
		}
	}
}
