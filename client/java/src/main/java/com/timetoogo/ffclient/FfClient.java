
package com.timetoogo.ffclient;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse.BodySubscriber;
import java.net.http.HttpResponse.BodySubscribers;
import java.util.List;
import java.util.Map.Entry;
import java.util.concurrent.Flow;
import java.util.logging.Logger;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.security.SecureRandom;

public class FfClient {
    private final Logger logger = Logger.getLogger(FfClient.class.getName());
    private final Charset UTF8 = Charset.forName("UTF-8");
    private FfConfig config;

    public FfClient(FfConfig config) {
        this.config = config;
    }

    public void sendRequest(HttpRequest request) {

    }

    static final class StringSubscriber implements Flow.Subscriber<ByteBuffer> {
        final BodySubscriber<byte[]> wrapped;

        StringSubscriber(BodySubscriber<byte[]> wrapped) {
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

    byte[] serializeHttpRequest(HttpRequest request) throws IOException {
        this.logger.info("Serializing HTTP request");
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();

        buffer.write(request.method().getBytes(UTF8));
        buffer.write(" ".getBytes(UTF8));
        buffer.write((request.uri().getPath().isEmpty() ? "/" : request.uri().getPath()).getBytes(UTF8));

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
            var stringSubscriber = new StringSubscriber(bodySubscriber);
            request.bodyPublisher().get().subscribe(stringSubscriber);
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

        var key = this.config.getPreSharedKey().getBytes(UTF8);
        var paddedKey = new byte[32];
        System.arraycopy(key, 0, paddedKey, 0, key.length);
        var keySpec = new SecretKeySpec(paddedKey, "AES");

        var iv = new byte[12];
        SecureRandom.getInstanceStrong().nextBytes(iv);

        var tagLength = 16;
        var gcmSpec = new GCMParameterSpec(tagLength * 8, iv);

        cipher.init(Cipher.ENCRYPT_MODE, keySpec, gcmSpec);

        byte[] cipherTextWithTag = cipher.doFinal(httpMessage);

        byte[] cipherText = new byte[cipherTextWithTag.length - tagLength];
        byte[] tag = new byte[tagLength];
        System.arraycopy(cipherTextWithTag, 0, cipherText, 0, cipherText.length);
        System.arraycopy(cipherTextWithTag, cipherText.length, tag, 0, tag.length);

        this.logger.info(String.format("Encrypted message into %d bytes", cipherText.length));

        request.getOptions().add(FfRequestOption.builder().type(FfRequestOption.Type.ENCRYPTION_MODE).length(1)
                .value(new byte[] { FfRequest.EncryptionMode.AES_256_GCM.getValue() }).build());

        request.getOptions().add(
                FfRequestOption.builder().type(FfRequestOption.Type.ENCRYPTION_IV).length(iv.length).value(iv).build());

        request.getOptions().add(FfRequestOption.builder().type(FfRequestOption.Type.ENCRYPTION_TAG).length(tag.length)
                .value(tag).build());

        return cipherText;
    }
}
