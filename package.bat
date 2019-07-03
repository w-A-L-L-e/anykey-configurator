
echo "compressing exe for distribution..."
windows-installer\upx.exe release\AnyKey.exe

echo "release\AnyKey.exe built successfull generating new installer..."
cd windows-installer
"c:\Program Files (x86)\NSIS\Bin\makensis.exe" anykey_installer.nsi
cd ..

echo "Configurator installer saved in windows-installer dir"

