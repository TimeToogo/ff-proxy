
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
import java.nio.ByteBuffer;
import java.nio.charset.Charset;

public class FfClient {
    private final Logger logger = Logger.getLogger(FfClient.class.getName());
    private final Charset UTF8 = Charset.forName("UTF-8");

    public void SendRequest(HttpRequest request) {

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
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();

        buffer.write(request.method().getBytes(UTF8));
        buffer.write(" ".getBytes(UTF8));
        buffer.write(request.uri().getPath().getBytes(UTF8));

        if (request.uri().getQuery() != null && !request.uri().getQuery().isEmpty()) {
            buffer.write("?".getBytes(UTF8));
            buffer.write(request.uri().getQuery().getBytes(UTF8));
        }

        buffer.write(" HTTP/1.1\n".getBytes(UTF8));

        for (Entry<String, List<String>> header : request.headers().map().entrySet()) {
            for (String value : header.getValue()) {
                buffer.write(header.getKey().getBytes(UTF8));
                buffer.write(": ".getBytes(UTF8));
                buffer.write(value.getBytes(UTF8));
                buffer.write("\n".getBytes(UTF8));
            }
        }

        buffer.write("\n\n".getBytes(UTF8));

        if (request.bodyPublisher().isPresent()) {
            var bodySubscriber = BodySubscribers.ofByteArray();
            var stringSubscriber = new StringSubscriber(bodySubscriber);
            request.bodyPublisher().get().subscribe(stringSubscriber);
            byte[] body = bodySubscriber.getBody().toCompletableFuture().join();

            buffer.write(body);
        }

        return buffer.toByteArray();
    }

}
