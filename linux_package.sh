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

qmake AnyKey.pro_linux
make

echo "copy new version..."
cp ../anykey_save_tool/anykey_save linux-installer/anykey_configurator-2.3.3/
cp ../anykey_save_tool/anykey_crd linux-installer/anykey_configurator-2.3.3/
cp AnyKey linux-installer/anykey_configurator-2.3.3/anykey_cnf

echo "cleanup..."

# config file is messing up our signing
# echo "add_return=1;copy_protect=0;auto_type=0;advanced_settings=0;" > mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey.cfg

rm -f linux-installer/anykey_configurator-2.3.3/anykey.cfg
rm -f linux-installer/anykey_configurator-2.3.3/devices.map
rm -f linux-installer/anykey_configurator-2.3.3/license.json

echo "Making final tarball..."
cd linux-installer
tar -czvf anykey_configurator-2.3.3.tar.gz anykey_configurator-2.3.3

cd ..

echo "done"


