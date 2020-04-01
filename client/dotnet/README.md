# FF Proxy - .NET Core Client

FF includes a client written in C# .NET Core v3.

## Installation

Install the latest prerelease version of FfClient from NuGet:

```
dotnet add package FfClient --version 1.0.0-prerelease-20200321140517
```

See latest versions [here](https://www.nuget.org/packages/FfClient/)

## Usage

### Using FfClient

The `FfClient` class provides the ability to send a `HttpRequestMessage` to an FF proxy:

```csharp
using System;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;
using FfClient;

namespace Example
{
    class Program
    {
        public async Task Run()
        {
            var config = new FfConfig()
            {
                IPAddress = IPAddress.Loopback,
                Port = 8080,
            };

            var client = new FfClient.FfClient(config);

            await client.SendRequest(new HttpRequestMessage()
            {
                Method = HttpMethod.Get,
                RequestUri = new Uri("https://google.com/"),
            });
        }
    }
}
```

### Using FfHttpRequestHandler

If you have existing code that relies on the built-in `HttpClient`, the SDK provides a custom request handler to send requests to an FF proxy.

```csharp
using System;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;
using FfClient;

namespace Example
{
    class Program
    {
        public async Task Run()
        {
            var config = new FfConfig()
            {
                IPAddress = IPAddress.Loopback,
                Port = 8080,
            };

            var handler = new FfHttpRequestHandler(
                config,
                mockResponse: new HttpResponseMessage()
                {
                    StatusCode = HttpStatusCode.Created
                }
            );

            HttpResponseMessage result;

            using (var httpClient = new HttpClient(handler))
            {
                result = await httpClient.GetAsync("https://www.google.com");
            };
        }
    }
}
```
