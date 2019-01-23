/*
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using P4API;
using P4Core;

namespace P4Inquiry
{
	/// <summary>
	/// Class providing a light wrapper around the P4.NET API; designed to simplify querying for changelist
	/// and label information
	/// </summary>
	public class P4Inquirer
	{
		#region Enumerations
		/// <summary>
		/// Enumeration signifying connection status to the Perforce server
		/// </summary>
		public enum EP4ConnectionStatus
		{
			/// <summary>
			/// Connected properly to the server
			/// </summary>
			P4CS_Connected,

			/// <summary>
			/// Disconnected from the server
			/// </summary>
			P4CS_Disconnected,

			/// <summary>
			/// Encountered a log-in error during the last connection attempt
			/// </summary>
			P4CS_LoginError,

			/// <summary>
			/// Encountered some other error during the last connection attempt
			/// </summary>
			P4CS_GeneralError
		};
		#endregion

		#region Member Variables
		/// <summary>
		/// P4.NET API connection to the Perforce server
		/// </summary>
		private P4Connection InquiryConnection = new P4Connection();

		/// <summary>
		/// Current status of the connection to the Perforce server
		/// </summary>
		private EP4ConnectionStatus ConnectionStatus = EP4ConnectionStatus.P4CS_Disconnected;
		#endregion

		#region Constructors
		/// <summary>
		/// Construct a P4Inquirer object
		/// </summary>
		/// <param name="InServer">Server name to connect to</param>
		/// <param name="InUser">User name to connect with</param>
		public P4Inquirer(String InServer, String InUser)
		{
			SetP4Server(InServer);
			SetP4User(InUser);
		}

		/// <summary>
		/// Construct a P4Inquirer object
		/// </summary>
		/// <param name="InServer">Server name to connect to</param>
		/// <param name="InUser">User name to connect with</param>
		/// <param name="InPassword">User password to connect with</param>
		public P4Inquirer(String InServer, String InUser, String InPassword)
			: this(InServer, InUser)
		{
			SetP4Password(InPassword);
		}

		/// <summary>
		/// Construct a P4Inquirer object
		/// </summary>
		public P4Inquirer() {}
		#endregion

		#region Finalizer
		/// <summary>
		/// Finalizer; ensure the Perforce connection is terminated
		/// </summary>
		~P4Inquirer()
		{
			Disconnect();
		}
		#endregion

		#region Connection-Related Methods
		/// <summary>
		/// Attempt to connect to the Perforce server
		/// </summary>
		/// <returns>Connection status after the connection attempt</returns>
		public EP4ConnectionStatus Connect()
		{
			// Only attempt to connect if not already successfully connected
			if (ConnectionStatus != EP4ConnectionStatus.P4CS_Connected)
			{
				// Assume disconnected state
				ConnectionStatus = EP4ConnectionStatus.P4CS_Disconnected;

				// Attempt to connect to the server
				try
				{
					InquiryConnection.Connect();
				}
				// Disconnect and return an error status if an exception was thrown
				catch (P4API.Exceptions.P4APIExceptions)
				{
					Disconnect();
					ConnectionStatus = EP4ConnectionStatus.P4CS_GeneralError;
				}

				// Check if the connection is a valid connection, not considering log-in credentials
				if (!InquiryConnection.IsValidConnection(false, false))
				{
					Disconnect();
					ConnectionStatus = EP4ConnectionStatus.P4CS_GeneralError;
				}

				// Check if the connection is a valid connection, considering log-in credentials
				else if (!InquiryConnection.IsValidConnection(true, false))
				{
					Disconnect();
					ConnectionStatus = EP4ConnectionStatus.P4CS_LoginError;
				}

				// Successfully connected if execution has reached this far!
				else
				{
					ConnectionStatus = EP4ConnectionStatus.P4CS_Connected;
				}
			}
			return ConnectionStatus;
		}

		/// <summary>
		/// Disconnect from the Perforce server
		/// </summary>
		public void Disconnect()
		{
			InquiryConnection.Disconnect();
			ConnectionStatus = EP4ConnectionStatus.P4CS_Disconnected;
		}

		/// <summary>
		/// Set the user name to login to the server with
		/// </summary>
		/// <param name="User">User name to use to login with</param>
		public void SetP4User(String User)
		{
			// Only attempt to change the user if we're not already connected
			if (ConnectionStatus != EP4ConnectionStatus.P4CS_Connected)
			{
				InquiryConnection.User = User;
			}
		}

		/// <summary>
		/// Set the server to attempt to login to
		/// </summary>
		/// <param name="Server">Name of the server to login to</param>
		public void SetP4Server(String Server)
		{
			// Only attempt to change the server if we're not already connected
			if (ConnectionStatus != EP4ConnectionStatus.P4CS_Connected)
			{
				InquiryConnection.Port = Server;
			}
		}

		/// <summary>
		/// Set the user password to attempt to login with
		/// </summary>
		/// <param name="Password">User password to login with</param>
		public void SetP4Password(String Password)
		{
			// Only attempt to change the password if we're not already connected
			if (ConnectionStatus != EP4ConnectionStatus.P4CS_Connected)
			{
				InquiryConnection.Password = Password;
			}
		}
		#endregion

		#region Queries
		/// <summary>
		/// Query the server for a single changelist specified by the query parameter
		/// </summary>
		/// <param name="InQuery">String query containing the relevant changelist number to query for</param>
		/// <returns>P4Changelist object representing the queried changelist, if it exists; otherwise, P4Changelist.InvalidChangelist</returns>
		public P4Changelist QueryChangelist(P4StringQuery InQuery)
		{
			// Assume an invalid changelist
			P4Changelist QueriedChangelist = P4Changelist.InvalidChangelist;
			
			// Can only query if currently connected to the server!
			if (ConnectionStatus == EP4ConnectionStatus.P4CS_Connected)
			{
				try
				{
					// Attempt to run a describe query on the changelist specified by the InQuery
					P4RecordSet ChangelistRecordSet;
					ChangelistRecordSet = InquiryConnection.Run("describe", InQuery.ToString());
					
					// If the record set returned a valid record, construct a P4Changelist from it
					if (ChangelistRecordSet.Records.Length > 0)
					{
						QueriedChangelist = new P4Changelist(ChangelistRecordSet[0]);
					}
				}
				catch (P4API.Exceptions.P4APIExceptions E)
				{
					Console.WriteLine("Error running Perforce command!\n{0}", E.Message);
				}
			}

			return QueriedChangelist;
		}

		/// <summary>
		/// Query the server for multiple changelists in a range specified by the query parameter
		/// </summary>
		/// <param name="InQuery">Timespan query representing a start and end time to query the Perforce server for changelists during that timeframe</param>
		/// <param name="OutChangelists">List of P4Changelists that will be populated with any valid changelist data returned from the query</param>
		public void QueryChangelists(P4TimespanQuery InQuery, out List<P4Changelist> OutChangelists)
		{
			OutChangelists = new List<P4Changelist>();

			// Only attempt to query if actually connected
			if (ConnectionStatus == EP4ConnectionStatus.P4CS_Connected)
			{
				try
				{
					// First, run a "changes" query to retrieve all of the submitted changelists between the timespan specified by the query
					List<String> ExtraArgs = new List<String> { "-l", "-s", "submitted" };
					ExtraArgs.AddRange( InQuery.ToString().Split( ' ' ) );
					P4RecordSet ChangelistRecordSet = InquiryConnection.Run("changes", ExtraArgs.ToArray());

					int NumChangelists = ChangelistRecordSet.Records.Length;
					
					// Set the capacity of the out list to reduce allocations; would be nice to do this up front in the constructor,
					// but can't do it due to the try/catch blocks and scope issues
					OutChangelists.Capacity = NumChangelists;

#if USE_DETAILED_CHANGELISTS
					// If detailed changelists are requested, an additional Perforce command is required to obtain that data. Gather
					// all of the changelist numbers returned by the previous "changes" command.
					String[] ChangelistNumbersToDescribe = new String[NumChangelists];
					for (int ChangelistIndex = 0; ChangelistIndex < NumChangelists; ++ChangelistIndex)
					{
						P4Record CurRecord = ChangelistRecordSet[ChangelistIndex];
						ChangelistNumbersToDescribe[ChangelistIndex] = CurRecord["change"];
					}

					// Next, run a "describe" command on each of the requested changelists. This will provide fully detailed
					// changelist information.
					P4RecordSet DetailedChangeListRecordSet = InquiryConnection.Run("describe", ChangelistNumbersToDescribe);
					foreach (P4Record CurRecord in DetailedChangeListRecordSet.Records)
					{
						OutChangelists.Add(new P4Changelist(CurRecord));
					}
#else
					// If detailed changelists aren't requested, the "changes" command has provided sufficient data in which to construct
					// a P4Changelist for each of the returned changelists
					foreach (P4Record CurRecord in ChangelistRecordSet)
					{
						OutChangelists.Add(new P4Changelist(CurRecord));
					}
#endif // #if USE_DETAILED_CHANGELISTS
				}
				catch (P4API.Exceptions.P4APIExceptions E)
				{
					Console.WriteLine("Error running Perforce command!\n{0}", E.Message);
				}
			}
		}

		/// <summary>
		/// Query the server for a single label, as specified by the query parameter
		/// </summary>
		/// <param name="InQuery">String query containing the name of the relevant label to query for</param>
		/// <returns>P4Label object representing the queried label, if it exists; otherwise, P4Label.InvalidLabel</returns>
		public P4Label QueryLabel(P4StringQuery InQuery)
		{
			// Assume an invalid label
			P4Label QueriedLabel = P4Label.InvalidLabel;

			// Can only perform the query if already connected to the server
			if (ConnectionStatus == EP4ConnectionStatus.P4CS_Connected)
			{
				try
				{
					// Run a "labels" command to retrieve the first label that matches the query string. The "labels" command
					// returns a result based on pattern matching, so also have to do a strcmp to ensure the correct label is returned.
					P4RecordSet LabelRecordSet = InquiryConnection.Run("labels", "-e", InQuery.QueryString, "-t", "-m", "1");
					if (LabelRecordSet.Records.Length > 0 && String.Compare(InQuery.QueryString, LabelRecordSet[0]["label"]) == 0)
					{
						QueriedLabel = new P4Label(LabelRecordSet[0]);
					}
				}
				catch (P4API.Exceptions.P4APIExceptions E)
				{
					Console.WriteLine("Error running Perforce command!\n{0}", E.Message);
				}
			}
			return QueriedLabel;
		}

		/// <summary>
		/// Query the server for all valid label names
		/// </summary>
		/// <param name="OutLabels">List populated with all valid labels queried from the server</param>
		public void QueryAllLabels(out List<P4Label> OutLabels)
		{
			OutLabels = new List<P4Label>();

			// Only can perform the query if actually connected to the server
			if (ConnectionStatus == EP4ConnectionStatus.P4CS_Connected)
			{
				try
				{
					// Run a "labels" command to retrieve all valid labels
					P4RecordSet LabelRecordSet = InquiryConnection.Run("labels", "-t");
					OutLabels.Capacity = LabelRecordSet.Records.Length;

					// Construct a new P4Label object for each valid, queried label
					foreach (P4Record CurRecord in LabelRecordSet)
					{
						OutLabels.Add(new P4Label(CurRecord));
					}
				}
				catch (P4API.Exceptions.P4APIExceptions E)
				{
					Console.WriteLine("Error running Perforce command!\n{0}", E.Message);
				}
			}
		}
		#endregion
	}
}
