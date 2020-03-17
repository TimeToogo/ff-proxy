
package com.timetoogo.ffclient;

import java.net.InetAddress;

public class FfConfig {
    private InetAddress ipAddress;
    private int port;
    private String preSharedKey;

    public InetAddress getIpAddress() {
        return ipAddress;
    }

    public void setIpAddress(InetAddress ipAddress) {
        this.ipAddress = ipAddress;
    }

    public String getPreSharedKey() {
        return preSharedKey;
    }

    public void setPreSharedKey(String preSharedKey) {
        this.preSharedKey = preSharedKey;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }

    void Validate() {
        if (this.ipAddress == null) {
            throw new RuntimeException("ipAddress must be set to valid IP address");
        }

        if (this.port > 0 && this.port < 65536) {
            throw new RuntimeException("port must be a valid port");
        }
    }
}
