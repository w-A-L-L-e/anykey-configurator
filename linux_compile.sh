#=============================================================================
# author        : Walter Schreppers
# filename      : mac_install.sh
# created       : 14/6/2020
# modified      :
# version       :
# copyright     : Walter Schreppers
# bugreport(log):
# description   : Compile and package helper script for Linux
#=============================================================================

sudo apt install qt5-default
cp AnyKey.pro_linux AnyKey.pro

echo "remove old version..."
rm AnyKey

qmake AnyKey.pro
make

echo "todo copy anykey_save and crd builds here"
# already copied using make gui in anykey_save_tool folder now!
#cp ~/bin/anykey_save AnyKey.app/Contents/MacOS/
#cp ~/bin/anykey_crd AnyKey.app/Contents/MacOS/

echo "done"

