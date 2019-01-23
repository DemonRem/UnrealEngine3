/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Threading;

namespace UnrealBuildTool
{
	/**
	 * Represents a file on disk that is used as an input or output of a build action.
	 * FileItems are created by calling FileItem.GetItemByPath, which creates a single FileItem for each unique file path.
	 */
	class FileItem
	{
		/** The action that produces the file. */
		public Action ProducingAction = null;

		/** The last write time of the file. */
		public DateTimeOffset LastWriteTime;

		/** Whether the file exists. */
		public bool bExists = false;

		/** The absolute path of the file. */
		public readonly string AbsolutePath;

		/** The absolute path of the file, stored as uppercase invariant. */
		public readonly string AbsolutePathUpperInvariant;

        /** The pch file that this file will use */
        public string PrecompiledHeaderIncludeFilename;

        /** Description of file, used for logging. */
        public string Description;

		/** The information about the file. */
		public FileInfo Info;

		/** Relative cost of action associated with producing this file. */
		public long RelativeCost = 0;

		/** A dictionary that's used to map each unique file name to a single FileItem object. */
		static Dictionary<string, FileItem> UniqueSourceFileMap = new Dictionary<string, FileItem>();

		/** @return The FileItem that represents the given file path. */
		public static FileItem GetItemByPath(string Path)
		{
			string FullPath = System.IO.Path.GetFullPath(Path);
			string InvariantPath = FullPath.ToUpperInvariant();
			FileItem Result = null;
			if( UniqueSourceFileMap.TryGetValue(InvariantPath, out Result) )
			{
				return Result;
			}
			else
			{
				return new FileItem(FullPath);
			}
		}

		/** If the given file path identifies a file that already exists, returns the FileItem that represents it. */
		public static FileItem GetExistingItemByPath(string Path)
		{
			FileItem Result = GetItemByPath(Path);
			if (Result.bExists)
			{
				return Result;
			}
			else
			{
				return null;
			}
		}

		/**
		 * Creates a text file with the given contents.  If the contents of the text file aren't changed, it won't write the new contents to
		 * the file to avoid causing an action to be considered outdated.
		 */
		public static FileItem CreateIntermediateTextFile(string AbsolutePath, string Contents)
		{
			// Create the directory if it doesn't exist.
			Directory.CreateDirectory(Path.GetDirectoryName(AbsolutePath));

			// Only write the file if its contents have changed.
			if (!File.Exists(AbsolutePath) || Utils.ReadAllText(AbsolutePath) != Contents)
			{
				File.WriteAllText(AbsolutePath, Contents);
			}

			return GetItemByPath(AbsolutePath);
		}

		/** Deletes the file. */
		public void Delete()
		{
			Debug.Assert(bExists);

			int MaxRetryCount = 3;
			int DeleteTryCount = 0;
			bool bFileDeletedSuccessfully = false;
			do
			{
				// If this isn't the first time through, sleep a little before trying again
				if( DeleteTryCount > 0 )
				{
					Thread.Sleep( 1000 );
				}
				DeleteTryCount++;
				try
				{
					// Delete the destination file if it exists
					FileInfo DeletedFileInfo = new FileInfo( AbsolutePath );
					if( DeletedFileInfo.Exists )
					{
						DeletedFileInfo.IsReadOnly = false;
						DeletedFileInfo.Delete();
					}
					// Success!
					bFileDeletedSuccessfully = true;
				}
				catch( Exception Ex )
				{
					Console.WriteLine( "Failed to delete file '" + AbsolutePath + "'" );
					Console.WriteLine( "    Exception: " + Ex.Message );
					if( DeleteTryCount < MaxRetryCount )
					{
						Console.WriteLine( "Attempting to retry..." );
					}
					else
					{
						Console.WriteLine( "ERROR: Exhausted all retries!" );
					}
				}
			}
			while( !bFileDeletedSuccessfully && ( DeleteTryCount < MaxRetryCount ) );
		}

		/** Initialization constructor. */
		public FileItem(string InAbsolutePath)
		{
			AbsolutePath = InAbsolutePath;
			// Convert to upper invariant as it's the unique key.
			AbsolutePathUpperInvariant = AbsolutePath.ToUpperInvariant();
			
			Info = new FileInfo(AbsolutePath);
			
			bExists = Info.Exists;
			if (bExists)
			{
                LastWriteTime = Info.LastWriteTimeUtc;
			}

			UniqueSourceFileMap[AbsolutePathUpperInvariant] = this;
		}

		public override string ToString()
		{
			return Path.GetFileName(AbsolutePath);
		}
	}

}
