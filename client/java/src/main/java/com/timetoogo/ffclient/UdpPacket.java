
package com.timetoogo.ffclient;

class UdpPacket {
    private byte[] value;
    private int length;

    public UdpPacket(byte[] value, int length) {
        this.value = value;
        this.length = length;
    }

    public int getLength() {
        return length;
    }

    public byte[] getValue() {
        return value;
    }
}
