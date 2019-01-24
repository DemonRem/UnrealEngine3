/*
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;

namespace P4Core
{
	/// <summary>
	/// Class providing a logical representation of a file in a Perforce changelist
	/// </summary>
	[System.Xml.Serialization.XmlType("File")]
	public class P4File
	{
		#region Member Variables
		/// <summary>
		/// Depot path of the file
		/// </summary>
		private String mFilePath = String.Empty;

		/// <summary>
		/// Perforce action associated with the file, such as add, integrate, delete, etc.
		/// </summary>
		private String mAssociatedAction = String.Empty;

		/// <summary>
		/// Type of the file, such as text, binary, etc.
		/// </summary>
		private String mFileType = String.Empty;

		/// <summary>
		/// Revision number of the file
		/// </summary>
		private int mRevisionNumber = int.MinValue;
		#endregion

		#region Properties
		/// <summary>
		/// Depot path of the file
		/// </summary>
		public String FilePath 
		{
			get { return mFilePath; }
			set { mFilePath = value; }
		}

		/// <summary>
		/// Perforce action associated with the file, such as add, integrate, delete, etc.
		/// </summary>
		public String AssociatedAction 
		{
			get { return mAssociatedAction; }
			set { mAssociatedAction = value; }
		}

		/// <summary>
		/// Type of the file, such as text, binary, etc.
		/// </summary>
		public String FileType 
		{
			get { return mFileType;  }
			set { mFileType = value; } 
		}

		/// <summary>
		/// Revision number of the file
		/// </summary>
		public int RevisionNumber 
		{ 
			get { return mRevisionNumber; }
			set { mRevisionNumber = value; }
		}
		#endregion
	}
}
