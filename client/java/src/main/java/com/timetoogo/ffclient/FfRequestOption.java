
package com.timetoogo.ffclient;

public class FfRequestOption {
    public enum Type {
        EOL(0), ENCRYPTION_MODE(1), ENCRYPTION_IV(2), ENCRYPTION_TAG(3), HTTPS(4);

        private int value;

        private Type(int value) {
            this.value = value;
        }

        public int getValue() {
            return this.value;
        }
    }

    private Type type;
    private int length;
    private byte[] value;

    public Type getType() {
        return type;
    }

    public byte[] getValue() {
        return value;
    }

    public int getLength() {
        return length;
    }

    public void setLength(int length) {
        this.length = length;
    }

    public void setType(Type type) {
        this.type = type;
    }

    public void setValue(byte[] value) {
        this.value = value;
    }
}
