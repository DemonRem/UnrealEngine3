using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;

namespace Controller
{
    class ScriptParser
    {
        private Main Parent = null;
        private StreamReader Script = null;
        private int LineCount = 0;

        private string[] PS3EnvVars = 
        {
            "CELL_SDK", "/c/PS3/cell/current",
            "CELL_SDK_WIN", "C:/PS3/cell/current",
            "SCE_PS3_ROOT", "C:/PS3/cell/current",
            "OSDIR", "C:/PS3/cell/current/host-win32",
            "MINGW", "C:/PS3/MinGW/bin;C:/PS3/MinGW/MSys/1.0/bin",
            //"PATH", "%MINGW%;%CELL_SDK_WIN%/host-win32/bin;%CELL_SDK_WIN%/host-win32/ppu/bin;%CELL_SDK_WIN%/host-win32/spu/bin;%CELL_SDK_WIN%/host-win32/Cg/bin;%PATH%"
        };

        private string MSVCApplication = "C:\\Program Files\\Microsoft Visual Studio 8\\Common7\\IDE\\devenv.exe";
        private string Platform = "PC";
        private string Configuration = "Release";
        private string ClientSpec = "";

        private COMMANDS CommandID = COMMANDS.Config;
        private ERRORS ErrorLevel = ERRORS.None;
        private string CommandLine = "";
        private string LogFileRootName = "";

        private bool CheckErrors = true;
        private bool CheckWarnings = true;
        private TimeSpan OperationTimeout = new TimeSpan( 0, 10, 0 );
        private int BuildingFromChangeList = 0;
        private int MostRecentBuild = 0;
        private List<string> CheckedOutFiles = new List<string>();

        private StringBuilder ChangesLog = new StringBuilder();

        public ScriptParser( Main InParent, string ScriptFileName, DateTime BuildStarted )
        {
            Parent = InParent;
            try
            {
                string ScriptFile = "Builder/Scripts/" + ScriptFileName;
                Script = new StreamReader( ScriptFile );
                Parent.Log( "Using build script \'" + ScriptFile + "\'", Color.Magenta );

                LogFileRootName = "Builder/Logs/Builder_[" + BuildStarted.Year + "-" + BuildStarted.Month + "-" + BuildStarted.Day + "_" + BuildStarted.Hour + "." + BuildStarted.Minute + "]";
            }
            catch
            {
                Parent.Log( "ERROR Loading build script", Color.Red );
            }
        }

        public void Destroy()
        {
            Parent.Log( LineCount.ToString() + " parsed", Color.Magenta );
            Script.Close();
        }

        public bool GetCheckErrors()
        {
            return ( CheckErrors );
        }

        public bool GetCheckWarnings()
        {
            return ( CheckWarnings );
        }

        public string[] GetPS3EnvVars()
        {
            return ( PS3EnvVars );
        }

        public string GetMSVCApplication()
        {
            return ( MSVCApplication );
        }

        public string GetPlatform()
        {
            return ( Platform );
        }

        public string GetConfiguration()
        {
            if( Platform.ToLower() == "xenon" )
            {
                return ( "Xe" + Configuration );
            }
            else if( Platform.ToLower() == "ps3" )
            {
                return ( "PS3_" + Configuration );
            }

            return ( Configuration );
        }

        public string GetClientSpec()
        {
            return ( ClientSpec );
        }

        public ERRORS GetErrorLevel()
        {
            return ( ErrorLevel );
        }

        public string GetCommandLine()
        {
            return ( CommandLine );
        }

        public string GetLogFileName()
        {
            string CommandName = CommandID.ToString();
            string LogFileName = LogFileRootName + "_" + LineCount.ToString() + "_" + CommandName + ".txt";
            return ( LogFileName );
        }

        public string GetChangesLog()
        {
            return ( ChangesLog.ToString() );
        }

        public COMMANDS GetCommandID()
        {
            return ( CommandID );
        }

        public int SafeStringToInt( string Number )
        {
            int Result = 0;

            try
            {
                Number = Number.Trim();
                Result = Int32.Parse( Number );
            }
            catch
            {
            }

            return ( Result );
        }

        public void AddCheckedOutFileSpec( string FileSpec )
        {
            CheckedOutFiles.Add( FileSpec );
        }

        public List<string> GetCheckedOutFiles()
        {
            return ( CheckedOutFiles );
        }

        public void ClearCheckedOutFiles()
        {
            CheckedOutFiles.Clear();
        }

        public int GetMostRecentBuild()
        {
            return ( MostRecentBuild );
        }

        public int GetBuildingFromChangeList()
        {
            return ( BuildingFromChangeList );
        }

        public void SetMostRecentBuild( string ChangeList )
        {
            MostRecentBuild = SafeStringToInt( ChangeList );
        }

        public TimeSpan GetTimeout()
        {
            return ( OperationTimeout );
        }

        public COMMANDS ParseNextLine()
        {
            string Line;

            if( Script == null )
            {
                ErrorLevel = ERRORS.NoScript;
                CommandID = COMMANDS.Error;
                return( CommandID );
            }

            Line = Script.ReadLine();
            if( Line == null )
            {
                Parent.Log( "[STATUS] Script parsing completed", Color.Magenta );
                CommandID = COMMANDS.Submit;
                return( CommandID );
            }

            LineCount++;
            CommandID = COMMANDS.Config;
            CommandLine = "";

            if( Line.StartsWith( "//" ) || Line.Length == 0 )
            {
                // Comment - ignore
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "status" ) )
            {
                // Status string - just echo
                Line = Line.Substring( "status".Length ).Trim();
                Parent.Log( "[STATUS] " + Line, Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "checkerrors" ) )
            {
                CheckErrors = true;
                Parent.Log( "Checking for errors ENABLED", Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "ignoreerrors" ) )
            {
                CheckErrors = false;
                Parent.Log( "Checking for errors DISABLED", Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "checkwarnings" ) )
            {
                CheckWarnings = true;
                Parent.Log( "Checking for warnings ENABLED", Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "ignorewarnings" ) )
            {
                CheckWarnings = false;
                Parent.Log( "Checking for warnings DISABLED", Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "clientspec" ) )
            {
                // Clientspec for source control to use
                ClientSpec = Line.Substring( "clientspec".Length ).Trim();
                Parent.Log( "ClientSpec set to \'" + ClientSpec + "\'", Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "timeout" ) )
            {
                // Number of minutes to failure
                int OpTimeout = SafeStringToInt( Line.Substring( "timeout".Length ) );
                OperationTimeout = new TimeSpan( 0, OpTimeout, 0 );
                Parent.Log( "Timeout set to " + OpTimeout.ToString() + " minutes", Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "devenv" ) )
            {
                // MSVC command line app
                MSVCApplication = Line.Substring( "devenv".Length ).Trim();
                Parent.Log( "MSVC executable set to \'" + MSVCApplication + "\'", Color.Magenta ); 
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "platform" ) )
            {
                // Platform we are interested in
                Platform = Line.Substring( "platform".Length ).Trim();
                Parent.Log( "Platform set to \'" + Platform + "\'", Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "config" ) )
            {
                // Platform we are interested in
                Configuration = Line.Substring( "config".Length ).Trim();
                Parent.Log( "Configuration set to \'" + Configuration + "\'", Color.Magenta );
                CommandID = COMMANDS.Config;
            }
            else if( Line.ToLower().StartsWith( "sync" ) )
            {
                CommandLine = Line.Substring( "sync".Length ).Trim();
                CommandID = COMMANDS.SCC_Sync;
            }
            else if( Line.ToLower().StartsWith( "checkout" ) )
            {
                // Filespec to checkout
                CommandLine = Line.Substring( "checkout ".Length ).Trim();
                CommandID = COMMANDS.SCC_Checkout;
            }
            else if( Line.ToLower().StartsWith( "revert" ) )
            {
                // All filespecs checked out with the checkout command
                CommandID = COMMANDS.SCC_Revert;
            }
            else if( Line.ToLower().StartsWith( "getchanges" ) )
            {
                CommandID = COMMANDS.SCC_GetChanges;
            }
            else if( Line.ToLower().StartsWith( "msvcclean" ) )
            {
                CommandLine = Line.Substring( "msvcclean".Length ).Trim();
                CommandID = COMMANDS.MSVCClean;
            }
            else if( Line.ToLower().StartsWith( "msvcbuild" ) )
            {
                CommandLine = Line.Substring( "msvcbuild".Length ).Trim();
                CommandID = COMMANDS.MSVCBuild;
            }
            else if( Line.ToLower().StartsWith( "msvcfull" ) )
            {
                CommandLine = Line.Substring( "msvcfull".Length ).Trim();
                CommandID = COMMANDS.MSVCFull;
            }
            else if( Line.ToLower().StartsWith( "gccbuild" ) )
            {
                CommandID = COMMANDS.GCCBuild;
            }
            else if( Line.ToLower().StartsWith( "buildscript" ) )
            {
                CommandLine = Line.Substring( "buildscript".Length ).Trim();
                CommandID = COMMANDS.BuildScript;
            }
            else if( Line.ToLower().StartsWith( "cook" ) )
            {
                CommandLine = Line.Substring( "cook".Length ).Trim();
                CommandID = COMMANDS.Cook;
            }
            else
            {
                CommandLine = Line;
                ErrorLevel = ERRORS.IllegalCommand;
                CommandID = COMMANDS.Error;
            }

            return ( CommandID );
        }

        // Goes over a changelist description and extracts the relevant information
        public void ProcessChangeList( string ChangeList, System.Array Output )
        {
            int ChangeListNumber = SafeStringToInt( ChangeList );
            if( ChangeListNumber > BuildingFromChangeList )
            {
                BuildingFromChangeList = ChangeListNumber;
            }

            foreach( string Line in Output )
            {
                if( Line.StartsWith( "Differences ..." ) )
                {
                    break;
                }

                ChangesLog.Append( Line + "\r\n" );
            }

            ChangesLog.Append( "\r\n" );
        }
    }
}
