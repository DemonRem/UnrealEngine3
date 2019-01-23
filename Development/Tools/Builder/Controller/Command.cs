using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;

namespace Controller
{
    class Command
    {
        private Main Parent = null;
        private P4 SCC = null;
        private ScriptParser Builder = null;

        private StreamWriter Log;
        private ERRORS ErrorLevel = ERRORS.None;
        private BuildProcess CurrentBuild = null;
        private DateTime StartTime = DateTime.Now;

        public Command( Main InParent, P4 InSCC, ScriptParser InBuilder )
        {
            Parent = InParent;
            SCC = InSCC;
            Builder = InBuilder;
        }

        public ERRORS GetErrorLevel()
        {
            return ( ErrorLevel );
        }

        private void SCC_Sync()
        {
            int ChangeList = 0;

            Log = new StreamWriter( Builder.GetLogFileName() );
            string CommandLine = Builder.GetCommandLine();

            // Optional changelist to sync to
            if( CommandLine.Length > 0 )
            {
                if( CommandLine.ToUpper() == CommandLine.ToLower() )
                {
                    // Likely a number
                    ChangeList = Builder.SafeStringToInt( CommandLine );
                }
            }

            try
            {
                if( ChangeList == 0 )
                {
                    Parent.Log( "[STATUS] Syncing to head", Color.Green );
                    SCC.SyncToChangeList( Log, Builder.GetClientSpec(), 0, "" );
                }
                else if( CommandLine.Length > 0 )
                {
                    Parent.Log( "[STATUS] Syncing to label \"" + CommandLine + "\'", Color.Green );
                    SCC.SyncToChangeList( Log, Builder.GetClientSpec(), 0, CommandLine );
                }
                else
                {
                    Parent.Log( "[STATUS] Syncing to changelist " + ChangeList.ToString(), Color.Green );
                    SCC.SyncToChangeList( Log, Builder.GetClientSpec(), ChangeList, "" );
                }

                ErrorLevel = SCC.GetErrorLevel();
            }
            catch
            {
                ErrorLevel = ERRORS.SCC_Sync;
            }
            Log.Close();
        }

        private void SCC_Checkout()
        {
            Log = new StreamWriter( Builder.GetLogFileName() );
            try
            {
                Parent.Log( "[STATUS] Checking out: " + Builder.GetCommandLine(), Color.Green );

                SCC.CheckoutFileSpec( Log, Builder.GetClientSpec(), Builder.GetCommandLine(), false );
                Builder.AddCheckedOutFileSpec( Builder.GetCommandLine() );

                ErrorLevel = SCC.GetErrorLevel();
            }
            catch
            {
                ErrorLevel = ERRORS.SCC_Checkout;
            }
            Log.Close();
        }

        private void SCC_Submit()
        {
            Log = new StreamWriter( Builder.GetLogFileName() );
            try
            {
                Parent.Log( "[STATUS] Submitting all files", Color.Green );
                SCC.Submit( Log, Builder.GetClientSpec(), Builder.GetBuildingFromChangeList() );

                Builder.ClearCheckedOutFiles();
                ErrorLevel = SCC.GetErrorLevel();
            }
            catch
            {
                ErrorLevel = ERRORS.SCC_Submit;
            }
            Log.Close();
        }

        private void SCC_Revert()
        {
            Log = new StreamWriter( Builder.GetLogFileName() );
            try
            {
                List<string> CheckedOutFiles = Builder.GetCheckedOutFiles();
                foreach( string FileSpec in CheckedOutFiles )
                {
                    Parent.Log( "[STATUS] Reverting: " + FileSpec, Color.Green );
                    SCC.Revert( Log, Builder.GetClientSpec(), FileSpec );
                }

                Builder.ClearCheckedOutFiles();
                ErrorLevel = SCC.GetErrorLevel();
            }
            catch
            {
                ErrorLevel = ERRORS.SCC_Revert;
            }
            Log.Close();
        }
        
        private void SCC_GetChanges()
        {
            Log = new StreamWriter( Builder.GetLogFileName() );
            try
            {                
                SCC.GetMostRecentBuild( Builder, Log, "UnrealEngine3Build-UberServer" );
                Parent.Log( "[STATUS] Most recent build was from ChangeList " + Builder.GetMostRecentBuild().ToString(), Color.Green );
                ErrorLevel = SCC.GetErrorLevel();

                if( ErrorLevel == ERRORS.None )
                {
                    SCC.GetChangesSinceLastBuild( Builder, Log );
                    Parent.Log( "[STATUS] Building from ChangeList " + Builder.GetBuildingFromChangeList().ToString(), Color.Green );
                    ErrorLevel = SCC.GetErrorLevel();
                }
            }
            catch
            {
                ErrorLevel = ERRORS.SCC_GetChanges;
            }
            Log.Close();
        }

        private void MSVC_Clean()
        {
            try
            {
                string CommandLine = Builder.GetCommandLine() + " /clean \"" + Builder.GetConfiguration() + "\" /out \"" + Builder.GetLogFileName() + "\"";

                Parent.Log( "[STATUS] Cleaning " + Builder.GetCommandLine(), Color.Green );
                CurrentBuild = new BuildProcess( Parent, null, Builder.GetMSVCApplication(), CommandLine, null );
                ErrorLevel = CurrentBuild.GetErrorLevel();
                StartTime = DateTime.Now;
            }
            catch
            {
                ErrorLevel = ERRORS.Process;
            }
        }

        private void MSVC_Build()
        {
            try
            {
                string CommandLine = Builder.GetCommandLine() + " /build \"" + Builder.GetConfiguration() + "\" /out \"" + Builder.GetLogFileName() + "\"";

                Parent.Log( "[STATUS] Building " + Builder.GetCommandLine(), Color.Green );
                CurrentBuild = new BuildProcess( Parent, null, Builder.GetMSVCApplication(), CommandLine, null );
                ErrorLevel = CurrentBuild.GetErrorLevel();
                StartTime = DateTime.Now;
            }
            catch
            {
                ErrorLevel = ERRORS.Process;
            }
        }

        private void GCC_Build()
        {
            try
            {
                string CommandLine = "-j2";
                Parent.Log( "[STATUS] Building " + Builder.GetCommandLine(), Color.Green );
                CurrentBuild = new BuildProcess( Parent, null, "make", CommandLine, Builder.GetPS3EnvVars() );
                ErrorLevel = CurrentBuild.GetErrorLevel();
                StartTime = DateTime.Now;
            }
            catch
            {
                ErrorLevel = ERRORS.Process;
            }
        }

        private void BuildScript()
        {
            try
            {
                string Executable = "Binaries/" + Builder.GetCommandLine() + "Game.com";
                string CommandLine = "make -full -unattended";

                StreamWriter Log = new StreamWriter( Builder.GetLogFileName() );

                Parent.Log( "[STATUS] Building script for " + Builder.GetCommandLine() + "Game", Color.Green );
                CurrentBuild = new BuildProcess( Parent, Log, Executable, CommandLine, null );
                ErrorLevel = CurrentBuild.GetErrorLevel();
                StartTime = DateTime.Now;
            }
            catch
            {
                ErrorLevel = ERRORS.Process;
            }
        }

        private void Cook()
        {
            try
            {
                string[] Parms = Builder.GetCommandLine().Split( " \t".ToCharArray() );
                string Executable = "Binaries/" + Parms[0] + "Game.com";

                string CommandLine = "CookPackages -platform=" + Builder.GetPlatform();
                for( int i = 1; i < Parms.Length; i++ )
                {
                    CommandLine += " " + Parms[i];
                }
                
                CommandLine += " -alwaysRecookmaps -alwaysRecookScript -updateInisAuto";

                StreamWriter Log = new StreamWriter( Builder.GetLogFileName() );

                Parent.Log( "[STATUS] Cooking maps for " + Builder.GetCommandLine() + "Game", Color.Green );
                CurrentBuild = new BuildProcess( Parent, Log, Executable, CommandLine, null );
                ErrorLevel = CurrentBuild.GetErrorLevel();
                StartTime = DateTime.Now;
            }
            catch
            {
                ErrorLevel = ERRORS.Process;
            }
        }

        public MODES Execute( COMMANDS CommandID )
        {
            switch( CommandID )
            {
                case COMMANDS.SCC_Sync:
                    SCC_Sync();
                    return ( MODES.Finalise );

                case COMMANDS.SCC_Checkout:
                    SCC_Checkout();
                    return ( MODES.Finalise );

                case COMMANDS.SCC_Submit:
                    SCC_Submit();
                    return ( MODES.Finalise );

                case COMMANDS.SCC_Revert:
                    SCC_Revert();
                    return ( MODES.Finalise );
            
                case COMMANDS.SCC_GetChanges:
                    SCC_GetChanges();
                    return( MODES.Finalise );

                case COMMANDS.MSVCClean:
                    MSVC_Clean();
                    return ( MODES.Monitor );

                case COMMANDS.MSVCBuild:
                    MSVC_Build();
                    return ( MODES.Monitor );

                case COMMANDS.GCCBuild:
                    GCC_Build();
                    return ( MODES.Monitor );

                case COMMANDS.BuildScript:
                    BuildScript();
                    return ( MODES.Monitor );

                case COMMANDS.Cook:
                    Cook();
                    return ( MODES.Monitor );
            }

            return( MODES.Init );
        }

        public MODES IsFinished()
        {
            // Also check for timeout
            if( CurrentBuild != null )
            {
                if( CurrentBuild.IsFinished )
                {
                    return ( MODES.Finalise );
                }

                if( DateTime.Now - StartTime > Builder.GetTimeout() )
                {
                    CurrentBuild.Kill();
                    ErrorLevel = ERRORS.TimedOut;
                    return ( MODES.Finalise );
                }
                return ( MODES.Monitor );
            }

            // No running build? Something went wrong
            return ( MODES.Finalise );
        }
    }
}
