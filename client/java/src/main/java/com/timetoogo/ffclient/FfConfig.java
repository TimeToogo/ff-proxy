
package com.timetoogo.ffclient;

import java.net.InetAddress;

public class FfConfig {
    private InetAddress ipAddress;
    private int port;
    private String preSharedKey;
    private int pbkdf2Iterations = 1000;

    private FfConfig() {

    }

    public static class Builder {
        private FfConfig config = new FfConfig();

        public Builder ipAddress(InetAddress ipAddress) {
            this.config.ipAddress = ipAddress;
            return this;
        }

        public Builder port(int port) {
            this.config.port = port;
            return this;
        }

        public Builder preSharedKey(String preSharedKey) {
            this.config.preSharedKey = preSharedKey;
            return this;
        }

        public Builder pbkdf2Iterations(int pbkdf2Iterations) {
            this.config.pbkdf2Iterations = pbkdf2Iterations;
            return this;
        }

        public FfConfig build() {
            this.validate();

            return this.config;
        }

        private void validate() {
            if (this.config.ipAddress == null) {
                throw new RuntimeException("ipAddress must be set to valid IP address");
            }

            if (this.config.port < 0 || this.config.port > 65536) {
                throw new RuntimeException("port must be a valid port");
            }
        }

    }

    public static Builder builder() {
        return new Builder();
    }

    public InetAddress getIpAddress() {
        return ipAddress;
    }

    public String getPreSharedKey() {
        return preSharedKey;
    }

    public int getPort() {
        return port;
    }

    public int getPbkdf2Iterations() {
      return pbkdf2Iterations;
    }
}
