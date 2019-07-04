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

echo "remove old version..."
rm -rf /Applications/AnyKey.app

echo "copy new version..."
cp -r AnyKey.app /Applications/
#also compress it ;)
# brew install upx

echo "compress it..."
upx -9 /Applications/AnyKey.app/Contents/MacOS/AnyKey
upx -9 /Applications/AnyKey.app/Contents/MacOS/anykey_crd
upx -9 /Applications/AnyKey.app/Contents/MacOS/anykey_save

#echo "update launchctl daemon and copy plist file..."
#cp mac_autostart_install/org.anykey.configurator.plist ~/Library/LaunchAgents/org.anykey.configurator.plist
#launchctl unload ~/Library/LaunchAgents/org.anykey.configurator.plist
#launchctl load ~/Library/LaunchAgents/org.anykey.configurator.plist

echo "done"


