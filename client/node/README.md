# FF Proxy - Node Client

FF includes a client library for Node.js written in Typescript.

## Installation

Install the latest prerelease version of `@timetoogo/ff-client` from NPM:

```
npm install --save @timetoogo/ff-client
```

See latest versions [here](https://www.npmjs.com/package/@timetoogo/ff-client)

## Usage

### Using FfClient

The `FfClient` class provides the ability to send a HTTP request to an FF proxy:

```js
const { FfClient } = require("@timetoogo/ff-client");

const client = new FfClient({
  ipAddress: "127.0.0.1",
  port: 8080,
  preSharedKey: 'mykey'
});

client.sendRequest({
  request: "GET / HTTP/1.1\nHost: www.google.com\n\n",
  https: true
});
```

### Using FfClientAgent

If you have existing code that relies on the built-in `http` or `https` modules, the SDK provides a custom request agent to send requests to an FF proxy.

```js
const http = require("http");
const { FfClientAgent } = require("@timetoogo/ff-client");

const agent = new FfClientAgent({
  ipAddress: "127.0.0.1",
  port: 8080,
  preSharedKey: 'mykey',
  mockResponse: 200
});

const request = http.request({
  method: "GET",
  path: "/",
  host: "www.google.com",
  agent
});

request.end();
```
