# FF Proxy - C Client

FF includes a client written in C. This is designed to be used via the CLI for testing or debugging purposes.

## Usage

Recommended usage is via Docker:

```bash
# Send HTTPS GET request to google
# (replace IP address and port with that of FF proxy)
echo "GET / HTTP/1.1\nHost: www.google.com\n\n" | docker run --rm -i \
    --entrypoint=ff_client \
    timetoogo/ff \
    --ip-address 123.123.123.123 \
    --port 1234 \
    --https \
    -vvv
```

Or it can be [installed locally from the source](/docs/installing-from-source.md).

#### Arguments

| Argument                    | Required | Description                                          |
| --------------------------- | -------- | ---------------------------------------------------- |
| `--ip-address <ip>`         | Yes      | The IP address of the FF proxy                       |
| `--port <port>`             | Yes      | The listening port of the FF proxy                   |
| `--pre-shared-key <key>`    | No       | The pre-shared key used to encrypt outgoing requests |
| `--pbkdf2-iterations <num>` | No       | The number of PBKDF2 iterations (default: 1000)      |
| `--https`                   | No       | Tell FF proxy to forwards the request over HTTPS     |
| `-v`, `-vv`, `-vvv`         | No       | Enable verbose logging                               |
