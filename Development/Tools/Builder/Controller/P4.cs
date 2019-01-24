/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Management;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using P4API;

namespace Controller
{
    public class P4
    {
        private Main Parent;
		private P4Connection IP4Net = null;
		private P4PendingChangelist Pending = null;
		private COMMANDS ErrorLevel = COMMANDS.None;
		private bool bSilenced = false;
        private int RetrySleepTimeInMS = 60 * 1000;
        private int MaxRetryCount = 3;

        public P4( Main InParent )
        {
            Parent = InParent;
            try
            {
				IP4Net = new P4Connection();
            }
            catch
            {
                Parent.Log( "Perforce connection FAILED", Color.Red );
                IP4Net = null;
                Parent.Ticking = false;
            }
        }

		public COMMANDS GetErrorLevel()
        {
            return ( ErrorLevel );
        }

        private void Write( ScriptParser Builder, P4RecordSet Records )
        {
			if( Builder != null && Records != null )
			{
				Builder.Write( Records.Errors.Length.ToString() + " errors reported" );
				foreach( string Error in Records.Errors )
				{
					Builder.Write( "Error: " + Error );
				}

				Builder.Write( Records.Warnings.Length.ToString() + " warnings reported" );
				foreach( string Warning in Records.Warnings )
				{
					Builder.Write( "Warning: " + Warning );
				}

				Builder.Write( Records.Messages.Length.ToString() + " messages" );
				foreach( string Message in Records.Messages )
				{
					Builder.Write( "Message: " + Message );
				}
			}
        }

		private void CreatePendingChangelist( ScriptParser Builder )
		{
			if( Pending == null )
			{
				try
				{
					IP4Net.Port = Builder.ServerInfo.Server;
					IP4Net.User = Builder.ServerInfo.User;
					IP4Net.Password = Builder.ServerInfo.Password;
					IP4Net.Connect();
					IP4Net.Client = Parent.MachineName;
					IP4Net.CWD = Environment.CurrentDirectory;
					IP4Net.ExceptionLevel = P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

					// Create a new pending changelist
					Pending = IP4Net.CreatePendingChangelist( "[BUILDER] '" + Builder.LabelInfo.BuildType + "' built from changelist " + Builder.LabelInfo.Changelist.ToString() );
					IP4Net.Disconnect();
				}
				catch( Exception Ex )
				{
					IP4Net.Disconnect();
					Builder.Write( "P4ERROR: CreatePendingChangelist " + Ex.Message );
					ErrorLevel = COMMANDS.SCC_CreatePendingChangelist;
				}
			}
		}

		private string LabelToTag( ScriptParser Builder )
		{
			string Revision = Parent.GetLabelToSync();
			string LabelName = Revision.TrimStart( "@".ToCharArray() );
			if( LabelName == "#head" )
			{
				Builder.Write( "P4ERROR: LabelToTag - Non existent label, cannot tag file to #head" );
				return( "" );
			}

			if( !GetLabelInfo( Builder, LabelName, null ) )
			{
				Builder.Write( "P4ERROR: LabelToTag - Non existent label, cannot tag file to: " + LabelName );
				return( "" );
			}

			return ( LabelName );
		}

		public bool GetClientRoot( Main.PerforceServerInfo ServerInfo )
		{
			Parent.RootFolder = "";
			P4RecordSet Records = RunSimpleCommand( ServerInfo, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetClientRoot, "info" );
			if( Records != null && Records.Records.Length > 0 )
			{
				Parent.RootFolder = Records.Records[0].Fields["clientRoot"].Trim();
			}

			return ( Parent.RootFolder.Length > 0 );
		}

		/**
		 * Run a simple Perforce command with retry
		 */
		private P4RecordSet RunSimpleCommand( Main.PerforceServerInfo ServerInfo, P4ExceptionLevels ExceptionLevel, COMMANDS ErrorType, string Command, params string[] Parameters )
		{
			P4RecordSet Output = null;
			bool bTransactionCompleted = false;
			int RetryCount = 0;

			// Work out the full description of the command
			string FullCommand = Command;
			foreach( string Parameter in Parameters )
			{
				FullCommand += " " + Parameter;
			}

			// Try the Perforce operation with retry if necessary
			while( !bTransactionCompleted )
			{
				try
				{
					ErrorLevel = COMMANDS.None;

					IP4Net.Port = ServerInfo.Server;
					IP4Net.User = ServerInfo.User;
					IP4Net.Password = ServerInfo.Password;
					IP4Net.Connect();
					IP4Net.Client = Parent.MachineName;
					IP4Net.CWD = Environment.CurrentDirectory;
					IP4Net.ExceptionLevel = ExceptionLevel;

					if( !bSilenced )
					{
						string Message = "Perforce: 'p4 " + FullCommand + "' on server " + ServerInfo.Server;
						if( RetryCount > 0 )
						{
							Message += " (retry #" + RetryCount.ToString() + ")";
						}
						Parent.Log( Message, Color.DarkGreen );
					}

					Output = IP4Net.Run( Command, Parameters );

					IP4Net.Disconnect();
					bTransactionCompleted = true;
				}
				catch( Exception Ex )
				{
					IP4Net.Disconnect();

					// If we're retrying, sleep for a bit and loop
					if( RetryCount < MaxRetryCount )
					{
						Parent.SendWarningMail( "Perforce communication error, retrying", "Command: " + FullCommand + Environment.NewLine + Environment.NewLine +
																						  "Server: " + ServerInfo.Server + Environment.NewLine +
																						  "Exception: " + Ex.Message, true );
						RetryCount++;

						// Allow the dialog to be responsive while sleeping
						int MaxCount = RetrySleepTimeInMS / 100;
						for( int Counter = 0; Counter < MaxCount; Counter++ )
						{
							Application.DoEvents();
							Thread.Sleep( 100 );
						}
					}
					else
					{
						// Otherwise, set the error and mark the transaction complete
						ErrorLevel = ErrorType;
						bTransactionCompleted = true;
					}
				}
			}

			return ( Output );
		}

		private P4RecordSet RunSimpleCommand( ScriptParser Builder, P4ExceptionLevels ExceptionLevel, COMMANDS ErrorType, string Command, params string[] Parameters )
		{
			Builder.Write( "Attempting 'p4 " + Command + "'" );
			P4RecordSet Records = RunSimpleCommand( Builder.ServerInfo, ExceptionLevel, ErrorType, Command, Parameters );
			if( ErrorLevel == ErrorType )
			{
				Builder.Write( "P4ERROR: Failed Perforce command '" + Command + "'" );
			}

			Write( Builder, Records );
			return( Records );
		}

        /** 
         * Converts a client filespec (e.g. //BUILDFARM/UnrealEngine3-Chair/...) to the depot equivalent
         * (e.g. //depot/ChairGame/main/...)
         */
        private string ConvertClientToDepotFileSpec( Main.PerforceServerInfo ServerInfo, string ClientFileSpec )
        {
            string DepotFileSpec = ClientFileSpec;

			P4RecordSet Records = RunSimpleCommand( ServerInfo, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetDepotFileSpec, "where", ClientFileSpec );
			if( Records != null && Records.Records.Length > 0 )
			{
				DepotFileSpec = Records.Records[0].Fields["depotFile"];
			}

			return ( DepotFileSpec );
        }

		private string ConvertClientToLocalFileSpec( Main.PerforceServerInfo ServerInfo, string ClientFileSpec )
		{
			string LocalFileSpec = ClientFileSpec;

			P4RecordSet Records = RunSimpleCommand( ServerInfo, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetDepotFileSpec, "where", ClientFileSpec );
			if( Records != null && Records.Records.Length > 0 )
			{
				LocalFileSpec = Records.Records[0].Fields["path"];
			}

			return ( LocalFileSpec );
		}

		/**
		 * Make sure Perforce is set up how we like
		 */
		public bool ValidateP4Settings( ScriptParser Builder )
		{
			bool bP4HasValidSettings = false;

			Main.PerforceServerInfo ServerInfo = new Main.PerforceServerInfo( Parent.DB, Parent.LabelRoot );

			P4RecordSet Records = RunSimpleCommand( ServerInfo, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetClientBranches, "client", "-o" );
			if( Records != null && Records.Records.Length > 0 )
			{
				// Ensure settings are correct
				bP4HasValidSettings = true;
				
				// Make sure the clientspec matches the machine name
				if( Records.Records[0]["Client"].ToLower() != Parent.MachineName.ToLower() )
				{
					Builder.Write( "VALIDATION ERROR: Clientspec name does not match machine name: " + Records.Records[0]["Client"] + " != " + Parent.MachineName );
					bP4HasValidSettings = false;
				}

				string Options = Records.Records[0]["Options"].Trim().ToLower();
				string[] ParsedOptions = Options.Split( " \t".ToCharArray() );
				foreach( string ParsedOption in ParsedOptions )
				{
					if( ParsedOption == "nomodtime" )
					{
						Builder.Write( "VALIDATION ERROR: modtime is not set" );
						bP4HasValidSettings = false;
					}
				}
			}

			return ( bP4HasValidSettings );
		}

		/**
		 * Gets a list of branches this clientpsec operates on
		 */
		public List<string> GetClientBranches( Main.PerforceServerInfo ServerInfo )
		{
			List<string> ClientBranches = new List<string>();

			P4RecordSet Records = RunSimpleCommand( ServerInfo, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetClientBranches, "client", "-o" );
			if( Records != null && Records.Records.Length > 0 && Records.Records[0].ArrayFields["View"] != null )
			{
				foreach( string View in Records.Records[0].ArrayFields["View"] )
				{
					// "View0 //depot/ChairGames/main/... //JSCOTT-X-30/UnrealEngine3-Chair/..."
					string[] Parms = View.Trim().Split( " ".ToCharArray() );

					// "View0" "//depot/ChairGames/main/..." "//JSCOTT-X-30/UnrealEngine3-Chair/..."
					if( Parms.Length < 2 )
					{
						continue;
					}

					// "//depot/ChairGames/main/..."
					if( Parms[0].StartsWith( "-" ) )
					{
						continue;
					}

					// "" "" "JSCOTT-X-30" "UnrealEngine3-Chair" "..."
					string[] Folders = Parms[1].Split( "/".ToCharArray() );
					if( Folders.Length < 5 )
					{
						continue;
					}

					// "UnrealEngine3-Chair"
					string BranchName = Folders[3].ToLower();
					if( !BranchName.StartsWith( Parent.LabelRoot.ToLower() ) )
					{
						continue;
					}

					if( !ClientBranches.Contains( Folders[3].ToLower() ) )
					{
						ClientBranches.Add( Folders[3].ToLower() );
					}
				}
			}

			return ( ClientBranches );
		}

		/** 
		 * Sync up the build scripts to #head while leaving the rest of the depot untouched
		 */
		public bool SyncBuildFiles( Main.PerforceServerInfo ServerInfo, string SourceControlBranch, string FileSpec )
		{
			// Set the RootFolder property based on the clientspec
			if( GetClientRoot( ServerInfo ) )
			{
				string ClientFileSpec = "//" + Parent.MachineName + "/" + SourceControlBranch + FileSpec;
				string DepotFileSpec = ConvertClientToDepotFileSpec( ServerInfo, ClientFileSpec );

				RunSimpleCommand( ServerInfo, P4ExceptionLevels.NoExceptionOnWarnings, COMMANDS.SCC_Sync, "sync", DepotFileSpec );
				return ( true );
			}

			return ( false );
		}

		public void SyncBuildFiles( ScriptParser Builder, string FileSpec )
		{
			if( Builder != null )
			{
				SyncBuildFiles( Builder.ServerInfo, Builder.BranchDef.Branch, FileSpec );
			}
		}

		/**
		 * Get the latest changelist for the files in this depot path
		 */
        public int GetLatestChangelist( ScriptParser Builder, string DepotPath )
        {
            int ChangelistValue = -1;

			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetLatestChangelist, "changes", "-s", "submitted", "-m1", DepotPath );
			if( Records != null && Records.Records.Length > 0 )
			{
				string ChangeListString = Records.Records[0].Fields["change"];
				ChangelistValue = Builder.SafeStringToInt( ChangeListString );
			}

			return( ChangelistValue );
        }

		/**
		 * Check the consistency of the client files versus the server and force a resync if necessary
		 */
        public void CheckConsistency( ScriptParser Builder, string FileSpec )
        {
			string ClientFileSpec = "//" + Parent.MachineName + "/" + Builder.BranchDef.Branch;
			if( FileSpec.Length > 0 )
			{
				ClientFileSpec += "/" + FileSpec;
			}

			string DepotFileSpec = ConvertClientToDepotFileSpec( Builder.ServerInfo, ClientFileSpec );
			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_CheckConsistency, "diff", "-se", DepotFileSpec );

			if( Records != null && Records.Records.Length > 0 )
			{
				string WarningMessage = "Warning: " + Records.Records.Length + " files are inconsistent with their server versions; forcing a resync." + Environment.NewLine;
				Builder.Write( WarningMessage );
				foreach( P4Record Record in Records )
				{
					Builder.Write( " ... forcing resync: " + Record["depotFile"] );
					RunSimpleCommand( Builder, P4ExceptionLevels.NoExceptionOnWarnings, COMMANDS.SCC_Sync, "sync", "-f", Record["depotFile"] );
					WarningMessage += Environment.NewLine + Record["depotFile"];
				}
                if (Builder.ErrorMode != ScriptParser.ErrorModeType.IgnoreErrors)
                {
					Parent.SendWarningMail( "Perforce Inconsistency", WarningMessage, false );
                }
			}
        }

		/**
		 * Sync a file spec to a dependency (which can be #head)
		 */
        public void SyncToRevision( ScriptParser Builder, string Revision, string FileSpec )
        {
			string ClientFileSpec = "//" + Parent.MachineName + "/" + Builder.BranchDef.Branch + "/" + FileSpec;
			string DepotFileSpec = ConvertClientToDepotFileSpec( Builder.ServerInfo, ClientFileSpec );

			RunSimpleCommand( Builder, P4ExceptionLevels.NoExceptionOnWarnings, COMMANDS.SCC_Sync, "sync", DepotFileSpec + Revision );
        }

		/**
		 * Sync a file spec to #head
		 */
		public void SyncToHead( ScriptParser Builder, string FileSpec )
		{
			RunSimpleCommand( Builder, P4ExceptionLevels.NoExceptionOnWarnings, COMMANDS.SCC_Sync, "sync", FileSpec + "#head" );
		}

		/**
		 * Sync a single changelist leaving all files not in that changelist untouched
		 */
        public void SyncSingleChangeList( ScriptParser Builder )
        {
			string ChangeList = Builder.CommandLine.Trim();
			string ClientFileSpec = "//" + Parent.MachineName + "/" + Builder.BranchDef.Branch + "/...";
			string DepotFileSpec = ConvertClientToDepotFileSpec( Builder.ServerInfo, ClientFileSpec );

			RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Sync, "sync", DepotFileSpec + "@" + ChangeList + ",@" + ChangeList );
        }

		/** 
		 * Gets the full name and email address of the Perforce user
		 */
		public bool GetUserInformation( ScriptParser Builder, string UserName, out string FullName, out string EmailAddress )
		{
			FullName = "";
			EmailAddress = "";

			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetUserInfo, "user", "-o", UserName );
			if( Records != null && Records.Records.Length > 0 )
			{
				FullName = Records.Records[0].Fields["FullName"];
				EmailAddress = Records.Records[0].Fields["Email"];
			}

			return( FullName != null && EmailAddress != null );
		}

		/**
		 * Open a files spec for edit
		 */
		public bool CheckoutFileSpec( ScriptParser Builder, string FileSpec )
		{
			// Create a pending changelist if none exists
			CreatePendingChangelist( Builder );

			// Make sure the files about to be checked out exist on disk
			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Checkout, "diff", "-sd", FileSpec );
			if( Records != null && Records.Records.Length > 0 )
			{
				string WarningMessage = "Warning: " + Records.Records.Length + " unopened files are missing on client; forcing a resync." + Environment.NewLine;
				Builder.Write( WarningMessage );
				foreach( P4Record Record in Records )
				{
					Builder.Write( " ... forcing resync: " + Record["depotFile"] );
					RunSimpleCommand( Builder, P4ExceptionLevels.NoExceptionOnWarnings, COMMANDS.SCC_Sync, "sync", "-f", Record["depotFile"] );
					WarningMessage += Environment.NewLine + Record["depotFile"];
				}

				Parent.SendWarningMail( "Perforce Inconsistency", WarningMessage, false );
			}

			Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Checkout, "edit", "-c", Pending.Number.ToString(), FileSpec );
			return ( Records != null && Records.Records.Length > 0 );
		}

		/** 
		 * Open a file spec for delete
		 */
		public bool OpenForDeleteFileSpec( ScriptParser Builder, string FileSpec )
		{
			// Create a pending changelist if none exists
			CreatePendingChangelist( Builder );

			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.NoExceptionOnWarnings, COMMANDS.SCC_OpenForDelete, "delete", "-c", Pending.Number.ToString(), FileSpec );
			return ( Records != null && Records.Records.Length > 0 );
		}

		/** 
		 * Delete any empty changelists
		 */
		public void DeleteEmptyChangelists( ScriptParser Builder )
		{
#if !DEBUG
			// Get all pending changelists for this client
			bSilenced = true;
			P4RecordSet Changelists = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetChangelists, "changes", "-c", Parent.MachineName, "-s", "pending" );
			if( Changelists != null )
			{
				foreach( P4Record Changelist in Changelists )
				{
					// Get the number of files open in this changelist
					P4RecordSet OpenedFiles = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetChangelists, "opened", "-c", Changelist.Fields["change"] );
					if( OpenedFiles != null && OpenedFiles.Records.Length == 0 )
					{
						// No files, so delete changelist
						RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetChangelists, "change", "-d", Changelist.Fields["change"] );
					}
				}
			}

			bSilenced = false;
#endif
		}

		/** 
		 * Revert any and all files opened on this client
		 */
		public void Revert( ScriptParser Builder, string FileSpec )
		{
			bSilenced = ( FileSpec == "..." );
			int RetryCount = 0;

			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_OpenedFiles, "opened", FileSpec );
			while( Records != null && Records.Records.Length > 0 && RetryCount < MaxRetryCount )
			{
#if !DEBUG
				RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Revert, "revert", FileSpec );
#endif
				// See if there are any files still open
				Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_OpenedFiles, "opened", FileSpec );

				// Retry if there are opened files left
				RetryCount++;
			}

			if( FileSpec == "..." )
			{
				// Clear out the pending changelist
				Pending = null;
			}

			bSilenced = false;
		}

		/** 
		 * Revert any unchanged files
		 */
		public void RevertUnchanged( ScriptParser Builder )
		{
			RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Revert, "revert", "-a" );
		}

		/**
		 * Get the version of the file we currently have on the local client
		 */
		public int GetHaveRevision( ScriptParser Builder, string DepotPath )
		{
			int HaveRev = 0;
			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetHaveRevision, "fstat", DepotPath );
			if( Records != null && Records.Records.Length > 0 )
			{
				string HaveRevString = Records.Records[0].Fields["haveRev"];
				HaveRev = Builder.SafeStringToInt( HaveRevString );
			}

			return( HaveRev );
		}

		/** 
		 * Extracts the pertinent info from a label
		 */
		public bool GetLabelInfo( ScriptParser Builder, string LabelName, LabelInfo Label )
		{
			bool bLabelExists = false;
			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetLabelInfo, "label", "-o", LabelName );

			if( Records != null && Records.Records.Length > 0 )
			{
				string Description = Records.Records[0].Fields["Description"];
				if( Description.Length > "[BUILDER]".Length && Description.StartsWith( "[BUILDER]" ) )
				{
					bLabelExists = true;
					if( Label != null )
					{
						Label.HandleDescription( Description );
					}
				}
			}

			return( bLabelExists );
		}

		/** 
		 * Delete an existing label
		 */
		public void DeleteLabel( ScriptParser Builder, string LabelName )
		{
			RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_LabelDelete, "label", "-d", LabelName );
		}

		/** 
		 * Creates a new label for the current branch
		 */
		public void CreateNewLabel( ScriptParser Builder )
		{
            try
            {
                ErrorLevel = COMMANDS.None;

				// Get the depot filespec of the current branch
				string[] Files = new string[1];
				string ClientFileSpec = "//" + Parent.MachineName + "/" + Builder.BranchDef.Branch + "/...";
				Files[0] = ConvertClientToDepotFileSpec( Builder.ServerInfo, ClientFileSpec );

				// Connect to the server and grab a form
                IP4Net.Port = Builder.ServerInfo.Server;
				IP4Net.User = Builder.ServerInfo.User;
				IP4Net.Password = Builder.ServerInfo.Password;
				IP4Net.Connect();

                IP4Net.Client = Parent.MachineName;
				IP4Net.CWD = Environment.CurrentDirectory;
				IP4Net.ExceptionLevel = P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

				P4Form LabelForm = IP4Net.Fetch_Form( "label", Builder.LabelInfo.GetLabelName() );

				LabelForm.Fields["Owner"] = IP4Net.User;

                Builder.LabelInfo.CreateLabelDescription();

				LabelForm.Fields["Description"] = Builder.LabelInfo.Description;
				LabelForm.Fields["Options"] = "unlocked";

				LabelForm.ArrayFields["View"] = Files;

				IP4Net.Save_Form( LabelForm );
				IP4Net.Disconnect();

				// Sync up the label to the file contents
				RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_LabelCreateNew, "labelsync", "-l", Builder.LabelInfo.GetLabelName() );

                Builder.NewLabelCreated = true;
            }
            catch( Exception Ex )
            {
                IP4Net.Disconnect();
				Builder.Write( "P4ERROR: CreateNewLabel exception: " + Ex.Message );
				ErrorLevel = COMMANDS.SCC_LabelCreateNew;
            }
		}

		/** 
		 * Updates the description field of a label
		 */
		public void UpdateLabelDescription( ScriptParser Builder )
		{
			if( !Builder.NewLabelCreated )
			{
				Builder.Write( "P4ERROR: UpdateLabelDescription - no label has been created that can be updated" );
				return;
			}

			try
            {
                ErrorLevel = COMMANDS.None;

                IP4Net.Port = Builder.ServerInfo.Server;
				IP4Net.User = Builder.ServerInfo.User;
				IP4Net.Password = Builder.ServerInfo.Password;
				IP4Net.Connect();

                IP4Net.Client = Parent.MachineName;
				IP4Net.CWD = Environment.CurrentDirectory;
				IP4Net.ExceptionLevel = P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

				P4Form LabelForm = IP4Net.Fetch_Form( "label", Builder.LabelInfo.GetLabelName() );

                Builder.LabelInfo.CreateLabelDescription();
				LabelForm.Fields["Description"] = Builder.LabelInfo.Description;

				IP4Net.Save_Form( LabelForm );
                IP4Net.Disconnect();
            }
            catch( Exception Ex )
            {
                IP4Net.Disconnect();

                // Otherwise, set the error
                Builder.Write( "P4ERROR: UpdateLabelDescription " + Ex.Message );
                ErrorLevel = COMMANDS.SCC_LabelUpdateDescription;
            }
		}

		/** 
		 * Tags a revision to a label
		 */
		public void Tag( ScriptParser Builder )
		{
			// Get label depending on dependencies
			string LabelName = LabelToTag( Builder );
			if( LabelName.Length > 0 )
			{
				DeleteLabel( Builder, Builder.CommandLine );
				if( ErrorLevel == COMMANDS.None )
				{
					RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Tag, "tag", "-l", Builder.CommandLine, "@" + LabelName );
				}
			}
		}

		/** 
		 * Adds a message to the current label description (for collating error reports)
		 */
		public void TagMessage( ScriptParser Builder, string Message )
		{
            string LabelName = Builder.LabelInfo.GetLabelName();
            if( !GetLabelInfo( Builder, LabelName, null ) )
            {
                // In case we're compiling with a define that hasn't created a label yet
                Parent.Log( "P4ERROR: TagMessage - Full label not found, trying root label", Color.DarkGreen );

                LabelName = Builder.LabelInfo.GetRootLabelName();
                if( !GetLabelInfo( Builder, LabelName, null ) )
                {
                    ErrorLevel = COMMANDS.SCC_TagMessage;
                    Parent.Log( "P4ERROR: TagMessage - Non existent label", Color.DarkGreen );
                    return;
                }
            }

			try
			{
				IP4Net.Port = Builder.ServerInfo.Server;
				IP4Net.User = Builder.ServerInfo.User;
				IP4Net.Password = Builder.ServerInfo.Password;
				IP4Net.Connect();

				IP4Net.Client = Parent.MachineName;
				IP4Net.CWD = Environment.CurrentDirectory;
				IP4Net.ExceptionLevel = P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

				P4Form LabelForm = IP4Net.Fetch_Form( "label", LabelName );

				string NewDescription = LabelForm.Fields["Description"] + Message;
				LabelForm.Fields["Description"] = NewDescription;

				IP4Net.Save_Form( LabelForm );
				IP4Net.Disconnect();
			}
			catch( Exception Ex )
			{
				IP4Net.Disconnect();

				// Otherwise, set the error
				Builder.Write( "P4ERROR: TagMessage " + Ex.Message );
				ErrorLevel = COMMANDS.SCC_TagMessage;
			}
		}

		/** 
		 * Get the changelists between the starting and ending revisions
		 */
		public void GetChangesInRange( ScriptParser Builder, string DepotPath, string StartingRevision, string EndingRevision )
		{
			// Clear out any preexisting changes already processed
			Builder.ChangeLists.Clear();
			Builder.ChangelistsCollated = false;

			P4RecordSet Records = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetChanges, "changes", DepotPath + "@" + StartingRevision + "," + EndingRevision );
			if( Records != null )
			{
				foreach( P4Record Record in Records )
				{
					string Changelist = Record.Fields["change"];

					P4RecordSet Description = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetChanges, "describe", Changelist );
					if( Description != null && Description.Records.Length > 0 )
					{
						Builder.ProcessChangeList( Description.Records[0] );
					}
				}
			}
		}

		/** 
		 * Get a list of files that require a resolve, or are locked
		 */
        public void AutoResolveFiles( ScriptParser Builder )
        {
			bool bFilesLocked = false;

			P4RecordSet OpenedFiles = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetIncorrectCheckedOutFiles, "opened", "-c", Pending.Number.ToString() );
			if( OpenedFiles != null )
			{
				foreach( P4Record OpenedRecord in OpenedFiles )
				{
					string DepotPath = OpenedRecord.Fields["depotFile"];
					P4RecordSet FileRecords = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_GetIncorrectCheckedOutFiles, "fstat", DepotPath );
					if( FileRecords != null && FileRecords.Records.Length > 0 )
					{
						P4Record FileRecord = FileRecords.Records[0];

						if( FileRecord.ArrayFields.ContainsKey( "otherLock" ) )
						{
							bFilesLocked = true;

							Builder.Write( "P4ERROR: File '" + DepotPath + "' is *LOCKED* by:" );
							foreach( string OtherLock in FileRecord.ArrayFields["otherLock"] )
							{
								Builder.Write( "\t" + OtherLock );
							}
						}
						else
						{
							// Auto resolve any files that require it
							int HaveRev = Builder.SafeStringToInt( FileRecord["haveRev"] );
							int HeadRev = Builder.SafeStringToInt( FileRecord["headRev"] );

							if( HaveRev != 0 && HeadRev != 0 )
							{
								if( HaveRev != HeadRev )
								{
									RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Resolve, "resolve", "-ay", DepotPath );
								}
							}
						}
					}
				}
            }

			// If there are any locked files, fail and report it
			if( bFilesLocked )
			{
				ErrorLevel = COMMANDS.SCC_FileLocked;
			}
        }

		/** 
		 * Submit all open files in the pending changelist
		 */
        public void Submit( ScriptParser Builder, bool bReportErrors )
        {
            try
            {
                ErrorLevel = COMMANDS.None;

				// Revert any unchanged files
				RevertUnchanged( Builder );

				if( Pending != null )
				{
					// Get a list of opened files in the changelist
					P4RecordSet OpenedFiles = RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Submit, "opened", "-c", Pending.Number.ToString() );
					if( OpenedFiles != null && OpenedFiles.Records.Length > 0 )
					{
						// Submit the changelist
						IP4Net.Port = Builder.ServerInfo.Server;
						IP4Net.User = Builder.ServerInfo.User;
						IP4Net.Password = Builder.ServerInfo.Password;
						IP4Net.Connect();

						IP4Net.Client = Parent.MachineName;
						IP4Net.CWD = Environment.CurrentDirectory;
						IP4Net.ExceptionLevel = P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings;

						// Submit the changelist
						Pending.Submit();
						IP4Net.Disconnect();

						Pending = null;

						// Work out the number of bytes submitted and post to the performance table
						long BytesSubmitted = 0;
						foreach( P4Record Record in OpenedFiles.Records )
						{
							string ClientFileSpec = Record.Fields["clientFile"];
							string LocalFileSpec = ConvertClientToLocalFileSpec( Builder.ServerInfo, ClientFileSpec );
							FileInfo Info = new FileInfo( LocalFileSpec );
							if( Info.Exists )
							{
								BytesSubmitted += Info.Length;
							}
						}
						Parent.DB.WritePerformanceData( Parent.MachineName, "PerforceBytesSubmitted", BytesSubmitted );
#if !DEBUG
						if( Builder.IsTagging )
						{
							// Tag the submitted files to the current label
							foreach( P4Record Record in OpenedFiles.Records )
							{
								string File = Record.Fields["depotFile"];
								RunSimpleCommand( Builder, P4ExceptionLevels.ExceptionOnBothErrorsAndWarnings, COMMANDS.SCC_Submit, "labelsync", "-l", Builder.LabelInfo.GetLabelName(), File );
							}
						}
#endif
					}
				}
            }
            catch( Exception Ex )
            {
                // Otherwise, set the error
				IP4Net.Disconnect();
				ErrorLevel = COMMANDS.SCC_Submit;
				if( bReportErrors )
				{
					Builder.Write( "P4ERROR: Submit " + Ex.Message );
				}
            }
        }

		/** 
		 * Gets all the changelists since the last good build of this type
		 */
        public void GetChangesSinceLastBuild( ScriptParser Builder )
        {
			// Ending changelist depending on dependencies
			// [SIDE EFFECT] Must call this before assigning the variable in the next statement
			// otherwise the the real changelist of the label (if we have one) will be gone.
			string EndingRevision = Parent.GetChangeListToSync();

			// ... in case there are no changelists to retrieve
			// [SIDE EFFECT] This is safe because the ProcessChangeList call that we're about
			// to do per changelist will update this Changelist value if it's greater.
			Builder.LabelInfo.Changelist = Builder.LastGoodBuild;

			// Start with one past the last good to avoid getting the description again
			int StartingRevisionNum = Builder.LastGoodBuild + 1;
			string StartingRevision = StartingRevisionNum.ToString();

			GetChangesInRange( Builder, "...", StartingRevision, EndingRevision );
        }
    }
}
