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
VERSION=2.3.3
rm -rf linux-installer/anykey_configurator-$VERSION
mkdir -p linux-installer/anykey_configurator-$VERSION

cp linux-installer/install.sh linux-installer/anykey_configurator-$VERSION/
cp ../anykey_save_tool/anykey_save linux-installer/anykey_configurator-$VERSION/
cp ../anykey_save_tool/anykey_crd linux-installer/anykey_configurator-$VERSION/
cp AnyKey linux-installer/anykey_configurator-$VERSION/anykey_cfg


# config file is messing up our signing
# echo "add_return=1;copy_protect=0;auto_type=0;advanced_settings=0;" > mac-installer/AnyKeyConfigurator/AnyKey.app/Contents/MacOS/anykey.cfg


echo "Making final tarball..."
cd linux-installer
tar -czvf anykey_configurator-$VERSION.tar.gz anykey_configurator-$VERSION
cd ..

echo "cleanup..."
rm -rf linux-installer/anykey_configurator-$VERSION

echo "done"


