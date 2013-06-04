# This is an NSIS install script.  
# To learn more about NSIS and the tools you need to use this file
# please see: http://nsis.sourceforge.net
#
# NOTE: There are other dependencies other than a proper NSIS installation
# 1) Script assumes that spl.exe is located ..\build\bin\Win32\XP_Release

# INPUT ARG: Invoke this script by passing the var VER_STR in the format Maj.Min.Inc.Bld
OutFile "..\build\strymon_lib_setup_$%VER_STR%.exe"

installDir $PROGRAMFILES\Strymon

Icon .\win\spl.ico

Section
  messageBox MB_YESNO "Is it OK to install the Strymon Librarian?" IDYES true IDNO alldone
  true:
   
  SetOutPath $INSTDIR\Librarian

  # Qt libs
  File .\win\redist\D3DCompiler_43.dll
  File .\win\redist\Qt5Core.dll
  File .\win\redist\Qt5Gui.dll
  File .\win\redist\Qt5Network.dll
  File .\win\redist\Qt5Widgets.dll
  File .\win\redist\icudt49.dll
  File .\win\redist\icuin49.dll
  File .\win\redist\icuuc49.dll
  File .\win\redist\libEGL.dll
  File .\win\redist\libGLESv2.dll
  File .\lgpl.txt

  # Copy the Qt platform file
  CreateDirectory $INSTDIR\Librarian\platforms
  SetOutPath $INSTDIR\Librarian\platforms
  File .\win\redist\platforms\qwindows.dll

  # The C++ Runtime
  SetOutPath $INSTDIR\Librarian
  File .\win\redist\msvcp100.dll
  File .\win\redist\msvcr100.dll

  # Main Executable
  File ..\build\bin\Win32\XP_Release\spl.exe

  # Create some shortcuts
  CreateShortCut "$SMPROGRAMS\Strymon\Librarian.lnk" "$INSTDIR\Librarian\spl.exe" "" "$INSTDIR\Librarian\spl.exe" 0
  CreateShortCut "$SMPROGRAMS\Strymon\Uninstall_Librarian.lnk" "$INSTDIR\Librarian\uninstall.exe" "" "$INSTDIR\Librarian\uninstall.exe" 0
  
  # Write installer
  WriteUninstaller $INSTDIR\Librarian\uninstall.exe

alldone:

SectionEnd

Section "Uninstall"

  messageBox MB_YESNO "Uninstall Strymon Librarian?" IDYES do_uninst IDNO uninst_alldone
  do_uninst:

  ; Delete the start menu links
  SetShellVarContext all
  Delete "$SMPROGRAMS\Strymon\Librarian.lnk"
  Delete "$SMPROGRAMS\Strymon\Uninstall_Librarian.lnk"
  RMDir  "$SMPROGRAMS\Strymon"

  ; Delete the application data
  Delete "$APPDATA_COMMON_FOLDER\Strymon\spl"

  ; Delete each file
  SetOutPath "$INSTDIR"

  Delete "D3DCompiler_43.dll"
  Delete "Qt5Core.dll"
  Delete "Qt5Gui.dll"
  Delete "Qt5Network.dll"
  Delete "Qt5Widgets.dll"
  Delete "icudt49.dll"
  Delete "icuin49.dll"
  Delete "icuuc49.dll"
  Delete "libEGL.dll"
  Delete "libGLESv2.dll"
  Delete "platforms\qwindows.dll"
  Delete "msvcp100.dll"
  Delete "msvcr100.dll"
  Delete "spl.exe"
  Delete "uninstall.exe"
  
  ; Delete the program folders  
  SetOutPath "$TEMP"
  RMDir $PROGRAMFILES\Strymon\Librarian\platforms
  RMDir $PROGRAMFILES\Strymon\Librarian

  ; If this is empty, it shall be deleted
  RMDir $PROGRAMFILES\Strymon
uninst_alldone:

SectionEnd
