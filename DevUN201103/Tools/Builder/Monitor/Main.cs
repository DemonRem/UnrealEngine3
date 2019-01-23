/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Management;
using System.DirectoryServices;

namespace Monitor
{
	public partial class Main : Form
	{
		///////////////////////////////////////////////////////////////////////

		// If it's ticking, we're still kicking 
		public bool Ticking = true;
		public bool Restart = false;
		private string MachineName = "";
		private string LoggedOnUser = "";

		// The primary builder database connection
		private SqlConnection BuilderDBConnection = null;

		// The next scheduled time we'll update the database
		DateTime NextUpdateCheck = DateTime.UtcNow;

		// Time periods, in seconds, between various checks
		uint UpdateCheckPeriod = 60;

		delegate void DelegateAddLine(string Line, Color TextColor);

#if DO_MORE_SOPHISTICATED_TRACKING
		// For consistent formatting to the US standard (05/29/2008 11:33:52)
		public const string US_TIME_DATE_STRING_FORMAT = "MM/dd/yyyy HH:mm:ss";

		// Our locally maintained table of pending builds
		private class PendingEntry
		{
			public PendingEntry(int InID, string InDescription)
			{
				ID = InID;
				TimePending = 0;
				Description = InDescription;
				LastUpdated = DateTime.UtcNow;
			}

			public int ID;
			public int TimePending;
			public string Description;
			public DateTime LastUpdated;
		};
		Dictionary<int, PendingEntry> PendingBuilds = new Dictionary<int, PendingEntry>();
#endif
		///////////////////////////////////////////////////////////////////////

		public Main()
		{
			InitializeComponents();

			MainWindow_SizeChanged(null, null);
		}

		public void Init()
		{
			// Show log window
			Show();

			// Initialize our connection with the builder database
			try
			{
				string ConnectionString = Properties.Settings.Default.DBConnection;
				BuilderDBConnection = new SqlConnection(ConnectionString);
				BuilderDBConnection.Open();

				Log("[STATUS] Connected to database OK", Color.DarkGreen);
			}
			catch
			{
				Log("[STATUS] Database connection FAILED", Color.Red);
				Ticking = false;
				BuilderDBConnection = null;
			}

			// Get system and user information
			GetInfo();
		}

		private void GetInfo()
		{
			ManagementObjectSearcher Searcher = new ManagementObjectSearcher("Select * from Win32_ComputerSystem");
			ManagementObjectCollection Collection = Searcher.Get();

			foreach (ManagementObject Object in Collection)
			{
				Object Value;

				Value = Object.GetPropertyValue("UserName");
				if (Value != null)
				{
					LoggedOnUser = Value.ToString();
				}

				Value = Object.GetPropertyValue("Name");
				if (Value != null)
				{
					MachineName = Value.ToString();
				}

				Log("Welcome \"" + LoggedOnUser + "\" running on \"" + MachineName + "\"", Color.Blue);
				break;
			}
		}

		public void Destroy()
		{
			if (BuilderDBConnection != null)
			{
				Log("[STATUS] Closing database connection", Color.DarkGreen);
				BuilderDBConnection.Close();
			}
		}

		private void MainWindow_SizeChanged(object sender, EventArgs e)
		{
			System.Drawing.Size logSize = new Size();
			logSize.Height = this.TextBox_Log.Parent.Size.Height - 55;
			logSize.Width = this.TextBox_Log.Parent.Size.Width - 10;
			this.TextBox_Log.Size = logSize;
		}

		private void Main_FormClosed(object sender, FormClosedEventArgs e)
		{
			Ticking = false;
		}

		public void Log(string Line, Color TextColour)
		{
			if (Line == null || !Ticking)
			{
				return;
			}

			// If we need to, invoke the delegate
			if (InvokeRequired)
			{
				Invoke(new DelegateAddLine(Log), new object[] { Line, TextColour });
				return;
			}

			DateTime Now = DateTime.UtcNow;
			string FullLine = DateTime.Now.ToString( "HH:mm:ss" ) + ": " + Line;

			TextBox_Log.Focus();
			TextBox_Log.SelectionLength = 0;

			// Only set the color if it is different than the foreground color
			if (TextBox_Log.SelectionColor != TextColour)
			{
				TextBox_Log.SelectionColor = TextColour;
			}

			TextBox_Log.AppendText(FullLine + Environment.NewLine);
		}

		public void Log(Array Lines, Color TextColour)
		{
			foreach (string Line in Lines)
			{
				Log(Line, TextColour);
			}
		}

#if DO_MORE_SOPHISTICATED_TRACKING
		private void UpdatePendingStatusTable(DateTime StartedUpdating)
		{
			Log("[STATUS] Updating pending status tables", Color.Black);

			string PendingQuery = "SELECT ID, Description FROM [Commands] WHERE ( Pending = 'False' )";
			try
			{
				SqlCommand Command = new SqlCommand(PendingQuery, BuilderDBConnection);
				SqlDataReader DataReader = Command.ExecuteReader();
				try
				{
					int BuildID;
					string BuildDescription;
					while (DataReader.Read())
					{
						// Get the next result
						BuildID = DataReader.GetInt32(0);
						BuildDescription = DataReader.GetString(1);
						
						// Update our local table
						PendingEntry ExistingValue;
						if (PendingBuilds.TryGetValue(BuildID, out ExistingValue))
						{
							// Update the existing entry
							ExistingValue.TimePending += Properties.Settings.Default.UpdatePeriod;
							ExistingValue.LastUpdated = StartedUpdating;
						}
						else
						{
							// Create a new entry for the table
							PendingBuilds.Add(BuildID, new PendingEntry(BuildID, BuildDescription));
						}
					}
				}
				catch
				{
				}
				DataReader.Close();
			}
			catch
			{
				Log("[ERROR] Builder Database reading " + PendingQuery, Color.Red);
			}
		}

		private void ReportCurrentState(DateTime StartedUpdating)
		{
			Dictionary<int, PendingEntry>.ValueCollection Values = PendingBuilds.Values;
			Dictionary<int, PendingEntry>.ValueCollection.Enumerator ValuesEnumerator = Values.GetEnumerator();
			if (Values.Count > 0)
			{
				while (ValuesEnumerator.MoveNext())
				{
					// For each entry, determine if it's still active and log
					// an appropriate entry for each case
					PendingEntry CurrentEntry = ValuesEnumerator.Current;
					string LogString = "";
					if (CurrentEntry.LastUpdated > StartedUpdating)
					{
						// This entry was updated during the most recent check
						LogString += "[STATUS] Build " + CurrentEntry.ID.ToString();
						LogString += " (" + CurrentEntry.Description.ToString() + ") ";
						LogString += "has been waiting for about " + CurrentEntry.TimePending.ToString() + " seconds";
					}


					Log(LogString, Color.Black);
				}
			}
		}
#endif

		private int ReadInt(string CommandString)
		{
			int Result = 0;
			try
			{
				SqlCommand Command = new SqlCommand(CommandString, BuilderDBConnection);
				SqlDataReader DataReader = Command.ExecuteReader();
				try
				{
					if (DataReader.Read())
					{
						Result = DataReader.GetInt32(0);
					}
				}
				catch
				{
				}
				DataReader.Close();
			}
			catch
			{
				Log("[ERROR] Database reading INT [" + CommandString + "]", Color.Red);
			}

			return (Result);
		}

		private double ReadDouble(string CommandString)
		{
			double Result = 0;
			try
			{
				SqlCommand Command = new SqlCommand(CommandString, BuilderDBConnection);
				SqlDataReader DataReader = Command.ExecuteReader();
				try
				{
					if (DataReader.Read())
					{
						Result = DataReader.GetDouble(0);
					}
				}
				catch
				{
				}
				DataReader.Close();
			}
			catch
			{
				Log("[ERROR] Database reading DOUBLE [" + CommandString + "]", Color.Red);
			}

			return (Result);
		}

		public bool ReadBool(string CommandString)
		{
			bool Result = false;
			try
			{
				SqlCommand Command = new SqlCommand(CommandString, BuilderDBConnection);
				SqlDataReader DataReader = Command.ExecuteReader();
				try
				{
					if (DataReader.Read())
					{
						Result = DataReader.GetBoolean(0);
					}
				}
				catch
				{
				}
				DataReader.Close();
			}
			catch
			{
				Log("[ERROR] Database reading BOOL [" + CommandString + "]", Color.Red);
			}

			return (Result);
		}

		public string ReadString(string CommandString)
		{
			string Result = "";
			try
			{
				SqlCommand Command = new SqlCommand(CommandString, BuilderDBConnection);
				SqlDataReader DataReader = Command.ExecuteReader();
				try
				{
					if (DataReader.Read())
					{
						Result = DataReader.GetString(0);
					}
				}
				catch
				{
				}
				DataReader.Close();
			}
			catch
			{
				Log("[ERROR] Database reading STRING [" + CommandString + "]", Color.Red);
			}

			return (Result);
		}

		protected void CheckForShutdown()
		{
			string CommandString = "SELECT Value FROM [Variables] WHERE ( Variable = 'StatusMessage' )";
			string Message = ReadString(CommandString);

			if (Message.Length > 0)
			{
				// Just exit - don't restart
				Ticking = false;
			}
		}

		private bool RecursiveGroupCheck(DirectoryEntry RootDirEntry, string Group, string UserName)
		{
			string Query = "(&(objectCategory=user)(memberOf=CN=" + Group + ",OU=Security Groups,DC=epicgames,DC=net)(CN=" + UserName + "))";
			DirectorySearcher DirSearcher = new DirectorySearcher(RootDirEntry, Query);
			// 500ms timeout
			DirSearcher.ClientTimeout = new TimeSpan(500 * 10000);

			if (DirSearcher.FindAll().Count > 0)
			{
				return (true);
			}

			Query = "(&(objectCategory=group)(memberOf=CN=" + Group + ",OU=Security Groups,DC=epicgames,DC=net))";
			DirSearcher = new DirectorySearcher(RootDirEntry, Query);

			foreach (SearchResult ChildGroup in DirSearcher.FindAll())
			{
				if (RecursiveGroupCheck(RootDirEntry, (string)ChildGroup.Properties["CN"][0], UserName))
				{
					return (true);
				}
			}

			return (false);
		}

		public class ProcedureParameter
		{
			public string Name;
			public object Value;
			public SqlDbType Type;
			public int Size;

			public ProcedureParameter(string InName, object InValue, SqlDbType InType, int InSize)
			{
				Name = InName;
				Value = InValue;
				Type = InType;
				Size = InSize;
			}
		}

		private void Update(string Procedure, ProcedureParameter[] Parms)
		{
			try
			{
				SqlCommand Command = new SqlCommand(Procedure, BuilderDBConnection);
				Command.CommandType = CommandType.StoredProcedure;

				foreach (ProcedureParameter Parm in Parms)
				{
					Command.Parameters.Add(Parm.Name, Parm.Type);
					Command.Parameters[Parm.Name].Value = Parm.Value;
				}

				Command.ExecuteNonQuery();
			}
			catch
			{
				Log("[ERROR] During Update called from procedure '" + Procedure + "'", Color.Red);
			}
		}

		private void WritePerformanceData(string MachineName, string KeyName, int Value)
		{
			ProcedureParameter[] Parms = 
			{
				new ProcedureParameter( "CounterName", KeyName, SqlDbType.Char, 64 ),
				new ProcedureParameter( "MachineName", MachineName, SqlDbType.Char, 64 ),
				new ProcedureParameter( "AppName", "Controller", SqlDbType.Char, 64 ),
				new ProcedureParameter( "IntValue", Value, SqlDbType.BigInt, 8 ),
				new ProcedureParameter( "DateTimeStamp", DateTime.UtcNow, SqlDbType.DateTime, 0 )
			};

			Update("CreatePerformanceData", Parms);
		}

		public void MonitorAndUpdateDatabase()
		{
#if ADDITIONAL_LOGGING
			Log("[STATUS] Monitoring and updating database", Color.Black);
#endif

			// Construct all of the query strings
			string ActivePrimaryJobsCountQuery = "SELECT COUNT(*) FROM [Jobs] WHERE ( Active = 1 AND Complete = 0 AND PrimaryBuild = 1 AND Killing = 0 AND BuildLogID > 0 )";
			string PendingPrimaryJobsCountQuery = "SELECT COUNT(*) FROM [Jobs] WHERE ( Active = 0 AND Complete = 0 AND PrimaryBuild = 1 AND Killing = 0 AND BuildLogID = 0 )";
			string ActiveNonPrimaryJobsCountQuery = "SELECT COUNT(*) FROM [Jobs] WHERE ( Active = 1 AND Complete = 0 AND PrimaryBuild = 0 AND Killing = 0 AND BuildLogID > 0 )";
			string PendingNonPrimaryJobsCountQuery = "SELECT COUNT(*) FROM [Jobs] WHERE ( Active = 0 AND Complete = 0 AND PrimaryBuild = 0 AND Killing = 0 AND BuildLogID = 0 )";

			string ActivePrimaryBuildsCountQuery = "SELECT COUNT(*) FROM [Commands] WHERE ( Command != 'Test/Default' AND PrimaryBuild = 1 AND Pending = 0 AND Killing = 0 AND BuildLogID IS NOT NULL )";
			string PendingPrimaryBuildsCountQuery = "SELECT COUNT(*) FROM [Commands] WHERE ( Command != 'Test/Default' AND PrimaryBuild = 1 AND Pending = 1 AND Killing = 0 AND BuildLogID IS NULL )";
			string ActiveNonPrimaryBuildsCountQuery = "SELECT COUNT(*) FROM [Commands] WHERE ( Command != 'Test/Default' AND PrimaryBuild = 0 AND Pending = 0 AND Killing = 0 AND BuildLogID IS NOT NULL )";
			string PendingNonPrimaryBuildsCountQuery = "SELECT COUNT(*) FROM [Commands] WHERE ( Command != 'Test/Default' AND PrimaryBuild = 0 AND Pending = 1 AND Killing = 0 AND BuildLogID IS NULL )";

			string PerfQueryFormat = "SELECT AVG({0}) FROM (SELECT {0}, Restart FROM [Builders] WHERE (State = 'Building') OR (State = 'Connected')) AS Expr1 WHERE (Restart IS NULL)";
			string AvgCPUBusyQuery = String.Format(PerfQueryFormat, "CPUBusy");
			string AvgDiskReadLatencyQuery = String.Format(PerfQueryFormat, "DiskReadLatency");
			string AvgDiskWriteLatencyQuery = String.Format(PerfQueryFormat, "DiskWriteLatency");
			string AvgDiskTransferLatencyQuery = String.Format(PerfQueryFormat, "DiskTransferLatency");
			string AvgDiskQueueLengthQuery = String.Format(PerfQueryFormat, "DiskQueueLength");
			string AvgDiskReadQueueLengthQuery = String.Format(PerfQueryFormat, "DiskReadQueueLength");
			string AvgSystemMemoryAvailableQuery = String.Format(PerfQueryFormat, "SystemMemoryAvailable");

			// Issue all of the database queries
			int ActivePrimaryJobsCount = ReadInt( ActivePrimaryJobsCountQuery );
			int PendingPrimaryJobsCount = ReadInt( PendingPrimaryJobsCountQuery );
			int ActiveNonPrimaryJobsCount = ReadInt( ActiveNonPrimaryJobsCountQuery );
			int PendingNonPrimaryJobsCount = ReadInt( PendingNonPrimaryJobsCountQuery );

			int ActivePrimaryBuildsCount = ReadInt( ActivePrimaryBuildsCountQuery );
			int PendingPrimaryBuildsCount = ReadInt( PendingPrimaryBuildsCountQuery );
			int ActiveNonPrimaryBuildsCount = ReadInt( ActiveNonPrimaryBuildsCountQuery );
			int PendingNonPrimaryBuildsCount = ReadInt( PendingNonPrimaryBuildsCountQuery );

			int AvgCPUBusy = (int)ReadDouble(AvgCPUBusyQuery);

			// The following results are in seconds, convert to milliseconds
			int AvgDiskReadLatency = (int)(ReadDouble(AvgDiskReadLatencyQuery) * 1000.0);
			int AvgDiskWriteLatency = (int)(ReadDouble(AvgDiskWriteLatencyQuery) * 1000.0);
			int AvgDiskTransferLatency = (int)(ReadDouble(AvgDiskTransferLatencyQuery) * 1000.0);
			
			// Generally small, scale by a factor of 100 to tease out some resolution
			int AvgDiskQueueLength = (int)(ReadDouble(AvgDiskQueueLengthQuery) * 100.0);
			int AvgDiskReadQueueLength = (int)(ReadDouble(AvgDiskReadQueueLengthQuery) * 100.0);
			
			// Store the value in MBs
			int AvgSystemMemoryAvailable = (int)(ReadDouble(AvgSystemMemoryAvailableQuery) / (double)(1024 * 1024));


			// Special signals to generate by hand, not via the system
			int RGCPassed = 0;
			if (RecursiveGroupCheck(new DirectoryEntry(), "UnrealProp Admins", "Derek Cornish") == true)
			{
				RGCPassed = 1;
			}

#if ADDITIONAL_LOGGING
			// Report what we've found this time around
			Log("[STATUS]     Primary Builds:", Color.Black);
			Log("[STATUS]         Active  = " + ActivePrimaryBuildsCount.ToString(), Color.Black);
			Log("[STATUS]         Pending = " + PendingPrimaryBuildsCount.ToString(), Color.Black);
			Log("[STATUS]     Non-primary Builds:", Color.Black);
			Log("[STATUS]         Active  = " + ActiveNonPrimaryBuildsCount.ToString(), Color.Black);
			Log("[STATUS]         Pending = " + PendingNonPrimaryBuildsCount.ToString(), Color.Black);
#endif

#if COLLECT_BUILD_MACHINE_STATS
			string BuildingBuildersCountQuery = "SELECT COUNT(*) FROM [Builders] WHERE ( State = 'Building' )";
			string ConnectedBuildersCountQuery = "SELECT COUNT(*) FROM [Builders] WHERE ( State = 'Connected' )";
			string ZombiedBuildersCountQuery = "SELECT COUNT(*) FROM [Builders] WHERE ( State = 'Zombied' )";
			string DeadBuildersCountQuery = "SELECT COUNT(*) FROM [Builders] WHERE ( State = 'Dead' )";

			int BuildingBuildersCount = ReadInt(BuildingBuildersCountQuery);
			int ConnectedBuildersCount = ReadInt(ConnectedBuildersCountQuery);
			int ZombiedBuildersCount = ReadInt(ZombiedBuildersCountQuery);
			int DeadBuildersCount = ReadInt(DeadBuildersCountQuery);

			Log("[STATUS]     Builders:", Color.Black);
			Log("[STATUS]         Alive = " + (BuildingBuildersCount + ConnectedBuildersCount).ToString(), Color.Black);
			Log("[STATUS]             Building  = " + BuildingBuildersCount.ToString(), Color.Black);
			Log("[STATUS]             Connected = " + ConnectedBuildersCount.ToString(), Color.Black);
			Log("[STATUS]         Zombied = " + ZombiedBuildersCount.ToString(), Color.Black);
			Log("[STATUS]         Dead = " + DeadBuildersCount.ToString(), Color.Black);
#endif
			// Push the results to the performance counter data table
			WritePerformanceData( MachineName, "ActivePrimaryBuilds", ActivePrimaryBuildsCount + ActivePrimaryJobsCount );
			WritePerformanceData( MachineName, "PendingPrimaryBuilds", PendingPrimaryBuildsCount + PendingPrimaryJobsCount );
			WritePerformanceData( MachineName, "ActiveNonPrimaryBuilds", ActiveNonPrimaryBuildsCount + ActiveNonPrimaryJobsCount );
			WritePerformanceData( MachineName, "PendingNonPrimaryBuilds", PendingNonPrimaryBuildsCount + PendingNonPrimaryJobsCount );

			WritePerformanceData( MachineName, "AvgBuildingCPUBusy", AvgCPUBusy );
			WritePerformanceData( MachineName, "AvgBuildingDiskReadLatency", AvgDiskReadLatency );
			WritePerformanceData( MachineName, "AvgBuildingDiskWriteLatency", AvgDiskWriteLatency );
			WritePerformanceData( MachineName, "AvgBuildingDiskTransferLatency", AvgDiskTransferLatency );
			WritePerformanceData( MachineName, "AvgBuildingDiskQueueLength", AvgDiskQueueLength );
			WritePerformanceData( MachineName, "AvgBuildingDiskReadQueueLength", AvgDiskReadQueueLength );
			WritePerformanceData( MachineName, "AvgBuildingSystemMemoryAvailable", AvgSystemMemoryAvailable );

			WritePerformanceData(MachineName, "RecursiveGroupCheck", RGCPassed);

#if DO_MORE_SOPHISTICATED_TRACKING
			// Gather all of the required information from the Commands table
			UpdatePendingStatusTable(StartedUpdating);

			// Garbage collect our stats while updating the PerformanceData
			ReportCurrentState(StartedUpdating);
#endif
		}

		public void Run()
		{
			if( Ticking && ( DateTime.UtcNow >= NextUpdateCheck ) )
			{
				NextUpdateCheck = DateTime.UtcNow.AddSeconds( UpdateCheckPeriod );
				MonitorAndUpdateDatabase();
			}
		}
	}
}