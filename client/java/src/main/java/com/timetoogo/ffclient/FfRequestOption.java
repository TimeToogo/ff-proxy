
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

    public static class Builder {
        private FfRequestOption option = new FfRequestOption();

        private Builder() {

        }

        public Builder type(Type type) {
            this.option.type = type;

            return this;
        }

        public Builder length(int length) {
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
    private int length;
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

    public int getLength() {
        return length;
    }
}
