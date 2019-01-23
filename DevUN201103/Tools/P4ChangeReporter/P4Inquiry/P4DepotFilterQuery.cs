/*
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;

namespace P4Inquiry
{
	/// <summary>
	/// Simple class designed to serve as a parameter for a query to the Perforce server; specifies any filtering to the
	/// depot that should occur as part of the query
	/// </summary>
	public class P4DepotFilterQuery
	{
		#region Member Variables
		/// <summary>
		/// Path to filter the depot by
		/// </summary>
		private String mFilterPath;
		#endregion

		#region Properties
		/// <summary>
		/// Path to filter the depot by
		/// </summary>
		public String FilterPath
		{
			get { return mFilterPath; }
			set { mFilterPath = value; }
		}
		#endregion

		#region Member Variables
		/// <summary>
		/// User to filter the depot by
		/// </summary>
		private String mFilterUser;
		#endregion

		#region Properties
		/// <summary>
		/// User to filter the depot by
		/// </summary>
		public String FilterUser
		{
			get { return mFilterUser; }
			set { mFilterUser = value; }
		}
		#endregion

		#region Constructors
		/// <summary>
		/// Construct a P4DepotFilterQuery object with the filter path and user initialized to the provided parameter
		/// </summary>
		/// <param name="InFilterPath">Path to filter the depot by</param>
		/// <param name="InFilterUser">User to filter the depot by</param>
		public P4DepotFilterQuery( String InFilterPath, String InFilterUser )
		{
			mFilterPath = InFilterPath;
			mFilterUser = InFilterUser;
		}

		/// <summary>
		/// Construct a P4DepotFilterQuery with an unfiltered depot path
		/// </summary>
		public P4DepotFilterQuery()
		{
			mFilterPath = "//depot/...";
			mFilterUser = "";
		}
		#endregion

		#region Public Methods
		/// <summary>
		/// Overridden version of ToString
		/// </summary>
		/// <returns>The path to filter the depot by</returns>
		public override string ToString()
		{
			String FilterString = mFilterPath;
			if( FilterUser.Length > 0 )
			{
				// P4 command for filtering by user is: "-u <username>"
				FilterString = "-u " + mFilterUser + " " + FilterString;
			}
			return FilterString;
		}
		#endregion
	}
}