#!/usr/bin/python
from byteport.clients import ByteportHttpClient

client = ByteportHttpClient( 'GoldenSpace', 'b43cb5709b37ff3125195b54', 'JosefinSim', initial_heartbeat=False,)

# NOTE: This will block the current thread!
client.poll_directory_and_store_upon_content_change('/tmp/byteport', 'JosefinShip', poll_interval=10)
