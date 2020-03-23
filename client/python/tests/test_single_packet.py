from ff_client import FfClient, FfConfig, FfRequest
import unittest
import logging


class TestFfClientSinglePacket(unittest.TestCase):
    def test_create_request_packets_for_get_request(self):
        client = FfClient(FfConfig(ip_address='127.0.0.1',
                                   port=8080, log_level=logging.DEBUG))

        http_request = "GET / HTTP/1.1\nHost: google.com.au\n\n"
        packets = client.create_request_packets(http_request, https=False)

        self.assertEqual(1, len(packets))

        packet1_buff = packets[0].payload
        packet1_len = packets[0].length
        ptr = 0

        self.assertEqual(59, packet1_len)

        # Request version
        self.assertEqual(FfRequest.Version.V1,
                         packet1_buff[ptr] << 8 | packet1_buff[ptr + 1])
        ptr += 2

        # Request ID
        self.assertNotEqual(0, (
            packet1_buff[ptr] << 56
            | packet1_buff[ptr + 1] << 48
            | packet1_buff[ptr + 2] << 40
            | packet1_buff[ptr + 3] << 32
            | packet1_buff[ptr + 4] << 24
            | packet1_buff[ptr + 5] << 16
            | packet1_buff[ptr + 6] << 8
            | packet1_buff[ptr + 7]
        ))
        ptr += 8

        # Total length
        self.assertEqual(len(http_request), (
            packet1_buff[ptr] << 24
            | packet1_buff[ptr + 1] << 16
            | packet1_buff[ptr + 2] << 8
            | packet1_buff[ptr + 3]
        ))
        ptr += 4

        # Chunk offset
        self.assertEqual(0, (
            packet1_buff[ptr] << 24
            | packet1_buff[ptr + 1] << 16
            | packet1_buff[ptr + 2] << 8
            | packet1_buff[ptr + 3]
        ))
        ptr += 4

        # Chunk length
        self.assertEqual(len(http_request), (
            packet1_buff[ptr] << 8
            | packet1_buff[ptr + 1]
        ))
        ptr += 2

        # EOL option type
        self.assertEqual(FfRequest.Option.Type.EOL, packet1_buff[ptr])
        ptr += 1

        # EOL option length
        self.assertEqual(0, packet1_buff[ptr] << 16 | packet1_buff[ptr + 1])
        ptr += 2

        # Payload
        self.assertEqual(bytearray(http_request.encode('utf8')),
                         packet1_buff[ptr:packet1_len])
