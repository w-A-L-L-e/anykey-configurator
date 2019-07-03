; anykey_installer.nsi
; Installs configurator, anykey_save and anykey_crd tools +
; TODO: add registry items for autostartup after reboots
;--------------------------------
!include x64.nsh

; The name of the installer
Name "AnyKey"
BrandingText "AnyKey Configurator v2.3.0"

; The file to write
OutFile "AnyKey Installer.exe"

Icon "..\anykey_app_logo.ico"
; UninstallIcon "..."


; The default installation directory
InstallDir $PROGRAMFILES\AnyKey

; Registry key to check for directory  
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\AnyKey" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "AnyKey configurator(required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\release\AnyKey.exe"
  File "..\release\anykey_save.exe"
  File "..\release\anykey_crd.exe"
  
  ${If} ${RunningX64}
	SetRegView 64
  ${Else}
	SetRegView 32
  ${EndIf}
  
  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\AnyKey" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "AnyKey" '$INSTDIR\AnyKey.exe -minimized' 

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AnyKey" "DisplayName" "AnyKey configurator"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AnyKey" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AnyKey" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AnyKey" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"
  ${If} ${RunningX64}
  	CreateDirectory "$SMPROGRAMS\AnyKey"
    CreateShortcut "$SMPROGRAMS\AnyKey\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
	CreateShortcut "$SMPROGRAMS\AnyKey\AnyKey Configurator.lnk" "$INSTDIR\AnyKey.exe" "" "$INSTDIR\AnyKey.exe" 0
    CreateShortcut "$SMPROGRAMS\AnyKey Configurator.lnk" "$INSTDIR\AnyKey.exe" "" "$INSTDIR\AnyKey.exe" 0
  ${Else}   
	CreateDirectory "$SMPROGRAMS\AnyKey"
    CreateShortcut "$SMPROGRAMS\AnyKey\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
    CreateShortcut "$SMPROGRAMS\AnyKey\AnyKey Configurator.lnk" "$INSTDIR\AnyKey.exe" "" "$INSTDIR\AnyKey.exe" 0
  ${EndIf}
SectionEnd

Function .oninstsuccess   
  Exec "$INSTDIR\AnyKey.exe"   
FunctionEnd


;--------------------------------
; Uninstaller

Section "Uninstall"
  ${If} ${RunningX64}
	SetRegView 64
  ${Else}
	SetRegView 32
  ${EndIf}

  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AnyKey"
  DeleteRegKey HKLM "SOFTWARE\AnyKey"
  DeleteRegValue HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "AnyKey"

  ; Remove files and uninstaller
  Delete $INSTDIR\AnyKey.exe
  Delete $INSTDIR\anykey_save.exe
  Delete $INSTDIR\anykey_crd.exe
  Delete $INSTDIR\license.json
  Delete $INSTDIR\anykey.cfg
  Delete $INSTDIR\devices.map
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\AnyKey\*.*"
  Delete "$SMPROGRAMS\AnyKey Configurator.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\AnyKey"
  RMDir "$INSTDIR"

SectionEnd
