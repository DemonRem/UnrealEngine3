/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Controller
{
    public class GameConfig
    {
		public GameConfig( string InGame, string InPlatform, string InConfiguration, List<string> InDefines, bool InLocal, bool InUse64BitBinaries )
        {
            GameName = InGame + "Game";
            Platform = InPlatform;
            Configuration = InConfiguration;
            IsLocal = InLocal;
			Use64BitBinaries = InUse64BitBinaries;
			Defines = new List<string>();
			if( InDefines != null )
			{
				Defines.AddRange( InDefines );
			}
		}

        public GameConfig( int Count, GameConfig Game )
        {
            switch( Count )
            {
			case 2:
				GameName = "Couple of Games";
				break;
			
			case 3:
			case 4:
				GameName = "Some Games";
				break;

			case 5:
				GameName = "Most Games";
				break;

			case 6:
				GameName = "All Games";
				break;
			}

            Platform = Game.Platform;
            Configuration = Game.Configuration;
        }

        //                  PC                  Xenon & PS3
        // Debug            Debug               Debug
        // Release          Release             Release
        // Shipping         ReleaseShippingPC   ReleaseLTCG
        // Test				*None*              ReleaseLTCG-DebugConsole

        public string GetOldConfiguration()
        {
            string OldConfiguration = Configuration;
            switch( Configuration.ToLower() )
            {
                case "debug":
                case "release":
                default:
                    break;

                case "shipping":
                    switch( Platform.ToLower() )
                    {
                        case "pc":
                            OldConfiguration = "ReleaseShippingPC";
                            break;

                        case "xenon":
                        case "xbox360":
                        case "ps3":
                        case "sonyps3":
                            OldConfiguration = "ReleaseLTCG";
                            break;
                    }
                    break;

                case "shipping-debug":
				case "shippingdebug":
				case "test":
					switch( Platform.ToLower() )
                    {
                        case "xenon":
                        case "xbox360":
                        case "ps3":
                        case "sonyps3":
						case "ngp":
                            OldConfiguration = "ReleaseLTCG-DebugConsole";
                            break;
						case "iphone":
						case "iphonedevice":
						case "iphonesimulator":
							OldConfiguration = "ShippingDebugConsole";
							break;
                    } 
                    break;
            }

            return ( OldConfiguration );
        }

        // Used for building all patforms in all configurations using MSDev
        public string GetBuildConfiguration()
        {
            string OldConfiguration = GetOldConfiguration();

            if( Platform.ToLower() == "xenon" || Platform.ToLower() == "xbox360" )
            {
                return ( "Xe" + OldConfiguration );
            }
            else if( Platform.ToLower() == "ps3" || Platform.ToLower() == "sonyps3" )
            {
                return ( "PS3_" + OldConfiguration );
            }

            return ( OldConfiguration );
        }

        // Used for the PS3 make application
        public string GetMakeConfiguration()
        {
            string OldConfiguration = GetOldConfiguration();

            if( Platform.ToLower() == "ps3" )
            {
                if( OldConfiguration.ToLower() == "debug" )
                {
                    return ( "debug" );
                }
                else if( OldConfiguration.ToLower() == "releaseltcg" )
                {
                    return ( "final_release" );
                }
                else if( OldConfiguration.ToLower() == "releaseltcg-debugconsole" )
                {
                    return ( "final_release_debugconsole" );
                }
                else
                {
                    return ( "release" );
                }
            }

            return ( OldConfiguration );
        }

        public string GetConfiguration()
        {
            string OldConfiguration = GetOldConfiguration();

            if( Platform.ToLower() == "xenon" || Platform.ToLower() == "xbox360" )
            {
                return ( "Xe" + OldConfiguration );
            }
            else if( Platform.ToLower() == "ps3" || Platform.ToLower() == "sonyps3" )
            {
                return ( "PS3" + OldConfiguration );
            }

            return ( OldConfiguration );
        }

        // FIXME
        public string GetProjectName( string Game )
        {
            string ProjectName = Platform + "Launch-" + Game + "Game";
            return ( ProjectName );
        }

        private List<string> GetPCBinaryName( string Extension )
        {
			List<string> Configs = new List<string>();

            if( Configuration.ToLower() == "release" )
            {
				Configs.Add( "Binaries/" + GetUBTPlatform() + "/" + GameName + GetSubPlatform() + GetDefined() + Extension );
            }
            else if( Configuration.ToLower() == "debug" )
            {
				Configs.Add( "Binaries/" + GetUBTPlatform() + "/DEBUG-" + GameName + GetSubPlatform() + Extension );
            }
            else if( Configuration.ToLower() == "releaseshippingpc" || Configuration.ToLower() == "shipping" )
            {
                if( GameName.ToLower() == "udkgame" )
                {
					Configs.Add( "Binaries/" + GetUBTPlatform() + "/UDK" + Extension );
                }
				else if( GameName.ToLower() == "mobilegame" )
				{
					Configs.Add( "Binaries/" + GetUBTPlatform() + "/UDKMobile" + Extension );
				}
				else
                {
					Configs.Add( "Binaries/" + GetUBTPlatform() + "/ShippingPC-" + GameName + GetSubPlatform() + Extension );
                }
            }

            return ( Configs );
        }

        private List<string> GetXenonBinaryName( string Extension0, string Extension1 )
        {
			List<string> Configs = new List<string>();

            Configs.Add( GameName + "-" + GetConfiguration() + GetDefined() + Extension0 );
			if( Extension1.Length > 0 )
			{
				Configs.Add( GameName + "-" + GetConfiguration() + GetDefined() + Extension1 );
			}

            return ( Configs );
        }

        public List<string> GetExecutableNames()
        {
			List<string> Configs = new List<string>();

            if( Platform.ToLower() == "win32" || Platform.ToLower() == "win64" || Platform.ToLower() == "win32server" )
            {
                Configs = GetPCBinaryName( ".exe" );
            }
			else if( Platform.ToLower() == "win32dll" )
			{
				Configs = GetPCBinaryName( ".dll" );
			}
			else if( Platform.ToLower() == "pc" || Platform.ToLower() == "pcserver" )
			{
				if( Configuration.ToLower() == "release" )
				{
					Configs.Add( "Binaries/" + GameName + GetSubPlatform() + ".exe" );
				}
				else if( Configuration.ToLower() == "debug" )
				{
					Configs.Add( "Binaries/DEBUG-" + GameName + GetSubPlatform() + ".exe" );
				}
				else if( Configuration.ToLower() == "releaseshippingpc" || Configuration.ToLower() == "shipping" )
				{
					if( GameName.ToLower() == "udkgame" )
					{
						Configs.Add( "Binaries/UDK.exe" );
					}
					else if( GameName.ToLower() == "mobilegame" )
					{
						Configs.Add( "Binaries/UDKMobile.exe" );
					}
					else
					{
						Configs.Add( "Binaries/ShippingPC-" + GameName + GetSubPlatform() + ".exe" );
					}
				}
			}
			else if( Platform.ToLower() == "xenon" )
			{
				Configs = GetXenonBinaryName( ".exe", ".xex" );
			}
			else if( Platform.ToLower() == "xbox360" )
			{
				Configs = GetXenonBinaryName( ".xex", "" );
			}
			else if( Platform.ToLower() == "ps3" )
			{
				Configs.Add( "Binaries/" + GameName.ToUpper() + "-" + GetConfiguration() + ".elf" );
			}
			else if( Platform.ToLower() == "sonyps3" )
			{
				Configs.Add( "Binaries/PS3/" + GameName.ToUpper() + "-" + GetConfiguration() + GetDefined() + ".elf" );
			}
			else if( Platform.ToLower() == "ngp" )
			{
				Configs.Add( "Binaries/NGP/" + GameName.ToUpper() + "-NGP" + GetConfiguration() + GetDefined() + ".elf" );
			}
			else if( Platform.ToLower() == "android" )
 			{
				Configs.Add( "Binaries/Android/libMobileGame-AndroidRelease.so" );
			}
			else if( Platform.ToLower() == "iphonedevice" ) 
			{
				Configs.Add( "Binaries/IPhone/" + GetConfiguration() + "-iphoneos/" + GameName.ToUpper() + "/" + GameName.ToUpper() );
				Configs.Add( "Binaries/IPhone/" + GetConfiguration() + "-iphoneos/" + GameName.ToUpper() + "/" + GameName.ToUpper() + ".stub" );
			}
			else if( Platform.ToLower() == "iphonesimulator" )
			{
				Configs.Add( "Binaries/IPhone/" + GetConfiguration() + "-iphonesimulator/" + GameName.ToUpper() + "/" + GameName.ToUpper() );
				Configs.Add( "Binaries/IPhone/" + GetConfiguration() + "-iphonesimulator/" + GameName.ToUpper() + "/" + GameName.ToUpper() + ".stub" );
			}

			return ( Configs );
        }

        public string GetComName()
        {
            string ComName = "";

			string DecoratedGameName = GameName;
			if( Configuration.ToLower() == "release" )
			{
				DecoratedGameName = GameName;
			}
			else if( Configuration.ToLower() == "releaseltcg" )
			{
				DecoratedGameName = "LTCG-" + GameName;
			}
			else if( Configuration.ToLower() == "debug" )
			{
				DecoratedGameName = "DEBUG-" + GameName;
			}
			else if( Configuration.ToLower() == "releaseshippingpc" || Configuration.ToLower() == "shipping" )
			{
				if( GameName.ToLower() == "udkgame" )
				{
					DecoratedGameName = "UDK";
				}
				else if( GameName.ToLower() == "mobilegame" )
				{
					DecoratedGameName = "UDKMobile";
				}
				else
				{
					DecoratedGameName = "ShippingPC-" + GameName;
				}
			}

            if( Platform.ToLower() == "win32"
				|| Platform.ToLower() == "win32server"
				|| Platform.ToLower() == "pcconsole"
				|| Platform.ToLower() == "win64"
                || Platform.ToLower() == "win32_sm4"
				|| Platform.ToLower() == "win32_sm5"
				|| Platform.ToLower() == "xbox360"
				|| Platform.ToLower() == "sonyps3"
				|| Platform.ToLower() == "ngp"
				|| Platform.ToLower() == "iphone"
				|| Platform.ToLower() == "iphonedevice"
				|| Platform.ToLower() == "iphonesimulator"
				|| Platform.ToLower() == "android"
				)
            {
				if( Use64BitBinaries )
				{
					ComName = "Binaries/Win64/" + DecoratedGameName + ".exe";
				}
				else
				{
					ComName = "Binaries/Win32/" + DecoratedGameName + ".exe";
				}
            }
			else
			{
				ComName = "Binaries/" + DecoratedGameName + ".exe";
			}

            return ( ComName );
        }

		public string GetUDKComName( string ComName, string Branch, string EncodedFolderName, string RootFolder )
		{
			// Add the absolute base path
			string ComPathName = Path.Combine( RootFolder, Path.Combine( EncodedFolderName, Branch ) );

			ComName = Path.Combine( ComPathName, ComName );

			return ( ComName );
		}

        public List<string> GetSymbolFileNames()
        {
			List<string> Configs = new List<string>();

			if( Platform.ToLower() == "win32" || Platform.ToLower() == "win64" || Platform.ToLower() == "win32server" || Platform.ToLower() == "win32dll" )
            {
                Configs = GetPCBinaryName( ".pdb" );
            }
			else if( Platform.ToLower() == "pc" )
            {
                if( Configuration.ToLower() == "release" )
                {
                    Configs.Add( "Binaries/Lib/" + GetConfiguration() + "/" + GameName + ".pdb" );
                }
                else if( Configuration.ToLower() == "releaseltcg" )
                {
                    Configs.Add( "Binaries/Lib/" + GetConfiguration() + "/LTCG-" + GameName + ".pdb" );
                }
                else if( Configuration.ToLower() == "debug" )
                {
                    Configs.Add( "Binaries/Lib/" + GetConfiguration() + "/DEBUG-" + GameName + ".pdb" );
                }
                else if( Configuration.ToLower() == "releaseshippingpc" || Configuration.ToLower() == "shipping" )
                {
                    if( GameName.ToLower() == "udkgame" )
                    {
                        Configs.Add( "Binaries/Lib/" + GetConfiguration() + "/UDK.pdb" );
                    }
					else if( GameName.ToLower() == "mobilegame" )
					{
						Configs.Add( "Binaries/Lib/" + GetConfiguration() + "/UDKMobile.pdb" );
					}
					else
                    {
                        Configs.Add( "Binaries/Lib/" + GetConfiguration() + "/ShippingPC-" + GameName + ".pdb" );
                    }
                }
            }
            else if( Platform.ToLower() == "xenon" )
            {
                Configs.Add( "Binaries/Xenon/lib/" + GetConfiguration() + "/" + GameName + "-" + GetConfiguration() + ".pdb" );
                Configs.Add( "Binaries/Xenon/lib/" + GetConfiguration() + "/" + GameName + "-" + GetConfiguration() + ".xdb" );
            }
            else if( Platform.ToLower() == "xbox360" )
            {
                Configs = GetXenonBinaryName( ".pdb", ".xdb" );
            }
			else if (Platform.ToLower() == "ps3" || Platform.ToLower() == "sonyps3" )
            {
            }
			else if( Platform.ToLower() == "ngp" )
			{
			}
			else if( Platform.ToLower() == "android" )
 			{
				Configs.Add( "Binaries/Android/ue3app-debug.apk" );
			}
			else if( Platform.ToLower() == "iphonedevice" )
			{
				Configs.Add( "Binaries/IPhone/" + GetConfiguration() + "-iphoneos/" + GameName.ToUpper() + "/" + GameName.ToUpper() + ".app.dSYM" );
			}
			else if( Platform.ToLower() == "iphonesimulator" )
			{
				Configs.Add( "Binaries/IPhone/" + GetConfiguration() + "-iphonesimulator/" + GameName.ToUpper() + "/" + GameName.ToUpper() + ".app.dSYM" );
			}

            return ( Configs );
        }

		public List<string> GetFilesToClean()
		{
			List<string> BinaryFiles = GetExecutableNames();

			switch( Platform.ToLower() )
			{
			case "pc":
			case "win32":
			case "win64":
			case "xenon":
			case "xbox360":
				string RootName = BinaryFiles[0].Remove( BinaryFiles[0].Length - 4 );
				BinaryFiles.Add( RootName + ".exp" );
				BinaryFiles.Add( RootName + ".lib" );
				break;

			case "ps3":
			case "sonyps3":
				break;
			}

			BinaryFiles.AddRange( GetSymbolFileNames() );

			return ( BinaryFiles );
		}

		public string GetCookedFolderPlatform()
		{
			string CookedFolder = Platform;

			if( CookedFolder.ToLower() == "pc" || CookedFolder.ToLower() == "win32" || CookedFolder.ToLower() == "win64" ||
				CookedFolder.ToLower() == "pc_sm4" || CookedFolder.ToLower() == "win32_sm4" || CookedFolder.ToLower() == "win64_sm4" ||
				CookedFolder.ToLower() == "pc_sm5" || CookedFolder.ToLower() == "win32_sm5" || CookedFolder.ToLower() == "win64_sm5" )
			{
				CookedFolder = "PC";
			}
			else if( CookedFolder.ToLower() == "pcconsole" || CookedFolder.ToLower() == "win32console" || CookedFolder.ToLower() == "win64console" )
			{
				CookedFolder = "PCConsole";
			}
			else if( CookedFolder.ToLower() == "pcserver" || CookedFolder.ToLower() == "win32server" || CookedFolder.ToLower() == "win64server" )
			{
				CookedFolder = "PCServer";
			}
			else if( CookedFolder.ToLower() == "xbox360" || CookedFolder.ToLower() == "xenon" )
			{
				CookedFolder = "Xenon";
			}
			else if( CookedFolder.ToLower() == "sonyps3" || CookedFolder.ToLower() == "ps3" )
			{
				CookedFolder = "PS3";
			}

			// The following should fall through:
			//
			// iPhone
			// Android
			// WinCE

			return ( CookedFolder );
		}

		public string GetShaderPlatform()
		{
			string ShaderPlatform = Platform;
			if( ShaderPlatform.ToLower() == "pc" || ShaderPlatform.ToLower() == "win32" || ShaderPlatform.ToLower() == "win64" )
			{
				ShaderPlatform = "PC";
			}
			else if( ShaderPlatform.ToLower() == "pc_sm4" || ShaderPlatform.ToLower() == "win32_sm4" )
			{
				ShaderPlatform = "PC_SM4";
			}
			else if( ShaderPlatform.ToLower() == "pc_sm5" || ShaderPlatform.ToLower() == "win32_sm5" )
			{
				ShaderPlatform = "PC_SM5";
			}
			else if( ShaderPlatform.ToLower() == "xbox360" || ShaderPlatform.ToLower() == "xenon" )
			{
				ShaderPlatform = "Xenon";
			}
			else if( ShaderPlatform.ToLower() == "sonyps3" || ShaderPlatform.ToLower() == "ps3" )
			{
				ShaderPlatform = "PS3";
			}

			return ( ShaderPlatform );
		}

        private List<string> GetShaderNames( string ShaderType, string Extension )
        {
			List<string> ShaderNames = new List<string>();
			string ShaderName = GameName + "/Content/" + ShaderType + "ShaderCache";

			if( Platform.ToLower() == "pc" || Platform.ToLower() == "win32" || Platform.ToLower() == "win64" || Platform.ToLower() == "pcserver" || Platform.ToLower() == "pcconsole" )
            {
				ShaderNames.Add( ShaderName + "-PC-D3D-SM3" + Extension );
				// Alternate
				ShaderNames.Add( ShaderName + "-PC-D3D-SM4" + Extension );
				ShaderNames.Add( ShaderName + "-PC-D3D-SM5" + Extension );
			}
            else if( Platform.ToLower() == "pc_sm4" || Platform.ToLower() == "win32_sm4" )
            {
				ShaderNames.Add( ShaderName + "-PC-D3D-SM4" + Extension );
				// Alternate
				ShaderNames.Add( ShaderName + "-PC-D3D-SM3" + Extension );
				ShaderNames.Add( ShaderName + "-PC-D3D-SM5" + Extension );
            }
			else if( Platform.ToLower() == "pc_sm5" || Platform.ToLower() == "win32_sm5" )
			{
				ShaderNames.Add( ShaderName + "-PC-D3D-SM5" + Extension );
				// Alternate
				ShaderNames.Add( ShaderName + "-PC-D3D-SM3" + Extension );
				ShaderNames.Add( ShaderName + "-PC-D3D-SM4" + Extension );
			}
            else if( Platform.ToLower() == "xenon" || Platform.ToLower() == "xbox360" )
            {
				ShaderNames.Add( ShaderName + "-Xbox360" + Extension );
            }
            else if( Platform.ToLower() == "ps3" || Platform.ToLower() == "sonyps3" )
            {
				ShaderNames.Add( ShaderName + "-PS3" + Extension );
            }

            return ( ShaderNames );
        }

        public string GetRefShaderName()
        {
			List<string> ShaderNames = GetShaderNames( "Ref", ".upk" );
            return ( ShaderNames[0] );
        }

        public string GetLocalShaderName()
        {
			List<string> ShaderNames = GetShaderNames( "Local", ".upk" );
			return ( ShaderNames[0] );
        }

		public string[] GetLocalShaderNames()
		{
			List<string> LocalShaderCaches = GetShaderNames( "Local", ".upk" );
			return ( LocalShaderCaches.ToArray() );
		}

        public string[] GetGlobalShaderNames()
        {
			List<string> GlobalShaderCaches = GetShaderNames( "Global", ".bin" );

			return ( GlobalShaderCaches.ToArray() );
		}
        
		public string GetGADCheckpointFileName()
		{
			return ( GameName + "/Content/GameAssetDatabase.checkpoint" );
		}

		public string GetConfigFolderName()
        {
			string Folder = GameName + "/config";
            return ( Folder );
        }

        public string GetCookedFolderName()
        {
			string Folder = GameName + "/Cooked" + GetCookedFolderPlatform();
            return ( Folder );
        }

		public string[] GetDLCCookedFolderNames( string ModName )
		{
			List<string> CookedFolders = new List<string>();

			string CookedFolder = GetCookedFolderPlatform();
			CookedFolders.Add( GameName + "/DLC/" + CookedFolder + "/" + ModName + "/Cooked" + CookedFolder );
			CookedFolders.Add( GameName + "/DLC/" + CookedFolder + "/" + ModName + "/Online" );
			CookedFolders.Add( GameName + "/Build/" + CookedFolder + "/Patch/" + ModName );
			CookedFolders.Add( GameName + "/Build/" + CookedFolder + "/DLC/" + ModName + "/Cooked" + CookedFolder );

			return ( CookedFolders.ToArray() );
		}

		public string GetCookedConfigFolderName()
        {
			string Folder = GameName + "/Config/" + GetCookedFolderPlatform() + "/Cooked";
            return ( Folder );
        }

		public string GetWrangledFolderName()
		{
			string Folder = GameName + "/CutdownPackages";
			return ( Folder );
		}

		public string GetGUDsFolderName()
		{
			string Folder = GameName + "/Content/GeneratedGUDs";
			return ( Folder );
		}

		public string GetDMSFolderName()
		{
			string Folder = GameName + "/Logs/MapSummaryData";
			return ( Folder );
		}

		public string GetPatchesFolderName()
		{
			string Folder = GameName + "/Patches";
			return ( Folder );
		}

		public string GetMobileShadersFolderName()
		{
			string Folder = GameName + "/Shaders/ES2";
			return ( Folder );
		}

		public string GetOriginalCookedFolderName()
        {
			string Folder = GameName + "/CookedOriginal";
			if( Platform.ToLower() == "pcserver" )
			{
				Folder = GameName + "/CookedOriginalServer";
			}
            return ( Folder );
        }

        public string GetPatchFolderName()
        {
			string Folder = GameName + "/Build/" + Platform + "/Patch";
            return ( Folder );
        }

		public string GetDLCFolderName()
		{
			string Folder = GameName + "/Build/" + Platform + "/DLC";
			return (Folder);
		}

		public string GetHashesFileName()
		{
			string Folder = GameName + "/Build/Hashes.sha";
			return ( Folder );
		}

		public string GetDialogFileName( string ModName, string Language, string RootName )
        {
            string DialogName;

			if( ModName.Length > 0 )
			{
				if( Language == "INT" )
				{
					DialogName = GameName + "/Content/DLC/" + ModName + "/Sounds/" + Language + "/" + RootName + ".upk";
				}
				else
				{
					DialogName = GameName + "/Content/DLC/" + ModName + "/Sounds/" + Language + "/" + RootName + "_" + Language + ".upk";
				}
			}
			else
			{
				if( Language == "INT" )
				{
					DialogName = GameName + "/Content/Sounds/" + Language + "/" + RootName + ".upk";
				}
				else
				{
					DialogName = GameName + "/Content/Sounds/" + Language + "/" + RootName + "_" + Language + ".upk";
				}
			}

            return ( DialogName );
        }

        public string GetFontFileName( string Language, string RootName )
        {
            string FontName;

            if( Language == "INT" )
            {
				FontName = GameName + "/Content/" + RootName + ".upk";
            }
            else
            {
				FontName = GameName + "/Content/" + RootName + "_" + Language + ".upk";
            }
            return ( FontName );
        }

        public string GetPackageFileName( string Language, string RootName )
        {
            string PackageName;

            if( Language == "INT" )
            {
                PackageName = RootName + ".upk";
            }
            else
            {
                PackageName = RootName + "_" + Language + ".upk";
            }
            return ( PackageName );
        }

        public string GetLayoutFileName( string SkuName, string[] Languages )
        {
			string Extension = "." + Platform;
			string Folder = Platform;

			switch( Platform.ToLower() )
			{
			case "xenon":
			case "xbox360":
				Extension = ".XGD";
				Folder = "Xbox360";
				break;

			case "ps3":
			case "sonyps3":
				Extension = ".GP3";
				Folder = "PS3";
				break;

			case "pcconsole":
				Extension = ".InstallDesignerProject";
				Folder = "PCConsole";
				break;

			default:
				break;
			}

			string LayoutName = GameName + "/Build/" + Folder + "/Layout";

			if( SkuName.Length > 0 )
			{
				LayoutName += "_" + SkuName.ToUpper();
			}

			if( Languages.Length > 0 )
			{
				LayoutName += "_";
				foreach( string Lang in Languages )
				{
					char FirstLetter = Lang.ToUpper()[0];
					LayoutName += FirstLetter;
				}
			}

			LayoutName += Extension;

            return ( LayoutName );
        }

		public List<string> GetCatNames()
		{
			List<string> ExeNames = GetExecutableNames();
			List<string> CatNames = new List<string>();

			CatNames.Add( ExeNames[0] + ".cat" );
			CatNames.Add( ExeNames[0] + ".cdf" );
			return ( CatNames );
		}

		public string GetCfgName()
		{
			List<string> ExeNames = GetExecutableNames();
			return ( ExeNames[0] + ".cfg" );
		}

		public string GetContentSpec()
		{
			string ContentSpec = GameName + "/Content/....upk";
			return ( ContentSpec );
		}

        public string GetTitle()
        {
            return ( "UnrealEngine3-" + Platform );
        }

        override public string ToString()
        {
            return ( GameName + " (" + Platform + " - " + Configuration + ")" ); 
        }

        public bool Similar( GameConfig Game )
        {
            return( Game.Configuration == Configuration && Game.Platform == Platform );
        }

        public void DeleteCutdownPackages( Main Parent )
        {
            Parent.DeleteDirectory( GameName + "/CutdownPackages", 0 );
        }

		private string GetSubPlatform()
		{
			switch( Platform.ToLower() )
			{
				case "win32server":
				case "pcserver":
					return ( "Server" );
				case "win32dll":
					return ( "DLL" );
			}

			return ( "" );
		}

		private string GetDefined()
		{
			if( Defines.Contains( "USE_MALLOC_PROFILER=1" ) )
			{
				return ( ".MProf" );
			}

			return ( "" );
		}

        private string GetUBTPlatform()
        {
            switch( Platform.ToLower() )
            {
                case "pc":
				case "win32server":
				case "pcserver":
				case "win32dll":
					return ( "Win32" );
                case "xenon":
                    return ( "Xbox360" );
				case "sonyps3":
					return ( "PS3" );
            }
            
            return( Platform );
        }

        private string GetUBTConfig()
        {
            switch( Configuration.ToLower() )
            {
                case "shipping-debug":
				case "shippingdebug":
				case "test":
					return ( "ShippingDebugConsole" );
            }

            return ( Configuration );
        }

		public string GetUBTCommandLine( bool IsPrimaryBuild, bool bAllowXGE, bool bAllowPCH, bool bSpecifyOutput )
		{
			string UBTPlatform = GetUBTPlatform();
			string UBTConfig = GetUBTConfig();

			List<string> Executables = GetExecutableNames();
			// Only do this for legacy branches
			if( Platform.ToLower() == "xenon" )
			{
				// Need the xex for UBT
				Executables[0] = Executables[1];
			}

			string UBTCommandLine = UBTPlatform + " " + UBTConfig + " " + GameName + GetSubPlatform() + " -verbose -noxgemonitor";

			if( bSpecifyOutput )
			{
				UBTCommandLine += " -output ../../" + Executables[0];
			}

			if( !bAllowPCH )
			{
				UBTCommandLine += " -nopch";
			}

			if( IsPrimaryBuild )
			{
				// For primary builds, keep everything local, with all debug info
				UBTCommandLine += " -noxge";

				// For iPhone platforms, always generate the dSYM
				if( UBTPlatform.ToLower() == "iphonedevice" ||
					UBTPlatform.ToLower() == "iphonesimulator" )
				{
					UBTCommandLine += " -gendsym";
				}
			}
			else
			{
				// For non-primary builds, allow distributed builds, don't generate any debug info,
				// and don't use PCHs or PDBs
				UBTCommandLine += " -nodebuginfo -nopdb";

				// XGE defaults to enabled, only disable when instructed to do so
				if( bAllowXGE == false )
				{
					UBTCommandLine += " -noxge";
				}
			}

			return ( UBTCommandLine );
		}

        // Name of the game
        public string GameName;
        // Platform
		public string Platform;
        // Build configuration eg. release
		public string Configuration;
        // Whether the build is compiled locally
        public bool IsLocal;
		// Whether to use the x64 or x86 binaries
		public bool Use64BitBinaries;
		// The defines associated with this build
		public List<string> Defines;
	}
}
