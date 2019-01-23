/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Ionic.Zip;

namespace Controller
{
	public partial class Command
	{
		private void CreateZip()
		{
			if( Builder.CurrentZip == null )
			{
				Builder.CurrentZip = new ZipFile();
				Builder.CurrentZip.CompressionLevel = Ionic.Zlib.CompressionLevel.None;
				Builder.CurrentZip.UseZip64WhenSaving = Zip64Option.Always;
				Builder.Write( " ...  ZipFile created" );
			}
		}

		private void ZipAddItem( COMMANDS Command )
		{
			try
			{
				string LogFileName = Builder.GetLogFileName( Command );
				Builder.OpenLog( LogFileName, false );

				// Create the zip if it doesn't exist
				CreateZip();

				string[] Params = Builder.SplitCommandline();
				if( Params.Length != 1 )
				{
					Builder.Write( "Error: incorrect number of parameters for " + Command.ToString() );
					ErrorLevel = Command;
				}
				else
				{
					switch( Command )
					{
					case COMMANDS.ZipAddImage:
						string ImageName = Path.Combine( Params[0], Builder.GetFolderName() + "." + Builder.ImageMode );
						Builder.Write( "Adding to Zip: " + ImageName );
						Builder.CurrentZip.AddFile( ImageName, "" );
						break;

					case COMMANDS.ZipAddFile:
						Builder.Write( "Adding to Zip: " + Builder.CommandLine );
						Builder.CurrentZip.AddFile( Params[0], "" );
						break;
					}
				}

				Builder.CloseLog();
			}
			catch
			{
				ErrorLevel = Command;
				Builder.Write( "Error: exception while adding item to zip" );
				Builder.CloseLog();
			}
		}

		private void ZipSave()
		{
			try
			{
				string LogFileName = Builder.GetLogFileName( COMMANDS.ZipSave );
				Builder.OpenLog( LogFileName, false );

				string[] Params = Builder.SplitCommandline();
				if( Params.Length != 1 )
				{
					Builder.Write( "Error: incorrect number of parameters for ZipSave" );
					ErrorLevel = COMMANDS.ZipSave;
				}
				else
				{
					string ArchiveName = Path.Combine( Params[0], Builder.GetFolderName() + ".zip" );
					Builder.Write( "Saving Zip: " + ArchiveName );
					Builder.CurrentZip.Save( ArchiveName );
				}

				// Now we've saved the zip, we're done with it
				Builder.Write( "Disposing of Zip" );
				Builder.CurrentZip.Dispose();
				Builder.CurrentZip = null;

				Builder.CloseLog();
			}
			catch
			{
				ErrorLevel = COMMANDS.ZipSave;
				Builder.Write( "Error: exception while saving zip" );
				Builder.CloseLog();
			}
		}

		private void RarAddItem( COMMANDS Command )
		{
			try
			{
				string LogFileName = Builder.GetLogFileName( Command );
				Builder.OpenLog( LogFileName, false );

				string[] Params = Builder.SplitCommandline();
				if( Params.Length != 2 )
				{
					Builder.Write( "Error: incorrect number of parameters for " + Command.ToString() );
					ErrorLevel = Command;
				}
				else
				{
					string FileName = "";
					switch( Command )
					{
					case COMMANDS.RarAddImage:
						FileName = Path.Combine( Params[1], Builder.GetFolderName() + "." + Builder.ImageMode );
						break;

					case COMMANDS.RarAddFile:
						FileName = Params[1];
						break;
					}

					Builder.Write( "Adding to Rar: " + FileName );
					
					string Executable = "\\\\epicgames.net\\Root\\Developers\\John.Scott\\WinRAR\\rar.exe";
					string ArchiveName = Path.Combine( Params[0], Builder.GetImageName() + ".rar" );
					string CommandLine = "a -m0 -ep " + ArchiveName + " " + FileName;
					
					CurrentBuild = new BuildProcess( Parent, Builder, Executable, CommandLine, "", true );
					ErrorLevel = CurrentBuild.GetErrorLevel();
				}

				StartTime = DateTime.UtcNow;
			}
			catch
			{
				ErrorLevel = Command;
				Builder.Write( "Error: exception while adding item to rar" );
				Builder.CloseLog();
			}
		}
	}
}