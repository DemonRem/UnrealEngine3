/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Data;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace PerforcePlugin
{
	class Program
	{
		public class BuilderDB
		{
			public bool bInitialized = false;
			private SqlConnection Connection = null;

			public bool OpenConnection()
			{
				try
				{
					Connection = new SqlConnection( "Data Source=DB-04; Initial Catalog=Builder; Integrated Security=True" );
					Connection.Open();

					Console.WriteLine( "Database connection opened successfully" + Environment.NewLine );
					return true;
				}
				catch( Exception Ex )
				{
					Console.WriteLine( "Database connection open failed with exception: " + Ex.ToString() );
					Connection = null;
					return false;
				}
			}

			public void CloseConnection()
			{
				if( Connection != null )
				{
					try
					{
						Connection.Close();
						Console.WriteLine( Environment.NewLine + "Database connection closed successfully" );
					}
					catch( System.Exception Ex )
					{
						Console.WriteLine( "Database connection close failed with exception: " + Ex.ToString() );
					}
					Connection = null;
				}
			}

			public string ReadString( string CommandString )
			{
				string Result = "";
				if( Connection != null )
				{
					SqlDataReader DataReader = new SqlCommand( CommandString, Connection ).ExecuteReader();
					if( DataReader != null )
					{
						try
						{
							if( DataReader.Read() )
							{
								Result = DataReader.GetString( 0 );
							}
						}
						catch( Exception Ex )
						{
							Console.WriteLine( "ReadString with \"" + CommandString + "\" failed with exception: " + Ex.ToString() );
						}
						DataReader.Close();
					}
				}
				return ( Result );
			}
			public Int32 ReadInt( string CommandString )
			{
				Int32 Result = 0;
				if( Connection != null )
				{
					SqlDataReader DataReader = new SqlCommand( CommandString, Connection ).ExecuteReader();
					if( DataReader != null )
					{
						try
						{
							if( DataReader.Read() )
							{
								Result = DataReader.GetInt32( 0 );
							}
						}
						catch( Exception Ex )
						{
							Console.WriteLine( "ReadString with \"" + CommandString + "\" failed with exception: " + Ex.ToString() );
						}
						DataReader.Close();
					}
				}
				return ( Result );
			}

			public string GetVariable( string Branch, string Var )
			{
				string Query = "SELECT Value FROM Variables WHERE ( Variable = '" + Var + "' AND Branch = '" + Branch + "' )";
				return ReadString( Query );
			}
			public string GetCISVariable( string Branch, string Var )
			{
				string Query = "SELECT " + Var + " FROM BranchConfig WHERE ( Branch = '" + Branch + "' )";
				return ReadInt( Query ).ToString();
			}
		}

		public static void OutputReceivedDataEventHandler( Object Sender, DataReceivedEventArgs Line )
		{
			if( ( Line != null ) && ( Line.Data != null ) )
			{
				Console.WriteLine( "        " + Line.Data );
			}
		}
		public static void ErrorReceivedDataEventHandler( Object Sender, DataReceivedEventArgs Line )
		{
			if( ( Line != null ) && ( Line.Data != null ) )
			{
				Console.WriteLine( "        " + Line.Data );
			}
		}

		static void Main( string[] args )
		{
			try
			{
				BuilderDB DBConnection = new BuilderDB();
				if( DBConnection.OpenConnection() )
				{
					// The [0] variable is assumed to the variable name we're syncing to
					string VariableNameToSyncTo = args[0];
					Console.WriteLine( "    Syncing the selected portion of the depot to " + VariableNameToSyncTo );

					// Build the table of values to sync to
					Dictionary<string, string> BranchNameMap = new Dictionary<string, string>();
					Dictionary<int, string> BranchNameCache = new Dictionary<int, string>();
					Int32 BranchNameCount = 0;
					for( int i = 1; i < args.Length; i++ )
					{
						// There are two ways for the branch name to be embedded in the file name and it
						// depends on whether the file name is in depot or workspace format
						bool bValidFilenameFormat =
							args[i].StartsWith( "//depot/" ) ||
							args[i].StartsWith( Environment.CurrentDirectory );

						if( bValidFilenameFormat )
						{
							// Parse out the branch name and cache it off
							if( args[i].StartsWith( "//depot/" ) )
							{
								// Depot naming format: extract the branch name (Pathname is //depot/BranchName/FileName)
								string[] ParsedPathName = args[i].Substring( "//depot/".Length ).Split( "/".ToCharArray() );
								BranchNameCache[i] = ParsedPathName[0];
							}
							else
							{
								Debug.Assert( args[i].StartsWith( Environment.CurrentDirectory ) );

								// Workspace naming format: extract the branch name (Pathname is WorkspaceRoot\BranchName\FileName)
								string[] ParsedPathName = args[i].Substring( Environment.CurrentDirectory.Length + 1 ).Split( "\\".ToCharArray() );
								BranchNameCache[i] = ParsedPathName[0];
							}

							// See if we already have it in the map
							if( !BranchNameMap.ContainsKey( BranchNameCache[i] ) )
							{
								BranchNameCount++;

								string ValueToSyncTo = "";
								if( VariableNameToSyncTo.StartsWith("CIS_") )
								{
									// If it's a CIS variable, strip off the CIS moniker, which is no longer used
									ValueToSyncTo = DBConnection.GetCISVariable( BranchNameCache[i], VariableNameToSyncTo.Replace( "CIS_", "" ) );
								}
								else
								{
									ValueToSyncTo = DBConnection.GetVariable( BranchNameCache[i], VariableNameToSyncTo );
								}
								
								if( ValueToSyncTo != "" )
								{
									// Add it to the map
									BranchNameMap[BranchNameCache[i]] = ValueToSyncTo;
									Console.WriteLine( "    [ " + VariableNameToSyncTo + ", " + BranchNameCache[i] + " = " + ValueToSyncTo + " ]" );
								}
								else
								{
									Console.WriteLine( "    Error, couldn't find the variable \"" + VariableNameToSyncTo + "\" for branch \"" + BranchNameCache[i] + "\"" );
									Console.WriteLine( "        Either this is an unrecognized branch, or one which doesn't have that variable defined in the build system." );
								}
							}
						}
					}
					Console.Write( Environment.NewLine );

					// If we found the necessary value for each branch we're syncing, continue
					if( BranchNameCount == BranchNameMap.Keys.Count )
					{
						for( int i = 1; i < args.Length; i++ )
						{
							string ValueToSyncTo = BranchNameMap[BranchNameCache[i]];

							Console.WriteLine( "        " + args[i] );

							Process FileSyncProcess = new Process();
							FileSyncProcess.StartInfo.FileName = "p4";
							FileSyncProcess.StartInfo.Arguments = "sync " + args[i] + "@" + ValueToSyncTo;
							FileSyncProcess.StartInfo.UseShellExecute = false;
							FileSyncProcess.StartInfo.RedirectStandardOutput = true;
							FileSyncProcess.StartInfo.RedirectStandardError = true;

							FileSyncProcess.EnableRaisingEvents = true;
							FileSyncProcess.OutputDataReceived += new DataReceivedEventHandler( OutputReceivedDataEventHandler );
							FileSyncProcess.ErrorDataReceived += new DataReceivedEventHandler( ErrorReceivedDataEventHandler );

							FileSyncProcess.Start();
							FileSyncProcess.BeginOutputReadLine();
							FileSyncProcess.BeginErrorReadLine();

							FileSyncProcess.WaitForExit();
						}
					}
				}
				DBConnection.CloseConnection();
			}
			catch( Exception Ex )
			{
				Console.WriteLine( "Tool has failed with exception: " + Ex.ToString() );
			}
		}
	}
}
