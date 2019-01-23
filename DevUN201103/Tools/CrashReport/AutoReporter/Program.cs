/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Windows.Forms;
using System.IO;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace AutoReporter
{
	class OutputLogFile
	{
		private StreamWriter MyStreamWriter;

		public OutputLogFile(string filename)
		{
			try
			{
				MyStreamWriter = new StreamWriter(filename);
			}
			catch(Exception)
			{
				MyStreamWriter = null;
			}
		}

		public void WriteLine(string line)
		{
			if(MyStreamWriter != null)
			{
				try
				{
					MyStreamWriter.WriteLine(line);
					MyStreamWriter.Flush();
				}
				catch(Exception)
				{

				}
			}
		}

		public void Close()
		{
			if(MyStreamWriter != null)
			{
				MyStreamWriter.Close();
			}
		}

	};

	static class Program
	{
		/** 
		 * Code for calling SendMessage.
		 */
		#region Win32 glue

		// import the SendMessage function so we can send windows messages to the UnrealConsole
		[DllImport("user32", CharSet = CharSet.Auto)]
		private static extern int SendMessage(HandleRef hWnd, int msg, int wParam, int lParam);

		[DllImport("user32", CharSet = CharSet.Auto)]
		private static extern int RegisterWindowMessage(string lpString);

		// Constants from the Platform SDK.
		private const int HWND_BROADCAST = 0xffff;

		#endregion

		/**
         * AutoReporter is a program for sending crash data to a web service.  
         * First it opens a log file on the local machine.  Then it parses the crash dump.
         * Next the ReportService is used to create a new report from data extracted from the crash dump.
         * The log file and ini dump are uploaded to the server.  If all this succeeds, the user is prompted 
         * to enter a description of the crash, which is then sent to the ReportService.
         * Finally, if no errors occur the dumps and log are deleted.
         * 
         * 3 arguments expected: AutoReport Dump file name, Log file name and Ini dump file name
         */
		[STAThread]
		static void Main(string[] args)
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);

			string logFileName;
			if(args.Length >= 2)
			{
				string logDirectory;
				int endOfLogPath = args[1].LastIndexOf('\\');
				logDirectory = args[1].Substring(0, endOfLogPath + 1);
				logFileName = logDirectory + "AutoReportLog.txt";
			}
			else
			{
				logFileName = "AutoReportLog.txt";
			}

			OutputLogFile LogFile = new OutputLogFile(logFileName);
			LogFile.WriteLine("Log opened: " + logFileName);
			LogFile.WriteLine("");
			LogFile.WriteLine("Current Time = " + DateTime.Now.ToString());
			LogFile.WriteLine("");
			LogFile.WriteLine("Arguments:");
			foreach(string arg in args)
			{
				LogFile.WriteLine(arg);
			}

			if(args.Length < 4)
			{
				LogFile.WriteLine("Expected 4 arguments: AutoReport Dump file name, Log file name, Ini dump file name and Mini dump file name");
				LogFile.Close();
				return;
			}

			// Check for additional options
			bool bForceUnattended = false;
			bool bShowBalloon = false;
			for( int ExtraArgIndex = 4; ExtraArgIndex < args.Length; ++ExtraArgIndex )
			{
				string CurArgString = args[ ExtraArgIndex ];

				// -Unattended : forces unattended mode regardless of command line string
				if( CurArgString.Equals( "-Unattended" , StringComparison.OrdinalIgnoreCase ) )
				{
					bForceUnattended = true;
				}

				// -Balloon : displays a system tray notify icon (balloon) and forces unattended mode
				else if( CurArgString.Equals( "-Balloon", StringComparison.OrdinalIgnoreCase ) )
				{
					bShowBalloon = true;

					// Unattended mode is implied with -Balloon
					bForceUnattended = true;
				}

				else
				{
					LogFile.WriteLine( String.Format( "Unrecognized parameter: {0}", CurArgString ) );
					LogFile.Close();
					return;
				}
			}



			ReportFile rFile = new ReportFile();
			ReportFileData reportData = new ReportFileData();
			LogFile.WriteLine("Parsing report file: " + args[0]);
			if(!rFile.ParseReportFile(args[0], reportData, LogFile))
			{
				LogFile.WriteLine("Failed to parse report file: " + args[0]);
				LogFile.Close();
				return;
			}

			bool bIsUnattended = reportData.CommandLine.Contains( "unattended" ) || bForceUnattended;

			LogFile.WriteLine( "Attempting to create a new crash description form..." );
			Form1 crashDescForm = new Form1();

			if( bShowBalloon )
			{
				crashDescForm.summary = "Handled error";
				crashDescForm.crashDesc = "Handled error";
			}
			else
			{
				crashDescForm.summary = "Unattended mode";
				crashDescForm.crashDesc = "Unattended mode";
			}

			
			int BalloonTimeInMs = 2000;
			if( bShowBalloon )
			{
				String MsgText = "An unexpected error has occurred but the application has recovered.  A report will be submitted to the QA database.";

				// If the error occurred in the editor then we'll remind them to save their changes
				if( reportData.EngineMode.Contains( "Editor" ) )
				{
					MsgText += "  Remember to save your work often.";
				}

				crashDescForm.ShowBalloon( MsgText, BalloonTimeInMs );
			}


			// Only summon the interactive report entry form if we're not running in unattended mode
			if( !bIsUnattended )
			{
				LogFile.WriteLine("Running attended...");
				crashDescForm.SetCallStack(reportData.CallStack);

				LogFile.WriteLine("Running the application...");
				Application.Run(crashDescForm);
			}

			LogFile.WriteLine("Crash Summary = " + crashDescForm.summary);
			LogFile.WriteLine("Crash Description = " + crashDescForm.crashDesc);

			LogFile.WriteLine("Registering report service...");
			ReportService.RegisterReport reportService = new ReportService.RegisterReport();
            

			Exception serviceException = new Exception("");
			bool serviceError = false;

            

            LogFile.WriteLine("Attempting to create a new crash...");
            int uniqueIndex = -1;
            try
            {
                uniqueIndex = reportService.CreateNewCrash( -1, reportData.ComputerName, reportData.UserName, reportData.GameName, reportData.PlatformName,
                    reportData.LanguageExt, reportData.TimeOfCrash, reportData.BuildVer, reportData.ChangelistVer, reportData.CommandLine,
                    reportData.BaseDir, reportData.CallStack, reportData.EngineMode);
            }
            catch (Exception e)
            {
                LogFile.WriteLine("AutoReporter had an exception in accessing the reportService! --> " + e.ToString());
                serviceException = e;
                serviceError = true;
                LogFile.WriteLine(e.Message);
            }

			LogFile.WriteLine("");
			LogFile.WriteLine("uniqueIndex = " + uniqueIndex.ToString());
			LogFile.WriteLine("");


            if (uniqueIndex == -1)
            {
                LogFile.WriteLine("The service failed to create a new Crash!");
                serviceError = true;
                serviceException = new Exception("The service failed to create a new Crash!");
            }


			LogFile.WriteLine("Attempting to create a new UpdateReportFiles instance...");
			UploadReportFiles reportUploader = new UploadReportFiles();
			bool fileUploadSuccess = false;

			if(!serviceError)
			{
				LogFile.WriteLine("Attempting to upload files...");
				try
				{
					fileUploadSuccess = reportUploader.UploadFiles(args[1], args[2], args[3], uniqueIndex, LogFile);
				}
				catch(Exception e)
				{
					LogFile.WriteLine("AutoReporter had an exception uploading files! --> " + e.ToString());
					serviceException = e;
					serviceError = true;
					LogFile.WriteLine(e.Message);
				}

				if(fileUploadSuccess)
				{
					LogFile.WriteLine("Successfully uploaded files");
				}
				else
				{
					LogFile.WriteLine("Failed to upload files!");
				}
			}

			
            //Update Crash with Summary and Description Info
            bool updateSuccess = false;

            LogFile.WriteLine("Attempting to add the crash description...");
            try
            {
                updateSuccess = reportService.AddCrashDescription(uniqueIndex, crashDescForm.crashDesc, crashDescForm.summary);
            }
            catch (Exception e)
            {
                LogFile.WriteLine("AutoReporter had an exception adding crash description! --> " + e.ToString());
                serviceException = e;
                serviceError = true;
                updateSuccess = false;
                LogFile.WriteLine(e.Message);
            }
       

			if(uniqueIndex != -1)
			{
				LogFile.WriteLine("Attempting to write the crash report URL...");

              #if DEBUG
                    string strCrashURL = Properties.Settings.Default.CrashURL_Debug + uniqueIndex;
#else
                string strCrashURL = Properties.Settings.Default.CrashURL + uniqueIndex;
                #endif
				LogFile.WriteLine("CrashReport url = " + strCrashURL);
				LogFile.WriteLine("");

				CrashURLDlg ShowURL = new CrashURLDlg();
				ShowURL.CrashURL_linkLabel.Text = strCrashURL;
				if(!bIsUnattended)
				{
					Clipboard.SetText( strCrashURL );
					ShowURL.ShowDialog();
				}

				if(reportData.PlatformName.Equals("PC", StringComparison.OrdinalIgnoreCase))
				{
					LogFile.WriteLine("Attempting to send the crash report Id to the game log file...");
					// On PC, so try just writing to the log.
					string AppLogFileName;

					AppLogFileName = reportData.BaseDir + "\\..\\" + reportData.GameName + "Game\\Logs\\Launch.log";

					LogFile.WriteLine("\n");
					LogFile.WriteLine("Attempting to open log file: " + AppLogFileName);

					try
					{
						using(StreamWriter AppLogWriter = new StreamWriter(AppLogFileName, true))
						{
							AppLogWriter.WriteLine("");
							AppLogWriter.WriteLine("CrashReport url = " + strCrashURL);
							AppLogWriter.WriteLine("");
							AppLogWriter.Flush();
						}
					}
					catch(System.Exception e)
					{
						LogFile.WriteLine("AutoReporter had an exception creating a stream writer! --> " + e.ToString());
					}
				}
			}

			if(updateSuccess)
			{
				LogFile.WriteLine("Successfully added crash description");
			}
			else
			{
				LogFile.WriteLine("Service failed to add crash description!");
				LogFile.Close();
				return;
			}

			LogFile.WriteLine("Closing the AutoReporter log file...");
			LogFile.Close();

			try
			{
				//everything was successful, so clean up dump and log files on client
				System.IO.File.Delete(args[0]);
				System.IO.File.Delete(args[2]);
				//todo: need to handle partial failure cases (some files didn't upload, etc) to know if we should delete the log
				//System.IO.File.Delete(logFileName);

			}
			catch(Exception e)
			{
				string ExcStr = "AutoReporter had an exception deleting the temp files!\n" + e.ToString();
				MessageBox.Show(ExcStr, "AutoReporter Status", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
			}

			if( bShowBalloon )
			{
				// Make sure the balloon has had time to display before we kill it
				System.Threading.Thread.Sleep( BalloonTimeInMs );
				crashDescForm.HideBalloon();
			}
		}
	}
}