#!/bin/sh

### BEGIN INIT INFO
# Provides:          Josefin-rcStartScript
# Required-Start:    $network $local_fs $remote_fs
# Required-Stop:     $network $local_fs $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Should-Start:      slapd cups
# Should-Stop:       slapd cups
# Short-Description: start Josefin owfs samba restart
### END INIT INFO

#
# If you want a command to always run, put it here
# File normally resides in /etc/init.d/
# Set chmod 777 on file, test by /etc/init.d/Josefin-rcStartScript  start
# AG 28 Jan 2014 
# AG 10 Maj-14 I have moved this to its own startup script that resides
# in the Josefin folder
#echo "Start OWFS"
# sudo /opt/owfs/bin/owfs -uall --usb_regulartime --allow_other /mnt/1wire/
# usb_regulartime used to handle LCD-display

# Carry out specific functions when asked to by the system

# Uncomment to set type of computer
# Raspberry pi
#CPUType=1  
 
# BeagleBone Black
CPUType=1

 case "$1" in
  start)
	if [ $CPUType -eq 1 ]; then
    echo "Start OWFS-Raspberry"
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
     /home/pi/Josefin/Josefin&
#		> /home/pi/logfile.txt 2>&1
    echo "Start Byteport reporting"
    python /home/pi/Josefin/ByteportReport.py&

  else
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
     ;;
		 
  stop)
    echo "Stopping Josefin"
    # kill application you want to stop
    killall Josefin&
    ;;
  *)
    echo "Usage: /etc/init.d/JosefinScript {start|stop}"
    exit 1
    ;;
 esac
 
#
 exit 0 
