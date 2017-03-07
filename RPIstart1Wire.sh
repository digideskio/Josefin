#!/bin/sh

### BEGIN INIT INFO
# Provides:          RPIstart1wire
# Required-Start:    $local_fs $syslog
# Required-Stop:     $local_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start OWFS at boot time
# Description:       Start OWFS at boot time
### END INIT INFO

# Raspberry PI (RPI) usb_regulartime used to handle LCD-display
		  /usr/bin/owserver -p 3001 -uall --usb_regulartime --allow_other 
      /usr/bin/owfs -s 3001 /mnt/1wire/
      /usr/bin/owhttpd -s 3001  -p 4001 
      
      
# Beagle Bone Black (BBB) usb_regulartime used to handle LCD-display
#		  /usr/bin/owserver -p 3001 -uall --usb_regulartime --allow_other 
#      /usr/bin/owfs -s 3001 /mnt/1wire/
#      /usr/bin/owhttpd -s 3001  -p 4001       



 

