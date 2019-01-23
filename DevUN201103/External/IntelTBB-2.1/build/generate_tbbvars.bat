@echo off
REM
REM Copyright 2005-2009 Intel Corporation.  All Rights Reserved.
REM
REM The source code contained or described herein and all documents related
REM to the source code ("Material") are owned by Intel Corporation or its
REM suppliers or licensors.  Title to the Material remains with Intel
REM Corporation or its suppliers and licensors.  The Material is protected
REM by worldwide copyright laws and treaty provisions.  No part of the
REM Material may be used, copied, reproduced, modified, published, uploaded,
REM posted, transmitted, distributed, or disclosed in any way without
REM Intel's prior express written permission.
REM
REM No license under any patent, copyright, trade secret or other
REM intellectual property right is granted to or conferred upon you by
REM disclosure or delivery of the Materials, either expressly, by
REM implication, inducement, estoppel or otherwise.  Any license under such
REM intellectual property rights must be express and approved by Intel in
REM writing.
REM
if exist tbbvars.bat exit
echo Generating tbbvars.bat
echo @echo off>tbbvars.bat
setlocal
for %%D in ("%tbb_root%") do set actual_root=%%~fD
if x%1==x goto without

echo SET TBB21_INSTALL_DIR=%actual_root%>>tbbvars.bat
echo SET TBB_ARCH_PLATFORM=%arch%\%runtime%>>tbbvars.bat
echo SET INCLUDE=%%TBB21_INSTALL_DIR%%\include;%%INCLUDE%%>>tbbvars.bat
echo SET LIB=%%TBB21_INSTALL_DIR%%\build\%1;%%LIB%%>>tbbvars.bat
echo SET PATH=%%TBB21_INSTALL_DIR%%\build\%1;%%PATH%%>>tbbvars.bat

if exist tbbvars.sh goto skipsh
set fslash_root=%actual_root:\=/%
echo Generating tbbvars.sh
echo #!/bin/sh>tbbvars.sh
echo export TBB21_INSTALL_DIR="%fslash_root%">>tbbvars.sh
echo TBB_ARCH_PLATFORM="%arch%\%runtime%">>tbbvars.sh
echo if [ -z "${PATH}" ]; then>>tbbvars.sh
echo     export PATH="${TBB21_INSTALL_DIR}/build/%1">>tbbvars.sh
echo else>>tbbvars.sh
echo     export PATH="${TBB21_INSTALL_DIR}/build/%1;$PATH">>tbbvars.sh
echo fi>>tbbvars.sh
echo if [ -z "${LIB}" ]; then>>tbbvars.sh
echo     export LIB="${TBB21_INSTALL_DIR}/build/%1">>tbbvars.sh
echo else>>tbbvars.sh
echo     export LIB="${TBB21_INSTALL_DIR}/build/%1;$LIB">>tbbvars.sh
echo fi>>tbbvars.sh
echo if [ -z "${INCLUDE}" ]; then>>tbbvars.sh
echo     export INCLUDE="${TBB21_INSTALL_DIR}/include">>tbbvars.sh
echo else>>tbbvars.sh
echo     export INCLUDE="${TBB21_INSTALL_DIR}/include;$INCLUDE">>tbbvars.sh
echo fi>>tbbvars.sh
:skipsh

if exist tbbvars.csh goto skipcsh
echo Generating tbbvars.csh
echo #!/bin/csh>tbbvars.csh
echo setenv TBB21_INSTALL_DIR "%actual_root%">>tbbvars.csh
echo setenv TBB_ARCH_PLATFORM "%arch%\%runtime%">>tbbvars.csh
echo if (! $?PATH) then>>tbbvars.csh
echo     setenv PATH "${TBB21_INSTALL_DIR}\build\%1">>tbbvars.csh
echo else>>tbbvars.csh
echo     setenv PATH "${TBB21_INSTALL_DIR}\build\%1;$PATH">>tbbvars.csh
echo endif>>tbbvars.csh
echo if (! $?LIB) then>>tbbvars.csh
echo     setenv LIB "${TBB21_INSTALL_DIR}\build\%1">>tbbvars.csh
echo else>>tbbvars.csh
echo     setenv LIB "${TBB21_INSTALL_DIR}\build\%1;$LIB">>tbbvars.csh
echo endif>>tbbvars.csh
echo if (! $?INCLUDE) then>>tbbvars.csh
echo     setenv INCLUDE "${TBB21_INSTALL_DIR}\include">>tbbvars.csh
echo else>>tbbvars.csh
echo     setenv INCLUDE "${TBB21_INSTALL_DIR}\include;$INCLUDE">>tbbvars.csh
echo endif>>tbbvars.csh
)
:skipcsh
exit

:without
set bin_dir=%CD%
echo SET tbb_root=%actual_root%>>tbbvars.bat
echo SET tbb_bin=%bin_dir%>>tbbvars.bat
echo SET TBB_ARCH_PLATFORM=%arch%\%runtime%>>tbbvars.bat
echo SET INCLUDE="%%tbb_root%%\include";%%INCLUDE%%>>tbbvars.bat
echo SET LIB="%%tbb_bin%%";%%LIB%%>>tbbvars.bat
echo SET PATH="%%tbb_bin%%";%%PATH%%>>tbbvars.bat

endlocal
