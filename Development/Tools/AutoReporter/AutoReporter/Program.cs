using System;
using System.Collections.Generic;
using System.Configuration;
using System.Windows.Forms;
using System.IO;

namespace AutoReporter
{
    class OutputLogFile
    {
        private StreamWriter MyStreamWriter;

        public OutputLogFile(string filename)
        {
            try {
                MyStreamWriter = new StreamWriter(filename);
            }
            catch (Exception e)
            {
                MyStreamWriter = null;
            }
        }

        public void WriteLine(string line)
        {
            if (MyStreamWriter != null)
            {
                try {
                    MyStreamWriter.WriteLine(line);
                    MyStreamWriter.Flush();
                }
                catch (Exception e)
                {

                }
            }
        }

        public void Close()
        {
            if (MyStreamWriter!= null)
            {
                MyStreamWriter.Close();
            }
        }

    };

    static class Program
    {
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
            
            string logFileName;
            if (args.Length >= 2)
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
            LogFile.WriteLine("Arguments:");
            foreach (string arg in args)
            {
                LogFile.WriteLine(arg);
            }

            if (args.Length != 4)
            {
                LogFile.WriteLine("Expected 4 arguments: AutoReport Dump file name, Log file name, Ini dump file name and Mini dump file name");
                LogFile.Close();
                return;
            }

            
            ReportFile rFile = new ReportFile();
            ReportFileData reportData = new ReportFileData();
            if (!rFile.ParseReportFile(args[0], reportData, LogFile))
            {
                LogFile.Close();
                return;
            }

            ReportService.RegisterReport reportService = new ReportService.RegisterReport();

            Exception serviceException = new Exception("");
            bool serviceError = false;

            int uniqueIndex = -1;
            try {
                uniqueIndex = reportService.CreateNewReport(reportData.ComputerName, reportData.UserName, reportData.GameName, 
                    reportData.LanguageExt, reportData.TimeOfCrash, reportData.BuildVer, reportData.ChangelistVer, reportData.CommandLine, 
                    reportData.BaseDir, reportData.CallStack, reportData.EngineMode);
            }
            catch (Exception e)
            {
                serviceException = e;
                serviceError = true;
                LogFile.WriteLine(e.Message);
            }

            LogFile.WriteLine("");
            LogFile.WriteLine("uniqueIndex = " + uniqueIndex.ToString());
            LogFile.WriteLine("");

            if (uniqueIndex == -1)
            {
                LogFile.WriteLine("The service failed to create a new report!");
                serviceError = true;
                serviceException = new Exception("The service failed to create a new report!");
            }

            UploadReportFiles reportUploader = new UploadReportFiles();
            bool fileUploadSuccess = false;

            if (!serviceError)
            {
                try
                {
                    fileUploadSuccess = reportUploader.UploadFiles(args[1], args[2], args[3], uniqueIndex, LogFile);
                }
                catch (Exception e)
                {
                    serviceException = e;
                    serviceError = true;
                    LogFile.WriteLine(e.Message);
                }

                if (fileUploadSuccess)
                {
                    LogFile.WriteLine("Successfully uploaded files");
                }
                else
                {
                    LogFile.WriteLine("Failed to upload files!");
                }
            }
            
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);


            Form1 crashDescForm = new Form1();
            crashDescForm.summary = "Unattended mode";
            crashDescForm.crashDesc = "Unattended mode";

            if (!reportData.CommandLine.Contains("unattended"))
            {
                crashDescForm.SetCallStack(reportData.CallStack);
                if (serviceError)
                {
                    crashDescForm.SetServiceError(serviceException.Message);
                }
                Application.Run(crashDescForm);
            }

            LogFile.WriteLine("Crash Summary = " + crashDescForm.summary);
            LogFile.WriteLine("Crash Description = " + crashDescForm.crashDesc);

            bool updateSuccess = false;
            
            try 
            {
                updateSuccess = reportService.AddCrashDescription(uniqueIndex, crashDescForm.crashDesc, crashDescForm.summary);
            }
            catch (Exception e)
            {
                serviceException = e;
                serviceError = true;
                updateSuccess = false;
                LogFile.WriteLine(e.Message);
            }

            if (updateSuccess)
            {
                LogFile.WriteLine("Successfully added crash description");
            }
            else
            {
                LogFile.WriteLine("Service failed to add crash description!");
                LogFile.Close();
                return;
            }

            LogFile.Close();

            try {
                //everything was successful, so clean up dump and log files on client
                System.IO.File.Delete(args[0]);
                System.IO.File.Delete(args[2]);
                System.IO.File.Delete(logFileName);

            } catch (Exception e) {
                
            }
            
         }
    }
}