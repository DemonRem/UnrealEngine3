using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Management;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Forms;

namespace Controller
{
    enum MODES
    {
        Init,
        Monitor,
        Finalise,
        Exit,
    }

    enum COMMANDS
    {
        Error,
        Config,
        SCC_Sync,
        SCC_Checkout,
        SCC_Submit,
        SCC_Revert,
        SCC_GetChanges,
        MSVCClean,
        MSVCBuild,
        GCCBuild,
        BuildScript,
        Cook,
        Submit,
        Finished,

        MSVCFull,       // Composite command - clean then build
    }

    enum ERRORS
    {
        None,
        NoScript,
        IllegalCommand,
        SCC_Sync,
        SCC_Checkout,
        SCC_Submit,
        SCC_Revert,
        SCC_GetChanges,
        SCC_GetClientRoot,
        Process,
        TimedOut,
    }
    
    public partial class Main : Form
    {
        BuilderDB DB = null;
        Emailer Mailer = null;
        P4 SCC = null;

        public bool Ticking = true;
        public string MachineName = "";
        private string LoggedOnUser = "";
        private DateTime LastPingTime = DateTime.Now;

        private int BuilderID = 0;
        private int BuildLogID = 0;
        private Command CurrentCommand = null;
        private Queue<COMMANDS> PendingCommands = new Queue<COMMANDS>();
        private ScriptParser Builder = null;
        private MODES Mode = 0;
        private string FinalStatus = "";

        delegate void DelegateAddLine( string Line, Color TextColor );

        private void EnsureDirectoryExists( string Path )
        {
            DirectoryInfo Dir = new DirectoryInfo( Path );
            if( !Dir.Exists )
            {
                Dir.Create();
            }
        }

        private void GetInfo()
        {
            ManagementObjectSearcher Searcher = new ManagementObjectSearcher( "Select * from Win32_ComputerSystem" );
            ManagementObjectCollection Collection = Searcher.Get();

            foreach( ManagementObject Object in Collection )
            {
                Object Value;

                Value = Object.GetPropertyValue( "UserName" );
                LoggedOnUser = Value.ToString();

                Value = Object.GetPropertyValue( "Name" );
                MachineName = Value.ToString();

                Log( "Welcome \"" + LoggedOnUser + "\" running on \"" + MachineName + "\"", Color.Blue );
                break;
            }
        }

        private void BroadcastMachine()
        {
            string Query, Command;
            int ID;

            Query = "SELECT ID FROM Builders WHERE ( Machine ='" + MachineName + "' AND State != 'Dead' AND State != 'Zombied' )";
            ID = DB.ReadInt( Query );
            while( ID != 0 )
            {
                Command = "UPDATE Builders SET State = 'Zombied' WHERE ( ID = " + ID.ToString() + " )";
                DB.Update( Command );

                Log( "Cleared zombie connection " + ID.ToString() + " from database", Color.Orange );
                ID = DB.ReadInt( Query );
            }

            Log( "Registering \'" + MachineName + "\' with database", Color.Blue );

            // Insert machine as new column
            Command = "INSERT INTO Builders( Machine ) VALUES ( '" + MachineName + "' )";
            DB.Update( Command );

            Query = "SELECT ID FROM Builders WHERE ( Machine = '" + MachineName + "' AND State is NULL )";
            ID = DB.ReadInt( Query );
            if( ID != 0 )
            {
                Command = "UPDATE Builders SET State = 'Connected' WHERE ( ID = " + ID.ToString() + " )";
                DB.Update( Command );

                Command = "UPDATE Builders SET StartTime = '" + DateTime.Now.ToString() + "' WHERE ( ID = " + ID.ToString() + " )";
                DB.Update( Command );
            }
        }

        private void MaintainMachine()
        {
            // Ping every 30 seconds
            TimeSpan PingTime = new TimeSpan( 0, 0, 30 );
            if( DateTime.Now - LastPingTime > PingTime )
            {
                string Command = "SELECT ID FROM Builders WHERE ( Machine = '" + MachineName + "' AND State != 'Dead' AND State != 'Zombied' )";
                int ID = DB.ReadInt( Command );
                if( ID != 0 )
                {
                    Command = "UPDATE Builders SET CurrentTime = '" + DateTime.Now.ToString() + "' WHERE ( ID = " + ID.ToString() + " )";
                    DB.Update( Command );
                }

                LastPingTime = DateTime.Now;
            }
        }

        private void UnbroadcastMachine()
        {
            Log( "Unregistering \'" + MachineName + "\' from database", Color.Blue );

            string Command = "SELECT ID FROM Builders WHERE ( Machine = '" + MachineName + "' AND State != 'Dead' AND State != 'Zombied' )";
            int ID = DB.ReadInt( Command );
            if( ID != 0 )
            {
                Command = "UPDATE Builders SET EndTime = '" + DateTime.Now.ToString() + "' WHERE ( ID = " + ID.ToString() + " )";
                DB.Update( Command ); 

                Command = "UPDATE Builders SET State = 'Dead' WHERE ( ID = " + ID.ToString() + " )";
                DB.Update( Command );
            }
        }

        public void Init()
        {
            EnsureDirectoryExists( "Builder" );
            EnsureDirectoryExists( "Builder\\Logs" );
            EnsureDirectoryExists( "Builder\\Scripts" );
            
            DB = new BuilderDB( this );
            Mailer = new Emailer( this );
            SCC = new P4( this );

            GetInfo();
            BroadcastMachine();

            Show();
        }

        public void Destroy()
        {
            UnbroadcastMachine();

            if( DB != null )
            {
                DB.Destroy();
            }
        }

        public Main()
        {
            InitializeComponent();
            
            MainWindow_SizeChanged( null, null );
        }

        private void MainWindow_SizeChanged( object sender, EventArgs e )
        {
            System.Drawing.Size logSize = new Size();
            logSize.Height = this.TextBox_Log.Parent.Size.Height - 55;
            logSize.Width = this.TextBox_Log.Parent.Size.Width - 10;
            this.TextBox_Log.Size = logSize;
        }

        public void Log( string Line, Color TextColour )
        {
            if( Line == null || !Ticking )
            {
                return;
            }

            // if we need to, invoke the delegate
            if( InvokeRequired )
            {
                Invoke( new DelegateAddLine( Log ), new object[] { Line, TextColour } );
                return;
            }

            // OutputDebugString
            System.Diagnostics.Debug.Write( Line + "\r\n" );

            DateTime Now = DateTime.Now;
            string FullLine = Now.ToLongTimeString() + ": " + Line;

            TextBox_Log.Focus();
            TextBox_Log.SelectionLength = 0;

            // Only set the color if it is different than the foreground colour
            if( TextBox_Log.SelectionColor != TextColour )
            {
                TextBox_Log.SelectionColor = TextColour;
            }

            TextBox_Log.AppendText( FullLine + "\r\n" );

            // Handle any special controls
            if( BuildLogID != 0 && Line.StartsWith( "[STATUS] " ) )
            {
                DB.SetString( "BuildLog", BuildLogID, "CurrentStatus", Line.Substring( 9 ) );
            }
        }

        public void Log( Array Lines, Color TextColour )
        {
            foreach( string Line in Lines )
            {
                Log( Line, TextColour );
            }
        }

        private void Cleanup()
        {
            // Revert all open files
            CurrentCommand = new Command( this, SCC, Builder );
            CurrentCommand.Execute( COMMANDS.SCC_Revert );

            DB.SetString( "Builders", BuilderID, "State", "Connected" );
            DB.Delete( "Builders", BuilderID, "CommandID" );
            DB.Delete( "Builders", BuilderID, "BuildLogID" );

            // Set the DB up with the result of the build
            DB.SetDateTime( "BuildLog", BuildLogID, "BuildEnded", DateTime.Now );
            DB.SetString( "BuildLog", BuildLogID, "CurrentStatus", FinalStatus );

            Builder = null;
            BuilderID = 0;
            BuildLogID = 0;
        }

        private void KillBuild( int ID )
        {
            if( BuilderID == -ID )
            {
                Mode = MODES.Exit;
                Log( "Process killed", Color.Red );

                Mailer.SendKilledMail( DB, BuilderID, BuildLogID );

                Cleanup();
            }
        }

        private void SpawnBuild( int ID )
        {
            string CommandString;

            DateTime BuildStarted = DateTime.Now;
            Builder = new ScriptParser( this, DB.GetBuildString( ID, "Command" ), BuildStarted );

            BuilderID = ID;

            // Set builder to building
            DB.SetString( "Builders", BuilderID, "State", "Building" );

            // Add a new entry with the command
            CommandString = "INSERT INTO BuildLog( BuildStarted ) VALUES ( '" + BuildStarted.ToString() + "' )";
            DB.Update( CommandString );

            CommandString = "SELECT ID FROM BuildLog WHERE ( CurrentStatus is NULL )";
            BuildLogID = DB.ReadInt( CommandString );

            if( BuildLogID != 0 )
            {
                CommandString = "UPDATE Builders SET BuildLogID = '" + BuildLogID.ToString() + "' WHERE ( ID = " + BuilderID.ToString() + " )";
                DB.Update( CommandString );
            }

            SCC.Init();
            PendingCommands.Clear();

            Mode = MODES.Init;
        }

        private MODES HandleError()
        {
            string Status = "Succeeded";

            // Internal error?
            ERRORS ErrorLevel = Builder.GetErrorLevel();

            if( ErrorLevel == ERRORS.None )
            {
                // or error that requires parsing the log
                ErrorLevel = CurrentCommand.GetErrorLevel();

                LogParser Parser = new LogParser( Builder );
                bool ReportEverything = ( ErrorLevel <= ERRORS.SCC_Sync && ErrorLevel >= ERRORS.SCC_GetClientRoot );
                Status = Parser.Parse( ReportEverything );
            }

            switch( ErrorLevel )
            {
                case ERRORS.None:
                    if( Status == "Succeeded" )
                    {
                        return ( MODES.Init );
                    }

                    Mailer.SendFailedMail( DB, BuilderID, BuildLogID, Status );

                    Log( "LOG ERROR: " + Builder.GetCommandID() + " " + Builder.GetCommandLine() + " failed", Color.Red );
                    Log( Status, Color.Red );
                    break;

                case ERRORS.NoScript:
                    Status = "No build script";
                    Log( "ERROR: " + Status, Color.Red );
                    Mailer.SendFailedMail( DB, BuilderID, BuildLogID, Status );
                    break;

                case ERRORS.IllegalCommand:
                    Status = "Illegal command: \"" + Builder.GetCommandLine() + "\'";
                    Log( "ERROR: " + Status, Color.Red );
                    Mailer.SendFailedMail( DB, BuilderID, BuildLogID, Status );
                    break;

                case ERRORS.SCC_Sync:
                case ERRORS.SCC_Checkout:
                case ERRORS.SCC_Revert:
                case ERRORS.SCC_Submit:
                case ERRORS.SCC_GetChanges:
                case ERRORS.SCC_GetClientRoot:
                    Status = Builder.GetCommandID() + " " + Builder.GetCommandLine() + " failed with error \'" + ErrorLevel.ToString() + "\'";
                    Log( "ERROR: " + Status, Color.Red );
                    Mailer.SendFailedMail( DB, BuilderID, BuildLogID, Status );
                    break;
                
                case ERRORS.Process:
                    Mailer.SendFailedMail( DB, BuilderID, BuildLogID, CurrentCommand.GetErrorLevel().ToString() );
                    break;

                case ERRORS.TimedOut:
                    Status = "\'" + Builder.GetCommandID() + " " + Builder.GetCommandLine() + "\' TIMED OUT after " + Builder.GetTimeout().Minutes.ToString() + " minutes";
                    Log( "ERROR: " + Status, Color.Red );
                    Mailer.SendFailedMail( DB, BuilderID, BuildLogID, Status );
                    break;
            }

            FinalStatus = "Failed";
            return ( MODES.Exit );
        }

        private MODES HandleComplete()
        {
            Mailer.SendSucceededMail( DB, BuilderID, BuildLogID, Builder.GetChangesLog() );

            DB.SetLastGoodBuild( BuilderID, Builder.GetBuildingFromChangeList(), DateTime.Now );

            return ( MODES.Exit );
        }

        private void RunBuild()
        {
            switch( Mode )
            {
                case MODES.Init:
                    COMMANDS CommandID;

                    // Get a new command ...
                    if( PendingCommands.Count > 0 )
                    {
                        // ... either pending 
                        CommandID = PendingCommands.Dequeue();
                    }
                    else
                    {
                        // ... or from the script
                        CommandID = Builder.ParseNextLine();
                    }

                    switch( CommandID )
                    {
                        case COMMANDS.Error:
                            Mode = MODES.Finalise;
                            break;

                        case COMMANDS.Submit:
                            CurrentCommand = new Command( this, SCC, Builder );
                            Mode = CurrentCommand.Execute( COMMANDS.SCC_Submit );

                            PendingCommands.Enqueue( COMMANDS.Finished );
                            break;

                        case COMMANDS.Finished:
                            Mode = HandleComplete();
                            break;

                        case COMMANDS.Config:
                            break;

                        case COMMANDS.MSVCFull:
                            // Fire off a new process safely
                            CurrentCommand = new Command( this, SCC, Builder );
                            Mode = CurrentCommand.Execute( COMMANDS.MSVCClean );

                            PendingCommands.Enqueue( COMMANDS.MSVCBuild );
                            break;

                        default:
                            // Fire off a new process safely
                            CurrentCommand = new Command( this, SCC, Builder );
                            Mode = CurrentCommand.Execute( CommandID );

                            if( CommandID == COMMANDS.SCC_GetChanges )
                            {
                                DB.SetInt( "BuildLog", BuildLogID, "ChangeList", Builder.GetBuildingFromChangeList() );
                            }
                            break;
                    }
                    break;

                case MODES.Monitor:
                    // Check for completion
                    Mode = CurrentCommand.IsFinished();
                    break;

                case MODES.Finalise:
                    // Analyse logs and restart or exit
                    Mode = HandleError();
                    break;

                case MODES.Exit:
                    Cleanup();
                    break;
            }
        }

        public void Run()
        {
            if( DB == null )
            {
                return;
            }

            // Ping the server to say we're still alive
            MaintainMachine();

            // Poll the DB for commands
            int ID = DB.Poll();

            if( ID < 0 )
            {
                KillBuild( ID );
            }
            else if( ID > 0 )
            {
                SpawnBuild( ID );
            }
            else if( BuildLogID != 0 )
            {
                RunBuild();
            }
        }

        private void Main_FormClosed( object sender, FormClosedEventArgs e )
        {
            Ticking = false;
        }
    }
}