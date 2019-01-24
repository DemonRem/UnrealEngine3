/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using Ionic.Zip;

namespace CookerSync
{
	partial class CookerSyncApp
	{
		static public bool SaveZip( List<List<ConsoleInterface.TOCInfo>> TOCList, ConsoleInterface.TOCSettings BuildSettings )
		{
			string OldCWD = Environment.CurrentDirectory;
			Environment.CurrentDirectory = Path.Combine( Environment.CurrentDirectory, ".." );

			Log( Color.Green, "[ZIPPING STARTED]" );
			DateTime StartTime = DateTime.UtcNow;

			foreach( string ZipFileName in BuildSettings.ZipFiles )
			{
				long RunningSourceTotal = 0;
				int ZipFileIndex = 0;

				string RootZipFileName = Path.ChangeExtension( ZipFileName, null );
				string IndexedZipFileName = null;

				// Create empty zip
				ZipFile Zip = null;

				// Copy all files into the zip
				foreach( List<ConsoleInterface.TOCInfo> TOC in TOCList )
				{
					// Copy each file from the table of contents into the zip
					foreach( ConsoleInterface.TOCInfo Entry in TOC )
					{
						// Create a new zip if necessary
						if( Zip == null )
						{
							IndexedZipFileName = RootZipFileName + "." + ZipFileIndex.ToString( "00" ) + ".zip";
							Zip = new ZipFile( IndexedZipFileName );
							Zip.CompressionLevel = Ionic.Zlib.CompressionLevel.Level9;
							Zip.BufferSize = 0x10000;
							Zip.UseUnicodeAsNecessary = true;
						}

						string FullFileName = Entry.FileName;
						if( FullFileName.StartsWith( "..\\" ) )
						{
							FullFileName = FullFileName.Substring( 3 );
						}

						Log( Color.Black, "Adding/updating " + FullFileName );
						Zip.UpdateFile( FullFileName );

						// Check for going to next zip file
						FileInfo Info = new FileInfo( FullFileName );
						RunningSourceTotal += Info.Length;
						if( RunningSourceTotal > 1024 * 1024 * 1024 )
						{
							Log( Color.Black, "Saving zip: " + IndexedZipFileName );
							Zip.Save();

							Zip = null;
							RunningSourceTotal = 0;
							ZipFileIndex++;
						}
					}
				}

				if( Zip != null )
				{
					// Save zip
					Log( Color.Black, "Saving final zip: " + IndexedZipFileName );
					Zip.Save();
				}
			}

			TimeSpan Duration = DateTime.UtcNow.Subtract( StartTime );
			Log( Color.Green, "Operation took " + Duration.Minutes.ToString() + ":" + Duration.Seconds.ToString( "D2" ) );
			Log( Color.Green, "[ZIPPING FINISHED]" );

			Environment.CurrentDirectory = OldCWD;
			return( true );
		}
	}
}