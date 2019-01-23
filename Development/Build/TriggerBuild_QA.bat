
REM if a QA build is being vetted when we do not want to trigger a new build
IF EXIST "\\build-server-01\BuildFlags\QABuildIsBeingVetted.txt" GOTO BRING_VETTED

REM if we are building another QA build then we don't want to force everyone to use the current QA build as it has been deprecated
del /F \\Build-server-01\BuildFlags\OnlyUseQABuildSemaphore.txt

call TriggerBuildWorker buildUE3QA.txt

GOTO END


:BRING_VETTED

REM
REM A QA build is already being vetted by QA team.  GO or NO GO it before triggering another QA build
REM


GOTO END

:END

