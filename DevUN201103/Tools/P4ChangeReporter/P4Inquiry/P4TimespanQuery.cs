/*
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;

namespace P4Inquiry
{
	/// <summary>
	/// Simple class designed to serve as a parameter for a query to the Perforce server that spans over a specified
	/// time frame; additionally allows for the depot to be filtered
	/// </summary>
	public class P4TimespanQuery : P4DepotFilterQuery
	{
		#region Member Variables
		/// <summary>
		/// Starting date of the timespan query
		/// </summary>
		private DateTime mQueryStartDate;

		/// <summary>
		/// Ending date of the timespan query
		/// </summary>
		private DateTime mQueryEndDate;
		#endregion

		#region Properties
		/// <summary>
		/// Starting date of the timespan query
		/// </summary>
		public DateTime QueryStartDate 
		{
			get { return mQueryStartDate; } 
		}

		/// <summary>
		/// Ending date of the timespan query
		/// </summary>
		public DateTime QueryEndDate
		{
			get { return mQueryEndDate; }
		}
		#endregion

		#region Constructors
		/// <summary>
		/// Construct a P4TimespanQuery from the dates specified as parameters; the dates are not required to be in chronological order
		/// </summary>
		/// <param name="InQueryDateOne">Either the start or end date of the query</param>
		/// <param name="InQueryDateTwo">Either the start of end date of the query, whichever wasn't specified by the first parameter</param>
		public P4TimespanQuery(DateTime InQueryDateOne, DateTime InQueryDateTwo)
		{
			// Order the passed in dates chronologically
			DetermineDateOrder(InQueryDateOne, InQueryDateTwo);
		}

		/// <summary>
		/// Construct a P4TimespanQuery from the dates specified as parameters; the dates are not required to be in chronological order
		/// </summary>
		/// <param name="InQueryDateOne">Either the start or end date of the query</param>
		/// <param name="InQueryDateTwo">Either the start of end date of the query, whichever wasn't specified by the first parameter</param>
		/// <param name="InDepotFilter">Path to filter the depot by</param>
		/// <param name="InUserFilter">User to filter the depot by</param>
		public P4TimespanQuery(DateTime InQueryDateOne, DateTime InQueryDateTwo, String InDepotFilter, String InUserFilter)
			: base(InDepotFilter, InUserFilter)
		{
			// Order the passed in dates chronologically
			DetermineDateOrder(InQueryDateOne, InQueryDateTwo);
		}
		#endregion

		#region Public Methods
		/// <summary>
		/// Overridden ToString method
		/// </summary>
		/// <returns>The query in a manner that Perforce expects</returns>
		public override string ToString()
		{
			return base.ToString() + "@" + QueryStartDate.ToLocalTime().ToString("yyyy/MM/dd:HH:mm:ss") + ",@" + QueryEndDate.ToLocalTime().ToString("yyyy/MM/dd:HH:mm:ss");
		}
		#endregion

		#region Helper Methods
		/// <summary>
		/// Sets the query start and end dates accordingly based on the values of the provided parameters
		/// </summary>
		/// <param name="InQueryDateOne">The start or end date for the query</param>
		/// <param name="InQueryDateTwo">The start or end date for the query, whichever wasn't specified by the first parameter</param>
		private void DetermineDateOrder(DateTime InQueryDateOne, DateTime InQueryDateTwo)
		{
			if (InQueryDateOne.CompareTo(InQueryDateTwo) < 0)
			{
				mQueryStartDate = InQueryDateOne;
				mQueryEndDate = InQueryDateTwo;
			}
			else
			{
				mQueryStartDate = InQueryDateTwo;
				mQueryEndDate = InQueryDateOne;
			}
		}
		#endregion
	}
}
