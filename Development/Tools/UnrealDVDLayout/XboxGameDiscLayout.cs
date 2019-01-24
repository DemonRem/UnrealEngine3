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
	public class AvatarAssetPack
	{
		[XmlAttribute]
		public string Include = "Yes";
	}

	public class Object
	{
		[XmlAttribute( "NAME" )]
		public string Name;
		[XmlAttribute( "SOURCE" )]
		public string Source;
		[XmlAttribute( "DEST" )]
		public string Dest;
		[XmlAttribute( "FILESPEC" )]
		public string Filespec;
		[XmlAttribute( "RECURSE" )]
		public string Recurse = "NO";
		[XmlAttribute( "LAYER" )]
		public string Layer;
		[XmlAttribute( "ALIGN" )]
		public int Align;

		[XmlIgnoreAttribute]
		public UnrealDVDLayout.ObjectType Type;
		[XmlIgnoreAttribute]
		public long Size;
		[XmlIgnoreAttribute]
		public long LBA;
		[XmlIgnoreAttribute]
		public long Blocks;

		public Object()
		{
		}

		// For all user objects placed on the DVD
		public Object( TOCInfo TOCEntry )
		{
			Name = TOCEntry.Group.GroupName;

			Source = "..\\..\\.." + TOCEntry.SubDirectory + "\\";
			Dest = TOCEntry.SubDirectory + "\\";
			Filespec = TOCEntry.Name;
			Layer = TOCEntry.Layer.ToString();
			Align = 1;

			Type = TOCEntry.Type;
			Size = TOCEntry.Size;
			Blocks = TOCEntry.SectorSize;
			LBA = 0;
		}

		// Try to append on new file to filespace
		public bool TryAppendNewFile( Object TestObject )
		{
			if( ( TestObject.Name == Name ) && ( TestObject.Source == Source ) && ( TestObject.Dest == Dest ) )
			{
				Filespec += ";" + TestObject.Filespec;
				return true;
			}
			return false;
		}
	}

	public class Disc
	{
		[XmlAttribute]
		public string Name;

		[XmlElement( "ADD" )]
		public List<Object> Objects = new List<Object>();

		// Current incremental LBA
		private long CurrentLayer0StartLBA = 0;
		// Current incremental LBA
		private long CurrentLayer1StartLBA = UnrealDVDLayout.XboxSectorsPerLayer;

		public Disc()
		{
		}

		public Disc( XboxGameDiscLayout Owner )
		{
			Name = "";

			CurrentLayer0StartLBA = 48 + 4096;
			CurrentLayer1StartLBA = UnrealDVDLayout.XboxSectorsPerLayer;
		}

		public long AddObject( Object Entry, int Layer )
		{
			// if this is a directory (not a file), ignore
			if( Entry.Type != UnrealDVDLayout.ObjectType.File )
			{
				return ( -1 );
			}

			bool bAppended = false;
			foreach( Object TestEntry in Objects )
			{
				if( TestEntry.TryAppendNewFile( Entry ) )
				{
					bAppended = true;
					break;
				}
			}

			if( !bAppended )
			{
				Objects.Add( Entry );
			}

			switch( Layer )
			{
			case -1:
				break;

			case 0:
				Entry.LBA = CurrentLayer0StartLBA;
				CurrentLayer0StartLBA += Entry.Blocks;
				break;

			case 1:
				Entry.LBA = CurrentLayer1StartLBA;
				CurrentLayer1StartLBA += Entry.Blocks;
				break;
			}

			if( CurrentLayer0StartLBA > UnrealDVDLayout.XboxSectorsPerLayer )
			{
				return ( -1 );
			}
			else if( CurrentLayer1StartLBA > UnrealDVDLayout.XboxSectorsPerLayer + UnrealDVDLayout.XboxSectorsPerLayer )
			{
				return ( -1 );
			}

			return ( Entry.Blocks );
		}

		public string GetSummary()
		{
			long Layer0Free = UnrealDVDLayout.XboxSectorsPerLayer - CurrentLayer0StartLBA;
			long Layer1Free = ( UnrealDVDLayout.XboxSectorsPerLayer * 2 ) - CurrentLayer1StartLBA;
			float MBFree = ( Layer0Free + Layer1Free ) / 512.0f;

			float GBUsed = ( CurrentLayer0StartLBA + ( CurrentLayer1StartLBA - UnrealDVDLayout.XboxSectorsPerLayer ) ) / ( 512.0f * 1024.0f );

			return ( GBUsed.ToString( "F2" ) + " GB used in layout (" + MBFree.ToString( "F2" ) + " MB Free)" );
		}

		public string GetPerfSummary()
		{
			long Layer0Free = UnrealDVDLayout.XboxSectorsPerLayer - CurrentLayer0StartLBA;
			long Layer1Free = ( UnrealDVDLayout.XboxSectorsPerLayer * 2 ) - CurrentLayer1StartLBA;

			long BytesUsed = ( CurrentLayer0StartLBA + ( CurrentLayer1StartLBA - UnrealDVDLayout.XboxSectorsPerLayer ) ) * 2048;

			return ( "DVDSize " + BytesUsed );
		}

		public string GetCapacity()
		{
			long Capacity = UnrealDVDLayout.XboxSectorsPerLayer * 2 * 2048;
			return ( Capacity.ToString() );
		}
	
		public string GetExtendedCapacity()
		{
			long Capacity = 4076064L * 2048L;
			return ( Capacity.ToString() );
		}	
	}

	[XmlRoot( ElementName = "LAYOUT" )]
	public class XboxGameDiscLayout
	{
		[XmlAttribute]
		public int MajorVersion = 2;
		[XmlAttribute]
		public int MinorVersion = 0;

		[XmlElement( "AVATARASSETPACK" )]
		public AvatarAssetPack AssetPack;
		[XmlElement( "DISC" )]
		public Disc Disc;

		[XmlIgnore]
		public List<string> Folders = new List<string>();

		public XboxGameDiscLayout()
		{
		}

		public XboxGameDiscLayout( string SourceFolder )
		{
			AssetPack = new AvatarAssetPack();
			Disc = new Disc( this );

			Folders.Add( "\\" );
		}

		// Add an object based on a TOC entry
		public bool AddObject( TOCInfo TOCEntry, int Layer, bool bFirstInGroup )
		{
			// do not append files that are not included
			if( Layer == -1 )
			{
				return false;
			}

			Object Entry = new Object( TOCEntry );
			long Blocks = Disc.AddObject( Entry, Layer );
			TOCEntry.Layer = Layer;
			TOCEntry.LBA = Entry.LBA;
			return ( Blocks >= 0 );
		}

		public string GetSummary()
		{
			return ( Disc.GetSummary() );
		}

		public string GetPerfSummary()
		{
			return ( Disc.GetPerfSummary() );
		}

		public string GetCapacity()
		{
			return ( Disc.GetCapacity() );
		}
		
		public string GetExtendedCapacity()
		{
			return ( Disc.GetExtendedCapacity() );
		}	
	}

	partial class UnrealDVDLayout
	{
		private void AddGroupToXGD( string GroupName )
		{
			// Get the group in order and sort the component files
			TOCGroup Group = TableOfContents.GetGroup( GroupName );
			List<TOCInfo> Entries = TableOfContents.GetEntriesInGroup( Group );
			TableOfContents.ApplySort( Entries );

			// Add to the DVD
			bool bFirstInGroup = true;
			foreach( TOCInfo Entry in Entries )
			{
				XboxDiscLayout.AddObject( Entry, Group.Layer, bFirstInGroup );
				bFirstInGroup = false;
			}
		}

		private void CreateXGDFile( bool Report )
		{
			// Create XGD file
			XboxDiscLayout = new XboxGameDiscLayout( TableOfContents.SourceFolder );

			// Work out the size required for each directory
			TableOfContents.CalcXboxDirectorySizes( Report );

			// Add all the objects to layer 0
			foreach( string GroupName in TableOfContents.Groups.TOCGroupLayer0 )
			{
				AddGroupToXGD( GroupName );
			}

			// Add all the objects to layer 1
			foreach( string GroupName in TableOfContents.Groups.TOCGroupLayer1 )
			{
				AddGroupToXGD( GroupName );
			}

			// Add all the objects to the scratch layer
			foreach( TOCGroup Group in TableOfContents.Groups.TOCGroupEntries )
			{
				if( Group.Layer == -1 )
				{
					AddGroupToXGD( Group.GroupName );
				}
			}

			if( Report )
			{
				Log( UnrealDVDLayout.VerbosityLevel.Informative, "[REPORT] " + XboxDiscLayout.GetSummary(), Color.Blue );
				Log( UnrealDVDLayout.VerbosityLevel.Informative, "[PERFCOUNTER] " + Options.GameName + XboxDiscLayout.GetPerfSummary(), Color.Blue );
				Log( UnrealDVDLayout.VerbosityLevel.Informative, "[PERFCOUNTER] Xbox360DVDCapacity " + XboxDiscLayout.GetCapacity(), Color.Blue );
				Log( UnrealDVDLayout.VerbosityLevel.Informative, "[PERFCOUNTER] Xbox360ExtendedDVDCapacity " + XboxDiscLayout.GetExtendedCapacity(), Color.Blue );
			}
		}

		public void SaveXGD()
		{
			// Create a layout based on the latest data
			CreateXGDFile( true );

			// Save it out
			GenericSaveFileDialog.Title = "Select XGD file to export...";
			GenericSaveFileDialog.DefaultExt = "*.XGD";
			GenericSaveFileDialog.Filter = "XGD files (*.XGD)|*.XGD";
			GenericSaveFileDialog.InitialDirectory = Environment.CurrentDirectory;
			if( GenericSaveFileDialog.ShowDialog() == DialogResult.OK )
			{
				Log( VerbosityLevel.Informative, "Saving ... '" + GenericSaveFileDialog.FileName + "'", Color.Blue );
				if( WriteXml<XboxGameDiscLayout>( GenericSaveFileDialog.FileName, XboxDiscLayout, "http://www.w3.org/2001/XMLSchema-instance" ) )
				{
					Log( VerbosityLevel.Informative, " ... successful", Color.Blue );
				}
			}
		}

		public void SaveXGDISO()
		{
			// Create a layout based on the latest data
			CreateXGDFile( true );

			// Save it out
			GenericSaveFileDialog.Title = "Select ISO/XSF file to export...";
			GenericSaveFileDialog.DefaultExt = "*.ISO";
			GenericSaveFileDialog.Filter = "ISO files (*.ISO)|*.ISO|XSF files (*.XSF)|*.XSF";
			GenericSaveFileDialog.InitialDirectory = Environment.CurrentDirectory;
			if( GenericSaveFileDialog.ShowDialog() == DialogResult.OK )
			{
				// Save temp
				string TempXGDName = Path.Combine( Environment.CurrentDirectory, Options.GameName + "Game\\Build\\Xbox360\\Temp.XGD" );
				if( WriteXml<XboxGameDiscLayout>( TempXGDName, XboxDiscLayout, "http://www.w3.org/2001/XMLSchema-instance" ) )
				{
					SaveXGDImage( TempXGDName, GenericSaveFileDialog.FileName );
				}
			}
		}

		public void SaveXGDImage( string XGDFileName, string ImageName )
		{
			string XEDKPath = Environment.GetEnvironmentVariable( "XEDK" );
			if( XEDKPath != null )
			{
				Log( VerbosityLevel.Informative, "Spawning xDiscBld for ... '" + ImageName + "'", Color.Blue );
				string CWD = Path.GetDirectoryName( XGDFileName );
				string FileName = Path.GetFileName( XGDFileName );
				SpawnProcess( Path.Combine( XEDKPath, "bin\\win32\\xDiscBld.exe" ), CWD, FileName, ImageName, "/F" );
			}
		}
	}
}
