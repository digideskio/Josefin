#!/bin/sh

echo "Start Tightvncserver"
sudo /usr/bin/tightvncserver :1

echo "Start OWFS"
sudo /usr/bin/owfs -uall --usb_regulartime --allow_other /mnt/1wire/
# usb_regulartime used to handle LCD-display


echo "Starting Josefin script"
    # run application you want to start
    /home/pi/Josefin/Josefin&
#		> /home/pi/logfile.txt 2>&1

echo "Start Byteport reporting"
python /home/pi/Josefin/ByteportReport.py&

 
 
#
 exit 0 
