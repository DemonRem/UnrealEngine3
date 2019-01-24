using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml.Serialization;

namespace PIB.FileWebServices
{
	public class GetFileListService
	{
		/// <summary>
		/// Class for parsing the UploadConfig XML request
		/// </summary>
		public class FileDesc
		{
			[XmlAttribute]
			public string Name;
			[XmlAttribute]
			public long Size;
			[XmlAttribute]
			public Guid MD5Sum;

			public FileDesc()
			{
				Name = "";
				Size = -1;
				MD5Sum = Guid.Empty;
			}

			public FileDesc( string Line )
			{
				string[] Elements = Line.Split( ' ' );
				if( Elements.Length == 5 )
				{
					try
					{
						// Extract the info from the TOC
						Size = Int64.Parse( Elements[0] );
						Name = Elements[3].Substring( "../".Length );
						MD5Sum = new Guid( Elements[4] );

						// Verify the file exists
						string FullName = Path.Combine( Properties.Settings.Default.RootFolder, Name );
						FileInfo Info = new FileInfo( FullName );
						if( !Info.Exists )
						{
							Size = -1;
						}
						else
						{
							// Verify the size is correct
							if( Info.Length != Size )
							{
								Size = -1;
							}
						}
					}
					catch
					{
						Size = -1;
					}
				}
			}

			public bool IsValid()
			{
				return ( Size > 0 );
			}
		}

		[XmlRoot( "GetFileList" )]
		public class GetFileList
		{
			[XmlElement]
			public List<FileDesc> Files = new List<FileDesc>();
		}

		[WebService( Name = "GetFileList", Version = 1, Desc = "Get a list of files." )]
		public GetFileList Execute( GetFileList Request )
		{
			if( Request == null )
			{
				Request = new GetFileList();
			}

			try
			{
				// Populate the list of files with MD5 checksums and verify the sizes
				string FullTOCFileName = Path.Combine( Properties.Settings.Default.RootFolder, Properties.Settings.Default.TOCFileName );
				StreamReader TOCReader = new StreamReader( FullTOCFileName );

				string Line = TOCReader.ReadLine();
				while( !TOCReader.EndOfStream )
				{
					FileDesc File = new FileDesc( Line );
					if( File.IsValid() )
					{
						Request.Files.Add( File );
					}
					Line = TOCReader.ReadLine();
				}

				TOCReader.Close();
			}
			catch
			{
			}

			return ( Request );
		}
	}
}