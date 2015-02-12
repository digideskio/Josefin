Instruktioner

Test att detta laddas ned
BBB
Starta OWFS samt Josefin genom dom tv� startup scripten.
OWFSStartScript.service
JosefinStartScript.service

Start OWFS on Beaglebone black
sudo /opt/owfs/bin/owfs -uall --usb_regulartime --allow_other /mnt/1wire/

Anm�ls till /lib/systemd/system     Googla p� webben http://mattrichardson.com/BeagleBone-System-Services/
Ladda in curl paketet
   för BBB gör du sudo opkg curl

	 
	Du måste även sätta upp Makefilen för curl 
	  LDLIBS += -lcurl # Support library for curl
		