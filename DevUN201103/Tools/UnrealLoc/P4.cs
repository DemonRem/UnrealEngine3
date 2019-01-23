/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime;
using System.Text;
using System.Windows.Forms;
using P4API;

namespace UnrealLoc
{
    class P4
    {
        UnrealLoc Main = null;
		private P4Connection IP4 = null;

        public P4( UnrealLoc InMain )
        {
            Main = InMain;

            try
            {
                IP4 = new P4Connection();
            }
            catch
            {
                Main.Log( UnrealLoc.VerbosityLevel.Critical, "Perforce connection FAILED", Color.Red );
                IP4 = null;
            }
        }

		// action = 'add'
		// workRev = '1'
		// depotFile = '//depot/UnrealEngine3-Builder/ExampleGame/Localization/ITA/examplegame.ITA'
		// clientFile = 'D:\Test\UnrealEngine3-Builder\ExampleGame\Localization\ITA\examplegame.ITA'
		// type = 'utf16'

		// action - add
		// workRev - #
		// depotFile = '//depot/UnrealEngine3-Builder/Engine/Localization/FRA/windrv.FRA'
		// clientFile = 'D:\Test\UnrealEngine3-Builder\Engine\Localization\FRA\windrv.FRA'
		// type = 'utf16'

		// action - reverted
		// depotFile = '//depot/UnrealEngine3-Builder/Binaries/UnrealLoc.exe.config'
		// haveRev = '1'
		// clientFile = 'D:\Test\UnrealEngine3-Builder\Binaries\UnrealLoc.exe.config'
		// oldAction = 'edit'
		
		private void LogResult( P4RecordSet Output )
		{
			foreach( P4Record Record in Output.Records )
			{
				string Line = "Unsupported action";

				switch( Record.Fields["action"] )
				{
				case "edit":
					Line = "P4: ClientFile: '" + Record.Fields["clientFile"] + "#" + Record.Fields["workRev"] + "' (" + Record.Fields["type"] + ") was checked out";
					break;

				case "add":
					Line = "P4: ClientFile: '" + Record.Fields["clientFile"] + "' was added as type '" + Record.Fields["type"] + "'";
					break;

				case "reverted":
					Line = "P4: ClientFile: '" + Record.Fields["clientFile"] + "#" + Record.Fields["haveRev"] + "' was reverted";
					break;
				}

				Main.Log( UnrealLoc.VerbosityLevel.Informative, Line, Color.Black );

				foreach( string Key in Record.Fields.Keys )
				{
					Main.Log( UnrealLoc.VerbosityLevel.Verbose, "'" + Key + "' = '" + Record.Fields[Key] + "'", Color.Magenta );
				}
			}
		}

        public bool AddToSourceControl( LanguageInfo Lang, string FileSpec )
        {
			P4RecordSet Output;
            bool Success = false;

            IP4.Connect();
            IP4.Client = Main.Options.ClientSpec;
            IP4.CWD = Environment.CurrentDirectory;
			IP4.ExceptionLevel = P4API.P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

            try
            {
				// Open the file for edit
                Main.Log( UnrealLoc.VerbosityLevel.Informative, "Executing 'p4 edit " + FileSpec + "'", Color.Black );
				Output = IP4.Run( "edit", FileSpec );

				LogResult( Output );
            }
			catch( P4API.Exceptions.RunException )
            {
				// ... which generates an exception if the file does not exist - so add it
                try
                {
					switch( Main.Options.LocFileType )
					{
					case UnrealLoc.EFileType.UTF16Mergable:
						Main.Log( UnrealLoc.VerbosityLevel.Informative, "Executing 'P4 add -t utf16 " + FileSpec + "'", Color.Black );
						Output = IP4.Run( "add", "-t", "utf16", FileSpec );
						LogResult( Output );
						break;

					case UnrealLoc.EFileType.BinaryExclusive:
						Main.Log( UnrealLoc.VerbosityLevel.Informative, "Executing 'P4 add -t binary+l " + FileSpec + "'", Color.Black );
						Output = IP4.Run( "add", "-t", "binary+l", FileSpec );
						LogResult( Output );
						break;
					}

                    Success = true;
                }
				catch( P4API.Exceptions.RunException Ex )
                {
                    Main.Error( Lang, "(Perforce) Add " + Ex.Message );
                }
            }

            IP4.Disconnect();
            return ( Success );
        }

        public bool RevertUnchanged()
        {
			P4RecordSet Output;
            bool Success = false;

            IP4.Connect();
            IP4.Client = Main.Options.ClientSpec;
            IP4.CWD = Environment.CurrentDirectory;
			IP4.ExceptionLevel = P4API.P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

            try
            {
                Main.Log( UnrealLoc.VerbosityLevel.Informative, "Executing 'P4 revert -a'", Color.Black );
				Output = IP4.Run( "revert", "-a" );
				LogResult( Output );

                Success = true;
            }
            catch
            {
            }

            IP4.Disconnect();
            return ( Success );
        }
    }
}
