:: A script for building the Win32 version of the Strymon Librarian
::
:: Copyright 2013-2015 Damage Control Engineering, LLC
::
:: CHECKLIST TO SETUP BUILD
:: 1) Make sure the QT paths are setup correctly
:: 2) Review the runtime redistributable subroutine
:: 3) Verify that the MSVC_DIR is setup correctly

@echo off 
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set LOAD_ENVI_ONLY=NO 
set BLD_SWITCHES=
set OK_TO_INST=YES
set OK_TO_BUILD=YES
set PLAT_DIR=%CD%
set PROJ_ROOT=%CD%\..
set PLAT=x86

:: The 'out-of-source' build directory
set BUILD_DIR=..\..\build
set MANUAL_VER=AUTO
set REDIST_PATH=%BUILD_DIR%\redist
set REDIST_INC_FILE=%BUILD_DIR%\redist.ins

set QMAKE_SPEC=win32-msvc2013
set MSVC_DIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC
set QT_ICU_VER=53
set D3DVER=47
set QT_ROOT=C:\Qt\Qt5.4.0_32\5.4\msvc2013
set MSVC_REDIST_VER=120
set MSVC_REDIST="%MSVC_DIR%\redist\%PLAT%\Microsoft.VC%MSVC_REDIST_VER%.CRT"

set CONFIG=release


:ARGPARSE
if %1X==X goto ARGDONE 
@echo TOKENS: %1 %2
:: /p to override the default build path
if /I %1x==/pathx (
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

:: /plat to manually specify the msvc tools platform (x86 or amd64)
if /I %1x==/platx (
 set PLAT=%2
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

if /I %1x==/createx (
 set BLD_SWITCHES=%BLD_SWITCHES% /create
 set OK_TO_BUILD=NO
 set OK_TO_INST=NO
 set MANUAL_VER=cur
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
@echo - /path ^<path^>     - optional - Path used to build project
@echo - /inst-only         - don't build or inc version, just build installer
@echo - /ver ^<version^>   - optional - force build to use specified version or current if
@echo -                                 'cur' is specified
@echo - /create            - optional - creatre VS project files only, do not invoke build
@echo - /plat ^<platform^> - optional - select the build platform (x86 or amd64) 
@echo -                                 defaults to x86 (32 bit)
@echo --------------------------------------------------------------------------------------  
goto :EOF

:ARGDONE

if not %MANUAL_VER% == AUTO goto set_ver
::python inc_version.py -f ..\spl\main.cpp -v kDcVersionString --inc_build
goto setup

:set_ver
if /I "%MANUAL_VER%" == "cur" goto setup 
::python inc_version.py -s %MANUAL_VER% -f ..\spl\main.cpp -v kDcVersionString

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


:start_build
@echo Start Build

:: Force a clean build - but be careful recursively deleting a directory!!!
if exist %BUILD_DIR%\spl\NUL rmdir /s /q %BUILD_DIR%
sleep .5
mkdir %BUILD_DIR%

pushd .
cd /D %BUILD_DIR%

@echo Reloacted to %CD%

:: Cleanup a previous build with same version
if exist %APP_EXE% del %APP_EXE%
set PATH=%QT_ROOT%\bin;%PATH%


@echo Setting up MSVC: "%MSVC_DIR%\vcvarsall.bat" %PLAT%
call "%MSVC_DIR%\vcvarsall.bat" %PLAT%

@echo call complete
if %LOAD_ENVI_ONLY%==YES goto :EOF

:: Make sure we are not in the same dir
if exist win_deploy.bat goto :ERROR3
@echo Invoking QMAKE: -platform %QMAKE_SPEC% -r -tp vc %PROJ_ROOT% 
qmake -platform %QMAKE_SPEC% -tp vc -r %PROJ_ROOT% 

if %OK_TO_BUILD% == NO goto chk_inst
msbuild /m lib/DcMidi/DcMidi.vcxproj /p:PlatformToolset=v120_xp /t:Rebuild /p:Configuration=%CONFIG% 
msbuild /m spl/spl.vcxproj /p:PlatformToolset=v120_xp /t:Rebuild /p:Configuration=%CONFIG% /p:BuildInParallel=true
popd

:chk_inst
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
   
  :: Create NSIS include files for the redist files
  echo SetOutPath $INSTDIR\Librarian > %REDIST_INC_FILE%
  echo File %REDIST_PATH%\D3DCompiler_%D3DVER%.dll >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\Qt5Core.dll >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\Qt5Gui.dll >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\Qt5Network.dll >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\Qt5Widgets.dll >> %REDIST_INC_FILE%

  echo File %REDIST_PATH%\icudt%QT_ICU_VER%.dll >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\icuin%QT_ICU_VER%.dll >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\icuuc%QT_ICU_VER%.dll >> %REDIST_INC_FILE%

  echo File %REDIST_PATH%\libEGL.dll >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\libGLESv2.dll >> %REDIST_INC_FILE%

  echo File %REDIST_PATH%\msvcp%MSVC_REDIST_VER%.dll >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\msvcr%MSVC_REDIST_VER%.dll  >> %REDIST_INC_FILE%

  echo # Copy the Qt platform file >> %REDIST_INC_FILE%
  echo CreateDirectory $INSTDIR\Librarian\platforms >> %REDIST_INC_FILE%
  echo SetOutPath $INSTDIR\Librarian\platforms >> %REDIST_INC_FILE%
  echo File %REDIST_PATH%\platforms\qwindows.dll >> %REDIST_INC_FILE% 

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


