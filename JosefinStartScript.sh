#!/bin/sh

### BEGIN INIT INFO
# Provides:          JosefinStartScript
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
# AG 28 Jan 2014 

echo "Start OWFS"
sudo /opt/owfs/bin/owfs -uall --usb_regulartime --allow_other /mnt/1wire/
# usb_regulartime used to handle LCD-display

# Carry out specific functions when asked to by the system
 case "$1" in
  start)
    echo "Starting Josefin"
    # run application you want to start
    /home/pi/Program/Code/Josefin&
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
