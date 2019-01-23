@echo off

:start

REM Build the solution for PC
set solution="Release|Mixed Platforms"
BuildConsole ..\Development\Src\UnrealEngine3.sln /Build /cfg=%solution%
if errorlevel 1 goto builderror

REM Compile all scripts
set solution=UnrealScript
call build_script.bat -updateinisauto -auto %1 %2 %3 %4 %5
if errorlevel 1 goto builderror

REM Build the solution again in case we updated headers
if not defined count (
	set count=1
	goto start
)

REM Build the debug solution for PC
set solution="Debug|Mixed Platforms"
BuildConsole ..\Development\Src\UnrealEngine3.sln /Build /cfg=%solution%
if errorlevel 1 goto builderror

Build the release solution for Xbox
set solution="XeRelease|Mixed Platforms"
BuildConsole ..\Development\Src\UnrealEngine3.sln /Build /cfg=%solution%
if errorlevel 1 goto builderror

REM Build the debug solution for Xbox
REM set solution="XeDebug|Mixed Platforms"
REM BuildConsole ..\Development\Src\UnrealEngine3.sln /Build /cfg=%solution%
REM if errorlevel 1 goto builderror

goto end

:builderror
echo Error encountered while building solution %solution%.

:end
