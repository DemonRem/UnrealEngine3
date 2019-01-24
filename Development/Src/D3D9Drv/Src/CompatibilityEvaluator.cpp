/*=============================================================================
CompatibilityEvaluator.cpp:
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3D9DrvPrivate.h"
#include "HardwareID.h"
#include "VideoDevice.h"

#include <algorithm>
#include <functional>

#if !UE3_LEAN_AND_MEAN && !DEDICATED_SERVER
#include "nvtt/nvtt.h"

/** 
 * Structures required by NVTT.
 */
struct FNVDummyOutputHandler : public nvtt::OutputHandler
{
public:
	FNVDummyOutputHandler() {}
	virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) {}
	virtual bool writeData(const void * data, int size) { return true; }
};

struct FNVDummyErrorHandler : public nvtt::ErrorHandler
{
public:
	FNVDummyErrorHandler() {}

	virtual void error(nvtt::Error e)
	{
		warnf(NAME_Warning, *FString::Printf(TEXT("nvtt::compress() failed with error '%s'"), ANSI_TO_TCHAR(nvtt::errorString(e))));
	}
};

/**
 * Helper for MeasureCPUPerformance
 */
static FLOAT TimeTextureCompression()
{
	// Generate a random 256x256 bitmap
	const UINT ImageSize = 256;
	TArray<FColor> Image;
	Image.Add(ImageSize * ImageSize);
	// Relying on FRandomStream returning the exact same numbers every run.
	FRandomStream Random(ImageSize);
	for (INT i = 0; i < Image.Num(); ++i)
	{
		Image(i) = FColor(Random.GetFraction() * 255, Random.GetFraction() * 255, Random.GetFraction() * 255, Random.GetFraction() * 255);
	}

	DOUBLE StartTime = appSeconds();

	nvtt::InputOptions inputOptions;
	inputOptions.setTextureLayout(nvtt::TextureType_2D, ImageSize, ImageSize);
	inputOptions.setMipmapGeneration(false, -1);
	verify(inputOptions.setMipmapData(Image.GetData(), ImageSize, ImageSize));

	nvtt::CompressionOptions compressionOptions;
	compressionOptions.setFormat(nvtt::Format_DXT1);
	compressionOptions.setQuality(nvtt::Quality_Production);

	FNVDummyErrorHandler errorHandler;
	nvtt::Compressor compressor;
	compressor.enableCudaAcceleration(false);
	FNVDummyOutputHandler outputHandler;
	nvtt::OutputOptions outputOptions;
	outputOptions.setOutputHandler(&outputHandler);
	outputOptions.setErrorHandler(&errorHandler);
	compressor.process(inputOptions, compressionOptions, outputOptions);
	DOUBLE EndTime = appSeconds();
	return (FLOAT)((EndTime-StartTime) * 1000.0);
}
#endif //!UE3_LEAN_AND_MEAN && !DEDICATED_SERVER

/**
 * Measures the performance of the CPU using an empirical measurement
 *
 * @return the time it took to run the test in ms. -1 if there was a problem.
 */
static FLOAT MeasureCPUPerformance()
{
#if _DEBUG || UE3_LEAN_AND_MEAN || DEDICATED_SERVER
	debugf(NAME_Init, TEXT("Skipping CPU performance test in debug build."));
	return -1.f;
#else
	// warm up the CPU for a few hundred MS first
	DOUBLE BeginTime;
	DOUBLE EndTime;
	for( INT StartupIter = 0; StartupIter < 4; ++StartupIter )
	{
		BeginTime = appSeconds();
		EndTime = BeginTime;
		do
		{
			EndTime = appSeconds();
		}
		while( EndTime - BeginTime < 0.05 );
		Sleep( 0 );
	}
	return TimeTextureCompression();
#endif
}

/**
 *	Assign compatibility settings based on specified Level.
 */
UBOOL SetCompatibilityLevelWindows( FCompatibilityLevelInfo Level, UBOOL bWriteToIni )
{
	Level = Level.ClampToValidRange();
	// Texture detail gets set based on the GPU level. Everything else gets set based on the composite level.
	FSystemSettingsData NewData = GSystemSettings.GetDefaultSettings(EFriendlySettingsLevel(Level.CompositeLevel));
	(FSystemSettingsDataTextureDetail&)NewData = (FSystemSettingsDataTextureDetail&)GSystemSettings.GetDefaultSettings(EFriendlySettingsLevel(Level.GPULevel));
	GSystemSettings.ApplyNewSettings(NewData, bWriteToIni);

	return TRUE;
}

enum CompatLevel
{
	CL_Unsupported,
	CL_1,
	CL_2,
	CL_3,
	CL_4,
	CL_5
};

enum CompatLevelCategories
{
	CLC_CPU,
	CLC_CPUMem,
	CLC_GPUMem,
	CLC_GPUShader,
	CLC_GPUDevice,
	CLC_OS,
	CLC_Count
};

/**
 * Determine the default compatibility level for the machine.
 */
FCompatibilityLevelInfo GetCompatibilityLevelWindows()
{
	const TCHAR* AppCompatSection = TEXT("AppCompat");

	// Get memory.
	MEMORYSTATUSEX M;
	M.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&M);
	// give a little slack on memory reported because it is never exactly that much.
	// So round up to multiples of this amount to report more "normalized" numbers
	const UINT MBOfSlack = 32;
	const UINT AdjustedCPUMemory = (UINT(M.ullTotalPhys/1024/1024) + MBOfSlack-1) & ~(MBOfSlack-1);

	// Get OS
	OSVERSIONINFOEX OsVersionInfo;
	ZeroMemory( &OsVersionInfo, sizeof( OsVersionInfo ) );
	OsVersionInfo.dwOSVersionInfoSize = sizeof( OsVersionInfo );
	GetVersionEx( (LPOSVERSIONINFO)&OsVersionInfo );

	UBOOL bWin2K = OsVersionInfo.dwMajorVersion >= 5;
	UBOOL bWinXP = bWin2K && OsVersionInfo.dwMinorVersion >= 1;
	UBOOL bWinServer2K3 = bWin2K && OsVersionInfo.dwMinorVersion >= 2;
	UBOOL bVista = OsVersionInfo.dwMajorVersion >= 6;
	UBOOL bWindows7 = bVista && OsVersionInfo.dwMinorVersion >= 1;
	UBOOL bRemoteDesktop = GetSystemMetrics(SM_REMOTESESSION) != 0;

#if !_WIN64 //@todo win64: CPU detection
	// Get CPU Info
	HardwareID cpuInfo;
	cpuInfo.DoDeviceDetection();
#endif
	// only measure CPU perf once because it takes ~1 sec
	const TCHAR* MeasuredCPUPerformanceKey = TEXT("MeasuredCPUScore");
	FLOAT CPUPerformance = -1.f;
	UBOOL bCPUPerformanceMeasuredThisTime = FALSE;
	// These are read from EngineIni not CompatIni because we want compat checks re-done whenever EngineIni is rebuilt.
	// Essentially, we shouldn't ever write to CompatIni
	if (!GConfig->GetFloat(AppCompatSection, MeasuredCPUPerformanceKey, CPUPerformance, GEngineIni))
	{
		CPUPerformance = MeasureCPUPerformance();
		GConfig->SetFloat(AppCompatSection, MeasuredCPUPerformanceKey, CPUPerformance, GEngineIni);
		GConfig->Flush(FALSE, GEngineIni);
		bCPUPerformanceMeasuredThisTime = TRUE;
	}

	// Get GPU Info
	VideoDevice vidInfo;
	vidInfo.DoDeviceDetection();

	debugf(NAME_Init, TEXT("OS stats:"));
	debugf(NAME_Init, TEXT("\t%s %s"), 
		bWindows7 ? TEXT("Windows 7") : bVista ? TEXT("Windows Vista") : bWinServer2K3 ? TEXT("Windows Server 2003") : bWinXP ? TEXT("Windows XP") : bWin2K ? TEXT("Windows 2000") : TEXT("Unknown Windows"),
		OsVersionInfo.szCSDVersion);
	debugf(NAME_Init, TEXT("\tRemoteDesktop=%d"), bRemoteDesktop);

	debugf(NAME_Init, TEXT("Memory stats:"));
	debugf(NAME_Init, TEXT("\tPhysical: %dMB"), INT(M.ullTotalPhys/1024/1024) );
	debugf(NAME_Init, TEXT("\tVirtual: %dMB"), INT(M.ullTotalVirtual/1024/1024) );
	debugf(NAME_Init, TEXT("\tPageFile: %dMB"), INT(M.ullTotalPageFile/1024/1024) );

	debugf(NAME_Init, TEXT("CPU stats:"));
	debugf(NAME_Init, TEXT("\tMeasuredPerformanceTime: %.3f (%s)"), CPUPerformance, bCPUPerformanceMeasuredThisTime ? TEXT("measured now") : TEXT("stored result"));
#if !_WIN64 //@todo win64: CPU detection
	debugf(NAME_Init, TEXT("\tHyperthreaded: %d"), (UBOOL)cpuInfo.IsHyperThreaded());
	debugf(NAME_Init, TEXT("\tNumProcessorsPerCPU: %d"), cpuInfo.GetNumProcessorsPerCPU());
	debugf(NAME_Init, TEXT("\tNumLogicalProcessors: %d"), cpuInfo.GetNumLogicalProcessors());
	debugf(NAME_Init, TEXT("\tNumPhysicalProcessors: %d"), cpuInfo.GetNumPhysicalProcessors());
	debugf(NAME_Init, TEXT("\tMaxSpeed: %d"), cpuInfo.GetMaxSpeed());
	debugf(NAME_Init, TEXT("\tCurrentSpeed: %d"), cpuInfo.GetCurrentSpeed());
	debugf(NAME_Init, TEXT("\tCoresPerProcessor: %d"), cpuInfo.GetCoresPerProcessor());
	//debugf(NAME_Init, TEXT("\tCpuStandardInfo: %u"), cpuInfo.GetCpuStandardInfo());
	debugf(NAME_Init, TEXT("\tIsOnBattery: %d"), (UBOOL)cpuInfo.IsOnBattery());
	debugf(NAME_Init, TEXT("\tBatteryLevel: %d"), cpuInfo.BatteryLevel());
	debugf(NAME_Init, TEXT("\tManufacturer: %s"), cpuInfo.GetManufacturer());
	//debugf(NAME_Init, TEXT("\tCPUMask: %d"), cpuInfo.GetCPUMask());
	debugf(NAME_Init, TEXT("\tCPUName: %s"), cpuInfo.GetCPUName());
	//debugf(NAME_Init, TEXT("\tCPUFeatures: %s"), cpuInfo.GetCPUFeatures());
	debugf(NAME_Init, TEXT("\tL1CacheSize: %d"), cpuInfo.GetL1CacheSize());
	debugf(NAME_Init, TEXT("\tL2CacheSize: %d"), cpuInfo.GetL2CacheSize());
	debugf(NAME_Init, TEXT("\tArchitecture: %s"), cpuInfo.GetArchitecture());
#endif

	debugf(NAME_Init, TEXT("GPU stats:"));
	debugf(NAME_Init, TEXT("\tVendorID: %08X"), vidInfo.GetVendorID());
	debugf(NAME_Init, TEXT("\tDeviceID: %08X"), vidInfo.GetDeviceID());
	debugf(NAME_Init, TEXT("\tDriverVersion: %u.%u.%u.%u"), 
		vidInfo.GetDriverVersion().MajorVersion, 
		vidInfo.GetDriverVersion().MinorVersion, 
		vidInfo.GetDriverVersion().RevisionVersion, 
		vidInfo.GetDriverVersion().BuildVersion);
	debugf(NAME_Init, TEXT("\tDeviceName: %s"), vidInfo.GetDeviceName());
	debugf(NAME_Init, TEXT("\tDriverName: %s"), vidInfo.GetDriverName());
	debugf(NAME_Init, TEXT("\tPixelShaderVersion: %u"), vidInfo.GetPixelShaderVersion());
	debugf(NAME_Init, TEXT("\tVertexShaderVersion: %u"), vidInfo.GetVertexShaderVersion());
	debugf(NAME_Init, TEXT("\tVRAMQuantity: %u"), vidInfo.GetVRAMQuantity());
	debugf(NAME_Init, TEXT("\tDedicatedVRAM: %u"), vidInfo.GetDedicatedVRAM());
	debugf(NAME_Init, TEXT("\tAdapterCount: %u"), vidInfo.GetAdapterCount());
	debugf(NAME_Init, TEXT("\tSupportsHardwareTnL: %d"), (UBOOL)vidInfo.SupportsHardwareTnL());
	// WRH - 2007/08/20 - We don't have the Crossfire DLL to properly detect ATI Crossfire configs.
	// Don't display SLI/Crossfire info at all. We don't differentiate based on it right now anyway.
	//debugf(NAME_Init, TEXT("\tnVidiaSLI: %d"), (UBOOL)vidInfo.GetnVidiaSLI());
	//debugf(NAME_Init, TEXT("\tATICrossfire: %d"), (UBOOL)vidInfo.GetATICrossfire());	

	CompatLevel CompatLevels[CLC_Count] = {CL_Unsupported};

	// Determine CPU Level
#if !_WIN64 //@todo win64: CPU detection
	{
		// give bonuses for multicore and hyperthreading
		FLOAT CPUMultiCoreMult = 1.0f;
		FLOAT CPUHyperThreadMult = 1.0f;
		if (cpuInfo.GetNumPhysicalProcessors() > 1)
		{
			GConfig->GetFloat(AppCompatSection, *FString::Printf(TEXT("CPUMultiCoreMult")), CPUMultiCoreMult, GCompatIni);
			// Don't ever let it degrade our score
			CPUMultiCoreMult = Max(1.f, CPUMultiCoreMult);
		}
		else if (cpuInfo.IsHyperThreaded())
		{
			GConfig->GetFloat(AppCompatSection, *FString::Printf(TEXT("CPUHyperThreadMult")), CPUHyperThreadMult, GCompatIni);
			// Don't ever let it degrade our score
			CPUHyperThreadMult = Max(1.f, CPUHyperThreadMult);
		}

		// see if we have CPUScore values to go by. If not, use CPUSpeed heuristics (which won't take into account different processors very well)
		// also don't do any performance-based detection in debug modes.
		FLOAT Dummy=0.f;
		if (GConfig->GetFloat(AppCompatSection, TEXT("CPUScore1"), Dummy, GCompatIni) && CPUPerformance > 0.f)
		{
			// Clock speeds are reported in MHz
			FLOAT CPUScoreRanges[FSL_LevelCount] = {1000.f};
			for (UINT i=0;i<FSL_LevelCount;++i)
			{
				GConfig->GetFloat(AppCompatSection, *FString::Printf(TEXT("CPUScore%d"), i+1), CPUScoreRanges[i], GCompatIni);
			}

			FLOAT cpuSpeed = CPUPerformance;
			// ranges are inverted here because they are times.
			const FLOAT* Find = std::upper_bound(CPUScoreRanges, CPUScoreRanges+ARRAY_COUNT(CPUScoreRanges), cpuSpeed / CPUMultiCoreMult / CPUHyperThreadMult, std::greater<FLOAT>());
			CompatLevels[CLC_CPU] = (CompatLevel)(Find-CPUScoreRanges);
		}
		else
		{
			// Clock speeds are reported in MHz
			FLOAT CPUSpeedRanges[FSL_LevelCount] = {1.0f};
			for (UINT i=0;i<FSL_LevelCount;++i)
			{
				GConfig->GetFloat(AppCompatSection, *FString::Printf(TEXT("CPUSpeed%d"), i+1), CPUSpeedRanges[i], GCompatIni);
				// convert to GHz
				CPUSpeedRanges[i] *= 1024;
			}

			FLOAT cpuSpeed = cpuInfo.GetMaxSpeed() * CPUMultiCoreMult * CPUHyperThreadMult;
			const FLOAT* Find = std::upper_bound(CPUSpeedRanges, CPUSpeedRanges+ARRAY_COUNT(CPUSpeedRanges), cpuSpeed);
			CompatLevels[CLC_CPU] = (CompatLevel)(Find-CPUSpeedRanges);
		}
	}
#else
	CompatLevels[CLC_CPU] = CL_5;
#endif

	// Determine CPUMemLevel
	{
		// Memory is reported in MB
		FLOAT CPUMemRanges[FSL_LevelCount] = {1.0f};

		for (UINT i=0;i<FSL_LevelCount;++i)
		{
			GConfig->GetFloat(AppCompatSection, *FString::Printf(TEXT("CPUMemory%d"), i+1), CPUMemRanges[i], GCompatIni);
			// convert to GB
			CPUMemRanges[i] *= 1024;
		}

		const FLOAT* Find = std::upper_bound(CPUMemRanges, CPUMemRanges+ARRAY_COUNT(CPUMemRanges), (FLOAT)AdjustedCPUMemory);
		CompatLevels[CLC_CPUMem] = (CompatLevel)(Find-CPUMemRanges);
	}

	// Determine GPUMemLevel
	{
		// Memory is reported in MB
		INT GPUMemRanges[FSL_LevelCount] = {128};

		for (UINT i=0;i<FSL_LevelCount;++i)
		{
			GConfig->GetInt(AppCompatSection, *FString::Printf(TEXT("GPUMemory%d"), i+1), GPUMemRanges[i], GCompatIni);
		}

		const INT* Find = std::upper_bound(GPUMemRanges, GPUMemRanges+ARRAY_COUNT(GPUMemRanges), (INT)vidInfo.GetVRAMQuantity());
		CompatLevels[CLC_GPUMem] = (CompatLevel)(Find-GPUMemRanges);
	}

	// Determine GPUShaderLevel
	{
		// Memory is reported in MB
		INT GPUShaderRanges[FSL_LevelCount] = {2};

		for (UINT i=0;i<FSL_LevelCount;++i)
		{
			GConfig->GetInt(AppCompatSection, *FString::Printf(TEXT("GPUShader%d"), i+1), GPUShaderRanges[i], GCompatIni);
		}
		// no need to check vertex shader version. We can use software if we need to (perhaps we should adjust for that, though).
		const INT* Find = std::upper_bound(GPUShaderRanges, GPUShaderRanges+ARRAY_COUNT(GPUShaderRanges), (INT)vidInfo.GetPixelShaderVersion());
		CompatLevels[CLC_GPUShader] = (CompatLevel)(Find-GPUShaderRanges);
	}

	// Determine GPUDeviceLevel
	{
		UBOOL bCompatLevelFound = FALSE;
		UBOOL bIsMobileCard = FALSE;

		FString AppCompatSection = FString::Printf(TEXT("AppCompatGPU-0x%04X"), vidInfo.GetVendorID());
		FString VendorName;
		UBOOL IniSectionFound = GConfig->GetString(*AppCompatSection, TEXT("VendorName"), VendorName, GCompatIni);
		FString MobileTag;

		// do the generic mapping first
		if (IniSectionFound)
		{
			FString DeviceIDStr = FString::Printf(TEXT("0x%04X"), vidInfo.GetDeviceID());
			MobileTag = GConfig->GetStr(*AppCompatSection, TEXT("VendorMobileTag"), GCompatIni);
			FString DeviceInfo = GConfig->GetStr(*AppCompatSection, *DeviceIDStr, GCompatIni);

			// if we found the device, use that compat level
			if (DeviceInfo.Len() > 2)
			{
				INT DeviceCompatLevel = appAtoi(*DeviceInfo.Left(1));
				FString DeviceName = DeviceInfo.Right(DeviceInfo.Len()-2);
				debugf(NAME_Init, TEXT("\tGPU DeviceID found in ini: (%d) %s"), DeviceCompatLevel, *DeviceName);
				// make sure we read a sane value
				if (DeviceCompatLevel >= CL_Unsupported && DeviceCompatLevel <= CL_5)
				{
					bCompatLevelFound = TRUE;
					CompatLevels[ CLC_GPUDevice ] = CompatLevel(DeviceCompatLevel);
				}
			}
			else
			{
				debugf(NAME_Init, TEXT("\tGPU DeviceID not found in ini."));

			}

			// detect mobile cards
			if (MobileTag.Len() > 0)
			{
				bIsMobileCard = appStristr(vidInfo.GetDeviceName(), *MobileTag) != NULL;
			}
		}

		// if the GPU was not found or not tested, use a generic means to bucket it
		if (!bCompatLevelFound)
		{
			// shader model 3 cards can get up to 4
			// mobile cards can get up to 3
			CompatLevels[ CLC_GPUDevice ] = 
				vidInfo.GetDedicatedVRAM() >= 512 && vidInfo.GetPixelShaderVersion() >= 3 && !bIsMobileCard
					? CL_4
					: vidInfo.GetDedicatedVRAM() >= 256
						? CL_3
					: vidInfo.GetDedicatedVRAM() >= 128
						? CL_2
						: CL_1;
		}
	}

	// Determine max compat level based on the OS
	{
		// this code is just here to reference these vars until we find a way to use the data
		CompatLevels[CLC_OS] = bVista || bWinXP ? CL_5 : CL_Unsupported;
	}

	UINT CompositeCompatLevel = (UINT)*std::min_element(CompatLevels, CompatLevels+ARRAY_COUNT(CompatLevels));
	// get the GPU stats out of the way to determine CPU stats
	UINT GPUCompatLevel = Min(CompatLevels[CLC_GPUDevice], CompatLevels[CLC_GPUMem]);
	CompatLevels[CLC_GPUMem] = CL_5;
	CompatLevels[CLC_GPUDevice] = CL_5;
	UINT CPUCompatLevel = (UINT)*std::min_element(CompatLevels, CompatLevels+ARRAY_COUNT(CompatLevels));
	
	return FCompatibilityLevelInfo(CompositeCompatLevel, CPUCompatLevel, GPUCompatLevel);
}
