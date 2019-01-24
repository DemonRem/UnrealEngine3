/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Data;
using System.Data.SqlClient;
using System.Drawing;
using System.Text;

namespace Controller
{
    public class ProcedureParameter
    {
        public string Name;
        public object Value;
        public SqlDbType Type;

        public ProcedureParameter( string InName, object InValue, SqlDbType InType )
        {
            Name = InName;
            Value = InValue;
            Type = InType;
        }
    }

    public partial class BuilderDB
    {
        private Main Parent = null;
        private SqlConnection Connection = null;
		private bool SendWarningEmailsOnDBErrors = false;
		private bool RetryAllConnections = true;
		private int RetrySleepTimeInMS = 5 * 1000;
		private int MaxRetryCount = 3;

        public BuilderDB( Main InParent )
        {
			Parent = InParent;

			if( !OpenConnection() )
			{
				// If opening the connection fails, stop the whole show
				Parent.Ticking = false;
			}
        }

		public bool OpenConnection()
		{
			try
			{
				Connection = new SqlConnection( Properties.Settings.Default.DBConnection );
				Connection.Open();

				Parent.Log( "Database connection opened successfully", Color.DarkGreen );
				return true;
			}
			catch( Exception Ex )
			{
				Parent.Log( "Database open exception: " + Ex.ToString(), Color.Red );
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
					Parent.Log( "Database connection closed successfully", Color.DarkGreen );
				}
				catch( System.Exception Ex )
				{
					Parent.Log( "Database close exception: " + Ex.ToString(), Color.Red );
				}

				Connection = null;
			}
		}

		public void Update( string CommandString )
		{
			bool TransactionCompleted = false;
			int RetryCount = 0;
			while( !TransactionCompleted )
			{
				try
				{
					SqlCommand Command = new SqlCommand( CommandString, Connection );
					Command.ExecuteNonQuery();
					TransactionCompleted = true;
				}
				catch( Exception Ex )
				{
					Parent.Log( "BuilderDB.Update exception: " + Ex.ToString(), Color.Red );
					Parent.Log( "Query: " + CommandString, Color.Red );

					// If we're retrying, sleep for a bit and loop
					if( RetryAllConnections && ( RetryCount < MaxRetryCount ) )
					{
						if( SendWarningEmailsOnDBErrors )
						{
							Parent.SendWarningMail( "Database exception, retrying", Ex.ToString(), false );
						}

						CloseConnection();

						do
						{
							System.Threading.Thread.Sleep( RetrySleepTimeInMS );
							RetryCount++;
						}
						while( !OpenConnection() && ( RetryCount < MaxRetryCount ) );
					}
					else
					{
						TransactionCompleted = true;
						if( SendWarningEmailsOnDBErrors )
						{
							Parent.SendWarningMail( "BuilderDB.Update exception", "Query: " + CommandString + "\nException: " + Ex.ToString(), false );
						}
					}
				}
			}
		}

		public int ReadInt( string Procedure, ProcedureParameter Parm0, ProcedureParameter Parm1, ProcedureParameter Parm2, ProcedureParameter Parm3 )
		{
			int Result = -1;
			bool TransactionCompleted = false;
			int RetryCount = 0;
			while( !TransactionCompleted )
			{
				try
				{
					SqlCommand Command = new SqlCommand( Procedure, Connection );
					Command.CommandType = CommandType.StoredProcedure;

					if( Parm0 != null )
					{
						Command.Parameters.Add( Parm0.Name, Parm0.Type );
						Command.Parameters[Parm0.Name].Value = Parm0.Value;

						if( Parm1 != null )
						{
							Command.Parameters.Add( Parm1.Name, Parm1.Type );
							Command.Parameters[Parm1.Name].Value = Parm1.Value;

							if( Parm2 != null )
							{
								Command.Parameters.Add( Parm2.Name, Parm2.Type );
								Command.Parameters[Parm2.Name].Value = Parm2.Value;

								if( Parm3 != null )
								{
									Command.Parameters.Add( Parm3.Name, Parm3.Type );
									Command.Parameters[Parm3.Name].Value = Parm3.Value;
								}
							}
						}
					}

					SqlDataReader DataReader = Command.ExecuteReader();

					try
					{
						if( DataReader.Read() )
						{
							Result = DataReader.GetInt32( 0 );
						}
					}
					catch( Exception Ex )
					{
						Parent.Log( "BuilderDB.ReadInt(ProcedureParameter) exception: (" + Procedure + ")" + Ex.ToString(), Color.Red );
					}

					DataReader.Close();
					TransactionCompleted = true;
				}
				catch( Exception Ex )
				{
					Parent.Log( "BuilderDB.ReadInt(ProcedureParameter) exception: (" + Procedure + ")" + Ex.ToString(), Color.Red );
					// If we're retrying, sleep for a bit and loop
					if( RetryAllConnections && ( RetryCount < MaxRetryCount ) )
					{
						if( SendWarningEmailsOnDBErrors )
						{
							Parent.SendWarningMail( "Database exception, retrying", Ex.ToString(), false );
						}

						CloseConnection();

						do
						{
							System.Threading.Thread.Sleep( RetrySleepTimeInMS );
							RetryCount++;
						}
						while( !OpenConnection() && ( RetryCount < MaxRetryCount ) );
					}
					else
					{
						TransactionCompleted = true;
						if( SendWarningEmailsOnDBErrors )
						{
							Parent.SendWarningMail( "BuilderDB.ReadInt(ProcedureParameter) exception", "Procedure: " + Procedure + "\nException: " + Ex.ToString(), false );
						}
					}
				}
			}
			return ( Result );
		}

		public void Update( string Procedure, ProcedureParameter[] Parms )
		{
			bool TransactionCompleted = false;
			int RetryCount = 0;
			while( !TransactionCompleted )
			{
				try
				{
					SqlCommand Command = new SqlCommand( Procedure, Connection );
					Command.CommandType = CommandType.StoredProcedure;

					foreach( ProcedureParameter Parm in Parms )
					{
						Command.Parameters.Add( Parm.Name, Parm.Type );
						Command.Parameters[Parm.Name].Value = Parm.Value;
					}

					Command.ExecuteNonQuery();
					TransactionCompleted = true;
				}
				catch( Exception Ex )
				{
					Parent.Log( "BuilderDB.Update exception: " + Ex.ToString(), Color.Red );

					// If we're retrying, sleep for a bit and loop
					if( RetryAllConnections && ( RetryCount < MaxRetryCount ) )
					{
						if( SendWarningEmailsOnDBErrors )
						{
							Parent.SendWarningMail( "Database exception, retrying", Ex.ToString(), false );
						}

						CloseConnection();

						do
						{
							System.Threading.Thread.Sleep( RetrySleepTimeInMS );
							RetryCount++;
						}
						while( !OpenConnection() && ( RetryCount < MaxRetryCount ) );
					}
					else
					{
						TransactionCompleted = true;
						if( SendWarningEmailsOnDBErrors )
						{
							Parent.SendWarningMail( "BuilderDB.Update exception", "Exception: " + Ex.ToString(), false );
						}
					}
				}
			}
		}

		private SqlDataReader ExecuteQuery( string Description, string Query, SqlConnection Connection )
		{
			SqlDataReader DataReader = null;
			bool TransactionCompleted = false;
			int RetryCount = 0;
			while( !TransactionCompleted )
			{
				try
				{
					SqlCommand Command = new SqlCommand( Query, Connection );
					DataReader = Command.ExecuteReader();
					TransactionCompleted = true;
				}
				catch( Exception Ex )
				{
					Parent.Log( Description + " exception: " + Ex.ToString(), Color.Red );
					Parent.Log( "Query: " + Query, Color.Red );

					// If we're retrying, sleep for a bit and loop
					if( RetryAllConnections && ( RetryCount < MaxRetryCount ) )
					{
						if( SendWarningEmailsOnDBErrors )
						{
							Parent.SendWarningMail( "Database exception, retrying", Ex.ToString(), false );
						}

						CloseConnection();

						do
						{
							System.Threading.Thread.Sleep( RetrySleepTimeInMS );
							RetryCount++;
						}
						while( !OpenConnection() && ( RetryCount < MaxRetryCount ) );
					}
					else
					{
						TransactionCompleted = true;
					}
				}
			}

			return ( DataReader );
		}

		public int ReadInt( string CommandString )
		{
			int Result = 0;
			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.ReadInt", CommandString, Connection );

			if( DataReader != null )
			{
				try
				{
					if( DataReader.Read() && !DataReader.IsDBNull( 0 ) )
					{
						Result = DataReader.GetInt32( 0 );
					}
				}
				catch( System.Exception Ex )
				{
					Parent.Log( "BuilderDB.ReadInt(" + CommandString + ") exception: " + Ex.ToString(), Color.Red );
				}

				DataReader.Close();
			}

			return ( Result );
		}

		public int ReadDecimal( string CommandString )
		{
			int Result = 0;

			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.ReadDecimal", CommandString, Connection );

			if( DataReader != null )
			{
				try
				{
					if( DataReader.Read() && !DataReader.IsDBNull( 0 ) )
					{
						Result = ( int )DataReader.GetDecimal( 0 );
					}
				}
				catch( System.Exception Ex )
				{
					Parent.Log( "BuilderDB.ReadDecimal(" + CommandString + ") exception: " + Ex.ToString(), Color.Red );
				}

				DataReader.Close();
			}

			return ( Result );
		}

		public double ReadDouble( string CommandString )
		{
			double Result = 0;

			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.ReadDouble", CommandString, Connection );

			if( DataReader != null )
			{
				try
				{
					if( DataReader.Read() && !DataReader.IsDBNull( 0 ) )
					{
						Result = ( double )DataReader.GetDecimal( 0 );
					}
				}
				catch( System.Exception Ex )
				{
					Parent.Log( "BuilderDB.ReadDouble(" + CommandString + ") exception: " + Ex.ToString(), Color.Red );
				}

				DataReader.Close();
			}

			return ( Result );
		}

		public bool ReadBool( string CommandString )
		{
			bool Result = false;

			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.ReadBool", CommandString, Connection );

			if( DataReader != null )
			{
				try
				{
					if( DataReader.Read() && !DataReader.IsDBNull( 0 ) )
					{
						Result = DataReader.GetBoolean( 0 );
					}
				}
				catch( System.Exception Ex )
				{
					Parent.Log( "BuilderDB.ReadBool(" + CommandString + ") exception: " + Ex.ToString(), Color.Red );
				}

				DataReader.Close();
			}

			return ( Result );
		}

		public string ReadString( string CommandString )
		{
			string Result = "";

			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.ReadString", CommandString, Connection );

			if( DataReader != null )
			{
				try
				{
					if( DataReader.Read() && !DataReader.IsDBNull( 0 ) )
					{
						Result = DataReader.GetString( 0 );
					}
				}
				catch( System.Exception Ex )
				{
					Parent.Log( "BuilderDB.ReadString(" + CommandString + ") exception: " + Ex.ToString(), Color.Red );
				}

				DataReader.Close();
			}

			return ( Result );
		}

		public DateTime ReadDateTime( string CommandString )
		{
			DateTime Result = DateTime.UtcNow;

			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.ReadDateTime", CommandString, Connection );

			if( DataReader != null )
			{
				try
				{
					if( DataReader.Read() && !DataReader.IsDBNull( 0 ) )
					{
						Result = DataReader.GetDateTime( 0 );
					}
				}
				catch( System.Exception Ex )
				{
					Parent.Log( "BuilderDB.ReadDateTime(" + CommandString + ") exception: " + Ex.ToString(), Color.Red );
				}

				DataReader.Close();
			}

			return ( Result );
		}

		public List<string> GetPerforceServers()
		{
			List<string> Servers = new List<string>();

			string Query = "SELECT DISTINCT Server FROM BranchConfig";
			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.GetPerforceServers", Query, Connection );

			if( DataReader != null )
			{
				try
				{
					while( DataReader.Read() )
					{
						Servers.Add( DataReader.GetString( 0 ) );
					}
				}
				catch( Exception Ex )
				{
					Parent.Log( "BuilderDB.GetPerforceServers exception: " + Ex.ToString(), Color.Red );
				}

				DataReader.Close();
			}

			return ( Servers );
		}

		public Main.BranchDefinition GetBranchDefinition( string Branch )
		{
			Main.BranchDefinition BranchDef = null;

			string Query = "SELECT Branch, Server, DirectXVersion, XDKVersion, PS3SDKVersion, PFXSubjectName FROM BranchConfig WHERE ( Branch = '" + Branch + "' )";
			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.GetBranchDefinition", Query, Connection );

			if( DataReader != null )
			{
				try
				{
					if( DataReader.Read() )
					{
						BranchDef = new Main.BranchDefinition();

						BranchDef.Branch = DataReader.GetString( 0 );
						BranchDef.Server = DataReader.GetString( 1 );
						BranchDef.DirectXVersion = DataReader.GetString( 2 );
						BranchDef.XDKVersion = DataReader.GetString( 3 );
						BranchDef.PS3SDKVersion = DataReader.GetString( 4 );
						BranchDef.PFXSubjectName = DataReader.GetString( 5 );
					}
				}
				catch( Exception Ex )
				{
					Parent.Log( "BuilderDB.GetBranchDefinition exception: " + Ex.ToString(), Color.Red );
					BranchDef = null;
				}

				DataReader.Close();
			}

			return ( BranchDef );
		}

		public void FindSuppressedJobs( ScriptParser Builder, long JobSpawnTime )
		{
			string Query = "SELECT Platform, Game, Config FROM Jobs WHERE ( SpawnTime = " + JobSpawnTime.ToString() + " AND Suppressed = 1 )";
			SqlDataReader DataReader = ExecuteQuery( "BuilderDB.FindSuppressedJobs", Query, Connection );

			if( DataReader != null )
			{
				try
				{
					while( DataReader.Read() )
					{
						string Platform = DataReader.GetString( 0 );
						string Game = DataReader.GetString( 1 );
						string Config = DataReader.GetString( 2 );

						GameConfig GameInfo = new GameConfig( Game, Platform, Config, null, true, true );
						Builder.LabelInfo.FailedGames.Add( GameInfo );
					}
				}
				catch( Exception Ex )
				{
					Parent.Log( "BuilderDB.FindSuppressedJobs exception: " + Ex.ToString(), Color.Red );
				}

				DataReader.Close();
			}
		}

        public string GetVariable( string Branch, string Var )
        {
            string Query = "SELECT Value FROM Variables WHERE ( Variable = '" + Var + "' AND Branch = '" + Branch + "' )";
            string Result = ReadString( Query );
            return ( Result );
        }

        public void SetLastAttemptedBuild( int CommandID, int ChangeList, DateTime Time )
        {
            if( CommandID != 0 )
            {
                if( ChangeList != 0 )
                {
                    SetInt( "Commands", CommandID, "LastAttemptedChangeList", ChangeList );
                }

                SetDateTime( "Commands", CommandID, "LastAttemptedDateTime", Time );
            }
        }

        public void SetLastFailedBuild( int CommandID, int ChangeList, DateTime Time )
        {
            if( CommandID != 0 )
            {
                if( ChangeList != 0 )
                {
                    SetInt( "Commands", CommandID, "LastFailedChangeList", ChangeList );
                }

                SetDateTime( "Commands", CommandID, "LastFailedDateTime", Time );
            }
        }

        public void SetLastGoodBuild( int CommandID, int ChangeList, DateTime Time )
        {
            if( CommandID != 0 )
            {
                SetInt( "Commands", CommandID, "LastGoodChangeList", ChangeList );
                SetDateTime( "Commands", CommandID, "LastGoodDateTime", Time );
            }
        }

        public void Trigger( int CurrentCommandID, string BuildDescription )
        {
            string Query = "SELECT ID FROM Commands WHERE ( Description = '" + BuildDescription + "' )";
            int CommandID = ReadInt( Query );

			if( CommandID != 0 )
			{
				Query = "SELECT Machine FROM Commands WHERE ( ID = " + CommandID.ToString() + " )";
				string Machine = ReadString( Query );

				if( Machine.Length == 0 )
				{
					Query = "SELECT Operator FROM Commands WHERE ( ID = " + CurrentCommandID.ToString() + " )";
					string User = ReadString( Query );

					Query = "UPDATE Commands SET Pending = '1', Operator = '" + User + "' WHERE ( ID = " + CommandID.ToString() + " )";
					Update( Query );
				}
				else
				{
					Parent.Log( " ... suppressing retrigger of '" + BuildDescription + "'", Color.Magenta );
					string Operator = GetString( "Commands", CommandID, "Operator" );
					Parent.SendAlreadyInProgressMail( Operator, CommandID, BuildDescription );
				}
			}
			else
			{
				Parent.SendErrorMail( "Build '" + BuildDescription + "' does not exist!", "" );
			}
        }

        public void Trigger( int CommandID )
        {
            string Query = "SELECT Operator FROM Commands WHERE ( ID = " + CommandID.ToString() + " )";
            string User = ReadString( Query );

            Query = "UPDATE Commands SET Pending = '1', Operator = '" + User + "' WHERE ( ID = " + CommandID.ToString() + " )";
            Update( Query );
        }

        public int PollForKillBuild()
        {
            string QueryString = "SELECT ID FROM Commands WHERE ( Machine = '" + Parent.MachineName + "' AND Killing = 1 )";
            int ID = ReadInt( QueryString );
            return ( ID );
        }

        public int PollForKillJob()
        {
            string QueryString = "SELECT ID FROM Jobs WHERE ( Machine = '" + Parent.MachineName + "' AND Killing = 1 AND Complete = 0 )";
            int ID = ReadInt( QueryString );
            return ( ID );
        }

        public int PollForBuild( string MachineName, bool PrimaryBuildsOnly )
        {
            int ID;

            // Check for machine locked timed build
            ProcedureParameter ParmMachine = new ProcedureParameter( "MachineName", MachineName, SqlDbType.VarChar );
            ProcedureParameter ParmLocked = new ProcedureParameter( "AnyMachine", false, SqlDbType.Bit );
            ProcedureParameter ParmPrimary = new ProcedureParameter( "PrimaryBuild", PrimaryBuildsOnly, SqlDbType.Bit );
            ID = ReadInt( "CheckTimedBuild2", ParmMachine, ParmLocked, ParmPrimary, null );
            if( ID >= 0 )
            {
                // Check for success
                if( ID != 0 )
                {
                    SetString( "Commands", ID, "Operator", "AutoTimer" );
                    return ( ID );
                }

                // Check for a machine locked triggered build
                ID = ReadInt( "CheckTriggeredBuild2", ParmMachine, ParmLocked, ParmPrimary, null );
                if( ID >= 0 )
                {
                    // Check for success
                    if( ID != 0 )
                    {
                        return ( ID );
                    }

                    // If this machine locked to a build then don't allow it to grab normal ones
                    if( Parent.MachineLock != 0 )
                    {
                        return ( 0 );
                    }

                    // Poll for a timed build to be grabbed by anyone
                    ParmLocked = new ProcedureParameter( "AnyMachine", true, SqlDbType.Bit );
                    ID = ReadInt( "CheckTimedBuild2", ParmMachine, ParmLocked, ParmPrimary, null );
                    if( ID >= 0 )
                    {
                        // Check for success
                        if( ID != 0 )
                        {
                            SetString( "Commands", ID, "Operator", "AutoTimer" );
                            return ( ID );
                        }

                        // Poll for a triggered build
                        ID = ReadInt( "CheckTriggeredBuild2", ParmMachine, ParmLocked, ParmPrimary, null );
                    }
                }
            }
            return ( ID );
        }

        public int PollForJob( string MachineName, bool PrimaryBuildsOnly )
        {
            int ID;

            // Poll for a job
            ProcedureParameter ParmMachine = new ProcedureParameter( "MachineName", MachineName, SqlDbType.VarChar );
            ProcedureParameter ParmPrimary = new ProcedureParameter( "PrimaryBuild", PrimaryBuildsOnly, SqlDbType.Bit );
            ID = ReadInt( "CheckJob2", ParmMachine, ParmPrimary, null, null );
            return ( ID );
        }

		public void KillAssociatedJobs( string LabelName )
		{
			string Query = "UPDATE Jobs SET Killing = 1 WHERE ( Label = '" + LabelName + "' )";
			Update( Query );
		}

        // The passed in CommandID wants to copy to the network - can it?
        public bool AvailableBandwidth( int CommandID )
        {
            StringBuilder Query = new StringBuilder();

            string Check = "SELECT ID FROM Commands WHERE ( ConchHolder is not NULL )";
            int ID = ReadInt( Check );
            if( ID != 0 )
            {
                Check = "SELECT ConchHolder FROM Commands WHERE ( ID = " + ID.ToString() + " )";
                DateTime Start = ReadDateTime( Check );
                // 60 minute timeout
                if( DateTime.UtcNow - Start > new TimeSpan( 0, 60, 0 ) )
                {
                    Delete( "Commands", ID, "ConchHolder" );
                }

                // No free slots
                return ( false );
            }

            // Atomically set the ConchHolder time
            if( CommandID != 0 )
            {
                ProcedureParameter Parm = new ProcedureParameter( "CommandID", CommandID, SqlDbType.Int );
                return ( ReadInt( "SetConchTime", Parm, null, null, null ) == 0 );
            }

            return ( false );
        }

        public int GetInt( string TableName, int ID, string Command )
        {
            int Result = 0;

            if( ID != 0 )
            {
                string Query = "SELECT " + Command + " FROM " + TableName + " WHERE ( ID = " + ID.ToString() + " )";
                Result = ReadInt( Query );
            }

            return ( Result );
        }

        public bool GetBool( string TableName, int ID, string Command )
        {
            bool Result = false;

            if( ID != 0 )
            {
                string Query = "SELECT " + Command + " FROM " + TableName + " WHERE ( ID = " + ID.ToString() + " )";
                Result = ReadBool( Query );
            }

            return ( Result );
        }

        public string GetString( string TableName, int ID, string Command )
        {
            string Result = "";

            if( ID != 0 )
            {
                string Query = "SELECT " + Command + " FROM " + TableName + " WHERE ( ID = " + ID.ToString() + " )";
                Result = ReadString( Query );
            }

            return ( Result );
        }

		public string GetStringForBranch( string Branch, string Command )
		{
			string Query = "SELECT " + Command + " FROM BranchConfig WHERE ( Branch = '" + Branch + "' )";
			string Result = ReadString( Query );

			return ( Result );
		}

		public int GetIntForBranch( string Branch, string Command )
		{
			string Query = "SELECT " + Command + " FROM BranchConfig WHERE ( Branch = '" + Branch + "' )";
			int Result = ReadInt( Query );

			return ( Result );
		}

		public string GetStringForServer( string Server, string Command )
		{
			string Query = "SELECT " + Command + " FROM BranchConfig WHERE ( Server = '" + Server + "' )";
			string Result = ReadString( Query );

			return ( Result );
		}

        public DateTime GetDateTime( string TableName, int ID, string Command )
        {
            DateTime Result = DateTime.UtcNow;

            if( ID != 0 )
            {
                string Query = "SELECT " + Command + " FROM " + TableName + " WHERE ( ID = " + ID.ToString() + " )";
                Result = ReadDateTime( Query );
            }

            return ( Result );
        }

        public void SetString( string TableName, int ID, string Field, string Info )
        {
            if( ID != 0 )
            {
                Info = Info.Replace( "'", "" );

                string Command = "UPDATE " + TableName + " SET " + Field + " = '" + Info + "' WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }

        public void SetInt( string TableName, int ID, string Field, int Info )
        {
            if( ID != 0 )
            {
                string Command = "UPDATE " + TableName + " SET " + Field + " = " + Info.ToString() + " WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }

		public void SetIntForBranch( string Branch, string Field, int Changelist )
		{
			string Command = "Update BranchConfig SET " + Field + " = " + Changelist.ToString() + " WHERE ( Branch = '" + Branch + "' )";
			Update( Command );
		}

        public void SetBool( string TableName, int ID, string Field, bool BoolInfo )
        {
            if( ID != 0 )
            {
                int Info = 0;
                if( BoolInfo )
                {
                    Info = 1;
                }
                string Command = "UPDATE " + TableName + " SET " + Field + " = " + Info.ToString() + " WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }

        public void SetDateTime( string TableName, int ID, string Field, DateTime Time )
        {
            if( ID != 0 )
            {
                string Command = "UPDATE " + TableName + " SET " + Field + " = '" + Time.ToString( Main.US_TIME_DATE_STRING_FORMAT ) + "' WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }

        public void Delete( string TableName, int ID, string Field )
        {
            if( ID != 0 )
            {
                string Command = "UPDATE " + TableName + " SET " + Field + " = null WHERE ( ID = " + ID.ToString() + " )";
                Update( Command );
            }
        }

        public void WritePerformanceData( string MachineName, string KeyName, long Value )
        {
			if( KeyName.Length == 0 || Value <= 0 )
			{
				return;
			}

            ProcedureParameter[] Parms = 
            {
                new ProcedureParameter( "CounterName", KeyName, SqlDbType.Char ),
                new ProcedureParameter( "MachineName", MachineName, SqlDbType.Char ),
                new ProcedureParameter( "AppName", "Controller", SqlDbType.Char ),
                new ProcedureParameter( "IntValue", Value, SqlDbType.BigInt ),
                new ProcedureParameter( "DateTimeStamp", DateTime.UtcNow, SqlDbType.DateTime )
            };

            Update( "CreatePerformanceData", Parms );
        }
    }
}
