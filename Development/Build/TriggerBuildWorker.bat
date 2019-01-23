REM this will actually create the file on the server for the buildmachine to use
REM additionally it will check and alert of builds already started


IF EXIST "\\Build-server-01\BuildFlags\QABuildIsBeingVetted.txt" GOTO QA_BEING_VETTED

IF EXIST "\\build-server-01\BuildFlags\%1" GOTO ALREADY_SET


echo > \\build-server-01\BuildFlags\%1
goto EXIT




:ALREADY_SET
REM
REM The buildtype: %1 has already been triggered
REM

pause
GOTO EXIT


:QA_BEING_VETTED
REM
REM A QA build is being vetted.  No builds will take place until it has been go'd or no go'd.  Find your local QA rep to expedite the process
REM 

pause
GOTO EXIT


:EXIT

