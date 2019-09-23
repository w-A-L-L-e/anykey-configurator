# AnyKey Configurator

This repository will contain the sources to the configurator show in the kickstarter edition on our campaign pages.
https://www.kickstarter.com/projects/715415099/anykey-the-usb-password-key

After installing first enter your activation code (this needs to be correct before you can proceed):

<img width="669" alt="Screen Shot 2019-06-22 at 16 44 32" src="https://user-images.githubusercontent.com/710803/59965360-26d33480-950d-11e9-9503-f92092bca22f.png">


The initial screen looks like this. Here you save your wanted password and go to advanced settings if you want Challenge Response and copy protect:

<img width="634" alt="anykey_configurator_june_part2" src="https://user-images.githubusercontent.com/710803/59965310-75340380-950c-11e9-9d31-b38a8a0aad76.png">

In advanced settings you can also alter your keyboard layout, set delays and do other various tweaks to suit your needs:
<img width="655" alt="anykey_configurator_june_part4" src="https://user-images.githubusercontent.com/710803/59965311-75cc9a00-950c-11e9-9cd7-c8bb33f3c54f.png">

This allows challenge response sha256 auto typing. Configuring and changing device salt.
Saving password to anykey. Auto type on insert with challenge response. Type after doing challenge response.
Changing keyboard layout and device delays (keypress and initial delays can be altered to fit your needs).

Master reset allows to factory reset a device if you lost the salt and linked this to a pc.
Upgrade salt is allowed to unlink from a foreign pc. Devices map file is managed by anykey_crd and anykey_save tools seperately.



Windows version is now also completed with installer added (we just need to sign the executable for easier distribution)

<img width="414" alt="anykey_windows_june_2019_advanced" src="https://user-images.githubusercontent.com/710803/59970343-94a64d00-955b-11e9-8449-94b602a09505.PNG">



