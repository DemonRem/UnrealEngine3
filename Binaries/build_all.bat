@echo off
setlocal

:START

REM ************ BUILD PC ************
REM Build the release solution for PC
set solution=Release
goto build_pc

:BUILD_PC_DEBUG
set solution=Debug
set pc_compile_pass=1


:BUILD_PC
"C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\IDE\devenv.com" D:\Code\UnrealEngine3\Development\Src\UnrealEngine3.sln /build "%solution%" /project "ExampleGame Win32"
"C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\IDE\devenv.com" D:\Code\UnrealEngine3\Development\Src\UnrealEngine3.sln /build "%solution%" /project "UTGame Win32"
"C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\IDE\devenv.com" D:\Code\UnrealEngine3\Development\Src\UnrealEngine3.sln /build "%solution%" /project "GearGame Win32"
if errorlevel 1 goto builderror
if defined pc_compile_pass goto check_xbox_buildflag


REM Compile all scripts
set solution=UnrealScript
call build_script.bat -updateinisauto -auto %1 %2 %3 %4 %5
if errorlevel 1 goto builderror


REM Build the solution again in case we updated headers
if not defined count (
	if %1!==-full! shift
	set count=1
	set pc_compile_pass=1
	goto start
) else if %1! NEQ skip_debug! (
	if not defined pc_compile_pass goto build_pc_debug
)

:CHECK_XBOX_BUILDFLAG
if %1! NEQ skip_xbox! (
	shift
	goto build_xbox_configs
) else (
	goto end
)


:BUILD_XBOX_CONFIGS

REM ***********  BUILD XBOX ************
REM Build the release solution for Xbox
set solution=Release
goto build_xbox

:BUILD_XBOX_DEBUG
if %1!==skip_xbox_debug! (
	goto end
)

set solution=Debug
set xbox_compile_pass=1

:BUILD_XBOX
"C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\IDE\devenv.com" D:\Code\UnrealEngine3\Development\Src\UnrealEngine3.sln /build "%solution%" /project "ExampleGame Xbox360"
"C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\IDE\devenv.com" D:\Code\UnrealEngine3\Development\Src\UnrealEngine3.sln /build "%solution%" /project "UTGame Xbox360"
"C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\IDE\devenv.com" D:\Code\UnrealEngine3\Development\Src\UnrealEngine3.sln /build "%solution%" /project "GearGame Xbox360"

if errorlevel 1 goto builderror
if not defined xbox_compile_pass goto build_xbox_debug

goto end

:builderror
echo Error encountered while building solution %solution%.

:end
