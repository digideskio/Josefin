import os
import time
import sys
import socket
import logging
import subprocess
import re
from byteport.factories import byteport_client_from_simple_argv


def collect_load_data(byteport_client, interval_sec=60):

    while True:
        loadavg = os.getloadavg()

        unix_stats = dict()
        unix_stats['la1'] = loadavg[0]
        unix_stats['la5'] = loadavg[1]
        unix_stats['la15'] = loadavg[2]

        try:
            #before = time.time()
            byteport_client.store(unix_stats)
            #print "It took %s seconds" % (time.time() - before)
        except Exception as e:
            # Catch and logg all errors and try again later is OK in this kind of use case
            logging.error(u'Error during Byteport API call: %s' % e)
        time.sleep(interval_sec)

'''
Simple script that collects three load figures from the system and uses the Byteport client
API class to post them to api.byteport.se.
'''
if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    logging.debug('====>>> Note DEBUG log level is enabled by default in this example client.')

    client = byteport_client_from_simple_argv()

    # Log anything by sending a dictionary to the store() method
    try:
        git_version = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).strip()
        client.store({'git_version': git_version})
    except Exception as e:
        logging.warn("Failed to obtain git version.")

    # Simple way to log text data around the system
    client.log("Byteport Python Example client started!", level='info')

    # Continous logging example
    collect_load_data(client, 1)
