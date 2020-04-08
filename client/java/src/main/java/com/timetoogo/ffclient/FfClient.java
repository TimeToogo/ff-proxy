
package com.timetoogo.ffclient;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse.BodySubscriber;
import java.net.http.HttpResponse.BodySubscribers;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.Map.Entry;
import java.util.concurrent.Flow;
import java.util.logging.Logger;

import javax.crypto.Cipher;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.security.SecureRandom;

public class FfClient {
    private final Logger logger = Logger.getLogger(FfClient.class.getName());
    private final Charset UTF8 = Charset.forName("UTF-8");
    private FfConfig config;
    private final int MAX_PACKET_LENGTH = 1300;

    static final class ByteArraySubscriber implements Flow.Subscriber<ByteBuffer> {
        final BodySubscriber<byte[]> wrapped;

        private ByteArraySubscriber(BodySubscriber<byte[]> wrapped) {
            this.wrapped = wrapped;
        }

        @Override
        public void onSubscribe(Flow.Subscription subscription) {
            wrapped.onSubscribe(subscription);
        }

        @Override
        public void onNext(ByteBuffer item) {
            wrapped.onNext(List.of(item));
        }

        @Override
        public void onError(Throwable throwable) {
            wrapped.onError(throwable);
        }

        @Override
        public void onComplete() {
            wrapped.onComplete();
        }
    }

    public FfClient(FfConfig config) {
        this.config = config;
    }

    public void sendRequest(HttpRequest httpRequest) throws Exception {
        var packets = this.createRequestPackets(httpRequest);

        this.logger.info("Creating socket");
        var socket = new DatagramSocket(0, InetAddress.getLocalHost());

        socket.setReuseAddress(true);
        socket.connect(this.config.getIpAddress(), this.config.getPort());

        for (var packet : packets) {
            socket.send(new DatagramPacket(packet.getValue(), packet.getLength()));
        }

        this.logger.info(String.format("Sent %d packets to %s:%d", packets.size(),
                this.config.getIpAddress().toString(), this.config.getPort()));

        socket.close();
    }

    List<UdpPacket> createRequestPackets(HttpRequest httpRequest) throws Exception {
        FfRequest request = new FfRequest();
        request.setVersion(FfRequest.Version.V1);
        request.setRequestId(this.getRandomInstance().nextLong());

        if (httpRequest.uri().getScheme().equalsIgnoreCase("https")) {
            request.getOptions().add(FfRequestOption.builder().type(FfRequestOption.Type.HTTPS)
                    .length((short) 1).value(new byte[] {1}).build());
        }

        byte[] httpMessage = this.serializeHttpRequest(httpRequest);

        if (this.config.getPreSharedKey() != null && !this.config.getPreSharedKey().isEmpty()) {
            httpMessage = this.encryptHttpMessage(httpMessage, request);
        }

        request.setPayload(httpMessage);

        request.getOptions().add(
                FfRequestOption.builder().type(FfRequestOption.Type.EOL).length((short) 0).build());

        return this.packetiseRequest(request);
    }

    byte[] serializeHttpRequest(HttpRequest request) throws IOException {
        this.logger.info("Serializing HTTP request");
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();

        buffer.write(request.method().getBytes(UTF8));
        buffer.write(" ".getBytes(UTF8));
        buffer.write(
                (request.uri().getPath().isEmpty() ? "/" : request.uri().getPath()).getBytes(UTF8));

        if (request.uri().getQuery() != null && !request.uri().getQuery().isEmpty()) {
            buffer.write("?".getBytes(UTF8));
            buffer.write(request.uri().getQuery().getBytes(UTF8));
        }

        buffer.write(" HTTP/1.1\n".getBytes(UTF8));

        if (request.uri().getHost() != null) {
            buffer.write("Host: ".getBytes(UTF8));
            buffer.write(request.uri().getHost().getBytes(UTF8));
            buffer.write("\n".getBytes(UTF8));
        }

        if (request.bodyPublisher().isPresent()) {
            var contentLength = request.bodyPublisher().get().contentLength();
            buffer.write("Content-Length: ".getBytes(UTF8));
            buffer.write(Long.toString(contentLength).getBytes(UTF8));
            buffer.write("\n".getBytes(UTF8));
        }

        for (Entry<String, List<String>> header : request.headers().map().entrySet()) {
            for (String value : header.getValue()) {
                buffer.write(header.getKey().getBytes(UTF8));
                buffer.write(": ".getBytes(UTF8));
                buffer.write(value.getBytes(UTF8));
                buffer.write("\n".getBytes(UTF8));
            }
        }

        buffer.write("\n".getBytes(UTF8));

        if (request.bodyPublisher().isPresent()) {
            var bodySubscriber = BodySubscribers.ofByteArray();
            var byteArraySubscriber = new ByteArraySubscriber(bodySubscriber);
            request.bodyPublisher().get().subscribe(byteArraySubscriber);
            byte[] body = bodySubscriber.getBody().toCompletableFuture().join();

            buffer.write(body);
        }

        byte[] bytes = buffer.toByteArray();

        this.logger.info(String.format("Serialized request to %d bytes", bytes.length));

        return bytes;
    }

    byte[] encryptHttpMessage(byte[] httpMessage, FfRequest request) throws Exception {
        this.logger.info("Encrypting payload");
        Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");

        var salt = new byte[16];
        this.getRandomInstance().nextBytes(salt);

        var spec = new PBEKeySpec(this.config.getPreSharedKey().toCharArray(), salt,
                this.config.getPbkdf2Iterations(), 256);
        var skf = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256");
        var derivedKey = skf.generateSecret(spec).getEncoded();
        var keySpec = new SecretKeySpec(derivedKey, "AES");

        var iv = new byte[12];
        this.getRandomInstance().nextBytes(iv);

        var tagLength = 16;
        var gcmSpec = new GCMParameterSpec(tagLength * 8, iv);

        cipher.init(Cipher.ENCRYPT_MODE, keySpec, gcmSpec);

        byte[] cipherTextWithTag = cipher.doFinal(httpMessage);

        byte[] cipherText = new byte[cipherTextWithTag.length - tagLength];
        byte[] tag = new byte[tagLength];
        System.arraycopy(cipherTextWithTag, 0, cipherText, 0, cipherText.length);
        System.arraycopy(cipherTextWithTag, cipherText.length, tag, 0, tag.length);

        this.logger.info(String.format("Encrypted message into %d bytes", cipherText.length));

        request.getOptions().add(FfRequestOption.builder()
                .type(FfRequestOption.Type.ENCRYPTION_MODE).length((short) 1)
                .value(new byte[] {FfRequest.EncryptionMode.AES_256_GCM.getValue()}).build());

        request.getOptions().add(FfRequestOption.builder().type(FfRequestOption.Type.ENCRYPTION_IV)
                .length((short) iv.length).value(iv).build());

        request.getOptions().add(FfRequestOption.builder().type(FfRequestOption.Type.ENCRYPTION_TAG)
                .length((short) tag.length).value(tag).build());

        request.getOptions()
                .add(FfRequestOption.builder().type(FfRequestOption.Type.KEY_DERIVE_MODE)
                        .length((short) 1)
                        .value(new byte[] {FfRequest.KeyDeriveMode.PBKDF2.getValue()}).build());

        request.getOptions()
                .add(FfRequestOption.builder().type(FfRequestOption.Type.KEY_DERIVE_SALT)
                        .length((short) salt.length).value(salt).build());

        return cipherText;
    }

    List<UdpPacket> packetiseRequest(FfRequest request) {
        this.logger.info("Packetising request");
        var packets = new ArrayList<UdpPacket>();

        int chunkOffset = 0;
        int bytesLeft = request.getPayload().length;

        while (bytesLeft > 0) {
            byte[] packetBuff = new byte[MAX_PACKET_LENGTH];
            int ptr = 0;

            ptr = this.writeShort(packetBuff, ptr, request.getVersion().getValue());
            ptr = this.writeLong(packetBuff, ptr, request.getRequestId());
            ptr = this.writeInt(packetBuff, ptr, request.getPayload().length);
            ptr = this.writeInt(packetBuff, ptr, chunkOffset);
            // Defer writing chuck length
            int chunkLengthPtr = ptr;
            ptr += 2;

            var options =
                    chunkOffset == 0 ? request.getOptions() : new ArrayList<FfRequestOption>() {
                        {
                            add(FfRequestOption.builder().type(FfRequestOption.Type.EOL)
                                    .length((short) 0).build());
                        }
                    };

            for (var option : options) {
                packetBuff[ptr++] = option.getType().getValue();
                ptr = this.writeShort(packetBuff, ptr, option.getLength());

                if (option.getLength() > 0) {
                    System.arraycopy(option.getValue(), 0, packetBuff, ptr, option.getLength());
                    ptr += option.getLength();
                }
            }

            short chunkLength = (short) Math.min(MAX_PACKET_LENGTH - ptr, bytesLeft);

            this.writeShort(packetBuff, chunkLengthPtr, chunkLength);

            System.arraycopy(request.getPayload(), chunkOffset, packetBuff, ptr, chunkLength);
            ptr += chunkLength;

            chunkOffset += chunkLength;
            bytesLeft -= chunkLength;

            packets.add(new UdpPacket(packetBuff, ptr));
        }

        this.logger.info(String.format("Converted request into %d packets", packets.size()));

        return packets;
    }

    private int writeShort(byte[] buff, int offset, short val) {
        int intVal = Short.toUnsignedInt(val);
        buff[offset++] = (byte) ((intVal & 0xff00) >>> 8);
        buff[offset++] = (byte) (intVal & 0xff);

        return offset;
    }

    private int writeInt(byte[] buff, int offset, int val) {
        long longVal = Integer.toUnsignedLong(val);
        buff[offset++] = (byte) ((longVal & 0xff000000) >>> 24);
        buff[offset++] = (byte) ((longVal & 0xff0000) >>> 16);
        buff[offset++] = (byte) ((longVal & 0xff00) >>> 8);
        buff[offset++] = (byte) (longVal & 0xff);

        return offset;
    }

    private int writeLong(byte[] buff, int offset, long val) {
        buff[offset++] = (byte) ((val & 0xff00000000000000L) >>> 56);
        buff[offset++] = (byte) ((val & 0xff000000000000L) >>> 48);
        buff[offset++] = (byte) ((val & 0xff0000000000L) >>> 40);
        buff[offset++] = (byte) ((val & 0xff00000000L) >>> 36);
        buff[offset++] = (byte) ((val & 0xff000000) >>> 24);
        buff[offset++] = (byte) ((val & 0xff0000) >>> 16);
        buff[offset++] = (byte) ((val & 0xff00) >>> 8);
        buff[offset++] = (byte) (val & 0xff);

        return offset;
    }

    private Random getRandomInstance() throws Exception {
        if (System.getenv("FF_WEAK_RANDOM") != null) {
            this.logger.warning("Using weak RNG");
            return new Random(System.currentTimeMillis());
        } else {
            return SecureRandom.getInstanceStrong();
        }
    }
}
