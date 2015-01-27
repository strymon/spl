OutFile $%INST_EXE%

installDir $PROGRAMFILES\Strymon

Icon .\win\spl.ico

Section
  messageBox MB_YESNO "Is it OK to install the Strymon Librarian?" IDYES true IDNO alldone
  true:
   
  SetOutPath $INSTDIR\Librarian

  # Delete everything and start over.
  # This will have issues if the application is still running
  Delete "Qt5Core.dll"
  Delete "D3DCompiler_43.dll"
  Delete "D3DCompiler_46.dll"
  Delete "D3DCompiler_47.dll"
  Delete "Qt5Gui.dll"
  Delete "Qt5Network.dll"
  Delete "Qt5Widgets.dll"
  Delete "icudt49.dll"
  Delete "icuin49.dll"
  Delete "icuuc49.dll"
  Delete "icudt51.dll"
  Delete "icuin51.dll"
  Delete "icuuc51.dll"
  Delete "icudt53.dll"
  Delete "icuin53.dll"
  Delete "icuuc53.dll"
  Delete "libEGL.dll"
  Delete "libGLESv2.dll"
  Delete "platforms\qwindows.dll"
  Delete "msvcp100.dll"
  Delete "msvcr100.dll"
  Delete "msvcp110.dll"
  Delete "msvcr110.dll"
  Delete "msvcp120.dll"
  Delete "msvcr120.dll"
  Delete "spl.exe"
  Delete "lgpl.txt"
  Delete "uninstall.exe"


# Copy the Qt platform file 
CreateDirectory $INSTDIR\Librarian\platforms 
SetOutPath $INSTDIR\Librarian\platforms 
File ..\..\build\redist\platforms\qwindows.dll  
   
# Bring in the runtime files
!include ..\..\build\redist.ins

  SetOutPath $INSTDIR\Librarian

  # Main Executable
  File $%APP_EXE%

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
  SetOutPath $INSTDIR

  !include ..\..\build\redist_remove.ins

  Delete "spl.exe" 
  Delete "lgpl.txt"
  
  ; Delete the program folders  
  SetOutPath $TEMP
  RMDir $INSTDIR\platforms
  
  Delete $INSTDIR\uninstall.exe
  RMDir $INSTDIR
  RMDir $INSTDIR\..
  ; If this is empty, it shall be deleted
  RMDir $PROGRAMFILES\Strymon
uninst_alldone:

SectionEnd
