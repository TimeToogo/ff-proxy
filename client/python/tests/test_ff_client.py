import unittest
from ff_client import FfClient, FfConfig

class TestFfClient(unittest.TestCase):
    def test_new(self):
        client = FfClient(FfConfig(ip_address = '127.0.0.1', port = 8080))
        
        self.assertEqual('127.0.0.1', client.config.ip_address)
        self.assertEqual(8080, client.config.port)
        
        
    def test_send_get_request(self):
        client = FfClient(FfConfig(ip_address = '127.0.0.1', port = 8080))
        
        client.send_request("GET / HTTP/1.1\nHost: google.com\n\n")