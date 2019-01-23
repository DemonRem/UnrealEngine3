using System;
using System.Collections;
using System.Drawing;
using System.Diagnostics;
using System.IO;

namespace Controller
{
    class BuildProcess
    {
        public bool IsFinished = false;

        private Main Parent;
        private Process RunningProcess;
        private ERRORS ErrorLevel = ERRORS.None;
        private StreamWriter Log = null;
        private string Executable;
        private string CommandLine;

        public ERRORS GetErrorLevel()
        {
            return ( ErrorLevel );
        }

        private void SetEnvVars( Process RunningProcess, string[] EnvVars )
        {
            for( int i = 0; i < EnvVars.Length; i += 2 )
            {
                if( !RunningProcess.StartInfo.EnvironmentVariables.ContainsKey( EnvVars[i] ) )
                {
                    RunningProcess.StartInfo.EnvironmentVariables.Add( EnvVars[i], EnvVars[i + 1] );
                }
            }
        }

        public BuildProcess( Main InParent, StreamWriter InLog, string InExecutable, string InCommandLine, string[] EnvVars )
        {
            Parent = InParent;
            Log = InLog;
            RunningProcess = new Process();
            Executable = InExecutable;
            CommandLine = InCommandLine;

            // Prepare a ProcessStart structure 
            RunningProcess.StartInfo.FileName = Executable;
            RunningProcess.StartInfo.Arguments = CommandLine;

            if( EnvVars != null )
            {
                SetEnvVars( RunningProcess, EnvVars );
            }

            // Redirect the output.
            RunningProcess.StartInfo.UseShellExecute = false;
            RunningProcess.StartInfo.RedirectStandardOutput = true;
            RunningProcess.StartInfo.RedirectStandardError = false;
            RunningProcess.StartInfo.CreateNoWindow = true;

            Parent.Log( "Spawning: " + Executable + " " + CommandLine, Color.Green );

            // Spawn the process - try to start the process, handling thrown exceptions as a failure.
            try
            {
                RunningProcess.OutputDataReceived += new DataReceivedEventHandler( PrintLog );
                RunningProcess.Start();
                RunningProcess.BeginOutputReadLine();

                RunningProcess.Exited += new EventHandler( ProcessExit );
                RunningProcess.EnableRaisingEvents = true;
            }
            catch
            {
                RunningProcess = null;
                IsFinished = true;
                Parent.Log( "PROCESS ERROR: Failed to start: " + Executable, Color.Red );

                if( Log != null )
                {
                    Log.Write( "PROCESS ERROR: Failed to start: " + Executable + "\r\n" );
                    Log.Close();
                }
                ErrorLevel = ERRORS.Process;
            }
        }

        public void Kill()
        {
            Parent.Log( "Killing: " + Executable, Color.Red );

            if( RunningProcess != null )
            {
                RunningProcess.Kill();
            }
            if( Log != null )
            {
                Log.Close();
            }
            IsFinished = true;
       }

        public void ProcessExit( object Sender, System.EventArgs e )
        {
            if( Log != null )
            {
                Log.Close();
            }
            IsFinished = true;
        }

        public void PrintLog( object Sender, DataReceivedEventArgs e )
        {
            if( Log != null )
            {
                string Line = e.Data;
                Log.Write( Line + "\r\n" );
            }
        }
    }
}
