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
    def __init__(self, ip_address: str, port: int, pre_shared_key: Optional[str] = None, pbkdf2_iterations: int = 1000, log_level=logging.ERROR):
        if not is_valid_ip(ip_address):
            raise ValueError('ip_address must be a valid IP address')

        if port <= 0 or port > 65536:
            raise ValueError('port must be a valid port number (0, 65536)')

        if pbkdf2_iterations <= 0:
            raise ValueError('pbkdf2_iterations must be greater than 0')

        self.ip_address = ip_address
        self.port = port
        self.pre_shared_key = pre_shared_key
        self.pbkdf2_iterations = pbkdf2_iterations
        self.log_level = log_level
