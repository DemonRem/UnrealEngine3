/**
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	partial class UE3BuildTarget
	{
		void AddPrivateAssembly(string Dir, string SubDir, string Name)
		{
			PrivateAssemblyInfo NewPrivateAssembly = new PrivateAssemblyInfo();
			NewPrivateAssembly.FileItem = FileItem.GetItemByPath(Dir + Name);
			NewPrivateAssembly.bShouldCopyToOutputDirectory = true;
			NewPrivateAssembly.DestinationSubDirectory = SubDir;
			GlobalCPPEnvironment.PrivateAssemblyDependencies.Add(NewPrivateAssembly);
		}

		void SetUpWindowsEnvironment()
        {            
            GlobalCPPEnvironment.Definitions.Add("_WINDOWS=1");
			GlobalCPPEnvironment.Definitions.Add("WIN32=1");
			GlobalCPPEnvironment.Definitions.Add("_WIN32_WINNT=0x0502");
			GlobalCPPEnvironment.Definitions.Add("WINVER=0x0502");

			// If compiling as a dll, set the relevant defines
			if( BuildConfiguration.bCompileAsdll )
			{
				GlobalCPPEnvironment.Definitions.Add( "_WINDLL" );
			}

			// OpenGL ES 2.0 emulation support
			bool bUseDynamicES2RHI = false;
			if( Game.ShouldCompileES2() && !IsBuildingDedicatedServer() )
			{
				// Make sure we have access to ES2Drv code
				if (File.Exists("ES2Drv/ES2Drv.vcproj"))
				{
					bUseDynamicES2RHI = true;
					GlobalCPPEnvironment.Definitions.Add( "USE_DYNAMIC_ES2_RHI=1" );
				}
			}

			// Add the Windows public module include paths.
			GlobalCPPEnvironment.IncludePaths.Add("OnlineSubsystemPC/Inc");
			GlobalCPPEnvironment.IncludePaths.Add("WinDrv/Inc");
			GlobalCPPEnvironment.IncludePaths.Add("D3D9Drv/Inc");
			GlobalCPPEnvironment.IncludePaths.Add("D3D11Drv/Inc");
			if( bUseDynamicES2RHI )
			{
				GlobalCPPEnvironment.IncludePaths.Add( "ES2Drv/Inc" );
			}
			GlobalCPPEnvironment.IncludePaths.Add( "XAudio2/Inc" );
			GlobalCPPEnvironment.IncludePaths.Add("Launch/Inc");
			GlobalCPPEnvironment.IncludePaths.Add("../Tools/UnrealLightmass/Public");
			GlobalCPPEnvironment.IncludePaths.Add("$(CommonProgramFiles)");

			if (Platform == UnrealTargetPlatform.Win64)
			{
				GlobalCPPEnvironment.IncludePaths.Add("../External/IntelTBB-2.1/Include");
				FinalLinkEnvironment.LibraryPaths.Add("../External/IntelTBB-2.1/em64t/vc9/lib");
			}

			string DesiredOSS = Game.GetDesiredOnlineSubsystem( GlobalCPPEnvironment, Platform );

			switch( DesiredOSS )
			{
			case "PC":
				SetUpPCEnvironment();
				break;

			case "Steamworks":
				SetUpSteamworksEnvironment();
				break;

			case "GameSpy":
				SetUpGameSpyEnvironment();
				break;

			case "Live":
				SetUpWindowsLiveEnvironment();
				break;

			default:
				throw new BuildException( "Requested OnlineSubsystem{0}, but that is not supported on PC", DesiredOSS );
			}

            // Compile and link for dedicated server
            SetUpDedicatedServerEnvironment();

			// Compile and link with libPNG
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/libPNG");
			GlobalCPPEnvironment.Definitions.Add("PNG_NO_FLOATING_POINT_SUPPORTED=1");

            // Compile and link with NVAPI
            GlobalCPPEnvironment.SystemIncludePaths.Add("../External/nvapi");

			// OpenGL ES 2.0 emulation support
			if( bUseDynamicES2RHI )
			{
				GlobalCPPEnvironment.IncludePaths.Add( "../External/ES2Emulation/NVIDIA/include" );
				if( Platform == UnrealTargetPlatform.Win64 )
				{
					FinalLinkEnvironment.LibraryPaths.Add( "../External/ES2Emulation/NVIDIA/lib/x64" );
				}
				else
				{
					FinalLinkEnvironment.LibraryPaths.Add( "../External/ES2Emulation/NVIDIA/lib/x86" );
				}
				FinalLinkEnvironment.AdditionalLibraries.Add( "libGLESv2.lib" );

				FinalLinkEnvironment.DelayLoadDLLs.Add( "libGLESv2.dll" );
			}

			// Compile and link with lzo.
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/lzopro/include");
			FinalLinkEnvironment.LibraryPaths.Add("../External/lzopro/lib");
			if( Platform == UnrealTargetPlatform.Win64 )
 			{
				// @todo win64: Enable LZOPro once we have 64-bit support
 				FinalLinkEnvironment.AdditionalLibraries.Add( "lzopro_64.lib" );
 			}
			else
 			{
 				FinalLinkEnvironment.AdditionalLibraries.Add( "lzopro.lib" );
 			}

			// Compile and link with zlib.
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/zlib/inc");
			FinalLinkEnvironment.LibraryPaths.Add("../External/zlib/Lib");
			if( Platform == UnrealTargetPlatform.Win64 )
			{
				FinalLinkEnvironment.AdditionalLibraries.Add( "zlib_64.lib" );
			}
			else
			{
				FinalLinkEnvironment.AdditionalLibraries.Add( "zlib.lib" );
			}

            // Compile and link with wxWidgets.
            if (UE3BuildConfiguration.bCompileWxWidgets)
            {
                SetupWxWindowsEnvironment();
            }

            // Explicitly exclude the MS C++ runtime libraries we're not using, to ensure other libraries we link with use the same
			// runtime library as the engine.
			if (Configuration == UnrealTargetConfiguration.Debug)
			{
				FinalLinkEnvironment.ExcludedLibraries.Add("MSVCRT");
				FinalLinkEnvironment.ExcludedLibraries.Add("MSVCPRT");
			}
			else
			{
				FinalLinkEnvironment.ExcludedLibraries.Add("MSVCRTD");
				FinalLinkEnvironment.ExcludedLibraries.Add("MSVCPRTD");
			}
			FinalLinkEnvironment.ExcludedLibraries.Add("LIBC");
			FinalLinkEnvironment.ExcludedLibraries.Add("LIBCMT");
			FinalLinkEnvironment.ExcludedLibraries.Add("LIBCPMT");
			FinalLinkEnvironment.ExcludedLibraries.Add("LIBCP");
			FinalLinkEnvironment.ExcludedLibraries.Add("LIBCD");
			FinalLinkEnvironment.ExcludedLibraries.Add("LIBCMTD");
			FinalLinkEnvironment.ExcludedLibraries.Add("LIBCPMTD");
			FinalLinkEnvironment.ExcludedLibraries.Add("LIBCPD");

			// Add the library used for the delayed loading of DLLs.
			FinalLinkEnvironment.AdditionalLibraries.Add( "delayimp.lib" );

			// Compile and link with FCollada.
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/FCollada-3.05B/FCollada/LibXML/include");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/FCollada-3.05B/FCollada");
			if ( Configuration == UnrealTargetConfiguration.Debug )
			{
				FinalLinkEnvironment.LibraryPaths.Add( "../External/FCollada-3.05B/lib/debug" );
			}
			else
			{
				FinalLinkEnvironment.LibraryPaths.Add( "../External/FCollada-3.05B/lib/release" );
			}
			
			if( Platform == UnrealTargetPlatform.Win64 )
			{
				FinalLinkEnvironment.AdditionalLibraries.Add( "FColladaU_64.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "FArchiveXML_64.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "LibXML_64.lib" );
			}
			else
			{
				FinalLinkEnvironment.AdditionalLibraries.Add( "FColladaU.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "FArchiveXML.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "LibXML.lib" );
			}


            if (UE3BuildConfiguration.bCompileFBX)
            {
                // Compile and link with FBX.
                GlobalCPPEnvironment.Definitions.Add("WITH_FBX=1");

                GlobalCPPEnvironment.SystemIncludePaths.Add("../External/FBX/2011.3.1/include");
                GlobalCPPEnvironment.SystemIncludePaths.Add("../External/FBX/2011.3.1/include/fbxfilesdk");

                FinalLinkEnvironment.LibraryPaths.Add("../External/FBX/2011.3.1/lib");
                if (Platform == UnrealTargetPlatform.Win64)
                {
                    if (Configuration == UnrealTargetConfiguration.Debug)
                    {
                        FinalLinkEnvironment.AdditionalLibraries.Add("fbxsdk_md2008_amd64d.lib");
                    }
                    else
                    {
                        FinalLinkEnvironment.AdditionalLibraries.Add("fbxsdk_md2008_amd64.lib");
                    }
                }
                else
                {
                    if (Configuration == UnrealTargetConfiguration.Debug)
                    {
                        FinalLinkEnvironment.AdditionalLibraries.Add("fbxsdk_md2008d.lib");
                    }
                    else
                    {
                        FinalLinkEnvironment.AdditionalLibraries.Add("fbxsdk_md2008.lib");
                    }
                }
            }
            else
            {
                GlobalCPPEnvironment.Definitions.Add("WITH_FBX=0");
            }

			// EasyHook is used for different methods in 32 or 64 bit Windows, but is needed for both
			{
				GlobalCPPEnvironment.SystemIncludePaths.Add( "../External/EasyHook" );
				FinalLinkEnvironment.LibraryPaths.Add( "../External/EasyHook/" );
			}

			// Compile and link with DirectX.
			GlobalCPPEnvironment.SystemIncludePaths.Add( "$(DXSDK_DIR)/include" );
			if( Platform == UnrealTargetPlatform.Win64 )
			{
				FinalLinkEnvironment.LibraryPaths.Add( "$(DXSDK_DIR)/Lib/x64" );
			}
			else
			{
				FinalLinkEnvironment.LibraryPaths.Add( "$(DXSDK_DIR)/Lib/x86" );
			}

			// Link against DirectX
			FinalLinkEnvironment.AdditionalLibraries.Add("dinput8.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("dxguid.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("XInput.lib");

			// link against wininet (used by FBX and Facebook)
			FinalLinkEnvironment.AdditionalLibraries.Add("wininet.lib");

			if (!IsBuildingDedicatedServer())
            {
                FinalLinkEnvironment.AdditionalLibraries.Add("d3d11.lib");
                FinalLinkEnvironment.DelayLoadDLLs.Add("d3d11.dll");
                FinalLinkEnvironment.AdditionalLibraries.Add("dxgi.lib");
                FinalLinkEnvironment.AdditionalLibraries.Add("d3d9.lib");
				FinalLinkEnvironment.DelayLoadDLLs.Add("dxgi.dll");
			}

			// Link against D3DX
			if( !UE3BuildConfiguration.bCompileLeanAndMeanUE3 )
			{
				FinalLinkEnvironment.AdditionalLibraries.Add("d3dcompiler.lib");
				if (Configuration == UnrealTargetConfiguration.Debug)
				{
					FinalLinkEnvironment.AdditionalLibraries.Add("d3dx9d.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("d3dx11d.lib");
				}
				else
				{
					FinalLinkEnvironment.AdditionalLibraries.Add("d3dx9.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("d3dx11.lib");
				}
			}

            // Required for 3D spatialisation in XAudio2
            FinalLinkEnvironment.AdditionalLibraries.Add( "X3DAudio.lib" );
            FinalLinkEnvironment.AdditionalLibraries.Add( "xapobase.lib" );
            FinalLinkEnvironment.AdditionalLibraries.Add( "XAPOFX.lib" );

            if (!IsBuildingDedicatedServer() && !UE3BuildConfiguration.bCompileLeanAndMeanUE3)
			{
				// Compile and link with NVIDIA Texture Tools.
				GlobalCPPEnvironment.SystemIncludePaths.Add("../External/nvTextureTools-2.0.6/src/src");
				FinalLinkEnvironment.LibraryPaths.Add("../External/nvTextureTools-2.0.6/lib");
				if( Platform == UnrealTargetPlatform.Win64 )
				{
					FinalLinkEnvironment.AdditionalLibraries.Add( "nvtt_64.lib" );
				}
				else
				{
					FinalLinkEnvironment.AdditionalLibraries.Add( "nvtt.lib" );
				}

				// Compile and link with NVIDIA triangle strip generator.
				FinalLinkEnvironment.LibraryPaths.Add("../External/nvTriStrip/Lib");
				if (Configuration == UnrealTargetConfiguration.Debug)
				{
					if( Platform == UnrealTargetPlatform.Win64 )
					{
						FinalLinkEnvironment.AdditionalLibraries.Add( "nvTriStripD_64.lib" );
					}
					else
					{
						FinalLinkEnvironment.AdditionalLibraries.Add( "nvTriStripD.lib" );
					}
				}
				else
				{
					if( Platform == UnrealTargetPlatform.Win64 )
					{
						FinalLinkEnvironment.AdditionalLibraries.Add( "nvTriStrip_64.lib" );
					}
					else
					{
						FinalLinkEnvironment.AdditionalLibraries.Add( "nvTriStrip.lib" );
					}
				}
			}

            // Compile and link with AMD Tootle.
            if (!IsBuildingDedicatedServer() && !UE3BuildConfiguration.bCompileLeanAndMeanUE3)
            {
                GlobalCPPEnvironment.SystemIncludePaths.Add("../External/tootle-2.2.150/include");
                FinalLinkEnvironment.LibraryPaths.Add("../External/tootle-2.2.150/lib");
                if (Platform == UnrealTargetPlatform.Win64)
                {
                    if (Configuration == UnrealTargetConfiguration.Debug)
                    {
                        FinalLinkEnvironment.AdditionalLibraries.Add("TootleStatic_2k8_MTDLL_d64.lib");
                    }
                    else
                    {
                        FinalLinkEnvironment.AdditionalLibraries.Add("TootleStatic_2k8_MTDLL64.lib");
                    }
                }
                else
                {
                    if (Configuration == UnrealTargetConfiguration.Debug)
                    {
                        FinalLinkEnvironment.AdditionalLibraries.Add("TootleStatic_2k8_MTDLL_d.lib");
                    }
                    else
                    {
                        FinalLinkEnvironment.AdditionalLibraries.Add("TootleStatic_2k8_MTDLL.lib");
                    }
                }
            }

            // Compile and link with libffi only if we are building UDK.
			if (GlobalCPPEnvironment.Definitions.Contains("UDK=1") && !UE3BuildConfiguration.bCompileLeanAndMeanUE3)
            {
                GlobalCPPEnvironment.SystemIncludePaths.Add("../External/libffi");
                FinalLinkEnvironment.AdditionalLibraries.Add("LibFFI.lib");
                if (Platform == UnrealTargetPlatform.Win64)
                {
                    if (Configuration == UnrealTargetConfiguration.Debug)
                    {
                        FinalLinkEnvironment.LibraryPaths.Add("../External/libffi/lib/x64/Debug");
                    }
                    else
                    {
                        FinalLinkEnvironment.LibraryPaths.Add("../External/libffi/lib/x64/Release");
                    }
                }
                else
                {
                    if (Configuration == UnrealTargetConfiguration.Debug)
                    {
                        FinalLinkEnvironment.LibraryPaths.Add("../External/libffi/lib/Win32/Debug");
                    }
                    else
                    {
                        FinalLinkEnvironment.LibraryPaths.Add("../External/libffi/lib/Win32/Release");
                    }
                }
                GlobalCPPEnvironment.Definitions.Add("WITH_LIBFFI=1");
            }

            // Compile and link with kissFFT
            if (!IsBuildingDedicatedServer() && !UE3BuildConfiguration.bCompileLeanAndMeanUE3)
            {
                GlobalCPPEnvironment.SystemIncludePaths.Add("../External/kiss_fft129");
                FinalLinkEnvironment.AdditionalLibraries.Add("KissFFT.lib");
                if (Platform == UnrealTargetPlatform.Win64)
                {
                    if (Configuration == UnrealTargetConfiguration.Debug)
                    {
                        FinalLinkEnvironment.LibraryPaths.Add("../External/kiss_fft129/lib/x64/Debug");
                    }
                    else
                    {
                        FinalLinkEnvironment.LibraryPaths.Add("../External/kiss_fft129/lib/x64/Release");
                    }
                }
                else
                {
                    if (Configuration == UnrealTargetConfiguration.Debug)
                    {
                        FinalLinkEnvironment.LibraryPaths.Add("../External/kiss_fft129/lib/Win32/Debug");
                    }
                    else
                    {
                        FinalLinkEnvironment.LibraryPaths.Add("../External/kiss_fft129/lib/Win32/Release");
                    }
                }
                GlobalCPPEnvironment.Definitions.Add("WITH_KISSFFT=1");
            }

			// Compile and link with Ogg/Vorbis.
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/libogg-1.1.3/include");
			GlobalCPPEnvironment.SystemIncludePaths.Add("../External/libvorbis-1.1.2/include");

			if( Platform == UnrealTargetPlatform.Win64 )
			{
				FinalLinkEnvironment.LibraryPaths.Add( "../External/libvorbis-1.1.2/win32/x64/Release" );
				FinalLinkEnvironment.LibraryPaths.Add( "../External/libogg-1.1.3/win32/x64/Release" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "vorbis_64.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "vorbisenc_64.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "vorbisfile_64.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "ogg_64.lib" );
			}
			else
			{
				FinalLinkEnvironment.LibraryPaths.Add( "../External/libvorbis-1.1.2/win32/Release" );
				FinalLinkEnvironment.LibraryPaths.Add( "../External/libogg-1.1.3/win32/Release" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "vorbis.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "vorbisenc.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "vorbisfile.lib" );
				FinalLinkEnvironment.AdditionalLibraries.Add( "ogg.lib" );
			}

            if (Platform == UnrealTargetPlatform.Win64)
            {
                FinalLinkEnvironment.AdditionalLibraries.Add("nvapi64.lib");
            }
            else
            {
                FinalLinkEnvironment.AdditionalLibraries.Add("nvapi.lib");
            }

			// Compile and link with Win32 API libraries.
			FinalLinkEnvironment.AdditionalLibraries.Add("rpcrt4.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("wsock32.lib");

			FinalLinkEnvironment.AdditionalLibraries.Add("dbghelp.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("comctl32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("Winmm.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("kernel32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("user32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("gdi32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("winspool.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("comdlg32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("advapi32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("shell32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("ole32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("oleaut32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("uuid.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("odbc32.lib");
			FinalLinkEnvironment.AdditionalLibraries.Add("odbccp32.lib");

			// Set a definition if the Xbox SDK is installed and support is enabled
			if( ( Xbox360SupportEnabled() ) &&
				( Environment.GetEnvironmentVariable("XEDK") != null ) )
			{
				// Compile and link with the Xbox 360 SDK Win32 API.
				GlobalCPPEnvironment.Definitions.Add("XDKINSTALLED=1");
				GlobalCPPEnvironment.SystemIncludePaths.Add("$(XEDK)/include/win32/");

				if( Platform == UnrealTargetPlatform.Win64 )
				{
					FinalLinkEnvironment.LibraryPaths.Add( "$(XEDK)/lib/x64/vs2008" );
				}
				else
				{
					FinalLinkEnvironment.LibraryPaths.Add( "$(XEDK)/lib/win32/vs2008" );
				}
			}
			else
			{
				GlobalCPPEnvironment.Definitions.Add("XDKINSTALLED=0");
			}

            // Link with GFx
            if (!GlobalCPPEnvironment.Definitions.Contains("WITH_GFx=0"))
            {
                UnrealTargetConfiguration ForcedConfiguration = Configuration;
                if (UE3BuildConfiguration.bForceScaleformRelease && Configuration == UnrealTargetConfiguration.Debug)
                {
                    ForcedConfiguration = UnrealTargetConfiguration.Release;
                }
            	switch(ForcedConfiguration)
			    {
				    case UnrealTargetConfiguration.Debug:
                        if (Platform == UnrealTargetPlatform.Win64)
                        {
                            FinalLinkEnvironment.LibraryPaths.Add(GFxDir + "/Lib/x64/Msvc90/Debug");
                        }
                        else
                        {
                            FinalLinkEnvironment.LibraryPaths.Add(GFxDir + "/Lib/Win32/Msvc90/Debug");
                        }
                        break;

					case UnrealTargetConfiguration.Shipping:
						if( Platform == UnrealTargetPlatform.Win64 )
						{
							FinalLinkEnvironment.LibraryPaths.Add( GFxDir + "/Lib/x64/Msvc90/Shipping" );
						}
						else
						{
							FinalLinkEnvironment.LibraryPaths.Add( GFxDir + "/Lib/Win32/Msvc90/Shipping" );
						}
						break;

                    default:
                        if (Platform == UnrealTargetPlatform.Win64)
                        {
                            FinalLinkEnvironment.LibraryPaths.Add(GFxDir + "/Lib/x64/Msvc90/Release");
                        }
                        else
                        {
                            FinalLinkEnvironment.LibraryPaths.Add(GFxDir + "/Lib/Win32/Msvc90/Release");
                        }
                        break;
                }

				// For UDK 
				if (ForcedConfiguration == UnrealTargetConfiguration.Shipping &&
					 GlobalCPPEnvironment.Definitions.Contains("UDK=1"))
				{
					FinalLinkEnvironment.AdditionalLibraries.Add("libgfx_amp.lib");
					GlobalCPPEnvironment.Definitions.Add("WITH_AMP=1");
					if (!GlobalCPPEnvironment.Definitions.Contains("WITH_GFx_IME=0"))
					{
						FinalLinkEnvironment.AdditionalLibraries.Add("libgfx_ime_amp.lib");
					}
					// ws2_32.lib is needed by amp
					FinalLinkEnvironment.AdditionalLibraries.Add("ws2_32.lib");
				}
				else
				{
					FinalLinkEnvironment.AdditionalLibraries.Add("libgfx.lib");
					if (!GlobalCPPEnvironment.Definitions.Contains("WITH_GFx_IME=0"))
					{
						FinalLinkEnvironment.AdditionalLibraries.Add("libgfx_ime.lib");
					}
				}

            }

			// Add the Win32-specific projects.
            if (IsBuildingDedicatedServer())
            {
            	NonGameProjects.Add(new UE3ProjectDesc("WinDrv/WinDrv.vcproj"));
            	NonGameProjects.Add(new UE3ProjectDesc("XAudio2/XAudio2.vcproj"));
            }
            else
            {
                NonGameProjects.Add(new UE3ProjectDesc("D3D11Drv/D3D11Drv.vcproj"));
				NonGameProjects.Add( new UE3ProjectDesc( "D3D9Drv/D3D9Drv.vcproj" ) );
				if( bUseDynamicES2RHI )
				{
					NonGameProjects.Add( new UE3ProjectDesc( "ES2Drv/ES2Drv.vcproj" ) );
				}
				NonGameProjects.Add( new UE3ProjectDesc( "WinDrv/WinDrv.vcproj" ) );
            	NonGameProjects.Add( new UE3ProjectDesc( "XAudio2/XAudio2.vcproj" ) );
            }
           
            // Add library paths for libraries included via pragma comment(lib)
			if( Platform == UnrealTargetPlatform.Win64 )
			{
                FinalLinkEnvironment.LibraryPaths.Add("../External/AgPerfMon/lib/win64");
                FinalLinkEnvironment.LibraryPaths.Add("../External/Bink/lib/x64/");
			}
			else
			{
                FinalLinkEnvironment.LibraryPaths.Add("../External/AgPerfMon/lib/win32");
                FinalLinkEnvironment.LibraryPaths.Add("../External/Bink/lib/Win32/");
			}
			FinalLinkEnvironment.LibraryPaths.Add("../External/GamersSDK/4.2.1/lib/Win32/");

            SetUpSpeedTreeEnvironment();

			FinalLinkEnvironment.LibraryPaths.Add("../External/DECtalk464/lib/Win32");
			if( Platform == UnrealTargetPlatform.Win64 )
			{
				FinalLinkEnvironment.LibraryPaths.Add( "../External/FaceFX/FxSDK/lib/x64/vs9/" );
				FinalLinkEnvironment.LibraryPaths.Add( "../External/FaceFX/FxCG/lib/x64/vs9/" );
				FinalLinkEnvironment.LibraryPaths.Add( "../External/FaceFX/FxAnalysis/lib/x64/vs9/" );
                FinalLinkEnvironment.LibraryPaths.Add("../External/FaceFX/Studio/External/libresample-0.1.3/lib/x64/vs9");
			}
			else
			{
				FinalLinkEnvironment.LibraryPaths.Add( "../External/FaceFX/FxSDK/lib/win32/vs9/" );
				FinalLinkEnvironment.LibraryPaths.Add( "../External/FaceFX/FxCG/lib/win32/vs9/" );
				FinalLinkEnvironment.LibraryPaths.Add( "../External/FaceFX/FxAnalysis/lib/win32/vs9/" );
                FinalLinkEnvironment.LibraryPaths.Add("../External/FaceFX/Studio/External/libresample-0.1.3/lib/win32/vs9");
			}
			
			if (Platform == UnrealTargetPlatform.Win64)
			{
				FinalLinkEnvironment.LibraryPaths.Add("../External/PhysX/SDKs/lib/win64/");
				FinalLinkEnvironment.LibraryPaths.Add("../External/PhysX/Nxd/lib/win64/");
				FinalLinkEnvironment.LibraryPaths.Add("../External/PhysX/SDKs/TetraMaker/lib/win64/");
                FinalLinkEnvironment.LibraryPaths.Add("../External/PhysX/APEX/lib/win64-PhysX_2.8.4/");
			}
			else
			{
				FinalLinkEnvironment.LibraryPaths.Add("../External/PhysX/SDKs/lib/win32/");
				FinalLinkEnvironment.LibraryPaths.Add("../External/PhysX/Nxd/lib/win32/");
				FinalLinkEnvironment.LibraryPaths.Add("../External/PhysX/SDKs/TetraMaker/lib/win32/");
                FinalLinkEnvironment.LibraryPaths.Add("../External/PhysX/APEX/lib/win32-PhysX_2.8.4/");
			}
			FinalLinkEnvironment.LibraryPaths.Add("../External/libPNG/lib/");
			FinalLinkEnvironment.LibraryPaths.Add("../External/nvDXT/Lib/");
			FinalLinkEnvironment.LibraryPaths.Add("../External/ConvexDecomposition/Lib/");
			if (Configuration == UnrealTargetConfiguration.Debug)
			{
				FinalLinkEnvironment.LibraryPaths.Add("../External/TestTrack/Lib/Debug/");
			}
			else
			{
				FinalLinkEnvironment.LibraryPaths.Add("../External/TestTrack/Lib/Release/");
			}

			if ( UE3BuildConfiguration.bBuildEditor )
            {
                NonGameProjects.Add( new UE3ProjectDesc( "GFxUIEditor/GFxUIEditor.vcproj" ) );
                GlobalCPPEnvironment.IncludePaths.Add("GFxUIEditor/Inc");
			}
			
			// For 64-bit builds, we'll forcibly ignore a linker warning with DirectInput.  This is
			// Microsoft's recommended solution as they don't have a fixed .lib for us.
			if( Platform == UnrealTargetPlatform.Win64 )
			{
				FinalLinkEnvironment.AdditionalArguments += " /ignore:4078";
			}

            FinalLinkEnvironment.LibraryPaths.Add("../External/nvapi/");

			// Setup CLR environment
			if( UE3BuildConfiguration.bAllowManagedCode )
			{
				// Set a global C++ definition so that any CLR-based features can turn themselves on
				GlobalCPPEnvironment.Definitions.Add( "WITH_MANAGED_CODE=1" );

				// Setup Editor DLL file path
				String EditorDLLFileName = "UnrealEdCSharp.dll";
				String EditorDLLSubDirectory = "Editor\\";
				if (Configuration == UnrealTargetConfiguration.Debug)
				{
					EditorDLLSubDirectory += "Debug\\";
				}
				else
				{
					EditorDLLSubDirectory += "Release\\";
				}
				String EditorDLLDirectory = Path.GetDirectoryName(OutputPath) + "\\..\\" + EditorDLLSubDirectory;
				String EditorDLLAssemblyPath = EditorDLLDirectory + "\\" + EditorDLLFileName;

				// Add C# projects
				NonGameProjects.Add( new UE3ProjectDesc( "UnrealEdCSharp/UnrealEdCSharp.csproj", EditorDLLAssemblyPath) );

				// Add C++/CLI projects
				NonGameProjects.Add( new UE3ProjectDesc( "UnrealEdCLR/UnrealEdCLR.vcproj", CPPCLRMode.CLREnabled ) );
				NonGameProjects.Add( new UE3ProjectDesc( "UnrealSwarm/SwarmInterfaceMake.vcproj", CPPCLRMode.CLREnabled ) );

				// Add required .NET Framework assemblies
				{
					String DotNet30AssemblyPath =
						Environment.GetFolderPath( Environment.SpecialFolder.ProgramFiles ) +
						"/Reference Assemblies/Microsoft/Framework/v3.0";
					String DotNet35AssemblyPath =
						Environment.GetFolderPath( Environment.SpecialFolder.ProgramFiles ) +
						"/Reference Assemblies/Microsoft/Framework/v3.5";

					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( "System.dll" );
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( "System.Data.dll" );
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( "System.Drawing.dll" );
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( "System.Xml.dll" );
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( "System.Management.dll" );
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( "System.Windows.Forms.dll" );

					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( DotNet30AssemblyPath + "/PresentationCore.dll" );
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( DotNet30AssemblyPath + "/PresentationFramework.dll" );
					GlobalCPPEnvironment.FrameworkAssemblyDependencies.Add( DotNet30AssemblyPath + "/WindowsBase.dll" );
				}

				// Add private assembly dependencies.  If any of these are changed, then CLR projects will
				// be forced to rebuild.
				{
					// The editor needs to be able to link against it's own .NET assembly dlls/exes.  For example,
					// this is needed so that we can reference the "UnrealEdCSharp" assembly.  At runtime, the
					// .NET loader will locate these using the settings specified in the target's app.config file					
					AddPrivateAssembly(EditorDLLDirectory, EditorDLLSubDirectory, EditorDLLFileName);

					// Add reference to AgentInterface
					string AgentDLLDirectory = Path.GetDirectoryName(OutputPath) + "\\..\\";
					AddPrivateAssembly(AgentDLLDirectory, "", "AgentInterface.dll");
				}


				// Use of WPF in managed projects requires single-threaded apartment model for CLR threads
				FinalLinkEnvironment.AdditionalArguments += " /CLRTHREADATTRIBUTE:STA";
			}
		}

        void SetUpDedicatedServerEnvironment()
        {
            // Check for dedicated server being requested
            if (IsBuildingDedicatedServer())
            {           
                // Add define
                GlobalCPPEnvironment.Definitions.Add("DEDICATED_SERVER=1");
                // Turn on NULL_RHI
                GlobalCPPEnvironment.Definitions.Add("USE_NULL_RHI=1");
                // Turn off TTS
                GlobalCPPEnvironment.Definitions.Add("WITH_TTS=0");
                // Turn off Speech Recognition
                GlobalCPPEnvironment.Definitions.Add("WITH_SPEECH_RECOGNITION=0");
            }
            else
            {
                // Add define
                GlobalCPPEnvironment.Definitions.Add("DEDICATED_SERVER=0");
            }
        }

        void SetupWxWindowsEnvironment()
        {
            // Compile and link with wxWidgets.
            GlobalCPPEnvironment.IncludePaths.Add("../External/wxWidgets/include");
            GlobalCPPEnvironment.IncludePaths.Add("../External/wxWidgets/lib/vc_dll");
            GlobalCPPEnvironment.IncludePaths.Add("../External/wxExtended/wxDockit/include");

            GlobalCPPEnvironment.Definitions.Add("WXUSINGDLL");
            GlobalCPPEnvironment.Definitions.Add("wxUSE_NO_MANIFEST");
            if (Platform == UnrealTargetPlatform.Win64)
            {
                FinalLinkEnvironment.LibraryPaths.Add("../External/wxWidgets/lib/vc_dll/x64");
                if (Configuration == UnrealTargetConfiguration.Debug)
                {
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_core_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_aui_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_xrc_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_richtext_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_qa_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_media_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_html_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_adv_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_net_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_xml_64.lib");
                }
                else
                {
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_core_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_aui_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_xrc_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_richtext_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_qa_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_media_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_html_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_adv_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_net_64.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_xml_64.lib");
                }
            }
            else
            {
                FinalLinkEnvironment.LibraryPaths.Add("../External/wxWidgets/lib/vc_dll/win32");
                if (Configuration == UnrealTargetConfiguration.Debug)
                {
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_core.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_aui.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_xrc.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_richtext.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_qa.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_media.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_html.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_adv.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_net.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28ud_xml.lib");
                }
                else
                {
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_core.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_aui.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_xrc.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_richtext.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_qa.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_media.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_html.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_adv.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_net.lib");
                    FinalLinkEnvironment.AdditionalLibraries.Add("wxmsw28u_xml.lib");
                }
            }
        }

		/** Creates an action to copy the specified file */
		void CreateFileCopyAction( FileItem SourceFile, FileItem DestinationFile )
		{
			Action CopyFileAction = new Action();

			// Specify the source file (prerequisite) for the action
			CopyFileAction.PrerequisiteItems.Add( SourceFile );

			// Setup the "copy file" action
			CopyFileAction.WorkingDirectory = Path.GetFullPath( "." );
			CopyFileAction.StatusDescription = string.Format( "- Copying '{0}'...", Path.GetFileName( SourceFile.AbsolutePath ) );
			CopyFileAction.bCanExecuteRemotely = false;

			// Setup xcopy.exe command line
			CopyFileAction.CommandPath = String.Format( "xcopy.exe" );
			CopyFileAction.CommandArguments = "";
			CopyFileAction.CommandArguments += " /Q";	// Don't display file items while copying
			CopyFileAction.CommandArguments += " /Y";	// Suppress prompts about overwriting files
			CopyFileAction.CommandArguments += " /I";	// Specifies that destination parameter is a directory, not a file
			CopyFileAction.CommandArguments +=
				String.Format( " \"{0}\" \"{1}\"",
					SourceFile.AbsolutePath, Path.GetDirectoryName( DestinationFile.AbsolutePath ) + Path.DirectorySeparatorChar );

			// We don't want to display "1 file(s) copied" text
			CopyFileAction.bShouldBlockStandardOutput = true;

			// We don't need to track the command executed for this action and doing so actually
			// causes unnecessary out-of-date prerequisites since the tracking file will almost
			// always be newer than the file being copied.
			CopyFileAction.bShouldTrackCommand = false;

			// Specify the output file
			CopyFileAction.ProducedItems.Add( DestinationFile );
		}


		/** Returns output files for any private assembly dependencies that need to be copied, including PDB files. */
		List<FileItem> GetCopiedPrivateAssemblyItems()
		{
			List<FileItem> CopiedFiles = new List<FileItem>();

			foreach( PrivateAssemblyInfo CurPrivateAssembly in GlobalCPPEnvironment.PrivateAssemblyDependencies )
			{
				if( CurPrivateAssembly.bShouldCopyToOutputDirectory )
				{
					// Setup a destination file item for the file we're copying
					string DestinationFilePath =
						Path.GetDirectoryName( OutputPath ) + "\\" +
						CurPrivateAssembly.DestinationSubDirectory +
						Path.GetFileName( CurPrivateAssembly.FileItem.AbsolutePath );
					FileItem DestinationFileItem = FileItem.GetItemByPath( DestinationFilePath );

					// Create the action to copy the file
					CreateFileCopyAction( CurPrivateAssembly.FileItem, DestinationFileItem );
					CopiedFiles.Add( DestinationFileItem );

					// Also create a file item for the PDB associated with this assembly
					string PDBFileName = Path.GetFileNameWithoutExtension( CurPrivateAssembly.FileItem.AbsolutePath ) + ".pdb";
					string PDBFilePath = Path.GetDirectoryName( CurPrivateAssembly.FileItem.AbsolutePath ) + "\\" + PDBFileName;
					if( File.Exists( PDBFilePath ) )
					{
						FileItem SourcePDBFileItem = FileItem.GetItemByPath( PDBFilePath );
						string DestinationPDBFilePath =
							Path.GetDirectoryName( OutputPath ) + "\\" +
							CurPrivateAssembly.DestinationSubDirectory +
							PDBFileName;
						FileItem DestinationPDBFileItem = FileItem.GetItemByPath( DestinationPDBFilePath );

						// Create the action to copy the PDB file
						CreateFileCopyAction( SourcePDBFileItem, DestinationPDBFileItem );
						CopiedFiles.Add( DestinationPDBFileItem );
					}
				}
			}

			return CopiedFiles;
		}

		
		List<FileItem> GetWindowsOutputItems()
		{
			// Verify that the user has specified the expected output extension.
			if( Path.GetExtension( OutputPath ).ToUpperInvariant() != ".EXE" && Path.GetExtension( OutputPath ).ToUpperInvariant() != ".DLL" )
			{
				throw new BuildException("Unexpected output extension: {0} instead of .EXE", Path.GetExtension(OutputPath));
			}

			// Put the non-executable output files (PDB, import library, etc) in the same directory as the executables
			FinalLinkEnvironment.OutputDirectory = Path.GetDirectoryName(OutputPath);

			if( AdditionalDefinitions.Contains( "USE_MALLOC_PROFILER=1" ) )
			{
				if( !OutputPath.ToLower().Contains( ".mprof.exe" ) )
				{
				OutputPath = Path.ChangeExtension( OutputPath, ".MProf.exe" );
			}
			}

			// Link the EXE file.
			FinalLinkEnvironment.OutputFilePath = OutputPath;
			FileItem EXEFile = FinalLinkEnvironment.LinkExecutable();

            // Do post build step for Windows Live if requested
            if (GlobalCPPEnvironment.Definitions.Contains("WITH_PANORAMA=1"))
            {
				// Creates a cfg file so Live can load
				string CfgFilePath = string.Format( "{0}.cfg", EXEFile.AbsolutePath );

				// If the file exists then make sure it's already writable so we can copy over it
				if( File.Exists( CfgFilePath ) )
				{
					File.SetAttributes( CfgFilePath, FileAttributes.Normal );
				}

				// Copy the live config file over our game-specific version
				string GameName = Game.GetGameName();
				string SrcFilePath = string.Format( ".\\{0}\\Live\\LiveConfig.xml", GameName );
				if( !File.Exists( SrcFilePath ) )
				{
					string GameNameBase = GameName.Replace( "Game", "Base" );
					SrcFilePath = string.Format( ".\\{0}\\Live\\LiveConfig.xml", GameNameBase );
				}

				File.Copy( SrcFilePath, CfgFilePath, true );
            }

            // Return a list of the output files.
			List<FileItem> OutputFiles = new List<FileItem>();
			OutputFiles.Add( EXEFile );
			
			// Also return any private assemblies that we copied to the output folder
			OutputFiles.AddRange( GetCopiedPrivateAssemblyItems() );

			return OutputFiles;
		}
	}
}
