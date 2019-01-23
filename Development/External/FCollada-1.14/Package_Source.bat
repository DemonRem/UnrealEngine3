rem Use this batch file to package up a new version of the FS import classes

rem Clean up the old version package files, if present
rm -f FCollada_Source.zip
rm -R -f Package_Temp

rem Copy all the interesting files: DLL, LIB, H, TXT over
rem to a temporary folder
mkdir Package_Temp
mkdir Package_Temp\FCDocument
mkdir Package_Temp\FMath
mkdir Package_Temp\FUtils
mkdir Package_Temp\FCollada.xcodeproj
mkdir Package_Temp\Documentation
mkdir Package_Temp\FColladaTest
mkdir Package_Temp\FColladaTest\FCTestExportImport
mkdir Package_Temp\FColladaTest\FCTestAssetManagement
mkdir Package_Temp\FColladaTest\FCTestXRef
mkdir Package_Temp\FColladaTest\Samples

copy FCDocument\*.HPP Package_Temp\FCDocument
copy FCDocument\*.CPP Package_Temp\FCDocument
copy FCDocument\*.H Package_Temp\FCDocument
copy FCDocument\*.VCPROJ Package_Temp\FCDocument

copy FMath\*.H Package_Temp\FMath
copy FMath\*.HPP Package_Temp\FMath
copy FMath\*.CPP Package_Temp\FMath
copy FMath\*.VCPROJ Package_Temp\FMath

copy FUtils\*.H Package_Temp\FUtils
copy FUtils\*.HPP Package_Temp\FUtils
copy FUtils\*.CPP Package_Temp\FUtils
copy FUtils\*.VCPROJ Package_Temp\FUtils

copy FColladaTest\*.H Package_Temp\FColladaTest
copy FColladaTest\*.HPP Package_Temp\FColladaTest
copy FColladaTest\*.CPP Package_Temp\FColladaTest
copy FColladaTest\*.VCPROJ Package_Temp\FColladaTest

copy FColladaTest\FCTestExportImport\*.H Package_Temp\FColladaTest\FCTestExportImport
copy FColladaTest\FCTestExportImport\*.HPP Package_Temp\FColladaTest\FCTestExportImport
copy FColladaTest\FCTestExportImport\*.CPP Package_Temp\FColladaTest\FCTestExportImport

copy FColladaTest\FCTestAssetManagement\*.H Package_Temp\FColladaTest\FCTestAssetManagement
copy FColladaTest\FCTestAssetManagement\*.HPP Package_Temp\FColladaTest\FCTestAssetManagement
copy FColladaTest\FCTestAssetManagement\*.CPP Package_Temp\FColladaTest\FCTestAssetManagement

copy FColladaTest\FCTestXRef\*.H Package_Temp\FColladaTest\FCTestXRef
copy FColladaTest\FCTestXRef\*.HPP Package_Temp\FColladaTest\FCTestXRef
copy FColladaTest\FCTestXRef\*.CPP Package_Temp\FColladaTest\FCTestXRef

del FColladaTest\Samples\TestOut.dae
del FColladaTest\Samples\FColladaTestLog.txt
copy FColladaTest\Samples\*.DAE Package_Temp\FColladaTest\Samples
copy FColladaTest\Samples\*.txt Package_Temp\FColladaTest\Samples

cp -R LibXML Package_Temp
rm -f -R Package_Temp\LibXML\.svn
rm -f -R Package_Temp\LibXML\include\.svn
rm -f -R Package_Temp\LibXML\include\libxml\.svn

copy *.TXT Package_Temp
copy *.vcproj Package_Temp
copy *.cpp Package_Temp
copy *.sln Package_Temp
copy *.h Package_Temp
copy *.def Package_Temp

copy FCollada.xcodeproj\*.pbxproj Package_Temp\FCollada.xcodeproj
copy Output\Doxygen\html\*.* Package_Temp\Documentation
copy Output\Doxygen\FCollada.html Package_Temp

rem Zip up the temporary directory
cd Package_Temp
zip -9 -r ..\FCollada_Source.zip .
cd ..

rem Clean up
rm -R -f Package_Temp
