from ff_client import FfClient, FfConfig, FfRequest
import unittest
import logging
import time

class TestFfClientMutiPacket(unittest.TestCase):
    def test_create_request_packets(self):
        client = FfClient(FfConfig(ip_address='127.0.0.1',
                                   port=8080,
                                   log_level=logging.DEBUG))

        http_request = "POST / HTTP/1.1\nHost: google.com.au\n\n".ljust(
            2000, '0')
        packets = client.create_request_packets(http_request, https = False)

        payload_options_length = 11 + 3 # Timstamp option + EOL option

        self.assertEqual(2, len(packets))

        packet1_buff = packets[0].payload
        packet1_len = packets[0].length
        ptr = 0

        # -- Packet 1 --

        self.assertEqual(1300, packet1_len)

        # Request version
        self.assertEqual(FfRequest.Version.V1,
                         packet1_buff[ptr] << 8 | packet1_buff[ptr + 1])
        ptr += 2

        # Request ID
        packet1_request_id = (
            packet1_buff[ptr] << 56
            | packet1_buff[ptr + 1] << 48
            | packet1_buff[ptr + 2] << 40
            | packet1_buff[ptr + 3] << 32
            | packet1_buff[ptr + 4] << 24
            | packet1_buff[ptr + 5] << 16
            | packet1_buff[ptr + 6] << 8
            | packet1_buff[ptr + 7]
        )
        self.assertNotEqual(0, packet1_request_id)
        ptr += 8

        # Total length
        self.assertEqual(len(http_request) + payload_options_length, (
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
        self.assertEqual(1277, (
            packet1_buff[ptr] << 8
            | packet1_buff[ptr + 1]
        ))
        ptr += 2

        self.assertEqual(FfRequest.Option.Type.BREAK, packet1_buff[ptr])
        ptr += 1
        # Break option length
        self.assertEqual(0, packet1_buff[ptr] << 16 | packet1_buff[ptr + 1])
        ptr += 2

        # Timestamp option type
        self.assertEqual(FfRequest.Option.Type.TIMESTAMP, packet1_buff[ptr])
        ptr += 1
        # Timestamp option length
        self.assertEqual(8, packet1_buff[ptr] << 16 | packet1_buff[ptr + 1])
        ptr += 2
        # Timestamp option value
        self.assertAlmostEqual(
            time.time(), 
            packet1_buff[ptr] << 56
                | packet1_buff[ptr + 1] << 48
                | packet1_buff[ptr + 2] << 40
                | packet1_buff[ptr + 3] << 32
                | packet1_buff[ptr + 4] << 24
                | packet1_buff[ptr + 5] << 16
                | packet1_buff[ptr + 6] << 8
                | packet1_buff[ptr + 7],
            delta = 5
        )
        ptr += 8

        # EOL option type
        self.assertEqual(FfRequest.Option.Type.EOL, packet1_buff[ptr])
        ptr += 1
        # EOL option length
        self.assertEqual(0, packet1_buff[ptr] << 16 | packet1_buff[ptr + 1])
        ptr += 2

        # Payload
        self.assertEqual(bytearray(http_request.encode('utf8'))[:1277 - payload_options_length],
                         packet1_buff[ptr:packet1_len])

        # -- Packet 2 --

        packet2_buff = packets[1].payload
        packet2_len = packets[1].length
        ptr = 0

        self.assertEqual(760, packet2_len)

        # Request version
        self.assertEqual(FfRequest.Version.V1,
                         packet2_buff[ptr] << 8 | packet2_buff[ptr + 1])
        ptr += 2

        # Request ID
        packet2_request_id = (
            packet2_buff[ptr] << 56
            | packet2_buff[ptr + 1] << 48
            | packet2_buff[ptr + 2] << 40
            | packet2_buff[ptr + 3] << 32
            | packet2_buff[ptr + 4] << 24
            | packet2_buff[ptr + 5] << 16
            | packet2_buff[ptr + 6] << 8
            | packet2_buff[ptr + 7]
        )
        self.assertNotEqual(0, packet2_request_id)
        ptr += 8

        # Total length
        self.assertEqual(len(http_request) + payload_options_length, (
            packet2_buff[ptr] << 24
            | packet2_buff[ptr + 1] << 16
            | packet2_buff[ptr + 2] << 8
            | packet2_buff[ptr + 3]
        ))
        ptr += 4

        # Chunk offset
        self.assertEqual(1277, (
            packet2_buff[ptr] << 24
            | packet2_buff[ptr + 1] << 16
            | packet2_buff[ptr + 2] << 8
            | packet2_buff[ptr + 3]
        ))
        ptr += 4

        # Chunk length
        self.assertEqual(len(http_request) - (1277 - payload_options_length), (
            packet2_buff[ptr] << 8
            | packet2_buff[ptr + 1]
        ))
        ptr += 2

        # EOL option type
        self.assertEqual(FfRequest.Option.Type.EOL, packet2_buff[ptr])
        ptr += 1
        # EOL option length
        self.assertEqual(0, packet2_buff[ptr] << 16 | packet2_buff[ptr + 1])
        ptr += 2

        # Payload
        self.assertEqual(bytearray(http_request.encode('utf8'))[1277 - payload_options_length:],
                         packet2_buff[ptr:packet2_len])

        # Request ID's
        self.assertEqual(packet1_request_id, packet2_request_id)
