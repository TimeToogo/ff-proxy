
package com.timetoogo.ffclient;

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;

public class FfRequest {
    public enum Version {
        V1(1);

        private int value;

        private Version(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }
    }

    private Version version;
    private long requestId;
    private List<FfRequestOption> options = new ArrayList<FfRequestOption>();
    private byte[] payload;

    public Version getVersion() {
        return version;
    }

    public void setVersion(Version version) {
        this.version = version;
    }

    public long getRequestId() {
        return requestId;
    }

    public void setRequestId(long requestId) {
        this.requestId = requestId;
    }

    public List<FfRequestOption> getOptions() {
        return options;
    }

    public void setOptions(List<FfRequestOption> options) {
        this.options = options;
    }

    public byte[] getPayload() {
        return payload;
    }

    public void setPayload(byte[] payload) {
        this.payload = payload;
    }
}
