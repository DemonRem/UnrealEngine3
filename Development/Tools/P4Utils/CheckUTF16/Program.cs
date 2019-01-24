/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using P4API;

namespace CheckEncoding
{
	class Program
	{
		static private P4Connection IP4 = null;
		static private int ValidFileCount = 0;

		static private List<string> PathsToIgnore = new List<string>()
		{
			"//depot/UnrealEngine3/Development/External/",
			"//depot/UnrealEngine3/Development/Src/Xenon/External/",
			"//depot/UnrealEngine3/Development/Src/PS3/External/"
		};

		static private bool Filtered( string FileName )
		{
			foreach( string PathToIgnore in PathsToIgnore )
			{
				if( FileName.ToLower().Contains( PathToIgnore.ToLower() ) )
				{
					return ( true );
				}
			}

			return ( false );
		}

		static private bool ValidateFile( string DepotFile, string Revision )
		{
			string FileSpec = DepotFile + "@=" + Revision;
            string TempFile = Path.GetTempFileName();

			// Get the data, and put it in a temp file
            P4RecordSet P4Output = IP4.Run( "print", "-q", "-o", TempFile, FileSpec );
            try
            {
                byte[] BinaryData = File.ReadAllBytes( TempFile );

				if( BinaryData.Length >= 2 )
				{
					// Check for the BOM
					if( BinaryData[0] != 0xff || BinaryData[1] != 0xfe )
					{
						Console.WriteLine( "'" + DepotFile + "' is CORRUPT! (missing LE BOM)" + Environment.NewLine + "Please convert to UTF-16 and resubmit" );
						return ( false );
					}

					if( BinaryData.Length >= 4 )
					{
						// Make sure at least 1 char in 16 is wide
						int ZeroCount = 0;
						for( int Index = 3; Index < BinaryData.Length; Index += 2 )
						{
							if( BinaryData[Index] == 0x00 )
							{
								ZeroCount++;
							}
						}

						if( ZeroCount < BinaryData.Length / 32 )
						{
							Console.WriteLine( "'" + DepotFile + "' is CORRUPT! (too many single byte characters)" + Environment.NewLine + "Please convert to UTF-16 and resubmit" );
							return ( false );
						}
					}
				}
            }
            finally
            {
				File.SetAttributes( TempFile, File.GetAttributes( TempFile ) & ~FileAttributes.ReadOnly );
                File.Delete( TempFile );
            }

			ValidFileCount++;
			return ( true );
		}

		// Validate text file does not contain any characters with the high-bit set.
		static bool ValidateFileText( string DepotFile, string Revision )
		{
			string FileSpec = DepotFile + "@=" + Revision;
			string TempFile = Path.GetTempFileName();

			// Get the data, and put it in a temp file
			P4RecordSet P4Output = IP4.Run( "print", "-q", "-o", TempFile, FileSpec );
			try
			{
				byte[] BinaryData = File.ReadAllBytes( TempFile );

				if( BinaryData.Length >= 3 && BinaryData[0] == 0xEF && BinaryData[1] == 0xBB && BinaryData[2] == 0xBF )
				{
					// Accept anything UTF-8 encoded with a BOM.
					// See http://en.wikipedia.org/wiki/Byte_order_mark#Unwanted_BOMs
				}
				else
				{
					int LineNum = 1;
					for( int Index = 0; Index < BinaryData.Length; Index++ )
					{
						if( BinaryData[Index] == '\n' )
						{
							LineNum++;
						}

						if( BinaryData[Index] > 127 )
						{
							Console.WriteLine( "'" + DepotFile + "' contains characters with the high-bit set, which can cause problems on non-US Windows." + Environment.NewLine + "The first instance is on line " + LineNum.ToString() + ". Please fix the file and resubmit." );
							return ( false );
						}
					}
				}
			}
			finally
			{
				File.SetAttributes( TempFile, File.GetAttributes( TempFile ) & ~FileAttributes.ReadOnly );
				File.Delete( TempFile );
			}

			ValidFileCount++;
			return ( true );
		}

		static void Main( string[] args )
		{
			bool Error = false;

			if( args.Length < 4 )
			{
				Console.WriteLine( "Checks the encoding of submitted files" );
				Console.WriteLine( " - checks that UTF-16 are UTF-16" );
				Console.WriteLine( " - checks that text files are plain text" );
				Console.WriteLine( "Usage: CheckUTF16 <p4port> <p4user> <p4password> <changelist>" );
				Environment.Exit( -1 );
			}

			try
			{
				P4RecordSet P4Output = null;
				ValidFileCount = 0;

				// Get a detailed description of the changelist
				IP4 = new P4Connection();
				IP4.Port = args[0];
				IP4.User = args[1];
				IP4.Password = args[2];
				IP4.Connect();
				try
				{
					P4Output = IP4.Run( "describe", args[3] );

					// Check the files in the changelist
					foreach( P4Record Record in P4Output )
					{
						for( int Index = 0; Index < Record.ArrayFields["type"].Length; Index++ )
						{
							if( !Filtered( Record.ArrayFields["depotFile"][Index] ) )
							{
								if( Record.ArrayFields["type"][Index].ToLower() == "utf16" )
								{
									Error |= !ValidateFile( Record.ArrayFields["depotFile"][Index], args[3] );
								}
								else if( Record.ArrayFields["type"][Index].ToLower() == "text" )
								{
									Error |= !ValidateFileText( Record.ArrayFields["depotFile"][Index], args[3] );
								}
							}
						}
					}
				}
				finally
				{
					IP4.Disconnect();
				}
			}
			catch( Exception Ex )
			{
				// If something went bad, allow the changelist to be submitted
				Console.WriteLine( "An error occurred trying to validate encoding of files; allowing submission" );
				Console.WriteLine( "Exception: " + Ex.ToString() );
				Environment.Exit( 0 );
			}

			if( ValidFileCount > 0 )
			{
				Console.WriteLine( ValidFileCount.ToString() + " files have been validated as being the correct format" );
			}

			if( Error )
			{
				Environment.Exit( 1 );
			}

			Environment.Exit( 0 );
		}
	}
}
