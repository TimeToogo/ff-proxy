class FfRequest:
    class Version:
        V1 = 1 

    class EncryptionMode:
        AES_256_GCM = 1

    class Option:
        class Type:
            EOL = 0
            ENCRYPTION_MODE = 1
            ENCRYPTION_IV = 2
            ENCRYPTION_TAG = 3
            HTTPS = 4
        
        def __init__(self, type: int, length: int, value: bytearray):
            self.type = type
            self.length = length
            self.value = value

    def __init__(self, version: int = None, request_id: int = None):
        self.version = version
        self.request_id = request_id
        self.options = []
        self.payload = None
