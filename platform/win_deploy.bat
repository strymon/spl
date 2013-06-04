:: Version format Maj.Min.Inc.Bld
set VER_STR=%1

:: MACHINE DEPENDENT SETUP
:: Setup requires the installation of nsis: ( http://nsis.sourceforge.net/Main_Page )
set nsi="\Program Files (x86)\NSIS\makensis.exe"
 
:: Build the project using visual studio.  The configuration XP_Release is setup to 
:: use the vs2010 compiler.  Using this configuration enabled support for the Windows XP Platform.
msbuild ..\spl\spl.vcxproj /p:Configuration=XP_Release;Platform=Win32 /t:Rebuild

:: Invoke the installer script
%nsi% win_inst.nsi /DVER_STR=%VER_STR% 

:: Compress the installer executable
7z.exe a -tzip ..\build\strymon_lib_setup_%VER_STR%.zip ..\build\strymon_lib_setup_%VER_STR%.exe

