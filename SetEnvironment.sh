#!/bin/sh

# This file is started by root cron
# 

  echo `date +"%a %x %X"` "Setup environment for Mqtt"
  export PKG_CONFIG_PATH=/home/pi/Josefin/libbyteport/staging/lib/pkg-config
  pkg-config --cflags --libs byteport   

 exit 0 
