;=============================================================================
;  author        : Walter Schreppers
;  filename      : package.bat
;  created       : 28/6/2019
;  modified      :
;  version       :
;  copyright     : Walter Schreppers
;  bugreport(log):
;  description   : Generate windows 10 installer and compress binaries
;=============================================================================

@echo off
cls

echo copy latest save and crd tools from anykey_save_tool 
copy ..\anykey_save_tool\anykey_save.exe release\anykey_save.exe
copy ..\anykey_save_tool\anykey_crd.exe release\anykey_crd.exe


; # echo Verify codeesign anykey_save
; # "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\signtool.exe" verify /v /pa release\anykey_save.exe
; # "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\signtool.exe" verify /v /pa release\anykey_crd.exe
; # "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\signtool.exe" verify /v /pa release\AnyKey.exe


echo Compressing executables for distribution...
windows-installer\upx.exe release\AnyKey.exe
windows-installer\upx.exe release\anykey_save.exe
windows-installer\upx.exe release\anykey_crd.exe

echo Code signing executables...
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\signtool.exe" sign /f anykey_certificate.p12 /p droors1281 /tr http://timestamp.comodoca.com/rfc3161 /fd sha256 /td sha256 release\AnyKey.exe
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\signtool.exe" sign /f anykey_certificate.p12 /p droors1281 /tr http://timestamp.comodoca.com/rfc3161 /fd sha256 /td sha256 release\anykey_save.exe
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\signtool.exe" sign /f anykey_certificate.p12 /p droors1281 /tr http://timestamp.comodoca.com/rfc3161 /fd sha256 /td sha256 release\anykey_crd.exe


echo Making installer for release\AnyKey.exe ...
cd windows-installer
"c:\Program Files (x86)\NSIS\Bin\makensis.exe" anykey_installer.nsi
cd ..

echo Code signing installer...
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\signtool.exe" sign /f anykey_certificate.p12 /p droors1281 /tr http://timestamp.comodoca.com/rfc3161 /fd sha256 /td sha256 "windows-installer\AnyKey Installer.exe"

echo Configurator installer saved in 'windows-installer' dir

