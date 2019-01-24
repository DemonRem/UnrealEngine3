/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Threading;

namespace Controller
{
	public partial class Command
	{
		private void FTPThreadProc( object SourceFileName )
		{
			try
			{
				string FullPath = Path.GetFullPath( ( string )SourceFileName );
				FileInfo Info = new FileInfo( FullPath );

				Parent.Log( "[STATUS] Starting to upload " + Info.Name, Color.Magenta );

				Uri DestURI = new Uri( "ftp://" + Builder.FTPServer + "/" + Info.Name );
				FtpWebRequest Request = ( FtpWebRequest )FtpWebRequest.Create( DestURI );

				// Set the credentials required to connect to the FTP server
				Request.Credentials = new NetworkCredential( Builder.FTPUserName, Builder.FTPPassword );

				// Automatically close the connection on completion
				Request.KeepAlive = false;

				// Set to upload a file
				Request.Method = WebRequestMethods.Ftp.UploadFile;

				// Send a binary file
				Request.UseBinary = true;

				// How much data to send
				Request.ContentLength = Info.Length;

				long TotalUploaded = 0;
				int MaxBufferLength = 65536;
				byte[] Buffer = new byte[MaxBufferLength];

				int PercentComplete = -1;

				FileStream Source = Info.OpenRead();
				Stream Destination = Request.GetRequestStream();

				int BufferLength = Source.Read( Buffer, 0, MaxBufferLength );
				while( BufferLength > 0 )
				{
					Destination.Write( Buffer, 0, BufferLength );
					BufferLength = Source.Read( Buffer, 0, MaxBufferLength );

					// Output the occasional status
					TotalUploaded += BufferLength;
					int NewPercentComplete = ( int )( ( 100.0 * TotalUploaded ) / Info.Length );
					if( NewPercentComplete != PercentComplete )
					{
						PercentComplete = NewPercentComplete;
						Parent.Log( "[STATUS] Uploading " + Info.Name + " : " + PercentComplete.ToString() + "% complete", Color.Magenta );
					}
				}

				Destination.Close();
				Source.Close();
			}
			catch( Exception Ex )
			{
				Parent.Log( "FTP ERROR: Exception while FTPing: " + Ex.ToString(), Color.Red );
				Builder.Write( "FTP ERROR: Exception while FTPing: " + Ex.ToString() );
			}
		}

		private void FTPSendFile( string FileName, COMMANDS Command )
		{
			try
			{
				string LogFileName = Builder.GetLogFileName( Command );
				Builder.OpenLog( LogFileName, false );

				if( Builder.FTPServer.Length == 0 || Builder.FTPUserName.Length == 0 || Builder.FTPPassword.Length == 0 )
				{
					Builder.Write( "Error: FTP server and credentials are not set!" );
					ErrorLevel = Command;
				}
				else
				{
					Builder.ManageFTPThread = new Thread( FTPThreadProc );
					Builder.ManageFTPThread.Start( FileName );
				}

				FileInfo Info = new FileInfo( Path.GetFullPath( FileName ) );
				Parent.DB.WritePerformanceData( Parent.MachineName, "BytesFTPed", Info.Length );

				StartTime = DateTime.UtcNow;
			}
			catch
			{
				ErrorLevel = Command;
				Builder.Write( "Error: exception while starting to FTP a file" );
				Builder.CloseLog();
			}
		}

		private void FTPSendFile()
		{
			string[] Params = Builder.SplitCommandline();
			if( Params.Length != 1 )
			{
				Builder.Write( "Error: incorrect number of parameters for FTPSendFile" );
				ErrorLevel = COMMANDS.FTPSendFile;
			}
			else
			{
				FTPSendFile( Params[0], COMMANDS.FTPSendFile );
			}
		}

		private void FTPSendImage()
		{
			string[] Params = Builder.SplitCommandline();
			if( Params.Length != 1 )
			{
				Builder.Write( "Error: incorrect number of parameters for FTPSendImage" );
				ErrorLevel = COMMANDS.FTPSendImage;
			}
			else
			{
				string SourceFile = Path.Combine( Params[0], Builder.GetImageName() + "." + Builder.ImageMode );
				FTPSendFile( SourceFile, COMMANDS.FTPSendImage );
			}
		}
	}
}
