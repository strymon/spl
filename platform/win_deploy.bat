:: A script for building the Win32 version of the Strymon Librarian
::
:: Copyright 2013 Damage Control Engineering, LLC
::
:: Usage:
:: win_deploy <no args>  - increment version build number in main.cpp, rebuild, create installer
:: win_deploy <ver_str> - set version in main.cpp, rebuild, create installer
::@echo off

if not "%1_" == "_" goto set_ver
python inc_version.py -f ..\spl\main.cpp -v kDcVersionString --inc_build
goto setup_and_build

:set_ver
python inc_version.py -s %1 -f ..\spl\main.cpp -v kDcVersionString

:setup_and_build

:: Extract version from main.cpp (and use this lame 'cmd.exe for loop' to set VER_STR with output of python script) <sigh>
for /f "tokens=*" %%a in ( 'python inc_version.py -a -f ..\spl\main.cpp -v kDcVersionString' ) do ( set VER_STR=%%a)

set APP_EXE=..\build\bin\Win32\XP_Release\spl.exe
set INST_EXE=..\build\strymon_lib_setup_%VER_STR%.exe 
set INST_ZIP=..\build\strymon_lib_setup_%VER_STR%.exe 

:: MACHINE DEPENDENT SETUP
:: Setup requires the installation of nsis: ( http://nsis.sourceforge.net/Main_Page )
set nsi="C:\Program Files (x86)\NSIS\makensis.exe"
 
:: Cleanup a previous build with same version
if exist %APP_EXE% del %APP_EXE%
if exist %INST_EXE% del %INST_EXE%
if exist %INST_ZIP% del %INST_ZIP%

:: Setup the VS202 build environment
::   Note, the Librarian can be build using the Mingw32 compiler, we use VS2012 in house. 
::   Again, Visual Studio is only required to use this script.
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools\VsDevCmd.bat"
::
:: Build the project using visual studio.  The configuration XP_Release is setup to 
:: use the vs2010 compiler.  Using this configuration enabled support for the Windows XP Platform.
msbuild ..\spl\spl.vcxproj /p:Configuration=XP_Release;Platform=Win32 /t:Rebuild
if not exist %APP_EXE% goto error:

:: Invoke the installer script
%nsi% win_inst.nsi
if not exist %INST_EXE% goto error

:: Compress the installer executable
7z.exe a -tzip %INST_ZIP% %INST_EXE%
if not exist %INST_ZIP% goto error

echo success

:post_build
:: Call optional post_build python script (use this to post to test system, etc...)
if exist post_build.py python post_build.py -f %INST_ZIP% -v %VER_STR% -p Librarian 

goto :EOF

:error
echog
echo.
echo failure
goto :EOF
