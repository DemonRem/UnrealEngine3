@REM call this batch file using 'store_symbols <changelist_number> <build_version>'


@REM ******************************************
@REM Setup environment variables (some need to be updated)
@REM ******************************************

@REM This is where the symbol store will be located - this needs to be changed
@SET SYMBOL_STORE_LOCATION=\\titan\UnrealEngine\Symbols

@REM This is the location of the symsrv.exe and other files
@SET SYMBOL_SERVER_EXEPATH="C:\Program Files\Debugging Tools for Windows\symstore.exe"

@REM The directory on the build machine which correspond to the main UnrealEngine3 directory
@SET BASE_DIRECTORY=C:\Build_UE3\UnrealEngine3-Gears

@REM The directory on the build machine that contains the UnrealEngine3.sln file
@SET BUILD_DIRECTORY=%BASE_DIRECTORY%\Development



@REM The directory on the build machine for the output files from the build (.exe files, pdb, etc)
@SET IMAGE_LOCATION_PC=%BASE_DIRECTORY%\Binaries

@REM The directory on the build machine for the output files from the build (.xex files, pdb, etc)
@SET IMAGE_LOCATION_XENON=%BASE_DIRECTORY%


@REM The directory on the build machine that contains the .pdb files for PC Release
@SET PDB_LOCATION_PC=%IMAGE_LOCATION_PC%\Lib

@REM The directory on the build machine that contains the .pdb files for Xenon Release
@SET PDB_LOCATION_XENON=%IMAGE_LOCATION_PC%\Xenon\Lib



@REM Delete any existing symbol index file as it is transient per session
@if exist %SYMBOL_STORE_LOCATION%\exe_index.txt del %SYMBOL_STORE_LOCATION%\exe_index.txt
@if exist %SYMBOL_STORE_LOCATION%\exe_xenon_index.txt del %SYMBOL_STORE_LOCATION%\exe_xenon_index.txt

@if exist %SYMBOL_STORE_LOCATION%\pdb_index.txt del %SYMBOL_STORE_LOCATION%\pdb_index.txt
@if exist %SYMBOL_STORE_LOCATION%\pdb_xenon_index.txt del %SYMBOL_STORE_LOCATION%\pdb_xenon_index.txt


@REM Build the index file for the symbol store;  this builds the list of files that will be added to the symbol store
%SYMBOL_SERVER_EXEPATH% add /p /g %IMAGE_LOCATION_PC% /l /f %IMAGE_LOCATION_PC%\*.exe /x %SYMBOL_STORE_LOCATION%\exe_index.txt /a /o
%SYMBOL_SERVER_EXEPATH% add /p /g %IMAGE_LOCATION_XENON% /l /f %IMAGE_LOCATION_XENON%\*.xex /x %SYMBOL_STORE_LOCATION%\exe_xenon_index.txt /a /o
%SYMBOL_SERVER_EXEPATH% add /p /g %IMAGE_LOCATION_XENON% /l /f %IMAGE_LOCATION_XENON%\*.pe /x %SYMBOL_STORE_LOCATION%\exe_xenon_index.txt /a /o

%SYMBOL_SERVER_EXEPATH% add /r /p /g %PDB_LOCATION_PC% /l /f %PDB_LOCATION_PC%\*.pdb /x %SYMBOL_STORE_LOCATION%\pdb_index.txt /a /o

%SYMBOL_SERVER_EXEPATH% add /r /p /g %PDB_LOCATION_XENON% /l /f %PDB_LOCATION_XENON%\*.pdb /x %SYMBOL_STORE_LOCATION%\pdb_xenon_index.txt /a /o



@REM Add files to the symbol store, using the index file as a guide
%SYMBOL_SERVER_EXEPATH% add /y %SYMBOL_STORE_LOCATION%\exe_index.txt /l /g %IMAGE_LOCATION_PC% /s %SYMBOL_STORE_LOCATION% /t "UnrealEngine3-PC" /v "%1" /c "%2" /o /compress 
%SYMBOL_SERVER_EXEPATH% add /y %SYMBOL_STORE_LOCATION%\exe_xenon_index.txt /l /g %IMAGE_LOCATION_XENON% /s %SYMBOL_STORE_LOCATION% /t "UnrealEngine3-Xenon" /v "%1" /c "%2" /o /compress 

%SYMBOL_SERVER_EXEPATH% add /y %SYMBOL_STORE_LOCATION%\pdb_index.txt /l /g %PDB_LOCATION_PC% /s %SYMBOL_STORE_LOCATION% /t "UnrealEngine3-PC" /v "%1" /c "%2" /o /compress 
%SYMBOL_SERVER_EXEPATH% add /y %SYMBOL_STORE_LOCATION%\pdb_xenon_index.txt /l /g %PDB_LOCATION_XENON% /s %SYMBOL_STORE_LOCATION% /t "UnrealEngine3-Xenon" /v "%1" /c "%2" /o /compress 




