from .ff_config import FfConfig
from .ff_request import FfRequest
import socket
import random

class FfClient:
    def __init__(self, config: FfConfig):
        self.config = config
        
    
    def send_request(self, req: str, https: bool = True):
        request = FfRequest(version = FfRequest.Version.V1, request_id = random.getrandbits(64))
        
        if (https):
            request.options.append(FfRequest.Option(FfRequest.Option.Type.HTTPS, bytearray([1])))
        
        
        if (self.config.pre_shared_key):
            req = self.encrypted_request_payload(request, req)
            
            
    def encrypted_request_payload(self, request: FfRequest, payload: str):
        