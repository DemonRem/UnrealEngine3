
using System;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using System.Text;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Net;
using System.Drawing.Imaging;
using UnrealNetwork;
using ConsoleInterface;


namespace UnrealConsole
{
    enum EConsoleState
    {
        CS_Running,
        CS_NotRunning,
        CS_Crashed,
        CS_Asserted,
        CS_RIP,
    };

    /// <summary>
    /// Summary description for Form1.
    /// </summary>
    public class UnrealConsole : System.Windows.Forms.Form
    {
        private System.Windows.Forms.MenuItem		IDM_EXIT;
        private System.Windows.Forms.MenuItem IDM_CLEARWINDOW;
        private System.Windows.Forms.MenuItem		IDM_SAVE;
        private System.Windows.Forms.MenuItem		IDM_STARTMEMORYCAPTURE;
        private System.Windows.Forms.MenuItem IDM_STOPMEMORYCAPTURE;
        private System.Windows.Forms.MenuItem		FileMenu;
        private System.Windows.Forms.MenuItem EditMenu;
        private System.Windows.Forms.MenuItem		ProfilingMenu;
        private System.Windows.Forms.MainMenu		MainMenu;
        private System.Windows.Forms.MenuItem		FileSeparator1;
        private System.Windows.Forms.StatusBar StatusBar;
        private System.Windows.Forms.MenuItem IDM_CLEARMEMORYCAPTURE;
        private IContainer components;

        /// <summary>
        /// Singleton instance of the DLLInterface
        /// </summary>
        static public DLLInterface DLLInterface;

        /// <summary>
        /// The index/ID for the connected target (-1 if nothing is connected, which means don't do anything!)
        /// </summary>
        static public int ConnectedTargetIndex;

        /// <summary>
        /// Platform currently in use
        /// </summary>
        public string PlatformName;

        /// <summary>
        /// Target currently connected to
        /// </summary>
        public string TargetName;

        private MenuItem IDM_CONNECT;
        private MenuItem IDM_WORDWRAP;
        private MenuItem menuItem1;
        private MenuItem menuItem2;
        private MenuItem IDM_REBOOT;
        private MenuItem IDM_SCREENCAPTURE;
        private TabPage MemoryPage;
        private RichTextBox MemoryText;
        private TabPage TTYPage;
        private TextBox TTYInput;
        private RichTextBox TTYText;
        private TabPage tabPage1;
        private PictureBox ScreenCaptureBox;
        private MenuItem IDM_UPDATESCREENTAB;
        private TabControl TabControl;

        public UnrealConsole(string InPlatform, string InTargetName)
        {
	        //
	        // Required for Windows Form Designer support
	        //
	        InitializeComponent();

	        AppName = Text;

	        DLLInterface = new DLLInterface();

	        LoadUserSettings();

	        // set the target
	        SetTarget(InPlatform, InTargetName);

	        // Add any constructor code after InitializeComponent call
	        LogWindow		= new FLogWindow( TTYText, TTYInput );
	        MemProfiler		= new FMemoryTracker(this, MemoryText);
	        GPUProfiler		= new FGPUProfiler( );

	        Show();
	        TTYInput.Focus();
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UnrealConsole));
            this.MainMenu = new System.Windows.Forms.MainMenu(this.components);
            this.FileMenu = new System.Windows.Forms.MenuItem();
            this.IDM_SAVE = new System.Windows.Forms.MenuItem();
            this.FileSeparator1 = new System.Windows.Forms.MenuItem();
            this.IDM_EXIT = new System.Windows.Forms.MenuItem();
            this.EditMenu = new System.Windows.Forms.MenuItem();
            this.IDM_CLEARWINDOW = new System.Windows.Forms.MenuItem();
            this.IDM_WORDWRAP = new System.Windows.Forms.MenuItem();
            this.menuItem1 = new System.Windows.Forms.MenuItem();
            this.IDM_CONNECT = new System.Windows.Forms.MenuItem();
            this.menuItem2 = new System.Windows.Forms.MenuItem();
            this.IDM_REBOOT = new System.Windows.Forms.MenuItem();
            this.IDM_SCREENCAPTURE = new System.Windows.Forms.MenuItem();
            this.IDM_UPDATESCREENTAB = new System.Windows.Forms.MenuItem();
            this.ProfilingMenu = new System.Windows.Forms.MenuItem();
            this.IDM_STARTMEMORYCAPTURE = new System.Windows.Forms.MenuItem();
            this.IDM_STOPMEMORYCAPTURE = new System.Windows.Forms.MenuItem();
            this.IDM_CLEARMEMORYCAPTURE = new System.Windows.Forms.MenuItem();
            this.StatusBar = new System.Windows.Forms.StatusBar();
            this.MemoryPage = new System.Windows.Forms.TabPage();
            this.MemoryText = new System.Windows.Forms.RichTextBox();
            this.TTYPage = new System.Windows.Forms.TabPage();
            this.TTYInput = new System.Windows.Forms.TextBox();
            this.TTYText = new System.Windows.Forms.RichTextBox();
            this.TabControl = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.ScreenCaptureBox = new System.Windows.Forms.PictureBox();
            this.MemoryPage.SuspendLayout();
            this.TTYPage.SuspendLayout();
            this.TabControl.SuspendLayout();
            this.tabPage1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ScreenCaptureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // MainMenu
            // 
            this.MainMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.FileMenu,
            this.EditMenu,
            this.menuItem1,
            this.ProfilingMenu});
            // 
            // FileMenu
            // 
            this.FileMenu.Index = 0;
            this.FileMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.IDM_SAVE,
            this.FileSeparator1,
            this.IDM_EXIT});
            this.FileMenu.Text = "&File";
            // 
            // IDM_SAVE
            // 
            this.IDM_SAVE.Index = 0;
            this.IDM_SAVE.Shortcut = System.Windows.Forms.Shortcut.CtrlS;
            this.IDM_SAVE.Text = "&Save log...";
            this.IDM_SAVE.Click += new System.EventHandler(this.IDM_SAVE_Click);
            // 
            // FileSeparator1
            // 
            this.FileSeparator1.Index = 1;
            this.FileSeparator1.Text = "-";
            // 
            // IDM_EXIT
            // 
            this.IDM_EXIT.Index = 2;
            this.IDM_EXIT.Shortcut = System.Windows.Forms.Shortcut.AltF4;
            this.IDM_EXIT.Text = "E&xit";
            this.IDM_EXIT.Click += new System.EventHandler(this.OnExit);
            // 
            // EditMenu
            // 
            this.EditMenu.Index = 1;
            this.EditMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.IDM_CLEARWINDOW,
            this.IDM_WORDWRAP});
            this.EditMenu.Text = "&Edit";
            // 
            // IDM_CLEARWINDOW
            // 
            this.IDM_CLEARWINDOW.Index = 0;
            this.IDM_CLEARWINDOW.Shortcut = System.Windows.Forms.Shortcut.F11;
            this.IDM_CLEARWINDOW.Text = "&Clear window";
            this.IDM_CLEARWINDOW.Click += new System.EventHandler(this.OnClearWindow);
            // 
            // IDM_WORDWRAP
            // 
            this.IDM_WORDWRAP.Index = 1;
            this.IDM_WORDWRAP.Shortcut = System.Windows.Forms.Shortcut.F10;
            this.IDM_WORDWRAP.Text = "Word Wrap";
            this.IDM_WORDWRAP.Click += new System.EventHandler(this.IDM_WORDWRAP_Click);
            // 
            // menuItem1
            // 
            this.menuItem1.Index = 2;
            this.menuItem1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.IDM_CONNECT,
            this.menuItem2,
            this.IDM_REBOOT,
            this.IDM_SCREENCAPTURE,
            this.IDM_UPDATESCREENTAB});
            this.menuItem1.Text = "Console";
            // 
            // IDM_CONNECT
            // 
            this.IDM_CONNECT.Index = 0;
            this.IDM_CONNECT.Shortcut = System.Windows.Forms.Shortcut.F2;
            this.IDM_CONNECT.Text = "Connect...";
            this.IDM_CONNECT.Click += new System.EventHandler(this.IDM_CONNECT_Click);
            // 
            // menuItem2
            // 
            this.menuItem2.Index = 1;
            this.menuItem2.Text = "-";
            // 
            // IDM_REBOOT
            // 
            this.IDM_REBOOT.Index = 2;
            this.IDM_REBOOT.Shortcut = System.Windows.Forms.Shortcut.F3;
            this.IDM_REBOOT.Text = "Reboot Target";
            this.IDM_REBOOT.Click += new System.EventHandler(this.IDM_REBOOT_Click);
            // 
            // IDM_SCREENCAPTURE
            // 
            this.IDM_SCREENCAPTURE.Index = 3;
            this.IDM_SCREENCAPTURE.Shortcut = System.Windows.Forms.Shortcut.F4;
            this.IDM_SCREENCAPTURE.Text = "Screen Capture (.jpg)...";
            this.IDM_SCREENCAPTURE.Click += new System.EventHandler(this.IDM_SCREENCAPTURE_Click);
            // 
            // IDM_UPDATESCREENTAB
            // 
            this.IDM_UPDATESCREENTAB.Index = 4;
            this.IDM_UPDATESCREENTAB.Shortcut = System.Windows.Forms.Shortcut.F5;
            this.IDM_UPDATESCREENTAB.Text = "Update Screen Tab";
            this.IDM_UPDATESCREENTAB.Click += new System.EventHandler(this.IDM_UPDATESCREENTAB_Click);
            // 
            // ProfilingMenu
            // 
            this.ProfilingMenu.Index = 3;
            this.ProfilingMenu.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
            this.IDM_STARTMEMORYCAPTURE,
            this.IDM_STOPMEMORYCAPTURE,
            this.IDM_CLEARMEMORYCAPTURE});
            this.ProfilingMenu.Text = "&Profilng";
            // 
            // IDM_STARTMEMORYCAPTURE
            // 
            this.IDM_STARTMEMORYCAPTURE.Index = 0;
            this.IDM_STARTMEMORYCAPTURE.Shortcut = System.Windows.Forms.Shortcut.F6;
            this.IDM_STARTMEMORYCAPTURE.Text = "&Start memory capture";
            this.IDM_STARTMEMORYCAPTURE.Click += new System.EventHandler(this.OnStartMemoryCapture);
            // 
            // IDM_STOPMEMORYCAPTURE
            // 
            this.IDM_STOPMEMORYCAPTURE.Enabled = false;
            this.IDM_STOPMEMORYCAPTURE.Index = 1;
            this.IDM_STOPMEMORYCAPTURE.Shortcut = System.Windows.Forms.Shortcut.ShiftF6;
            this.IDM_STOPMEMORYCAPTURE.Text = "&Stop memory capture";
            this.IDM_STOPMEMORYCAPTURE.Click += new System.EventHandler(this.OnStopMemoryCapture);
            // 
            // IDM_CLEARMEMORYCAPTURE
            // 
            this.IDM_CLEARMEMORYCAPTURE.Index = 2;
            this.IDM_CLEARMEMORYCAPTURE.Shortcut = System.Windows.Forms.Shortcut.F7;
            this.IDM_CLEARMEMORYCAPTURE.Text = "&Clear memory capture data";
            this.IDM_CLEARMEMORYCAPTURE.Click += new System.EventHandler(this.OnClearMemoryCapture);
            // 
            // StatusBar
            // 
            this.StatusBar.Location = new System.Drawing.Point(0, 747);
            this.StatusBar.Name = "StatusBar";
            this.StatusBar.Size = new System.Drawing.Size(1284, 22);
            this.StatusBar.TabIndex = 1;
            // 
            // MemoryPage
            // 
            this.MemoryPage.Controls.Add(this.MemoryText);
            this.MemoryPage.Location = new System.Drawing.Point(4, 22);
            this.MemoryPage.Name = "MemoryPage";
            this.MemoryPage.Size = new System.Drawing.Size(1276, 716);
            this.MemoryPage.TabIndex = 1;
            this.MemoryPage.Text = "Memory";
            // 
            // MemoryText
            // 
            this.MemoryText.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.MemoryText.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MemoryText.HideSelection = false;
            this.MemoryText.Location = new System.Drawing.Point(0, 0);
            this.MemoryText.Name = "MemoryText";
            this.MemoryText.ReadOnly = true;
            this.MemoryText.Size = new System.Drawing.Size(1276, 720);
            this.MemoryText.TabIndex = 0;
            this.MemoryText.TabStop = false;
            this.MemoryText.Text = "";
            this.MemoryText.WordWrap = false;
            // 
            // TTYPage
            // 
            this.TTYPage.Controls.Add(this.TTYInput);
            this.TTYPage.Controls.Add(this.TTYText);
            this.TTYPage.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TTYPage.Location = new System.Drawing.Point(4, 22);
            this.TTYPage.Name = "TTYPage";
            this.TTYPage.Size = new System.Drawing.Size(1276, 716);
            this.TTYPage.TabIndex = 0;
            this.TTYPage.Text = "TTY";
            // 
            // TTYInput
            // 
            this.TTYInput.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.TTYInput.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TTYInput.HideSelection = false;
            this.TTYInput.Location = new System.Drawing.Point(0, 696);
            this.TTYInput.Name = "TTYInput";
            this.TTYInput.Size = new System.Drawing.Size(1276, 20);
            this.TTYInput.TabIndex = 0;
            this.TTYInput.WordWrap = false;
            this.TTYInput.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.OnTTYInputKey);
            this.TTYInput.TextChanged += new System.EventHandler(this.OnTTYInputChanged);
            this.TTYInput.KeyDown += new System.Windows.Forms.KeyEventHandler(this.OnTYYInputKeyDown);
            // 
            // TTYText
            // 
            this.TTYText.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.TTYText.BackColor = System.Drawing.SystemColors.Window;
            this.TTYText.Font = new System.Drawing.Font("Courier New", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TTYText.HideSelection = false;
            this.TTYText.Location = new System.Drawing.Point(0, 0);
            this.TTYText.Name = "TTYText";
            this.TTYText.ReadOnly = true;
            this.TTYText.Size = new System.Drawing.Size(1276, 696);
            this.TTYText.TabIndex = 2;
            this.TTYText.TabStop = false;
            this.TTYText.Text = "";
            this.TTYText.WordWrap = false;
            // 
            // TabControl
            // 
            this.TabControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.TabControl.Controls.Add(this.TTYPage);
            this.TabControl.Controls.Add(this.MemoryPage);
            this.TabControl.Controls.Add(this.tabPage1);
            this.TabControl.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TabControl.Location = new System.Drawing.Point(0, 4);
            this.TabControl.Name = "TabControl";
            this.TabControl.SelectedIndex = 0;
            this.TabControl.Size = new System.Drawing.Size(1284, 742);
            this.TabControl.TabIndex = 1;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.ScreenCaptureBox);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(1276, 716);
            this.tabPage1.TabIndex = 2;
            this.tabPage1.Text = "Screen";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // ScreenCaptureBox
            // 
            this.ScreenCaptureBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.ScreenCaptureBox.Location = new System.Drawing.Point(0, 0);
            this.ScreenCaptureBox.Name = "ScreenCaptureBox";
            this.ScreenCaptureBox.Size = new System.Drawing.Size(1280, 720);
            this.ScreenCaptureBox.TabIndex = 0;
            this.ScreenCaptureBox.TabStop = false;
            // 
            // UnrealConsole
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(1284, 769);
            this.Controls.Add(this.StatusBar);
            this.Controls.Add(this.TabControl);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Menu = this.MainMenu;
            this.Name = "UnrealConsole";
            this.Text = "Unreal Console";
            this.Closing += new System.ComponentModel.CancelEventHandler(this.OnClosing);
            this.Load += new System.EventHandler(this.OnLoad);
            this.MemoryPage.ResumeLayout(false);
            this.TTYPage.ResumeLayout(false);
            this.TTYPage.PerformLayout();
            this.TabControl.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.ScreenCaptureBox)).EndInit();
            this.ResumeLayout(false);

        }
        #endregion

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main( string[] Args ) 
        {
	        // parse arguments

	        string TargetName = null;
	        string Platform = null;
	        // go over argument list
	        foreach (string Arg in Args)
	        {
		        if (Arg.ToLower().StartsWith("platform="))
		        {
			        Platform = Arg.Substring(9);
		        }
		        else if (Arg.ToLower().StartsWith("-platform="))
		        {
			        Platform = Arg.Substring(10);
		        }
		        // anything else if platform name
		        else
		        {
			        // make sure it's not already set
			        if (TargetName == null)
			        {
				        if (Arg.ToLower().StartsWith("target="))
				        {
					        TargetName = Arg.Substring(7);
				        }
				        else
				        {
					        TargetName = Arg;
				        }
			        }
		        }
	        }

	        UnrealConsole ConsoleApp = new UnrealConsole(Platform, TargetName);


	        while ( ConsoleApp.IsRunning )
	        {
                ConsoleApp.Tick();
                Application.DoEvents();
		        System.Threading.Thread.Sleep( 1 );
	        }
        }
		
        private void Tick( )
        {
	        if (ConnectedTargetIndex != -1)
	        {
		        LogWindow.ReceiveOutputTextFromTarget();

		        if (IsConnected())
		        {
			        if (!ProcessPackets())
			        {
				        Disconnect();
			        }
		        }
	        }
        }

        private void OnLoad(object sender, System.EventArgs e)
        {
        }

        private void OnClosing(object sender, System.ComponentModel.CancelEventArgs e)
        {
	        Disconnect();
	        IsRunning = false;
        }

        private void OnExit(object sender, System.EventArgs e)
        {
	        Disconnect();
	        IsRunning = false;
//			Application.Exit();
        }

        /// <summary>
        /// Sets the target, and if no target is given, then it will popup a dialog to choose one
        /// </summary>
        private void SetTarget(string NewPlatformName, string NewTargetName)
        {
	        // storage for whtether or not the connection dialog was canceled
	        bool bDialogWasCancelled = false;

	        // disconnect from previous target
	        if (ConnectedTargetIndex != -1)
	        {
		        DLLInterface.DisconnectFromTarget(ConnectedTargetIndex);
	        }

	        // if we received no target name, reuse last one
	        if (NewTargetName == null)
	        {
		        // if we have a a platform specified or last used platform and that platform has been connected, use it
		        if (NewPlatformName != null ||
			        (LastUsedSettings.LastPlatform != null && LastUsedSettings.LastTargets != null))
		        {
			        // if we didn't specify one, use the last used one
			        if (NewPlatformName == null)
			        {
				        NewPlatformName = LastUsedSettings.LastPlatform;
			        }

			        // get the matching index
			        int PlatformIndex = DLLInterface.GetPlatformIndex(NewPlatformName);
					
			        // validate the platform and make sure there was a last target
			        if (PlatformIndex == -1 || LastUsedSettings.LastTargets == null ||
				        PlatformIndex >= LastUsedSettings.LastTargets.Length ||
				        LastUsedSettings.LastTargets[PlatformIndex] == null)
			        {
				        NewPlatformName = null;
			        }
			        else
			        {
				        NewTargetName = LastUsedSettings.LastTargets[PlatformIndex];
			        }
		        }

		        // if we didn't get a valid target name, ask the user to pick one
		        if (NewTargetName == null)
		        {
			        NetworkConnectionDialog Dlg = new NetworkConnectionDialog();
			        if (Dlg.ShowDialog(this) == DialogResult.OK)
			        {
				        NewTargetName = Dlg.SelectedTarget.Name;
				        NewPlatformName = Dlg.SelectedTarget.Platform;
			        }
			        else
			        {
				        // remember that this was canceled
				        bDialogWasCancelled = true;
			        }
			        Dlg.Dispose();
		        }
	        }
	        // if we got a target name, but no platform, then ask each platform for that targetname
	        else if (NewPlatformName == null)
	        {
		        // go through each platform until we get one
		        for (int PlatformIndex = 0; PlatformIndex < DLLInterface.GetNumPlatforms() && PlatformName == null; PlatformIndex++)
		        {
			        // activate the platform interface
			        DLLInterface.ActivatePlatform(DLLInterface.GetPlatformName(PlatformIndex));
			        // go over all the targets for this platform
			        for (int TargetIndex = 0; TargetIndex < DLLInterface.GetNumTargets(); TargetIndex++)
			        {
				        if (NewTargetName.Equals(DLLInterface.GetTargetName(TargetIndex), StringComparison.OrdinalIgnoreCase))
				        {
					        NewPlatformName = DLLInterface.GetPlatformName(PlatformIndex);
					        break;
				        }
			        }
		        }
	        }

	        // mark us as not connected for now
	        ConnectedTargetIndex = -1;
	        if (DLLInterface.ActivatePlatform(NewPlatformName))
	        {
		        // attempt to connect to target
		        ConnectedTargetIndex = DLLInterface.ConnectToTarget(NewTargetName);
	        }
			
	        // update title bar
	        if (ConnectedTargetIndex == -1)
	        {
		        Text = AppName + ": <disconnected>";
		        if (!bDialogWasCancelled)
		        {
			        // if we failed to connect, put up the dialog again, unless we had just canceled a dialog
			        // (we use NullPlatform as the platform name so the recursion won't try to just reuse the last used platform)
			        SetTarget("NullPlatform", null);
		        }
	        }
	        else
	        {
		        // if we connected, update our saved info
		        TargetName = NewTargetName;
		        PlatformName = NewPlatformName;

		        Text = AppName + ": " + TargetName + " [" + PlatformName + "]";

		        // cache settings
		        LastUsedSettings.LastPlatform = PlatformName;
		        if (LastUsedSettings.LastTargets == null || 
			        LastUsedSettings.LastTargets.Length < DLLInterface.GetNumPlatforms())
		        {
			        LastUsedSettings.LastTargets = new string[DLLInterface.GetNumPlatforms()];
		        }
		        // get the index for the platform
		        int PlatformIndex = DLLInterface.GetPlatformIndex(PlatformName);
		        LastUsedSettings.LastTargets[PlatformIndex] = TargetName;

		        SaveUserSettings();
	        }
        }

        private String path = ".";

        private void OnOpenCapture(object sender, System.EventArgs e)
        {
	        OpenFileDialog openFileDialog1 = new OpenFileDialog();
	        openFileDialog1.InitialDirectory = path;
	        openFileDialog1.Filter = "Capture files (*.bin)|*.bin|All files (*.*)|*.*";
	        openFileDialog1.FilterIndex = 1;
	        openFileDialog1.RestoreDirectory = true;

	        if (openFileDialog1.ShowDialog() == DialogResult.OK)
	        {
		        Stream DataStream = openFileDialog1.OpenFile();
		        if ( DataStream != null )
		        {
			        // Show that we are going to be busy
			        Cursor = Cursors.WaitCursor;

			        String Filename = Path.GetFileNameWithoutExtension( openFileDialog1.FileName );
			        path = Path.GetDirectoryName( openFileDialog1.FileName );
			        Filename = path + @"\" + Filename + ".csv";

			        GPUProfiler.ConvertToExcel( DataStream, Filename );

			        DataStream.Close();
		        }
	        }
        }

        /// <summary>
        /// Creates our connection to the remote server. Creates a new file object
        /// to receive the remote data
        /// </summary>
        private bool Connect()
        {
	        // ask the DLL interface for the connected target's game IP address
	        uint IPAddrInt = DLLInterface.GetGameIPAddress(UnrealConsole.ConnectedTargetIndex);
	        if (IPAddrInt == 0)
	        {
		        MessageBox.Show("Failed to get game IP address for " + TargetName + ".");
		        Connection = null;
	        }
	        else
	        {
		        // convert int into a real IP address
		        IPAddress IPAddr = new IPAddress(IPAddrInt);
		        // make a connection with default ports
		        Connection = new ServerConnection("ConnectedTarget", UnrealNetwork.PlatformType.Unknown, IPAddr, 13500, 13502);
	        }

	        bool bOk = true;
	        if (Connection != null)
	        {
		        try
		        {
			        // Set up our in/out connections
			        Connection.Connect();
			        // Tell the server we're ready to start receiving data
			        Connection.SendConnectRequest();
		        }
		        catch (Exception e)
		        {
			        bOk = false;
			        MessageBox.Show("Failed to connect to server " + Connection.Address.ToString() +
				        "\r\nDue to: " + e.ToString());
		        }
		        if (bOk)
		        {
			        // Update the title bar
			        LogWindow.Print("Connected to [" + Connection.Name + ":" + Connection.Address.ToString() + "]\r\n");

			        // tell console to stop tracking memory
			        Connection.SendCommand("STARTTRACKING");

			        // Try to load symbols for the currently running executable
			        if (DLLInterface.LoadSymbols(ConnectedTargetIndex, null))
			        {
				        LogWindow.Print("Symbols loaded.\r\n");
			        }
			        else
			        {
				        LogWindow.Print("Failed to load symbols to look up callstack descriptions!\r\n");
			        }

			        // Update menu
//					IDM_CAPTUREGPU.Enabled = true;

			        TabControl.SelectedIndex = TTYPage.TabIndex;
		        }
		        else
		        {
			        Connection = null;
		        }
	        }
	        else
	        {
		        bOk = false;
	        }

	        return bOk;
        }

        /// <summary>
        /// Handles disconnecting from a remote server. Prompts the user
        /// </summary>
        /// <returns>True if the user chose to disconnect, false otherwise</returns>
        private void Disconnect()
        {
	        if ( IsConnected() )
	        {
		        // Update menu
//				IDM_CAPTUREGPU.Enabled = false;

		        // tell console to stop tracking memory
		        Connection.SendCommand("STOPTRACKING");

		        // Unload all symbols
		        DLLInterface.UnloadAllSymbols();

		        // Tells the server we are no longer interested in receiving packets
		        Connection.SendDisconnectRequest();
		        // Disconnect from the server
		        Connection.Disconnect();
		        LogWindow.Print( "Disconnected...\r\n" );
		        // Process any outstanding packets so that all the data is accounted for
		        ProcessPackets();
		        // Mark the connection as empty
		        Connection = null;
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
        /// Whether the last load attempt worked or not
        /// </summary>
//		private bool bLoadedOk;

        /// <summary>
        /// Logs the node information for debugging purposes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e">The info about the bad node type</param>
        protected void XmlSerializer_UnknownNode(object sender, XmlNodeEventArgs e)
        {
//			bLoadedOk = false;
	        Console.WriteLine("Unknown Node:" + e.Name + "\t" + e.Text);
        }

        /// <summary>
        /// Logs the bad attribute information for debugging purposes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e">The attribute info</param>
        protected void XmlSerializer_UnknownAttribute(object sender, XmlAttributeEventArgs e)
        {
//			bLoadedOk = false;
	        System.Xml.XmlAttribute attr = e.Attr;
	        Console.WriteLine("Unknown attribute " + attr.Name + "='" + attr.Value + "'");
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
		        XmlStream = new FileStream(Name,FileMode.OpenOrCreate,
			        FileAccess.Read,FileShare.None,256 * 1024,false);
		        // Creates an instance of the XmlSerializer class so we can
		        // read the settings object
		        XmlSerializer ObjSer = new XmlSerializer(typeof(SavedSettings));
		        // Add our callbacks for a busted XML file
		        ObjSer.UnknownNode += new XmlNodeEventHandler(XmlSerializer_UnknownNode);
		        ObjSer.UnknownAttribute += new XmlAttributeEventHandler(XmlSerializer_UnknownAttribute);
		        // Create an object graph from the XML data
		        LastUsedSettings = (SavedSettings)ObjSer.Deserialize(XmlStream);
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
		        if (LastUsedSettings == null)
		        {
			        LastUsedSettings = new SavedSettings();
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

        /// <summary>
        /// Updates the SecondPerCycle value from packet data
        /// </summary>
        /// <param name="Data">The data to read the value from</param>
        public void UpdateConversionFactor(Byte[] Data, ref int CurrentOffset)
        {
	        SecondsPerCycle = ByteStreamConverter.ToDouble(Data,ref CurrentOffset);
        }

        /// <summary>
        /// Processes any pending packets by converting them into object form
        /// </summary>
        private bool ProcessPackets( )
        {
	        bool bSuccessful = false;
	        if (IsConnected())
	        {
		        bSuccessful = true;

		        // Vars used for figuring out the type of updates needed
		        int NumPacketsProcessed = 0;
		        int NumFramesAdded = 0;

		        int MaxNumPackets = 100;
		        int PacketNum = 0;
		        // While there are packets to be processed
		        for (Packet packet = Connection.GetNextPacket();
			        packet != null && PacketNum < MaxNumPackets;
			        packet = Connection.GetNextPacket(), PacketNum++)
		        {
			        int CurrentOffset = 0;
			        while ( CurrentOffset < packet.Data.Length )
			        {
				        NumPacketsProcessed++;
				        // Get the 2 character packet code
				        string PacketType = Encoding.ASCII.GetString(packet.Data,CurrentOffset,2);
				        CurrentOffset += 2;

				        // Route to the correct class/handler based upon packet type
				        switch (PacketType)
				        {
					        // SD == Server disconnected
					        case "SD":
						        bSuccessful = false;
						        break;

					        // ST = Server Transmission
					        case "ST":
					        {
						        int NumChunks = 1;// ByteStreamConverter.ToInt(packet.Data, ref CurrentOffset);
						        for (int Chunk = 0; Chunk < NumChunks; Chunk++)
						        {
							        string Channel = ByteStreamConverter.ToString(packet.Data, ref CurrentOffset);
							        int Size = ByteStreamConverter.ToInt(packet.Data, ref CurrentOffset);
							        ProcessServerTransmission(Channel, packet.Data, CurrentOffset, Size);
							        CurrentOffset += Size;
						        }
						        break;
					        }

					        // Ux == Update an existing stat
					        case "UC":
					        case "UD":
					        case "UF":
						        break;

					        // GD == Group description
					        case "GD":
						        break;

					        // PC == Sends the value of Seconds Per Cycle so that we
					        // can properly convert from cycles to ms
					        case "PC":
					        {
						        UpdateConversionFactor(packet.Data, ref CurrentOffset);
						        break;
					        }

					        // NF == New frame
					        case "NF":
					        {
						        NumFramesAdded++;
						        break;
					        }

					        default:
					        {
						        Console.WriteLine("Unknown packet type " + PacketType);
						        CurrentOffset = packet.Data.Length;
						        break;
					        }
				        }
			        }
		        }
		        // Don't do the updating of data unless needed
		        if (NumPacketsProcessed > 0)
		        {
		        }
	        }
	        return bSuccessful;
        }

        private bool				IsRunning = true;
        private FLogWindow			LogWindow;
        private FMemoryTracker		MemProfiler;
        private FGPUProfiler		GPUProfiler;
        private string				AppName;
        private SavedSettings		LastUsedSettings;
        public double				SecondsPerCycle = 0.0001;
        private ServerConnection	Connection;

        private void OnTTYInputKey(object sender, System.Windows.Forms.KeyPressEventArgs e)
        {
	        string Command = LogWindow.ProcessKeyInput( e );
	        if ( Command != null && IsConnected() )
	        {
		        Connection.SendCommand( Command );
	        }
        }

        private void OnTYYInputKeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
        {
	        LogWindow.ProcessKeyDown( e );
        }

        private void OnTTYInputChanged(object sender, System.EventArgs e)
        {
	        LogWindow.ProcessInputChanged();
        }

        private void IDM_OnClearWindow_Click(object sender, System.EventArgs e)
        {
	        LogWindow.ClearOutput();
        }

        private void ProcessServerTransmission( string Channel, Byte[] Data, int Offset, int Size )
        {
	        switch ( Channel )
	        {
		        case "LOG":		// OutputDebugString()
		        {
			        string Message = Encoding.ASCII.GetString(Data, Offset, Size);
			        LogWindow.Print( Message );
			        break;
		        }
		        case "MEM":
		        {
			        string Message = Encoding.ASCII.GetString(Data, Offset, Size);
			        MemProfiler.ProcessMemoryTrackingCommands(Message);
			        break;
		        }
	        }
        }

        private void IDM_CONNECT_Click(object sender, EventArgs e)
        {
	        // change targets with the popup dialog ("NullPlatform" to make sure the dialog pops up)
	        SetTarget("NullPlatform", null);
        }

        private void OnCaptureGPU(object sender, System.EventArgs e)
        {
	        //@TODO
        }

        private void OnStartMemoryCapture(object sender, System.EventArgs e)
        {
	        // connect to game IP
	        if (Connect())
	        {
		        IDM_STARTMEMORYCAPTURE.Enabled = false;
		        IDM_STOPMEMORYCAPTURE.Enabled = true;
		        TabControl.SelectedIndex = MemoryPage.TabIndex;
	        }
        }

        private void OnStopMemoryCapture(object sender, System.EventArgs e)
        {
	        IDM_STARTMEMORYCAPTURE.Enabled = true;
	        IDM_STOPMEMORYCAPTURE.Enabled = false;
	        MemProfiler.DumpMemoryTracking(0);

	        Disconnect();
        }

        private void OnClearWindow(object sender, System.EventArgs e)
        {
	        switch ( TabControl.SelectedIndex )
	        {
		        case 0:
			        LogWindow.ClearOutput();
			        break;

		        case 1:
			        MemProfiler.ClearOutput();
			        break;
	        }
        }

        private void OnClearMemoryCapture(object sender, System.EventArgs e)
        {
	        MemProfiler.ClearCaptureData();
        }

        private void IDM_WORDWRAP_Click(object sender, EventArgs e)
        {
	        IDM_WORDWRAP.Checked = LogWindow.ToggleWordWrap();
        }

        private void IDM_REBOOT_Click(object sender, EventArgs e)
        {
	        if (ConnectedTargetIndex != -1)
	        {
		        DLLInterface.Reboot(ConnectedTargetIndex);
	        }
        }

        private void IDM_SCREENCAPTURE_Click(object sender, EventArgs e)
        {
	        // make DLL grab the screenshot
	        if (!DLLInterface.ScreenshotBMP(ConnectedTargetIndex, "TempScreenshot.bmp"))
	        {
		        MessageBox.Show("Failed to take a screenshot.");
		        return;
	        }

	        // load the screenshot image
	        Image Screenshot = Image.FromFile("TempScreenshot.bmp");

	        SaveFileDialog Dialog = new SaveFileDialog();
	        Dialog.DefaultExt = "jpg";
	        Dialog.Filter = "JPEG Files (*.jpg)|*.jpg";

	        // remember current directory
	        string CurrentDir = Directory.GetCurrentDirectory();
			
	        if (Dialog.ShowDialog(this) == DialogResult.OK && Dialog.FileName.Length > 0)
	        {
		        LogWindow.Print(" ... saving screenshot as \"" + Dialog.FileName + "\"\r\n");
		        Screenshot.Save(Dialog.FileName, ImageFormat.Jpeg);
	        }

	        // restore current directory
	        Directory.SetCurrentDirectory(CurrentDir);
        }

        private void IDM_SAVE_Click(object sender, EventArgs e)
        {
	        // save the log window to a text file
	        SaveFileDialog Dialog = new SaveFileDialog();
	        Dialog.DefaultExt = "txt";
	        Dialog.FileName = TargetName + "_log.txt";
	        Dialog.Filter = "Log files (*.txt;*.log)|*.txt;*.log|All files (*.*)|*.*";

	        if (Dialog.ShowDialog(this) == DialogResult.OK && Dialog.FileName.Length > 0)
	        {
		        // write the string to a file
		        string Munged = TTYText.Text.Replace("\n", "\r\n");
		        File.WriteAllText(Dialog.FileName, Munged);
	        }
        }

        private void IDM_UPDATESCREENTAB_Click(object sender, EventArgs e)
        {
            // Dispose previous one to clear file handle.
            if (ScreenCaptureBox.Image != null)
            {
                ScreenCaptureBox.Image.Dispose();
                ScreenCaptureBox.Image = null;
            }
            
            // make DLL grab the screenshot
            if (!DLLInterface.ScreenshotBMP(ConnectedTargetIndex, "TempScreenCapture.bmp"))
            {
                MessageBox.Show("Failed to take a screenshot.");
                return;
            }

            // load the screenshot image
            ScreenCaptureBox.Image = Image.FromFile("TempScreenCapture.bmp");
        }
    }
}
