import sys
import re
import socket
import logging
from optparse import OptionParser

from http_clients import ByteportHttpClient

def byteport_client_from_optparse():

    usage = "usage: %prog [options]"
    parser = OptionParser(usage)
    parser.add_option("-n", "--namespace", dest="namespace", help="Namespace name", metavar="NAMESPACE")
    parser.add_option("-k", "--api_key", dest="api_key", help="Namespace API key", metavar="API_KEY")
    parser.add_option("-d", "--device_uid", dest="device_uid", help="Device UID", metavar="DEVICE_UID")
    parser.add_option("-p", "--proxy_port", dest="proxy_port", help="SOCKS5 Proxy port", metavar="PROXY_PORT")

    (options, args) = parser.parse_args()

    if options.namespace is None or options.api_key is None:
        return None, parser

    if options.device_uid is None:
        hostname = socket.gethostname()
        device_uid = re.sub('[^0-9a-zA-Z]+', '_', hostname)
    else:
        device_uid = options.device_uid

    # Create client object
    client = ByteportHttpClient(
        options.namespace, options.api_key, device_uid, proxy_port=options.proxy_port)

    return client, parser


def byteport_client_from_simple_argv():
    # Will return a Client based on the argv given during start of the python program.
    # Useful for any kind of nifty utils started from the shell.
    #
    # sys.argv[n]
    # 0   - name of program
    # 1   - namespace
    # 2   - api key
    # 3   - (optional) device uid if other than 'hostname'
    # 4   - (optional) SOCKS5 proxy port
    #
    # returns a ByteportHttpPostClient() object

    if len(sys.argv) < 3:
        print "Usage: %s <namespace name> <namespace api write key> [device uid] [proxy port]" % sys.argv[0]
        exit(1)

    namespace = sys.argv[1]
    namespace_api_write_key = sys.argv[2]

    try:
        device_uid = sys.argv[3]
    except Exception:
        hostname = socket.gethostname()
        device_uid = re.sub('[^0-9a-zA-Z]+', '_', hostname)

    try:
        proxy_port = int(sys.argv[4])
    except Exception:
        proxy_port = None

    logging.info("Namespace  :   %s" % namespace)
    logging.info("API key    :   %s" % namespace_api_write_key)
    logging.info("Device UID :   %s" % device_uid)
    logging.info("Proxy port :   %s" % proxy_port)

    # Create client object
    return ByteportHttpClient(namespace, namespace_api_write_key, device_uid, proxy_port=proxy_port)


