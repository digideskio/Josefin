import logging
import time
from byteport.factories import byteport_client_from_optparse

#
# Suggested improvement:
# https://pypi.python.org/pypi/watchdog/0.5.4
#

'''

Example utility to poll a (small) file that contain text-data (UTF-8) and whenever changes are detected,
store it to Byteport. It will not base64-encode the data found in the file.

'''
if __name__ == "__main__":

    MAX_FILE_SIZE = 128

    logging.basicConfig(level=logging.INFO)

    client, parser = byteport_client_from_optparse()

    parser.add_option("-f", "--file_name", dest="file_name", help="File to poll for changes", metavar="FILE_NAME")
    parser.add_option("-m", "--field_name", dest="field_name", help="Field name to store data under", metavar="FIELD_NAME")

    (options, args) = parser.parse_args()

    if client is None or options.file_name is None:
        parser.print_help()
        exit(1)

    last_file_data = None

    while True:
        with open(options.file_name, 'r') as the_file:
            file_data = the_file.read()

            if len(file_data) > MAX_FILE_SIZE:
                raise MemoryError("Will not read files larger than %s bytes with this implementation!" % MAX_FILE_SIZE)

            if file_data != last_file_data:
                client.store({options.field_name: file_data})
                last_file_data = file_data

            time.sleep(2)
