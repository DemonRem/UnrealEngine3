REM if we do a  PC only build when we must immediately follow it with a xenon build so we maintain parity

IF EXIST "\\build-server-01\BuildFlags\buildUE3PCS_ALL.txt" GOTO ALREADY_SET

call TriggerBuildWorker buildUE3PCS_ALL.txt


:GOTO ALREADY_SET
REM just exit as the flag is set and we are probably full rebuilding atm