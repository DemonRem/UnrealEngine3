@echo off
REM  
REM MAKEFILE REGENERATION SCRIPT (for WinXP)
REM
REM author: Francesco Montorsi (frm@users.sourceforge.net)
REM date: 7-4-2005


echo.
echo.
echo Regenerating makefiles...
echo.
bakefile_gen
cd ..\..\samples\build\bakefiles
bakefile_gen
cd ..\..\..\build\bakefiles
echo.
echo Regeneration completed.
echo.
echo.
pause
