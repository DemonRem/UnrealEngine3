/*
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;

namespace P4Core
{
	/// <summary>
	/// Utility class to hold various relevant constant values related to Perforce
	/// </summary>
	public sealed class P4Constants
	{
		/// <summary>
		/// Unix Epoch Time: 1/1/1970; Perforce returns timestamps as number of seconds from this date
		/// </summary>
		public static readonly DateTime UnixEpochTime = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Utc);

		/// <summary>
		/// Private constructor to prevent instantiation
		/// </summary>
		private P4Constants()
		{
		}
	}
}