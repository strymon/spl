:: A script for building the Win32 version of the Strymon Librarian
::
:: Copyright 2013 Damage Control Engineering, LLC
::
:: CHECKLIST TO SETUP BUILD
:: 1) Make sure the QT paths are setup correctly
:: 2) Review the runtime redistributable subroutine
:: 3) Verify that the MSVC_DIR is setup correctly
::
:: Usage:
:: win_deploy <no args>  - increment version build number in main.cpp, rebuild, create installer
:: win_deploy <ver_str> - set version in main.cpp, rebuild, create installer
@echo off 
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set BLD_SWITCHES=
set OK_TO_INST=YES
set OK_TO_BUILD=YES
set PLAT_DIR=%CD%
set PROJ_ROOT=%CD%\..

:: The 'out-of-source' build directory
set BUILD_DIR=..\..\build
set MANUAL_VER=AUTO
set REDIST_PATH=%BUILD_DIR%\redist.inc

:: VS2010 and QT 5.1.1 is the current default
set USE_VS_2012=NO
set VSVER=2010
set PLAT=x86
set QTMAKESPEC=win32-msvc2010
set MSVC_DIR=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC
:: Spect the version of the qt icu files e.g. icuuc51.dll
set QT_ICU_VER=51
set D3DVER=43
set QT_ROOT=C:\Qt\Qt5.1.1_VS2010\5.1.1\msvc2010
set QT_SETUP=%QT_ROOT%\bin\qtenv2.bat
set MSVC_REDIST_VER=100
set MSVC_REDIST="%MSVC_DIR%\redist\x86\Microsoft.VC%MSVC_REDIST_VER%.CRT"

set CONFIG=release


:ARGPARSE
if %1X==X goto ARGDONE 
@echo TOKENS: %1 %2
:: /p to override the default build path
if /I %1x==/px (
 set BUILD_DIR=%2
 shift
 shift
 goto ARGPARSE 
) 

:: /ver to manually specify the version and override the automated version.
if /I %1x==/verx (
 set MANUAL_VER=%2
 shift
 shift
 goto ARGPARSE 
) 

:: /inst-only (don't build, just create an installer)
if /I %1x==/inst-onlyx (
 set OK_TO_BUILD=NO
 set MANUAL_VER=cur
 shift
 goto ARGDONE 
) 

if /I %1x==/vs2012x (
 set USE_VS_2012=YES
 shift
 goto ARGPARSE 
) 

if /I %1x==/createx (
 set BLD_SWITCHES=%BLD_SWITCHES% /create
 set OK_TO_INST=NO
 shift
 goto ARGPARSE 
) 

:: Print the script usage 
if /I %1x==/helpx (
 goto USEAGE_HELP
) 

@echo PARAM ERROR: %*

:USEAGE_HELP

@echo --------------------------------------------------------------------------------------  
@echo - USEAGE
@echo - 
@echo - /p ^<path^>       - optional - Path used to build project
@echo - /inst-only        - don't build or inc version, just build installer
@echo - /ver ^<version^>  - optional - force build to use specified version or current if
@echo -                                'cur' is specified
@echo - /vs2012           - optional - force build to use the MSVC 2012 compiler
@echo - /create           - optional - creatre VS project files only, do not build
@echo - 
@echo --------------------------------------------------------------------------------------  
goto :EOF

:ARGDONE

if %USE_VS_2012% == NO goto confset
:: QT 5.1.1 VS 2012 settings:w
set VSVER=2012
set QTMAKESPEC=win32-msvc2012
set MSVC_DIR=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC
set QT_ICU_VER=51
set D3DVER=46
set QT_ROOT=C:\Qt\Qt5.1.1_V2012\5.1.1\msvc2012
set QT_SETUP=%QT_ROOT%\bin\qtenv2.bat
set MSVC_REDIST_VER=110
set MSVC_REDIST="%MSVC_DIR%\redist\%PLAT%\Microsoft.VC%MSVC_REDIST_VER%.CRT"

:confset

if not %MANUAL_VER% == AUTO goto set_ver
python inc_version.py -f ..\spl\main.cpp -v kDcVersionString --inc_build
goto setup

:set_ver
if /I "%MANUAL_VER%" == "cur" goto setup 
python inc_version.py -s %MANUAL_VER% -f ..\spl\main.cpp -v kDcVersionString

:setup
:: Extract version from main.cpp (and use this lame 'cmd.exe for loop' to set VER_STR with output from python script) <sigh>
for /f "tokens=*" %%a in ( 'python inc_version.py -a -f ..\spl\main.cpp -v kDcVersionString' ) do ( set VER_STR=%%a)
echo Using Version : %VER_STR%

set APP_EXE=%BUILD_DIR%\spl\%CONFIG%\spl.exe
set INST_EXE=%BUILD_DIR%\strymon_lib_setup_%VER_STR%.exe 
set INST_ZIP=%BUILD_DIR%\strymon_lib_setup_%VER_STR%.zip 

:: MACHINE DEPENDENT SETUP
:: Setup requires the installation of nsis: ( http://nsis.sourceforge.net/Main_Page )
set nsi="C:\Program Files (x86)\NSIS\makensis.exe"

if  %OK_TO_BUILD% == NO goto prep_inst

:start_build

:: Cleanup a previous build with same version
if exist %APP_EXE% del %APP_EXE%

:: Create vs projects and execute build
call bld_msvc.bat /vsver %VSVER% /plat %PLAT% /outpath %BUILD_DIR% /projroot %PROJ_ROOT% /vcargs "%MSVC_DIR%\vcvarsall.bat" /qtenv "%QT_SETUP%" /makespec %QTMAKESPEC% %BLD_SWITCHES%
if %OK_TO_INST%==NO goto alldone

if not exist %APP_EXE% goto error_no_exe:

:prep_inst
echo %CD%


if exist %INST_EXE% del %INST_EXE%
if exist %INST_ZIP% del %INST_ZIP%

call :createRedist
if %createRedist%==1 (goto error)

:: Invoke the installer script
%nsi% win_inst.nsi /DREDIST_FILE=%REDIST_PATH%

if not exist %INST_EXE% goto error

:: Compress the installer executable
::7z.exe a -tzip %INST_ZIP% %INST_EXE%
::if not exist %INST_ZIP% goto error

echo success

:post_build
:: Call optional post_build python script (use this to post to test system, etc...)
::if exist post_build.py python post_build.py -f %INST_ZIP% -v %VER_STR% -p Librarian 
if exist R:\__ShowDetails.exe copy /V /B /Z /Y  %INST_EXE% R:\Internal

:alldone
goto :EOF

:error
echo.
echo.
echo failure
goto :EOF
:error_no_exe
echo.
echo %APP_EXE% was not found, aborting install creation.
goto :EOF

:: Code to copy run-time files from Qt and MSVC as well as
:: any other static files such as the lgpl.
:createRedist

  set createRedist=1

  if not exist %REDIST_PATH% mkdir %REDIST_PATH%
  if not exist %REDIST_PATH%\platforms mkdir %REDIST_PATH%\platforms
  copy /v "%QT_ROOT%\plugins\platforms\qwindows.dll" %REDIST_PATH%\platforms
  copy /v "%QT_ROOT%\bin\D3DCompiler_%D3DVER%.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\Qt5Core.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\Qt5Gui.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\Qt5Network.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\Qt5Widgets.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\icudt%QT_ICU_VER%.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\icuin%QT_ICU_VER%.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\icuuc%QT_ICU_VER%*.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\libEGL.dll" %REDIST_PATH%
  copy /v "%QT_ROOT%\bin\libGLESv2.dll" %REDIST_PATH%
  copy /v %MSVC_REDIST%\msvcp%MSVC_REDIST_VER%.dll %REDIST_PATH%
  copy /v %MSVC_REDIST%\msvcr%MSVC_REDIST_VER%.dll %REDIST_PATH%
  copy /v %PLAT_DIR%\lgpl.txt %REDIST_PATH%

  if not exist %REDIST_PATH%\platforms\qwindows.dll goto redistcopyerror
  if not exist %REDIST_PATH%\D3DCompiler_%D3DVER%.dll goto redistcopyerror
  if not exist %REDIST_PATH%\Qt5Core.dll goto redistcopyerror
  if not exist %REDIST_PATH%\Qt5Gui.dll goto redistcopyerror
  if not exist %REDIST_PATH%\Qt5Network.dll goto redistcopyerror
  if not exist %REDIST_PATH%\Qt5Widgets.dll goto redistcopyerror
  if not exist %REDIST_PATH%\icudt%QT_ICU_VER%.dll goto redistcopyerror
  if not exist %REDIST_PATH%\icuin%QT_ICU_VER%.dll goto redistcopyerror
  if not exist %REDIST_PATH%\icuuc%QT_ICU_VER%.dll goto redistcopyerror
  if not exist %REDIST_PATH%\libEGL.dll goto redistcopyerror
  if not exist %REDIST_PATH%\libGLESv2.dll goto redistcopyerror

  if not exist %REDIST_PATH%\msvcp%MSVC_REDIST_VER%.dll goto redistcopyerror
  if not exist %REDIST_PATH%\msvcr%MSVC_REDIST_VER%.dll goto redistcopyerror
   
  echo SetOutPath $INSTDIR\Librarian > %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\D3DCompiler_%D3DVER%.dll >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\Qt5Core.dll >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\Qt5Gui.dll >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\Qt5Network.dll >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\Qt5Widgets.dll >> %BUILD_DIR%\redist.ins

  echo File %REDIST_PATH%\icudt%QT_ICU_VER%.dll >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\icuin%QT_ICU_VER%.dll >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\icuuc%QT_ICU_VER%.dll >> %BUILD_DIR%\redist.ins

  echo File %REDIST_PATH%\libEGL.dll >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\libGLESv2.dll >> %BUILD_DIR%\redist.ins

  echo File %REDIST_PATH%\msvcp%MSVC_REDIST_VER%.dll >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\msvcr%MSVC_REDIST_VER%.dll  >> %BUILD_DIR%\redist.ins

  echo # Copy the Qt platform file >> %BUILD_DIR%\redist.ins
  echo CreateDirectory $INSTDIR\Librarian\platforms >> %BUILD_DIR%\redist.ins
  echo SetOutPath $INSTDIR\Librarian\platforms >> %BUILD_DIR%\redist.ins
  echo File %REDIST_PATH%\platforms\qwindows.dll >> %BUILD_DIR%\redist.ins 

  setlocal
  set incfile=%BUILD_DIR%\redist_remove.ins

  echo # Delete runtime > %incfile%
  echo Delete platforms\qwindows.dll >> %incfile%
  echo Delete D3DCompiler_%D3DVER%.dll >> %incfile%
  echo Delete Qt5Core.dll >> %incfile%
  echo Delete Qt5Gui.dll >> %incfile%
  echo Delete Qt5Network.dll >> %incfile%
  echo Delete Qt5Widgets.dll >> %incfile%
  echo Delete icudt%QT_ICU_VER%.dll >> %incfile%
  echo Delete icuin%QT_ICU_VER%.dll >> %incfile%
  echo Delete icuuc%QT_ICU_VER%.dll >> %incfile%
  echo Delete libEGL.dll >> %incfile%
  echo Delete libGLESv2.dll >> %incfile%

  echo Delete msvcp%MSVC_REDIST_VER%.dll >> %incfile%
  echo Delete msvcr%MSVC_REDIST_VER%.dll  >> %incfile%
  endlocal

  set createRedist=0
goto :EOF

:redistcopyerror
@echo ERROR - problem coping redist/runtime files
goto :EOF


