Instruktioner
cd /home
Sätt chown -R root *

Sätta upp ny image/distribution
apt-get owfs
apt-get tightvncserver
apt-get samba

Installera Gordons libwiringpi
http://wiringpi.com/download-and-install/

Sätta upp Byteportanrop via Python
 git clone https://github.com/iGW/byteport-api/
 $ cd byteport-api/python
 $ sudo pip install -r requirements.txt
 $ sudo python ./setup.py install

Installera samba, sätt lösen för user pi
sudo smbpasswd -a pi
editera /etc/samba/smb.conf och lägg till dom kataloger du vill nå via samba (på pc)


Installera service 
sudo update-rd.d JosefinStartScript default
insserv mydaemon

Denna anropar StartJosefin som ligger i Josefin katalogen. Kolla alla pathar...
Kommer starta OWFS, Tightvncserver, Josefin

Check etc/init.d/Josefin-rcStartupScript

Start OWFS on Beaglebone black
sudo /usr/bin/owfs -uall --usb_regulartime --allow_other /mnt/1wire/

Anm�ls till /lib/systemd/system     Googla p� webben http://mattrichardson.com/BeagleBone-System-Services/
Ladda in curl paketet
   för BBB gör du sudo opkg curl
	 libcurl4-gnutls-dev

	 
	Du måste även sätta upp Makefilen för curl 
	  LDLIBS += -lcurl # Support library for curl
		
		Felsökning mm
		start etch0: dhclient eth0
		