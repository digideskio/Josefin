#!/bin/sh


echo "Start OWFS"
owfs -uall --usb_regulartime --allow_other /mnt/1wire/
#sudo /opt/owfs/bin/owfs -uall --usb_regulartime --allow_other /mnt/1wire/

# usb_regulartime used to handle LCD-display
echo "Start Application Josefin"
/home/root/Josefin/Josefin&

# Carry out specific functions when asked to by the system
 
 
#
 exit 0 
