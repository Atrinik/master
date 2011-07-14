; NSIS installer script to make Atrinik client win32 installer.
; To make the installer, place this script in your client directory and
; run `makensis client_installer.nsi`

!define CLIENT_VERSION "2.5"

; Installer name.
Name "Atrinik Client ${CLIENT_VERSION}"

; Installer filename.
OutFile "atrinik-client-${CLIENT_VERSION}.exe"

; Installer icon.
Icon "bitmaps\icon.ico"

; Default installation directory.
InstallDir "$PROGRAMFILES\Atrinik Client ${CLIENT_VERSION}"

; Registry key.
InstallDirRegKey HKLM "Software\Atrinik-Client-${CLIENT_VERSION}" "Install_Dir"

; Pages.
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

; Play nice with UAC in Vista and newer.
RequestExecutionLevel admin

; What to install.
Section "Client (required)"
  SectionIn RO

  SetOutPath $INSTDIR
  File "atrinik.exe"
  File "updater.exe"
  File "*.dll"
  File "COPYING"
  File "README"
  File "INSTALL"
  File "timidity.cfg"
  File "make_win32/tools/gunzip.exe"
  File "make_win32/tools/tar.exe"
  File "make_win32/tools/updater.bat"

  CreateDirectory $INSTDIR\bitmaps
  SetOutPath $INSTDIR\bitmaps
  File /r "bitmaps\*.*"

  CreateDirectory $INSTDIR\cache
  SetOutPath $INSTDIR\cache
  File /r "cache\*.*"

  CreateDirectory $INSTDIR\data
  SetOutPath $INSTDIR\data
  File /r "data\*.*"

  CreateDirectory $INSTDIR\fonts
  SetOutPath $INSTDIR\fonts
  File /r "fonts\*.*"

  CreateDirectory $INSTDIR\gfx_user
  SetOutPath $INSTDIR\gfx_user
  File /r "gfx_user\*.*"

  CreateDirectory $INSTDIR\media
  SetOutPath $INSTDIR\media
  File /r "media\*.*"

  CreateDirectory $INSTDIR\settings
  SetOutPath $INSTDIR\settings
  File /r "settings\*.*"

  CreateDirectory $INSTDIR\sfx
  SetOutPath $INSTDIR\sfx
  File /r "sfx\*.*"

  CreateDirectory $INSTDIR\srv_files
  SetOutPath $INSTDIR\srv_files
  File "srv_files\*.*"

  CreateDirectory $INSTDIR\timidity
  SetOutPath $INSTDIR\timidity
  File /r "timidity\*.*"

  SetOutPath $INSTDIR

  WriteRegStr HKLM "SOFTWARE\Atrinik-Client-${CLIENT_VERSION}" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}" "DisplayName" "Atrinik Client"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}" "DisplayVersion" "${CLIENT_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}" "DisplayIcon" "$INSTDIR\bitmaps\icon.ico"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}" "Publisher" "Atrinik Team"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}" "HelpLink" "http://www.atrinik.org"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

; Optional start menu shortcuts.
Section "Start Menu Shortcuts"
  SetShellVarContext all

  CreateDirectory "$SMPROGRAMS\Atrinik Client ${CLIENT_VERSION}"
  CreateShortCut "$SMPROGRAMS\Atrinik Client ${CLIENT_VERSION}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Atrinik Client ${CLIENT_VERSION}\Atrinik Client.lnk" "$INSTDIR\updater.exe" "" "$INSTDIR\bitmaps\icon.ico"
SectionEnd

; Optional desktop shortcut.
Section /o "Desktop Shortcut"
  SetShellVarContext all
  
  CreateShortCut "$DESKTOP\Atrinik Client.lnk" "$INSTDIR\updater.exe" "" "$INSTDIR\bitmaps\icon.ico"
SectionEnd

; Uninstaller.
Section "Uninstall"
  SetShellVarContext all

  ; Remove registry keys.
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Atrinik-Client-${CLIENT_VERSION}"
  DeleteRegKey HKLM "SOFTWARE\Atrinik-Client-${CLIENT_VERSION}"

  Delete /REBOOTOK "$DESKTOP\Atrinik Client.lnk"
  RMDir /r /REBOOTOK "$SMPROGRAMS\Atrinik Client ${CLIENT_VERSION}"
  RMDir /r /REBOOTOK "$INSTDIR"
SectionEnd
