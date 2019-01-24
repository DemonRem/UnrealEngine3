/*
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Xml.Serialization;
using P4API;

namespace P4Core
{
	/// <summary>
	/// Class providing a logical representation of the data contained within a changelist on Perforce
	/// </summary>
	public class P4Changelist
	{
		#region Static Constants
		
		/// <summary>
		/// Invalid changelist, used as an easy means to check if an operation successfully produced a valid changelist or not
		/// </summary>
		public static readonly P4Changelist InvalidChangelist = new P4Changelist();
		#endregion

		#region Member Variables
		/// <summary>
		/// Changelist number
		/// </summary>
		private int mNumber = int.MinValue;

		/// <summary>
		/// Changelist submission date, stored in UTC time
		/// </summary>
		private DateTime mDate = DateTime.MinValue;

		/// <summary>
		/// Client which submitted the changelist
		/// </summary>
		private String mClient = String.Empty;

		/// <summary>
		/// User who submitted the changelist
		/// </summary>
		private String mUser = String.Empty;

		/// <summary>
		/// Changelist description
		/// </summary>
		private String mDescription = String.Empty;

#if USE_DETAILED_CHANGELISTS
		/// <summary>
		/// Information regarding the files affected by the changelist
		/// </summary>
		private List<P4File> mFiles = new List<P4File>();
#endif // #if USE_DETAILED_CHANGELISTS
		#endregion

		#region Properties
		/// <summary>
		/// Changelist number
		/// </summary>
		public int Number
		{
			get { return mNumber; }
			set { mNumber = value; }
		}

		/// <summary>
		/// Changelist submission date, stored in UTC time
		/// </summary>
		public DateTime Date
		{
			get { return mDate; }
			set { mDate = value; }
		}

		/// <summary>
		/// Client which submitted the changelist
		/// </summary>
		public String Client
		{
			get { return mClient; }
			set { mClient = value; }
		}

		/// <summary>
		/// User who submitted the changelist
		/// </summary>
		public String User
		{
			get { return mUser; }
			set { mUser = value; }
		}

		/// <summary>
		/// Changelist description; Ignored during Xml serialization
		/// </summary>
		[XmlIgnore()]
		public String Description
		{
			get { return mDescription; }
			set { mDescription = value; }
		}
#if USE_DETAILED_CHANGELISTS
		/// <summary>
		/// Information regarding the files affected by the changelist
		/// </summary>
		public List<P4File> Files
		{
			get { return mFiles; }
			set { mFiles = value; }
		}
#endif // #if USE_DETAILED_CHANGELISTS
		#endregion

		#region Constructors
		/// <summary>
		/// Construct a P4Changelist from a P4Record retrieved from the P4.NET API
		/// </summary>
		/// <param name="InChangelistRecord">P4Record containing changelist data, retrived from the P4.NET API</param>
		public P4Changelist(P4Record InChangelistRecord)
		{
			// Attempt to retrieve the changelist number from the P4Record
			if (InChangelistRecord.Fields.ContainsKey("change"))
			{
				int.TryParse(InChangelistRecord["change"], out mNumber);
			}

			// Attempt to retrieve the submission date from the P4Record
			if (InChangelistRecord.Fields.ContainsKey("time"))
			{
				long SecondsSinceEpoch = 0;
				long.TryParse(InChangelistRecord["time"], out SecondsSinceEpoch);

				// Perforce returns the date as "seconds since the Unix epoch," so adjust the time accordingly
				mDate = P4Constants.UnixEpochTime.AddSeconds(SecondsSinceEpoch);
			}

			// Attempt to retrieve the client from the P4Record
			if (InChangelistRecord.Fields.ContainsKey("client"))
			{
				mClient = InChangelistRecord["client"];
			}

			// Attempt to retrieve the user from the P4Record
			if (InChangelistRecord.Fields.ContainsKey("user"))
			{
				mUser = InChangelistRecord["user"];
			}

			// Attempt to retrieve the description from the P4Record
			if (InChangelistRecord.Fields.ContainsKey("desc"))
			{
				mDescription = InChangelistRecord["desc"];
			}

#if USE_DETAILED_CHANGELISTS
			// Attempt to retrieve information about affected files from the P4Record
			if (InChangelistRecord.ArrayFields.ContainsKey("depotFile"))
			{
				int NumDepotFiles = InChangelistRecord.ArrayFields["depotFile"].Length;
				mFiles.Capacity = NumDepotFiles;

				// Iterate over each affected file, recovering the relevant information for each file
				for (int DepotFileIndex = 0; DepotFileIndex < NumDepotFiles; ++DepotFileIndex)
				{
					P4File NewFileInfo = new P4File();
					NewFileInfo.FilePath = InChangelistRecord.ArrayFields["depotFile"][DepotFileIndex];

					// Attempt to retrieve the Perforce action performed on this file (add, delete, edit, integrate, etc.)
					if (InChangelistRecord.ArrayFields.ContainsKey("action"))
					{
						NewFileInfo.AssociatedAction = InChangelistRecord.ArrayFields["action"][DepotFileIndex];
					}

					// Attempt to retrieve the file type
					if (InChangelistRecord.ArrayFields.ContainsKey("type"))
					{
						NewFileInfo.FileType = InChangelistRecord.ArrayFields["type"][DepotFileIndex];
					}

					// Attempt to retrieve the file revision
					if (InChangelistRecord.ArrayFields.ContainsKey("rev"))
					{
						int RevNumber = 0;
						int.TryParse(InChangelistRecord.ArrayFields["rev"][DepotFileIndex] ?? String.Empty, out RevNumber);
						NewFileInfo.RevisionNumber = RevNumber;
					}
					mFiles.Add(NewFileInfo);
				}
			}
#endif // #if USE_DETAILED_CHANGELISTS
		}

		/// <summary>
		/// Default constructor; necessary for Xml serializer to work, however shouldn't be used by others, so made private
		/// </summary>
		private P4Changelist()
		{
		}
		#endregion
	}
}
