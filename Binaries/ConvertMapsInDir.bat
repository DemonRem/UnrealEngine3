@ECHO OFF
echo CHECKING OUT NON-SP FILES
for /f %%a IN ('dir /b /S %1\MP*.gear %1\POC_*.gear %1\*_P.gear %1\LBC*.gear') do call CheckoutMap.bat %%a
echo CHECKING OUT SP FILES
for /f %%a IN ('dir /b /S %1\SP_*.gear') do call CheckoutMap.bat %%a
echo CONVERTING FILES
for /f %%a IN ('dir /b /S %1\MP*.gear %1\POC_*.gear %1\*_P.gear %1\LBC*.gear') do ConvertMapsToNavMesh.bat %%~na