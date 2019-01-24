/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using System.Text;
using System.Text.RegularExpressions;

using UnrealDVDLayout.Properties;

namespace UnrealDVDLayout
{
	public class TOCInfo
	{
		// The type of the TOC entry
		public UnrealDVDLayout.ObjectType Type;
		// DVD destination layer (0, 1 or -1 for none)
		public int Layer = -1;
		// Exact byte size of file
		public long Size = 0;
		// Size in 2048 byte sectors
		public long SectorSize = 0;
		// Size of the file when decompressed
		public long DecompressedSize = 0;
		// Index of file, used for sorting read requests (aka lsn)
		public long LBA = 0;
		// The original TOC file this entry came from
		public string OwnerTOC;
		// Full path to file
		public string Path = "";
		// The folder name
		public string SubDirectory = "";
		// The file name
		public string Name = "";
		// Set to true if the file is included more than once
		public bool Duplicate = false;
		// Set to true if the file is a TOC file
		public bool IsTOC = false;
		// Set to true if the file is the fingerprint file
		public bool IsFingerprint = false;
		// First numeric value in the string (used for sorting)
		public int Numeric = 0;
		// The hex string of the 128 bit checksum
		public string CRCString = "";
		// The name of the group this entry currently resides in
		public TOCGroup Group = null;

		public TOCInfo( string Line )
		{
			string[] Elements = Line.Split( ' ' );
			if( Elements.Length == 5 )
			{
				try
				{
					Type = UnrealDVDLayout.ObjectType.File;
					Size = Int64.Parse( Elements[0] );
					SectorSize = ( Size + ( UnrealDVDLayout.BytesPerSector - 1 ) ) / UnrealDVDLayout.BytesPerSector;
					DecompressedSize = Int64.Parse( Elements[1] );
					Path = Elements[3].TrimStart( '.' );
					CRCString = Elements[4];
				}
				catch
				{
					Path = "";
				}
			}
		}

		public TOCInfo( FileInfo Info, string RelativeName )
		{
			try
			{
				Type = UnrealDVDLayout.ObjectType.File;
				Size = Info.Length;
				SectorSize = ( Size + ( UnrealDVDLayout.BytesPerSector - 1 ) ) / UnrealDVDLayout.BytesPerSector;
				Path = RelativeName;
			}
			catch
			{
				Path = "";
			}
		}

		public TOCInfo( UnrealDVDLayout.ObjectType InType, string Folder )
		{
			Type = InType;
			SectorSize = 1;
			Path = Folder;
		}

		public void DeriveData( string TOCFileName )
		{
			// Set the owning TOC
			OwnerTOC = TOCFileName;

			// Create the split name
			int SlashIndex = Path.LastIndexOf( '\\' );
			SubDirectory = Path.Substring( 0, SlashIndex );
			Name = Path.Substring( SlashIndex + 1 );

			// Check for a TOC
			if( OwnerTOC != null )
			{
				if( OwnerTOC.ToLower().Contains( Name.ToLower() ) )
				{
					IsTOC = true;
					Size = 0;
				}
			}

			// Set up the TOC files
			if( OwnerTOC != null && Size == 0 )
			{
				FileInfo Info = new FileInfo( OwnerTOC );
				if( Info.Exists )
				{
					Size = Info.Length;
					SectorSize = ( Size + ( UnrealDVDLayout.BytesPerSector - 1 ) ) / UnrealDVDLayout.BytesPerSector;
				}
			}

			if( SubDirectory.Length == 0 )
			{
				SubDirectory = "\\";
			}

			// Find the numeric value
			string Number = "";
			bool ParsingNumber = false;
			foreach( char Letter in Name )
			{
				if( Letter >= '0' && Letter <= '9' )
				{
					ParsingNumber = true;
					Number += Letter;
				}
				else if( ParsingNumber )
				{
					break;
				}
			}

			if( Number.Length > 0 )
			{
				Numeric = Int32.Parse( Number );
			}
		}
	}

	public enum GroupSortType
	{
		Alphabetical,
		ReverseAlphabetical,
		FirstNumeric,
		Explicit
	};

	public class TOCGroup
	{
		[XmlAttribute]
		public string GroupName;

		[XmlAttribute]
		public List<string> Files = new List<string>();

		[XmlAttribute]
		public string RegExp = "";

		[XmlAttribute]
		public GroupSortType SortType = GroupSortType.Alphabetical;

		[XmlAttribute("SortOrder")]
		public string RawSortOrderString = "";

		[XmlIgnore]
		public List<Regex> SortOrderRegEx = new List<Regex>();

		[XmlIgnore]
		public int Layer = -1;

		[XmlIgnore]
		public long Size = 0;

		[XmlIgnore]
		public long SectorSize = 0;

		[XmlIgnore]
		public bool GroupSelected = false;

		[XmlIgnore]
		public bool LayerSelected = false;

		public TOCGroup()
		{
		}

		public void UnpackSortOrdering()
		{
			string[] StringSortOrder = RawSortOrderString.Split( ' ' );
			SortOrderRegEx.Clear();

			for( int i = 0; i < StringSortOrder.Length; i++ )
			{
				string StringRegExp = StringSortOrder[i];
				SortOrderRegEx.Add( new Regex( StringRegExp, RegexOptions.IgnoreCase | RegexOptions.Compiled ) );
			}
		}

		public int GetSortIndex( TOCInfo Info )
		{
			for( int i = 0; i < SortOrderRegEx.Count; i++ )
			{
				Match RegExpMatch = SortOrderRegEx[i].Match( Info.Path );
				if( RegExpMatch.Success )
				{
					return i;
				}
			}
			// no match, just throw it alphabetically at the end
			return SortOrderRegEx.Count;
		}
	}

	public class TOCGroups
	{
		// All groups that exist
		[XmlElement]
		public List<TOCGroup> TOCGroupEntries = new List<TOCGroup>();

		// Groups assigned to layer 0
		[XmlElement]
		public List<string> TOCGroupLayer0 = new List<string>();

		// Groups assigned to layer 1
		[XmlElement]
		public List<string> TOCGroupLayer1 = new List<string>();

		public TOCGroups()
		{
		}
	}

	public class TOC
	{
		private UnrealDVDLayout Main = null;
		private float TotalSizeGB = 0.0f;

		public string SourceFolder = "";
		public List<TOCInfo> TOCFileEntries = new List<TOCInfo>();
		public TOCGroups Groups = new TOCGroups();

		public TOC( UnrealDVDLayout InMain, string TOCFolder )
		{
			Main = InMain;
			SourceFolder = TOCFolder;
		}

		public int CompareTOCInfo( TOCInfo X, TOCInfo Y )
		{
			int CompareReturn = 0;
			if( X.Group != null && X.Group == Y.Group )
			{
				switch( X.Group.SortType )
				{
					case GroupSortType.FirstNumeric:
						if( X.Numeric != Y.Numeric )
						{
							CompareReturn = ( X.Numeric - Y.Numeric );
						}
						else if( Y.Name.Length != X.Name.Length )
						{
							CompareReturn = ( Y.Name.Length - X.Name.Length );
						}
						else
						{
							CompareReturn = String.Compare( X.Path, Y.Path, true );
						}
						break;

					case GroupSortType.ReverseAlphabetical:
						CompareReturn = ( String.Compare( Y.Path, X.Path, true ) );
						break;

					case GroupSortType.Explicit:
						{
							int XIndex = X.Group.GetSortIndex( X );
							int YIndex = Y.Group.GetSortIndex( Y );
							if( XIndex == YIndex )
							{
								CompareReturn = ( String.Compare( X.Path, Y.Path, true ) );
							}
							else
							{
								CompareReturn = ( XIndex - YIndex );
							}
						}
						break;
				}

				// ... else fall through to the default alphabetical
			}

			CompareReturn = ( String.Compare( X.Path, Y.Path, true ) );

			if( X.Layer == 0 )
			{
				CompareReturn = -CompareReturn;
			}

			return CompareReturn;
		}

		public string GetSummary()
		{
			return ( TOCFileEntries.Count.ToString() + " files totaling " + TotalSizeGB.ToString( "F2" ) + " GB" );
		}

		// Find all unique folders and add an entry for them
		public void CreateFolders()
		{
			List<string> Folders = new List<string>();

			// Get the unique folder names
			foreach( TOCInfo TOCEntry in TOCFileEntries )
			{
				string[] FolderNames = TOCEntry.SubDirectory.Split( '\\' );
				string FolderName = "";

				for( int Index = 1; Index < FolderNames.Length; Index++ )
				{
					FolderName += "\\" + FolderNames[Index];
					bool bFolderExists = false;
					foreach( string Folder in Folders )
					{
						if( Folder.ToLower() == FolderName.ToLower() )
						{
							bFolderExists = true;
							break;
						}
					}

					if( !bFolderExists )
					{
						Folders.Add( FolderName );
					}
				}
			}

			// Put them in a reasonable order
			Folders.Sort();

			// Create the relevant TOC entries
			foreach( string Folder in Folders )
			{
				if( Folder.Length > 1 )
				{
					TOCInfo TOCEntry = new TOCInfo( UnrealDVDLayout.ObjectType.Directory, Folder );
					TOCEntry.DeriveData( null );
					TOCFileEntries.Add( TOCEntry );
				}
			}
		}

		public bool GetXboxSystemUpdateFile()
		{
			string SystemUpdatePath = SourceFolder + "\\$SystemUpdate";
			Directory.CreateDirectory( SystemUpdatePath );

			// Get the path to the update file for this XDK
			string Path = Environment.GetEnvironmentVariable( "XEDK" );
			Path += "\\redist\\xbox\\$SystemUpdate";

			DirectoryInfo DirInfo = new DirectoryInfo( Path );
			foreach( FileInfo Info in DirInfo.GetFiles() )
			{
				Info.CopyTo( SystemUpdatePath + "\\" + Info.Name, true );

				FileInfo SystemUpdateFileInfo = new FileInfo( SystemUpdatePath + "\\" + Info.Name );

				TOCInfo TOCFileEntry = new TOCInfo( SystemUpdateFileInfo, "\\$SystemUpdate\\" + Info.Name );
				TOCFileEntry.DeriveData( null );

				if( TOCFileEntry.Path.Length > 0 )
				{
					TOCFileEntries.Add( TOCFileEntry );
					return ( true );
				}
			}

			return ( false );
		}

		private string GetConfigSuffix()
		{
			string Suffix;
			switch( Main.Options.BuildConfiguration )
			{
			case 1: // Release
				Suffix = "Release";
				break;

			case 2: // Debug
				Suffix = "ReleaseLTCG-DebugConsole";
				break;

			case 0: // Shipping
			default:
				Suffix = "ReleaseLTCG";
				break;
			}
			return Suffix;
		}

		private bool GetBinary( string Source, string Dest )
		{
			FileInfo Info = new FileInfo( Source );
			if( Info.Exists )
			{
				FileInfo BinaryInfo = new FileInfo( SourceFolder + Dest );
				if( BinaryInfo.Exists )
				{
					BinaryInfo.IsReadOnly = false;
				}

				Info.CopyTo( SourceFolder + Dest, true );

				BinaryInfo = new FileInfo( SourceFolder + Dest );

				TOCInfo TOCFileEntry = new TOCInfo( BinaryInfo, Dest );
				TOCFileEntry.DeriveData( null );

				if( TOCFileEntry.Path.Length > 0 )
				{
					TOCFileEntries.Add( TOCFileEntry );
					return ( true );
				}
			}

			return ( false );
		}

		public bool GetXex( string GameName )
		{
			string XexName = SourceFolder + "\\" + GameName + "-Xe" + GetConfigSuffix() + ".xex";
			if( GetBinary( XexName, "\\default.xex" ) )
			{
				return( true );
			}

			Main.Error( "CANNOT FIND XEX FILE!!!  Check your config ...." );
			return ( false );
		}

		public bool GetEboot( string GameName )
		{
			string EbootName = SourceFolder + "\\Binaries\\PS3\\" + GameName + "-PS3" + GetConfigSuffix() + ".elf";
			if( GetBinary( EbootName, "\\EBOOT.BIN" ) )
			{
				return( true );
			}
			
			Main.Error( "CANNOT FIND ELF FILE!!!  Check your config ...." );
			return ( false );
		}

		private void RecursiveFolderCopy( DirectoryInfo SourceFolderInfo, DirectoryInfo DestFolderInfo )
		{
			foreach( FileInfo SourceFileInfo in SourceFolderInfo.GetFiles() )
			{
				string DestFileName = Path.Combine( DestFolderInfo.FullName, SourceFileInfo.Name );

				FileInfo DestFileInfo = new FileInfo( DestFileName );
				if( DestFileInfo.Exists )
				{
					DestFileInfo.IsReadOnly = false;
					DestFileInfo.Delete();
				}

				File.Copy( SourceFileInfo.FullName, DestFileName );
			}

			foreach( DirectoryInfo SourceSubFolderInfo in SourceFolderInfo.GetDirectories() )
			{
				string DestFolderName = Path.Combine( DestFolderInfo.FullName, SourceSubFolderInfo.Name );
				try
				{
					Directory.CreateDirectory( DestFolderName );
				}
				catch
				{
				}
				RecursiveFolderCopy( SourceSubFolderInfo, new DirectoryInfo( DestFolderName ) );
			}
		}

		public bool CopySupportFiles( string GameName )
		{
			DirectoryInfo SourceDirInfo = new DirectoryInfo( SourceFolder + "\\Development\\Install\\Support\\Support" );
			DirectoryInfo DestDirInfo = new DirectoryInfo( SourceFolder + "\\Support" );

			if( !DestDirInfo.Exists )
			{
				Directory.CreateDirectory( DestDirInfo.FullName );
			}

			RecursiveFolderCopy( SourceDirInfo, DestDirInfo );
			return ( true );
		}

		public bool GetNXEArt( string GameName )
		{
			string NXEArtName = SourceFolder + "\\nxeart";
			FileInfo Info = new FileInfo( NXEArtName );
			if( Info.Exists )
			{
				TOCInfo TOCFileEntry = new TOCInfo( Info, "\\nxeart" );
				TOCFileEntry.DeriveData( null );

				if( TOCFileEntry.Path.Length > 0 )
				{
					TOCFileEntries.Add( TOCFileEntry );
					return ( true );
				}
			}

			Main.Error( "CANNOT FIND NXEART FILE!!!  Check your config ...." );

			return ( false );
		}

		public bool Read( string TOCFileName, string RelativeName, string FingerprintName )
		{
			// Read in and parse the TOC
			FileInfo TOCFile = new FileInfo( TOCFileName );
			if( !TOCFile.Exists )
			{
				Main.Error( "TOC file does not exist!" );
				return ( false );
			}

			StreamReader TOCFileStream = new StreamReader( TOCFileName );

			while( !TOCFileStream.EndOfStream )
			{
				string Line = TOCFileStream.ReadLine();

				TOCInfo TOCFileEntry = new TOCInfo( Line );
				TOCFileEntry.DeriveData( TOCFileName );

				// Mark this file as the fingerprint so it gets added unencrypted to the gp3
				if( FingerprintName.Length > 0 )
				{
					TOCFileEntry.IsFingerprint = TOCFileEntry.Name.ToLower().Contains( FingerprintName.ToLower() );
				}

				if( TOCFileEntry.Path.Length > 0 )
				{
					// Explicitly add the TOC as the last entry
					if( !TOCFileEntry.IsTOC )
					{
						TOCFileEntries.Add( TOCFileEntry );
					}
				}
				else
				{
					Main.Warning( "Invalid TOC entry '" + Line + "' in '" + TOCFileName );
				}
			}

			TOCFileStream.Close();

			// Add in the TOC
			TOCInfo TOCEntry = new TOCInfo( TOCFile, RelativeName );
			TOCEntry.DeriveData( TOCFileName );
			TOCFileEntries.Add( TOCEntry );

			return ( true );
		}

		public void CheckDuplicates()
		{
			// All groups are null, so this is a simple alpha sort
			ApplySort( TOCFileEntries );

			TOCInfo LastTOCEntry = null;
			foreach( TOCInfo TOCEntry in TOCFileEntries )
			{
				if( LastTOCEntry != null )
				{
					if( LastTOCEntry.Path == TOCEntry.Path )
					{
						TOCEntry.Duplicate = true;
						Main.Warning( "Duplicate TOC entry '" + TOCEntry.Path + "'" );
					}
				}

				LastTOCEntry = TOCEntry;
			}
		}

		public bool FinishSetup()
		{
			// unpack explicit sort ordering
			foreach( TOCGroup TempGroup in Groups.TOCGroupEntries )
			{
				TempGroup.UnpackSortOrdering();
			}

			// Sort for a basic layout
			ApplySort( TOCFileEntries );

			// Calculate sizes
			long TotalSize = 0;
			foreach( TOCInfo TOCFileEntry in TOCFileEntries )
			{
				TotalSize += TOCFileEntry.Size;
			}
			TotalSizeGB = TotalSize / ( 1024.0f * 1024.0f * 1024.0f );

			foreach( string GroupName in Groups.TOCGroupLayer0 )
			{
				TOCGroup Group = GetGroup( GroupName );
				Group.Layer = 0;
			}

			foreach( string GroupName in Groups.TOCGroupLayer1 )
			{
				TOCGroup Group = GetGroup( GroupName );
				Group.Layer = 1;
			}

			Main.Log( UnrealDVDLayout.VerbosityLevel.Informative, "[REPORT] TOC contained " + GetSummary(), Color.Blue );
			return ( true );
		}

		public List<string> GetGroupNames()
		{
			List<string> GroupNames = new List<string>();

			foreach( TOCGroup Group in Groups.TOCGroupEntries )
			{
				GroupNames.Add( Group.GroupName );
			}

			return ( GroupNames );
		}

		public TOCGroup GetGroup( string GroupName )
		{
			foreach( TOCGroup Group in Groups.TOCGroupEntries )
			{
				if( Group.GroupName == GroupName )
				{
					return ( Group );
				}
			}

			TOCGroup NewGroup = new TOCGroup();
			NewGroup.GroupName = GroupName;
			Groups.TOCGroupEntries.Add( NewGroup );

			Main.Log( UnrealDVDLayout.VerbosityLevel.Informative, " ... adding new group: '" + GroupName + "'", Color.Blue );

			return ( NewGroup );
		}

		public List<TOCInfo> GetEntriesInGroup( TOCGroup Group )
		{
			List<TOCInfo> Entries = new List<TOCInfo>();
			foreach( TOCInfo Entry in TOCFileEntries )
			{
				if( Entry.Group == Group )
				{
					Entries.Add( Entry );
				}
			}

			return ( Entries );
		}

		public void RemoveGroup( string GroupName )
		{
			if( GroupName != null )
			{
				foreach( TOCGroup Group in Groups.TOCGroupEntries )
				{
					if( Group.GroupName == GroupName )
					{
						Main.Log( UnrealDVDLayout.VerbosityLevel.Informative, " ... removing group: '" + GroupName + "'", Color.Blue );
						Groups.TOCGroupEntries.Remove( Group );
						break;
					}
				}
			}
		}

		public void AddFileToGroup( TOCGroup Group, string FileName )
		{
			foreach( string File in Group.Files )
			{
				if( FileName.ToLower() == File.ToLower() )
				{
					return;
				}
			}

			Group.Files.Add( FileName );
		}

		public void AddGroupToLayer( TOCGroup Group, int Layer )
		{
			Group.Layer = Layer;
			switch( Layer )
			{
				case 0:
					if( !Groups.TOCGroupLayer0.Contains( Group.GroupName ) )
					{
						Groups.TOCGroupLayer0.Add( Group.GroupName );
					}
					break;

				case 1:
					if( !Groups.TOCGroupLayer1.Contains( Group.GroupName ) )
					{
						Groups.TOCGroupLayer1.Add( Group.GroupName );
					}
					break;

				case -1:
					Groups.TOCGroupLayer0.Remove( Group.GroupName );
					Groups.TOCGroupLayer1.Remove( Group.GroupName );
					break;
			}
		}

		public void MoveGroup<T>( List<T> Groups, T Group, int Direction )
		{
			if( Direction == 1 )
			{
				int Index = Groups.IndexOf( Group );
				if( Index > 0 )
				{
					Groups.Remove( Group );
					Groups.Insert( Index - 1, Group );
				}
			}
			else if( Direction == -1 )
			{
				int Index = Groups.IndexOf( Group );
				if( Index < Groups.Count - 1 )
				{
					Groups.Remove( Group );
					Groups.Insert( Index + 1, Group );
				}
			}
		}

		public void MoveGroupInGroups( TOCGroup Group, int Direction )
		{
			MoveGroup<TOCGroup>( Groups.TOCGroupEntries, Group, Direction );
			Group.GroupSelected = true;
		}

		public void MoveGroupInLayer( TOCGroup Group, int Layer, int Direction )
		{
			if( Layer == 0 )
			{
				MoveGroup<string>( Groups.TOCGroupLayer0, Group.GroupName, Direction );
			}
			else
			{
				MoveGroup<string>( Groups.TOCGroupLayer1, Group.GroupName, Direction );
			}

			Group.LayerSelected = true;
		}

		public bool CollateFiles( string Expression, string GroupName )
		{
			// Sanity check the reg exp
			try
			{
				Regex RX = new Regex( Expression, RegexOptions.IgnoreCase | RegexOptions.Compiled );
			}
			catch
			{
				return ( false );
			}

			TOCGroup Group = GetGroup( GroupName );
			Group.RegExp = Expression;

			return ( true );
		}

		public void ApplySort( List<TOCInfo> Entries )
		{
			Entries.Sort( CompareTOCInfo );
		}

		public void CalcXboxDirectorySizes( bool Report )
		{
			Dictionary<string, TOCInfo> Dirs = new Dictionary<string, TOCInfo>();

			// Find all directories
			foreach( TOCInfo TOCEntry in TOCFileEntries )
			{
				TOCGroup Group = TOCEntry.Group;
				if( Group != null && Group.Layer != -1 )
				{
					if( TOCEntry.Type == UnrealDVDLayout.ObjectType.Directory )
					{
						TOCEntry.Size = 0;
						TOCEntry.SectorSize = 0;
						Dirs.Add( TOCEntry.Path, TOCEntry );
					}
				}
			}

			if( Report )
			{
				Main.Log( UnrealDVDLayout.VerbosityLevel.Informative, "Found " + Dirs.Count + " directories", Color.Blue );
			}
		}

		public void UpdateTOCFromGroups()
		{
			// Clear out the in group settings
			foreach( TOCInfo TOCEntry in TOCFileEntries )
			{
				TOCEntry.Group = null;
			}

			// Zero out all the group sizes
			foreach( TOCGroup Group in Groups.TOCGroupEntries )
			{
				Group.Size = 0;
				Group.SectorSize = 0;
			}

			// Filter out anything that passes the regular expression
			foreach( TOCGroup Group in Groups.TOCGroupEntries )
			{
				Regex RX = new Regex( Group.RegExp, RegexOptions.IgnoreCase | RegexOptions.Compiled );

				foreach( TOCInfo TOCEntry in TOCFileEntries )
				{
					if( TOCEntry.Group == null && !TOCEntry.Duplicate )
					{
						Match RegExpMatch = RX.Match( TOCEntry.Path );
						if( RegExpMatch.Success )
						{
							TOCEntry.Group = Group;
							Group.Size += TOCEntry.Size;
							Group.SectorSize += TOCEntry.SectorSize;
						}
						else
						{
							// Try each special case if the regexp doesn't pass
							foreach( string File in Group.Files )
							{
								if( File.ToLower() == TOCEntry.Path.ToLower() )
								{
									TOCEntry.Group = Group;
									Group.Size += TOCEntry.Size;
									Group.SectorSize += TOCEntry.SectorSize;
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}
