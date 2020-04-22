using System;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using FfClient;
using Xunit;

namespace FfClient.Tests
{
    public class FfHttpRequestHandlerTest
    {
        private FfConfig MockConfig()
        {
            return new FfConfig()
            {
                IPAddress = IPAddress.Loopback,
                Port = 8080,
                PreSharedKey = "test"
            };
        }

        [Fact]
        public async Task TestSendHttpRequestWithNullMockDefaultTo200()
        {
            var handler = new FfHttpRequestHandler(this.MockConfig());

            HttpResponseMessage result;

            using (var httpClient = new HttpClient(handler))
            {
                result = await httpClient.GetAsync("http://google.com");
            }

            Assert.Equal(200, (int)result.StatusCode);
            Assert.Null(result.Content);
        }

        [Fact]
        public async Task TestSendHttpRequestWithCustomMock()
        {
            var handler = new FfHttpRequestHandler(
                this.MockConfig(),
                mockResponse: new HttpResponseMessage()
                {
                    StatusCode = HttpStatusCode.Created
                }
            );

            HttpResponseMessage result;

            using (var httpClient = new HttpClient(handler))
            {
                result = await httpClient.PostAsync("http://google.com", new StringContent("test"));
            }

            Assert.Equal(HttpStatusCode.Created, result.StatusCode);
            Assert.Null(result.Content);
        }
    }
}
