using System;
using System.Diagnostics;
using System.Drawing;
using System.Collections;
using System.IO;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Serialization;
using System.Security.Cryptography;
using ConsoleInterface;

namespace CookerTools
{
	public interface IOutputHandler
	{
		void OutputText(string Text);
		void OutputText(string Text, Color OutputColor);
	};

	#region TOCInfo class
	public class TOCInfo
	{
		public string   SourceFilename;
		public string   DestinationFilename;
		public string   ConsoleFilename;
		public int      Size;
		public int      CompressedSize;
		public int      StartSector;
		public string   CRC;
		public DateTime LastWriteTime;
		public bool		bIsForTarget;
 
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="InConsoleFilename"></param>
		/// <param name="InSize"></param>
		/// <param name="InCompressedSize"></param>
		public TOCInfo(string InSourceFilename, string InDestinationFilename, string InConsoleFilename, int InSize, int InCompressedSize, int InStartSector, string InCRC, DateTime InLastWriteTime, bool bInIsForTarget)
		{
			SourceFilename = InSourceFilename;
			DestinationFilename = InDestinationFilename;
			ConsoleFilename = InConsoleFilename;
			Size = InSize;
			CompressedSize = InCompressedSize;
			StartSector = InStartSector;
			CRC = InCRC;
			LastWriteTime = InLastWriteTime;
			bIsForTarget = bInIsForTarget;
		}
	}
	#endregion

	#region CookerTools class
	/// <summary>
	/// Helper functions for cooking/syncing/etc
	/// </summary>
	public class CookerToolsClass
	{

		#region Variables

		/// <summary>
		/// String used for hex conversion (there is probably a function that can do this already)
		/// </summary>
		private const string HexDigits = "0123456789abcdef";

		/** Directory where the application started from (ie C:\UnrealEngine3\Binaries) */
		private string StartupPath;

		/** Output device to send log output to												*/
		private IOutputHandler OutputHandler;

		/// <summary>
		/// A list of all known consoles
		/// </summary>
		private ArrayList KnownTargetList;

		/// <summary>
		/// Holds the set of console names that we are going to perform actions on
		/// </summary>
		private ArrayList TargetNames;

		/// <summary>
		/// Holds the set of console names that were actually connected to
		/// </summary>
		private ArrayList ConnectedTargetNames = new ArrayList();

		/// <summary>
		/// Holds the set of console indices that we are going to perform actions on
		/// </summary>
		private ArrayList ConnectedTargets = new ArrayList();

		/// <summary>
		/// Holds the set of target paths that the user wants to perform copies to
		/// </summary>
		private ArrayList RequestedTargetPaths = new ArrayList();

		/// <summary>
		/// Holds the set of target paths that we are going to perform copies to
		/// </summary>
		private ArrayList TargetPaths = new ArrayList(); 

		/// <summary>
		/// Files that failed to copy
		/// </summary		
		private ArrayList FailedCopies = new ArrayList();

		/** 
		 * GameName that is cached at start of cooking operation so user can fiddle with
		 * the combo box while the cooking takes place without affecting the result.
		 */
		private string CachedGameName;

		/// <summary>
		/// Standard location to copy to on the target
		/// </summary>
		private const string DefaultBaseDirectory = "UnrealEngine3\\";

		/// <summary>
		/// Desired location to copy to on the target
		/// </summary>
		private string BaseDirectory;

		/// <summary>
		/// The hardcoded name to use for the table of contents
		/// </summary>
		private string TOCFilename;

		/// <summary>
		/// The string appended to file names that contains the original uncompressed size of fully compressed files
		/// </summary>
		private const string ManifestExtension = ".uncompressed_size";

		/// <summary>
		/// The CRC creation provider
		/// </summary>
		private MD5 Hasher = MD5.Create();

		/// <summary>
		/// Cached directory name
		/// </summary>
		private string OldDirectory = "";

		/// <summary>
		/// Cached directory name
		/// </summary>
		private ArrayList CRCMismatchedFiles;

		/// <summary>
		/// Indicates that every file should be copied regardless of the time stamp
		/// </summary>
		public bool Force = false;

		/// <summary>
		/// Indicates that the tools should skip the actual file copy during sync (i.e. preview the operation)
		/// </summary>
		public bool NoSync = false;

		/// <summary>
		/// Indicates that CRCs should be computed during TOC creation
		/// </summary>
		public bool ComputeCRC = false;

		/// <summary>
		/// Indicates that the tool should use the CRCs from the existing TOC file
		/// </summary>
		public bool MergeExistingCRC = false;

		/// <summary>
		/// Indicates that the tool should verify any PC side copy using the generated CRCs (can only be used if ComputeCRC is also enabled)
		/// </summary>
		public bool VerifyCopy = false;

		/// <summary>
		/// Indicates that the tool should output additional information
		/// </summary>
		public bool VerboseOutput = false;

		/// <summary>
		/// Settings for the game (Example, UT, etc)
		/// </summary>
		private GameSettings CachedGameSettings = null;

		/// <summary>
		/// Settings for the platform (PC, Xbox, etc)
		/// </summary>
		private PlatformSettings CachedPlatformSettings = null;

		/// <summary>
		/// Is there a valid, DLLInterface-set DVD layout file?
		/// </summary>
		private bool bHasDVDLayoutFile;

		/// <summary>
		/// Indicates that the tool should create the target PC directories if they don't exist
		/// </summary>

		/// <summary>
		/// Interface to platform specific C++ DLLs
		/// </summary>
		private DLLInterface DLLInterface = new DLLInterface();

		/// <summary>
		/// Remember the platform we are cooking for
		/// </summary>
		private string SavedPlatform;

		/// <summary>
		/// Are we dealing with the PC platform (which has no *Tools.dll)
		/// </summary>
		private bool bIsPC;

		/// <summary>
		/// Is there a valid platform activated?
		/// </summary>
		private bool bIsActivated;

		#endregion

		#region Initialization, etc
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="InOutputHandler">Interface to an object to output to</param>
		public CookerToolsClass(IOutputHandler InOutputHandler)
		{
			StartupPath = null;
			OutputHandler = InOutputHandler;
		}

		public bool Activate(string Platform)
		{
			// get a friendly, known version
			string FriendlyPlatformName = DLLInterface.GetFriendlyPlatformName(Platform);

			// default to not-activated
			bIsActivated = false;

			// handle special case for PC
			if (FriendlyPlatformName == "PC")
			{
				// force case
				bIsPC = true;
			}
			else
			{
				if (DLLInterface.ActivatePlatform(FriendlyPlatformName) == false)
				{
					return false;
				}

				// success!
				bIsActivated = true;

				// find all the known consoles according to DLL
				KnownTargetList = new ArrayList();

				// Grab the list of consoles
				try
				{
					// Initialize the consoles list
					int NumTargets = DLLInterface.GetNumTargets();
					for (int Index = 0; Index < NumTargets; Index++)
					{
						// get the name of the target
						string TargetName = DLLInterface.GetTargetName(Index);
						KnownTargetList.Add(TargetName);
					}
				}
				catch (Exception /* e */ )
				{
					KnownTargetList.Clear();
				}

				// @todo: Validate we have a default console?
			}

			// save off the activated platform name
			SavedPlatform = FriendlyPlatformName;

			// generate TOC filename
			TOCFilename = SavedPlatform + "TOC.txt";

			Stream XmlStream = null;
			try
			{
				string XMLPathName = StartupPath + "CookerFrontend_" + SavedPlatform + ".xml";

				// Get the XML data stream to read from
				XmlStream = new FileStream(XMLPathName, FileMode.Open, FileAccess.Read, FileShare.None, 256 * 1024, false);

				// Creates an instance of the XmlSerializer class so we can read the settings object
				XmlSerializer ObjSer = new XmlSerializer(typeof(PlatformSettings));
				// Add our callbacks for a busted XML file
				ObjSer.UnknownNode += new XmlNodeEventHandler(XmlSerializer_UnknownNode);
				ObjSer.UnknownAttribute += new XmlAttributeEventHandler(XmlSerializer_UnknownAttribute);

				// Create an object graph from the XML data
				CachedPlatformSettings = (PlatformSettings)ObjSer.Deserialize(XmlStream);
			}
			catch (Exception e)
			{
				Console.WriteLine(e.ToString());
			}
			finally
			{
				if (XmlStream != null)
				{
					// Done with the file so close it
					XmlStream.Close();
				}
			}
				
			return true;
		}

		public void SetGameInfo(string GameName)
		{
			CachedGameName = GameName;

			Stream XmlStream = null;
			try
			{
				string XMLPathName = StartupPath + "\\..\\" + CachedGameName + "Game\\Config\\CookerFrontend_game.xml";
				// Get the XML data stream to read from
				XmlStream = new FileStream(XMLPathName, FileMode.Open, FileAccess.Read, FileShare.None, 256 * 1024, false);

				// Creates an instance of the XmlSerializer class so we can read the settings object
				XmlSerializer ObjSer = new XmlSerializer(typeof(GameSettings));
				// Add our callbacks for a busted XML file
				ObjSer.UnknownNode += new XmlNodeEventHandler(XmlSerializer_UnknownNode);
				ObjSer.UnknownAttribute += new XmlAttributeEventHandler(XmlSerializer_UnknownAttribute);

				// Create an object graph from the XML data
				CachedGameSettings = (GameSettings)ObjSer.Deserialize(XmlStream);

				bHasDVDLayoutFile = false;
				if (CachedGameSettings.XGDFileRelativePath != null && CachedGameSettings.XGDFileRelativePath != "" &&
					File.Exists(CachedGameSettings.XGDFileRelativePath))
				{
					// tell the DLL the name of the dvd layout file, and
					// remember if it succeeded
					bHasDVDLayoutFile = DLLInterface.SetDVDLayoutFile(StartupPath + "\\" + CachedGameSettings.XGDFileRelativePath);
				}
			}
			catch (Exception e)
			{
				Console.WriteLine(e.ToString());
			}
			finally
			{
				if (XmlStream != null)
				{
					// Done with the file so close it
					XmlStream.Close();
				}
			}
		}

		public void SetConsoleInfo(ArrayList InConsoleNames)
		{
			TargetNames = InConsoleNames;
		}

		public void SetBaseDirectory(string InBaseDirectory)
		{
			// if specified, turn it into the format we want
			if (InBaseDirectory != null && InBaseDirectory != "")
			{
				BaseDirectory = InBaseDirectory;
				// add a trailing slash if needed (its expected later)
				if ( !BaseDirectory.EndsWith("\\") )
				{
					BaseDirectory += "\\";
				}
				// remove the first slash if there is one
				if ( BaseDirectory.StartsWith("\\") )
				{
					BaseDirectory = BaseDirectory.Substring(1, BaseDirectory.Length - 1);
				}
			}
			else
			{
				BaseDirectory = DefaultBaseDirectory;
			}

		}

		/// <summary>
		/// Logs the bad XML information for debugging purposes
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e">The attribute info</param>
		protected void XmlSerializer_UnknownAttribute(object sender, XmlAttributeEventArgs e)
		{
			System.Xml.XmlAttribute attr = e.Attr;
			Console.WriteLine("Unknown attribute " + attr.Name + "='" + attr.Value + "'");
		}

		/// <summary>
		/// Logs the node information for debugging purposes
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e">The info about the bad node type</param>
		protected void XmlSerializer_UnknownNode(object sender, XmlNodeEventArgs e)
		{
			Console.WriteLine("Unknown Node:" + e.Name + "\t" + e.Text);
		}

		public void SetPathInfo(ArrayList InRequestedTargetPaths)
		{
			RequestedTargetPaths = InRequestedTargetPaths;
		}

		/// <summary>
		/// Sets the gamename and list of consoles to act one
		/// </summary>
		/// <param name="InStartupPath">Current working directory</param>
		/// <param name="InGameName">Name of the game ("War")</param>
		/// <param name="InConsoleNames">List of console names we will act on later</param>
		/// <param name="InBaseDirectory">Optional location to sync files to on the target (null for default)</param>
		public void SetGameAndConsoleInfo(string InStartupPath, string InGameName, ArrayList InConsoleNames, string InBaseDirectory)
		{
			StartupPath = InStartupPath;
			SetGameInfo(InGameName);
			SetConsoleInfo(InConsoleNames);
			SetBaseDirectory(InBaseDirectory);
			SetPathInfo(new ArrayList());
		}

		/// <summary>
		/// Sets the gamename and list of consoles to act one
		/// </summary>
		/// <param name="InStartupPath">Current working directory</param>
		/// <param name="InGameName">Name of the game ("War")</param>
		/// <param name="InConsoleNames">List of console names we will act on later</param>
		/// <param name="InBaseDirectory">Optional location to sync files to on the target (null for default)</param>
		/// <param name="InRequestedTargetPaths">Target directorys to act on</param>
		public void SetGameAndConsoleInfo(string InStartupPath, string InGameName, ArrayList InConsoleNames, string InBaseDirectory, ArrayList InRequestedTargetPaths)
		{
			StartupPath = InStartupPath;
			SetGameInfo(InGameName);
			SetConsoleInfo(InConsoleNames);
			SetBaseDirectory(InBaseDirectory);
			SetPathInfo(InRequestedTargetPaths);
		}

		public bool VerifyGameInfo(string InGameName)
		{
			string SourceFolder = StartupPath + "\\..\\" + InGameName + "Game\\"; 
			DirectoryInfo SrcDirectory = new DirectoryInfo( SourceFolder );
			return SrcDirectory.Exists;
		}

		/// <summary>
		/// Return the cached game name
		/// </summary>
		/// <returns></returns>
		public string GetGameName()
		{
			return CachedGameName;
		}

		/// <summary>
		/// Return the list of all known consoles
		/// </summary>
		/// <returns></returns>
		public ArrayList GetKnownConsoles()
		{
			return KnownTargetList;
		}

		/// <summary>
		/// Get name of default console
		/// </summary>
		/// <returns></returns>
		public string GetDefaultConsole()
		{
			// @todo: Add a real interface to getting the default, instead of just the first one
			return KnownTargetList.Count > 0 ? KnownTargetList[0].ToString() : "";
		}

		#endregion

		#region Target helper functions.

		/**
		 * Create passed in folder on target. Supports hierachical creation.
		 * 
		 * @param	Folder	Path of folder to create.
		 */
		private void TargetMakeDirectory( string Folder )
		{
			// find the last slash, if there is one
			int LastSlash = Folder.LastIndexOf("\\");
			// make sure there was a slash, and it wasn't the first character
			if (LastSlash > 0)
			{
				// if there was a valid slash, make the outer directory
				TargetMakeDirectory(Folder.Substring(0, LastSlash));
			}

			// Convert relative path to platform specific one.
			Folder = BaseDirectory + Folder;

			// Do the action on each console
			foreach (int ConsoleIndex in ConnectedTargets)
			{
				// Create folder, no suppport for hierachical creation.
				if ( VerboseOutput )
				{
					OutputHandler.OutputText("Creating " + Folder);
				}
				// striup off any trailing slash
				if (Folder.LastIndexOf("\\") == Folder.Length - 1)
				{
					Folder = Folder.Substring(0, Folder.Length - 1);
				}

				// don't worry about any errors, because its probbly that it already existed
				// @todo: make already exist not an error in the dll, and then handle errors
				DLLInterface.MakeDirectory(ConsoleIndex, Folder);
			}
		}

		/**
		 * Copies file from source to destination path.  The copy only happens if the destination
		 * file does not exist, has a different length from the source file, or is older than the
		 * source file.
		 * 
		 * The Force and NoSync flags modify the behavior of the function.  Force causes the file
		 * to always be treated as out of date, and the NoSync flag causes the function to not
		 * actually copy the file (logging is still enabled so that this can be used to preview
		 * the operation)
		 * 
		 * @param	SourcePath		Source path
		 * @param	DestPath		Destination path
		 */
		private bool TargetCopyFile( int ConsoleIndex, string SourcePath, string DestPath )
		{
			bool bCopySucceeded = true;

			if ( Force || DLLInterface.NeedsToSendFile( ConsoleIndex, SourcePath, DestPath ) )
			{
				if ( !NoSync )
				{
					OutputHandler.OutputText("Copying " + SourcePath + " to " + ConnectedTargetNames[ConsoleIndex] + ":" + DestPath);

					try
					{
						bCopySucceeded = DLLInterface.SendFile(ConsoleIndex, SourcePath, DestPath);
					}
					catch( System.Runtime.InteropServices.COMException )
					{
						bCopySucceeded = false;
					}
					if( !bCopySucceeded )
					{
						OutputHandler.OutputText( "==> COPY FAILED", Color.Red );
						try
						{
							FailedCopies.Add( "Failed copy: " + ConnectedTargetNames[ConsoleIndex] + ":" + DestPath );
						}
						catch( System.Exception ) {}
					}
				}
				else
				{
					try
					{
						OutputHandler.OutputText("Would copy " + SourcePath + " to " + ConnectedTargetNames[ConsoleIndex] + ":" + DestPath);
					}
					catch( System.Exception ) {}
				}
			}

			return bCopySucceeded;
		}

		private bool PcMakeDirectory( string Folder )
		{
			// Do the action on each path
			foreach (String Root in TargetPaths)
			{
				string Path = Root + Folder;

				DirectoryInfo Info = new DirectoryInfo( Path );
				if ( !Info.Exists )
				{
					OutputHandler.OutputText("Creating " + Path);
					try 
					{
						Info.Create();
					}
					catch ( Exception e )
					{
						OutputHandler.OutputText("=> Error: " + e.Message, Color.Red );
						return false;
					}
				}
			}

			return true;
		}

		private bool PcRequireCopy( FileInfo SrcFile, FileInfo DstFile )
		{
			if ( !SrcFile.Exists )
			{
				return false;
			}

			if ( Force )
			{
				return true;
			}

			if ( !DstFile.Exists )
			{
				return true;
			}

			// compare the lengths
			if ( DstFile.Length != SrcFile.Length )
			{
				return true;
			}

			// compare the time stamps
			if ( DateTime.Compare( DstFile.LastWriteTimeUtc, SrcFile.LastWriteTimeUtc ) < 0 )
			{
				return true;
			}

			return false;
		}

		private bool PcCopyFile( string SourcePath, string DestPath, string SrcCRC )
		{
			bool bCopySucceeded = true;

			FileInfo SrcFile = new FileInfo( SourcePath );
			FileInfo DstFile = new FileInfo( DestPath );

			if ( PcRequireCopy( SrcFile, DstFile ) )
			{
				if ( !NoSync )
				{
					OutputHandler.OutputText( "Copying " + SourcePath + " to " + DestPath );

					try
					{
						if ( !DstFile.Directory.Exists )
						{
							DstFile.Directory.Create();
							DstFile.Directory.Refresh();
						}

						if ( DstFile.Exists )
						{
							DstFile.Attributes = FileAttributes.Normal;
							DstFile.Refresh();
						}

						SrcFile.CopyTo( DestPath, true );
					}
					catch( Exception e )
					{
						bCopySucceeded = false;

						OutputHandler.OutputText( "==> FAILED:\r\n\t" + e.Message, Color.Red );
					}
				}
				else
				{
					if ( !VerifyCopy )
					{
						OutputHandler.OutputText("Would copy " + SourcePath + " to " + DestPath);
					}
					else
					{
						OutputHandler.OutputText("Verifying " + DestPath);
					}
				}

				if ( VerifyCopy && bCopySucceeded )
				{
					DstFile.Refresh();
					if ( DstFile.Exists )
					{
						if ( SrcCRC != "0" )
						{
							string DstCRC = CreateCRC( DstFile );

							if ( SrcCRC != DstCRC )
							{
								OutputHandler.OutputText( "==> ERROR: CRC Mismatch", Color.Red );
								CRCMismatchedFiles.Add( "Error: CRC Mismatch for '" + DestPath + "'" );
							}
						}
						else
						{
							OutputHandler.OutputText( "Note: No CRC available for '" + DestPath + "'", Color.Red );	
						}
					}
					else
					{
						if ( VerboseOutput || NoSync )
						{
							OutputHandler.OutputText( "==> ERROR: Missing destination file '" + DestPath + "'", Color.Red );
						}

						CRCMismatchedFiles.Add( "Error: Missing file '" + DestPath + "'" );
					}
				}
			}

			return bCopySucceeded;
		}

		/**
		 * Initializes the console communication.
		 */
		bool TargetInit()
		{
			// do nothing for PC
			if (bIsPC)
			{
				return true;
			}

			// Get each console that was checked
			foreach (string CheckedConsole in TargetNames)
			{
				// Search for an existing console with this name
				// if we successfully connected to the target, then use it
				int TargetIndex = DLLInterface.ConnectToTarget(CheckedConsole);
				if (TargetIndex != -1)
				{
					ConnectedTargets.Add(TargetIndex);
					ConnectedTargetNames.Add(CheckedConsole);
				}
				else
				{
					OutputHandler.OutputText("Failed to connect to " + CheckedConsole + "!!", Color.Red);
				}
			}

			// this was successful if we found at least one target we can use
			if (ConnectedTargets.Count > 0)
			{
				return true;
			}
	
			OutputHandler.OutputText("", Color.Red);
			OutputHandler.OutputText("Error: Failed to find any usable target!", Color.Red);
			return false;
		}

		/**
		 *  Closes any opened connections from TargetInit()
		 */
		void TargetExit()
		{
			// do nothing for PC
			if (bIsPC)
			{
				return;
			}

			// clear out any connected consoles
			foreach (int TargetIndex in ConnectedTargets)
			{
				DLLInterface.DisconnectFromTarget(TargetIndex);
			}

			// we aren't connected to anything
			TargetNames.Clear();
			ConnectedTargetNames.Clear();

			// empty out the list
			ConnectedTargets.Clear();
		}

		/**
		 * Initializes the target paths.
		 */
		bool PcInit()
		{
			// used to indicate potential existance
			bool bWouldExist = false;

			// clear out any connected consoles to reconnect
			TargetPaths.Clear();

			// Get each console that was checked
			foreach (string Path in RequestedTargetPaths)
			{
				DirectoryInfo Info = new DirectoryInfo( Path );
				if ( !Info.Exists )
				{
					if ( NoSync )
					{
						OutputHandler.OutputText("Would create directory: '" + Path + "'");
						bWouldExist = true;
					}
					else
					{
						OutputHandler.OutputText("Creating directory: '" + Path + "'");

						try 
						{
							Info.Create();
						}
						catch ( Exception e )
						{
							OutputHandler.OutputText("Error: " + e.Message, Color.Red);
						}

						Info.Refresh();
					}
				}

				if ( Info.Exists || bWouldExist )
				{
					String TargetPath = Path;
					if ( !TargetPath.EndsWith( "\\" ) )
					{
						TargetPath = TargetPath + "\\";
					}

					TargetPaths.Add(TargetPath + BaseDirectory);
				}
			}

			// this was successful if we found at least one target we can use
			if (TargetPaths.Count > 0)
			{
				return true;
			}
	
			OutputHandler.OutputText("Error: Failed to find any valid target paths.", Color.Red);
			return false;
		}

		private String CreateCRC( FileInfo SrcFile )
		{
			FileStream Stream = SrcFile.Open(FileMode.Open, FileAccess.Read, FileShare.Read);

			Byte[] HashData = Hasher.ComputeHash( Stream );

			Stream.Close();

			System.Text.StringBuilder HashCodeBuilder = new System.Text.StringBuilder( HashData.Length * 2 );

			for ( int Index = 0; Index < HashData.Length; ++Index )
			{
				Byte HashDigit = HashData[Index];

				HashCodeBuilder.Append( HexDigits[HashDigit / 16] );
				HashCodeBuilder.Append( HexDigits[HashDigit % 16] );
			}

			return HashCodeBuilder.ToString();
		}

		/**
		 * Finds the stat of the file on the target console if the DVD layout file is set for this game
		 * 
		 * @param	Pathname			Target console path to the file to look for start
		 * @return	Start sector on the DVD of the file, or 0 if no DVD layour file can be found
		 */
		private unsafe int TocFindStartSector(string Directory, string Filename)
		{
			if (bHasDVDLayoutFile)
			{
				uint LBAHi = 0;
				uint LBALo = 0;
				// ask the DLL for the start sector of the file
				DLLInterface.GetDVDFileStartSector(Directory + "\\" + Filename, &LBAHi, &LBALo);
				return (int)LBALo;
			}
			// return 0 if we have no layout
			return 0;
		}

		private void TocAddFile( ArrayList TOC, FileInfo SrcFile, string SourceFolder, string DestinationFolder, string TargetSourceFolder, bool bIsForTarget )
		{
			// Full source and destination filenames.
			string		SourceFile				= SourceFolder + SrcFile.ToString();
			string		DestinationFile			= DestinationFolder + SrcFile.ToString();
			string		TargetFile				= TargetSourceFolder + SrcFile.ToString();

			// add every file we attempt to copy to the TOC

			// by default, this file wasn't compressed fully
			int UncompressedSize = 0;

			string ManifestFilename = SourceFile + ManifestExtension;

			// look to see if there is an .uncompressed_size file 
			if (File.Exists(ManifestFilename))
			{
				// if so, open it and read the size out of it
				StreamReader Reader = File.OpenText(ManifestFilename);
				string Line = Reader.ReadLine();
				UncompressedSize = int.Parse(Line);
				Reader.Close();
			}

			// This is the same as SrcFile - still needed?
			FileInfo ExistingFile = new FileInfo( SrcFile.FullName );

			int StartSector = TocFindStartSector(SrcFile.Directory.ToString(), SrcFile.Name);

			if( SrcFile.Exists )
			{
				// add the file size, and optional uncompressed size
				TOC.Add(new TOCInfo(SourceFile, DestinationFile, TargetFile, (int)SrcFile.Length, UncompressedSize, StartSector, "0", ExistingFile.LastWriteTimeUtc, bIsForTarget));
			}
			else
			{
				// In case the file is force added and doesn't exist - eg. the TOC file
				TOC.Add(new TOCInfo(SourceFile, DestinationFile, TargetFile, 0, UncompressedSize, StartSector, "0", System.DateTime.UtcNow, bIsForTarget));
			}
		}

		/**
		 * Builds table of content list for files matching pattern in source folder.
		 * 
		 * @param	SourceFolder		Source folder
		 * @param	SourcePattern		Pattern to match when looking for files
		 * @param	DestinationFolder	Destination folder (optional)
		 * @param	bRecursive			Recursively copy files inside directories inside the given directory
		 */
		private void TocIncludeFiles( ArrayList TOC, string SourceFolder, string SourcePattern, string DestinationFolder, bool bRecursive )
		{
			TocIncludeFiles(TOC, SourceFolder, SourcePattern, DestinationFolder, bRecursive, true);
		}

		/**
		 * Builds table of content list for files matching pattern in source folder.
		 * 
		 * @param	SourceFolder		Source folder
		 * @param	SourcePattern		Pattern to match when looking for files
		 * @param	DestinationFolder	Destination folder (optional)
		 * @param	bRecursive			Recursively copy files inside directories inside the given directory
		 * @param	bIsForTarget		If false, this file will be copied to a UNC path only, and not put in the TOC file
		 */
		private void TocIncludeFiles( ArrayList TOC, string SourceFolder, string SourcePattern, string DestinationFolder, bool bRecursive, bool bIsForTarget )
		{
			// Use source folder if no destination folder was passed in.
			if( DestinationFolder == null )
			{
				DestinationFolder = SourceFolder;
			}

			// remember the original source/dest folders for recursive calls
			string OriginalDestinationFolder = DestinationFolder;
			string OriginalSourceFolder = SourceFolder;

			// replace special tags
			SourceFolder = SourceFolder.Replace("%GAME%", CachedGameName + "Game");
			SourcePattern = SourcePattern.Replace("%GAME%", CachedGameName + "Game");
			DestinationFolder = DestinationFolder.Replace("%GAME%", CachedGameName + "Game");
			SourceFolder = SourceFolder.Replace("%PLATFORM%", SavedPlatform);
			SourcePattern = SourcePattern.Replace("%PLATFORM%", SavedPlatform);
			DestinationFolder = DestinationFolder.Replace("%PLATFORM%", SavedPlatform);

			// Convert relative paths to platform specific ones.
			string TargetSourceFolder	= "..\\" + DestinationFolder;
			SourceFolder		= StartupPath + "\\..\\" + SourceFolder;

			// make sure destination folder exists
			try
			{
				// Find files matching pattern.
				DirectoryInfo SourceDirectory = new DirectoryInfo( SourceFolder );
				FileInfo[] Files = SourceDirectory.GetFiles( SourcePattern );
				foreach( FileInfo SrcFile in Files )
				{
					TocAddFile( TOC, SrcFile, SourceFolder, DestinationFolder, TargetSourceFolder, bIsForTarget );
				}

				// if we want to recursively copy directories, do that now
				if (bRecursive)
				{
					DirectoryInfo[] Dirs = SourceDirectory.GetDirectories( "*.*" );
					foreach( DirectoryInfo Dir in Dirs )
					{
						// copy the files in that directory
						TocIncludeFiles(TOC, OriginalSourceFolder + Dir.Name + "\\", SourcePattern, OriginalDestinationFolder + Dir.Name + "\\" , bRecursive, bIsForTarget);
					}
				}
			}
			// gracefully handle copying from non existant folder.
			catch( System.IO.DirectoryNotFoundException )
			{
			}
		}

		private void TocGenerateCRC( ArrayList TOC )
		{
			OutputHandler.OutputText( "\r\n[GENERATING CRC STARTED]", Color.Green );
			DateTime StartTime = DateTime.UtcNow;

			foreach (TOCInfo Entry in TOC)
			{
				if ( Entry.CRC != "0" )
					continue;

				FileInfo File = new FileInfo( Entry.SourceFilename );
				if ( File.Name == TOCFilename )
				{
					// ignore the TOC file
					Entry.CRC = "0";
				}
				else
				{
					OutputHandler.OutputText( Entry.SourceFilename );

					if ( File.Exists )
					{													
						Entry.CRC = CreateCRC( File );
					}
					else
					{
						Entry.CRC = "0";
					}

					if ( VerboseOutput )
					{
						OutputHandler.OutputText("\t... " + Entry.CRC);
					}
				}
			}

			TimeSpan Duration = DateTime.UtcNow.Subtract( StartTime );
			OutputHandler.OutputText( "Operation took " + Duration.Minutes.ToString() + ":" + Duration.Seconds.ToString( "D2" ), Color.Green );
			OutputHandler.OutputText( "[GENERATING CRC FINISHED]", Color.Green );
		}

		/**
		 * Builds the table of contents required for the game.
		 * 
		 * @return table of contents
		 */
		private ArrayList TocBuild()
		{
			ArrayList TOC = new ArrayList();

			string PlatformName = SavedPlatform;
			// handle the fact that Xbox360 uses directories called Xenon :(
			if (PlatformName == "Xbox360")
			{
				PlatformName = "Xenon";
			}

			// shared splash screens (uses SavedPlatform because the directory is Xbox360, not Xenon)
			TocIncludeFiles(TOC, "Engine\\Splash\\" + SavedPlatform + "\\", "*.bmp", null, false);

            // include the engine stats dir so the automated perf testing can create .html files
            TocIncludeFiles(TOC, "Engine\\Stats\\", "*.*", null, true);

            // include the cooked inis
			TocIncludeFiles(TOC, CachedGameName + "Game\\Config\\" + PlatformName + "\\Cooked\\", "Coalesced.ini", null, false);

			// include all localization files
			//
			// NOTE: If this directory changes, you will NEED to change the code in FConfigCacheIni.cpp that
			// assumes the name of the directory!
			TocIncludeFiles(TOC, CachedGameName + "Game\\Localization\\", "Coalesced.*", null, false);

			// Game specific files.
			TocIncludeFiles(TOC, CachedGameName + "Game\\Cooked" + PlatformName + "\\", "*.xxx", null, false);

			// Splash screens (uses SavedPlatform because the directory is Xbox360, not Xenon)
			TocIncludeFiles(TOC, CachedGameName + "Game\\Splash\\" + SavedPlatform + "\\", "*.bmp", null, false);

			// Bink movies and subtitle text
            TocIncludeFiles(TOC, CachedGameName + "Game\\Movies\\", "*.bik", null, false);
            TocIncludeFiles(TOC, CachedGameName + "Game\\Movies\\", "*.txt", null, false);

			// UnrealFrontend temporary exec text file
			TocIncludeFiles(TOC, "Binaries\\", "UnrealFrontend_TmpExec.txt", null, false);
			
			if( CachedGameSettings != null && CachedGameSettings.SyncPaths != null )
			{
				foreach (PathInfo Info in CachedGameSettings.SyncPaths)
				{
					TocIncludeFiles( TOC, Info.Path, Info.Wildcard, Info.DestPath, Info.bIsRecursive, Info.bIsForTarget );
				}
			}

			if (CachedPlatformSettings != null && CachedPlatformSettings.SyncPaths != null)
			{
				foreach (PathInfo Info in CachedPlatformSettings.SyncPaths)
				{
					TocIncludeFiles(TOC, Info.Path, Info.Wildcard, Info.DestPath, Info.bIsRecursive, Info.bIsForTarget);
				}
			}

			// *ALWAYS* include TOC
			string DestinationFolder = CachedGameName + "Game\\";
			string TargetSourceFolder = "..\\" + DestinationFolder;
			string SourceFolder = StartupPath + "\\..\\" + DestinationFolder;

			TocAddFile( TOC, new FileInfo( TOCFilename ), SourceFolder, DestinationFolder, TargetSourceFolder, true );

			return TOC;
		}

		/**
		 * Reads the table of contents from disk.
		 * 
		 * @param	TOCPath		Source path for table of contents
		 * @return  TOC         Table of contents
		 */
		private ArrayList TocRead( String TOCPath )
		{
			ArrayList TOC = new ArrayList();

			try
			{			
				StreamReader Reader = new StreamReader(TOCPath, false);
			
				while( true )
				{
					String Line = Reader.ReadLine();
					if ( Line == null )
						break;

					String[] Words = Line.Split(' ');

					if ( Words.Length < 3 )
						continue;

					int Size                = Int32.Parse(Words[0]);
					int CompressedSize      = Int32.Parse(Words[1]);
					int StartSector			= Int32.Parse(Words[2]);
					string ConsoleFilenname = Words[3];
					string CRC              = ( Words.Length > 4 ) ? Words[4] : "0";

					TOC.Add( new TOCInfo( "", "", ConsoleFilenname, Size, CompressedSize, StartSector, CRC, new DateTime(0), true ) );
				}

				Reader.Close();

				return TOC;
			}
			catch( Exception /* e */ )
			{
				return new ArrayList();
			}
		}

		private TOCInfo TocFindInfoFromConsoleName( ArrayList TOC, String ConsoleName )
		{
			foreach (TOCInfo Entry in TOC)
			{
				if ( Entry.ConsoleFilename == ConsoleName )
					return Entry;
			}

			return null;
		}

		private void TocMerge( ArrayList CurrentTOC, ArrayList OldTOC, DateTime TocLastWriteTime )
		{
			//OutputHandler.OutputText( "TOC time: " + TocLastWriteTime.ToLongDateString() + " " + TocLastWriteTime.ToLongTimeString() );
		
			foreach (TOCInfo CurrentEntry in CurrentTOC)
			{
				TOCInfo OldEntry = TocFindInfoFromConsoleName( OldTOC, CurrentEntry.ConsoleFilename );
				if ( OldEntry != null )
				{
					//OutputHandler.OutputText( "File time: " + CurrentEntry.LastWriteTime.ToLongDateString() + " " + CurrentEntry.LastWriteTime.ToLongTimeString() );

					if ( DateTime.Compare( TocLastWriteTime, CurrentEntry.LastWriteTime ) < 0 )
					{
						OutputHandler.OutputText( "TOC older than file: '" + CurrentEntry.ConsoleFilename + "'" );
						continue;
					}

					//OutputHandler.OutputText( "TOC timestamp up to date for: '" + CurrentEntry.ConsoleFilename + "'" );

					if ( CurrentEntry.Size == OldEntry.Size && CurrentEntry.CompressedSize == OldEntry.CompressedSize )
						CurrentEntry.CRC = OldEntry.CRC;
					//else
					//	OutputHandler.OutputText( "TOC size mismatch for: '" + CurrentEntry.ConsoleFilename + "'" );

				}
			}
		}

		private void TocPrint( ArrayList TOC )
		{
			foreach (TOCInfo Entry in TOC)
			{
				String Text;

				// skip files that don't need to be in the TOC file
				if (!Entry.bIsForTarget)
				{
					continue;
				}

				if ( Entry.CRC != null )
				{
					Text = String.Format( "{0} {1} {2} {3} {4}", Entry.Size, Entry.CompressedSize, Entry.StartSector, Entry.ConsoleFilename, Entry.CRC );
				}
				else
				{
					Text = String.Format( "{0} {1} {2} {3}", Entry.Size, Entry.CompressedSize, Entry.StartSector, Entry.ConsoleFilename );
				}
	
				OutputHandler.OutputText( Text );
			}
		}

		/**
		 * Writes the table of contents to disk.
		 * 
		 * @param	TOC			Table of contents
		 * @param	TOCPath		Destination path for table of contents
		 */
		public void TocWrite( ArrayList TOC, String TOCPath )
		{
			try
			{
				// Make sure the file is not read only
				FileInfo Info = new FileInfo( TOCPath );
				if( Info.Exists )
				{
					Info.Attributes = FileAttributes.Normal;		
				}

				// delete it first because sometimes it appends, not overwrites, unlike the false says in new StreamWriter!!
				File.Delete( TOCPath );

				// Write out each element of the TOC file
				StreamWriter Writer = new StreamWriter(TOCPath, false);
				foreach (TOCInfo Entry in TOC)
				{
					// skip files that don't need to be in the TOC file
					if (!Entry.bIsForTarget)
					{
						continue;
					}

					if (Entry.CRC != null)
					{
						Writer.WriteLine("{0} {1} {2} {3} {4}", Entry.Size, Entry.CompressedSize, Entry.StartSector, Entry.ConsoleFilename, Entry.CRC);
					}
					else
					{
						Writer.WriteLine("{0} {1} {2} {3} 0", Entry.Size, Entry.CompressedSize, Entry.StartSector, Entry.ConsoleFilename);
					}
				}
				Writer.Close();
			}
			catch( System.Exception e )
			{
				OutputHandler.OutputText( "Error: Failed to write TOC file - " + e.Message, Color.Red );
			}
		}

		/**
		 * Ensures the folder exists on the console.
		 */		
		private void TargetEnsureDirectoryExists( string Name )
		{
			string NewDirectory = "";

			// If the path has changed, create the folder.
			int LastIndex = Name.LastIndexOf( '\\' );
			if( LastIndex > 0 )
			{
				NewDirectory = Name.Substring( 0, Name.LastIndexOf( '\\' ) );

				if ( NewDirectory != OldDirectory )
				{
					TargetMakeDirectory( NewDirectory );
					OldDirectory = NewDirectory;
				}
			}
		}

		/**
		 * Syncs the console with the PC.
		 * 
		 * @return true if succcessful
		 */
		private bool TargetSync( ArrayList TOC )
		{
			if (PlatformNeedsToSync() == false)
			{
				return true;
			}

			// Initialize console communication.
			if (TargetInit() == false)
			{
				TargetExit();
				return false;
			}

			OutputHandler.OutputText( "\r\n[SYNCING DATA STARTED]", Color.Green );
			DateTime StartTime = DateTime.UtcNow;

			if ( !NoSync )
			{
				// make sure the base directory exists. To do so, we need to temporarily make the base directory just blank
				string OldBaseDirectory = BaseDirectory;
				BaseDirectory = "";

				// create the base directory
				TargetMakeDirectory(OldBaseDirectory);

				// restore the original
				BaseDirectory = OldBaseDirectory;

				// make the logs directory, because nothing is copied to it, so it won't get created by default
				TargetMakeDirectory( CachedGameName + "Game\\Logs\\" );
			}

			// Do the action on each console
			OldDirectory = "";
			FailedCopies.Clear();

			foreach (int ConsoleIndex in ConnectedTargets)
			{
				foreach (TOCInfo Entry in TOC)
				{
					// skip files that don't need to be copied to a target
					if (!Entry.bIsForTarget)
					{
						continue;
					}

					TargetEnsureDirectoryExists(Entry.DestinationFilename);

					if (TargetCopyFile( ConsoleIndex, Entry.SourceFilename, BaseDirectory + Entry.DestinationFilename ) == false)
					{
						// stop copying to this target (an error will already be printed out)
						break;
					}
				}
			}

			foreach( string Message in FailedCopies )
			{
				OutputHandler.OutputText( Message, Color.Red );
			}

			TimeSpan Duration = DateTime.UtcNow.Subtract( StartTime );
			OutputHandler.OutputText( "Operation took " + Duration.Minutes.ToString() + ":" + Duration.Seconds.ToString( "D2" ), Color.Green );
			OutputHandler.OutputText( "[SYNCING WITH CONSOLE FINISHED]", Color.Green );

			TargetExit();
			return true;
		}


		/**
		 * Syncs the console with the PC.
		 * 
		 * @return true if succcessful
		 */
		public bool PcSync( ArrayList TOC )
		{
			// Initialize the target paths.
			if (PcInit() == false)
			{
				return false;
			}

			OutputHandler.OutputText( "\r\n[SYNCING WITH FOLDERS STARTED]", Color.Green );
			DateTime StartTime = DateTime.UtcNow;

			CRCMismatchedFiles = new ArrayList();

			if ( !NoSync )
			{
				// make the logs directory, because nothing is copied to it, so it won't get created by default
				if ( !PcMakeDirectory( CachedGameName + "Game\\Logs\\" ) )
					return false;
			}

			// Do the action on each path
			foreach (String Path in TargetPaths)
			{
				// Copy each file from the table of contents
				foreach (TOCInfo Entry in TOC)
				{
					if ( !PcCopyFile( Entry.SourceFilename, Path + Entry.DestinationFilename, Entry.CRC ) )
						return false;
				}
			}

			foreach( string File in CRCMismatchedFiles )
			{
				OutputHandler.OutputText( File, Color.Red );
			}

			TimeSpan Duration = DateTime.UtcNow.Subtract( StartTime );
			OutputHandler.OutputText( "Operation took " + Duration.Minutes.ToString() + ":" + Duration.Seconds.ToString( "D2" ), Color.Green );
			OutputHandler.OutputText( "[SYNCING WITH FOLDERS FINISHED]", Color.Green );

			return true;
		}

		public bool TargetSync()
		{
			string TOCPath = StartupPath + "\\..\\" + CachedGameName + "Game\\" + TOCFilename;

			// Build up the file-list (we always need to do this)

			ArrayList TOC = TocBuild();

			// Merge the CRCs from the existing TOC file (if there is one)

			if ( MergeExistingCRC )
			{
				if ( VerboseOutput )
					OutputHandler.OutputText( "Reading TOC " + TOCPath );

				FileInfo TocInfo = new FileInfo( TOCPath );

				if ( TocInfo.Exists )
				{
					ArrayList PreviousTOC = TocRead( TOCPath );
					TocMerge( TOC, PreviousTOC, TocInfo.LastWriteTimeUtc );
				}
			}

			if ( ComputeCRC )
			{
				TocGenerateCRC( TOC );

				if ( VerboseOutput && !NoSync )
					OutputHandler.OutputText( "" );
			}

			if ( !NoSync )
			{
				if ( VerboseOutput )
					OutputHandler.OutputText( "Writing TOC " + TOCPath );
				TocWrite( TOC, TOCPath );
			}

			if ( TargetNames.Count > 0 )
			{
				if ( !TargetSync( TOC ) )
					return false;
			}

			if ( RequestedTargetPaths.Count > 0 )
			{
				if ( !PcSync( TOC ) )
					return false;
			}

			return true;
		}

		private bool UnrealConsoleAlreadyRunning( Process[] Processes, string ConsoleName )
		{
			string WindowTitle;
			// build a string to match the window text of the UnrealConsole app
			WindowTitle = "Unreal Console: " + ConsoleName;

			foreach( Process aProcess in Processes )
			{
				if( aProcess.MainWindowTitle.StartsWith(WindowTitle) )
				{
					return( true );
				}
			}
			return( false );
		}

		private void SpawnUnrealConsole( string WorkingDirectory, string ConsoleName )
		{
			// Launch UnrealConsole only if it isn't already running
			ProcessStartInfo PSI = new ProcessStartInfo();

			PSI.WorkingDirectory = WorkingDirectory;
			PSI.FileName = WorkingDirectory + "\\UnrealConsole.exe";
			PSI.Arguments = ConsoleName + " platform=" + SavedPlatform;
			PSI.UseShellExecute = false;

			System.Diagnostics.Process.Start( PSI );
			
			// Allow time for the process to start before the 360 gets rebooted
			System.Threading.Thread.Sleep( 1000 );
		}

		/// <summary>
		/// Reboots the target(s) with to the given executable
		/// </summary>
		/// <param name="Executable"></param>
		/// <param name="MediaPath"></param>
		/// <param name="CommandLine"></param>
		public bool TargetReboot( bool bShouldRunGame, string Configuration, string CommandLine )
		{
			if (!bIsActivated)
			{
				return false;
			}

			bool bWasSuccessful = true;
			if( TargetInit() == false )
			{
				TargetExit();
				return( false );
			}

			// Only spawn UnrealConsole if we're running the game
			if(bShouldRunGame)
			{
				string WorkingDirectory = System.Environment.CurrentDirectory;

				Process[] Processes;
				Processes = System.Diagnostics.Process.GetProcessesByName( "UnrealConsole" );

				// Spawn a UnrealConsole for each valid console
				foreach( string ConsoleName in ConnectedTargetNames )
				{
					if( !UnrealConsoleAlreadyRunning( Processes, ConsoleName ) )
					{
						SpawnUnrealConsole( WorkingDirectory, ConsoleName );
					}
				}
			}

			// Do the action on each console
			foreach (int ConsoleIndex in ConnectedTargets)
			{
				try
				{
					if (bShouldRunGame)
					{
						string BaseDirWithNoTrailingSlash = BaseDirectory.Substring(0, BaseDirectory.Length - 1);
						DLLInterface.RebootAndRun(ConsoleIndex, Configuration, BaseDirWithNoTrailingSlash, CachedGameName, CommandLine);
					}
					else
					{
						DLLInterface.Reboot(ConsoleIndex);
					}
				}

				catch( System.Runtime.InteropServices.COMException )
				{
					OutputHandler.OutputText( "Failed to reboot target: " + ConnectedTargetNames[ConsoleIndex] );
					bWasSuccessful = false;
				}
			}

			TargetExit();
			return bWasSuccessful;
		}

		/**
		 * @return true if the current platform needs to sync to the target 
		 */
		public bool PlatformNeedsToSync()
		{
			// @hack: Yet another C++->Managed C++->C# weirdness - PS3 DLL returns false from the function
			// but it is true when it gets here.
			if (SavedPlatform == "PS3" || !bIsActivated)
			{
				return false;
			}
			return DLLInterface.PlatformNeedsToCopyFiles();
		}

		#endregion

	}
	#endregion
}

