REM this will remove the lock on new builds

del /F \\Build-server-01\BuildFlags\OnlyUseQABuildSemaphore.txt

del /F \\Build-server-01\BuildFlags\QABuildIsBeingVetted.txt

pause

