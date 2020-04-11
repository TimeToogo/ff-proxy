# FF Proxy

FF is a proxy server which enables you to _fire and forget_ HTTP requests.
That is, sending a HTTP request to a remote server, without waiting for a response or even the network latency required to establish a connection to that server.

Additionally, FF provides the ability to protect sensitive payloads by encrypting the data in transit between both the client and upstream servers.

**Disclaimer:**
This project was merely a learning exercise as well as my first sizeable project delving into C or even systems programming as a whole. If you somehow manage to find a use-case, please do **not** use this in production. If you need a protocol with reduced reliability and minimal overhead please look into [CoAP](https://en.wikipedia.org/wiki/Constrained_Application_Protocol).

## How it works

In summary, FF proxy server listens for HTTP requests over UDP and forwards them to their destination server over TCP for processing.

For the readers not familiar with the protocols discussed here, read on.
HTTP requests and responses are [almost](https://en.wikipedia.org/wiki/HTTP/3) always transmitted over a TCP connection. The TCP protocol ensures that data is exchanged accurately and reliably. In doing so, much of the communication between a client and server is for the purpose of verifying the has been data exchanged correctly rather than performing the data exchange itself.

To establish a TCP connection, a "handshake" must be completed between the client and the server using an exchange of SYN and ACK packets.

![TCP Handshake](https://www.lucidchart.com/publicSegments/view/ed4a8546-97b4-4d09-8c7d-6da5130648b1/image.png)

In ELI5 terms:

- the client says "hello, here is a random number 123456" to the server
- the server responds with a "hi there, i received your number 123456, my random number is 654321"
- finally the client says "thanks for your number, here is the data I want to send you ...".

The so-called random numbers are known as _sequence numbers_ and correspond to the current offset of the data sent or received over the connection, relative to the initial sequence number.

However I digress, the TCP handshake process involves transferring, at minimum, two packets between the client and remote server before data is able to be exchanged across the connection, hence the client must wait for at least one network round trip before initiating a HTTP request.

On the other hand, we have the UDP protocol. UDP is very simple comparatively and does not provide the reliability guarantees of TCP. Essentially UDP supports:

- sending independent packets of data
- receiving independent packets of data
- broadcasting packets to multiple hosts (cool but not relevant here)

![UDP Communication](https://www.lucidchart.com/publicSegments/view/05fbc518-1dba-4df5-8a45-70affb1c106f/image.png)

UDP lets us avoid the overhead of the network by removing any need for a handshake process before data can be exchanged. However HTTP web servers typically do not listen for traffic on UDP ports hence is not a viable solution on it's own.

FF Proxy makes it possible to send HTTP requests over UDP by acting as the middle man. Listening for HTTP requests over UDP and forwarding the requests to the destination web server over TCP.

![FF Proxy](https://www.lucidchart.com/publicSegments/view/1a17a71b-13fd-467d-8380-ccc6d0622514/image.png)

Hence FF proxy allows clients to reduce HTTP request latency to near zero at the cost not receiving the HTTP response from the remote server or any guarantees that the request was even received.

### FF protocol

FF supports forwarding a raw HTTP request message encapsulated within a single UDP packet. However UDP packet sizes are often restricted by a [path MTU](https://en.wikipedia.org/wiki/Path_MTU_Discovery), often less than 1500 bytes per packet.

To support forwarding larger HTTP requests, FF implements it's own protocol layer on top of UDP.
This protocol supports the fragmentation of HTTP requests into a stream of UDP packets. These packets are then reassembled as they are received by an FF proxy and consequently, the HTTP request is forwarded over TCP once reassembly completes.
The structure of an FF packet is divided into a fixed length header, followed by multiple request options and finally followed by the payload.

![FF Packet structure](https://www.lucidchart.com/publicSegments/view/7fd9c439-c776-4f96-bca9-d99f1a80eef9/image.png)

### Encryption and HTTPS

FF supports the protection of sensitive payloads in transit by performing encryption between the client and the proxy in combination with initiating a HTTPS request to the upstream server. Since the client and FF proxy do not perform bidirectional communication, no key negotiation can take place. Hence FF implements symmetric encryption (AES-256-GCM) using a pre-shared key between that is configured on both the client and the proxy.

## Usage

### Proxy

The recommended installation method is via Docker:

```bash
# This will expose an FF proxy on UDP port 1234 on the host and port 100 in the container
docker run --rm -it \
    -p 1234:100/udp \
    timetoogo/ff \
    --port 100 \
    -vvv
```

Or it can be [installed locally from the source](/docs/installing-from-source.md).

#### Arguments

| Argument                    | Required | Description                                                                                         |
| --------------------------- | -------- | --------------------------------------------------------------------------------------------------- |
| `--port <port>`             | Yes      | The UDP port to listen for incoming requests                                                        |
| `--ip-address <ip>`         | No       | The IP address for which to accept incoming packets, defaulting to IPv4 wildcard address: _0.0.0.0_ |
| `--ipv6-v6only`             | No       | When listening on IPv6 don't accept IPv4 connections                                                |
| `--pre-shared-key <key>`    | No       | The pre-shared key used to decrypt incoming requests                                                |
| `--pbkdf2-iterations <num>` | No       | The number of iterations used to derive the encryption key using PBKDF2 (default: 1000)             |
| `-v`, `-vv`, `-vvv`         | No       | Enable verbose logging                                                                              |

#### Testing

The most primitive interaction with FF proxy can be initiated using `netcat`:

```bash
# Send an unencrypted HTTP GET request to google via a local FF proxy on port 1234
echo -e "GET / HTTP/1.1\nHost: www.google.com\n\n" | nc -uw0 127.0.0.1 1234
```

### Clients

This project also includes client libraries which can used to initiate requests to an FF proxy.
The following languages have client libraries available:

- [C (cli)](./client/c/README.md)
- [Node.js](./client/node/README.md)
- [.NET Core](./client/dotnet/README.md)
- [Python](./client/python/README.md)
- [Java](./client/java/README.md)
