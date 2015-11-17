#!/usr/bin/python
from byteport.clients import ByteportHttpClient
import socket
import fcntl
import struct

NAMESPACE = "GoldenSpace"
API_KEY = "b43cb5709b37ff3125195b54"
POLL_INTERVAL = 30
SHIP_HW_ADDR = "b8:27:eb:a8:39:85"	 # <====== FIXA 
uid = "JosefinShip" # Set default value

'''
		Returns the MAC for a given network interface name as given by ifconfig etc.
'''
def getHwAddr(ifname):
		s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		info = fcntl.ioctl(s.fileno(), 0x8927, struct.pack('256s', ifname[:15]))
		return ''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1]


'''
		Python main method, execution starts here:
'''
if __name__ == '__main__':

		try:
				current_mac = getHwAddr('wlan0')
		except Exception:
				current_mac = getHwAddr('eth0')
		else:
				current_mac = None	# <== whatever, just so the compare below fails and returns a "Sim" 
				
		print(current_mac)		
		if current_mac == SHIP_HW_ADDR:
				uid = 'JosefinShip'
				print ("Josefin Ship set")
		else:
				uid = 'JosefinSim'
				print ("Josefin SIM set")
		client = ByteportHttpClient(NAMESPACE, API_KEY, uid, initial_heartbeat=False)

		# NOTE: This will block the current thread!
		client.poll_directory_and_store_upon_content_change('/tmp/byteport', uid, poll_interval=POLL_INTERVAL)
