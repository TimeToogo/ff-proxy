# FF Proxy - Java Client

FF includes a client library for use in Java projects.

## Installation

The client library can be installed from BinTray JCenter.

See [here](https://bintray.com/timetoogo/ffclient/ffclient) for instructions.

## Usage

The `FfClient` class provides the ability to send a `HttpRequest` to an FF proxy:

```java
package com.example.java;

import java.net.InetAddress;
import java.net.URI;
import java.net.http.HttpRequest;
import com.timetoogo.ffclient.FfClient;
import com.timetoogo.ffclient.FfConfig;


public class App {
    public static void main(String[] args) throws Exception {
        var client = new FfClient(
            FfConfig
                .builder()
                .ipAddress(InetAddress.getLoopbackAddress())
                .port(8080)
                .preSharedKey("mykey")
                .build()
        );

        var request = HttpRequest.newBuilder()
                .GET()
                .uri(URI.create("https://www.google.com"))
                .build();

        client.sendRequest(request);
    }
}
```
