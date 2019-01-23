/*
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using P4API;

namespace P4Core
{
	/// <summary>
	/// Class providing a logical representation of the data contained within a label on Perforce
	/// </summary>
	public class P4Label
	{
		#region Static Constants
		/// <summary>
		/// Invalid label, used as an easy means to check if an operation successfully produced a valid label or not 
		/// </summary>
		public static readonly P4Label InvalidLabel = new P4Label();
		#endregion

		#region Member Variables
		/// <summary>
		/// Name of the label
		/// </summary>
		private String mName = String.Empty;

		/// <summary>
		/// Label's description
		/// </summary>
		private String mDescription = String.Empty;

		/// <summary>
		/// Owner of the label
		/// </summary>
		private String mOwner = String.Empty;

		/// <summary>
		/// Label options, such as locked/unlocked
		/// </summary>
		private String mOptions = String.Empty;

		/// <summary>
		/// Date the label was last updated, in UTC time
		/// </summary>
		private DateTime mLastUpdatedDate = DateTime.MinValue;

		/// <summary>
		/// Date the label was last accessed, in UTC time
		/// </summary>
		private DateTime mLastAccessedDate = DateTime.MinValue;
		#endregion

		#region Properties
		/// <summary>
		/// Name of the label
		/// </summary>
		public String Name
		{
			get { return mName; }
		}

		/// <summary>
		/// Label's description
		/// </summary>
		public String Description
		{
			get { return mDescription; }
		}

		/// <summary>
		/// Owner of the label
		/// </summary>
		public String Owner
		{
			get { return mOwner; }
		}

		/// <summary>
		/// Label options, such as locked/unlocked
		/// </summary>
		public String Options
		{
			get { return mOptions; }
		}

		/// <summary>
		/// Date the label was last updated, in UTC time
		/// </summary>
		public DateTime LastUpdatedDate
		{
			get { return mLastUpdatedDate; }
		}

		/// <summary>
		/// Date the label was last accessed, in UTC time
		/// </summary>
		public DateTime LastAccessedDate
		{
			get { return mLastAccessedDate; }
		}
		#endregion

		#region Constructors
		/// <summary>
		/// Construct a P4Label from a P4Record retrieved from the P4.NET API 
		/// </summary>
		/// <param name="InLabelRecord">P4Record containing label data, retrieved from the P4.NET API</param>
		public P4Label(P4Record InLabelRecord)
		{
			// Attempt to retrieve the label name from the P4Record
			if (InLabelRecord.Fields.ContainsKey("label"))
			{
				mName = InLabelRecord["label"];
			}

			// Attempt to retrieve the description from the P4Record
			if (InLabelRecord.Fields.ContainsKey("Description"))
			{
				mDescription = InLabelRecord["Description"];
			}

			// Attempt to retrieve the owner from the P4Record
			if (InLabelRecord.Fields.ContainsKey("Owner"))
			{
				mOwner = InLabelRecord["Owner"];
			}

			// Attempt to retrieve the label options from the P4Record
			if (InLabelRecord.Fields.ContainsKey("Options"))
			{
				mOptions = InLabelRecord["Options"];
			}

			// Attempt to retrieve the last updated time from the P4Record
			long SecondsSinceEpoch = 0;
			if (InLabelRecord.Fields.ContainsKey("Update"))
			{
				long.TryParse(InLabelRecord["Update"], out SecondsSinceEpoch);

				// Perforce returns the time as "seconds since the Unix epoch," so adjust accordingly
				mLastUpdatedDate = P4Constants.UnixEpochTime.AddSeconds(SecondsSinceEpoch);
			}

			// Attempt to retrieve the last accessed time from the P4Record
			if (InLabelRecord.Fields.ContainsKey("Access"))
			{
				SecondsSinceEpoch = 0;
				long.TryParse(InLabelRecord["Access"], out SecondsSinceEpoch);

				// Perforce returns the time as "seconds since the Unix epoch," so adjust accordingly
				mLastAccessedDate = P4Constants.UnixEpochTime.AddSeconds(SecondsSinceEpoch);
			}
		}

		/// <summary>
		/// Private default constructor; used exclusively for the invalid label
		/// </summary>
		private P4Label() {}
		#endregion
	}
}
