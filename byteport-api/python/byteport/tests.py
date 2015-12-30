import unittest
import datetime

from http_clients import ByteportHttpGetClient, ByteportHttpPostClient


class TestHttpClients(unittest.TestCase):

    hostname = 'localhost:8000'
    # hostname = 'api.byteport.se'
    byteport_api_store_url = 'http://%s/services/store/' % hostname

    namespace = 'test'
    device_uid = '6000'
    key = 'TEST'

    def test_should_handle_all_supported_timetamps_correctly(self):
        client = ByteportHttpGetClient(
            byteport_api_store_url=self.byteport_api_store_url,
            namespace_name=self.namespace,
            api_key=self.key,
            default_device_uid=self.device_uid
        )

        # integer input
        int_input = 0
        expected_result = '0'
        result = client.auto_timestamp(int_input)
        self.assertEqual(expected_result, result)

        # float input with exact precision
        float_input = 1.012345
        expected_result = '1.012345'
        result = client.auto_timestamp(float_input)
        self.assertEqual(expected_result, result)

        # float input with higher precision, should round
        float_input = 1.01234599999
        expected_result = '1.012346'
        result = client.auto_timestamp(float_input)
        self.assertEqual(expected_result, result)

        # datetime input, second precision
        datetime_input = datetime.datetime.strptime('1970-01-01T00:00:02', '%Y-%m-%dT%H:%M:%S')
        expected_result = '2'
        result = client.auto_timestamp(datetime_input)
        self.assertEqual(expected_result, result)

        # datetime input, micro-second precision
        datetime_input = datetime.datetime.strptime('1970-01-01T00:00:02.012345', '%Y-%m-%dT%H:%M:%S.%f')
        expected_result = '2.012345'
        result = client.auto_timestamp(datetime_input)
        self.assertEqual(expected_result, result)

        # datetime input, micro-second precision, later timestamp
        datetime_input = datetime.datetime.strptime('2015-05-01T00:00:00.012345', '%Y-%m-%dT%H:%M:%S.%f')
        expected_result = '1430438400.012345'
        result = client.auto_timestamp(datetime_input)
        self.assertEqual(expected_result, result)
