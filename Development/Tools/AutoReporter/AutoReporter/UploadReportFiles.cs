using System;
using System.Net;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Text;
using System.IO;

namespace AutoReporter
{
    class UploadReportFiles
    {
        /** 
         * UploadFiles - uploads the two files using HTTP POST
         * 
         * @param LogFilename - the local log file
         * @param IniFilename - the local ini file
         * @param uniqueID - the id of the crash report associated with these files. 
         * @return bool - true if successful
         */
        public bool UploadFiles(string LogFilename, string IniFilename, string MiniDumpFilename, int uniqueID, OutputLogFile logFile)
        {
            logFile.WriteLine("Uploading files " + LogFilename + ", " + IniFilename + " and " + MiniDumpFilename);

            try {
                WebClient client = new WebClient();
                string UploadReportURL = "http://crashreport/UploadReportFiles.aspx";
                //string UploadReportURL = "http://localhost/AutoReportService/UploadReportFiles.aspx";

                client.Headers.Add("NewFolderName", uniqueID.ToString());

                byte[] responseArray = client.UploadFile(UploadReportURL, "POST", LogFilename);
                responseArray = client.UploadFile(UploadReportURL, "POST", IniFilename);

                client.Headers.Add("SaveFileName", "MiniDump.dmp");
                responseArray = client.UploadFile(UploadReportURL, "POST", MiniDumpFilename);
                
            } catch (WebException webEx) {
                logFile.WriteLine(webEx.Message);
                return false;
            }

            return true;
        }
    }
}
