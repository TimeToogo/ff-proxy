from typing import Optional
import socket
import logging


def is_valid_ip(s: str):
    try:
        socket.inet_aton(s)
    except:
        return False
    else:
        return True


class FfConfig:
    def __init__(self, ip_address: str, port: int, pre_shared_key: Optional[str] = None, log_level=logging.ERROR):
        if not is_valid_ip(ip_address):
            raise ValueError('ip_address must be a valid IP address')

        if port <= 0 or port > 65536:
            raise ValueError('port must be a valid port number (0, 65536)')

        self.ip_address = ip_address
        self.port = port
        self.pre_shared_key = pre_shared_key
        self.log_level = log_level;
