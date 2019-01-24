/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using System.Windows.Forms;

namespace UnrealDVDLayout
{
	public class param
	{
		[XmlAttributeAttribute()]
		public string key;

		[XmlAttributeAttribute()]
		public string fmt;

		[XmlAttributeAttribute()]
		public string max_len;

		[XmlTextAttribute()]
		public string Value;
	}

	public class dir
	{
		[XmlElementAttribute( "dir" )]
		public List<dir> dir1 = new List<dir>();

		[XmlAttributeAttribute]
		public string targ_name;

		public dir()
		{
		}

		public dir( string FolderName )
		{
			targ_name = FolderName;
		}

		public void AddFolders( Queue<string> Folders )
		{
			string NewFolder = Folders.Dequeue();
			if( Folders.Count == 0 )
			{
				dir1.Add( new dir( NewFolder ) );
			}
			else
			{
				foreach( dir Folder in dir1 )
				{
					if( Folder.targ_name == NewFolder )
					{
						Folder.AddFolders( Folders );
						break;
					}
				}
			}
		}
	}

	public class psprojectDirs
	{
		[XmlElementAttribute( "dir" )]
		public List<dir> dir = new List<dir>();

		public void AddFolders( Queue<string> Folders )
		{
			string NewFolder = Folders.Dequeue();
			if( Folders.Count == 0 )
			{
				dir.Add( new dir( NewFolder ) );
			}
			else
			{
				foreach( dir Folder in dir )
				{
					if( Folder.targ_name == NewFolder )
					{
						Folder.AddFolders( Folders );
						break;
					}
				}
			}
		}
	}

	public class psprojectFiles
	{
		[XmlElementAttribute( "file" )]
		public List<psprojectFilesFile> file;
	}

	public class psprojectFilesFile
	{
		[XmlElementAttribute]
		public string ps3logodat;

		[XmlArrayAttribute]
		[XmlArrayItemAttribute( "entry", typeof( psprojectFilesFileLicdatEntry ) )]
		public psprojectFilesFileLicdatEntry[] licdat;

		[XmlElementAttribute( "paramsfo" )]
		public List<psprojectFilesFileParamsfo> paramsfo;

		[XmlArrayAttribute]
		[XmlArrayItemAttribute( "param", typeof( param ) )]
		public param[] ps3discsfb;

		[XmlAttributeAttribute]
		public bool enc;

		[XmlAttributeAttribute]
		public bool locked;

		[XmlAttributeAttribute]
		public long rank;

		[XmlAttributeAttribute]
		public long lsn;

		[XmlAttributeAttribute]
		public string targ_path;

		[XmlAttributeAttribute]
		public string orig_path;

		[XmlAttributeAttribute]
		public bool memfile;

		[XmlAttributeAttribute]
		public string fixed_fsize;

		[XmlAttributeAttribute]
		public string mem_len;

		public psprojectFilesFile()
		{
		}

		public psprojectFilesFile( string SourceFile, bool bEncrypt )
		{
			enc = bEncrypt;
			locked = false;
			rank = 128;
			lsn = 0;
			orig_path = "..\\..\\.." + SourceFile;

			if( SourceFile.ToUpper().Contains( "TROPDIR" ) )
			{
				targ_path = "PS3_GAME" + SourceFile.Replace( '\\', '/' ).ToUpper();
			}
			else
			{
				targ_path = "PS3_GAME/USRDIR" + SourceFile.Replace( '\\', '/' ).ToUpper();
			}
		}
	}

	public class psprojectFilesFileLicdatEntry
	{
		[XmlAttributeAttribute]
		public long type;

		[XmlAttributeAttribute]
		public string id;
	}

	public class psprojectFilesFileParamsfo
	{
		[XmlElementAttribute( "param" )]
		public List<param> param;

		[XmlAttributeAttribute]
		public bool add_hidden;
	}

	public class psprojectVolume
	{
		[XmlElementAttribute]
		public string disc_type;

		[XmlElementAttribute]
		public string ps3_disc_name;

		[XmlElementAttribute()]
		public string ps3_date;

		[XmlElementAttribute]
		public string ps3_copyright;

		[XmlElementAttribute]
		public string ps3_producer;

		[XmlElementAttribute]
		public string volume_id;

		[XmlElementAttribute]
		public string ts_dir;

		[XmlElementAttribute]
		public string fs_type;
	}

	public class psproject
	{
		[XmlElementAttribute]
		public psprojectVolume volume;
		[XmlElementAttribute]
		public psprojectFiles files;
		[XmlElementAttribute]
		public psprojectDirs dirs;

		// Add an object based on a TOC entry
		public bool AddObject( TOCInfo TOCEntry, int Layer )
		{
			// do not append files that are not included
			if( Layer == -1 || TOCEntry.Type != UnrealDVDLayout.ObjectType.File )
			{
				return false;
			}

			string FullName = Path.Combine( TOCEntry.SubDirectory, TOCEntry.Name );
			psprojectFilesFile file = new psprojectFilesFile( FullName, !TOCEntry.IsFingerprint );
			files.file.Add( file );

			return ( true );
		}

		public string GetSummary()
		{
			return ( "Not implemented" );
		}

		public string GetPerfSummary()
		{
			return ( "Not implemented" );
		}

		public string GetCapacity()
		{
			return ( "Not implemented" );
		}

		public bool AddFolder( string FolderPath )
		{
			string[] FolderArray = FolderPath.Split( "/\\".ToCharArray() );
			Queue<string> Folders = new Queue<string>( FolderArray );

			dirs.AddFolders( Folders );

			return ( true );
		}
	}

	public partial class UnrealDVDLayout
	{
		private void AddGroupToGP3( string GroupName )
		{
			// Get the group in order and sort the component files
			TOCGroup Group = TableOfContents.GetGroup( GroupName );
			List<TOCInfo> Entries = TableOfContents.GetEntriesInGroup( Group );
			TableOfContents.ApplySort( Entries );

			// Add to the DVD
			foreach( TOCInfo Entry in Entries )
			{
				PS3DiscLayout.AddObject( Entry, Group.Layer );
			}
		}

		private void AddFolders()
		{
			// Make sure the directory structure exists
			if( PS3DiscLayout.dirs == null )
			{
				PS3DiscLayout.dirs = new psprojectDirs();
			}

			// Add the system folders
			PS3DiscLayout.AddFolder( "PS3_GAME" );
			PS3DiscLayout.AddFolder( "PS3_GAME/LICDIR" );
			PS3DiscLayout.AddFolder( "PS3_GAME/USRDIR" );
			PS3DiscLayout.AddFolder( "PS3_UPDATE" );

			// Add all the directories to the dirs section
			foreach( TOCInfo Entry in TableOfContents.TOCFileEntries )
			{
				if( Entry.Type == UnrealDVDLayout.ObjectType.Directory )
				{
					if( Entry.Path.ToUpper().Contains( "TROPDIR" ) )
					{
						// Redirect the trophy folder to one path up
						PS3DiscLayout.AddFolder( "PS3_GAME" + Entry.Path.ToUpper() );
					}
					else
					{
						PS3DiscLayout.AddFolder( "PS3_GAME/USRDIR" + Entry.Path.ToUpper() );
					}
				}
			}
		}

		private void UpdateTimeStamps()
		{
			// <ps3_date>2010-11-03</ps3_date>
			PS3DiscLayout.volume.ps3_date = DateTime.UtcNow.ToString( "yyyy-MM-dd" );
			// <ts_dir>2010-11-03 19:19:16</ts_dir>
			PS3DiscLayout.volume.ts_dir = DateTime.UtcNow.ToString( "yyyy-MM-dd HH:mm:ss" );
		}

		private void CreateGP3File( bool Report )
		{
			if( GP3TemplateName.Length > 0 )
			{
				// Create the gp3 file by reading in the template
				PS3DiscLayout = ReadXml<psproject>( GP3TemplateName );
				if( PS3DiscLayout.files == null )
				{
					Error( "Could not load GP3_template file .... " + GP3TemplateName );
				}
				else
				{
					// Add all the objects to layer 0
					foreach( string GroupName in TableOfContents.Groups.TOCGroupLayer0 )
					{
						AddGroupToGP3( GroupName );
					}

					foreach( string GroupName in TableOfContents.Groups.TOCGroupLayer1 )
					{
						AddGroupToGP3( GroupName );
					}

					AddFolders();
					UpdateTimeStamps();

					if( Report )
					{
						//	Log( UnrealDVDLayout.VerbosityLevel.Informative, "[REPORT] " + PS3DiscLayout.GetSummary(), Color.Blue );
						//	Log( UnrealDVDLayout.VerbosityLevel.Informative, "[PERFCOUNTER] " + Options.GameName + PS3DiscLayout.GetPerfSummary(), Color.Blue );
						//	Log( UnrealDVDLayout.VerbosityLevel.Informative, "[PERFCOUNTER] Xbox360DVDCapacity " + PS3DiscLayout.GetCapacity(), Color.Blue );
					}
				}
			}
		}

		public void SaveGP3()
		{
			if( GP3TemplateName.Length == 0 )
			{
				Error( "No GP3 template set ...." );
			}
			else
			{
				// Create the GP3 file from the template
				CreateGP3File( true );

				// Save it out
				GenericSaveFileDialog.Title = "Select GP3 file to export...";
				GenericSaveFileDialog.DefaultExt = "*.GP3";
				GenericSaveFileDialog.Filter = "GP3 files (*.GP3)|*.GP3";
				GenericSaveFileDialog.InitialDirectory = Environment.CurrentDirectory;
				if( GenericSaveFileDialog.ShowDialog() == DialogResult.OK )
				{
					Log( VerbosityLevel.Informative, "Saving ... '" + GenericSaveFileDialog.FileName + "'", Color.Blue );
					if( WriteXml<psproject>( GenericSaveFileDialog.FileName, PS3DiscLayout, "" ) )
					{
						Log( VerbosityLevel.Informative, " ... successful", Color.Blue );
					}
				}
			}
		}

		public void SaveGP3ISO()
		{
			if( GP3TemplateName.Length == 0 )
			{
				Error( "No GP3 template set ...." );
			}
			else
			{
				// Create a layout based on the latest data
				CreateGP3File( true );

				// Save it out
				GenericSaveFileDialog.Title = "Select ISO file to export...";
				GenericSaveFileDialog.DefaultExt = "*.ISO";
				GenericSaveFileDialog.Filter = "ISO files (*.ISO)|*.ISO";
				GenericSaveFileDialog.InitialDirectory = Environment.CurrentDirectory;
				if( GenericSaveFileDialog.ShowDialog() == DialogResult.OK )
				{
					// Save temp
					string TempGP3Name = Path.Combine( Environment.CurrentDirectory, Options.GameName + "Game\\Build\\PS3\\Temp.GP3" );
					if( WriteXml<psproject>( TempGP3Name, PS3DiscLayout, "" ) )
					{
						SaveGP3Image( TempGP3Name, GenericSaveFileDialog.FileName );
					}
				}
			}
		}

		public void SaveGP3Image( string GP3FileName, string ImageName )
		{
			string PS3SDKPath = Environment.GetEnvironmentVariable( "SCE_PS3_ROOT" );
			if( PS3SDKPath != null )
			{
				Log( VerbosityLevel.Informative, "Spawning ps3cmd for ... '" + ImageName + "'", Color.Blue );
				string CWD = Path.GetDirectoryName( GP3FileName );
				string FileName = Path.GetFileName( GP3FileName );
				SpawnProcess( Path.Combine( PS3SDKPath, "Tools/ps3gen/tool/ps3cmd.exe" ), CWD, "build", "--verbose", FileName, ImageName );
			}
		}
	}
}

