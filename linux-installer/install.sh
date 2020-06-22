#!/bin/bash
clear
echo "AnyKey linux installer"
echo "======================"

mkdir -p /etc/anykey

if [ ! -e "/etc/anykey/license.json" ]
then
  if ! [ -x "$(command -v wget)" ]; then
    echo "Error wget is not installed. It's needed to fetch your license.json"
    echo "run this first and retry: sudo apt-get install wget"
    exit 1
  fi

  echo "Enter your activation code:"
  read activation_code

  wget --quiet http://anykey.shop/activate/$activation_code.json
  
  if [ ! -e "$activation_code.json" ]
  then
    echo "Invalid activation code please re-run installer"
    exit 1
  else
    mv $activation_code.json license.json
    cp license.json /etc/anykey/
  fi
fi

echo "Copying binaries to /usr/bin/..."
cp anykey_save /usr/bin/
cp anykey_crd /usr/bin/
cp anykey_cfg /usr/bin/

echo "Adding 50-anykey.rules for device mapping"
echo 'KERNEL=="ttyACM[0-9]*", SUBSYSTEM=="tty", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="8036", ATTRS{product}=="AnyKey" SYMLINK="ttyAnyKey", MODE="0666"' > /etc/udev/rules.d/50-anykey.rules

echo "Now restarting udev..."
# sudo udevadm control --reload-rules && sudo service udev restart && sudo udevadm trigger
udevadm control --reload-rules && service udev restart && udevadm trigger

echo "Making devices.map and anykey.cfg writeable in /etc/anykey. For higher security chown/chmod it for your specific users"
touch /etc/anykey/devices.map
touch /etc/anykey/anykey.cfg
chmod 666 /etc/anykey/devices.map
chmod 666 /etc/anykey/anykey.cfg

echo "Installing Qt libraries"
apt install qt5-default

echo "All done"
echo "The cli-tools anykey_save, anykey_crd and the gui configurator anykey_cfg are installed in /usr/bin"
echo "Type anykey_cfg to start configuring your AnyKey"
