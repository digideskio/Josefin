Instruktioner
cd /home
Sätt chown -R root *

Sätta upp ny image/distribution
apt-get owfs
https://wiki.m.nu/index.php/OWFS_p%C3%A5_Rasperry_Pi#Se_till_s.C3.A5_att_OWFS_startar_automatiskt_vid_boot
Install [RPI/BB]start1wire in /etc/init.d and do update-rc.d...

apt-get tightvncserver
apt-get samba

Samba: Setup user and add user with password
adduser xxx
sambpasswd -a xxx

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
    
 Addera support för Byteport libbyteport
Lägg in i .bashrc

# AG 20161204 Add Env for mqtt (Axel Alatalo)
export PKG_CONFIG_PATH=/home/pi/Josefin/libbyteport/staging/lib/pkg-config
pkg-config --cflags --libs byteport   
 
    
		