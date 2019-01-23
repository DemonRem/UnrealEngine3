BuildConsole ..\Development\Src\UnrealEngine3.sln /Clean /cfg="Release|Mixed Platforms"
BuildConsole ..\Development\Src\UnrealEngine3.sln /Clean /cfg="Debug|Mixed Platforms"
BuildConsole ..\Development\Src\UnrealEngine3.sln /Clean /cfg="XeRelease|Mixed Platforms"
BuildConsole ..\Development\Src\UnrealEngine3.sln /Clean /cfg="XeDebug|Mixed Platforms"

rd ..\Development\Intermediate\ /s /q

del Lib\Release\*.* /q
del Lib\ReleaseLTCG\*.* /q
del Lib\Debug\*.* /q

del Xenon\Lib\XeRelease\*.* /q
del Xenon\Lib\XeReleaseLTCG\*.* /q
del Xenon\Lib\XeReleaseLTCG-DebugConsole\*.* /q
del Xenon\Lib\XeDebug\*.* /q

del ..\ExampleGame\Script\*.u /q
del ..\UTGame\Script\*.u /q
del ..\WarGame\Script\*.u /q
del ..\GearGame\Script\*.u /q

del ..\ExampleGame\ScriptFinalRelease\*.u /q
del ..\UTGame\ScriptFinalRelease\*.u /q
del ..\WarGame\ScriptFinalRelease\*.u /q
del ..\GearGame\ScriptFinalRelease\*.u /q

