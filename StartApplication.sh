#!/bin/sh

# This file is started by root cron
# 
# crontab -e
# add line: @reboot /home/pi/Josefin/StartApplication.sh > /home/pi/Josefin/logfile.txt 2>&1
#
# If you want a command to always run, put it here
# AG 30 Jan 2016 
# in the Josefin folder
#echo "Start OWFS"
# sudo /opt/owfs/bin/owfs -uall --usb_regulartime --allow_other /mnt/1wire/
# usb_regulartime used to handle LCD-display

# Uncomment to set type of computer
# Raspberry pi
#CPUType=1  
 
# BeagleBone Black
CPUType=1

	if [ $CPUType -eq 1 ]; then
    echo "Start OWFS-Raspberry"
# usb_regulartime used to handle LCD-display
		  /usr/bin/owserver -p 3001 -uall --usb_regulartime --allow_other 
      /usr/bin/owfs -s 3001 /mnt/1wire/
      /usr/bin/owhttpd -s 3001  -p 4001 

		
		# Create Byteport directory, if not exists
    if [ ! -d /tmp/ByteportReports ]; then
      mkdir /tmp/ByteportReports
			echo "Create /tmp/ByteportReport directory"
    fi
		
    echo "Starting Josefin script"
    # run application you want to start
     /home/pi/Josefin/Josefin&
#		> /home/pi/logfile.txt 2>&1
    echo "Start Byteport reporting"
    python /home/pi/Josefin/ByteportReport.py&

    
    #export USER=pi
    echo "Starting Tightvnc"
    su - pi -c '/usr/bin/tightvncserver :1'  

  
  else # Beagle bone defined
	  echo "Start OWFS Beagle Bone Black"
# usb_regulartime used to handle LCD-display
	  sudo /usr/bin/owfs -uall --usb_regulartime --allow_other /mnt/1wire/
		
# Start Tightvncserver		
		sudo /usr/bin/tightvncserver :1
		
# Create Byteport directory, if not exists
    if [ ! -d /tmp/ByteportReports ]; then
      mkdir /tmp/ByteportReports
			echo "Create /tmp/ByteportReport directory"
    fi
		
    echo "Starting Josefin script"
    # run application you want to start
    /home/arne/Josefin/Josefin&
#		> /home/arne/logfile.txt 2>&1

    echo "Start Byteport reporting"
    python /home/arne/Josefin/ByteportReport.py&
  fi 
  
#
 exit 0 
