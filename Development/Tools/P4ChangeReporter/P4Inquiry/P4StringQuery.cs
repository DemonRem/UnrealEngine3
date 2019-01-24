/*
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;

namespace P4Inquiry
{
	/// <summary>
	/// Simple class designed to serve as a parameter for a query to the Perforce server
	/// </summary>
	public class P4StringQuery
	{
		#region Member Variables
		/// <summary>
		/// String used to specify a query; can be used to represent a label name, changelist number, etc.
		/// </summary>
		private String mQueryString;
		#endregion

		#region Properties
		/// <summary>
		/// String used to specify a query; can be used to represent a label name, changelist number, etc.
		/// </summary>
		public String QueryString
		{
			get { return mQueryString; }
		}
		#endregion

		#region Constructors
		/// <summary>
		/// Construct a P4StringQuery
		/// </summary>
		/// <param name="InQueryString">String to use as the query string</param>
		public P4StringQuery(String InQueryString)
		{
			mQueryString = InQueryString;
		}
		#endregion

		#region Public Methods
		/// <summary>
		/// Overridden version of ToString()
		/// </summary>
		/// <returns>The string to use as the query string</returns>
		public override string ToString()
		{
			return QueryString;
		}
		#endregion
	}
}
