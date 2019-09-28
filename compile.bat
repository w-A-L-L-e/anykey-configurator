:: =============================================================================
::  author        : Walter Schreppers
::  filename      : compile.bat
::  created       : 1/3/2018
::  modified      :
::  version       :
::  copyright     : Walter Schreppers
::  bugreport(log):
::  description   : Compile batch file for MinGW windows 10
::=============================================================================

@echo off
cls
echo Setting path to use mingw of Qt
set PATH=C:\WinAVR-20100110\bin;C:\WinAVR-20100110\utils\bin;C:\Program Files (x86)\Intel\iCLS Client\;C:\Program Files\Intel\iCLS Client\;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\WINDOWS\System32\OpenSSH\;C:\Program Files\Git\cmd;C:\Qt\Qt5.6.3\Tools\mingw492_32\bin;C:\Qt\Static\5.6.3\bin;C:\Users\Walter Schreppers\AppData\Local\Microsoft\WindowsApps

echo copying .pro_windows to .pro file
copy AnyKey.pro_windows AnyKey.pro

echo make clean, and run qmake
qmake AnyKey.pro_windows
make -f Makefile.release clean

echo compile release configurator...
make -f Makefile.release
