#=============================================================================
# author        : Walter Schreppers
# filename      : mac_install.sh
# created       : 1/3/2018
# modified      :
# version       :
# copyright     : Walter Schreppers
# bugreport(log):
# description   : Compile and package helper script for Mac OS
#=============================================================================

#qmake AnyKey.pro_mac
#make
./mac_compile.sh

# already copied using make gui in anykey_save_tool folder now!
#cp ~/bin/anykey_save AnyKey.app/Contents/MacOS/
#cp ~/bin/anykey_crd AnyKey.app/Contents/MacOS/
#echo "remove old version..."
#rm -rf /Applications/AnyKey.app

echo "create new empty dir for dmg generation..."
mkdir -p mac-installer/AnyKeyConfigurator
rm -rf mac-installer/AnykeyConfigurator
mkdir -p mac-installer/AnyKeyConfigurator

echo "copy new version..."
cp -r AnyKey.app mac-installer/AnyKeyConfigurator/

echo "cleanup..."

# config file is messing up our signing
echo "add_return=1;copy_protect=0;auto_type=0;advanced_settings=0;" > mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey.cfg
# icns file was offending, find . -type f -name '*.icns' -exec xattr -c {} \; fixed it
rm -f mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey.cfg
rm -f mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/devices.map
rm -f mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/license.json

echo "skipping upx compress as it fails on MacOS Big Sur"
#echo "compress binaries for distribution..."
#upx -9 mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/AnyKey
#upx -9 mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey_crd
#upx -9 mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey_save

# you download and open your .cer file developerID_application.cer after you open it, open xcode
# then just export all certificates by going into accounts and clicking + sign, I chose developer id application
# then export it by right clicking. Then you can use this with the full name to sign (ow yes also open the .p12 file first, it asks
# password and use the same when signing) : more details here https://help.apple.com/xcode/mac/current/#/dev154b28f09?sub=dev23755c6c6

echo "codesign binaries..."

#thes use the comodo certificate that we also have on windows, but it basically does not work at all (you have to right-click in order to install)
#codesign -s "AnyKey" mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/AnyKey
#codesign -s "AnyKey" mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey_save
#codesign -s "AnyKey" mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey_crd

# mac os developer certificate for 99$ yeah that's becoming a cash-cow for apple isn't it...
codesign -s "Developer ID Application: Walter Schreppers (X2ZBWYWFFA)" mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey_save
codesign -s "Developer ID Application: Walter Schreppers (X2ZBWYWFFA)" mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey_crd
codesign -s "Developer ID Application: Walter Schreppers (X2ZBWYWFFA)" --deep --force mac-installer/AnyKeyConfigurator/AnyKey.app


echo "create .dmg image"
cd mac-installer/AnyKeyConfigurator
ln -s /Applications Applications
cd ..

# UDRW - UDIF read/write image -> use to change icon size
# UDRO - UDIF read-only image
# UDCO - UDIF ADC-compressed image
# UDZO - UDIF zlib-compressed image -> final image
# ULFO - UDIF lzfse-compressed image (OS X 10.11+ only)
# UDBZ - UDIF bzip2-compressed image (Mac OS X 10.4+ only)
hdiutil create -volname "AnyKey Configurator"  -srcfolder AnyKeyConfigurator -ov -format UDRW AnyKeyConfigurator.dmg
open .

DATE=`date "+%m-%d-%Y"`
echo "Now change icon sizes before making final image..."
echo "Open AnykeyConfigurator.dmg go inside and press APPLE+j key, set icons to larger size and default to icon view"
read

#rm AnyKeyInstaller.dmg

echo "Making final image..."
hdiutil convert AnyKeyConfigurator.dmg -format UDZO -o AnyKeyInstaller_$DATE.dmg
echo "Now codesign the installer dmg also"
#codesign -s "AnyKey" AnyKeyInstaller_$DATE.dmg
codesign -s "Developer ID Application: Walter Schreppers (X2ZBWYWFFA)" AnyKeyInstaller_$DATE.dmg

echo "removing temp dmg..."
rm AnyKeyConfigurator.dmg
cd ..

echo "done"


