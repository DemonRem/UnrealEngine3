/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Globalization;
using P4Core;
using P4Inquiry;
using P4ChangeParser;

namespace P4ChangeParserApp
{
	/// <summary>
	/// Simple command-line based application to query and export Perforce changelists using
	/// the new description tagging system
	/// </summary>
	class P4ChangeParserApp
	{
		#region Static Constants
		/// <summary>
		/// Array of the accepted date formats for query parameters
		/// </summary>
		private static readonly String[] AcceptedDateFormats = 
		{
			"yyyy/MM/dd:HH:mm:ss",
			"yyyy/MM/dd:H:m:s",
			"yyyy/MM/dd",
			"yyyy/M/d:HH:mm:ss",
			"yyyy/M/d:H:m:s",
			"yyyy/M/d",
			"MM/dd/yyyy:HH:mm:ss",
			"MM/dd/yyyy:H:m:s",
			"MM/dd/yyyy",
			"M/d/yyyy:HH:mm:ss",
			"M/d/yyyy:H:m:s",
			"M/d/yyyy"
		};

		/// <summary>
		/// Perforce server to connect to; Currently hard-coded to reduce number of command line arguments
		/// </summary>
		private static readonly String P4Server = "p4-server.epicgames.net:1666";

		/// <summary>
		/// String displayed when a malformed command is entered
		/// </summary>
		private static readonly String MalformedCommand = "Error: Malformed Command.";

		/// <summary>
		/// String demonstrating valid query operands
		/// </summary>
		private static readonly String QueryUsage = "Query Operand: <ChangelistNumber> | <LabelName> | <Date>";
		
		/// <summary>
		/// String demonstrating valid command usage
		/// </summary>
		private static readonly String CommandUsage = "Command Usage: [-p //depotPath/...] <OutputFile.ValidExtension> <QueryOperand> <QueryOperand>\n" + QueryUsage;
		
		/// <summary>
		/// String to prompt for a new command
		/// </summary>
		private static readonly String PromptForCommand = "Enter a new command:\n" + CommandUsage;

#if COMMAND_LINE_ONLY
		/// <summary>
		/// String to show command line usage
		/// </summary>
		private static readonly String CommandlineUsage = "Usage: P4ChangeParserApp <p4user> <p4password> [-p //depotPath/...] <OutputFile.ValidExtension> <QueryOperand> <QueryOperand>\n" + QueryUsage;
#else
		/// <summary>
		/// String to show command line usage
		/// </summary>
		private static readonly String CommandlineUsage = "Usage: P4ChangeParserApp <p4user> <p4password> [command]";
#endif // #if COMMAND_LINE_ONLY

		/// <summary>
		/// String to show for a Perforce connection error
		/// </summary>
		private static readonly String ConnectionError = "Unable to connect to Perforce. Please check your settings.";
		
		/// <summary>
		/// String to show when an invalid query operand is entered
		/// </summary>
		private static readonly String InvalidQueryOperand = "Invalid query operands.";

		/// <summary>
		/// String to check for to allow the user to exit
		/// </summary>
		private static readonly String ExitString = "exit";

		/// <summary>
		/// String to check for to allow the user to specify the current time
		/// </summary>
		private static readonly String NowString = "now";

		/// <summary>
		/// String array representing delimiters between commands
		/// </summary>
		private static readonly String[] CommandDelimiters = { " " };
		#endregion

		#region Static Member Variables
		/// <summary>
		/// Path to filter the depot by
		/// </summary>
		private static String FilteredDepotPath = "//depot/...";

		/// <summary>
		/// User to filter the depot by
		/// </summary>
		private static String FilteredUser = "";

		/// <summary>
		/// Mapping of file extensions to export formats
		/// </summary>
		private static Dictionary<String, P4ParsedChangelist.EExportFormats> ExtensionToExportFormatMap;

		/// <summary>
		/// File to output results to
		/// </summary>
		private static String OutputFile = String.Empty;

		/// <summary>
		/// Export format to use, based on output file extension
		/// </summary>
		private static P4ParsedChangelist.EExportFormats ExportFormat;

		/// <summary>
		/// String representing a query operand
		/// </summary>
		private static String QueryStartString = String.Empty;

		/// <summary>
		/// String representing a query operand
		/// </summary>
		private static String QueryEndString = String.Empty;

		/// <summary>
		/// Time representing a query operand
		/// </summary>
		private static DateTime QueryStartTime;

		/// <summary>
		/// Time representing a query operand
		/// </summary>
		private static DateTime QueryEndTime;

		/// <summary>
		/// Boolean to track if a valid command is pending execution or not
		/// </summary>
		private static bool bValidCommandPending = false;

		/// <summary>
		/// Instance of P4Inquirer to query the server for changelists, labels, etc.
		/// </summary>
		private static P4Inquirer PerforceInquirer = new P4Inquirer();

		/// <summary>
		/// List of the changelists resulting from a P4Inquirer query
		/// </summary>
		private static List<P4Changelist> QueriedChangelists = new List<P4Changelist>();

		/// <summary>
		/// List of parsed changelists resulting from the parsing of a P4Inquirer query
		/// </summary>
		private static List<P4ParsedChangelist> ParsedChangelists = new List<P4ParsedChangelist>();
		#endregion

		#region Main
		static void Main(String[] Args)
		{
			// Set up the extension to export type map
			ExtensionToExportFormatMap = new Dictionary<String, P4ParsedChangelist.EExportFormats>();
			ExtensionToExportFormatMap.Add("htm", P4ParsedChangelist.EExportFormats.EF_Html);
			ExtensionToExportFormatMap.Add("html", P4ParsedChangelist.EExportFormats.EF_Html);
			ExtensionToExportFormatMap.Add("xml", P4ParsedChangelist.EExportFormats.EF_Xml);
			ExtensionToExportFormatMap.Add("txt", P4ParsedChangelist.EExportFormats.EF_PlainText );

			// Parse the command line arguments for relevant data
			ParseCommandlineArgs(Args);

			// Set the perforce server and attempt to connect to it
			PerforceInquirer.SetP4Server(P4Server);
			P4Inquirer.EP4ConnectionStatus ConnStatus = PerforceInquirer.Connect();

			// Ensure the connection occurred without a hitch
			if (ConnStatus != P4Inquirer.EP4ConnectionStatus.P4CS_Connected)
			{
				DisplayError(ConnectionError, true);
			}

			// Attempt to execute a command if one came in on the command line
			ExecuteCommand();

#if COMMAND_LINE_ONLY
			// If executing in command line only mode, exit after successfully running the command
			Exit(1);
#else
			// If executing in console mode, poll for commands repeatedly until the user quits
			for (;;)
			{
				Console.WriteLine(PromptForCommand);
				String CurInput = Console.ReadLine();
				Console.WriteLine("\n");
				ParseCommand(CurInput.Split(CommandDelimiters, StringSplitOptions.RemoveEmptyEntries));
				ExecuteCommand();
			}
#endif // #if COMMAND_LINE_ONLY
		}
		#endregion

		#region Helper Methods
		/// <summary>
		/// Exit the application with the specified error code
		/// </summary>
		/// <param name="ErrorCode">Error code to exit with</param>
		private static void Exit(int ErrorCode)
		{
			// Disconnect from the perforce connection when exiting
			PerforceInquirer.Disconnect();
			Environment.Exit(ErrorCode);
		}

		/// <summary>
		/// Displays the provided string to the console as an error
		/// </summary>
		/// <param name="InError">Error message to display</param>
		/// <param name="bForceExit">If TRUE, forcibly exit after writing out the message</param>
		private static void DisplayError(String InError, bool bForceExit)
		{
			Console.WriteLine("{0}\n", InError);
			if (bForceExit)
			{
				Exit(-1);
			}
		}

		/// <summary>
		/// Helper function to parse the command line arguments
		/// </summary>
		/// <param name="Args"></param>
		private static void ParseCommandlineArgs(String[] Args)
		{
#if COMMAND_LINE_ONLY
			if (Args.Length < 5)
#else
			if (Args.Length < 2)
#endif // #if COMMAND_LINE_ONLY
			{
				// Display an error to the screen and exit if the required arguments weren't found
				DisplayError(CommandlineUsage, true);
			}

			// Initialize the Perforce user/password based on the command line arguments
			int ArgsIndex = 0;
			PerforceInquirer.SetP4User(Args[ArgsIndex++]);
			PerforceInquirer.SetP4Password(Args[ArgsIndex++]);

			// If a command was specified on the command line, parse it and prep it for execution
			if (Args.Length > 2)
			{
				String[] CommandStrings = new String[Args.Length-ArgsIndex];
				Array.Copy(Args, ArgsIndex, CommandStrings, 0, Args.Length - ArgsIndex);
				ParseCommand(CommandStrings);
			}

		}

		/// <summary>
		/// Parse an array of strings into a command to query the Perforce server
		/// </summary>
		/// <param name="Commands"></param>
		private static void ParseCommand(String[] Commands)
		{
			// Assume the command will be valid
			bool bValidCommand = true;
			int CommandIndex = 0;

			// If the user explicitly asked to exit, exit the application immediately
			if ( Commands.Length == 1 && String.Compare(Commands[CommandIndex], ExitString) == 0)
			{
				Exit(1);
			}

			// Loop to catch the existence of any optional arguments/switches/flags that start with a hyphen
			for (; CommandIndex < Commands.Length && Commands[CommandIndex].StartsWith("-"); ++CommandIndex)
			{
				// If the "-p" flag is specified, the user is attempting to filter the depot by path
				if (String.Compare(Commands[CommandIndex], "-p") == 0)
				{
					// Ensure there is at least one more argument (the filter), and that it meets the formatting requirements
					// necessary to be used in a Perforce query
					if (CommandIndex + 1 < Commands.Length)
					{
						++CommandIndex;
						if (!Commands[CommandIndex].StartsWith("//depot/") || !Commands[CommandIndex].EndsWith("..."))
						{
							bValidCommand = false;
						}
						else
						{
							FilteredDepotPath = Commands[CommandIndex];
						}
					}
					else
					{
						bValidCommand = false;
					}
				}

				// If the "-u" flag is specified, the user is attempting to filter the depot by user
				if( String.Compare( Commands[ CommandIndex ], "-u" ) == 0 )
				{
					// Ensure there is at least one more argument (the filter), and that it meets the formatting requirements
					// necessary to be used in a Perforce query
					if( CommandIndex + 1 < Commands.Length )
					{
						++CommandIndex;
						FilteredUser = Commands[ CommandIndex ];
					}
					else
					{
						bValidCommand = false;
					}
				}
			}

			// After the optional arguments are accounted for, there are three more required arguments. Ensure they exist, or else
			// the command is invalid.
			if (CommandIndex + 2 >= Commands.Length)
			{
				bValidCommand = false;
			}
			else
			{
				// Extract the output file from the arguments
				OutputFile = Commands[CommandIndex++];

				// Make sure the output file has a required extension that corresponds to a valid export format
				int ExtensionIndex = OutputFile.LastIndexOf(".") + 1;
				if (ExtensionIndex <= 0 || ExtensionIndex >= OutputFile.Length || !ExtensionToExportFormatMap.TryGetValue(OutputFile.Substring(ExtensionIndex), out ExportFormat))
				{
					bValidCommand = false;
				}
				
				// Extract the query strings from the arguments
				QueryStartString = Commands[CommandIndex++];
				QueryEndString = Commands[CommandIndex++];
			}

			// If there are more commands left we haven't parsed, then extra data has been included and this is an invalid command
			if (CommandIndex != Commands.Length)
			{
				bValidCommand = false;
			}

			// If the command is valid, update the related boolean so ExecuteCommand knows all the relevant data is ok to use
			if (bValidCommand)
			{
				bValidCommandPending = true;
			}
			else
			{
				DisplayError(MalformedCommand, false);
			}
		}

		/// <summary>
		/// Attempt to execute a command based on cached data
		/// </summary>
		private static void ExecuteCommand()
		{
			// Ensure the command data is valid before proceeding
			if (bValidCommandPending)
			{
				// Determine if both of the times specified by the command line are valid
				if (DetermineQueryTime(QueryStartString, out QueryStartTime) && DetermineQueryTime(QueryEndString, out QueryEndTime))
				{
					ParsedChangelists = new List<P4ParsedChangelist>();

					// Query the changelists specified by the time frame from the query operands
					PerforceInquirer.QueryChangelists(new P4TimespanQuery(QueryStartTime, QueryEndTime, FilteredDepotPath, FilteredUser), out QueriedChangelists);
					
					// From each valid queried changelist, construct a parsed changelist (if the changelist supports the new tagging system)
					foreach (P4Changelist CurrentCL in QueriedChangelists)
					{
						if (P4ParsedChangelist.ChangelistSupportsTagging(CurrentCL))
						{
							ParsedChangelists.Add(new P4ParsedChangelist(CurrentCL));
						}
					}
					Console.WriteLine("Query returned:\n{0} Changelist(s)\n{1} Changelist(s) supporting tagging\n", QueriedChangelists.Count, ParsedChangelists.Count);

					// If at least one changelist was parsed, export it in Xml format to the specified output file
					if (ParsedChangelists.Count > 0)
					{
						P4ParsedChangelist.Export(ExportFormat, ParsedChangelists, OutputFile);
						Console.WriteLine("Data exported to {0}!\n", OutputFile);
					}
				}
				// Warn the user of an invalid query operand
				else
				{
					DisplayError(InvalidQueryOperand, false);
				}
			}

			// Clear both changelists lists
			QueriedChangelists.Clear();
			ParsedChangelists.Clear();
			bValidCommandPending = false;
		}

		/// <summary>
		/// Attempt to determine a timestamp from the provided string and convert it to a DateTime object
		/// </summary>
		/// <param name="InQueryString">String to check for a timestamp</param>
		/// <param name="OutQueryTime">DateTime object to populate with the timestamp from the provided string</param>
		/// <returns></returns>
		private static bool DetermineQueryTime(String InQueryString, out DateTime OutQueryTime)
		{
			// Assume the query time is valid
			bool bValidQueryTime = true;
			DateTime PotentialQueryTime;
			
			// Attempt to parse the string into a changelist number 
			int ChangelistNumber = 0;
			if (int.TryParse(InQueryString, out ChangelistNumber))
			{
				// If the string was successfully parsed into a CL number, attempt to query for that CL to extract a timestamp from
				P4Changelist RequestedCL = PerforceInquirer.QueryChangelist(new P4StringQuery(InQueryString));
				OutQueryTime = RequestedCL.Date;
				
				// If the changelist was invalid, return that a query time could not be determined
				if (RequestedCL == P4Changelist.InvalidChangelist)
				{
					bValidQueryTime = false;
				}
			}
			// Try to parse the string into a valid date format
			else if (DateTime.TryParseExact(InQueryString, AcceptedDateFormats, new CultureInfo("en-US"), DateTimeStyles.AssumeLocal, out PotentialQueryTime))
			{
				OutQueryTime = PotentialQueryTime;
			}
			// Check if the user typed the string specifying the current time, and if so, use the current time as the query time
			else if (String.Compare(InQueryString.ToLower(), NowString) == 0)
			{
				OutQueryTime = DateTime.UtcNow;
			}
			// If no other check has successfully parsed the string, assume it's a label name
			else
			{
				// Query for the requested label name
				P4Label RequestedLabel = PerforceInquirer.QueryLabel(new P4StringQuery(InQueryString));
				OutQueryTime = RequestedLabel.LastUpdatedDate;

				// If the label was invalid, return that a query time could not be determined
				if (RequestedLabel == P4Label.InvalidLabel)
				{
					bValidQueryTime = false;
				}
			}
			return bValidQueryTime;
		}
		#endregion
	}
}
