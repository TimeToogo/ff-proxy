
package com.timetoogo.ffclient;

import java.util.ArrayList;
import java.util.List;

public class FfRequest {
    public enum Version {
        V1((short) 1);

        private short value;

        private Version(short value) {
            this.value = value;
        }

        public short getValue() {
            return value;
        }
    }

    public enum EncryptionMode {
        AES_256_GCM((byte) 1);

        private byte value;

        private EncryptionMode(byte value) {
            this.value = value;
        }

        public byte getValue() {
            return value;
        }
    }

    public enum KeyDeriveMode {
        PBKDF2((byte) 1);

        private byte value;

        private KeyDeriveMode(byte value) {
            this.value = value;
        }

        public byte getValue() {
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
