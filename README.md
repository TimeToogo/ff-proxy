# FF proxy

FF is a proxy server which enables you to _fire and forget_ HTTP requests.
That is, sending a HTTP request to a remote server, without waiting for a response or even the network latency required to establish a connection to that server. 

Additionally, FF provides the ability to protect sensitive payloads by encrypting the data in transit between both the client and upstream servers.

**Disclaimer:**
This project was merely a learning exercise as well as my first sizeable project delving into C or even systems programming as a whole. Please do **not** use this in production.

## How it works

In summary, FF proxy server listens for HTTP requests over UDP and forwards them to the destination servers over TCP for processing.

For the readers not familiar with the protocols discussed here, read on.
HTTP requests and responses are [almost](https://en.wikipedia.org/wiki/HTTP/3) always transmitted over a TCP connection. The TCP protocol provides ensures that data is exchanged accurately and reliably. In doing so, much of the communication between a client and server is for the purpose of verifying the has been data exchanged correctly rather than performing the data exchange itself.

To establish a TCP connection, a "handshake" must be completed between the client and the server using an exchange of SYN and ACK packets.

![TCP Handshake](https://www.lucidchart.com/publicSegments/view/ed4a8546-97b4-4d09-8c7d-6da5130648b1/image.png)

In ELI5 terms:

- the client says "hello, here is a random number 123456" to the server
- the server responds with a "hi there, i received your number 123456, my random number is 654321"
- finally the client says "thanks for your number, here is the data I want to send you ...".

The so-called random numbers are known as _sequence numbers_ and correspond to the current offset of the data sent or received over the connection.

However I digress, the TCP handshake process involves sending, at minimum, two packets between the client and remote server before a HTTP request (or any payload for that matter) is able to be exchanged, hence the client must wait for at least one network round trip before sending the HTTP request payload.

On the other hand, we have the UDP protocol. UDP is very simple comparatively and does not provide the guarantees that TCP does. Essentially UDP allows
 - a client to send independent packets of data
 - receive independent packet of data
 - broadcast packets to multiple hosts (cool but not relevant here)

![UDP Communication](https://www.lucidchart.com/publicSegments/view/05fbc518-1dba-4df5-8a45-70affb1c106f/image.png)

UDP lets us avoid the overhead of the network by removing any need for a handshake process before data can be exchanged. However HTTP web servers typically do not listen for traffic on UDP ports hence is not a viable solution on it's own.

FF Proxy makes it possible to send HTTP requests over UDP by acting as the middle man. Listening for HTTP requests over UDP and forwarding the the requests to the destination web server over TCP.

![FF Proxy](https://www.lucidchart.com/publicSegments/view/1a17a71b-13fd-467d-8380-ccc6d0622514/image.png)

Hence FF proxy allows clients to reduce HTTP request latency to near zero at the cost not receiving the HTTP response from the remote server.

### Large requests

TODO

### Encryption and HTTPS

TODO

## Usage

### Proxy

The recommended installation method is via Docker:

```
docker run --rm -it \
    -p 1234:100/udp \
    timetoogo/ff \
    --port 100 \
    --ip-address 0.0.0.0 \
    -vvv
```

TODO Sdks
