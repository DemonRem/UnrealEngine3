/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Management;
using System.Net;
using System.Net.Mail;
using System.Text;
using Controller;

namespace Controller
{
    public class Emailer
    {
        string MailServer = Properties.Settings.Default.MailServer;

        Main Parent;
        P4 SCC;

        public Emailer( Main InParent, P4 InP4 )
        {
            Parent = InParent;
            SCC = InP4;
            Parent.Log( "Emailer created successfully", Color.DarkGreen );
        }

        private string PerforceUserToEmail( string User )
        {
			// "john_scott" -> "john.scott@epicgames.com"
			string Email = User.Replace( '_', '.' );
			Email += "@epicgames.com";

			return ( Email );
        }

		private List<MailAddress> SplitDistro( string Emails )
		{
			List<MailAddress> MailAddresses = new List<MailAddress>();
			string[] Addresses = Emails.Split( ';' );
			foreach( string Address in Addresses )
			{
				if( Address.Trim().Length > 0 )
				{
					MailAddress AddrTo = null;
					try
					{
						AddrTo = new MailAddress( Address.Trim() );
					}
					catch
					{
					}

					if( AddrTo != null )
					{
						MailAddresses.Add( AddrTo );
					}
				}
			}

			return( MailAddresses );
		}

        private void SendMail( string To, string Cc, string Subject, string Body, bool CCEngineQA, bool CCIT, MailPriority Priority )
        {
#if !DEBUG
            try
            {
                SmtpClient Client = new SmtpClient( MailServer );
                if( Client != null )
                {
					MailAddress AddrTo = null;
					MailMessage Message = new MailMessage();

					Message.From = new MailAddress( Properties.Settings.Default.BuilderEmail );
                    Message.Priority = Priority;

                    if( To.Length > 0 )
                    {
						List<MailAddress> ToAddresses = SplitDistro( To );
						foreach( MailAddress Address in ToAddresses )
                    	{
                    		Message.To.Add(Address);
                    	}
                    }

					// Add in any carbon copy addresses
					if( Cc.Length > 0 )
					{
						List<MailAddress> CcAddresses = SplitDistro( Cc );
						foreach( MailAddress Address in CcAddresses )
						{
							Message.CC.Add( Address );
						}
					}

					if( CCEngineQA )
                    {
						AddrTo = new MailAddress( "EngineQA@epicgames.com" );
                        Message.CC.Add( AddrTo );
					
						Message.ReplyTo = AddrTo;
					}

					if( CCIT )
					{
						AddrTo = new MailAddress( "woody.ent@epicgames.com" );
						Message.CC.Add( AddrTo );

						AddrTo = new MailAddress( "shane.smith@epicgames.com" );
						Message.CC.Add( AddrTo );
					}

					// Quietly notify everyone that listens to build notifications
                    AddrTo = new MailAddress( Properties.Settings.Default.BuilderAdminEmail );
                    Message.Bcc.Add( AddrTo );

					Message.Subject = Subject;
                    Message.Body = Body;
                    Message.IsBodyHtml = false;

                    Client.Send( Message );

                    Parent.Log( "Email sent to: " + To, Color.Orange );
                }
            }
            catch( Exception e )
            {
                Parent.Log( "Failed to send email to: " + To, Color.Orange );
                Parent.Log( "'" + e.Message + "'", Color.Orange );
            }
#endif
		}

        private string BuildTime( ScriptParser Builder )
        {
            StringBuilder Result = new StringBuilder();

			Result.Append( "Build type: '" + Builder.BuildDescription + "'" + Environment.NewLine );
			Result.Append( "    Began at " + Builder.BuildStartedAt.ToLocalTime() + Environment.NewLine );
			Result.Append( "    Ended at " + DateTime.Now + Environment.NewLine );
			Result.Append( "    Elapsed Time was " );

			TimeSpan Duration = DateTime.UtcNow - Builder.BuildStartedAt;
            if( Duration.Hours > 0 )
            {
                Result.Append( Duration.Hours.ToString() + " hour(s) " );
            }
            if( Duration.Hours > 0 || Duration.Minutes > 0 )
            {
                Result.Append( Duration.Minutes.ToString() + " minute(s) " );
            }
            Result.Append( Duration.Seconds.ToString() + " second(s)" + Environment.NewLine );

            return( Result.ToString() );
        }

		private string GetReportStatus( ScriptParser Builder )
		{
			string ReportStatus = "";
			List<string> StatusReport = Builder.GetStatusReport();
			if( StatusReport.Count > 0 )
			{
				ReportStatus += Environment.NewLine;
				foreach( string Status in StatusReport )
				{
					ReportStatus += Status.Replace( "[REPORT]", "[WARNING]" ) + Environment.NewLine;
				}
			}

			return ( ReportStatus );
		}

        private string AddOperator( ScriptParser Builder, int CommandID, string To )
        {
            if( Builder.Operator.Length > 0 && Builder.Operator != "AutoTimer" && Builder.Operator != "LocalUser" )
            {
                if( To.Length > 0 )
                {
                    To += ";";
                }
                To += Builder.Operator + "@epicgames.com";
            }
            return ( To );
        }

        private string AddKiller( string Killer, int CommandID, string To )
        {
            if( Killer.Length > 0 && Killer != "LocalUser" )
            {
                if( To.Length > 0 )
                {
                    To += ";";
                }
                To += Killer + "@epicgames.com";
            }

            return ( To );
        }

        private string GetWMIValue( string Key, ManagementObject ManObject )
        {
            Object Value;

            try
            {
                Value = ManObject.GetPropertyValue( Key );
                if( Value != null )
                {
                    return ( Value.ToString() );
                }
            }
            catch
            {
            }

            return ( "" );
        }

        private string Signature()
        {
            return ( Environment.NewLine + "Cheers" + Environment.NewLine + Parent.MachineName + Environment.NewLine );
        }

		private string ConstructBaseSubject( string Subject, ScriptParser Builder, bool ShowGameAndPlatform, string Action )
		{
			// [BUILDER][BRANCH][GAME][PLATFORM][ACTION] Command
			if( Builder != null )
			{
				if( Builder.BranchDef.Branch.Length > "UnrealEngine3-".Length )
				{
					Subject += "[" + Builder.BranchDef.Branch.Substring( "UnrealEngine3-".Length ).ToUpper() + "]";
				}

				if( ShowGameAndPlatform && Builder.LabelInfo != null )
				{
					if( Builder.LabelInfo.Game.Length > 0 )
					{
						Subject += "[" + Builder.LabelInfo.Game.ToUpper() + "]";
					}
					if( Builder.LabelInfo.Platform.Length > 0 )
					{
						Subject += "[" + Builder.LabelInfo.Platform.ToUpper() + "]";
					}
				}
			}

			if( Action.Length > 0 )
			{
				Subject += "[" + Action.ToUpper() + "]";
			}

			return ( Subject );
		}

		private string ConstructSubject( string Subject, ScriptParser Builder, bool ShowGameAndPlatform, string Action, string Additional )
		{
			Subject = ConstructBaseSubject( Subject, Builder, ShowGameAndPlatform, Action );

			if( Builder != null )
			{
				Subject += " " + Builder.BuildDescription;
			}

			if( Additional.Length > 0 )
			{
				Subject += " (" + Additional + ")";
			}

			return ( Subject );
		}

        public void SendRestartedMail()
        {
            StringBuilder Body = new StringBuilder();

            string Subject = "[BUILDER] Builder synced and restarted!";

			// List installed software
            Body.Append( "Controller compiled: " + Parent.CompileDateTime.ToString() + Environment.NewLine + Environment.NewLine );
            Body.Append( "MSVC8 version: " + Parent.MSVC8Version + Environment.NewLine );
			Body.Append( "MSVC9 version: " + Parent.MSVC9Version + Environment.NewLine );
            Body.Append( "XDK version: " + Parent.XDKVersion + Environment.NewLine );
            Body.Append( "PS3 SDK version: " + Parent.PS3SDKVersion + Environment.NewLine );
            Body.Append( Environment.NewLine + "Path: " + Environment.GetEnvironmentVariable( "PATH" ) + Environment.NewLine + Environment.NewLine );

			// Available branches
			Body.Append( "Available branches -" + Environment.NewLine );
			foreach( Main.BranchDefinition Branch in Parent.AvailableBranches )
			{
				Body.Append( "Branch: '" + Branch.Branch + "' on server '" + Branch.Server + "'" + Environment.NewLine );
			}
			Body.Append( Environment.NewLine );

			// Type and speed of processor
            ManagementObjectSearcher Searcher = new ManagementObjectSearcher( "Select * from CIM_Processor" );
            ManagementObjectCollection Collection = Searcher.Get();

            foreach( ManagementObject Object in Collection )
            {
                Body.Append( GetWMIValue( "Name", Object ) + Environment.NewLine );
                break;
            }

			// Number of cores/threads
            Searcher = new ManagementObjectSearcher( "Select * from Win32_Processor" );
            Collection = Searcher.Get();

            foreach( ManagementObject Object in Collection )
            {
                string NumberOfProcs = GetWMIValue( "NumberOfLogicalProcessors", Object );
                Body.Append( "Processors: " + NumberOfProcs + Environment.NewLine );
                if( NumberOfProcs.Length > 0 )
                {
                    // Best guess at number of processors
                    int NumProcessors = Int32.Parse( NumberOfProcs );
                    if( NumProcessors < 2 )
                    {
                        NumProcessors = 2;
                    }
                    Parent.NumProcessors = NumProcessors;

                    // Best guess at number of jobs to spawn
                    int NumJobs = ( NumProcessors * 3 ) / 2;
                    if( NumJobs < 2 )
                    {
                        NumJobs = 2;
                    }
                    else if( NumJobs > 8 )
                    {
                        NumJobs = 8;
                    }
                    Parent.NumJobs = NumJobs;
                }
                break;
            }

			// BIOS type
            Searcher = new ManagementObjectSearcher( "Select * from CIM_BIOSElement" );
            Collection = Searcher.Get();

            foreach( ManagementObject Object in Collection )
            {
                Body.Append( GetWMIValue( "Name", Object ) + Environment.NewLine );
            }

            Body.Append( Environment.NewLine );

			// Amount and type of memory
            Searcher = new ManagementObjectSearcher( "Select * from CIM_PhysicalMemory" );
            Collection = Searcher.Get();
			long Memory = 0;

            foreach( ManagementObject Object in Collection )
            {
                string Capacity = GetWMIValue( "Capacity", Object );
                string Speed = GetWMIValue( "Speed", Object );
                Body.Append( "Memory: " + Capacity + " bytes at " + Speed + " MHz" + Environment.NewLine );

				Memory += Int64.Parse( Capacity );
            }

			Parent.PhysicalMemory = ( int )( Memory / ( 1024 * 1024 * 1024 ) );
			Body.Append( "Total memory: " + Parent.PhysicalMemory + " GB" + Environment.NewLine );

            Body.Append( Environment.NewLine );

			// Available disks and free space
            Searcher = new ManagementObjectSearcher( "Select * from Win32_LogicalDisk" );
            Collection = Searcher.Get();

            foreach( ManagementObject Object in Collection )
            {
                string DriveType = GetWMIValue( "DriveType", Object );
                if( DriveType == "3" )
                {
                    Int64 Size = 0;
                    Int64 FreeSpace = 0;

                    string Name = GetWMIValue( "Caption", Object );
                    string SizeInfo = GetWMIValue( "Size", Object );
                    string FreeSpaceInfo = GetWMIValue( "FreeSpace", Object );

                    try
                    {
                        Size = Int64.Parse( SizeInfo ) / ( 1024 * 1024 * 1024 );
                        FreeSpace = Int64.Parse( FreeSpaceInfo ) / ( 1024 * 1024 * 1024 );
                    }
                    catch
                    {
                    }

                    Body.Append( "'" + Name + "' hard disk: " + Size.ToString() + "GB (" + FreeSpace.ToString() + "GB free)" + Environment.NewLine );
                }
            }

            Body.Append( Signature() );

			SendMail( Properties.Settings.Default.BuilderAdminEmail, "", Subject, Body.ToString(), false, false, MailPriority.Low );
        }

        public void SendTriggeredMail( ScriptParser Builder, int CommandID )
        {
            string To = AddOperator( Builder, CommandID, Builder.TriggerAddress );

            string Subject = ConstructSubject( "[BUILDER]", Builder, false, "Triggered", "" );
            StringBuilder Body = new StringBuilder();

			Body.Append( "Build type: '" + Builder.BuildDescription + "'" + Environment.NewLine );
            Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), true, false, MailPriority.Low );
        }

        public void SendKilledMail( ScriptParser Builder, int CommandID, int BuildLogID, string Killer )
        {
            string To = AddOperator( Builder, CommandID, "" );
            To = AddKiller( Killer, CommandID, To );

			string Subject = ConstructSubject( "[BUILDER]", Builder, false, "Killed", "" );
            StringBuilder Body = new StringBuilder();

			Body.Append( BuildTime( Builder ) );
			Body.Append( "    Working from changelist " + Builder.LabelInfo.Changelist + Environment.NewLine );
			Body.Append( "    Started by " + Builder.Operator + Environment.NewLine );
            Body.Append( "    Killed by " + Killer + Environment.NewLine );
            Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), true, false, MailPriority.High );
        }

		public string PrintChangesInRange( ScriptParser Builder, string DepotPath, string StartingRevision, string EndingRevision, out string UsersToEmail )
		{
			StringBuilder Output = new StringBuilder();
			UsersToEmail = "";

			SCC.GetChangesInRange( Builder, DepotPath, StartingRevision, EndingRevision );
			foreach( ScriptParser.ChangeList CL in Builder.ChangeLists )
			{
				DateTime Time = new DateTime( 1970, 1, 1 );
				Time += new TimeSpan( ( long )CL.Time * 10000 * 1000 );
				Output.Append( Environment.NewLine + "Changelist " + CL.Number.ToString() + " submitted by " + CL.User + " on " + Time.ToLocalTime().ToString() + Environment.NewLine );
				Output.Append( Environment.NewLine + CL.Description + Environment.NewLine );
				Output.Append( "Affected files..." + Environment.NewLine + Environment.NewLine );
				foreach( string CLFile in CL.Files )
				{
					Output.Append( CLFile + Environment.NewLine );
				}
				Output.Append( Environment.NewLine );
				Output.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );
				Output.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );

				// Compose the user's email address
				string FullName, EmailAddress;
				if( SCC.GetUserInformation( Builder, CL.User, out FullName, out EmailAddress ) )
				{
					UsersToEmail += ( FullName + "<" + EmailAddress + ">" + ";" );
				}
			}

			return Output.ToString();
		}

		private string HandleCISMessage( ScriptParser Builder, string FailureMessage, ref StringBuilder Body )
		{
			// Determine if it's possible this change actually broke the build
			string BuildName = "";
			string Additional = "";
			int LastGoodCL = 9999999;
			int LastFailedCL = 0;
			bool bValidBuildNameAndCLs = false;

			try
			{
				// Extract the build name
				int LParenthesis = Builder.BuildDescription.IndexOf( '(' );
				int RParenthesis = Builder.BuildDescription.LastIndexOf( ')' );
				if( LParenthesis > 0 && RParenthesis > 0 && RParenthesis - LParenthesis > 1 )
				{
					BuildName = Builder.BuildDescription.Substring( LParenthesis + 1, RParenthesis - ( LParenthesis + 1 ) );
					if( BuildName.Length > 0 )
					{
						// Query for the last good and failed changelists
						LastGoodCL = Parent.DB.GetIntForBranch( Builder.BranchDef.Branch, "LastGood" + BuildName );
						if( LastGoodCL > 0 )
						{
							LastFailedCL = Parent.DB.GetIntForBranch( Builder.BranchDef.Branch, "LastFail" + BuildName );
							if( LastFailedCL > 0 )
							{
								bValidBuildNameAndCLs = true;
							}
						}
					}
				}
			}
			catch
			{
				Body.Append( "[For Build System Admins] Unable to determine extract build name from : " + Builder.BuildDescription + Environment.NewLine );
			}

			// Check for any known systemic issues and adjust the message
			bool bIsSystemicFailure = false;
			if( FailureMessage.Contains( " TIMED OUT after " ) )
			{
				bIsSystemicFailure = true;
			}

			if( bValidBuildNameAndCLs && !bIsSystemicFailure )
			{
				// If the current state of the build is good, then this CL might have broken the build
				if( LastGoodCL > LastFailedCL )
				{
					Additional = " NEWLY BROKEN IN CHANGELIST " + Builder.LabelInfo.Changelist.ToString() + "!";
					Body.Append( "A CIS Failure has been detected and it's HIGHLY LIKELY that it's a result of your changes." + Environment.NewLine );
					Body.Append( "             Your changelist was: " + Builder.LabelInfo.Changelist.ToString() + Environment.NewLine );
					Body.Append( "    The last good changelist was: " + LastGoodCL.ToString() + Environment.NewLine + Environment.NewLine );
					Body.Append( "It is possible that a change between the last good changelist and yours is actually responsible, or the failure is systemic. Please verify." + Environment.NewLine );
					Body.Append( Environment.NewLine );
				}
				else
				{
					Additional = " Already broken";
					Body.Append( "A CIS Failure has been detected but it's unlikely a result of your changes." + Environment.NewLine );
					Body.Append( "             Your changelist was: " + Builder.LabelInfo.Changelist.ToString() + Environment.NewLine );
					Body.Append( "    The last good changelist was: " + LastGoodCL.ToString() + Environment.NewLine );
					Body.Append( Environment.NewLine );
				}
			}
			else
			{
				Body.Append( "A CIS Failure has been detected and it's possible that it's a result of your changes." + Environment.NewLine );
				Body.Append( "             Your changelist was: " + Builder.LabelInfo.Changelist.ToString() + Environment.NewLine );
				Body.Append( "    The last good changelist was: " + LastGoodCL.ToString() + Environment.NewLine );
				Body.Append( "It is possible that a change between the last good changelist and yours is actually responsible, or the failure is systemic. Please verify." + Environment.NewLine );
				Body.Append( Environment.NewLine );
			}

			string CISWarningMessage = "So, you have either broken the build or checked in something while the build was " +
									   "broken by someone else; the changes you made are listed below. If you're responsible " +
									   "for the breakage, please fix as soon as possible.";

			Body.Append( CISWarningMessage + Environment.NewLine );
			Body.Append( Environment.NewLine );
			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );
			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );
			Body.Append( Environment.NewLine );

			return ( Additional );
		}

        public void SendFailedMail( ScriptParser Builder, int CommandID, int BuildLogID, string FailureMessage, string LogFileName, string OptionalDistro )
        {
            // It's a job, so tag the failure message to the active label (if it's not a CIS job)
			if( CommandID == 0 && Builder.IsPrimaryBuild )
            {
                string JobStatus = Environment.NewLine + "Job failed on " + Parent.MachineName + ":" + Environment.NewLine;
                JobStatus += "Detailed log copied to: " + Properties.Settings.Default.FailedLogLocation.Replace( '/', '\\' ) + "\\" + LogFileName + Environment.NewLine + Environment.NewLine;
                JobStatus += FailureMessage;
                JobStatus += Environment.NewLine;

                SCC.TagMessage( Builder, JobStatus );
                return;
            }

            // It's a normal build script - send a mail as usual
            string To = AddOperator( Builder, CommandID, Builder.FailAddress + ";" + OptionalDistro );
			string Subject = "";

            StringBuilder Body = new StringBuilder();

			if( CommandID != 0 || Builder.IsPrimaryBuild )
			{
				// Report the full info for a build failing
				Subject = ConstructSubject( "[BUILDER]", Builder, true, "Failed", "" );
			
				Body.Append( BuildTime( Builder ) );

				// Only report these for primary builds
				Body.Append( "    Current failing changelist " + Builder.LabelInfo.Changelist + Environment.NewLine );
				Body.Append( "    Last successful changelist " + Builder.LastGoodBuild + Environment.NewLine );
			}
			else
			{
				// If this is a CIS job, issue the standard warning
				Subject = ConstructBaseSubject( "[CIS]", Builder, true, "Failed" );

				Subject += HandleCISMessage( Builder, FailureMessage, ref Body );

				Body.Append( BuildTime( Builder ) );
			}

			Body.Append( Environment.NewLine + "Detailed log copied to: " + Properties.Settings.Default.FailedLogLocation.Replace( '/', '\\' ) + "\\" + LogFileName + Environment.NewLine + Environment.NewLine);

			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );
            Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );

			// If this is a build running from a single changelist, describe it
			if( Builder.LabelInfo.RevisionType == RevisionType.ChangeList )
			{
				string UserEmail;
				Body.Append( PrintChangesInRange( Builder, "...", Builder.LabelInfo.Changelist.ToString(), Builder.LabelInfo.Changelist.ToString(), out UserEmail ) );
				To += ";" + UserEmail;
			}

			Body.Append( Environment.NewLine + FailureMessage + Environment.NewLine + Environment.NewLine );
			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );
			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );

			Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), Builder.OfficialBuild, false, MailPriority.High );
        }

        // Sends mail stating the version of the build that was just created
        public void SendSucceededMail( ScriptParser Builder, int CommandID, int BuildLogID, string FinalStatus )
        {
            string To = AddOperator( Builder, CommandID, Builder.SuccessAddress );

            string Label = Builder.LabelInfo.GetLabelName();
			string Subject = ConstructSubject( "[BUILDER]", Builder, false, "Succeeded", Label );
			StringBuilder Body = new StringBuilder();

            Body.Append( BuildTime( Builder ) );
			Body.Append( "    Current successful changelist " + Builder.LabelInfo.Changelist + Environment.NewLine );
			Body.Append( "         First failing changelist " + Builder.LastFailedBuild + Environment.NewLine );

            if( Builder.NewLabelCreated )
            {
                if( Label.Length > 0 )
                {
                    Body.Append( Environment.NewLine + "Build is labeled '" + Label + "'" + Environment.NewLine + Environment.NewLine );
                }
            }

			Body.Append( GetReportStatus( Builder ) );

            Body.Append( Environment.NewLine + FinalStatus + Environment.NewLine );
            Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), true, false, MailPriority.Normal );
        }

        // Sends mail stating the version of the build has started passing again
        public void SendNewSuccessMail( ScriptParser Builder, int CommandID, int BuildLogID, string FinalStatus )
        {
            string To = AddOperator( Builder, CommandID, Builder.SuccessAddress );

			int Changelist = Builder.LabelInfo.Changelist;
			string Label = Builder.LabelInfo.GetLabelName();
			string Subject = ConstructSubject( "[BUILDER]", Builder, false, "Fixed", Changelist.ToString() );

			StringBuilder Body = new StringBuilder();

            Body.Append( BuildTime( Builder ) );
			Body.Append( "    Current successful changelist " + Builder.LabelInfo.Changelist + Environment.NewLine );
			Body.Append( "         First failing changelist " + Builder.LastFailedBuild + Environment.NewLine );

            if( Builder.NewLabelCreated )
            {
                if( Label.Length > 0 )
                {
                    Body.Append( Environment.NewLine + "Build is labeled '" + Label + "'" + Environment.NewLine + Environment.NewLine );
                }
            }

            Body.Append( Environment.NewLine + FinalStatus + Environment.NewLine );
            Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), true, false, MailPriority.Normal );
        }

		// Sends mail stating the version of the build has started passing again
		public void SendSuppressedMail( ScriptParser Builder, int CommandID, int BuildLogID, string Status )
		{
			string To = AddOperator( Builder, CommandID, Builder.SuccessAddress );

			int Changelist = Builder.LabelInfo.Changelist;
			string Label = Builder.LabelInfo.GetLabelName();
			string Subject = ConstructSubject( "[BUILDER]", Builder, true, "Suppressed", Changelist.ToString() );
			StringBuilder Body = new StringBuilder();

			if( Builder.NewLabelCreated )
			{
				if( Label.Length > 0 )
				{
					Body.Append( Environment.NewLine + "Build is labeled '" + Label + "'" + Environment.NewLine + Environment.NewLine );
				}
			}

			Body.Append( Environment.NewLine + Status + Environment.NewLine );
			Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), Builder.OfficialBuild, false, MailPriority.Normal );
		}

		// Sends mail stating the version of the build has started passing again
		public void SendCISMail( ScriptParser Builder, string BuildName, string StartingRevision, string EndingRevision )
		{
			string Subject = ConstructBaseSubject( "[CIS]", Builder, false, "Fixed" );
			Subject += " Fixed in changelist " + EndingRevision.ToString();

			StringBuilder Body = new StringBuilder();

			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );
			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );

			Body.Append( Environment.NewLine );
			Body.Append( BuildTime( Builder ) );
			Body.Append( "    First changelist of this range " + StartingRevision + Environment.NewLine );
			Body.Append( "     Last changelist of this range " + EndingRevision + Environment.NewLine );
			Body.Append( Environment.NewLine );

			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );
			Body.Append( "-------------------------------------------------------------------------------" + Environment.NewLine );

			string UserEmail;
			Body.Append( PrintChangesInRange( Builder, "...", StartingRevision, EndingRevision, out UserEmail ) );
			string To = UserEmail;

			Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), true, false, MailPriority.Normal );
		}
		
		// Sends mail stating the version of the build that was used to create the data
        public void SendPromotedMail( ScriptParser Builder, int CommandID )
        {
            string To = AddOperator( Builder, CommandID, Builder.SuccessAddress );

			string Subject = ConstructSubject( "[BUILDER]", Builder, true, "Promoted", Builder.LabelInfo.GetLabelName() );
			StringBuilder Body = new StringBuilder();

            string Label = Builder.LabelInfo.GetLabelName();
            if( Label.Length > 0 )
            {
                Body.Append( Environment.NewLine + "The build labeled '" + Label + "' was promoted (details below)" + Environment.NewLine + Environment.NewLine );
            }

            Body.Append( Environment.NewLine + Builder.GetChanges( "" ) + Environment.NewLine );
            Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), true, false, MailPriority.Normal );
        }

        // Sends mail stating the changes on a per user basis
        public void SendUserChanges( ScriptParser Builder, string User )
        {
            string QAChanges = Builder.GetChanges( User );

            // If there are no changes, don't send an email
            if( QAChanges.Length == 0 )
            {
                return;
            }

			string Email = PerforceUserToEmail( User );

            string Subject = "[QA] 'QA Build Changes' (" + Builder.LabelInfo.GetLabelName() + ")";
            StringBuilder Body = new StringBuilder();

            Body.Append( "When creating your Build Summary notes, if it is possible, please include the relevant changelist #s next to your build summary entries." + Environment.NewLine );
            Body.Append( Environment.NewLine );
			Body.Append( "Thanks!" + Environment.NewLine );
			Body.Append( Environment.NewLine );
			Body.Append( "--Major Tasks--" + Environment.NewLine );
			Body.Append( "Please list the major focus of your work in the engine/editor this month.  More for marketing newsletters and PR so please list this as a high level overview and not a technical write-up." + Environment.NewLine );
            Body.Append( Environment.NewLine );
            Body.Append( "Example:" + Environment.NewLine );
            Body.Append( "Increasing console performance and decreasing memory used." + Environment.NewLine );
            Body.Append( "" + Environment.NewLine );
			Body.Append( "--Build Upgrade Notes (BUNs)--" + Environment.NewLine );
			Body.Append( "A more technical write-up of what your main focus has been.  The Build Summary can still contain all the details of that work." + Environment.NewLine );
            Body.Append( Environment.NewLine );
			Body.Append( "* New tools/features being worked on but not quite ready for use." + Environment.NewLine );
			Body.Append( "   * This is an example of a great new tool on the way, but still needing work." + Environment.NewLine );
			Body.Append( "* New tools/features ready for use." + Environment.NewLine );
			Body.Append( "* Changes requiring a re-save or re-compile." + Environment.NewLine );
			Body.Append( "* Subtle changes that might be tricky to merge." + Environment.NewLine );
			Body.Append( Environment.NewLine );
			Body.Append( "--Build Summary--" + Environment.NewLine );
			Body.Append( "Please start all lines in the summary with uppercase letters and try to use the grouping outlined below to add your changes.  Do not use tabs, but instead use 3 spaces to delineate each new section." + Environment.NewLine );
            Body.Append( Environment.NewLine );
			Body.Append( "Feel free to create new sub-sections within the Engine, Rendering, etc. sections to better group your changes." + Environment.NewLine );
			Body.Append( Environment.NewLine );
			Body.Append( "General:" + Environment.NewLine );
            Body.Append( "   * Engine" + Environment.NewLine );
            Body.Append( "      * Misc" + Environment.NewLine );
            Body.Append( "         * This is an example of a general bug fix in the engine" + Environment.NewLine );
			Body.Append( "      * AI" + Environment.NewLine );
			Body.Append( "      * Animation" + Environment.NewLine );
            Body.Append( "      * Audio" + Environment.NewLine );
            Body.Append( "      * Cooker" + Environment.NewLine );
			Body.Append( "      * Crowd System" + Environment.NewLine );
			Body.Append( "      * Dedicated Server" + Environment.NewLine );
			Body.Append( "      * Live" + Environment.NewLine );
            Body.Append( "      * Loc" + Environment.NewLine );
			Body.Append( "      * Network" + Environment.NewLine );
			Body.Append( "      * Optimizations" + Environment.NewLine );
            Body.Append( "      * Particles" + Environment.NewLine );
            Body.Append( "      * Physics" + Environment.NewLine );
            Body.Append( "      * Script Compiler" + Environment.NewLine );
            Body.Append( "   * Rendering" + Environment.NewLine );
			Body.Append( "      * Decals" + Environment.NewLine );
			Body.Append( "      * Misc" + Environment.NewLine );
            Body.Append( "      * Optimizations" + Environment.NewLine );
			Body.Append( "      * Proc Buildings" + Environment.NewLine );
            Body.Append( "   * Editor" + Environment.NewLine );
            Body.Append( "      * Misc" + Environment.NewLine );
            Body.Append( "      * AnimSet Viewer" + Environment.NewLine );
			Body.Append( "      * Cascade" + Environment.NewLine );
			Body.Append( "      * Kismet" + Environment.NewLine );
            Body.Append( "      * Matinee" + Environment.NewLine );
            Body.Append( "      * Preferences" + Environment.NewLine );
            Body.Append( "      * UI Editor" + Environment.NewLine );
            Body.Append( "      * Viewports" + Environment.NewLine );
            Body.Append( "   * Tools" + Environment.NewLine );
			Body.Append( "      * Misc" + Environment.NewLine );
			Body.Append( "      * Build system" + Environment.NewLine );
            Body.Append( "      * CIS" + Environment.NewLine );
            Body.Append( "      * UnrealConsole" + Environment.NewLine );
            Body.Append( "      * UnrealFrontend" + Environment.NewLine );
            Body.Append( "      * UnrealProp" + Environment.NewLine );
            Body.Append( "   * UI" + Environment.NewLine );	
            Body.Append( "      * Misc" + Environment.NewLine );
            Body.Append( "      * Data stores" + Environment.NewLine );
            Body.Append( "      * UI Editor" + Environment.NewLine );
			Body.Append( "   * UTGame" + Environment.NewLine );
			Body.Append( "      * Misc" + Environment.NewLine );
			Body.Append( "      * Content" + Environment.NewLine );
			Body.Append( "" + Environment.NewLine );
            Body.Append( "Xbox 360:" + Environment.NewLine );
            Body.Append( "   * Rendering" + Environment.NewLine );
            Body.Append( "   * Misc" + Environment.NewLine );
            Body.Append( Environment.NewLine );
            Body.Append( "PS3:" + Environment.NewLine );
            Body.Append( "   * Rendering" + Environment.NewLine );
            Body.Append( "   * Misc" + Environment.NewLine );
			Body.Append( "Mobile:" + Environment.NewLine );
			Body.Append( "   * Rendering" + Environment.NewLine );
			Body.Append( "   * Misc" + Environment.NewLine );

            string Label = Builder.LabelInfo.GetLabelName();
            if( Label.Length > 0 )
            {
                Body.Append( Environment.NewLine + "The build labeled '" + Label + "' is the QA build (details below)" + Environment.NewLine + Environment.NewLine );
            }

            Body.Append( Environment.NewLine + QAChanges + Environment.NewLine );
            Body.Append( Signature() );

			SendMail( Email, Builder.CarbonCopyAddress, Subject, Body.ToString(), true, false, MailPriority.Normal );
        }

        // Send a mail stating a cook has been copied to the network
        public void SendPublishedMail( ScriptParser Builder, int CommandID, int BuildLogID )
        {
            string To = AddOperator( Builder, CommandID, Builder.SuccessAddress );

			string Subject = ConstructSubject( "[BUILDER]", Builder, true, "Published", Builder.GetFolderName() );
			StringBuilder Body = new StringBuilder();

            Body.Append( BuildTime( Builder ) );

            List<string> Dests = Builder.GetPublishDestinations();
            if( Dests.Count > 0 )
            {
                Body.Append( Environment.NewLine + "Build was published to -" + Environment.NewLine );
                foreach( string Dest in Dests )
                {
					string WindowsFriendlyDest = Dest.Replace( "/", "\\" );
					Body.Append( "\t\"" + WindowsFriendlyDest + " \"" + Environment.NewLine );
                }
            }

			Body.Append( GetReportStatus( Builder ) );

            string Label = Builder.SyncedLabel;
            if( Label.Length > 0 )
            {
                Body.Append( Environment.NewLine + "The build labeled '" + Label + "' was used to cook the data (details below)" + Environment.NewLine + Environment.NewLine );
            }

            Body.Append( Environment.NewLine + Builder.GetChanges( "" ) + Environment.NewLine );
            Body.Append( Signature() );

			SendMail( To, Builder.CarbonCopyAddress, Subject, Body.ToString(), true, false, MailPriority.Normal );
        }

        public void SendAlreadyInProgressMail( string Operator, int CommandID, string BuildType )
        {
            // Don't send in progress emails for CIS or Sync type builds
            if( BuildType.StartsWith( "CIS" ) || BuildType.StartsWith( "Sync" ) )
            {
                return;
            }

            string To = Operator + "@epicgames.com";
			string Subject = ConstructSubject( "[BUILDER]", null, false, "InProgress", BuildType );
			StringBuilder Body = new StringBuilder();

            string FinalStatus = "Build '" + BuildType + "' not retriggered because it is already building.";

            Body.Append( Environment.NewLine + FinalStatus + Environment.NewLine );
            Body.Append( Signature() );

			SendMail( To, "", Subject, Body.ToString(), false, false, MailPriority.Normal );
        }

		private void SendInfoMail( string InfoType, string Title, string Content, bool CCIT, MailPriority Priority )
		{
			string Subject = "[BUILDER] " + InfoType + " (" + Title + ")";

			StringBuilder Body = new StringBuilder();
			Body.Append( Content + Environment.NewLine );
			Body.Append( Signature() );

			SendMail( "", "", Subject, Body.ToString(), false, CCIT, Priority );
		}

        public void SendWarningMail( string Title, string Warnings, bool CCIT )
        {
			SendInfoMail( "Warning!", Title, Warnings, CCIT, MailPriority.Normal );
        }

		public void SendErrorMail( string Title, string Errors, bool CCIT )
		{
			SendInfoMail( "Error!", Title, Errors, CCIT, MailPriority.High );
		}

        public void SendGlitchMail()
        {
            string To = "john.scott@epicgames.com";

            string Subject = "[BUILDER] NETWORK GLITCH";
            StringBuilder Body = new StringBuilder();

            Body.Append( Environment.NewLine + "Network diagnostics" + Environment.NewLine + Environment.NewLine );

            ManagementObjectSearcher Searcher = new ManagementObjectSearcher( "Select * from Win32_NetworkConnection" );
            ManagementObjectCollection Collection = Searcher.Get();

            foreach( ManagementObject Object in Collection )
            {
                Body.Append( GetWMIValue( "LocalName", Object ) + Environment.NewLine );
                Body.Append( GetWMIValue( "Name", Object ) + Environment.NewLine );
                Body.Append( GetWMIValue( "RemoteName", Object ) + Environment.NewLine );
                Body.Append( GetWMIValue( "ProviderName", Object ) + Environment.NewLine );
                Body.Append( GetWMIValue( "ResourceType", Object ) + Environment.NewLine );

                Body.Append( GetWMIValue( "Caption", Object ) + Environment.NewLine );
                Body.Append( GetWMIValue( "ConnectionState", Object ) + Environment.NewLine );
                Body.Append( GetWMIValue( "ConnectionType", Object ) + Environment.NewLine );
                Body.Append( GetWMIValue( "DisplayType", Object ) + Environment.NewLine );
                Body.Append( GetWMIValue( "Status", Object ) + Environment.NewLine );
                Body.Append( Environment.NewLine );
            }

            Body.Append( Signature() );

			SendMail( To, "", Subject, Body.ToString(), true, true, MailPriority.High );
        }
    }
}
