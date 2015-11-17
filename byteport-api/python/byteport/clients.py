import urllib
import urllib2
import logging
import base64
import zlib
import bz2
import datetime
import re
import os
import socks
import json

from urllib2 import HTTPError
from utils import DictDiffer

# Non standard imports, try to reduce if possible
import pytz

from socksipyhandler import SocksiPyHandler


class ByteportClientException(Exception):
    pass


class ByteportConnectException(ByteportClientException):
    pass


class ByteportClientForbiddenException(ByteportClientException):
    pass


class ByteportClientDeviceNotFoundException(ByteportClientException):
    pass


class ByteportClientUnsupportedCompressionException(ByteportClientException):
    pass


class ByteportClientUnsupportedTimestampTypeException(ByteportClientException):
    pass


class ByteportClientInvalidFieldNameException(ByteportClientException):
    pass


class ByteportClientInvalidDataTypeException(ByteportClientException):
    pass


class AbstractByteportClient:

    # Byteport supports milli-second precision timestamps but this client sends micro-second precision
    # timestamps if possible to support a possible future enhancement.
    #
    # Helper that can take a timestamp as epoch as string or number, or a datetime object
    # it will return a unix epoch as float converted to a string since we do not want
    # the string conversion to be left to other layers leading to possible precision
    # or rounding errors
    def auto_timestamp(self, timestamp):
        if type(timestamp) is int:
            fs = float(timestamp)
        elif type(timestamp) is float:
            fs = timestamp
        elif type(timestamp) is datetime.datetime:
            as_utc = self.timestamp_as_utc(timestamp)
            as_micros = self.unix_time_micros(as_utc)
            fs = as_micros / 1e6
        else:
            raise ByteportClientUnsupportedTimestampTypeException("Invalid format for auto_timestamp(): " % type(timestamp))

        # Will not leave trailing zeros, see
        # http://stackoverflow.com/questions/2440692/formatting-floats-in-python-without-superfluous-zeros
        return ('%f' % fs).rstrip('0').rstrip('.')

    def unix_time_micros(self, datetime_object):
        td = (datetime_object - datetime.datetime(1970, 1, 1, tzinfo=pytz.utc))
        u_secs = td.microseconds + ((td.seconds + td.days * 24 * 3600) * 10**6)
        return u_secs

    def timestamp_as_utc(self, datetime_object):
        if datetime_object.tzinfo:
            return datetime_object
        else:
            return pytz.utc.localize(datetime_object)

    def special_match(self, strg, search=re.compile(r'[^-a-zA-Z0-9_:]').search):
        return not bool(search(strg))

    def verify_name(self, name):
        if len(name) < 1 or len(name) > 32:
            return False

        if name.startswith('-') or name.startswith('_') or name.endswith('-') or name.endswith('_'):
            return False

        return self.special_match(name)

    def verify_field_name(self, field_name):
        try:
            self.verify_name(field_name)
        except Exception:
            raise ByteportClientInvalidFieldNameException()

    def utf8_encode_value(self, value):
        try:
            # Any string that can be UTF-8 encoded are valid data for Byteport HTTP API
            return (u'%s' % value).encode('utf8')
        except Exception:
            raise ByteportClientInvalidDataTypeException()

    def convert_data_to_utf8(self, data):
        utf8_data = dict()
        for field_name, value in data.iteritems():
            self.verify_field_name(field_name)
            value_as_utf8 = self.utf8_encode_value(value)

            utf8_data[field_name] = value_as_utf8

        return utf8_data


class AbstractByteportHttpClient(AbstractByteportClient):

    DEFAULT_BYTEPORT_API_STORE_URL = 'http://api.byteport.se/services/store/'
    DEFAULT_BYTEPORT_API_STORE_URL_HTTPS = 'http://api.byteport.se/services/store/'

    def __init__(self,
                 namespace_name,
                 api_key,
                 default_device_uid,
                 byteport_api_store_url=DEFAULT_BYTEPORT_API_STORE_URL,
                 proxy_type=socks.PROXY_TYPE_SOCKS5,
                 proxy_addr="127.0.0.1",
                 proxy_port=None,
                 proxy_username=None,
                 proxy_password=None,
                 initial_heartbeat=True
                 ):

        self.namespace_name = namespace_name
        self.device_uid = default_device_uid
        self.api_key = api_key
        self.base_url = byteport_api_store_url+'%s' % namespace_name
        logging.info('Storing data to Byteport using %s/%s/' % (self.base_url, default_device_uid))

        # Ie. for tunneling HTTP via SSH, first do:
        # ssh -D 5000 -N username@sshserver.org
        if proxy_port is not None:
            self.opener = urllib2.build_opener(SocksiPyHandler(proxy_type, proxy_addr, proxy_port))
            logging.info("Connecting through type %s proxy at %s:%s" % (proxy_type, proxy_addr, proxy_port))
        else:
            self.opener = None

        # Make empty test call to verify the credentials
        if initial_heartbeat:
            # This can also act as heart beat, no need to send data to signal "online" in Byteport
            self.store()

    def make_request(self, url, device_uid, data=None):

        try:
            logging.debug(url)
            # Set a valid User agent tag since api.byteport.se is CloudFlared
            # TODO: add a proper user-agent and make sure CloudFlare can handle it
            if self.opener:
                response = self.opener.open(url)
            else:
                # NOTE: If data != None, the request will be a POST request instead
                if data is not None:
                    data = urllib.urlencode(data)
                req = urllib2.Request(url, headers={'User-Agent': 'Mozilla/5.0'}, data=data)

                response = urllib2.urlopen(req)
            logging.debug(u'Response: %s' % response.read())

        except HTTPError as http_error:
            logging.error(u'%s' % http_error)
            if http_error.code == 403:
                message = u'Verify that the namespace %s does allow writes by HTTP, and also if the correct' \
                          u'request method is set and usd (ie GET and/or POST), and that the ' \
                          u'API key is correct.' % self.namespace_name
                logging.info(message)
                raise ByteportClientForbiddenException(message)
            if http_error.code == 404:
                message = u'Make sure the device %s is registered under ' \
                          u'namespace %s.' % (device_uid, self.namespace_name)
                logging.info(message)
                raise ByteportClientDeviceNotFoundException(message)

        except urllib2.URLError as e:
            logging.error(u'%s' % e)
            logging.info(u'Got URLError, make sure you have the correct network connections (ie. to the internet)!')
            if self.opener is not None:
                logging.info(u'Make sure your proxy settings are correct and you can connect to the proxy host you specified.')
            raise ByteportConnectException(u'Failed to connect to byteport, check your network and proxy settings and setup.')

    # Simple wrapper for logging with ease
    def log(self, message, level='info', device_uid=None):
        self.store({level: message}, device_uid)

    def store(self, data=None, device_uid=None):
        raise NotImplementedError("Store is not implemented in this class that is considered Abstract")


class ByteportHttpPostClient(AbstractByteportHttpClient):

    #
    #    Store a single file vs a field name to Byteport via HTTP POST with optional compresstion
    #
    def base64_encode_and_store_file(self, field_name, path_to_file,
                                           device_uid=None, timestamp=None, compression=None):

        if timestamp is not None:
            timestamp = self.auto_timestamp(timestamp)

        with open(path_to_file, 'r') as content_file:
            file_data = content_file.read()
            self.base64_encode_and_store(field_name, file_data, device_uid, timestamp, compression)

    #
    #   Store a single file vs a field name with no encoding or compression
    #
    def store_file(self, field_name, path_to_file, device_uid=None, timestamp=None):

        if timestamp is not None:
            timestamp = self.auto_timestamp(timestamp)

        with open(path_to_file, 'r') as content_file:
            data = {field_name: content_file.read()}

            if timestamp is not None:
                timestamp = self.auto_timestamp(timestamp)
                data['_ts'] = timestamp

            self.store(data, device_uid)

    def sorted_ls(self, path):
        mtime = lambda f: os.stat(os.path.join(path, f)).st_mtime
        return list(sorted(os.listdir(path), key=mtime))

    def store_directory(self, directory_path, device_uid, timestamp=None):

        directory_data = dict()

        # Get a list of files sorted by time
        list_of_files_in_directory = self.sorted_ls(directory_path)

        # Dump files with content to dictionary
        for file_name in list_of_files_in_directory:
            path_to_file = directory_path + '/' + file_name
            with open(path_to_file, 'r') as content_file:
                directory_data[file_name] = content_file.read()

        self.store(directory_data, device_uid=device_uid, timestamp=timestamp)

    '''
        NOTE: Move to some kind of "layer-2" helper module instead. let implementor handle the loop?
    '''
    def poll_directory_and_store_upon_content_change(self, directory_path, device_uid, timestamp=None, poll_interval=5):

        # initial empty data
        last_data = dict()

        while True:
            current_data = dict()

            # Get a list of files sorted by time
            list_of_files_in_directory = self.sorted_ls(directory_path)

            # Dump files with content to dictionary
            for file_name in list_of_files_in_directory:
                path_to_file = directory_path + '/' + file_name
                with open(path_to_file, 'r') as content_file:
                    current_data[file_name] = content_file.read()

            # This will obtain the keys that has changed value
            changed_data = DictDiffer(current_data, last_data).changed()
            added_data = DictDiffer(current_data, last_data).added()

            data_to_send = dict()
            for key in changed_data:
                data_to_send[key] = current_data[key]

            for key in added_data:
                data_to_send[key] = current_data[key]

            if len(data_to_send) > 0:
                try:
                    self.store(data_to_send, device_uid=device_uid, timestamp=timestamp)
                    last_data = current_data
                except Exception as e:
                    logging.warn("Failed to store data, reason was: %s" % e)

            time.sleep(poll_interval)

    #
    #    Store a single data block vs a field name to Byteport via HTTP POST
    #
    def base64_encode_and_store(self, field_name, fileobj,
                                      device_uid=None, timestamp=None, compression=None):

        if compression is None:
            data_block = fileobj
        elif compression == 'gzip':
            data_block = zlib.compress(fileobj)
        elif compression == 'bzip2':
            data_block = bz2.compress(fileobj)
        else:
            raise ByteportClientUnsupportedCompressionException("Unsupported compression method '%s'" % compression)

        data = {field_name: base64.b64encode(data_block)}

        if timestamp is not None:
            timestamp = self.auto_timestamp(timestamp)
            data['_ts'] = timestamp

        self.store(data, device_uid)

    def store(self, data=None, device_uid=None, timestamp=None):
        if data is None:
            data = dict()
        if device_uid is None:
            device_uid = self.device_uid

        data['_key'] = self.api_key
        url = '%s/%s/' % (self.base_url, device_uid)

        # Encode data to UTF-8 before storing
        utf8_encoded_data = self.convert_data_to_utf8(data)

        self.make_request(url, device_uid, utf8_encoded_data)


'''
    The preferred way of accessing byteport is through HTTPS POST calls.

    This class reflects that.

'''
class ByteportHttpClient(ByteportHttpPostClient):
    pass

'''
    Simple client for sending data using HTTP GET request (ie. data goes as request parameters)

    Use the ByteportHttpPostClient for most cases unless you have very good reason for using this method.

    Since URLs are limited to 2Kb, the maximum allowed data to send is limited for each request.

'''
class ByteportHttpGetClient(AbstractByteportHttpClient):

    # Can use another device_uid to override the one used in the constructor
    # Useful for Clients that acts as proxies for other devices, ie. over a sensor-network
    def store(self, data=None, device_uid=None, timestamp=None):
        if data is None:
            data = dict()
        if device_uid is None:
            device_uid = self.device_uid

        data['_key'] = self.api_key

        if timestamp is not None:
            float_timestamp = self.auto_timestamp(timestamp)
            data['_ts'] = float_timestamp

        # Encode data to UTF-8 before storing
        utf8_encoded_data = self.convert_data_to_utf8(data)

        encoded_data = urllib.urlencode(utf8_encoded_data)

        url = '%s/%s/?%s' % (self.base_url, device_uid, encoded_data)

        self.make_request(url, device_uid)


try:
    from stompest.config import StompConfig
    from stompest.protocol import StompSpec
    from stompest.sync import Stomp
    from stompest.error import StompConnectionError
except ImportError:
    print "Could not import Stompest library. The STOMP client will not be supported."
    print ""
    print "If you need to use that client, please do:"
    print "pip install stompest"

import time

class ByteportStompClient(AbstractByteportClient):
    DEFAULT_BROKER_HOSTS = ['broker.igw.se', 'broker1.igw.se', 'broker2.igw.se', 'broker3.igw.se']
    QUEUE_NAME = '/queue/simple_string_dev_message'

    client = None

    def __init__(self, namespace, login, passcode, broker_hosts=DEFAULT_BROKER_HOSTS):

        self.namespace = str(namespace)

        for broker_host in broker_hosts:
            broker_url = 'tcp://%s:61613' % broker_host
            self.CONFIG = StompConfig(broker_url, version=StompSpec.VERSION_1_2)
            self.client = Stomp(self.CONFIG)

            try:
                self.client.connect(headers={'login': login, 'passcode': passcode}, host=namespace)
                print "Connected to %s using protocol version %s" % (broker_host, self.client.session.version)
            except StompConnectionError:
                pass

    def disconnect(self):
        self.client.disconnect()

    def __send_json_message(self, json):
        self.client.send(self.QUEUE_NAME, json)

    def __send_message(self, uid, data_string, timestamp=None):

        if timestamp:
            timestamp = self.auto_timestamp(timestamp)
        else:
            timestamp = int(time.time())

        message = dict()
        message['uid'] = str(uid)
        message['namespace'] = self.namespace
        message['data'] = str(data_string)
        message['timestamp'] = str(timestamp)

        self.__send_json_message(json.dumps([message]))

    def store(self, data=None, device_uid=None, timestamp=None):
        if type(data) != dict:
            raise ByteportClientException("Data must be of type dict")

        for key in data.keys():
            self.verify_field_name(key)

        delimited_data = ';'.join("%s=%s" % (key, self.utf8_encode_value(val)) for (key, val) in data.iteritems())

        self.__send_message(device_uid, delimited_data, timestamp)

