echo "Setting path to use mingw of Qt"
set PATH=C:\WinAVR-20100110\bin;C:\WinAVR-20100110\utils\bin;C:\Program Files (x86)\Intel\iCLS Client\;C:\Program Files\Intel\iCLS Client\;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\WINDOWS\System32\OpenSSH\;C:\Program Files\Git\cmd;C:\Qt\Qt5.6.3\Tools\mingw492_32\bin;C:\Qt\Static\5.6.3\bin;C:\Users\Walter Schreppers\AppData\Local\Microsoft\WindowsApps

echo "make clean, and run qmake"
qmake AnyKey.pro_windows
make -f Makefile.release clean

echo "compile again with make"
make -f Makefile.release

echo "compressing exe for distribution..."
windows-installer\upx.exe release\AnyKey.exe

echo "release\AnyKey.exe built successfull"