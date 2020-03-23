from ff_client import FfClient, FfConfig, FfRequest
import unittest
import logging


class TestFfClient(unittest.TestCase):
    def test_new(self):
        client = FfClient(FfConfig(ip_address='127.0.0.1',
                                   port=8080, log_level=logging.DEBUG))

        self.assertEqual('127.0.0.1', client.config.ip_address)
        self.assertEqual(8080, client.config.port)

    def test_encrypt_payload(self):
        client = FfClient(FfConfig(ip_address='127.0.0.1', port=8080,
                                   pre_shared_key='testabc', log_level=logging.DEBUG))

        request = FfRequest(version=FfRequest.Version.V1,
                            request_id=123)
        payload = "GET / HTTP/1.1\nHost: google.com\n\n"

        encrypted_payload = client.encrypted_request_payload(request, payload)

        self.assertGreater(len(encrypted_payload), 0)
        self.assertNotEqual(payload, encrypted_payload)

        encryption_mode = [i for i in request.options if i.type ==
                           FfRequest.Option.Type.ENCRYPTION_MODE][0].value
        self.assertEqual(FfRequest.EncryptionMode.AES_256_GCM,
                         encryption_mode[0])

        encryption_iv = [i for i in request.options if i.type ==
                         FfRequest.Option.Type.ENCRYPTION_IV][0].value
        self.assertEqual(12, len(encryption_iv))

        encryption_tag = [i for i in request.options if i.type ==
                          FfRequest.Option.Type.ENCRYPTION_TAG][0].value
        self.assertEqual(16, len(encryption_tag))

    def test_send_get_request(self):
        client = FfClient(FfConfig(ip_address='127.0.0.1',
                                   port=8080, log_level=logging.DEBUG))

        client.send_request('GET / HTTP/1.1\nHost: google.com\n\n')

    def test_send_encrypted_get_request(self):
        client = FfClient(FfConfig(ip_address='127.0.0.1',
                                   pre_shared_key='testkey',
                                   port=8080, log_level=logging.DEBUG))

        client.send_request('GET / HTTP/1.1\nHost: google.com\n\n')
