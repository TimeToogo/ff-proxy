class UdpPacket:
    def __init__(self,  length: int, payload: bytearray):
        self.length = length
        self.payload = payload
