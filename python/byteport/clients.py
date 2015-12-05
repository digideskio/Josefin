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
import cookielib

from urllib2 import HTTPError
from utils import DictDiffer

# Non standard imports, try to reduce if possible
import pytz

from socksipyhandler import SocksiPyHandler


class ByteportClientException(Exception):
    pass


class ByteportConnectException(ByteportClientException):
    pass

class ByteportLoginFailedException(Exception):
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


class ByteportHTTPRedirectHandler(urllib2.HTTPRedirectHandler):
    def http_error_302(self, req, fp, code, msg, headers):
        print "Cookie Manip Right Here"
        return urllib2.HTTPRedirectHandler.http_error_302(self, req, fp, code, msg, headers)

    http_error_301 = http_error_303 = http_error_307 = http_error_302


class ByteportHttpClient(AbstractByteportClient):

    DEFAULT_BYTEPORT_API_PROTOCOL = 'http'
    DEFAULT_BYTEPORT_API_HOSTNAME = 'api.byteport.se'

    # Storing
    DEFAULT_BYTEPORT_STORE_PATH = '/api/v1/timeseries/'
    DEFAULT_BYTEPORT_API_STORE_URL = '%s://%s%s' % (DEFAULT_BYTEPORT_API_PROTOCOL,
                                                    DEFAULT_BYTEPORT_API_HOSTNAME,
                                                    DEFAULT_BYTEPORT_STORE_PATH)

    # DATETIME FORMAT
    ISO8601 = '%Y-%m-%dT%H:%M:%S.%f'

    # APIV1 URLS
    LOGIN_PATH = '/api/v1/login/'
    LOGOUT_PATH = '/api/v1/logout/'
    SESSION_PATH = '/api/v1/session/'
    ECHO_PATH = '/api/v1/echo/'

    LIST_NAMESPACES      = '/api/v1/namespace/'
    QUERY_DEVICES        = '/api/v1/search_devices/'
    GET_DEVICE           = '/api/v1/namespace/%s/device/'
    GET_DEVICE_TYPE      = '/api/v1/namespace/%s/device_type/'
    GET_FIRMWARE         = '/api/v1/namespace/%s/device_type/%s/firmware/'
    GET_FIELD_DEFINITION = '/api/v1/namespace/%s/device_type/%s/field_definition/'

    LOAD_TIMESERIES_DATA = '/api/v1/timeseries/%s/%s/%s/'

    def __init__(self,
                 namespace_name=None,
                 api_key=None,
                 default_device_uid=None,
                 byteport_api_hostname=DEFAULT_BYTEPORT_API_HOSTNAME,
                 proxy_type=socks.PROXY_TYPE_SOCKS5,
                 proxy_addr="127.0.0.1",
                 proxy_port=None,
                 proxy_username=None,
                 proxy_password=None,
                 initial_heartbeat=True
                 ):

        # If any of the following are left as default (None), no store methods can be used
        self.namespace_name = namespace_name
        self.api_key = api_key

        if None in [namespace_name, api_key]:
            logging.info("Store functions using API-key methods are disabled as no Namespace or API-key was supplied.")
            self.store_enabled = False
        else:
            self.store_enabled = True

        self.device_uid = default_device_uid
        self.byteport_api_hostname = byteport_api_hostname

        # Ie. for tunneling HTTP via SSH, first do:
        # ssh -D 5000 -N username@sshserver.org
        if proxy_port is not None:
            self.opener = urllib2.build_opener(SocksiPyHandler(proxy_type, proxy_addr, proxy_port))
            logging.info("Connecting through type %s proxy at %s:%s" % (proxy_type, proxy_addr, proxy_port))
        else:
            self.opener = None

        self.cookiejar = cookielib.CookieJar()

        if self.store_enabled:
            self.store_base_url = '%s://%s%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL,
                                                    byteport_api_hostname,
                                                    self.DEFAULT_BYTEPORT_STORE_PATH,
                                                    namespace_name)

            logging.info('Storing data to Byteport using %s/%s/' % (self.store_base_url, default_device_uid))

            # Make empty test call to verify the credentials
            if initial_heartbeat:
                # This can also act as heart beat, no need to send data to signal "online" in Byteport
                self.store()

    def login(self, username, password, login_path=LOGIN_PATH):

        url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, login_path)

        # This will induce a GET-call to obtain the csrftoken needed for the actual login
        self.make_request(url)

        # Now, also extract the value of the csrftoken since we need it as a post data also
        csrftoken = self.__get_value_of_cookie('csrftoken')

        if csrftoken is None:
            raise ByteportClientException("Failed to extract csrftoken.")

        # And make the POST-call to login
        try:
            self.make_request(url=url,
                              post_data={'username': username,
                                         'password': password,
                                         'csrfmiddlewaretoken': csrftoken}
                              )
        except ByteportClientForbiddenException as e:
            raise ByteportLoginFailedException("Failed to login user with name %s" % username)

        # Make sure the sessionid cookie is present in the cookie jar now
        for cookie in self.cookiejar:
            if cookie.name == 'sessionid':
                return

        raise ByteportLoginFailedException("Failed to login user with name %s" % username)

    def __get_value_of_cookie(self, cookie_name):
        for cookie in self.cookiejar:
            if cookie.name == cookie_name:
                return cookie.value
        return None

    def list_namespaces(self):
        url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, self.LIST_NAMESPACES)

        return json.loads(self.make_request(url).read())

    def query_devices(self, term, full=False, limit=20):
        request_parameters = {'term': term, 'full': u'%s' % full, 'limit': limit}
        encoded_data = urllib.urlencode(request_parameters)
        url = '%s://%s%s?%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL,
                                self.byteport_api_hostname,
                                self.QUERY_DEVICES,
                                encoded_data)

        return json.loads(self.make_request(url).read())

#TODO: Deprecated. Remove at some point.
    def get_device(self, namespace, uid):
        base_url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, self.GET_DEVICE)

        encoded_data = urllib.urlencode( {'uid':u'%s' % uid } )
        url = base_url % (namespace) + "?%s" % encoded_data
        return json.loads(self.make_request(url).read())

#TODO: Deprecated. Remove at some point.
    def list_devices(self, namespace, full=False):
        base_url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, self.GET_DEVICE)
        request_parameters = {'full': u'%s' % full}
        encoded_data = urllib.urlencode(request_parameters)

        url = base_url % namespace + '?%s' % encoded_data

        return json.loads(self.make_request(url).read())

    def get_devices(self, namespace, key=None):
        base_url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, self.GET_DEVICE)
        request_parameters = {}
        if( key is not None ):
            request_parameters['key'] = key

        url = base_url % namespace + '?%s' % urllib.urlencode(request_parameters)

        return json.loads(self.make_request(url).read())


    def get_device_types(self, namespace, key=None):
        base_url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, self.GET_DEVICE_TYPE)
        request_parameters = {}
        if( key is not None ):
            request_parameters['key'] = key

        url = base_url % namespace + '?%s' % urllib.urlencode(request_parameters)

        return json.loads(self.make_request(url).read())

    def get_firmwares(self, namespace, device_type_id, key=None):
        base_url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, self.GET_FIRMWARE)
        request_parameters = {}
        if( key is not None ):
            request_parameters['key'] = key

        url = base_url % (namespace, device_type_id) + '?%s' % urllib.urlencode(request_parameters)

        return json.loads(self.make_request(url).read())

    def get_field_definitions(self, namespace, device_type_id, key=None):
        base_url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, self.GET_FIELD_DEFINITION)
        request_parameters = {}
        if key is not None:
            request_parameters['key'] = key

        url = base_url % (namespace, device_type_id) + '?%s' % urllib.urlencode(request_parameters)

        return json.loads(self.make_request(url).read())

    def load_timeseries_data(self, namespace, uid, field_name, from_time, to_time):
        base_url = '%s://%s%s' % (self.DEFAULT_BYTEPORT_API_PROTOCOL, self.byteport_api_hostname, self.LOAD_TIMESERIES_DATA)
        request_parameters = {'from': from_time.strftime(self.ISO8601), 'to': to_time.strftime(self.ISO8601)}
        encoded_data = urllib.urlencode(request_parameters)

        url = base_url % (namespace, uid, field_name) + '?%s' % encoded_data

        return json.loads(self.make_request(url).read())

    def make_request(self, url, post_data=None):

        try:
            logging.debug(url)
            # Set a valid User agent tag since api.byteport.se is CloudFlared
            # TODO: add a proper user-agent and make sure CloudFlare can handle it
            if self.opener:
                return self.opener.open(url)
            else:
                headers = {'User-Agent': 'Mozilla/5.0'}

                # NOTE: If post_data != None, the request will be a POST request instead
                if post_data is not None:
                    post_data = urllib.urlencode(post_data)

                req = urllib2.Request(url, headers=headers, data=post_data)

                opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(self.cookiejar))
                return opener.open(req)

        except HTTPError as http_error:
            logging.error(u'HTTPError accessing %s, Error was: %s' % (url, http_error))
            if http_error.code == 403:
                message = u'403, You were not allowed to access the requested resource.'
                logging.info(message)
                raise ByteportClientForbiddenException(message)
            if http_error.code == 404:
                message = u'404, Make sure the device(s) is registered under ' \
                          u'namespace %s.' % self.namespace_name
                logging.info(message)
                raise ByteportClientDeviceNotFoundException(message)

        except urllib2.URLError as e:
            logging.error(u'URLError accessing %s, Error was: %s' % (url, e))
            logging.info(u'Got URLError, make sure you have the correct network connections (ie. to the internet)!')
            if self.opener is not None:
                logging.info(u'Make sure your proxy settings are correct and you can connect to the proxy host you specified.')
            raise ByteportConnectException(u'Failed to connect to byteport, check your network and proxy settings and setup.')

    # Simple wrapper for logging with ease
    def log(self, message, level='info', device_uid=None):
        self.store({level: message}, device_uid)

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
        url = '%s/%s/' % (self.store_base_url, device_uid)

        # Encode data to UTF-8 before storing
        utf8_encoded_data = self.convert_data_to_utf8(data)

        self.make_request(url, utf8_encoded_data)

'''
    Simple client for sending data using HTTP GET request (ie. data goes as request parameters)

    Use the ByteportHttpPostClient for most cases unless you have very good reason for using this method.

    Since URLs are limited to 2Kb, the maximum allowed data to send is limited for each request.

    WARNING: May become deprecated!

'''
class ByteportHttpGetClient(ByteportHttpClient):

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

        # By URL-encoding, the make_request call will be made using GET-request
        encoded_data = urllib.urlencode(utf8_encoded_data)

        url = '%s/%s/?%s' % (self.store_base_url, device_uid, encoded_data)

        self.make_request(url)


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
                # Convention: set vhost to the namespace. This will require a message-boss consuming on this vhost!!!
                vhost = namespace
                self.client.connect(headers={'login': login, 'passcode': passcode}, host=vhost)
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

