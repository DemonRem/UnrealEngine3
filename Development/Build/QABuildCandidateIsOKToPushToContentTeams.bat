REM this will push the candidate .ini infoz to the .ini file that the .exe actually uses

copy /Y \\Build-server-01\BuildFlags\QABuildInfoCandidateToBeSignedOffOn.ini \\Build-server-01\BuildFlags\QABuildInfo.ini

echo > \\build-server-01\BuildFlags\OnlyUseQABuildSemaphore.txt

del /F \\Build-server-01\BuildFlags\QABuildIsBeingVetted.txt


pause

