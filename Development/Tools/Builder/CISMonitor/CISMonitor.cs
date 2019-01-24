/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Data.SqlClient;
using System.Deployment.Application;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.ServiceProcess;
using System.Text;
using System.Threading;
using System.Xml;
using System.Xml.Serialization;
using System.Windows.Forms;
using P4API;

namespace CISMonitor
{
    public partial class CISMonitor : Form
    {
        public enum SubmitState
        {
            CISUnknown,
            CISSQLServerDown,
            CISBadFailed,
            CISBadPending,
			CISBadSuccessful,
			CISGoodFailed,
			CISGoodPending,
            CISGoodSuccessful
        };

        public bool Ticking = false;
		public bool Restart = false;
		public SettableOptions Options = null;

        private P4Connection IP4Net = null;
        private System.Drawing.Icon GoodGoodIcon = null;
        private System.Drawing.Icon GoodPendingIcon = null;
        private System.Drawing.Icon GoodBadIcon = null;
        private System.Drawing.Icon BadGoodIcon = null;
        private System.Drawing.Icon BadPendingIcon = null;
        private System.Drawing.Icon BadBadIcon = null; 
		private DateTime LastCheckTime = new DateTime( 2000, 1, 1 );
        private bool LoggedIn = false;
		private bool OverallBuildsAreGood = false;
		private SubmitState OverallState = SubmitState.CISUnknown;
		private int OverallNumPendingChangelists = 0;
		private bool BalloonTickActive = false;

        public CISMonitor()
        {
        }

		private string GetOptionsFileName()
		{
			string BaseDirectory = Application.StartupPath;
			if( ApplicationDeployment.IsNetworkDeployed )
			{
				BaseDirectory = ApplicationDeployment.CurrentDeployment.DataDirectory;
			}

			return( Path.Combine( BaseDirectory, "CISMonitor.Options.xml" ) );
		}

        public void Init( string[] args )
        {
			Options = UnrealControls.XmlHandler.ReadXml<SettableOptions>( GetOptionsFileName() );

            // Init and precache the UI elements
            InitializeComponent();
            ConfigurationGrid.SelectedObject = Options;

            System.Version Version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
            DateTime CompileDateTime = DateTime.Parse( "01/01/2000" ).AddDays( Version.Build ).AddSeconds( Version.Revision * 2 );
            VersionText.Text = "Compiled: " + CompileDateTime.ToString( "d MMM yyyy HH:mm" ) + " (.NET)";

            // Create an interface to Perforce
			IP4Net = new P4Connection();

            GoodGoodIcon = Resources.green_green;
            GoodPendingIcon = Resources.green_yellow;
            GoodBadIcon = Resources.green_red;
            BadGoodIcon = Resources.red_green;
            BadPendingIcon = Resources.red_yellow;
            BadBadIcon = Resources.red_red;

            Ticking = true;

			// Work out the potential branches we are interested in
			PopulateBranchConfig();

            // Ping the SQL server initially to ensure we have connectivity
			GetSQLChangeLists();
        }

        public void Destroy()
        {
            Dispose();

			UnrealControls.XmlHandler.WriteXml<SettableOptions>( Options, GetOptionsFileName(), "" );
        }

        public void Run()
        {
            // Poll periodically
#if DEBUG
            TimeSpan Interval = new TimeSpan( 0, 0, 10 );
#else
			int Randomiser = new Random().Next( 45 );
			TimeSpan Interval = new TimeSpan( 0, 0, Options.PollingInterval + Randomiser );
#endif
			if( DateTime.UtcNow < LastCheckTime + Interval )
            {
                return;
            }
			LastCheckTime = DateTime.UtcNow;

            // Update the status
            try
            {
				// Grab the latest changelist states from the db
				if( GetSQLChangeLists() != "OK" )
                {
                    OverallState = SubmitState.CISSQLServerDown;
                    UpdateTrayIcon( false );
                }
				// Check to see if the db has seen any new changelists
				else
				{
					foreach( BranchConfig Branch in Options.BranchesToMonitor )
					{
						// Only interested in branches we need to monitor with changelists that have changed
						if( Branch.Monitor && Branch.Dirty )
						{
							Branch.Dirty = false;

							// Make sure we are logged in
							if( !UpdateLoginState() )
							{
								Branch.CurrentState = SubmitState.CISUnknown;
								UpdateTrayIcon( false );
							}
							else
							{
								// No longer monitor if CIS has been disabled
								if( Branch.LastAttemptedChangelist < 0 )
								{
									Branch.Monitor = false;
								}
								else
								{
									// Set the appropriate tooltip
									if( Branch.LastGoodChangelist > Branch.LastFailChangelist )
									{
										BuildState.Text = "There are no current CIS failures.";
										Branch.BuildIsGood = true;
									}
									else
									{
										BuildState.Text = "CIS is currently failing!";
										Branch.BuildIsGood = false;
									}

									// Exit if we aren't logged in, it will retry on the next loop
									if( !LoggedIn )
									{
										return;
									}

									// Check for changelists that haven't passed CIS
									P4RecordSet P4Output = GetPerforceChangelists( "//depot/" + Branch.Name + "/...@" + ( Branch.LastGoodChangelist + 1 ).ToString() + ",#head", true );

									// Calculate and merge results
									Branch.CurrentState = CheckSubmitState( Branch, P4Output );
								}
							}
						}
					}

					// Find the overall state of the build
					SubmitState OldOverallState = OverallState;
					OverallState = SubmitState.CISGoodSuccessful;
					OverallNumPendingChangelists = 0;

					foreach( BranchConfig Branch in Options.BranchesToMonitor )
					{
						if( Branch.Monitor )
						{
							if( Branch.CurrentState < OverallState )
							{
								OverallState = Branch.CurrentState;
								OverallNumPendingChangelists = Branch.NumPendingChangelists;
							}
						}
					}

					OverallBuildsAreGood = ( OverallState != SubmitState.CISBadSuccessful )
											&& ( OverallState != SubmitState.CISBadPending )
											&& ( OverallState != SubmitState.CISBadFailed );

					// Update the UI
					if( OverallState != OldOverallState )
					{
						UpdateTrayIcon( false );
					}
				}
            }
            catch( Exception Ex )
            {
                Debug.WriteLine( Ex.Message );
            }
        }

		private List<string> GetBranches( SqlConnection Connection, string Query )
		{
			List<string> Result = new List<string>();

			try
			{
				using( SqlCommand Command = new SqlCommand( Query, Connection ) )
				{
					SqlDataReader DataReader = Command.ExecuteReader();
					while( DataReader.Read() )
					{
						Result.Add( DataReader.GetString( 0 ) );
					}

					DataReader.Close();
				}
			}
			catch
			{
			}

			return ( Result );
		}

		private int GetJobCount( string Changelist, string BranchName )
		{
			// Get the number of jobs that were submitted to CIS for this changelist
			Int32 Result = 0;
			using( SqlConnection Connection = new SqlConnection( Properties.Settings.Default.DBConnection ) )
			{
				Connection.Open();
				try
				{
					string CISJobCountQuery =
						"SELECT Count(ID) FROM Jobs WHERE ( PrimaryBuild = 0 ) AND ( Killing = 0 ) " +
						"AND ( Label = '" + Changelist + "' ) AND ( Branch = '" + BranchName + "' )";

					using( SqlCommand Command = new SqlCommand( CISJobCountQuery, Connection ) )
					{
						SqlDataReader DataReader = Command.ExecuteReader();
						if( DataReader.Read() )
						{
							Result = DataReader.GetInt32( 0 );
						}
						DataReader.Close();
					}
				}
				catch
				{
				}
				Connection.Close();
			}
			return ( Result );
		}

		public void PopulateBranchConfig()
		{
#if false
			Options.BranchesToMonitor = new List<BranchConfig>();
			Options.BranchesToMonitor.Add( new BranchConfig( "UnrealEngine3", true ) );
#else
			bool NewSettings = false;
			if( Options.BranchesToMonitor == null )
			{
				Options.BranchesToMonitor = new List<BranchConfig>();
				NewSettings = true;
			}

			List<string> Branches = new List<string>();

			// Get the branches that we could potentially be monitoring
			using( SqlConnection Connection = new SqlConnection( Properties.Settings.Default.DBConnection ) )
			{
				Connection.Open();
				string Query = "SELECT Branch FROM BranchConfig WHERE LastAttemptedOverall > 0";
				Branches = GetBranches( Connection, Query );

				Connection.Close();
			}

			// If the branch doesn't exist, add it with default values
			foreach( string Branch in Branches )
			{
				bool AlreadyMonitoring = false;
				foreach( BranchConfig BranchInfo in Options.BranchesToMonitor )
				{
					if( Branch == BranchInfo.Name )
					{
						AlreadyMonitoring = true;
						BranchInfo.HasCIS = true;
						break;
					}
				}

				if( !AlreadyMonitoring )
				{
					// Default monitoring the main branch to on, and other branches to off
					bool DefaultMonitor = false;
					if( NewSettings && Branch == "UnrealEngine3" )
					{
						DefaultMonitor = true;
					}

					Options.BranchesToMonitor.Add( new BranchConfig( Branch, DefaultMonitor ) );
				}
			}

			// Clear out any stale branches
			bool BranchesToDelete = true;
			while( BranchesToDelete )
			{
				BranchesToDelete = false;
				foreach( BranchConfig BranchInfo in Options.BranchesToMonitor )
				{
					if( !BranchInfo.HasCIS )
					{
						Options.BranchesToMonitor.Remove( BranchInfo );
						BranchesToDelete = true;
						break;
					}
				}
			}

			// Associate branches with tool strip menu items
			Queue<ToolStripMenuItem> MenuItemBranches = new Queue<ToolStripMenuItem>();
			MenuItemBranches.Enqueue( ToolStripMenuItemBuildStatus1 );
			MenuItemBranches.Enqueue( ToolStripMenuItemBuildStatus2 );
			MenuItemBranches.Enqueue( ToolStripMenuItemBuildStatus3 );
			MenuItemBranches.Enqueue( ToolStripMenuItemBuildStatus4 );
			MenuItemBranches.Enqueue( ToolStripMenuItemBuildStatus5 );

			foreach( BranchConfig Branch in Options.BranchesToMonitor )
			{
				if( Branch.Monitor )
				{
					ToolStripMenuItem MenuItem = MenuItemBranches.Dequeue();
					MenuItem.Visible = true;
					MenuItem.Text = "Build Status (" + Branch.Name + ")";
					MenuItem.Tag = Branch.Name;
				}
			}

			foreach( ToolStripMenuItem MenuItem in MenuItemBranches )
			{
				MenuItem.Visible = false;
			}
#endif
		}

		private int GetChangeList( SqlConnection Connection, string Query )
		{
			int Result = -1;

			try
			{
				using( SqlCommand Command = new SqlCommand( Query, Connection ) )
				{
					SqlDataReader DataReader = Command.ExecuteReader();
					if( DataReader.Read() )
					{
						Result = DataReader.GetInt32( 0 );
					}

					DataReader.Close();
				}
			}
			catch
			{
			}

			return ( Result );
		}

        // Get the relevant changelists of the CIS build in question
        public string GetSQLChangeLists()
        {
			string SqlState = "OK";
            int Result;

			try
			{
				using( SqlConnection Connection = new SqlConnection( Properties.Settings.Default.DBConnection ) )
				{
					Connection.Open();

					foreach( BranchConfig Branch in Options.BranchesToMonitor )
					{
						if( Branch.Monitor )
						{
							Result = GetChangeList( Connection, "SELECT LastAttemptedOverall FROM BranchConfig WHERE ( Branch = '" + Branch.Name + "' )" );
							if( Result > 0 )
							{
								Branch.Dirty |= ( Branch.LastAttemptedChangelist != Result );
								Branch.LastAttemptedChangelist = Result;
							}

							Result = GetChangeList( Connection, "SELECT LastGoodOverall FROM BranchConfig WHERE ( Branch = '" + Branch.Name + "' )" );
							if( Result > 0 )
							{
								Branch.Dirty |= ( Branch.LastGoodChangelist != Result );
								Branch.LastGoodChangelist = Result;
							}

							Result = GetChangeList( Connection, "SELECT LastFailOverall FROM BranchConfig WHERE ( Branch = '" + Branch.Name + "' )" );
							if( Result > 0 )
							{
								Branch.Dirty |= ( Branch.LastFailChangelist != Result );
								Branch.LastFailChangelist = Result;
							}
						}
					}

					Connection.Close();
				}
			}
			catch( Exception Ex )
			{
				SqlState = Ex.ToString();
			}

            return ( SqlState );
        }

        // Work out if we have any changelists between last good CIS build and head
        private SubmitState CheckSubmitState( BranchConfig Branch, P4RecordSet P4Output )
        {
			if( P4Output == null )
			{
				return ( Branch.CurrentState );
			}

			Branch.NumPendingChangelists = P4Output.Records.Length;

			bool bHaveChangesInRange = (P4Output.Records.Length != 0);
			if( !bHaveChangesInRange )
			{
				// We don't have any changes in the range, done
				return ( Branch.BuildIsGood ? SubmitState.CISGoodSuccessful : SubmitState.CISBadSuccessful );
			}

			// Make sure we have changes in the range that were submitted to CIS
			bool bHaveCISChangesInRange = false;
			foreach( P4Record Record in P4Output )
			{
				string Changelist = Record.Fields["change"];
				if( GetJobCount( Changelist, Branch.Name ) > 0 )
				{
					bHaveCISChangesInRange = true;
					break;
				}
			}

			if( !bHaveCISChangesInRange )
			{
				// We don't have any CIS changes in the range, done
				return ( Branch.BuildIsGood ? SubmitState.CISGoodSuccessful : SubmitState.CISBadSuccessful );
			}

			if( Branch.BuildIsGood )
            {
                // Build is good, but our changes aren't done
				return SubmitState.CISGoodPending;
            }
            else
            {
                // Build is bad, so check the changelists between the last good and the last failed
				P4RecordSet P4OutputBlamer = GetPerforceChangelists( "//depot/" + Branch.Name + "/...@" + ( Branch.LastGoodChangelist + 1 ).ToString() + ",@" + Branch.LastFailChangelist.ToString(), true );

				if( P4OutputBlamer == null || P4OutputBlamer.Records.Length > 0 )
				{
					// We have changes in the failing range, so we're on the hook
					return SubmitState.CISBadFailed;
				}
            }

			// Build is bad, but not all of our changes are done processing yet
			return SubmitState.CISBadPending;
        }

		private void DisplayPerforceError( string Message )
		{
#if DEBUG
			Debug.WriteLine( Message );
#endif
			if( Message.Contains( "Access for user" ) )
			{
				System.Windows.Forms.MessageBox.Show( "Perforce user name \"" + IP4Net.User + "\" not recognized by server. Contact your Perforce administrator.",
													  "CIS Monitor: Perforce Connection Error",
													  System.Windows.Forms.MessageBoxButtons.OK,
													  System.Windows.Forms.MessageBoxIcon.Error );
			}
			else if( Message.Contains( "Perforce password" ) )
			{
				System.Windows.Forms.MessageBox.Show( "Incorrect Password for \"" + IP4Net.User + "\". Contact your Perforce administrator.",
													  "CIS Monitor: Perforce Connection Error",
													  System.Windows.Forms.MessageBoxButtons.OK,
													  System.Windows.Forms.MessageBoxIcon.Error );
			}
			else if( Message.Contains( "Unable to connect to the Perforce Server!" ) )
			{
				System.Windows.Forms.MessageBox.Show( "Unable to connect user \"" + IP4Net.User + "\" to the default Perforce Server \"" + IP4Net.Port.ToString() + "\". Contact your Perforce administrator.",
													  "CIS Monitor: Perforce Connection Error",
													  System.Windows.Forms.MessageBoxButtons.OK,
													  System.Windows.Forms.MessageBoxIcon.Error );
			}

			LoggedIn = false;
		}

        private bool UpdateLoginState()
        {
            if( !LoggedIn )
            {
                P4RecordSet Output;
                try
                {
                    // Try to auto login
                    IP4Net.CWD = "";
                    IP4Net.ExceptionLevel = P4API.P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

                    Output = IP4Net.Run( "login", "-s" );

                    LoggedIn = true;
                }
                catch( Exception Ex )
                {
                    DisplayPerforceError( Ex.Message );
                }
            }

            return ( LoggedIn );
        }

        private string[] GetChangelistDescriptions()
        {
            P4RecordSet DescribeOutput = null;
            List<string> CollatedOutput = new List<string>();

			foreach( BranchConfig Branch in Options.BranchesToMonitor )
			{
				if( Branch.Monitor )
				{
					P4RecordSet Output = GetPerforceChangelists( "//depot/" + Branch.Name + "/...@" + ( Branch.LastGoodChangelist + 1 ).ToString() + ",@" + Branch.LastFailChangelist, false );
					if( Output != null )
					{
						IP4Net.Connect();

						foreach( P4Record Record in Output )
						{
							string Changelist = Record.Fields["change"];

							try
							{
								IP4Net.CWD = "";
								IP4Net.ExceptionLevel = P4API.P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

								DescribeOutput = IP4Net.Run( "describe", Changelist );
							}
							catch
							{
							}

							if( DescribeOutput != null )
							{
								if( DescribeOutput[0].Fields["user"] != "build_machine" )
								{
									CollatedOutput.Add( Environment.NewLine );
									CollatedOutput.Add( "Changelist: " + Changelist + " by " + DescribeOutput[0].Fields["user"] + Environment.NewLine );

									CollatedOutput.Add( DescribeOutput[0].Fields["desc"] + Environment.NewLine );
									CollatedOutput.Add( "Files affected:" + Environment.NewLine );

									foreach( string DepotFile in DescribeOutput[0].ArrayFields["depotFile"] )
									{
										CollatedOutput.Add( "    " + DepotFile + Environment.NewLine );
									}
								}
							}
						}

						IP4Net.Disconnect();
					}
				}
			}

            return ( CollatedOutput.ToArray() );
        }

		private P4RecordSet GetPerforceChangelists( string RevisionRange, bool bUserSpecific )
        {
			P4RecordSet Output = null;

            IP4Net.Connect();
            try
            {
                IP4Net.CWD = "";
				IP4Net.ExceptionLevel = P4API.P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

				if( bUserSpecific )
				{
					Output = IP4Net.Run( "changes", "-u", IP4Net.User, "-L", RevisionRange );
				}
				else
				{
					Output = IP4Net.Run( "changes", "-L", RevisionRange );
				}
            }
            catch( Exception Ex )
            {
				DisplayPerforceError( Ex.Message );
				OverallState = SubmitState.CISUnknown;
			}

            IP4Net.Disconnect();
            return ( Output );
        }

        private void AppendChangelists()
        {
			foreach( BranchConfig Branch in Options.BranchesToMonitor )
			{
				if( Branch.Monitor )
				{
					P4RecordSet P4Output = GetPerforceChangelists( "//depot/" + Branch.Name + "/...@" + ( Branch.LastGoodChangelist + 1 ).ToString() + ",@" + Branch.LastFailChangelist, true );
					if( P4Output != null )
					{
						// Construct build report
						BuildState.BalloonTipText += Environment.NewLine + Environment.NewLine;

						foreach( P4Record Record in P4Output )
						{
							string Changelist = Record.Fields["change"];
							string Description = Record.Fields["desc"];

							Description = Description.Replace( '\n', ' ' );

							// Clamp to the max length
							if( Description.Length > 48 )
							{
								Description = Description.Substring( 0, 48 );
								// Let the user know the line is truncated
								Description += "...";
							}

							// Append to the balloon text
							BuildState.BalloonTipText += Changelist + ": " + Description + Environment.NewLine;
						}
					}
				}
			}
        }

        private void UpdateTrayIcon( bool Force )
        {
            bool ShowBalloon = true;

            switch( OverallState )  
            {
                case SubmitState.CISBadSuccessful:
					BuildState.Icon = BadGoodIcon;
					BuildState.BalloonTipTitle = "CIS has detected the build is BAD!";
					BuildState.BalloonTipText = "You have no pending changelists";
					BuildState.BalloonTipIcon = ToolTipIcon.Info;
					ShowBalloon = Options.ShowSuccessBalloon;
					break;

                case SubmitState.CISGoodSuccessful:
                    BuildState.Icon = GoodGoodIcon;
					BuildState.BalloonTipTitle = "CIS has verified the build is GOOD!";
					BuildState.BalloonTipText = "You have no pending changelists";
                    BuildState.BalloonTipIcon = ToolTipIcon.Info;
                    ShowBalloon = Options.ShowSuccessBalloon;
                    break;

                case SubmitState.CISBadPending:
                case SubmitState.CISGoodPending:
					BuildState.Icon = OverallBuildsAreGood ? GoodPendingIcon : BadPendingIcon;
                    BuildState.BalloonTipTitle = "CIS is processing your changelists...";
					BuildState.BalloonTipText = "You have " + OverallNumPendingChangelists.ToString() + " changelists waiting to be verified.";
                    BuildState.BalloonTipIcon = ToolTipIcon.Info;
                    ShowBalloon = Options.ShowPendingBalloon;
                    break;

                case SubmitState.CISBadFailed:
                case SubmitState.CISGoodFailed:
					BuildState.Icon = OverallBuildsAreGood ? GoodBadIcon : BadBadIcon;
                    BuildState.BalloonTipTitle = "CIS is failing, and it could be due to your changelists";
					BuildState.BalloonTipText = "You have " + OverallNumPendingChangelists.ToString() + " changelists that are part of a failing build.";
                    AppendChangelists();
                    BuildState.BalloonTipIcon = ToolTipIcon.Error;

                    if( Options.PlaySound )
                    {
                        System.Media.SystemSounds.Exclamation.Play();
                    }
                    break;

                case SubmitState.CISUnknown:
                    BuildState.Icon = GoodPendingIcon;
                    // the P4PASSWD is not set and there is no valid ticket
                    BuildState.BalloonTipTitle = "You are not logged in to the Perforce server...";
                    BuildState.BalloonTipText = "Please login using p4win or p4v.";
                    break;

                case SubmitState.CISSQLServerDown:
                    BuildState.Icon = GoodPendingIcon;
                    BuildState.BalloonTipTitle = "The SQL server is not responding...";
                    BuildState.BalloonTipText = "Please wait while retrying.";
                    break;
            }

            if( Force || ShowBalloon )
            {
                BuildState.ShowBalloonTip( 1000 );
            }
        }

        private void ShowChangelists( object sender, MouseEventArgs e )
        {
            if( !BalloonTickActive && e.Button == MouseButtons.Left )
            {
                UpdateTrayIcon( true );
            }
			else if( e.Button == MouseButtons.Right )
			{
				ToolStripMenuItemFailingChanges.Enabled = !OverallBuildsAreGood;
			}
        }

        private void BalloonTipShown( object sender, EventArgs e )
        {
            BalloonTickActive = true;
        }

        private void BalloonTipClosed( object sender, EventArgs e )
        {
            BalloonTickActive = false;
        }

        private void BalloonTipClicked( object sender, EventArgs e )
        {
            BalloonTickActive = false;
			if( !OverallBuildsAreGood )
            {
				string[] Output = GetChangelistDescriptions();

				new ChangeDetails( Options, Output );
            }
        }

		private void MenuItemFailingChangesClick( object sender, EventArgs e )
		{
			if( !OverallBuildsAreGood )
			{
				string[] Output = GetChangelistDescriptions();

				new ChangeDetails( Options, Output );
			}
		}

        private void ExitMenuItemClicked( object sender, EventArgs e )
        {
            Ticking = false;
        }

        private void MenuItemStatusClick( object sender, EventArgs e )
        {
            UpdateTrayIcon( true );
        }

        private void MenuItemConfigureClick( object sender, EventArgs e )
        {
            Show();
        }

        private void ButtonOKClick( object sender, EventArgs e )
        {
            Hide();
			PopulateBranchConfig();

			UnrealControls.XmlHandler.WriteXml<SettableOptions>( Options, GetOptionsFileName(), "" );
        }

        private void MenuItemBuildStatusClick( object sender, EventArgs e )
        {
			ToolStripMenuItem MenuItem = ( ToolStripMenuItem )sender;
			string BranchName = ( string )MenuItem.Tag;
			
			Process.Start( "http://bob/buildStatus.aspx?BranchName=" + BranchName );
        }
    }

	public class BranchConfig
	{
		[XmlIgnore]
		public bool Dirty = false;
		[XmlIgnore]
		public bool HasCIS = false;
		[XmlIgnore]
		public bool BuildIsGood = false;
		[XmlIgnore]
		public CISMonitor.SubmitState CurrentState = CISMonitor.SubmitState.CISUnknown;
		[XmlIgnore]
		public int NumPendingChangelists = 0;

		[CategoryAttribute( "Configuration" )]
		[ReadOnlyAttribute( true )]
		public string Name { get; set; }

		[CategoryAttribute( "Configuration" )]
		public bool Monitor { get; set; }

		[CategoryAttribute( "Information" )]
		[ReadOnlyAttribute( true )]
		[XmlIgnore]
		public int LastAttemptedChangelist { get; set; }

		[CategoryAttribute( "Information" )]
		[ReadOnlyAttribute( true )]
		[XmlIgnore]
		public int LastGoodChangelist { get; set; }

		[CategoryAttribute( "Information" )]
		[ReadOnlyAttribute( true )]
		[XmlIgnore]
		public int LastFailChangelist { get; set; }

		public BranchConfig()
		{
			Name = "";
			Monitor = false;
			LastAttemptedChangelist = -1;
			LastGoodChangelist = -1;
			LastFailChangelist = -1;
		}

		public BranchConfig( string InName, bool InMonitor )
		{
			Name = InName;
			Monitor = InMonitor;
			HasCIS = true;
		}
	}

    public class SettableOptions
    {
        [CategoryAttribute( "Preferences" )]
        [DescriptionAttribute( "Shows the tip balloon when your changelists pass CIS." )]
		[XmlElement]
		public bool ShowSuccessBalloon { get; set; }

        [CategoryAttribute( "Preferences" )]
        [DescriptionAttribute( "Shows the tip balloon when unverified changelists are detected." )]
		[XmlElement]
		public bool ShowPendingBalloon { get; set; }

        [CategoryAttribute( "Preferences" )]
        [DescriptionAttribute( "Play a sound when the build starts failing." )]
		[XmlElement]
		public bool PlaySound { get; set; }

		[CategoryAttribute( "Branches" )]
		[DescriptionAttribute( "List of branches to monitor for CIS results." )]
		[XmlElement]
		public List<BranchConfig> BranchesToMonitor { get; set; }

        [CategoryAttribute( "Preferences" )]
        [DescriptionAttribute( "Polling interval in seconds (default is 120, with clamps at 60 and 300)." )]
		[XmlElement]
		public int PollingInterval
        {
            get 
            { 
                return ( LocalPollingInterval ); 
            }
            set
            {
                if( value < 60 )
                {
                    value = 60;
                }
                else if( value > 300 )
                {
                    value = 300;
                }
                LocalPollingInterval = value; 
            }
        }
        [XmlAttribute]
        private int LocalPollingInterval = 120;
    }
}
