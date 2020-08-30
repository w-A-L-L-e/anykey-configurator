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

# sudo apt install qt5-default
echo "remove old version..."
rm AnyKey

qmake AnyKey.pro_linux
make clean
make

echo "done"

