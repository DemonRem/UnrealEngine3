/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;

namespace UnrealLoc
{
    public class FileEntry
    {
        private UnrealLoc Main = null;
        private LanguageInfo Lang = null;

        public string FileName { get; set; }
        public string Extension { get; set; }
        public string RelativeName { get; set; }
		public DateTime LastWriteTime { get; set; }
		public bool HasNewLocEntries { get; set; }

        private ObjectEntryHandler FileObjectEntryHandler;

        public List<ObjectEntry> GetObjectEntries()
        {
            return ( FileObjectEntryHandler.GetObjectEntries() );
        }

        public int GetObjectCount()
        {
            return ( FileObjectEntryHandler.GetObjectCount() );
        }

        public void GenerateLocObjects( FileEntry DefaultFE )
        {
            FileObjectEntryHandler.GenerateLocObjects( DefaultFE );
        }

        public void RemoveOrphans()
        {
            FileObjectEntryHandler.RemoveOrphans();
        }

        public bool CreateDirectory( string FolderName )
        {
            DirectoryInfo DirInfo = new DirectoryInfo( FolderName );
            DirInfo.Create();

            return( true );
        }

        public bool WriteLocFiles()
        {
            Main.Log( UnrealLoc.VerbosityLevel.Informative, " ... creating loc file: " + RelativeName, Color.Black );

            FileInfo LocFileInfo = new FileInfo( RelativeName );
            CreateDirectory( LocFileInfo.DirectoryName );

            if( Main.AddToSourceControl( Lang, RelativeName ) )
            {
                Lang.FilesCreated++;
				LocFileInfo = new FileInfo( RelativeName );
			}

            if( !LocFileInfo.IsReadOnly || !LocFileInfo.Exists )
            {
                StreamWriter File = new StreamWriter( RelativeName, false, System.Text.Encoding.Unicode );
                FileObjectEntryHandler.WriteLocFiles( File );
                File.Close();
                return ( true );
            }

            return ( false );
        }

        public bool WriteDiffLocFiles( string Folder )
        {
            string LocFileName = Folder + "\\" + Lang.LangID + "\\" + FileName + "." + Lang.LangID;
            Main.Log( UnrealLoc.VerbosityLevel.Informative, " ... creating loc diff file: " + LocFileName, Color.Black );
            FileInfo LocFileInfo = new FileInfo( LocFileName );
            CreateDirectory( LocFileInfo.DirectoryName );

            if( LocFileInfo.Exists )
            {
                LocFileInfo.IsReadOnly = false;
                LocFileInfo.Delete();
            }

            StreamWriter File = new StreamWriter( LocFileName, false, System.Text.Encoding.Unicode );
            FileObjectEntryHandler.WriteDiffLocFiles( File );
            File.Close();

            return ( false );
        }

        public bool ImportText( string FileName )
        {
            return( FileObjectEntryHandler.ImportText( FileName ) );
        }

        public FileEntry( UnrealLoc InMain, LanguageInfo InLang, string InName, string InExtension, string InRelativeName, DateTime InLastWriteTime )
        {
            Main = InMain;
            Lang = InLang;
            FileName = InName;
            Extension = InExtension;
            RelativeName = InRelativeName;
			LastWriteTime = InLastWriteTime;
            FileObjectEntryHandler = new ObjectEntryHandler( Main, Lang, this );
            FileObjectEntryHandler.FindObjects();
        }

        public FileEntry( UnrealLoc InMain, LanguageInfo InLang, FileEntry DefaultFE )
        {
            Main = InMain;
            Lang = InLang;
            FileName = DefaultFE.FileName;
            Extension = "." + Lang.LangID;

            RelativeName = DefaultFE.RelativeName;
            RelativeName = RelativeName.Replace( ".INT", Extension.ToUpper() );
            RelativeName = RelativeName.Replace( "\\INT\\", "\\" + Lang.LangID + "\\" );

            FileObjectEntryHandler = new ObjectEntryHandler( Main, Lang, this );
        }
    }

    public class FileEntryHandler
    {
        private UnrealLoc Main = null;
        private LanguageInfo Lang = null;
        public List<FileEntry> FileEntries;

        public FileEntryHandler( UnrealLoc InMain, LanguageInfo InLang )
        {
            Main = InMain;
            Lang = InLang;
            FileEntries = new List<FileEntry>();
        }

        public FileEntry CreateFile( FileEntry DefaultFileEntry )
        {
            FileEntry FileElement = new FileEntry( Main, Lang, DefaultFileEntry );
            FileEntries.Add( FileElement );

            Main.Log( UnrealLoc.VerbosityLevel.Informative, "Created file '" + FileElement.RelativeName + "'", Color.Blue );
            return ( FileElement );
        }

		public bool AddFile( FileEntry OldFileEntry, string LangFolder, FileInfo File )
        {
			// Remove old file entry
			if( OldFileEntry != null )
			{
				FileEntries.Remove( OldFileEntry );
			}

            // Key off root name
            string FileName = File.Name.Substring( 0, File.Name.Length - File.Extension.Length ).ToLower();

			// Check for duplicates
			if( OldFileEntry == null )
			{
				foreach( FileEntry FE in FileEntries )
				{
					if( FE.FileName == FileName )
					{
						return ( false );
					}
				}
			}

            // Remember extension, root name and full name
            string Extension = File.Extension.ToUpper();
            string RelativeName = LangFolder + "\\" + FileName + Extension;
			
            FileEntry FileElement = new FileEntry( Main, Lang, FileName, Extension, RelativeName, File.LastWriteTimeUtc );
            FileEntries.Add( FileElement );
            return ( true );
        }

        private FileEntry FileExists( string DefaultFileName )
        {
            foreach( FileEntry ExistingFE in FileEntries )
            {
                if( DefaultFileName == ExistingFE.FileName )
                {
                    return ( ExistingFE );
                }
            }

            return ( null );
        }

		private FileEntry FindFileEntry( string RelativeName )
		{
			foreach( FileEntry ExistingFE in FileEntries )
			{
				if( RelativeName.ToLower() == ExistingFE.RelativeName.ToLower() )
				{
					return ( ExistingFE );
				}
			}

			return ( null );
		}

        public List<FileEntry> GetFileEntries()
        {
            return ( FileEntries );
        }

        public int GetCount()
        {
            return( FileEntries.Count );
        }

        public bool GenerateLocFiles( LanguageInfo DefaultLangInfo )
        {
            List<FileEntry> DefaultFileEntries = DefaultLangInfo.GetFileEntries();
            foreach( FileEntry DefaultFE in DefaultFileEntries )
            {
                FileEntry LocFE = FileExists( DefaultFE.FileName );
                if( LocFE == null )
                {
                    LocFE = CreateFile( DefaultFE );
                }

                LocFE.GenerateLocObjects( DefaultFE );
            }

            return ( true );
        }

        public void RemoveOrphans()
        {
            foreach( FileEntry FE in FileEntries )
            {
                FE.RemoveOrphans();
            }
        }

        public bool WriteLocFiles()
        {
            foreach( FileEntry FE in FileEntries )
            {
                FE.WriteLocFiles();
            }

            return ( true );
        }

        public bool WriteDiffLocFiles( string Folder )
        {
            foreach( FileEntry FE in FileEntries )
            {
                if( FE.HasNewLocEntries )
                {
                    FE.WriteDiffLocFiles( Folder );
                }
            }

            return ( true );
        }

        public string GetRelativePath()
        {
            string LangFolder = "";

            if( Main.GameName == "Engine" )
            {
                LangFolder = Main.GameName + "\\Localization\\" + Lang.LangID;
            }
            else
            {
                LangFolder = Main.GameName + "Game\\Localization\\" + Lang.LangID;
            }

            return ( LangFolder );
        }

        public bool FindLocFiles()
        {
            string LangFolder = GetRelativePath();
            string ExtLangID = "." + Lang.LangID;

            DirectoryInfo DirInfo = new DirectoryInfo( LangFolder );
            if( DirInfo.Exists )
            {
                foreach( FileInfo File in DirInfo.GetFiles() )
                {
                    if( File.Extension.ToUpper() == ExtLangID )
                    {
                        AddFile( null, LangFolder, File );
                    }
                }

                Main.Log( UnrealLoc.VerbosityLevel.Informative, " ... found " + GetCount().ToString() + " files for " + Lang.LangID, Color.Black );
                return ( true );
            }

            return ( false );
        }

		public void ReloadChangedFiles()
		{
			string LangFolder = GetRelativePath();
			string ExtLangID = "." + Lang.LangID;

			DirectoryInfo DirInfo = new DirectoryInfo( LangFolder );
			if( DirInfo.Exists )
			{
				foreach( FileInfo File in DirInfo.GetFiles() )
				{
					if( File.Extension.ToUpper() == ExtLangID )
					{
						// Find FileEntry
						string RelativeName = File.FullName.Substring( Main.BranchRootFolder.Length + 1 );
						FileEntry FE = FindFileEntry( RelativeName );

						if( FE != null )
						{
							// Check timestamps
							if( FE.LastWriteTime < File.LastWriteTimeUtc )
							{
								Main.Log( UnrealLoc.VerbosityLevel.Informative, " ... updating " + RelativeName, Color.Black );
								AddFile( FE, LangFolder, File );
							}
						}
					}
				}
			}
		}

        public bool ImportText( string FileName )
        {
            string RootName = FileName.Substring( FileName.LastIndexOf( '\\' ) + 1 ).ToLower();
            RootName = RootName.Replace( "." + Lang.LangID.ToLower(), "" );
            FileEntry FE = FileExists( RootName );
            if( FE == null )
            {
                Main.Log( UnrealLoc.VerbosityLevel.Simple, " ... INT version of '" + RootName + "' not found.", Color.Red );
                return ( false );
            }

            return( FE.ImportText( FileName ) );
        }
    }
}
