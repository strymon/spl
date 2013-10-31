@echo off

set TARGET_VS=2010
set CREATE_ONLY=N
set QT_SETUP="C:\Qt\Qt5.1.1_V2012\5.1.1\msvc2012\bin\qtenv2.bat"
set MSVC_SETUP="C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
set QMAKE_SPEC=win32-msvc2012 
set PLAT=x86

:ARGPARSE
@echo TOKENS: %1 %2 
if %1X==X goto ARGDONE 
:: /p to override the default build path
:: Valid args: 2010, 2012
if /I %1x==/vsverx (
 set TARGET_VS=%2
  shift
  shift
 goto ARGPARSE 
) 

:: Hack, pass in /create and this script create Visual Studio projects
:: Note - use the "Convert to Qt Addin project" is you are using the QT VS Addin
if /I %1x == /createx (
  set CREATE_ONLY=Y
  shift
  goto ARGPARSE 
) 

if /I %1x == /qtenvx (
  set QT_SETUP=%2
  shift
  shift
  goto ARGPARSE 
) 
if /I %1x == /vcargsx (
  set MSVC_SETUP=%2
  shift
  shift
  goto ARGPARSE 
) 

if /I %1x == /makespecx (
  set QMAKE_SPEC=%2
  shift
  shift
  goto ARGPARSE 
) 

if /I %1x == /outpathx (
  set BUILDPATH=%2
  shift
  shift
  goto ARGPARSE 
) 

if /I %1x == /platx (
 set PLAT=%2
 shift
 shift
 goto ARGPARSE 
) 

if /I %1x == /projrootx (
  set PROJ_ROOT=%2
  shift
 shift
  goto ARGPARSE 
) 

@echo ERROR: Unknown Arg: %1 
goto :EOF
:ARGDONE

::call :echoall

pushd .

:: Only do the setup once so the shell can be reused
if %DC_BLD_INIT%_ == YES_ goto doBuild
set DC_BLD_INIT=YES

echo Setting up build environment 
call %QT_SETUP%
call %MSVC_SETUP% %PLAT%

:doBuild
popd

:: Force a clean build - but be careful recursively deleting a directory!!!
:: For safety - only delete this path if a Makefile is found
if exist %BUILDPATH%\spl\NUL rmdir /s /q %BUILDPATH%
sleep .5
mkdir %BUILDPATH%

pushd .
cd /D %BUILDPATH%
:: Make sure we are not in the same dir
if exist win_deploy.bat goto :ERROR3
@echo Invoking QMAKE: -platform %QMAKE_SPEC% -r -tp vc %PROJ_ROOT% 
qmake -platform %QMAKE_SPEC% -r -tp vc %PROJ_ROOT% 

if %CREATE_ONLY% == Y goto :EOF

msbuild QRtMidi/QRtMidi.vcxproj /t:Rebuild /p:Configuration=Release 
msbuild spl/spl.vcxproj /t:Rebuild /p:Configuration=Release
popd
goto :EOF



:ERROR1
@echo must specify build path
goto :EOF
:ERROR2
@echo must specify project root path
goto :EOF

:ERROR3
@echo ERROR: The build directory was somehow not created: %BUILDPATH% 
goto :EOF


:echoall
@echo TARGET_VS %TARGET_VS%
@echo CREATE_ONLY %CREATE_ONLY%
@echo BUILDPATH %BUILDPATH%
@echo QT_SETUP %QT_SETUP% 
@echo MSVC_SETUP %MSVC_SETUP%
@echo QMAKE_SPEC %QMAKE_SPEC%
@echo BUILDPATH %BUILDPATH%
@echo PLAT %PLAT% 
@echo PROJ_ROOT %PROJ_ROOT%
goto :EOF
