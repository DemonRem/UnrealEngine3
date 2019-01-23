using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Controller
{
    class LogParser
    {
        private ScriptParser Builder = null;
        private StreamReader Log = null;
        private string FinalError;
        private bool FoundError = false;

        public LogParser( ScriptParser InBuilder )
        {
            Builder = InBuilder;

            try
            {
                Log = new StreamReader( Builder.GetLogFileName() );
            }
            catch
            {
            }
        }

        public string Parse( bool ReportEntireLog )
        {
            string Line;    

            if( Log == null )
            {
                return ( "Failed to open log: \'" + Builder.GetLogFileName() + "\'" );
            }

            // Read in the entire log file
            Line = Log.ReadLine();
            while( Line != null )
            {
                if( Line.StartsWith( "------" ) )
                {
                    FinalError += Line + "\r\n";
                }
                else if( Builder.GetCheckErrors() && 
                         ( Line.IndexOf( " : error" ) >= 0 
                         || Line.IndexOf( ": error:" ) >= 0 
                         || Line.IndexOf( ": fatal error" ) >= 0 ) )
                {
                    FoundError = true;
                    FinalError += Line + "\r\n";
                }
                else if( Builder.GetCheckErrors() &&
                         ( Line.IndexOf( "Error," ) >= 0 ) )
                {
                    FoundError = true;
                    FinalError += Line + "\r\n";
                }
                else if( Builder.GetCheckWarnings() &&
                         ( Line.IndexOf( " : warning" ) >= 0
                         || Line.IndexOf( ": warning:" ) >= 0 ) )
                {
                    FoundError = true;
                    FinalError += Line + "\r\n";
                }
                else if( ReportEntireLog )
                {
                    FoundError = true;
                    FinalError += Line + "\r\n";
                }

                Line = Log.ReadLine();
            }

            Log.Close();

            if( FoundError )
            {
                return ( FinalError );
            }

            return ( "Succeeded" );
        }
    }
}
