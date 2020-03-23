from .ff_config import FfConfig
from .ff_request import FfRequest
from .udp_packet import UdpPacket
import socket
import struct
import logging
from Cryptodome.Random import get_random_bytes, random
from Cryptodome.Cipher import AES
from typing import List


class FfClient:
    MAX_PACKET_LENGTH = 1300

    def __init__(self, config: FfConfig):
        self.config = config
        self.logger = self.init_default_logger()

    def init_default_logger(self):
        logger = logging.getLogger('ff')
        logger.setLevel(self.config.log_level)
        ch = logging.StreamHandler()
        formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        ch.setFormatter(formatter)

        logger.addHandler(ch)

        return logger

    def send_request(self, http_request: str, https: bool = True):
        packets = self.create_request_packets(http_request, https)

        self.logger.info('Creating socket')

        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            self.logger.info('Sending %d packets to %s:%s' % (
                len(packets), self.config.ip_address, self.config.port))

            for packet in packets:
                sock.sendto(packet.payload[:packet.length],
                            (self.config.ip_address, self.config.port))
            
            self.logger.info('Packets sent!')

    def create_request_packets(self, http_request: str, https: bool = True):
        self.logger.debug('Initialising ff request')
        request = FfRequest(version=FfRequest.Version.V1,
                            request_id=random.getrandbits(64))

        if https:
            request.options.append(FfRequest.Option(
                FfRequest.Option.Type.HTTPS, 1, bytearray([1])))

        if (self.config.pre_shared_key):
            http_request = self.encrypted_request_payload(
                request, http_request)

        request.options.append(FfRequest.Option(
            FfRequest.Option.Type.EOL, 0, bytearray([])))

        request.payload = http_request

        return self.packetise_request(request)

    def encrypted_request_payload(self, request: FfRequest, payload: str) -> str:
        self.logger.debug('Encrypting request payload')

        if not self.config.pre_shared_key:
            raise RuntimeError(
                'Cannot encrypt payload without pre_shared_key set')

        iv = get_random_bytes(12)
        padded_key = self.config.pre_shared_key.encode(
            'utf8').ljust(32, b'\0')
        cipher = AES.new(key=padded_key, mode=AES.MODE_GCM,
                         nonce=iv, mac_len=16)

        ciphertext, tag = cipher.encrypt_and_digest(
            bytearray(payload.encode('utf8')))

        request.options.append(FfRequest.Option(
            FfRequest.Option.Type.ENCRYPTION_MODE,
            1,
            bytearray([FfRequest.EncryptionMode.AES_256_GCM])
        ))

        request.options.append(FfRequest.Option(
            FfRequest.Option.Type.ENCRYPTION_IV,
            len(iv),
            bytearray(iv)
        ))

        request.options.append(FfRequest.Option(
            FfRequest.Option.Type.ENCRYPTION_TAG,
            len(tag),
            bytearray(tag)
        ))

        self.logger.debug('Encrypted request into %d bytes' % len(ciphertext))
        return ciphertext

    def packetise_request(self, request: FfRequest) -> List[UdpPacket]:
        self.logger.debug('Packetising request')

        if isinstance(request.payload, bytearray):
            payload = request.payload
        elif isinstance(request.payload, str):
            payload = bytearray(request.payload.encode('utf8'))
        elif isinstance(request.payload, bytes):
            payload = bytearray(request.payload)
        else:
            raise ValueError(
                'Unknown request payload type, expecting bytearray, str or bytes, got %s' % request.payload)

        packets = []
        chunk_offset = 0
        bytes_left = len(payload)

        while(bytes_left > 0):
            packet_buff = bytearray(self.MAX_PACKET_LENGTH)
            ptr = 0

            # @see https://docs.python.org/2/library/struct.html#format-characters
            ptr += self.pack_into_size('!H', packet_buff, ptr, request.version)
            ptr += self.pack_into_size('!Q', packet_buff,
                                       ptr, request.request_id)
            ptr += self.pack_into_size('!I', packet_buff,
                                       ptr, len(request.payload))
            ptr += self.pack_into_size('!I', packet_buff, ptr, chunk_offset)
            # defer writing chunk length
            chunk_length_ptr = ptr
            ptr += 2

            options = request.options if chunk_offset == 0 else [
                FfRequest.Option(FfRequest.Option.Type.EOL, 0, bytearray([]))]

            for option in options:
                ptr += self.pack_into_size('!B', packet_buff, ptr, option.type)
                ptr += self.pack_into_size('!H',
                                           packet_buff, ptr, option.length)
                for i in option.value:
                    packet_buff[ptr] = i
                    ptr += 1

            chunk_length = min(self.MAX_PACKET_LENGTH - ptr, bytes_left)
            self.pack_into_size('!H', packet_buff,
                                chunk_length_ptr, chunk_length)

            for i in range(chunk_offset, chunk_offset + chunk_length):
                packet_buff[ptr] = payload[i]
                ptr += 1

            bytes_left -= chunk_length
            chunk_offset += chunk_length

            packets.append(UdpPacket(ptr, packet_buff))

        self.logger.debug('Packetised request into %d packets' % len(packets))

        return packets

    def pack_into_size(self, format: str, buff: bytearray, offset: int, value) -> int:
        size = struct.calcsize(format)
        struct.pack_into(format, buff, offset, value)

        return size
