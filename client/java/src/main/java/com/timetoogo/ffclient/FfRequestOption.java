
package com.timetoogo.ffclient;

public class FfRequestOption {
    public enum Type {
        EOL((byte) 0), ENCRYPTION_MODE((byte) 1), ENCRYPTION_IV((byte) 2), ENCRYPTION_TAG(
                (byte) 3), HTTPS((byte) 4), KEY_DERIVE_MODE((byte) 5), KEY_DERIVE_SALT((byte) 6);

        private byte value;

        private Type(byte value) {
            this.value = value;
        }

        byte getValue() {
            return this.value;
        }
    }

    public static class Builder {
        private FfRequestOption option = new FfRequestOption();

        private Builder() {

        }

        public Builder type(Type type) {
            this.option.type = type;

            return this;
        }

        public Builder length(short length) {
            this.option.length = length;

            return this;
        }

        public Builder value(byte[] value) {
            this.option.value = value;

            return this;
        }

        public FfRequestOption build() {
            return this.option;
        }
    }

    private Type type;
    private short length;
    private byte[] value;

    private FfRequestOption() {

    }

    public static Builder builder() {
        return new Builder();
    }

    public Type getType() {
        return type;
    }

    public byte[] getValue() {
        return value;
    }

    public short getLength() {
        return length;
    }
}
