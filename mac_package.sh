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

qmake AnyKey.pro_mac
make

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
echo "add_return=1;copy_protect=0;auto_type=0;advanced_settings=0;" > mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey.cfg
rm -f mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/devices.map
rm -f mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/license.json


echo "compress binaries for distribution..."
upx -9 mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/AnyKey
upx -9 mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey_crd
upx -9 mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey_save

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
read

echo "Making final image..."
hdiutil convert AnyKeyConfigurator.dmg -format UDZO -o AnyKeyConfigurator_$DATE.dmg

echo "done"


