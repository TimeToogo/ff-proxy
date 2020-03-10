using System;
using System.Net.Http;
using System.Threading.Tasks;
using System.Threading;

namespace FfClient
{
    public class FfHttpRequestHandler : HttpMessageHandler
    {
        private readonly FfClient ffClient;
        private readonly HttpResponseMessage mockResponse;

        public FfHttpRequestHandler(FfConfig config, HttpResponseMessage mockResponse = null)
        {
            this.ffClient = new FfClient(config);
            this.mockResponse = mockResponse ?? new HttpResponseMessage();
        }

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            await this.ffClient.SendRequest(request);

            return this.mockResponse; 
        }
    }
}
